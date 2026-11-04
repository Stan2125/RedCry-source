// ---------------------------------------------------------------------------
//  RedMAX - vrstva hledání
//
//  Přepis z RedMAX_408_f.pas (Delphi 5 / VCL, cp1250) do prostého C++17.
//  Čísla řádků v komentářích odkazují do originálu.
//
//  Vrstva vznikla v Kroku 2 po částech (2/1 hodnocení, 2/2 alfa-beta a kořen,
//  2/3 hash tabulka, 2/4 null move) a Krok 2.5 ji doladil. Ukazatel "<- tento
//  commit", který tu stával, se záměrně nenahrazuje jiným: soubor přežije
//  desítky commitů a takový ukazatel stárne ze své podstaty. Co se kdy udělalo,
//  ví git log a docs/POZNAMKY-REDMAX.md.
//
//  Odstraněno: veškerá VCL. Hodnocení v originále četlo CheckBox1.Checked
//              (ř. 1277) - tady je z toho Nastaveni::pozicni_kriteria.
//              Globální strana_na_tahu_v_sachu (ř. 472), které Oceneni_pozice
//              nastavovalo jako vedlejší efekt a AlfaBeta si ho hned četlo
//              (ř. 1646), je členem StavHledani.
//
//  ZÁSADA: hodnocení je čistou funkcí pozice. Táž pozice musí dostat totéž
//  skóre bez ohledu na to, kudy se k ní hledání dostalo - jinak hash tabulka
//  vrací cizí čísla. Originál to porušoval na třech místech (rozbor 3.3);
//  všechna tři jsou tady opravená a hlídá je test 5 v tools/redmax.
// ---------------------------------------------------------------------------

#pragma once

#include "red_max_core.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

namespace redmax {

// --- konstanty hledání a hodnocení (ř. 23-49, 87-116) -----------------------

constexpr int hodnota_matu          = 10000;

constexpr int material_pro_koncovku = 4 * hodnota_Veze + 2 * hodnota_Strelce;

// Bonusy pořadí tahů (bonus_hlavni_varianty a spol.) jsou v red_max_core.h -
// přiděluje je uloz_tah, viz PoradiTahu.

// Rezerva líného hodnocení (ř. 49). Předpoklad "poziční složka nepřekročí
// hodnotu střelce" není ověřený - viz rozbor 3.4. Skutečné maximum měří
// StavHledani::max_abs_pozicni_hodnota a vypisuje ho hledani_test.
constexpr int max_pozicniho_oceneni = hodnota_Strelce;

// Sloupce a řady v souřadnicích mailboxu 10x12 (ř. 87-103).
// Sloupec je pole mod 10, řada je pole div 10 - proto Rada_1 = 2, ne 1.
constexpr int Sloupec_A = 1, Sloupec_B = 2, Sloupec_C = 3, Sloupec_D = 4;
constexpr int Sloupec_E = 5, Sloupec_F = 6, Sloupec_G = 7, Sloupec_H = 8;
constexpr int Rada_1 = 2, Rada_3 = 4, Rada_8 = 9;

// --- typ_obou_barev (ř. 17-20) ----------------------------------------------
struct typ_obou_barev {
    int bily  = 0;
    int cerny = 0;
};

// --- z_na_typ a KillerTyp (ř. 411-419) --------------------------------------
struct z_na_typ {
    int z_pole  = 0;
    int na_pole = 0;
};

struct KillerTyp {
    z_na_typ Killer1;
    z_na_typ Killer2;
};

// --- hash tabulka (ř. 437-445) ----------------------------------------------
//
// Originál měl v záznamu osm intů a dva nezávislé 32bitové klíče: hash_value
// na ověření a hash_index na adresu. Že jsou 32bitové, je ale omyl - záměr byl
// 64 bitů, jenže `integer` je v Delphi 5 32bitový a `shl 32` je na x86 posun
// o nulu (rozbor 3.6). S poctivým 64bitovým klíčem stačí jeden: spodní bity
// dávají adresu, celý klíč se uloží a ověřuje.
//
// Záznam je 16 bajtů. `hodnota` se vejde do int16: největší možná materiálová
// bilance je 11 480 (devět dam a všechny těžké figury proti holému králi)
// plus poziční složka pod 350, matová hodnota je 10 000.
enum class DruhMeze : int8_t {
    HorniMez = -1,   // fail-low, skutečná hodnota není větší
    Presne   =  0,
    DolniMez =  1    // fail-high, skutečná hodnota není menší
};

struct HashZaznam {
    uint64_t klic     = 0;
    int16_t  hodnota  = 0;
    int8_t   horizont = 0;    // v hledání klidu i záporný
    DruhMeze druh     = DruhMeze::Presne;
    uint8_t  z_pole   = 0;    // 0 = záznam nenese tah
    uint8_t  na_pole  = 0;
    uint8_t  obsazeno = 0;

