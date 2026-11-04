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
//  Popis převodu je v crafty_core.h a v docs/POZNAMKY-CRAFTY.md. Těla funkcí
//  jsou řádek po řádku z RedCrafty_481_f.cpp; nad každou je rozsah řádků,
//  odkud pochází. Komentáře uvnitř jsou Hyattovy.
// ---------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "crafty_core.h"

namespace crafty {

// --- globály -----------------------------------------------------------------
// RedCrafty_481_f.h, ř. 550-550
TREE *local[1];

// RedCrafty_481_f.h, ř. 559-681
unsigned char  first_one_8bit[256];
unsigned char  last_one_8bit[256];
unsigned char  pop_cnt_8bit[256];
unsigned char  connected_passed[256];
unsigned char  file_spread[256];
signed char    is_outside[256][256];
signed char    is_outside_c[256][256];
BITBOARD mask_1;
BITBOARD mask_2;
BITBOARD mask_3;
BITBOARD mask_8;
BITBOARD mask_16;
BITBOARD mask_112;
BITBOARD mask_120;
BITBOARD mask_clear_entry;
BITBOARD       w_pawn_random[64];
BITBOARD       b_pawn_random[64];
BITBOARD       w_knight_random[64];
BITBOARD       b_knight_random[64];
BITBOARD       w_bishop_random[64];
BITBOARD       b_bishop_random[64];
BITBOARD       w_rook_random[64];
BITBOARD       b_rook_random[64];
BITBOARD       w_queen_random[64];
BITBOARD       b_queen_random[64];
BITBOARD       w_king_random[64];
BITBOARD       b_king_random[64];
BITBOARD       castle_random_w[2];
BITBOARD       castle_random_b[2];
BITBOARD       enpassant_random[65];
BITBOARD       wtm_random[2];
BITBOARD       clear_mask[65];
BITBOARD       clear_mask_rl90[65];
BITBOARD       clear_mask_rl45[65];
BITBOARD       clear_mask_rr45[65];
BITBOARD       set_mask[65];
BITBOARD       set_mask_rl90[65];
BITBOARD       set_mask_rl45[65];
BITBOARD       set_mask_rr45[65];
BITBOARD       rank_mask[8];
BITBOARD       file_mask[8];
BITBOARD       mask_pawn_duo[64];
BITBOARD       mask_pawn_protected_b[64];
BITBOARD       mask_pawn_protected_w[64];
BITBOARD       right_side_mask[8];
BITBOARD       left_side_mask[8];
BITBOARD       right_side_empty_mask[8];
BITBOARD       left_side_empty_mask[8];
BITBOARD       mask_efgh, mask_fgh, mask_abc,mask_abcd;
BITBOARD       mask_abs7_w, mask_abs7_b;
BITBOARD       mask_advance_2_w;
BITBOARD       mask_advance_2_b;
BITBOARD       mask_left_edge;
BITBOARD       mask_right_edge;
BITBOARD       mask_A7H7;
BITBOARD       mask_A2H2;
BITBOARD       mask_A3B3;
BITBOARD       mask_B3C3;
BITBOARD       mask_F3G3;
BITBOARD       mask_G3H3;
BITBOARD       mask_A6B6;
BITBOARD       mask_B6C6;
BITBOARD       mask_F6G6;
BITBOARD       mask_G6H6;
BITBOARD       mask_white_OO;
BITBOARD       mask_white_OOO;
BITBOARD       mask_black_OO;
BITBOARD       mask_black_OOO;
BITBOARD       stonewall_white;
BITBOARD       stonewall_black;
BITBOARD       e7_e6;
BITBOARD       e2_e3;
BITBOARD       closed_white;
BITBOARD       closed_black;
BITBOARD       mask_kr_trapped_w[3];
BITBOARD       mask_qr_trapped_w[3];
BITBOARD       mask_kr_trapped_b[3];
BITBOARD       mask_qr_trapped_b[3];
BITBOARD       good_bishop_kw;
BITBOARD       good_bishop_qw;
BITBOARD       good_bishop_kb;
BITBOARD       good_bishop_qb;
BITBOARD       light_squares;
BITBOARD       dark_squares;
BITBOARD       not_rook_pawns;
BITBOARD       plus1dir[65];
BITBOARD       plus7dir[65];
BITBOARD       plus8dir[65];
BITBOARD       plus9dir[65];
BITBOARD       minus1dir[65];
BITBOARD       minus7dir[65];
BITBOARD       minus8dir[65];
BITBOARD       minus9dir[65];
BITBOARD       mask_eptest[64];
BITBOARD       stalemate_sqs[64];
BITBOARD       edge_moves[64];
BITBOARD       w_pawn_attacks[64];
BITBOARD       b_pawn_attacks[64];
BITBOARD       knight_attacks[64];
BITBOARD       bishop_attacks[64];
BITBOARD       queen_attacks[64];
BITBOARD       rook_attacks[64];
BITBOARD       king_attacks[64];
BITBOARD       king_attacks_1[64];
BITBOARD       king_attacks_2[64];
BITBOARD       obstructed[64][64];
unsigned char first_one[65536];
unsigned char last_one[65536];
signed char    directions[64][64];
BITBOARD     rook_attacks_r0[64][256];
int          rook_mobility_r0[64][256];
BITBOARD     rook_attacks_rl90[64][256];
int          rook_mobility_rl90[64][256];
BITBOARD     bishop_attacks_rl45[64][256];
BITBOARD     bishop_attacks_rr45[64][256];
int          bishop_mobility_rl45[64][256];
int          bishop_mobility_rr45[64][256];
BITBOARD       mask_pawn_isolated[64];
BITBOARD       mask_pawn_passed_w[64];
BITBOARD       mask_pawn_passed_b[64];
BITBOARD       mask_no_pawn_attacks_w[64];
BITBOARD       mask_no_pawn_attacks_b[64];
BITBOARD       mask_wk_4th, mask_wq_4th, mask_bk_4th, mask_bq_4th;

// RedCrafty_481_f.h, ř. 688-690
int            last_mate_score;
int            history_w[4096], history_b[4096];
PATH           last_pv;

// RedCrafty_481_f.h, ř. 729-736
char      initial_position[80] =               {""};
int       largest_positional_score =            300;
int       lazy_eval_cutoff =                    200;
int       wtm =                                   1;
int       opening =                               1;
int       middle_game =                           0;
int       end_game =                              0;
int       moves_out_of_book =                     0;

// RedCrafty_481_f.h, ř. 757-757
int       move_number =                           1;

// RedCrafty_481_f.h, ř. 775-777
char           buffer[512];
int            nargs;
char           *args[32];

// RedCrafty_481_f.h, ř. 923-923
  const char xlate[15] = {'q','r','b',0,'k','n','p',0,'P','N','K',0,'B','R','Q'};

// RedCrafty_481_f.h, ř. 945-1059
int init_r90[64] = { 56, 48, 40, 32, 24, 16,  8,  0,
                     57, 49, 41, 33, 25, 17,  9,  1,
                     58, 50, 42, 34, 26, 18, 10,  2,
                     59, 51, 43, 35, 27, 19, 11,  3,
                     60, 52, 44, 36, 28, 20, 12,  4,
                     61, 53, 45, 37, 29, 21, 13,  5,
                     62, 54, 46, 38, 30, 22, 14,  6,
                     63, 55, 47, 39, 31, 23, 15,  7 };

int init_l90[64] = {  7, 15, 23, 31, 39, 47, 55, 63,
                      6, 14, 22, 30, 38, 46, 54, 62,
                      5, 13, 21, 29, 37, 45, 53, 61,
                      4, 12, 20, 28, 36, 44, 52, 60,
                      3, 11, 19, 27, 35, 43, 51, 59,
                      2, 10, 18, 26, 34, 42, 50, 58,
                      1,  9, 17, 25, 33, 41, 49, 57,
                      0,  8, 16, 24, 32, 40, 48, 56 };

int init_l45[64] = {                0,
                                  2,  5,
                                9, 14, 20,
                             27, 35,  1,  4,
                            8, 13, 19, 26, 34,
                         42,  3,  7, 12, 18, 25,
                       33, 41, 48,  6, 11, 17, 24,
                     32, 40, 47, 53, 10, 16, 23, 31,
                       39, 46, 52, 57, 15, 22, 30,
                         38, 45, 51, 56, 60, 21,
                           29, 37, 44, 50, 55,
                             59, 62, 28, 36,
                               43, 49, 54,
                                 58, 61,
                                   63 };

int init_ul45[64] = {               0,
                                  8,  1,
                               16,  9,  2,
                             24, 17, 10,  3,
                           32, 25, 18, 11,  4,
                         40, 33, 26, 19, 12,  5,
                       48, 41, 34, 27, 20, 13,  6,
                     56, 49, 42, 35, 28, 21, 14,  7,
                       57, 50, 43, 36, 29, 22, 15,
                         58, 51, 44, 37, 30, 23,
                           59, 52, 45, 38, 31,
                             60, 53, 46, 39,
                               61, 54, 47,
                                 62, 55,
                                   63 };

int init_r45[64] = {               28,
                                 21, 15,
                               10,  6,  3,
                              1,  0, 36, 29,
                           22, 16, 11,  7,  4,
                          2, 43, 37, 30, 23, 17,
                       12,  8,  5, 49, 44, 38, 31,
                     24, 18, 13,  9, 54, 50, 45, 39,
                       32, 25, 19, 14, 58, 55, 51,
                         46, 40, 33, 26, 20, 61,
                           59, 56, 52, 47, 41,
                             34, 27, 63, 62,
                               60, 57, 53,
                                 48, 42,
                                   35  };

int init_ur45[64] = {               7,
                                  6, 15,
                                5, 14, 23,
                              4, 13, 22, 31,
                            3, 12, 21, 30, 39,
                          2, 11, 20, 29, 38, 47,
                        1, 10, 19, 28, 37, 46, 55,
                      0,  9, 18, 27, 36, 45, 54, 63,
                        8, 17, 26, 35, 44, 53, 62,
                         16, 25, 34, 43, 52, 61,
                           24, 33, 42, 51, 60,
                             32, 41, 50, 59,
                               40, 49, 58,
                                 48, 57,
                                   56 };

int diagonal_length[64] = {         1,
                                  2,  2,
                                3,  3,  3,
                              4,  4,  4,  4,
                            5,  5,  5,  5,  5,
                          6,  6,  6,  6,  6,  6,
                        7,  7,  7,  7,  7,  7,  7,
                      8,  8,  8,  8,  8,  8,  8,  8,
                        7,  7,  7,  7,  7,  7,  7,
                          6,  6,  6,  6,  6,  6,
                            5,  5,  5,  5,  5,
                              4,  4,  4,  4,
                                3,  3,  3,
                                  2,  2,
                                    1 };

  char      bishop_shift_rl45[64] = { 63, 61, 58, 54, 49, 43, 36, 28,
                                      61, 58, 54, 49, 43, 36, 28, 21,
                                      58, 54, 49, 43, 36, 28, 21, 15,
                                      54, 49, 43, 36, 28, 21, 15, 10,
                                      49, 43, 36, 28, 21, 15, 10,  6,
                                      43, 36, 28, 21, 15, 10,  6,  3,
                                      36, 28, 21, 15, 10,  6,  3,  1,
                                      28, 21, 15, 10,  6,  3,  1,  0 };

