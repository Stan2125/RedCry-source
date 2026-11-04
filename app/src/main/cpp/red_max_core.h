// ---------------------------------------------------------------------------
//  RedMAX - jádro šachového enginu
//
//  Přepis z RedMAX_408_f.pas (Delphi 5 / VCL, cp1250) do prostého C++17.
//  Zdrojové názvy jsou zachovány česky a co nejblíže originálu, aby šlo
//  kterýkoli řádek dohledat v Pascalu.
//
//  Odstraněno: veškerá VCL (TForm1, TImage, Label, Memo, SpinEdit, RadioGroup),
//              kreslení, PGN, editace pozice, AutoPlay, hledání, hodnocení
//              pozice, hash tabulka, killery, hlavní varianta, null move.
//              Dále pole `pohyblivost` - plnilo se v nejteplejší funkci
//              (uloz_tah, ř. 738 a 742) a četlo se jen na ř. 1502-1503,
//              které jsou zakomentované. Viz POZNAMKY-REDMAX.md.
//  Zachováno:  reprezentace pozice, tabulky Offset / FigOffset / FigMaterial,
//              generátor tahů, provedení a vzetí tahu zpět, detekce napadení.
//
//  Indexy polí zůstávají přesně jako v Pascalu: mailbox 0..119, A1 = 21,
//  H8 = 98, okraj 100. Přečíslování na 0..63 by bylo nejrychlejší způsob,
//  jak si do portu zanést chybu.
//
//  Tahy jsou PSEUDOLEGÁLNÍ. Originál legalitu netestuje - pozná ji až podle
//  skóre v Oceneni_pozice (ř. 1251-1263), protože braní krále se negeneruje.
//  Volající tedy musí po zahraj_tah ověřit napadene_pole na králi té strany,
//  která právě táhla. Je to jiný přístup než u Light Redu, který má generátor
//  legálních tahů; obě jádra se záměrně neslučují.
// ---------------------------------------------------------------------------

#pragma once

#include <string>

