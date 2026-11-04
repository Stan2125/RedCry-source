// ---------------------------------------------------------------------------
//  Light Red - vrstva hledání (implementace)
//  Přepis vnor_se + serad_tahy + pridej_hodnotu_lepsim_tahum
//  + zjisti_zda_je_legalni_NEJLEPSI_tah z Light_Red_211_2_f.pas.
// ---------------------------------------------------------------------------

#include "light_red_search.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace lightred {

namespace {

constexpr double nekonecno  = 999999.0;
constexpr double prioritaC  = 0.000001;

// Původní konst_MH bylo 20. Hloubka rekurze je omezená max_hledej_klid,
// tady je rezerva.
constexpr int MAX_UROVNI = 40;

inline Strana oponent(Strana s) { return s == BILY ? CERNY : BILY; }

inline bool stejny_tah(const Tah& a, const Tah& b) {
    return a.z_pole.x  == b.z_pole.x  && a.z_pole.y  == b.z_pole.y &&
           a.na_pole.x == b.na_pole.x && a.na_pole.y == b.na_pole.y;
}

// --- zahajovací preference (const lepsi_tahy_bileho / _cerneho) -------------
struct PreferovanyTah {
    Figura figura;
    int    zX, zY, naX, naY;
    double hodnota;
};

// prioritaA = 0.02, B = 0.03, C = 0.000001, E = 0.01, F = 0.007
const PreferovanyTah lepsi_tahy_bileho[] = {
    {PESEC  , 5, 2, 5, 4, 0.040},   // e4
    {PESEC  , 4, 2, 4, 4, 0.030},   // d4
    {JEZDEC , 7, 1, 6, 3, 0.037},   // Jf3
    {JEZDEC , 2, 1, 3, 3, 0.030},   // Jc3
    {KRAL   , 5, 1, 7, 1, 0.030},   // 0-0
    {KRAL   , 5, 1, 3, 1, 0.020},   // 0-0-0
    {STRELEC, 6, 1, 3, 4, 0.020},   // Sc4
    {STRELEC, 3, 1, 6, 4, 0.020},   // Sf4
    {VEZ    , 1, 1, 4, 1, 0.020},   // Vad1
    {VEZ    , 6, 1, 5, 1, 0.020},   // Vfe1
    {PESEC  , 3, 2, 3, 4, 0.010},   // c4
    {PESEC  , 6, 2, 6, 4, 0.010},   // f4
    {STRELEC, 3, 1, 2, 2, 0.010},   // Sb2
    {STRELEC, 6, 1, 7, 2, 0.010},   // Sg2
    {JEZDEC , 2, 1, 4, 2, 0.010},   // Jbd2
    {JEZDEC , 7, 1, 5, 2, 0.010},   // Jge2
    {STRELEC, 6, 1, 5, 2, 0.010},   // Se2
    {STRELEC, 3, 1, 4, 2, 0.010},   // Sd2
    {STRELEC, 6, 1, 2, 5, 0.010},   // Sb5
    {STRELEC, 3, 1, 7, 5, 0.010},   // Sg5
    {PESEC  , 3, 4, 3, 5, 0.007},   // c5
    {PESEC  , 4, 4, 4, 5, 0.007},   // d5
    {PESEC  , 5, 4, 5, 5, 0.007},   // e5
    {PESEC  , 6, 4, 6, 5, 0.007},   // f5
    {JEZDEC , 4, 2, 2, 3, 0.007},   // Jdb3
    {JEZDEC , 4, 2, 3, 4, 0.007},   // Jdc4
    {JEZDEC , 4, 2, 5, 4, 0.007},   // Jde4
    {JEZDEC , 5, 2, 7, 3, 0.007},   // Jeg3
    {JEZDEC , 5, 2, 6, 4, 0.007},   // Jef4
    {JEZDEC , 5, 2, 4, 4, 0.007},   // Jed4
    {DAMA   , 4, 1, 1, 4, 0.007},   // Da4
    {DAMA   , 4, 1, 8, 5, 0.007},   // Dh5
    {VEZ    , 8, 1, 5, 1, 0.007},   // Vhe1
    {PESEC  , 4, 2, 4, 3, 0.007},   // d3
    {PESEC  , 4, 3, 4, 4, 0.007},   // d4
    {PESEC  , 5, 2, 5, 3, 0.007},   // e3
    {PESEC  , 5, 3, 5, 4, 0.007},   // e4
    {JEZDEC , 3, 3, 5, 4, 0.007},   // Jce4
    {JEZDEC , 3, 3, 4, 5, 0.007},   // Jcd5
    {JEZDEC , 6, 3, 4, 4, 0.007},   // Jfd4
    {JEZDEC , 6, 3, 5, 5, 0.007},   // Jfe5
};

const PreferovanyTah lepsi_tahy_cerneho[] = {
    {PESEC  , 5, 7, 5, 5, 0.040},   // e5
    {PESEC  , 4, 7, 4, 5, 0.030},   // d5
    {JEZDEC , 7, 8, 6, 6, 0.037},   // Jf6
    {JEZDEC , 2, 8, 3, 6, 0.030},   // Jc6
    {KRAL   , 5, 8, 7, 8, 0.030},   // 0-0
    {KRAL   , 5, 8, 3, 8, 0.020},   // 0-0-0
    {STRELEC, 6, 8, 3, 5, 0.020},   // Sc5
    {STRELEC, 3, 8, 6, 5, 0.020},   // Sf5
    {VEZ    , 1, 8, 4, 8, 0.020},   // Vad8
    {VEZ    , 6, 8, 5, 8, 0.020},   // Vfe8
    {PESEC  , 3, 7, 3, 5, 0.010},   // c5
    {PESEC  , 6, 7, 6, 5, 0.010},   // f5
    {STRELEC, 3, 8, 2, 7, 0.010},   // Sb7
    {STRELEC, 6, 8, 7, 7, 0.010},   // Sg7
    {JEZDEC , 2, 8, 4, 7, 0.010},   // Jbd7
    {JEZDEC , 7, 8, 5, 7, 0.010},   // Jge7
    {STRELEC, 6, 8, 5, 7, 0.010},   // Se7
    {STRELEC, 3, 8, 4, 7, 0.010},   // Sd7
    {STRELEC, 6, 8, 2, 4, 0.010},   // Sb4
    {STRELEC, 3, 8, 7, 4, 0.010},   // Sg4
    {PESEC  , 3, 5, 3, 4, 0.007},   // c4
    {PESEC  , 4, 5, 4, 4, 0.007},   // d4
    {PESEC  , 5, 5, 5, 4, 0.007},   // e4
    {PESEC  , 6, 5, 6, 4, 0.007},   // f4
    {JEZDEC , 4, 7, 2, 6, 0.007},   // Jdb6
    {JEZDEC , 4, 7, 3, 5, 0.007},   // Jdc5
    {JEZDEC , 4, 7, 5, 5, 0.007},   // Jde5
    {JEZDEC , 5, 7, 7, 6, 0.007},   // Jeg6
    {JEZDEC , 5, 7, 6, 5, 0.007},   // Jef5
    {JEZDEC , 5, 7, 4, 5, 0.007},   // Jed5
    {DAMA   , 4, 8, 1, 5, 0.007},   // Da5
    {DAMA   , 4, 8, 8, 4, 0.007},   // Dh4
    {VEZ    , 8, 8, 5, 8, 0.007},   // Vhe8
    {PESEC  , 4, 7, 4, 6, 0.007},   // d6
    {PESEC  , 4, 6, 4, 5, 0.007},   // d5
    {PESEC  , 5, 7, 5, 6, 0.007},   // e6
    {PESEC  , 5, 6, 5, 5, 0.007},   // e5
    {JEZDEC , 3, 6, 4, 4, 0.007},   // Jc6d4
    {JEZDEC , 3, 6, 5, 5, 0.007},   // Jc6e5
    {JEZDEC , 6, 6, 4, 5, 0.007},   // Jf6d5
    {JEZDEC , 6, 6, 5, 4, 0.007},   // Jf6e4
};

// ---------------------------------------------------------------------------
//  zjisti_zda_je_legalni_NEJLEPSI_tah - je uložený killer v téhle pozici
//  pseudolegální?
//
//  Pro nepěšce je "smí táhnout z A na B" totéž co "napadá B z A", takže se
//  dá použít zjisti_jen_napadeni z jádra. Král tím pádem odmítne rošádu
//  (|dx| = 2), přesně jak to dělal originál.
// ---------------------------------------------------------------------------
bool je_pseudolegalni(const Pozice& p, const Tah& t) {
    const Bod z = t.z_pole, na = t.na_pole;
    if (z.x == na.x && z.y == na.y) return false;
    if (z.x < 1 || z.x > 8 || z.y < 1 || z.y > 8) return false;
    if (na.x < 1 || na.x > 8 || na.y < 1 || na.y > 8) return false;

    const Policko& f = p.sachovnice[z.x][z.y];
    if (f.strana != p.na_tahu_je) return false;
    if (p.sachovnice[na.x][na.y].strana == f.strana) return false;

    if (f.figura != PESEC)
        return zjisti_jen_napadeni(p, na.x, na.y, z.x, z.y);

    // --- pěšec ---
    if (na.x == z.x) {
        // postup vpřed
        if (p.sachovnice[na.x][na.y].strana != NIKDO) return false;
        const int smer = (f.strana == BILY) ? 1 : -1;
        if (na.y == z.y + smer) return true;
        if (f.strana == BILY && na.y == 4 && z.y == 2 &&
            p.sachovnice[z.x][3].strana == NIKDO) return true;
        if (f.strana == CERNY && na.y == 5 && z.y == 7 &&
            p.sachovnice[z.x][6].strana == NIKDO) return true;
        return false;
    }

    // braní do strany
    if (f.strana == BILY && std::abs(na.x - z.x) == 1 && na.y - z.y == 1 &&
        p.sachovnice[na.x][na.y].strana == CERNY) return true;
    if (f.strana == CERNY && std::abs(na.x - z.x) == 1 && na.y - z.y == -1 &&
        p.sachovnice[na.x][na.y].strana == BILY) return true;

    // en passant
    if (p.en_passant.x == na.x && p.en_passant.y == na.y &&
        std::abs(na.x - z.x) == 1) {
        if (f.strana == BILY  && z.y == 5) return true;
        if (f.strana == CERNY && z.y == 4) return true;
    }
    return false;
}

// --- jedna hladina pole pozice[-1..konst_MH] --------------------------------
struct Uroven {
    Pozice p;

