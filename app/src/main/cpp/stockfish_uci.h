// ---------------------------------------------------------------------------
//  Stockfish 19 - vrstva UCI
//
//  Dvojče crafty_uci.h: stejné metody, stejné příkazy, stejný výstup.
//  Záměrně bez vstupu a výstupu - příkaz jde dovnitř jako řetězec, odpovědi
//  vypadnou jako seznam řádků. Použije ji konzolová aplikace
//  (tools/stockfish/uci_main.cpp) a v kroku S3 most do Androidu.
//
//  Namespace sfredcry, protože Stockfish je obsazený. Stockfish sám se
//  nijak neupravuje (app/src/main/cpp/stockfish/PUVOD.md). Vrstva stojí na
//  jeho třídě Stockfish::Engine, ne na UCIEngine se stdin a stdout.
//
//  Nad rámec UCI zná tytéž příkazy jako ostatní motory:
//      legalmoves          seznam legálních tahů v aktuální pozici
//      sanmoves            totéž i se zápisem v SAN, jako <uci>:<san>
//      applymove <tah>     zahraje tah a přepne stranu
//      d                   FEN a stav (normalni / mat / pat / remiza50)
//
//  ROZDÍLY PROTI CRAFTY, které stojí za přečtení:
//   - Pravidla pro UI počítá vlastní Stockfish::Position vrstvy (legální
//     tahy, FEN, stav). Engine dostane před každým `go` celou partii přes
//     set_position(výchozí FEN, tahy), takže zná historii a opakování.
//   - Žádný tah = `bestmove (none)` (Stockfish sám, jako RedMAX).
//   - Volby Depth, Hash a Threads. Hash a Threads jdou rovnou do voleb
//     Stockfishe (Engine::get_options).
//   - Od kroku S6 i UCI_LimitStrength a UCI_Elo (meze Search::Skill,
//     1320..3190). Oslabené hledání skončí po iteraci, v níž Stockfish
//     vybírá slabší tah (hloubka nejvýš 1 + level), a hlásí jen hlavní
//     variantu; `bestmove` je vybraný tah, ne první tah té varianty.
//   - `bestmove` bez `ponder`: bestMove v ChessEngine.kt bere celý zbytek
//     řádku jako tah.
//
//  Hlavička schválně neincluduje nic ze Stockfishe: jeho hlavičky závisí
//  na makrech IS_64BIT, USE_NEON … a ta nesmí prosáknout do ostatních
//  překladových jednotek (most JNI). Stav je proto za ukazatelem (Stav).
// ---------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sfredcry {

// Snímek běžícího hledání. Čte se z JINÉHO vlákna než to, které hledá,
// proto samé atomiky a žádný zámek - UI vlákno se nesmí zablokovat.
class PrubeznyStav {
public:
    void vynuluj();
    // skore je "cp X" nebo "mate N" (UCIEngine::format_score), tah v zápisu UCI
    void zapis(int hloubka, const std::string &skore, long long uzlu, const std::string &tah);
    // "depth 5 score cp 12 nodes 13314 pv e2e4", nebo prázdné, když nic neběželo
    std::string text() const;

private:
    std::atomic<int>           hloubka_{0};
    std::atomic<bool>          mat_{false};   // skore_ je počet tahů do matu, ne cp
    std::atomic<int>           skore_{0};
    std::atomic<long long>     uzlu_{0};
    std::atomic<std::uint64_t> tah_{0};       // až 8 znaků zápisu UCI, 0 = žádný
};

class UciEngine {
public:
    UciEngine();
    ~UciEngine();

    UciEngine(const UciEngine &) = delete;
    UciEngine &operator=(const UciEngine &) = delete;

    // Zpracuje jeden příkazový řádek a vrátí řádky odpovědi.
    std::vector<std::string> prikaz(const std::string &radek);

    // Přeruší běžící hledání (Engine::stop je atomický). Volatelné z jiného
    // vlákna než prikaz(); zámek nebere a brát nesmí - hledání ho drží.
    void zastav_hledani();

    bool ma_skoncit() const { return skoncit_; }

    // Průběh běžícího hledání. Bez zámku, volatelné z jiného vlákna.
    const PrubeznyStav &prubeh() const { return prubeh_; }

private:
    struct Stav;

    std::vector<std::string> proved_go(const std::vector<std::string> &slova);
    std::vector<std::string> nastav_volbu(const std::vector<std::string> &slova);
    bool nastav_pozici(const std::vector<std::string> &slova, size_t od);
    bool najdi_a_zahraj(const std::string &zapis);

    std::unique_ptr<Stav> s_;
    PrubeznyStav          prubeh_{};
    std::atomic<bool>     zastavit_{false};
    int                   max_hloubka_;
    bool                  skoncit_ = false;
};

// Nejvyšší hloubka, kterou hledání Stockfishe unese. Iterační smyčka
// (stockfish/search.cpp, ř. 333-336):
//     while (rootDepth + 1 < MAX_PLY && !threads.stop && ...)
//     {
//         rootDepth++;
// a MAX_PLY = 246 (stockfish/types.h, ř. 116). Poslední iterace začne
// s rootDepth = 244 a zvýší ho na 245. Ohlašuje se v `option name Depth ... max`.
constexpr int max_hloubka_hledani = 245;

// Hloubka pro `go` bez depth a bez času. Zástupná hodnota do S4.
constexpr int vychozi_hloubka = 20;

// Meze voleb Hash a Threads (krok S3/A). Stockfish sám povoluje Hash až
// 33554432 MB (MaxHashMB v stockfish/engine.h) a Threads až
// max(1024, 4 * počet jader) (MaxThreads v stockfish/engine.cpp). Když pak
// alokace nebo vytvoření vlákna selže, skončí exit() (tt.cpp:195,
// memory.h:76, thread.cpp:64) a v aplikaci by to ukončilo celý proces.
// Vrstva proto ohlašuje a přijímá jen tyto meze a hodnotu nad nimi odmítne
// dřív, než se dostane ke Stockfishi. Výchozí hodnoty zůstávají 16 MB a 1.
constexpr int max_hash_mb = 256;
// std::thread::hardware_concurrency(), aspoň 1 (funkce smí vrátit 0).
int max_vlaken();

// pomocné: rozdělení řádku na slova
std::vector<std::string> rozdel(const std::string &s);

// Hlavní varianta v SAN (krok U14/B): tahy UCI oddělené mezerami, zahrané
// po jednom z pozice [fen], na zápisy SAN oddělené mezerami. Neplatný nebo
// nelegální tah převod ukončí (vrátí se zápisy před ním), neplatný FEN dá
// prázdný řetězec. Slouží VŠEM motorům aplikace, ne jen Stockfishi.
//
// Bez instance motoru a bez zámku, vlastní Position na zásobníku: volatelná
// z libovolného vlákna, kdykoli, i během hledání kteréhokoli motoru.
std::string san_varianty(const std::string &fen, const std::string &uci_tahy);

}  // namespace sfredcry