    // Stáří záznamu (Krok 4/1). Dřív tu byla jen výplň do šestnácti bajtů,
    // takže je to zadarmo. Nese generaci, ve které se záznam zapsal; podle
    // ní politika nahrazování pozná záznam z pozice, která už na desce není.
    uint8_t  stari    = 0;
};

// --- trvalý stav motoru (Krok 4/1) ------------------------------------------
//
// Co přežije mezi tahy. Vlastní to VOLAJÍCÍ, tedy vrstva UCI, ne jedno
// hledání.
//
// Do Kroku 4 tohle všechno leželo v StavHledani, které si `hledej` vyrábělo
// samo při každém volání - hash tabulka se tedy po každém tahu zahodila.
// Hledání dalšího tahu přitom sdílí s předchozím velkou část stromu a právě
// v tom je transpoziční tabulka nejužitečnější. (Zadání Kroku 2 říkalo
// "alokuj až při založení hledání", což šlo číst obojím způsobem.)
struct TrvalyStav {
    // Zobristova čísla (ř. 2245-2256). Vygenerují se JEDNOU za život motoru.
    // Dokud v tabulce leží záznamy, přegenerovat se nesmějí: každý uložený
    // klíč by pak ukazoval jinam. Se stejným semínkem by to bylo neškodné,
    // ale patří to k jednorázové části a tady to tak je.
    //
    // Index figury je `sachovnice[pole] + 6`, tedy 0..12. Okrajová pole se
    // do klíče nepočítají; v originále se počítají, jenže indexem 106, což
    // je dávno za koncem pole - viz ROZBOR-REDMAX.md, § 3.2.
    uint64_t hash_pole[velikost_sachovnice_10x12 + 1][13]{};
    uint64_t hash_stav[14]{};   // 1..8 sloupec e.p., 9 barva na tahu, 10..13 rošády

    std::vector<HashZaznam> tabulka;
    uint64_t maska = 0;          // počet záznamů je mocnina dvojky
    int      velikost_mb = 0;    // podle čeho je tabulka alokovaná

    // Generace, tedy stáří. Zvyšuje se při každém `go`.
    uint8_t generace = 0;

    bool zalozeno = false;
};

// --- průběžné hlášení (v originále není) -------------------------------------
//
// Vrstva UCI (Krok 3) potřebuje po každé DOKONČENÉ hloubce vědět, co hledání
// zatím našlo: jednak aby to vydala jako `info depth ...`, jednak aby si UI
// vlákno mohlo přečíst průběh, aniž by čekalo na konec hledání.
//
// Volá se na tom vlákně, které zavolalo hledej(), a jen mezi iteracemi
// prohlubování - uvnitř AlfaBeta nikdy. Hledání na ni nečeká a nic si z ní
// nebere; kdo si chce hodnotu odložit, ať to udělá sám.
struct PrubeznyVysledek {
    int      hloubka = 0;
    int      hodnota = 0;
    long     uzlu = 0;
    typ_tahu nejlepsi_tah{};
    bool     tah_nalezen = false;
};

using HlaseniFn = void (*)(void* kontext, const PrubeznyVysledek&);

// --- režim bonusu za hlavní variantu (bod D1 Kroku 2.5) ----------------------
//
// `uloz_tah` přiděluje bonus 500 tahu z `tabulka_hlavni_varianty[0][hloubka]`,
// tedy z KOŘENOVÉ varianty pro tuhle hloubku (ř. 746-751). Na cestě po hlavní
// variantě je to ten správný tah; ve všech ostatních uzlech je to cizí tah,
// který s pozicí nesouvisí - a s pěti sty předběhne i braní dámy. Čím širší
// strom, tím víc uzlů mimo hlavní variantu, a tím to víc škodí.
//
// Rozhoduje se podle naměřeného počtu uzlů, ne podle úvahy; čísla jsou
// v POZNAMKY-REDMAX.md, oddíl 2.5/D.
enum class RezimPV {
    Vzdy,                  // jak to má originál
    Nikdy,                 // bonus se nepřiděluje vůbec, killery zůstávají
    JenNaHlavniVariante    // jen v uzlech, do kterých se došlo samými prvními tahy
};

// --- nastavení hledání ------------------------------------------------------
// Přepínače nejsou uživatelská výbava, ale nástroj ověřování: každý z nich
// musí jít vypnout a výsledek se buď nesmí změnit vůbec (hash, pořadí tahů),
// nebo se smí změnit jen v mezích heuristiky (null move).
struct Nastaveni {
    int  max_hloubka      = 6;
    int  cas_ms           = 0;     // 0 = neomezeno
    // Počet záznamů se z toho odvodí jako mocnina dvojky. Čtyři megabajty jsou
    // 262 144 záznamů po 16 bajtech. Změřeno (bod D2 Kroku 2.5): mezi 4, 16
    // a 64 MB je na hloubce 7 rozdíl 2 %, tabulka je tedy při téhle hloubce
    // nasycená už při čtyřech - a míří se na telefon, tak ať zůstanou.
    int  velikost_hash_mb = 4;
    bool pouzij_hash      = true;
    bool pouzij_null_move = true;
    bool pouzij_killery   = true;
    bool pozicni_kriteria = true;  // CheckBox1.Checked (ř. 1277)

