/**
 * Pure Minesweeper grid — headless, no Orx / Android / camera / input.
 *
 * Contract:
 * - Moore neighborhood. N(v) = |adj ∩ mines|.
 * - States: closed / open / flagged / question. Flagged never opens via Reveal.
 * - Question does NOT count toward chord N; chord may open a '?'.
 * - Reveal: no-op if open or flagged; '?' opens like closed; mine → lose;
 *   N=0 flood (skip flagged only).
 * - Chord: open N≥1; |adj ∩ Flagged| == N → Reveal each non-flagged neighbor.
 * - Win: all non-mines open. Lose: any mine opened.
 */
#ifndef MINES_H
#define MINES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MINES_MAX_W 64
#define MINES_MAX_H 64

typedef enum MinesCellState {
  MINES_CLOSED   = 0,
  MINES_OPEN     = 1,
  MINES_FLAGGED  = 2,
  MINES_QUESTION = 3
} MinesCellState;

typedef enum MinesStatus {
  MINES_PLAYING = 0,
  MINES_WON     = 1,
  MINES_LOST    = 2
} MinesStatus;

typedef struct MinesCell {
  uint8_t is_mine;
  uint8_t adjacent;
  MinesCellState state;
} MinesCell;

typedef struct MinesBoard {
  int width;
  int height;
  int mine_count;
  MinesStatus status;
  MinesCell cells[MINES_MAX_W * MINES_MAX_H];
} MinesBoard;

int mines_init(MinesBoard *b, int width, int height);
int mines_load_layout(MinesBoard *b, const char *layout);
int mines_from_layout(MinesBoard *b, int width, int height, const char *layout);
/** Place mine_count mines uniformly (seeded LCG); compute adjacent; all closed. */
int mines_place_random(MinesBoard *b, int mine_count, uint32_t seed);

MinesCell *mines_at(MinesBoard *b, int x, int y);
const MinesCell *mines_at_c(const MinesBoard *b, int x, int y);
int mines_in_bounds(const MinesBoard *b, int x, int y);

int mines_count_adj_flags(const MinesBoard *b, int x, int y);
int mines_count_neighbors(const MinesBoard *b, int x, int y);

void mines_reveal(MinesBoard *b, int x, int y);
void mines_flag(MinesBoard *b, int x, int y);
/** Toggle/set question mark on a covered cell. Does not count toward chord N. */
void mines_question(MinesBoard *b, int x, int y);
void mines_chord(MinesBoard *b, int x, int y);
void mines_check_win(MinesBoard *b);

#ifdef __cplusplus
}
#endif

#endif /* MINES_H */
