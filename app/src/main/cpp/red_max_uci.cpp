// ---------------------------------------------------------------------------
//  RedMAX - vrstva UCI (implementace)
// ---------------------------------------------------------------------------

#include "red_max_uci.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace redmax {

namespace {

const char* JMENO = "RedMAX 4.08";
const char* AUTOR = "Stanislav, prepis do C++ 2026";

// Nad touhle mezí se skóre považuje za matové - táž mez jako v hledání.
constexpr int mez_matu = hodnota_matu - 1000;

inline std::string mala_pismena(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// --- skóre do zápisu UCI ----------------------------------------------------
//
// Hodnocení RedMAXu je rovnou v setinách pěšce (hodnota_Pesce = 100), takže
// `score cp` je hodnota beze změny.
//
// Matové skóre nese hledání vztažené KE KOŘENI (kořen má hloubku 0):
// matovaný uzel vzdálený d půltahů od kořene má hodnotu
// ±(hodnota_matu - (d + 1)), protože krále sebere až tah v hloubce d + 1
// (oceneni_pozice vrací hodnota_matu - hloubka). pultahy = d + 1.
//
// UCI chce počet TAHŮ, kladný když mat dává strana na tahu, záporný když ho
// dostává (jako Stockfish). Mat dávaný v N tazích je d = 2N - 1, tedy
// pultahy = 2N. Mat dostávaný v N tazích je d = 2N, pultahy = 2N + 1.
// V obou případech N = pultahy / 2:
//   mat 1 tahem          d = 1, pultahy 2  ->  mate 1
//   mat 2 tahy           d = 3, pultahy 4  ->  mate 2
//   Kb8 Vh8# (dostává)   d = 2, pultahy 3  ->  mate -1
std::string skore_do_uci(int hodnota) {
    if (hodnota > mez_matu || hodnota < -mez_matu) {
        const int pultahy = hodnota_matu - std::abs(hodnota);
        // RedCry krok U2: bylo (pultahy + 1) / 2, dostávaný mat pak vyšel
        // o tah vyšší než podle UCI (Kb8 Vh8# "mate -2" místo "mate -1").
        // Nález z kroku S2/A, docs/POZNAMKY-STOCKFISH.md a POZNAMKY-REDMAX.md.
        const int tahy = pultahy / 2;
        const int n = (hodnota > 0) ? tahy : -tahy;
        return "score mate " + std::to_string(n);
    }
    return "score cp " + std::to_string(hodnota);
}

unsigned zabal_tah(const typ_tahu& t) {
    return (static_cast<unsigned>(t.z_pole) << 16) |
           (static_cast<unsigned>(t.na_pole) << 8) |
            static_cast<unsigned>(t.premenena_figura);
}

typ_tahu rozbal_tah(unsigned k) {
    typ_tahu t;
    t.z_pole           = static_cast<int>((k >> 16) & 0xFF);
    t.na_pole          = static_cast<int>((k >>  8) & 0xFF);
    t.premenena_figura = static_cast<int>(k & 0xFF);
    return t;
}

// Král strany, která právě táhla, zůstal napadený - tah byl nelegální.
// Po zahraj_tah je barva otočená, takže táhla strana -barva.
bool tah_byl_nelegalni(const Pozice& p) {
    const int tahnouci = -p.barva;
    const int kral = (tahnouci == bily) ? p.pozice_bileho_krale : p.pozice_cerneho_krale;
    return napadene_pole(p, kral, p.barva);
}

bool strana_na_tahu_v_sachu(const Pozice& p) {
    const int kral = (p.barva == bily) ? p.pozice_bileho_krale : p.pozice_cerneho_krale;
    return napadene_pole(p, kral, -p.barva);
}

// --- srovnání pozice zpátky na hloubku 0 ------------------------------------
//
// `zahraj_tah` zvyšuje hloubku a pole indexovaná hloubkou nechává na nové
// úrovni; `vezmi_tah_zpet` jen sníží index zpátky. To je pro hledání správné,
// ale partie se hraje na jedné úrovni: kdyby se hloubka po každém zahraném
// tahu zvedla, po padesáti tazích by se psalo za konec polí.
//
// Přesune se tedy to, co na nové hloubce vzniklo, na index 0. Přes FEN by to
// šlo taky (`nacti_fen(zapis_fen(p))`), ale tohle je přesnější: `pohyb[]`
// zůstane, jak je, a nezredukuje se na čtyři bity práv na rošádu.
void srovnej_na_koren(Pozice& p) {
    const int h = p.hloubka;
    if (h != 0) {
        p.materialova_bilance[0]            = p.materialova_bilance[h];
        p.celkovy_material_na_sachovnici[0] = p.celkovy_material_na_sachovnici[h];
        p.en_passant[0]                     = p.en_passant[h];
        p.hloubka = 0;
    }
    p.cilove_pole[0]      = 0;
    p.hranice_zasobniku[0] = 0;
    p.Index = 0;
}

// --- SAN --------------------------------------------------------------------
//
// `sloupec_pole` a `rada_pole` jsou v jádře jen jako soubor-lokální inline
// funkce, takže tu jsou znovu. Mailbox 0..119: A1 = 21, H8 = 98.
inline int san_sloupec(int pole) { return pole % 10; }       // 1..8
inline int san_rada(int pole)    { return pole / 10 - 1; }   // 1..8

// Písmena figur jsou ANGLICKÁ bez ohledu na jazyk aplikace - tak se SAN píše
// všude a tak ho bude číst i PGN. Pěšec písmeno nemá.
char pismeno_figury(int typ) {
    switch (typ) {
        case b_Vez:     return 'R';
        case b_Strelec: return 'B';
        case b_Jezdec:  return 'N';
        case b_Dama:    return 'Q';
        case b_Kral:    return 'K';
        default:        return '\0';   // pěšec
    }
}

void pripoj_pole(std::string& s, int pole) {
    s += static_cast<char>('a' + san_sloupec(pole) - 1);
    s += static_cast<char>('0' + san_rada(pole));
}

// Co je o jednom legálním tahu potřeba vědět, než se dá zapsat. Rozlišení
// se počítá až z celé sady, proto se nejdřív posbírá tohle a teprve pak
// se skládají řetězce.
struct SanZaznam {
    typ_tahu tah{};
    int      typ_figury = prazdne_pole;   // kladný typ táhnoucího kamene
    bool     bere = false;
    bool     sach = false;
    bool     mat  = false;
};

}  // namespace

// --- legální tahy -----------------------------------------------------------
std::vector<std::string> legalni_tahy(Pozice& p) {
    std::vector<std::string> ven;

    p.poradi = PoradiTahu{};       // žádné bonusy, pořadí je tu jedno
    p.hranice_zasobniku[p.hloubka] = p.Index;
    Generuj_tahy(p, 1);

    const int od = p.hranice_zasobniku[p.hloubka];
    const int po = p.hranice_zasobniku[p.hloubka + 1];
    for (int i = od; i < po; ++i) {
        zahraj_tah(p, i);
        const bool legalni = !tah_byl_nelegalni(p);
        vezmi_tah_zpet(p, i);
        if (legalni) ven.push_back(tah_na_text(p.Zasobnik_tahu[i]));
    }

    p.Index = od;                  // zásobník zpátky, jak byl
    return ven;
}

// --- legální tahy i se zápisem v SAN ----------------------------------------
//
// Dva průchody. První zahraje každý pseudolegální tah, zahodí nelegální
// a u zbylých si zapamatuje to, co se dá zjistit jen se zahraným tahem: šach
// a mat. Druhý skládá řetězce, protože rozlišení potřebuje vidět všechny
// ostatní legální tahy naráz.
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem(Pozice& p) {
    p.poradi = PoradiTahu{};       // žádné bonusy, pořadí je tu jedno
    p.hranice_zasobniku[p.hloubka] = p.Index;
    Generuj_tahy(p, 1);

    const int od = p.hranice_zasobniku[p.hloubka];
    const int po = p.hranice_zasobniku[p.hloubka + 1];

    std::vector<SanZaznam> zaznamy;
    zaznamy.reserve(static_cast<size_t>(po - od));

    for (int i = od; i < po; ++i) {
        SanZaznam z;
        // KOPIE, ne odkaz do zásobníku: vnořená generace níž sice píše až nad
        // tenhle úsek, ale odkaz do pole, do kterého se mezitím zapisuje, je
        // zbytečná past.
        z.tah        = p.Zasobnik_tahu[i];
        z.typ_figury = std::abs(p.sachovnice[z.tah.z_pole]);

        // Braní: u pěšce stačí změna sloupce, a tím se bez zvláštní větve
        // pokryje i braní mimochodem, kde je cílové pole prázdné. U ostatních
        // figur rozhoduje obsazenost cílového pole.
        z.bere = (z.typ_figury == b_Pesec)
                     ? (san_sloupec(z.tah.z_pole) != san_sloupec(z.tah.na_pole))
                     : (p.sachovnice[z.tah.na_pole] != prazdne_pole);

        zahraj_tah(p, i);
        const bool je_legalni = !tah_byl_nelegalni(p);
        if (je_legalni) {
            z.sach = strana_na_tahu_v_sachu(p);
            // Vnořené volání legalni_tahy() na o jednu vyšší hloubce. Jde to:
            // pole jsou indexovaná hloubkou, takže vnitřní generace si vezme
            // hranice_zasobniku[hloubka] = Index (což je konec tahů vnějšího
            // cyklu), zapíše NAD ně a na konci vrátí Index tam, kde ho našla.
            // Vnější úsek [od, po) se tím nepřepíše.
            z.mat = z.sach && legalni_tahy(p).empty();
        }
        vezmi_tah_zpet(p, i);

        if (je_legalni) zaznamy.push_back(z);
    }

    p.Index = od;                  // zásobník zpátky, jak byl

    std::vector<std::pair<std::string, std::string>> ven;
    ven.reserve(zaznamy.size());

    for (const SanZaznam& z : zaznamy) {
        std::string san;

        if (z.tah.druh_rosady == mala_rosada) {
            san = "O-O";           // písmeno O, ne nula
        } else if (z.tah.druh_rosady == velka_rosada) {
            san = "O-O-O";
        } else if (z.typ_figury == b_Pesec) {
            // Pěšec nemá písmeno a rozlišení nikdy nepotřebuje; u braní se
            // sloupec výchozího pole uvádí VŽDY, i když je tah jednoznačný.
            if (z.bere) {
                san += static_cast<char>('a' + san_sloupec(z.tah.z_pole) - 1);
                san += 'x';
            }
            pripoj_pole(san, z.tah.na_pole);
            if (z.tah.premenena_figura != prazdne_pole) {
                san += '=';
                san += pismeno_figury(z.tah.premenena_figura);
            }
        } else {
            san += pismeno_figury(z.typ_figury);

            // Rozlišení jen tehdy, když na totéž pole může JINÁ figura téhož
            // druhu a téže barvy. Počítá se ze `zaznamy`, tedy z LEGÁLNÍCH
            // tahů - vázaný jezdec, který tam legálně nemůže, rozlišení
            // nevynucuje.
            bool kolize = false, sloupec_kolize = false, rada_kolize = false;
            for (const SanZaznam& j : zaznamy) {
                if (j.typ_figury != z.typ_figury) continue;
                if (j.tah.na_pole != z.tah.na_pole) continue;
                if (j.tah.z_pole == z.tah.z_pole) continue;
                kolize = true;
                if (san_sloupec(j.tah.z_pole) == san_sloupec(z.tah.z_pole)) sloupec_kolize = true;
                if (san_rada(j.tah.z_pole)    == san_rada(z.tah.z_pole))    rada_kolize    = true;
            }
            if (kolize) {
                // Přednost má sloupec, pak řada, nakonec celé výchozí pole.
                if (!sloupec_kolize) {
                    san += static_cast<char>('a' + san_sloupec(z.tah.z_pole) - 1);
                } else if (!rada_kolize) {
                    san += static_cast<char>('0' + san_rada(z.tah.z_pole));
                } else {
                    pripoj_pole(san, z.tah.z_pole);
                }
            }

            if (z.bere) san += 'x';
            pripoj_pole(san, z.tah.na_pole);
        }

        if      (z.mat)  san += '#';
        else if (z.sach) san += '+';

        ven.emplace_back(tah_na_text(z.tah), san);
    }

    return ven;
}

// --- rozpočet času ----------------------------------------------------------
//
// Schválně jednoduché: zbývající čas dělený předpokládaným počtem tahů plus
// tři čtvrtiny přírůstku, a nikdy víc než polovina toho, co zbývá - aby se
// motor nedostal do situace, kdy mu po jednom tahu nezbude nic. Až bude na
// čem měřit, vymění se tahle jedna funkce a nic jiného.
int rozpocitej_cas(int zbyva_ms, int prirustek_ms, int tahu_do_kontroly) {
    if (zbyva_ms <= 0) return 0;

    const int tahu = (tahu_do_kontroly > 0) ? tahu_do_kontroly : 30;
    int cas = zbyva_ms / tahu + (prirustek_ms * 3) / 4;

    const int strop = zbyva_ms / 2;
    if (cas > strop) cas = strop;
    if (cas < 10)    cas = 10;
    return cas;
}

// --- PrubeznyStav -----------------------------------------------------------
void PrubeznyStav::vynuluj() {
    hloubka_.store(0, std::memory_order_relaxed);
    skore_.store(0, std::memory_order_relaxed);
    uzlu_.store(0, std::memory_order_relaxed);
    tah_.store(0, std::memory_order_relaxed);
}

void PrubeznyStav::zapis(const PrubeznyVysledek& p) {
    skore_.store(p.hodnota, std::memory_order_relaxed);
    uzlu_.store(static_cast<long long>(p.uzlu), std::memory_order_relaxed);
    tah_.store(p.tah_nalezen ? zabal_tah(p.nejlepsi_tah) : 0u,
               std::memory_order_relaxed);
    // hloubka se zapisuje POSLEDNÍ - je to příznak, že ostatní tři hodnoty
    // už patří k sobě
    hloubka_.store(p.hloubka, std::memory_order_release);
}

std::string PrubeznyStav::text() const {
    const int h = hloubka_.load(std::memory_order_acquire);
    if (h <= 0) return {};
    std::ostringstream o;
    o << "depth " << h
      << ' ' << skore_do_uci(skore_.load(std::memory_order_relaxed))
      << " nodes " << uzlu_.load(std::memory_order_relaxed);
    const unsigned k = tah_.load(std::memory_order_relaxed);
    if (k) o << " pv " << tah_na_text(rozbal_tah(k));
    return o.str();
}

void UciEngine::hlaseni_trampolina(void* kontext, const PrubeznyVysledek& p) {
    UciEngine* self = static_cast<UciEngine*>(kontext);
    self->prubeh_.zapis(p);
    std::ostringstream o;
    o << "info depth " << p.hloubka
      << ' ' << skore_do_uci(p.hodnota)
      << " nodes " << p.uzlu;
    if (p.tah_nalezen) o << " pv " << tah_na_text(p.nejlepsi_tah);
    self->info_radky_.push_back(o.str());
}

std::vector<std::string> rozdel(const std::string& s) {
    std::vector<std::string> ven;
    std::istringstream in(s);
    std::string w;
    while (in >> w) ven.push_back(w);
    return ven;
}

UciEngine::UciEngine() {
    nast_.max_hloubka = 6;
    // Zobristova čísla a tabulku založit hned, ať je motor použitelný i bez
    // `ucinewgame`. Opakované volání nic nezkazí, je to idempotentní.
    zaloz_motor(trvaly_, nast_.velikost_hash_mb);
    nova_hra();
}

void UciEngine::nova_hra() {
    nacti_uvodni_pozici(pozice_);
    // Nová partie nedědí záznamy po předchozí. Zobristova čísla zůstávají -
    // ta se měnit nesmějí, dokud v tabulce něco leží.
    vymaz_hash(trvaly_);
    historie_.clear();
    zapamatuj_pozici();
}

// Klíč aktuální pozice na konec historie (Krok 4/2). Volá se po každé změně
// pozice, takže poslední prvek je vždycky ta pozice, ve které se bude hledat.
void UciEngine::zapamatuj_pozici() {
    historie_.push_back(urci_klic_od_nuly(pozice_, trvaly_));
}

void UciEngine::zastav_hledani() {
    zastav_.store(true, std::memory_order_relaxed);
}

// --- najde tah podle zápisu e2e4 / a7a8q a zahraje ho -----------------------
bool UciEngine::najdi_a_zahraj(const std::string& zapis) {
    if (zapis.size() < 4) return false;

    pozice_.poradi = PoradiTahu{};
    pozice_.hranice_zasobniku[pozice_.hloubka] = pozice_.Index;
    Generuj_tahy(pozice_, 1);

    const int od = pozice_.hranice_zasobniku[pozice_.hloubka];
    const int po = pozice_.hranice_zasobniku[pozice_.hloubka + 1];

    int nalezeny = -1;
    for (int i = od; i < po; ++i) {
        if (tah_na_text(pozice_.Zasobnik_tahu[i]) != zapis) continue;
        zahraj_tah(pozice_, i);
        const bool legalni = !tah_byl_nelegalni(pozice_);
        vezmi_tah_zpet(pozice_, i);
        if (legalni) { nalezeny = i; break; }
    }

    // Proměna bez určené figury: UI se na ni nemusí ptát, doplní se dáma -
    // stejně jako u Light Redu.
    if (nalezeny < 0 && zapis.size() == 4) {
        const std::string sdama = zapis + "q";
        for (int i = od; i < po; ++i) {
            if (tah_na_text(pozice_.Zasobnik_tahu[i]) != sdama) continue;
            zahraj_tah(pozice_, i);
            const bool legalni = !tah_byl_nelegalni(pozice_);
            vezmi_tah_zpet(pozice_, i);
            if (legalni) { nalezeny = i; break; }
        }
    }

    if (nalezeny < 0) { pozice_.Index = od; return false; }

    const typ_tahu tah = pozice_.Zasobnik_tahu[nalezeny];
    const bool byl_pesec = std::abs(pozice_.sachovnice[tah.z_pole]) == b_Pesec;
    const bool bere      = tah.brana_figura != prazdne_pole;
    const int  tahl      = pozice_.barva;

    zahraj_tah(pozice_, nalezeny);
    srovnej_na_koren(pozice_);

    if (byl_pesec || bere) pozice_.pultahy_bez_zmeny = 0;
    else                   ++pozice_.pultahy_bez_zmeny;
    if (tahl == cerny) ++pozice_.cislo_tahu;

    // Tudy jdou obě cesty, jak se tah dostane do partie: `applymove` z UI
    // i každý tah z `position ... moves ...`. Kdyby se historie plnila jinde,
    // fungovalo by to jen v jedné z nich.
    zapamatuj_pozici();

    return true;
}

// --- position startpos|fen ... [moves ...] ----------------------------------
bool UciEngine::nastav_pozici(const std::vector<std::string>& slova, size_t od) {
    size_t i = od;
    if (i >= slova.size()) return false;

    // Nová pozice je začátek nové partie, ať přijde odkudkoli: historie se
    // vyprázdní a naplní se znovu, jak se přehrávají tahy z `moves`.
    historie_.clear();

    if (slova[i] == "startpos") {
        nacti_uvodni_pozici(pozice_);
        ++i;
    } else if (slova[i] == "fen") {
        ++i;
        std::string fen;
        while (i < slova.size() && slova[i] != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += slova[i];
            ++i;
        }
        if (!nacti_fen(pozice_, fen)) return false;
    } else {
        return false;
    }

    // Výchozí pozice partie. Tahy z `moves` si klíč přidají samy
    // v najdi_a_zahraj.
    zapamatuj_pozici();

    if (i < slova.size() && slova[i] == "moves") {
        ++i;
        for (; i < slova.size(); ++i) {
            if (!najdi_a_zahraj(slova[i])) return false;
        }
    }
    return true;
}

// --- setoption name <jméno> value <hodnota> ---------------------------------
std::vector<std::string> UciEngine::nastav_volbu(const std::vector<std::string>& slova) {
    std::string jmeno, hodnota;
    size_t i = 1;
    if (i < slova.size() && slova[i] == "name") {
        ++i;
        while (i < slova.size() && slova[i] != "value") {
            if (!jmeno.empty()) jmeno += ' ';
            jmeno += slova[i];
            ++i;
        }
    }
    if (i < slova.size() && slova[i] == "value") {
        ++i;
        while (i < slova.size()) {
            if (!hodnota.empty()) hodnota += ' ';
            hodnota += slova[i];
            ++i;
        }
    }

    const std::string j = mala_pismena(jmeno);
    const std::string h = mala_pismena(hodnota);
    const bool ano = (h == "true" || h == "1" || h == "yes" || h == "on");

    if (j == "hash") {
        // Tabulka teď žije mezi tahy, takže změna velikosti znamená skutečnou
        // REALOKACI, ne jen poznámku do nastavení. Maska se mění spolu s ní;
        // od návratu dál nesmí nikdo držet index spočítaný ze staré masky.
        // Indexy se počítají jen uvnitř jednoho hledání, takže tady mezi
        // dvěma `go` je to bezpečné místo.
        const int mb = std::atoi(hodnota.c_str());
        nast_.velikost_hash_mb = (mb < 1) ? 1 : (mb > 256 ? 256 : mb);
        zmen_velikost_hashe(trvaly_, nast_.velikost_hash_mb);
    } else if (j == "nullmove") {
        nast_.pouzij_null_move = ano;
    } else if (j == "killery") {
        nast_.pouzij_killery = ano;
    } else if (j == "pvbonus") {
        if      (h == "vzdy")  nast_.rezim_pv = RezimPV::Vzdy;
        else if (h == "nikdy") nast_.rezim_pv = RezimPV::Nikdy;
        else                   nast_.rezim_pv = RezimPV::JenNaHlavniVariante;
    } else if (j == "pozicnikriteria") {
        nast_.pozicni_kriteria = ano;
    } else if (j == "faillow") {
        nast_.ukladej_fail_low = ano;
    } else if (j == "trvalahash") {
        // Měřicí přepínač (Krok 4/1): false = tabulka se před každým hledáním
        // vynuluje, tedy chování před Krokem 4. Pro hru se nevypíná.
        nast_.trvala_hash = ano;
    } else if (j == "opakovaniprespartii") {
        // Měřicí přepínač (Krok 4/2): false = detekce opakování jako před
        // Krokem 4/2, tedy jen čtyři půltahy zpátky a bez historie partie.
        nast_.opakovani_pres_partii = ano;
    } else if (j == "pravidlo50") {
        // Měřicí přepínač (Krok 17/B): false = hledání o pravidle 50 tahů
        // neví, tedy chování do Kroku 17. Pro hru se nevypíná.
        nast_.pravidlo_50 = ano;
    } else if (j == "starizaznamu") {
        // Měřicí přepínač (Krok 4/1): false = politika nahrazování z Kroku 2,
        // která o stáří záznamu neví.
        nast_.stari_zaznamu = ano;
    } else if (j == "depth") {
        nast_.max_hloubka = std::max(1, std::atoi(hodnota.c_str()));
    } else {
        return {"info string neznama volba: " + jmeno};
    }

    return {};
}

// --- go ---------------------------------------------------------------------
std::vector<std::string> UciEngine::proved_go(const std::vector<std::string>& slova) {
    Nastaveni n = nast_;
    n.cas_ms = 0;

    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
    int movetime = 0;
    bool nekonecno = false;
    bool zadana_hloubka = false;

    for (size_t i = 1; i < slova.size(); ++i) {
        const std::string& s = slova[i];
        auto cislo = [&](int implicit) {
            return (i + 1 < slova.size()) ? std::atoi(slova[++i].c_str()) : implicit;
        };
        if      (s == "depth")     { n.max_hloubka = std::max(1, cislo(n.max_hloubka));
                                     zadana_hloubka = true; }
        else if (s == "movetime")  movetime = cislo(0);
        else if (s == "wtime")     wtime = cislo(0);
        else if (s == "btime")     btime = cislo(0);
        else if (s == "winc")      winc = cislo(0);
        else if (s == "binc")      binc = cislo(0);
        else if (s == "movestogo") movestogo = cislo(0);
        else if (s == "infinite")  nekonecno = true;
    }

    if (movetime > 0) {
        n.cas_ms = movetime;
    } else if (!nekonecno) {
        const int zbyva = (pozice_.barva == bily) ? wtime : btime;
        const int prir  = (pozice_.barva == bily) ? winc  : binc;
        if (zbyva > 0) n.cas_ms = rozpocitej_cas(zbyva, prir, movestogo);
    }

    // Když čas rozhoduje a hloubku nikdo nezadal, ať hloubka nebrzdí: strop
    // je maximum, které hledání unese, a zastaví ho stopky. Bez tohohle by
    // `go movetime 5000` skončilo na šesté hloubce a zbytek času zahodilo.
    if ((n.cas_ms > 0 || nekonecno) && !zadana_hloubka) {
        n.max_hloubka = max_hloubka_hledani;
    }

    zastav_.store(false, std::memory_order_relaxed);
    prubeh_.vynuluj();
    info_radky_.clear();
    n.hlaseni = &UciEngine::hlaseni_trampolina;
    n.hlaseni_kontext = this;

    const auto t0 = std::chrono::steady_clock::now();
    const Vysledek v = hledej(pozice_, n, zastav_, trvaly_, &historie_);
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - t0).count();

