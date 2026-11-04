// ---------------------------------------------------------------------------
//  Stockfish 19 - vrstva UCI (implementace)
//
//  Vzor je crafty_uci.cpp. Co je jinak a proč, je v stockfish_uci.h
//  a v docs/POZNAMKY-STOCKFISH.md (krok S2).
//
//  Tahle překladová jednotka se překládá se stejnými definicemi jako
//  knihovna stockfish (CMakeLists.txt), protože includuje její hlavičky.
// ---------------------------------------------------------------------------

#include "stockfish_uci.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "stockfish/attacks.h"
#include "stockfish/engine.h"
#include "stockfish/misc.h"
#include "stockfish/movegen.h"
#include "stockfish/position.h"
#include "stockfish/score.h"
#include "stockfish/search.h"
#include "stockfish/types.h"
#include "stockfish/uci.h"
#include "stockfish/ucioption.h"

namespace sfredcry {

namespace SF = Stockfish;

namespace {

// Tabulky útoků a klíče Zobrist jsou globály Stockfishe. Inicializují se
// jednou pro celý proces a před prvním Engine, stejně jako v jeho main.cpp
// (tools/stockfish/main.cpp: Attacks::init(); Position::init();).
// Tune::init z main.cpp se vynechává: bez maker TUNE nic neladí.
void inicializuj_stockfish() {
    static std::once_flag jednou;
    std::call_once(jednou, [] {
        SF::Attacks::init();
        SF::Position::init();
    });
}

inline std::string mala_pismena(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Tah na zápis UCI. Rošádu Stockfish kóduje jako "král bere věž",
// UCIEngine::move z ní udělá e1g1 / e1c1 (Chess960 je vypnuté).
inline std::string uci(SF::Move m) { return SF::UCIEngine::move(m, false); }

// první slovo z hlavní varianty, nebo prázdné
std::string prvni_tah(std::string_view pv) {
    const size_t mezera = pv.find(' ');
    return std::string(pv.substr(0, mezera));
}

std::uint64_t zabal(const std::string &tah) {
    std::uint64_t v = 0;
    for (size_t i = 0; i < tah.size() && i < 8; ++i)
        v |= static_cast<std::uint64_t>(static_cast<unsigned char>(tah[i])) << (8 * i);
    return v;
}

// Volbu Stockfishe jde nastavit jen přes OptionsMap::setoption (operator[]
// je const). Jména sem chodí jen Hash, Threads, UCI_LimitStrength a UCI_Elo
// (všechny existují), takže jeho výpis "No such option" na std::cout
// nenastane.
void nastav_volbu_sf(SF::OptionsMap &o, const std::string &jmeno, const std::string &hodnota) {
    std::istringstream in("name " + jmeno + " value " + hodnota);
    o.setoption(in);
}

std::string rozbal(std::uint64_t v) {
    std::string s;
    while (v) {
        s += static_cast<char>(v & 0xff);
        v >>= 8;
    }
    return s;
}

// --- zápis SAN ---------------------------------------------------------------
//
// Jediný nový šachový kód kroku S2: Stockfish SAN neumí. Pravidla (legální
// tahy, šach, mat) ale dodává Stockfish, tady se jen skládá řetězec:
//   - rošáda O-O / O-O-O. Stockfish ji kóduje jako "král bere věž", strana
//     se pozná podle toho, jestli je věž vpravo, nebo vlevo od krále,
//   - figura KQRBN, pěšec bez písmena,
//   - rozlišení podle JINÝCH LEGÁLNÍCH tahů téže figury na totéž pole:
//     sloupec, když stačí, jinak řada, jinak obojí. Vázaná figura, která na
//     pole legálně nemůže, rozlišení nevynucuje,
//   - x u braní. Pěšec bere se sloupcem (exd5), mimochodem bez přípony,
//   - =Q, =R, =B, =N u proměny,
//   - + / # po zahrání tahu: # když soupeř nemá legální tah.
// Pozici vrátí, jak ji dostal (do_move / undo_move).
std::string san(SF::Position &pos, SF::Move m) {
    using namespace SF;
    const Square od = m.from_sq(), na = m.to_sq();
    std::string z;

    if (m.type_of() == CASTLING) {
        z = na > od ? "O-O" : "O-O-O";
    } else {
        const PieceType pt = type_of(pos.moved_piece(m));
        if (pt == PAWN) {
            if (pos.capture(m)) {
                z += static_cast<char>('a' + file_of(od));
                z += 'x';
            }
        } else {
            z += " PNBRQK"[pt];
            if (pt != KING) {
                bool jiny = false, stejny_sloupec = false, stejna_rada = false;
                for (const Move k : MoveList<LEGAL>(pos)) {
                    if (k == m || k.to_sq() != na || k.from_sq() == od) continue;
                    if (k.type_of() == CASTLING) continue;
                    if (type_of(pos.moved_piece(k)) != pt) continue;
                    jiny = true;
                    if (file_of(k.from_sq()) == file_of(od)) stejny_sloupec = true;
                    if (rank_of(k.from_sq()) == rank_of(od)) stejna_rada = true;
                }
                if (jiny) {
                    if (!stejny_sloupec) {
                        z += static_cast<char>('a' + file_of(od));
                    } else if (!stejna_rada) {
                        z += static_cast<char>('1' + rank_of(od));
                    } else {
                        z += static_cast<char>('a' + file_of(od));
                        z += static_cast<char>('1' + rank_of(od));
                    }
                }
            }
            if (pos.capture(m)) z += 'x';
        }
        z += static_cast<char>('a' + file_of(na));
        z += static_cast<char>('1' + rank_of(na));
        if (m.type_of() == PROMOTION) {
            z += '=';
            z += " PNBRQK"[m.promotion_type()];
        }
    }

    if (pos.gives_check(m)) {
        StateInfo st;
        pos.do_move(m, st, nullptr);
        const bool mat = MoveList<LEGAL>(pos).size() == 0;
        pos.undo_move(m);
        z += mat ? '#' : '+';
    }
    return z;
}

}  // namespace

// --- stav vrstvy ------------------------------------------------------------
//
// Engine hledá, vlastní Position odpovídá UI (legální tahy, FEN, stav).
// Obě se staví ze stejných dvou věcí: výchozího FEN a seznamu tahů.
struct UciEngine::Stav {
    SF::Engine                   engine;
    std::unique_ptr<SF::Position> pos = std::make_unique<SF::Position>();
    SF::StateListPtr              stavy = std::make_unique<std::deque<SF::StateInfo>>(1);
    std::string                   vychozi_fen = SF::StartFEN;
    std::vector<std::string>      tahy;

