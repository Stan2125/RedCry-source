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
//  Crafty 18.15 (RedCrafty) - hodnocení a hledání
//
//  Popis převodu je v crafty_search.h a v docs/POZNAMKY-CRAFTY.md. Těla
//  funkcí jsou řádek po řádku z RedCrafty_481_f.cpp; nad každou je rozsah
//  řádků, odkud pochází. Komentáře uvnitř jsou Hyattovy a Stanovy.
// ---------------------------------------------------------------------------

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "crafty_search.h"

namespace crafty {

// --- globály -----------------------------------------------------------------
// RedCrafty_481_f.h, ř. 548-549
POSITION       display;
FILE           *log_file;

// RedCrafty_481_f.h, ř. 551-555
FILE           *books_file;
FILE           *normal_bs_file;
FILE           *history_file;
FILE           *auto_file;


// RedCrafty_481_f.h, ř. 682-687
BITBOARD       white_minor_pieces;
BITBOARD       black_minor_pieces;
BITBOARD       white_pawn_race_wtm[64];
BITBOARD       white_pawn_race_btm[64];
BITBOARD       black_pawn_race_wtm[64];
BITBOARD       black_pawn_race_btm[64];

// RedCrafty_481_f.h, ř. 691-728
HASH_ENTRY      *trans_ref_a_orig;
HASH_ENTRY      *trans_ref_b_orig;
PAWN_HASH_ENTRY *pawn_hash_table_orig;
HASH_ENTRY      *trans_ref_a;
HASH_ENTRY      *trans_ref_b;
PAWN_HASH_ENTRY *pawn_hash_table;
int            hash_maska;
int            hash_maskb;
unsigned int   pawn_hash_mask;
signed char    pval_b[64];
signed char    nval_b[64];
signed char    bval_b[64];
signed char    rval_b[64];
signed char    qval_b[64];
signed char    kval_bn[64];
signed char    kval_bk[64];
signed char    kval_bq[64];
signed char    black_outpost[64];
signed char    king_defects_b[64];
int            move_actually_played;
int            whisper_depth;
unsigned int   opponent_start_time, opponent_end_time;
char           cmd_buffer[512];
int            auto232;
int            auto232_delay;
int            book_move;
ROOT_MOVE      root_moves[256];
int       display_options =            4095-256-512;
int       early_exit =                           99;
int       search_depth =                          0;
int       EGTB_search =                           0;
unsigned int search_nodes =                       0;
int       search_move =                           0;
volatile int   quit=0;
int       output_format =                         0;




// RedCrafty_481_f.h, ř. 737-756
int       hash_table_size =                  262144;
//int       hash_table_size =                   65536;
int       pawn_hash_table_size =              32768;
// oprava č. 3 (PORT-CRAFTY.md, oddíl 3): log_hash byl 16 při
// hash_table_size=262144, takže se adresovala jen čtvrtina tabulky.
// hash_table_size se teď odvozuje z log_hash v inicializuj_hledani().
int       log_hash =                             18;
int       log_pawn_hash =                        15;
signed char transposition_id =                    0;
int       initialized =                           0;
signed char resign =                              9;
signed char resign_count =                        5;
signed char draw_count =                         10;
int       crafty_is_white =                       0;
int       ics =                                   0;
int       xboard =                                0;
int       ponder_moves[220];
int       num_ponder_moves;
int       last_opponent_move =                    0;
int       force =                                 0;
int       trojan_check =                          0;
int       computer_opponent =                     0;
int       draw_score[2] =                    {0, 0};

// RedCrafty_481_f.h, ř. 758-774
int       tc_time_remaining =                180000;
int       tc_time =                          180000;
int       tc_time_remaining_opponent =       180000;
int       tc_moves_remaining =                   60;
int       tc_moves =                             60;
signed char resign_counter =                      0;
signed char draw_counter =                        0;
int       usage_level =                           0;
int       input_status =                          0;
int       pong =                                  0;
int       time_limit =                          100;
signed char puzzling =                            0;
signed char thinking =                            0;
signed char pondering =                           0;
int       time_used_opponent =                    0;
int            ponder_value;
int            last_root_value;

// RedCrafty_481_f.h, ř. 778-922
signed char draw_offer_pending =                  0;
int       abs_draw_score =                        0;
int       tc_increment =                          0;
int            last_value;
char      book_hint[512] =                     {""};
int       null_min =                       3*INCPLY;  /* R=2 */
int       null_max =                       4*INCPLY;  /* R=3 */
signed char booking =                             0;
int            easy_move;
// port: atomický, protože ho zapisuje i jiné vlákno (stop); čte se na
// týchž místech jako v originálu
std::atomic<signed char> abort_search;
int            root_wtm;
unsigned int   parallel_splits;
unsigned int   parallel_stops;
unsigned int   max_split_blocks;
int       EGTB_draw =                             0;
int       EGTB_use =                              0;
int       EGTBlimit =                             0;
int            burp;
int            n_root_moves;
int            root_value;
int            iteration_depth;
int            root_alpha;
int            root_beta;
int            root_print_ok;
int       trace_level =                           0;
int       analyze_mode =                          0;
unsigned int noise_level =                   100000;
int       EGTB_cache_size =      EGTB_CACHE_DEFAULT;
void      *EGTB_cache =                    (void*)0;
FILE           *computer_bs_file;
int       EGTB_setup =                            0;
int       test_mode =                             0;

/*  score for bishop pair varies depending on how many pawns are
    on the board (0-8)                                           */
signed char    bishop_pair[9] = {20, 20, 20, 20, 20, 20, 20, 15, 8};
  const char square_color[64]  = { 1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1,
                                   1, 0, 1, 0, 1, 0, 1, 0,
                                   0, 1, 0, 1, 0, 1, 0, 1 };
  const char b_n_mate_dark_squares[64] =
                             { 100,  90,  80,  70,  60,  50,  40,  30,
                                90,  80,  70,  60,  50,  40,  30,  40,
                                80,  70,  60,  50,  40,  30,  40,  50,
                                70,  60,  50,  40,  30,  40,  50,  60,
                                60,  50,  40,  30,  40,  50,  60,  70,
                                50,  40,  30,  40,  50,  60,  70,  80,
                                40,  30,  40,  50,  60,  70,  80,  90,
                                30,  40,  50,  60,  70,  80,  90, 100};

  const char b_n_mate_light_squares[64] =
                              { 30,  40,  50,  60,  70,  80,  90, 100,
                                40,  30,  40,  50,  60,  70,  80,  90,
                                50,  40,  30,  40,  50,  60,  70,  80,
                                60,  50,  40,  30,  40,  50,  60,  70,
                                70,  60,  50,  40,  30,  40,  50,  60,
                                80,  70,  60,  50,  40,  30,  40,  50,
                                90,  80,  70,  60,  50,  40,  30,  40,
                               100,  90,  80,  70,  60,  50,  40,  30};

  const char mate[64] =       {100, 96, 93, 90, 90, 93, 96,100,
                                96, 80, 70, 60, 60, 70, 80, 96,
                                93, 70, 60, 50, 50, 60, 70, 93,
                                90, 60, 50, 40, 40, 50, 60, 90,
                                90, 60, 50, 40, 40, 50, 60, 90,
                                93, 70, 60, 50, 50, 60, 70, 93,
                                96, 80, 70, 60, 60, 70, 80, 96,
                               100, 96, 93, 90, 90, 93, 96,100};



int cbEGTBCompBytes = 0;
FILE           *book_file;
FILE           *book_lrn_file;
FILE           *position_file;
FILE           *position_lrn_file;
int            temper_b[64], temper_w[64];
int            tropism[128];
char           pawn_rams[9];
int       incheck_depth =                        60;
int       onerep_depth =                         45;
int       pushpp_depth =                         45;
int       recap_depth =                          45;
int       mate_depth =                           45;
char      hint[512] =                          {""};
char      version[6] =                    {VERSION};
int       number_of_GMs =                        9;

  int       batch_mode =                            0; /* no asynch reads */
  int       swindle_mode =                          1; /* try to swindle */
  int       annotate_mode =                         0;
  int       call_flag =                             0;
  int       crafty_rating =                      2500;
  int       opponent_rating =                    2500;
  int       last_search_value =                   500;
  int       DGT_active =                            0;
  int       to_dgt =                                0;
  int       from_dgt =                              0;
  int       pgn_suggested_percent =                 0;
  char      pgn_event[32] = {"?"};
  char      pgn_site[32] = {"?"};
  char      pgn_date[32] = {"????.??.??"};
  char      pgn_round[32] = {"?"};
  char      pgn_white[64] = {"unknown"};
  char      pgn_white_elo[32] = {""};
  char      pgn_black[64] = {"Crafty " VERSION};
  char      pgn_black_elo[32] = {""};
  char      pgn_result[32] = {"*"};
  int       number_of_blockers =                    0;
  char      blocker_list[512][20] = {
                                     {""}};
  int       number_of_IMs =                         0;
  char      IM_list[512][20] =       {
                                      {""}};
  int       number_to_play =                        0;
  char      toplay_list[512][20] =  {
                                     {""}};
  int       number_of_computers =                   0;
  char      computer_list[512][20] = {
                                      {""}};
  int       number_auto_kibitzers =                 7;

  char      auto_kibitz_list[64][20] = {
                                      {"diepx"},
                                      {"ferret"},
                                      {"hossa"},
                                      {"lambchop"},
                                      {"moron"},
                                      {"otter"},
                                      {"zarkovx"}};
  int       log_id =                                1;
  char           log_filename[64];
  char           history_filename[64];
  char      channel_title[32] =                  {""};
  char      book_path[128] =                {BOOKDIR};
  char      log_path[128] =                  {LOGDIR};
  char      tb_path[128] =                    {TBDIR};
  char      rc_path[128] =                    {RCDIR};
  int            total_moves;
  int       post =                                  0;


// RedCrafty_481_f.h, ř. 924-944
  const char empty[9]  = {0,'1','2','3','4','5','6','7','8'};

  const char king_tropism_n[8]      = {3,3,3,2,1,0,0,0};
  const char king_tropism_b[8]      = {3,3,3,2,1,0,0,0};
  const char king_tropism_r[8]      = {3,3,3,2,1,0,0,0};
  const char king_tropism_at_r[8]   = {4,3,1,0,0,0,0,0};
  const char king_tropism_q[8]      = {4,4,4,2,1,0,0,0};
  const char king_tropism_at_q[8]   = {5,5,3,0,0,0,0,0};

  int       show_book =                             0;
  int            book_learn_eval[LEARN_INTERVAL];
  int            book_learn_depth[LEARN_INTERVAL];
  BOOK_POSITION  book_buffer[BOOK_CLUSTER_SIZE];
  BOOK_POSITION  books_buffer[BOOK_CLUSTER_SIZE];
  int       book_random =                           1;
  int       book_accept_mask =                    ~03;
  int       book_reject_mask =                      3;
  int       book_search_trigger =                  20;
  int       use_asymmetry =                         1;



// RedCrafty_481_f.h, ř. 1060-1341

  signed char  pval_w[64] = { 0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  3,  3,  0,  0,  0,
                              0,  0,  2,  3,  3,  0,  0,  0,
                              0,  0,  2,  2,  2,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0,
                              0,  0,  0,  0,  0,  0,  0,  0};

  signed char  nval_w[64] = { -8, -8, -8, -8, -8, -8, -8, -8,
                              -8, -8,  0,  0,  0,  0, -8, -8,
                              -8,  2,  2,  2,  2,  2,  2, -8,
                              -8,  2,  4,  4,  4,  4,  2, -8,
                              -8,  2,  6,  6,  6,  6,  2, -8,
                              -8,  2,  6,  6,  6,  6,  2, -8,
                             -24,-24,  4,  4,  4,  4,-24,-24,
                             -32,-16, -8, -8, -8, -8,-16,-32};

  signed char  bval_w[64] = {-16,-16, -8, -8, -8, -8,-16,-16,
                             -16,  2,  0,  0,  0,  0,  2,-16,
                              -4,  2,  2,  2,  2,  2,  2, -4,
                              -4,  2,  4,  4,  4,  4,  2, -4,
                              -4,  2,  6,  6,  6,  6,  2, -4,
                              -4,  2,  6,  6,  6,  6,  2, -4,
                             -16,  2,  4,  4,  4,  4,  2,-16,
                             -16,-16,  0,  0,  0,  0,-16,-16};

  signed char  rval_w[64] = {  0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0,
                               0,  0,  4,  6,  6,  4,  0,  0};

  signed char  qval_w[64] = {-10, -8,  0,  0,  0,  0, -8,-10,
                             -10,  2,  8,  8,  8,  8,  2,-10,
                             -10,-10,  4, 10, 10,  4,-10,-10,
                             -10,  2, 10, 12, 12, 10,  2,-10,
                               0,  2, 10, 12, 12, 10,  2,  0,
                               0,  2,  4, 10, 10,  4,  2,  0,
                             -15,  0,  4,  5,  5,  4,  0,-15,
                               0,  0,  0,  0,  0,  0,  0,  0};

  signed char  kval_wn[64] ={-50,-40,-30,-20,-20,-30,-40,-50,
                             -30,-20,-10,  0,  0,-10,-20,-30,
                             -30,-10,  0, 10, 10,  0,-10,-30,
                             -30,-10, 20, 30, 30, 20,-10,-30,
                             -30,-10, 30, 40, 40, 30,-10,-30,
                             -30,-10, 30, 40, 40, 30,-10,-30,
                             -30,-30,  0,  0,  0,  0,-30,-30,
                             -50,-30,-30,-30,-30,-30,-30,-50};

  signed char  kval_wk[64] ={-90,-70,-50,-40,-30,-20,-20,-30,
                             -70,-50,-30,-20,-10,-10,-10,-10,
                             -70,-50,-30,-10, 20, 20, 20,  0,
                             -70,-50,-30,-10, 30, 30, 30,  0,
                             -70,-50,-30,-10, 30, 30, 30,  0,
                             -70,-50,-30,-10, 30, 30, 30,  0,
                             -70,-50,-30,-30,  0,  0,  0,-10,
                             -90,-70,-50,-30,-30,-30,-30,-30};

  signed char  kval_wq[64] ={-30,-20,-20,-30,-40,-50,-70,-90,
                             -10,-10,-10,-10,-20,-30,-50,-70,
                               0, 20, 20, 20,-10,-30,-50,-70,
                               0, 30, 30, 30,-10,-30,-50,-70,
                               0, 30, 30, 30,-10,-30,-50,-70,
                               0, 30, 30, 30,-10,-30,-50,-70,
                             -10,  0,  0,  0,-20,-30,-50,-70,
                             -30,-30,-30,-30,-30,-50,-70,-90};

  signed char     white_outpost[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 4, 4, 0, 0, 0,
                                        0, 0, 8,10,10, 8, 0, 0,
                                        0, 0, 8,12,12, 8, 0, 0,
                                        0, 0, 4, 8, 8, 4, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0 };

  signed char king_defects_w[64] ={ 1, 0, 2, 3, 3, 2, 0, 1,
                                    1, 1, 2, 3, 3, 2, 1, 1,
                                    3, 3, 3, 3, 3, 3, 3, 3,
                                    4, 4, 4, 4, 4, 4, 4, 4,
                                    5, 5, 5, 5, 5, 5, 5, 5,
                                    6, 6, 6, 6, 6, 6, 6, 6,
                                    7, 7, 7, 7, 7, 7, 7, 7,
                                    8, 8, 8, 8, 8, 8, 8, 8};

/*
   the following values are for king safety.  The 'temper' array is used
   to scale the number of defects found into some sort of 'sane' score so that
   small pertubations in king safety can produce signficant scoring changes,
   but large pertubations won't cause Crafty to sacrifice pieces.
*/
  const int temper[64] =
    {   0,   4,  15,  24,  36,  42,  54,  72, /*   0-   7 */
       81,  90,  99, 108, 117, 126, 135, 144, /*   8-  15 */
      148, 152, 156, 160, 164, 168, 172, 175, /*  16-  23 */
      180, 180, 180, 180, 180, 180, 180, 180, /*  24-  31 */
      180, 180, 180, 180, 180, 180, 180, 180, /*  32-  39 */
      180, 180, 180, 180, 180, 180, 180, 180, /*  47-  47 */
      180, 180, 180, 180, 180, 180, 180, 180, /*  48-  55 */
      180, 180, 180, 180, 180, 180, 180, 180};/*  56-  63 */
/*
   the following array cuts the king safety term down to a value that can
   be used in the king tropism calculation.  this lets the safety of a king
   influence how important getting pieces closer to the king really is.  and
   is primarily used to help when both sides castle to the same side of the
   board which means that if I disrupt my kingside, I am also disrupting my
   opponent's kingside.  now I can figure out who comes out better in such
   situations. These values are in units of 1/16th.  Which means that a value
   of 16 will use the entire tropism value, a value of 32 will use 2x the
   normal tropism value, and a value of 8 will use 1/2 the normal value.
   this array is indexed by the tempered king safety value, and lower-
   numbered elements correspond to safe king positions, while higher-numbered
   elements represent disrupted kingsides.
*/
  const char ttemper[64] =   { 16, 16, 16, 16, 17, 17, 18, 18, /*   0-   7 */
                               19, 19, 20, 20, 21, 21, 22, 22, /*   8-  15 */
                               23, 23, 24, 24, 25, 25, 26, 26, /*  16-  23 */
                               27, 27, 28, 28, 29, 29, 30, 30, /*  24-  31 */
                               31, 31, 32, 32, 32, 32, 32, 32, /*  32-  39 */
                               32, 32, 32, 32, 32, 32, 32, 32, /*  40-  47 */
                               32, 32, 32, 32, 32, 32, 32, 32, /*  48-  55 */
                               32, 32, 32, 32, 32, 32, 32, 32};/*  56-  63 */
/*
   penalty for a pawn that is missing from its initial square in front
   of the castled king.  ie h3 would count as 'one missing pawn' while
   h4 counts as two since two squares in front of the king are now missing
   a pawn shelter.
*/
  const char missing[8] =         { 0, 1, 2, 3, 4, 5, 6, 7};
/*
   penalty for a fully open file in front of the castled king.
*/
  const char openf[4] =           { 0, 8, 13, 16};
/*
   penalty for a half-open (one side's pawn is missing) file in front of
   the castled king.
*/
  const char hopenf[4] =          { 0, 3, 7, 9};
/*
   this value controls king safety asymmetry.  0 is fully symmetrical.
   this works by modifying the king safety scores for the opponent by
   the amount specified.  -20 reduces the opponent's king safety scores,
   making crafty's king safety more important.  +20 increases the opponent's
   king safety, making Crafty's less important (this will tend to increase
   aggressiveness while - values will make Crafty more passive/defensive.)
*/
  int king_safety_asymmetry =                  -40;
/*
   this value scales king safety up or down equally for both sides.  A
   value of 100 leaves the values as they are.  values below 100 drop
   the king safety scores for both sides proportionally.
*/
  int king_safety_scale =                      100;
/*
   this value scales blocked pawn scores up/down.  Increasing this value
   will make Crafty try harder to not block pawn positions, which will aim
   for more open positions.
*/
  int blocked_scale =                          100;
/*
   this value scales passed pawn scores up/down.  Increasing this value
   will make Crafty evaluate some passed pawn scores higher, such as the
   "outside passed pawn scoring."
*/
  int passed_scale =                           100;
/*
   this value scales pawn scores up/down.  Increasing this value will
   make Crafty try harder to preserve its pawn structure / wreck the
   opponent's pawn structure.
*/
  int pawn_scale =                             100;
/*
   this value scales king tropism up or down.  the default is 100 which
   uses the built-in scores.  150 will increase tropism scores by 50%
   which will make the program more aggressive, but probably less
   positionally aware as a result.
*/
  int king_safety_tropism =                    100;
/*
   the following is basically a 'defect' table for kings on a specific
   square.  this lets it figure out that the king doesn't want to go to
   (say f3 or e4 or anywhere near the center) with heavy pieces still on
   the board.
*/

  char      GM_list[512][20] =       {
                                      {"astr"},
                                      {"dreev"},
                                      {"garompon"},
                                      {"jean-reno"},
                                      {"lorenzo"},
                                      {"mecking"},
                                      {"nikephoros"},
                                      {"ruslam"},
                                      {"sweere"}};

  const int  king_tropism[128] =
                                { -25, -25, -20, -20, -15, -15, -10,  -5,
                                    0,   2,   5,  12,  15,  20,  30,  40,
                                   50,  60,  70,  80,  90, 100, 110, 120,
                                  125, 125, 130, 130, 135, 135, 140, 145,
                                  150, 160, 170, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180,
                                  180, 180, 180, 180, 180, 180, 180, 180};

  const char connected_passed_pawn_value[8] = { 0, 0, 0, 12, 24, 48, 100, 0};

  const int hidden_passed_pawn_value[8] ={ 0, 0, 0, 0, 0, 16, 24, 0};
  const int passed_pawn_value[8] ={ 0, 8, 16, 32, 60, 120, 160, 0};
  const char blockading_passed_pawn_value[8] = { 0, 8, 12, 16, 30, 45, 60, 0};

  const char isolated_pawn_value[9] = {0, 12, 24, 56, 66, 80, 90, 100, 110};
  const char isolated_pawn_of_value[9] = {0, 5, 10, 15, 20, 25, 30, 35, 40};

  const char doubled_pawn_value[7] ={ 0, 0, 5, 10, 20, 25, 30};

  const char pawn_rams_v[9] =      { 0, 0, 6, 24, 40, 64, 80, 94, 96};

  const char supported_passer[8] = { 0, 0, 0, 0, 12, 60, 100, 0};

  const char outside_passed[128] ={ 96, 48, 48, 48, 45, 42, 40, 40,
                                    36, 36, 32, 32, 28, 28, 24, 24,
                                    20, 20, 16, 16, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12,
                                    12, 12, 12, 12, 12, 12, 12, 12};

  const char majority[128] =      { 72, 36, 36, 36, 34, 31, 30, 30,
                                    27, 27, 24, 24, 21, 21, 18, 18,
                                    15, 15, 12, 12, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10,
                                    10, 10, 10, 10, 10, 10, 10, 10};

  const char     push_extensions[64] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0,
                                         1, 1, 1, 1, 1, 1, 1, 1,
                                         0, 0, 0, 0, 0, 0, 0, 0 };

  const int p_values[15] =       {QUEEN_VALUE,ROOK_VALUE,BISHOP_VALUE,0,
                                  KING_VALUE,KNIGHT_VALUE,PAWN_VALUE,
                                  0,PAWN_VALUE,KNIGHT_VALUE,KING_VALUE,
                                  0, BISHOP_VALUE,ROOK_VALUE,QUEEN_VALUE};

// RedCrafty_481_f.h, ř. 24-25
std::vector<std::string> seznam_Next_Best_Move;
int status_Next_Best_Move=0;

// --- port: globály -----------------------------------------------------------

std::chrono::steady_clock::time_point sta_cas;
double casovy_limit_ms=99999000.0;   // GUI mělo "Sec: 99999"
int port_bez_hashe=0;
HlaseniFn hlaseni_prubehu=nullptr;
void *hlaseni_prubehu_kontext=nullptr;
int hlaseni_print=0;
int hlaseni_illegal_move=0;
int hlaseni_clearing_hash_tables=0;
int hlaseni_clearing_hash_tables_50=0;
int hlaseni_malloc_failed=0;
int hlaseni_kontrol_casu=0;
int next_time_check=4096;
int nodes_between_time_checks=4096;

// --- funkce ------------------------------------------------------------------

// RedCrafty_481_f.cpp, ř. 1154-1162: KingPawnSquare
int KingPawnSquare(int pawn, int king, int queen, int ptm) {
  int pdist, kdist;
  pdist=abs((pawn>>3)-(queen>>3));
  kdist=(abs((king>>3)-(queen>>3)) > abs((king&7)-(queen&7))) ?
    abs((king>>3)-(queen>>3)) : abs((king&7)-(queen&7));
  if (!ptm) pdist++;
  if (pdist < kdist) return(0);
  else return(1);
}

// RedCrafty_481_f.cpp, ř. 1164-1223: InitializePieceMasks
void InitializePieceMasks(void) {
  int i, j;
/*
    initialize masks used to evaluate development, which includes
    minor piece squares.
*/
  white_minor_pieces=mask_2>>1 | mask_2>>5;
  black_minor_pieces=mask_2>>57 | mask_2>>61;
/*
    initialize masks used to evaluate pawn races.  these masks are
    used to determine if the opposing king is in a position to stop a
    passed pawn from racing down and queening.
*/
  for (i=0;i<64;i++) {
    white_pawn_race_wtm[i]=0;
    white_pawn_race_btm[i]=0;
    black_pawn_race_wtm[i]=0;
    black_pawn_race_btm[i]=0;
  }
  for (j=8;j<56;j++) {
    for (i=0;i<64;i++) {
/* white pawn, wtm */
      if (j < 16) {
        if (KingPawnSquare(j+8,i,(j&7)+56,1))
          white_pawn_race_wtm[j]=white_pawn_race_wtm[j] | SetMask(i);
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,1))
          white_pawn_race_wtm[j]=white_pawn_race_wtm[j] | SetMask(i);
      }
/* white pawn, ChangeSide(wtm) */
      if (j < 16) {
        if (KingPawnSquare(j+8,i,(j&7)+56,0))
          white_pawn_race_btm[j]=white_pawn_race_btm[j] | SetMask(i);
      }
      else {
        if (KingPawnSquare(j,i,(j&7)+56,0))
          white_pawn_race_btm[j]=white_pawn_race_btm[j] | SetMask(i);
      }
/* black pawn, wtm */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,0))
          black_pawn_race_wtm[j]=black_pawn_race_wtm[j] | SetMask(i);
      }
      else {
        if (KingPawnSquare(j,i,j&7,0))
          black_pawn_race_wtm[j]=black_pawn_race_wtm[j] | SetMask(i);
      }
/* black pawn, ChangeSide(wtm) */
      if (j > 47) {
        if (KingPawnSquare(j-8,i,j&7,1))
          black_pawn_race_btm[j]=black_pawn_race_btm[j] | SetMask(i);
      }
      else {
        if (KingPawnSquare(j,i,j&7,1))
          black_pawn_race_btm[j]=black_pawn_race_btm[j] | SetMask(i);
      }
    }
  }
}

// RedCrafty_481_f.cpp, ř. 1796-1823: InitializeHashTables
void InitializeHashTables(void) {
  int i;
  transposition_id=0;
  if (!trans_ref_a || !trans_ref_b) return;
  for (i=0;i<hash_table_size;i++) {
    (trans_ref_a+i)->word1=(BITBOARD) 7<<61;
    (trans_ref_a+i)->word2=0;
  }
  for (i=0;i<2*hash_table_size;i++) {
    (trans_ref_b+i)->word1=(BITBOARD) 7<<61;
    (trans_ref_b+i)->word2=0;
  }
  if (!pawn_hash_table) return;
  for (i=0;i<pawn_hash_table_size;i++) {
    (pawn_hash_table+i)->key=0;
    (pawn_hash_table+i)->p_score=0;
    (pawn_hash_table+i)->sta_protected=0;
    (pawn_hash_table+i)->black_defects_k=0;
    (pawn_hash_table+i)->black_defects_q=0;
    (pawn_hash_table+i)->white_defects_k=0;
    (pawn_hash_table+i)->white_defects_q=0;
    (pawn_hash_table+i)->passed_w=0;
    (pawn_hash_table+i)->passed_w=0;
    (pawn_hash_table+i)->outside=0;
    (pawn_hash_table+i)->candidates_w=0;
    (pawn_hash_table+i)->candidates_b=0;
  }
}