    double alfa = 0.0;
    double beta = 0.0;
    double hodnota = 0.0;

    int i = 0;                  // vydej_se, ale číslované od nuly
    int nakonec_zahraj = -1;
    int max_hloubka = 0;

    Tah    nejlepsi_tah[2]{};
    Figura nejlepsi_bere[2]{ZADNA, ZADNA};
    double nejlepsi_alfabeta[2]{0.0, 0.0};
    int    pocet_nejlepsich = 0;
    int    ktery_nacten = 0;    // 0 = celý seznam, 1 = killer A, 2 = killer B

    // Záloha killerů rodiče, pořízená při vstupu do uzlu. Bez ní by uzel
    // četl sloty, které si sám mezitím přepsal. Viz puvodni_chyba_killeru.
    Tah    zdroj_tah[2]{};
    Figura zdroj_bere[2]{ZADNA, ZADNA};
    int    zdroj_pocet = 0;
};

class Hledac {
public:
    explicit Hledac(const NastaveniHledani& n) : nast(n) {
        zakladni_hloubka = std::max(1, nast.max_hloubka);
        klid = std::max(zakladni_hloubka, nast.max_hledej_klid);
        klid = std::min(klid, MAX_UROVNI - 3);
    }

    VysledekHledani spust(const Pozice& start);

private:
    Uroven& U(int h) { return hladiny[h + 1]; }

