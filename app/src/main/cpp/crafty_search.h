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
//  Převzato z docs/zdroje/redcrafty/RedCrafty_481_f.cpp a .h stejně jako
//  crafty_core (skript tools/crafty/prevod.py + sestav.py). Jména beze
//  změny, nad každou funkcí rozsah řádků v originálu (počítáno podle LF).
//
//  Změny proti originálu v kroku C2/A, jen ty, bez kterých to na Androidu
//  nejde (POZNAMKY-CRAFTY.md):
//   - Application->ProcessMessages() ze Search pryč
//   - výpisy do Label a Memo pryč; výpis PV do Memo2 v DisplayPV nahrazuje
//     průběžné hlášení (PrubeznyVysledek, stejný tvar jako u RedMAXu)
//   - čas přes std::chrono::steady_clock (sta_cas), limit casovy_limit_ms
//     místo CSpinEdit1 v sekundách
//   - abort_search je std::atomic: zapisuje ho i jiné vlákno (stop), čte
//     se na týchž místech jako v originálu
//   - TStringList seznam_Next_Best_Move -> std::vector<std::string>
//   - Print a InputMove(ICS) místo do Memo1 počítají čítače hlaseni_*
//   - port_bez_hashe: přepínač jen pro test (hash vypnutý), výchozí 0
//
//  Krok C2/B: opravy z rozboru (PORT-CRAFTY.md, oddíl 3) č. 1 (test_mode),
//  3 (log_hash), 4 (kontrola času), 6 (RootMoveList) a 8 (Iterate při matu)
//  jsou v crafty_search.cpp ručně, označené "oprava č. N". Skript vyrábí
//  věrný stav C2/A bez nich.
//
//  Includovat jako POSLEDNÍ, po standardních hlavičkách (makra, viz
//  crafty_core.h).
// ---------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include "crafty_core.h"

namespace crafty {

// --- makra -------------------------------------------------------------------
// RedCrafty_481_f.h, ř. 27-104
#  define            PAWN_CONNECTED_PASSED_6TH         (100)
#  define                    KING_KING_TROPISM          (15)
#  define                       BISHOP_TRAPPED         (175)
#  define           BISHOP_PLUS_PAWNS_ON_COLOR           (4)
#  define           BISHOP_OVER_KNIGHT_ENDGAME          (30)
#  define                      BISHOP_MOBILITY           (3)
#  define                   BISHOP_KING_SAFETY          (10)
#  define                          ROOK_ON_7TH          (30)
#  define                    ROOK_ABSOLUTE_7TH          (20)
#  define              ROOK_CONNECTED_7TH_RANK          (16)
#  define                       ROOK_OPEN_FILE          (24)
#  define                  ROOK_HALF_OPEN_FILE           (5)
#  define             ROOK_CONNECTED_OPEN_FILE          (10)
#  define              ROOK_BEHIND_PASSED_PAWN          (40)
#  define                         ROOK_TRAPPED          (30)
#  define                         ROOK_LIMITED          (20)

#  define               QUEEN_ROOK_ON_7TH_RANK          (10)
#  define                    QUEEN_KING_SAFETY           (6)
#  define                     QUEEN_VS_2_ROOKS          (50)
#  define                      QUEEN_IS_STRONG          (30)
#  define                QUEEN_OFFSIDE_TROPISM           (8)

#  define                KING_SAFETY_MATE_G2G7           (3)
#  define              KING_SAFETY_MATE_THREAT         (600)
#  define                       KING_BACK_RANK          (12)
#  define                KING_SAFETY_STONEWALL           (7)
#  define                KING_SAFETY_OPEN_FILE           (4)
#  define                      CASTLE_OPPOSITE           (5)

#  define                 DEVELOPMENT_THEMATIC           (6)
#  define                  DEVELOPMENT_UNMOVED           (7)
#  define                  BLOCKED_CENTER_PAWN          (12)
#  define            DEVELOPMENT_LOSING_CASTLE          (20)
#  define              DEVELOPMENT_NOT_CASTLED          (24)
#  define              DEVELOPMENT_QUEEN_EARLY          (12)

#define EG_MAT                   14
#  define LockInit(p)
#  define Lock(p)
#  define UnLock(p)
#define MobilityRook(a)   (MobilityRank(a)+MobilityFile(a))
#define MobilityBishop(a) (MobilityDiaga1(a)+MobilityDiagh1(a))
#define MobilityQueen(a)  (MobilityBishop(a)+MobilityRook(a))
#  define MobilityRank(a)                                                   \
     (rook_mobility_r0[(a)][(tree->pos.w_occupied|tree->pos.b_occupied)>>   \
                            (56-((a)&56))&255])
#  define MobilityFile(a)                                                   \
     (rook_mobility_rl90[(a)][tree->pos.occupied_rl90>>                     \
                             (56-(((a)&7)<<3))&255])
#  define MobilityDiaga1(a)                                                 \
     (bishop_mobility_rl45[(a)][tree->pos.occupied_rl45>>                   \
                               bishop_shift_rl45[(a)]&255])
#  define MobilityDiagh1(a)                                                 \
     (bishop_mobility_rr45[(a)][tree->pos.occupied_rr45>>                   \
                               bishop_shift_rr45[(a)]&255])

#define SavePV(tree,ply,value,ph) do {                                      \
          tree->pv[ply-1].path[ply-1]=tree->current_move[ply-1];            \
          tree->pv[ply-1].pathl=ply-1;                                      \
          tree->pv[ply-1].pathh=ph;                                         \
          tree->pv[ply-1].pathd=iteration_depth;} while(0)