  char      bishop_shift_rr45[64] = { 28, 36, 43, 49, 54, 58, 61, 63,
                                      21, 28, 36, 43, 49, 54, 58, 61,
                                      15, 21, 28, 36, 43, 49, 54, 58,
                                      10, 15, 21, 28, 36, 43, 49, 54,
                                       6, 10, 15, 21, 28, 36, 43, 49,
                                       3,  6, 10, 15, 21, 28, 36, 43,
                                       1,  3,  6, 10, 15, 21, 28, 36,
                                       0,  1,  3,  6, 10, 15, 21, 28 };

// --- funkce ------------------------------------------------------------------

// RedCrafty_481_f.cpp, ř. 219-328: InitializeZeroMasks
void InitializeZeroMasks(void)
{
  int i, j, dist, maxd;

//#if !defined(CRAY1) && !defined(USE_ASSEMBLY_B)

  int maskl,maskr;
  first_one[0]=16;
  last_one[0]=16;
  for (i=1;i<65536;i++){
    maskl=32768;
    for(j=0;j<16;j++){
      if ((maskl & i)){
        first_one[i]=j;
        break;
      }
      maskl=maskl>>1;
    }
    maskr=1;
    for(j=0;j<16;j++){
      if ((maskr & i)){
        last_one[i]=15-j;
        break;
      }
      maskr=maskr<<1;
    }
  }

//#endif


  first_one_8bit[0]=8;
  last_one_8bit[0]=8;
  pop_cnt_8bit[0]=0;
  connected_passed[0]=0;
  for (i=0;i<256;i++){
    pop_cnt_8bit[i]=0;
    connected_passed[i]=0;
    for (j=0;j<8;j++)
      if (i & (1<<j)) pop_cnt_8bit[i]++;
    for (j=0;j<8;j++){
      if (i & (1<<(7-j))){
        first_one_8bit[i]=j;
        break;
      }
    }
    for (j=7;j>=0;j--){
      if (i & (1<<(7-j))){
        last_one_8bit[i]=j;
        break;
      }
    }
    for (j=7;j>0;j--){
      if ((i & (3<<(7-j))) == (3<<(7-j))){
        connected_passed[i]=j;
        break;
      }
    }
    dist=0;
    maxd=0;
    for (j=1;j<256;j<<=1) if (i & j) break;
    for (;j<256;j<<=1) {
      if (j & i) {
        maxd=(maxd>dist) ? maxd : dist;
        dist=0;
      }
      else dist++;
    }
    file_spread[i]=maxd;
  }
/*
  is_outside[p][a] /is_outside_c[p][a] values:
  p=8 bit mask for passed pawns
  a=8 bit mask for all pawns on board
  p must have left-most or right-most bit set when compared to
  mask 'a'.  and this bit must be separated from the next bit
  by at least one file (ie the outside passed pawn is 2 files
  from the rest of the pawns, at least.  for is_outside_c[] the
  candidate passer only has to be outside by 1 file.

  ppsq = square that contains a (potential) passed pawn.
  psql = leftmost pawn, period.
  psqr = rightmost pawn, period.
  0 -> passed pawn is not 'outside'
  1 -> passed pawn is 'outside'
  2 -> passed pawn is 'outside' on both sides of board
*/

  for (i=0;i<256;i++) {
    for (j=0;j<256;j++) {
      int ppsq1, ppsq2, psql, psqr;
      is_outside[i][j]=0;
      is_outside_c[i][j]=0;
      ppsq1=first_one_8bit[i];
      if (ppsq1 < 8) {
        psql=first_one_8bit[j&(255-(128>>ppsq1))];
        if (ppsq1 < psql-1) is_outside[i][j]+=1;
        if (ppsq1 <= psql+1) is_outside_c[i][j]+=1;
      }
      ppsq2=last_one_8bit[i];
      if (ppsq2 < 8) {
        psqr=last_one_8bit[j&(255-(128>>ppsq2))];
        if (ppsq2 > psqr+1) is_outside[i][j]+=1;
        if (ppsq2 >= psqr-1) is_outside_c[i][j]+=1;
      }
      if (ppsq1==ppsq2 && is_outside[i][j]>0) is_outside[i][j]=1;
      if (ppsq1==ppsq2 && is_outside_c[i][j]>0) is_outside_c[i][j]=1;
    }
  }
}

// RedCrafty_481_f.cpp, ř. 331-340: Mask
BITBOARD Mask(int arg1) {
  BITBOARD i;
  i=(BITBOARD) -1;
  if (arg1 == 128)
    return(0);
  else if (arg1 > 64)
    return(i>>(arg1-64));
  else
    return(i<<(64-arg1));
}

// RedCrafty_481_f.cpp, ř. 342-350: FirstOne
int FirstOne(BITBOARD arg1) {
    if (arg1>>48)
      return (first_one[arg1>>48]);
    if ((arg1>>32)&65535)
      return (first_one[(arg1>>32)&65535]+16);
    if ((arg1>>16)&65535)
      return (first_one[(arg1>>16)&65535]+32);
    return (first_one[arg1&65535]+48);
}

// RedCrafty_481_f.cpp, ř. 352-360: LastOne
int LastOne(BITBOARD arg1) {
    if (arg1&65535)
      return (last_one[arg1&65535]+48);
    if ((arg1>>16)&65535)
      return (last_one[(arg1>>16)&65535]+32);
    if ((arg1>>32)&65535)
      return (last_one[(arg1>>32)&65535]+16);
    return (last_one[arg1>>48]);
}

// RedCrafty_481_f.cpp, ř. 362-524: InitializeMasks
void InitializeMasks(void) {
  int i, j;
/*
  specific masks to avoid Mask() procedure call if possible.
*/

    mask_1=Mask(1);
    mask_2=Mask(2);
    mask_3=Mask(3);
    mask_8=Mask(8);
    mask_16=Mask(16);
    mask_112=Mask(112);
    mask_120=Mask(120);

  mask_clear_entry=~(Mask(111) | Mask(2)>>3);
/*
  masks to set/clear a bit on a specific square
*/
  for (i=0;i<64;i++) {
    ClearMask(i)=~(mask_1>>i);
    ClearMaskRL45(i)=~(mask_1>>init_l45[i]);
    ClearMaskRR45(i)=~(mask_1>>init_r45[i]);
    ClearMaskRL90(i)=~(mask_1>>init_l90[i]);
    SetMask(i)=mask_1>>i;
    SetMaskRL45(i)=mask_1>>init_l45[i];
    SetMaskRR45(i)=mask_1>>init_r45[i];
    SetMaskRL90(i)=mask_1>>init_l90[i];
  }
  ClearMask(BAD_SQUARE)=0;
  ClearMaskRL45(BAD_SQUARE)=0;
  ClearMaskRR45(BAD_SQUARE)=0;
  ClearMaskRL90(BAD_SQUARE)=0;
  SetMask(BAD_SQUARE)=0;
  SetMaskRL45(BAD_SQUARE)=0;
  SetMaskRR45(BAD_SQUARE)=0;
  SetMaskRL90(BAD_SQUARE)=0;
/*
  masks to select bits on a specific rank or file
*/
  rank_mask[0]=mask_8;
  for (i=1;i<8;i++) rank_mask[i]=rank_mask[i-1]>>8;
  file_mask[FILEA]=mask_1;
  for (i=1;i<8;i++) file_mask[FILEA]=file_mask[FILEA] | file_mask[FILEA]>>8;
  for (i=1;i<8;i++) file_mask[i]=file_mask[i-1]>>1;
/*
  masks to determine if a pawn is protected by another pawn or not.
  also masks to detect "duos" (pawns side-by-side only).
*/
  for (i=8;i<56;i++) {
    if (File(i)>0 && File(i)<7) {
      mask_pawn_duo[i]=SetMask(i-1) | SetMask(i+1);
      mask_pawn_protected_w[i]=SetMask(i-1) | SetMask(i+1);
      if (i > 15) mask_pawn_protected_w[i]|=SetMask(i-7) | SetMask(i-9);
      mask_pawn_protected_b[i]=SetMask(i-1) | SetMask(i+1);
      if (i < 48) mask_pawn_protected_b[i]|=SetMask(i+7) | SetMask(i+9);
    }
    else if (File(i) == 0) {
      mask_pawn_duo[i]=SetMask(i+1);
      mask_pawn_protected_w[i]=SetMask(i+1);
      if (i > 15) mask_pawn_protected_w[i]|=SetMask(i-7);
      mask_pawn_protected_b[i]=SetMask(i+1);
      if (i < 48) mask_pawn_protected_b[i]|=SetMask(i+9);
    }
    else if (File(i) == 7) {
      mask_pawn_duo[i]=SetMask(i-1);
      mask_pawn_protected_w[i]=SetMask(i-1);
      if (i > 15) mask_pawn_protected_w[i]|=SetMask(i-9);
      mask_pawn_protected_b[i]=SetMask(i-1);
      if (i < 48) mask_pawn_protected_b[i]|=SetMask(i+7);
    }
  }
/*
  masks to select bits on either half of board
*/
  for (i=0;i<8;i++) {
    right_side_mask[i]=0;
    for (j=i+2;j<8;j++)
      right_side_mask[i]=right_side_mask[i] | file_mask[j];
    left_side_mask[i]=0;
    for (j=i-2;j>=0;j--)
      left_side_mask[i]=left_side_mask[i] | file_mask[j];
  }
  for (i=0;i<8;i++) {
    right_side_empty_mask[i]=0;
    for (j=i+1;j<8;j++)
      right_side_empty_mask[i]=right_side_empty_mask[i] | file_mask[j];
    left_side_empty_mask[i]=0;
    for (j=i-1;j>=0;j--)
      left_side_empty_mask[i]=left_side_empty_mask[i] | file_mask[j];
  }
  mask_fgh=file_mask[FILEF] | file_mask[FILEG] | file_mask[FILEH];
  mask_efgh=file_mask[FILEE] | file_mask[FILEF] |
            file_mask[FILEG] | file_mask[FILEH];
  mask_abc=file_mask[FILEA] | file_mask[FILEB] | file_mask[FILEC];
  mask_abcd=file_mask[FILEA] | file_mask[FILEB] |
            file_mask[FILEC] | file_mask[FILED];
  mask_kr_trapped_w[0]=SetMask(H2);
  mask_kr_trapped_w[1]=SetMask(H1) | SetMask(H2);
  mask_kr_trapped_w[2]=SetMask(G1) | SetMask(H1) | SetMask(H2);
  mask_qr_trapped_w[0]=SetMask(A2);
  mask_qr_trapped_w[1]=SetMask(A1) | SetMask(A2);
  mask_qr_trapped_w[2]=SetMask(A1) | SetMask(B1) | SetMask(A2);
  mask_kr_trapped_b[0]=SetMask(H7);
  mask_kr_trapped_b[1]=SetMask(H8) | SetMask(H7);
  mask_kr_trapped_b[2]=SetMask(H8) | SetMask(G8) | SetMask(H7);
  mask_qr_trapped_b[0]=SetMask(A7);
  mask_qr_trapped_b[1]=SetMask(A8) | SetMask(A7);
  mask_qr_trapped_b[2]=SetMask(A8) | SetMask(B8) | SetMask(A7);

  mask_abs7_w=rank_mask[RANK7] ^ (SetMask(H7) | SetMask(A7));
  mask_abs7_b=rank_mask[RANK2] ^ (SetMask(H2) | SetMask(A2));

  mask_A7H7=SetMask(A7) | SetMask(H7);
  mask_A2H2=SetMask(A2) | SetMask(H2);

  mask_A3B3=SetMask(A3) | SetMask(B3);
  mask_B3C3=SetMask(B3) | SetMask(C3);
  mask_F3G3=SetMask(F3) | SetMask(G3);
  mask_G3H3=SetMask(G3) | SetMask(H3);
  mask_A6B6=SetMask(A6) | SetMask(B6);
  mask_B6C6=SetMask(B6) | SetMask(C6);
  mask_F6G6=SetMask(F6) | SetMask(G6);
  mask_G6H6=SetMask(G6) | SetMask(H6);
  mask_white_OO=SetMask(F1) | SetMask(G1);
  mask_white_OOO=SetMask(B1) | SetMask(C1) | SetMask(D1);
  mask_black_OO=SetMask(F8) | SetMask(G8);
  mask_black_OOO=SetMask(B8) | SetMask(C8) | SetMask(D8);

  for (i=0;i<64;i++) {
    stalemate_sqs[i]=0;
    edge_moves[i]=0;
  }
  for (i=0;i<8;i++) {
    stalemate_sqs[i]|=SetMask(i+16);
    if (File(i) == 0) stalemate_sqs[i]|=SetMask(i+17);
    if (File(i) == 7) stalemate_sqs[i]|=SetMask(i+15);
    stalemate_sqs[i+56]|=SetMask(i+40);
    if (File(i) == 0) stalemate_sqs[i+56]|=SetMask(i+41);
    if (File(i) == 7) stalemate_sqs[i+56]|=SetMask(i+39);
  }
  for (i=0;i<64;i+=8) {
    stalemate_sqs[i]|=SetMask(i+2);
    if (Rank(i) == 0) stalemate_sqs[i]|=SetMask(i+10);
    if (Rank(i) == 7) stalemate_sqs[i]|=SetMask(i-6);
    stalemate_sqs[i+7]|=SetMask(i+5);
    if (Rank(i) == 0) stalemate_sqs[i+7]|=SetMask(i+13);
    if (Rank(i) == 7) stalemate_sqs[i+7]|=SetMask(i-3);
  }
  for (i=0;i<63;i++) {
    for (j=0;j<63;j++) {
      if (stalemate_sqs[i]&SetMask(j)) {
        if (Rank(i)==0 || Rank(i)==7) {
          if (File(i) > 0) edge_moves[i]|=SetMask(i-1);
          if (File(i) < 7) edge_moves[i]|=SetMask(i+1);
        }
        if (File(i)==0 || File(i)==7) {
          if (Rank(i) > 0) edge_moves[i]|=SetMask(i-8);
          if (Rank(i) < 7) edge_moves[i]|=SetMask(i+8);
        }
      }
    }
  }
}

// RedCrafty_481_f.cpp, ř. 527-571: Random32
/*
A 32 bit random number generator. An implementation in C of the algorithm given by
Knuth, the art of computer programming, vol. 2, pp. 26-27. We use e=32, so
we have to evaluate y(n) = y(n - 24) + y(n - 55) mod 2^32, which is implicitly
done by unsigned arithmetic.
*/

unsigned int Random32(void) {
  /*
  random numbers from Mathematica 2.0.
  SeedRandom = 1;
  Table[Random[Integer, {0, 2^32 - 1}]
  */
  static const unsigned long x[55] = {
    1410651636UL, 3012776752UL, 3497475623UL, 2892145026UL, 1571949714UL,
    3253082284UL, 3489895018UL, 387949491UL, 2597396737UL, 1981903553UL,
    3160251843UL, 129444464UL, 1851443344UL, 4156445905UL, 224604922UL,
    1455067070UL, 3953493484UL, 1460937157UL, 2528362617UL, 317430674UL,
    3229354360UL, 117491133UL, 832845075UL, 1961600170UL, 1321557429UL,
    747750121UL, 545747446UL, 810476036UL, 503334515UL, 4088144633UL,
    2824216555UL, 3738252341UL, 3493754131UL, 3672533954UL, 29494241UL,
    1180928407UL, 4213624418UL, 33062851UL, 3221315737UL, 1145213552UL,
    2957984897UL, 4078668503UL, 2262661702UL, 65478801UL, 2527208841UL,
    1960622036UL, 315685891UL, 1196037864UL, 804614524UL, 1421733266UL,
    2017105031UL, 3882325900UL, 810735053UL, 384606609UL, 2393861397UL };
  static int init = 1;
  static unsigned long y[55];
  static int j, k;
  unsigned long ul;

  if (init)
  {
    int i;

    init = 0;
    for (i = 0; i < 55; i++) y[i] = x[i];
    j = 24 - 1;
    k = 55 - 1;
  }

  ul = (y[k] += y[j]);
  if (--j < 0) j = 55 - 1;
  if (--k < 0) k = 55 - 1;
  return((unsigned int)ul);
}

// RedCrafty_481_f.cpp, ř. 573-581: Random64
BITBOARD Random64(void) {
  BITBOARD result;
  unsigned int r1, r2;

  r1=Random32();
  r2=Random32();
  result=r1 | (BITBOARD) r2<<32;
  return (result);
}

// RedCrafty_481_f.cpp, ř. 583-621: InitializeRandomHash
/*
********************************************************************************
*                                                                              *
*   InitializeRandomHash() is called to initialize the tables of random      *
*   numbers used to produce the incrementally-updated hash keys.  note that    *
*   this uses a treendom number generator rather than the C library one    *
*   since there is no uniformity in the number of bits returned by the         *
*   standard library routines, it varies from 16 bits to 64.                   *
*                                                                              *
********************************************************************************
*/
void InitializeRandomHash(void) {
  int i;
  for (i=0;i<64;i++) {
    w_pawn_random[i]=Random64();
    b_pawn_random[i]=Random64();
    w_knight_random[i]=Random64();
    b_knight_random[i]=Random64();
    w_bishop_random[i]=Random64();
    b_bishop_random[i]=Random64();
    w_rook_random[i]=Random64();
    b_rook_random[i]=Random64();
    w_queen_random[i]=Random64();
    b_queen_random[i]=Random64();
    w_king_random[i]=Random64();
    b_king_random[i]=Random64();
  }
  for (i=0;i<2;i++) {
    castle_random_w[i]=Random64();
    castle_random_b[i]=Random64();
  }
  enpassant_random[0]=0;
  for (i=1;i<65;i++) {
    enpassant_random[i]=Random64();
  }
  for (i=0;i<2;i++) {
    wtm_random[i]=Random64();
  }
}

// RedCrafty_481_f.cpp, ř. 623-671: InitializeFindAttacks
/*
********************************************************************************
*                                                                              *
*   Initlialize_Find_Attacks() is used to find the attacks from <square> that  *
*   exist on the 8-bit vector supplied as <pieces>.  <pieces> represents a     *
*   rank, file or diagonal, based on the rotated bit-boards.                   *
*                                                                              *
********************************************************************************
*/
int InitializeFindAttacks(int square, int pieces, int length) {
  int result, start;
  result=0;
/*
 ----------------------------------------------------------
|                                                          |
|   find attacks to left of <square>.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (square < 7) {
    start=1<<(square+1);
    while (start < 256) {
      result=result | start;
      if (pieces & start) break;
      start=start<<1;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   find attacks to left of <square>.                      |
|                                                          |
 ----------------------------------------------------------
*/
  if (square > 0) {
    start=1<<(square-1);
    while (start > 0) {
      result=result | start;
      if (pieces & start) break;
      start=start>>1;
    }
  }
/*
  printf("square=%d  pieces=%d\n",square,pieces);
  printf("result=%d\n",result);
  printf("length=%d  result=%d\n",length,result&(1<<length)-1);
*/
  return(result&((1<<length)-1));
}

// RedCrafty_481_f.cpp, ř. 673-681: PopCnt
int PopCnt(BITBOARD a) {
  int c=0;

  while(a) {
    c++;
    a &= a - 1;
  }
  return(c);
}

// RedCrafty_481_f.cpp, ř. 684-1027: InitializeAttackBoards
void InitializeAttackBoards(void) {

  int i, j, frank, ffile, trank, tfile;
  int sq, lastsq;
  static const int knightsq[8]={-17,-15,-10,-6,6,10,15,17};
  static const int bishopsq[4]={-9,-7,7,9};
  static const int rooksq[4]={-8,-1,1,8};
  BITBOARD sqs;
/*
   initialize pawn attack boards
*/
  for(i=0;i<64;i++) {
    w_pawn_attacks[i]=0;
    if (i < 56)
      for(j=2;j<4;j++) {
        sq=i+bishopsq[j];
        if((abs(sq/8-i/8)==1) &&
           (abs((sq&7) - (i&7))==1) &&
              (sq < 64) && (sq > -1))
          w_pawn_attacks[i]=w_pawn_attacks[i] | mask_1>>sq;
      }
    b_pawn_attacks[i]=0;
    if (i > 7)
      for(j=0;j<2;j++) {
        sq=i+bishopsq[j];
        if((abs(sq/8-i/8)==1) &&
           (abs((sq&7)-(i&7))==1) &&
              (sq < 64) && (sq > -1))
          b_pawn_attacks[i]=b_pawn_attacks[i] | mask_1>>sq;
      }
  }
/*
   initialize knight attack board
*/
  for(i=0;i<64;i++) {
    knight_attacks[i]=0;
    frank=i/8;
    ffile=i&7;
    for(j=0;j<8;j++) {
      sq=i+knightsq[j];
      if((sq < 0) || (sq > 63)) continue;
      trank=sq/8;
      tfile=sq&7;
      if((abs(frank-trank) > 2) ||
         (abs(ffile-tfile) > 2)) continue;
      knight_attacks[i]=knight_attacks[i] | mask_1>>sq;
    }
  }
/*
   initialize bishop/queen attack boards and masks
*/
  for(i=0;i<64;i++) {
    bishop_attacks[i]=0;
    for(j=0;j<4;j++) {
      sq=i;
      lastsq=sq;
      sq=sq+bishopsq[j];
      while((abs(sq/8-lastsq/8)==1) &&
            (abs((sq&7)-(lastsq&7))==1) &&
            (sq < 64) && (sq > -1)) {
        bishop_attacks[i]=bishop_attacks[i] | mask_1>>sq;
        queen_attacks[i]=queen_attacks[i] | mask_1>>sq;
        if(bishopsq[j]==7)
          plus7dir[i]=plus7dir[i] | mask_1>>sq;
        else if(bishopsq[j]==9)
          plus9dir[i]=plus9dir[i] | mask_1>>sq;
        else if(bishopsq[j]==-7)
          minus7dir[i]=minus7dir[i] | mask_1>>sq;
        else
          minus9dir[i]=minus9dir[i] | mask_1>>sq;
        lastsq=sq;
        sq=sq+bishopsq[j];
      }
    }
  }
  plus1dir[64]=0;
  plus7dir[64]=0;
  plus8dir[64]=0;
  plus9dir[64]=0;
  minus1dir[64]=0;
  minus7dir[64]=0;
  minus8dir[64]=0;
  minus9dir[64]=0;
/*
   initialize rook/queen attack boards
*/
  for(i=0;i<64;i++) {
    rook_attacks[i]=0;
    for(j=0;j<4;j++) {
      sq=i;
      lastsq=sq;
      sq=sq+rooksq[j];
      while((((abs(sq/8-lastsq/8)==1) &&
             (abs((sq&7)-(lastsq&7))==0)) ||
            ((abs(sq/8-lastsq/8)==0) &&
             (abs((sq&7)-(lastsq&7))==1))) &&
            (sq < 64) && (sq > -1)) {
        rook_attacks[i]=rook_attacks[i] | mask_1>>sq;
        queen_attacks[i]=queen_attacks[i] | mask_1>>sq;
        if(rooksq[j]==1)
          plus1dir[i]=plus1dir[i] | mask_1>>sq;
        else if(rooksq[j]==8)
          plus8dir[i]=plus8dir[i] | mask_1>>sq;
        else if(rooksq[j]==-1)
          minus1dir[i]=minus1dir[i] | mask_1>>sq;
        else
          minus8dir[i]=minus8dir[i] | mask_1>>sq;
        lastsq=sq;
        sq=sq+rooksq[j];
      }
    }
  }
/*
   initialize king attack board
*/
  for(i=0;i<64;i++) {
    king_attacks[i]=0;
    king_attacks_1[i]=0;
    king_attacks_2[i]=0;
    for (j=0;j<64;j++) {
      if (Distance(i,j) == 1)
        king_attacks[i]=king_attacks[i] | SetMask(j);
      if (Distance(i,j) <= 1)
        king_attacks_1[i]=king_attacks_1[i] | SetMask(j);
      if (Distance(i,j) <= 2)
        king_attacks_2[i]=king_attacks_2[i] | SetMask(j);
    }
  }
/*
  direction[sq1][sq2] gives the "move direction" to move from
  sq1 to sq2.  obstructed[sq1][sq2] gives a bit vector that indicates
  which squares must be unoccupied in order for <sq1> to attack <sq2>,
  assuming a sliding piece is involved.  to use this, you simply have
  to Or(obstructed[sq1][sq2],occupied_squares) and if the result is
  "0" then a sliding piece on sq1 would attack sq2 and vice-versa.
*/
  for (i=0;i<64;i++) {
    for (j=0;j<64;j++)
      obstructed[i][j]=(BITBOARD) -1;
    sqs=plus1dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=1;
      obstructed[i][j]=plus1dir[i] ^ plus1dir[j-1];
      staClear(j,sqs);
    }
    sqs=plus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=7;
      obstructed[i][j]=plus7dir[i] ^ plus7dir[j-7];
      staClear(j,sqs);
    }
    sqs=plus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=8;
      obstructed[i][j]=plus8dir[i] ^ plus8dir[j-8];
      staClear(j,sqs);
    }
    sqs=plus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=9;
      obstructed[i][j]=plus9dir[i] ^ plus9dir[j-9];
      staClear(j,sqs);
    }
    sqs=minus1dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-1;
      obstructed[i][j]=minus1dir[i] ^ minus1dir[j+1];
      staClear(j,sqs);
    }
    sqs=minus7dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-7;
      obstructed[i][j]=minus7dir[i] ^ minus7dir[j+7];
      staClear(j,sqs);
    }
    sqs=minus8dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-8;
      obstructed[i][j]=minus8dir[i] ^ minus8dir[j+8];
      staClear(j,sqs);
    }
    sqs=minus9dir[i];
    while (sqs) {
      j=FirstOne(sqs);
      directions[i][j]=-9;
      obstructed[i][j]=minus9dir[i] ^ minus9dir[j+9];
      staClear(j,sqs);
    }
  }