    // Bod D1 Kroku 2.5. `pouzij_killery = false` vypíná pořadí tahů celé
    // (killery i hlavní variantu); tenhle přepínač rozlišuje uvnitř zapnutého
    // pořadí, komu se bonus za hlavní variantu přidělí.
    //
    // ZMĚNĚNO PROTI ORIGINÁLU podle naměřeného počtu uzlů (tools/redmax/mereni):
    // na hloubce 7 stojí `Vzdy` 6 985 016 uzlů, `JenNaHlavniVariante`
    // 4 628 069, tedy o 51 % míň, a nalezený tah je ve všech deseti pozicích
    // týž. `Nikdy` vyjde s hashem nastejno, ale bez hashe hůř.
    RezimPV rezim_pv = RezimPV::JenNaHlavniVariante;

    // Bod D2 Kroku 2.5. Do hash tabulky se ukládá jen při
    // `horizont >= min_horizont_pro_hash`. Originál má na ř. 1886
    // `horizont > 2`, tedy tři; mělké uzly - kterých je nejvíc - se tak
    // neuloží nikdy. Hodnota pod -7 znamená "ukládat vždy", protože hlouběji
    // než na -7 se AlfaBeta nedostane (ř. 1667).
    //
    // ZMĚNĚNO PROTI ORIGINÁLU podle naměřeného počtu uzlů: se stropem 3 dá
    // hash na hloubce 7 jen 1,33x, s jedničkou 2,11x. "Vždy" už nepřidá nic
    // (2,08x) a zapíše 1,7x víc záznamů, takže se nebere. Jednička znamená
    // "všechny uzly vlastního hledání, žádný z hledání klidu".
    int min_horizont_pro_hash = 1;

    // Krok 4/1, jen kvůli měření. `false` znamená, že se tabulka na začátku
    // každého hledání vynuluje, tedy chování před Krokem 4 - jinak by se nedalo
    // změřit, co trvalost udělala. Pro hru se nevypíná.
    bool trvala_hash = true;

    // Krok 4/1, taky jen kvůli měření. `false` vrátí politiku nahrazování
    // na tu, kterou měl Krok 2: o stáří se nestará.
    bool stari_zaznamu = true;

    // Krok 4/2, zase jen kvůli měření. `false` vrátí detekci opakování na to,
    // co uměla do té doby: jediný bod čtyři půltahy zpátky a historie partie
    // vůbec. Pro hru se nevypíná - kvůli tomu končilo 45 partií z 98
    // opakováním.
    bool opakovani_pres_partii = true;

    // Krok 17/B: remíza podle pravidla 50 tahů v hledání. Zapnuto pro hru;
    // `false` jen kvůli měření a srovnávacímu zápasu, stejně jako
    // opakovani_pres_partii. Viz StavHledani::pultahy a AlfaBeta.
    bool pravidlo_50 = true;

    // Ukládat do hash tabulky i horní meze (fail-low). Originál je spočítá,
    // ale zahodí (ř. 1888), takže větev -1 při čtení je mrtvá - rozbor 3.5.
    // Bod 8 zadání Kroku 2: naportovat věrně, tedy neukládat, pak zkusmo
    // zapnout a rozhodnout podle naměřeného počtu uzlů.
    //
    // ZAPNUTO v Kroku 3/0. Se stropem `horizont > 2` to vycházelo 1,02x
    // a neukládalo se; po 2.5/D2, kdy se strop snížil na `horizont > 0`,
    // vychází 1,05x - lepší v osmi pozicích z deseti a bez jediné změny
    // nalezeného tahu nebo hodnoty. Čísla jsou v POZNAMKY-REDMAX.md.
    bool ukladej_fail_low = true;

