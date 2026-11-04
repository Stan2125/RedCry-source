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
//  Crafty 18.15 (RedCrafty) - pravidla: generátor tahů, make/unmake
//
//  Převzato z docs/zdroje/redcrafty/RedCrafty_481_f.cpp a .h (C++ Builder,
//  cp1250). Jména funkcí, struktur, maker a tabulek jsou PŘESNĚ jako
//  v originálu, aby šla každá funkce porovnat diffem s Crafty 18.15.
//  Nad každým blokem je jeho rozsah řádků v RedCrafty_481_f.*; řádky se
//  počítají podle LF, stejně jako v PORT-CRAFTY.md a jako to dělá grep -n.
//
//  Změny proti originálu, všechny mechanické (POZNAMKY-CRAFTY.md):
//   - celé v namespace crafty, bez TForm1::
//   - typedef unsigned __int64 BITBOARD -> uint64_t, BEZZNAMÉNKOVÝ; se
//     znaménkovým typem by aritmetický posuv rozsypal celý engine
//   - bez klíčového slova register
//   - výpisy do Memo1 hlásily chybové stavy; nahrazují je čítače hlaseni_*
//     a testy kontrolují, že zůstaly nulové
//   - globály jsou v originálu definované přímo v hlavičce; tady jsou
//     definice v crafty_core.cpp a v hlavičce jen extern
//   - ReadParse bere const char *delims (řetězcový literál není char*)
//
//  Zatím jen pravidla. Hodnocení, hledání, hash tabulky, knihovna zahájení,
//  čas ani UCI tu nejsou (kroky C2 a C3).
//
//  PAST: SetMask(i) = mask_1 >> i, tedy A1 je NEJVYŠŠÍ bit. Bílý pěšec proto
//  postupuje posunem DOPRAVA (>>8), černý doleva (<<8). Vypadá to prohozeně
//  a není, neopravovat (PORT-CRAFTY.md, oddíl 2).
//
//  POZOR na makra: hlavička definuje přes stovku maker s obecnými jmény
//  (Set, File, Rank, Check, Occupied, Material, HashKey...) a makra neznají
//  jmenné prostory. Includovat ji jako POSLEDNÍ, po standardních hlavičkách,
//  a jen ze souborů Crafty.
// ---------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace crafty {

// RedCrafty_481_f.h, ř. 333: typedef unsigned __int64 BITBOARD;
typedef uint64_t BITBOARD;

// --- makra -------------------------------------------------------------------
// RedCrafty_481_f.h, ř. 105-105
#define MAXPLY 65
// RedCrafty_481_f.h, ř. 108-178
#define ClearMask(a)        (clear_mask[a])
#define ClearMaskRL90(a)    (clear_mask_rl90[a])
#define ClearMaskRL45(a)    (clear_mask_rl45[a])
#define ClearMaskRR45(a)    (clear_mask_rr45[a])
#define SetMask(a)          (set_mask[a])
#define SetMaskRL45(a)      (set_mask_rl45[a])
#define SetMaskRR45(a)      (set_mask_rr45[a])
#define SetMaskRL90(a)      (set_mask_rl90[a])
#define File(x)             ((x)&7)
#define Rank(x)       (((x)>>3)&7)
#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))
#define FirstOne8Bit(a) (first_one_8bit[a])
#define LastOne8Bit(a) (last_one_8bit[a])
#define PopCnt8Bit(a) (pop_cnt_8bit[a])
/*
    Attack() is used to determine if a sliding piece on 'from' can reach
    'to'.  the only requirement is that there be no pieces along the pathway
    connecting from and to.
*/
#define Attack(from,to) (!(obstructed[from][to] & Occupied))

#define FileDistance(a,b) abs(((a)&7) - ((b)&7))
#define RankDistance(a,b) abs(((a)>>3) - ((b)>>3))
#define Distance(a,b) Max(FileDistance(a,b),RankDistance(a,b))
#define staClear(a,b)          b=ClearMask(a)&(b)
#define PcOnSq(sq)            (tree->pos.board[sq])
#define HashEP(a,b)         b=enpassant_random[a]^(b)
#define HashCastleW(a,b)    b=castle_random_w[a]^(b)
#define HashCastleB(a,b)    b=castle_random_b[a]^(b)
#define PAWN_VALUE              100
#define KNIGHT_VALUE            300
#define BISHOP_VALUE            300
#define ROOK_VALUE              500
#define QUEEN_VALUE             900
#define KING_VALUE            40000
#define TotalPieces           (tree->pos.total_pieces)
#define Occupied              (tree->pos.w_occupied|tree->pos.b_occupied)
#define ChangeSide(x) ((x)^1)
#define HashKey               (tree->pos.hash_key)
#define Check(wtm)  \
  Attacked(tree, (wtm)?WhiteKingSQ:BlackKingSQ,ChangeSide(wtm))