    // `hledej` nechá kořenové tahy ležet v zásobníku a `Index` za nimi.
    // Generuj_tahy pak příště začne až tam (ř. 250 jádra), takže by každé
    // další `go` bez změny pozice ukroulo dalších dvacet až čtyřicet míst
    // a po stovce hledání by zásobník přetekl. V partii se to neprojeví -
    // po každém tahu se pozice srovná - ale kdo mačká "Přemýšlej" dokola,
    // ten by se tam dostal. Na úrovni UCI je pozice vždycky kořen, takže
    // se invariant "v klidu je Index nula" obnoví tady.
    srovnej_na_koren(pozice_);

    // jeden info řádek za každou dokončenou hloubku, jak je nasbírala
    // trampolína, a nakonec souhrn s časem
    std::vector<std::string> ven = info_radky_;

    std::ostringstream info;
    info << "info depth " << (v.dosazena_hloubka > 0 ? v.dosazena_hloubka : 1)
         << ' ' << skore_do_uci(v.hodnota)
         << " nodes " << v.uzlu
         << " time " << ms;
    if (v.tah_nalezen) {
        info << " pv";
        for (const typ_tahu& t : v.hlavni_varianta) info << ' ' << tah_na_text(t);
    }
    ven.push_back(info.str());

    if (v.preruseno) ven.push_back("info string hledani preruseno");

