// ---------------------------------------------------------------------------
//  RedMAX - jádro šachového enginu (implementace)
//
//  Přepis z RedMAX_408_f.pas. Čísla řádků v komentářích odkazují do originálu.
//  Žádné I/O, žádná vlákna, žádný globální stav - viz red_max_core.h.
// ---------------------------------------------------------------------------

#include "red_max_core.h"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace redmax {

// --- tabulky (ř. 65-95) -----------------------------------------------------

const int Offset[poc_smeru_tahu_vsech_kamenu + 1] = {
    -9, -11, 9, 11,                       // diagonální kroky
    -1, 10, 1, -10,                       // přímé kroky
    19, 21, 12, -8, -19, -21, -12, 8      // skoky jezdce
};

const FigOffsetTyp FigOffset[poc_typu_figur + 1] = {
    {  0,  0, false },   // prázdné pole
    { -1, -1, false },   // pěšec - do smyčky po směrech se nikdy nechytí
    {  4,  7, true  },   // věž
    {  0,  3, true  },   // střelec
    {  8, 15, false },   // jezdec
    {  0,  7, true  },   // dáma
    {  0,  7, false }    // král
};

const int FigMaterial[poc_typu_figur + 1] = {
    0, hodnota_Pesce, hodnota_Veze, hodnota_Strelce,
    hodnota_Jezdce, hodnota_Damy, hodnota_Krale
};

const int zakladni_postaveni[velikost_sachovnice_10x12 + 1] = {
    100,100,100,100,100,100,100,100,100,100,
    100,100,100,100,100,100,100,100,100,100,
    100,  2,  4,  3,  5,  6,  3,  4,  2,100,
    100,  1,  1,  1,  1,  1,  1,  1,  1,100,
    100,  0,  0,  0,  0,  0,  0,  0,  0,100,
    100,  0,  0,  0,  0,  0,  0,  0,  0,100,
    100,  0,  0,  0,  0,  0,  0,  0,  0,100,
    100,  0,  0,  0,  0,  0,  0,  0,  0,100,
    100, -1, -1, -1, -1, -1, -1, -1, -1,100,
    100, -2, -4, -3, -5, -6, -3, -4, -2,100,
    100,100,100,100,100,100,100,100,100,100,
    100,100,100,100,100,100,100,100,100,100
};

