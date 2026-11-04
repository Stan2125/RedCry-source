// ---------------------------------------------------------------------------
//  RedMAX - vrstva hledání (implementace)
//
//  Přepis z RedMAX_408_f.pas. Čísla řádků v komentářích odkazují do originálu.
//  Žádné I/O, žádná vlákna, žádný globální stav - viz red_max_search.h.
// ---------------------------------------------------------------------------

#include "red_max_search.h"

#include <cstdlib>
#include <memory>
#include <random>

namespace redmax {

namespace {

// --- hodnotové tabulky (ř. 145-199) -----------------------------------------
//
// V Pascalu jsou obě pěšcové tabulky deklarované jako array[0..H7], tedy
// o deset prvků kratší než centralizační. Pěšec na osmé řadě se v regulérní
// pozici nevyskytne (promění se), ale nacti_fen takový FEN nezamítne, a pak
// by se četlo za koncem pole. Tady jsou proto obě dlouhé po H8; přidané prvky
// jsou nuly, takže na hodnotách to nic nemění.

const int centralizacni_tabulka[H8 + 1] = {
//  ---   A   B   C   D   E   F   G   H  ---
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  4,  0,  8, 12, 12,  8,  0,  4,  0,
      0,  4,  8, 12, 16, 16, 12,  8,  4,  0,
      0,  8, 12, 20, 20, 20, 20, 12,  8,  0,
      0, 12, 16, 20, 24, 24, 20, 16, 12,  0,
      0, 12, 16, 20, 24, 24, 20, 16, 12,  0,
      0,  8, 12, 20, 20, 20, 20, 12,  8,  0,
      0,  4,  8, 12, 16, 16, 12,  8,  4,  0,
      0,  4,  0,  8, 12, 12,  8,  0,  4
};

const int tabulka_bilych_pescu[H8 + 1] = {
//  ---   A   B   C   D   E   F   G   H  ---
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  4,  4,  0,  0,  0,  6,  6,  6,  0,
      0,  6,  6,  8,  8,  8,  4,  6,  6,  0,
      0,  8,  8, 16, 22, 22,  4,  4,  4,  0,
      0, 10, 10, 20, 26, 26, 10, 10, 10,  0,
      0, 12, 12, 22, 28, 28, 14, 14, 14,  0,
      0, 18, 18, 28, 32, 32, 20, 20, 20,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0
};

const int tabulka_cernych_pescu[H8 + 1] = {
//  ---   A   B   C   D   E   F   G   H  ---
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0, 18, 18, 28, 32, 32, 20, 20, 20,  0,
      0, 12, 12, 22, 28, 28, 14, 14, 14,  0,
      0, 10, 10, 20, 26, 26, 10, 10, 10,  0,
      0,  8,  8, 16, 22, 22,  4,  4,  4,  0,
      0,  6,  6,  8,  8,  8,  4,  6,  6,  0,
      0,  4,  4,  0,  0,  0,  6,  6,  6,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0
};

// --- kontrola_pesci_a_vezmi (ř. 1035-1074) ----------------------------------
// Spočítá, která pole kontrolují pěšci, kolik je ve kterém sloupci pěšců
// a kolik věží. To potřebují obě pěšcové funkce i hodnocení krále a věží.
void kontrola_pesci_a_vezmi(const Pozice& p, StavHledani& stav) {
    for (int i = 0; i <= velikost_sachovnice_10x12; ++i) {
        stav.pole_kontrolovana_pesci[i].bily  = 0;
        stav.pole_kontrolovana_pesci[i].cerny = 0;
    }
    // včetně okrajových sloupců 0 a 9, pak se nemusí zkoumat sólo (ř. 1046)
    for (int i = Sloupec_A - 1; i <= Sloupec_H + 1; ++i) {
        stav.pescu_ve_sloupci[i].bily  = 0;
        stav.pescu_ve_sloupci[i].cerny = 0;
        stav.vezi_ve_sloupci[i].bily   = 0;
        stav.vezi_ve_sloupci[i].cerny  = 0;
    }

    for (int i = A1; i <= H8; ++i) {
        switch (p.sachovnice[i]) {
            case b_Pesec:
                ++stav.pole_kontrolovana_pesci[i + 9].bily;
                ++stav.pole_kontrolovana_pesci[i + 11].bily;
                ++stav.pescu_ve_sloupci[i % 10].bily;
                break;
            case c_Pesec:
                ++stav.pole_kontrolovana_pesci[i - 9].cerny;
                ++stav.pole_kontrolovana_pesci[i - 11].cerny;
                ++stav.pescu_ve_sloupci[i % 10].cerny;
                break;
            case c_Vez:
                ++stav.vezi_ve_sloupci[i % 10].cerny;
                break;
            case b_Vez:
                ++stav.vezi_ve_sloupci[i % 10].bily;
                break;
            default:
                break;
        }
    }
}

// --- oceneni_cernych_pescu (ř. 1078-1159, SBBewertung, str. 415) ------------
// Vrací hodnotu z pohledu černého; volající ji odečítá.
int oceneni_cernych_pescu(const Pozice& p, const StavHledani& stav,
                          int soucet_materialu,
                          int pole, int rada, int sloupec, int vyvinuto) {
    int hodnota;

    rada = (Rada_8 + Rada_1) - rada;    // ř. 1082: pro černého se řada zrcadlí

    if (soucet_materialu > material_pro_koncovku) {
        hodnota = tabulka_cernych_pescu[pole];
        // při neukončeném vývinu neposune pěšce dopředu
        if (vyvinuto < 4) {
            if ((sloupec >= Sloupec_F || sloupec <= Sloupec_B) && rada > Rada_3) {
                hodnota -= 15;
            }
        }
    } else {
        // koncovka, tlač pěšce dopředu
        hodnota = rada * 4;
    }

    if (stav.pescu_ve_sloupci[sloupec].cerny > 1) {     // dva nad sebou
        hodnota -= 4;
        if (stav.pescu_ve_sloupci[sloupec - 1].cerny == 0 &&
            stav.pescu_ve_sloupci[sloupec + 1].cerny == 0) {
            hodnota -= 8;   // izolovaný dvojpěšec
        }
    } else {
        if (stav.pescu_ve_sloupci[sloupec - 1].cerny == 0 &&
            stav.pescu_ve_sloupci[sloupec + 1].cerny == 0) {
            hodnota -= 6;   // izolovaný pěšec
        }
    }

    // dvojice nebo podporovaný pěšec dostane bonus (ř. 1111)
    if (stav.pole_kontrolovana_pesci[pole].cerny > 0 ||
        stav.pole_kontrolovana_pesci[pole - 10].cerny > 0) {
        hodnota += rada / 2;
    }

    if (stav.pescu_ve_sloupci[sloupec].bily == 0) {
        // opožděný pěšec v polootevřeném sloupci: nemá podporu vlastního pěšce
        // a nemůže dopředu, protože pole před ním kontrolují soupeřovi pěšci
        if (stav.pole_kontrolovana_pesci[pole].cerny == 0 &&
            stav.pole_kontrolovana_pesci[pole - 10].bily >
            stav.pole_kontrolovana_pesci[pole - 10].cerny) {
            hodnota -= 5;
            if (stav.vezi_ve_sloupci[sloupec].bily > 0) hodnota -= 5;
        } else {
            // volný pěšec?
            for (int j = pole; j >= A3; j -= 10) {
                if (stav.pole_kontrolovana_pesci[j].bily > 0) return hodnota;
            }

            // v koncovce je volný pěšec důležitější než ve střední hře
            if (soucet_materialu < material_pro_koncovku) {
                hodnota += rada * 16;
                if (stav.vezi_ve_sloupci[sloupec].cerny > 0) hodnota += rada * 2;
                if (stav.vezi_ve_sloupci[sloupec].bily  > 0) hodnota -= rada * 2;
                // koncovka jen s pěšci, tudíž mají zvláštní hodnotu
                if (soucet_materialu == 0) hodnota += rada * 8;
                if (stav.pole_kontrolovana_pesci[pole].cerny > 0 ||
                    stav.pole_kontrolovana_pesci[pole - 10].cerny > 0) {
                    hodnota += rada * 4;
                }
                // Oprava § 3.12: blokáda SOUPEŘOVOU figurou, tedy bílou.
                // Originál tu měl `< 0` (ř. 1144) stejně jako v bílé větvi
                // (ř. 1225), jenže před černým pěšcem je záporná hodnota
                // figura VLASTNÍ, kdežto před bílým soupeřova - táž srážka
                // tak u každé barvy trestala něco jiného. Srovnáno na bílého.
                //
                // Vyloučení okraje jako na ř. 913: `hranice_sachovnice` je
                // 100, tedy kladná, takže by se při `> 0` počítala jako
                // soupeřova figura. Černý pěšec na řadě 1 být nemůže, takže
                // se sem okraj nedostane, ale ať to nesvádí k záměně.
                // (Bílá větev guard nepotřebuje: 100 není < 0.)
                if (p.sachovnice[pole - 10] > 0 &&
                    p.sachovnice[pole - 10] != hranice_sachovnice &&
                    stav.pole_kontrolovana_pesci[pole - 10].cerny == 0) {
                    hodnota -= rada * 4;
                }
            } else {
                hodnota += rada * 8;
                if (stav.pole_kontrolovana_pesci[pole].cerny > 0 ||
                    stav.pole_kontrolovana_pesci[pole - 10].cerny > 0) {
                    hodnota += rada * 2;
                }
            }
        }
    }
    return hodnota;
}

// --- oceneni_bilych_pescu (ř. 1163-1239, WBBewertung, str. 421) -------------
int oceneni_bilych_pescu(const Pozice& p, const StavHledani& stav,
                         int soucet_materialu,
                         int pole, int rada, int sloupec, int vyvinuto) {
    int hodnota;

    if (soucet_materialu > material_pro_koncovku) {
        hodnota = tabulka_bilych_pescu[pole];
        if (vyvinuto < 4) {
            if ((sloupec >= Sloupec_F || sloupec <= Sloupec_B) && rada > Rada_3) {
                hodnota -= 15;
            }
        }
    } else {
        hodnota = rada * 4;
    }

    if (stav.pescu_ve_sloupci[sloupec].bily > 1) {      // dva nad sebou
        hodnota -= 4;
        if (stav.pescu_ve_sloupci[sloupec - 1].bily == 0 &&
            stav.pescu_ve_sloupci[sloupec + 1].bily == 0) {
            hodnota -= 8;   // izolovaný dvojpěšec
        }
    } else {
        if (stav.pescu_ve_sloupci[sloupec - 1].bily == 0 &&
            stav.pescu_ve_sloupci[sloupec + 1].bily == 0) {
            hodnota -= 6;   // izolovaný pěšec
        }
    }

    // je-li pěšec kontrolován jiným, nebo je-li pole před ním kontrolováno
    // sousedním pěšcem (ř. 1193)
    if (stav.pole_kontrolovana_pesci[pole].bily > 0 ||
        stav.pole_kontrolovana_pesci[pole + 10].bily > 0) {
        hodnota += rada / 2;
    }

    if (stav.pescu_ve_sloupci[sloupec].cerny == 0) {
        // opožděný pěšec
        if (stav.pole_kontrolovana_pesci[pole].bily == 0 &&
            stav.pole_kontrolovana_pesci[pole + 10].cerny >
            stav.pole_kontrolovana_pesci[pole + 10].bily) {
            hodnota -= 5;
            if (stav.vezi_ve_sloupci[sloupec].cerny > 0) hodnota -= 5;
        } else {
            // volný pěšec?
            for (int j = pole; j <= H6; j += 10) {
                if (stav.pole_kontrolovana_pesci[j].cerny > 0) return hodnota;
            }

            if (soucet_materialu < material_pro_koncovku) {
                hodnota += rada * 16;
                if (stav.vezi_ve_sloupci[sloupec].bily  > 0) hodnota += rada * 2;
                if (stav.vezi_ve_sloupci[sloupec].cerny > 0) hodnota -= rada * 2;
                if (soucet_materialu == 0) hodnota += rada * 8;
                if (stav.pole_kontrolovana_pesci[pole].bily > 0 ||
                    stav.pole_kontrolovana_pesci[pole + 10].bily > 0) {
                    hodnota += rada * 4;
                }
                if (p.sachovnice[pole + 10] < 0 &&
                    stav.pole_kontrolovana_pesci[pole + 10].bily == 0) {
                    hodnota -= rada * 4;
                }
            } else {
                // volný pěšec ve střední hře
                hodnota += rada * 8;
                if (stav.pole_kontrolovana_pesci[pole].bily > 0 ||
                    stav.pole_kontrolovana_pesci[pole + 10].bily > 0) {
                    hodnota += rada * 2;
                }
            }
        }
    }
    return hodnota;
}

// --- rošáda odvozená z desky ------------------------------------------------
// Originál poznal rošádu podle `rosada[]` (ř. 1287-1288, 1310 a 1433), jenže
// to pole závisí na CESTĚ, ne na pozici: nastaví ho zahraj_tah a vezmi_tah_zpet
// ho zase shodí. FEN ho nenese a v hashi není vůbec, takže se do hodnocení
// dostat nesmí - viz zásada o čistotě hodnocení.
//
// Tady se rošáda odvozuje z desky: král na G1 a věž na F1 (malá rošáda), nebo
// král na C1 a věž na D1 (velká). NENÍ TO NAPROSTO PŘESNÉ - král, který dojde
// na g1 po svých, a věž, která se postaví na f1, se budou tvářit jako po malé
// rošádě. Z FEN ale `rosada[]` obnovitelné nebylo vůbec, takže je to zlepšení
// proti stavu, kdy by pro každou načtenou pozici vycházelo "nerochoval nikdo".
bool bily_rochoval(const Pozice& p) {
    return (p.sachovnice[G1] == b_Kral && p.sachovnice[F1] == b_Vez) ||
           (p.sachovnice[C1] == b_Kral && p.sachovnice[D1] == b_Vez);
}

bool cerny_rochoval(const Pozice& p) {
    return (p.sachovnice[G8] == c_Kral && p.sachovnice[F8] == c_Vez) ||
           (p.sachovnice[C8] == c_Kral && p.sachovnice[D8] == c_Vez);
}

// --- vývin z desky ----------------------------------------------------------
// Oprava (b) zadání, rozbor 3.3. Originál počítal vývin z pohyb[B1], pohyb[C1],
// pohyb[F1], pohyb[G1] a z rosada[] (ř. 1287-1297). Obojí závisí na CESTĚ, ne
// na pozici: jezdec, co jde b1-c3-b1, nechá pohyb[B1] = 2, jezdec, co se
// nehnul, nulu - a pozice je přitom táž. V hashi ty čtyři údaje nejsou vůbec,
// takže by tabulka vracela cizí čísla. Navíc nacti_fen pohyb[] vynuluje
// a rosada[] FEN nenese, takže by každá testovací pozice vycházela jako
// "nikdo nic nevyvinul".
//
// Tady se vývin odvozuje z desky: kolik z polí B1, C1, F1, G1 neobsazuje
// figura, která tam patří v základním postavení, plus jedna ZA ROŠÁDU. Ta
// pátá položka je přesně to, co originál napsal jako
// `vyvin_bileho := rosada[bily + 1]` (ř. 1287) - tedy rošáda, ne jakýkoli
// pohyb krále. Rozsah 0 až 5 je týž jako v originále.
int vyvin_bileho_z_desky(const Pozice& p) {
    int vyvin = 0;
    if (p.sachovnice[B1] != b_Jezdec)  ++vyvin;
    if (p.sachovnice[C1] != b_Strelec) ++vyvin;
    if (p.sachovnice[F1] != b_Strelec) ++vyvin;
    if (p.sachovnice[G1] != b_Jezdec)  ++vyvin;
    if (bily_rochoval(p))              ++vyvin;
    return vyvin;
}

int vyvin_cerneho_z_desky(const Pozice& p) {
    int vyvin = 0;
    if (p.sachovnice[B8] != c_Jezdec)  ++vyvin;
    if (p.sachovnice[C8] != c_Strelec) ++vyvin;
    if (p.sachovnice[F8] != c_Strelec) ++vyvin;
    if (p.sachovnice[G8] != c_Jezdec)  ++vyvin;
    if (cerny_rochoval(p))             ++vyvin;
    return vyvin;
}

// --- ztracená rošáda --------------------------------------------------------
// Oprava (c) zadání. Originál srážel za ztracenou rošádu podmínkou
//    rosada[strana] = nepravda  A  (pohyb[E] > 0 NEBO (pohyb[H] > 0 A pohyb[A] > 0))
// (ř. 1310-1314 a 1433-1437).
//
// Pravý činitel je "nemá už ani jedno právo na rošádu" a je čistý: pohyb[E1],
// pohyb[H1] a pohyb[A1] jsou přesně ta práva, která nese FEN i hash. Zbývá
// levý činitel, tedy "ta strana ještě nerochovala" - a ten zastane
// bily_rochoval / cerny_rochoval výš. Trestá se tím ZTRÁTA PRÁVA BEZ ROŠÁDY,
// ne ztráta práva jako taková.
//
// Původní znění opravy (c) v zadání Kroku 2 bylo jiné a bylo chybné: levý
// činitel měl nahradit test "král nestojí na výchozím poli NEBO na něm stojí,
// ale nemá už ani jedno právo". Ten se s pravým činitelem zkrátí na prostý
// test práv, takže srážka 35 padla i na stranu, která rochovala - originál ji
// tam neuplatňoval. Viz POZNAMKY-REDMAX.md, oddíl 2.5.
bool bily_uz_nerochuje(const Pozice& p) {
    return p.pohyb[E1] > 0 || (p.pohyb[H1] > 0 && p.pohyb[A1] > 0);
}

bool cerny_uz_nerochuje(const Pozice& p) {
    return p.pohyb[E8] > 0 || (p.pohyb[H8] > 0 && p.pohyb[A8] > 0);
}

}  // namespace

// --- Oceneni_pozice (ř. 1033-1539) ------------------------------------------
int oceneni_pozice(const Pozice& p, int Alfa, int Beta, int strana, StavHledani& stav) {
    // nejprve, jestli má protivník mat, nebo strana stojí v šachu (ř. 1250)
    stav.lze_sebrat_krale = false;
    if (strana == bily) {
        if (napadene_pole(p, p.pozice_cerneho_krale, bily)) {
            stav.lze_sebrat_krale = true;
            return hodnota_matu - p.hloubka;
        }
        stav.strana_na_tahu_v_sachu = napadene_pole(p, p.pozice_bileho_krale, cerny);
    } else {
        if (napadene_pole(p, p.pozice_bileho_krale, cerny)) {
            stav.lze_sebrat_krale = true;
            return hodnota_matu - p.hloubka;
        }
        stav.strana_na_tahu_v_sachu = napadene_pole(p, p.pozice_cerneho_krale, bily);
    }

    int hodnota = p.materialova_bilance[p.hloubka];
    const int soucet_materialu = p.celkovy_material_na_sachovnici[p.hloubka];

    // líné hodnocení (ř. 1267-1273): je-li materiál dost daleko za oknem,
    // poziční složku nemá smysl počítat
    if (stav.nastaveni.line_hodnoceni &&
        soucet_materialu > hodnota_Strelce + hodnota_Strelce) {
        if (hodnota < Alfa - max_pozicniho_oceneni || hodnota > Beta + max_pozicniho_oceneni) {
            ++stav.bez_pozicnich_kriterii;
            return hodnota;
        }
    }

    int pozicni_hodnota = 0;

    if (stav.nastaveni.pozicni_kriteria) {      // CheckBox1.Checked (ř. 1277)
        ++stav.s_pozicnimi_kriterii;
        kontrola_pesci_a_vezmi(p, stav);

        int cerni_strelci = 0, bili_strelci = 0, pocet_pescu = 0;
        int bile_veze_v_78r = 0, cerne_veze_v_78r = 0;

        // oprava (b): vývin z desky, ne z pohyb[] a rosada[]
        const int vyvin_bileho  = vyvin_bileho_z_desky(p);
        const int vyvin_cerneho = vyvin_cerneho_z_desky(p);

        for (int i = Rada_1; i <= Rada_8; ++i) {
            int pole = i * 10;
            for (int j = Sloupec_A; j <= Sloupec_H; ++j) {
                ++pole;
                switch (p.sachovnice[pole]) {

                case c_Kral:
                    if (soucet_materialu < material_pro_koncovku) {
                        // koncovková centralizace krále
                        pozicni_hodnota -= centralizacni_tabulka[pole];
                    } else {
                        // oprava (c): ztráta práva na rošádu BEZ rošády,
                        // bez rosada[] a bez závislosti na cestě
                        if (cerny_uz_nerochuje(p) && !cerny_rochoval(p)) {
                            pozicni_hodnota += 35;
                        }

                        pozicni_hodnota += 4 * centralizacni_tabulka[pole];
                        for (int k = -1; k <= 1; ++k) {
                            if (p.sachovnice[pole - 10 + k] == c_Pesec) pozicni_hodnota -= 15;
                            if (p.sachovnice[pole - 20 + k] == c_Pesec) pozicni_hodnota -= 6;
                            if (stav.pescu_ve_sloupci[j + k].bily == 0 &&
                                stav.vezi_ve_sloupci[j + k].bily > 0) {
                                pozicni_hodnota += 12;
                            }
                        }
                    }
                    break;

                case c_Dama:
                    // zabránit dámě ve vstupu do hry při zahájení
                    if (vyvin_cerneho < 4) {
                        if (pole < A8) pozicni_hodnota += 15;
                    } else {
                        const int dama_ke_krali_rada =
                            std::abs((p.pozice_bileho_krale / 10) - (pole / 10));
                        const int dama_ke_krali_sloupec =
                            std::abs((p.pozice_bileho_krale % 10) - (pole % 10));
                        pozicni_hodnota += 2 * (dama_ke_krali_rada + dama_ke_krali_sloupec);
                    }
                    // ř. 1337 (bonus za vlastní dámu podle parity hloubky)
                    // NEPORTOVÁN - oprava (a), rozbor 3.3
                    break;

                case c_Jezdec:
                    pozicni_hodnota -= centralizacni_tabulka[pole];
                    break;

                case c_Strelec:
                    if ((pole == D6 || pole == E6) && p.sachovnice[pole + 10] == c_Pesec) {
                        pozicni_hodnota += 20;
                    }
                    ++cerni_strelci;
                    pozicni_hodnota -= centralizacni_tabulka[pole];
                    break;

                case c_Vez:
                    if (pole <= H2) ++cerne_veze_v_78r;
                    if (j >= Sloupec_C && j <= Sloupec_E) pozicni_hodnota -= 4;
                    if (stav.pescu_ve_sloupci[j].cerny == 0) {
                        pozicni_hodnota -= 7;
                        if (stav.pescu_ve_sloupci[j].bily == 0) pozicni_hodnota -= 11;
                    }
                    break;

                case c_Pesec:
                    pozicni_hodnota -= oceneni_cernych_pescu(p, stav, soucet_materialu,
                                                             pole, i, j, vyvin_cerneho);
                    ++pocet_pescu;
                    break;

                case prazdne_pole:
                    break;

                case b_Pesec:
                    pozicni_hodnota += oceneni_bilych_pescu(p, stav, soucet_materialu,
                                                            pole, i, j, vyvin_bileho);
                    ++pocet_pescu;
                    break;

                case b_Vez:
                    if (pole >= A7) ++bile_veze_v_78r;
                    if (j >= Sloupec_C && j <= Sloupec_E) pozicni_hodnota += 4;
                    if (stav.pescu_ve_sloupci[j].bily == 0) {
                        pozicni_hodnota += 7;
                        if (stav.pescu_ve_sloupci[j].cerny == 0) pozicni_hodnota += 11;
                    }
                    break;

                case b_Strelec:
                    if ((pole == D3 || pole == E3) && p.sachovnice[pole - 10] == b_Pesec) {
                        pozicni_hodnota -= 20;
                    }
                    ++bili_strelci;
                    pozicni_hodnota += centralizacni_tabulka[pole];
                    break;

                case b_Jezdec:
                    pozicni_hodnota += centralizacni_tabulka[pole];
                    break;

                case b_Dama:
                    if (vyvin_bileho < 4) {
                        if (pole > H1) pozicni_hodnota -= 15;
                    } else {
                        const int dama_ke_krali_rada =
                            std::abs((p.pozice_cerneho_krale / 10) - (pole / 10));
                        const int dama_ke_krali_sloupec =
                            std::abs((p.pozice_cerneho_krale % 10) - (pole % 10));
                        pozicni_hodnota -= 2 * (dama_ke_krali_rada + dama_ke_krali_sloupec);
                    }
                    // ř. 1414 - totéž co ř. 1337, neportováno
                    break;

                case b_Kral:
                    if (soucet_materialu < material_pro_koncovku) {
                        pozicni_hodnota += centralizacni_tabulka[pole];
                        if (std::abs(pole - p.pozice_cerneho_krale) == 20 ||
                            std::abs(pole - p.pozice_cerneho_krale) == 2) {
                            int k = 10;
                            if (soucet_materialu == 0) k = 30;
                            // p.barva je strana na tahu, tedy totéž co `strana`
                            if (p.barva == bily) pozicni_hodnota -= k;
                            else                 pozicni_hodnota += k;
                        }
                    } else {
                        // oprava (c)
                        if (bily_uz_nerochuje(p) && !bily_rochoval(p)) {
                            pozicni_hodnota -= 35;
                        }

                        pozicni_hodnota -= 4 * centralizacni_tabulka[pole];
                        for (int k = -1; k <= 1; ++k) {
                            if (p.sachovnice[pole + 10 + k] == b_Pesec) pozicni_hodnota += 15;
                            if (p.sachovnice[pole + 20 + k] == b_Pesec) pozicni_hodnota += 6;
                            if (stav.pescu_ve_sloupci[j + k].cerny == 0 &&
                                stav.vezi_ve_sloupci[j + k].cerny > 0) {
                                pozicni_hodnota -= 12;
                            }
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }

        // nedostatečný materiál (ř. 1454-1463)
        if (pocet_pescu == 0) {
            if (soucet_materialu <= hodnota_Strelce ||
                soucet_materialu == 2 * hodnota_Jezdce ||
                (soucet_materialu <= 2 * hodnota_Strelce &&
                 std::abs(p.materialova_bilance[p.hloubka]) < hodnota_Pesce)) {
                return 0;
            }
        }

        if (bili_strelci  >= 2) pozicni_hodnota += 20;
        if (cerni_strelci >= 2) pozicni_hodnota -= 20;

        if (bile_veze_v_78r > 0 && p.pozice_cerneho_krale >= A7) {
            pozicni_hodnota += 18;
            if (bile_veze_v_78r > 1) pozicni_hodnota += 25;
        }
        if (cerne_veze_v_78r > 0 && p.pozice_bileho_krale <= H2) {
            pozicni_hodnota -= 18;
            if (cerne_veze_v_78r > 1) pozicni_hodnota -= 25;
        }

        // aby si strana neblokovala pěšci vstup střelců do hry (ř. 1478-1494)
        if (p.sachovnice[C1] == b_Strelec || p.sachovnice[D2] == b_Strelec) {
            if (p.sachovnice[D2] == b_Pesec) pozicni_hodnota -= 10;
            if (p.sachovnice[E3] == b_Pesec) pozicni_hodnota -= 10;
        }
        if (p.sachovnice[F1] == b_Strelec || p.sachovnice[E2] == b_Strelec) {
            if (p.sachovnice[E2] == b_Pesec) pozicni_hodnota -= 10;
            if (p.sachovnice[D3] == b_Pesec) pozicni_hodnota -= 10;
        }
        if (p.sachovnice[C8] == c_Strelec || p.sachovnice[D7] == c_Strelec) {
            if (p.sachovnice[D7] == c_Pesec) pozicni_hodnota += 10;
            if (p.sachovnice[E6] == c_Pesec) pozicni_hodnota += 10;
        }
        if (p.sachovnice[F8] == c_Strelec || p.sachovnice[E7] == c_Strelec) {
            if (p.sachovnice[E7] == c_Pesec) pozicni_hodnota += 10;
            if (p.sachovnice[D6] == c_Pesec) pozicni_hodnota += 10;
        }

        // Poziční složka se dosud počítala z pohledu bílého (ř. 1497).
        if (strana == cerny) pozicni_hodnota = -pozicni_hodnota;

        // Zakomentované bloky ř. 1500-1535 (zbytky po pohyblivosti a bonus
        // za otevřené sloupce ve středu) se neportují.

        const int velikost = std::abs(pozicni_hodnota);
        if (velikost > stav.max_abs_pozicni_hodnota) stav.max_abs_pozicni_hodnota = velikost;
    }

    return hodnota + pozicni_hodnota;
}


// ============================================================================
//  Vlastní hledání (Krok 2/2)
// ============================================================================

namespace {

// Vrstva hledání nasype do jádra, který tah má na téhle hloubce dostat bonus
// za hlavní variantu nebo za killera. V originále si to uloz_tah bralo samo
// z globálů (ř. 746-751); tady to jádro dostává, viz PoradiTahu.
//
// Hash tah doplní commit 2/3. Přepínač pouzij_killery vypíná killery
// i hlavní variantu zároveň - test 3 je ověřuje jako jednu věc, protože obojí
// je pořadí tahů a ani jedno nesmí měnit výsledek.
void priprav_poradi(Pozice& p, const StavHledani& stav, int hash_z_pole, int hash_na_pole,
                    bool na_hlavni_variante) {
    p.poradi = PoradiTahu{};

    if (stav.nastaveni.pouzij_killery) {
        const int h = p.hloubka;

        // bod D1 Kroku 2.5: komu se bonus za hlavní variantu přidělí
        const bool pv = (stav.nastaveni.rezim_pv == RezimPV::Vzdy) ||
                        (stav.nastaveni.rezim_pv == RezimPV::JenNaHlavniVariante &&
                         na_hlavni_variante);
        if (pv) {
            p.poradi.pv_z_pole  = stav.hlavni_varianta[0][h].z_pole;
            p.poradi.pv_na_pole = stav.hlavni_varianta[0][h].na_pole;
        }
        p.poradi.k1_z_pole  = stav.killery[h].Killer1.z_pole;
        p.poradi.k1_na_pole = stav.killery[h].Killer1.na_pole;
        p.poradi.k2_z_pole  = stav.killery[h].Killer2.z_pole;
        p.poradi.k2_na_pole = stav.killery[h].Killer2.na_pole;
    }

    // bonus za tah z hash tabulky (ř. 754-761 a 801-808)
    if (hash_z_pole != 0) {
        p.poradi.hash_z_pole  = hash_z_pole;
        p.poradi.hash_na_pole = hash_na_pole;
        p.poradi.hash_platny  = true;
    }
}

// kopiruj_hlavni_variantu (ř. 1578-1589). Do řádku `hloubka` dá tenhle tah
// a za něj variantu z řádku o hloubku níž, až po ukončovací nulu.
//
// Originál žádnou mez neměl: při dost dlouhé variantě by `hloubka + i`
// přerostlo rozsah pole. Prakticky k tomu nedojde (AlfaBeta se vrací na
// hloubce max_hloubka_hledani), ale hlídat mez tím, že se na ni nikdo
// nedostane, je křehké - stejná úvaha jako u hranice_zasobniku v Kroku 1.
void kopiruj_hlavni_variantu(const Pozice& p, StavHledani& stav, int cislo_tahu) {
    const int h = p.hloubka;
    stav.hlavni_varianta[h][h].z_pole  = p.Zasobnik_tahu[cislo_tahu].z_pole;
    stav.hlavni_varianta[h][h].na_pole = p.Zasobnik_tahu[cislo_tahu].na_pole;

    int i = 0;
    while (true) {
        ++i;
        if (h + i > max_hloubka_hledani) {
            stav.hlavni_varianta[h][h + i] = z_na_typ{};   // pole je o prvek delší
            break;
        }
        stav.hlavni_varianta[h][h + i] = stav.hlavni_varianta[h + 1][h + i];
        if (stav.hlavni_varianta[h + 1][h + i].z_pole == 0) break;
    }
}

// Přerušení zvenčí (oprava 5 zadání). Originál se ptal na polohu myši
// a při přerušení vracel stromem -9999 jako by to bylo skóre (ř. 1827-1850).
// Tady se jen nastaví příznak; volající výsledek zahodí.
constexpr long kontrola_po_uzlech = 16384;   // ř. 1826

bool preruseno(StavHledani& stav) {
    if (stav.doslo_k_preruseni) return true;
    if (stav.pocet_prozkoumanych_pozic % kontrola_po_uzlech != 0) return false;

    if (stav.zastav != nullptr && stav.zastav->load(std::memory_order_relaxed)) {
        stav.doslo_k_preruseni = true;
        return true;
    }
    if (stav.cas_omezen && std::chrono::steady_clock::now() >= stav.konec_casu) {
        stav.doslo_k_preruseni = true;
        return true;
    }
    return false;
}

}  // namespace

// ============================================================================
//  Hash tabulka (Krok 2/3)
// ============================================================================

namespace {

// Nad touhle mezí se skóre považuje za matové (ř. 1689, 1887).
constexpr int mez_matu = hodnota_matu - 1000;

// Oprava 4 zadání. Skóre se stromem nese vztažené KE KOŘENI: hodnocení vrací
// hodnota_matu - hloubka, takže "mat za n půltahů od kořene" je pro celý strom
// totéž číslo. Do tabulky ale patří vzdálenost od TOHOHLE uzlu, protože týž
// záznam se přečte i z jiné hloubky - jinak by mat ve dvou z jedné větve
// vyšel jako mat v pěti z jiné.
//
// Originál to neřešil: korekci při čtení má zakomentovanou (ř. 1687), matová
// skóre proto vůbec neukládá (ř. 1887) a má k tomu obcházku s horizont = 999
// (ř. 3048-3050). Ta se neportuje.
int do_tabulky(int hodnota, int hloubka) {
    if (hodnota >  mez_matu) return hodnota + hloubka;
    if (hodnota < -mez_matu) return hodnota - hloubka;
    return hodnota;
}

int z_tabulky(int hodnota, int hloubka) {
    if (hodnota >  mez_matu) return hodnota - hloubka;
    if (hodnota < -mez_matu) return hodnota + hloubka;
    return hodnota;
}

DruhMeze druh_z_fail(int fail) {
    if (fail > 0) return DruhMeze::DolniMez;
    if (fail < 0) return DruhMeze::HorniMez;
    return DruhMeze::Presne;
}

// Záznam pro tuhle pozici, nebo nullptr.
const HashZaznam* najdi_v_hashi(const StavHledani& stav, int hloubka) {
    if (stav.trvaly == nullptr || stav.trvaly->tabulka.empty()) return nullptr;
    const HashZaznam& z = stav.trvaly->tabulka[stav.klic[hloubka] & stav.trvaly->maska];
    if (z.obsazeno == 0 || z.klic != stav.klic[hloubka]) return nullptr;
    return &z;
}

// Oprava 1 zadání, rozbor 3.1. Originál zapisoval týž záznam dvěma funkcemi,
// každou s jinou podmínkou přepisu (ř. 1889-1890), takže mohl vzniknout
// záznam s klíčem nové pozice a hodnotou staré. Tady je to jeden zápis celého
// záznamu a jedna politika: hlubší záznam se nepřepíše, stejně hluboký ano.
void zapis_do_hashe(StavHledani& stav, int hloubka, int horizont,
                    int hodnota, DruhMeze druh, int z_pole, int na_pole) {
    if (!stav.nastaveni.pouzij_hash) return;
    if (stav.trvaly == nullptr || stav.trvaly->tabulka.empty()) return;

    TrvalyStav& t = *stav.trvaly;
    HashZaznam& z = t.tabulka[stav.klic[hloubka] & t.maska];

    // Politika nahrazování (Krok 4/1). Tabulka teď přežívá mezi tahy, takže
    // se v ní hromadí hluboké záznamy z pozic, které už na desce nejsou -
    // a kdyby se nepřepisovaly, tabulka by po pár tazích přestala přijímat
    // cokoli nového. Záznam z PŘEDCHOZÍ generace se proto přepíše vždycky;
    // záznam z TÉŽE generace jen tehdy, když nový není mělčí.
    //
    // Se `stari_zaznamu = false` zbyde politika z Kroku 2, tedy jen ta druhá
    // půlka. Je to přepínač na měření, ne na hraní.
    if (z.obsazeno != 0) {
        const bool z_teze_generace = !stav.nastaveni.stari_zaznamu ||
                                     z.stari == t.generace;
        if (z_teze_generace && z.horizont > horizont) return;
    }

    z.klic     = stav.klic[hloubka];
    z.hodnota  = static_cast<int16_t>(do_tabulky(hodnota, hloubka));
    z.horizont = static_cast<int8_t>(horizont);
    z.druh     = druh;
    z.z_pole   = static_cast<uint8_t>(z_pole);
    z.na_pole  = static_cast<uint8_t>(na_pole);
    z.obsazeno = 1;
    z.stari    = t.generace;
    ++stav.hash_zapsano;
}

// urci_hash_value_z_predchozi (ř. 2864-3010). Volá se HNED po zahraj_tah,
// takže p.barva je už soupeřova a p.sachovnice odpovídá stavu po tahu.
//
// Originál tu má čtyřicet řádků zvláštních případů pro práva na rošádu
// (ř. 2916-2952) a vlastní větev pro rošádu i pro proměnu. Tady je to psané
// přímo z toho, co se na desce změnilo; výsledek je týž a test 4 to ověřuje
// v každém uzlu proti přepočtu od nuly. Doslovný přepis by nešel: záznam tahu
// má proti originálu rozdělené pole en_passant (viz Krok 1, bod 1).
void urci_klic_z_predchozi(const Pozice& p, StavHledani& stav, int cislo_tahu) {
    const int h = p.hloubka;
    const typ_tahu& tah = p.Zasobnik_tahu[cislo_tahu];
    const int z_pole  = tah.z_pole;
    const int na_pole = tah.na_pole;
    const int tahnouci = -p.barva;      // barva je po tahu už soupeřova

    uint64_t k = stav.klic[h - 1];

    // figura odchází z výchozího pole; u proměny odchází pěšec
    const int puvodni = (tah.premenena_figura != prazdne_pole)
                      ? (tahnouci * b_Pesec)
                      : p.sachovnice[na_pole];

    // Počítadlo pravidla 50 tahů (Krok 17/B). Tahle funkce je jediné místo,
    // kudy v hledání prochází každý tah (volá se hned po každém zahraj_tah),
    // a `puvodni` už drží táhnoucí figuru - u proměny pěšce. brana_figura je
    // spolehlivá i u braní mimochodem (Krok 6).
    const bool nuluje = tah.brana_figura != prazdne_pole ||
                        puvodni == b_Pesec || puvodni == -b_Pesec;
    stav.pultahy[h] = nuluje ? 0 : stav.pultahy[h - 1] + 1;
    k ^= stav.trvaly->hash_pole[z_pole][puvodni + 6];
    k ^= stav.trvaly->hash_pole[z_pole][prazdne_pole + 6];

    // a přichází na cílové, kde mohla stát braná figura
    int brana = prazdne_pole;
    if (tah.druh != Druh::EnPassant && tah.brana_figura != prazdne_pole) {
        brana = p.barva * tah.brana_figura;
    }
    k ^= stav.trvaly->hash_pole[na_pole][brana + 6];
    k ^= stav.trvaly->hash_pole[na_pole][p.sachovnice[na_pole] + 6];

    // braní mimochodem sebralo pěšce jinde, než kam se táhlo
    if (tah.druh == Druh::EnPassant) {
        k ^= stav.trvaly->hash_pole[tah.ep_brane_pole][(p.barva * b_Pesec) + 6];
        k ^= stav.trvaly->hash_pole[tah.ep_brane_pole][prazdne_pole + 6];
    }

    // rošáda přesune i věž (ř. 2955-2999)
    if (tah.druh_rosady == mala_rosada) {
        k ^= stav.trvaly->hash_pole[na_pole + 1][(tahnouci * b_Vez) + 6];
        k ^= stav.trvaly->hash_pole[na_pole + 1][prazdne_pole + 6];
        k ^= stav.trvaly->hash_pole[na_pole - 1][prazdne_pole + 6];
        k ^= stav.trvaly->hash_pole[na_pole - 1][(tahnouci * b_Vez) + 6];
    } else if (tah.druh_rosady == velka_rosada) {
        k ^= stav.trvaly->hash_pole[na_pole - 2][(tahnouci * b_Vez) + 6];
        k ^= stav.trvaly->hash_pole[na_pole - 2][prazdne_pole + 6];
        k ^= stav.trvaly->hash_pole[na_pole + 1][prazdne_pole + 6];
        k ^= stav.trvaly->hash_pole[na_pole + 1][(tahnouci * b_Vez) + 6];
    }

    k ^= stav.trvaly->hash_stav[9];              // barva na tahu se vždy otočí

    const int ep_pred = p.en_passant[h - 1];
    const int ep_ted  = p.en_passant[h];
    if (ep_pred != nelegalni) k ^= stav.trvaly->hash_stav[ep_pred % 10];
    if (ep_ted  != nelegalni) k ^= stav.trvaly->hash_stav[ep_ted  % 10];

    // Práva na rošádu. zahraj_tah zvýší pohyb[] právě u dvou polí, z_pole
    // a na_pole, takže stav před tahem se dá přesně dopočítat zpátky.
    auto pohyb_pred = [&](int pole) {
        int v = p.pohyb[pole];
        if (pole == z_pole)  --v;
        if (pole == na_pole) --v;
        return v;
    };
    auto prava = [&](bool pred) {
        uint64_t r = 0;
        const int e1 = pred ? pohyb_pred(E1) : p.pohyb[E1];
        const int h1 = pred ? pohyb_pred(H1) : p.pohyb[H1];
        const int a1 = pred ? pohyb_pred(A1) : p.pohyb[A1];
        const int e8 = pred ? pohyb_pred(E8) : p.pohyb[E8];
        const int h8 = pred ? pohyb_pred(H8) : p.pohyb[H8];
        const int a8 = pred ? pohyb_pred(A8) : p.pohyb[A8];
        if (e1 == 0 && h1 == 0) r ^= stav.trvaly->hash_stav[10];
        if (e1 == 0 && a1 == 0) r ^= stav.trvaly->hash_stav[11];
        if (e8 == 0 && h8 == 0) r ^= stav.trvaly->hash_stav[12];
        if (e8 == 0 && a8 == 0) r ^= stav.trvaly->hash_stav[13];
        return r;
    };
    k ^= prava(true) ^ prava(false);

    stav.klic[h] = k;

    // test 4 - táž metoda, jakou si autor originálu psal ručně na ř. 2004
    if (stav.nastaveni.kontroluj_hash) {
        if (urci_klic_od_nuly(p, *stav.trvaly) != k) ++stav.hash_neshod;
    }
}

}  // namespace

// --- urci_hash_value_pro_uroven (ř. 2817-2861) ------------------------------
uint64_t urci_klic_od_nuly(const Pozice& p, const TrvalyStav& trvaly) {
    uint64_t k = 0;

    // Originál jede y = 0..119 včetně okrajů, jenže okraj má hodnotu 100
    // a index do tabulky je pak 106 - dávno za koncem řádku o třinácti
    // prvcích. Kontroly rozsahu jsou v .cfg vypnuté, takže to tiše čte cizí
    // paměť. Tady se okraje přeskakují: jsou pořád stejné, takže by do klíče
    // přispěly jen konstantou, která se stejně vyruší.
    for (int y = 0; y <= velikost_sachovnice_10x12; ++y) {
        const int f = p.sachovnice[y];
        if (f == hranice_sachovnice) continue;
        k ^= trvaly.hash_pole[y][f + 6];
    }

    const int ep = p.en_passant[p.hloubka];
    if (ep != nelegalni) k ^= trvaly.hash_stav[ep % 10];

    if (p.barva == bily) k ^= trvaly.hash_stav[9];

    if (p.pohyb[E1] == 0 && p.pohyb[H1] == 0) k ^= trvaly.hash_stav[10];
    if (p.pohyb[E1] == 0 && p.pohyb[A1] == 0) k ^= trvaly.hash_stav[11];
    if (p.pohyb[E8] == 0 && p.pohyb[H8] == 0) k ^= trvaly.hash_stav[12];
    if (p.pohyb[E8] == 0 && p.pohyb[A8] == 0) k ^= trvaly.hash_stav[13];

    return k;
}

// --- založení a údržba trvalého stavu (Krok 4/1) ----------------------------
// Zobristova čísla (ř. 2245-2256) a vymaz_hash_tabulku (ř. 3012-3034).
//
// Do Kroku 4 to byla jedna funkce, kterou si `hledej` volalo při každém
// volání - tabulka se tím po každém tahu zahodila. Teď je to rozdělené podle
// ŽIVOTNOSTI: čísla a alokace jednou za život motoru, vynulování při nové
// partii, a při každém `go` už z tabulky nic.
namespace {

// Počet záznamů jako mocnina dvojky odvozená z MB. Originál měl napevno
// 16 * 65536 záznamů po osmi intech, tedy 32 MB - na telefon moc.
size_t pocet_zaznamu(int velikost_mb) {
    const int mb = (velikost_mb < 1) ? 1 : velikost_mb;
    const size_t bajtu = static_cast<size_t>(mb) * 1024u * 1024u;
    size_t pocet = 1;
    while (pocet * 2 * sizeof(HashZaznam) <= bajtu) pocet *= 2;
    return pocet;
}

}  // namespace

void zaloz_motor(TrvalyStav& trvaly, int velikost_mb) {
    if (!trvaly.zalozeno) {
        // Pevné semínko. Originál nevolá Randomize, takže má klíče při každém
        // spuštění stejné; pro testy je ta opakovatelnost výhoda a zachovává
        // se. Generuje se to JEDNOU - dokud v tabulce leží záznamy, nesmějí
        // se čísla změnit, každý uložený klíč by pak ukazoval jinam.
        std::mt19937_64 rng(20030408ull);
        for (int y = 0; y <= velikost_sachovnice_10x12; ++y) {
            for (int i = 0; i < 13; ++i) trvaly.hash_pole[y][i] = rng();
        }
        for (int i = 0; i < 14; ++i) trvaly.hash_stav[i] = rng();
        trvaly.zalozeno = true;
    }

    // Idempotentní: táž velikost neudělá nic, takže se obsah tabulky
    // neztratí. Jiná velikost znamená realokaci.
    if (trvaly.velikost_mb != velikost_mb || trvaly.tabulka.empty()) {
        zmen_velikost_hashe(trvaly, velikost_mb);
    }
}

void vymaz_hash(TrvalyStav& trvaly) {
    // Oprava 2 zadání Kroku 2: originál maže `for i := 0 to rozsah_hash`,
    // tedy o prvek za konec pole (rozbor 3.2).
    std::fill(trvaly.tabulka.begin(), trvaly.tabulka.end(), HashZaznam{});
    trvaly.generace = 0;
}

void zmen_velikost_hashe(TrvalyStav& trvaly, int velikost_mb) {
    const size_t pocet = pocet_zaznamu(velikost_mb);
    trvaly.tabulka.assign(pocet, HashZaznam{});
    trvaly.maska = pocet - 1;
    trvaly.velikost_mb = velikost_mb;
    trvaly.generace = 0;
}

// --- dalsi_nejlepsi_tah (ř. 1610-1625) --------------------------------------
// Lineární výběr maxima, žádné setřídění dopředu - tak to má originál
// a zadání to výslovně nechává být, dokud se nezměří, že to vadí.
int dalsi_nejlepsi_tah(Pozice& p) {
    int lepsi_tah = -1;
    int lepsi_hodnota = -hodnota_matu;

    for (int i = p.hranice_zasobniku[p.hloubka]; i < p.hranice_zasobniku[p.hloubka + 1]; ++i) {
        if (p.Zasobnik_tahu[i].hodnota > lepsi_hodnota) {
            lepsi_tah = i;
            lepsi_hodnota = p.Zasobnik_tahu[i].hodnota;
        }
    }

    if (lepsi_tah >= 0) p.Zasobnik_tahu[lepsi_tah].hodnota = -hodnota_matu;
    return lepsi_tah;
}

// --- opakování pozice (Krok 4/2) --------------------------------------------
//
// Do Kroku 4/2 to byl jediný řádek: `h >= 4 && klic[h] == klic[h - 4]`. Tedy
// jeden jediný bod čtyři půltahy zpátky, a jen uvnitř hledání - historii
// partie před kořenem motor nevidel vůbec. Stálo to skoro polovinu partií:
// ve velkém zápase skončilo 45 z 98 partií trojím opakováním, protože strana,
// která vedla, neviděla na opakujícím tahu nic špatného.
//
// Teď se klíč uzlu porovnává se VŠÍM, co je za ním:
//   (a) s cestou od kořene, tedy klic[0 .. h-1], a
//   (b) s klíči odehrané partie.
//
// VĚDOMÉ ZJEDNODUŠENÍ: stačí JEDNA shoda a vrací se 0. Formálně je remíza až
// při trojím opakování, ale takhle to dělá většina motorů a je to přesně to,
// co odstraní přeskupování - strana, která vede, uvidí u opakujícího tahu
// nulu místo výhry a nezahraje ho. Kdyby se čekalo na třetí výskyt, motor by
// první dva zahrál klidně dál a k ničemu by to nebylo.
//
// POZOR NA KOŘEN. Historie končí klíčem kořenové pozice (vložil se tam, když
// se zahrál poslední tah partie), takže v kořeni by se pozice shodla sama se
// sebou a motor by okamžitě hlásil remízu. Test se proto dělá jen při h >= 1.
// Tam je porovnání s kořenem naopak správně: vrátit se po dvou tazích do
// kořenové pozice opakování je.
bool opakovani(const StavHledani& stav, int h) {
    if (h < 1) return false;

    if (!stav.nastaveni.opakovani_pres_partii) {
        // Chování do Kroku 4/2: jediný bod čtyři půltahy zpátky, historie
        // partie se neuvažuje. Přepínač je na měření, ne na hraní.
        return h >= 4 && stav.klic[h] == stav.klic[h - 4];
    }

    const uint64_t k = stav.klic[h];

    // Historie a cesta od kořene se čtou jako JEDNA posloupnost, protože
    // historie končí týmž klíčem, jakým cesta začíná:
    //     historie[0 .. m-1], pak klic[1 .. h]
    // Uzel sám leží na kombinovaném indexu `zaklad + h`.
    const int m = (stav.historie != nullptr)
                ? static_cast<int>(stav.historie->size()) : 0;
    const int zaklad = (m > 0) ? (m - 1) : 0;

    // O dva půltahy zpátky se pozice zopakovat nemůže - obě strany musely
    // táhnout a tah pozici vždycky změní. Začíná se proto o čtyři a jde se po
    // dvou: jen pozice s touž stranou na tahu mají šanci se shodovat, protože
    // barva na tahu je v klíči.
    for (int i = zaklad + h - 4; i >= 0; i -= 2) {
        const uint64_t kk = (i >= zaklad) ? stav.klic[i - zaklad]
                                          : (*stav.historie)[i];
        if (kk == k) return true;
    }
    return false;
}

// --- AlfaBeta (ř. 1628-1911) ------------------------------------------------
int AlfaBeta(Pozice& p, int Alfa, int Beta, int horizont, bool null_move,
             bool na_hlavni_variante, StavHledani& stav, bool* pozice_nelegalni) {
    (void)null_move;   // null move přijde v commitu 2/4

    if (pozice_nelegalni != nullptr) *pozice_nelegalni = false;

    ++stav.pocet_prozkoumanych_pozic;

    const int h = p.hloubka;
    stav.hlavni_varianta[h][h] = z_na_typ{};        // ř. 1644

    int hodnota = oceneni_pozice(p, Alfa, Beta, p.barva, stav);

    // V pozici lze sebrat soupeřova krále, předchozí tah byl tedy nelegální.
    // Originál to poznával podle skóre (ř. 1665); tady podle příznaku, viz
    // StavHledani::lze_sebrat_krale. Vrací se totéž a na témž místě: řádky
    // 1657-1663 by sice ještě zvedly horizont, ale ř. 1665 se vrátí dřív,
    // než se ten horizont k čemukoli použije.
    if (stav.lze_sebrat_krale) {
        if (pozice_nelegalni != nullptr) *pozice_nelegalni = true;
        return hodnota;
    }

    const int sach = stav.strana_na_tahu_v_sachu ? 1 : 0;

    // Remíza opakováním (ř. 1649), od Kroku 4/2 přes celou partii - viz
    // `opakovani` výš.
    //
    // Nezávisí to na tom, jestli je zapnutá hash tabulka: klíče se počítají
    // vždycky. Kdyby to na ní záviselo, test 1 by porovnával dvě různá hledání.
    //
    // Proti originálu se test dělá až za návratem u nelegální pozice. Tam by
    // to stejně nedávalo smysl a takhle se nemůže stát, že se nelegální pozice
    // ohlásí jako remíza.
    //
    // DO HASH TABULKY SE TA NULA NESMÍ ULOŽIT: patří CESTĚ, ne pozici. Táž
    // pozice dosažená jinudy se neopakuje a nula by pro ni byla lež - otrávilo
    // by to tabulku přesně tím způsobem, proti kterému stojí zásada z Kroku 2,
    // že hodnocení musí být funkcí hašované pozice. Drží to tenhle `return`:
    // je PŘED čtením z tabulky i před jakýmkoli zapis_do_hashe, takže se
    // z tohohle uzlu neuloží nic.
    if (opakovani(stav, h)) return 0;

    // Remíza podle pravidla 50 tahů (Krok 17/B), hned vedle opakování a ze
    // stejných důvodů PŘED tabulkou: ta nula patří CESTĚ, klíč hash tabulky
    // počítadlo nenese, a tak se nesmí dostat do tabulky. Tenhle `return` je
    // před čtením i před jakýmkoli zápisem.
    //
    // `sach == 0` dává přednost matu (FIDE 9.6.2, 5.1.1): strana v šachu se
    // prohledá dál - nemá-li tah, je to mat; má-li, vrátí jí nulu dítě.
    // Legální tahy se tedy zvlášť generovat nemusí. h >= 1: kořen se
    // nevyhodnocuje, UCI musí vrátit bestmove a UI v koncové pozici
    // hledání vůbec nespustí.
    //
    // VĚDOMÁ NEPŘESNOST: klíč počítadlo nenese, takže záznam z pozice
    // s nízkým počítadlem se může použít i při vysokém. Má to většina motorů;
    // neopravuje se, viz POZNAMKY-REDMAX.md.
    if (stav.nastaveni.pravidlo_50 && h >= 1 && stav.pultahy[h] >= 100 && sach == 0) return 0;

    // prodloužení při šachu a při braní na stejném poli (ř. 1657-1663)
    if (horizont >= 0 &&
        ((sach == pravda && h + horizont < stav.max_rozsireni_hloubky + 9) ||
         (h >= 2 && h + horizont < stav.max_rozsireni_hloubky &&
          p.cilove_pole[h] == p.cilove_pole[h - 1] &&
          hodnota >= Alfa - 150 && hodnota <= Beta + 150))) {
        ++horizont;
    }

    if (horizont < -7 || h >= max_hloubka_hledani) return hodnota;

    // stání na místě v hledání klidu (ř. 1670)
    if (hodnota >= Beta && horizont + sach <= 1) return hodnota;

    // --- čtení z hash tabulky (ř. 1676-1741) --------------------------------
    const HashZaznam* zaznam = stav.nastaveni.pouzij_hash ? najdi_v_hashi(stav, h) : nullptr;

    // Tah ze záznamu se opíše hned: null move níž může do téhož místa tabulky
    // zapsat, a ukazatel by pak ukazoval na cizí záznam.
    int hash_z_pole = 0, hash_na_pole = 0;
    if (zaznam != nullptr) {
        hash_z_pole  = zaznam->z_pole;
        hash_na_pole = zaznam->na_pole;
    }

    if (zaznam != nullptr && horizont > 0 && zaznam->horizont >= horizont) {
        const int hod = z_tabulky(zaznam->hodnota, h);
        if (zaznam->druh == DruhMeze::Presne) {
            ++stav.hash_nalezeno;
            return hod;
        }
        // fail-high: stačí, když hodnota beztak odsekne
        if (zaznam->druh == DruhMeze::DolniMez && hod >= Beta) {
            ++stav.hash_nalezeno;
            return hod;
        }
        // fail-low: stačí, když je hodnota stejně pod oknem
        if (zaznam->druh == DruhMeze::HorniMez && hod <= Alfa) {
            ++stav.hash_nalezeno;
            return hod;
        }
    }

    // --- null move (ř. 1744-1788) -------------------------------------------
    //
    // Nechá soupeře táhnout dvakrát za sebou. Když ani tak neprorazí, je
    // pozice tak dobrá, že se dá odseknout. R = 2, materiálová podmínka
    // > 400 na OBOU stranách (v koncovce se null move plete kvůli zugzwangu),
    // ne hned od kořene, ne v hledání klidu a ne v šachu.
    if (stav.nastaveni.pouzij_null_move &&
        !null_move &&
        h > 1 &&
        horizont > 0 &&
        (p.celkovy_material_na_sachovnici[h] + p.materialova_bilance[h]) / 2 > 400 &&
        (p.celkovy_material_na_sachovnici[h] - p.materialova_bilance[h]) / 2 > 400 &&
        sach == nepravda) {

        // zahraj nulový tah (ř. 1754-1761)
        ++p.hloubka;
        p.cilove_pole[p.hloubka] = 0;
        p.en_passant[p.hloubka]  = nelegalni;
        p.materialova_bilance[p.hloubka] = -p.materialova_bilance[p.hloubka - 1];
        p.celkovy_material_na_sachovnici[p.hloubka] =
            p.celkovy_material_na_sachovnici[p.hloubka - 1];
        p.barva = -p.barva;

        // Hranice zásobníku se předá "nazpátek" (ř. 1761). Vypadá to jako
        // přepis rodičova seznamu tahů, ale je to správně: null move se řeší
        // ještě před Generuj_tahy níž, takže rodič svoje tahy ještě nemá
        // a nulový potomek si jimi jen půjčí místo.
        p.hranice_zasobniku[p.hloubka] = p.hranice_zasobniku[p.hloubka - 1];

        // klíč: otočit barvu a zapomenout případné braní mimochodem (ř. 1764-1771)
        uint64_t k = stav.klic[p.hloubka - 1] ^ stav.trvaly->hash_stav[9];
        if (p.en_passant[p.hloubka - 1] != nelegalni) {
            k ^= stav.trvaly->hash_stav[p.en_passant[p.hloubka - 1] % 10];
        }
        stav.klic[p.hloubka] = k;
        // Nulový tah je tichý půltah - nic nebere a pěšec netáhne.
        stav.pultahy[p.hloubka] = stav.pultahy[p.hloubka - 1] + 1;

        // Nulový tah není první tah uzlu, takže hlavní variantu opouští.
        const int null_hodnota =
            -AlfaBeta(p, -Beta, -Beta + 1, horizont - 1 - 2, true, false, stav);

        // vrať nulový tah zpět (ř. 1778-1780)
        p.barva = -p.barva;
        --p.hloubka;

        if (stav.doslo_k_preruseni) return hodnota;   // volající to zahodí

        if (null_hodnota >= Beta) return null_hodnota;
    }

    priprav_poradi(p, stav, hash_z_pole, hash_na_pole, na_hlavni_variante);
    Generuj_tahy(p, horizont);    // horizont <= 0 znamená jen braní

    // ř. 1793-1797: v hledání klidu se vychází z hodnoty pozice, protože
    // stát na místě je taky možnost
    int lepsi_hodnota = (horizont > 0) ? -hodnota_matu : hodnota;

    bool nalezen_legalni_tah = false;
    bool odseknuto = false;
    int  tah_odseknuti = -1;

    // co se nakonec uloží do hash tabulky (ř. 1800-1802)
    int pom_hodnota = -hodnota_matu;
    int pom_fail    = -1;      // -1 horní mez, 0 přesně, 1 dolní mez
    int pom_tah     = -1;

    // Hlavní varianta padá v okamžiku, kdy se v uzlu vezme jiný než první tah
    // (bod D1 Kroku 2.5). "První" je ten, který vybral dalsi_nejlepsi_tah
    // napoprvé, tedy tah s nejvyšší hodnotou pořadí.
    bool prvni_tah = true;

    int i = dalsi_nejlepsi_tah(p);
    while (i >= 0) {
        zahraj_tah(p, i);
        urci_klic_z_predchozi(p, stav, i);      // ř. 1810
        bool tah_byl_nelegalni = false;
        hodnota = -AlfaBeta(p, -Beta, -Alfa, horizont - 1, false,
                            na_hlavni_variante && prvni_tah, stav, &tah_byl_nelegalni);
        vezmi_tah_zpet(p, i);
        prvni_tah = false;

        if (!tah_byl_nelegalni) nalezen_legalni_tah = true;

        if (preruseno(stav)) return lepsi_hodnota;   // volající to zahodí

        if (hodnota > lepsi_hodnota) {
            lepsi_hodnota = hodnota;
            if (hodnota >= Beta) {
                kopiruj_hlavni_variantu(p, stav, i);
                pom_hodnota = hodnota;
                pom_fail = 1;
                pom_tah = i;
                odseknuto = true;
                tah_odseknuti = i;
                break;
            }
            if (hodnota > Alfa) {
                kopiruj_hlavni_variantu(p, stav, i);
                pom_hodnota = hodnota;
                pom_fail = 0;
                pom_tah = i;
                Alfa = hodnota;
            } else if (hodnota > pom_hodnota) {
                pom_hodnota = hodnota;
                pom_fail = -1;
                pom_tah = i;
            }
        } else if (hodnota > pom_hodnota) {
            // ošetření kvůli hledání klidu (ř. 1876-1880)
            pom_hodnota = hodnota;
            pom_fail = -1;
            pom_tah = i;
        }

        i = dalsi_nejlepsi_tah(p);
    }

    // --- zápis do hash tabulky (ř. 1886-1893) -------------------------------
    //
    // Originál ukládal jen při horizont > 2 a jen nematové hodnoty; matové
    // vynechával, protože neuměl převést vzdálenost (rozbor 3.7). Tady se
    // ukládají i matové, převedené v do_tabulky (oprava 4).
    //
    // Strop `horizont > 2` je teď Nastaveni::min_horizont_pro_hash, aby se dal
    // změřit - bod D2 Kroku 2.5. Mělkých uzlů je nejvíc a originál je neuloží
    // nikdy.
    //
    // Fail-low záznamy (pom_fail = -1) originál počítá, ale zahodí (ř. 1888,
    // rozbor 3.5). Zůstává to tak, dokud se nezměří, že zapnutí pomůže -
    // bod 8 zadání. Přepínač je Nastaveni::ukladej_fail_low.
    if (horizont >= stav.nastaveni.min_horizont_pro_hash &&
        (pom_fail > -1 || stav.nastaveni.ukladej_fail_low)) {
        zapis_do_hashe(stav, h, horizont, pom_hodnota, druh_z_fail(pom_fail),
                       pom_tah >= 0 ? p.Zasobnik_tahu[pom_tah].z_pole : 0,
                       pom_tah >= 0 ? p.Zasobnik_tahu[pom_tah].na_pole : 0);
    }

    // killery (ř. 1895-1899)
    //
    // Originál je ukládá u každého tahu, který odsekl, tedy i u braní.
    // Běžně se to dělá jen u tichých tahů, ale to je změna pořadí tahů,
    // ne oprava - zadání pořadí tahů výslovně nechává být. Kdyby se to někdy
    // měnilo, spolehlivý test na braní je brana_figura != prazdne_pole,
    // ne Druh::Brani (braná proměna má Druh::Promena).
    if (odseknuto) {
        stav.killery[h].Killer2 = stav.killery[h].Killer1;
        stav.killery[h].Killer1.z_pole  = p.Zasobnik_tahu[tah_odseknuti].z_pole;
        stav.killery[h].Killer1.na_pole = p.Zasobnik_tahu[tah_odseknuti].na_pole;
    }

    // Mat a pat (ř. 1901-1907, oprava 4 zadání).
    //
    // Originál to poznával podle toho, že lepsi_hodnota vyšla přesně
    // -(hodnota_matu - (hloubka + 1)), tedy podle skóre. Tady se počítá, jestli
    // se našel aspoň jeden legální tah, takže detekce nezávisí na tom, jak se
    // matové hodnoty ukládají. Rozdíl je i věcný: pozice, ve které strana na
    // tahu nemá ani jeden PSEUDOLEGÁLNÍ tah, vycházela v originále jako
    // -hodnota_matu, tedy jako mat, i když šlo o pat.
    //
    // V hledání klidu se negenerují tiché tahy, takže "žádný legální tah"
    // tam nic neznamená - proto jen pro horizont > 0.
    //
    // RedCry krok M1: mat a pat jen tehdy, když se prošly všechny tahy.
    // Nelegální tah vrací -(hodnota_matu - (h + 1)) a v okně u matové meze
    // (Beta <= tato hodnota) jím smyčka odsekne dřív, než dojde na legální
    // tah. nalezen_legalni_tah pak zůstal false a uzel se vyložil jako mat
    // (nebo pat mimo šach) a uložil do trvalé tabulky jako PŘESNÁ hodnota.
    // Další hledání jí věřilo: v partii s Chessmasterem 2100 RedMAX zahrál
    // 29. Ne7+ místo 29. Qxg8#, protože pozici po Ne7+ četl z tabulky jako
    // mat černého (docs/POZNAMKY-REDMAX.md, Krok M1).
    //
    // Po odseknutí se vrací lepsi_hodnota jako dolní mez. To je pravda -
    // horší než "matován hned" uzel být nemůže - a stejně to dělá originál.
    // Oprava zároveň zavírá patovou variantu téže chyby, kterou má originál
    // na ř. 1901-1907 (odseknutí mimo šach vrátilo 0 a zapsalo ji).
    if (horizont > 0 && !nalezen_legalni_tah && !odseknuto) {
        const int konec = (sach == nepravda) ? 0                        // pat
                                             : -(hodnota_matu - (h + 1));  // mat
        // Je to konečná hodnota pozice, ne mez, takže se uloží jako přesná.
        // Originál na ř. 1904 volal zápis s tahem, který nemusel existovat.
        zapis_do_hashe(stav, h, horizont, konec, DruhMeze::Presne, 0, 0);
        return konec;
    }

    return lepsi_hodnota;
}

// --- prvni_tah_alfa_beta (ř. 1941-2133) -------------------------------------
Vysledek hledej(Pozice& p, const Nastaveni& n, std::atomic<bool>& zastav,
                TrvalyStav& trvaly, const std::vector<uint64_t>* historie) {
    Vysledek v;

    auto stav = std::make_unique<StavHledani>();
    stav->nastaveni = n;
    stav->zastav = &zastav;
    if (n.cas_ms > 0) {
        stav->cas_omezen = true;
        stav->konec_casu = std::chrono::steady_clock::now() + std::chrono::milliseconds(n.cas_ms);
    }

    // Trvalý stav (Krok 4/1). Založení je idempotentní, takže tabulka i čísla
    // přežijí z minulého `go`; realokuje se jen při změně velikosti.
    zaloz_motor(trvaly, n.velikost_hash_mb);

    // Přepínač na měření: `false` vrátí chování před Krokem 4, tedy prázdná
    // tabulka na začátku každého hledání.
    if (!n.trvala_hash) vymaz_hash(trvaly);

    // Nová generace. Podle ní politika nahrazování pozná záznam z pozice,
    // která už na desce není. Přetečení uint8_t nevadí: záleží jen na tom,
    // jestli se stáří rovná, a dvě stě padesát šest tahů stará hodnota už
    // v tabulce stejně neleží.
    ++trvaly.generace;
    stav->trvaly = &trvaly;
    stav->historie = historie;

    // Kořenové tahy si vygeneruje hledání samo. V originále je měl připravené
    // GUI a prvni_tah_alfa_beta se jen podíval na hranice_zasobniku[1]
    // (ř. 1953).
    p.hranice_zasobniku[p.hloubka] = p.Index;
    stav->klic[p.hloubka] = urci_klic_od_nuly(p, trvaly);   // ř. 1963
    stav->pultahy[p.hloubka] = p.pultahy_bez_zmeny;         // Krok 17/B
    priprav_poradi(p, *stav, 0, 0, true);   // kořen na hlavní variantě leží
    Generuj_tahy(p, 1);

    const int prvni_tah = p.hranice_zasobniku[p.hloubka];
    const int za_poslednim = p.hranice_zasobniku[p.hloubka + 1];
    const int pocet_tahu = za_poslednim - prvni_tah;
    if (pocet_tahu <= 0) return v;      // tah_nalezen zůstává false

    const int max_hloubka = (n.max_hloubka < 1) ? 1
                          : (n.max_hloubka > max_hloubka_hledani ? max_hloubka_hledani
                                                                 : n.max_hloubka);

    int Alfa = 0, Beta = 0;
    bool byl_legalni_tah = false;

    // Hlavní varianta nejlepšího KOŘENOVÉHO tahu.
    //
    // Opisuje se hned, jak se takový tah najde, ne až po smyčce: ř. 1988
    // (`hlavni_varianta[hloubka][hloubka] = {}` před každým kořenovým tahem)
    // ji zase vynuluje, takže po doběhnutí smyčky v tabulce zbyde varianta
    // jen tehdy, když byl nejlepší náhodou ten poslední tah. Do Kroku 3 se to
    // neprojevilo - testy Kroku 2 četly jen nejlepsi_tah.
    std::vector<typ_tahu> pv_nejlepsiho;

    auto opis_variantu = [&](int cislo_tahu) {
        pv_nejlepsiho.clear();
        for (int k = 0; k <= max_hloubka_hledani; ++k) {
            if (stav->hlavni_varianta[0][k].z_pole == 0) break;
            typ_tahu t{};
            if (k == 0) {
                t = p.Zasobnik_tahu[cislo_tahu];   // celý záznam, tedy i s proměnou
            } else {
                t.z_pole  = stav->hlavni_varianta[0][k].z_pole;
                t.na_pole = stav->hlavni_varianta[0][k].na_pole;
            }
            pv_nejlepsiho.push_back(t);
        }
    };

    for (int horizont = 1; horizont <= max_hloubka; ++horizont) {
        // aspirační okno (ř. 1967-1973)
        if (horizont == 1) {
            Alfa = -hodnota_matu;
            Beta =  hodnota_matu;
        } else {
            Beta = Alfa + 30;
            Alfa = Alfa - 30;
        }
        stav->max_rozsireni_hloubky = horizont + 3;

        bool horizont_dokoncen = true;
        byl_legalni_tah = false;

        for (int poradi = 0; poradi < pocet_tahu; ++poradi) {
            const int i = prvni_tah + poradi;
            stav->hlavni_varianta[p.hloubka][p.hloubka] = z_na_typ{};   // ř. 1988

            bool nelegalni = false;
            zahraj_tah(p, i);
            urci_klic_z_predchozi(p, *stav, i);     // ř. 2004
            int hodnota = -AlfaBeta(p, -Beta, -Alfa, horizont - 1, false,
                                    poradi == 0, *stav, &nelegalni);
            vezmi_tah_zpet(p, i);
            if (!nelegalni) byl_legalni_tah = true;

            if (preruseno(*stav)) { horizont_dokoncen = false; break; }

            if (poradi == 0) {
                // první tah se hledá naostro; okno se otevře, když vypadne ven
                if (hodnota < Alfa) {
                    Alfa = -hodnota_matu;
                    Beta = hodnota;
                    zahraj_tah(p, i);
                    urci_klic_z_predchozi(p, *stav, i);
                    hodnota = -AlfaBeta(p, -Beta, -Alfa, horizont - 1, false, true, *stav);
                    vezmi_tah_zpet(p, i);
                    if (preruseno(*stav)) { horizont_dokoncen = false; break; }
                } else if (hodnota >= Beta) {
                    Alfa = hodnota;
                    Beta = hodnota_matu;
                    zahraj_tah(p, i);
                    urci_klic_z_predchozi(p, *stav, i);
                    hodnota = -AlfaBeta(p, -Beta, -Alfa, horizont - 1, false, true, *stav);
                    vezmi_tah_zpet(p, i);
                    if (preruseno(*stav)) { horizont_dokoncen = false; break; }
                }

                Alfa = hodnota;
                Beta = Alfa + 1;            // nulové okno pro zbytek (ř. 2036)
                kopiruj_hlavni_variantu(p, *stav, i);
                opis_variantu(i);

            } else if (hodnota > Alfa) {
                // zjištěn lepší tah, nutno znovu projet s otevřeným oknem
                const int stara_alfa = Alfa;
                Alfa = hodnota;
                Beta = hodnota_matu;
                zahraj_tah(p, i);
                urci_klic_z_predchozi(p, *stav, i);
                hodnota = -AlfaBeta(p, -Beta, -Alfa, horizont - 1, false, false, *stav);
                vezmi_tah_zpet(p, i);
                if (preruseno(*stav)) { horizont_dokoncen = false; break; }

                if (hodnota > stara_alfa) {
                    Alfa = hodnota;
                    Beta = Alfa + 1;
                    kopiruj_hlavni_variantu(p, *stav, i);
                    opis_variantu(i);

                    // lepší tah na začátek zásobníku (ř. 2086-2090)
                    const typ_tahu tmp = p.Zasobnik_tahu[i];
                    for (int j = i; j > prvni_tah; --j) {
                        p.Zasobnik_tahu[j] = p.Zasobnik_tahu[j - 1];
                    }
                    p.Zasobnik_tahu[prvni_tah] = tmp;
                } else {
                    Alfa = stara_alfa;
                    Beta = Alfa + 1;
                }
            }
        }

        if (!horizont_dokoncen) break;      // nedokončený horizont se zahazuje

        // dokončený horizont je výsledek
        v.tah_nalezen = byl_legalni_tah;
        v.nejlepsi_tah = p.Zasobnik_tahu[prvni_tah];
        v.hodnota = Alfa;
        v.dosazena_hloubka = horizont;

        // Dokončený horizont teprve vydává variantu ven; nedokončený se
        // zahazuje celý, takže se `v.hlavni_varianta` nesmí přepsat dřív.
        v.hlavni_varianta = pv_nejlepsiho;

        // Průběžné hlášení. Až TADY, tedy až je `v` celé vyplněné - volaný
        // se na ně dívá jako na hotový výsledek téhle hloubky.
        if (n.hlaseni != nullptr) {
            PrubeznyVysledek pv;
            pv.hloubka       = horizont;
            pv.hodnota       = v.hodnota;
            pv.uzlu          = stav->pocet_prozkoumanych_pozic;
            pv.nejlepsi_tah  = v.nejlepsi_tah;
            pv.tah_nalezen   = v.tah_nalezen;
            n.hlaseni(n.hlaseni_kontext, pv);
        }

        // nalezený mat neprodlužovat (ř. 2101)
        if (Alfa > 9000 || Alfa < -9000) break;
    }

    // Mat nebo pat v kořeni: žádný kořenový tah nebyl legální. Originál to
    // neřešil - vrátil by první tah ze zásobníku, i když je nelegální.
    if (!byl_legalni_tah && !stav->doslo_k_preruseni) {
        const int kral = (p.barva == bily) ? p.pozice_bileho_krale : p.pozice_cerneho_krale;
        const bool v_sachu = napadene_pole(p, kral, -p.barva);
        v.tah_nalezen = false;
        v.hlavni_varianta.clear();
        v.hodnota = v_sachu ? -hodnota_matu : 0;
    }

    v.preruseno = stav->doslo_k_preruseni;
    v.uzlu = stav->pocet_prozkoumanych_pozic;
    v.max_abs_pozicni_hodnota = stav->max_abs_pozicni_hodnota;
    v.bez_pozicnich_kriterii = stav->bez_pozicnich_kriterii;
    v.s_pozicnimi_kriterii = stav->s_pozicnimi_kriterii;
    v.hash_nalezeno = stav->hash_nalezeno;
    v.hash_zapsano = stav->hash_zapsano;
    v.hash_neshod = stav->hash_neshod;
    return v;
}

// Pohodlná varianta: vlastní dočasný trvalý stav, tedy tabulka jen na tohle
// jedno hledání - jako před Krokem 4. Používají ji měření a ty testy, kterým
// jde o hledání samo, ne o partii.
//
// POZOR, není to zadarmo: alokuje a nuluje se celá tabulka, u výchozích
// čtyř megabajtů tedy 262 144 záznamů. Kdo volá hledej v cyklu, ať si trvalý
// stav drží sám.
Vysledek hledej(Pozice& p, const Nastaveni& n, std::atomic<bool>& zastav) {
    TrvalyStav trvaly;
    return hledej(p, n, zastav, trvaly);
}

}  // namespace redmax