#define CaptureOrPromote(a) (((a)>>15)&63)
#define            MERGE_BLOCK           1000
#define SORT_ALL_MOVES            7
#define REMAINING_MOVES          10
#define HASH_MOVE                 1
#define GENERATE_CAPTURE_MOVES    2
#define CAPTURE_MOVES             3
#define KILLER_MOVE_1             4
#define KILLER_MOVE_2             5
#define GENERATE_ALL_MOVES        6
#define SORT_ALL_MOVES            7
#define HISTORY_MOVES_1           8
#define HISTORY_MOVES_2           9
#define REMAINING_MOVES          10
#define ROOT_MOVES               11

// RedCrafty_481_f.h, ř. 106-107
#define lock_t int
#define CPUS 1
// RedCrafty_481_f.h, ř. 179-179
#define DrawScore(wtm)                 (draw_score[wtm])
// RedCrafty_481_f.h, ř. 233-331
#define INCPLY                   60
#define UPPER                     2
#define AVOID_NULL_MOVE           4
#define EXACT                     3
#define LOWER                     1
#define         LEARN_INTERVAL             10
#define      BOOK_CLUSTER_SIZE           2000
#define      LEARN_COUNTER_BAD            -80
#  define                       PAWN_ADVANCE_A           (3)
#  define                       PAWN_ADVANCE_B           (3)
#  define                       PAWN_ADVANCE_C           (3)
#  define                       PAWN_ADVANCE_D           (3)
#  define                       PAWN_ADVANCE_E           (3)
#  define                       PAWN_ADVANCE_F           (3)
#  define                       PAWN_ADVANCE_G           (3)
#  define                       PAWN_ADVANCE_H           (3)
#  define                          PAWN_BASE_A           (0)
#  define                          PAWN_BASE_B           (3)
#  define                          PAWN_BASE_C           (6)
#  define                          PAWN_BASE_D           (9)
#  define                          PAWN_BASE_E           (9)
#  define                          PAWN_BASE_F           (6)
#  define                          PAWN_BASE_G           (3)
#  define                          PAWN_BASE_H           (0)
#  define                            BAD_TRADE         (160)
#  define                          EIGHT_PAWNS          (10)
#  define                        PAWNS_BLOCKED          (12)
#  define                  CENTER_PAWN_UNMOVED          (16)
#  define                             PAWN_DUO           (2)
#  define                             PAWN_JAM          (10)
#  define                         PAWN_WEAK_P1          (12)
#  define                         PAWN_WEAK_P2          (20)
#  define           PAWN_PROTECTED_PASSER_WINS          (50)
#  define                        WON_KP_ENDING         (150)

#if defined(NT_i386)
#   define BMF   "%I64u"
#   define BMF6  "%6I64u"
#   define BMF10 "%10I64u"
#else
#   define BMF   "%llu"
#   define BMF6  "%6llu"
#   define BMF10 "%10llu"
#endif

# define    EGTB_CACHE_DEFAULT 1024*1024