    // Zpětná volání plní tohle. Běží na hlavním vlákně hledání, čte se až
    // po wait_for_search_finished (podmínková proměnná = synchronizace).
    std::vector<std::string>     info_radky;
    std::string                  bestmove;
    std::optional<SF::Engine::InfoFull> posledni;
    std::string                  posledni_skore;
    std::string                  posledni_pv;

    Stav() {
        pos->set(vychozi_fen, false, &stavy->back());
    }

    // Pozice z FEN a tahů do vlastní Position. Tahy už prošly
    // Engine::set_position, takže tu nemůže nic selhat.
    void postav_pozici() {
        auto p = std::make_unique<SF::Position>();
        auto st = std::make_unique<std::deque<SF::StateInfo>>(1);
        p->set(vychozi_fen, false, &st->back());
        for (const std::string &t : tahy) {
            const SF::Move m = SF::UCIEngine::to_move(*p, t);
            st->emplace_back();
            p->do_move(m, st->back(), nullptr);
        }
        pos = std::move(p);
        stavy = std::move(st);
    }
};

// --- PrubeznyStav -----------------------------------------------------------
void PrubeznyStav::vynuluj() {
    hloubka_.store(0, std::memory_order_relaxed);
    mat_.store(false, std::memory_order_relaxed);
    skore_.store(0, std::memory_order_relaxed);
    uzlu_.store(0, std::memory_order_relaxed);
    tah_.store(0, std::memory_order_relaxed);
}

void PrubeznyStav::zapis(int hloubka, const std::string &skore_txt, long long uzlu,
                         const std::string &tah) {
    const bool mat = skore_txt.rfind("mate ", 0) == 0;
    const size_t mezera = skore_txt.find(' ');
    const int hodnota = mezera == std::string::npos ? 0 : std::atoi(skore_txt.c_str() + mezera + 1);
    mat_.store(mat, std::memory_order_relaxed);
    skore_.store(hodnota, std::memory_order_relaxed);
    uzlu_.store(uzlu, std::memory_order_relaxed);
    tah_.store(zabal(tah), std::memory_order_relaxed);
    // hloubka se zapisuje POSLEDNÍ - je to příznak, že ostatní hodnoty
    // už patří k sobě
    hloubka_.store(hloubka, std::memory_order_release);
}

std::string PrubeznyStav::text() const {
    const int h = hloubka_.load(std::memory_order_acquire);
    if (h <= 0) return {};
    std::ostringstream o;
    o << "depth " << h
      << " score " << (mat_.load(std::memory_order_relaxed) ? "mate " : "cp ")
      << skore_.load(std::memory_order_relaxed)
      << " nodes " << uzlu_.load(std::memory_order_relaxed);
    const std::string t = rozbal(tah_.load(std::memory_order_relaxed));
    if (!t.empty()) o << " pv " << t;
    return o.str();
}

int max_vlaken() {
    const unsigned n = std::thread::hardware_concurrency();
    return n == 0 ? 1 : static_cast<int>(n);
}

std::vector<std::string> rozdel(const std::string &s) {
    std::vector<std::string> ven;
    std::istringstream in(s);
    std::string w;
    while (in >> w) ven.push_back(w);
    return ven;
}

// --- konstrukce -------------------------------------------------------------
//
// Engine se staví až po inicializaci tabulek (Stav se vytváří v těle
// konstruktoru, ne v seznamu inicializátorů). Všechna zpětná volání se
// nastaví tak, aby nic nešlo na std::cout:
//   - on_update_full, on_bestmove   plní odpověď na `go`,
//   - on_update_no_moves             nic (mat a pat hlásí vrstva sama),
//   - on_iter                        nic (currmove UI nepotřebuje),
//   - on_start                       nic,
//   - on_verify_network              nic; Engine::verify_network ho volá
//                                    bez kontroly, prázdné std::function
//                                    by vyhodilo bad_function_call.
// Informační zprávy voleb (Threads vrací popis rozdělení vláken) jdou do
// OptionsMap::info, a ten se nenastavuje - bez něj je Option::operator= tichý.
UciEngine::UciEngine() : max_hloubka_(vychozi_hloubka) {
    inicializuj_stockfish();
    s_ = std::make_unique<Stav>();
    Stav *s = s_.get();

    s->engine.set_on_update_no_moves([](const SF::Engine::InfoShort &) {});
    s->engine.set_on_iter([](const SF::Engine::InfoIter &) {});
    s->engine.set_on_start([] {});
    s->engine.set_on_verify_network([](std::string_view) {});

    s->engine.set_on_update_full([this, s](const SF::Engine::InfoFull &i) {
        // Krok S6: při oslabení (UCI_LimitStrength) hledá Stockfish s MultiPV
        // aspoň 4 (search.cpp, iterative_deepening) a hlásí každou variantu
        // zvlášť. Vrstva bere jen hlavní variantu, jinak by průběžný stav
        // i závěrečný souhrn ukazovaly poslední (nejhorší) z nich. Při plné
        // síle je MultiPV 1 a nic se nemění.
        if (i.multiPV != 1) return;
        const std::string sk = SF::UCIEngine::format_score(i.score);
        std::ostringstream o;
        o << "info depth " << i.depth << " score " << sk << " nodes " << i.nodes;
        if (!i.pv.empty()) o << " pv " << i.pv;
        s->info_radky.push_back(o.str());
        s->posledni = i;
        s->posledni_skore = sk;
        s->posledni_pv = std::string(i.pv);   // string_view platí jen během volání
        prubeh_.zapis(i.depth, sk, static_cast<long long>(i.nodes), prvni_tah(i.pv));
    });

    s->engine.set_on_bestmove([s](std::string_view bm, std::string_view) {
        s->bestmove = std::string(bm);
    });

    // Výchozí volby: Threads 1, Hash 16 - obojí je i výchozí hodnota
    // Stockfishe, nastavuje se výslovně, ať je to vidět. Po měření na
    // telefonech ponecháno (POZNAMKY-STOCKFISH.md, kroky S4/A a S4/B).
    nastav_volbu_sf(s->engine.get_options(), "Threads", "1");
    nastav_volbu_sf(s->engine.get_options(), "Hash", "16");
}

UciEngine::~UciEngine() {
    if (s_) {
        s_->engine.stop();
        s_->engine.wait_for_search_finished();
    }
}

// Stockfish vynuluje threads.stop až v Engine::go (start_thinking). Stop,
// který přijde mezi začátkem proved_go a tímhle okamžikem, by se ztratil,
// proto si ho vrstva pamatuje ve vlastním příznaku a po go ho zopakuje.
void UciEngine::zastav_hledani() {
    zastavit_.store(true);
    s_->engine.stop();
}

// --- najde tah podle zápisu e2e4 / a7a8q a zahraje ho -----------------------
bool UciEngine::najdi_a_zahraj(const std::string &zapis) {
    std::string t = mala_pismena(zapis);
    SF::Move m = SF::UCIEngine::to_move(*s_->pos, t);
    // Proměna bez určené figury: doplní se dáma, stejně jako u Crafty a RedMAXu.
    if (m == SF::Move::none() && t.size() == 4) {
        m = SF::UCIEngine::to_move(*s_->pos, t + "q");
        if (m != SF::Move::none()) t += "q";
    }
    if (m == SF::Move::none()) return false;
    s_->stavy->emplace_back();
    s_->pos->do_move(m, s_->stavy->back(), nullptr);
    s_->tahy.push_back(t);
    return true;
}

// --- position startpos|fen ... [moves ...] ----------------------------------
//
// Stockfish 19 ověřuje FEN přísně. FEN se přijme jen tehdy, když ho přijme
// Engine::set_position; při odmítnutí zůstane stav vrstvy (výchozí FEN,
// tahy, vlastní Position) beze změny. Engine::pos po odmítnutí nedává
// smysl, ale před každým go se staví znovu.
bool UciEngine::nastav_pozici(const std::vector<std::string> &slova, size_t od) {
    size_t i = od;
    if (i >= slova.size()) return false;

    std::string fen;
    if (slova[i] == "startpos") {
        fen = SF::StartFEN;
        ++i;
    } else if (slova[i] == "fen") {
        ++i;
        while (i < slova.size() && slova[i] != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += slova[i];
            ++i;
        }
    } else {
        return false;
    }

    std::vector<std::string> zadane;
    if (i < slova.size() && slova[i] == "moves")
        for (++i; i < slova.size(); ++i) zadane.push_back(mala_pismena(slova[i]));

    // Nejdřív FEN samotný, pak tahy po jednom na pomocné pozici, aby se
    // doplnila dáma u proměny bez figury (a7a8 -> a7a8q) stejně jako
    // v applymove. Engine::set_position s takto doplněnými tahy je pak
    // závěrečné ověření.
    if (s_->engine.set_position(fen, {}).has_value()) return false;

    SF::Position p;
    std::deque<SF::StateInfo> st(1);
    if (p.set(fen, false, &st.back()).has_value()) return false;
    std::vector<std::string> tahy;
    for (std::string t : zadane) {
        SF::Move m = SF::UCIEngine::to_move(p, t);
        if (m == SF::Move::none() && t.size() == 4) {
            m = SF::UCIEngine::to_move(p, t + "q");
            if (m != SF::Move::none()) t += "q";
        }
        if (m == SF::Move::none()) return false;
        st.emplace_back();
        p.do_move(m, st.back(), nullptr);
        tahy.push_back(t);
    }
    if (s_->engine.set_position(fen, tahy).has_value()) return false;

    s_->vychozi_fen = fen;
    s_->tahy = std::move(tahy);
    s_->postav_pozici();
    return true;
}

// --- setoption name <jméno> value <hodnota> ---------------------------------
std::vector<std::string> UciEngine::nastav_volbu(const std::vector<std::string> &slova) {
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
    if (j == "depth") {
        max_hloubka_ = std::min(max_hloubka_hledani, std::max(1, std::atoi(hodnota.c_str())));
        return {};
    }
    if (j == "uci_limitstrength") {
        // Krok S6. Jen "true" / "false" jako Stockfish (typ check).
        const std::string h = mala_pismena(hodnota);
        if (h != "true" && h != "false")
            return {"info string neplatna hodnota: " + jmeno + " " + hodnota + " (true nebo false)"};
        nastav_volbu_sf(s_->engine.get_options(), "UCI_LimitStrength", h);
        return {};
    }
    if (j == "uci_elo") {
        // Krok S6. Meze jsou Stockfishe (Search::Skill), ne vrstvy; mimo ně
        // se hodnota nezmění, stejně přísně jako Hash a Threads.
        const int od = SF::Search::Skill::LowestElo, po = SF::Search::Skill::HighestElo;
        char *konec = nullptr;
        errno = 0;
        const long long v = std::strtoll(hodnota.c_str(), &konec, 10);
        if (hodnota.empty() || errno == ERANGE || *konec != '\0' || v < od || v > po)
            return {"info string neplatna hodnota: " + jmeno + " " + hodnota
                    + " (povoleno " + std::to_string(od) + ".." + std::to_string(po) + ")"};
        SF::OptionsMap &o = s_->engine.get_options();
        nastav_volbu_sf(o, "UCI_Elo", std::to_string(v));
        if (int(o["UCI_Elo"]) != v)
            return {"info string neplatna hodnota: " + jmeno};
        return {};
    }
    if (j == "hash" || j == "threads") {
        // Meze vrstvy (max_hash_mb, max_vlaken) se hlídají DŘÍV, než hodnota
        // dojde ke Stockfishi: nad nimi by alokace nebo vlákno mohly selhat
        // a Stockfish by skončil exit(). Hodnota mimo meze se nezmění.
        // Celé číslo stejně přísně jako Stockfish (value_in_range
        // v ucioption.cpp): strtoll, nic za číslem.
        const std::string sf_jmeno = j == "hash" ? "Hash" : "Threads";
        const int mez = j == "hash" ? max_hash_mb : max_vlaken();
        char *konec = nullptr;
        errno = 0;
        const long long v = std::strtoll(hodnota.c_str(), &konec, 10);
        if (hodnota.empty() || errno == ERANGE || *konec != '\0' || v < 1 || v > mez)
            return {"info string neplatna hodnota: " + jmeno + " " + hodnota
                    + " (povoleno 1.." + std::to_string(mez) + ")"};
        // Option::operator= hodnotu mimo své meze tiše zahodí; kdyby se to
        // přesto stalo, ohlásí se to tady.
        SF::OptionsMap &o = s_->engine.get_options();
        nastav_volbu_sf(o, sf_jmeno, std::to_string(v));
        if (int(o[sf_jmeno]) != v)
            return {"info string neplatna hodnota: " + jmeno};
        return {};
    }
    return {"info string neznama volba: " + jmeno};
}

// --- go ---------------------------------------------------------------------
std::vector<std::string> UciEngine::proved_go(const std::vector<std::string> &slova) {
    Stav *s = s_.get();
    SF::Search::LimitsType limits;
    // Konstruktor LimitsType startTime nenastavuje; bez něj by movetime
    // počítal od nesmyslu. Stejně jako UCIEngine::parse_limits (uci.cpp, ř. 195).
    limits.startTime = SF::now();

    int hloubka = 0;
    bool zadana_hloubka = false, zadany_cas = false;
    for (size_t i = 1; i < slova.size(); ++i) {
        const std::string &w = slova[i];
        auto cislo = [&]() -> long long {
            return (i + 1 < slova.size()) ? std::atoll(slova[++i].c_str()) : 0;
        };
        if      (w == "depth")     { hloubka = static_cast<int>(cislo()); zadana_hloubka = true; }
        else if (w == "movetime")  { limits.movetime = cislo(); zadany_cas = limits.movetime > 0; }
        else if (w == "wtime")     { limits.time[SF::WHITE] = cislo(); zadany_cas = true; }
        else if (w == "btime")     { limits.time[SF::BLACK] = cislo(); zadany_cas = true; }
        else if (w == "winc")      limits.inc[SF::WHITE] = cislo();
        else if (w == "binc")      limits.inc[SF::BLACK] = cislo();
        else if (w == "movestogo") limits.movestogo = static_cast<int>(cislo());
        else if (w == "infinite")  limits.infinite = 1;
    }
    // Hloubka: zadaná, jinak Depth z voleb. Když rozhoduje čas nebo infinite
    // a hloubku nikdo nezadal, ať nebrzdí (depth 0 = bez omezení), jako
    // u Crafty. Strop je v každém případě max_hloubka_hledani.
    if (zadana_hloubka)                          limits.depth = std::max(1, hloubka);
    else if (!zadany_cas && !limits.infinite)    limits.depth = max_hloubka_;
    if (limits.depth > max_hloubka_hledani) limits.depth = max_hloubka_hledani;

    // Krok S6: oslabený Stockfish vybírá tah v iteraci 1 + int(level)
    // (Skill::time_to_pick, search.cpp ř. 558) a zahraje ho (ř. 625-629).
    // Hledání by jinak běželo do konce času, proto hloubka nejvýš 1 + int(level).
    // Iterace s výběrem proběhne, smyčka končí až po ní (ř. 333). Level počítá
    // Stockfish (Search::Skill), vzorec se neopisuje.
#ifndef REDCRY_SF_BEZ_ZKRACENI  // krok E1/C: měřicí přepínač, viz #endif
    {
        const SF::OptionsMap &o = s->engine.get_options();
        if (int(o["UCI_LimitStrength"])) {
            const SF::Search::Skill skill{int(o["Skill Level"]), int(o["UCI_Elo"])};
            if (skill.enabled()) {
                const int vyber = 1 + int(skill.level);
                limits.depth = limits.depth ? std::min(limits.depth, vyber) : vyber;
            }
        }
    }
#endif  // REDCRY_SF_BEZ_ZKRACENI jen v rozhodčím emu_zapas_plny, v aplikaci neaktivní
    // Engine dostane celou partii, takže zná historii (opakování, 50 tahů).
    s->engine.set_position(s->vychozi_fen, s->tahy);

    prubeh_.vynuluj();
    s->info_radky.clear();
    s->bestmove.clear();
    s->posledni.reset();
    s->posledni_skore.clear();
    s->posledni_pv.clear();

    const auto t0 = std::chrono::steady_clock::now();
    zastavit_.store(false);
    s->engine.go(limits);
    if (zastavit_.load()) s->engine.stop();   // stop, který přišel před go
    s->engine.wait_for_search_finished();
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - t0).count();
    const bool preruseno = zastavit_.load();