    // Test 4: v každém uzlu spočítat klíč ještě jednou od nuly a porovnat
    // s přírůstkovým. Je to drahé, proto jen pod přepínačem - stejná metoda,
    // jakou si autor originálu psal ručně na ř. 2004.
    bool kontroluj_hash = false;

    // Hlášení po každé dokončené hloubce, viz PrubeznyVysledek. nullptr = nic.
    HlaseniFn hlaseni = nullptr;
    void*     hlaseni_kontext = nullptr;

    // Líné hodnocení (ř. 1267-1273). V originále se vypnout nedá a pro hru
    // se vypínat nemá - je to jen nástroj testů. Hodnota, kterou líné
    // hodnocení vrátí, totiž závisí na okně Alfa-Beta, a fail-soft ji pak
    // nese stromem nahoru. Vypnutím se hodnocení stane nezávislé na okně
    // a jde tak odlišit, co v rozdílu skóre dělá ono a co prodlužování
    // hloubky, které na okně závisí taky (ř. 1661).
    bool line_hodnoceni   = true;
};

// --- stav jednoho hledání ---------------------------------------------------
// V originále to byly souborové globály (ř. 459-508). Tady je vlastní volající
// a jádro o nich neví. Krok 2/2 sem přidá killery, hlavní variantu a počítadlo
// uzlů, Krok 2/3 hash tabulku.
struct StavHledani {
    Nastaveni nastaveni;

    // pracovní pole hodnocení (ř. 462-465)
    // Originál nuluje pole_kontrolovana_pesci jen na A1..H8 (ř. 1040-1043),
    // ačkoli pěšci zapisují i mimo ten rozsah (i +- 9, i +- 11, tedy 10..109).
    // Nepřečtou se, protože pěšec stojí vždy na řadě 2 až 7, ale nechávat
    // v poli smetí nemá důvod - tady se nuluje celé.
    typ_obou_barev pole_kontrolovana_pesci[velikost_sachovnice_10x12 + 1];
    typ_obou_barev pescu_ve_sloupci[Sloupec_H + 2];
    typ_obou_barev vezi_ve_sloupci[Sloupec_H + 2];

    // kolikrat_bez_pozicnich_kriterii / kolikrat_s_pozicnimi_kriterii
    // (ř. 488-489) - kolikrát se uplatnilo líné hodnocení
    long bez_pozicnich_kriterii = 0;
    long s_pozicnimi_kriterii   = 0;

    // strana_na_tahu_v_sachu (ř. 472). Oceneni_pozice ho nastaví, AlfaBeta
    // si ho přečte hned po návratu (ř. 1646).
    bool strana_na_tahu_v_sachu = false;

    // V originále není. Oceneni_pozice ho nastaví, když v pozici lze sebrat
    // soupeřova krále, tedy když byl předchozí tah nelegální. Originál to
    // poznával porovnáním skóre s hodnota_matu - hloubka (ř. 1665); tady je to
    // oddělené, aby detekce patu nezávisela na tom, jak se ukládají matové
    // hodnoty - to zadání Kroku 2 vysloveně žádá (oprava 4).
    bool lze_sebrat_krale = false;

    // Měření rezervy líného hodnocení, viz max_pozicniho_oceneni.
    // Jedno porovnání na hodnocenou pozici; v hlubokém hledání se to ztratí.
    int max_abs_pozicni_hodnota = 0;

    // --- vlastní hledání (ř. 467-490) ---------------------------------------

    // KillerTabulka (ř. 481), dva killer tahy na hloubku
    KillerTyp killery[max_hloubka_hledani + 1];

    // tabulka_hlavni_varianty (ř. 479). Trojúhelníková tabulka: řádek `hloubka`
    // drží variantu od téhle hloubky dál, ukončenou záznamem se z_pole = 0.
    // Pořadí tahů čte řádek 0, tedy variantu z kořene.
    z_na_typ hlavni_varianta[max_hloubka_hledani + 1][max_hloubka_hledani + 2];

    // max_rozsireni_hloubky (ř. 473). Kořen ho nastaví na horizont + 3.
    int max_rozsireni_hloubky = 0;