// RedCrafty_481_f.cpp, ř. 2306-2833: GenerateCheckEvasions
/* modified 08/23/99 */
/*
********************************************************************************
*                                                                              *
*   GenerateCheckEvasions() is used to generate moves when the king is in      *
*   check.                                                                     *
*                                                                              *
*   three types of check-evasion moves are generated:                          *
*                                                                              *
*   (1) generate king moves to squares that are not attacked by the opponent's *
*   pieces.  this includes capture and non-capture moves.                      *
*                                                                              *
*   (2) generate interpositions along the rank/file that the checking attack   *
*   is coming along (assuming (a) only one piece is checking the king, and     *
*   (b) the checking piece is a sliding piece [bishop, rook, queen]).          *
*                                                                              *
*   (3) generate capture moves, but only to the square(s) that are giving      *
*   check.  captures are a special case.  if there is one checking piece, then *
*   capturing it by any piece is tried.  if there are two pieces checking the  *
*   king, then the only legal capture to try is for the king to capture one of *
*   the checking pieces that is on an un-attacked square.                      *
*                                                                              *
********************************************************************************
*/
int* GenerateCheckEvasions(TREE *tree, int ply, int wtm, int *move) {
  BITBOARD target, targetc, targetp, piecebd, moves;
  BITBOARD padvances1, padvances2, pcapturesl, pcapturesr;
  BITBOARD padvances1_all, empty, checksqs;
  int from, to, temp;
  int king_square, checkers, checking_square;
  int check_direction1=0, check_direction2=0;

/*
 ----------------------------------------------------------
|                                                          |
|   first, determine how many pieces are attacking the     |
|   king and where they are, so we can figure out how to   |
|   legally get out of check.                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    king_square=WhiteKingSQ;
    checksqs=AttacksTo(tree,king_square) & BlackPieces;
    checkers=PopCnt(checksqs);
    if (checkers == 1) {
      checking_square=FirstOne(AttacksTo(tree,king_square) & BlackPieces);
      if (PcOnSq(checking_square) != -pawn)
        check_direction1=directions[checking_square][king_square];
      target=InterposeSquares(check_direction1,king_square,checking_square);
      target=target | (AttacksTo(tree,king_square) & BlackPieces);
      target=target | BlackKing;
    }
    else {
      target=BlackKing;
      checking_square=FirstOne(checksqs);
      if (PcOnSq(checking_square) != -pawn)
        check_direction1=directions[checking_square][king_square];
      staClear(checking_square,checksqs);
      checking_square=FirstOne(checksqs);
      if (PcOnSq(checking_square) != -pawn)
        check_direction2=directions[checking_square][king_square];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   the next step is to produce the set of valid           |
|   destination squares.  for king moves, this is simply   |
|   the set of squares that are not attacked by enemy      |
|   pieces (if there are any such squares.)                |
|                                                          |
|   then, if the checking piece is not a knight, we need   |
|   to know the checking direction so that we can either   |
|   move the king "off" of that direction, or else "block" |
|   that direction.                                        |
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=king_attacks[from] & ~WhitePieces;
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      if (!Attacked(tree,to,0) && (directions[from][to] != check_direction1) &&
                                 (directions[from][to] != check_direction2))
        *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (checkers == 1) {
      piecebd=WhiteKnights;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=knight_attacks[from] & target;
          temp=from+(knight<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteBishops;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksBishop(from) & target;
          temp=from+(bishop<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteRooks;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksRook(from) & target;
          temp=from+(rook<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=WhiteQueens;
      while (piecebd) {
        from=LastOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksQueen(from) & target;
          temp=from+(queen<<12);
          while (moves) {
            to=LastOne(moves);
            *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 2 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares.     |
|                                                          |
 ----------------------------------------------------------
*/
      empty=~Occupied;
      targetp=target & empty;
      padvances1=WhitePawns>>8 & targetp;
      padvances1_all=WhitePawns>>8& empty;
      padvances2=(padvances1_all & (mask_8>>16))>>8 & targetp;
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
      while (padvances2) {
        to=LastOne(padvances2);
        if (!PinnedOnKing(tree,wtm,to-16)) *move++=(to-16)|(to<<6)|(pawn<<12);
        staClear(to,padvances2);
      }
      while (padvances1) {
        to=LastOne(padvances1);
        if (!PinnedOnKing(tree,wtm,to-8)) {
          if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
          else {
            *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(rook<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(bishop<<18);
            *move++=(to-8)|(to<<6)|(pawn<<12)|(knight<<18);
          }
        }
        staClear(to,padvances1);
      }

      targetc=BlackPieces | EnPassantTarget(ply);
      targetc=targetc & target;
      if (BlackPawns & target & EnPassantTarget(ply)<<8)
        targetc=targetc | EnPassantTarget(ply);
      pcapturesl=(WhitePawns & mask_left_edge)>>7 & targetc;
      pcapturesr=(WhitePawns & mask_right_edge)>>9 & targetc;
      while (pcapturesl) {
        to=LastOne(pcapturesl);
        if (!PinnedOnKing(tree,wtm,to-7)) {
          if (to < 56) {
            int cap=(PcOnSq(to)) ? -PcOnSq(to) : pawn;
            *move++=(to-7)|(to<<6)|(pawn<<12)|(cap<<15);
          }
          else {
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(queen<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(rook<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(bishop<<18);
            *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(knight<<18);
          }
        }
        staClear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=LastOne(pcapturesr);
        if (!PinnedOnKing(tree,wtm,to-9)) {
          if (to < 56) {
            int cap=(PcOnSq(to)) ? -PcOnSq(to) : pawn;
            *move++=(to-9)|(to<<6)|(pawn<<12)|(cap<<15);
          }
          else {
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(queen<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(rook<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(bishop<<18);
            *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(knight<<18);
          }
        }
        staClear(to,pcapturesr);
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine how many pieces are attacking the     |
|   king and where they are, so we can figure out how to   |
|   legally get out of check.                              |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    king_square=BlackKingSQ;
    checksqs=AttacksTo(tree,king_square) & WhitePieces;
    checkers=PopCnt(checksqs);
    if (checkers == 1) {
      checking_square=FirstOne(AttacksTo(tree,king_square) & WhitePieces);
      if (PcOnSq(checking_square) != pawn)
        check_direction1=directions[checking_square][king_square];
      target=InterposeSquares(check_direction1,king_square,checking_square);
      target=target | (AttacksTo(tree,king_square) & WhitePieces);
      target=target | WhiteKing;
    }
    else {
      target=WhiteKing;
      checking_square=FirstOne(checksqs);
      if (PcOnSq(checking_square) != pawn)
        check_direction1=directions[checking_square][king_square];
      staClear(checking_square,checksqs);
      checking_square=FirstOne(checksqs);
      if (PcOnSq(checking_square) != pawn)
        check_direction2=directions[checking_square][king_square];
    }
/*
 ----------------------------------------------------------
|                                                          |
|   the first step is to produce the set of valid          |
|   destination squares.  for king moves, this is simply   |
|   the set of squares that are not attacked by enemy      |
|   pieces (if there are any such squares.)                |
|                                                          |
|   then, if the checking piece is not a knight, we need   |
|   to know the checking direction so that we can either   |
|   move the king "off" of that direction, or else "block" |
|   that direction.                                        |
|                                                          |
|   first produce king moves by cycling through the        |
|   *_king board to locate a [from] square and then        |
|   cycling through attacks_to[] to locate to squares      |
|   that the king on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    from=king_square;
    moves=king_attacks[from] & ~BlackPieces;
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      if (!Attacked(tree,to,1) && (directions[from][to] != check_direction1) &&
                                 (directions[from][to] != check_direction2))
        *move++=temp|(to<<6)|(PcOnSq(to)<<15);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce knight moves by cycling through the       |
|   *_knight board to locate a [from] square and then      |
|   cycling through knight_attacks[] to locate to squares  |
|   that a knight on [from] attacks.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (checkers == 1) {
      piecebd=BlackKnights;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=knight_attacks[from] & target;
          temp=from+(knight<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PcOnSq(to)<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce bishop moves by cycling through the       |
|   *_bishop board to locate a [from] square and then      |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackBishops;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksBishop(from) & target;
          temp=from+(bishop<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PcOnSq(to)<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce rook moves by cycling through the         |
|   *_rook board to locate a [from] square and then        |
|   generate the AttacksFrom() bitmap which supplies the   |
|   list of valid <to> squares.                            |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackRooks;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksRook(from) & target;
          temp=from+(rook<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PcOnSq(to)<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce queen moves by cycling through the        |
|   *_queen board to locate a [from] square and then       |
|                                                          |
 ----------------------------------------------------------
*/
      piecebd=BlackQueens;
      while (piecebd) {
        from=FirstOne(piecebd);
        if (!PinnedOnKing(tree,wtm,from)) {
          moves=AttacksQueen(from) & target;
          temp=from+(queen<<12);
          while (moves) {
            to=FirstOne(moves);
            *move++=temp|(to<<6)|(PcOnSq(to)<<15);
            staClear(to,moves);
          }
        }
        staClear(from,piecebd);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn moves.  we do    |
|   this in 4 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares;     |
|   (3) remove the a-pawn(s) then shift the pawns          |
|   diagonally left then and with enemy occupied squares;  |
|   (4) remove the h-pawn(s) then shift the pawns          |
|   diagonally right then and with enemy occupied squares; |
|   note that enemy occupied squares includes the special  |
|   case of the enpassant target square also.              |
|                                                          |
 ----------------------------------------------------------
*/
      empty=~Occupied;
      targetp=target & empty;
      padvances1=BlackPawns<<8 & targetp;
      padvances1_all=BlackPawns<<8 & empty;
      padvances2=(padvances1_all & (mask_120<<16))<<8 & targetp;
/*
 ----------------------------------------------------------
|                                                          |
|   now that we got 'em, we simply enumerate the to        |
|   squares as before, but in four steps since we have     |
|   four sets of potential moves.                          |
|                                                          |
 ----------------------------------------------------------
*/
      while (padvances2) {
        to=FirstOne(padvances2);
        if (!PinnedOnKing(tree,wtm,to+16)) *move++=(to+16)|(to<<6)|(pawn<<12);
        staClear(to,padvances2);
      }
      while (padvances1) {
        to=FirstOne(padvances1);
        if (!PinnedOnKing(tree,wtm,to+8)) {
          if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
          else {
            *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(rook<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(bishop<<18);
            *move++=(to+8)|(to<<6)|(pawn<<12)|(knight<<18);
          }
        }
        staClear(to,padvances1);
      }

      targetc=WhitePieces | EnPassantTarget(ply);
      targetc=targetc & target;
      if (WhitePawns & target & EnPassantTarget(ply)>>8)
        targetc=targetc | EnPassantTarget(ply);
      pcapturesl=(BlackPawns & mask_left_edge)<<9 & targetc;
      pcapturesr=(BlackPawns & mask_right_edge)<<7 & targetc;
      while (pcapturesl) {
        to=FirstOne(pcapturesl);
        if (!PinnedOnKing(tree,wtm,to+9)) {
          if (to > 7) {
            int cap=(PcOnSq(to)) ? PcOnSq(to) : pawn;
            *move++=(to+9)|(to<<6)|(pawn<<12)|(cap<<15);
          }
          else {
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(queen<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(rook<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(bishop<<18);
            *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(knight<<18);
          }
        }
        staClear(to,pcapturesl);
      }
      while (pcapturesr) {
        to=FirstOne(pcapturesr);
        if (!PinnedOnKing(tree,wtm,to+7)) {
          if (to > 7) {
            int cap=(PcOnSq(to)) ? PcOnSq(to) : pawn;
            *move++=(to+7)|(to<<6)|(pawn<<12)|(cap<<15);
          }
          else {
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(queen<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(rook<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(bishop<<18);
            *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(knight<<18);
          }
        }
        staClear(to,pcapturesr);
      }
    }
  }
  return(move);
}

// RedCrafty_481_f.cpp, ř. 4376-4521: HashProbe
/* last modified 09/14/99 */
/*
********************************************************************************
*                                                                              *
*   HashProbe() is used to retrieve entries from the transposition table so    *
*   this sub-tree won't have to be searched again if we reach a position that  *
*   has been searched previously.  a transposition table position contains the *
*   following data packed into 128 bits with each item taking the number of    *
*   bits given in the table below:                                             *
*                                                                              *
*     bits     name  SL  description                                           *
*       3       age  61  search id to identify old trans/ref entries.          *
*       2      type  59  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*       1    threat  58  threat extension flag, 1 -> extend this position.     *
*       5    unused  53  unused at present time.                               *
*      21      move  32  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*      15     draft  17  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  note that this is in units of 1/60th of a  *
*                        ply.                                                  *
*      17     value   0  unsigned integer value of this position + 65536.      *
*                        this might be a good score or search bound.           *
*                                                                              *
*      64       key   0  64 bit hash signature, used to verify that this entry *
*                        goes with the current board position.                 *
*                                                                              *
********************************************************************************
*/
int HashProbe(TREE *tree, int ply, int depth, int wtm, int *alpha,
           int *beta, int *threat) {
  BITBOARD word1, word2;
  int type, draft, avoid_null=0, val, pieces, null_depth;
  unsigned int word1l, word1r;
  BITBOARD temp_hashkey;
  HASH_ENTRY *htable;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  tree->hash_move[ply]=0;
  if (port_bez_hashe) return(0);  // port: přepínač pro test, viz crafty_search.h
#if !defined(FAST)
  tree->transposition_probes++;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, check both "parts" of the hash table to see if    |
|   this position is in either one.                        |
|                                                          |
 ----------------------------------------------------------
*/
  pieces=(wtm) ? TotalWhitePieces : TotalBlackPieces;
  null_depth=(depth>6*INCPLY && pieces>8) ? null_max : null_min;
  temp_hashkey=(wtm) ? HashKey : ~HashKey;
  htable=trans_ref_a+((int) temp_hashkey&hash_maska);
  word1=htable->word1;
  word2=word1^htable->word2;
  if (word2 == temp_hashkey) do {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    word1l=word1>>32;
    word1r=word1;
    val=(word1r & 0377777)-65536;
    draft=word1r>>17;
    tree->hash_move[ply]=word1l & 07777777;
    *threat=(word1l>>26) & 01;
    type=(word1l>>27) & 03;
    if ((type & UPPER) &&
        depth-null_depth <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) break;

    switch (type) {
      case EXACT:
        if (abs(val) > MATE-300) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT);
      case UPPER:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER);
        }
        break;
      case LOWER:
        if (val >= *beta) {
          *beta=val;
          return(LOWER);
        }
        break;
    }
  } while(0);

  htable=trans_ref_b+((int)temp_hashkey&hash_maskb);
  word1=htable->word1;
  word2=word1^htable->word2;
  if (word2 == temp_hashkey) {
#if !defined(FAST)
    tree->transposition_hits++;
#endif
    word1l=word1>>32;
    word1r=word1;
    val=(word1r & 0377777)-65536;
    draft=word1r>>17;
    if (tree->hash_move[ply]==0) tree->hash_move[ply]=word1l & 07777777;
    *threat=(word1l>>26) & 01;
    type=(word1l>>27) & 03;
    if ((type & UPPER) &&
        depth-null_depth <= draft &&
        val < *beta) avoid_null=AVOID_NULL_MOVE;
    if (depth > draft) return(avoid_null);

    switch (type) {
      case EXACT:
        if (abs(val) > MATE-300) {
          if (val > 0) val-=(ply-1);
          else val+=(ply-1);
        }
        *alpha=val;
        return(EXACT);
      case UPPER:
        if (val <= *alpha) {
          *alpha=val;
          return(UPPER);
        }
        return(avoid_null);
      case LOWER:
        if (val >= *beta) {
          *beta=val;
          return(LOWER);
        }
        return(avoid_null);
    }
  }
  return(avoid_null);
}

// RedCrafty_481_f.cpp, ř. 4524-4911: Iterate
/* last modified 11/20/01 */
/*
********************************************************************************
*                                                                              *
*   Iterate() is the routine used to drive the iterated search.  it repeatedly *
*   calls search, incrementing the search depth after each call, until either  *
*   time is exhausted or the maximum set search depth is reached.              *
*                                                                              *
********************************************************************************
*/
int Iterate(int wtm, int search_type, int root_list_done) {
  // port: AnsiString s (ř. 4535) patřil jen k výpisům do Memo1
  ROOT_MOVE temp;
  int wpawn, bpawn, TB_use_ok;
  int i, value=0;
  int twtm, used;
  int correct, correct_count, material=0, sorted;
  TREE * const tree=local[0];
/*
 ----------------------------------------------------------
|                                                          |
|  initialize.                                             |
|                                                          |
|  produce the root move list, which is ordered and kept   |
|  for the duration of this search (the order may change   |
|  as new best moves are backed up to the root of course.) |
|                                                          |
 ----------------------------------------------------------
*/
  largest_positional_score=Max(largest_positional_score*3/4,300);
  lazy_eval_cutoff=Max(lazy_eval_cutoff*3/4,200);
  if (wtm) {
    draw_score[0]=-abs_draw_score;
    draw_score[1]=abs_draw_score;
  }
  else {
    draw_score[0]=abs_draw_score;
    draw_score[1]=-abs_draw_score;
  }
  abort_search=0;
  book_move=0;
  root_wtm=wtm;
  PreEvaluate(tree,wtm);
  whisper_depth=0;
  tree->nodes_searched=0;
  tree->fail_high=0;
  tree->fail_high_first=0;
  parallel_splits=0;
  parallel_stops=0;
  max_split_blocks=0;
  TB_use_ok=1;
  if (TotalWhitePawns && TotalBlackPawns) {
    wpawn=FirstOne(WhitePawns);
    bpawn=FirstOne(BlackPawns);
    if (FileDistance(wpawn,bpawn) == 1) {
      if(((Rank(wpawn)==RANK2) && (Rank(bpawn)>RANK3)) ||
         ((Rank(bpawn)==RANK7) && (Rank(wpawn)<RANK6)) ||
         EnPassant(1)) TB_use_ok=0;
    }
  }
  if (booking || !Book(tree,wtm,root_list_done)) do {
    if (abort_search) break;
    if (!root_list_done) RootMoveList(wtm);
    if (EGTB_draw && !puzzling && swindle_mode) EGTB_use=0;
    else EGTB_use=EGTBlimit;
    if (EGTBlimit && !EGTB_use)
      ;  // port: bylo Memo1 "Drawn at root, trying for swindle." (ř. 4590)
    correct_count=0;
    burp=15*100;
    transposition_id=(transposition_id+1)&7;
    if (!transposition_id) transposition_id++;
    tree->evaluations=0;
#if !defined(FAST)
    tree->transposition_hits=0;
    tree->transposition_probes=0;
    tree->pawn_probes=0;
    tree->pawn_hits=0;
#endif
    tree->egtb_probes=0;
    tree->egtb_probes_successful=0;
    tree->check_extensions_done=0;
    tree->mate_extensions_done=0;
    tree->recapture_extensions_done=0;
    tree->passed_pawn_extensions_done=0;
    tree->one_reply_extensions_done=0;
    root_wtm=wtm;
/*
 ----------------------------------------------------------
|                                                          |
|  if there are no legal moves, it is either mate or draw  |
|  depending on whether the side to move is in check or    |
|  not (mate or stalemate.)                                |
|                                                          |
 ----------------------------------------------------------
*/
    if (n_root_moves == 0) {
      tree->pv[0].pathl=0;
      tree->pv[0].pathd=10;
      if (Check(wtm)) {
        root_value=-(MATE-1);
      }
      else {
        root_value=DrawScore(wtm);
      }
      tree->nodes_searched=1;
      if (!puzzling) last_root_value=root_value;
      return(root_value);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now set the search time and iteratively call Search()  |
|   to analyze the position deeper and deeper.  note that  |
|   Search() is called with an alpha/beta window roughly   |
|   1/3 of a pawn on either side of the score last         |
|   returned by search.  also, after the first root move   |
|   is searched, this window is collapsed to n and n+1     |
|   (where n is the value for the first root move.)  often |
|   a better move is found, which causes search to return  |
|   <beta> as the score.  we then relax beta depending on  |
|   its value:  if beta = alpha+1, set beta to alpha+1/3   |
|   of a pawn;  if beta = alpha+1/3 pawn, then set beta to |
|   + infinity.                                            |
|                                                          |
 ----------------------------------------------------------
*/
    iteration_depth=1;
    if (last_pv.pathd > 1)
      iteration_depth=last_pv.pathd+1;
    abort_search=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now install the learned position information in the    |
|   hash table.                                            |
|                                                          |
 ----------------------------------------------------------
*/

  // Stanley smazal

/*
 ----------------------------------------------------------
|                                                          |
|   set the initial search bounds based on the last search |
|   or default values.                                     |
|                                                          |
 ----------------------------------------------------------
*/
    tree->pv[0]=last_pv;
    if (iteration_depth > 1) {
      root_alpha=last_value-40;
      root_value=last_value-40;
      root_beta=last_value+40;
    }
    else {
      root_alpha=-MATE-1;
      root_value=-MATE-1;
      root_beta=MATE+1;
    }

  // Stanley smazal SMP

    root_print_ok=0;
    for (;iteration_depth<=60;iteration_depth++) {
/*
 ----------------------------------------------------------
|                                                          |
|   now install the old PV into the hash table so that     |
|   these moves will be followed first.                    |
|                                                          |
 ----------------------------------------------------------
*/
      twtm=wtm;
      for (i=1;i<=(int) tree->pv[0].pathl;i++) {
        tree->pv[i]=tree->pv[i-1];
        if (!LegalMove(tree,i,twtm,tree->pv[i].path[i])) {
          Print(4095,"ERROR, not installing bogus move at ply=%d\n",i);
          Print(4095,"not installing from=%d  to=%d  piece=%d\n",
                From(tree->pv[i].path[i]), To(tree->pv[i].path[i]),
                Piece(tree->pv[i].path[i]));
          break;
        }
        HashStorePV(tree, i, twtm);
        MakeMove(tree, i,tree->pv[0].path[i],twtm);
        twtm=ChangeSide(twtm);
      }
      for (i--;i>0;i--) {
        twtm=ChangeSide(twtm);
        UnmakeMove(tree, i,tree->pv[0].path[i],twtm);
      }
        // port: výpisy do Memo1 a Label1 "Hloubka" (ř. 4715-4720) pryč

/*
        if (((value<-MATE+300)||(value>MATE-300)) &&
            (iteration_depth>=4)) abort_search=1;  // moje přerušení, když najde mat
*/
        if ((((value<-MATE+300)||(value>MATE-300)) &&
             (iteration_depth>=5)) ||
            (((value<-MATE+5)||(value>MATE-5)) &&
              (iteration_depth>=4)))
            // oprava č. 8 (PORT-CRAFTY.md, oddíl 3): místo abort_search=1
            // před SearchRoot výskok z iterační smyčky. SearchRoot se tak
            // vůbec nerozběhne a tree->pv[0] zůstane z poslední dokončené
            // iterace.
            break;  // moje přerušení, když najde mat

      while (1) {

  // Stanley smazal SMP

        value=SearchRoot(tree,root_alpha, root_beta, wtm, iteration_depth*INCPLY+30);

        root_print_ok=tree->nodes_searched > noise_level;
        if (abort_search) break;
        if (value >= root_beta) {
          // port: výpis do Memo1 (ř. 4741-4746) pryč
          root_moves[0].status|=2;
          root_moves[0].status&=255-128;
          if (root_moves[0].status&1) root_alpha=-MATE-1;
          else root_alpha=root_beta-1;
          root_value=root_alpha;
          root_beta=MATE+1;
          root_moves[0].nodes=0;
        }
        else if (value <= root_alpha) {
          // port: výpis do Memo1 (ř. 4756-4761) pryč
          if (!(root_moves[0].status&2)) {
            root_moves[0].status&=255-128;
            root_moves[0].nodes=0;
            root_moves[0].status|=1;
            if (root_moves[0].status&2) root_beta=MATE+1;
            root_alpha=-MATE-1;
            root_value=root_alpha;
            easy_move=0;
          }
          else break;
        }
        else break;
      }
      if (root_value>root_alpha && root_value<root_beta)
        last_root_value=root_value;

// Stan smazal

/*
 ----------------------------------------------------------
|                                                          |
|   if the search terminated normally, then dump the PV    |
|   and search statistics (we don't do this if the search  |
|   aborts because the opponent doesn't make the predicted |
|   move...)                                               |
|                                                          |
 ----------------------------------------------------------
*/
      twtm=wtm;
      do {
        sorted=1;
        for (i=1;i<n_root_moves-1;i++) {
          if (root_moves[i].nodes < root_moves[i+1].nodes) {
            temp=root_moves[i];
            root_moves[i]=root_moves[i+1];
            root_moves[i+1]=temp;
            sorted=0;
          }
        }
      } while(!sorted);
/*
 ----------------------------------------------------------
|                                                          |
|   notice if there are multiple moves that are producing  |
|   large trees.  if so, don't search those in parallel by |
|   setting the flag to avoid this.                        |
|                                                          |
 ----------------------------------------------------------
*/
      for (i=0;i<n_root_moves;i++) root_moves[i].status=0;
      if (root_moves[0].nodes > 1000)
        for (i=0;i<Min(n_root_moves,4);i++) {
          if (root_moves[i].nodes > root_moves[0].nodes/2)
            root_moves[i].status|=64;
        }
/*
 ----------------------------------------------------------
|                                                          |
|   if requested, print the ply=1 move list along with the |
|   node counts for the tree each move produced.           |
|                                                          |
 ----------------------------------------------------------
*/
      // Stan smazal

      for (i=0;i<n_root_moves;i++) root_moves[i].nodes=0;
      if (!abort_search && (root_print_ok ||
          correct_count>=early_exit || value>MATE-300 || tree->pv[0].pathh==2)) {
        if (value != -(MATE-1)) {
          DisplayPV(tree,5,wtm,0,value,&tree->pv[0]);   // Stan: param time=0
        }
      }
      root_alpha=value-40;
      root_value=root_alpha;
      root_beta=value+40;
      if (iteration_depth>3 && value>MATE-300 &&
          value>=(MATE-iteration_depth-1) && value>last_mate_score) break;
      if ((iteration_depth >= search_depth) && search_depth) break;
      if (abort_search) break;
      if (correct_count >= early_exit) break;

/* Stanley uzav:      if (iteration_depth>3 && TotalPieces<=EGTBlimit && TB_use_ok &&
          EGTB_use && !EGTB_search && EGTBProbe(tree, 1, wtm, &i)) break;
*/

      if (search_nodes && tree->nodes_searched>search_nodes) break;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   search done, now display statistics, depending on the  |
|   verbosity level set.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    used=0;
#if !defined(FAST)
    for (i=0;i<hash_table_size;i++) {
      if ((trans_ref_a+i)->word1>>61 == transposition_id) used++;
    }
    for (i=0;i<hash_table_size*2;i++) {
      if ((trans_ref_b+i)->word1>>61 == transposition_id) used++;
    }
#endif
    tree->evaluations=(tree->evaluations) ? tree->evaluations : 1;
//    if ((!abort_search || time_abort) && !puzzling) {
      tree->fail_high++;
      tree->fail_high_first++;
      material=Material/PAWN_VALUE;

    // port: statistiky do Memo1 (ř. 4872-4896) pryč

  // Stanley smazal SMP

//    }
  } while(0);
  else {
    last_root_value=0;
    root_value=0;
    book_move=1;
    tree->pv[0]=tree->pv[1];
  }
  search_move=0;
  // port (krok C3/A): "if (quit) exit(0);" (ř. 4909) pryč - quit nikdo
  // nenastavuje, ale exit v knihovně by shodil celou aplikaci
  return(last_root_value);
}

// RedCrafty_481_f.cpp, ř. 4914-5071: OutputMove
/* last modified 01/22/01 */
/*
********************************************************************************
*                                                                              *
*   OutputMove() is responsible for converting the internal move format to a   *
*   string that can be displayed.  first, it simply converts the from/to       *
*   squares to fully-qualified algebraic (which includes O-O and O-O-O for     *
*   castling moves).  next, we try several "shortcut" forms and call           *
*   input_move(silent=1) to let it silently check the move for uniqueness.     *
*   as soon as we get a non-ambiguous move, we return that text string.        *
*                                                                              *
********************************************************************************
*/
char* OutputMove(TREE *tree, int move, int ply, int wtm) {
  static char text_move[10], new_text[10];
  int *mvp;
  char *text;
  static const char piece_names[8] = { ' ','P','N','K',' ','B','R','Q'};

  text=text_move;
/*
   special case for null-move, used only in tracing the search
   by dumping the tree of everything searched.
*/
  if (move == 0) {
    strcpy(text,"null");
    return(text);
  }
  do {
/*
   check for castling first
*/
    if ((Piece(move) == king) &&
        (abs(From(move)-To(move)) == 2)) {
      if (wtm) {
        if (To(move) == 2) strcpy(text_move,"O-O-O");
        else strcpy(text_move,"O-O");
      }
      else {
        if (To(move) == 58) strcpy(text_move,"O-O-O");
        else strcpy(text_move,"O-O");
      }
      break;
    }
/*
   not a castling move.  convert to fully-qualified algebraic form
   first.
*/
    text=new_text;
    if ((int) Piece(move) > pawn) *text++=piece_names[Piece(move)];
    *text++=(From(move) & 7)+'a';
    *text++=(From(move) / 8)+'1';
    if (Captured(move)) *text++='x';
    *text++=(To(move) & 7)+'a';
    *text++=(To(move) / 8)+'1';
    if (Promote(move)) {
      *text++='=';
      *text++=piece_names[Promote(move)];
    }
    *text='\0';
    strcpy(text_move,new_text);
    if (output_format > 0) break;
/*
   now, try some short forms.  if the moving piece is a pawn, and
   it is not capturing anything, simply try the destination square
   first: (e2e4->e4)
*/
    if (Piece(move) == pawn) {
      if (!Captured(move)) {
        strcpy(text_move,new_text+2);
        if (OutputGood(tree,text_move,ply,wtm)) break;
      }
/*
   if the move is a pawn capturing a piece, try the short-form for pawn
   capture moves: (e4xf5->exf5)
*/
      text_move[0]=new_text[0];
      strcpy(text_move+1,new_text+2);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   punt, return the fully-qualified move.
*/
      strcpy(text_move,new_text);
      break;
    }
/*
   if the move is a piece move, but not a capture, then try the short-
   form for non-capturing moves:  (Ng1f3->Nf3)
*/
    if (!Captured(move)) {
      text_move[0]=new_text[0];
      strcpy(text_move+1,new_text+3);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   now try to qualify with the origin file only: (Ng1f3->Ngf3)
*/
      text_move[0]=new_text[0];
      text_move[1]=new_text[1];
      strcpy(text_move+2,new_text+3);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   now try to qualify with the origin rank only: (Ng1f3->N1f3)
*/
      text_move[0]=new_text[0];
      strcpy(text_move+1,new_text+2);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   punt, return the fully-qualified move.
*/
      strcpy(text_move,new_text);
      break;
    }
    else {
/*
   if the move is a piece move, and a capture, then try the short-
   form for capturing moves:  (Ng1xf3->Nxf3)
*/
      text_move[0]=new_text[0];
      strcpy(text_move+1,new_text+3);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   next, try adding in the origin file: (Ng1xf3->Ngxf3)
*/
      text_move[0]=new_text[0];
      text_move[1]=new_text[1];
      strcpy(text_move+2,new_text+3);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   next, try adding in the origin rank: (Ng1xf3->N1xf3)
*/
      text_move[0]=new_text[0];
      strcpy(text_move+1,new_text+2);
      if (OutputGood(tree,text_move,ply,wtm)) break;
/*
   punt, return the fully-qualified move.
*/
      strcpy(text_move,new_text);
      break;
    }
  } while (0);
/*
   determine if it's a checking move, or if it's mate.  if so,
   append "+" or "#" as appropriate.
*/
  text=text_move+strlen(text_move);
  tree->position[MAXPLY]=tree->position[ply];
  MakeMove(tree, MAXPLY, move, wtm);
  if (Check(ChangeSide(wtm))) {
    mvp=GenerateCheckEvasions(tree,MAXPLY+1, ChangeSide(wtm), tree->move_list+4800);
    if (mvp == (tree->move_list+4800))
      *text++='#';
    else
      *text++='+';
  }
  UnmakeMove(tree, MAXPLY, move, wtm);
  *text=0;
  return (text_move);
}

// RedCrafty_481_f.cpp, ř. 5116-5129: Print
void Print(int vb, const char *fmt, ...) {
  hlaseni_print++;  // port: bylo Memo1, ř. 5117
/*
  va_list ap;
  va_start(ap,fmt);
  if (vb&display_options) vprintf(fmt, ap);
  fflush(stdout);
  if (time_limit>99 || tc_time_remaining>6000 || vb==4095) {
    if (log_file) vfprintf(log_file, fmt, ap);
    if (log_file) fflush(log_file);
  }
  va_end(ap);
*/
}

// RedCrafty_481_f.cpp, ř. 5131-5347: InputMove
/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   InputMove() is responsible for converting a move from a text string to     *
*   the internal move format.  it allows the so-called "reduced algebraic      *
*   move format" which makes the origin square optional unless required for    *
*   clarity.  it also accepts as little as required to remove ambiguity from   *
*   the move, by using GenerateMoves() to produce a set of legal moves         *
*   that the text can be applied against to eliminate those moves not          *
*   intended.  hopefully, only one move will remain after the elimination      *
*   and legality checks.                                                       *
*                                                                              *
********************************************************************************
*/
int InputMove(TREE *tree, char *text, int ply, int wtm, int silent,
              int ponder_list) {
  int moves[220], *mv, *mvp, *goodmove=0;
  int piece=-1, capture, promote, give_check;
  int ffile, frank, tfile, trank;
  int current, i, nleft;
  char *goodchar;
  char movetext[128];
  static const char pieces[17]={' ',' ','P','p','N','n','K','k',' ',' ',
                                'B','B','R','r','Q','q','\0'};
  static const char pro_pieces[17]={' ',' ','P','p','N','n','K','k',' ',' ',
                                    'B','b','R','r','Q','q','\0'};

/*
   check for fully-qualified input (f1e1) and handle if needed.
*/
  if (strlen(text) == 0) return(0);
  if ((text[0] >= 'a') && (text[0] <= 'h') &&
      (text[1] >= '1') && (text[1] <= '8') &&
      (text[2] >= 'a') && (text[2] <= 'h') &&
      (text[3] >= '1') && (text[3] <= '8'))
    return(InputMoveICS(tree,text,ply,wtm,silent,ponder_list));
/*
   initialize move structure in case an error is found
*/
  tree->position[MAXPLY]=tree->position[ply];
  strcpy(movetext,text);
  moves[0]=0;
  piece=0;
  capture=0;
  promote=0;
  give_check=0;
  frank=-1;
  ffile=-1;
  trank=-1;
  tfile=-1;
  goodchar=strchr(movetext,'#');
  if (goodchar) *goodchar=0;
/*
   first, figure out what each character means.  the first thing to
   do is eliminate castling moves
*/
  if (!strcmp(movetext,"o-o") || !strcmp(movetext,"o-o+") ||
      !strcmp(movetext,"O-O") || !strcmp(movetext,"O-O+") ||
      !strcmp(movetext,"0-0") || !strcmp(movetext,"0-0+")) {
    piece=king;
    if(wtm) {
      ffile=4;
      frank=0;
      tfile=6;
      trank=0;
    }
    else {
      ffile=4;
      frank=7;
      tfile=6;
      trank=7;
    }
  }
  else
    if (!strcmp(movetext,"o-o-o") || !strcmp(movetext,"o-o-o+") ||
        !strcmp(movetext,"O-O-O") || !strcmp(movetext,"O-O-O+") ||
        !strcmp(movetext,"0-0-0") || !strcmp(movetext,"0-0-0+")) {
      piece=king;
      if(wtm) {
        ffile=4;
        frank=0;
        tfile=2;
        trank=0;
      }
      else {
        ffile=4;
        frank=7;
        tfile=2;
        trank=7;
      }
    }
  else {
/*
   ok, it's not a castling.  check for the first two characters of "bb" which
   indicates that the first "b" really means "B" since pawn advances don't
   require a source file.
*/
    if ((movetext[0] == 'b') && (movetext[1] == 'b')) movetext[0]='B';
/*
   now, start by picking off the check indicator (+) if one is present.
*/
    if (strchr(movetext,'+')) {
      *strchr(movetext,'+')=0;
      give_check=1;
    }
/*
   now, continue by picking off the promotion piece if one is present.  this
   is indicated by something like =q on the end of the move string.
*/
    if (strchr(movetext,'=')) {
      goodchar=strchr(movetext,'=');
      goodchar++;
      promote=(strchr(pro_pieces,*goodchar)-pro_pieces) >> 1;
      *strchr(movetext,'=')=0;
    }
/*
   the next thing to do is extract the last rank/file designators since
   the destination is required.  note that we can have either or both.
*/
    current=strlen(movetext)-1;
    trank=movetext[current]-'1';
    if ((trank >= 0) && (trank <= 7))
      movetext[current]=0;
    else
      trank=-1;
    current=strlen(movetext)-1;
    tfile=movetext[current]-'a';
    if ((tfile >= 0) && (tfile <= 7))
      movetext[current]=0;
    else
      tfile=-1;
    if (strlen(movetext)) {
/*
   now check the first character to see if it's a piece indicator
   (PpNnBbRrQqKk).  if so, strip it off.
*/
      if (strchr("  PpNnBBRrQqKk",*movetext)) {
        piece=(strchr(pieces,movetext[0])-pieces) >> 1;
        for(i=0;i<(int) strlen(movetext);i++)
          movetext[i]=movetext[i+1];
      }
/*
   now that we have the destination and the moving piece (if any)
   the next step is to see if the last character is now an "x"
   indicating a capture   if so, set the capture flag, remove the
   trailing "x" and continue.
*/
      if ((strlen(movetext)) && (movetext[strlen(movetext)-1] == 'x')) {
        capture=1;
        movetext[strlen(movetext)-1]=0;
      }
      else
        capture=0;
/*
   now, all that can be left is a rank designator, a file designator
   or both.  if the last character a number, then the first (if present)
   has to be a letter.
*/
      if (strlen(movetext)) {
        ffile=movetext[0]-'a';
        if ((ffile < 0) || (ffile > 7)) {
          ffile=-1;
          frank=movetext[0]-'1';
          if ((frank < 0) || (frank > 7)) piece=-1;
        }
        else {
          if (strlen(movetext) == 2) {
            frank=movetext[1]-'1';
            if ((frank < 0) || (frank > 7)) piece=-1;
          }
        }
      }
    }
  }
  if (!piece) piece=1;
  if (!ponder_list) {
    mvp=GenerateCaptures(tree,MAXPLY, wtm, moves);
    mvp=GenerateNonCaptures(tree,MAXPLY, wtm, mvp);
  }
  else {
    for (i=0;i<num_ponder_moves;i++)
    moves[i]=ponder_moves[i];
    mvp=moves+num_ponder_moves;
  }
  for (mv=&moves[0];mv<mvp;mv++) {
    if (piece && (Piece(*mv) != piece)) *mv=0;
    if ((ffile >= 0) && (File(From(*mv)) != ffile)) *mv=0;
    if (capture && (!Captured(*mv))) *mv=0;
    if (promote && (Promote(*mv) != promote)) *mv=0;
    if ((frank >= 0)  && (Rank(From(*mv)) != frank)) *mv=0;
    if ((tfile >= 0)  && (File(To(*mv)) != tfile)) *mv=0;
    if ((trank >= 0)  && (Rank(To(*mv)) != trank)) *mv=0;
    if (!ponder_list && *mv) {
      MakeMove(tree,MAXPLY, *mv, wtm);
      if (Check(wtm) || (give_check && !Check(ChangeSide(wtm)))) {
        UnmakeMove(tree,MAXPLY, *mv, wtm);
        *mv=0;
      }
      else UnmakeMove(tree,MAXPLY, *mv, wtm);
    }
  }
  nleft=0;
  for (mv=&moves[0];mv<mvp;mv++) {
    if (*mv) {
      nleft++;
      goodmove=mv;
    }
  }
  if (nleft == 1) return(*goodmove);
  if (!silent) {
    hlaseni_illegal_move++;  // port: bylo Memo1, ř. 5342-5344
  }
  return(0);
}

// RedCrafty_481_f.cpp, ř. 5543-5565: AttacksTo
/* last modified 06/26/99 */
/*
********************************************************************************
*                                                                              *
*   AttacksTo() is used to produce a BITBOARD which is a map of all squares    *
*   that directly attack this <square>.  the non-sliding pieces are trivial    *
*   to detect, but for sliding pieces, we use a rotated bitboard trick.  the   *
*   idea is to compute the squares a queen would attack, if it was standing on *
*   <square> and then look at the last square attacked in each direction to    *
*   determine if it is a sliding piece that moves in the right direction.  to  *
*   finish up, we simply need to Or() all these attackers together.            *
*                                                                              *
********************************************************************************
*/
BITBOARD AttacksTo(TREE *tree, int square) {

  return((w_pawn_attacks[square] & BlackPawns) |
         (b_pawn_attacks[square] & WhitePawns) |
         (knight_attacks[square] & (BlackKnights | WhiteKnights)) |
         (AttacksBishop(square) & BishopsQueens) |
         (AttacksRook(square) & RooksQueens) |
         (king_attacks[square] & (BlackKing | WhiteKing)));
}

// RedCrafty_481_f.cpp, ř. 5567-5619: InterposeSquares
/*
********************************************************************************
*                                                                              *
*   InterposeSquares() is used to compute the set of squares that block an     *
*   attack on the king by a sliding piece, by interposing any piece between    *
*   the attacking piece and the king on the same ray.                          *
*                                                                              *
********************************************************************************
*/
BITBOARD InterposeSquares(int check_direction, int king_square,
                                  int checking_square) {
  BITBOARD target;
/*
 ----------------------------------------------------------
|                                                          |
|   if this is a check from a single sliding piece, then   |
|   we can interpose along the checking rank/file/diagonal |
|   and block the check.  otherwise, interposing is not a  |
|   possibility.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  switch (check_direction) {
    case +1:
      target=plus1dir[king_square-1] ^ plus1dir[checking_square];
      break;
    case +7:
      target=plus7dir[king_square-7] ^ plus7dir[checking_square];
      break;
    case +8:
      target=plus8dir[king_square-8] ^ plus8dir[checking_square];
      break;
    case +9:
      target=plus9dir[king_square-9] ^ plus9dir[checking_square];
      break;
    case -1:
      target=minus1dir[king_square+1] ^ minus1dir[checking_square];
      break;
    case -7:
      target=minus7dir[king_square+7] ^ minus7dir[checking_square];
      break;
    case -8:
      target=minus8dir[king_square+8] ^ minus8dir[checking_square];
      break;
    case -9:
      target=minus9dir[king_square+9] ^ minus9dir[checking_square];
      break;
    default:
      target=0;
      break;
  }
  return(target);
}

// RedCrafty_481_f.cpp, ř. 5621-5714: PinnedOnKing
/*
********************************************************************************
*                                                                              *
*   PinnedOnKing() is used to determine if the piece on <square> is pinned     *
*   against the king, so that it's illegal to move it.  this is used to screen *
*   potential moves by GenerateCheckEvasions() so that illegal moves are not   *
*   produced.                                                                  *
*                                                                              *
********************************************************************************
*/
int PinnedOnKing(TREE *tree, int wtm, int square) {
  int ray;
  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine if the piece being moved is on the    |
|   same diagonal, rank or file as the king.               |
|                                                          |
 ----------------------------------------------------------
*/
    ray=directions[square][WhiteKingSQ];
    if (!ray) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if they are on the same ray, then determine if the     |
|   king blocks a bishop attack in one direction from this |
|   square and a bishop or queen blocks a bishop attack    |
|   on the same diagonal in the opposite direction.        |
|                                                          |
 ----------------------------------------------------------
*/
    switch (abs(ray)) {
    case 1:
      if (AttacksRank(square) & WhiteKing)
        return((AttacksRank(square) & RooksQueens & BlackPieces) != 0);
      else return(0);
    case 7:
      if (AttacksDiaga1(square) & WhiteKing)
        return((AttacksDiaga1(square) & BishopsQueens & BlackPieces) != 0);
      else return(0);
    case 8:
      if (AttacksFile(square) & WhiteKing)
        return((AttacksFile(square) & RooksQueens & BlackPieces) != 0);
      else return(0);
    case 9:
      if (AttacksDiagh1(square) & WhiteKing)
        return((AttacksDiagh1(square) & BishopsQueens & BlackPieces) != 0);
      else return(0);
    }
  }
  else {
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine if the piece being moved is on the    |
|   same diagonal, rank or file as the king.               |
|                                                          |
 ----------------------------------------------------------
*/
    ray=directions[BlackKingSQ][square];
    if (!ray) return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   if they are on the same ray, then determine if the     |
|   king blocks a bishop attack in one direction from this |
|   square and a bishop or queen blocks a bishop attack    |
|   on the same diagonal in the opposite direction.        |
|                                                          |
 ----------------------------------------------------------
*/
    switch (abs(ray)) {
    case 1:
      if (AttacksRank(square) & BlackKing)
        return((AttacksRank(square) & RooksQueens & WhitePieces) != 0);
      else return(0);
    case 7:
      if (AttacksDiaga1(square) & BlackKing)
        return((AttacksDiaga1(square) & BishopsQueens & WhitePieces) != 0);
      else return(0);
    case 8:
      if (AttacksFile(square) & BlackKing)
        return((AttacksFile(square) & RooksQueens & WhitePieces) != 0);
      else return(0);
    case 9:
      if (AttacksDiagh1(square) & BlackKing)
        return((AttacksDiagh1(square) & BishopsQueens & WhitePieces) != 0);
      else return(0);
    }
  }
  return(0);
}

// RedCrafty_481_f.cpp, ř. 5738-5931: PreEvaluate
/* last modified 11/15/00 */
/*
********************************************************************************
*                                                                              *
*   PreEvaluate() is used to set the piece/square tables.  these tables        *
*   contain evaluation parameters that are not dynamic in nature and don't     *
*   change during the course of a single iteration.                            *
*                                                                              *
*   there is one piece/square table for each type of piece on the board, one   *
*   for each side so that opposite sides evaluate things from their per-       *
*   spective.                                                                  *
*                                                                              *
********************************************************************************
*/
void PreEvaluate(TREE *tree, int crafty_is_white) {
  int i, j;
  static int hashing_opening = 0;
  static int hashing_middle_game = 0;
  static int hashing_end_game = 0;
  static int last_crafty_is_white = 0;
  static int last_trojan_check = 0;
  static int last_clear = 0;
  int pawn_advance[8], pawn_base[8];
/*
 ----------------------------------------------------------
|                                                          |
|   the first step is to determine if we are in the        |
|   opening (not castled), middle-game, or end-game (with  |
|   material <= 15 for both sides).                        |
|                                                          |
 ----------------------------------------------------------
*/
  Phase();
/*
 ----------------------------------------------------------
|                                                          |
|   pawn advances.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  pawn_advance[0]=PAWN_ADVANCE_A;
  pawn_advance[1]=PAWN_ADVANCE_B;
  pawn_advance[2]=PAWN_ADVANCE_C;
  pawn_advance[3]=PAWN_ADVANCE_D;
  pawn_advance[4]=PAWN_ADVANCE_E;
  pawn_advance[5]=PAWN_ADVANCE_F;
  pawn_advance[6]=PAWN_ADVANCE_G;
  pawn_advance[7]=PAWN_ADVANCE_H;
  pawn_base[0]=PAWN_BASE_A;
  pawn_base[1]=PAWN_BASE_B;
  pawn_base[2]=PAWN_BASE_C;
  pawn_base[3]=PAWN_BASE_D;
  pawn_base[4]=PAWN_BASE_E;
  pawn_base[5]=PAWN_BASE_F;
  pawn_base[6]=PAWN_BASE_G;
  pawn_base[7]=PAWN_BASE_H;
/*
 ----------------------------------------------------------
|                                                          |
|   pawns.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=1;i<7;i++)
    for (j=0;j<8;j++)
      pval_w[i*8+j]=pawn_base[j]+pawn_advance[j]*(i-1);
  for (j=A6;j<A8;j++)
    pval_w[j]+=PAWN_JAM;
  pval_w[D2]-=CENTER_PAWN_UNMOVED;
  pval_w[E2]-=CENTER_PAWN_UNMOVED;
  for (i=6;i>0;i--)
    for (j=0;j<8;j++)
      pval_b[i*8+j]=pawn_base[j]+pawn_advance[j]*(6-i);
  for (j=A2;j<A4;j++)
    pval_b[j]+=PAWN_JAM;
  pval_b[D7]-=CENTER_PAWN_UNMOVED;
  pval_b[E7]-=CENTER_PAWN_UNMOVED;
/*
 ----------------------------------------------------------
|                                                          |
|   now we set the king safety values based on the values  |
|   set by the user, or the default values.                |
|                                                          |
 ----------------------------------------------------------
*/
  if (crafty_is_white) {
    for (i=0;i<64;i++) {
      temper_w[i]=temper[i];
      temper_b[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
    }
  }
  else {
    for (i=0;i<64;i++) {
      temper_w[i]=temper[i]+temper[i]*king_safety_asymmetry/100;
      temper_b[i]=temper[i];
    }
  }
  for (i=0;i<64;i++) {
    temper_w[i]=temper_w[i]*king_safety_scale/100;
    temper_b[i]=temper_b[i]*king_safety_scale/100;
  }
  for (i=0;i<128;i++)
    tropism[i]=king_tropism[i]*king_safety_tropism/100;
  for (i=0;i<9;i++)
    pawn_rams[i]=blocked_scale*pawn_rams_v[i]/100;
/*
 ----------------------------------------------------------
|                                                          |
|   now check to see if the "trojan check" code should be  |
|   turned on.  basically if the king is in the corner,    |
|   the opponent has placed a piece on g4/g5, and both     |
|   sides have pawns attacking that piece, and queens are  |
|   still on the board, then it is a threat that must be   |
|   handled.                                               |
|                                                          |
|   this is handled as 4 separate cases for each corner of |
|   the board, for simplicity.                             |
|                                                          |
 ----------------------------------------------------------
*/
  trojan_check=0;
  if (BlackQueens && BlackRooks) {
    if (WhiteKingSQ==G1 || WhiteKingSQ==H1) {
      if (SetMask(G4)&BlackKnights || SetMask(G4)&BlackBishops) {
        if (SetMask(H3)&WhitePawns && SetMask(H5)&BlackPawns) trojan_check=1;
      }
    }
    if (WhiteKingSQ==B1 || WhiteKingSQ==A1) {
      if (SetMask(B4)&BlackKnights || SetMask(B4)&BlackBishops) {
        if (SetMask(A3)&WhitePawns && SetMask(A5)&BlackPawns) trojan_check=1;
      }
    }
  }
  if (WhiteQueens && WhiteRooks) {
    if (BlackKingSQ==G8 || BlackKingSQ==H8) {
      if (SetMask(G5)&WhiteKnights || SetMask(G5)&WhiteBishops) {
        if (SetMask(H6)&BlackPawns && SetMask(H4)&WhitePawns) trojan_check=1;
      }
    }
    if (BlackKingSQ==B8 || BlackKingSQ==A8) {
      if (SetMask(B5)&WhiteKnights || SetMask(B5)&WhiteBishops) {
        if (SetMask(A6)&BlackPawns && SetMask(A4)&BlackPawns) trojan_check=1;
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now, if any of the values above were changed, we must  |
|   clear the appropriate hash tables so that the new      |
|   values will be used to compute scores.                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (((last_crafty_is_white != crafty_is_white) ||
       (last_trojan_check    != trojan_check) ||
       (hashing_opening      != opening) ||
       (hashing_middle_game  != middle_game) ||
       (hashing_end_game     != end_game)) && !test_mode) {
/*
 ------------------------------------------------
|                                                |
|   if anything changed, the transposition table |
|   must be cleared of positional evaluations.   |
|                                                |
 ------------------------------------------------
*/
    hlaseni_clearing_hash_tables++;  // port: bylo Memo1, ř. 5905-5906
    ClearHashTableScores(1);
    last_clear=move_number;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now for a kludge.  If we are beyond the halfway point  |
|   with respect to a 50 move draw, then clear the hash    |
|   scores every 10 moves to avoid hiding the 50 move      |
|   result from the search due to hash hits.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(0) > 50) {
    if (last_clear<move_number-10 || Rule50Moves(0)>90) {
      ClearHashTableScores(0);
      hlaseni_clearing_hash_tables_50++;  // port: bylo Memo1, ř. 5923
    }
  }
  hashing_opening=opening;
  hashing_middle_game=middle_game;
  hashing_end_game=end_game;
  last_crafty_is_white=crafty_is_white;
  last_trojan_check=trojan_check;
}

// RedCrafty_481_f.cpp, ř. 5934-5937: Book
int Book(TREE *tree, int wtm, int root_list_done) {
  // Stan upravil
  return(0);
}

// RedCrafty_481_f.cpp, ř. 5940-6136: RootMoveList
/* last modified 07/16/99 */
/*
********************************************************************************
*                                                                              *
*   RootMoveList() is used to set up the ply one move list.  it is a  more     *
*   accurate ordering of the move list than that done for plies deeper than    *
*   one.  briefly, Quiesce() is used to obtain the positional score plus the   *
*   expected gain/loss for pieces that can be captured.                        *
*                                                                              *
********************************************************************************
*/
void RootMoveList(int wtm) {
  int *mvp, *lastm, rmoves[256], sort_value[256];
  int i, done, temp, value;
  TREE * const tree=local[0];
  int tb_value;
  int mating_via_tb=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if the position at the root is a draw, based on EGTB   |
|   results, we are going to behave differently.  we will  |
|   extract the root moves that are draws, and toss the    |
|   losers out.  then, we will do a normal search on the   |
|   moves that draw to try and chose the most difficult    |
|   drawing move.                                          |
|                                                          |
 ----------------------------------------------------------
*/

//  Stan smazal:

/*
 ----------------------------------------------------------
|                                                          |
|   first, use GenerateMoves() to generate the set of      |
|   legal moves from the root position.                    |
|                                                          |
 ----------------------------------------------------------
*/
  easy_move=0;
  lastm=GenerateCaptures(tree, 1, wtm, rmoves);
  lastm=GenerateNonCaptures(tree, 1, wtm, lastm);
  n_root_moves=lastm-rmoves;
/*
 ----------------------------------------------------------
|                                                          |
|   now make each move and use Evaluate() to compute the   |
|   positional evaluation.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  for (mvp=rmoves;mvp<lastm;mvp++) {
    value=-4000000;
    MakeMove(tree, 1, *mvp, wtm);
    if (!Check(wtm)) do {
      tree->current_move[1]=*mvp;
      // Stan smazal
      value=-Quiesce(tree,-MATE,MATE,ChangeSide(wtm),2);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if this move is part of the previous    |
|   principal variation.  it was good in the search, we    |
|   should try it first now.                               |
|                                                          |
 ----------------------------------------------------------
*/

      if((Piece(*mvp)    == Piece(last_pv.path[1])) &&
         (From(*mvp)     == From(last_pv.path[1])) &&
         (To(*mvp)       == To(last_pv.path[1])) &&
         (Captured(*mvp) == Captured(last_pv.path[1])) &&
         (Promote(*mvp)  == Promote(last_pv.path[1])))
        value+=2000000;
/*
 ----------------------------------------------------------
|                                                          |
|   fudge the score for promotions so that promotion to a  |
|   queen is tried first.  since the positional score is   |
|   computed before the piece is removed, under-promoting  |
|   sometimes looks better.                                |
|                                                          |
 ----------------------------------------------------------
*/
      if (Promote(*mvp) && (Promote(*mvp) != queen)) value-=50;
    } while(0);
    sort_value[mvp-rmoves]=value;
    UnmakeMove(tree, 1, *mvp, wtm);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now sort the moves into order based on the sum of the  |
|   positional score less the possible hung pieces, and    |
|   factoring in a bonus for following the PV move.        |
|                                                          |
 ----------------------------------------------------------
*/
  do {
    done=1;
    for (i=0;i<lastm-rmoves-1;i++) {
      if (sort_value[i] < sort_value[i+1]) {
        temp=sort_value[i];
        sort_value[i]=sort_value[i+1];
        sort_value[i+1]=temp;
        temp=rmoves[i];
        rmoves[i]=rmoves[i+1];
        rmoves[i+1]=temp;
        done=0;
      }
    }
  } while(!done);
/*
 ----------------------------------------------------------
|                                                          |
|   now trim the move list to eliminate those moves that   |
|   "hung" the king and are illegal.                       |
|                                                          |
 ----------------------------------------------------------
*/
  for (;n_root_moves;n_root_moves--)
    if (sort_value[n_root_moves-1] > -3000000) break;
  if (sort_value[0] > 1000000) sort_value[0]-=2000000;
  if (sort_value[0] > sort_value[1]+200 &&
      ((To(rmoves[0]) == To(last_opponent_move) &&
        Captured(rmoves[0]) == Piece(last_opponent_move)) ||
      sort_value[0] < PAWN_VALUE)) easy_move=1;
/*
 ----------------------------------------------------------
|                                                          |
|   debugging output to dump root move list and the stuff  |
|   used to sort them, for testing and debugging.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (display_options & 512) {
    Print(512,"%d moves at root\n",n_root_moves);
    Print(512,"        move   score\n");
    for (i=0;i<n_root_moves;i++) {
      tree->current_move[1]=rmoves[i];
      Print(512,"%12s",OutputMove(tree,rmoves[i],1,wtm));
      Print(512,"%8d\n",sort_value[i]);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now check to see if we are in the special mode where   |
|   moves need to be searched because of missing EGTBs.    |
|                                                          |
 ----------------------------------------------------------
*/

  // Stan smazal

/*
 ----------------------------------------------------------
|                                                          |
|   now copy the root moves into the root_move structure   |
|   array for use by NextRootMove().                       |
|                                                          |
 ----------------------------------------------------------
*/
  for (i=0;i<n_root_moves;i++) {
    root_moves[i].move=rmoves[i];
    root_moves[i].nodes=0;
    root_moves[i].status=0;
  }


    // Next Best Move:
//  Memo1->Lines->Add( " root moves >>> " + IntToStr( n_root_moves));
  if (status_Next_Best_Move) {     // pozor, pouze v případě, že stisknuto F8
  // oprava č. 6: záloha pro případ, že by se vyřadily všechny tahy
  ROOT_MOVE zaloha_root_moves[256];
  const int zaloha_n_root_moves=n_root_moves;
  for (i=0;i<n_root_moves;i++) zaloha_root_moves[i]=root_moves[i];
  for (i=0;i<n_root_moves;i++) {
    // port: výpis do Memo1 (ř. 6115) pryč

    for (int ssii=0; ssii<(int) seznam_Next_Best_Move.size(); ssii++) {
//      AnsiString a1 = AnsiString(OutputMove(tree,root_moves[i].move,0,wtm));
//      AnsiString a2 = seznam_Next_Best_Move->Strings[ssii];
      if (std::string(OutputMove(tree,root_moves[i].move,0,wtm)) ==
          seznam_Next_Best_Move[ssii]) {
        root_moves[i] = root_moves[n_root_moves-1];
        n_root_moves--;
        // port: výpis do Memo1 (ř. 6124) pryč
//        Memo1->Lines->Add( " výmaz " + a1);
        // oprava č. 6 (PORT-CRAFTY.md, oddíl 3): na pozici i je teď tah
        // přesunutý z konce seznamu a ten se musí otestovat taky - bez
        // i-- by ho i++ přeskočilo a vnitřní smyčka by ho porovnala jen
        // se zbytkem seznamu Next Best Move
        i--;
        break;
      };
    }
  }
  // oprava č. 6: vyřazené všechny tahy by Iterate vzal jako mat nebo pat
  // (n_root_moves == 0, ř. 4619); v tom případě se seznam nepoužije
  if (n_root_moves == 0) {
    n_root_moves=zaloha_n_root_moves;
    for (i=0;i<n_root_moves;i++) root_moves[i]=zaloha_root_moves[i];
  }
  }
//  Memo1->Lines->Add( " <<< root moves " + IntToStr( n_root_moves));




  return;
}

// RedCrafty_481_f.cpp, ř. 6138-6188: HashStorePV
/* last modified 01/30/99 */
/*
********************************************************************************
*                                                                              *
*   HashStorePV() is called by Iterate() to insert the PV moves so they will   *
*   be searched before any other moves.                                        *
*                                                                              *
********************************************************************************
*/
void HashStorePV(TREE *tree, int ply, int wtm) {
  HASH_ENTRY *htablea, *htableb;
  BITBOARD temp_hashkey;
/*
 ----------------------------------------------------------
|                                                          |
|   first, compute the initial hash address and choose     |
|   which hash table (based on color) to probe.            |
|                                                          |
 ----------------------------------------------------------
*/
  if (port_bez_hashe) return;  // port: přepínač pro test, viz crafty_search.h
  temp_hashkey=(wtm) ? HashKey : ~HashKey;
  htablea=trans_ref_a+(((int) temp_hashkey)&hash_maska);
  htableb=trans_ref_b+(((int) temp_hashkey)&hash_maskb);
/*
 ----------------------------------------------------------
|                                                          |
|   now "fill in the blank" and build a table entry from   |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if ((htablea->word2^htablea->word1) == temp_hashkey &&
      (htablea->word1>>61)) {
    htablea->word2=htablea->word2^htablea->word1;
    htablea->word1&=~((BITBOARD) 07777777<<32);
    htablea->word1|=(BITBOARD) tree->pv[ply].path[ply]<<32;
    htablea->word2=htablea->word2^htablea->word1;
  }
  else if ((htableb->word2^htableb->word1) == temp_hashkey) {
    htableb->word2=htableb->word2^htableb->word1;
    htableb->word1&=~((BITBOARD) 07777777<<32);
    htableb->word1|=(BITBOARD) tree->pv[ply].path[ply]<<32;
    htableb->word2=htableb->word2^htableb->word1;
  }
  else {
    htableb->word1=(BITBOARD) 65536;
    htableb->word1|=((BITBOARD) ((transposition_id<<2)+WORTHLESS))<<59;
    htableb->word1|=(BITBOARD) tree->pv[ply].path[ply]<<32;
    htableb->word2=temp_hashkey^htableb->word1;
  }
}

// RedCrafty_481_f.cpp, ř. 6190-6387: SearchRoot
/* modified 10/23/01 */
/*
********************************************************************************
*                                                                              *
*   SearchRoot() is the recursive routine used to implement the alpha/beta     *
*   negamax search (similar to minimax but simpler to code.)  SearchRoot() is  *
*   only called when ply=1.  it is somewhat different from Search() in that    *
*   some things (null move search, hash lookup, etc.) are not useful at the    *
*   root of the tree.  SearchRoot() calls Search() to search any positions     *
*   that are below ply=1.                                                      *
*                                                                              *
********************************************************************************
*/
int SearchRoot(TREE *tree, int alpha, int beta, int wtm, int depth) {
  int first_move=1;
  int initial_alpha, value;
  int extensions, extended;
  BITBOARD begin_root_nodes;
  // port: int delka (ř. 6208) patřil jen k výpisům do Memo1
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.  set NextMove() status to 0 so it will     |
|   know what has to be done.                              |
|                                                          |
 ----------------------------------------------------------
*/
  tree->in_check[2]=0;
  initial_alpha=alpha;
  tree->in_check[1]=Check(wtm);
  tree->next_status[1].phase=ROOT_MOVES;
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.  note that Search() culls any     |
|   move that is not legal by using Check().  the special  |
|   case is that we must find one legal move to search to  |
|   confirm that it's not a mate or draw.                  |
|                                                          |
 ----------------------------------------------------------
*/
  // port: čítač aktualni_tah (ř. 6232) sloužil jen pro Label3
  while ((tree->phase[1]=NextRootMove(tree,tree,wtm))) {
    // port: Label3, Label4 a Memo1 (ř. 6234-6244) pryč

/*
 ----------------------------------------------------------
|                                                          |
|   now make the move and search the resulting position.   |
|                                                          |
 ----------------------------------------------------------
*/
    MakeMove(tree,1,tree->current_move[1],wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   if the move to be made checks the opponent, then we    |
|   need to remember that he's in check and also extend    |
|   the depth by one ply for him to get out.               |
|                                                          |
 ----------------------------------------------------------
*/
    extended=0;
    if (Check(ChangeSide(wtm))) {
      tree->in_check[2]=1;
      tree->check_extensions_done++;
      extended+=incheck_depth;
    }
    else tree->in_check[2]=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a pawn to the 7th rank, we need to look     |
|   deeper to see if it can promote.                       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Piece(tree->current_move[1])==pawn &&
      push_extensions[To(tree->current_move[1])]) {
      tree->passed_pawn_extensions_done++;
      extended+=pushpp_depth;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now call Search to produce a value for this move.      |
|                                                          |
 ----------------------------------------------------------
*/
    begin_root_nodes=tree->nodes_searched;
    if (extended > 2*INCPLY) extended=2*INCPLY;
    extensions=extended-INCPLY;
    if (first_move) {
      // port: výpis do Memo1 (ř. 6294-6298) pryč

      if (depth+extensions >= INCPLY)
        value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL,0);
      else
        value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),2);

      // port: výpis do Memo1 (ř. 6306-6311) pryč
      if (abort_search) {
        UnmakeMove(tree,1,tree->current_move[1],wtm);
        return(alpha);
      }
      first_move=0;
    }
    else {
      if (depth+extensions >= INCPLY)
        value=-Search(tree,-alpha-1,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL,0);
      else
        value=-Quiesce(tree,-alpha-1,-alpha,ChangeSide(wtm),2);
      if (abort_search) {
        UnmakeMove(tree,1,tree->current_move[1],wtm);
        return(alpha);
      }
      if ((value > alpha) && (value < beta)) {
      // port: Memo1 a Label13 (ř. 6329-6331) pryč

      if (depth+extensions >= INCPLY)
        value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                      depth+extensions,2,DO_NULL,0);
      else
        value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),2);
      // port: výpis do Memo1 (ř. 6338-6339) pryč
        if (abort_search) {
          UnmakeMove(tree,1,tree->current_move[1],wtm);
          return(alpha);
        }
      }
    }
    root_moves[tree->root_move].nodes=tree->nodes_searched-begin_root_nodes;
    if (value > alpha) {
      SearchOutput(tree,value,beta);
      root_value=alpha;
      if(value >= beta) {
        History(tree,1,depth,wtm,tree->current_move[1]);
        UnmakeMove(tree,1,tree->current_move[1],wtm);
        return(value);
      }
      alpha=value;
    }
    root_value=alpha;
    UnmakeMove(tree,1,tree->current_move[1],wtm);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   all moves have been searched.  if none were legal,     |
|   return either MATE or DRAW depending on whether the    |
|   side to move is in check or not.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (abort_search) return(0);
  if (first_move == 1) {
    value=(Check(wtm)) ? -(MATE-1) : DrawScore(wtm);
    if (value >=alpha && value <beta) {
      tree->pv[0].pathl=0;
      tree->pv[0].pathh=0;
      tree->pv[0].pathd=iteration_depth;
      SearchOutput(tree,value,beta);
#if defined(TRACE)
      if (1 <= trace_level) printf("Search() no moves!  ply=1\n");  // port: bylo Memo1
#endif
    }
    return(value);
  }
  else {
    History(tree,1,depth,wtm,tree->pv[1].path[1]);
    return(alpha);
  }
}

// RedCrafty_481_f.cpp, ř. 6389-6482: DisplayPV
/* last modified 01/11/99 */
/*
********************************************************************************
*                                                                              *
*   DisplayPV() is used to display a PV during the search.  it will also note  *
*   when the PV was terminated by a hash table hit and will check the hash     *
*   entries to see if the PV can be extended by using moves from hits.         *
*                                                                              *
********************************************************************************
*/
void DisplayPV(TREE *tree, int level, int wtm, int time, int value, PATH *pv) {
  char buffer[512], *buffp, *bufftemp;
  int i, t_move_number, type, j, dummy;
  int nskip=0, twtm=wtm;
  root_print_ok=root_print_ok || tree->nodes_searched>noise_level ||
                abs(value)>MATE-300;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  // port: AnsiString s a Memo2 (ř. 6412-6413) pryč

  // Stanley smazal SMP

  if (level==5) type=4; else type=2;
  t_move_number=move_number;
/* Stan uzav
  if (display_options&64) sprintf(buffer," %d.",move_number);
  else buffer[0]=0;
  if ((display_options&64) && !wtm) sprintf(buffer+strlen(buffer)," ...");
*/
  for (i=1;i<=(int) pv->pathl;i++) {
    if ((display_options&64) && i>1 && wtm) {
//      Memo1->Lines->Add(IntToStr(t_move_number));
    }
//    s.sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,pv->path[i],i,wtm));
    // port: výpis tahu do Memo2 (ř. 6429-6430) pryč
    MakeMove(tree,i,pv->path[i],wtm);
    wtm=ChangeSide(wtm);
    if (wtm) t_move_number++;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the pv was terminated prematurely by a trans/ref    |
|   hit, see if any more moves are in the trans/ref table  |
|   and if so, add 'em to the end of the PV so we will     |
|   have better move ordering next iteration.              |
|                                                          |
 ----------------------------------------------------------
*/
  if(pv->pathh == 1) {
    // port: Memo2 (ř. 6446) pryč
    for (i=pv->pathl+1;i<MAXPLY;i++) {
      HashProbe(tree,i,0,wtm,&dummy,&dummy,&dummy);
      if (tree->hash_move[i] && LegalMove(tree,i,wtm,tree->hash_move[i])) {
        pv->path[i]=tree->hash_move[i];
        for (j=1;j<i;j++)
          if (pv->path[i] == pv->path[j]) break;
        if (j < i) break;
        pv->pathl++;
//        if ((display_options&64) && wtm) sprintf(buffer+strlen(buffer)," %d.",t_move_number);
//        sprintf(buffer+strlen(buffer)," %s",OutputMove(tree,pv->path[i],i,wtm));
        // port: Memo2 (ř. 6457) pryč
        MakeMove(tree,i,pv->path[i],wtm);
      }
      else break;
      wtm=ChangeSide(wtm);
      if (wtm) t_move_number++;
    }
    // port: Memo2 (ř. 6464) pryč
  }
  else if(pv->pathh == 2)
    ;  // port: Memo2 (ř. 6467) pryč

  // Stanley smazal

  for (i=pv->pathl;i>0;i--) {
    wtm=ChangeSide(wtm);
    UnmakeMove(tree,i,pv->path[i],wtm);
  }
  // port: místo výpisu do Memo2 (ř. 6475-6481) průběžné hlášení
  if (hlaseni_prubehu) {
    PrubeznyVysledek p;
    p.hloubka=iteration_depth;
    p.hodnota=value;
    p.uzlu=(long) tree->nodes_searched;
    p.nejlepsi_tah=pv->path[1];
    p.tah_nalezen=pv->pathl>=1;
    hlaseni_prubehu(hlaseni_prubehu_kontext,p);
  }
}

// RedCrafty_481_f.cpp, ř. 6484-6488: OutputGood
int OutputGood(TREE *tree, char* text, int ply, int wtm) {
  int new_move;
  new_move=InputMove(tree,text,ply,wtm,1,0);
  return (Piece(new_move));
}

// RedCrafty_481_f.cpp, ř. 6502-6539: ClearHashTableScores
void ClearHashTableScores(int dopawnstoo) {
  int i;

  if (trans_ref_a && trans_ref_b) {
    for (i=0;i<hash_table_size;i++) {
      int age=(trans_ref_a+i)->word1>>61;
      if (age) {
        (trans_ref_a+i)->word2^=(trans_ref_a+i)->word1;
        (trans_ref_a+i)->word1=((trans_ref_a+i)->word1 &
                                mask_clear_entry) | (BITBOARD) 65536;
        (trans_ref_a+i)->word2^=(trans_ref_a+i)->word1;
      }
    }
    for (i=0;i<2*hash_table_size;i++) {
      (trans_ref_b+i)->word2^=(trans_ref_b+i)->word1;
      (trans_ref_b+i)->word1=((trans_ref_b+i)->word1 &
                              mask_clear_entry) | (BITBOARD) 65536;
      (trans_ref_b+i)->word2^=(trans_ref_b+i)->word1;
    }
    if (dopawnstoo) {
      for (i=0;i<pawn_hash_table_size;i++) {
        (pawn_hash_table+i)->key=0;
        (pawn_hash_table+i)->p_score=0;
        (pawn_hash_table+i)->sta_protected=0;
        (pawn_hash_table+i)->black_defects_k=0;
        (pawn_hash_table+i)->black_defects_q=0;
        (pawn_hash_table+i)->white_defects_k=0;
        (pawn_hash_table+i)->white_defects_q=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->passed_w=0;
        (pawn_hash_table+i)->outside=0;
        (pawn_hash_table+i)->candidates_w=0;
        (pawn_hash_table+i)->candidates_b=0;
      }
    }
  }
  local[0]->pawn_score.key=0;
}

// RedCrafty_481_f.cpp, ř. 6542-6604: Phase
/* last modified 10/18/99 */
/*
********************************************************************************
*                                                                              *
*   Phase() is use to determine the phase of the game: opening, middlegame or  *
*   endgame.  the three phases are identified by the following criteria:       *
*                                                                              *
*   (a) opening:  if any minor pieces are still on their original squares, or  *
*       the king has not yet castled nor given up the right to castle, then    *
*       opening development rules still apply.                                 *
*   (b) middle-game:  if the opening criteria are not met, and the end-game    *
*       criteria are not met, then all that's left is a middle-game.           *
*   (c) end-game:  if both sides have less than 17 points of material, not     *
*       counting pawns, (Q=9, R=5, BN=3) then we are in an end-game.           *
*                                                                              *
********************************************************************************
*/
void Phase(void) {
  int t_opening, t_middle_game, t_end_game;
  TREE * const tree=local[0];

/*
 ----------------------------------------------------------
|                                                          |
|   if both  sides have not yet lost the right to          |
|   castle, then this is still an opening position.        |
|                                                          |
 ----------------------------------------------------------
*/
  t_opening=opening;
  t_middle_game=middle_game;
  t_end_game=end_game;
  if (opening) {
    do {
      if (move_number < 26) {
        if (WhiteCastle(1)>0) break;
        if (BlackCastle(1)>0) break;
      }
      opening=0;
      middle_game=1;
      end_game=0;
    } while (0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if the piece point count drops below 17 (two rooks and |
|   two minor pieces) then this is an end-game.            |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces < 15 && TotalBlackPieces < 15) {
    opening=0;
    middle_game=0;
    end_game=1;
  }
  // port: výpisy fáze do Memo1 (ř. 6598-6603) pryč
}

// RedCrafty_481_f.cpp, ř. 6606-7726: Evaluate
/* last modified 05/09/02 */
/*
********************************************************************************
*                                                                              *
*   Evaluate() is used to evaluate the chess board.  broadly, it addresses     *
*   four (4) distinct areas:  (1) material score which is simply a summing of  *
*   piece types multiplied by piece values;  (2) pawn scoring which considers  *
*   placement of pawns and also evaluates passed pawns, particularly in end-   *
*   game situations;  (3) piece scoring which evaluates the placement of each  *
*   piece as well as things like piece mobility;  (4) king safety which        *
*   considers the pawn shelter around the king along with material present to  *
*   facilitate an attack.                                                      *
*                                                                              *
********************************************************************************
*/

int Evaluate(TREE *tree, int ply, int wtm, int alpha, int beta) {
  BITBOARD temp;
  int square, file, score, tscore, w_tropism=0, b_tropism=0;
  int w_spread, b_spread, trop, can_win=3;
#if defined(DEBUGEV)
  int lastsc;
#endif
/*
**********************************************************************
*                                                                    *
*   check for draws due to insufficient material and adjust the      *
*   score as necessary.  this code also handles a special endgame    *
*   case where one side has only a lone king, and the king has no    *
*   legal moves.  this has been shown to break a few evaluation      *
*   terms such as bishop + wrong color rook pawn.  if this case is   *
*   detected, a drawscore is returned.                               *
*                                                                    *
**********************************************************************
*/
  if (TotalWhitePieces<9 && TotalBlackPieces<9) {
    can_win=EvaluateWinner(tree);
    if (EvaluateStalemate(tree,wtm)) can_win=0;
  }
  if (can_win == 0) return(DrawScore(wtm));
  score=EvaluateMaterial(tree);
#ifdef DEBUGEV
  printf("score[material]=                  %4d\n",score);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   determine if this is position should be evaluated to force mate  *
*   (neither side has pawns) or if it should be evaluated normally.  *
*   call EvaluatePawns() to evaluate the current pawn position.      *
*   this routine modifies the "passed" pawn bit-vector which         *
*   indicates whether a pawn on each file is passed or not.          *
*                                                                    *
**********************************************************************
*/
  tree->all_pawns=BlackPawns|WhitePawns;
  if ((TotalWhitePawns+TotalBlackPawns) == 0) do {
    int ms=EvaluateMate(tree);
    if (ms == 99999) break;
    score+=ms;
#ifdef DEBUGEV
    printf("score[mater]=                     %4d (%+d)\n",score,score-lastsc);
#endif
    if (score>DrawScore(1) && !(can_win&1)) return(DrawScore(wtm));
    if (score<DrawScore(1) && !(can_win&2)) return(DrawScore(wtm));
    return((wtm) ? score : -score);
  } while(0);
#if !defined(FAST)
  tree->pawn_probes++;
#endif
  if (PawnHashKey == tree->pawn_score.key) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    score+=tree->pawn_score.p_score;
  }
  else score+=EvaluatePawns(tree);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[pawns]=                     %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   if there are any passed pawns, first call EvaluatePassedPawns()  *
*   to evaluate them.  then, if one side has a passed pawn and the   *
*   other side has no pieces, call EvaluatePassedPawnRaces() to see  *
*   if the passed pawn can be stopped from promoting.  finally, we   *
*   use tree->pawn_score.outside to see if one side has an outside   *
*   passed pawn that represents a nearly won endgame advantage.      *
*                                                                    *
**********************************************************************
*/
  if (tree->pawn_score.passed_b || tree->pawn_score.passed_w) {
    int pscore=EvaluatePassedPawns(tree);
    if (tree->pawn_score.outside&8)
      pscore+=2*outside_passed[(int) TotalBlackPieces];
    else if (tree->pawn_score.outside&4)
      pscore+=outside_passed[(int) TotalBlackPieces];
    if (tree->pawn_score.outside&128)
      pscore-=2*outside_passed[(int) TotalWhitePieces];
    else if (tree->pawn_score.outside&64)
      pscore-=outside_passed[(int) TotalWhitePieces];
    if ((TotalWhitePieces==0 && tree->pawn_score.passed_b) ||
        (TotalBlackPieces==0 && tree->pawn_score.passed_w))
      pscore+=EvaluatePassedPawnRaces(tree,wtm);
    score+=pscore*passed_scale/100;
  }
  if (!(tree->pawn_score.outside&12)) {
    if (tree->pawn_score.outside&2)
      score+=2*majority[(int) TotalBlackPieces];
    else if (tree->pawn_score.outside&1)
      score+=majority[(int) TotalBlackPieces];
  }
  if (!(tree->pawn_score.outside&192)) {
     if (tree->pawn_score.outside&32)
       score-=2*majority[(int) TotalWhitePieces];
     else if (tree->pawn_score.outside&16)
       score-=majority[(int) TotalWhitePieces];
  }
  if (TotalWhitePieces+TotalBlackPieces == 0) {
    int pscore=0;
    int wfile=File(WhiteKingSQ);
    int bfile=File(BlackKingSQ);
    if (wfile+wtm > bfile+1) {
      if (wfile < FirstOne8Bit(tree->pawn_score.allb))
        pscore+=WON_KP_ENDING;
    }
    else if (wfile-wtm < bfile-1) {
      if (wfile > LastOne8Bit(tree->pawn_score.allb))
        pscore+=WON_KP_ENDING;
    }
    if (bfile > wfile+1-wtm) {
      if (bfile < FirstOne8Bit(tree->pawn_score.allw))
        pscore+=-WON_KP_ENDING;
    }
    else if (bfile < wfile-1+wtm) {
      if (bfile > LastOne8Bit(tree->pawn_score.allw))
        pscore+=-WON_KP_ENDING;
    }
    score+=pscore*passed_scale/100;
  }
  else {
    if (!TotalBlackPieces) {
      w_spread=file_spread[tree->pawn_score.passed_w|
                           tree->pawn_score.candidates_w];
      if (w_spread>1) score+=(w_spread-1)*(outside_passed[0]>>1);
    }
    if (!TotalWhitePieces) {
      b_spread=file_spread[tree->pawn_score.passed_b|
                           tree->pawn_score.candidates_b];
      if (b_spread>1) score-=(b_spread-1)*(outside_passed[0]>>1);
    }
  }
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[passed pawns]=              %4d (%+d)\n",score,score-lastsc);
    printf("score[passed pawns] (flags) =     %4x\n",tree->pawn_score.outside);
    lastsc=score;
  }
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the bishop is trapped at a2 or h2 with |
|   a pawn at b2 or g2 that can advance one square and     |
|   trap the bishop, or a pawn at b3 or g3 that has        |
|   trapped the bishop already.  also make sure that this  |
|   pawn is defended to close the trap.                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteBishops) {
    if (WhiteBishops&mask_A7H7) {
      if (WhiteBishops&SetMask(A7) && SetMask(B6)&BlackPawns)
        score-=BISHOP_TRAPPED;
      else if (WhiteBishops&SetMask(H7) && SetMask(G6)&BlackPawns)
        score-=BISHOP_TRAPPED;
    }
  }
  if (BlackBishops) {
    if (BlackBishops&mask_A2H2) {
      if (BlackBishops&SetMask(A2) && SetMask(B3)&WhitePawns)
        score+=BISHOP_TRAPPED;
      else if (BlackBishops&SetMask(H2) && SetMask(G3)&WhitePawns)
        score+=BISHOP_TRAPPED;
    }
  }
/*
**********************************************************************
*                                                                    *
*   call EvaluateDevelopment() to evaluate development.  note that   *
*   we only do this when either side has not castled.                *
*                                                                    *
**********************************************************************
*/
  if (WhiteCastle(1)) score+=EvaluateDevelopmentW(tree,ply);
  if (BlackCastle(1)) score+=EvaluateDevelopmentB(tree,ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[development]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   now evaluate king safety by analyzing the pawn shelter in front  *
*   of the king to determine if there are holes, open files or other *
*   weaknesses that will make an attack difficult to repel.  one     *
*   special case is to detect the presence of a bishop that protects *
*   squares left after the b/g pawn has been advanced, leaving holes *
*   around the king, and pawns/bishops and queen threatening on g2   *
*   and related squares.                                             *
*                                                                    *
**********************************************************************
*/
  tree->endgame=(TotalWhitePieces<=EG_MAT || TotalBlackPieces<=EG_MAT);
  tree->w_kingsq=WhiteKingSQ;
  tree->b_kingsq=BlackKingSQ;
  tree->w_safety=0;
  tree->b_safety=0;
  if (!tree->endgame) score+=EvaluateKingSafety(tree,ply);
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[king safety]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*  check to see if we can take a lazy exit and avoid the time-       *
*  consuming part of the evaluation code.                            *
*                                                                    *
**********************************************************************
*/
  if (can_win==3) {
    const int tscore=(wtm)?score:-score;
    if (tscore-lazy_eval_cutoff >= beta) return(beta);
    if (tscore+lazy_eval_cutoff <= alpha) return(alpha);
  }
  tscore=score;
  tree->evaluations++;
/*
**********************************************************************
*                                                                    *
*  now evaluate the kings.  these tests are done dynamically, since  *
*  they depend on various pieces that can't be hashed in the king    *
*  safety defect counts kept in the pawn hash table.                 *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->endgame) {
    if ((tree->all_pawns & mask_efgh) && (tree->all_pawns & mask_abcd))
      score+=kval_wn[WhiteKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score+=kval_wk[WhiteKingSQ];
    else if (tree->all_pawns & mask_abcd)
      score+=kval_wq[WhiteKingSQ];
  }
  if (WhiteKingSQ < A2) {
    if (!(king_attacks[WhiteKingSQ]&rank_mask[RANK2] &
             ~WhitePawns)) score-=KING_BACK_RANK;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the king has been forced to move and   |
|   has trapped a rook at a1/a2/b1/g1/h1/h2, if so, then   |
|   penalize the trapped rook to help extricate it.        |
|                                                          |
 ----------------------------------------------------------
*/
    if (WhiteKingSQ > E1) {
      if (WhiteRooks&mask_kr_trapped_w[FILEH-WhiteKingSQ])
        score-=ROOK_TRAPPED;
    }
    else if (WhiteKingSQ < D1) {
      if (WhiteRooks&mask_qr_trapped_w[WhiteKingSQ])
        score-=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(white)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif

/*
 ----------------------------------------------------------
|                                                          |
|   black king.                                            |
|                                                          |
|   first, check for a weak back rank.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->endgame) {
    if ((tree->all_pawns & mask_efgh) && (tree->all_pawns & mask_abcd))
      score-=kval_bn[BlackKingSQ];
    else if (tree->all_pawns & mask_efgh)
      score-=kval_bk[BlackKingSQ];
    else if (tree->all_pawns & mask_abcd)
      score-=kval_bq[BlackKingSQ];
  }
  if (BlackKingSQ > H7) {
    if (!(king_attacks[BlackKingSQ]&rank_mask[RANK7]&
             ~BlackPawns)) score+=KING_BACK_RANK;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the king has been forced to move and   |
|   has trapped a rook at a1/a2/b1/g1/h1/h2, if so, then   |
|   penalize the trapped rook to help extricate it.        |
|                                                          |
 ----------------------------------------------------------
*/
    if (BlackKingSQ > E8) {
      if (BlackRooks&mask_kr_trapped_b[FILEH-File(BlackKingSQ)])
        score+=ROOK_TRAPPED;
    }
    else if (BlackKingSQ < D8) {
      if (BlackRooks&mask_qr_trapped_b[File(BlackKingSQ)])
        score+=ROOK_TRAPPED;
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[kings(black)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif

/*
**********************************************************************
*                                                                    *
*   knight evaluation includes centralization and "outposts".        *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white knights.                                         |
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.  an outpost is much      |
|   stronger if it is supported by two pawns (capturing it |
|   results in a protected passed pawn) or if the opponent |
|   has no minor piece that can attack it.                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteKnights;
  while(temp) {
    square=FirstOne(temp);
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&BlackPawns)) {
      int defenders, safe=0;
      score+=white_outpost[square];
      if (b_pawn_attacks[square]&WhitePawns) {
        defenders=PopCnt(b_pawn_attacks[square]&WhitePawns);
        if (defenders && !BlackKnights) {
          if (light_squares&SetMask(square)) {
            if (!(light_squares&BlackBishops)) safe=1;
          }
          else {
            if (!(dark_squares&BlackBishops)) safe=1;
          }
        }
        if (safe || defenders == 2) score+=white_outpost[square];
        else if (defenders == 1) score+=white_outpost[square]>>1;
      }
      if ((square==D6 || square==E6) &&
          PcOnSq(square+8)==-pawn) score+=BLOCKED_CENTER_PAWN;
      if ((square==D3 || square==E3) &&
          PcOnSq(square-8)==pawn) score-=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "nval_*".                                |
|                                                          |
 ----------------------------------------------------------
*/
    score+=nval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_n[Distance(square,tree->b_kingsq)];
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(white)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black knights.                                         |
|                                                          |
|   first, evaluate for "outposts" which is a knight that  |
|   can't be driven off by an enemy pawn, and which is     |
|   supported by a friendly pawn.  an outpost is much      |
|   stronger if it is supported by two pawns (capturing it |
|   results in a protected passed pawn) or if the opponent |
|   has no minor piece that can attack it.                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackKnights;
  while(temp) {
    square=FirstOne(temp);
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_w[square]&WhitePawns)) {
      int defenders, safe=0;
      score-=black_outpost[square];
      if (w_pawn_attacks[square]&BlackPawns) {
        defenders=PopCnt(w_pawn_attacks[square]&BlackPawns);
        if (defenders && !WhiteKnights) {
          if (light_squares&SetMask(square)) {
            if (!(light_squares&WhiteBishops)) safe=1;
          }
          else {
            if (!(dark_squares&WhiteBishops)) safe=1;
          }
        }
        if (safe || defenders == 2) score-=black_outpost[square];
        else if (defenders == 1) score-=black_outpost[square]>>1;
      }
      if ((square==D3 || square==E3) &&
          PcOnSq(square-8)==pawn) score-=BLOCKED_CENTER_PAWN;
      if ((square==D6 || square==E6) &&
          PcOnSq(square+8)==-pawn) score+=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in centralization score from the piece/square |
|   table "nval_*".                                |
|                                                          |
 ----------------------------------------------------------
*/
    score-=nval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_n[Distance(square,tree->w_kingsq)];
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[knights(black)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   bishop evaluation includes mobility, centralization as well as   *
*   a bonus for having the bishop pair.  a special case is a bishop  *
*   that is fianchettoed in front of a castled king.  this bishop is *
*   defending very weak squares and is therefore more valuable and   *
*   should not be traded.                                            *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white bishops                                          |
|                                                          |
|   first, locate each bishop and add in its static score  |
|   from the bishop piece/square table.                    |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteBishops;
  while(temp) {
    square=FirstOne(temp);
    score+=bval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the mobility score.                       |
|                                                          |
 ----------------------------------------------------------
*/
    score+=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   now add in a bonus for a bishop blocking a center pawn |
|   at D6/E6 as that is very cramping.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (white_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&BlackPawns)) {
      if ((square==D6 || square==E6) &&
          PcOnSq(square+8)==-pawn) score+=BLOCKED_CENTER_PAWN;
      if ((square==D3 || square==E3) &&
          PcOnSq(square-8)==pawn) score-=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=king_tropism_b[Distance(square,tree->b_kingsq)];
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(white)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black bishops                                          |
|                                                          |
|   first, locate each bishop and add in its static score  |
|   from the bishop piece/square table.                    |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackBishops;
  while(temp) {
    square=FirstOne(temp);
    score-=bval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   then fold in the mobility score.                       |
|                                                          |
 ----------------------------------------------------------
*/
    score-=BISHOP_MOBILITY*MobilityBishop(square);
/*
 ----------------------------------------------------------
|                                                          |
|   now add in a bonus for a bishop blocking a center pawn |
|   at D3/E3 as that is very cramping.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (black_outpost[square] &&
        !(mask_no_pawn_attacks_b[square]&WhitePawns)) {
      if ((square==D3 || square==E3) &&
          PcOnSq(square-8)==pawn) score-=BLOCKED_CENTER_PAWN;
      if ((square==D6 || square==E6) &&
          PcOnSq(square+8)==-pawn) score+=BLOCKED_CENTER_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=king_tropism_b[Distance(square,tree->w_kingsq)];
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishops(black)]=            %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now, give either side a bonus for having two bishops.  |
|   and penalize pawns on the same color as the bishop, if |
|   one side has only one bishop.                          |
|                                                          |
|   also add in a bonus for a bishop, if there are pawns   |
|   on both sides of the board in an endgame, because a    |
|   bishop is much more valuable there.                    |
|                                                          |
|   add in a bonus for filling holes when the king has     |
|   castled and played b3/g3/b6/g6.                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteBishops) {
    if (WhiteBishops&(WhiteBishops-1))
      score+=bishop_pair[TotalWhitePawns];
    else {
      if (WhiteBishops&light_squares)
        score-=PopCnt(WhitePawns&light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      else
        score-=PopCnt(WhitePawns&dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      if (TotalWhitePieces<7 && !BlackBishops &&
          tree->all_pawns&mask_fgh && tree->all_pawns&mask_abc)
        score+=BISHOP_OVER_KNIGHT_ENDGAME;
    }
    if (!tree->endgame) {
      if (File(tree->w_kingsq) > FILEE) {
        if (!(WhitePawns&SetMask(G2)) && (WhitePawns&SetMask(G3)) &&
            Distance(tree->w_kingsq,G2)==1 && WhiteBishops&good_bishop_kw)
          score+=BISHOP_KING_SAFETY;
      }
      else if (File(tree->w_kingsq) < FILED) {
        if (!(WhitePawns&SetMask(B2)) && (WhitePawns&SetMask(B3)) &&
            Distance(tree->w_kingsq,B2)==1 && WhiteBishops&good_bishop_qw)
          score+=BISHOP_KING_SAFETY;
      }
    }
  }
  if (BlackBishops) {
    if (BlackBishops&(BlackBishops-1)) {
      score-=bishop_pair[TotalBlackPawns];
    }
    else {
      if (BlackBishops&light_squares)
        score+=PopCnt(BlackPawns&light_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      else
        score+=PopCnt(BlackPawns&dark_squares)*BISHOP_PLUS_PAWNS_ON_COLOR;
      if (TotalBlackPieces<7 && !WhiteBishops &&
          tree->all_pawns&mask_fgh && tree->all_pawns&mask_abc)
        score-=BISHOP_OVER_KNIGHT_ENDGAME;
    }
    if (!tree->endgame) {
      if (File(tree->b_kingsq) > FILEE) {
        if (!(BlackPawns&SetMask(G7)) && (BlackPawns& SetMask(G6)) &&
            Distance(tree->b_kingsq,G7)==1 && BlackBishops&good_bishop_kb)
          score-=BISHOP_KING_SAFETY;
      }
      else if (File(tree->b_kingsq) < FILED) {
        if (!(BlackPawns&SetMask(B7)) && (BlackPawns&SetMask(B6)) &&
            Distance(tree->b_kingsq,B7)==1 && BlackBishops&good_bishop_qb)
          score-=BISHOP_KING_SAFETY;
      }
    }
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[bishop pair]=               %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   rook evaluation includes several simple cases, including open    *
*   files, 7th rank, connected, etc.                                 *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white rooks                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteRooks;
  while(temp) {
    int mobility;
    square=FirstOne(temp);
    file=File(square);
    score+=rval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(file_mask[file]&tree->all_pawns)) {
      score+=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->b_kingsq);
    }
    else {
      if (tree->pawn_score.open_files) {
        unsigned char rankmvs=AttacksRank(square)>>(56-(square&0x38));
        if (!(rankmvs & tree->pawn_score.open_files))
          score-=ROOK_OPEN_FILE>>1;
      }
      if (!(file_mask[file]&WhitePawns)) {
        score+=ROOK_HALF_OPEN_FILE;
        trop=FileDistance(square,tree->b_kingsq);
      }
      else if (!(plus8dir[square]&WhitePawns)) {
        trop=FileDistance(square,tree->b_kingsq);
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook can move horizontally.  if not, it is  |
|   somewhat precarious in how it is situated.             |
|                                                          |
 ----------------------------------------------------------
*/
    mobility=0;
    if (File(square)<FILEH && PcOnSq(square+1)<=0) mobility++;
    if (File(square)>FILEA && PcOnSq(square-1)<=0) mobility++;
    if (!mobility) score-=ROOK_LIMITED;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is given a bonus.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file&tree->pawn_score.passed_w) {
      const int pawnsq=LastOne(WhitePawns&file_mask[file]);
      if (square < pawnsq) {
        if (!(BlackPieces&SetMask(pawnsq+8)))
          score+=ROOK_BEHIND_PASSED_PAWN;
        else
          score+=ROOK_BEHIND_PASSED_PAWN>>1;
      }
    }
    if (128>>file&tree->pawn_score.passed_b) {
      const int pawnsq=FirstOne(BlackPawns&file_mask[file]);
      if (pawnsq < square) score+=ROOK_BEHIND_PASSED_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   finally check to see if any rooks are on the 7th rank, |
|   with the opponent having pawns on that rank and the    |
|   opponent's king being hemmed in on the 7th/8th rank.   |
|   if so, and another rook or queen is also on the 7th,   |
|   then this is a *strong* positional advantage.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK7 && (BlackKingSQ>H7 ||
                                BlackPawns&rank_mask[RANK7])) {
      score+=ROOK_ON_7TH;
      if (tree->pawn_score.passed_w && BlackKingSQ>H7 &&
          !(BlackPieces&mask_abs7_w)) score+=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square)&(WhiteRooks|WhiteQueens))
        score+=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    w_tropism+=Max(king_tropism_at_r[trop],
                   king_tropism_r[Distance(square,tree->b_kingsq)]);
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(white)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black rooks                                            |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackRooks;
  while(temp) {
    int mobility;
    square=FirstOne(temp);
    file=File(square);
    score-=rval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   determine if the rook is on an open file.  if it is,   |
|   determine if this rook attacks another friendly rook,  |
|   making it difficult to drive the rooks off the file.   |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(file_mask[file]&tree->all_pawns)) {
      score-=ROOK_OPEN_FILE;
      trop=FileDistance(square,tree->w_kingsq);
    }
    else {
      unsigned char rankmvs=AttacksRank(square)>>(56-(square&0x38));
      if (!(rankmvs & tree->pawn_score.open_files))
        score+=ROOK_OPEN_FILE>>1;
      if (!(file_mask[file]&BlackPawns)) {
        score-=ROOK_HALF_OPEN_FILE;
        trop=FileDistance(square,tree->w_kingsq);
      }
      else if (!(minus8dir[square]&BlackPawns)) {
        trop=FileDistance(square,tree->w_kingsq);
      }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook can move horizontally.  if not, it is  |
|   somewhat precarious in how it is situated.             |
|                                                          |
 ----------------------------------------------------------
*/
    mobility=0;
    if (File(square)<FILEH && PcOnSq(square+1)>=0) mobility++;
    if (File(square)>FILEA && PcOnSq(square-1)>=0) mobility++;
    if (!mobility) score+=ROOK_LIMITED;
/*
 ----------------------------------------------------------
|                                                          |
|   see if the rook is behind a passed pawn.  if it is,    |
|   it is given a bonus.                                   |
|                                                          |
 ----------------------------------------------------------
*/
    if (128>>file&tree->pawn_score.passed_b) {
      const int pawnsq=FirstOne(BlackPawns&file_mask[file]);
      if (square > pawnsq) {
        if (!(WhitePieces&SetMask(pawnsq-8)))
          score-=ROOK_BEHIND_PASSED_PAWN;
        else
          score-=ROOK_BEHIND_PASSED_PAWN>>1;
      }
    }
    if (128>>file&tree->pawn_score.passed_w) {
      const int pawnsq=LastOne(WhitePawns&file_mask[file]);
      if (pawnsq > square) score-=ROOK_BEHIND_PASSED_PAWN;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   finally check to see if any rooks are on the 7th rank, |
|   with the opponent having pawns on that rank and the    |
|   opponent's king being hemmed in on the 7th/8th rank.   |
|   if so, and another rook or queen is also on the 7th,   |
|   then this is a *strong* positional advantage.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK2 && (WhiteKingSQ<A2 ||
                                WhitePawns&rank_mask[RANK2])) {
      score-=ROOK_ON_7TH;
      if (tree->pawn_score.passed_b && WhiteKingSQ<A2 &&
          !(WhitePieces&mask_abs7_b)) score-=ROOK_ABSOLUTE_7TH;
      if (AttacksRank(square)&(BlackRooks|BlackQueens))
        score-=ROOK_CONNECTED_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    b_tropism+=Max(king_tropism_at_r[trop],
                   king_tropism_r[Distance(square,tree->w_kingsq)]);
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[rooks(black)]=              %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
**********************************************************************
*                                                                    *
*   queen evaluation includes centralization, plus some bonuses that *
*   affect the opponent's king safety.                               *
*                                                                    *
**********************************************************************
*/
/*
 ----------------------------------------------------------
|                                                          |
|   white queens                                           |
|                                                          |
|   first locate each queen and obtain it's centralization |
|   score from the static piece/square table for queens.   |
|                                                          |
|   then, if the opposing side's king safety is much worse |
|   than the king safety for this side, add in a bonus to  |
|   keep the queen around.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=WhiteQueens;
  while(temp) {
    square=FirstOne(temp);
    score+=qval_w[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong position on   |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK7 && (BlackPawns&rank_mask[RANK7] ||
         (BlackKingSQ > H7))) {
      if (AttacksRank(square)&WhiteRooks) score+=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the white tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(WhitePawns&plus8dir[square]))
      trop=FileDistance(square,tree->b_kingsq);
    w_tropism+=Max(king_tropism_q[Distance(square,tree->b_kingsq)],
                   king_tropism_at_q[trop]);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if the opponent's king position is not  |
|   safe, since the queen is a strong attacking piece.     |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->b_safety > tree->w_safety+4) score+=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalWhitePawns > 4) {
      int offside=(File(square)<FILEC && File(tree->b_kingsq)>FILEE) ||
                  (File(square)>FILEF && File(tree->b_kingsq)<FILED);
      if (offside) {
        w_tropism-=QUEEN_OFFSIDE_TROPISM;
        w_tropism=Max(w_tropism,-8);
      }
    }
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(white)]=             %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   black queens                                           |
|                                                          |
|   first locate each queen and obtain it's centralization |
|   score from the static piece/square table for queens.   |
|                                                          |
|   then, if the opposing side's king safety is much worse |
|   than the king safety for this side, add in a bonus to  |
|   keep the queen around.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  temp=BlackQueens;
  while(temp) {
    square=FirstOne(temp);
    score-=qval_b[square];
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if the queen is in a strong position on   |
|   the 7th rank supported by a rook on the 7th.  if so,   |
|   the positional advantage is almost overwhelming.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square)==RANK2 && (WhitePawns&rank_mask[RANK2] ||
         (WhiteKingSQ < A2))) {
      if (AttacksRank(square)&BlackRooks) score-=QUEEN_ROOK_ON_7TH_RANK;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   adjust the black tropism count for this piece.         |
|                                                          |
 ----------------------------------------------------------
*/
    trop=7;
    if (!(BlackPawns&minus8dir[square]))
      trop=FileDistance(square,tree->w_kingsq);
    b_tropism+=Max(king_tropism_q[Distance(square,tree->w_kingsq)],
                   king_tropism_at_q[trop]);
/*
 ----------------------------------------------------------
|                                                          |
|   add in a bonus if the opponent's king position is not  |
|   safe, since the queen is a strong attacking piece.     |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->w_safety > tree->b_safety+4) score-=QUEEN_IS_STRONG;
/*
 ----------------------------------------------------------
|                                                          |
|   add in a penalty for being on the wrong side of the    |
|   board, which can lead to attacks by the opponent.      |
|                                                          |
 ----------------------------------------------------------
*/
    if (TotalBlackPawns > 4) {
      int offside=(File(square)<FILEC && File(tree->w_kingsq)>FILEE) ||
                  (File(square)>FILEF && File(tree->w_kingsq)<FILED);
      if (offside) {
        b_tropism-=QUEEN_OFFSIDE_TROPISM;
        b_tropism=Max(b_tropism,-8);
      }
     }
    staClear(square,temp);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[queens(black)]=             %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the king tropism score, which take into    |
|   account _all_ pieces for each side that are close to   |
|   the opponent's king. the calculation uses three values |
|   for each side.  the 'total tropism' number, multiplied |
|   by the opponent's king safety, but this is first run   |
|   through an indirect table probe to get a multiplier    |
|   value.  the ttemper[] array provides a number between  |
|   16 and N, where 16 says use all the tropism numbers,   |
|   32 says to double the tropism scores (ie this array is |
|   specified in units of 1/16th).                         |
|                                                          |
 ----------------------------------------------------------
*/
  score+=((tropism[w_tropism+8]*ttemper[tree->b_safety])>>4)-
         ((tropism[b_tropism+8]*ttemper[tree->w_safety])>>4);
#ifdef DEBUGEV
  if (score != lastsc) {
    printf("score[king tropism]=              %4d (%+d)\n",score,score-lastsc);
    printf("w_safety=%d  b_safety=%d\n",tree->w_safety,tree->b_safety);
    printf("w_tropism=%d  b_tropism=%d\n",w_tropism,b_tropism);
    printf("wtrop.score=%d\n",(tropism[w_tropism+8]*ttemper[tree->b_safety])>>4);
    printf("btrop.score=%d\n",(tropism[b_tropism+8]*ttemper[tree->w_safety])>>4);
  }
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the ending has only bishops of opposite colors, the |
|   score is pulled closer to a draw.  if the score says   |
|   one side is winning, but that side doesn't have enough |
|   material to win, the score is set to DRAW.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces<=11 && TotalBlackPieces<=11) {
    if (WhiteBishops && BlackBishops &&
        (!(WhiteBishops&(WhiteBishops-1)) &&
         !(BlackBishops&(BlackBishops-1)))) {
      if (square_color[FirstOne(BlackBishops)] !=
          square_color[FirstOne(WhiteBishops)]) {
        if (abs(Material) > 2*PAWN_VALUE) {
          if (TotalWhitePieces==3 && TotalBlackPieces==3)
            score=score>>1;
          else if (TotalWhitePieces==TotalBlackPieces)
            score=((score-Material)>>1)+Material;
        }
        else {
          if (TotalWhitePieces==3 && TotalBlackPieces==3)
            score=score>>2;
        }
      }
    }
  }
  if (can_win != 3) {
    if (can_win&2 && score>DrawScore(1)) score=DrawScore(1);
    else if (can_win&1 && score<DrawScore(1)) score=DrawScore(1);
  }
#ifdef DEBUGEV
  if (score != lastsc)
    printf("score[draws]=                     %4d (%+d)\n",score,score-lastsc);
  lastsc=score;
#endif
  if (abs(score-tscore) > lazy_eval_cutoff)
    lazy_eval_cutoff=abs(score-tscore);
  if (score > Material) {
    if (score-Material > largest_positional_score)
      largest_positional_score=score-Material;
  }
  else {
    if (Material-score > largest_positional_score)
      largest_positional_score=Material-score;
  }
  return((wtm) ? score : -score);
}

// RedCrafty_481_f.cpp, ř. 7728-7833: EvaluateDevelopmentB
/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDevelopmentB() is used to encourage the program to develop its     *
*   pieces before moving its queen.  standard developmental principles are     *
*   applied.  they include:  (1) don't move the queen until minor pieces are   *
*   developed;  (2) advance the center pawns as soon as possible;  (3) don't   *
*   move the king unless its a castling move.                                  *
*                                                                              *
********************************************************************************
*/
int EvaluateDevelopmentB(TREE *tree, int ply) {
  int possible, real, score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings, then also     |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(SetMask(E4)&WhitePawns) && SetMask(D4)&WhitePawns) {
    if (SetMask(C7)&BlackPawns &&
        SetMask(C6)&(BlackKnights|BlackBishops))
    score+=DEVELOPMENT_THEMATIC;
  }
  if (Occupied&((BlackPawns&rank_mask[RANK7])<<8)&
      (file_mask[FILED]|file_mask[FILEE]))
    score+=BLOCKED_CENTER_PAWN;
#ifdef DEBUGDV
  printf("developmentB.1 score=%d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackCastle(ply)>0 &&
      !(BlackQueens&SetMask(D8))) score+=DEVELOPMENT_QUEEN_EARLY;
#ifdef DEBUGDV
  printf("developmentB.2 score=%d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't moved at the beginning of the       |
|   search, but it has moved somewhere in the current      |
|   search path, make *sure* it's a castle move or else    |
|   penalize the loss of castling privilege.               |
|                                                          |
|   if it *is* a castle move, penalize the king if it is   |
|   castling to one side when the other side is much safer |
|   but will it will take longer to prepare to castle in   |
|   that safer direction.                                  |
|                                                          |
|   the final test is to see if castling rights have been  |
|   lost due to moving one rook.  if so, check to see if   |
|   the remaining castle right would put the king in an    |
|   unsafe position which is just as bad.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackCastle(1) == 3) {
    possible=0;
    real=0;
    if (BlackCastle(ply) < 0) {
      if (File(BlackKingSQ) >= FILEE) {
        real=tree->pawn_score.black_defects_k;
        possible=tree->pawn_score.black_defects_q;
      }
      else {
        real=tree->pawn_score.black_defects_q;
        possible=tree->pawn_score.black_defects_k;
      }
    }
    else if (BlackCastle(1)==3 && BlackCastle(ply)>0 && BlackCastle(ply)!=3) {
      if (BlackCastle(ply)&1) {
        real=tree->pawn_score.black_defects_k;
        possible=tree->pawn_score.black_defects_q;
      }
      else {
        real=tree->pawn_score.black_defects_q;
        possible=tree->pawn_score.black_defects_k;
      }
    }
    if (possible < real) score+=temper_b[2*(real-possible)];
  }
  if (BlackCastle(1) > 0) {
    if (BlackCastle(ply) == 0) {
      int wq;
      wq=(WhiteQueens) ? 2 : 1;
      score+=(!root_wtm) ? 2*wq*DEVELOPMENT_LOSING_CASTLE :
                             wq*DEVELOPMENT_LOSING_CASTLE;
    }
    if (BlackCastle(ply) > 0) score+=DEVELOPMENT_NOT_CASTLED;
  }
#ifdef DEBUGDV
  printf("developmentB.3 score=%d\n",score);
#endif
  return(score);
}

// RedCrafty_481_f.cpp, ř. 7835-7940: EvaluateDevelopmentW
/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateDevelopmentW() is used to encourage the program to develop its     *
*   pieces before moving its queen.  standard developmental principles are     *
*   applied.  they include:  (1) don't move the queen until minor pieces are   *
*   developed;  (2) advance the center pawns as soon as possible;  (3) don't   *
*   move the king unless its a castling move.                                  *
*                                                                              *
********************************************************************************
*/
int EvaluateDevelopmentW(TREE *tree, int ply) {
  int possible, real, score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, some "thematic" things, which includes don't    |
|   block the c-pawn in queen-pawn openings, then also     |
|   check to see if center pawns are blocked.              |
|                                                          |
 ----------------------------------------------------------
*/
  if (!(SetMask(E4)&WhitePawns) && SetMask(D4)&WhitePawns) {
    if (SetMask(C2)&WhitePawns &&
        SetMask(C3)&(WhiteKnights|WhiteBishops))
    score-=DEVELOPMENT_THEMATIC;
  }
  if (Occupied&((WhitePawns&rank_mask[RANK2])>>8) &
                       (file_mask[FILED]|file_mask[FILEE]))
    score-=BLOCKED_CENTER_PAWN;
#ifdef DEBUGDV
  printf("developmentW.1 score=%d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if all minor pieces aren't developed, then penalize    |
|   the queen if it has moved.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteCastle(ply)>0 &&
      !(WhiteQueens&SetMask(D1))) score-=DEVELOPMENT_QUEEN_EARLY;
#ifdef DEBUGDV
  printf("developmentW.2 score=%d\n",score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   if the king hasn't moved at the beginning of the       |
|   search, but it has moved somewhere in the current      |
|   search path, make *sure* it's a castle move or else    |
|   penalize the loss of castling privilege.               |
|                                                          |
|   if it *is* a castle move, penalize the king if it is   |
|   castling to one side when the other side is much safer |
|   but will it will take longer to prepare to castle in   |
|   that safer direction.                                  |
|                                                          |
|   the final test is to see if castling rights have been  |
|   lost due to moving one rook.  if so, check to see if   |
|   the remaining castle right would put the king in an    |
|   unsafe position which is just as bad.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhiteCastle(1) == 3) {
    possible=0;
    real=0;
    if (WhiteCastle(ply) < 0) {
      if (File(WhiteKingSQ) >= FILEE) {
        real=tree->pawn_score.white_defects_k;
        possible=tree->pawn_score.white_defects_q;
      }
      else {
        real=tree->pawn_score.white_defects_q;
        possible=tree->pawn_score.white_defects_k;
      }
    }
    else if (WhiteCastle(ply)>0 && WhiteCastle(ply)!=3) {
      if (WhiteCastle(ply)&1) {
        real=tree->pawn_score.white_defects_k;
        possible=tree->pawn_score.white_defects_q;
      }
      else {
        real=tree->pawn_score.white_defects_q;
        possible=tree->pawn_score.white_defects_k;
      }
    }
    if (possible < real) score-=temper_w[2*(real-possible)];
  }
  if (WhiteCastle(1) > 0) {
    int bq;
    if (WhiteCastle(ply) == 0) {
      bq=(BlackQueens) ? 2 : 1;
      score-=(root_wtm) ? 2*bq*DEVELOPMENT_LOSING_CASTLE :
                            bq*DEVELOPMENT_LOSING_CASTLE;
    }
    if (WhiteCastle(ply) > 0) score-=DEVELOPMENT_NOT_CASTLED;
  }
#ifdef DEBUGDV
  printf("developmentW.3 score=%d\n",score);
#endif
  return(score);
}

// RedCrafty_481_f.cpp, ř. 7942-8006: EvaluateMate
/* last modified 12/17/99 */
/*
********************************************************************************
*                                                                              *
*   EvaluateMate() is used to evaluate positions where neither side has pawns  *
*   and one side has enough material to force checkmate.  it simply trys to    *
*   force the losing king to the edge of the board, and then to the corner     *
*   where mates are easier to find.                                            *
*                                                                              *
********************************************************************************
*/
int EvaluateMate(TREE *tree) {
  int mate_score=DrawScore(1);
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has a bishop+knight and the other side has |
|   no pieces or pawns, then use the special bishop_knight |
|   scoring board for the losing king to force it to the   |
|   right corner for mate.                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if ((TotalBlackPieces==0) && (TotalWhitePieces==6) &&
      (!WhitePawns) && (!BlackPawns) && WhiteBishops && WhiteKnights) {
    if (dark_squares&WhiteBishops)
      mate_score=b_n_mate_dark_squares[BlackKingSQ];
    else
      mate_score=b_n_mate_light_squares[BlackKingSQ];
  }
  if ((TotalBlackPieces==6) && (TotalWhitePieces==0) &&
      (!WhitePawns) && (!BlackPawns) && BlackBishops && BlackKnights) {
    if (dark_squares&BlackBishops)
      mate_score=-b_n_mate_dark_squares[WhiteKingSQ];
    else
      mate_score=-b_n_mate_light_squares[WhiteKingSQ];
  }
  if (!mate_score) {
/*
 ----------------------------------------------------------
|                                                          |
|   if white is winning, force the black king to the edge  |
|   of the board.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    if (Material >0) {
      mate_score=mate[BlackKingSQ];
      mate_score-=(Distance(WhiteKingSQ,BlackKingSQ)-3)*KING_KING_TROPISM;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if black is winning, force the white king to the edge  |
|   of the board.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    else if (Material < 0) {
      mate_score=-mate[WhiteKingSQ];
      mate_score+=(Distance(WhiteKingSQ,BlackKingSQ)-3)*KING_KING_TROPISM;
    }
  }
  return(mate_score);
}

// RedCrafty_481_f.cpp, ř. 8008-8082: EvaluateMaterial
/* last modified 11/27/00 */
/*
********************************************************************************
*                                                                              *
*   EvaluateMaterial() is used to evaluate material on the board.  it really   *
*   accomplishes detecting cases where one side has made a 'bad trade' as the  *
*   comments below show.                                                       *
*                                                                              *
********************************************************************************
*/
int EvaluateMaterial(TREE *tree) {
  int score;
/*
**********************************************************************
*                                                                    *
*   we start with the raw Material balance for the current position. *
*                                                                    *
**********************************************************************
*/
  score=Material;
/*
**********************************************************************
*                                                                    *
*   check 1.  if one side is a whole piece ahead, this is good, as a *
*   piece for 3 pawns is generally bad, unless the pawns are so far  *
*   advanced that the search can see them causing problems already.  *
*                                                                    *
*   check 2.  if one side has a rook more than the other side, then  *
*   determine if the other side has two extra pieces.  this is a bad *
*   idea, trading two pieces for (say) a rook + pawn.                *
*                                                                    *
*   check 3.  if one side has two rooks or a queen vs three pieces   *
*   then the three pieces are often deadly, so that this is also a   *
*   bad trade.                                                       *
*                                                                    *
**********************************************************************
*/
  do {
    if (TotalWhitePieces<4 && TotalBlackPieces<4) break;
    if (WhiteMinors != BlackMinors) {
      if (WhiteMajors == BlackMajors) {
        if (WhiteMinors > BlackMinors)
          score+=BAD_TRADE;
        else
          score-=BAD_TRADE;
      }
      else if (BlackMajors-1 == WhiteMajors) {
        if (WhiteMinors > BlackMinors+1) score+=BAD_TRADE;
        else if (WhiteMinors > BlackMinors) score-=BAD_TRADE;
      }
      else if (WhiteMajors-1 == BlackMajors) {
        if (BlackMinors > WhiteMinors+1) score-=BAD_TRADE;
        else if (BlackMinors > WhiteMinors) score+=BAD_TRADE;
      }
      else if (abs(WhiteMajors-BlackMajors) == 2) {
        if (WhiteMinors > BlackMinors+2) score+=BAD_TRADE;
        else if (BlackMinors > WhiteMinors+2) score-=BAD_TRADE;
      }
      break;
    }
    else if (WhiteMajors == BlackMajors) {
      if (WhiteQueens && !BlackQueens) score+=BAD_TRADE;
      else if (!WhiteQueens && BlackQueens) score-=BAD_TRADE;
    }
    else {
      if (WhiteMajors>BlackMajors && WhiteMinors==BlackMinors) score+=BAD_TRADE;
      else if (BlackMajors>WhiteMajors && WhiteMinors==BlackMinors) score-=BAD_TRADE;
    }
  } while(0);
#ifdef DEBUGM
  printf("score[material]=                  %4d\n",Material);
  printf("score[bad trade]=                 %4d\n",score);
#endif
  return (score);
}

// RedCrafty_481_f.cpp, ř. 8084-8240: EvaluateKingSafety
/* last modified 05/14/01 */
/*
********************************************************************************
*                                                                              *
*   EvaluateKingSafety() is used to evaluate king safety for both sides, based *
*   on the pawns around the king and the material left on the board.           *
*                                                                              *
********************************************************************************
*/
int EvaluateKingSafety(TREE *tree, int ply) {
  int score=0;
/*
 ----------------------------------------------------------
|                                                          |
|   first, check for the "trojan horse" attack where the   |
|   opponent offers a piece to open the h-file with a very |
|   difficult to refute attack.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (trojan_check) {
    if (root_wtm && File(WhiteKingSQ) >= FILEE) {
      if (!(tree->all_pawns&file_mask[FILEH])) {
        if (BlackRooks && BlackQueens) score-=KING_SAFETY_MATE_THREAT;
      }
    }
    if (!root_wtm && File(BlackKingSQ) >= FILEE) {
      if (!(tree->all_pawns&file_mask[FILEH])) {
        if (WhiteRooks && WhiteQueens) score+=KING_SAFETY_MATE_THREAT;
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   Now do normal scoring, if the king has castled, the    |
|   pawns in front are important.  If not castled yet, the |
|   pawns on the kingside should be preserved for this.    |
|                                                          |
 ----------------------------------------------------------
*/
  tree->w_safety=king_defects_w[WhiteKingSQ];
  if (WhiteCastle(ply) <= 0) {
    if (File(WhiteKingSQ) >= FILEE) {
      if (File(WhiteKingSQ) == FILEH) tree->w_kingsq&=62;
      tree->w_safety+=tree->pawn_score.white_defects_k;
      if (root_wtm && use_asymmetry) {
        if (File(BlackKingSQ) <= FILED) tree->w_safety+=CASTLE_OPPOSITE;
      }
      if (!(WhitePawns&SetMask(G2))) {
        if (SetMask(F3)&(BlackPawns|BlackBishops) && BlackQueens)
          tree->w_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(WhiteKingSQ)==FILEE) {
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEF]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
    else if (File(WhiteKingSQ) <= FILED) {
      if (File(WhiteKingSQ) == FILEA) tree->w_kingsq|=1;
      tree->w_safety+=tree->pawn_score.white_defects_q;
      if (root_wtm && use_asymmetry) {
        if (File(BlackKingSQ) >= FILEE) tree->w_safety+=CASTLE_OPPOSITE;
      }
      if (!(WhitePawns&SetMask(B2))) {
        if (SetMask(C3)&(BlackPawns|BlackBishops) && BlackQueens)
          tree->w_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(WhiteKingSQ)==FILED) {
        if (!(tree->all_pawns&file_mask[FILEC]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
  }
  else {
    if (WhiteCastle(ply) != 3) {
      if (WhiteCastle(ply)&1)
        tree->w_safety+=tree->pawn_score.white_defects_k>>1;
      else if (WhiteCastle(ply)&2)
        tree->w_safety+=tree->pawn_score.white_defects_q>>1;
    }
    if (!(tree->all_pawns&file_mask[FILED]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
    if (!(tree->all_pawns&file_mask[FILEE]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE;
    if (!(tree->all_pawns&file_mask[FILEF]))
      tree->w_safety+=KING_SAFETY_OPEN_FILE>>1;
  }

  tree->b_safety=king_defects_b[BlackKingSQ];
  if (BlackCastle(ply) <= 0) {
    if (File(BlackKingSQ) >= FILEE) {
      if (File(BlackKingSQ) == FILEH) tree->b_kingsq&=62;
      tree->b_safety+=tree->pawn_score.black_defects_k;
      if (!root_wtm && use_asymmetry) {
        if (File(WhiteKingSQ) <= FILED) tree->b_safety+=CASTLE_OPPOSITE;
      }
      if (!(BlackPawns&SetMask(G7))) {
        if (SetMask(F6)&(WhitePawns|WhiteBishops) && WhiteQueens)
          tree->b_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(BlackKingSQ)==FILEE) {
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEF]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
    else if (File(BlackKingSQ) <= FILED) {
      if (File(BlackKingSQ) == FILEA) tree->b_kingsq|=1;
      tree->b_safety+=tree->pawn_score.black_defects_q;
      if (!root_wtm && use_asymmetry) {
        if (File(WhiteKingSQ) >= FILEE) tree->b_safety+=CASTLE_OPPOSITE;
      }
      if (!(BlackPawns&SetMask(B7))) {
        if (SetMask(C6)&(WhitePawns|WhiteBishops) && WhiteQueens)
          tree->b_safety+=KING_SAFETY_MATE_G2G7;
      }
      if (File(BlackKingSQ)==FILED) {
        if (!(tree->all_pawns&file_mask[FILEC]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
        if (!(tree->all_pawns&file_mask[FILED]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE;
        if (!(tree->all_pawns&file_mask[FILEE]))
          tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
      }
    }
  }
  else {
    if (BlackCastle(ply) != 3) {
      if (BlackCastle(ply)&1)
        tree->b_safety+=tree->pawn_score.black_defects_k>>1;
      else if (BlackCastle(ply)&2)
        tree->b_safety+=tree->pawn_score.black_defects_q>>1;
    }
    if (!(tree->all_pawns&file_mask[FILED]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
    if (!(tree->all_pawns&file_mask[FILEE]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE;
    if (!(tree->all_pawns&file_mask[FILEF]))
      tree->b_safety+=KING_SAFETY_OPEN_FILE>>1;
  }
  if (!WhiteQueens) tree->b_safety>>=1;
  if (!BlackQueens) tree->w_safety>>=1;
  score-=temper_w[tree->w_safety]-temper_b[tree->b_safety];
  return(score);
}

// RedCrafty_481_f.cpp, ř. 8242-8412: EvaluatePassedPawns
/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePassedPawns() is used to evaluate passed pawns and the danger      *
*   they produce.  the first bonus is for a passed pawn that has reached the   *
*   6th rank and is supported by the king, making it very difficult to stop it *
*   from queening.  the second case is two connected passed pawns on the 6th   *
*   or 7th rank, with the opposing side having little material to stop them.   *
*                                                                              *
********************************************************************************
*/
int EvaluatePassedPawns(TREE *tree) {
  int file, square, score=0;
  int white_king_sq, black_king_sq;
  int pawns;

/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.  at the same time, check to see if the  |
|   is blockaded by an enemy piece.  if so then the pawn   |
|   is less valuable since it can't advance easily.  as    |
|   material is removed, passed pawns also become more     |
|   valuable.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->pawn_score.passed_b) {
    black_king_sq=BlackKingSQ;
    pawns=tree->pawn_score.passed_b;
    while (pawns) {
      file=FirstOne8Bit(pawns);
      pawns&=~(128>>file);
      square=FirstOne(BlackPawns&file_mask[file]);
      if (FileDistance(square,black_king_sq)==1 &&
          Rank(black_king_sq)<=Rank(square))
        score-=supported_passer[RANK8-Rank(square)];
      if (SetMask(square-8)&Occupied) {
        score+=blockading_passed_pawn_value[RANK8-Rank(square)];
      }
    }
#ifdef DEBUGPP
  printf("score.1 after black passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has any connected passed pawns.  |
|   if so, and they have both reached the 6th/7th rank,    |
|   then they are very dangerous.                          |
|                                                          |
 ----------------------------------------------------------
*/
    pawns=tree->pawn_score.passed_b;
    while ((file=connected_passed[pawns])) {
      int square1,square2;
      pawns&=~(128>>file);
      square1=FirstOne(BlackPawns&file_mask[file-1]);
      square2=FirstOne(BlackPawns&file_mask[file]);
      score-=connected_passed_pawn_value[7-Max(Rank(square1),Rank(square2))];
      if (Rank(square1) > RANK3) continue;
      if (Rank(square2) > RANK3) continue;
      if (TotalWhitePieces < queen_v &&
          !(SetMask(square1-8)&WhitePieces) &&
          !(SetMask(square2-8)&WhitePieces)) {
        score-=PAWN_CONNECTED_PASSED_6TH;
        if (wtm) {
          if (!(WhiteKing&black_pawn_race_wtm[square1]) &&
              !(WhiteKing&black_pawn_race_wtm[square2]))
            score-=PAWN_CONNECTED_PASSED_6TH;
        }
        else {
          if (!(WhiteKing&black_pawn_race_btm[square1]) &&
              !(WhiteKing&black_pawn_race_btm[square2]))
            score-=PAWN_CONNECTED_PASSED_6TH;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after black passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has any passed pawns.  if so,    |
|   and the king supports the pawn, then the pawn is even  |
|   more valuable.  at the same time, check to see if the  |
|   is blockaded by an enemy piece.  if so then the pawn   |
|   is less valuable since it can't advance easily.  as    |
|   material is removed, passed pawns also become more     |
|   valuable.                                              |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->pawn_score.passed_w) {
    white_king_sq=WhiteKingSQ;
    pawns=tree->pawn_score.passed_w;
    while (pawns) {
      file=FirstOne8Bit(pawns);
      pawns&=~(128>>file);
      square=LastOne(WhitePawns&file_mask[file]);
      if (FileDistance(square,white_king_sq)==1 &&
          Rank(white_king_sq)>=Rank(square))
        score+=supported_passer[Rank(square)];
      if (SetMask(square+8)&Occupied) {
        score-=blockading_passed_pawn_value[Rank(square)];
      }
    }
#ifdef DEBUGPP
  printf("score.1 after white passers = %d\n", score);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has any connected passed pawns.  |
|   if so, and they have both reached the 6th/7th rank,    |
|   then they are very dangerous.                          |
|                                                          |
 ----------------------------------------------------------
*/
    pawns=tree->pawn_score.passed_w;
    while ((file=connected_passed[pawns])) {
      int square1,square2;
      pawns&=~(128>>file);
      square1=LastOne(WhitePawns&file_mask[file-1]);
      square2=LastOne(WhitePawns&file_mask[file]);
      score+=connected_passed_pawn_value[Min(Rank(square1),Rank(square2))];
      if (Rank(square1) < RANK6) continue;
      if (Rank(square2) < RANK6) continue;
      if (TotalBlackPieces < queen_v &&
          !(SetMask(square1+8)&BlackPieces) &&
          !(SetMask(square2+8)&BlackPieces)) {
        score+=PAWN_CONNECTED_PASSED_6TH;
        if (wtm) {
          if (!(BlackKing&white_pawn_race_wtm[square1]) &&
              !(BlackKing&white_pawn_race_wtm[square2]))
            score+=PAWN_CONNECTED_PASSED_6TH;
        }
        else {
          if (!(BlackKing&white_pawn_race_btm[square1]) &&
              !(BlackKing&white_pawn_race_btm[square2]))
            score+=PAWN_CONNECTED_PASSED_6TH;
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("score.2 after white passers = %d\n", score);
#endif
  if (TotalBlackPawns==1 && TotalWhitePawns==0 &&
      TotalBlackPieces==5 && TotalWhitePieces==5) {
    square=FirstOne(BlackPawns);
    if (FileDistance(WhiteKingSQ,square)<=1 &&
        Rank(WhiteKingSQ)<Rank(square)) return(0);
    if (Rank(BlackKingSQ)>Rank(square) ||
        FileDistance(BlackKingSQ,square) > 1) return(0);
  }
  if (TotalWhitePawns==1 && TotalBlackPawns==0 &&
      TotalWhitePieces==5 && TotalBlackPieces==5) {
    square=FirstOne(WhitePawns);
    if (FileDistance(BlackKingSQ,square)<=1 &&
        Rank(BlackKingSQ)>Rank(square)) return(0);
    if (Rank(WhiteKingSQ)<Rank(square) ||
        FileDistance(WhiteKingSQ,square) > 1) return(0);
  }
  return(score);
}

// RedCrafty_481_f.cpp, ř. 8414-8791: EvaluatePassedPawnRaces
/* last modified 12/03/01 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePassedPawnRaces() is used to evalaute passed pawns when one        *
*   side has passed pawns and the other side (or neither) has pieces.  in      *
*   such a case, the critical question is can the defending king stop the pawn *
*   from queening or is it too far away?  if only one side has pawns that can  *
*   "run" then the situation is simple.  when both sides have pawns that can   *
*   "run" it becomes more complex as it then becomes necessary to see if       *
*   pawn queens with check, or if either pawn queens and simultaneously        *
*   attacks the opposing side's queening square.  for the special case of a    *
*   single pawn, the simple evaluation rules are used:  king two squares in    *
*   front of the pawn=win, one square in front with opposition=win,  king on   *
*   6th pawn close by is a win.  rook pawns are handled separately and are     *
*   harder to queen.                                                           *
*                                                                              *
********************************************************************************
*/
int EvaluatePassedPawnRaces(TREE *tree, int wtm) {
  int file, square;
  int white_queener=8, white_square=0;
  int black_queener=8, black_square=0;
  int white_pawn=0, black_pawn=0, queen_distance;
  int pawnsq;
  BITBOARD tempw, tempb;
  int passed;

/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white has one pawn and neither side    |
|   has any pieces.  if so, use the simple pawn evaluation |
|   logic.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (WhitePawns && !BlackPawns &&
      !TotalWhitePieces && !TotalBlackPieces) do {
    pawnsq=LastOne(WhitePawns);
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if (Rank(WhiteKingSQ) <= Rank(pawnsq)) break;
/*
 ------------------------------------------------
|                                                |
|   first a special case.  if this is a rook     |
|   pawn, then the king must be on the adjacent  |
|   file, and be closer to the queening square   |
|   than the opposing king.                      |
|                                                |
 ------------------------------------------------
*/
    if (File(pawnsq) == FILEA) {
      if ((File(WhiteKingSQ) == FILEB) &&
          (Distance(WhiteKingSQ,A8) < Distance(BlackKingSQ,A8)))
        return(QUEEN_VALUE-BISHOP_VALUE);
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(WhiteKingSQ) == FILEG) &&
          (Distance(WhiteKingSQ,H8) < Distance(BlackKingSQ,H8)))
        return(QUEEN_VALUE-BISHOP_VALUE);
      break;
    }
/*
 ------------------------------------------------
|                                                |
|   if king is two squares in front of the pawn  |
|   then it's a win immediately.  if the king is |
|   on the 6th rank and closer to the pawn than  |
|   the opposing king, it's also a win.          |
|                                                |
 ------------------------------------------------
*/
    if (Distance(WhiteKingSQ,pawnsq) < Distance(BlackKingSQ,pawnsq)) {
      if (Rank(WhiteKingSQ) > Rank(pawnsq)+1)
        return(QUEEN_VALUE-BISHOP_VALUE);
      if (Rank(WhiteKingSQ) == RANK6)
        return(QUEEN_VALUE-BISHOP_VALUE);
    }
/*
 ------------------------------------------------
|                                                |
|   last chance:  if the king is one square in   |
|   front of the pawn and has the opposition,    |
|   then it's still a win.                       |
|                                                |
 ------------------------------------------------
*/
    if ((Rank(WhiteKingSQ) == Rank(pawnsq)+1) &&
        HasOpposition(wtm,WhiteKingSQ,BlackKingSQ))
      return(QUEEN_VALUE-BISHOP_VALUE);
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black has one pawn and neither side    |
|   has any pieces.  if so, use the simple pawn evaluation |
|   logic.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (BlackPawns && !WhitePawns &&
      !TotalWhitePieces && !TotalBlackPieces) do {
    pawnsq=FirstOne(BlackPawns);
/*
 ------------------------------------------------
|                                                |
|   king must be in front of the pawn or we      |
|   go no further.                               |
|                                                |
 ------------------------------------------------
*/
    if (Rank(BlackKingSQ) >= Rank(pawnsq)) break;
/*
 ------------------------------------------------
|                                                |
|   first a special case.  if this is a rook     |
|   pawn, then the king must be on the adjacent  |
|   file, and be closer to the queening square   |
|   than the opposing king.                      |
|                                                |
 ------------------------------------------------
*/
    if (File(pawnsq) == FILEA) {
      if ((File(BlackKingSQ) == FILEB) &&
          (Distance(BlackKingSQ,A1) < Distance(WhiteKingSQ,A1)))
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      break;
    }
    else if (File(pawnsq) == FILEH) {
      if ((File(BlackKingSQ) == FILEG) &&
          (Distance(BlackKingSQ,H1) < Distance(WhiteKingSQ,H1)))
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      break;
    }
/*
 ------------------------------------------------
|                                                |
|   if king is two squares in front of the pawn  |
|   then it's a win immediately.  if the king is |
|   on the 6th rank and closer to the pawn than  |
|   the opposing king, it's also a win.          |
|                                                |
 ------------------------------------------------
*/
    if (Distance(BlackKingSQ,pawnsq) < Distance(WhiteKingSQ,pawnsq)) {
      if (Rank(BlackKingSQ) < Rank(pawnsq)-1)
        return(-(QUEEN_VALUE-BISHOP_VALUE));
      if (Rank(BlackKingSQ) == RANK3)
        return(-(QUEEN_VALUE-BISHOP_VALUE));
    }
/*
 ------------------------------------------------
|                                                |
|   last chance:  if the king is one square in   |
|   front of the pawn and has the opposition,    |
|   then it's still a win.                       |
|                                                |
 ------------------------------------------------
*/
    if ((Rank(BlackKingSQ) == Rank(pawnsq)-1) &&
        HasOpposition(ChangeSide(wtm),BlackKingSQ,WhiteKingSQ))
      return(-(QUEEN_VALUE-BISHOP_VALUE));
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if white is out of pieces and black has   |
|   passed pawns.  if so, see if any of these passed pawns |
|   can outrun the defending king and promote.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (!TotalWhitePieces && tree->pawn_score.passed_b) {
    passed=tree->pawn_score.passed_b;
    while ((file=FirstOne8Bit(passed)) != 8) {
      passed&=~(128>>file);
      square=FirstOne(BlackPawns&file_mask[file]);
      if ((wtm && !(black_pawn_race_wtm[square]&WhiteKing)) ||
          (ChangeSide(wtm) && !(black_pawn_race_btm[square]&WhiteKing))) {
        queen_distance=Rank(square);
        if (BlackKing&minus8dir[square]) {
          if (file==FILEA || file==FILEH) queen_distance=99;
          queen_distance++;
        }
        if (Rank(square) == RANK7) queen_distance--;
        if (queen_distance < black_queener) {
          black_queener=queen_distance;
          black_square=file;
          black_pawn=square;
        }
      }
    }
    if (PopCnt8Bit(tree->pawn_score.passed_b) > 1) {
      int left=FirstOne(file_mask[FirstOne8Bit(tree->pawn_score.passed_b)]&BlackPawns);
      int right=FirstOne(file_mask[LastOne8Bit(tree->pawn_score.passed_b)]&BlackPawns);
      if (File(right)-File(left) > 1) {
        if (!(black_pawn_race_btm[left]&SetMask(right))) {
          queen_distance=Rank(left);
          if (Rank(left) == RANK7) queen_distance--;
          if (queen_distance < black_queener) {
            black_queener=queen_distance;
            black_square=File(left);
            black_pawn=left;
          }
        }
        if (!(black_pawn_race_btm[right]&SetMask(left))) {
          queen_distance=Rank(right);
          if (Rank(right) == RANK7) queen_distance--;
          if (queen_distance < black_queener) {
            black_queener=queen_distance;
            black_square=File(right);
            black_pawn=right;
          }
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("black pawn on %d can promote at %d in %d moves.\n",
         black_pawn,black_square,black_queener);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if black is out of pieces and white has   |
|   passed pawns.  if so, see if any of these passed pawns |
|   can outrun the defending king and promote.             |
|                                                          |
 ----------------------------------------------------------
*/
  if (!TotalBlackPieces && tree->pawn_score.passed_w) {
    passed=tree->pawn_score.passed_w;
    while ((file=FirstOne8Bit(passed)) != 8) {
      passed&=~(128>>file);
      square=LastOne(WhitePawns&file_mask[file]);
      if ((wtm && !(white_pawn_race_wtm[square]&BlackKing)) ||
          (ChangeSide(wtm) && !(white_pawn_race_btm[square]&BlackKing))) {
        queen_distance=RANK8-Rank(square);
        if (WhiteKing&plus8dir[square]) {
          if (file==FILEA || file==FILEH) queen_distance=99;
          queen_distance++;
        }
        if (Rank(square) == RANK2) queen_distance--;
        if (queen_distance < white_queener) {
          white_queener=queen_distance;
          white_square=file+A8;
          white_pawn=square;
        }
      }
    }
    if (PopCnt8Bit(tree->pawn_score.passed_w) > 1) {
      int left=LastOne(file_mask[FirstOne8Bit(tree->pawn_score.passed_w)]&WhitePawns);
      int right=LastOne(file_mask[LastOne8Bit(tree->pawn_score.passed_w)]&WhitePawns);
      if (File(right)-File(left) > 1) {
        if (!(white_pawn_race_wtm[left]&SetMask(right))) {
          queen_distance=RANK8-Rank(left);
          if (Rank(left) == RANK2) queen_distance--;
          if (queen_distance < white_queener) {
            white_queener=queen_distance;
            white_square=RANK8+File(left);
            white_pawn=left;
          }
        }
        if (!(white_pawn_race_wtm[right]&SetMask(left))) {
          queen_distance=RANK8-Rank(right);
          if (Rank(right) == RANK2) queen_distance--;
          if (queen_distance < white_queener) {
            white_queener=queen_distance;
            white_square=RANK8+File(right);
            white_pawn=right;
          }
        }
      }
    }
  }
#ifdef DEBUGPP
  printf("white pawn on %d can promote at %d in %d moves.\n",
         white_pawn,white_square,white_queener);
#endif

  do {
    if ((white_queener==8) && (black_queener==8)) break;
/*
 ----------------------------------------------------------
|                                                          |
|   now that we know which pawns can outrun the kings for  |
|   each side, we need to do the following:  if one side   |
|   queens before the other (two moves or more) then that  |
|   side wins.                                             |
|                                                          |
 ----------------------------------------------------------
*/
    if ((white_queener < 8) && (black_queener == 8))
      return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
    else if ((black_queener < 8) && (white_queener == 8))
      return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
    if (ChangeSide(wtm)) black_queener--;
    if (white_queener < black_queener)
      return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
    else if (black_queener < white_queener-1)
      return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
    if ((white_queener==8) || (black_queener==8)) break;
/*
 ----------------------------------------------------------
|                                                          |
|   if the white pawn queens one move before black, then   |
|   if the new queen checks the black king, or the new     |
|   queen attacks the queening square of black, white wins |
|   unless the black king is protecting the black queening |
|   square in which case it's a draw.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (white_queener == black_queener) {
      tempw=WhitePieces;
      staClear(white_pawn,WhitePieces);
      WhitePieces=WhitePieces|SetMask(white_square);
      tempb=BlackPieces;
      staClear(black_pawn,BlackPieces);
      BlackPieces=BlackPieces|SetMask(black_square);
      if (Attack(BlackKingSQ,white_square)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
      }
      if (Attack(black_square,white_square) &&
          !(king_attacks[black_square]&BlackKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(QUEEN_VALUE-BISHOP_VALUE+(5-white_queener)*PAWN_VALUE/10);
      }
      WhitePieces=tempw;
      BlackPieces=tempb;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if the black pawn queens one move before white, then   |
|   if the new queen checks the white king, or the new     |
|   queen attacks the queening square of white, black wins |
|   unless the white king is protecting the white queening |
|   square in which case it's a draw.                      |
|                                                          |
 ----------------------------------------------------------
*/
    if (black_queener == white_queener-1) {
      tempw=WhitePieces;
      staClear(white_pawn,WhitePieces);
      WhitePieces=WhitePieces|SetMask(white_square);
      tempb=BlackPieces;
      staClear(black_pawn,BlackPieces);
      BlackPieces=BlackPieces|SetMask(black_square);
      if (Attack(WhiteKingSQ,black_square)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
      }
      if (Attack(white_square,black_square) &&
          !(king_attacks[white_square]&WhiteKing)) {
        WhitePieces=tempw;
        BlackPieces=tempb;
        return(-(QUEEN_VALUE-BISHOP_VALUE+(5-black_queener)*PAWN_VALUE/10));
      }
      WhitePieces=tempw;
      BlackPieces=tempb;
    }
  } while(0);
  return(0);
}

// RedCrafty_481_f.cpp, ř. 8793-9828: EvaluatePawns
/* last modified 12/10/01 */
/*
********************************************************************************
*                                                                              *
*   EvaluatePawns() is used to evaluate pawns.  broadly, it addresses three    *
*   distinct actions:  (1) basic pawn scoring is hashed, the first thing done  *
*   is to see if the score is in the pawn hash table;  (2) passed pawn scoring *
*   for positions with pieces left;  (3) passed pawns where one side has a     *
*   passer and the other side has no pieces, so that the pawn can potentially  *
*   outrun the opposing king and promote.                                      *
*                                                                              *
********************************************************************************
*/
int EvaluatePawns(TREE *tree) {
  PAWN_HASH_ENTRY *ptable;
  BITBOARD pawns;
  BITBOARD temp, left, right;
  BITBOARD wp_moves, bp_moves;
  int score=0;
  int pns, square, file;
  int w_isolated, b_isolated;
  int w_isolated_of, b_isolated_of;
  int w_unblocked, b_unblocked;
  int wop, bop;
  int defenders, attackers, weakness, blocked, sq;
  int kside_open_files, qside_open_files;
  int kside_half_open_files_b, kside_half_open_files_w;
  int qside_half_open_files_b, qside_half_open_files_w;
  int kside_w, kside_b, qside_w, qside_b;
  int qmissb, kmissb, qmissw, kmissw;
#if defined(DEBUGP)
  int lastsc=0;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   first check to see if this position has been handled   |
|   before.  if so, we can skip the work saved in the pawn |
|   hash table.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  ptable=pawn_hash_table+(PawnHashKey&pawn_hash_mask);
  if (ptable->key == PawnHashKey) {
#if !defined(FAST)
    tree->pawn_hits++;
#endif
    tree->pawn_score=*ptable;
    return(tree->pawn_score.p_score);
  }
  tree->pawn_score.key=PawnHashKey;

  tree->pawn_score.allw=0;
  tree->pawn_score.white_defects_k=0;
  tree->pawn_score.white_defects_q=0;
  tree->pawn_score.candidates_w=0;
  tree->pawn_score.passed_w=0;

  tree->pawn_score.allb=0;
  tree->pawn_score.black_defects_k=0;
  tree->pawn_score.black_defects_q=0;
  tree->pawn_score.candidates_b=0;
  tree->pawn_score.passed_b=0;

  tree->pawn_score.outside=0;
  tree->pawn_score.sta_protected=0;
  tree->pawn_score.open_files=255;
  w_isolated=0;
  w_isolated_of=0;
  b_isolated=0;
  b_isolated_of=0;
  w_unblocked=0;
  b_unblocked=0;
  kside_open_files=0;
  qside_open_files=0;
  kside_half_open_files_b=0;
  kside_half_open_files_w=0;
  qside_half_open_files_b=0;
  qside_half_open_files_w=0;
  qmissb=0;
  kmissb=0;
  qmissw=0;
  kmissw=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if there are 8 pawns of one color, penalize crafty     |
|   for not opening the position.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (use_asymmetry) {
    if (root_wtm) {
      if (TotalWhitePawns == 8) score-=EIGHT_PAWNS;
    }
    else {
      if (TotalBlackPawns == 8) score+=EIGHT_PAWNS;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, determine which squares pawns can reach.        |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=WhitePawns;
  wp_moves=0;
  while (pawns) {
    square=FirstOne(pawns);
    tree->pawn_score.allw|=128>>File(square);
    for (sq=square;sq<A7;sq+=8) {
      wp_moves|=SetMask(sq);
      if (SetMask(sq+8)&tree->all_pawns) break;
      defenders=PopCnt(b_pawn_attacks[sq+8]&WhitePawns);
      attackers=PopCnt(w_pawn_attacks[sq+8]&BlackPawns);
      if (attackers-defenders > 0) break;
    }
    staClear(square,pawns);
  }
  pawns=BlackPawns;
  bp_moves=0;
  while (pawns) {
    square=FirstOne(pawns);
    tree->pawn_score.allb|=128>>File(square);
    for (sq=square;sq>H2;sq-=8) {
      bp_moves|=SetMask(sq);
      if (SetMask(sq-8)&tree->all_pawns) break;
      attackers=PopCnt(b_pawn_attacks[sq-8]&WhitePawns);
      defenders=PopCnt(w_pawn_attacks[sq-8]&BlackPawns);
      if (attackers-defenders > 0) break;
    }
    staClear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   white pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=WhitePawns;
  while (pawns) {
    square=LastOne(pawns);
    file=File(square);
    tree->pawn_score.open_files&=(~128>>file);
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate pawn advances.  center pawns are encouraged   |
|   to advance, while wing pawns are pretty much neutral.  |
|                                                          |
 ----------------------------------------------------------
*/
    score+=pval_w[square];
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[static] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate isolated pawns, which also detects that the   |
|   file is half-open making it easier to attack from the  |
|   front if rooks are on the board.  if the pawn is       |
|   isolated, all the weak pawn analysis is skipped.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_isolated[square]&WhitePawns)) {
      w_isolated++;
      if (!(plus8dir[square]&BlackPawns)) w_isolated_of++;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate blocked pawns.  these are pawns that can not  |
|   advance because they are blocked, or because they will |
|   be instantly lost due to other enemy pawns preventing  |
|   them from moving at all.                               |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      blocked=1;
      do {
        for (sq=square;sq<Min(square+24,A8);sq+=8) {
          defenders=PopCnt(b_pawn_attacks[sq]&wp_moves);
          attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
          if (attackers-defenders > 1) break;
          else if (attackers) {
            blocked=0;
            break;
          }
          if (SetMask(sq+8)&tree->all_pawns) break;
        }
        if (sq >= Min(square+24,A8)) blocked=0;
      } while(0);
      if (!blocked) w_unblocked++;
/*
 ----------------------------------------------------------
|                                                          |
|  evaluate weak pawns.  weak pawns are evaluated by the   |
|  following rules:  (1) if a pawn is defended by a pawn,  |
|  it isn't weak;  (2) if a pawn is undefended by a pawn   |
|  and advances one (or two if it hasn't moved yet) ranks  |
|  and is defended fewer times than it is attacked, it is  |
|  weak.  note that the penalty is greater if the pawn is  |
|  on an open file.  note that an isolated pawn is just    |
|  another case of a weak pawn, since it can never be      |
|  defended by a pawn.                                     |
|                                                          |
|  test the pawn where it is, and as it advances.  note    |
|  whether it can end up with more pawn defenders than     |
|  pawn attackers as it advances.                          |
|                                                          |
 ----------------------------------------------------------
*/
      do {
        weakness=0;
        if (plus8dir[square]&BlackPawns) break;
        defenders=PopCnt(b_pawn_attacks[square]&WhitePawns);
        attackers=PopCnt(w_pawn_attacks[square]&BlackPawns);
        if (defenders > attackers) break;
        defenders=PopCnt(b_pawn_attacks[square+8]&WhitePawns);
        attackers=PopCnt(w_pawn_attacks[square+8]&BlackPawns);
        if (attackers && attackers >= defenders) weakness=attackers-defenders+1;
        else if (attackers || defenders) break;
        if (!weakness) break;
/*
 ----------------------------------------------------------
|                                                          |
|  if the pawn can be defended by a pawn, and that pawn    |
|  can safely advance, then this pawn is not weak.         |
|                                                          |
 ----------------------------------------------------------
*/
        if ((temp=mask_no_pawn_attacks_w[square]&WhitePawns)) {
          if (file > FILEA) {
            const BITBOARD temp1=temp&file_mask[file-1];
            attackers=1;
            if (temp1) {
              const int defend_sq=LastOne(temp1);
              for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
                attackers=1;
                if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
                attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
                if (attackers) break;
              }
              if (!attackers) weakness=0;
            }
            if (!weakness) break;
          }
          if (file < FILEH) {
            const BITBOARD temp1=temp&file_mask[file+1];
            if (temp1) {
              const int defend_sq=LastOne(temp1);
              for (sq=defend_sq;sq<(Rank(square)<<3);sq+=8) {
                attackers=1;
                if (sq != defend_sq && tree->all_pawns&SetMask(sq)) break;
                attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
                if (attackers) break;
              }
              if (!attackers) weakness=0;
            }
          }
        }
        if (weakness > 0) {
          if (weakness == 3) score-=PAWN_WEAK_P2;
          else if (weakness) score-=PAWN_WEAK_P1;
        }
      } while(0);
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[weak] file=%d,     score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
      if ((pns=PopCnt(file_mask[file]&WhitePawns)) > 1) {
        score-=doubled_pawn_value[pns];
      }
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[doubled] file=%d,  score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn to see it if forms a "duo" which is two   |
|  pawns side-by-side.                                     |
|                                                          |
 ----------------------------------------------------------
*/
      if (mask_pawn_duo[square]&WhitePawns)
        score+=PAWN_DUO;
#ifdef DEBUGP
      if (score != lastsc)
        printf("white pawn[duo] file=%d,      score=%d\n", file,score);
      lastsc=score;
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_passed_w[square]&BlackPawns)) {
      score+=passed_pawn_value[Rank(square)];
      if (minus8dir[square]&WhitePawns)
        score-=passed_pawn_value[Rank(square)]>>1;
      if (mask_pawn_protected_w[square]&WhitePawns)
        tree->pawn_score.sta_protected|=1;
      tree->pawn_score.passed_w|=128>>file;
#ifdef DEBUGP
      printf("white pawn[passed]            file=%d\n", file);
      if (score != lastsc)
        printf("white pawn[passed]            score=%d\n", score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now determine if this pawn is a candidate passer,      |
|   since we now know it isn't passed.  a candidate is a   |
|   pawn on a file with no enemy pawns in front of it, and |
|   if it advances until it contacts an enemy pawn, and it |
|   is defended as many times as it is attacked when it    |
|   reaches that pawn, then all that is left is to see if  |
|   it is passed when the attacker(s) get removed.         |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      if (!(file_mask[File(square)]&BlackPawns) &&
          mask_pawn_isolated[square]&WhitePawns &&
          !(w_pawn_attacks[square]&BlackPawns)) {
        attackers=1;
        defenders=0;
        for (sq=square;sq<A7;sq+=8) {
          if (SetMask(sq+8)&tree->all_pawns) break;
          defenders=PopCnt(b_pawn_attacks[sq]&wp_moves);
          attackers=PopCnt(w_pawn_attacks[sq]&BlackPawns);
          if (attackers) break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_w[sq+8]&BlackPawns)) {
            tree->pawn_score.candidates_w|=128>>file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_w&128)) {
        if (file <= FILED) {
          if (WhitePawns&file_mask[file+1] && WhitePawns&file_mask[file+2]) {
            if (!(BlackPawns&file_mask[file]) &&
                !(BlackPawns&file_mask[file+2]) &&
                !(BlackPawns&file_mask[file+3]) &&
                PopCnt(BlackPawns&file_mask[file+1])<=2)
              tree->pawn_score.candidates_w|=128>>file;
          }
        }
        else {
          if (WhitePawns&file_mask[file-1] && WhitePawns&file_mask[file-2]) {
            if (!(BlackPawns&file_mask[file]) &&
                !(BlackPawns&file_mask[file-2]) &&
                !(BlackPawns&file_mask[file-3]) &&
                PopCnt(BlackPawns&file_mask[file-1])<=2)
              tree->pawn_score.candidates_w|=128>>file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_w&(128>>file))
        printf("white pawn[candidate]       square=%d\n", square);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate "hidden" passed pawns.  simple case is a pawn |
|   chain (white) at b5, a6, with a black pawn at a7.      |
|   it appears the b-pawn is backward, with a ram at a6/a7 |
|   but this is misleading, because the pawn at a6 is      |
|   really passed when white plays b6.                     |
|                                                          |
 ----------------------------------------------------------
*/
      if (Rank(square) > RANK5 && SetMask(square+8)&BlackPawns &&
          !(mask_pawn_passed_w[square+8]&BlackPawns) &&
          ((File(square) < FILEH && SetMask(square-7)&WhitePawns &&
            !(plus8dir[square-7]&BlackPawns) &&
            (File(square) == FILEG || !(plus8dir[square-6]&BlackPawns))) ||
           (File(square) > FILEA && SetMask(square-9)&WhitePawns &&
            !(plus8dir[square-9]&BlackPawns) &&
            (File(square) == FILEB || !(plus8dir[square-10]&BlackPawns)))))
        score+=hidden_passed_pawn_value[Rank(square)];
#ifdef DEBUGP
    if (score != lastsc)
      printf("white pawn[hidden] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
    }
    staClear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   black pawns.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  pawns=BlackPawns;
  while(pawns) {
    square=FirstOne(pawns);
    file=File(square);
    tree->pawn_score.open_files&=(~128>>file);
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate pawn advances.  center pawns are encouraged   |
|   to advance, while wing pawns are pretty much neutral.  |
|                                                          |
 ----------------------------------------------------------
*/
    score-=pval_b[square];
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[static] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate isolated pawns, which also detects that the   |
|   file is half-open making it easier to attack from the  |
|   front if rooks are on the board.  if the pawn is       |
|   isolated, all the weak pawn analysis is skipped.       |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_isolated[square]&BlackPawns)) {
      b_isolated++;
      if (!(minus8dir[square]&WhitePawns)) b_isolated_of++;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate blocked pawns.  these are pawns that can not  |
|   advance because they are blocked, or because they will |
|   be instantly lost due to other enemy pawns preventing  |
|   them from moving at all.                               |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      blocked=1;
      do {
        for (sq=square;sq>Max(square-24,H1);sq-=8) {
          attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
          defenders=PopCnt(w_pawn_attacks[sq]&bp_moves);
          if (attackers-defenders > 1) break;
          else if (attackers) {
            blocked=0;
            break;
          }
          if (SetMask(sq-8)&tree->all_pawns) break;
        }
        if (sq <= Max(square-24,H1)) blocked=0;
      } while(0);
      if (!blocked) b_unblocked++;
/*
 ----------------------------------------------------------
|                                                          |
|  evaluate weak pawns.  weak pawns are evaluated by the   |
|  following rules:  (1) if a pawn is defended by a pawn,  |
|  it isn't weak;  (2) if a pawn is undefended by a pawn   |
|  and advances one (or two if it hasn't moved yet) ranks  |
|  and is defended fewer times than it is attacked, it is  |
|  weak.  note that the penalty is greater if the pawn is  |
|  on an open file.  note that an isolated pawn is just    |
|  another case of a weak pawn, since it can never be      |
|  defended by a pawn.                                     |
|                                                          |
|  test the pawn where it is, and as it advances.  note    |
|  whether it can end up with more pawn defenders than     |
|  pawn attackers as it advances.                          |
|                                                          |
 ----------------------------------------------------------
*/
      do {
        weakness=0;
        if (minus8dir[square]&WhitePawns) break;
        attackers=PopCnt(b_pawn_attacks[square]&WhitePawns);
        defenders=PopCnt(w_pawn_attacks[square]&BlackPawns);
        if (defenders > attackers) break;
        attackers=PopCnt(b_pawn_attacks[square-8]&WhitePawns);
        defenders=PopCnt(w_pawn_attacks[square-8]&BlackPawns);
        if (attackers && attackers >= defenders) weakness=attackers-defenders+1;
        else if (attackers || defenders) break;
        if (!weakness) break;
/*
 ----------------------------------------------------------
|                                                          |
|  if the pawn can be defended by a pawn, and that pawn    |
|  can safely advance, then this pawn is not weak.         |
|                                                          |
 ----------------------------------------------------------
*/
        if ((temp=mask_no_pawn_attacks_b[square]&BlackPawns)) {
          if (file > FILEA) {
            const BITBOARD temp1=temp&file_mask[file-1];
            attackers=1;
            if (temp1) {
              const int defend_sq=FirstOne(temp1);
              for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
                attackers=1;
                if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
                attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
                if (attackers) break;
              }
              if (!attackers) weakness=0;
            }
            if (!weakness) break;
          }
          if (file < FILEH) {
            const BITBOARD temp1=temp&file_mask[file+1];
            if (temp1) {
              const int defend_sq=FirstOne(temp1);
              for (sq=defend_sq;sq>=((Rank(square)+1)<<3);sq-=8) {
                attackers=1;
                if (sq!=defend_sq && tree->all_pawns&SetMask(sq)) break;
                attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
                if (attackers) break;
              }
              if (!attackers) weakness=0;
            }
          }
        }

        if (weakness > 0) {
          if (weakness == 3) score+=PAWN_WEAK_P2;
          else if (weakness) score+=PAWN_WEAK_P1;
        }
      } while(0);
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[weak] file=%d,     score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate doubled pawns.  if there are other pawns on   |
|   this file, penalize this pawn.                         |
|                                                          |
 ----------------------------------------------------------
*/
      if ((pns=PopCnt(file_mask[file]&BlackPawns)) > 1) {
        score+=doubled_pawn_value[pns];
      }
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[doubled] file=%d,  score=%d\n", file,score);
      lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  test the pawn to see it if forms a "duo" which is two   |
|  pawns side-by-side.                                     |
|                                                          |
 ----------------------------------------------------------
*/
      if (mask_pawn_duo[square]&BlackPawns)
        score-=PAWN_DUO;
#ifdef DEBUGP
      if (score != lastsc)
        printf("black pawn[duo] file=%d,      score=%d\n", file,score);
      lastsc=score;
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate passed pawns.                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (!(mask_pawn_passed_b[square]&WhitePawns)) {
      score-=passed_pawn_value[(RANK8-Rank(square))];
      if (plus8dir[square]&BlackPawns)
        score+=passed_pawn_value[RANK8-Rank(square)]>>1;
      if (mask_pawn_protected_b[square]&BlackPawns)
        tree->pawn_score.sta_protected|=2;
      tree->pawn_score.passed_b|=128>>file;
#ifdef DEBUGP
      printf("black pawn[passed]            file=%d\n", file);
      if (score != lastsc)
        printf("black pawn[passed]            score=%d\n", score);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now determine if this pawn is a candidate passer,      |
|   since we now know it isn't passed.  a candidate is a   |
|   pawn on a file with no enemy pawns in front of it, and |
|   if it advances until it contacts an enemy pawn, and it |
|   is defended as many times as it is attacked when it    |
|   reaches that pawn, then all that is left is to see if  |
|   it is passed when the attacker(s) get removed.         |
|                                                          |
 ----------------------------------------------------------
*/
    else {
      if (!(file_mask[File(square)]&WhitePawns) &&
          mask_pawn_isolated[square]&BlackPawns &&
          !(b_pawn_attacks[square]&WhitePawns)) {
        attackers=1;
        defenders=0;
        for (sq=square;sq>H2;sq-=8) {
          if (SetMask(sq-8)&tree->all_pawns) break;
          attackers=PopCnt(b_pawn_attacks[sq]&WhitePawns);
          defenders=PopCnt(w_pawn_attacks[sq]&bp_moves);
          if (attackers) break;
        }
        if (attackers <= defenders) {
          if (!(mask_pawn_passed_b[sq-8]&WhitePawns)) {
            tree->pawn_score.candidates_b|=128>>file;
          }
        }
      }
      if (!(tree->pawn_score.candidates_b&128)) {
        if (file <= FILED) {
          if (BlackPawns&file_mask[file+1] && BlackPawns&file_mask[file+2]) {
            if (!(WhitePawns&file_mask[file]) &&
                !(WhitePawns&file_mask[file+2]) &&
                !(WhitePawns&file_mask[file+3]) &&
                PopCnt(WhitePawns&file_mask[file+1])<=2)
              tree->pawn_score.candidates_b|=128>>file;
          }
        }
        else {
          if (BlackPawns&file_mask[file-1] && BlackPawns&file_mask[file-2]) {
            if (!(WhitePawns&file_mask[file]) &&
                !(WhitePawns&file_mask[file-2]) &&
                  !(WhitePawns&file_mask[file-3]) &&
                PopCnt(WhitePawns&file_mask[file-1])<=2)
              tree->pawn_score.candidates_b|=128>>file;
          }
        }
      }
#ifdef DEBUGP
      if (tree->pawn_score.candidates_b&(128>>file))
        printf("black pawn[candidate]       square=%d\n", square);
#endif
    }
/*
 ----------------------------------------------------------
|                                                          |
|   evaluate "hidden" passed pawns.  simple case is a pawn |
|   chain (white) at b5, a6, with a black pawn at a7.      |
|   it appears the b-pawn is backward, with a ram at a6/a7 |
|   but this is misleading, because the pawn at a6 is      |
|   really passed when white plays b6.                     |
|                                                          |
 ----------------------------------------------------------
*/
    if (Rank(square) < RANK4 && SetMask(square-8)&WhitePawns &&
        !(mask_pawn_passed_b[square-8]&WhitePawns) &&
        ((File(square) < FILEH && SetMask(square+9)&BlackPawns &&
          !(minus8dir[square+9]&WhitePawns) &&
          (File(square) == FILEG || !(minus8dir[square+10]&WhitePawns))) ||
         (File(square) > FILEA && SetMask(square+7)&BlackPawns &&
          !(minus8dir[square+7]&WhitePawns) &&
          (File(square) == FILEB || !(minus8dir[square+6]&WhitePawns))))) {
      score-=hidden_passed_pawn_value[(RANK8-Rank(square))];
    }
#ifdef DEBUGP
    if (score != lastsc)
      printf("black pawn[hidden] file=%d,   score=%d\n", file,score);
    lastsc=score;
#endif
    staClear(square,pawns);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the penalty for isolated pawns, which is   |
|   non-linear to penalize more isolani more severely.     |
|   note that the penalty penalizes the side with the      |
|   most isolated pawns, in an exponential rate.           |
|                                                          |
 ----------------------------------------------------------
*/
  score-=isolated_pawn_value[w_isolated];
  score-=isolated_pawn_of_value[w_isolated_of];
  score+=isolated_pawn_value[b_isolated];
  score+=isolated_pawn_of_value[b_isolated_of];
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[isolated]          score=%d\n", score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now fold in the bonus for unblocked pawns.  the real   |
|   issue is keeping "lever" possibilities for each side   |
|   available, so there is some opportunity for breaking   |
|   the position open, rather than letting it become       |
|   totally blocked.                                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns>4 && TotalBlackPawns>4) {
    if (w_unblocked <= 2)
      score-=PAWNS_BLOCKED*(3-w_unblocked)*blocked_scale/100;
    if (b_unblocked <= 2)
      score+=PAWNS_BLOCKED*(3-b_unblocked)*blocked_scale/100;
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[unblocked]         score=%d\n", score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now add in the pawn ram penalty.  the idea here is     |
|   that rams hinder tactics by blocking the position,     |
|   which sidesteps the computer's strong point.           |
|                                                          |
 ----------------------------------------------------------
*/
  if (use_asymmetry && !computer_opponent) {
    if (root_wtm) {
      if (TotalWhitePawns > 5)
        score-=pawn_rams[PopCnt(WhitePawns&(BlackPawns<<8))];
    }
    else {
      if (TotalBlackPawns > 5)
        score+=pawn_rams[PopCnt(WhitePawns&(BlackPawns<<8))];
    }
  }
#ifdef DEBUGP
  if (score != lastsc)
    printf("pawn[rams]              score=%d\n",score);
  lastsc=score;
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now evaluate king safety.                              |
|                                                          |
|   the first step is to step across the board and note    |
|   which files are open/half open.  since this is common  |
|   to both kings, this is only done once since it is the  |
|   same for both players.                                 |
|                                                          |
|   at the same time we note if any of the three pawns in  |
|   front of the king have moved, and count those as well. |
|                                                          |
 ----------------------------------------------------------
*/
  left=file_mask[FILEA];
  right=file_mask[FILEH];
  for (file=0;file<3;file++) {
    if (file < 2) {
      kside_w=1;
      kside_b=1;
    }
    else {
      kside_w=kside_open_files+kside_half_open_files_b+kside_half_open_files_w+kmissw;
      kside_b=kside_open_files+kside_half_open_files_b+kside_half_open_files_w+kmissb;
    }
    if (!(right&tree->all_pawns))
      kside_open_files++;
    else {
      if (kside_w) {
        if (!(right&WhitePawns)) {
          kside_half_open_files_w++;
        }
        else if (!(WhitePawns&SetMask(H2-file))) {
          kmissw++;
          if (!(WhitePawns&SetMask(H3-file))) kmissw++;
          if (file == 1) kmissw++;
        }
      }
      if (kside_b) {
        if (!(right&BlackPawns)) {
          kside_half_open_files_b++;
        }
        else if (!(BlackPawns&SetMask(H7-file))) {
          kmissb++;
          if (!(BlackPawns&SetMask(H6-file))) kmissb++;
          if (file == 1) kmissb++;
        }
      }
    }
    right=right<<1;

    if (file < 2) {
      qside_w=1;
      qside_b=1;
    }
    else {
      qside_w=qside_open_files+qside_half_open_files_b+qside_half_open_files_w+qmissw;
      qside_b=qside_open_files+qside_half_open_files_b+qside_half_open_files_w+qmissb;
    }
    if (!(left&tree->all_pawns))
      qside_open_files++;
    else {
      if (qside_w) {
        if (!(left&WhitePawns)) {
          qside_half_open_files_w++;
        }
        else if (!(WhitePawns&SetMask(A2+file))) {
          qmissw++;
          if (!(WhitePawns&SetMask(A3+file))) qmissw++;
          if (file == 1) qmissw++;
        }
      }
      if (qside_b) {
        if (!(left&BlackPawns)) {
          qside_half_open_files_b++;
        }
        else if (!(BlackPawns&SetMask(A7+file))) {
          qmissb++;
          if (!(BlackPawns&SetMask(A6+file))) qmissb++;
          if (file == 1) qmissb++;
        }
      }
    }
    left=left>>1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now a special case, where one side has advanced two    |
|   pawns (ie pawns at f2, g3, h3, where the two advances  |
|   are not quite as dangerous as they seem.               |
|                                                          |
 ----------------------------------------------------------
*/
  if (kmissw) {
    if ((mask_F3G3&WhitePawns) == mask_F3G3) kmissw--;
    else if ((mask_G3H3&WhitePawns) == mask_G3H3) kmissw--;
  }
  if (qmissw) {
    if ((mask_A3B3&WhitePawns) == mask_A3B3) qmissw--;
    else if ((mask_B3C3&WhitePawns) == mask_B3C3) qmissw--;
  }
  if (kmissb) {
    if ((mask_F6G6&BlackPawns) == mask_F6G6) kmissb--;
    else if ((mask_G6H6&BlackPawns) == mask_G6H6) kmissb--;
  }
  if (qmissb) {
    if ((mask_A6B6&BlackPawns) == mask_A6B6) qmissb--;
    else if ((mask_B6C6&BlackPawns) == mask_B6C6) qmissb--;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now we take the number of open/half-open files, plus   |
|   the number of 'missing' pawns on the 2nd rank in front |
|   of the king, and fold them into a 'defect count.'  we  |
|   run these raw counts 'indirect' through a scaling      |
|   procedure so that two open files are far worse than    |
|   one, and so forth.                                     |
|                                                          |
 ----------------------------------------------------------
*/
  tree->pawn_score.white_defects_k=missing[kmissw]+
                                   openf[kside_open_files]+
                                   hopenf[kside_half_open_files_w]+
                                   (hopenf[kside_half_open_files_b]>>1);
  tree->pawn_score.white_defects_q=missing[qmissw]+
                                   openf[qside_open_files]+
                                   hopenf[qside_half_open_files_w]+
                                   (hopenf[qside_half_open_files_b]>>1);
  tree->pawn_score.black_defects_k=missing[kmissb]+
                                   openf[kside_open_files]+
                                   (hopenf[kside_half_open_files_w]>>1)+
                                   hopenf[kside_half_open_files_b];
  tree->pawn_score.black_defects_q=missing[qmissb]+
                                   openf[qside_open_files]+
                                   (hopenf[qside_half_open_files_w]>>1)+
                                   hopenf[qside_half_open_files_b];
#if defined(DEBUGK)
  printf("white: kmissing=%d  kopen=%d  khalf=%d\n",
         missing[kmissw],openf[kside_open_files], hopenf[kside_half_open_files_w]);
  printf("white: qmissing=%d  qopen=%d  qhalf=%d\n",
         missing[qmissw],openf[qside_open_files], hopenf[qside_half_open_files_w]);
  printf("black: kmissing=%d  kopen=%d  khalf=%d\n",
         missing[kmissb],openf[kside_open_files], hopenf[kside_half_open_files_b]);
  printf("black: qmissing=%d  qopen=%d  qhalf=%d\n",
         missing[qmissb],openf[qside_open_files], hopenf[qside_half_open_files_b]);
  printf("white, defects=%d(q)  %d(k)\n",
         tree->pawn_score.white_defects_q,tree->pawn_score.white_defects_k);
  printf("black, defects=%d(q)  %d(k)\n",
         tree->pawn_score.black_defects_q,tree->pawn_score.black_defects_k);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|   now look for the "stonewall" formation in the white    |
|   pawns and penalize the kingside safety if this type of |
|   pawn position is found.  this also catched closed type |
|   positions also.                                        |
|                                                          |
 ----------------------------------------------------------
*/
  if (use_asymmetry) {
    if (root_wtm) {
      if (PopCnt(BlackPawns&stonewall_black)==2 &&
          WhitePawns&e2_e3)
        tree->pawn_score.white_defects_k+=KING_SAFETY_STONEWALL;
    }
    else {
      if (PopCnt(WhitePawns&stonewall_white)==2 &&
          BlackPawns&e7_e6)
        tree->pawn_score.black_defects_k+=KING_SAFETY_STONEWALL;
    }
  }
#if defined(DEBUGK)
  printf("white.SW, defects=%d(q)  %d(k)\n",
         tree->pawn_score.white_defects_q,tree->pawn_score.white_defects_k);
  printf("black.SW, defects=%d(q)  %d(k)\n",
         tree->pawn_score.black_defects_q,tree->pawn_score.black_defects_k);
#endif
/*
 ----------------------------------------------------------
|                                                          |
|  evaluate outside passed pawns by analyzing the passed   |
|  pawns for both sides. we use a pre-computed table that  |
|  can determine if one side has a passed pawn that is to  |
|  the left of all other pawns, or to the right of all     |
|  other pawns.  if both seem to have an outside passed    |
|  pawn, this means one side has one on one side of the    |
|  board while the other side has a passed pawn on the     |
|  opposite side of the board.  this case is treated as    |
|  neither having an outside passer.                       |
|                                                          |
|  an outside passer is defined as a pawn closer to one    |
|  edge of the board than _all_ other pawns on the board.  |
|  and that pawn must have at least one file between it    |
|  and any other pawns on the board.  this last limitation |
|  means we have to do a little work here to remove our    |
|  own pawn from the test if (say) we have a passed pawn   |
|  on the a-file, and one of our pawns on the b-file.      |
|                                                          |
|  we repeat for candidate passed pawns as well.           |
|                                                          |
|  tree->pawn_score.outside is a bitmap with 8 bits:       |
|                                                          |
|        xxxx xxx1   (1) -> white has outside candidate    |
|        xxxx xx1x   (2) -> white has 2 outside candidates |
|        xxxx x1xx   (4) -> white has outside passer       |
|        xxxx 1xxx   (8) -> white has 2 outside passers    |
|        xxx1 xxxx  (16) -> black has outside candidate    |
|        xx1x xxxx  (32) -> black has 2 outside candidates |
|        x1xx xxxx  (64) -> black has outside passer       |
|        1xxx xxxx (128) -> black has 2 outside passers    |
|                                                          |
 ----------------------------------------------------------
*/
  wop=is_outside[tree->pawn_score.passed_w][tree->pawn_score.allb];
  bop=is_outside[tree->pawn_score.passed_b][tree->pawn_score.allw];
  if (!wop || !bop) {
    if (wop > 1) tree->pawn_score.outside|=8;
    else if (wop && TotalWhitePawns>1) tree->pawn_score.outside|=4;
    if (bop > 1) tree->pawn_score.outside|=128;
    else if (bop && TotalBlackPawns>1) tree->pawn_score.outside|=64;
  }

  wop=is_outside_c[tree->pawn_score.candidates_w][tree->pawn_score.allb];
  bop=is_outside_c[tree->pawn_score.candidates_b][tree->pawn_score.allw];
  if (!wop || !bop) {
    if (wop > 1) tree->pawn_score.outside|=2;
    else if (wop) tree->pawn_score.outside|=1;
    if (bop > 1) tree->pawn_score.outside|=32;
    else if (bop) tree->pawn_score.outside|=16;
  }

  if (!(tree->pawn_score.outside & 0xcc)) {
    if (tree->pawn_score.candidates_w && !tree->pawn_score.candidates_b)
      tree->pawn_score.outside|=1;
    if (tree->pawn_score.candidates_b && !tree->pawn_score.candidates_w)
      tree->pawn_score.outside|=16;
    if (tree->pawn_score.sta_protected&1 && !(tree->pawn_score.sta_protected&2) &&
        PopCnt8Bit((int)(tree->pawn_score.passed_b))<2)
      tree->pawn_score.outside|=4;
    if (tree->pawn_score.sta_protected&2 && !(tree->pawn_score.sta_protected&1) &&
        PopCnt8Bit((int)(tree->pawn_score.passed_w))<2)
      tree->pawn_score.outside|=64;
  }
/*
  if (tree->pawn_score.outside) {
    printf(">>>>>>>>>>>>>>>>>>>  outside=%x\n",tree->pawn_score.outside);
    printf("w_passed=%x  b_passed=%x\n",
           tree->pawn_score.passed_w,tree->pawn_score.passed_b);
    printf("w_candidates=%x  b_candidates=%x\n",
           tree->pawn_score.candidates_w,tree->pawn_score.candidates_b);
    wop=is_outside[tree->pawn_score.passed_w][tree->pawn_score.allb];
    bop=is_outside[tree->pawn_score.passed_b][tree->pawn_score.allw];
    printf("allb=%x  allw=%x\n",tree->pawn_score.allb,tree->pawn_score.allw);
    printf("passed, wop=%d  bop=%d\n",wop,bop);
    wop=is_outside_c[tree->pawn_score.candidates_w][tree->pawn_score.allb];
    bop=is_outside_c[tree->pawn_score.candidates_b][tree->pawn_score.allw];
    printf("candidates, wop=%d  bop=%d\n",wop,bop);
    printf("protected=%x\n",tree->pawn_score.protected);
  }
*/
/*
 ----------------------------------------------------------
|                                                          |
|   store the results in the pawn hash table for reuse at  |
|   a later time as needed.                                |
|                                                          |
 ----------------------------------------------------------
*/
  score=score*pawn_scale/100;
  tree->pawn_score.p_score=score;
  *ptable=tree->pawn_score;
  return(score);
}

// RedCrafty_481_f.cpp, ř. 9830-9895: EvaluateStalemate
/* last modified 04/17/02 */
/*
********************************************************************************
*                                                                              *
*   EvaluateStaleMate() is used to determine if one side has only a king and   *
*   the king has no legal moves.  this is used to correct a codition that can  *
*   confuse the "bishop + wrong rook pawn = draw" code (as well as others that *
*   are similar.  if the side with the lone king is on move and has no legal   *
*   moves, then a draw score is returned.                                      *
*                                                                              *
********************************************************************************
*/
int EvaluateStalemate(TREE *tree, int wtm) {
  int stalemate=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if white has only a king and it can't legally move,    |
|   and white is on move, then this is a stalemate.        |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (TotalWhitePieces+TotalWhitePawns == 0) {
      BITBOARD sm_possible=stalemate_sqs[WhiteKingSQ]&BlackKing;
      if (sm_possible) {
        BITBOARD moves=edge_moves[WhiteKingSQ];
        stalemate=1;
        while (moves) {
          int square=FirstOne(moves);
          if (!Attacked(tree,square,0)) {
            stalemate=0;
            break;
          }
          moves^=SetMask(square);
        }
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if black has only a king and it can't legally move,    |
|   and white is on move, then this is a stalemate.        |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    if (TotalBlackPieces+TotalBlackPawns == 0) {
      BITBOARD sm_possible=stalemate_sqs[BlackKingSQ]&WhiteKing;
      if (sm_possible) {
        BITBOARD moves=edge_moves[BlackKingSQ];
        stalemate=1;
        while (moves) {
          int square=FirstOne(moves);
          if (!Attacked(tree,square,1)) {
            stalemate=0;
            break;
          }
          moves^=SetMask(square);
        }
      }
    }
  }
  return (stalemate);
}

// RedCrafty_481_f.cpp, ř. 9897-10132: EvaluateWinner
/* last modified 10/17/01 */
/*
********************************************************************************
*                                                                              *
*   EvaluateWinner() is used to determine if one side (or both) are in a       *
*   position where winning is impossible.                                      *
*                                                                              *
*   return values:                                                             *
*        0    ->     neither side can win, this is a dead drawn position.      *
*        1    ->     white can win, black can not win.                         *
*        2    ->     white can not win, black can win.                         *
*        3    ->     both white and black can win.                             *
*                                                                              *
********************************************************************************
*/
int EvaluateWinner(TREE *tree) {
  int can_win=3;
/*
 ----------------------------------------------------------
|                                                          |
|   if lots of material is left, it's not a draw.          |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePieces > 8 || TotalBlackPieces >8) return(can_win);
  if (TotalWhitePieces==6 && WhiteBishops && TotalWhitePawns) return(can_win);
  if (TotalBlackPieces==6 && BlackBishops && TotalBlackPawns) return(can_win);
  if (TotalWhitePieces==0 && TotalBlackPieces==0) {
    if ((WhitePawns|BlackPawns)&not_rook_pawns) return(can_win);
  }
  if (!TotalBlackPieces) {
    if (!TotalBlackPawns) can_win&=1;
    if (TotalBlackPawns==1 && TotalWhitePieces) can_win&=1;
  }
  if (!TotalWhitePieces) {
    if (!TotalWhitePawns) can_win&=2;
    if (TotalWhitePawns==1 && TotalBlackPieces) can_win&=2;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if white has a pawn, then either the pawn had better   |
|   not be a rook pawn, or else white had better have the  |
|   right color bishop or any other piece, otherwise it is |
|   not winnable if the black king can get to the queening |
|   square first.                                          |
|                                                          |
|   special case:  if both sides have the right colored    |
|   bishop, it is still most likely drawn.                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns) do {
    if (WhitePawns&not_rook_pawns) continue;
    if (TotalWhitePieces>3 || (TotalWhitePieces==3 && WhiteKnights)) continue;
    if (TotalWhitePieces==0) {
      if (file_mask[FILEA]&WhitePawns &&
          file_mask[FILEH]&WhitePawns) continue;
    }
    if (!(WhitePawns&not_rook_pawns)) {
      if (WhiteBishops) {
        if (!BlackBishops) {
          if (WhiteBishops&dark_squares) {
            if (file_mask[FILEH]&WhitePawns) continue;
          }
          else if (file_mask[FILEA]&WhitePawns) continue;
        }
        else {
          if (WhiteBishops&dark_squares && !(BlackBishops&dark_squares)) {
            if (file_mask[FILEH]&WhitePawns) continue;
          }
          else if (WhiteBishops&light_squares && !(BlackBishops&light_squares)) {
            if (file_mask[FILEA]&WhitePawns) continue;
          }
        }
      }
      if (!(WhitePawns&file_mask[FILEA]) ||
          !(WhitePawns&file_mask[FILEH])) {
        if (WhitePawns&file_mask[FILEA]) {
          int bkd, wkd, pd;
          bkd=Distance(BlackKingSQ,A8);
          if (bkd <= 1) can_win&=2;
          else {
            wkd=Distance(WhiteKingSQ,A8);
            pd=Distance(LastOne(WhitePawns&file_mask[FILEA]),A8);
            if (bkd<(wkd-wtm) && bkd<=(pd-wtm)) can_win&=2;
          }
          continue;
        }
        else {
          int bkd, wkd, pd;
          bkd=Distance(BlackKingSQ,H8);
          if (bkd <= 1) can_win&=2;
          else {
            wkd=Distance(WhiteKingSQ,H8);
            pd=Distance(LastOne(BlackPawns&file_mask[FILEH]),H8);
            if (bkd<(wkd-wtm) && bkd<=(pd-wtm)) can_win&=2;
          }
          continue;
        }
      }
    }
  } while (0);
/*
 ----------------------------------------------------------
|                                                          |
|   if black has a pawn, then either the pawn had better   |
|   not be a rook pawn, or else black had better have the  |
|   right color bishop or any other piece, otherwise it is |
|   not winnable if the white king can get to the queening |
|   square first.                                          |
|                                                          |
|   special case:  if both sides have the right colored    |
|   bishop, it is still most likely drawn.                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalBlackPawns) do {
    if (BlackPawns&not_rook_pawns) continue;
    if (TotalBlackPieces>3 || (TotalBlackPieces==3 && BlackKnights)) continue;
    if (TotalBlackPieces==0) {
      if (file_mask[FILEA]&BlackPawns &&
          file_mask[FILEH]&BlackPawns) continue;
    }
    if (!(BlackPawns&not_rook_pawns)) {
      if (BlackBishops) {
        if (!WhiteBishops) {
          if (BlackBishops&dark_squares) {
            if (file_mask[FILEA]&BlackPawns) continue;
          }
          else if (file_mask[FILEH]&BlackPawns) continue;
        }
        else {
          if (BlackBishops&dark_squares && !(WhiteBishops&dark_squares)) {
            if (file_mask[FILEA]&BlackPawns) continue;
          }
          else if (BlackBishops&dark_squares && !(WhiteBishops&dark_squares)) {
            if (file_mask[FILEH]&BlackPawns) continue;
          }
        }
      }
      if (!(BlackPawns&file_mask[FILEA]) ||
          !(BlackPawns&file_mask[FILEH])) {
        if (BlackPawns&file_mask[FILEA]) {
          int bkd, wkd, pd;
          wkd=Distance(WhiteKingSQ,A1);
          if (wkd <= 1) can_win&=1;
          else {
            bkd=Distance(BlackKingSQ,A1);
            pd=Distance(FirstOne(BlackPawns&file_mask[FILEA]),A1);
            if (wkd<(bkd+wtm) && wkd<=(pd+wtm)) can_win&=1;
          }
          continue;
        }
        else {
          int bkd, wkd, pd;
          wkd=Distance(WhiteKingSQ,H1);
          if (wkd <= 1) can_win&=1;
          else {
            bkd=Distance(BlackKingSQ,H1);
            pd=Distance(FirstOne(BlackPawns&file_mask[FILEH]),H1);
            if (wkd<(bkd+wtm) && wkd<=(pd+wtm)) can_win&=1;
          }
          continue;
        }
      }
    }
  } while (0);
/*
 ----------------------------------------------------------
|                                                          |
|   if both sides have pawns, the game is not a draw for   |
|   lack of material.  also, if one side has at least a    |
|   B+N, then it's not a drawn position.                   |
|                                                          |
|   if one side has a rook, while the other side has a     |
|   minor + pawns, then the rook can't possibly win.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns && TotalBlackPawns) return(can_win);
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has two bishops, and the other side has    |
|   a single kinght, the two bishops win.                  |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns==0 && TotalWhitePieces==6 &&
      TotalBlackPieces==3) {
    if (WhiteKnights || !BlackKnights) can_win&=2;
  }
  else if (TotalBlackPawns==0 && TotalBlackPieces==6 &&
      TotalWhitePieces==3) {
    if (BlackKnights || !WhiteKnights) can_win&=1;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has a rook and piece, while the other side |
|   has a rook and pawn(s), the rook+piece can not win.    |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns==0 && TotalWhitePieces==8 &&
      TotalBlackPieces==5) can_win&=2;
  if (TotalBlackPawns==0 && TotalBlackPieces==8 &&
      TotalWhitePieces==5) can_win&=1;
/*
 ----------------------------------------------------------
|                                                          |
|   if one side is only a piece ahead and has no pawns,    |
|   and he only has one or two pieces left, then he can't  |
|   win.                                                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns==0 && TotalWhitePieces<=6 &&
      TotalWhitePieces-TotalBlackPieces==3) can_win&=2;
  if (TotalBlackPawns==0 && TotalBlackPieces<=6 &&
      TotalBlackPieces-TotalWhitePieces==3) can_win&=1;
/*
 ----------------------------------------------------------
|                                                          |
|   if one side has only a rook and the other side has one |
|   minor piece, neither side can win.                     |
|                                                          |
 ----------------------------------------------------------
*/
  if (TotalWhitePawns+TotalBlackPawns==0) {
    if (TotalWhitePieces==5 && TotalBlackPieces==3) can_win=0;
    if (TotalWhitePieces==3 && TotalBlackPieces==5) can_win=0;
  }
  return(can_win);
}

// RedCrafty_481_f.cpp, ř. 10157-10276: InputMoveICS
/* last modified 03/11/97 */
/*
********************************************************************************
*                                                                              *
*   InputMoveICS() is responsible for converting a move from the ics format    *
*   [from][to][promote] to the program's internal format.                      *
*                                                                              *
********************************************************************************
*/
int InputMoveICS(TREE *tree, char *text, int ply, int wtm, int silent,
                 int ponder_list) {
  int moves[220], *mv, *mvp, *goodmove=0;
  int piece=-1, promote;
  int ffile, frank, tfile, trank;
  int i, nleft;
  char movetext[128];
  static const char pieces[17]={' ',' ','P','p','N','n','K','k',' ',' ',
                                'B','b','R','r','Q','q','\0'};
/*
   initialize move structure in case an error is found
*/
  if (strlen(text) == 0) return(0);
  tree->position[MAXPLY]=tree->position[ply];
  strcpy(movetext,text);
  moves[0]=0;
  promote=0;
/*
   first, figure out what each character means.  the first thing to
   do is eliminate castling moves
*/
  if (!strcmp(movetext,"o-o") || !strcmp(movetext,"O-O") ||
      !strcmp(movetext,"0-0")) {
    piece=king;
    if(wtm) {
      ffile=4;
      frank=0;
      tfile=6;
      trank=0;
    }
    else {
      ffile=4;
      frank=7;
      tfile=6;
      trank=7;
    }
  }
  else
    if (!strcmp(movetext,"o-o-o") || !strcmp(movetext,"O-O-O") ||
        !strcmp(movetext,"0-0-0")) {
      piece=king;
      if(wtm) {
        ffile=4;
        frank=0;
        tfile=2;
        trank=0;
      }
      else {
        ffile=4;
        frank=7;
        tfile=2;
        trank=7;
      }
    }
  else {
/*
   the next thing to do is extract the last rank/file designators since
   the destination is required.  note that we can have either or both.
*/
    ffile=movetext[0]-'a';
    frank=movetext[1]-'1';
    tfile=movetext[2]-'a';
    trank=movetext[3]-'1';
/*
   now, continue by picking off the promotion piece if one is present.  this
   is indicated by something like q on the end of the move string.
*/
    if (movetext[4]=='=')
      promote=(strchr(pieces,movetext[5])-pieces) >> 1;
    else if ((movetext[4] != 0) && (movetext[4] != ' '))
      promote=(strchr(pieces,movetext[4])-pieces) >> 1;
  }
  if (!ponder_list) {
    mvp=GenerateCaptures(tree,MAXPLY, wtm, moves);
    mvp=GenerateNonCaptures(tree,MAXPLY, wtm, mvp);
  }
  else {
    for (i=0;i<num_ponder_moves;i++) moves[i]=ponder_moves[i];
    mvp=moves+num_ponder_moves;
  }
  for (mv=&moves[0];mv<mvp;mv++) {
    if (auto232 && Promote(*mv) && (Promote(*mv) != queen)) *mv = 0;
    if (!auto232 && Promote(*mv) != promote) *mv=0;
    if (Rank(From(*mv)) != frank) *mv=0;
    if (File(From(*mv)) != ffile) *mv=0;
    if (Rank(To(*mv)) != trank) *mv=0;
    if (File(To(*mv)) != tfile) *mv=0;
    if (!ponder_list && *mv) {
      MakeMove(tree,MAXPLY, *mv, wtm);
      if(Check(wtm)) {
        UnmakeMove(tree,MAXPLY, *mv, wtm);
        *mv=0;
      }
      else UnmakeMove(tree,MAXPLY, *mv, wtm);
    }
  }
  nleft=0;
  for (mv=&moves[0];mv<mvp;mv++) {
    if (*mv) {
      nleft++;
      goodmove=mv;
    }
  }
  if (nleft == 1) return(*goodmove);
  if (!silent) {
    hlaseni_illegal_move++;  // port: bylo Memo1, ř. 10271-10273
  }
  return(0);
}

// RedCrafty_481_f.cpp, ř. 10319-10489: Quiesce
/* last modified 11/20/01 */
/*
********************************************************************************
*                                                                              *
*   Quiesce() is the recursive routine used to implement the alpha/beta        *
*   negamax search (similar to minimax but simpler to code.)  Quiesce() is     *
*   called whenever there is no "depth" remaining so that only capture moves   *
*   are searched deeper.                                                       *
*                                                                              *
********************************************************************************
*/
int Quiesce(TREE *tree, int alpha, int beta, int wtm, int ply) {
  int o_alpha, value, delta;
  int *next_move;
  int *goodmv, *movep, moves=0, *sortv, temp;
/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (ply >= MAXPLY-1) return(beta);
  tree->nodes_searched++;
  next_time_check--;  // oprava č. 4: v originálu Crafty je, RedCrafty ho vyhodil
  tree->last[ply]=tree->last[ply-1];
  o_alpha=alpha;
/*
 ----------------------------------------------------------
|                                                          |
|   now call Evaluate() to produce the "stand-pat" score   |
|   that will be returned if no capture is acceptable.     |
|   if this score is > alpha, then we also have to save    |
|   the "path" to this node as it is the PV that leads     |
|   to this score.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  value=Evaluate(tree,ply,wtm,alpha,beta);
  if (value > alpha) {
    if (value >= beta) return(value);
    alpha=value;
    tree->pv[ply].pathl=ply-1;
    tree->pv[ply].pathh=0;
    tree->pv[ply].pathd=iteration_depth;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   generate captures and sort them based on (a) the value |
|   of the captured piece - the value of the capturing     |
|   piece if this is > 0; or, (b) the value returned by    |
|   Swap().  if the value of the captured piece won't      |
|   bring the material score back up to near alpha, that   |
|   capture is discarded as "futile."                      |
|                                                          |
 ----------------------------------------------------------
*/
  tree->last[ply]=GenerateCaptures(tree, ply, wtm, tree->last[ply-1]);
  delta=alpha-largest_positional_score-(wtm?Material:-Material);
  goodmv=tree->last[ply-1];
  sortv=tree->sort_value;
  for (movep=tree->last[ply-1];movep<tree->last[ply];movep++) {
    if (((wtm)?TotalBlackPieces:TotalWhitePieces) > 12) {
      if (p_values[Captured(*movep)+7]+p_values[Promote(*movep)+7] >= delta) {
        if (Captured(*movep) == king) return(beta);
        if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7] ||
            (p_values[Piece(*movep)+7] <= p_values[Captured(*movep)+7] &&
             delta<=0)) {
          *goodmv++=*movep;
          *sortv++=p_values[Captured(*movep)+7];
          moves++;
        }
        else {
          temp=Swap(tree,From(*movep),To(*movep),wtm);
          if (temp >= 0) {
            *sortv++=temp;
            *goodmv++=*movep;
            moves++;
          }
        }
      }
    }
    else {
      int val=((wtm)?TotalBlackPieces:TotalWhitePieces)*PAWN_VALUE;
      if (p_values[Captured(*movep)+7]+p_values[Promote(*movep)+7]>=delta ||
          val-p_values[Captured(*movep)+7]<=BISHOP_VALUE) {
        if (Captured(*movep) == king) return(beta);
        if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7] ||
            (p_values[Piece(*movep)+7] <= p_values[Captured(*movep)+7] &&
             delta<=0)) {
          *goodmv++=*movep;
          *sortv++=p_values[Captured(*movep)+7];
          moves++;
        }
        else {
          temp=Swap(tree,From(*movep),To(*movep),wtm);
          if (temp >= 0) {
            *sortv++=temp;
            *goodmv++=*movep;
            moves++;
          }
        }
      }
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   don't disdain the lowly bubble sort here.  the list of |
|   captures is always short, and experiments with other   |
|   algorithms are always slightly slower.                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (moves > 1) {
    int done;
    int *end=tree->last[ply-1]+moves-1;
    do {
      done=1;
      sortv=tree->sort_value;
      movep=tree->last[ply-1];
      for (;movep<end;movep++,sortv++)
        if (*sortv < *(sortv+1)) {
          temp=*sortv;
          *sortv=*(sortv+1);
          *(sortv+1)=temp;
          temp=*movep;
          *movep=*(movep+1);
          *(movep+1)=temp;
          done=0;
        }
    } while(!done);
  }
  next_move=tree->last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  while (moves--) {
    tree->current_move[ply]=*(next_move++);
    MakeMove(tree,ply,tree->current_move[ply],wtm);
    value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),ply+1);
    UnmakeMove(tree,ply,tree->current_move[ply],wtm);
    if (value > alpha) {
      if(value >= beta) return(value);
      alpha=value;
    }
    if (tree->stop) return(0);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   all moves have been searched.  return the search       |
|   result that was found.  if the result is not the       |
|   original alpha score, then we need to return the PV    |
|   that is associated with this score.                    |
|                                                          |
 ----------------------------------------------------------
*/
  if (alpha != o_alpha) {
    memcpy(&tree->pv[ply-1].path[ply],&tree->pv[ply].path[ply],
           (tree->pv[ply].pathl-ply+1)*sizeof(int));
    memcpy(&tree->pv[ply-1].pathh,&tree->pv[ply].pathh,3);
    tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];
  }
  return(alpha);
}

// RedCrafty_481_f.cpp, ř. 10491-10542: NextRootMove
/* last modified 10/18/99 */
/*
********************************************************************************
*                                                                              *
*   NextRootMove() is used to select the next move from the root move list.    *
*                                                                              *
********************************************************************************
*/
int NextRootMove(TREE *tree, TREE *mytree, int wtm) {
  int done, i;
/*
 ----------------------------------------------------------
|                                                          |
|   for the moves at the root of the tree, the list has    |
|   already been generated and sorted.  on entry, test     |
|   the searched_this_root_move[] array and then take the  |
|   moves in the order they appear in the move list.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (!annotate_mode && !pondering && !booking &&
      n_root_moves==1 && iteration_depth>4) {
    abort_search=1;
    return(NONE);
  }
  done=0;
  for (i=0;i<n_root_moves;i++) if (root_moves[i].status&128) done++;
  if (done==1 && (root_moves[0].status&128) && root_value==root_alpha &&
      !(root_moves[0].status&2)) return(NONE);
  for (i=0;i<n_root_moves;i++)
    if (!(root_moves[i].status&128)) {
      if (search_move) {
        if (search_move > 0) {
          if(root_moves[i].move != search_move) {
            root_moves[i].status|=128;
            continue;
          }
        }
        else {
          if(root_moves[i].move == -search_move) {
            root_moves[i].status|=128;
            continue;
          }
        }
      }
      tree->current_move[1]=root_moves[i].move;
      tree->root_move=i;
      root_moves[i].status|=128;
      return(ROOT_MOVES);
    }
  return(NONE);
}

// RedCrafty_481_f.cpp, ř. 10544-10994: Search
/* last modified 10/23/01 */
/*
********************************************************************************
*                                                                              *
*   Search() is the recursive routine used to implement the alpha/beta         *
*   negamax search (similar to minimax but simpler to code.)  Search() is      *
*   called whenever there is "depth" remaining so that all moves are subject   *
*   to searching, or when the side to move is in check, to make sure that this *
*   side isn't mated.  Search() recursively calls itself until depth is ex-    *
*   hausted, at which time it calls Quiesce() instead.                         *
*                                                                              *
********************************************************************************
*/
int Search(TREE *tree, int alpha, int beta, int wtm, int depth,
           int ply, int do_null, int lp_recapture) {
  int moves_searched=0;
  int o_alpha, value=0;
  int extensions, extended, recapture, pieces;
  int mate_threat=0;
/*
 ----------------------------------------------------------
|                                                          |
|   check to see if we have searched enough nodes that it  |
|   is time to peek at how much time has been used, or if  |
|   is time to check for operator keyboard input.  this is |
|   usually enough nodes to force a time/input check about |
|   once per second, except when the target time per move  |
|   is very small, in which case we try to check the time  |
|   at least 10 times during the search.                   |
|                                                          |
 ----------------------------------------------------------
*/
  tree->nodes_searched++;
  // oprava č. 4 (PORT-CRAFTY.md, oddíl 3): místo nodes_searched %4096, které
  // se dalo přeskočit (nodes_searched roste i v Quiesce a ve větvi pro
  // nelegální tah), vlastní čítač jako v originálu Crafty. Snižuje se tady
  // a v Quiesce, kontrola času tedy proběhne jednou za 4096 uzlů.
  if (--next_time_check <= 0) {           // moje vlastní přerušení
    next_time_check=nodes_between_time_checks;
//  if (tree->nodes_searched & 4095 == 0) {           // moje vlastní přerušení
    hlaseni_kontrol_casu++;  // port: jen pro měření (oprava č. 4)
    // port: Label2, Label5, ProcessMessages a myš vlevo nahoře (ř. 10579-10586)
    // pryč. Stop teď přichází z jiného vlákna přímo do abort_search, který je
    // atomický. Čas: steady_clock místo Now(), limit v ms místo CSpinEdit1 v s.
    if (std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now()-sta_cas).count() > casovy_limit_ms)
      abort_search=1;
  }

  if (ply >= MAXPLY-1) return(beta);
/*
 ----------------------------------------------------------
|                                                          |
|   check for draw by repetition.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (RepetitionCheck(tree,ply,wtm)) {
    value=DrawScore(wtm);
    if (value < beta) SavePV(tree,ply,value,0);
#if defined(TRACE)
    if(ply <= trace_level) printf("draw by repetition detected, ply=%d.\n",ply);
#endif
    return(value);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now call HashProbe() to see if this position has been  |
|   searched before.  if so, we may get a real score,      |
|   produce a cutoff, or get nothing more than a good move |
|   to try first.  there are four cases to handle:         |
|                                                          |
|   1. HashProbe() returned "EXACT" if this score is       |
|   greater than beta, return beta.  otherwise, return the |
|   score.  In either case, no further searching is needed |
|   from this position.  note that lookup verified that    |
|   the table position has sufficient "draft" to meet the  |
|   requirements of the current search depth remaining.    |
|                                                          |
|   2. HashProbe() returned "UPPER" which means that       |
|   when this position was searched previously, every move |
|   was "refuted" by one of its descendents.  as a result, |
|   when the search was completed, we returned alpha at    |
|   that point.  we simply return alpha here as well.      |
|                                                          |
|   3. HashProbe() returned "LOWER" which means that       |
|   when we encountered this position before, we searched  |
|   one branch (probably) which promptly refuted the move  |
|   at the previous ply.                                   |
|                                                          |
|   4. HashProbe() returned "AVOID_NULL_MOVE" which means  |
|   the hashed score/bound was no good, but it indicated   |
|   that trying a null-move in this position would be a    |
|   waste of time.                                         |
|                                                          |
 ----------------------------------------------------------
*/
  switch (HashProbe(tree,ply,depth,wtm,&alpha,&beta,&mate_threat)) {
    case EXACT:
      if(alpha < beta) SavePV(tree,ply,alpha,1);
      return(alpha);
    case LOWER:
      return(beta);
    case UPPER:
      return(alpha);
    case AVOID_NULL_MOVE:
      do_null=0;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now it's time to try a probe into the endgame table-   |
|   base files.  this is done if we notice that there are  |
|   5 or fewer pieces left on the board.  EGTB_use tells   |
|   us how many pieces to probe on.  note that this can be |
|   zero when trying to swindle the opponent, so that no   |
|   probes are done since we know it is a draw.            |
|                                                          |
 ----------------------------------------------------------
*/

    // Stanley smazal

/*
 ----------------------------------------------------------
|                                                          |
|   initialize.                                            |
|                                                          |
 ----------------------------------------------------------
*/
  tree->in_check[ply+1]=0;
  o_alpha=alpha;
  tree->last[ply]=tree->last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|  first, we try a null move to see if we can get a quick  |
|  cutoff with only a little work.  this operates as       |
|  follows.  instead of making a legal move, the side on   |
|  move 'passes' and does nothing.  the resulting position |
|  is searched to a shallower depth than normal (usually   |
|  one ply less but settable by the operator) this should  |
|  result in a cutoff or at least should set the lower     |
|  bound better since anything should be better than not   |
|  doing anything.                                         |
|                                                          |
|  this is skipped for any of the following reasons:       |
|                                                          |
|  1.  the side on move is in check.  the null move        |
|      results in an illegal position.                     |
|  2.  no more than one null move can appear in succession |
|      or else the search will degenerate into nothing.    |
|  3.  the side on move has little material left making    |
|      zugzwang positions more likely.                     |
|                                                          |
|  the null-move search is also used to detect certain     |
|  types of threats.  the original idea of using the value |
|  returned by the null-move search was reported by C.     |
|  Donninger, but was modified by Bruce Moreland (Ferret)  |
|  in the following way:  if the null-move search returns  |
|  a score that says "mated in N" then this position is a  |
|  dangerous one, because not moving gets the side to move |
|  mated.  we extend the search one ply in this case, al-  |
|  though, as always, no more than one ply of extensions   |
|  are allowed at any one level in the tree.  note also    |
|  that this "threat" condition is hashed so that later,   |
|  if the hash table says "don't try the null move because |
|  it likely will fail low, we still know that this is a   |
|  threat position and that it should be extended.         |
|                                                          |
 ----------------------------------------------------------
*/
  pieces=(wtm) ? TotalWhitePieces : TotalBlackPieces;
  if (do_null && !tree->in_check[ply] && pieces && (pieces>5 || depth<421)) {
    BITBOARD save_hash_key;
    int null_depth;
    tree->current_move[ply]=0;
    tree->phase[ply]=NULL_MOVE;
   // Stanley smazal TRACE
    tree->position[ply+1]=tree->position[ply];
    Rule50Moves(ply+1)++;
    save_hash_key=HashKey;
    if (EnPassant(ply)) {
      HashEP(EnPassant(ply+1),HashKey);
      EnPassant(ply+1)=0;
    }
    null_depth=(depth>6*INCPLY && pieces>8) ? null_max : null_min;
    if (null_depth) {
      if (depth-null_depth >= INCPLY)
        value=-Search(tree,-beta,1-beta,ChangeSide(wtm),
                      depth-null_depth,ply+1,NO_NULL,0);
      else
        value=-Quiesce(tree,-beta,1-beta,ChangeSide(wtm),ply+1);
      HashKey=save_hash_key;
      if (abort_search || tree->stop) return(0);
      if (value >= beta) {
        HashStore(tree,ply,depth,wtm,LOWER,value,mate_threat);
        return(value);
      }
      if (value == -MATE+ply+2) mate_threat=1;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   if there is no best move from the hash table, and this |
|   is a PV node, then we need a good move to search       |
|   first.  while killers and history moves are good, they |
|   are not "good enough".  the simplest action is to try  |
|   a shallow search (depth-2) to get a move.  note that   |
|   when we call Search() with depth-2, it, too, will      |
|   not have a hash move, and will therefore recursively   |
|   continue this process, hence the name "internal        |
|   iterative deepening."                                  |
|                                                          |
 ----------------------------------------------------------
*/
  tree->next_status[ply].phase=HASH_MOVE;
  if (tree->hash_move[ply]==0 && do_null && depth>=3*INCPLY) do {
    if (ply & 1) {
      if (alpha!=root_alpha || beta!=root_beta) break;
    }
    else {
      if (alpha!=-root_beta || beta!=-root_alpha) break;
    }
    tree->current_move[ply]=0;
    if (depth-2*INCPLY >= INCPLY)
      value=Search(tree,alpha,beta,wtm,depth-2*INCPLY,ply,DO_NULL,0);
    else
      value=Quiesce(tree,alpha,beta,wtm,ply);
    if (abort_search || tree->stop) return(0);
    if (value <= alpha) {
      if (depth-2*INCPLY >= INCPLY)
        value=Search(tree,-MATE,beta,wtm,depth-2*INCPLY,ply,DO_NULL,0);
      else
        value=Quiesce(tree,-MATE,beta,wtm,ply);
      if (abort_search || tree->stop) return(0);
      if (value < beta) {
        if ((int) tree->pv[ply-1].pathl >= ply)
          tree->hash_move[ply]=tree->pv[ply-1].path[ply];
      }
      else tree->hash_move[ply]=tree->current_move[ply];
    }
    else if (value < beta) {
      if ((int) tree->pv[ply-1].pathl >= ply)
        tree->hash_move[ply]=tree->pv[ply-1].path[ply];
    }
    else tree->hash_move[ply]=tree->current_move[ply];
    tree->last[ply]=tree->last[ply-1];
    tree->next_status[ply].phase=HASH_MOVE;
  } while(0);
/*
 ----------------------------------------------------------
|                                                          |
|   now iterate through the move list and search the       |
|   resulting positions.  note that Search() culls any     |
|   move that is not legal by using Check().  the special  |
|   case is that we must find one legal move to search to  |
|   confirm that it's not a mate or draw.                  |
|                                                          |
 ----------------------------------------------------------
*/
  while ((tree->phase[ply]=(tree->in_check[ply]) ?
         NextEvasion(tree,ply,wtm) : NextMove(tree,ply,wtm))) {
   // Stanley smazal TRACE
/*
 ----------------------------------------------------------
|                                                          |
|   now make the move and search the resulting position.   |
|   if we are in check, the current move must be legal     |
|   since NextEvasion ensures this, otherwise we have to   |
|   make sure the side-on-move is not in check after the   |
|   move to weed out illegal moves and save time.          |
|                                                          |
 ----------------------------------------------------------
*/
    MakeMove(tree,ply,tree->current_move[ply],wtm);
    if (tree->in_check[ply] || !Check(wtm)) {
/*
 ----------------------------------------------------------
|                                                          |
|   if the move to be made checks the opponent, then we    |
|   need to remember that he's in check and also extend    |
|   the depth by one ply for him to get out.               |
|                                                          |
 ----------------------------------------------------------
*/
      extended=0;
      if (Check(ChangeSide(wtm))) {
        tree->in_check[ply+1]=1;
        tree->check_extensions_done++;
        extended+=incheck_depth;
      }
      else tree->in_check[ply+1]=0;
/*
 ----------------------------------------------------------
|                                                          |
|   if the null move found that the side on move gets      |
|   mated by not moving, then there must be some strong    |
|   threat at this position.  extend the search to make    |
|   sure it is analyzed carefully.                         |
|                                                          |
 ----------------------------------------------------------
*/
      if (mate_threat) {
        extended+=mate_depth;
        tree->mate_extensions_done++;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if two successive moves are capture / re-capture so    |
|   that the material score is restored, extend the search |
|   by one ply on the re-capture since it is pretty much   |
|   forced and easy to analyze.                            |
|                                                          |
 ----------------------------------------------------------
*/
      recapture=0;
      if (!extended && Captured(tree->current_move[ply-1]) &&
          To(tree->current_move[ply-1]) == To(tree->current_move[ply]) &&
          (p_values[Captured(tree->current_move[ply-1])+7] ==
           p_values[Captured(tree->current_move[ply])+7] ||
           Promote(tree->current_move[ply-1])) &&
          !lp_recapture) {
        tree->recapture_extensions_done++;
        extended+=recap_depth;
        recapture=1;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if we push a passed pawn, we need to look deeper to    |
|   see if it is a legitimate threat.                      |
|                                                          |
 ----------------------------------------------------------
*/
      if (Piece(tree->current_move[ply])==pawn &&
        push_extensions[To(tree->current_move[ply])]) {
        tree->passed_pawn_extensions_done++;
        extended+=pushpp_depth;
      }
/*
 ----------------------------------------------------------
|                                                          |
|   if there's only one legal move, extend the search one  |
|   additional ply since this node is very easy to search. |
|                                                          |
 ----------------------------------------------------------
*/
      if (!moves_searched) {
        if (tree->in_check[ply] && tree->last[ply]-tree->last[ply-1] == 1) {
          tree->one_reply_extensions_done++;
          extended+=onerep_depth;
        }
/*
 ----------------------------------------------------------
|                                                          |
|   now it is time to call Search()/Quiesce to find out if |
|   this move is reasonable or not.                        |
|                                                          |
 ----------------------------------------------------------
*/
        if (extended) {
          if (extended > INCPLY) extended=INCPLY;
          if (ply > 2*iteration_depth) extended>>=1;
        }
        extensions=extended-INCPLY;
        if (depth+extensions >= INCPLY)
          value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL,recapture);
        else
          value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),ply+1);
        if (abort_search || tree->stop) {
          UnmakeMove(tree,ply,tree->current_move[ply],wtm);
          return(0);
        }
      }
      else {
        if (extended) {
          if (extended > INCPLY) extended=INCPLY;
          if (ply > 2*iteration_depth) extended>>=1;
        }
        extensions=extended-INCPLY;
        if (depth+extensions >= INCPLY)
          value=-Search(tree,-alpha-1,-alpha,ChangeSide(wtm),
                        depth+extensions,ply+1,DO_NULL,recapture);
        else
          value=-Quiesce(tree,-alpha-1,-alpha,ChangeSide(wtm),ply+1);
        if (abort_search || tree->stop) {
          UnmakeMove(tree,ply,tree->current_move[ply],wtm);
          return(0);
        }
        if (value>alpha && value<beta) {
          if (depth+extensions >= INCPLY)
            value=-Search(tree,-beta,-alpha,ChangeSide(wtm),
                          depth+extensions,ply+1,DO_NULL,recapture);
          else
            value=-Quiesce(tree,-beta,-alpha,ChangeSide(wtm),ply+1);
          if (abort_search || tree->stop) {
            UnmakeMove(tree,ply,tree->current_move[ply],wtm);
            return(0);
          }
        }
      }
      if (value > alpha) {
        if(value >= beta) {
          History(tree,ply,depth,wtm,tree->current_move[ply]);
          UnmakeMove(tree,ply,tree->current_move[ply],wtm);
          HashStore(tree,ply,depth,wtm,LOWER,value,mate_threat);
          tree->fail_high++;
          if (!moves_searched) tree->fail_high_first++;
          return(value);
        }
        alpha=value;
      }
      moves_searched++;
    } else tree->nodes_searched++;
    UnmakeMove(tree,ply,tree->current_move[ply],wtm);

  // Stanley smazal SMP

  }
/*
 ----------------------------------------------------------
|                                                          |
|   all moves have been searched.  if none were legal,     |
|   return either MATE or DRAW depending on whether the    |
|   side to move is in check or not.                       |
|                                                          |
 ----------------------------------------------------------
*/
  if (moves_searched == 0) {
    value=(Check(wtm)) ? -(MATE-ply) : DrawScore(wtm);
    if (value>=alpha && value<beta) {
      SavePV(tree,ply,value,0);
#if defined(TRACE)
      if (ply <= trace_level) printf("Search() no moves!  ply=%d\n",ply);
#endif
    }
    return(value);
  }
  else {
    if (alpha != o_alpha) {
      memcpy(&tree->pv[ply-1].path[ply],&tree->pv[ply].path[ply],(tree->pv[ply].pathl-ply+1)*sizeof(int));
      memcpy(&tree->pv[ply-1].pathh,&tree->pv[ply].pathh,3);
      tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];
      History(tree,ply,depth,wtm,tree->pv[ply].path[ply]);
    }
    HashStore(tree,ply,depth,wtm,(alpha==o_alpha)?UPPER:EXACT,alpha,mate_threat);
    return(alpha);
  }
}

// RedCrafty_481_f.cpp, ř. 10996-11059: SearchOutput
/* modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   SearchOutput() is used to print the principal variation whenever it        *
*   changes.  one additional feature is that SearchOutput() will try to do     *
*   something about variations truncated by the transposition table.  if the   *
*   variation was cut short by a transposition table hit, then we can make the *
*   last move, add it to the end of the variation and extend the depth of the  *
*   variation to cover it.                                                     *
*                                                                              *
********************************************************************************
*/
void SearchOutput(TREE *tree, int value, int bound) {
  int wtm;
  int i;
  ROOT_MOVE temp_rm;
/*
 ----------------------------------------------------------
|                                                          |
|   first, move the best move to the top of the ply-1 move |
|   list if it's not already there, so that it will be the |
|   first move tried in the next iteration.                |
|                                                          |
 ----------------------------------------------------------
*/
  root_print_ok=root_print_ok || tree->nodes_searched>noise_level ||
                abs(value)>MATE-300;
  wtm=root_wtm;
  if (!abort_search) {
    whisper_depth=iteration_depth;
    for (i=0;i<n_root_moves;i++)
      if (tree->current_move[1] == root_moves[i].move) break;
    if (i && i<n_root_moves) {
      temp_rm=root_moves[i];
      for (;i>0;i--) root_moves[i]=root_moves[i-1];
      root_moves[0]=temp_rm;
      easy_move=0;
    }
/*
 ----------------------------------------------------------
|                                                          |
|   if this is not a fail-high move, then output the PV    |
|   by walking down the path being backed up.              |
|                                                          |
 ----------------------------------------------------------
*/
    if(value < bound) {
      UnmakeMove(tree,1,tree->pv[1].path[1],root_wtm);
      DisplayPV(tree,6, wtm, 0, value, &tree->pv[1]);   // Stan: param time=0
      MakeMove(tree,1,tree->pv[1].path[1],root_wtm);
    }
    else {
      if (tree->current_move[1] != tree->pv[1].path[1]) {
        tree->pv[1].path[1]=tree->current_move[1];
        tree->pv[1].pathl=1;
        tree->pv[1].pathh=0;
        tree->pv[1].pathd=iteration_depth;
      }
    }
    tree->pv[0]=tree->pv[1];
    local[0]->pv[0]=tree->pv[1];
  }
}

// RedCrafty_481_f.cpp, ř. 11061-11111: History
/* last modified 05/03/99 */
/*
********************************************************************************
*                                                                              *
*   History() is used to maintain the history database.  this is a set of      *
*   counts, indexed by the 12-bit value [to,from], that contains the relative  *
*   effectiveness of this move as a refutation.  this effectiveness is simply  *
*   an arbitrary value that varies significantly and inversely to the depth of *
*   the sub-tree refuted by each move.                                         *
*                                                                              *
*   these routines also maintain the killer move lists as well, since they are *
*   similar in nature.                                                         *
*                                                                              *
********************************************************************************
*/
void History(TREE *tree, int ply, int depth, int wtm, int move) {
  int index;
/*
 ----------------------------------------------------------
|                                                          |
|   if the best move so far is a capture or a promotion,   |
|   return, since we try good captures and promotions      |
|   before searching history heuristic moves anyway.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (CaptureOrPromote(move)) return;
/*
 ----------------------------------------------------------
|                                                          |
|   otherwise, use the [to,from] as an index and increment |
|   the appropriate history table/entry.                   |
|                                                          |
 ----------------------------------------------------------
*/
  index=move & 4095;
  if (wtm) history_w[index]+=depth*depth;
  else history_b[index]+=depth*depth;
/*
 ----------------------------------------------------------
|                                                          |
|   now, add the same move to the current killer moves if  |
|   it is not already there.                               |
|                                                          |
 ----------------------------------------------------------
*/
  if (tree->killers[ply].move1 != move) {
    tree->killers[ply].move2=tree->killers[ply].move1;
    tree->killers[ply].move1=move;
  }
}

// RedCrafty_481_f.cpp, ř. 11143-11171: HasOpposition
/*
********************************************************************************
*                                                                              *
*   HasOpposition() is used to determine if one king stands in "opposition"    *
*   to the other.  if the kings are opposed on the same file or else are       *
*   opposed on the same diagonal, then the side not-to-move has the opposition *
*   and the side-to-move must give way.                                        *
*                                                                              *
********************************************************************************
*/
int HasOpposition(int on_move, int white_king, int black_king) {
  int file_distance, rank_distance;
  file_distance=FileDistance(white_king,black_king);
  rank_distance=RankDistance(white_king,black_king);
  if (rank_distance < 2) return(1);
  if (on_move) {
    if (rank_distance&1) {
      rank_distance--;
      if (file_distance&1) file_distance--;
    }
    else if (file_distance&1) {
      file_distance--;
      if (rank_distance&1) rank_distance--;
    }
  }
  if (!(file_distance&1) && !(rank_distance&1)) return(1);
  if (!(file_distance&1) && !(rank_distance&1)) return(1);
  return(0);
}

// RedCrafty_481_f.cpp, ř. 11173-11296: Swap
/* last modified 04/04/00 */
/*
********************************************************************************
*                                                                              *
*   Swap() is used to analyze capture moves to see whether or not they appear  *
*   to be profitable.  the basic algorithm is extremely fast since it uses the *
*   bitmaps to determine which squares are attacking the [target] square.      *
*                                                                              *
*   the algorithm is quite simple.  using the attack bitmaps, we enumerate all *
*   the pieces that are attacking [target] for either side.  then we simply    *
*   use the lowest piece (value) for the correct side to capture on [target].  *
*   we continually "flip" sides taking the lowest piece each time.             *
*                                                                              *
*   as a piece is used, if it is a sliding piece (pawn, bishop, rook or queen) *
*   we "peek" behind it to see if it is attacked by a sliding piece in the     *
*   direction away from the piece being captured.  if so, and that sliding     *
*   piece moves in this direction, then it is added to the list of attackers   *
*   since its attack has been "uncovered" by moving the capturing piece.       *
*                                                                              *
********************************************************************************
*/
int Swap(TREE *tree, int source, int target, int wtm) {
  BITBOARD attacks;
  int attacked_piece;
  int square, direction;
  int color, nc=1;
  int swap_list[32];
/*
 ----------------------------------------------------------
|                                                          |
|   determine which squares attack <target> for each side. |
|   initialize by placing the piece on <target> first in   |
|   the list as it is being captured to start things off.  |
|                                                          |
 ----------------------------------------------------------
*/
  attacks=AttacksTo(tree,target);
  attacked_piece=p_values[PcOnSq(target)+7];
/*
 ----------------------------------------------------------
|                                                          |
|   the first piece to capture on <target> is the piece    |
|   standing on <source>.                                  |
|                                                          |
 ----------------------------------------------------------
*/
  color=ChangeSide(wtm);
  swap_list[0]=attacked_piece;
  attacked_piece=p_values[PcOnSq(source)+7];
  staClear(source,attacks);
  direction=directions[target][source];
  if (direction) attacks=SwapXray(tree,attacks,source,direction);
/*
 ----------------------------------------------------------
|                                                          |
|   now pick out the least valuable piece for the correct  |
|   side that is bearing on <target>.  as we find one, we  |
|   call SwapXray() to add the piece behind this piece     |
|   that is indirectly bearing on <target> (if any).       |
|                                                          |
 ----------------------------------------------------------
*/
  while (attacks) {
    if (color) {
      if (WhitePawns & attacks)
        square=FirstOne(WhitePawns & attacks);
      else if (WhiteKnights & attacks)
        square=FirstOne(WhiteKnights & attacks);
      else if (WhiteBishops & attacks)
        square=FirstOne(WhiteBishops & attacks);
      else if (WhiteRooks & attacks)
        square=FirstOne(WhiteRooks & attacks);
      else if (WhiteQueens & attacks)
        square=FirstOne(WhiteQueens & attacks);
      else if (WhiteKing & attacks)
        square=WhiteKingSQ;
      else break;
    }
    else {
      if (BlackPawns & attacks)
        square=FirstOne(BlackPawns & attacks);
      else if (BlackKnights & attacks)
        square=FirstOne(BlackKnights & attacks);
      else if (BlackBishops & attacks)
        square=FirstOne(BlackBishops & attacks);
      else if (BlackRooks & attacks)
        square=FirstOne(BlackRooks & attacks);
      else if (BlackQueens & attacks)
        square=FirstOne(BlackQueens & attacks);
      else if (BlackKing & attacks)
        square=BlackKingSQ;
      else break;
    }
/*
 ------------------------------------------------
|                                                |
|  located the least valuable piece bearing on   |
|  <target>.  remove it from the list and then   |
|  find out if a sliding piece behind it attacks |
|  through this piece.                           |
|                                                |
 ------------------------------------------------
*/
    swap_list[nc]=-swap_list[nc-1]+attacked_piece;
    attacked_piece=p_values[PcOnSq(square)+7];
    staClear(square,attacks);
    direction=directions[target][square];
    if (direction) attacks=SwapXray(tree,attacks,square,direction);
    nc++;
    color=ChangeSide(color);
  }
/*
 ----------------------------------------------------------
|                                                          |
|   starting at the end of the sequence of values, use a   |
|   "minimax" like procedure to decide where the captures  |
|   will stop.                                             |
|                                                          |
 ----------------------------------------------------------
*/
  while(--nc)
    if(swap_list[nc] > -swap_list[nc-1]) swap_list[nc-1]=-swap_list[nc];
  return (swap_list[0]);
}

// RedCrafty_481_f.cpp, ř. 11298-11362: RepetitionCheck
/* last modified 03/09/00 */
/*
********************************************************************************
*                                                                              *
*   RepetitionCheck() is used to detect a draw by repetition.  it saves the    *
*   current position in the repetition list each time it is called.  the list  *
*   contains all positions encountered since the last irreversible move        *
*   (capture or pawn push).                                                    *
*                                                                              *
*   RepetitionCheck() then scans the list to determine if this position has    *
*   occurred before.  if so, the position will be treated as a draw by         *
*   Search().   note that for a repetition to happen in the first two plies    *
*   of the tree, the position has to be repeated three times, while for plies  *
*   beyond two, two repetitions trigger a draw score.                          *
*                                                                              *
*   RepetitionCheck() also handles 50-move draws.  the position[] structure    *
*   countains the count of moves since the last capture or pawn push.  when    *
*   this value reaches 100 (plies, which is 50 moves) the score is set to      *
*   DRAW.                                                                      *
*                                                                              *
********************************************************************************
*/
int RepetitionCheck(TREE *tree, int ply, int wtm) {
  BITBOARD *replist, *thispos, *lastpos;
/*
 ----------------------------------------------------------
|                                                          |
|   if the 50-move rule has been reached, then adjust the  |
|   score to reflect the impending draw.                   |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(ply) > 99) return(1);
/*
 ----------------------------------------------------------
|                                                          |
|   insert the board into the next slot in the repetition  |
|   list.  then scan the list.  we look for the case where |
|   the position has been seen one time before, unless we  |
|   are at ply 1 or 2, where we must have seen the same    |
|   position twice prior to this.                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    thispos=tree->rephead_w+((ply-2)>>1);
    lastpos=tree->replist_w;
  }
  else {
    thispos=tree->rephead_b+((ply-2)>>1);
    lastpos=tree->replist_b;
  }
  *thispos=HashKey;
  if (ply > 3) {
    for (replist=thispos-1;replist>=lastpos;replist--)
      if(HashKey == *replist) return(1);
  }
  else {
    int count=0;
    for (replist=thispos-1;replist>=lastpos;replist--)
      if(HashKey == *replist) count++;
    if (count > 1) return(1);
  }
  return(0);
}

// RedCrafty_481_f.cpp, ř. 11364-11456: HashStore
/* last modified 09/14/99 */
/*
********************************************************************************
*                                                                              *
*   HashStore() is used to store entries into the transposition table so that  *
*   this sub-tree won't have to be searched again if the same position is      *
*   reached.  a transposition/refutation table position contains the following *
*   data packed into 128 bits, with each item taking the number of bits given  *
*   in the table below.                                                        *
*                                                                              *
*     bits     name  SL  description                                           *
*       3       age  61  search id to identify old trans/ref entried.          *
*       2      type  59  0->value is worthless; 1-> value represents a fail-   *
*                        low bound; 2-> value represents a fail-high bound;    *
*                        3-> value is an exact score.                          *
*       1    threat  58  threat extension flag, 1 -> extend this position.     *
*       5    unused  53  unused at present time.                               *
*      21      move  32  best move from the current position, according to the *
*                        search at the time this position was stored.          *
*      15     draft  17  the depth of the search below this position, which is *
*                        used to see if we can use this entry at the current   *
*                        position.  note that this is in units of 1/60th of a  *
*                        ply.                                                  *
*      17     value   0  unsigned integer value of this position + 65536.      *
*                        this might be a good score or search bound.           *
*                                                                              *
*      64       key   0  64 bit hash signature, used to verify that this entry *
*                        goes with the current board position.                 *
*                                                                              *
********************************************************************************
*/
void HashStore(TREE *tree, int ply, int depth, int wtm, int type,
               int value, int threat) {
  BITBOARD word1, word2;
  HASH_ENTRY *htablea, *htableb;
  int draft, age;
  unsigned int word1l, word1r;
/*
 ----------------------------------------------------------
|                                                          |
|   "fill in the blank" and build a table entry from       |
|   current search information.                            |
|                                                          |
 ----------------------------------------------------------
*/
  if (port_bez_hashe) return;  // port: přepínač pro test, viz crafty_search.h
  word1l=((((transposition_id<<2)+type)<<1)+threat)<<26;
  if (type == EXACT) {
    if (value > MATE-300) value=value+ply-1;
    else if (value < -MATE+300) value=value-ply+1;
    if ((int) tree->pv[ply].pathl >= ply)
      word1l|=tree->pv[ply].path[ply];
  }
  else if (type == LOWER) {
    word1l|=tree->current_move[ply];
    value=Min(value,MATE-300);
  }
  else {
    value=Max(value,-MATE+300);
  }
  word1r=(depth<<17)+value+65536;
  word1=word1r+((BITBOARD)word1l<<32);
  word2=(wtm) ? HashKey : ~HashKey;
/*
 ----------------------------------------------------------
|                                                          |
|   if the draft of this entry is greater than the draft   |
|   of the entry in the "depth-priority" table, or if the  |
|   entry in the depth-priority table is from an old       |
|   search, move that entry to the always-store table and  |
|   then replace the depth-priority table entry by the new |
|   hash result.                                           |
|                                                          |
 ----------------------------------------------------------
*/
  htablea=trans_ref_a+((int) word2&hash_maska);
  draft=(int) htablea->word1>>17 & 077777;
  age=htablea->word1>>61;
  age=age && (age!=transposition_id);
  if (age || (depth >= draft)) {
    if ((word1^word2) != htablea->word2) {
      htableb=trans_ref_b+((int) (htablea->word2)&hash_maskb);
      htableb->word1=htablea->word1;
      htableb->word2=htablea->word2;
    }
    htablea->word1=word1;
    htablea->word2=word1^word2;
  }
  else {
    htableb=trans_ref_b+((int) word2&hash_maskb);
    htableb->word1=word1;
    htableb->word2=word1^word2;
  }
}

// RedCrafty_481_f.cpp, ř. 11458-11600: NextEvasion
/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   NextEvasion() is used to select the next move from the current move list   *
*   when the king is in check.  it tries the following things to get out of    *
*   check:                                                                     *
*                                                                              *
*     1.  if one piece is attacking the king (aren't bitboard attack vectors   *
*         wonderful?) try to capture the checking piece first.  we use the     *
*         normal capture logic that tries winning or even captures first, and  *
*         postpones losing captures until other safe moves have been tried.    *
*                                                                              *
*     2.  try moving the king to a safe square (one that is not already under  *
*         attack, otherwise this would do nothing...)                          *
*                                                                              *
*     3.  If more than one piece is attacking the king, we can give up as we   *
*         can't do anything but move the king, which we've already tried.      *
*         therefore, assuming one attacker, if it's a knight, again we are     *
*         done as we can't interpose anything.  if it's a bishop, rook, or     *
*         queen, try interposing.  we simply have to find the attacker (easy)  *
*         the attackee (the king's square, also easy) and use the precomputed  *
*         mask to produce a bit vector of the squares between these two        *
*         squares.  then pass this to GenerateCheckEvasions() as targets       *
*         and we have all interposing moves.  of course, we still try them     *
*         in "sane" order of safe followed by any.                             *
*                                                                              *
********************************************************************************
*/
int NextEvasion(TREE *tree, int ply, int wtm) {
  int *movep, *sortv;

  switch (tree->next_status[ply].phase) {
/*
 ----------------------------------------------------------
|                                                          |
|   first generate all legal moves by using the special    |
|   GenerateCheckEvasions() function, so we can determine  |
|   if this is a one-legal-reply-to-check position.        |
|                                                          |
 ----------------------------------------------------------
*/
  case HASH_MOVE:
    tree->last[ply]=GenerateCheckEvasions(tree, ply, wtm, tree->last[ply-1]);
/*
 ----------------------------------------------------------
|                                                          |
|   now try the transposition table move (which might be   |
|   the principal variation move as we first move down the |
|   tree).                                                 |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->hash_move[ply]) {
      tree->next_status[ply].phase=SORT_ALL_MOVES;
      tree->current_move[ply]=tree->hash_move[ply];
      if (ValidMove(tree,ply,wtm,tree->current_move[ply])) return(HASH_MOVE);
      else Print(128,"bad move from hash table, ply=%d\n",ply);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now sort the moves based on the expected gain or loss. |
|   this is deferred until now to see if the hash move is  |
|   good enough to produce a cutoff and avoid this work.   |
|                                                          |
 ----------------------------------------------------------
*/
  case SORT_ALL_MOVES:
    tree->next_status[ply].phase=REMAINING_MOVES;
    if (tree->hash_move[ply]) {
      for (movep=tree->last[ply-1],sortv=tree->sort_value;
           movep<tree->last[ply];movep++,sortv++)
        if (*movep == tree->hash_move[ply]) {
          *sortv=-999999;
          *movep=0;
        }
        else {
          if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7])
            *sortv=p_values[Captured(*movep)+7]-p_values[Piece(*movep)+7];
          else *sortv=Swap(tree,From(*movep),To(*movep),wtm);
        }
    }
    else {
      for (movep=tree->last[ply-1],sortv=tree->sort_value;
           movep<tree->last[ply];movep++,sortv++)
        if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7])
          *sortv=p_values[Captured(*movep)+7]-p_values[Piece(*movep)+7];
        else *sortv=Swap(tree,From(*movep),To(*movep),wtm);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   this is a simple insertion sort algorithm.  it seems   |
|   be no faster than a normal bubble sort, but using this |
|   eliminated a lot of explaining about "why?". :)        |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->last[ply] > tree->last[ply-1]+1) {
      int temp1, temp2, *tmovep, *tsortv;
      int *end;
      sortv=tree->sort_value+1;
      movep=tree->last[ply-1]+1;
      end=tree->last[ply];
      for (;movep<end;movep++,sortv++) {
        temp1=*movep;
        temp2=*sortv;
        tmovep=movep-1;
        tsortv=sortv-1;
        while (tmovep>=tree->last[ply-1] && *tsortv<temp2) {
          *(tsortv+1)=*tsortv;
          *(tmovep+1)=*tmovep;
          tmovep--;
          tsortv--;
        }
        *(tmovep+1)=temp1;
        *(tsortv+1)=temp2;
      }
    }
    tree->next_status[ply].last=tree->last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   now try the rest of the set of moves.                  |
|                                                          |
 ----------------------------------------------------------
*/
  case REMAINING_MOVES:
    for (;tree->next_status[ply].last<tree->last[ply];
           tree->next_status[ply].last++)
      if ((*tree->next_status[ply].last)) {
        tree->current_move[ply]=*tree->next_status[ply].last++;
        return(REMAINING_MOVES);
      }
    return(NONE);

  default:
    tichy_printf("oops!  next_status.phase is bad! [evasion %d]\n",  // port: bylo printf
           tree->next_status[ply].phase);
    return(NONE);
  }
}

// RedCrafty_481_f.cpp, ř. 11602-11852: NextMove
/* last modified 10/20/99 */
/*
********************************************************************************
*                                                                              *
*   NextMove() is used to select the next move from the current move list.     *
*                                                                              *
********************************************************************************
*/
int NextMove(TREE *tree, int ply, int wtm) {
  int *bestp, *movep, *sortv;
  int history_value, bestval, index;

  switch (tree->next_status[ply].phase) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, try the transposition table move (which will be |
|   the principal variation move as we first move down the |
|   tree).                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  case HASH_MOVE:
    tree->next_status[ply].phase=GENERATE_CAPTURE_MOVES;
    if (tree->hash_move[ply]) {
      tree->current_move[ply]=tree->hash_move[ply];
      if (ValidMove(tree,ply,wtm,tree->current_move[ply])) return(HASH_MOVE);
      else Print(128,"bad move from hash table, ply=%d\n",ply);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   generate captures and sort them based on (a) the value |
|   of the captured piece - the value of the capturing     |
|   piece if this is > 0; or, (b) the value returned by    |
|   Swap().                                                |
|                                                          |
 ----------------------------------------------------------
*/
  case GENERATE_CAPTURE_MOVES:
    tree->next_status[ply].phase=CAPTURE_MOVES;
    tree->last[ply]=GenerateCaptures(tree, ply, wtm, tree->last[ply-1]);
    tree->next_status[ply].remaining=0;
    if (tree->hash_move[ply]) {
      for (movep=tree->last[ply-1],sortv=tree->sort_value;
           movep<tree->last[ply];movep++,sortv++)
        if (*movep == tree->hash_move[ply]) {
          *sortv=-999999;
          *movep=0;
          tree->hash_move[ply]=0;
        }
        else {
          if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7]) {
            *sortv=p_values[Captured(*movep)+7]-p_values[Piece(*movep)+7];
            tree->next_status[ply].remaining++;
          }
          else {
            *sortv=Swap(tree,From(*movep),To(*movep),wtm);
            if (*sortv >= 0)  tree->next_status[ply].remaining++;
          }
        }
    }
    else {
      for (movep=tree->last[ply-1],sortv=tree->sort_value;
           movep<tree->last[ply];movep++,sortv++)
        if (p_values[Piece(*movep)+7] < p_values[Captured(*movep)+7]) {
          *sortv=p_values[Captured(*movep)+7]-p_values[Piece(*movep)+7];
          tree->next_status[ply].remaining++;
        }
        else {
          *sortv=Swap(tree,From(*movep),To(*movep),wtm);
          if (*sortv >= 0)  tree->next_status[ply].remaining++;
        }
    }
/*
 ----------------------------------------------------------
|                                                          |
|   this is a simple insertion sort algorithm.  it seems   |
|   be no faster than a normal bubble sort, but using this |
|   eliminated a lot of explaining about "why?". :)        |
|                                                          |
 ----------------------------------------------------------
*/
    if (tree->last[ply] > tree->last[ply-1]+1) {
      int temp1, temp2, *tmovep, *tsortv;
      int *end;
      sortv=tree->sort_value+1;
      movep=tree->last[ply-1]+1;
      end=tree->last[ply];
      for (;movep<end;movep++,sortv++) {
        temp1=*movep;
        temp2=*sortv;
        tmovep=movep-1;
        tsortv=sortv-1;
        while (tmovep>=tree->last[ply-1] && *tsortv<temp2) {
          *(tsortv+1)=*tsortv;
          *(tmovep+1)=*tmovep;
          tmovep--;
          tsortv--;
        }
        *(tmovep+1)=temp1;
        *(tsortv+1)=temp2;
      }
    }
    tree->next_status[ply].last=tree->last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   try the captures moves, which are in order based on    |
|   the expected gain of material.  captures that lose     |
|   material have been excluded from this phase.           |
|                                                          |
 ----------------------------------------------------------
*/
  case CAPTURE_MOVES:
    if (tree->next_status[ply].remaining) {
      tree->current_move[ply]=*(tree->next_status[ply].last);
      *tree->next_status[ply].last++=0;
      tree->next_status[ply].remaining--;
      if (!tree->next_status[ply].remaining) tree->next_status[ply].phase=KILLER_MOVE_1;
      return(CAPTURE_MOVES);
    }
    tree->next_status[ply].phase=KILLER_MOVE_1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the killer moves.  this phase tries the two   |
|   killers for the current ply without generating moves,  |
|   which saves time if a cutoff occurs.                   |
|                                                          |
 ----------------------------------------------------------
*/
  case KILLER_MOVE_1:
    if ((tree->hash_move[ply] != tree->killers[ply].move1) &&
        ValidMove(tree,ply,wtm,tree->killers[ply].move1)) {
      tree->current_move[ply]=tree->killers[ply].move1;
      tree->next_status[ply].phase=KILLER_MOVE_2;
      return(KILLER_MOVE_1);
    }
  case KILLER_MOVE_2:
    if ((tree->hash_move[ply] != tree->killers[ply].move2) &&
        ValidMove(tree,ply,wtm,tree->killers[ply].move2)) {
      tree->current_move[ply]=tree->killers[ply].move2;
      tree->next_status[ply].phase=GENERATE_ALL_MOVES;
      return(KILLER_MOVE_2);
    }
    tree->next_status[ply].phase=GENERATE_ALL_MOVES;
/*
 ----------------------------------------------------------
|                                                          |
|   now, generate all non-capturing moves.                 |
|                                                          |
 ----------------------------------------------------------
*/
  case GENERATE_ALL_MOVES:
    tree->last[ply]=GenerateNonCaptures(tree, ply, wtm, tree->last[ply]);
    tree->next_status[ply].phase=HISTORY_MOVES_1;
/*
 ----------------------------------------------------------
|                                                          |
|   now, try the history moves.  this phase takes the      |
|   complete move list, and passes over them in a classic  |
|   selection-sort, choosing the move with the highest     |
|   history score.  this phase is only done one time, as   |
|   it also purges the hash and killer moves from the list.|
|                                                          |
 ----------------------------------------------------------
*/
  case HISTORY_MOVES_1:
    tree->next_status[ply].remaining=1;
    tree->next_status[ply].phase=HISTORY_MOVES_2;
    bestval=0;
    bestp=0;
    for (movep=tree->last[ply-1];movep<tree->last[ply];movep++)
      if (*movep) {
        if (*movep == tree->hash_move[ply] ||
            *movep == tree->killers[ply].move1 ||
            *movep == tree->killers[ply].move2) *movep=0;
        else {
          index=*movep&4095;
          history_value= (wtm) ? history_w[index] : history_b[index];
          if (history_value > bestval) {
            bestval=history_value;
            bestp=movep;
          }
        }
      }
    if (bestp) {
      tree->current_move[ply]=*bestp;
      *bestp=0;
      return(HISTORY_MOVES_1);
    }
    goto remaining_moves;
/*
 ----------------------------------------------------------
|                                                          |
|   now, continue with the history moves, but since one    |
|   pass has been made over the complete move list, there  |
|   are no hash/killer moves left in the list, so the test |
|   for these can be avoided.                              |
|                                                          |
 ----------------------------------------------------------
*/
  case HISTORY_MOVES_2:
    bestval=0;
    bestp=0;
    for (movep=tree->last[ply-1];movep<tree->last[ply];movep++)
      if (*movep) {
        index=*movep&4095;
        history_value= (wtm) ? history_w[index] : history_b[index];
        if (history_value > bestval) {
          bestval=history_value;
          bestp=movep;
        }
      }
    if (bestval) {
      tree->current_move[ply]=*bestp;
      *bestp=0;
      tree->next_status[ply].remaining++;
      if (tree->next_status[ply].remaining > 3) {
        tree->next_status[ply].phase=REMAINING_MOVES;
        tree->next_status[ply].last=tree->last[ply-1];
      }
      return(HISTORY_MOVES_2);
    }
  remaining_moves:
    tree->next_status[ply].phase=REMAINING_MOVES;
    tree->next_status[ply].last=tree->last[ply-1];
/*
 ----------------------------------------------------------
|                                                          |
|   now try the rest of the set of moves.                  |
|                                                          |
 ----------------------------------------------------------
*/
  case REMAINING_MOVES:
    for (;tree->next_status[ply].last<tree->last[ply];
         tree->next_status[ply].last++)
      if (*tree->next_status[ply].last) {
        tree->current_move[ply]=*tree->next_status[ply].last;
        *tree->next_status[ply].last++=0;
        return(REMAINING_MOVES);
      }
    return(NONE);

  default:
    Print(4095,"oops!  next_status.phase is bad! [normal %d]\n",
          tree->next_status[ply].phase);
    return(NONE);
  }
}

// RedCrafty_481_f.cpp, ř. 11854-11884: SwapXray
/*
********************************************************************************
*                                                                              *
*   SwapXray() is used to determine if a piece is "behind" the piece on        *
*   <from>, and this piece would attack <to> if the piece on <from> were moved *
*   (as in playing out sequences of swaps).  if so, this indirect attacker is  *
*   added to the list of attackers bearing to <to>.                            *
*                                                                              *
********************************************************************************
*/
BITBOARD SwapXray(TREE *tree, BITBOARD attacks, int from, int direction) {
  switch (direction) {
  case 1:
    return(attacks | (AttacksRank(from) & RooksQueens & plus1dir[from]));
  case 7:
    return(attacks | (AttacksDiaga1(from) & BishopsQueens & plus7dir[from]));
  case 8:
    return(attacks | (AttacksFile(from) & RooksQueens & plus8dir[from]));
  case 9:
    return(attacks | (AttacksDiagh1(from) & BishopsQueens & plus9dir[from]));
  case -1:
    return(attacks | (AttacksRank(from) & RooksQueens & minus1dir[from]));
  case -7:
    return(attacks | (AttacksDiaga1(from) & BishopsQueens & minus7dir[from]));
  case -8:
    return(attacks | (AttacksFile(from) & RooksQueens & minus8dir[from]));
  case -9:
    return(attacks | (AttacksDiagh1(from) & BishopsQueens & minus9dir[from]));
  }
  return(attacks);
}

// RedCrafty_481_f.cpp, ř. 11886-12058: ValidMove
/* last modified 03/22/01 */
/*
********************************************************************************
*                                                                              *
*   ValidMove() is used to verify that a move is valid.  it is primarily used  *
*   to confirm that a move retrieved from the transposition/refutation table   *
*   is valid in the current position by checking the move against the current  *
*   chess board, castling status, en passant status, etc.                      *
*                                                                              *
********************************************************************************
*/
int ValidMove(TREE *tree, int ply, int wtm, int move) {
/*
 ----------------------------------------------------------
|                                                          |
|   make sure that the piece on <from> is the right color. |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (PcOnSq(From(move)) != Piece(move)) return(0);
  }
  else {
    if (PcOnSq(From(move)) != -Piece(move)) return(0);
  }
  switch (Piece(move)) {
/*
 ----------------------------------------------------------
|                                                          |
|   null-moves are caught as it is possible for a killer   |
|   move entry to be zero at certain times.                |
|                                                          |
 ----------------------------------------------------------
*/
    case none:
      return(0);
/*
 ----------------------------------------------------------
|                                                          |
|   king moves are validated here if the king is moving    |
|   two squares at one time (castling moves).  otherwise   |
|   fall into the normal piece validation routine below.   |
|   for castling moves, we need to verify that the         |
|   castling status is correct to avoid "creating" a new   |
|   rook or king.                                          |
|                                                          |
 ----------------------------------------------------------
*/
    case king:
      if (abs(From(move)-To(move)) == 2) {
        if (wtm) {
          if (WhiteCastle(ply) > 0) {
            if (To(move) == C1) {
              if ((!(WhiteCastle(ply)&2)) ||
                  (Occupied&mask_white_OOO) ||
                  (AttacksTo(tree,C1)&BlackPieces) ||
                  (AttacksTo(tree,D1)&BlackPieces) ||
                  (AttacksTo(tree,E1)&BlackPieces)) return(0);
            }
            else if (To(move) == G1) {
              if ((!(WhiteCastle(ply)&1)) ||
                  (Occupied&mask_white_OO) ||
                  (AttacksTo(tree,E1)&BlackPieces) ||
                  (AttacksTo(tree,F1)&BlackPieces) ||
                  (AttacksTo(tree,G1)&BlackPieces)) return(0);
            }
          }
          else return(0);
        }
        else {
          if (BlackCastle(ply) > 0) {
            if (To(move) == C8) {
              if ((!(BlackCastle(ply)&2)) ||
                  (Occupied&mask_black_OOO) ||
                  (AttacksTo(tree,C8)&WhitePieces) ||
                  (AttacksTo(tree,D8)&WhitePieces) ||
                  (AttacksTo(tree,E8)&WhitePieces)) return(0);
            }
            if (To(move) == 62) {
              if ((!(BlackCastle(ply)&1)) ||
                  (Occupied & mask_black_OO) ||
                  (AttacksTo(tree,E8)&WhitePieces) ||
                  (AttacksTo(tree,F8)&WhitePieces) ||
                  (AttacksTo(tree,G8)&WhitePieces)) return(0);
            }
          }
          else return(0);
        }
        return(1);
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   check for a normal pawn advance.                       |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn:
      if (abs(From(move)-To(move)) == 8) {
        if (wtm) {
          if ((From(move)<To(move)) && !PcOnSq(To(move))) return(1);
        }
        else {
          if ((From(move)>To(move)) && !PcOnSq(To(move))) return(1);
        }
        return(0);
      }
      else if (abs(From(move)-To(move)) == 16) {
        if (wtm) {
          if (!PcOnSq(To(move)-8) && !PcOnSq(To(move))) return(1);
        }
        else {
          if (!PcOnSq(To(move)+8) && !PcOnSq(To(move))) return(1);
        }
        return(0);
      }
      if (!Captured(move)) return(0);

/*
 ----------------------------------------------------------
|                                                          |
|   check for an en passant capture which is somewhat      |
|   unusual in that the [to] square does not contain the   |
|   pawn being captured.  make sure that the pawn being    |
|   captured advanced two ranks the previous move.         |
|                                                          |
 ----------------------------------------------------------
*/
      if (wtm) {
        if ((PcOnSq(To(move))==0) &&
            (PcOnSq(To(move)-8)==-pawn) &&
            (EnPassantTarget(ply)&SetMask(To(move)))) return(1);
      }
      else {
        if ((PcOnSq(To(move))==0) &&
            (PcOnSq(To(move)+8)==pawn) &&
            (EnPassantTarget(ply)&SetMask(To(move)))) return(1);
      }
/*
 ----------------------------------------------------------
|                                                          |
|   normal moves are all checked the same way.             |
|                                                          |
 ----------------------------------------------------------
*/
    case queen:
    case rook:
    case bishop:
      if (Attack(From(move),To(move))) break;
      return(0);
    case knight:
      break;
  }
/*
 ----------------------------------------------------------
|                                                          |
|   all normal moves are validated in the same manner, by  |
|   checking the from and to squares and also the attack   |
|   status for completeness.                               |
|                                                          |
 ----------------------------------------------------------
*/
  if (wtm) {
    if (Captured(move)==-PcOnSq(To(move)) &&
        Captured(move)!=king) return(1);
  }
  else {
    if (Captured(move)==PcOnSq(To(move)) &&
        Captured(move)!=king) return(1);
  }
  return(0);
}

// --- port: funkce ------------------------------------------------------------

void inicializuj_hledani() {
  static bool hotovo=false;
  int i, j;

  inicializuj();
  if (hotovo) return;
  InitializePieceMasks();   // Initialize, ř. 1837
  // oprava č. 3: velikosti tabulek z log_hash a log_pawn_hash, ne dvě
  // nezávislá čísla (Crafty drží hash_table_size == 1<<log_hash)
  hash_table_size=1<<log_hash;
  pawn_hash_table_size=1<<log_pawn_hash;
  // RedCrafty_481_f.cpp, ř. 1847-1883 (z Initialize)
  trans_ref_a_orig=(HASH_ENTRY *) malloc(16*hash_table_size+15);
  trans_ref_b_orig=(HASH_ENTRY *) malloc(16*2*hash_table_size+15);
  pawn_hash_table_orig=(PAWN_HASH_ENTRY *) malloc(sizeof(PAWN_HASH_ENTRY)*pawn_hash_table_size+15);
  trans_ref_a=(HASH_ENTRY*) (((ptrdiff_t) trans_ref_a_orig+15)&~15);
  trans_ref_b=(HASH_ENTRY*) (((ptrdiff_t) trans_ref_b_orig+15)&~15);
  pawn_hash_table=(PAWN_HASH_ENTRY*) (((ptrdiff_t) pawn_hash_table_orig+15)&~15);
  if (!trans_ref_a || !trans_ref_b) {
    hlaseni_malloc_failed++;  // port: bylo Memo1, ř. 1854
    free(trans_ref_a_orig);
    free(trans_ref_b_orig);
    free(pawn_hash_table_orig);
    hash_table_size=0;
    pawn_hash_table_size=0;
    log_hash=0;
    log_pawn_hash=0;
    trans_ref_a=0;
    trans_ref_b=0;
    pawn_hash_table=0;
  }
  InitializeHashTables();
  hash_maska=(1<<log_hash)-1;
  hash_maskb=(1<<(log_hash+1))-1;
  pawn_hash_mask=(1<<(log_pawn_hash))-1;

  for (i=0;i<8;i++)
    for (j=0;j<8;j++) {
      pval_b[i*8+j]=pval_w[(7-i)*8+j];
      nval_b[i*8+j]=nval_w[(7-i)*8+j];
      bval_b[i*8+j]=bval_w[(7-i)*8+j];
      rval_b[i*8+j]=rval_w[(7-i)*8+j];
      qval_b[i*8+j]=qval_w[(7-i)*8+j];
      kval_bn[i*8+j]=kval_wn[(7-i)*8+j];
      kval_bk[i*8+j]=kval_wk[(7-i)*8+j];
      kval_bq[i*8+j]=kval_wq[(7-i)*8+j];
      black_outpost[i*8+j]=white_outpost[(7-i)*8+j];
      king_defects_b[i*8+j]=king_defects_w[(7-i)*8+j];
    }

  hotovo=true;
}

int hledej(int hloubka, double cas_ms) {
  TREE * const tree=local[0];

  // Button3Click, ř. 155-175, bez GUI, stanley_int a čítače nodes
  sta_cas=std::chrono::steady_clock::now();
  casovy_limit_ms=cas_ms;
  next_time_check=nodes_between_time_checks;  // oprava č. 4
  // oprava č. 1 (PORT-CRAFTY.md, oddíl 3): Button3Click nastavoval test_mode=1
  // (ř. 159) a PreEvaluate pak po změně strany motoru nebo fáze hry nemazal
  // poziční skóre z hash tabulek (ř. 5892-5896).
  test_mode=0;
  search_depth=hloubka;
  lazy_eval_cutoff=200;
  largest_positional_score=300;
  thinking=1;
  tree->position[1]=tree->position[0];
  display=tree->pos;
  crafty_is_white=wtm;
  thinking=1;
  last_pv.pathd=0;
  last_pv.pathl=0;
  display=tree->pos;
  Iterate(wtm,think,0);
  thinking=0;
  // oprava č. 8: tah z hlavní varianty před vrácením ověřit. Když neprojde,
  // vrátí se první (nejlépe seřazený) kořenový tah; bez legálních tahů 0.
  {
    const int tah=tree->pv[0].path[1];
    if (tah && ValidMove(tree,1,wtm,tah)) return tah;
    return (n_root_moves > 0) ? root_moves[0].move : 0;
  }
}

}  // namespace crafty