    std::vector<std::string> ven = std::move(s->info_radky);
    s->info_radky.clear();

    const SF::MoveList<SF::LEGAL> legalni(*s->pos);
    if (legalni.size() == 0) {
        ven.push_back(std::string("info string ") + (s->pos->checkers() ? "mat" : "pat"));
        ven.push_back("bestmove (none)");
        return ven;
    }

    // Závěrečný souhrn jako u ostatních motorů: poslední hlášení a čas.
    if (s->posledni) {
        std::ostringstream info;
        info << "info depth " << s->posledni->depth << " score " << s->posledni_skore
             << " nodes " << s->posledni->nodes << " time " << ms;
        if (!s->posledni_pv.empty()) info << " pv " << s->posledni_pv;
        ven.push_back(info.str());
    }
    if (preruseno) ven.push_back("info string hledani preruseno");

    // Druhá pojistka jako u Crafty: tah z hledání musí být mezi legálními
    // tahy vlastní pozice. Stockfish hledá z téže pozice, takže by to mělo
    // platit vždy; kdyby ne, je to chyba vrstvy a má být vidět.
    std::string tah = s->bestmove;
    bool je_legalni = false;
    for (const SF::Move m : legalni)
        if (uci(m) == tah) { je_legalni = true; break; }
    if (!je_legalni) {
        ven.push_back("info string tah z hledani nebyl legalni (" + tah + "), nahrazen "
                      + uci(*legalni.begin()));
        tah = uci(*legalni.begin());
    }
    ven.push_back("bestmove " + tah);
    return ven;
}

// --- rozcestník --------------------------------------------------------------
std::vector<std::string> UciEngine::prikaz(const std::string &radek) {
    const std::vector<std::string> slova = rozdel(radek);
    if (slova.empty()) return {};

    const std::string &c = slova[0];

    if (c == "uci") {
        // engine_info(true) = "Stockfish 19\nid author the Stockfish developers ..."
        std::vector<std::string> ven;
        std::istringstream info(SF::engine_info(true));
        std::string r;
        bool prvni = true;
        while (std::getline(info, r)) {
            ven.push_back(prvni ? "id name " + r : r);
            prvni = false;
        }
        ven.push_back("option name Depth type spin default " + std::to_string(vychozi_hloubka)
                      + " min 1 max " + std::to_string(max_hloubka_hledani));
        // Meze vrstvy, ne Stockfishe (MaxHashMB, MaxThreads), viz stockfish_uci.h.
        ven.push_back("option name Hash type spin default 16 min 1 max "
                      + std::to_string(max_hash_mb));
        ven.push_back("option name Threads type spin default 1 min 1 max "
                      + std::to_string(max_vlaken()));
        // Krok S6: oslabení přes Elo. Meze a výchozí hodnota jsou Stockfishe
        // (Search::Skill, engine.cpp). Skill Level a MultiPV se nevystavují.
        ven.push_back("option name UCI_LimitStrength type check default false");
        ven.push_back("option name UCI_Elo type spin default "
                      + std::to_string(SF::Search::Skill::LowestElo) + " min "
                      + std::to_string(SF::Search::Skill::LowestElo) + " max "
                      + std::to_string(SF::Search::Skill::HighestElo));
        ven.push_back("uciok");
        return ven;
    }

    if (c == "isready")    return {"readyok"};
    if (c == "quit")       { skoncit_ = true; return {}; }
    if (c == "stop")       { zastav_hledani(); return {}; }
    if (c == "setoption")  return nastav_volbu(slova);
    if (c == "go")         return proved_go(slova);

    if (c == "ucinewgame") {
        s_->engine.search_clear();
        s_->vychozi_fen = SF::StartFEN;
        s_->tahy.clear();
        s_->postav_pozici();
        return {};
    }

    if (c == "position") {
        if (!nastav_pozici(slova, 1)) return {"info string neplatne position"};
        return {};
    }

    // --- příkazy nad rámec UCI, pro UI ---

    if (c == "legalmoves") {
        std::string s;
        for (const SF::Move m : SF::MoveList<SF::LEGAL>(*s_->pos)) {
            if (!s.empty()) s += ' ';
            s += uci(m);
        }
        return {s};
    }

    // Dvojtečka je oddělovač proto, že se nevyskytuje ani v zápisu UCI, ani
    // v SAN. Pořadí je totéž jako u legalmoves (MoveList<LEGAL>).
    if (c == "sanmoves") {
        std::string s;
        SF::Position &p = *s_->pos;
        for (const SF::Move m : SF::MoveList<SF::LEGAL>(p)) {
            if (!s.empty()) s += ' ';
            s += uci(m);
            s += ':';
            s += san(p, m);
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
        // Pořadí je závazné (krok 16): (1) žádný legální tah -> mat, nebo
        // pat, (2) jinak aspoň 100 půltahů bez braní a tahu pěšcem ->
        // remiza50, (3) jinak normalni. Mat na stém půltahu je tedy mat.
        const SF::Position &p = *s_->pos;
        const bool je_tah = SF::MoveList<SF::LEGAL>(p).size() > 0;
        const char *st = !je_tah ? (p.checkers() ? "mat" : "pat")
                       : (p.rule50_count() >= 100) ? "remiza50"
                       : "normalni";
        return {p.fen(), std::string("stav ") + st};
    }

    return {"info string neznamy prikaz: " + c};
}

// --- hlavní varianta v SAN (krok U14/B) --------------------------------------
//
// Mimo anonymní namespace, aby ji šlo volat zvenku, a přitom v téže
// překladové jednotce kvůli san(). Vlastní Position a StateInfo na zásobníku,
// žádná instance motoru, žádný zámek, žádný sdílený zapisovaný stav: globály
// Stockfishe (útoky, Zobrist) se po inicializuj_stockfish() jen čtou.
// Volatelné proto kdykoli, i během hledání kteréhokoli motoru.
std::string san_varianty(const std::string &fen, const std::string &uci_tahy) {
    inicializuj_stockfish();
    SF::Position p;
    std::deque<SF::StateInfo> st(1);
    if (p.set(fen, false, &st.back()).has_value()) return {};
    std::string vysledek;
    for (const std::string &t : rozdel(uci_tahy)) {
        // Tah stejně jako jinde ve vrstvě; neplatný nebo nelegální vrátí
        // Move::none() a převod tím končí.
        const SF::Move m = SF::UCIEngine::to_move(p, mala_pismena(t));
        if (m == SF::Move::none()) break;
        if (!vysledek.empty()) vysledek += ' ';
        vysledek += san(p, m);
        st.emplace_back();
        p.do_move(m, st.back(), nullptr);
    }
    return vysledek;
}

}  // namespace sfredcry
