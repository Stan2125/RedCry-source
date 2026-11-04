// ---------------------------------------------------------------------------
//  Light Red - vrstva hledání
//
//  Přepis procedury vnor_se z Light_Red_211_2_f.pas (řádky 2424-2936)
//  spolu se serad_tahy, pridej_hodnotu_lepsim_tahum
//  a zjisti_zda_je_legalni_NEJLEPSI_tah.
//
//  Struktura je zachovaná: NE negamax, ale výslovný minimax se sudými
//  a lichými hladinami. Sudá hladina patří straně, která je na tahu v kořeni,
//  a maximalizuje přes 'alfa'; lichá patří soupeři a minimalizuje přes 'beta'.
//  Převod na negamax by kód zkrátil, ale změnil by chování, a o to tady nejde.
//
//  Modul sahá do light_red_core.h a nic v něm nemění.
// ---------------------------------------------------------------------------

#pragma once

#include "light_red_core.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace lightred {

// Průběžný výsledek jedné dokončené hloubky. Ohlašuje se na tom vlákně,
// které zavolalo hledej(), ještě než hledání skončí.
struct PrubeznyVysledek {
    int      hloubka = 0;
    double   hodnota = 0.0;
    uint64_t listu = 0;
    Tah      nejlepsi_tah{};
    bool     tah_nalezen = false;
};

using HlaseniFn = void (*)(void* kontext, const PrubeznyVysledek&);

struct NastaveniHledani {
    // pozice[0].max_hloubka v originále
    int max_hloubka = 4;

    // max_hledej_klid v originále. GUI mu dávalo TUTÉŽ hodnotu jako
    // max_hloubka, čímž se prohlubování při šachu a braní zneškodnilo
    // (viz POZNAMKY-LIGHTRED-HLEDANI.md, bod 1). Hodnota větší než max_hloubka
    // tu funkci probudí.
    int max_hledej_klid = 4;

    // pridej_hodnotu_lepsim_tahum - tabulka 41 zahajovacích tahů pro každou
    // stranu, uplatňuje se jen v kořeni.
    bool zahajovaci_preference = true;

    // Věrně zopakuje chybu originálu: sloty nejlepsi_tah[1..2] v rodičovské
    // úrovni slouží zároveň jako zásoba killer tahů a jako místo, kam si
    // potomci zapisují nález. Uzel si tak přepíše zdroj pod rukama a při
    // dogenerování zbytku vyřadí tah, který nikdy nezkusil - tedy přehlédne
    // legální tahy. Výchozí false = opraveno (zdroj se na začátku uzlu
    // zazálohuje). true jen pro srovnání s původním programem.
    bool puvodni_chyba_killeru = false;

    // Vypne odřezávání větví i killer tahy. Slouží k důkazu, že ani jedno
    // nemění výsledek hledání - jen ho zrychluje. Pro hru nepoužívat,
    // je to řádově pomalejší.
    bool overovaci_rezim = false;

    // Prohledá postupně hloubky 1, 2, ... až max_hloubka a drží si výsledek
    // poslední DOKONČENÉ. Bez toho by přerušené hledání nevrátilo nic, protože
    // originál žádné postupné prohlubování neměl. Na výsledek nepřerušeného
    // hledání to nemá vliv - poslední kolo je totéž jako jedno hledání
    // na plnou hloubku.
    bool postupne_prohlubovani = false;

    // Časový strop v milisekundách; 0 = bez omezení.
    int cas_ms = 0;

    // Ukazatel na příznak, kterým jde hledání přerušit z jiného vlákna.
    // Na Androidu tudy chodí "stop" z UI vlákna do JNI vlákna.
    const std::atomic<bool>* zastav = nullptr;

    // Volá se po každé DOKONČENÉ hloubce postupného prohlubování.
    // Uplatní se jen při postupne_prohlubovani = true.
    HlaseniFn hlaseni = nullptr;
    void*     hlaseni_kontext = nullptr;
};

struct VysledekHledani {
    Tah      nejlepsi_tah{};
    bool     tah_nalezen = false;
    double   hodnota = 0.0;        // pozice[0].alfa
    Prubeh   stav = NORMALNI;      // MAT / PAT / NORMALNI v kořeni
    uint64_t listu = 0;            // pocet_poslednich
    uint64_t matu = 0;             // pocet_matu
    double   sekundy = 0.0;
    bool     preruseno = false;    // došel čas nebo přišel povel stop
    int      dosazena_hloubka = 0; // poslední dokončená hloubka
};

// Prohledá pozici a vrátí tah, který engine zahraje.
// Vstupní pozice se nemění.
VysledekHledani hledej(const Pozice& start, const NastaveniHledani& n);

// serad_tahy - MVV/LVA řazení. Vystaveno kvůli testům.
// V kořeni (kořen = true) se zachová hodnota 'trideni' předplněná
// zahajovací tabulkou, hlouběji se nuluje.
void serad_tahy(Pozice& p, bool koren);

// pridej_hodnotu_lepsim_tahum - naplní trideni a lepsi_tah v kořeni.
void pridej_hodnotu_lepsim_tahum(Pozice& p);

}  // namespace lightred
