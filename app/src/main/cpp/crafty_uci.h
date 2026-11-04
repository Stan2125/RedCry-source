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
//  Crafty - vrstva UCI
//
//  Dvojče red_max_uci.h: stejné metody, stejné příkazy, stejný výstup.
//  Záměrně bez vstupu a výstupu - příkaz jde dovnitř jako řetězec, odpovědi
//  vypadnou jako seznam řádků. Použije ji konzolová aplikace
//  (tools/crafty/uci_main.cpp) i most do Androidu (crafty_jni_bridge.cpp).
//
//  Nad rámec UCI zná tytéž příkazy jako RedMAX:
//      legalmoves          seznam legálních tahů v aktuální pozici
//      sanmoves            totéž i se zápisem v SAN, jako <uci>:<san>
//      applymove <tah>     zahraje tah a přepne stranu
//      d                   FEN a stav (normalni / mat / pat / remiza50)
//
//  ROZDÍLY PROTI REDMAXU, které stojí za přečtení:
//   - Stav Crafty jsou globály (crafty_core, crafty_search), ne objekt.
//     Instance UciEngine je proto nejvýš jedna naráz - hlídá to most JNI
//     (druhé nativeNew vrátí 0). Konstruktor uvede globály do úplně
//     čistého stavu, takže nová instance po zrušení staré začíná jako
//     první (PORT-CRAFTY.md, oddíl 6a).
//   - Historii partie pro opakování nedrží tahle vrstva, ale Crafty sám
//     (replist_w/b, plní je SetBoard a MakeMoveRoot).
//   - Žádný tah = `bestmove 0000` (RedMAX píše `bestmove (none)`; Kotlin
//     bere oba tvary, viz bestMove v ChessEngine.kt).
//   - Volba je jen Depth. Hash tabulka Crafty má pevnou velikost (12 MB,
//     log_hash=18) a přepínače hledání se nevystavují.
//
//  Includovat jako POSLEDNÍ, po standardních hlavičkách (makra Crafty).
// ---------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "crafty_search.h"

namespace crafty {

// Snímek běžícího hledání. Čte se z JINÉHO vlákna než to, které hledá,
// proto samé atomiky a žádný zámek - UI vlákno se nesmí zablokovat.
class PrubeznyStav {
public:
    void vynuluj();
    void zapis(const PrubeznyVysledek &p);
    // "depth 5 score cp 12 nodes 13314 pv e2e4", nebo prázdné, když nic neběželo
    std::string text() const;

private:
    std::atomic<int>       hloubka_{0};
    std::atomic<int>       skore_{0};     // skóre Crafty, z pohledu strany na tahu
    std::atomic<long long> uzlu_{0};
    std::atomic<int>       tah_{0};       // tah ve formátu Crafty, 0 = žádný
};

class UciEngine {
public:
    UciEngine();

    UciEngine(const UciEngine &) = delete;
    UciEngine &operator=(const UciEngine &) = delete;

    // Zpracuje jeden příkazový řádek a vrátí řádky odpovědi.
    std::vector<std::string> prikaz(const std::string &radek);

    // Přeruší běžící hledání (abort_search je atomický). Volatelné z jiného
    // vlákna než prikaz(); zámek nebere a brát nesmí - hledání ho drží.
    void zastav_hledani();

    bool ma_skoncit() const { return skoncit_; }

    // Průběh běžícího hledání. Bez zámku, volatelné z jiného vlákna.
    const PrubeznyStav &prubeh() const { return prubeh_; }

private:
    void cisty_stav();
    void nova_hra();
    bool nastav_pozici(const std::vector<std::string> &slova, size_t od);
    bool najdi_a_zahraj(const std::string &zapis);
    std::vector<std::string> proved_go(const std::vector<std::string> &slova);
    std::vector<std::string> nastav_volbu(const std::vector<std::string> &slova);

    static void hlaseni_trampolina(void *kontext, const PrubeznyVysledek &p);

    PrubeznyStav             prubeh_{};
    std::vector<std::string> info_radky_{};
    int                      max_hloubka_ = 6;
    bool                     skoncit_ = false;
};

// Nejvyšší hloubka, kterou hledání Crafty unese: iterační smyčka v Iterate
// jde do iteration_depth <= 60 (ř. 4688). MAXPLY (65) je strop délky cesty,
// ne počtu iterací. Ohlašuje se v `option name Depth ... max`.
constexpr int max_hloubka_hledani = 60;

// pomocné: rozdělení řádku na slova
std::vector<std::string> rozdel(const std::string &s);

// Legální tahy v aktuální pozici (tree->position[0], wtm) v zápisu UCI,
// v pořadí generátoru Crafty (braní, pak ostatní). Pozici vrátí, jak ji dostal.
std::vector<std::string> legalni_tahy();

// Legální tahy i se zápisem v SAN: první prvek dvojice je zápis UCI, druhý
// SAN z OutputMove Crafty. Množina prvních prvků je totožná s legalni_tahy().
std::vector<std::pair<std::string, std::string>> tahy_se_zapisem();

// Tah Crafty na zápis UCI ("e2e4", "a7a8q").
std::string tah_na_uci(int move);

// Rozpočet času na jeden tah z `wtime`/`btime`, stejně jako u RedMAXu.
int rozpocitej_cas(int zbyva_ms, int prirustek_ms, int tahu_do_kontroly);

}  // namespace crafty
