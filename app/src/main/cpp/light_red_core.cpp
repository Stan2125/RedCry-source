// ---------------------------------------------------------------------------
//  Light Red - jádro šachového enginu (implementace)
//  Přepis z Light_Red_211_2_f.pas. Odchylky od originálu jsou označeny
//  komentářem "ODCHYLKA" a popsané v POZNAMKY-LIGHTRED.md.
// ---------------------------------------------------------------------------

#include "light_red_core.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

namespace lightred {

// hodnota_figur v Pascalu: (dama, vez, strelec, jezdec, pesec, kral)
// tady indexováno přímo enumem Figura
const double hodnota_figur[7] = {
    0.0,      // ZADNA
    1.0,      // PESEC
    3.5,      // JEZDEC
    3.5,      // STRELEC
    5.5,      // VEZ
    10.0,     // DAMA
    10000.0   // KRAL
};

const double hodnota_rosady = 0.02;

namespace {

// vektory_damy - seřazené do opačných dvojic, jako v originále
const Bod vektory_damy[8] = {
    {1, 0}, {-1, 0}, {1, 1}, {-1, -1}, {0, 1}, {0, -1}, {-1, 1}, {1, -1}
};
const Bod vektory_veze[4]    = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
const Bod vektory_strelce[4] = {{1, 1}, {-1, -1}, {-1, 1}, {1, -1}};
const Bod pohyb_jezdce[8]    = {
    {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
};
const Figura mozne_promeny_pesce[4] = {DAMA, VEZ, STRELEC, JEZDEC};

inline bool na_desce(int x, int y) { return x >= 1 && x <= 8 && y >= 1 && y <= 8; }

// Vyprázdní pozici. Záměrně po jednotlivých položkách: "p = Pozice{}" tvoří
// dočasný objekt o velikosti celého pole tahů a GCC 13 na tom padá
// (internal compiler error v gimple_add_tmp_var).
void resetuj(Pozice& p) {
    for (int x = 0; x <= 8; ++x)
        for (int y = 0; y <= 8; ++y)
            p.sachovnice[x][y] = Policko{NIKDO, ZADNA};
    p.na_tahu_je = BILY;
    p.r_Bm = p.r_Bv = p.r_Cm = p.r_Cv = false;
    p.en_passant = Bod{0, 0};
    p.pocet_moznych_tahu = 0;
    p.stav = NORMALNI;
    p.co_schramstne = 0.0;
    p.pultahy_bez_zmeny = 0;
    p.cislo_tahu = 1;
    p.pozice_bileho_krale  = Bod{5, 1};
    p.pozice_cerneho_krale = Bod{5, 8};
}

inline Strana oponent(Strana s) { return s == BILY ? CERNY : BILY; }

// přidání tahu do seznamu, s kontrolou přetečení
inline Tah& novy_tah(Pozice& p) {
    if (p.pocet_moznych_tahu >= MAX_TAHU - 1) {
        // v originále by se tiše přepisovala paměť za polem tah[1..100]
        return p.tahy[MAX_TAHU - 1];
    }
    Tah& t = p.tahy[p.pocet_moznych_tahu++];
    t.promen_pesce = ZADNA;
    t.trideni = 0.0;
    t.lepsi_tah = 0.0;
    return t;
}

inline void uloz_tah(Pozice& p, int zx, int zy, int nx, int ny,
                     Figura promena = ZADNA) {
    Tah& t = novy_tah(p);
    t.z_pole  = {zx, zy};
    t.na_pole = {nx, ny};
    // ODCHYLKA: originál promen_pesce u nepromenných tahů vůbec nenastavoval
    // a v poli zůstávala hodnota z předchozí generace. Tady je vždy ZADNA.
    t.promen_pesce = promena;
}

}  // namespace

// ---------------------------------------------------------------------------
//  zjisti_jen_napadeni
//  Napadá figura na (figX,figY) pole (cilX,cilY)?
//  V Pascalu byly parametry (zjn_x1,zjn_y1) = cíl, (zjn_x2,zjn_y2) = figura.
// ---------------------------------------------------------------------------
bool zjisti_jen_napadeni(const Pozice& p, int cilX, int cilY, int figX, int figY) {
    const bool sam_na_sebe = (cilX == figX && cilY == figY);

    const Policko& cil = p.sachovnice[cilX][cilY];
    const Policko& fig = p.sachovnice[figX][figY];

    // --- aby nebral vlastní figuru ---
    if (cil.strana == fig.strana && !sam_na_sebe) return false;

    bool je_legalni = true;

    switch (fig.figura) {

    // --- pohyb střelce ---
    case STRELEC: {
        if (std::abs(cilX - figX) != std::abs(cilY - figY)) {
            je_legalni = false;
            break;   // ODCHYLKA: originál i po tomhle selhání pokračoval do
                     // skenovací smyčky s nesmyslným vektorem a uměl číst
                     // MIMO pole (např. střelec a8 dotazovaný na e8).
                     // Viz POZNAMKY-LIGHTRED.md, bod 1.
        }
        if (std::abs(cilX - figX) > 1) {
            Bod vektor{cilX < figX ? -1 : 1, cilY < figY ? -1 : 1};
            Bod skenuj{figX, figY};
            do {
                skenuj.x += vektor.x;
                skenuj.y += vektor.y;
                if (p.sachovnice[skenuj.x][skenuj.y].figura != ZADNA)
                    je_legalni = false;
            } while (std::abs(skenuj.x - cilX) > 1 && je_legalni);
        }
        break;
    }

    // --- pohyb jezdce ---
    case JEZDEC: {
        if (!sam_na_sebe) {
            const int dx = std::abs(cilX - figX);
            const int dy = std::abs(cilY - figY);
            if (!((dx == 2 && dy == 1) || (dx == 1 && dy == 2)))
                je_legalni = false;
        }
        break;
    }

    // --- pohyb věže ---
    case VEZ: {
        if (sam_na_sebe) break;
        if (cilX != figX && cilY != figY) { je_legalni = false; break; }

        Bod vektor{0, 0};
        if (cilX == figX) {
            vektor.x = 0;
            vektor.y = (cilY < figY) ? -1 : 1;
            if (std::abs(cilY - figY) <= 1) vektor.y = 0;
        } else {
            vektor.y = 0;
            vektor.x = (cilX < figX) ? -1 : 1;
            if (std::abs(cilX - figX) <= 1) vektor.x = 0;
        }
        if (vektor.x != vektor.y) {
            Bod skenuj{figX, figY};
            do {
                skenuj.x += vektor.x;
                skenuj.y += vektor.y;
                if (p.sachovnice[skenuj.x][skenuj.y].figura != ZADNA)
                    je_legalni = false;
            } while (std::abs(skenuj.x - cilX) != 1 &&
                     std::abs(skenuj.y - cilY) != 1 && je_legalni);
        }
        break;
    }

    // --- pohyb dámy ---
    case DAMA: {
        if (sam_na_sebe) break;
        const bool primo = (cilX == figX || cilY == figY);
        const bool sikmo = (std::abs(cilX - figX) == std::abs(cilY - figY));
        if (!primo && !sikmo) { je_legalni = false; break; }

        if (primo) {
            // jako věž
            Bod vektor{0, 0};
            if (cilX == figX) {
                vektor.x = 0;
                vektor.y = (cilY < figY) ? -1 : 1;
                if (std::abs(cilY - figY) <= 1) vektor.y = 0;
            } else {
                vektor.y = 0;
                vektor.x = (cilX < figX) ? -1 : 1;
                if (std::abs(cilX - figX) <= 1) vektor.x = 0;
            }
            if (vektor.x != vektor.y) {
                Bod skenuj{figX, figY};
                do {
                    skenuj.x += vektor.x;
                    skenuj.y += vektor.y;
                    if (p.sachovnice[skenuj.x][skenuj.y].figura != ZADNA)
                        je_legalni = false;
                } while (std::abs(skenuj.x - cilX) != 1 &&
                         std::abs(skenuj.y - cilY) != 1 && je_legalni);
            }
        } else {
            // jako střelec
            if (std::abs(cilX - figX) > 1) {
                Bod vektor{cilX < figX ? -1 : 1, cilY < figY ? -1 : 1};
                Bod skenuj{figX, figY};
                do {
                    skenuj.x += vektor.x;
                    skenuj.y += vektor.y;
                    if (p.sachovnice[skenuj.x][skenuj.y].figura != ZADNA)
                        je_legalni = false;
                } while (std::abs(skenuj.x - cilX) > 1 && je_legalni);
            }
        }
        break;
    }

    // --- pohyb krále ---
    case KRAL: {
        if (!sam_na_sebe) {
            if (std::abs(cilX - figX) > 1 || std::abs(cilY - figY) > 1)
                je_legalni = false;
        }
        break;
    }

    // --- napadání pěšcem (jen šikmo, postup vpřed není napadení) ---
    case PESEC: {
        if (!sam_na_sebe) {
            const bool bere_bily  = (fig.strana == BILY)  &&
                                    (std::abs(cilX - figX) == 1) && (cilY - figY == 1);
            const bool bere_cerny = (fig.strana == CERNY) &&
                                    (std::abs(cilX - figX) == 1) && (cilY - figY == -1);
            if (!bere_bily && !bere_cerny) je_legalni = false;
        }
        break;
    }

    case ZADNA:
    default:
        break;
    }

    return je_legalni;
}

// ---------------------------------------------------------------------------
//  otestuj_zda_neni_napadene
// ---------------------------------------------------------------------------
bool otestuj_zda_neni_napadene(const Pozice& p, int x, int y, Strana utocnik) {
    for (int fy = 1; fy <= 8; ++fy)
        for (int fx = 1; fx <= 8; ++fx)
            if (p.sachovnice[fx][fy].strana == utocnik)
                if (zjisti_jen_napadeni(p, x, y, fx, fy)) return true;
    return false;
}

// ---------------------------------------------------------------------------
//  zjisti_vsechny_ZBYLE_legalni_tahy  (pseudolegální generátor)
// ---------------------------------------------------------------------------
void zjisti_vsechny_legalni_tahy(Pozice& p) {
    const Strana ja  = p.na_tahu_je;
    const Strana on  = oponent(ja);

    p.pocet_moznych_tahu = 0;

    for (int pomY = 1; pomY <= 8; ++pomY) {
        for (int pomX = 1; pomX <= 8; ++pomX) {

            if (p.sachovnice[pomX][pomY].strana != ja) continue;

            switch (p.sachovnice[pomX][pomY].figura) {

            case DAMA:
            case VEZ:
            case STRELEC: {
                const Bod* vektory;
                int pocet;
                switch (p.sachovnice[pomX][pomY].figura) {
                    case DAMA:    vektory = vektory_damy;    pocet = 8; break;
                    case VEZ:     vektory = vektory_veze;    pocet = 4; break;
                    default:      vektory = vektory_strelce; pocet = 4; break;
                }
                for (int i = 0; i < pocet; ++i) {
                    Bod rel{pomX + vektory[i].x, pomY + vektory[i].y};
                    bool skonci_linii = false;
                    while (na_desce(rel.x, rel.y) &&
                           p.sachovnice[rel.x][rel.y].strana != ja &&
                           !skonci_linii) {
                        uloz_tah(p, pomX, pomY, rel.x, rel.y);
                        if (p.sachovnice[rel.x][rel.y].strana == on)
                            skonci_linii = true;
                        rel.x += vektory[i].x;
                        rel.y += vektory[i].y;
                    }
                }
                break;
            }

            case JEZDEC: {
                for (int i = 0; i < 8; ++i) {
                    const Bod rel{pomX + pohyb_jezdce[i].x, pomY + pohyb_jezdce[i].y};
                    if (na_desce(rel.x, rel.y) &&
                        p.sachovnice[rel.x][rel.y].strana != ja)
                        uloz_tah(p, pomX, pomY, rel.x, rel.y);
                }
                break;
            }

            case PESEC: {
                if (ja == BILY) {
                    // --- postup vpřed ---
                    if (pomY < 8 && p.sachovnice[pomX][pomY + 1].strana == NIKDO) {
                        if (pomY == 7) {
                            for (int i = 0; i < 4; ++i)
                                uloz_tah(p, pomX, pomY, pomX, pomY + 1,
                                         mozne_promeny_pesce[i]);
                        } else {
                            uloz_tah(p, pomX, pomY, pomX, pomY + 1);
                        }
                        // --- první o dvě pole ---
                        if (pomY == 2 && p.sachovnice[pomX][pomY + 2].strana == NIKDO)
                            uloz_tah(p, pomX, pomY, pomX, pomY + 2);
                    }
                    // --- braní doprava / doleva ---
                    for (int dx = -1; dx <= 1; dx += 2) {
                        const int nx = pomX + dx;
                        if (pomY < 8 && nx >= 1 && nx <= 8 &&
                            p.sachovnice[nx][pomY + 1].strana == CERNY) {
                            if (pomY == 7) {
                                for (int i = 0; i < 4; ++i)
                                    uloz_tah(p, pomX, pomY, nx, pomY + 1,
                                             mozne_promeny_pesce[i]);
                            } else {
                                uloz_tah(p, pomX, pomY, nx, pomY + 1);
                            }
                        }
                    }
                    // --- braní en passant ---
                    if (p.en_passant.y == pomY + 1 &&
                        (p.en_passant.x == pomX + 1 || p.en_passant.x == pomX - 1))
                        uloz_tah(p, pomX, pomY, p.en_passant.x, p.en_passant.y);

                } else {
                    // --- postup vpřed ---
                    if (pomY > 1 && p.sachovnice[pomX][pomY - 1].strana == NIKDO) {
                        if (pomY == 2) {
                            for (int i = 0; i < 4; ++i)
                                uloz_tah(p, pomX, pomY, pomX, pomY - 1,
                                         mozne_promeny_pesce[i]);
                        } else {
                            uloz_tah(p, pomX, pomY, pomX, pomY - 1);
                        }
                        if (pomY == 7 && p.sachovnice[pomX][pomY - 2].strana == NIKDO)
                            uloz_tah(p, pomX, pomY, pomX, pomY - 2);
                    }
                    for (int dx = -1; dx <= 1; dx += 2) {
                        const int nx = pomX + dx;
                        if (pomY > 1 && nx >= 1 && nx <= 8 &&
                            p.sachovnice[nx][pomY - 1].strana == BILY) {
                            if (pomY == 2) {
                                for (int i = 0; i < 4; ++i)
                                    uloz_tah(p, pomX, pomY, nx, pomY - 1,
                                             mozne_promeny_pesce[i]);
                            } else {
                                uloz_tah(p, pomX, pomY, nx, pomY - 1);
                            }
                        }
                    }
                    if (p.en_passant.y == pomY - 1 &&
                        (p.en_passant.x == pomX + 1 || p.en_passant.x == pomX - 1))
                        uloz_tah(p, pomX, pomY, p.en_passant.x, p.en_passant.y);
                }
                break;
            }

            case KRAL: {
                for (int i = 0; i < 8; ++i) {
                    const Bod rel{pomX + vektory_damy[i].x, pomY + vektory_damy[i].y};
                    if (na_desce(rel.x, rel.y) &&
                        p.sachovnice[rel.x][rel.y].strana != ja)
                        uloz_tah(p, pomX, pomY, rel.x, rel.y);
                }

                // --- rošáda ---
                const int r = (ja == BILY) ? 1 : 8;
                const bool mala  = (ja == BILY) ? p.r_Bm : p.r_Cm;
                const bool velka = (ja == BILY) ? p.r_Bv : p.r_Cv;

                if (mala &&
                    p.sachovnice[6][r].figura == ZADNA &&
                    p.sachovnice[7][r].figura == ZADNA &&
                    !otestuj_zda_neni_napadene(p, 5, r, on) &&
                    !otestuj_zda_neni_napadene(p, 6, r, on) &&
                    !otestuj_zda_neni_napadene(p, 7, r, on))
                    uloz_tah(p, 5, r, 7, r);

                if (velka &&
                    p.sachovnice[4][r].figura == ZADNA &&
                    p.sachovnice[3][r].figura == ZADNA &&
                    p.sachovnice[2][r].figura == ZADNA &&
                    !otestuj_zda_neni_napadene(p, 5, r, on) &&
                    !otestuj_zda_neni_napadene(p, 4, r, on) &&
                    !otestuj_zda_neni_napadene(p, 3, r, on))
                    uloz_tah(p, 5, r, 3, r);
                break;
            }

            case ZADNA:
            default:
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
//  zahraj_tah
// ---------------------------------------------------------------------------
void zahraj_tah(Pozice& p, const Tah& tah) {
    const Bod z  = tah.z_pole;
    const Bod na = tah.na_pole;

    const Figura tazena = p.sachovnice[z.x][z.y].figura;
    const Strana tahnul = p.sachovnice[z.x][z.y].strana;

    // ODCHYLKA: originál u sebrání krále násobil hodnotu zbývající hloubkou
    // (hodnota_figur[6] * (max_hledej_klid - hloubka + 1)). To je záležitost
    // hledání, ne desky - patří do vrstvy vyhledávání, ne sem.
    p.co_schramstne = hodnota_figur[p.sachovnice[na.x][na.y].figura];

    // --- braní en passant ---
    if (na.x == p.en_passant.x && na.y == p.en_passant.y && tazena == PESEC) {
        p.co_schramstne = hodnota_figur[PESEC];
        p.sachovnice[na.x][z.y].figura = ZADNA;
        p.sachovnice[na.x][z.y].strana = NIKDO;
    }

    // --- proměna pěšce ---
    if ((na.y == 1 || na.y == 8) && tazena == PESEC) {
        // ODCHYLKA: originál neměl větev else; při neplatné hodnotě
        // promen_pesce nechal na cílovém poli původní figuru a jen jí přepsal
        // barvu. Tady je záloha na dámu.
        Figura nova = tah.promen_pesce;
        if (nova != DAMA && nova != VEZ && nova != STRELEC && nova != JEZDEC)
            nova = DAMA;
        p.sachovnice[na.x][na.y].figura = nova;
        p.sachovnice[na.x][na.y].strana = tahnul;
        p.co_schramstne += hodnota_figur[nova] - hodnota_figur[PESEC];
    } else {
        // --- normální přemístění figury ---
        p.sachovnice[na.x][na.y] = p.sachovnice[z.x][z.y];
    }

    p.sachovnice[z.x][z.y].figura = ZADNA;
    p.sachovnice[z.x][z.y].strana = NIKDO;

    // --- znemožni nelegální rošádu (z pole i na pole) ---
    for (const Bod& b : {z, na}) {
        if (b.y == 1) {
            if (b.x == 1) p.r_Bv = false;
            if (b.x == 5) { p.r_Bv = false; p.r_Bm = false; }
            if (b.x == 8) p.r_Bm = false;
        }
        if (b.y == 8) {
            if (b.x == 1) p.r_Cv = false;
            if (b.x == 5) { p.r_Cv = false; p.r_Cm = false; }
            if (b.x == 8) p.r_Cm = false;
        }
    }

    // --- rošáda -> pohni ještě věží ---
    if (p.sachovnice[na.x][na.y].figura == KRAL && std::abs(z.x - na.x) == 2) {
        if (na.x == 7) {
            p.sachovnice[6][z.y] = p.sachovnice[8][z.y];
            p.sachovnice[8][z.y].figura = ZADNA;
            p.sachovnice[8][z.y].strana = NIKDO;
        } else {
            p.sachovnice[4][z.y] = p.sachovnice[1][z.y];
            p.sachovnice[1][z.y].figura = ZADNA;
            p.sachovnice[1][z.y].strana = NIKDO;
        }
        // POZOR: v originále se tady bonus za malou rošádu nejdřív spočítal
        // s násobkem 1.1 a hned nato ho bezpodmínečně přepsala tahle řádka.
        // Ten výpočet byl tedy mrtvý kód. Zachováno tak, jak to bylo.
        p.co_schramstne = hodnota_rosady;
    }

    // --- zapamatuj si, co lze en passant ---
    if (p.sachovnice[na.x][na.y].figura == PESEC && std::abs(na.y - z.y) == 2) {
        p.en_passant.x = na.x;
        p.en_passant.y = (z.y + na.y) / 2;
    } else {
        p.en_passant = {0, 0};
    }

    // --- zapamatuj si pozici krále ---
    if (p.sachovnice[na.x][na.y].figura == KRAL) {
        if (p.sachovnice[na.x][na.y].strana == BILY) p.pozice_bileho_krale = na;
        else                                          p.pozice_cerneho_krale = na;
    }
}

// ---------------------------------------------------------------------------
//  vymazej_tahy_kdy_je_v_sachu
// ---------------------------------------------------------------------------
void vymazej_tahy_kdy_je_v_sachu(Pozice& p) {
    const Strana ja = p.na_tahu_je;
    const Strana on = oponent(ja);

    int pomT = 0;
    while (pomT < p.pocet_moznych_tahu) {
        // ODCHYLKA: originál používal jako odkladiště pozice[konst_MH],
        // tedy prvek pole indexovaný 20. Při hloubce 20 by si to přepsalo
        // vlastní data. Tady je to lokální kopie.
        Pozice zkus = p;
        zahraj_tah(zkus, p.tahy[pomT]);

        // najdi krále táhnoucí strany
        Bod kral{0, 0};
        for (int y = 1; y <= 8 && kral.x == 0; ++y)
            for (int x = 1; x <= 8; ++x)
                if (zkus.sachovnice[x][y].figura == KRAL &&
                    zkus.sachovnice[x][y].strana == ja) { kral = {x, y}; break; }

        bool v_sachu = (kral.x == 0)   // král zmizel = tah ho nechal sebrat
                       ? true
                       : otestuj_zda_neni_napadene(zkus, kral.x, kral.y, on);

        if (v_sachu) {
            p.tahy[pomT] = p.tahy[p.pocet_moznych_tahu - 1];
            --p.pocet_moznych_tahu;
        } else {
            ++pomT;
        }
    }

    // --- mat, pat nebo normální stav ---
    if (p.pocet_moznych_tahu > 0) {
        p.stav = NORMALNI;
    } else {
        Bod kral{0, 0};
        for (int y = 1; y <= 8 && kral.x == 0; ++y)
            for (int x = 1; x <= 8; ++x)
                if (p.sachovnice[x][y].figura == KRAL &&
                    p.sachovnice[x][y].strana == ja) { kral = {x, y}; break; }
        const bool sach = (kral.x != 0) &&
                          otestuj_zda_neni_napadene(p, kral.x, kral.y, on);
        p.stav = sach ? MAT : PAT;
    }
}

// ---------------------------------------------------------------------------
//  nacti_uvodni_pozici
// ---------------------------------------------------------------------------
void nacti_uvodni_pozici(Pozice& p) {
    resetuj(p);

    for (int x = 1; x <= 8; ++x)
        for (int y = 3; y <= 6; ++y)
            p.sachovnice[x][y] = {NIKDO, ZADNA};

    for (int x = 1; x <= 8; ++x) {
        p.sachovnice[x][1].strana = BILY;
        p.sachovnice[x][2].strana = BILY;
        p.sachovnice[x][7].strana = CERNY;
        p.sachovnice[x][8].strana = CERNY;
        p.sachovnice[x][2].figura = PESEC;
        p.sachovnice[x][7].figura = PESEC;
    }

    const Figura rada[9] = {ZADNA, VEZ, JEZDEC, STRELEC, DAMA, KRAL, STRELEC, JEZDEC, VEZ};
    for (int x = 1; x <= 8; ++x) {
        p.sachovnice[x][1].figura = rada[x];
        p.sachovnice[x][8].figura = rada[x];
    }

    p.r_Bm = p.r_Bv = p.r_Cm = p.r_Cv = true;
    p.en_passant = {0, 0};
    p.na_tahu_je = BILY;
    p.pozice_bileho_krale  = {5, 1};
    p.pozice_cerneho_krale = {5, 8};
}

// ---------------------------------------------------------------------------
//  FEN  (v originále neexistuje, přidáno kvůli testům)
// ---------------------------------------------------------------------------
bool nacti_fen(Pozice& p, const std::string& fen) {
    resetuj(p);

    std::istringstream in(fen);
    std::string deska, kdo, rosady, ep;
    if (!(in >> deska >> kdo >> rosady >> ep)) return false;

    int x = 1, y = 8;
    for (char c : deska) {
        if (c == '/') { --y; x = 1; continue; }
        if (std::isdigit(static_cast<unsigned char>(c))) { x += c - '0'; continue; }
        if (x > 8 || y < 1) return false;
        const Strana s = std::isupper(static_cast<unsigned char>(c)) ? BILY : CERNY;
        Figura f;
        switch (std::tolower(static_cast<unsigned char>(c))) {
            case 'p': f = PESEC;   break;
            case 'n': f = JEZDEC;  break;
            case 'b': f = STRELEC; break;
            case 'r': f = VEZ;     break;
            case 'q': f = DAMA;    break;
            case 'k': f = KRAL;    break;
            default:  return false;
        }
        p.sachovnice[x][y] = {s, f};
        if (f == KRAL) {
            if (s == BILY) p.pozice_bileho_krale = {x, y};
            else            p.pozice_cerneho_krale = {x, y};
        }
        ++x;
    }

    p.na_tahu_je = (kdo == "w") ? BILY : CERNY;
    p.r_Bm = rosady.find('K') != std::string::npos;
    p.r_Bv = rosady.find('Q') != std::string::npos;
    p.r_Cm = rosady.find('k') != std::string::npos;
    p.r_Cv = rosady.find('q') != std::string::npos;

    if (ep == "-" || ep.size() < 2) p.en_passant = {0, 0};
    else p.en_passant = {ep[0] - 'a' + 1, ep[1] - '0'};

    // pátý a šestý údaj jsou nepovinné
    int pultahy = 0, tah = 1;
    if (in >> pultahy) p.pultahy_bez_zmeny = pultahy;
    if (in >> tah)     p.cislo_tahu = tah < 1 ? 1 : tah;

    return true;
}

std::string zapis_fen(const Pozice& p) {
    std::string s;
    for (int y = 8; y >= 1; --y) {
        int prazdna = 0;
        for (int x = 1; x <= 8; ++x) {
            const Policko& c = p.sachovnice[x][y];
            if (c.figura == ZADNA) { ++prazdna; continue; }
            if (prazdna) { s += char('0' + prazdna); prazdna = 0; }
            const char* znaky = " pnbrqk";
            char z = znaky[c.figura];
            s += (c.strana == BILY) ? char(std::toupper(z)) : z;
        }
        if (prazdna) s += char('0' + prazdna);
        if (y > 1) s += '/';
    }
    s += (p.na_tahu_je == BILY) ? " w " : " b ";
    std::string r;
    if (p.r_Bm) r += 'K';
    if (p.r_Bv) r += 'Q';
    if (p.r_Cm) r += 'k';
    if (p.r_Cv) r += 'q';
    s += r.empty() ? "-" : r;
    s += ' ';
    if (p.en_passant.x == 0) s += '-';
    else { s += char('a' + p.en_passant.x - 1); s += char('0' + p.en_passant.y); }
    s += ' ';
    s += std::to_string(p.pultahy_bez_zmeny);
    s += ' ';
    s += std::to_string(p.cislo_tahu);
    return s;
}

std::string tah_na_text(const Tah& t) {
    std::string s;
    s += char('a' + t.z_pole.x - 1);
    s += char('0' + t.z_pole.y);
    s += char('a' + t.na_pole.x - 1);
    s += char('0' + t.na_pole.y);
    switch (t.promen_pesce) {
        case DAMA:    s += 'q'; break;
        case VEZ:     s += 'r'; break;
        case STRELEC: s += 'b'; break;
        case JEZDEC:  s += 'n'; break;
        default: break;
    }
    return s;
}

}  // namespace lightred