    if (v.tah_nalezen) {
        ven.push_back("bestmove " + tah_na_text(v.nejlepsi_tah));
    } else if (v.preruseno) {
        // Přerušeno dřív, než doběhla první hloubka. Není to mat ani pat -
        // hledání jen nestihlo nic vrátit, a tvrdit tu mat by bylo horší
        // než přiznat, že nic není.
        ven.push_back("info string preruseno pred prvni dokoncenou hloubkou");
        ven.push_back("bestmove (none)");
    } else {
        const bool sach = strana_na_tahu_v_sachu(pozice_);
        ven.push_back(std::string("info string ") + (sach ? "mat" : "pat"));
        ven.push_back("bestmove (none)");
    }
    return ven;
}

// --- rozcestník --------------------------------------------------------------
std::vector<std::string> UciEngine::prikaz(const std::string& radek) {
    const std::vector<std::string> slova = rozdel(radek);
    if (slova.empty()) return {};

    const std::string& c = slova[0];

    if (c == "uci") {
        return {
            std::string("id name ") + JMENO,
            std::string("id author ") + AUTOR,
            "option name Hash type spin default 4 min 1 max 256",
            "option name NullMove type check default true",
            "option name Killery type check default true",
            "option name PVBonus type combo default JenNaHV var Vzdy var Nikdy var JenNaHV",
            "option name PozicniKriteria type check default true",
            "option name FailLow type check default true",
            "option name Depth type spin default 6 min 1 max 50",
            "option name TrvalaHash type check default true",
            "option name StariZaznamu type check default true",
            "option name OpakovaniPresPartii type check default true",
            "option name Pravidlo50 type check default true",
            "uciok"
        };
    }

    if (c == "isready")    return {"readyok"};
    if (c == "ucinewgame") { nova_hra(); return {}; }
    if (c == "quit")       { skoncit_ = true; return {}; }
    if (c == "stop")       { zastav_hledani(); return {}; }
    if (c == "setoption")  return nastav_volbu(slova);
    if (c == "go")         return proved_go(slova);

    if (c == "position") {
        if (!nastav_pozici(slova, 1)) return {"info string neplatne position"};
        return {};
    }

    // --- příkazy nad rámec UCI, pro UI ---

    if (c == "legalmoves") {
        std::string s;
        for (const std::string& t : legalni_tahy(pozice_)) {
            if (!s.empty()) s += ' ';
            s += t;
        }
        return {s};
    }

    // Dvojtečka je oddělovač proto, že se nevyskytuje ani v zápisu UCI, ani
    // v SAN. Rovnítko ne - to je uvnitř zápisu proměny (e8=Q).
    if (c == "sanmoves") {
        std::string s;
        for (const auto& t : tahy_se_zapisem(pozice_)) {
            if (!s.empty()) s += ' ';
            s += t.first;
            s += ':';
            s += t.second;
        }
        return {s};
    }

    if (c == "applymove") {
        if (slova.size() < 2) return {"info string chybi tah"};
        if (!najdi_a_zahraj(slova[1])) return {"info string nelegalni tah: " + slova[1]};
        return {"ok"};
    }

    if (c == "d" || c == "board") {
        // PRVNÍ řádek je FEN a nic jiného - nativeFen z něj bere první řádek.
        // Poradi je zavazne: (1) zadny legalni tah -> mat, nebo pat,
        // (2) jinak aspon 100 pultahu bez brani a tahu pescem -> remiza50,
        // (3) jinak normalni. Mat na stem pultahu je tedy mat (FIDE 5.1.1,
        // 9.6.2). Remiza50 je jen ohlaseni pro UI - hledani o pravidle nevi.
        const bool je_tah = !legalni_tahy(pozice_).empty();
        const char* st = !je_tah ? (strana_na_tahu_v_sachu(pozice_) ? "mat" : "pat")
                       : (pozice_.pultahy_bez_zmeny >= 100) ? "remiza50"
                       : "normalni";
        return {zapis_fen(pozice_), std::string("stav ") + st};
    }

    return {"info string neznamy prikaz: " + c};
}

}  // namespace redmax
