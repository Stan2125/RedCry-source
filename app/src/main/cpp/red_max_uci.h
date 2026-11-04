// ---------------------------------------------------------------------------
//  RedMAX - vrstva UCI
//
//  Záměrně bez jakéhokoli vstupu a výstupu: příkaz jde dovnitř jako řetězec,
//  odpovědi vypadnou jako seznam řádků. Díky tomu ji použije stejně dobře
//  konzolová aplikace (red_max_uci_main.cpp) i most do Androidu
//  (red_max_jni_bridge.cpp).
//
//  Nad rámec UCI zná čtyři příkazy navíc, které potřebuje UI k zvýrazňování
//  polí, k vykreslení desky a k zápisu partie:
//      legalmoves          seznam legálních tahů v aktuální pozici
//      sanmoves            totéž i se zápisem v notaci SAN, jako <uci>:<san>
//      applymove <tah>     zahraje tah a přepne stranu
//      d                   FEN a stav (normalni / mat / pat)
//
//  VLASTNÍ SADA, NE SPOLEČNÁ ABSTRAKCE S LIGHT REDEM. Tvar je schválně týž
//  jako v uci.h, ale je to druhý opis, ne sdílený kód: `lightred::UciEngine`
//  drží `lightred::Pozice` a `NastaveniHledani`, tenhle drží `redmax::Pozice`
//  a `redmax::Nastaveni`, a ta dvě jádra se slučovat nemají. Zobecňovat se má
//  podle druhého a třetího případu, ne podle prvního - a Crafty i Stockfish
//  mluví UCI samy, takže tuhle vrstvu použijí úplně jinak.
//
//  DVA ROZDÍLY PROTI LIGHT REDU, které stojí za přečtení:
//   - Generátor RedMAXu vydává PSEUDOLEGÁLNÍ tahy, takže si legalitu filtruje
//     tahle vrstva sama (legalni_tahy níž). Light Red má generátor legálních.
//   - `bestmove (none)` místo `bestmove 0000`. Viz POZNAMKY-REDMAX.md, 3/1.
// ---------------------------------------------------------------------------

#pragma once

#include "red_max_core.h"
#include "red_max_search.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace redmax {

// Snímek běžícího hledání. Čte se z JINÉHO vlákna než to, které hledá,
// proto samé atomiky a žádný zámek - UI vlákno se nesmí zablokovat.
class PrubeznyStav {
public:
    void vynuluj();
    void zapis(const PrubeznyVysledek& p);
    // "depth 5 score cp 12 nodes 13314 pv e2e4", nebo prázdné, když nic neběželo
    std::string text() const;

private:
    std::atomic<int>       hloubka_{0};
    std::atomic<int>       skore_{0};     // syrové skóre hledání, ne centipawny
    std::atomic<long long> uzlu_{0};
    std::atomic<unsigned>  tah_{0};       // zabalené z_pole, na_pole, proměna
};

class UciEngine {
public:
    UciEngine();

    // Zpracuje jeden příkazový řádek a vrátí řádky odpovědi.
    std::vector<std::string> prikaz(const std::string& radek);

    // Nastaví příznak přerušení. Volatelné z jiného vlákna než prikaz().
    // Zámek nebere a brát nesmí - hledání ho v tu chvíli drží.
    void zastav_hledani();

    bool ma_skoncit() const { return skoncit_; }

    // Průběh běžícího hledání. Bez zámku, volatelné z jiného vlákna.
    const PrubeznyStav& prubeh() const { return prubeh_; }

    const Pozice& aktualni_pozice() const { return pozice_; }

private:
    void nova_hra();
    bool nastav_pozici(const std::vector<std::string>& slova, size_t od);
    bool najdi_a_zahraj(const std::string& zapis);

    // Zapíše klíč aktuální pozice na konec historie (Krok 4/2).
    void zapamatuj_pozici();
    std::vector<std::string> proved_go(const std::vector<std::string>& slova);
    std::vector<std::string> nastav_volbu(const std::vector<std::string>& slova);

    static void hlaseni_trampolina(void* kontext, const PrubeznyVysledek& p);

    PrubeznyStav             prubeh_{};
    std::vector<std::string> info_radky_{};
    Pozice                   pozice_{};
    Nastaveni                nast_{};

    // Hash tabulka a Zobristova čísla, která přežijí mezi tahy (Krok 4/1).
    // Vlastní je tahle vrstva, ne jedno hledání - proto je motor jediný, kdo
    // rozhoduje, kdy se tabulka vynuluje (nová partie) a kdy přealokuje
    // (setoption name Hash).
    TrvalyStav               trvaly_{};

    // Klíče pozic ODEHRANÉ PARTIE, od výchozí pozice po tu aktuální (Krok
    // 4/2). Poslední prvek je vždycky klíč pozice, ve které se bude hledat.
    //
    // Bez tohohle vidělo hledání jen čtyři půltahy zpátky a partie končily
    // opakováním: ve velkém zápase 45 z 98. Detaily v POZNAMKY-REDMAX.md.
    //
    // Nezkracuje se při braní ani při tahu pěšcem, přestože by se to nabízelo
    // (po takovém tahu se dřívější pozice zopakovat nemůže). Je to jen
    // optimalizace, ne správnost, a pár set osmibajtových čísel nikomu nevadí
    // - zkracování by byla zbytečná příležitost k chybě.
    std::vector<uint64_t>    historie_{};
    std::atomic<bool>        zastav_{false};
    bool                     skoncit_ = false;
};

// pomocné: rozdělení řádku na slova
std::vector<std::string> rozdel(const std::string& s);

// Legální tahy v pozici. Generátor RedMAXu vydává pseudolegální, takže se
// každý zahraje a vezme zpět a kouká se na krále. Pozici vrátí, jak ji dostal.
std::vector<std::string> legalni_tahy(Pozice& p);

// Legální tahy i s jejich zápisem v algebraické notaci: první prvek dvojice
// je zápis UCI ("g1f3"), druhý SAN ("Nf3"). Množina prvních prvků je vždycky
// totožná s tím, co vrátí legalni_tahy() - je to tentýž generátor a tentýž
// filtr legality.
//
// Proč celá sada naráz a ne jeden tah na požádání: rozlišení (Nbd2 proti Nfd2)
// se počítá z toho, které JINÉ figury téhož druhu mohou na totéž pole, takže
// i zápis jednoho tahu potřebuje celý legální seznam. Vydat rovnou všechny
// tedy nestojí nic navíc - a UI si díky tomu vystačí s jedním voláním po
// změně pozice. Pozici vrátí, jak ji dostal.
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem(Pozice& p);

// Rozpočet času na jeden tah z `wtime`/`btime`. Vydělené schválně do vlastní
// funkce, ať se dá vyměnit, aniž by se sahalo do zbytku vrstvy.
int rozpocitej_cas(int zbyva_ms, int prirustek_ms, int tahu_do_kontroly);

}  // namespace redmax