#define WhitePawns            (tree->pos.w_pawn)
#define WhiteKnights          (tree->pos.w_knight)
#define WhiteBishops          (tree->pos.w_bishop)
#define WhiteRooks            (tree->pos.w_rook)
#define WhiteQueens           (tree->pos.w_queen)
#define WhiteKingSQ           (tree->pos.white_king)
#define AttacksBishop(a)  (AttacksDiaga1(a)|AttacksDiagh1(a))
#  define AttacksDiaga1(a)                                                \
      bishop_attacks_rl45[(a)][(tree->pos.occupied_rl45>>                 \
                                bishop_shift_rl45[(a)])&255]
#  define AttacksDiagh1(a)                                                \
      bishop_attacks_rr45[(a)][(tree->pos.occupied_rr45>>                 \
                                bishop_shift_rr45[(a)])&255]
#define BishopsQueens         (tree->pos.bishops_queens)
#define WhitePieces           (tree->pos.w_occupied)
#define AttacksRook(a)    (AttacksRank(a)|AttacksFile(a))
#  define AttacksRank(a)                                                  \
      rook_attacks_r0[(a)][((tree->pos.w_occupied|tree->pos.b_occupied)>> \
                           (56-((a)&56)))&255]
#  define AttacksFile(a)                                                  \
      rook_attacks_rl90[(a)][(tree->pos.occupied_rl90>>                   \
                             (56-(((a)&7)<<3)))&255]
#define RooksQueens           (tree->pos.rooks_queens)
#define WhiteKing             (SetMask(tree->pos.white_king))
#define BlackPawns            (tree->pos.b_pawn)
#define BlackKnights          (tree->pos.b_knight)
#define BlackPieces           (tree->pos.b_occupied)
#define BlackKing             (SetMask(tree->pos.black_king))
#define BlackKingSQ           (tree->pos.black_king)
// RedCrafty_481_f.h, ř. 180-232
#define From(a)             ((a)&63)
#define To(a)               (((a)>>6)&63)
#define Piece(a)            (((a)>>12)&7)
#define Captured(a)         (((a)>>15)&7)
#define Promote(a)          (((a)>>18)&7)
#define MATE                  32768
#define AttacksQueen(a)   (AttacksBishop(a)|AttacksRook(a))
#define EnPassant(ply)        (tree->position[ply].enpassant_target)
#define EnPassantTarget(ply)  (EnPassant(ply) ? SetMask(EnPassant(ply)) : 0)
#define BlackBishops          (tree->pos.b_bishop)
#define BlackRooks            (tree->pos.b_rook)
#define BlackQueens           (tree->pos.b_queen)
#define WhiteCastle(ply)      (tree->position[ply].w_castle)
#define BlackCastle(ply)      (tree->position[ply].b_castle)
#define PawnHashKey           (tree->pos.pawn_hash_key)
#define Rule50Moves(ply)      (tree->position[ply].rule_50_moves)
#define ClearRL90(a,b)      b=ClearMaskRL90(a)&(b)
#define OccupiedRL90          (tree->pos.occupied_rl90)
#define OccupiedRL45          (tree->pos.occupied_rl45)
#define OccupiedRR45          (tree->pos.occupied_rr45)
#define ClearRL45(a,b)      b=ClearMaskRL45(a)&(b)
#define ClearRR45(a,b)      b=ClearMaskRR45(a)&(b)
#define SetRL90(a,b)        b=SetMaskRL90(a)|(b)
#define SetRL45(a,b)        b=SetMaskRL45(a)|(b)
#define SetRR45(a,b)        b=SetMaskRR45(a)|(b)
#define ClearSet(a,b)       b=((a)^(b))
#define Material              (tree->pos.material_evaluation)
#define TotalBlackPawns       (tree->pos.black_pawns)
#define TotalWhitePawns       (tree->pos.white_pawns)
#define Set(a,b)            b=SetMask(a)|(b)
#define TotalWhitePieces      (tree->pos.white_pieces)
#define WhiteMinors           (tree->pos.white_minors)
#define WhiteMajors           (tree->pos.white_majors)
#define TotalBlackPieces      (tree->pos.black_pieces)
#define BlackMinors           (tree->pos.black_minors)
#define BlackMajors           (tree->pos.black_majors)
#define HashPB(a,b)         b=b_pawn_random[a]^(b)
#define HashPW(a,b)         b=w_pawn_random[a]^(b)
#define HashNB(a,b)         b=b_knight_random[a]^(b)
#define HashNW(a,b)         b=w_knight_random[a]^(b)
#define HashBB(a,b)         b=b_bishop_random[a]^(b)
#define HashBW(a,b)         b=w_bishop_random[a]^(b)
#define HashRB(a,b)         b=b_rook_random[a]^(b)
#define HashRW(a,b)         b=w_rook_random[a]^(b)
#define HashQB(a,b)         b=b_queen_random[a]^(b)
#define HashQW(a,b)         b=w_queen_random[a]^(b)
#define HashKB(a,b)         b=b_king_random[a]^(b)
#define HashKW(a,b)         b=w_king_random[a]^(b)
#define HashEP(a,b)         b=enpassant_random[a]^(b)
#define HashCastleW(a,b)    b=castle_random_w[a]^(b)
#define HashCastleB(a,b)    b=castle_random_b[a]^(b)
#define SlidingDiag(piece)    (((piece) & 5) == 5)
#define SlidingRow(piece)     (((piece) & 6) == 6)

