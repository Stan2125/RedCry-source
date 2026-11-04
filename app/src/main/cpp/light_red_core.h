// ---------------------------------------------------------------------------
//  Light Red - jádro šachového enginu
//
//  Přepis z Light_Red_211_2_f.pas (Delphi / VCL) do prostého C++17.
//  Zdrojové názvy jsou zachovány česky a co nejblíže originálu, aby šlo
//  kterýkoli řádek dohledat v Pascalu.
//
//  Odstraněno: veškerá VCL (TForm1, Label, Gauge, TImage, TPoint),
//              kreslení, PGN, menu, knížka zahájení, hledání.
//  Zachováno:  reprezentace pozice, generátor tahů, provedení tahu,
//              detekce napadení, filtr nelegálních tahů, mat/pat.
//
//  Indexy polí zůstávají 1..8 jako v Pascalu; pole jsou [9][9] a index 0
//  se nepoužívá. Přečíslování na 0..7 by bylo nejrychlejší způsob, jak si
//  do portu zanést chybu.
// ---------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace lightred {

// --- typ_figura -------------------------------------------------------------
enum Figura : uint8_t { ZADNA = 0, PESEC, JEZDEC, STRELEC, VEZ, DAMA, KRAL };

// --- typ_strana -------------------------------------------------------------
enum Strana : uint8_t { BILY = 0, CERNY, NIKDO };

// --- typ_vysledku_prubeh ----------------------------------------------------
enum Prubeh : uint8_t { NORMALNI = 0, MAT, PAT };

// --- typ_sachovnice ---------------------------------------------------------
struct Policko {
    Strana strana = NIKDO;
    Figura figura = ZADNA;
};

// náhrada za TPoint
struct Bod {
    int x = 0;
    int y = 0;
};

// --- typ_vysledku_figura ----------------------------------------------------
struct Tah {
    Bod    z_pole{};
    Bod    na_pole{};
    Figura promen_pesce = ZADNA;   // v originále se u nepromenných tahů
                                   // nenastavovalo vůbec - viz POZNAMKY-LIGHTRED.md
    double trideni   = 0.0;
    double lepsi_tah = 0.0;
};

// Originál měl tah[1..100]. V pozicích s mnoha figurami může počet
// pseudolegálních tahů 100 přesáhnout (teoretické maximum legálních je 218),
// takže je pole zvětšené. Viz POZNAMKY-LIGHTRED.md, bod 4.
constexpr int MAX_TAHU = 256;

// --- pozice_typ (jeden prvek) -----------------------------------------------
struct Pozice {
    Policko sachovnice[9][9]{};    // [x][y], 1..8

    Strana na_tahu_je = BILY;

    bool r_Bm = false;             // bílý malá rošáda
    bool r_Bv = false;             // bílý velká
    bool r_Cm = false;             // černý malá
    bool r_Cv = false;             // černý velká

    Bod en_passant{0, 0};          // 0,0 = žádné

    Tah tahy[MAX_TAHU]{};
    int pocet_moznych_tahu = 0;

    Prubeh stav = NORMALNI;

    // co poslední zahraný tah sebral - inkrementální hodnocení
    double co_schramstne = 0.0;

    // Počítadla z FEN. Hledání je nepoužívá, ale UI a pravidlo o padesáti
    // tazích ano. Aktualizují se tam, kde se přepíná strana na tahu -
    // zahraj_tah stranu nepřepíná, takže je nemění.
    int pultahy_bez_zmeny = 0;
    int cislo_tahu = 1;

    Bod pozice_bileho_krale{5, 1};
    Bod pozice_cerneho_krale{5, 8};
};

// --- hodnoty figur (const hodnota_figur v Pascalu) --------------------------
// dama, vez, strelec, jezdec, pesec, kral
extern const double hodnota_figur[7];   // indexováno enumem Figura
extern const double hodnota_rosady;

// --- rozhraní ---------------------------------------------------------------

// nacti_uvodni_pozici
void nacti_uvodni_pozici(Pozice& p);

// V originále neexistuje - přidáno kvůli testovacím pozicím perftu.
bool nacti_fen(Pozice& p, const std::string& fen);
std::string zapis_fen(const Pozice& p);

// zjisti_jen_napadeni: napadá figura stojící na (figX,figY) pole (cilX,cilY)?
bool zjisti_jen_napadeni(const Pozice& p, int cilX, int cilY, int figX, int figY);

// otestuj_zda_neni_napadene: je pole (x,y) napadeno některou figurou strany
// 'utocnik'?
bool otestuj_zda_neni_napadene(const Pozice& p, int x, int y, Strana utocnik);

// zjisti_vsechny_ZBYLE_legalni_tahy - generátor. Pozor: tahy jsou
// PSEUDOLEGÁLNÍ, král smí zůstat v šachu. Originál to řeší až tím, že se král
// dá sebrat (hodnota 10000).
void zjisti_vsechny_legalni_tahy(Pozice& p);

// vymazej_tahy_kdy_je_v_sachu - odfiltruje tahy nechávající vlastního krále
// v šachu a nastaví stav na MAT / PAT / NORMALNI.
// V originále bylo natvrdo na hloubku 0; tady je parametrizované.
void vymazej_tahy_kdy_je_v_sachu(Pozice& p);

// zahraj_tah - provede tah na desce, aktualizuje rošádová práva, en passant,
// pozici krále a co_schramstne. Stranu na tahu NEPŘEPÍNÁ (stejně jako originál).
void zahraj_tah(Pozice& p, const Tah& tah);

// pomůcka: převod tahu na zápis typu "e2e4" / "a7a8q"
std::string tah_na_text(const Tah& tah);

}  // namespace lightred