#if defined(MACOS)
#  if !defined(BOOKDIR)
#    define     BOOKDIR    "Books"
#  endif
#  if !defined(LOGDIR)
#    define      LOGDIR     "Logs"
#  endif
#  if !defined(TBDIR)
#    define       TBDIR       "TB"
#  endif
#  if !defined(RCDIR)
#    define       RCDIR        "."
#  endif
#else
#  if !defined(BOOKDIR)
#    define     BOOKDIR        "."
#  endif
#  if !defined(LOGDIR)
#    define      LOGDIR        "."
#  endif
#  if !defined(TBDIR)
#    define       TBDIR     "./TB"
#  endif
#  if !defined(RCDIR)
#    define       RCDIR        "."
#  endif
#endif
# define    VERSION                            "18.15"
#define     LEARN_COUNTER_GOOD           +100
#define         CAP_SCORE_GOOD           +150
#define          CAP_SCORE_BAD           -100
#define WORTHLESS                 0
#define NULL_MOVE                 0
#define DO_NULL                   1
#define NO_NULL                   0
#define NONE                      0

#define             SORT_BLOCK        4000000
#define	TB_CB_CACHE_CHUNK			8192 /* Must be power of 2 */

#define  VInitSqCtr(rgCtr, rgSquares, piece, bitboard) {         \
  int  cPieces=0;                                                \
  BITBOARD bbTemp=(bitboard);                                    \
  while (bbTemp) {                                               \
    const squaret sq=FirstOne (bbTemp);                          \
    (rgSquares)[(piece)*C_PIECES+cPieces]=sq;                    \
    cPieces++;                                                   \
    staClear(sq, bbTemp);                                           \
  }                                                              \
  (rgCtr)[(piece)]=cPieces;                                      \
}

// --- typy --------------------------------------------------------------------
// RedCrafty_481_f.h, ř. 407-407
typedef enum {think=1, puzzle=2, book=3, annotate=4} SEARCH_TYPE;
// RedCrafty_481_f.h, ř. 514-524
typedef struct {
  BITBOARD nodes;
  int move;
/*
  xxxx xxx1 = failed low
  xxxx xx1x = failed high
  1xxx xxxx = done (searched)
  x1xx xxxx = don't search in parallel
*/
  unsigned char status;
} ROOT_MOVE;
// RedCrafty_481_f.h, ř. 528-531
typedef struct {
  BITBOARD word1;
  BITBOARD word2;
} HASH_ENTRY;
// RedCrafty_481_f.h, ř. 533-544
#if (defined(NT_i386) || defined(NT_AXP))
#pragma pack(4)
#endif
typedef struct {
  BITBOARD position;
  unsigned int status_played;
  float learn;
  int CAP_score;
} BOOK_POSITION;
#if (defined(NT_i386) || defined(NT_AXP))
#pragma pack()
#endif

// --- průběžné hlášení (v originálu není) -------------------------------------
//
// Volá se z DisplayPV místo výpisu PV do Memo2, tedy po každé dokončené
// iteraci (DisplayPV z Iterate, level 5) a při změně PV na kořeni
// (SearchOutput, level 6), vždy za podmínek originálu. Na vlákně hledání.
struct PrubeznyVysledek {
    int  hloubka = 0;        // iteration_depth
    int  hodnota = 0;        // z pohledu strany na tahu v kořeni
    long uzlu = 0;
    int  nejlepsi_tah = 0;   // tah ve formátu Crafty (From/To/Piece/...)
    bool tah_nalezen = false;
};

using HlaseniFn = void (*)(void *kontext, const PrubeznyVysledek &);