// --- typy --------------------------------------------------------------------
// RedCrafty_481_f.h, ř. 335-360
typedef struct {
  BITBOARD key;
  short    p_score;

  unsigned char allb;
  unsigned char black_defects_k;
  unsigned char black_defects_q;
  unsigned char passed_b;
  unsigned char candidates_b;

  unsigned char allw;
  unsigned char white_defects_k;
  unsigned char white_defects_q;
  unsigned char passed_w;
  unsigned char candidates_w;

  unsigned char sta_protected;     // původně protected ale to je klíčové slovo
  unsigned char outside;
  unsigned char open_files;
} PAWN_HASH_ENTRY;

typedef struct {
  int phase;
  int remaining;
  int *last;
} NEXT_MOVE;
// RedCrafty_481_f.h, ř. 372-385
typedef struct {
  int path[MAXPLY];
  unsigned char pathh;
  unsigned char pathl;
  unsigned char pathd;
} PATH;

typedef struct {
  int             move1;
  int             move2;
} KILLER;

typedef enum {none=0, pawn=1, knight=2, king=3,
              bishop=5, rook=6, queen=7} PIECE;
// RedCrafty_481_f.h, ř. 387-388
typedef enum {empty_v=0, pawn_v=1, knight_v=3,
              bishop_v=3, rook_v=5, queen_v=9} PIECE_V;
// RedCrafty_481_f.h, ř. 390-390
typedef enum {RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8} ranks;
// RedCrafty_481_f.h, ř. 395-405
typedef enum { A1,B1,C1,D1,E1,F1,G1,H1,
               A2,B2,C2,D2,E2,F2,G2,H2,
               A3,B3,C3,D3,E3,F3,G3,H3,
               A4,B4,C4,D4,E4,F4,G4,H4,
               A5,B5,C5,D5,E5,F5,G5,H5,
               A6,B6,C6,D6,E6,F6,G6,H6,
               A7,B7,C7,D7,E7,F7,G7,H7,
               A8,B8,C8,D8,E8,F8,G8,H8,
               BAD_SQUARE } squares;

typedef enum {FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH} files;
// RedCrafty_481_f.h, ř. 413-509
typedef  struct {
  BITBOARD       w_occupied;
  BITBOARD       b_occupied;
  BITBOARD       occupied_rl90;
  BITBOARD       occupied_rl45;
  BITBOARD       occupied_rr45;
  BITBOARD       rooks_queens;
  BITBOARD       bishops_queens;
  BITBOARD       w_pawn;
  BITBOARD       w_knight;
  BITBOARD       w_bishop;
  BITBOARD       w_rook;
  BITBOARD       w_queen;
  BITBOARD       b_pawn;
  BITBOARD       b_knight;
  BITBOARD       b_bishop;
  BITBOARD       b_rook;
  BITBOARD       b_queen;
  BITBOARD       hash_key;
  BITBOARD       pawn_hash_key;
  int            material_evaluation;
  signed char    white_king;
  signed char    black_king;
  signed char    board[64];
  signed char    white_pieces;
  signed char    white_minors;
  signed char    white_majors;
  signed char    white_pawns;
  signed char    black_pieces;
  signed char    black_minors;
  signed char    black_majors;
  signed char    black_pawns;
  signed char    total_pieces;
} POSITION;