namespace redmax {

// --- konstanty (Pascal ř. 22-204) -------------------------------------------

constexpr int velikost_sachovnice_10x12   = 119;
constexpr int poc_smeru_tahu_vsech_kamenu = 15;
constexpr int velikost_zasobniku_tahu     = 2000;
constexpr int poc_typu_figur              = 6;
constexpr int max_hloubka_hledani         = 50;

constexpr int pravda    = 1;
constexpr int nepravda  = 0;

constexpr int hodnota_Pesce   =  100;
constexpr int hodnota_Veze    =  550;
constexpr int hodnota_Strelce =  350;
constexpr int hodnota_Jezdce  =  340;
constexpr int hodnota_Damy    = 1000;
constexpr int hodnota_Krale   =    0;   // každá strana má jednoho, proto nula

// Figury: barva * typ dá figuru správné barvy. Na tom stojí proměna
// (barva * premenena_figura) i rošáda (barva * b_Vez) - zachovat.
constexpr int c_Kral    = -6;
constexpr int c_Dama    = -5;
constexpr int c_Jezdec  = -4;
constexpr int c_Strelec = -3;
constexpr int c_Vez     = -2;
constexpr int c_Pesec   = -1;
constexpr int prazdne_pole = 0;
constexpr int b_Pesec   =  1;
constexpr int b_Vez     =  2;
constexpr int b_Strelec =  3;
constexpr int b_Jezdec  =  4;
constexpr int b_Dama    =  5;
constexpr int b_Kral    =  6;
constexpr int hranice_sachovnice = 100;

constexpr int bily  =  1;
constexpr int cerny = -1;

constexpr int zadna_rosada = 0;
constexpr int mala_rosada  = 1;
constexpr int velka_rosada = 2;

constexpr int legalni   = 1;
constexpr int nelegalni = 0;

constexpr int A1 = 21, B1 = 22, C1 = 23, D1 = 24, E1 = 25, F1 = 26, G1 = 27, H1 = 28;
constexpr int                             D2 = 34, E2 = 35,                   H2 = 38;
constexpr int A3 = 41,                    D3 = 44, E3 = 45;
constexpr int A6 = 71,                    D6 = 74, E6 = 75,                   H6 = 78;
constexpr int A7 = 81,                    D7 = 84, E7 = 85,                   H7 = 88;
constexpr int A8 = 91, B8 = 92, C8 = 93, D8 = 94, E8 = 95, F8 = 96, G8 = 97, H8 = 98;

constexpr int material_v_zakl_postaveni =
    4 * (hodnota_Veze + hodnota_Strelce + hodnota_Jezdce) + (2 * hodnota_Damy);

// Bonusy pořadí tahů (ř. 27-30). Přidělují se v uloz_tah a uloz_utocny_tah,
// tedy tady v jádře, ačkoli to, KTERÝ tah je má dostat, ví až vrstva hledání -
// viz PoradiTahu níž.
constexpr int bonus_hlavni_varianty = 500;
constexpr int bonus_1_killer_tahu   = 250;
constexpr int bonus_2_killer_tahu   = 150;
constexpr int bonus_hash_nej_tahu   = 1500;

// --- FigOffsetTyp (Pascal ř. 11-15) -----------------------------------------
struct FigOffsetTyp {
    int  zacatek;
    int  konec;
    bool delka_kroku;      // v Pascalu pravda/nepravda jako integer
};

extern const int          Offset[poc_smeru_tahu_vsech_kamenu + 1];
extern const FigOffsetTyp FigOffset[poc_typu_figur + 1];
extern const int          FigMaterial[poc_typu_figur + 1];
extern const int          zakladni_postaveni[velikost_sachovnice_10x12 + 1];

// --- druh tahu --------------------------------------------------------------
// Přidáno proti originálu. Tam pole Zasobnik_tahu[].en_passant znamenalo dvě
// různé věci - u dvojkroku cílové pole pro braní mimochodem (ř. 884, 908),
// u braní mimochodem pole braného pěšce (ř. 829) - a zahraj_tah je rozlišoval
// testem, zda je to pole prázdné (ř. 2650). Fungovalo to jen proto, že se ep
// řeší ještě před přesunem figury na ř. 2666. Tady je místo toho explicitní
// druh a dvě oddělená pole. Viz POZNAMKY-REDMAX.md, bod 1.
//
// POZOR: `Druh::Brani` NENÍ spolehlivý test na to, jestli tah bere - braná
// proměna má `Druh::Promena` a neprázdnou `brana_figura`. Kdo se ptá na braní
// (třeba proto, že killery se ukládají jen u tichých tahů), musí se ptát
// `brana_figura != prazdne_pole`.
enum class Druh {
    Tichy,
    Brani,
    Dvojkrok,     // dvojkrok pěšce, nastavuje ep_cil
    EnPassant,    // braní mimochodem, nastavuje ep_brane_pole
    Rosada,
    Promena       // proměna, i když je zároveň braním (brana_figura to řekne)
};

// --- typ_tahu (Pascal ř. 421-429) -------------------------------------------
struct typ_tahu {
    int  z_pole           = 0;
    int  na_pole          = 0;
    int  brana_figura     = prazdne_pole;   // vždy kladný typ (ABS)
    int  premenena_figura = prazdne_pole;
    int  druh_rosady      = zadna_rosada;
    int  ep_cil           = nelegalni;      // jen u Druh::Dvojkrok
    int  ep_brane_pole    = nelegalni;      // jen u Druh::EnPassant
    Druh druh             = Druh::Tichy;
    int  hodnota          = 0;              // pořadí tahů
};

// --- pořadí tahů ------------------------------------------------------------
// V originále sahal `uloz_tah` přímo do `tabulka_hlavni_varianty`,
// `KillerTabulka` a `hash_table` (ř. 745-761 a 794-808), tedy do souborových
// globálů vrstvy hledání. Jádro na vrstvu hledání záviset nesmí a globály
// nezavádíme, takže se to obrátilo: vrstva hledání sem před každým voláním
// `Generuj_tahy` zapíše, který tah má na TÉHLE hloubce dostat který bonus,
// a jádro už jen porovnává. Hodnoty bonusů ani pravidla se nemění.
//
// Nevyplněná položka má z_pole = 0, což není žádné pole šachovnice (21..98),
// takže se nikdy netrefí - stejně jako vynulované tabulky v Pascalu.
struct PoradiTahu {
    int  pv_z_pole = 0,   pv_na_pole = 0;     // tah hlavní varianty
    int  k1_z_pole = 0,   k1_na_pole = 0;     // 1. killer
    int  k2_z_pole = 0,   k2_na_pole = 0;     // 2. killer
    int  hash_z_pole = 0, hash_na_pole = 0;   // nejlepší tah z hash tabulky
    bool hash_platny = false;                 // je záznam v tabulce pro tuhle pozici?
};

// --- stav (v Pascalu souborové globály, ř. 459-486) -------------------------
// Žádná globální proměnná; vše je členem této struktury a volající ji vlastní.
// Pole indexovaná hloubkou zůstávají indexovaná hloubkou: vezmi_tah_zpet nic
// nerestauruje, jen sníží hloubku. To je nosný princip originálu.
struct Pozice {
    int sachovnice[velikost_sachovnice_10x12 + 1]{};
    int pohyb[H8 + 1]{};        // počítá pohyby figury, z toho plynou práva na rošádu
    int rosada[3]{};            // rosada[barva + 1] - už ta strana rochovala?
    int barva  = bily;
    int hloubka = 0;
    int pozice_bileho_krale = E1;
    int pozice_cerneho_krale = E8;
    int Index = 0;