    // --- hash (ř. 504-508) --------------------------------------------------

    // Zobristova čísla a vlastní tabulka jsou v TrvalyStav, protože přežívají
    // mezi tahy. Hledání na ně sahá přes tenhle ukazatel; nullptr znamená
    // hledání bez hash tabulky (na to je ale i Nastaveni::pouzij_hash).
    TrvalyStav* trvaly = nullptr;

    // hash_value / hash_index (ř. 504-505), v portu jeden klíč na hloubku.
    // Tohle naopak patří JEDNOMU hledání - je to cesta od kořene dolů.
    uint64_t klic[max_hloubka_hledani + 2]{};

    // Půltahy bez braní a tahu pěšcem, po hloubkách - Krok 17/B. Pozice to
    // nevede: zahraj_tah počítadlo nemění a Pozice::pultahy_bez_zmeny je jen
    // hodnota z FEN kořene. pultahy[kořen] je právě ta, pak ho posouvá
    // urci_klic_z_predchozi (všechny tahy hledání jdou přes ni) a null move.
    // Stejně jako klic[] patří CESTĚ, ne pozici.
    int pultahy[max_hloubka_hledani + 2]{};

    // Klíče pozic ODEHRANÉ PARTIE, kvůli detekci opakování (Krok 4/2).
    // Vlastní je volající (vrstva UCI), tady je jen ukazatel; nullptr znamená
    // "historii neznám", což je stav testů a měření, která hledají v jedné
    // pozici bez partie kolem ní.
    //
    // Poslední prvek je klíč KOŘENOVÉ pozice, tedy totéž co klic[0] - vložil
    // se tam, když se zahrál poslední tah partie.
    const std::vector<uint64_t>* historie = nullptr;

    long hash_nalezeno = 0;   // kolikrát se ze záznamu dala rovnou vzít hodnota
    long hash_zapsano  = 0;

    // pocet_prozkoumanych_pozic (ř. 490). Počet VSTUPŮ do AlfaBeta, tedy
    // včetně hledání klidu - tahle definice se nesmí změnit, až se budou
    // motory porovnávat na žebříčku.
    long pocet_prozkoumanych_pozic = 0;

    // Přerušení zvenčí. Originál se řídil polohou myši (ř. 1827) a při
    // přerušení vracel kouzelné -9999, které se propagovalo stromem jako
    // skóre (ř. 1848). Tady se jen nastaví příznak, hledání se vrátí
    // a volající výsledek zahodí.
    const std::atomic<bool>* zastav = nullptr;
    bool doslo_k_preruseni = false;
    std::chrono::steady_clock::time_point konec_casu{};
    bool cas_omezen = false;

    // kolikrát se přírůstkový klíč rozešel s přepočtem od nuly (test 4)
    long hash_neshod = 0;
};

// --- výsledek hledání -------------------------------------------------------
struct Vysledek {
    typ_tahu nejlepsi_tah{};
    bool     tah_nalezen = false;   // false = mat nebo pat, není co táhnout
    int      hodnota = 0;
    int      dosazena_hloubka = 0;  // poslední DOKONČENÝ horizont
    long     uzlu = 0;
    std::vector<typ_tahu> hlavni_varianta;
    bool     preruseno = false;

    // Měření podle bodu 7 zadání. Hlavní varianta nese u druhého a dalšího
    // tahu jen výchozí a cílové pole - tabulka hlavní varianty v originále
    // víc neuchovává. První tah je celý záznam, takže i s proměnou.
    int max_abs_pozicni_hodnota = 0;
    long bez_pozicnich_kriterii = 0;
    long s_pozicnimi_kriterii   = 0;
    long hash_nalezeno = 0;
    long hash_zapsano  = 0;

