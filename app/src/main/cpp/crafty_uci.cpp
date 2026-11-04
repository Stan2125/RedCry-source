/*
Crafty 18.15 - copyright notice of the original author, reproduced unchanged:

 *******************************************************************************
 *                                                                             *
 *  Crafty, copyright 1996-2001 by Robert M. Hyatt, Ph.D., Associate Professor *
 *  of Computer and Information Sciences, University of Alabama at Birmingham. *
 *                                                                             *
 *  All rights reserved.  No part of this program may be reproduced in any     *
 *  form or by any means, for other than your personal use, without the        *
 *  express written permission of the author.  This program may not be used in *
 *  whole, nor in part, to enter any computer chess competition without        *
 *  written permission from the author.  Such permission will include the      *
 *  requirement that the program be entered under the name "Crafty" so that    *
 *  the program's ancestry will be known.                                      *
 *                                                                             *
 *  Copies of the source must contain the original copyright notice intact.    *
 *                                                                             *
 *  Any changes made to this software must also be made public to comply with  *
 *  the original intent of this software distribution project.  These          *
 *  restrictions apply whether the distribution is being done for free or as   *
 *  part or all of a commercial product.  The author retains sole ownership    *
 *  and copyright on this program except for 'personal use' explained below.   *
 *                                                                             *
 *  personal use includes any use you make of the program yourself, either by  *
 *  playing games with it yourself, or allowing others to play it on your      *
 *  machine,  and requires that if others use the program, it must be clearly  *
 *  identified as "Crafty" to anyone playing it (on a chess server as one      *
 *  example).  Personal use does not allow anyone to enter this into a chess   *
 *  tournament where other program authors are invited to participate.  IE you *
 *  can do your own local tournament, with Crafty + other programs, since this *
 *  is for your personal enjoyment.  But you may not enter Crafty into an      *
 *  event where it will be in competition with other programs/programmers      *
 *  without permission as stated previously.                                   *
 *                                                                             *
 *  Crafty is the "son" (direct descendent) of Cray Blitz.  it is designed     *
 *  totally around the bit-board data structure for reasons of speed of ex-    *
 *  ecution, ease of adding new knowledge, and a *significantly* cleaner       *
 *  overall design.  it is written totally in ANSI C with some few UNIX system *
 *  calls required for I/O, etc.                                               *
 *                                                                             *
 *******************************************************************************
*/

// ---------------------------------------------------------------------------
//  Crafty - vrstva UCI (implementace)
//
//  Vzor je red_max_uci.cpp. Co je jinak a proč, je v crafty_uci.h
//  a v docs/POZNAMKY-CRAFTY.md (krok C3/B).
// ---------------------------------------------------------------------------

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "crafty_uci.h"