    typ_tahu Zasobnik_tahu[velikost_zasobniku_tahu + 1]{};

    // hranice_zasobniku se zapisuje na [hloubka + 1] (ř. 1018), takže potřebuje
    // o prvek víc než ostatní. V Pascalu je stejně velké jako ostatní, ale za
    // konec se tam nikdy nezapíše: AlfaBeta se na ř. 1665 vrací při
    // hloubka >= max_hloubka_hledani dřív, než na ř. 1791 zavolá generátor.
    // Prvek navíc je tedy preventivní, ne oprava živé chyby.
    // Viz POZNAMKY-REDMAX.md, bod 5.
    int hranice_zasobniku[max_hloubka_hledani + 2]{};

    int cilove_pole[max_hloubka_hledani + 1]{};
    int en_passant[max_hloubka_hledani + 1]{};
    int materialova_bilance[max_hloubka_hledani + 1]{};
    int celkovy_material_na_sachovnici[max_hloubka_hledani + 1]{};

    // Počítadla z FEN. Originál je nemá. zahraj_tah je NEMĚNÍ: pultahy_bez_zmeny
    // vede vrstva UCI mezi tahy partie (najdi_a_zahraj) a hledání si ho od
    // Kroku 17/B vede samo po hloubkách, viz StavHledani::pultahy.
    int pultahy_bez_zmeny = 0;
    int cislo_tahu = 1;

    // Vstup od vrstvy hledání pro pořadí tahů na aktuální hloubce. Jádro do
    // něj nikdy nezapisuje, jen ho čte v uloz_tah a uloz_utocny_tah.
    PoradiTahu poradi;

    // Přetečení zásobníku tahů je v ostrém buildu tiché: generování se zastaví
    // a volající dostane méně tahů, ale nic mu to neřekne - perft pak vypíše
    // menší, ale věrohodné číslo. Tenhle příznak se v takovém případě nastaví
    // a testy si ho po doběhnutí zkontrolují. Nuluje ho jen nacti_fen
    // a nacti_uvodni_pozici, ne vezmi_tah_zpet.
    bool zasobnik_pretekl = false;
};

// --- rozhraní ---------------------------------------------------------------

// nacti_uvodni_pozici (ř. 513-558), jen ta část bez GUI
void nacti_uvodni_pozici(Pozice& p);

// V originále neexistuje - přidáno kvůli testovacím pozicím perftu.
// Práva na rošádu se překládají do pohyb[], viz POZNAMKY-REDMAX.md, bod 3.
bool nacti_fen(Pozice& p, const std::string& fen);
std::string zapis_fen(const Pozice& p);

// napadene_pole (ř. 641-726). `strana` je ÚTOČNÍK.
bool napadene_pole(const Pozice& p, int pole, int strana);

// Generuj_tahy (ř. 729-1020). vsechny_tahy = 0 znamená jen braní (hledání
// klidu); proměny se generují i tehdy - ř. 878 a 902 záměrně nejsou uvnitř
// podmínky, není to překlep.
// Tahy skončí v Zasobnik_tahu na indexech
// [hranice_zasobniku[hloubka], hranice_zasobniku[hloubka + 1]).
void Generuj_tahy(Pozice& p, int vsechny_tahy);

// zahraj_tah (ř. 2633-2697). Zvyšuje hloubku a PŘEPÍNÁ stranu na tahu.
void zahraj_tah(Pozice& p, int cislo_tahu);

// vezmi_tah_zpet (ř. 2699-2745). Snižuje hloubku a přepíná stranu zpět.
// Pole indexovaná hloubkou nerestauruje - nemusí.
void vezmi_tah_zpet(Pozice& p, int cislo_tahu);

// pomůcka: převod tahu na zápis typu "e2e4" / "a7a8q"
std::string tah_na_text(const typ_tahu& tah);

}  // namespace redmax