namespace {

// sloupec 1..8 (a..h), řada 1..8
inline int pole_z_souradnic(int sloupec, int rada) { return 21 + (rada - 1) * 10 + (sloupec - 1); }
inline int sloupec_pole(int pole) { return pole % 10; }
inline int rada_pole(int pole)    { return pole / 10 - 1; }

// Originál při přetečení zásobníku volal MessageDlg a Halt(1) (ř. 773).
// Ukončit proces v knihovně nelze: v ladicím buildu spadne assert, v ostrém
// se generování tiše zastaví.
bool je_misto(Pozice& p) {
    if (p.Index < velikost_zasobniku_tahu) return true;
    p.zasobnik_pretekl = true;
    assert(false && "RedMAX: zasobnik tahu pretekl");
    return false;
}

// Bonus za hlavní variantu a killery (ř. 746-752 a 794-799). Tři větve se
// vzájemně vylučují, přesně jako Pascalí if-else-if.
int bonus_varianty_a_killeru(const PoradiTahu& poradi, int z_pole, int na_pole) {
    if (poradi.pv_z_pole == z_pole && poradi.pv_na_pole == na_pole) return bonus_hlavni_varianty;
    if (poradi.k1_z_pole == z_pole && poradi.k1_na_pole == na_pole) return bonus_1_killer_tahu;
    if (poradi.k2_z_pole == z_pole && poradi.k2_na_pole == na_pole) return bonus_2_killer_tahu;
    return 0;
}

// Bonus za tah z hash tabulky (ř. 754-761 a 801-808). Přičítá se navíc,
// nevylučuje se s předchozím.
int bonus_hash_tahu(const PoradiTahu& poradi, int z_pole, int na_pole) {
    if (poradi.hash_platny &&
        poradi.hash_z_pole == z_pole && poradi.hash_na_pole == na_pole) {
        return bonus_hash_nej_tahu;
    }
    return 0;
}

// uloz_tah (ř. 734-777). Vrací false, když se tah neuložil - volající pak
// nesmí sáhnout na Zasobnik_tahu[Index - 1].
bool uloz_tah(Pozice& p, int z_pole, int na_pole) {
    if (!je_misto(p)) return false;

    typ_tahu& t = p.Zasobnik_tahu[p.Index];
    t.z_pole           = z_pole;
    t.na_pole          = na_pole;
    t.brana_figura     = prazdne_pole;
    t.premenena_figura = prazdne_pole;
    t.druh_rosady      = zadna_rosada;
    t.ep_cil           = nelegalni;
    t.ep_brane_pole    = nelegalni;
    t.druh             = Druh::Tichy;

    // Pořadí tahů (ř. 745-761). Tichý tah bez bonusu má hodnotu 0.
    t.hodnota = bonus_varianty_a_killeru(p.poradi, z_pole, na_pole)
              + bonus_hash_tahu(p.poradi, z_pole, na_pole);

    ++p.Index;
    return true;
}

// uloz_utocny_tah (ř. 779-819)
bool uloz_utocny_tah(Pozice& p, int z_pole, int na_pole) {
    // Braní krále se negeneruje (ř. 781). Legalita se netestuje, pozná se až
    // podle skóre - proto tahle jediná pojistka.
    if (p.sachovnice[na_pole] == b_Kral || p.sachovnice[na_pole] == c_Kral) return false;
    if (!je_misto(p)) return false;

    typ_tahu& t = p.Zasobnik_tahu[p.Index];
    t.z_pole       = z_pole;
    t.na_pole      = na_pole;
    t.brana_figura = std::abs(p.sachovnice[na_pole]);

    // MVV - LVA/8, celočíselné dělení jako Pascalí div (ř. 786)
    t.hodnota = FigMaterial[std::abs(p.sachovnice[na_pole])]
              - (FigMaterial[std::abs(p.sachovnice[z_pole])] / 8);

    // braní na poli, kam se bralo v předchozím půltahu (ř. 788-792)
    if (p.hloubka > 0 && na_pole == p.cilove_pole[p.hloubka - 1]) {
        t.hodnota += 300;
    }
    // hlavní varianta, killery (ř. 794-799) a hash tah (ř. 801-808)
    t.hodnota += bonus_varianty_a_killeru(p.poradi, z_pole, na_pole)
               + bonus_hash_tahu(p.poradi, z_pole, na_pole);

    t.premenena_figura = prazdne_pole;
    t.druh_rosady      = zadna_rosada;
    t.ep_cil           = nelegalni;
    t.ep_brane_pole    = nelegalni;
    t.druh             = Druh::Brani;

    ++p.Index;
    return true;
}

// uloz_en_passant_tah (ř. 821-838)
bool uloz_en_passant_tah(Pozice& p, int z_pole, int na_pole, int ep) {
    if (p.sachovnice[na_pole] == b_Kral || p.sachovnice[na_pole] == c_Kral) return false;
    if (!je_misto(p)) return false;

    typ_tahu& t = p.Zasobnik_tahu[p.Index];
    t.z_pole           = z_pole;
    t.na_pole          = na_pole;
    t.brana_figura     = b_Pesec;       // vždy kladný typ, jako ABS jinde
    t.premenena_figura = prazdne_pole;
    t.druh_rosady      = zadna_rosada;
    t.ep_cil           = nelegalni;
    t.ep_brane_pole    = ep;            // pole braného pěšce
    t.druh             = Druh::EnPassant;
    t.hodnota          = hodnota_Pesce; // ř. 830

    ++p.Index;
    return true;
}

// zjisti_promeny_pesce (ř. 840-855). Pořadí dáma, jezdec, střelec, věž.
void zjisti_promeny_pesce(Pozice& p, int z_pole, int na_pole) {
    if (p.sachovnice[na_pole] == prazdne_pole) {
        for (int i = b_Dama; i >= b_Vez; --i) {
            if (!uloz_tah(p, z_pole, na_pole)) return;
            p.Zasobnik_tahu[p.Index - 1].premenena_figura = i;
            p.Zasobnik_tahu[p.Index - 1].druh = Druh::Promena;
        }
    } else {
        for (int i = b_Dama; i >= b_Vez; --i) {
            if (!uloz_utocny_tah(p, z_pole, na_pole)) return;
            p.Zasobnik_tahu[p.Index - 1].premenena_figura = i;
            p.Zasobnik_tahu[p.Index - 1].druh = Druh::Promena;
        }
    }
}

}  // namespace