typedef struct {
  unsigned char enpassant_target;
  signed   char w_castle;
  signed   char b_castle;
  unsigned char rule_50_moves;
} SEARCH_POSITION;

struct sta_tree {
  POSITION        pos;
  PAWN_HASH_ENTRY pawn_score;
  NEXT_MOVE       next_status[MAXPLY];
  BITBOARD        save_hash_key[MAXPLY+2];
  BITBOARD        replist_w[128];
  BITBOARD        replist_b[128];
  BITBOARD        *rephead_w;
  BITBOARD        *rephead_b;
  BITBOARD        all_pawns;
  BITBOARD        nodes_searched;
  BITBOARD        save_pawn_hash_key[MAXPLY+2];
  SEARCH_POSITION   position[MAXPLY+2];
  int             current_move[MAXPLY];
  int             hash_move[MAXPLY];
  int             *last[MAXPLY];
  PATH            pv[MAXPLY];
  unsigned int    fail_high;
  unsigned int    fail_high_first;
  unsigned int    evaluations;
  unsigned int    transposition_probes;
  unsigned int    transposition_hits;
  unsigned int    pawn_probes;
  unsigned int    pawn_hits;
  unsigned int    egtb_probes;
  unsigned int    egtb_probes_successful;
  unsigned int    check_extensions_done;
  unsigned int    recapture_extensions_done;
  unsigned int    passed_pawn_extensions_done;
  unsigned int    one_reply_extensions_done;
  unsigned int    mate_extensions_done;
  KILLER          killers[MAXPLY];
  int             move_list[5120];
  int             sort_value[256];
  signed char     in_check[MAXPLY];
  signed char     phase[MAXPLY];
  int             search_value;
  int             w_safety, b_safety;
  int             w_kingsq, b_kingsq;
  int             endgame;
  int             root_move;
  volatile char   stop;
  volatile char   done;
  char            root_move_text[16];
  char            remaining_moves_text[16];
  int             alpha;
  int             beta;
  int             value;
  int             wtm;
  int             depth;
  int             ply;
  int             mate_threat;
  int             lp_recapture;
  int             used;
};
// RedCrafty_481_f.h, ř. 526-526
typedef struct sta_tree TREE;