// --- globály (definice jsou v crafty_search.cpp) ----------------------------
// RedCrafty_481_f.h, ř. 548-549
extern POSITION display;
extern FILE *log_file;
// RedCrafty_481_f.h, ř. 551-555
extern FILE *books_file;
extern FILE *normal_bs_file;
extern FILE *history_file;
extern FILE *auto_file;
// RedCrafty_481_f.h, ř. 682-687
extern BITBOARD white_minor_pieces;
extern BITBOARD black_minor_pieces;
extern BITBOARD white_pawn_race_wtm[64];
extern BITBOARD white_pawn_race_btm[64];
extern BITBOARD black_pawn_race_wtm[64];
extern BITBOARD black_pawn_race_btm[64];
// RedCrafty_481_f.h, ř. 691-728
extern HASH_ENTRY *trans_ref_a_orig;
extern HASH_ENTRY *trans_ref_b_orig;
extern PAWN_HASH_ENTRY *pawn_hash_table_orig;
extern HASH_ENTRY *trans_ref_a;
extern HASH_ENTRY *trans_ref_b;
extern PAWN_HASH_ENTRY *pawn_hash_table;
extern int hash_maska;
extern int hash_maskb;
extern unsigned int pawn_hash_mask;
extern signed char pval_b[64];
extern signed char nval_b[64];
extern signed char bval_b[64];
extern signed char rval_b[64];
extern signed char qval_b[64];
extern signed char kval_bn[64];
extern signed char kval_bk[64];
extern signed char kval_bq[64];
extern signed char black_outpost[64];
extern signed char king_defects_b[64];
extern int move_actually_played;
extern int whisper_depth;
extern unsigned int opponent_start_time, opponent_end_time;
extern char cmd_buffer[512];
extern int auto232;
extern int auto232_delay;
extern int book_move;
extern ROOT_MOVE root_moves[256];
extern int display_options;
extern int early_exit;
extern int search_depth;
extern int EGTB_search;
extern unsigned int search_nodes;
extern int search_move;
extern volatile int quit;
extern int output_format;
// RedCrafty_481_f.h, ř. 737-756
extern int hash_table_size;
extern int pawn_hash_table_size;
extern int log_hash;
extern int log_pawn_hash;
extern signed char transposition_id;
extern int initialized;
extern signed char resign;
extern signed char resign_count;
extern signed char draw_count;
extern int crafty_is_white;
extern int ics;
extern int xboard;
extern int ponder_moves[220];
extern int num_ponder_moves;
extern int last_opponent_move;
extern int force;
extern int trojan_check;
extern int computer_opponent;
extern int draw_score[2];
// RedCrafty_481_f.h, ř. 758-774
extern int tc_time_remaining;
extern int tc_time;
extern int tc_time_remaining_opponent;
extern int tc_moves_remaining;
extern int tc_moves;
extern signed char resign_counter;
extern signed char draw_counter;
extern int usage_level;
extern int input_status;
extern int pong;
extern int time_limit;
extern signed char puzzling;
extern signed char thinking;
extern signed char pondering;
extern int time_used_opponent;
extern int ponder_value;
extern int last_root_value;
// RedCrafty_481_f.h, ř. 778-922
extern signed char draw_offer_pending;
extern int abs_draw_score;
extern int tc_increment;
extern int last_value;
extern char book_hint[512];
extern int null_min;
extern int null_max;
extern signed char booking;
extern int easy_move;
extern std::atomic<signed char> abort_search;
extern int root_wtm;
extern unsigned int parallel_splits;
extern unsigned int parallel_stops;
extern unsigned int max_split_blocks;
extern int EGTB_draw;
extern int EGTB_use;
extern int EGTBlimit;
extern int burp;
extern int n_root_moves;
extern int root_value;
extern int iteration_depth;
extern int root_alpha;
extern int root_beta;
extern int root_print_ok;
extern int trace_level;
extern int analyze_mode;
extern unsigned int noise_level;
extern int EGTB_cache_size;
extern void *EGTB_cache;
extern FILE *computer_bs_file;
extern int EGTB_setup;
extern int test_mode;
extern signed char bishop_pair[9];
extern const char square_color[64];
extern const char b_n_mate_dark_squares[64];
extern const char b_n_mate_light_squares[64];
extern const char mate[64];
extern int cbEGTBCompBytes;
extern FILE *book_file;
extern FILE *book_lrn_file;
extern FILE *position_file;
extern FILE *position_lrn_file;
extern int temper_b[64], temper_w[64];
extern int tropism[128];
extern char pawn_rams[9];
extern int incheck_depth;
extern int onerep_depth;
extern int pushpp_depth;
extern int recap_depth;
extern int mate_depth;
extern char hint[512];
extern char version[6];
extern int number_of_GMs;
extern int batch_mode;
extern int swindle_mode;
extern int annotate_mode;
extern int call_flag;
extern int crafty_rating;
extern int opponent_rating;
extern int last_search_value;
extern int DGT_active;
extern int to_dgt;
extern int from_dgt;
extern int pgn_suggested_percent;
extern char pgn_event[32];
extern char pgn_site[32];
extern char pgn_date[32];
extern char pgn_round[32];
extern char pgn_white[64];
extern char pgn_white_elo[32];
extern char pgn_black[64];
extern char pgn_black_elo[32];
extern char pgn_result[32];
extern int number_of_blockers;
extern char blocker_list[512][20];
extern int number_of_IMs;
extern char IM_list[512][20];
extern int number_to_play;
extern char toplay_list[512][20];
extern int number_of_computers;
extern char computer_list[512][20];
extern int number_auto_kibitzers;
extern char auto_kibitz_list[64][20];
extern int log_id;
extern char log_filename[64];
extern char history_filename[64];
extern char channel_title[32];
extern char book_path[128];
extern char log_path[128];
extern char tb_path[128];
extern char rc_path[128];
extern int total_moves;
extern int post;
// RedCrafty_481_f.h, ř. 924-944
extern const char empty[9];
extern const char king_tropism_n[8];
extern const char king_tropism_b[8];
extern const char king_tropism_r[8];
extern const char king_tropism_at_r[8];
extern const char king_tropism_q[8];
extern const char king_tropism_at_q[8];
extern int show_book;
extern int book_learn_eval[LEARN_INTERVAL];
extern int book_learn_depth[LEARN_INTERVAL];
extern BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
extern BOOK_POSITION books_buffer[BOOK_CLUSTER_SIZE];
extern int book_random;
extern int book_accept_mask;
extern int book_reject_mask;
extern int book_search_trigger;
extern int use_asymmetry;
// RedCrafty_481_f.h, ř. 1060-1341
extern signed char pval_w[64];
extern signed char nval_w[64];
extern signed char bval_w[64];
extern signed char rval_w[64];
extern signed char qval_w[64];
extern signed char kval_wn[64];
extern signed char kval_wk[64];
extern signed char kval_wq[64];
extern signed char white_outpost[64];
extern signed char king_defects_w[64];
extern const int temper[64];
extern const char ttemper[64];
extern const char missing[8];
extern const char openf[4];
extern const char hopenf[4];
extern int king_safety_asymmetry;
extern int king_safety_scale;
extern int blocked_scale;
extern int passed_scale;
extern int pawn_scale;
extern int king_safety_tropism;
extern char GM_list[512][20];
extern const int king_tropism[128];
extern const char connected_passed_pawn_value[8];
extern const int hidden_passed_pawn_value[8];
extern const int passed_pawn_value[8];
extern const char blockading_passed_pawn_value[8];
extern const char isolated_pawn_value[9];
extern const char isolated_pawn_of_value[9];
extern const char doubled_pawn_value[7];
extern const char pawn_rams_v[9];
extern const char supported_passer[8];
extern const char outside_passed[128];
extern const char majority[128];
extern const char push_extensions[64];
extern const int p_values[15];