// --- napadene_pole (ř. 641-726) ---------------------------------------------
// `strana` je útočník: vrací true, když strana `strana` napadá `pole`.
bool napadene_pole(const Pozice& p, int pole, int strana) {
    if (strana == bily) {
        if (p.sachovnice[pole - 9] == b_Pesec || p.sachovnice[pole - 11] == b_Pesec) {
            return true;
        }
    }
    if (strana == cerny) {
        if (p.sachovnice[pole + 9] == c_Pesec || p.sachovnice[pole + 11] == c_Pesec) {
            return true;
        }
    }

    for (int i = 8; i <= 15; ++i) {
        const int na_pole = pole + Offset[i];
        if (strana == bily) {
            if (p.sachovnice[na_pole] == b_Jezdec) return true;
        } else {
            if (p.sachovnice[na_pole] == c_Jezdec) return true;
        }
    }

    for (int i = 0; i <= 7; ++i) {
        int na_pole = pole;
        const int smer_figury = Offset[i];
        int klouzej = 0;
        do {
            ++klouzej;
            na_pole += smer_figury;
        } while (p.sachovnice[na_pole] == prazdne_pole);

        if (p.sachovnice[na_pole] == hranice_sachovnice) continue;

        const int Figura = p.sachovnice[na_pole];
        if (strana == bily) {
            if (Figura > 0) {
                if (Figura == b_Kral) {
                    if (klouzej <= 1) return true;
                } else {
                    if (FigOffset[Figura].zacatek <= i && FigOffset[Figura].konec >= i) {
                        return true;
                    }
                }
            }
        } else {
            if (Figura < 0) {
                if (Figura == c_Kral) {
                    if (klouzej <= 1) return true;
                } else {
                    if (FigOffset[-Figura].zacatek <= i && FigOffset[-Figura].konec >= i) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// --- Generuj_tahy (ř. 729-1020) ---------------------------------------------
void Generuj_tahy(Pozice& p, int vsechny_tahy) {
    p.Index = p.hranice_zasobniku[p.hloubka];
    // `pohyblivost[hloubka] := 0` (ř. 863) neportováno - viz hlavička.

    for (int z_pole = A1; z_pole <= H8; ++z_pole) {
        int Figura = p.sachovnice[z_pole];

        if (Figura == prazdne_pole || Figura == hranice_sachovnice) continue;
        if (!((p.barva == bily && Figura > 0) || (p.barva == cerny && Figura < 0))) continue;

        Figura = std::abs(Figura);

        if (Figura == b_Pesec) {
            if (p.barva == bily) {
                if (p.sachovnice[z_pole + 10] == prazdne_pole) {
                    if (z_pole >= A7) {
                        // ř. 878: záměrně NENÍ uvnitř podmínky vsechny_tahy > 0
                        zjisti_promeny_pesce(p, z_pole, z_pole + 10);
                    } else {
                        if (vsechny_tahy > 0) {
                            uloz_tah(p, z_pole, z_pole + 10);
                            if (z_pole <= H2 && p.sachovnice[z_pole + 20] == prazdne_pole) {
                                if (uloz_tah(p, z_pole, z_pole + 20)) {
                                    p.Zasobnik_tahu[p.Index - 1].ep_cil = z_pole + 10;
                                    p.Zasobnik_tahu[p.Index - 1].druh   = Druh::Dvojkrok;
                                }
                            }
                        }
                    }
                }
                // ř. 889: okraj je 100, tedy kladný, takže test < 0 ho vyloučí sám
                if (p.sachovnice[z_pole + 11] < 0) {
                    if (z_pole >= A7) zjisti_promeny_pesce(p, z_pole, z_pole + 11);
                    else              uloz_utocny_tah(p, z_pole, z_pole + 11);
                }
                if (p.sachovnice[z_pole + 9] < 0) {
                    if (z_pole >= A7) zjisti_promeny_pesce(p, z_pole, z_pole + 9);
                    else              uloz_utocny_tah(p, z_pole, z_pole + 9);
                }
            } else {
                if (p.sachovnice[z_pole - 10] == prazdne_pole) {
                    if (z_pole <= H2) {
                        // ř. 902: totéž co ř. 878
                        zjisti_promeny_pesce(p, z_pole, z_pole - 10);
                    } else {
                        if (vsechny_tahy > 0) {
                            uloz_tah(p, z_pole, z_pole - 10);
                            if (z_pole >= A7 && p.sachovnice[z_pole - 20] == prazdne_pole) {
                                if (uloz_tah(p, z_pole, z_pole - 20)) {
                                    p.Zasobnik_tahu[p.Index - 1].ep_cil = z_pole - 10;
                                    p.Zasobnik_tahu[p.Index - 1].druh   = Druh::Dvojkrok;
                                }
                            }
                        }
                    }
                }
                // ř. 913: bílé figury jsou kladné jako okraj, proto test navíc.
                // Asymetrie proti ř. 889 je jen jiný zápis téhož, neunifikovat.
                if (p.sachovnice[z_pole - 11] > 0 && p.sachovnice[z_pole - 11] != hranice_sachovnice) {
                    if (z_pole <= H2) zjisti_promeny_pesce(p, z_pole, z_pole - 11);
                    else              uloz_utocny_tah(p, z_pole, z_pole - 11);
                }
                if (p.sachovnice[z_pole - 9] > 0 && p.sachovnice[z_pole - 9] != hranice_sachovnice) {
                    if (z_pole <= H2) zjisti_promeny_pesce(p, z_pole, z_pole - 9);
                    else              uloz_utocny_tah(p, z_pole, z_pole - 9);
                }
            }
        } else {
            for (int i = FigOffset[Figura].zacatek; i <= FigOffset[Figura].konec; ++i) {
                int na_pole = z_pole + Offset[i];
                while (p.sachovnice[na_pole] == prazdne_pole) {
                    if (vsechny_tahy > 0) uloz_tah(p, z_pole, na_pole);
                    if (!FigOffset[Figura].delka_kroku) break;
                    na_pole += Offset[i];
                }
                if (p.sachovnice[na_pole] != hranice_sachovnice &&
                    ((p.barva == bily  && p.sachovnice[na_pole] < 0) ||
                     (p.barva == cerny && p.sachovnice[na_pole] > 0)) &&
                    (FigOffset[Figura].delka_kroku ||
                     (!FigOffset[Figura].delka_kroku && na_pole == z_pole + Offset[i]))) {
                    uloz_utocny_tah(p, z_pole, na_pole);
                }
            }
        }
    }

    // braní mimochodem (ř. 951-961)
    if (p.en_passant[p.hloubka] != nelegalni) {
        const int ep = p.en_passant[p.hloubka];
        if (p.barva == bily) {
            if (p.sachovnice[ep - 9]  == b_Pesec) uloz_en_passant_tah(p, ep - 9,  ep, ep - 10);
            if (p.sachovnice[ep - 11] == b_Pesec) uloz_en_passant_tah(p, ep - 11, ep, ep - 10);
        } else {
            if (p.sachovnice[ep + 9]  == c_Pesec) uloz_en_passant_tah(p, ep + 9,  ep, ep + 10);
            if (p.sachovnice[ep + 11] == c_Pesec) uloz_en_passant_tah(p, ep + 11, ep, ep + 10);
        }
    }

    // rošáda (ř. 963-1017). Práva plynou z pohyb[]; napadene_pole tu hlídá
    // i pole, přes které král jde, i to, na které dojde.
    if (p.barva == bily) {
        if (p.pozice_bileho_krale == E1 && p.pohyb[E1] == 0) {
            if (p.pohyb[H1] == 0 &&
                p.sachovnice[H1] == b_Vez &&
                p.sachovnice[F1] == prazdne_pole &&
                p.sachovnice[G1] == prazdne_pole &&
                !napadene_pole(p, E1, cerny) &&
                !napadene_pole(p, F1, cerny) &&
                !napadene_pole(p, G1, cerny)) {
                if (uloz_tah(p, E1, G1)) {
                    p.Zasobnik_tahu[p.Index - 1].druh_rosady = mala_rosada;
                    p.Zasobnik_tahu[p.Index - 1].druh        = Druh::Rosada;
                }
            }
            if (p.pohyb[A1] == 0 &&
                p.sachovnice[A1] == b_Vez &&
                p.sachovnice[D1] == prazdne_pole &&
                p.sachovnice[C1] == prazdne_pole &&
                p.sachovnice[B1] == prazdne_pole &&
                !napadene_pole(p, E1, cerny) &&
                !napadene_pole(p, D1, cerny) &&
                !napadene_pole(p, C1, cerny)) {
                if (uloz_tah(p, E1, C1)) {
                    p.Zasobnik_tahu[p.Index - 1].druh_rosady = velka_rosada;
                    p.Zasobnik_tahu[p.Index - 1].druh        = Druh::Rosada;
                }
            }
        }
    } else {
        if (p.pozice_cerneho_krale == E8 && p.pohyb[E8] == 0) {
            if (p.pohyb[H8] == 0 &&
                p.sachovnice[H8] == c_Vez &&
                p.sachovnice[F8] == prazdne_pole &&
                p.sachovnice[G8] == prazdne_pole &&
                !napadene_pole(p, E8, bily) &&
                !napadene_pole(p, F8, bily) &&
                !napadene_pole(p, G8, bily)) {
                if (uloz_tah(p, E8, G8)) {
                    p.Zasobnik_tahu[p.Index - 1].druh_rosady = mala_rosada;
                    p.Zasobnik_tahu[p.Index - 1].druh        = Druh::Rosada;
                }
            }
            if (p.pohyb[A8] == 0 &&
                p.sachovnice[A8] == c_Vez &&
                p.sachovnice[D8] == prazdne_pole &&
                p.sachovnice[C8] == prazdne_pole &&
                p.sachovnice[B8] == prazdne_pole &&
                !napadene_pole(p, E8, bily) &&
                !napadene_pole(p, D8, bily) &&
                !napadene_pole(p, C8, bily)) {
                if (uloz_tah(p, E8, C8)) {
                    p.Zasobnik_tahu[p.Index - 1].druh_rosady = velka_rosada;
                    p.Zasobnik_tahu[p.Index - 1].druh        = Druh::Rosada;
                }
            }
        }
    }

    p.hranice_zasobniku[p.hloubka + 1] = p.Index;   // ř. 1018
}

// --- zahraj_tah (ř. 2633-2697) ----------------------------------------------
void zahraj_tah(Pozice& p, int cislo_tahu) {
    const typ_tahu& tah = p.Zasobnik_tahu[cislo_tahu];
    const int z_pole  = tah.z_pole;
    const int na_pole = tah.na_pole;

    ++p.hloubka;
    p.cilove_pole[p.hloubka] = na_pole;
    p.en_passant[p.hloubka]  = nelegalni;
    p.materialova_bilance[p.hloubka] = -p.materialova_bilance[p.hloubka - 1];
    p.celkovy_material_na_sachovnici[p.hloubka] =
        p.celkovy_material_na_sachovnici[p.hloubka - 1];

    ++p.pohyb[z_pole];
    ++p.pohyb[na_pole];

    // Originál tu rozlišoval dvojkrok od braní mimochodem testem na prázdnost
    // pole (ř. 2650), což platilo jen díky tomu, že se to řeší ještě před
    // přesunem figury níž. Tady rozhoduje druh; pořadí operací zůstává.
    if (tah.druh == Druh::Dvojkrok) {
        p.en_passant[p.hloubka] = tah.ep_cil;
    } else if (tah.druh == Druh::EnPassant) {
        p.sachovnice[tah.ep_brane_pole] = prazdne_pole;
        p.materialova_bilance[p.hloubka] -= hodnota_Pesce;
    } else {
        if (tah.brana_figura != prazdne_pole) {
            const int materialova_zmena = FigMaterial[tah.brana_figura];
            p.materialova_bilance[p.hloubka] -= materialova_zmena;
            // pěšci se do celkového materiálu nepočítají
            if (materialova_zmena != hodnota_Pesce) {
                p.celkovy_material_na_sachovnici[p.hloubka] -= materialova_zmena;
            }
        }
    }

    p.sachovnice[na_pole] = p.sachovnice[z_pole];
    p.sachovnice[z_pole]  = prazdne_pole;

    if (tah.premenena_figura != prazdne_pole) {
        p.sachovnice[na_pole] = p.barva * tah.premenena_figura;
        const int materialova_zmena = FigMaterial[tah.premenena_figura] - hodnota_Pesce;
        p.materialova_bilance[p.hloubka] -= materialova_zmena;
        p.celkovy_material_na_sachovnici[p.hloubka] += materialova_zmena + hodnota_Pesce;
    } else {
        // pohyb[] se pro věž při rošádě záměrně neinkrementuje (ř. 2647-2648
        // pokrývá jen pole krále). Nemá to důsledky, neopravovat.
        if (tah.druh_rosady == mala_rosada) {
            p.sachovnice[na_pole + 1] = prazdne_pole;
            p.sachovnice[na_pole - 1] = p.barva * b_Vez;
            p.rosada[p.barva + 1] = pravda;
        } else if (tah.druh_rosady == velka_rosada) {
            p.sachovnice[na_pole - 2] = prazdne_pole;
            p.sachovnice[na_pole + 1] = p.barva * b_Vez;
            p.rosada[p.barva + 1] = pravda;
        }
    }

    if (p.sachovnice[na_pole] == b_Kral) {
        p.pozice_bileho_krale = na_pole;
    } else if (p.sachovnice[na_pole] == c_Kral) {
        p.pozice_cerneho_krale = na_pole;
    }

    p.barva = -p.barva;
}

// --- vezmi_tah_zpet (ř. 2699-2745) ------------------------------------------
// Pole indexovaná hloubkou nerestauruje, jen sníží hloubku - nosný princip
// originálu, nepřepisovat na snapshot.
void vezmi_tah_zpet(Pozice& p, int cislo_tahu) {
    const typ_tahu& tah = p.Zasobnik_tahu[cislo_tahu];
    const int z_pole  = tah.z_pole;
    const int na_pole = tah.na_pole;

    p.barva = -p.barva;          // barva je teď zase strana, která táhla
    --p.hloubka;

    p.sachovnice[z_pole]  = p.sachovnice[na_pole];
    p.sachovnice[na_pole] = prazdne_pole;

    if (tah.druh == Druh::EnPassant) {
        p.sachovnice[tah.ep_brane_pole] = -p.barva;   // pěšec soupeře
    } else if (tah.brana_figura != prazdne_pole) {
        p.sachovnice[na_pole] = (-p.barva) * tah.brana_figura;
    }

    --p.pohyb[z_pole];
    --p.pohyb[na_pole];

    if (tah.druh_rosady == mala_rosada) {
        p.sachovnice[na_pole + 1] = p.barva * b_Vez;
        p.sachovnice[na_pole - 1] = prazdne_pole;
        p.rosada[p.barva + 1] = nepravda;
    } else if (tah.druh_rosady == velka_rosada) {
        p.sachovnice[na_pole - 2] = p.barva * b_Vez;
        p.sachovnice[na_pole + 1] = prazdne_pole;
        p.rosada[p.barva + 1] = nepravda;
    }

    if (tah.premenena_figura != prazdne_pole) {
        p.sachovnice[z_pole] = p.barva;      // zpátky pěšec: barva * b_Pesec
    }

    if (p.sachovnice[z_pole] == b_Kral) {
        p.pozice_bileho_krale = z_pole;
    } else if (p.sachovnice[z_pole] == c_Kral) {
        p.pozice_cerneho_krale = z_pole;
    }
}

// --- nacti_uvodni_pozici (ř. 513-558) ---------------------------------------
void nacti_uvodni_pozici(Pozice& p) {
    for (int i = 0; i <= velikost_sachovnice_10x12; ++i) p.sachovnice[i] = zakladni_postaveni[i];
    p.pozice_bileho_krale  = E1;
    p.pozice_cerneho_krale = E8;
    for (int i = 0; i <= 2; ++i) p.rosada[i] = nepravda;
    // Originál nuluje jen A1..H8 (ř. 532); tady celé pole, ať nezůstane nic
    // neinicializovaného.
    for (int i = 0; i <= H8; ++i) p.pohyb[i] = 0;
    p.en_passant[0] = nelegalni;
    p.celkovy_material_na_sachovnici[0] = material_v_zakl_postaveni;
    p.materialova_bilance[0] = 0;
    p.hranice_zasobniku[0] = 0;
    p.hloubka = 0;
    p.barva = bily;
    p.Index = 0;
    p.cilove_pole[0] = 0;
    p.pultahy_bez_zmeny = 0;
    p.cislo_tahu = 1;
    p.zasobnik_pretekl = false;
}

// --- FEN --------------------------------------------------------------------
// V originále není. Práva na rošádu se překládají do pohyb[], protože z něj
// generátor práva odvozuje (ř. 961-1017) a v Kroku 2 z něj bude hodnocení
// počítat vývin (ř. 1290-1297).

namespace {

// dopočítá materiál a pozice králů z rozestavení
void dopocitej_material(Pozice& p) {
    int material_bily = 0, material_cerny = 0, celkem = 0;
    for (int pole = A1; pole <= H8; ++pole) {
        const int f = p.sachovnice[pole];
        if (f == prazdne_pole || f == hranice_sachovnice) continue;
        const int typ = std::abs(f);
        if (f > 0) material_bily += FigMaterial[typ];
        else       material_cerny += FigMaterial[typ];
        // celkový materiál na šachovnici pěšce (ani krále) nepočítá - stejně
        // jako ho zahraj_tah při braní pěšce neodečítá
        if (typ != b_Pesec && typ != b_Kral) celkem += FigMaterial[typ];
    }
    p.celkovy_material_na_sachovnici[0] = celkem;
    // materialova_bilance je z pohledu strany na tahu
    p.materialova_bilance[0] = p.barva * (material_bily - material_cerny);
}

}  // namespace

bool nacti_fen(Pozice& p, const std::string& fen) {
    Pozice cista;                 // výchozí hodnoty ze struktury
    p = cista;
    for (int i = 0; i <= velikost_sachovnice_10x12; ++i) {
        p.sachovnice[i] = zakladni_postaveni[i];   // kvůli okrajům
    }
    for (int pole = A1; pole <= H8; ++pole) {
        if (p.sachovnice[pole] != hranice_sachovnice) p.sachovnice[pole] = prazdne_pole;
    }

    std::istringstream in(fen);
    std::string deska, kdo, rosady, ep;
    if (!(in >> deska >> kdo >> rosady >> ep)) return false;

    int sloupec = 1, rada = 8;
    bool ma_bileho_krale = false, ma_cerneho_krale = false;
    for (char c : deska) {
        if (c == '/') { --rada; sloupec = 1; continue; }
        if (std::isdigit(static_cast<unsigned char>(c))) { sloupec += c - '0'; continue; }
        if (sloupec > 8 || rada < 1) return false;

        const int znamenko = std::isupper(static_cast<unsigned char>(c)) ? bily : cerny;
        int typ;
        switch (std::tolower(static_cast<unsigned char>(c))) {
            case 'p': typ = b_Pesec;   break;
            case 'r': typ = b_Vez;     break;
            case 'b': typ = b_Strelec; break;
            case 'n': typ = b_Jezdec;  break;
            case 'q': typ = b_Dama;    break;
            case 'k': typ = b_Kral;    break;
            default:  return false;
        }
        const int pole = pole_z_souradnic(sloupec, rada);
        p.sachovnice[pole] = znamenko * typ;
        if (typ == b_Kral) {
            if (znamenko == bily) { p.pozice_bileho_krale = pole;  ma_bileho_krale  = true; }
            else                  { p.pozice_cerneho_krale = pole; ma_cerneho_krale = true; }
        }
        ++sloupec;
    }
    if (!ma_bileho_krale || !ma_cerneho_krale) return false;

    p.barva = (kdo == "w") ? bily : cerny;

    // Práva na rošádu -> pohyb[]. Chybí-li malá, táhla věž; chybí-li velká,
    // táhla druhá věž; chybí-li obě, táhl král.
    for (int i = 0; i <= H8; ++i) p.pohyb[i] = 0;
    const bool bila_mala  = rosady.find('K') != std::string::npos;
    const bool bila_velka = rosady.find('Q') != std::string::npos;
    const bool cerna_mala  = rosady.find('k') != std::string::npos;
    const bool cerna_velka = rosady.find('q') != std::string::npos;
    if (!bila_mala)  p.pohyb[H1] = 1;
    if (!bila_velka) p.pohyb[A1] = 1;
    if (!bila_mala && !bila_velka) p.pohyb[E1] = 1;
    if (!cerna_mala)  p.pohyb[H8] = 1;
    if (!cerna_velka) p.pohyb[A8] = 1;
    if (!cerna_mala && !cerna_velka) p.pohyb[E8] = 1;

    if (ep == "-" || ep.size() < 2) {
        p.en_passant[0] = nelegalni;
    } else {
        const int s = ep[0] - 'a' + 1;
        const int r = ep[1] - '0';
        if (s < 1 || s > 8 || r < 1 || r > 8) return false;
        p.en_passant[0] = pole_z_souradnic(s, r);
    }

    for (int i = 0; i <= 2; ++i) p.rosada[i] = nepravda;
    p.hloubka = 0;
    p.Index = 0;
    p.hranice_zasobniku[0] = 0;
    p.cilove_pole[0] = 0;
    dopocitej_material(p);

    // pátý a šestý údaj jsou nepovinné; jen se uchovají, nikde se nepoužívají
    int pultahy = 0, cislo = 1;
    if (in >> pultahy) p.pultahy_bez_zmeny = pultahy < 0 ? 0 : pultahy;
    if (in >> cislo)   p.cislo_tahu = cislo < 1 ? 1 : cislo;

    return true;
}

std::string zapis_fen(const Pozice& p) {
    static const char* symbol = ".prbnqk";   // podle typu 0..6

    std::string out;
    for (int rada = 8; rada >= 1; --rada) {
        int prazdnych = 0;
        for (int sloupec = 1; sloupec <= 8; ++sloupec) {
            const int f = p.sachovnice[pole_z_souradnic(sloupec, rada)];
            if (f == prazdne_pole) { ++prazdnych; continue; }
            if (prazdnych > 0) { out += static_cast<char>('0' + prazdnych); prazdnych = 0; }
            const char c = symbol[std::abs(f)];
            out += (f > 0) ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        }
        if (prazdnych > 0) out += static_cast<char>('0' + prazdnych);
        if (rada > 1) out += '/';
    }

    out += (p.barva == bily) ? " w " : " b ";

    // Práva se odvozují zpět z pohyb[] a z toho, že král i věž opravdu stojí
    // na svých polích.
    std::string rosady;
    if (p.pohyb[E1] == 0 && p.pohyb[H1] == 0 &&
        p.sachovnice[E1] == b_Kral && p.sachovnice[H1] == b_Vez) rosady += 'K';
    if (p.pohyb[E1] == 0 && p.pohyb[A1] == 0 &&
        p.sachovnice[E1] == b_Kral && p.sachovnice[A1] == b_Vez) rosady += 'Q';
    if (p.pohyb[E8] == 0 && p.pohyb[H8] == 0 &&
        p.sachovnice[E8] == c_Kral && p.sachovnice[H8] == c_Vez) rosady += 'k';
    if (p.pohyb[E8] == 0 && p.pohyb[A8] == 0 &&
        p.sachovnice[E8] == c_Kral && p.sachovnice[A8] == c_Vez) rosady += 'q';
    out += rosady.empty() ? "-" : rosady;

    out += ' ';
    const int ep = p.en_passant[p.hloubka];
    if (ep == nelegalni) {
        out += '-';
    } else {
        out += static_cast<char>('a' + sloupec_pole(ep) - 1);
        out += static_cast<char>('0' + rada_pole(ep));
    }

    out += ' ';
    out += std::to_string(p.pultahy_bez_zmeny);
    out += ' ';
    out += std::to_string(p.cislo_tahu);
    return out;
}

// --- pomůcka ----------------------------------------------------------------
std::string tah_na_text(const typ_tahu& tah) {
    static const char* symbol = ".prbnqk";

    std::string s;
    s += static_cast<char>('a' + sloupec_pole(tah.z_pole) - 1);
    s += static_cast<char>('0' + rada_pole(tah.z_pole));
    s += static_cast<char>('a' + sloupec_pole(tah.na_pole) - 1);
    s += static_cast<char>('0' + rada_pole(tah.na_pole));
    if (tah.premenena_figura != prazdne_pole) s += symbol[tah.premenena_figura];
    return s;
}

}  // namespace redmax