    void vnor_se(int h);
    void nacti_zdroj_killeru(int h);
    bool mam_prestat();
    bool killer_pouzitelny(int h, int k);
    void generuj_zbyle(int h);
    void zapis_killer(int h, const Tah& tah, Figura bere, double skore, bool maximum);
    void kopiruj_dolu(int h);

    NastaveniHledani nast;
    int zakladni_hloubka = 4;
    int klid = 4;

    Uroven   hladiny[MAX_UROVNI];
    uint64_t listu = 0;
    uint64_t matu  = 0;

    bool     zastaveno = false;
    bool     ma_limit = false;
    int      citac_kontroly = 0;
    std::chrono::steady_clock::time_point konec_casu;
};

// Přerušení: povel zvenčí nebo vypršelý čas. Hodiny se nečtou na každém uzlu.
bool Hledac::mam_prestat() {
    if (zastaveno) return true;
    if (nast.zastav && nast.zastav->load(std::memory_order_relaxed)) {
        zastaveno = true;
        return true;
    }
    if (ma_limit && ++citac_kontroly >= 1024) {
        citac_kontroly = 0;
        if (std::chrono::steady_clock::now() >= konec_casu) {
            zastaveno = true;
            return true;
        }
    }
    return false;
}

// --- záloha killerů rodiče ---------------------------------------------------
void Hledac::nacti_zdroj_killeru(int h) {
    Uroven& a = U(h);
    Uroven& r = U(h - 1);
    a.zdroj_tah[0]  = r.nejlepsi_tah[0];
    a.zdroj_tah[1]  = r.nejlepsi_tah[1];
    a.zdroj_bere[0] = r.nejlepsi_bere[0];
    a.zdroj_bere[1] = r.nejlepsi_bere[1];
    a.zdroj_pocet   = r.pocet_nejlepsich;
}

// je k-tý zálohovaný killer v téhle pozici použitelný?
bool Hledac::killer_pouzitelny(int h, int k) {
    Uroven& a = U(h);
    const Tah& t = a.zdroj_tah[k];
    return je_pseudolegalni(a.p, t) &&
           a.p.sachovnice[t.na_pole.x][t.na_pole.y].figura == a.zdroj_bere[k];
}

// --- kopie jen nezbytného, jak to dělal originál ----------------------------
void Hledac::kopiruj_dolu(int h) {
    Uroven& a = U(h);
    Uroven& d = U(h + 1);
    std::memcpy(d.p.sachovnice, a.p.sachovnice, sizeof(d.p.sachovnice));
    d.p.na_tahu_je = oponent(a.p.na_tahu_je);
    d.p.r_Bm = a.p.r_Bm;
    d.p.r_Bv = a.p.r_Bv;
    d.p.r_Cm = a.p.r_Cm;
    d.p.r_Cv = a.p.r_Cv;
    d.p.en_passant = a.p.en_passant;
    d.p.pozice_bileho_krale  = a.p.pozice_bileho_krale;
    d.p.pozice_cerneho_krale = a.p.pozice_cerneho_krale;
    d.max_hloubka = a.max_hloubka;
}

// --- generátor + vyřazení tahů, které už zkusil killer ----------------------
void Hledac::generuj_zbyle(int h) {
    Uroven& a = U(h);

    // V režimu původní chyby se zdroj čte znovu, tedy i s tím, co si uzel
    // mezitím sám do rodiče zapsal.
    if (nast.puvodni_chyba_killeru) nacti_zdroj_killeru(h);

    zjisti_vsechny_legalni_tahy(a.p);

    for (int k = 0; k < a.zdroj_pocet; ++k) {
        // Druhá vada originálu: killer, který se vůbec nedal nasadit
        // (typicky rošáda - je_pseudolegalni u krále odmítá skok o dvě pole),
        // se přesto ze seznamu vyřadil, protože vyřazení porovnává jen
        // souřadnice. Tah tak z prohledávání zmizel, aniž by se zkusil.
        if (!nast.puvodni_chyba_killeru && !killer_pouzitelny(h, k)) continue;

        for (int i = 0; i < a.p.pocet_moznych_tahu; ++i) {
            const Tah& t = a.p.tahy[i];
            if (!stejny_tah(t, a.zdroj_tah[k])) continue;
            if (a.p.sachovnice[t.na_pole.x][t.na_pole.y].figura != a.zdroj_bere[k])
                continue;
            const bool je_pesec =
                a.p.sachovnice[t.z_pole.x][t.z_pole.y].figura == PESEC;
            if (je_pesec && t.promen_pesce != a.zdroj_tah[k].promen_pesce)
                continue;
            a.p.tahy[i] = a.p.tahy[a.p.pocet_moznych_tahu - 1];
            --a.p.pocet_moznych_tahu;
            break;
        }
    }
}

// --- záznam nejlepšího tahu pro sourozence (killer se dvěma sloty) ----------
void Hledac::zapis_killer(int h, const Tah& tah, Figura bere, double skore,
                          bool maximum) {
    if (nast.overovaci_rezim) return;
    Uroven& r = U(h - 1);

    auto lepsi = [maximum](double a, double b) { return maximum ? (a < b) : (a > b); };

    if (r.pocet_nejlepsich == 0) {
        r.nejlepsi_tah[0]       = tah;
        r.nejlepsi_bere[0]      = bere;
        r.nejlepsi_alfabeta[0]  = skore;
        r.pocet_nejlepsich      = 1;
        return;
    }

    if (lepsi(r.nejlepsi_alfabeta[0], skore)) {
        if (!stejny_tah(r.nejlepsi_tah[0], tah)) {
            r.nejlepsi_tah[1]      = r.nejlepsi_tah[0];
            r.nejlepsi_bere[1]     = r.nejlepsi_bere[0];
            r.nejlepsi_alfabeta[1] = r.nejlepsi_alfabeta[0];
            r.pocet_nejlepsich     = 2;
        }
        r.nejlepsi_tah[0]      = tah;
        r.nejlepsi_bere[0]     = bere;
        r.nejlepsi_alfabeta[0] = skore;
    } else if (lepsi(r.nejlepsi_alfabeta[1], skore)) {
        if (!stejny_tah(r.nejlepsi_tah[0], tah)) {
            r.nejlepsi_tah[1]      = tah;
            r.nejlepsi_bere[1]     = bere;
            r.nejlepsi_alfabeta[1] = skore;
            r.pocet_nejlepsich     = 2;
        }
    }
}

// ---------------------------------------------------------------------------
//  vnor_se
// ---------------------------------------------------------------------------
void Hledac::vnor_se(int h) {
    Uroven& a = U(h);
    Uroven& r = U(h - 1);
    const bool licha = (h % 2) != 0;

    a.i = 0;
    a.alfa = -nekonecno;
    a.beta =  nekonecno;
    a.pocet_nejlepsich = 0;
    a.nejlepsi_alfabeta[0] = a.nejlepsi_alfabeta[1] = licha ? -nekonecno : nekonecno;

    // --- konec větve: dosažená hloubka, nebo někdo přišel o krále ---
    if (!(h < a.max_hloubka && std::fabs(a.hodnota) < 100)) {
        double pom_hodnota = 0.0;

        // pokud bere chráněné pole -> pravděpodobně o figuru zase přijde
        if (h >= 1 && std::fabs(a.p.co_schramstne) > 0.9 &&
                      std::fabs(a.p.co_schramstne) < 90) {
            const Bod b = r.p.tahy[r.i].na_pole;
            const Figura stoji = a.p.sachovnice[b.x][b.y].figura;
            pom_hodnota = (stoji == KRAL)
                          ? hodnota_figur[KRAL] * (klid - h + 1)
                          : hodnota_figur[stoji];
            a.p.sachovnice[b.x][b.y].strana = NIKDO;
            a.p.sachovnice[b.x][b.y].figura = ZADNA;
            if (!otestuj_zda_neni_napadene(a.p, b.x, b.y, a.p.na_tahu_je))
                pom_hodnota = 0.0;
        }

        if (!licha) a.alfa = a.hodnota + pom_hodnota;
        else        a.beta = a.hodnota - pom_hodnota;

        ++listu;
        return;
    }

    a.p.stav = NORMALNI;
    a.p.pocet_moznych_tahu = 0;

    // --- načte první nejlepší tah, pokud lze ---
    nacti_zdroj_killeru(h);
    if (!nast.overovaci_rezim && a.zdroj_pocet > 0) {
        for (int k = 0; k < a.zdroj_pocet && a.p.pocet_moznych_tahu == 0; ++k) {
            if (killer_pouzitelny(h, k)) {
                a.p.tahy[0] = a.zdroj_tah[k];
                a.p.pocet_moznych_tahu = 1;
                a.ktery_nacten = k + 1;
            }
        }
    }

    if (a.p.pocet_moznych_tahu == 0) {
        generuj_zbyle(h);
        if (h == 0) {
            vymazej_tahy_kdy_je_v_sachu(a.p);
            if (nast.zahajovaci_preference) pridej_hodnotu_lepsim_tahum(a.p);
        }
        if (h + 1 < zakladni_hloubka) serad_tahy(a.p, h == 0);
        a.ktery_nacten = 0;
    }

    // --- procházení tahů ---
    while (a.i < a.p.pocet_moznych_tahu) {
        if (mam_prestat()) break;

        Uroven& d = U(h + 1);
        const Tah tah = a.p.tahy[a.i];
        const Figura bere = a.p.sachovnice[tah.na_pole.x][tah.na_pole.y].figura;

        kopiruj_dolu(h);
        zahraj_tah(d.p, tah);

        // --- prohloubí větev, pokud tažená figura dává šach ---
        bool prodlouzeno = false;
        if (h + 1 >= zakladni_hloubka - 1 && h + 1 <= klid) {
            const Bod k = (d.p.na_tahu_je == BILY) ? d.p.pozice_bileho_krale
                                                   : d.p.pozice_cerneho_krale;
            if (zjisti_jen_napadeni(d.p, k.x, k.y, tah.na_pole.x, tah.na_pole.y)) {
                d.max_hloubka += 2;
                prodlouzeno = true;
            }
        }
        // --- jinak prohloubí při braní figury ---
        if (!prodlouzeno && d.p.co_schramstne > 0.9 &&
            h + 1 >= zakladni_hloubka && h + 1 <= klid &&
            h + 1 == d.max_hloubka) {
            d.max_hloubka += 1;
        }
        if (d.max_hloubka > klid) d.max_hloubka = klid;

        if (licha) d.p.co_schramstne = -d.p.co_schramstne;

        // --- inkrementální hodnocení ---
        d.hodnota = a.hodnota + d.p.co_schramstne;
        if (d.p.co_schramstne != 0.0) {
            const double bonus =
                (static_cast<double>(klid - (h + 1) + 1) / klid) / 100.0;
            if ((h + 1) % 2 != 0) d.hodnota += bonus;
            else                  d.hodnota -= bonus;
        }
        if (h == 0) d.hodnota += tah.lepsi_tah;

        vnor_se(h + 1);

        if (!licha) {
            // sudá hladina - hrající strana hledá maximum
            if (h == 0) {
                if (!zastaveno && a.alfa < d.beta) {
                    a.alfa = d.beta;
                    a.nakonec_zahraj = a.i;
                }
            } else if (nast.overovaci_rezim || d.beta < r.beta) {
                if (a.alfa < d.beta) {
                    a.alfa = d.beta;
                    zapis_killer(h, tah, bere, a.alfa, true);
                }
            } else {
                a.i = a.p.pocet_moznych_tahu;   // odřez
                a.alfa = d.beta;
            }
        } else {
            // lichá hladina - soupeř hledá minimum
            if (nast.overovaci_rezim || d.alfa > r.alfa) {
                if (a.beta > d.alfa) {
                    a.beta = d.alfa;
                    zapis_killer(h, tah, bere, a.beta, false);
                }
            } else {
                a.i = a.p.pocet_moznych_tahu;   // odřez
                a.beta = d.alfa;
            }
        }

        // --- dále podle potřeby načte druhý killer, popř. zbytek tahů ---
        if (a.ktery_nacten == 2 && a.i < a.p.pocet_moznych_tahu) {
            generuj_zbyle(h);
            if (h + 1 < zakladni_hloubka) serad_tahy(a.p, h == 0);
            a.ktery_nacten = 0;
            a.i = -1;
        }
        if (a.ktery_nacten == 1 && a.i < a.p.pocet_moznych_tahu) {
            a.p.pocet_moznych_tahu = 0;
            if (nast.puvodni_chyba_killeru) nacti_zdroj_killeru(h);
            if (a.zdroj_pocet == 2 && killer_pouzitelny(h, 1)) {
                a.p.tahy[0] = a.zdroj_tah[1];
                a.p.pocet_moznych_tahu = 1;
            }
            if (a.p.pocet_moznych_tahu == 0) {
                generuj_zbyle(h);
                if (h + 1 < zakladni_hloubka) serad_tahy(a.p, h == 0);
                a.ktery_nacten = 0;
            } else {
                a.ktery_nacten = 2;
            }
            a.i = -1;
        }

        ++a.i;
    }

    // --- otestuje, zda na větvi není mat nebo pat ---
    // ODCHYLKA: originál se ptal na pozice[h+1].na_tahu_je, což je při nula
    // prohledaných tazích zbytek po jiné větvi. Tady je to spočítané.
    const Strana utocnik = oponent(a.p.na_tahu_je);
    const Bod kral = (a.p.na_tahu_je == BILY) ? a.p.pozice_bileho_krale
                                              : a.p.pozice_cerneho_krale;
    if (licha) {
        if (a.beta > hodnota_figur[KRAL] - 100) {
            if (otestuj_zda_neni_napadene(a.p, kral.x, kral.y, utocnik))
                a.beta /= 100.0;      // soupeř je matový
            else
                a.beta = -10.0;       // pat soupeře - nechceme
            ++matu;
        }
    } else {
        if (a.alfa < -(hodnota_figur[KRAL] - 100)) {
            if (otestuj_zda_neni_napadene(a.p, kral.x, kral.y, utocnik))
                a.alfa /= 100.0;      // dostávám mat
            else
                a.alfa = 10.0;        // pat mě zachraňuje
            ++matu;
        }
    }
}

VysledekHledani Hledac::spust(const Pozice& start) {
    const auto t0 = std::chrono::steady_clock::now();

    VysledekHledani v;

    Uroven& minus = hladiny[0];       // pozice[-1]
    minus.pocet_nejlepsich = 0;
    minus.alfa = -nekonecno;
    minus.beta =  nekonecno;
    minus.i = 0;

    Uroven& koren = U(0);
    koren.p = start;
    koren.p.pocet_moznych_tahu = 0;
    koren.hodnota = 0.0;
    koren.p.co_schramstne = 0.0;
    koren.max_hloubka = zakladni_hloubka;

    if (nast.cas_ms > 0) {
        ma_limit = true;
        konec_casu = t0 + std::chrono::milliseconds(nast.cas_ms);
    }

    // pozice krále se v originále před hledáním hledala skenem
    for (int y = 1; y <= 8; ++y)
        for (int x = 1; x <= 8; ++x)
            if (koren.p.sachovnice[x][y].figura == KRAL) {
                if (koren.p.sachovnice[x][y].strana == BILY)
                    koren.p.pozice_bileho_krale = {x, y};
                else
                    koren.p.pozice_cerneho_krale = {x, y};
            }

    // Je-li legální tah jediný, originál ho zahrál bez hledání.
    Pozice zkouska = start;
    zjisti_vsechny_legalni_tahy(zkouska);
    vymazej_tahy_kdy_je_v_sachu(zkouska);
    v.stav = zkouska.stav;
    if (zkouska.pocet_moznych_tahu == 0) {
        v.sekundy = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - t0).count();
        return v;
    }
    if (zkouska.pocet_moznych_tahu == 1) {
        v.nejlepsi_tah = zkouska.tahy[0];
        v.tah_nalezen = true;
        v.sekundy = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - t0).count();
        return v;
    }

    vnor_se(0);

    v.stav = koren.p.stav;
    if (koren.nakonec_zahraj >= 0 &&
        koren.nakonec_zahraj < koren.p.pocet_moznych_tahu) {
        v.nejlepsi_tah = koren.p.tahy[koren.nakonec_zahraj];
        v.tah_nalezen = true;
    }
    v.hodnota = koren.alfa;
    v.preruseno = zastaveno;
    v.listu = listu;
    v.matu  = matu;
    v.sekundy = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
    return v;
}

}  // namespace