// RedCrafty_481_f.h, ř. 24-25 (Next Best Move); TStringList -> vector
extern std::vector<std::string> seznam_Next_Best_Move;
extern int status_Next_Best_Move;

// --- port: globály -----------------------------------------------------------

// Místo TDateTime sta_cas (ř. 557) a CSpinEdit1->Value v sekundách.
extern std::chrono::steady_clock::time_point sta_cas;
extern double casovy_limit_ms;

// Přepínač jen pro test: HashProbe nic nenajde, HashStore a HashStorePV nic
// neuloží. Výchozí 0 = chování originálu. Pěšcová hash tabulka se nevypíná.
extern int port_bez_hashe;

extern HlaseniFn hlaseni_prubehu;
extern void *hlaseni_prubehu_kontext;

// Náhrady výpisů do Memo1 (čítače, testy je čtou).
extern int hlaseni_print;                     // Print, ř. 5117 (chyby hledání)
extern int hlaseni_illegal_move;              // InputMove, InputMoveICS
extern int hlaseni_clearing_hash_tables;      // PreEvaluate, ř. 5905-5906
extern int hlaseni_clearing_hash_tables_50;   // PreEvaluate, ř. 5923
extern int hlaseni_malloc_failed;             // Initialize, ř. 1854
extern int hlaseni_kontrol_casu;              // Search: kolikrát proběhla kontrola času