    // Test 4: kolikrát se přírůstkový klíč rozešel s přepočtem od nuly.
    // Uplatní se jen při Nastaveni::kontroluj_hash.
    long hash_neshod = 0;
};

// --- rozhraní hledání -------------------------------------------------------

// prvni_tah_alfa_beta (ř. 1941-2133), bez GUI.
//
// Na rozdíl od originálu nalezený tah NEZAHRAJE (ř. 2117-2118) - pozici vrátí
// v tom stavu, v jakém ji dostal, a tah jen ohlásí. Na tom stojí adaptér
// v Kroku 3.
//
// Při přerušení se výsledek posledního nedokončeného horizontu zahodí a vrátí
// se poslední dokončený, s preruseno = true.
// Hledání s TRVALÝM stavem: hash tabulka a Zobristova čísla přežijí do
// dalšího volání. Tohle je varianta, kterou používá vrstva UCI.
// `historie` jsou klíče pozic odehrané partie, poslední z nich je klíč
// pozice, ve které se hledá. Slouží jen detekci opakování; nullptr znamená,
// že motor partii kolem pozice nezná.
Vysledek hledej(Pozice& p, const Nastaveni& n, std::atomic<bool>& zastav,
                TrvalyStav& trvaly,
                const std::vector<uint64_t>* historie = nullptr);

// Pohodlná varianta s vlastním dočasným trvalým stavem - tabulka tedy žije
// jen po dobu tohohle jednoho hledání, jako před Krokem 4. Používají ji
// měření a ty testy, kterým jde o hledání samo, ne o partii.
Vysledek hledej(Pozice& p, const Nastaveni& n, std::atomic<bool>& zastav);

// AlfaBeta (ř. 1628-1911). Vystaveno kvůli testům; hledej() si ho volá samo.
// `pozice_nelegalni` se nastaví, když v pozici lze sebrat soupeřova krále,
// tedy když byl předchozí tah nelegální. Originál to poznával podle skóre
// hodnota_matu - hloubka; tady je to oddělené, aby detekce patu nezávisela
// na tom, jak se ukládají matové hodnoty.
//
// `na_hlavni_variante` je pravda v uzlu, do kterého se došlo od kořene samými
// prvními tahy. V originále takový příznak není - přibyl kvůli
// RezimPV::JenNaHlavniVariante (bod D1 Kroku 2.5) a při ostatních režimech
// se nečte.
int AlfaBeta(Pozice& p, int Alfa, int Beta, int horizont, bool null_move,
             bool na_hlavni_variante, StavHledani& stav,
             bool* pozice_nelegalni = nullptr);

// dalsi_nejlepsi_tah (ř. 1610-1625). Lineární výběr maxima; vybraný tah se
// označí hodnotou -hodnota_matu, aby se nevybral podruhé. Vrací -1, když už
// žádný tah nezbývá.
int dalsi_nejlepsi_tah(Pozice& p);

// Založí Zobristova čísla pevným semínkem a alokuje hash tabulku. Je to
// IDEMPOTENTNÍ: druhé volání s touž velikostí neudělá nic, takže se obsah
// tabulky ani čísla neztratí. Jiná velikost znamená realokaci.
// hledej() si to volá samo; volající to může zavolat dopředu.
void zaloz_motor(TrvalyStav& trvaly, int velikost_mb);

// Vynuluje tabulku i generaci; Zobristova čísla nechá být. Volá se při
// ucinewgame - nová partie nemá dědit záznamy po předchozí.
void vymaz_hash(TrvalyStav& trvaly);

// Přealokuje tabulku na jinou velikost a vynuluje ji. Volá se při
// `setoption name Hash`. Maska se mění spolu s ní, takže po návratu už nikdo
// nesmí držet index spočítaný ze staré masky.
void zmen_velikost_hashe(TrvalyStav& trvaly, int velikost_mb);

// urci_hash_value_pro_uroven (ř. 2817-2861). Klíč pozice spočítaný od nuly.
// Proti přírůstkovému klíči ho porovnává test 4.
uint64_t urci_klic_od_nuly(const Pozice& p, const TrvalyStav& trvaly);

// --- rozhraní ---------------------------------------------------------------

// Oceneni_pozice (ř. 1033-1539). Kladné skóre je dobré pro stranu `strana`.
//
// `strana` je strana na tahu; originál ji na obou voláních (ř. 1645 a 2628)
// předává jako `barva`, takže `strana == p.barva` vždy platí. Parametr
// zůstává kvůli shodě s originálem.
//
// Alfa a Beta slouží jen línému hodnocení (ř. 1268): je-li materiál dost
// daleko za oknem, poziční složka se vůbec nepočítá. Kdo chce hodnotu úplnou,
// zavolá s plným oknem.
//
// Pozor: stojí-li král strany, která NENÍ na tahu, v šachu, je pozice
// nelegální (soupeř nechal krále napadeného) a funkce vrátí hodnota_matu
// mínus hloubku, tedy jedinou hodnotu, která na hloubce závisí. Volající,
// kterému jde o hodnotu pozice, si tenhle případ musí vyloučit sám -
// stejně jako to dělá AlfaBeta.
int oceneni_pozice(const Pozice& p, int Alfa, int Beta, int strana, StavHledani& stav);

}  // namespace redmax