// --- globály (definice jsou v crafty_core.cpp) ------------------------------
// RedCrafty_481_f.h, ř. 550-550
extern TREE *local[1];
// RedCrafty_481_f.h, ř. 559-681
extern unsigned char first_one_8bit[256];
extern unsigned char last_one_8bit[256];
extern unsigned char pop_cnt_8bit[256];
extern unsigned char connected_passed[256];
extern unsigned char file_spread[256];
extern signed char is_outside[256][256];
extern signed char is_outside_c[256][256];
extern BITBOARD mask_1;
extern BITBOARD mask_2;
extern BITBOARD mask_3;
extern BITBOARD mask_8;
extern BITBOARD mask_16;
extern BITBOARD mask_112;
extern BITBOARD mask_120;
extern BITBOARD mask_clear_entry;
extern BITBOARD w_pawn_random[64];
extern BITBOARD b_pawn_random[64];
extern BITBOARD w_knight_random[64];
extern BITBOARD b_knight_random[64];
extern BITBOARD w_bishop_random[64];
extern BITBOARD b_bishop_random[64];
extern BITBOARD w_rook_random[64];
extern BITBOARD b_rook_random[64];
extern BITBOARD w_queen_random[64];
extern BITBOARD b_queen_random[64];
extern BITBOARD w_king_random[64];
extern BITBOARD b_king_random[64];
extern BITBOARD castle_random_w[2];
extern BITBOARD castle_random_b[2];
extern BITBOARD enpassant_random[65];
extern BITBOARD wtm_random[2];
extern BITBOARD clear_mask[65];
extern BITBOARD clear_mask_rl90[65];
extern BITBOARD clear_mask_rl45[65];
extern BITBOARD clear_mask_rr45[65];
extern BITBOARD set_mask[65];
extern BITBOARD set_mask_rl90[65];
extern BITBOARD set_mask_rl45[65];
extern BITBOARD set_mask_rr45[65];
extern BITBOARD rank_mask[8];
extern BITBOARD file_mask[8];
extern BITBOARD mask_pawn_duo[64];
extern BITBOARD mask_pawn_protected_b[64];
extern BITBOARD mask_pawn_protected_w[64];
extern BITBOARD right_side_mask[8];
extern BITBOARD left_side_mask[8];
extern BITBOARD right_side_empty_mask[8];
extern BITBOARD left_side_empty_mask[8];
extern BITBOARD mask_efgh, mask_fgh, mask_abc,mask_abcd;
extern BITBOARD mask_abs7_w, mask_abs7_b;
extern BITBOARD mask_advance_2_w;
extern BITBOARD mask_advance_2_b;
extern BITBOARD mask_left_edge;
extern BITBOARD mask_right_edge;
extern BITBOARD mask_A7H7;
extern BITBOARD mask_A2H2;
extern BITBOARD mask_A3B3;
extern BITBOARD mask_B3C3;
extern BITBOARD mask_F3G3;
extern BITBOARD mask_G3H3;
extern BITBOARD mask_A6B6;
extern BITBOARD mask_B6C6;
extern BITBOARD mask_F6G6;
extern BITBOARD mask_G6H6;
extern BITBOARD mask_white_OO;
extern BITBOARD mask_white_OOO;
extern BITBOARD mask_black_OO;
extern BITBOARD mask_black_OOO;
extern BITBOARD stonewall_white;
extern BITBOARD stonewall_black;
extern BITBOARD e7_e6;
extern BITBOARD e2_e3;
extern BITBOARD closed_white;
extern BITBOARD closed_black;
extern BITBOARD mask_kr_trapped_w[3];
extern BITBOARD mask_qr_trapped_w[3];
extern BITBOARD mask_kr_trapped_b[3];
extern BITBOARD mask_qr_trapped_b[3];
extern BITBOARD good_bishop_kw;
extern BITBOARD good_bishop_qw;
extern BITBOARD good_bishop_kb;
extern BITBOARD good_bishop_qb;
extern BITBOARD light_squares;
extern BITBOARD dark_squares;
extern BITBOARD not_rook_pawns;
extern BITBOARD plus1dir[65];
extern BITBOARD plus7dir[65];
extern BITBOARD plus8dir[65];
extern BITBOARD plus9dir[65];
extern BITBOARD minus1dir[65];
extern BITBOARD minus7dir[65];
extern BITBOARD minus8dir[65];
extern BITBOARD minus9dir[65];
extern BITBOARD mask_eptest[64];
extern BITBOARD stalemate_sqs[64];
extern BITBOARD edge_moves[64];
extern BITBOARD w_pawn_attacks[64];
extern BITBOARD b_pawn_attacks[64];
extern BITBOARD knight_attacks[64];
extern BITBOARD bishop_attacks[64];
extern BITBOARD queen_attacks[64];
extern BITBOARD rook_attacks[64];
extern BITBOARD king_attacks[64];
extern BITBOARD king_attacks_1[64];
extern BITBOARD king_attacks_2[64];
extern BITBOARD obstructed[64][64];
extern unsigned char first_one[65536];
extern unsigned char last_one[65536];
extern signed char directions[64][64];
extern BITBOARD rook_attacks_r0[64][256];
extern int rook_mobility_r0[64][256];
extern BITBOARD rook_attacks_rl90[64][256];
extern int rook_mobility_rl90[64][256];
extern BITBOARD bishop_attacks_rl45[64][256];
extern BITBOARD bishop_attacks_rr45[64][256];
extern int bishop_mobility_rl45[64][256];
extern int bishop_mobility_rr45[64][256];
extern BITBOARD mask_pawn_isolated[64];
extern BITBOARD mask_pawn_passed_w[64];
extern BITBOARD mask_pawn_passed_b[64];
extern BITBOARD mask_no_pawn_attacks_w[64];
extern BITBOARD mask_no_pawn_attacks_b[64];
extern BITBOARD mask_wk_4th, mask_wq_4th, mask_bk_4th, mask_bq_4th;
// RedCrafty_481_f.h, ř. 688-690
extern int last_mate_score;
extern int history_w[4096], history_b[4096];
extern PATH last_pv;
// RedCrafty_481_f.h, ř. 729-736
extern char initial_position[80];
extern int largest_positional_score;
extern int lazy_eval_cutoff;
extern int wtm;
extern int opening;
extern int middle_game;
extern int end_game;
extern int moves_out_of_book;
// RedCrafty_481_f.h, ř. 757-757
extern int move_number;
// RedCrafty_481_f.h, ř. 775-777
extern char buffer[512];
extern int nargs;
extern char *args[32];
// RedCrafty_481_f.h, ř. 923-923
extern const char xlate[15];
// RedCrafty_481_f.h, ř. 945-1059
extern int init_r90[64];
extern int init_l90[64];
extern int init_l45[64];
extern int init_ul45[64];
extern int init_r45[64];
extern int init_ur45[64];
extern int diagonal_length[64];
extern char bishop_shift_rl45[64];
extern char bishop_shift_rr45[64];