// oprava č. 4: čítač uzlů do další kontroly času (jako next_time_check
// a nodes_between_time_checks v originálu Crafty)
extern int next_time_check;
extern int nodes_between_time_checks;

// --- funkce, v pořadí z RedCrafty_481_f.cpp ----------------------------------
int KingPawnSquare(int pawn, int king, int queen, int ptm);
void InitializePieceMasks(void);
void InitializeHashTables(void);
int* GenerateCheckEvasions(TREE *tree, int ply, int wtm, int *move);
int HashProbe(TREE *tree, int ply, int depth, int wtm, int *alpha, int *beta, int *threat);
int Iterate(int wtm, int search_type, int root_list_done);
char* OutputMove(TREE *tree, int move, int ply, int wtm);
void Print(int vb, const char *fmt, ...);
int InputMove(TREE *tree, char *text, int ply, int wtm, int silent, int ponder_list);
BITBOARD AttacksTo(TREE *tree, int square);
BITBOARD InterposeSquares(int check_direction, int king_square, int checking_square);
int PinnedOnKing(TREE *tree, int wtm, int square);
void PreEvaluate(TREE *tree, int crafty_is_white);
int Book(TREE *tree, int wtm, int root_list_done);
void RootMoveList(int wtm);
void HashStorePV(TREE *tree, int ply, int wtm);
int SearchRoot(TREE *tree, int alpha, int beta, int wtm, int depth);
void DisplayPV(TREE *tree, int level, int wtm, int time, int value, PATH *pv);
int OutputGood(TREE *tree, char* text, int ply, int wtm);
void ClearHashTableScores(int dopawnstoo);
void Phase(void);
int Evaluate(TREE *tree, int ply, int wtm, int alpha, int beta);
int EvaluateDevelopmentB(TREE *tree, int ply);
int EvaluateDevelopmentW(TREE *tree, int ply);
int EvaluateMate(TREE *tree);
int EvaluateMaterial(TREE *tree);
int EvaluateKingSafety(TREE *tree, int ply);
int EvaluatePassedPawns(TREE *tree);
int EvaluatePassedPawnRaces(TREE *tree, int wtm);
int EvaluatePawns(TREE *tree);
int EvaluateStalemate(TREE *tree, int wtm);
int EvaluateWinner(TREE *tree);
int InputMoveICS(TREE *tree, char *text, int ply, int wtm, int silent, int ponder_list);
int Quiesce(TREE *tree, int alpha, int beta, int wtm, int ply);
int NextRootMove(TREE *tree, TREE *mytree, int wtm);
int Search(TREE *tree, int alpha, int beta, int wtm, int depth, int ply, int do_null, int lp_recapture);
void SearchOutput(TREE *tree, int value, int bound);
void History(TREE *tree, int ply, int depth, int wtm, int move);
int HasOpposition(int on_move, int white_king, int black_king);
int Swap(TREE *tree, int source, int target, int wtm);
int RepetitionCheck(TREE *tree, int ply, int wtm);
void HashStore(TREE *tree, int ply, int depth, int wtm, int type, int value, int threat);
int NextEvasion(TREE *tree, int ply, int wtm);
int NextMove(TREE *tree, int ply, int wtm);
BITBOARD SwapXray(TREE *tree, BITBOARD attacks, int from, int direction);
int ValidMove(TREE *tree, int ply, int wtm, int move);

// --- port: funkce ------------------------------------------------------------

// inicializuj() z crafty_core a k tomu zbytek Initialize (ř. 1837,
// 1847-1883): InitializePieceMasks, alokace a vyčištění hash tabulek,
// zrcadlení hodnotových tabulek pro černého. Jen při prvním volání.
void inicializuj_hledani();

// Náhrada Button3Click (ř. 154-178) bez GUI: nastaví totéž co originál
// (kromě test_mode, oprava č. 1) a zavolá Iterate. Vrací nejlepší tah
// tree->pv[0].path[1]; tah nehraje, MakeMoveRoot je na volajícím.
// hloubka = search_depth (CSpinEdit2, 0 = bez omezení), cas_ms = časový
// limit (CSpinEdit1). Vrácený tah je ověřený přes ValidMove (oprava č. 8).
int hledej(int hloubka, double cas_ms);

}  // namespace crafty