namespace crafty {

namespace {

const char *JMENO = "RedCrafty 4.81 (Crafty 18.15)";
const char *AUTOR = "Robert Hyatt, uprava Stanislav, prepis do C++ 2026";

// Výchozí pozice bez půltahů a čísla tahu. Viz nastav_pozici: FEN výchozí
// pozice se staví stejně jako `startpos`.
const char *START_FEN_4 = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

// Historie partie v replist_w/b má 128 položek na stranu a RepetitionCheck
// za její konec během hledání zapisuje až (ply-2)/2 další (ř. 11343-11350),
// tedy nejvýš 32. Delší historie se proto zkrátí na posledních
// HISTORIE_PONECHAT pozic; starší než 100 půltahů stejně nic nerozhodují
// (pravidlo 50 tahů, Rule50Moves > 99).
constexpr int HISTORIE_MAX = 64;
constexpr int HISTORIE_PONECHAT = 48;

inline std::string mala_pismena(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// --- skóre do zápisu UCI ----------------------------------------------------
//
// Hodnocení Crafty je v setinách pěšce (PAWN_VALUE = 100), `score cp` je tedy
// hodnota beze změny. Hledání vrací skóre z pohledu strany na tahu v kořeni
// (SearchRoot je negamax; ověřeno v test_uci.sh, oddíl Znaménko skóre).
//
// Mat: Search vrací -(MATE-ply) v uzlu, kde strana na tahu nemá tah (ř.
// 10975), kořen je ply 1. Matovaný uzel na ply p má tedy v kořeni hodnotu
// ±(MATE-p) a leží d = p-1 půltahů od kořene. pultahy = MATE-|hodnota| = p
// = d+1.
//
// UCI chce počet TAHŮ, kladný když mat dává strana na tahu, záporný když ho
// dostává (jako Stockfish, uci.cpp format_score). Mat dávaný v N tazích je
// matovaný uzel na d = 2N-1, tedy pultahy = 2N. Mat dostávaný v N tazích je
// d = 2N, pultahy = 2N+1. V obou případech N = pultahy / 2:
//   mat 1 tahem          p = 2  ->  mate 1
//   mat 2 tahy           p = 4  ->  mate 2
//   Kb8 Vh8# (dostává)   p = 3  ->  mate -1
// Stejný vzorec jako RedMAX (red_max_uci.cpp) a stejná čísla jako Stockfish.
std::string skore_do_uci(int hodnota) {
    if (hodnota > MATE - 300 || hodnota < -(MATE - 300)) {
        const int pultahy = MATE - std::abs(hodnota);
        // RedCry krok U2: bylo (pultahy + 1) / 2, dostávaný mat pak vyšel
        // o tah vyšší než podle UCI (Kb8 Vh8# "mate -2" místo "mate -1").
        // Nález z kroku S2/A, docs/POZNAMKY-STOCKFISH.md a POZNAMKY-CRAFTY.md.
        const int tahy = pultahy / 2;
        return "score mate " + std::to_string(hodnota > 0 ? tahy : -tahy);
    }
    return "score cp " + std::to_string(hodnota);
}

// prvních n polí FEN oddělených mezerou
std::string pole_fen(const std::string &fen, int n) {
    std::istringstream in(fen);
    std::string w, ven;
    for (int i = 0; i < n && (in >> w); ++i) {
        if (!ven.empty()) ven += ' ';
        ven += w;
    }
    return ven;
}

// pole č. i (od 0), nebo prázdné
std::string pole_fen_i(const std::string &fen, int i) {
    std::istringstream in(fen);
    std::string w;
    for (int k = 0; k <= i; ++k)
        if (!(in >> w)) return std::string();
    return w;
}

bool strana_na_tahu_v_sachu() {
    TREE *const tree = local[0];
    return Check(wtm);
}

// Legální tahy v kořeni: generátor Crafty je pseudolegální, takže se každý
// tah zahraje (MakeMove na ply 0, pozice position[0]) a vezme zpět.
std::vector<int> legalni_tahy_crafty() {
    TREE *const tree = local[0];
    int tahy[256];
    int *konec = GenerateCaptures(tree, 0, wtm, tahy);
    konec = GenerateNonCaptures(tree, 0, wtm, konec);
    std::vector<int> v;
    for (int *mv = tahy; mv < konec; ++mv) {
        MakeMove(tree, 0, *mv, wtm);
        if (!Check(wtm)) v.push_back(*mv);
        UnmakeMove(tree, 0, *mv, wtm);
    }
    return v;
}

// Zkrácení historie partie (viz HISTORIE_MAX).
void zkrat_historii(BITBOARD *zacatek, BITBOARD *&hlava) {
    if (hlava - zacatek > HISTORIE_MAX) {
        std::memmove(zacatek, hlava - HISTORIE_PONECHAT, HISTORIE_PONECHAT * sizeof(BITBOARD));
        hlava = zacatek + HISTORIE_PONECHAT;
    }
}

// Pole mimochodem z FEN nechat jen tehdy, když ho někdo opravdu může vzít,
// přesně jak ho nastavuje MakeMove (ř. 3396 a 3480: mask_eptest pole vedle
// pěšce, který táhl o dvě). SetBoard to kontroluje po svém a přes okraj
// desky špatně (POZNAMKY-CRAFTY.md, C1, nález 4); jeho kód se nemění, oprava
// je tady. Bez ní by měla pozice z FEN jiný hash_key než táž pozice dosažená
// tahy (enpassant_random navíc) a opakování by se neprojevilo. Klíč už
// SetBoard zapsal do historie, takže se opraví i tam.
void oprav_ep_z_fen() {
    TREE *const tree = local[0];
    const int ep = EnPassant(0);
    if (!ep) return;
    BITBOARD berouci;
    if (wtm) berouci = mask_eptest[ep - 8] & WhitePawns;   // černý pěšec stojí na ep-8
    else     berouci = mask_eptest[ep + 8] & BlackPawns;   // bílý pěšec stojí na ep+8
    if (berouci) return;
    HashEP(ep, HashKey);
    EnPassant(0) = 0;
    if (wtm) tree->rephead_w[-1] = HashKey;
    else     tree->rephead_b[-1] = HashKey;
}

// Výchozí pozice tak, jak ji staví originál (NewGame -> InitializeChessBoard):
// fáze `opening`, na rozdíl od SetBoard, který každou pozici vede jako
// střední hru (ř. 1785-1787). Historie tahů a killery se vynulují stejně
// jako v SetBoard (ř. 1769-1781). Klíč výchozí pozice se zapíše do
// historie opakování - SetBoard to dělá (ř. 1763-1766), InitializeChessBoard
// ne, a obě cesty mají vést ke stejné historii.
void nastav_vychozi() {
    TREE *const tree = local[0];
    InitializeChessBoard(&tree->position[0]);
    last_mate_score = 0;
    std::memset(history_w, 0, sizeof(history_w));
    std::memset(history_b, 0, sizeof(history_b));
    for (int i = 0; i < MAXPLY; i++) {
        tree->killers[i].move1 = 0;
        tree->killers[i].move2 = 0;
    }
    last_pv.pathd = 0;
    last_pv.pathl = 0;
    tree->pv[0].pathd = 0;
    tree->pv[0].pathl = 0;
    move_number = 1;
    *tree->rephead_w++ = HashKey;
}

}  // namespace

// --- tah na zápis UCI --------------------------------------------------------
std::string tah_na_uci(int move) {
    static const char promena[8] = {0, 0, 'n', 0, 0, 'b', 'r', 'q'};
    if (!move) return "0000";
    std::string s;
    s += static_cast<char>('a' + File(From(move)));
    s += static_cast<char>('1' + Rank(From(move)));
    s += static_cast<char>('a' + File(To(move)));
    s += static_cast<char>('1' + Rank(To(move)));
    if (Promote(move)) s += promena[Promote(move)];
    return s;
}

// --- legální tahy -----------------------------------------------------------
std::vector<std::string> legalni_tahy() {
    std::vector<std::string> ven;
    for (int mv : legalni_tahy_crafty()) ven.push_back(tah_na_uci(mv));
    return ven;
}

// --- legální tahy i se zápisem v SAN ----------------------------------------
//
// Zápis dělá OutputMove Crafty (ř. 4914-5071): písmena KQRBN, x, =Q, O-O
// a O-O-O, rozlišení nejdřív sloupcem, pak řadou, pak celým polem (každý
// kandidát ověří InputMove, který počítá jen s LEGÁLNÍMI tahy, takže vázaná
// figura rozlišení nevynucuje), + a # podle GenerateCheckEvasions. To je
// přesně náš tvar, proto se nepíše vlastní (POZNAMKY-CRAFTY.md, C3/B).
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem() {
    TREE *const tree = local[0];
    std::vector<std::pair<std::string, std::string>> ven;
    for (int mv : legalni_tahy_crafty()) {
        // OutputMove vrací statický buffer - hned zkopírovat
        const std::string san = OutputMove(tree, mv, 0, wtm);
        ven.emplace_back(tah_na_uci(mv), san);
    }
    return ven;
}

// --- rozpočet času (opis z red_max_uci.cpp) ---------------------------------
int rozpocitej_cas(int zbyva_ms, int prirustek_ms, int tahu_do_kontroly) {
    if (zbyva_ms <= 0) return 0;

    const int tahu = (tahu_do_kontroly > 0) ? tahu_do_kontroly : 30;
    int cas = zbyva_ms / tahu + (prirustek_ms * 3) / 4;

    const int strop = zbyva_ms / 2;
    if (cas > strop) cas = strop;
    if (cas < 10)    cas = 10;
    return cas;
}

// --- PrubeznyStav -----------------------------------------------------------
void PrubeznyStav::vynuluj() {
    hloubka_.store(0, std::memory_order_relaxed);
    skore_.store(0, std::memory_order_relaxed);
    uzlu_.store(0, std::memory_order_relaxed);
    tah_.store(0, std::memory_order_relaxed);
}

void PrubeznyStav::zapis(const PrubeznyVysledek &p) {
    skore_.store(p.hodnota, std::memory_order_relaxed);
    uzlu_.store(static_cast<long long>(p.uzlu), std::memory_order_relaxed);
    tah_.store(p.tah_nalezen ? p.nejlepsi_tah : 0, std::memory_order_relaxed);
    // hloubka se zapisuje POSLEDNÍ - je to příznak, že ostatní tři hodnoty
    // už patří k sobě
    hloubka_.store(p.hloubka, std::memory_order_release);
}

std::string PrubeznyStav::text() const {
    const int h = hloubka_.load(std::memory_order_acquire);
    if (h <= 0) return {};
    std::ostringstream o;
    o << "depth " << h
      << ' ' << skore_do_uci(skore_.load(std::memory_order_relaxed))
      << " nodes " << uzlu_.load(std::memory_order_relaxed);
    const int k = tah_.load(std::memory_order_relaxed);
    if (k) o << " pv " << tah_na_uci(k);
    return o.str();
}

void UciEngine::hlaseni_trampolina(void *kontext, const PrubeznyVysledek &p) {
    UciEngine *self = static_cast<UciEngine *>(kontext);
    self->prubeh_.zapis(p);
    std::ostringstream o;
    o << "info depth " << p.hloubka
      << ' ' << skore_do_uci(p.hodnota)
      << " nodes " << p.uzlu;
    if (p.tah_nalezen) o << " pv " << tah_na_uci(p.nejlepsi_tah);
    self->info_radky_.push_back(o.str());
}

std::vector<std::string> rozdel(const std::string &s) {
    std::vector<std::string> ven;
    std::istringstream in(s);
    std::string w;
    while (in >> w) ven.push_back(w);
    return ven;
}

UciEngine::UciEngine() {
    cisty_stav();
}

// Úplně čistý stav, jako po spuštění programu: globály v .so přežijí konec
// instance, takže nová instance nesmí nic zdědit. Tabulky a alokace hash
// jsou jen jednou (inicializuj_hledani), zbytek se nuluje pokaždé.
//
// Statické proměnné uvnitř PreEvaluate (hashing_*, last_crafty_is_white,
// last_trojan_check, last_clear, ř. 5754-5759) se zvenku vynulovat nedají.
// Rozhodují jen o tom, jestli PreEvaluate na začátku prvního hledání zavolá
// ClearHashTableScores, a tabulky jsou v tu chvíli prázdné - na výsledek to
// vliv nemá (test ve tools/crafty/san.cpp, oddíl 5, porovnává uzly).
void UciEngine::cisty_stav() {
    inicializuj_hledani();
    std::memset(static_cast<void *>(local[0]), 0, sizeof(TREE));
    inicializuj();                 // strom local[0] a výchozí pozice
    InitializeHashTables();
    std::memset(history_w, 0, sizeof(history_w));
    std::memset(history_b, 0, sizeof(history_b));
    std::memset(static_cast<void *>(&last_pv), 0, sizeof(last_pv));
    last_mate_score = 0;
    last_value = 0;
    last_root_value = 0;
    root_value = 0;
    seznam_Next_Best_Move.clear();
    status_Next_Best_Move = 0;
    abort_search = 0;
    hlaseni_prubehu = nullptr;
    hlaseni_prubehu_kontext = nullptr;
    // Crafty volá DisplayPV (a s ním průběžné hlášení) až po noise_level
    // uzlech (ř. 4738, 6403). Nula = po každé dokončené iteraci, jako
    // u RedMAXu. Je to Crafty volba "noise", ne změna kódu.
    noise_level = 0;
    nastav_vychozi();
}

void UciEngine::nova_hra() {
    cisty_stav();
}

void UciEngine::zastav_hledani() {
    abort_search = 1;
}

// --- najde tah podle zápisu e2e4 / a7a8q a zahraje ho -----------------------
bool UciEngine::najdi_a_zahraj(const std::string &zapis) {
    if (zapis.size() < 4) return false;
    TREE *const tree = local[0];
    const std::vector<int> tahy = legalni_tahy_crafty();

    int nalezeny = 0;
    for (int mv : tahy)
        if (tah_na_uci(mv) == zapis) { nalezeny = mv; break; }
    // Proměna bez určené figury: doplní se dáma, stejně jako u RedMAXu.
    if (!nalezeny && zapis.size() == 4)
        for (int mv : tahy)
            if (tah_na_uci(mv) == zapis + "q") { nalezeny = mv; break; }
    if (!nalezeny) return false;

    // Tudy jdou obě cesty, jak se tah dostane do partie: `applymove` z UI
    // i každý tah z `position ... moves ...`. MakeMoveRoot přenese pozici
    // do position[0], vede Rule50Moves a zapíše klíč do historie opakování.
    MakeMoveRoot(tree, nalezeny, wtm);
    if (!wtm) move_number++;       // táhl černý
    wtm = ChangeSide(wtm);
    zkrat_historii(tree->replist_w, tree->rephead_w);
    zkrat_historii(tree->replist_b, tree->rephead_b);
    return true;
}

// --- position startpos|fen ... [moves ...] ----------------------------------
bool UciEngine::nastav_pozici(const std::vector<std::string> &slova, size_t od) {
    size_t i = od;
    if (i >= slova.size()) return false;
    TREE *const tree = local[0];

    if (slova[i] == "startpos") {
        nastav_vychozi();
        ++i;
    } else if (slova[i] == "fen") {
        ++i;
        std::string fen;
        while (i < slova.size() && slova[i] != "moves") {
            if (!fen.empty()) fen += ' ';
            fen += slova[i];
            ++i;
        }
        // FEN výchozí pozice (tak ji posílá aplikace) jako `startpos`: jinak
        // by SetBoard partii od začátku vedl jako střední hru a hodnocení by
        // se lišilo podle toho, kterou cestou pozice přišla.
        if (pole_fen(fen, 4) == START_FEN_4) {
            nastav_vychozi();
        } else {
            if (!nastav_fen(fen.c_str())) return false;
            oprav_ep_z_fen();
        }
        // SetBoard pole 5 a 6 nečte (C1, nález 2): půltahy a číslo tahu
        // z FEN sem dosadit ručně, jinak nefunguje remiza50 z FEN.
        const std::string pultahy = pole_fen_i(fen, 4);
        const std::string cislo = pole_fen_i(fen, 5);
        if (!pultahy.empty()) {
            const int h = std::atoi(pultahy.c_str());
            Rule50Moves(0) = static_cast<unsigned char>(h < 0 ? 0 : (h > 255 ? 255 : h));
        }
        if (!cislo.empty()) {
            const int c = std::atoi(cislo.c_str());
            move_number = c < 1 ? 1 : c;
        }
    } else {
        return false;
    }

    if (i < slova.size() && slova[i] == "moves") {
        ++i;
        for (; i < slova.size(); ++i)
            if (!najdi_a_zahraj(slova[i])) return false;
    }
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
    if (mala_pismena(jmeno) == "depth") {
        max_hloubka_ = std::min(max_hloubka_hledani, std::max(1, std::atoi(hodnota.c_str())));
        return {};
    }
    return {"info string neznama volba: " + jmeno};
}

// --- go ---------------------------------------------------------------------
std::vector<std::string> UciEngine::proved_go(const std::vector<std::string> &slova) {
    int hloubka = max_hloubka_;
    int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0;
    int movetime = 0;
    bool nekonecno = false;
    bool zadana_hloubka = false;

    for (size_t i = 1; i < slova.size(); ++i) {
        const std::string &s = slova[i];
        auto cislo = [&](int implicit) {
            return (i + 1 < slova.size()) ? std::atoi(slova[++i].c_str()) : implicit;
        };
        if      (s == "depth")     { hloubka = std::max(1, cislo(hloubka)); zadana_hloubka = true; }
        else if (s == "movetime")  movetime = cislo(0);
        else if (s == "wtime")     wtime = cislo(0);
        else if (s == "btime")     btime = cislo(0);
        else if (s == "winc")      winc = cislo(0);
        else if (s == "binc")      binc = cislo(0);
        else if (s == "movestogo") movestogo = cislo(0);
        else if (s == "infinite")  nekonecno = true;
    }
    if (hloubka > max_hloubka_hledani) hloubka = max_hloubka_hledani;

    int cas = 0;
    if (movetime > 0) {
        cas = movetime;
    } else if (!nekonecno) {
        const int zbyva = wtm ? wtime : btime;
        const int prir  = wtm ? winc  : binc;
        if (zbyva > 0) cas = rozpocitej_cas(zbyva, prir, movestogo);
    }

    // Když rozhoduje čas a hloubku nikdo nezadal, ať hloubka nebrzdí:
    // search_depth = 0 znamená v Iterate "bez omezení" (ř. 4839), strop je
    // pak 60 iterací a zastaví ho stopky. Jako u RedMAXu.
    if ((cas > 0 || nekonecno) && !zadana_hloubka) hloubka = 0;
    // Bez času jako v GUI originálu: "Sec: 99999".
    const double cas_ms = (cas > 0) ? static_cast<double>(cas) : 99999000.0;

    prubeh_.vynuluj();
    info_radky_.clear();
    hlaseni_prubehu = &UciEngine::hlaseni_trampolina;
    hlaseni_prubehu_kontext = this;
    last_root_value = 0;           // ať se při přerušení nevypíše skóre minulého hledání

    const auto t0 = std::chrono::steady_clock::now();
    int tah = hledej(hloubka, cas_ms);
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - t0).count();
    hlaseni_prubehu = nullptr;
    hlaseni_prubehu_kontext = nullptr;
    const bool preruseno = abort_search != 0;