// --- funkce, v pořadí z RedCrafty_481_f.cpp ----------------------------------
void InitializeZeroMasks(void);
BITBOARD Mask(int arg1);
int FirstOne(BITBOARD arg1);
int LastOne(BITBOARD arg1);
void InitializeMasks(void);
unsigned int Random32(void);
BITBOARD Random64(void);
void InitializeRandomHash(void);
int InitializeFindAttacks(int square, int pieces, int length);
int PopCnt(BITBOARD a);
void InitializeAttackBoards(void);
void InitializePawnMasks(void);
int ReadParse(char *buffer, char *args[], const char *delims);
void SetChessBitBoards(SEARCH_POSITION *new_pos);
void InitializeChessBoard(SEARCH_POSITION *new_pos);
int Attacked(TREE *tree, int square, int wtm);
void SetBoard(SEARCH_POSITION *position, int nargs, char *args[], int special);
int LegalMove(TREE *tree, int ply, int wtm, int move);
int* GenerateCaptures(TREE *tree, int ply, int wtm, int *move);
int* GenerateNonCaptures(TREE *tree, int ply, int wtm, int *move);
void MakeMove(TREE *tree, int ply, int move, int wtm);
void MakeMoveRoot(TREE *tree, int move, int wtm);
void UnmakeMove(TREE *tree, int ply, int move, int wtm);

// --- port --------------------------------------------------------------------

// Náhrady výpisů do Memo1. Originál tu hlásil chybový stav do okna a jel dál;
// port jede dál taky, jen stav započítá. Testy kontrolují, že zůstaly nulové.
extern int hlaseni_side_not_on_move_in_check;   // SetBoard, ř. 1791
extern int hlaseni_makemove_captured_king;      // MakeMove, ř. 3847-3852
extern int hlaseni_unmakemove_captured_king;    // UnmakeMove, ř. 4364-4368

// Náhrada printf (krok C3/A): knihovna nic nevypisuje, jen počítá.
// Volá ji SetBoard (špatný FEN) a NextEvasion (chybná fáze).
extern int hlaseni_printf;
int tichy_printf(const char *fmt, ...);

// Náhrada FormActivate (ř. 59-76) a Initialize (ř. 1825-1884) v rozsahu,
// který potřebují pravidla: vyrobí strom local[0], naplní tabulky a postaví
// výchozí pozici. Hash tabulky nealokuje. Tabulky plní jen při prvním
// volání - Random32 má vnitřní stav a druhé volání by dalo jiné klíče.
void inicializuj();

// FEN -> tree->position[0] přes ReadParse a SetBoard, stejně jako to dělal
// originál s initial_position. SetBoard chybu neohlásí, jen ji vypíše
// a postaví výchozí pozici; proto se vrací false, když se deska po SetBoard
// neshoduje se zadáním. Pole 5 a 6 (půltahy, číslo tahu) SetBoard
// nečte - Rule50Moves je po něm vždy 0.
bool nastav_fen(const char *fen);

// Zápis pozice tree->pos s příznaky z tree->position[ply]. Originál nic
// takového nemá (SetBoard jen čte). Rošáda s hodnotou -4 (už provedená) se
// zapíše jako žádná.
std::string zapis_fen(TREE *tree, int ply, int wtm);

}  // namespace crafty