#if defined(COMPACT_ATTACKS)
  ComputeAttacksAndMobility();
#else
  {
    int diag_sq[64] = {                0,
                                     1,  0,
                                   2,  1,  0,
                                 3,  2,  1,  0,
                               4,  3,  2,  1,  0,
                             5,  4,  3,  2,  1,  0,
                           6,  5,  4,  3,  2,  1,  0,
                         7,  6,  5,  4,  3,  2,  1,  0,
                           6,  5,  4,  3,  2,  1,  0,
                             5,  4,  3,  2,  1,  0,
                               4,  3,  2,  1,  0,
                                 3,  2,  1,  0,
                                   2,  1,  0,
                                     1,  0,
                                       0 };

    int bias_rl45[64] = {              0,
                                     1,  1,
                                   3,  3,  3,
                                 6,  6,  6,  6,
                              10, 10, 10, 10, 10,
                            15, 15, 15, 15, 15, 15,
                          21, 21, 21, 21, 21, 21, 21,
                        28, 28, 28, 28, 28, 28, 28, 28,
                          36, 36, 36, 36, 36, 36, 36,
                            43, 43, 43, 43, 43, 43,
                              49, 49, 49, 49, 49,
                                54, 54, 54, 54,
                                  58, 58, 58,
                                    61, 61,
                                      63 };
    int square, pcs, attacks;
    int rsq, tsq;
    int mask;

/*
  initialize the rotated attack board that is based on the
  normal chess
*/
    for (square=0;square<64;square++) {
      for (i=0;i<256;i++) {
        rook_attacks_r0[square][i]=0;
        rook_mobility_r0[square][i]=0;
      }
      for (pcs=0;pcs<256;pcs++) {
        attacks=InitializeFindAttacks(7-File(square),pcs,8);
        while (attacks) {
          sq=first_one_8bit[attacks];
          rook_attacks_r0[square][pcs]=
            rook_attacks_r0[square][pcs] | SetMask((square&56)+sq);
          attacks=attacks&(~(1<<(7-sq)));
        }
        rook_mobility_r0[square][pcs]=PopCnt(rook_attacks_r0[square][pcs]);
      }
    }
/*
  initialize the rotated attack board that is based on one that
  rotated left 90 degrees (which lines up a file horizontally,
  rather than its normal vertical orientation.)
*/
    for (square=0;square<64;square++) {
      for (i=0;i<256;i++) {
        rook_attacks_rl90[square][i]=0;
        rook_mobility_rl90[square][i]=0;
      }
      for (pcs=0;pcs<256;pcs++) {
        attacks=InitializeFindAttacks(Rank(square),pcs,8);
        while (attacks) {
          sq=first_one_8bit[attacks];
          rook_attacks_rl90[square][pcs]=
            rook_attacks_rl90[square][pcs] |
               SetMask(init_r90[((square&7)<<3)+sq]);
          attacks=attacks&(~(1<<(7-sq)));
        }
        rook_mobility_rl90[square][pcs]=PopCnt(rook_attacks_rl90[square][pcs]);
      }
    }
/*
  initialize the rotated attack board that is based on one that is
  rotated left 45 degrees (which lines up the (a8-h1) diagonal
  horizontally.
*/
    for (square=0;square<64;square++) {
      for (i=0;i<256;i++) {
        bishop_attacks_rl45[square][i]=0;
        bishop_mobility_rl45[square][i]=0;
      }
      for (pcs=0;pcs<(1<<diagonal_length[init_l45[square]]);pcs++) {
        rsq=init_l45[square];
        tsq=diag_sq[rsq];
        attacks=InitializeFindAttacks(tsq,pcs,diagonal_length[rsq])<<
                          (8-diagonal_length[rsq]);
        while (attacks) {
          sq=first_one_8bit[attacks];
          bishop_attacks_rl45[square][pcs]=
            bishop_attacks_rl45[square][pcs] |
               SetMask(init_ul45[sq+bias_rl45[rsq]]);
          attacks=attacks&(~(1<<(7-sq)));
        }
      }
      mask=(1<<diagonal_length[init_l45[square]])-1;
      for (pcs=0;pcs<256;pcs++) {
        if ((pcs&mask) != pcs)
          bishop_attacks_rl45[square][pcs]=
            bishop_attacks_rl45[square][pcs&mask];
        bishop_mobility_rl45[square][pcs]=
          PopCnt(bishop_attacks_rl45[square][pcs]);
      }
    }
/*
  initialize the rotated attack board that is based on one that is
  rotated right 45 degrees (which lines up the (a1-h8) diagonal
  horizontally,
*/
    for (square=0;square<64;square++) {
      for (i=0;i<256;i++) {
        bishop_attacks_rr45[square][i]=0;
        bishop_mobility_rr45[square][i]=0;
      }
      for (pcs=0;pcs<(1<<diagonal_length[init_r45[square]]);pcs++) {
        rsq=init_r45[square];
        tsq=diag_sq[rsq];
        attacks=InitializeFindAttacks(tsq,pcs,diagonal_length[rsq])<<
                          (8-diagonal_length[rsq]);
        while (attacks) {
          sq=first_one_8bit[attacks];
          bishop_attacks_rr45[square][pcs]=
            bishop_attacks_rr45[square][pcs] |
               SetMask(init_ur45[sq+bias_rl45[rsq]]);
          attacks=attacks&(~(1<<(7-sq)));
        }
      }
      mask=(1<<diagonal_length[init_r45[square]])-1;
      for (pcs=0;pcs<256;pcs++) {
        if ((pcs&mask) != pcs)
          bishop_attacks_rr45[square][pcs]=
            bishop_attacks_rr45[square][pcs&mask];
        bishop_mobility_rr45[square][pcs]=
          PopCnt(bishop_attacks_rr45[square][pcs]);
      }
    }
  }
#endif
}

