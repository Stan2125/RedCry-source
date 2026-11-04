// ---------------------------------------------------------------------------
//  Light Red - vrstva UCI
//
//  Záměrně bez jakéhokoli vstupu a výstupu: příkaz jde dovnitř jako řetězec,
//  odpovědi vypadnou jako seznam řádků. Díky tomu ji použije stejně dobře
//  konzolová aplikace (uci_main.cpp) i most do Androidu (jni_bridge.cpp).
//
//  Nad rámec UCI zná čtyři příkazy navíc, které potřebuje UI k zvýrazňování
//  polí, k vykreslení desky a k zápisu partie:
//      legalmoves          seznam legálních tahů v aktuální pozici
//      sanmoves            totéž i se zápisem v notaci SAN, jako <uci>:<san>
//      applymove <tah>     zahraje tah a přepne stranu
//      d                   FEN a stav (normalni / mat / pat)
// ---------------------------------------------------------------------------

#pragma once

#include "light_red_core.h"
#include "light_red_search.h"

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace lightred {

// Snímek běžícího hledání. Čte se z JINÉHO vlákna než to, které hledá,
// proto samé atomiky a žádný zámek - UI vlákno se nesmí zablokovat.
class PrubeznyStav {
public:
    // material = bilance pozice z pohledu strany na tahu (RedCry krok L1)
    void vynuluj(double material = 0.0);
    void zapis(const PrubeznyVysledek& p);
    // "depth 5 score cp 12 nodes 13314 pv e2e4", nebo prázdné, když nic neběželo
    std::string text() const;

private:
    std::atomic<int>      hloubka_{0};
    std::atomic<int>      skore_{0};
    std::atomic<long long> listu_{0};
    std::atomic<unsigned> tah_{0};      // zabalené z_pole, na_pole, proměna
    // Nastaví ho vynuluj() před hledáním a pak se jen čte ze stejného
    // vlákna jako zapis(), proto nemusí být atomický.
    double                material_ = 0.0;
};

class UciEngine {
public:
    UciEngine();

    // Zpracuje jeden příkazový řádek a vrátí řádky odpovědi.
    std::vector<std::string> prikaz(const std::string& radek);

    // Nastaví příznak přerušení. Volatelné z jiného vlákna než prikaz().
    void zastav_hledani();

    bool ma_skoncit() const { return skoncit_; }

    // Průběh běžícího hledání. Bez zámku, volatelné z jiného vlákna.
    const PrubeznyStav& prubeh() const { return prubeh_; }

    const Pozice& aktualni_pozice() const { return pozice_; }

private:
    void nova_hra();
    bool nastav_pozici(const std::vector<std::string>& slova, size_t od);
    bool najdi_tah(const std::string& zapis, Tah& ven) const;
    void zahraj_a_prepni(const Tah& t);
    std::vector<std::string> proved_go(const std::vector<std::string>& slova);
    std::vector<std::string> nastav_volbu(const std::vector<std::string>& slova);

    static void hlaseni_trampolina(void* kontext, const PrubeznyVysledek& p);

    PrubeznyStav     prubeh_{};
    std::vector<std::string> info_radky_{};
    double           material_ = 0.0;   // RedCry krok L1, viz proved_go
    Pozice           pozice_{};
    NastaveniHledani nast_{};
    std::atomic<bool> zastav_{false};
    bool             skoncit_ = false;
};

// pomocné: rozdělení řádku na slova
std::vector<std::string> rozdel(const std::string& s);

// Legální tahy i s jejich zápisem v algebraické notaci: první prvek dvojice
// je zápis UCI ("g1f3"), druhý SAN ("Nf3"). Pořadí i množina jsou totéž,
// co dá `legalmoves` - je to tentýž generátor a tentýž filtr legality.
//
// Proč celá sada naráz a ne jeden tah na požádání: rozlišení (Nbd2 proti Nfd2)
// se počítá z toho, které JINÉ figury téhož druhu mohou na totéž pole, takže
// i zápis jednoho tahu potřebuje celý legální seznam. Vydat rovnou všechny
// tedy nestojí nic navíc - a UI si díky tomu vystačí s jedním voláním po
// změně pozice. Pozice se nemění, pracuje se na kopii.
//
// Druhý opis téhož v red_max_uci.h, ne sdílená abstrakce: jádra mají jinou
// reprezentaci desky i jiný typ tahu.
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem(const Pozice& pozice);

}  // namespace lightred