// ---------------------------------------------------------------------------
//  serad_tahy  (MVV/LVA)
// ---------------------------------------------------------------------------
void serad_tahy(Pozice& p, bool koren) {
    const int n = p.pocet_moznych_tahu;
    if (n <= 0) return;

    for (int i = 0; i < n; ++i) {
        Tah& t = p.tahy[i];
        if (!koren) t.trideni = 0.0;
        t.trideni += hodnota_figur[p.sachovnice[t.na_pole.x][t.na_pole.y].figura]
                   - hodnota_figur[p.sachovnice[t.z_pole.x][t.z_pole.y].figura]
                     / 100000000.0;
    }

    // ODCHYLKA: originál měl vlastní quicksort, který si jako odkládací
    // místo bral tah[100] - při více než 100 tazích by si přepsal data.
    std::stable_sort(p.tahy, p.tahy + n,
                     [](const Tah& x, const Tah& y) { return x.trideni > y.trideni; });
}

// ---------------------------------------------------------------------------
//  pridej_hodnotu_lepsim_tahum
// ---------------------------------------------------------------------------
void pridej_hodnotu_lepsim_tahum(Pozice& p) {
    const bool bily = (p.na_tahu_je == BILY);
    const PreferovanyTah* tab = bily ? lepsi_tahy_bileho : lepsi_tahy_cerneho;
    const int poc_lt = bily ? static_cast<int>(std::size(lepsi_tahy_bileho))
                            : static_cast<int>(std::size(lepsi_tahy_cerneho));

    for (int i = 0; i < p.pocet_moznych_tahu; ++i) {
        Tah& t = p.tahy[i];
        t.trideni = 0.0;
        t.lepsi_tah = 0.0;
        const Figura tazena = p.sachovnice[t.z_pole.x][t.z_pole.y].figura;

        for (int k = 0; k < poc_lt; ++k) {
            if (t.z_pole.x == tab[k].zX && t.z_pole.y == tab[k].zY &&
                t.na_pole.x == tab[k].naX && t.na_pole.y == tab[k].naY &&
                tazena == tab[k].figura) {
                t.trideni = tab[k].hodnota;
                t.lepsi_tah = t.trideni;
            }
        }

        if (tazena == PESEC) {
            const int postup = bily ? t.na_pole.y : (9 - t.na_pole.y);
            t.trideni += postup * prioritaC;
            t.lepsi_tah = t.trideni;
        }
    }

    // POZNÁMKA: originál tu ještě odečítal 50 za tahy, které si uživatel
    // ručně označil tlačítky (preference_clovekem). To je ladicí funkce GUI,
    // do enginu nepatří.
}