// RedCrafty_481_f.cpp, ř. 1029-1152: InitializePawnMasks
void InitializePawnMasks(void) {
  int i;
  BITBOARD m1,m2;
/*
    initialize isolated pawn masks, which are nothing more than 1's on
    the files adjacent to the pawn file.
*/
  for (i=0;i<64;i++) {
    if (!(i&7)) mask_pawn_isolated[i]=file_mask[(i&7)+1];
    else if ((i&7) == 7) mask_pawn_isolated[i]=file_mask[(i&7)-1];
    else mask_pawn_isolated[i]=file_mask[(i&7)-1] | file_mask[(i&7)+1];
  }
/*
    initialize passed pawn masks, which are nothing more than 1's on
    the pawn's file and the adjacent files, but only on ranks that are
    in "front" of the pawn.
*/
  for (i=0;i<64;i++) {
    if (!(i&7)) {
      mask_pawn_passed_w[i]=plus8dir[i] | plus8dir[i+1];
      mask_pawn_passed_b[i]=minus8dir[i] | minus8dir[i+1];
    }
    else if ((i&7) == 7) {
      mask_pawn_passed_w[i]=plus8dir[i-1] | plus8dir[i];
      mask_pawn_passed_b[i]=minus8dir[i-1] | minus8dir[i];
    }
    else {
      mask_pawn_passed_w[i]=plus8dir[i-1] | plus8dir[i] | plus8dir[i+1];
      mask_pawn_passed_b[i]=minus8dir[i-1] | minus8dir[i] | minus8dir[i+1];
    }
  }
/*
    these masks are used to determine if the other side has any pawns
    that can attack [square].
*/
  for (i=8;i<56;i++) {
    if (!(i&7)) {
      mask_no_pawn_attacks_w[i]=minus8dir[i+1];
      mask_no_pawn_attacks_b[i]=plus8dir[i+1];
    }
    else if ((i&7) == 7) {
      mask_no_pawn_attacks_w[i]=minus8dir[i-1];
      mask_no_pawn_attacks_b[i]=plus8dir[i-1];
    }
    else {
      mask_no_pawn_attacks_w[i]=minus8dir[i-1] | minus8dir[i+1];
      mask_no_pawn_attacks_b[i]=plus8dir[i+1] | plus8dir[i-1];
    }
  }
/*
    enpassant pawns are on either file adjacent to the current file, and
    on the same rank.
*/
  for (i=0;i<64;i++) mask_eptest[i]=0;
  for (i=25;i<31;i++) mask_eptest[i]=SetMask(i-1) | SetMask(i+1);
  for (i=33;i<39;i++) mask_eptest[i]=SetMask(i-1) | SetMask(i+1);
  mask_eptest[A4]=SetMask(B4);
  mask_eptest[H4]=SetMask(G4);
  mask_eptest[A5]=SetMask(B5);
  mask_eptest[H5]=SetMask(G5);
/*
  these two masks have 1's on dark squares and light squares
  to test to see if pawns/bishops are on them.
*/
  m1=Mask(1);
  m2=m1>>1;
  for (i=1;i<4;i++) {
    m1=m1 | m1>>2;
    m2=m2 | m2>>2;
  }
  for (i=0;i<64;i+=8) {
    if ((i/8)&1) {
      dark_squares=dark_squares | m2>>i;
      light_squares=light_squares | m1>>i;
    }
    else {
      dark_squares=dark_squares | m1>>i;
      light_squares=light_squares | m2>>i;
    }
  }
/*
  this mask is used to detect that one side has pawns, but all
  are rook pawns.
*/
  not_rook_pawns=file_mask[FILEB] | file_mask[FILEC] |
                 file_mask[FILED] | file_mask[FILEE] |
                 file_mask[FILEF] | file_mask[FILEG];
/*
  these two masks have 1's on everywhere but the left or right
  files, used to prevent pawns from capturing off the edge of
  the board and wrapping around.rom capturing off the edge of
*/
  mask_left_edge=~file_mask[FILEA];
  mask_right_edge=~file_mask[FILEH];
  mask_advance_2_w=rank_mask[RANK3];
  mask_advance_2_b=rank_mask[RANK6];
/*
  these masks have 1's on the squares where it is useful to have a bishop
  when the b or g pawn is missing or pushed one square.
*/
  good_bishop_kw=SetMask(F1) | SetMask(H1) | SetMask(G2);
  good_bishop_qw=SetMask(A1) | SetMask(C1) | SetMask(B2);
  good_bishop_kb=SetMask(G7) | SetMask(F8) | SetMask(H8);
  good_bishop_qb=SetMask(B7) | SetMask(A8) | SetMask(C8);
/*
  these masks are used to detect that opponent pawns are getting very
  close to the king.
*/
  mask_wq_4th=SetMask(A4) | SetMask(B4) | SetMask(C4);
  mask_wk_4th=SetMask(F4) | SetMask(G4) | SetMask(H4);
  mask_bq_4th=SetMask(A5) | SetMask(B5) | SetMask(C5);
  mask_bk_4th=SetMask(F5) | SetMask(G5) | SetMask(H5);

/*
  these masks are used to detect that the opponent is trying to set up
  a stonewall type pawn formation.
*/
  stonewall_white=SetMask(D4) | SetMask(F4);
  closed_white=SetMask(E4) | SetMask(D3) | SetMask(C4);
  e2_e3=SetMask(E2) | SetMask(E3);
  stonewall_black=SetMask(D5) | SetMask(F5);
  closed_black=SetMask(E5) | SetMask(D6) | SetMask(C5);
  e7_e6=SetMask(E7) | SetMask(E6);
}

// RedCrafty_481_f.cpp, ř. 1225-1250: ReadParse
/* last modified 05/06/97 */
/*
********************************************************************************
*                                                                              *
*   ReadParse() takes one complete command-line, and breaks it up into tokens. *
*   common delimiters are used, such as " ", ",", "/" and ";", any of which    *
*   delimit fields.                                                            *
*                                                                              *
********************************************************************************
*/
int ReadParse(char *buffer, char *args[], const char *delims) {
  char *next, tbuffer[512];
  int nargs;

  strcpy(tbuffer,buffer);
  for (nargs=0;nargs<16;nargs++) *(args[nargs])=0;
  next=strtok(tbuffer,delims);
  if (!next) return(0);
  strcpy(args[0],next);
  for (nargs=1;nargs<32;nargs++) {
    next=strtok(0,delims);
    if (!next) break;
    strcpy(args[nargs],next);
  }
  return(nargs);
}

// RedCrafty_481_f.cpp, ř. 1252-1450: SetChessBitBoards
void SetChessBitBoards(SEARCH_POSITION *new_pos) {
  int i;
  TREE * const tree=local[0];
  tree->pos.hash_key=0;
  tree->pos.pawn_hash_key=0;
/*
   place pawns
*/
  tree->pos.w_pawn=0;
  tree->pos.b_pawn=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i]==pawn) {
      tree->pos.w_pawn=tree->pos.w_pawn | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ w_pawn_random[i];
      tree->pos.pawn_hash_key=tree->pos.pawn_hash_key^w_pawn_random[i];
    }
    if(tree->pos.board[i]==-pawn) {
      tree->pos.b_pawn=tree->pos.b_pawn | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ b_pawn_random[i];
      tree->pos.pawn_hash_key=tree->pos.pawn_hash_key^b_pawn_random[i];
    }
  }
/*
   place knights
*/
  tree->pos.w_knight=0;
  tree->pos.b_knight=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == knight) {
      tree->pos.w_knight=tree->pos.w_knight | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ w_knight_random[i];
    }
    if(tree->pos.board[i] == -knight) {
      tree->pos.b_knight=tree->pos.b_knight | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ b_knight_random[i];
    }
  }
/*
   place bishops
*/
  tree->pos.w_bishop=0;
  tree->pos.b_bishop=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == bishop) {
      tree->pos.w_bishop=tree->pos.w_bishop | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ w_bishop_random[i];
    }
    if(tree->pos.board[i] == -bishop) {
      tree->pos.b_bishop=tree->pos.b_bishop | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ b_bishop_random[i];
    }
  }
/*
   place rooks
*/
  tree->pos.w_rook=0;
  tree->pos.b_rook=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == rook) {
      tree->pos.w_rook=tree->pos.w_rook | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ w_rook_random[i];
    }
    if(tree->pos.board[i] == -rook) {
      tree->pos.b_rook=tree->pos.b_rook | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ b_rook_random[i];
    }
  }
/*
   place queens
*/
  tree->pos.w_queen=0;
  tree->pos.b_queen=0;
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == queen) {
      tree->pos.w_queen=tree->pos.w_queen | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ w_queen_random[i];
    }
    if(tree->pos.board[i] == -queen) {
      tree->pos.b_queen=tree->pos.b_queen | SetMask(i);
      tree->pos.hash_key=tree->pos.hash_key ^ b_queen_random[i];
    }
  }
/*
   place kings
*/
  for (i=0;i<64;i++) {
    if(tree->pos.board[i] == king) {
      tree->pos.white_king=i;
      tree->pos.hash_key=tree->pos.hash_key ^ w_king_random[i];
    }
    if(tree->pos.board[i] == -king) {
      tree->pos.black_king=i;
      tree->pos.hash_key=tree->pos.hash_key ^ b_king_random[i];
    }
  }
  if (new_pos->enpassant_target)
    HashEP(new_pos->enpassant_target,tree->pos.hash_key);
  if (!(new_pos->w_castle&1)) HashCastleW(0,tree->pos.hash_key);
  if (!(new_pos->w_castle&2)) HashCastleW(1,tree->pos.hash_key);
  if (!(new_pos->b_castle&1)) HashCastleB(0,tree->pos.hash_key);
  if (!(new_pos->b_castle&2)) HashCastleB(1,tree->pos.hash_key);
/*
   initialize combination boards that show multiple pieces.
*/
  tree->pos.bishops_queens=tree->pos.w_bishop | tree->pos.w_queen |
                           tree->pos.b_bishop | tree->pos.b_queen;
  tree->pos.rooks_queens=tree->pos.w_rook | tree->pos.w_queen |
                         tree->pos.b_rook | tree->pos.b_queen;
  tree->pos.w_occupied=tree->pos.w_pawn | tree->pos.w_knight |
                       tree->pos.w_bishop | tree->pos.w_rook |
                       tree->pos.w_queen | SetMask(tree->pos.white_king);
  tree->pos.b_occupied=tree->pos.b_pawn | tree->pos.b_knight |
                       tree->pos.b_bishop | tree->pos.b_rook |
                       tree->pos.b_queen | SetMask(tree->pos.black_king);
/*
  now initialize rotated occupied bitboards.
*/
  tree->pos.occupied_rl90=0;
  tree->pos.occupied_rl45=0;
  tree->pos.occupied_rr45=0;
  for (i=0;i<64;i++) {
    if (tree->pos.board[i]) {
      tree->pos.occupied_rl90=tree->pos.occupied_rl90 | SetMaskRL90(i);
      tree->pos.occupied_rl45=tree->pos.occupied_rl45 | SetMaskRL45(i);
      tree->pos.occupied_rr45=tree->pos.occupied_rr45 | SetMaskRR45(i);
    }
  }
/*
   initialize black/white piece counts.
*/
  tree->pos.white_pieces=0;
  tree->pos.white_majors=0;
  tree->pos.white_minors=0;
  tree->pos.white_pawns=0;
  tree->pos.black_pieces=0;
  tree->pos.black_majors=0;
  tree->pos.black_minors=0;
  tree->pos.black_pawns=0;
  tree->pos.material_evaluation=0;
  for (i=0;i<64;i++) {
    switch (tree->pos.board[i]) {
      case pawn:
        tree->pos.material_evaluation+=PAWN_VALUE;
        tree->pos.white_pawns+=pawn_v;
        break;
      case knight:
        tree->pos.material_evaluation+=KNIGHT_VALUE;
        tree->pos.white_pieces+=knight_v;
        tree->pos.white_minors++;
        break;
      case bishop:
        tree->pos.material_evaluation+=BISHOP_VALUE;
        tree->pos.white_pieces+=bishop_v;
        tree->pos.white_minors++;
        break;
      case rook:
        tree->pos.material_evaluation+=ROOK_VALUE;
        tree->pos.white_pieces+=rook_v;
        tree->pos.white_majors++;
        break;
      case queen:
        tree->pos.material_evaluation+=QUEEN_VALUE;
        tree->pos.white_pieces+=queen_v;
        tree->pos.white_majors+=2;
        break;
      case -pawn:
        tree->pos.material_evaluation-=PAWN_VALUE;
        tree->pos.black_pawns+=pawn_v;
        break;
      case -knight:
        tree->pos.material_evaluation-=KNIGHT_VALUE;
        tree->pos.black_pieces+=knight_v;
        tree->pos.black_minors++;
        break;
      case -bishop:
        tree->pos.material_evaluation-=BISHOP_VALUE;
        tree->pos.black_pieces+=bishop_v;
        tree->pos.black_minors++;
        break;
      case -rook:
        tree->pos.material_evaluation-=ROOK_VALUE;
        tree->pos.black_pieces+=rook_v;
        tree->pos.black_majors++;
        break;
      case -queen:
        tree->pos.material_evaluation-=QUEEN_VALUE;
        tree->pos.black_pieces+=queen_v;
        tree->pos.black_majors+=2;
        break;
      default:
        ;
    }
  }
  TotalPieces=PopCnt(Occupied);
  if (new_pos == &tree->position[0]) {
    tree->rephead_b=tree->replist_b;
    tree->rephead_w=tree->replist_w;
  }
}

// RedCrafty_481_f.cpp, ř. 1453-1530: InitializeChessBoard
void InitializeChessBoard(SEARCH_POSITION *new_pos) {
  int i;
  TREE * const tree=local[0];

  if (strlen(initial_position)) {
    static char a1[80], a2[16], a3[16], a4[16], a5[16];
    static char *args[16]={a1,a2,a3,a4,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5,a5};
    int nargs;

    nargs=ReadParse(initial_position,args," ;");
    SetBoard(new_pos,nargs,args,1);
  }
  else {
    for(i=0;i<64;i++) tree->pos.board[i]=none;
    new_pos->rule_50_moves=0;
    opening=1;
    middle_game=0;
    end_game=0;
    lazy_eval_cutoff=200;
    largest_positional_score=300;
    wtm=1;
/*
   place pawns
*/
    for (i=0;i<8;i++) {
      tree->pos.board[i+8]=pawn;
      tree->pos.board[i+48]=-pawn;
    }
/*
   place knights
*/
    tree->pos.board[B1]=knight;
    tree->pos.board[G1]=knight;
    tree->pos.board[B8]=-knight;
    tree->pos.board[G8]=-knight;
/*
   place bishops
*/
    tree->pos.board[C1]=bishop;
    tree->pos.board[F1]=bishop;
    tree->pos.board[C8]=-bishop;
    tree->pos.board[F8]=-bishop;
/*
   place rooks
*/
    tree->pos.board[A1]=rook;
    tree->pos.board[H1]=rook;
    tree->pos.board[A8]=-rook;
    tree->pos.board[H8]=-rook;
/*
   place queens
*/
    tree->pos.board[D1]=queen;
    tree->pos.board[D8]=-queen;
/*
   place kings
*/
    tree->pos.board[E1]=king;
    tree->pos.board[E8]=-king;
/*
   initialize castling status so all castling is legal.
*/
    new_pos->w_castle=3;
    new_pos->b_castle=3;
/*
   initialize 50 move counter.
*/
    new_pos->rule_50_moves=0;
/*
   initialize enpassant status.
*/
    new_pos->enpassant_target=0;
/*
   now, set the bit-boards.
*/
    SetChessBitBoards(new_pos);
  }
}