    TREE *const tree = local[0];
    const std::vector<int> tahy = legalni_tahy_crafty();

    std::vector<std::string> ven = info_radky_;
    std::ostringstream info;
    info << "info depth " << (tree->pv[0].pathd > 0 ? static_cast<int>(tree->pv[0].pathd) : 1)
         << ' ' << skore_do_uci(last_root_value)
         << " nodes " << tree->nodes_searched
         << " time " << ms;
    if (tree->pv[0].pathl >= 1 && !tahy.empty()) {
        info << " pv";
        for (int i = 1; i <= static_cast<int>(tree->pv[0].pathl); ++i)
            info << ' ' << tah_na_uci(tree->pv[0].path[i]);
    }
    ven.push_back(info.str());
    if (preruseno) ven.push_back("info string hledani preruseno");

    if (tahy.empty()) {
        ven.push_back(std::string("info string ") + (strana_na_tahu_v_sachu() ? "mat" : "pat"));
        ven.push_back("bestmove 0000");
        return ven;
    }

    // Tah z hledání musí být mezi legálními. hledej() ho ověřuje přes
    // ValidMove (oprava č. 8), tohle je druhá pojistka proti tahu, který by
    // byl platný jen pseudolegálně nebo zbyl z minulého hledání (přerušení
    // před první dokončenou iterací). Náhrada je první kořenový tah - ten
    // RootMoveList seřadil jako nejlepší a nelegální vyřadil.
    const auto je_legalni = [&](int mv) {
        return std::find(tahy.begin(), tahy.end(), mv) != tahy.end();
    };
    if (!je_legalni(tah)) {
        tah = (n_root_moves > 0 && je_legalni(root_moves[0].move)) ? root_moves[0].move : tahy[0];
        ven.push_back("info string tah z hledani nebyl legalni, nahrazen " + tah_na_uci(tah));
    }
    ven.push_back("bestmove " + tah_na_uci(tah));
    return ven;
}

// --- rozcestník --------------------------------------------------------------
std::vector<std::string> UciEngine::prikaz(const std::string &radek) {
    const std::vector<std::string> slova = rozdel(radek);
    if (slova.empty()) return {};

    const std::string &c = slova[0];

    if (c == "uci") {
        return {
            std::string("id name ") + JMENO,
            std::string("id author ") + AUTOR,
            "option name Depth type spin default 6 min 1 max " + std::to_string(max_hloubka_hledani),
            "uciok"
        };
    }

    if (c == "isready")    return {"readyok"};
    if (c == "ucinewgame") { nova_hra(); return {}; }
    if (c == "quit")       { skoncit_ = true; return {}; }
    if (c == "stop")       { zastav_hledani(); return {}; }
    if (c == "setoption")  return nastav_volbu(slova);
    if (c == "go")         return proved_go(slova);

    if (c == "position") {
        if (!nastav_pozici(slova, 1)) return {"info string neplatne position"};
        return {};
    }

    // --- příkazy nad rámec UCI, pro UI ---

    if (c == "legalmoves") {
        std::string s;
        for (const std::string &t : legalni_tahy()) {
            if (!s.empty()) s += ' ';
            s += t;
        }
        return {s};
    }

    // Dvojtečka je oddělovač proto, že se nevyskytuje ani v zápisu UCI, ani
    // v SAN. Rovnítko ne - to je uvnitř zápisu proměny (e8=Q).
    if (c == "sanmoves") {
        std::string s;
        for (const auto &t : tahy_se_zapisem()) {
            if (!s.empty()) s += ' ';
            s += t.first;
            s += ':';
            s += t.second;
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
        TREE *const tree = local[0];
        const bool je_tah = !legalni_tahy_crafty().empty();
        const char *st = !je_tah ? (strana_na_tahu_v_sachu() ? "mat" : "pat")
                       : (Rule50Moves(0) >= 100) ? "remiza50"
                       : "normalni";
        return {zapis_fen(tree, 0, wtm), std::string("stav ") + st};
    }

    return {"info string neznamy prikaz: " + c};
}

}  // namespace crafty