// jakýkoli legální tah - poslední záchrana, když se nestihlo vůbec nic
static bool prvni_legalni(const Pozice& start, Tah& ven) {
    Pozice q = start;
    zjisti_vsechny_legalni_tahy(q);
    vymazej_tahy_kdy_je_v_sachu(q);
    if (q.pocet_moznych_tahu == 0) return false;
    serad_tahy(q, false);
    ven = q.tahy[0];
    return true;
}

VysledekHledani hledej(const Pozice& start, const NastaveniHledani& n) {
    if (!n.postupne_prohlubovani) {
        Hledac h(n);
        VysledekHledani v = h.spust(start);
        v.dosazena_hloubka = v.preruseno ? 0 : n.max_hloubka;
        return v;
    }

    const auto t0 = std::chrono::steady_clock::now();
    const int rezerva_klidu = std::max(0, n.max_hledej_klid - n.max_hloubka);

    VysledekHledani nejlepsi;
    bool mam_dokoncene = false;

    for (int d = 1; d <= n.max_hloubka; ++d) {
        NastaveniHledani kolo = n;
        kolo.postupne_prohlubovani = false;
        kolo.max_hloubka = d;
        kolo.max_hledej_klid = d + rezerva_klidu;

        if (n.cas_ms > 0) {
            const double uply = std::chrono::duration<double, std::milli>(
                                    std::chrono::steady_clock::now() - t0).count();
            const double zbyva = n.cas_ms - uply;
            // Nové kolo je zhruba pětkrát dražší než minulé. Když nezbývá
            // aspoň polovina rozpočtu, nemá smysl ho začínat.
            if (mam_dokoncene && zbyva < n.cas_ms * 0.5) break;
            if (zbyva <= 1.0) break;
            kolo.cas_ms = static_cast<int>(zbyva);
        }

        Hledac h(kolo);
        VysledekHledani v = h.spust(start);

        if (v.preruseno) {
            // nedokončené kolo se zahazuje, platí poslední úplné
            if (!mam_dokoncene && v.tah_nalezen) {
                nejlepsi = v;
                mam_dokoncene = true;   // aspoň něco použitelného
            }
            nejlepsi.preruseno = true;
            break;
        }

        v.dosazena_hloubka = d;
        nejlepsi = v;
        mam_dokoncene = true;

        if (n.hlaseni) {
            PrubeznyVysledek p;
            p.hloubka = d;
            p.hodnota = v.hodnota;
            p.listu = v.listu;
            p.nejlepsi_tah = v.nejlepsi_tah;
            p.tah_nalezen = v.tah_nalezen;
            n.hlaseni(n.hlaseni_kontext, p);
        }

        // Nalezený mat nebo pat dál prohlubovat netřeba.
        if (!v.tah_nalezen) break;
    }

    if (!nejlepsi.tah_nalezen) {
        Tah nouzovy;
        if (prvni_legalni(start, nouzovy)) {
            nejlepsi.nejlepsi_tah = nouzovy;
            nejlepsi.tah_nalezen = true;
        }
    }

    nejlepsi.sekundy = std::chrono::duration<double>(
                           std::chrono::steady_clock::now() - t0).count();
    return nejlepsi;
}

}  // namespace lightred