// RedCrafty_481_f.cpp, ř. 1532-1559: Attacked
/* last modified 06/26/99 */
/*
********************************************************************************
*                                                                              *
*   Attacked() is used to determine if <square> is attacked by "wtm".  the     *
*   algorithm is simple, and is based on the AttacksTo() algorithm, but,       *
*   rather than returning a bitmap of squares attacking <square> it returns a  *
*   "1" as soon as it finds anything that attacks <square>.                    *
*                                                                              *
******************************************************************************** */
int Attacked(TREE *tree, int square, int wtm) {
  if (wtm) {
    if (b_pawn_attacks[square] & WhitePawns) return(1);
    if (knight_attacks[square] & WhiteKnights) return(1);
    if (AttacksBishop(square) & BishopsQueens & WhitePieces) return(1);
    if (AttacksRook(square) & RooksQueens & WhitePieces) return(1);
    if (king_attacks[square] & WhiteKing) return(1);
    return(0);
  }
  else {
    if (w_pawn_attacks[square] & BlackPawns) return(1);
    if (knight_attacks[square] & BlackKnights) return(1);
    if (AttacksBishop(square) & BishopsQueens & BlackPieces) return(1);
    if (AttacksRook(square) & RooksQueens & BlackPieces) return(1);
    if (king_attacks[square] & BlackKing) return(1);
    return(0);
  }
}

// RedCrafty_481_f.cpp, ř. 1562-1794: SetBoard
/* last modified 11/20/01 */
/*
********************************************************************************
*                                                                              *
*   SetBoard() is used to set up the board in any position desired.  it uses   *
*   a forsythe-like string of characters to describe the board position.       *
*                                                                              *
*   the standard piece codes p,n,b,r,q,k are used to denote the type of piece  *
*   on a square, upper/lower case are used to indicate the side (program/opp.) *
*   of the piece.                                                              *
*                                                                              *
*   the pieces are entered with the rank on the program's side of the board    *
*   entered first, and the rank on the opponent's side entered last.  to enter *
*   empty squares, use a number between 1 and 8 to indicate how many adjacent  *
*   squares are empty.  use a / to terminate each rank after all of the pieces *
*   for that rank have been entered.                                           *
*                                                                              *
*   the following input will setup the board position that given below:        *
*                                                                              *
*         K2R/PPP////q/5ppp/7k/ b - -                                          *
*                                                                              *
*   this assumes that k represents a white king and -q represents a black      *
*   queen.                                                                     *
*                                                                              *
*                          k  *  *  r  *  *  *  *                              *
*                          p  p  p  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                          *  *  *  *  *  *  *  *                              *
*                         -q  *  *  *  *  *  *  *                              *
*                          *  *  *  *  * -p -p -p                              *
*                          *  *  *  *  *  *  * -k                              *
*                                                                              *
*   the field after the final "/" should be either b or w to indicate which    *
*   side is "on move."  after this side-to-move field any of the following     *
*   characters can appear to indicate the following:  KQ: white can castle     *
*   kingside/queenside/both;  kq: same for black;  a1-h8: indicates the        *
*   square occupied by a pawn that can be captured en passant.                 *
*                                                                              *
******************************************************************************** */
void SetBoard(SEARCH_POSITION *position, int nargs, char *args[], int special) {
  int twtm, i, match, num, pos, square, tboard[64];
  int bcastle, ep, wcastle;
  char input[80];
  static const char bdinfo[] = {'q','r','b','*','k','n','p','*','P','N',
                                'K','*','B','R', 'Q','*','1','2','3','4',
                                '5','6','7','8','/'};
  static const char status[13]={'K','Q','k','q','a','b','c','d','e','f','g',
                                'h',' '};
  int whichsq;
  static const int firstsq[8]={56,48,40,32,24,16,8,0};
  TREE * const tree=local[0];

  if (special)
    strcpy(input,initial_position);
  else
    strcpy(input,args[0]);
  for (i=0;i<64;i++) tboard[i]=0;
/*
 ----------------------------------------------------------
|                                                          |
|   scan the input string searching for pieces, numbers    |
|   [empty squares], slashes [end-of-rank] and a blank     |
|   [end of board, start of castle status].                |
|                                                          |
 ----------------------------------------------------------
*/
  whichsq=0;
  square=firstsq[whichsq];
  num=0;
  for (pos=0;pos<(int) strlen(args[0]);pos++) {
    for (match=0;match<25 && args[0][pos]!=bdinfo[match];match++);
    if (match > 24) break;
/*
   "/" -> end of this rank.
*/
    else if (match == 24) {
      num=0;
      if (whichsq > 6) break;
      square=firstsq[++whichsq];
    }
/*
   "1-8" -> empty squares.
*/
    else if (match >= 16) {
      num+=match-15;
      square+=match-15;
      if (num > 8) {
        tichy_printf("more than 8 squares on one rank\n");
        return;
      }
      continue;
    }
/*
   piece codes.
*/
    else {
      if (++num > 8) {
        tichy_printf("more than 8 squares on one rank\n");
        return;
      }
      tboard[square++]=match-7;
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   now extract (a) side to move [w/b], (b) castle status  |
|   [KkQq for white/black king-side ok, white/black queen- |
|   side ok], (c) enpassant target square.                 |
|                                                          |
 ----------------------------------------------------------
*/
  twtm=0;
  ep=0;
  wcastle=0;
  bcastle=0;
/*
 ----------------------------------------------------------
|                                                          |
|   side to move.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  if (args[1][0] == 'w') twtm=1;
  else if (args[1][0] == 'b') twtm=0;
  else tichy_printf("side to move is bad\n");
/*
 ----------------------------------------------------------
|                                                          |
|   castling/enpassant status.                             |
|                                                          |
 ----------------------------------------------------------
*/
  if (nargs>2 && strlen(args[2])) {
    if (strcmp(args[2],"-")) {
      for (pos=0;pos<(int) strlen(args[2]);pos++) {
        for (match=0;(match<13) && (args[2][pos]!=status[match]);match++);
        if (match == 0) wcastle+=1;
        else if (match == 1) wcastle+=2;
        else if (match == 2) bcastle+=1;
        else if (match == 3) bcastle+=2;
        else if (args[2][0]!='-') tichy_printf("castling status is bad.\n");
      }
    }
  }
  if (nargs>3 && strlen(args[3])) {
    if (strcmp(args[3],"-")) {
      if (args[3][0]>='a' && args[3][0]<='h' &&
          args[3][1]>'0' && args[3][1]<'9') {
        ep=(args[3][1]-'1')*8+args[3][0]-'a';
      }
      else if (args[3][0]!='-') tichy_printf("enpassant status is bad.\n");
    }
  }
  for (i=0;i<64;i++) PcOnSq(i)=tboard[i];
  position->w_castle=wcastle;
  position->b_castle=bcastle;
  position->enpassant_target=0;
  if (ep) {
    if (Rank(ep) == RANK6) {
      if (PcOnSq(ep-8) != -pawn) ep=0;
    }
    else if (Rank(ep) == RANK3) {
      if (PcOnSq(ep+8) != pawn) ep=0;
    }
    else ep=0;
    if (!ep) {
      tichy_printf("enpassant status is bad (must be on 3rd/6th rank only.\n");
      ep=0;
    }
    position->enpassant_target=ep;
  }
  position->rule_50_moves=0;
/*
 ----------------------------------------------------------
|                                                          |
|   now check the castling status and enpassant status to  |
|   make sure that the board is in a state that matches    |
|   these.                                                 |
|                                                          |
 ----------------------------------------------------------
*/
  if (((position->w_castle & 2) && (PcOnSq(A1) != rook)) ||
      ((position->w_castle & 1) && (PcOnSq(H1) != rook)) ||
      ((position->b_castle & 2) && (PcOnSq(A8) != -rook)) ||
      ((position->b_castle & 1) && (PcOnSq(H8) != -rook))) {
    tichy_printf("ERROR-- castling status does not match board position\n");
    InitializeChessBoard(&tree->position[0]);
  }
  if ((twtm && position->enpassant_target &&
       (PcOnSq(position->enpassant_target+8) != -pawn) &&
       (PcOnSq(position->enpassant_target-7) != pawn) &&
       (PcOnSq(position->enpassant_target-9) != pawn)) ||
      (ChangeSide(twtm) && position->enpassant_target &&
       (PcOnSq(position->enpassant_target-8) != pawn) &&
       (PcOnSq(position->enpassant_target+7) != -pawn) &&
       (PcOnSq(position->enpassant_target+9) != -pawn))) {
    position->enpassant_target=0;
  }
  SetChessBitBoards(position);
  tree->rephead_b=tree->replist_b;
  tree->rephead_w=tree->replist_w;
  if (twtm) *tree->rephead_w++=HashKey;
  else *tree->rephead_b++=HashKey;
  position->rule_50_moves=0;
  if (!special) {
    last_mate_score=0;
    for (i=0;i<4096;i++) {
      history_w[i]=0;
      history_b[i]=0;
    }
    for (i=0;i<MAXPLY;i++) {
      tree->killers[i].move1=0;
      tree->killers[i].move2=0;
    }
    last_pv.pathd=0;
    last_pv.pathl=0;
    tree->pv[0].pathd=0;
    tree->pv[0].pathl=0;
    moves_out_of_book=0;
    lazy_eval_cutoff=200;
    largest_positional_score=300;
    opening=0;
    middle_game=1;
    end_game=0;
    wtm=twtm;
  }
  if (Check(!wtm)) {
    hlaseni_side_not_on_move_in_check++;  // port: bylo Memo1, ř. 1791
    InitializeChessBoard(&tree->position[0]);
  }
}

// RedCrafty_481_f.cpp, ř. 1887-1915: LegalMove
/* last modified 07/07/98 */
/*
********************************************************************************
*                                                                              *
*   LegalMove() tests a move to confirm it is absolutely legal.  it should not *
*   be used inside the search, but can be used to check a 21-bit (compressed)  *
*   move to be sure it is safe to make it on the permanent game board.         *
*                                                                              *
********************************************************************************
*/
int LegalMove(TREE *tree, int ply, int wtm, int move) {
  int moves[220], *mv, *mvp;
/*
   generate moves, then eliminate any that are illegal.
*/
  if (move == 0) return(0);
  tree->position[MAXPLY]=tree->position[ply];
  mvp=GenerateCaptures(tree,MAXPLY, wtm, moves);
  mvp=GenerateNonCaptures(tree,MAXPLY, wtm, mvp);
  for (mv=&moves[0];mv<mvp;mv++) {
    MakeMove(tree,MAXPLY, *mv, wtm);
    if (!Check(wtm) && move==*mv) {
      UnmakeMove(tree,MAXPLY, *mv, wtm);
      return(1);
    }
    UnmakeMove(tree,MAXPLY, *mv, wtm);
  }
  return(0);
}

// RedCrafty_481_f.cpp, ř. 1974-2304: GenerateCaptures
/* modified 03/23/99 */
/*
********************************************************************************
*                                                                              *
*   GenerateCaptures() is used to generate capture and pawn promotion moves    *
*   from the current position.                                                 *
*                                                                              *
*   the destination square set is the set of squares occupied by opponent      *
*   pieces, plus the set of squares on the 8th rank that pawns can advance to  *
*   and promote.                                                               *
*                                                                              *
********************************************************************************
*/
int* GenerateCaptures(TREE *tree, int ply, int wtm, int *move) {
  BITBOARD target, piecebd, moves;
  BITBOARD  promotions, pcapturesl, pcapturesr;
  int from, to, temp;

/*
  Port: tady byl zakomentovaný ladicí čítač stanley_int s výpisem do
  Memo1 (RedCrafty_481_f.cpp, ř. 1992-1995). Vyhozen.
*/

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
  if (wtm) {
    piecebd=WhiteKnights;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=knight_attacks[from] & BlackPieces;
      temp=from+(knight<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
        staClear(to,moves);
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
      moves=AttacksBishop(from) & BlackPieces;
      temp=from+(bishop<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
        staClear(to,moves);
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
      moves=AttacksRook(from) & BlackPieces;
      temp=from+(rook<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
        staClear(to,moves);
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
      moves=AttacksQueen(from) & BlackPieces;
      temp=from+(queen<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
        staClear(to,moves);
      }
      staClear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=WhiteKingSQ;
    moves=king_attacks[from] & BlackPieces;
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      *move++=temp|(to<<6)|((-PcOnSq(to))<<15);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn promotions.  we  |
|   do this by masking  all pawns but those on the 7th     |
|   rank and then advancing them ahead if the square in    |
|   front is empty.                                        |
|                                                          |
 ----------------------------------------------------------
*/
    promotions=(WhitePawns & rank_mask[RANK7])>>8 & ~Occupied;
    while (promotions) {
      to=LastOne(promotions);
      *move++=(to-8)|(to<<6)|(pawn<<12)|(queen<<18);
      staClear(to,promotions);
    }

    target=BlackPieces | EnPassantTarget(ply);
    pcapturesl=(WhitePawns & mask_left_edge)>>7 & target;
    while (pcapturesl) {
      to=LastOne(pcapturesl);
      if (to < 56) {
        int cap=(PcOnSq(to)) ? -PcOnSq(to) : pawn;
        *move++=(to-7)|(to<<6)|(pawn<<12)|(cap<<15);
      }
      else
        *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(queen<<18);
      staClear(to,pcapturesl);
    }

    pcapturesr=(WhitePawns & mask_right_edge)>>9 & target;
    while (pcapturesr) {
      to=LastOne(pcapturesr);
      if (to < 56) {
        int cap=(PcOnSq(to)) ? -PcOnSq(to) : pawn;
        *move++=(to-9)|(to<<6)|(pawn<<12)|(cap<<15);
      }
      else
        *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(queen<<18);
      staClear(to,pcapturesr);
    }
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
  else {
    piecebd=BlackKnights;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=knight_attacks[from] & WhitePieces;
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PcOnSq(to)<<15);
        staClear(to,moves);
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
      moves=AttacksBishop(from) & WhitePieces;
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PcOnSq(to)<<15);
        staClear(to,moves);
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
      moves=AttacksRook(from) & WhitePieces;
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PcOnSq(to)<<15);
        staClear(to,moves);
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
    piecebd=BlackQueens;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=AttacksQueen(from) & WhitePieces;
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6)|(PcOnSq(to)<<15);
        staClear(to,moves);
      }
      staClear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=BlackKingSQ;
    moves=king_attacks[from] & WhitePieces;
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6)|(PcOnSq(to)<<15);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   another exception is capturing enpassant.  the first   |
|   step is to generate all possible pawn promotions.  we  |
|   do this by masking  all pawns but those on the 7th     |
|   rank and then advancing them ahead if the square in    |
|   front is empty.                                        |
|                                                          |
 ----------------------------------------------------------
*/
    promotions=(BlackPawns & rank_mask[RANK2])<<8 & ~Occupied;
    while (promotions) {
      to=FirstOne(promotions);
      *move++=(to+8)|(to<<6)|(pawn<<12)|(queen<<18);
      staClear(to,promotions);
    }

    target=WhitePieces | EnPassantTarget(ply);
    pcapturesl=(BlackPawns & mask_left_edge)<<9 & target;
    while (pcapturesl) {
      to=FirstOne(pcapturesl);
      if (to > 7) {
        int cap=(PcOnSq(to)) ? PcOnSq(to) : pawn;
        *move++=(to+9)|(to<<6)|(pawn<<12)|(cap<<15);
      }
      else
        *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(queen<<18);
      staClear(to,pcapturesl);
    }

    pcapturesr=(BlackPawns & mask_right_edge)<<7 & target;
    while (pcapturesr) {
      to=FirstOne(pcapturesr);
      if (to > 7) {
        int cap=(PcOnSq(to)) ? PcOnSq(to) : pawn;
        *move++=(to+7)|(to<<6)|(pawn<<12)|(cap<<15);
      }
      else
        *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(queen<<18);
      staClear(to,pcapturesr);
    }
  }
  return(move);
}

// RedCrafty_481_f.cpp, ř. 2836-3247: GenerateNonCaptures
/*
********************************************************************************
*                                                                              *
*   GenerateNonCaptures() is used to generate non-capture moves from the       *
*   current position.                                                          *
*                                                                              *
*   once the valid destination squares are known, we have to locate a friendly *
*   piece to get a attacks_to[] entry.  we then produce the moves for this     *
*   piece by using the source square as [from] and enumerating each square it  *
*   attacks into [to].                                                         *
*                                                                              *
*   pawns are handled differently.  regular pawn moves are produced by         *
*   shifting the pawn bitmap 8 bits "forward" and anding this with the         *
*   complement of the occupied squares bitmap  double advances are then        *
*   produced by anding the pawn bitmap with a mask containing 1's on the       *
*   second rank, shifting this 16 bits "forward" and then anding this with     *
*   the complement of the occupied squares bitmap as before.  if [to]          *
*   reaches the 8th rank, we produce a set of four moves, promoting the pawn   *
*   to knight, bishop, rook and queen.                                         *
*                                                                              *
********************************************************************************
*/
int* GenerateNonCaptures(TREE *tree, int ply, int wtm, int *move) {
  BITBOARD target, piecebd, moves;
  BITBOARD  padvances1, padvances2, pcapturesl, pcapturesr;
  int from, to, temp;

/*
  Port: tady byl zakomentovaný ladicí čítač stanley_int s výpisem do
  Memo1 (RedCrafty_481_f.cpp, ř. 2863-2866). Vyhozen.
*/

  if (wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, produce castling moves if it is legal.          |
|                                                          |
 ----------------------------------------------------------
*/
    if (WhiteCastle(ply) > 0) {
      if ((WhiteCastle(ply)&1) && !(Occupied & mask_white_OO) &&
          !Attacked(tree,E1,0) && !Attacked(tree,F1,0) && !Attacked(tree,G1,0)) {
        *move++=12676;
      }
      if ((WhiteCastle(ply)&2) && !(Occupied & mask_white_OOO) &&
          !Attacked(tree,C1,0) && !Attacked(tree,D1,0) && !Attacked(tree,E1,0)) {
        *move++=12420;
      }
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
    target=~Occupied;
    piecebd=WhiteKnights;
    while (piecebd) {
      from=LastOne(piecebd);
      moves=knight_attacks[from] & target;
      temp=from+(knight<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
      moves=AttacksBishop(from) & target;
      temp=from+(bishop<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
      moves=AttacksRook(from) & target;
      temp=from+(rook<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
      moves=AttacksQueen(from) & target;
      temp=from+(queen<<12);
      while (moves) {
        to=LastOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
      }
      staClear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=WhiteKingSQ;
    moves=king_attacks[from] & target;
    temp=from+(king<<12);
    while (moves) {
      to=LastOne(moves);
      *move++=temp|(to<<6);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   first we generate all possible pawn moves.  we do      |
|   this in 4 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares;     |
|   (3) remove the a-pawn(s) then shift the pawns          |
|   diagonally left then and with enemy occupied squares;  |
|   (4) remove the h-pawn(s) then shift the pawns          |
|   diagonally right then and with enemy occupied squares. |
|   note that the only captures produced are under-        |
|   promotions, because the rest were done in GenCap.      |
|                                                          |
 ----------------------------------------------------------
*/
    padvances1=WhitePawns>>8 & target;
    padvances2=(padvances1 & mask_advance_2_w)>>8 & target;
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
      *move++=(to-16)|(to<<6)|(pawn<<12);
      staClear(to,padvances2);
    }

    while (padvances1) {
      to=LastOne(padvances1);
      if (to < 56) *move++=(to-8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to-8)|(to<<6)|(pawn<<12)|(rook<<18);
        *move++=(to-8)|(to<<6)|(pawn<<12)|(bishop<<18);
        *move++=(to-8)|(to<<6)|(pawn<<12)|(knight<<18);
      }
      staClear(to,padvances1);
    }

    target=BlackPieces & rank_mask[RANK8];
    pcapturesl=(WhitePawns & mask_left_edge)>>7 & target;
    pcapturesr=(WhitePawns & mask_right_edge)>>9 & target;
    while (pcapturesl) {
      to=LastOne(pcapturesl);
      *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(rook<<18);
      *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(bishop<<18);
      *move++=(to-7)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(knight<<18);
      staClear(to,pcapturesl);
    }
    while (pcapturesr) {
      to=LastOne(pcapturesr);
      *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(rook<<18);
      *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(bishop<<18);
      *move++=(to-9)|(to<<6)|(pawn<<12)|((-PcOnSq(to))<<15)|(knight<<18);
      staClear(to,pcapturesr);
    }
  }
/*
 ----------------------------------------------------------
|                                                          |
|   first, produce castling moves if it is legal.          |
|                                                          |
 ----------------------------------------------------------
*/
  else {
    if (BlackCastle(ply) > 0) {
      if ((BlackCastle(ply)&1) && !(Occupied & mask_black_OO) &&
          !Attacked(tree,E8,1) && !Attacked(tree,F8,1) && !Attacked(tree,G8,1)) {
        *move++=16316;
      }
      if ((BlackCastle(ply)&2) && !(Occupied & mask_black_OOO) &&
          !Attacked(tree,C8,1) && !Attacked(tree,D8,1) && !Attacked(tree,E8,1)) {
        *move++=16060;
      }
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
    target=~Occupied;
    piecebd=BlackKnights;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=knight_attacks[from] & target;
      temp=from+(knight<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
      moves=AttacksBishop(from) & target;
      temp=from+(bishop<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
      moves=AttacksRook(from) & target;
      temp=from+(rook<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
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
    piecebd=BlackQueens;
    while (piecebd) {
      from=FirstOne(piecebd);
      moves=AttacksQueen(from) & target;
      temp=from+(queen<<12);
      while (moves) {
        to=FirstOne(moves);
        *move++=temp|(to<<6);
        staClear(to,moves);
      }
      staClear(from,piecebd);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce king moves by cycling through the         |
|   *_king board to locate a [from] square and then        |
|   cycling through king_attacks[] to locate to squares    |
|   that a king on [from] attacks.                         |
|                                                          |
 ----------------------------------------------------------
*/
    from=BlackKingSQ;
    moves=king_attacks[from] & target;
    temp=from+(king<<12);
    while (moves) {
      to=FirstOne(moves);
      *move++=temp|(to<<6);
      staClear(to,moves);
    }
/*
 ----------------------------------------------------------
|                                                          |
|   now, produce pawn moves.  this is done differently due |
|   to inconsistencies in the way a pawn moves when it     |
|   captures as opposed to normal non-capturing moves.     |
|   first we generate all possible pawn moves.  we do      |
|   this in 4 operations:  (1) shift the pawns forward one |
|   rank then and with empty squares;  (2) shift the pawns |
|   forward two ranks and then and with empty squares;     |
|   (3) remove the a-pawn(s) then shift the pawns          |
|   diagonally left then and with enemy occupied squares;  |
|   (4) remove the h-pawn(s) then shift the pawns          |
|   diagonally right then and with enemy occupied squares. |
|   note that the only captures produced are under-        |
|   promotions, because the rest were done in GenCap.      |
|                                                          |
 ----------------------------------------------------------
*/
    padvances1=BlackPawns<<8 & target;
    padvances2=(padvances1 & mask_advance_2_b)<<8 & target;
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
      *move++=(to+16)|(to<<6)|(pawn<<12);
      staClear(to,padvances2);
    }
    while (padvances1) {
      to=FirstOne(padvances1);
      if (to > 7) *move++=(to+8)|(to<<6)|(pawn<<12);
      else {
        *move++=(to+8)|(to<<6)|(pawn<<12)|(rook<<18);
        *move++=(to+8)|(to<<6)|(pawn<<12)|(bishop<<18);
        *move++=(to+8)|(to<<6)|(pawn<<12)|(knight<<18);
      }
      staClear(to,padvances1);
    }
    target=WhitePieces & rank_mask[RANK1];
    pcapturesl=(BlackPawns & mask_left_edge)<<9 & target;
    pcapturesr=(BlackPawns & mask_right_edge)<<7 & target;
    while (pcapturesl) {
      to=FirstOne(pcapturesl);
      *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(rook<<18);
      *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(bishop<<18);
      *move++=(to+9)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(knight<<18);
      staClear(to,pcapturesl);
    }
    while (pcapturesr) {
      to=FirstOne(pcapturesr);
      *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(rook<<18);
      *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(bishop<<18);
      *move++=(to+7)|(to<<6)|(pawn<<12)|(PcOnSq(to)<<15)|(knight<<18);
      staClear(to,pcapturesr);
    }
  }
  return(move);
}

// RedCrafty_481_f.cpp, ř. 3250-3857: MakeMove
/* last modified 08/23/99 */
/*
********************************************************************************
*                                                                              *
*   MakeMove() is responsible for updating the position database whenever a    *
*   piece is moved.  it performs the following operations:  (1) update the     *
*   board structure itself by moving the piece and removing any captured       *
*   piece.  (2) update the hash keys.  (3) update material counts.  (4) update *
*   castling status.  (5) update number of moves since last reversible move.   *
*                                                                              *
********************************************************************************
*/
void MakeMove(TREE *tree, int ply, int move, int wtm) {
  int piece, from, to, captured, promote;
  BITBOARD bit_move;
/*
 ----------------------------------------------------------
|                                                          |
|   first, clear the EnPassant_Target bit-mask.  moving a  |
|   pawn two ranks will set it later in MakeMove().        |
|                                                          |
 ----------------------------------------------------------
*/

  tree->position[ply+1]=tree->position[ply];
  tree->save_hash_key[ply]=HashKey;
  tree->save_pawn_hash_key[ply]=PawnHashKey;
  if (EnPassant(ply+1)) {
    HashEP(EnPassant(ply+1),HashKey);
    EnPassant(ply+1)=0;
  }
  Rule50Moves(ply+1)++;
/*
 ----------------------------------------------------------
|                                                          |
|   now do the piece-specific things by calling the        |
|   appropriate routine.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  piece=Piece(move);
  from=From(move);
  to=To(move);
  captured=Captured(move);
  promote=Promote(move);
MakePieceMove:
  ClearRL90(from,OccupiedRL90);
  ClearRL45(from,OccupiedRL45);
  ClearRR45(from,OccupiedRR45);
  SetRL90(to,OccupiedRL90);
  SetRL45(to,OccupiedRL45);
  SetRR45(to,OccupiedRR45);
  bit_move=SetMask(from) | SetMask(to);
  PcOnSq(from)=0;
  switch (piece) {
/*
********************************************************************************
*                                                                              *
*   make pawn moves.  there are two special cases:  (a) enpassant captures     *
*   where the captured pawn is not on the "to" square and must be removed in   *
*   a different way, and (2) pawn promotions (where the "Promote" variable     *
*   is non-zero) requires updating the appropriate bit boards since we are     *
*   creating a new piece.                                                      *
*                                                                              *
********************************************************************************
*/
  case pawn:
    if (wtm) {
      ClearSet(bit_move,WhitePawns);
      ClearSet(bit_move,WhitePieces);
      HashPW(from,HashKey);
      HashPW(from,PawnHashKey);
      HashPW(to,HashKey);
      HashPW(to,PawnHashKey);
      if (captured == 1) {
        if(!PcOnSq(to)) {
          ClearRL90(to-8,OccupiedRL90);
          ClearRL45(to-8,OccupiedRL45);
          ClearRR45(to-8,OccupiedRR45);
          staClear(to-8,BlackPawns);
          staClear(to-8,BlackPieces);
          HashPB(to-8,HashKey);
          HashPB(to-8,PawnHashKey);
          PcOnSq(to-8)=0;
          Material+=PAWN_VALUE;
          TotalBlackPawns--;
          TotalPieces--;
          captured=0;
        }
      }
      PcOnSq(to)=pawn;
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
      if (promote) {
        TotalWhitePawns--;
        Material-=PAWN_VALUE;
        staClear(to,WhitePawns);
        HashPW(to,HashKey);
        HashPW(to,PawnHashKey);
        switch (promote) {
        case knight:
          Set(to,WhiteKnights);
          HashNW(to,HashKey);
          PcOnSq(to)=knight;
          TotalWhitePieces+=knight_v;
          WhiteMinors++;
          Material+=KNIGHT_VALUE;
          break;
        case bishop:
          Set(to,WhiteBishops);
          Set(to,BishopsQueens);
          HashBW(to,HashKey);
          PcOnSq(to)=bishop;
          TotalWhitePieces+=bishop_v;
          WhiteMinors++;
          Material+=BISHOP_VALUE;
          break;
        case rook:
          Set(to,WhiteRooks);
          Set(to,RooksQueens);
          HashRW(to,HashKey);
          PcOnSq(to)=rook;
          TotalWhitePieces+=rook_v;
          WhiteMajors++;
          Material+=ROOK_VALUE;
          break;
        case queen:
          Set(to,WhiteQueens);
          Set(to,BishopsQueens);
          Set(to,RooksQueens);
          HashQW(to,HashKey);
          PcOnSq(to)=queen;
          TotalWhitePieces+=queen_v;
          WhiteMajors+=2;
          Material+=QUEEN_VALUE;
          break;
        }
      }
      else
        if (((to-from) == 16) && (mask_eptest[to] & BlackPawns)) {
          EnPassant(ply+1)=to-8;
          HashEP(to-8,HashKey);
        }
    }
    else {
      ClearSet(bit_move,BlackPawns);
      ClearSet(bit_move,BlackPieces);
      HashPB(from,HashKey);
      HashPB(from,PawnHashKey);
      HashPB(to,HashKey);
      HashPB(to,PawnHashKey);
      if (captured == 1) {
        if(!PcOnSq(to)) {
          ClearRL90(to+8,OccupiedRL90);
          ClearRL45(to+8,OccupiedRL45);
          ClearRR45(to+8,OccupiedRR45);
          staClear(to+8,WhitePawns);
          staClear(to+8,WhitePieces);
          HashPW(to+8,HashKey);
          HashPW(to+8,PawnHashKey);
          PcOnSq(to+8)=0;
          Material-=PAWN_VALUE;
          TotalWhitePawns--;
          TotalPieces--;
          captured=0;
        }
      }
      PcOnSq(to)=-pawn;
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
      if (promote) {
        TotalBlackPawns--;
        Material+=PAWN_VALUE;
        staClear(to,BlackPawns);
        HashPB(to,HashKey);
        HashPB(to,PawnHashKey);
        switch (promote) {
        case knight:
          Set(to,BlackKnights);
          HashNB(to,HashKey);
          PcOnSq(to)=-knight;
          TotalBlackPieces+=knight_v;
          BlackMinors++;
          Material-=KNIGHT_VALUE;
          break;
        case bishop:
          Set(to,BlackBishops);
          Set(to,BishopsQueens);
          HashBB(to,HashKey);
          PcOnSq(to)=-bishop;
          TotalBlackPieces+=bishop_v;
          BlackMinors++;
          Material-=BISHOP_VALUE;
          break;
        case rook:
          Set(to,BlackRooks);
          Set(to,RooksQueens);
          HashRB(to,HashKey);
          PcOnSq(to)=-rook;
          TotalBlackPieces+=rook_v;
          BlackMajors++;
          Material-=ROOK_VALUE;
          break;
        case queen:
          Set(to,BlackQueens);
          Set(to,BishopsQueens);
          Set(to,RooksQueens);
          HashQB(to,HashKey);
          PcOnSq(to)=-queen;
          TotalBlackPieces+=queen_v;
          BlackMajors+=2;
          Material-=QUEEN_VALUE;
          break;
        }
      }
      else
        if (((from-to) == 16) && (mask_eptest[to] & WhitePawns)) {
          EnPassant(ply+1)=to+8;
          HashEP(to+8,HashKey);
        }
    }
    Rule50Moves(ply+1)=0;
    break;
/*
********************************************************************************
*                                                                              *
*   make knight moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case knight:
    if (wtm) {
      ClearSet(bit_move,WhiteKnights);
      ClearSet(bit_move,WhitePieces);
      HashNW(from,HashKey);
      HashNW(to,HashKey);
      PcOnSq(to)=knight;
    }
    else {
      ClearSet(bit_move,BlackKnights);
      ClearSet(bit_move,BlackPieces);
      HashNB(from,HashKey);
      HashNB(to,HashKey);
      PcOnSq(to)=-knight;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   make bishop moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case bishop:
    staClear(from,BishopsQueens);
    Set(to,BishopsQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteBishops);
      ClearSet(bit_move,WhitePieces);
      HashBW(from,HashKey);
      HashBW(to,HashKey);
      PcOnSq(to)=bishop;
    }
    else {
      ClearSet(bit_move,BlackBishops);
      ClearSet(bit_move,BlackPieces);
      HashBB(from,HashKey);
      HashBB(to,HashKey);
      PcOnSq(to)=-bishop;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   make rook moves.  the only special case handling required is to determine  *
*   if x_castle is non-zero [x=w or b based on side to move].  if it is non-   *
*   zero, the value must be corrected if either rook is moving from its        *
*   original square, so that castling with that rook becomes impossible.       *
*                                                                              *
********************************************************************************
*/
  case rook:
    staClear(from,RooksQueens);
    Set(to,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteRooks);
      ClearSet(bit_move,WhitePieces);
      HashRW(from,HashKey);
      HashRW(to,HashKey);
      PcOnSq(to)=rook;
      if (WhiteCastle(ply+1) > 0) {
        if ((from == A1) && (WhiteCastle(ply+1)&2)) {
          WhiteCastle(ply+1)&=1;
          HashCastleW(1,HashKey);
        }
        else if ((from == H1) && (WhiteCastle(ply+1)&1)) {
          WhiteCastle(ply+1)&=2;
          HashCastleW(0,HashKey);
        }
      }
    }
    else {
      ClearSet(bit_move,BlackRooks);
      ClearSet(bit_move,BlackPieces);
      HashRB(from,HashKey);
      HashRB(to,HashKey);
      PcOnSq(to)=-rook;
      if (BlackCastle(ply+1) > 0) {
        if ((from == A8) && (BlackCastle(ply+1)&2)) {
          BlackCastle(ply+1)&=1;
          HashCastleB(1,HashKey);
        }
        else if ((from == H8) && (BlackCastle(ply+1)&1)) {
          BlackCastle(ply+1)&=2;
          HashCastleB(0,HashKey);
        }
      }
    }
    break;
/*
********************************************************************************
*                                                                              *
*   make queen moves                                                           *
*                                                                              *
********************************************************************************
*/
  case queen:
    staClear(from,BishopsQueens);
    Set(to,BishopsQueens);
    staClear(from,RooksQueens);
    Set(to,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteQueens);
      ClearSet(bit_move,WhitePieces);
      HashQW(from,HashKey);
      HashQW(to,HashKey);
      PcOnSq(to)=queen;
    }
    else {
      ClearSet(bit_move,BlackQueens);
      ClearSet(bit_move,BlackPieces);
      HashQB(from,HashKey);
      HashQB(to,HashKey);
      PcOnSq(to)=-queen;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   make king moves.  the only special case is castling, which is indicated    *
*   by from=E1, to=G1 for o-o as an example.  the king is moving from e1-g1    *
*   which is normally illegal.  in this case, the correct rook is also moved.  *
*                                                                              *
*   note that moving the king in any direction resets the x_castle [x=w or b]  *
*   flag indicating that castling is not possible in *this* position.          *
*                                                                              *
********************************************************************************
*/
  case king:
    if (wtm) {
      ClearSet(bit_move,WhitePieces);
      HashKW(from,HashKey);
      HashKW(to,HashKey);
      PcOnSq(to)=king;
      WhiteKingSQ=to;
      if (WhiteCastle(ply) > 0) {
        if (WhiteCastle(ply+1)&2) HashCastleW(1,HashKey);
        if (WhiteCastle(ply+1)&1) HashCastleW(0,HashKey);
        if (abs(to-from) == 2) WhiteCastle(ply+1)=-4;
        else WhiteCastle(ply+1)=0;
        if (abs(to-from) == 2) {
          piece=rook;
          if (to == G1) {
            from=H1;
            to=F1;
            goto MakePieceMove;
          }
          else {
            from=A1;
            to=D1;
            goto MakePieceMove;
          }
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPieces);
      HashKB(from,HashKey);
      HashKB(to,HashKey);
      PcOnSq(to)=-king;
      BlackKingSQ=to;
      if (BlackCastle(ply+1) > 0) {
        if (BlackCastle(ply+1)&2) HashCastleB(1,HashKey);
        if (BlackCastle(ply+1)&1) HashCastleB(0,HashKey);
        if (abs(to-from) == 2) BlackCastle(ply+1)=-4;
        else BlackCastle(ply+1)=0;
        if (abs(to-from) == 2) {
          piece=rook;
          if (to == G8) {
            from=H8;
            to=F8;
            goto MakePieceMove;
          }
          else {
            from=A8;
            to=D8;
            goto MakePieceMove;
          }
        }
      }
    }
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to "gracefully" remove a piece from the game board since it *
*   is being captured.  this includes updating the board structure.            *
*                                                                              *
********************************************************************************
*/
  if(captured) {
    Rule50Moves(ply+1)=0;
    TotalPieces--;
    if (promote) piece=promote;
    switch (captured) {
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured pawn.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn:
      if (wtm) {
        staClear(to,BlackPawns);
        staClear(to,BlackPieces);
        HashPB(to,HashKey);
        HashPB(to,PawnHashKey);
        Material+=PAWN_VALUE;
        TotalBlackPawns--;
      }
      else {
        staClear(to,WhitePawns);
        staClear(to,WhitePieces);
        HashPW(to,HashKey);
        HashPW(to,PawnHashKey);
        Material-=PAWN_VALUE;
        TotalWhitePawns--;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured knight.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case knight:
      if (wtm) {
        staClear(to,BlackKnights);
        staClear(to,BlackPieces);
        HashNB(to,HashKey);
        TotalBlackPieces-=knight_v;
        BlackMinors--;
        Material+=KNIGHT_VALUE;
      }
      else {
        staClear(to,WhiteKnights);
        staClear(to,WhitePieces);
        HashNW(to,HashKey);
        TotalWhitePieces-=knight_v;
        WhiteMinors--;
        Material-=KNIGHT_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured bishop.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case bishop:
      if (!SlidingDiag(piece)) staClear(to,BishopsQueens);
      if (wtm) {
        staClear(to,BlackBishops);
        staClear(to,BlackPieces);
        HashBB(to,HashKey);
        TotalBlackPieces-=bishop_v;
        BlackMinors--;
        Material+=BISHOP_VALUE;
      }
      else {
        staClear(to,WhiteBishops);
        staClear(to,WhitePieces);
        HashBW(to,HashKey);
        TotalWhitePieces-=bishop_v;
        WhiteMinors--;
        Material-=BISHOP_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured rook.                                |
|                                                          |
 ----------------------------------------------------------
*/
    case rook:
      if (!SlidingRow(piece)) staClear(to,RooksQueens);
      if (wtm) {
        staClear(to,BlackRooks);
        staClear(to,BlackPieces);
        HashRB(to,HashKey);
        if (BlackCastle(ply) > 0) {
          if ((to == A8) && (BlackCastle(ply+1)&2)) {
            BlackCastle(ply+1)&=1;
            HashCastleB(1,HashKey);
          }
          else if ((to == H8) && (BlackCastle(ply+1)&1)) {
            BlackCastle(ply+1)&=2;
            HashCastleB(0,HashKey);
          }
        }
        TotalBlackPieces-=rook_v;
        BlackMajors--;
        Material+=ROOK_VALUE;
      }
      else {
        staClear(to,WhiteRooks);
        staClear(to,WhitePieces);
        HashRW(to,HashKey);
        if (WhiteCastle(ply) > 0) {
          if ((to == A1) && (WhiteCastle(ply+1)&2)) {
            WhiteCastle(ply+1)&=1;
            HashCastleW(1,HashKey);
          }
          else if ((to == H1) && (WhiteCastle(ply+1)&1)) {
            WhiteCastle(ply+1)&=2;
            HashCastleW(0,HashKey);
          }
        }
        TotalWhitePieces-=rook_v;
        WhiteMajors--;
        Material-=ROOK_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured queen.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case queen:
      if (!SlidingDiag(piece)) staClear(to,BishopsQueens);
      if (!SlidingRow(piece)) staClear(to,RooksQueens);
      if (wtm) {
        staClear(to,BlackQueens);
        staClear(to,BlackPieces);
        HashQB(to,HashKey);
        TotalBlackPieces-=queen_v;
        BlackMajors-=2;
        Material+=QUEEN_VALUE;
      }
      else {
        staClear(to,WhiteQueens);
        staClear(to,WhitePieces);
        HashQW(to,HashKey);
        TotalWhitePieces-=queen_v;
        WhiteMajors-=2;
        Material-=QUEEN_VALUE;
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   remove a captured king. [this is an error condition]   |
|                                                          |
 ----------------------------------------------------------
*/
    case king:
      hlaseni_makemove_captured_king++;  // port: bylo Memo1, ř. 3847-3852
    }
  }

  return;
}

// RedCrafty_481_f.cpp, ř. 3858-3899: MakeMoveRoot
/*
********************************************************************************
*                                                                              *
*   MakeMoveRoot() is used to make a move at the root of the game tree,        *
*   before any searching is done.  it uses MakeMove() to execute the move,      *
*   but then copies the resulting position back to position[0], the actual     *
*   board position.  it handles the special-case of the draw-by-repetition     *
*   rule by maintaining a list of previous positions, which is reset each time *
*   a non-reversible (pawn move or capture move) is made.                      *
*                                                                              *
********************************************************************************
*/
void MakeMoveRoot(TREE *tree, int move, int wtm) {
/*
 ----------------------------------------------------------
|                                                          |
|   first, make the move and replace position[0] with the  |
|   new position.                                          |
|                                                          |
 ----------------------------------------------------------
*/
  MakeMove(tree,0,move,wtm);
/*
 ----------------------------------------------------------
|                                                          |
|   now, if this is a non-reversible move, reset the       |
|   repetition list pointer to start the count over.       |
|                                                          |
 ----------------------------------------------------------
*/
  if (Rule50Moves(1) == 0) {
    tree->rephead_b=tree->replist_b;
    tree->rephead_w=tree->replist_w;
  }
  WhiteCastle(1)=Max(0,WhiteCastle(1));
  BlackCastle(1)=Max(0,BlackCastle(1));
  tree->position[0]=tree->position[1];
  if (ChangeSide(wtm))
    *tree->rephead_w++=HashKey;
  else
    *tree->rephead_b++=HashKey;
}

// RedCrafty_481_f.cpp, ř. 3903-4373: UnmakeMove
/* last modified 03/11/98 */
/*
********************************************************************************
*                                                                              *
*   UnmakeMove() is responsible for updating the position database whenever a  *
*   move is retracted.  it is the exact inverse of MakeMove().                 *
*                                                                              *
********************************************************************************
*/
void UnmakeMove(TREE *tree, int ply, int move, int wtm) {
  int piece, from, to, captured, promote;
  BITBOARD bit_move;
/*
 ----------------------------------------------------------
|                                                          |
|   first, take care of the hash key if there's a possible |
|   enpassant pawn capture.                                |
|                                                          |
 ----------------------------------------------------------
*/
  HashKey=tree->save_hash_key[ply];
  PawnHashKey=tree->save_pawn_hash_key[ply];
/*
 ----------------------------------------------------------
|                                                          |
|   now do the piece-specific things by calling the        |
|   appropriate routine.                                   |
|                                                          |
 ----------------------------------------------------------
*/
  piece=Piece(move);
  from=From(move);
  to=To(move);
  captured=Captured(move);
  promote=Promote(move);
UnmakePieceMove:
  SetRL90(from,OccupiedRL90);
  SetRL45(from,OccupiedRL45);
  SetRR45(from,OccupiedRR45);
  ClearRL90(to,OccupiedRL90);
  ClearRL45(to,OccupiedRL45);
  ClearRR45(to,OccupiedRR45);
  bit_move=SetMask(from) | SetMask(to);
  PcOnSq(to)=0;
  switch (piece) {

/*
********************************************************************************
*                                                                              *
*   unmake pawn moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case pawn:
    if (wtm) {
      ClearSet(bit_move,WhitePawns);
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=pawn;
      if (captured == 1) {
        if(EnPassant(ply) == to) {
          TotalPieces++;
          SetRL90(to-8,OccupiedRL90);
          SetRL45(to-8,OccupiedRL45);
          SetRR45(to-8,OccupiedRR45);
          Set(to-8,BlackPawns);
          Set(to-8,BlackPieces);
          PcOnSq(to-8)=-pawn;
          Material-=PAWN_VALUE;
          TotalBlackPawns++;
          captured=0;
        }
      }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
      if (promote) {
        TotalWhitePawns++;
        Material+=PAWN_VALUE;
        staClear(to,WhitePawns);
        staClear(to,WhitePieces);
        switch (promote) {
        case knight:
          staClear(to,WhiteKnights);
          TotalWhitePieces-=knight_v;
          WhiteMinors--;
          Material-=KNIGHT_VALUE;
          break;
        case bishop:
          staClear(to,WhiteBishops);
          staClear(to,BishopsQueens);
          TotalWhitePieces-=bishop_v;
          WhiteMinors--;
          Material-=BISHOP_VALUE;
          break;
        case rook:
          staClear(to,WhiteRooks);
          staClear(to,RooksQueens);
          TotalWhitePieces-=rook_v;
          WhiteMajors--;
          Material-=ROOK_VALUE;
          break;
        case queen:
          staClear(to,WhiteQueens);
          staClear(to,BishopsQueens);
          staClear(to,RooksQueens);
          TotalWhitePieces-=queen_v;
          WhiteMajors-=2;
          Material-=QUEEN_VALUE;
          break;
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPawns);
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-pawn;
      if (captured == 1) {
        if(EnPassant(ply) == to) {
          TotalPieces++;
          SetRL90(to+8,OccupiedRL90);
          SetRL45(to+8,OccupiedRL45);
          SetRR45(to+8,OccupiedRR45);
          Set(to+8,WhitePawns);
          Set(to+8,WhitePieces);
          PcOnSq(to+8)=pawn;
          Material+=PAWN_VALUE;
          TotalWhitePawns++;
          captured=0;
        }
      }
/*
 --------------------------------------------------------------------
|                                                                    |
|  if this is a pawn promotion, remove the pawn from the counts      |
|  then update the correct piece board to reflect the piece just     |
|  created.                                                          |
|                                                                    |
 --------------------------------------------------------------------
*/
      if (promote) {
        TotalBlackPawns++;
        Material-=PAWN_VALUE;
        staClear(to,BlackPawns);
        staClear(to,BlackPieces);
        switch (promote) {
        case knight:
          staClear(to,BlackKnights);
          TotalBlackPieces-=knight_v;
          BlackMinors--;
          Material+=KNIGHT_VALUE;
          break;
        case bishop:
          staClear(to,BlackBishops);
          staClear(to,BishopsQueens);
          TotalBlackPieces-=bishop_v;
          BlackMinors--;
          Material+=BISHOP_VALUE;
          break;
        case rook:
          staClear(to,BlackRooks);
          staClear(to,RooksQueens);
          TotalBlackPieces-=rook_v;
          BlackMajors--;
          Material+=ROOK_VALUE;
          break;
        case queen:
          staClear(to,BlackQueens);
          staClear(to,BishopsQueens);
          staClear(to,RooksQueens);
          TotalBlackPieces-=queen_v;
          BlackMajors-=2;
          Material+=QUEEN_VALUE;
          break;
        }
      }
    }
    break;

/*
********************************************************************************
*                                                                              *
*   unmake knight moves.                                                       *
*                                                                              *
********************************************************************************
*/
  case knight:
    if (wtm) {
      ClearSet(bit_move,WhiteKnights);
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=knight;
    }
    else {
      ClearSet(bit_move,BlackKnights);
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-knight;
    }
    break;

/*
********************************************************************************
*                                                                              *
*   unmake bishop moves.                                                       *
*                                                                              *
********************************************************************************
*/
  case bishop:
    ClearSet(bit_move,BishopsQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteBishops);
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=bishop;
    }
    else {
      ClearSet(bit_move,BlackBishops);
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-bishop;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake rook moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case rook:
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteRooks);
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=rook;
    }
    else {
      ClearSet(bit_move,BlackRooks);
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-rook;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake queen moves.                                                        *
*                                                                              *
********************************************************************************
*/
  case queen:
    ClearSet(bit_move,BishopsQueens);
    ClearSet(bit_move,RooksQueens);
    if (wtm) {
      ClearSet(bit_move,WhiteQueens);
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=queen;
    }
    else {
      ClearSet(bit_move,BlackQueens);
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-queen;
    }
    break;
/*
********************************************************************************
*                                                                              *
*   unmake king moves.                                                         *
*                                                                              *
********************************************************************************
*/
  case king:
    if (wtm) {
      ClearSet(bit_move,WhitePieces);
      PcOnSq(from)=king;
      WhiteKingSQ=from;
      if (abs(to-from) == 2) {
        if (to == G1) {
          from=H1;
          to=F1;
          piece=rook;
          goto UnmakePieceMove;
        }
        else {
          from=A1;
          to=D1;
          piece=rook;
          goto UnmakePieceMove;
        }
      }
    }
    else {
      ClearSet(bit_move,BlackPieces);
      PcOnSq(from)=-king;
      BlackKingSQ=from;
      if (abs(to-from) == 2) {
        if (to == G8) {
          from=H8;
          to=F8;
          piece=rook;
          goto UnmakePieceMove;
        }
        else {
          from=A8;
          to=D8;
          piece=rook;
          goto UnmakePieceMove;
        }
      }
    }
    break;
  }
/*
********************************************************************************
*                                                                              *
*   now it is time to restore a piece that was captured.                       *
*                                                                              *
********************************************************************************
*/
  if(captured) {
    TotalPieces++;
    SetRL90(to,OccupiedRL90);
    SetRL45(to,OccupiedRL45);
    SetRR45(to,OccupiedRR45);
    switch (captured) {
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured pawn.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case pawn:
      if (wtm) {
        Set(to,BlackPawns);
        Set(to,BlackPieces);
        PcOnSq(to)=-pawn;
        Material-=PAWN_VALUE;
        TotalBlackPawns++;
      }
      else {
        Set(to,WhitePawns);
        Set(to,WhitePieces);
        PcOnSq(to)=pawn;
        Material+=PAWN_VALUE;
        TotalWhitePawns++;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured knight.                             |
|                                                          |
 ----------------------------------------------------------
*/
    case knight:
      if (wtm) {
        Set(to,BlackKnights);
        Set(to,BlackPieces);
        PcOnSq(to)=-knight;
        TotalBlackPieces+=knight_v;
        BlackMinors++;
        Material-=KNIGHT_VALUE;
      }
      else {
        Set(to,WhiteKnights);
        Set(to,WhitePieces);
        PcOnSq(to)=knight;
        TotalWhitePieces+=knight_v;
        WhiteMinors++;
        Material+=KNIGHT_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured bishop.                             |
|                                                          |
 ----------------------------------------------------------
*/
    case bishop:
      Set(to,BishopsQueens);
      if (wtm) {
        Set(to,BlackBishops);
        Set(to,BlackPieces);
        PcOnSq(to)=-bishop;
        TotalBlackPieces+=bishop_v;
        BlackMinors++;
        Material-=BISHOP_VALUE;
      }
      else {
        Set(to,WhiteBishops);
        Set(to,WhitePieces);
        PcOnSq(to)=bishop;
        TotalWhitePieces+=bishop_v;
        WhiteMinors++;
        Material+=BISHOP_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured rook.                               |
|                                                          |
 ----------------------------------------------------------
*/
    case rook:
      Set(to,RooksQueens);
      if (wtm) {
        Set(to,BlackRooks);
        Set(to,BlackPieces);
        PcOnSq(to)=-rook;
        TotalBlackPieces+=rook_v;
        BlackMajors++;
        Material-=ROOK_VALUE;
      }
      else {
        Set(to,WhiteRooks);
        Set(to,WhitePieces);
        PcOnSq(to)=rook;
        TotalWhitePieces+=rook_v;
        WhiteMajors++;
        Material+=ROOK_VALUE;
      }
    break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured queen.                              |
|                                                          |
 ----------------------------------------------------------
*/
    case queen:
      Set(to,BishopsQueens);
      Set(to,RooksQueens);
      if (wtm) {
        Set(to,BlackQueens);
        Set(to,BlackPieces);
        PcOnSq(to)=-queen;
        TotalBlackPieces+=queen_v;
        BlackMajors+=2;
        Material-=QUEEN_VALUE;
      }
      else {
        Set(to,WhiteQueens);
        Set(to,WhitePieces);
        PcOnSq(to)=queen;
        TotalWhitePieces+=queen_v;
        WhiteMajors+=2;
        Material+=QUEEN_VALUE;
      }
      break;
/*
 ----------------------------------------------------------
|                                                          |
|   restore a captured king. [this is an error condition]  |
|                                                          |
 ----------------------------------------------------------
*/
    case king:
      hlaseni_unmakemove_captured_king++;  // port: bylo Memo1, ř. 4364-4368
    }
  }

  return;
}

// --- port --------------------------------------------------------------------

int hlaseni_side_not_on_move_in_check = 0;
int hlaseni_printf = 0;

// Knihovna nesmí psát na stdout: v aplikaci to nikam nevede a v konzolové
// aplikaci (tools/crafty) je stdout kanál UCI, který by výpis rozbil.
// Hyattovy printf v SetBoard a v hledání proto volají tohle. Formátovací
// řetězce zůstávají v kódu jako popis chyby.
int tichy_printf(const char *, ...) {
  hlaseni_printf++;
  return 0;
}
int hlaseni_makemove_captured_king = 0;
int hlaseni_unmakemove_captured_king = 0;

namespace {
// FormActivate alokoval strom mallocem (ř. 66) a 32 bufferů pro args
// po 128 bajtech (ř. 71). Tady statická paměť.
TREE hlavni_strom;
char pamet_args[32][128];
bool tabulky_hotove = false;
}  // namespace

void inicializuj() {
  TREE *tree;
  int i;

  // FormActivate, ř. 66-71
  local[0]=&hlavni_strom;
  local[0]->used=1;
  local[0]->stop=0;
  local[0]->ply=1;
  tree=local[0];
  for (i=0;i<32;i++) args[i]=pamet_args[i];

  // Initialize(0), ř. 1832-1843. InitializePieceMasks (ř. 1837) chybí:
  // plní jen masky pro hodnocení pěšcových závodů. Hash tabulky
  // (ř. 1847-1869) a zrcadlení hodnotových tabulek (ř. 1871-1883) taky.
  if (!tabulky_hotove) {
    InitializeZeroMasks();
    InitializeMasks();
    InitializeRandomHash();
    InitializeAttackBoards();
    InitializePawnMasks();
    tabulky_hotove=true;
  }
  InitializeChessBoard(&tree->position[0]);
  tree->last[0]=tree->move_list;
  tree->last[1]=tree->move_list;
}

bool nastav_fen(const char *fen) {
  TREE * const tree=local[0];
  const int pred=hlaseni_side_not_on_move_in_check;
  const char *mezera;
  std::string deska;

  // ReadParse kopíruje každé slovo do args[i] po 128 bajtech a SetBoard
  // kopíruje desku do input[80]; obojí strcpy bez kontroly délky.
  if (std::strlen(fen) >= 128) return false;
  mezera=std::strchr(fen,' ');
  deska=mezera ? std::string(fen,mezera-fen) : std::string(fen);
  if (deska.size() >= 80) return false;

  std::strcpy(buffer,fen);
  nargs=ReadParse(buffer,args," ;");
  if (nargs < 2) return false;
  SetBoard(&tree->position[0],nargs,args,0);
  if (hlaseni_side_not_on_move_in_check != pred) return false;

  const std::string zapsano=zapis_fen(tree,0,wtm);
  return zapsano.compare(0,zapsano.find(' '),deska) == 0;
}

std::string zapis_fen(TREE *tree, int ply, int wtm) {
  std::string f, r;
  int rank, file, prazdnych, p;

  for (rank=7;rank>=0;rank--) {
    prazdnych=0;
    for (file=0;file<8;file++) {
      p=PcOnSq(rank*8+file);
      if (!p) { prazdnych++; continue; }
      if (prazdnych) { f+=(char) ('0'+prazdnych); prazdnych=0; }
      f+=xlate[p+7];
    }
    if (prazdnych) f+=(char) ('0'+prazdnych);
    if (rank) f+='/';
  }
  f+=wtm ? " w " : " b ";
  if (WhiteCastle(ply) > 0) {
    if (WhiteCastle(ply)&1) r+='K';
    if (WhiteCastle(ply)&2) r+='Q';
  }
  if (BlackCastle(ply) > 0) {
    if (BlackCastle(ply)&1) r+='k';
    if (BlackCastle(ply)&2) r+='q';
  }
  f+=r.empty() ? "-" : r;
  f+=' ';
  if (EnPassant(ply)) {
    f+=(char) ('a'+File(EnPassant(ply)));
    f+=(char) ('1'+Rank(EnPassant(ply)));
  }
  else f+='-';
  f+=' '+std::to_string(Rule50Moves(ply))+' '+std::to_string(move_number);
  return f;
}

}  // namespace crafty
