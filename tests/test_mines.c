/**
 * Tester gate fixtures for pure Minesweeper grid.
 *
 * Build/run:
 *   make -C /workspace/repos/minesweeper test
 *   # or:
 *   gcc -std=c99 -Wall -Wextra -Isrc/game -Isrc/view -o tests/test_mines \
 *       tests/test_mines.c src/game/mines.c src/view/board_camera.c && ./tests/test_mines
 */
#include "mines.h"
#include "board_camera.h"

#include <stdio.h>

static int g_failed = 0;
static int g_passed = 0;

#define ASSERT_TRUE(cond, msg) do { \
  if (!(cond)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
    g_failed++; \
  } else { g_passed++; } \
} while (0)

#define ASSERT_EQ(a, b, msg) do { \
  long _a = (long)(a), _b = (long)(b); \
  if (_a != _b) { \
    fprintf(stderr, "FAIL %s:%d: %s (got %ld expected %ld)\n", \
            __FILE__, __LINE__, msg, _a, _b); \
    g_failed++; \
  } else { g_passed++; } \
} while (0)

static MinesCellState st(const MinesBoard *b, int x, int y)
{
  return mines_at_c(b, x, y)->state;
}

/* (1) zero-flood full N=0 component stops at numbers/flags */
static void fixture1_zero_flood_stops_at_numbers_and_flags(void)
{
  MinesBoard b;
  /* Mine at (0,0); flag a zero-path cell (2,2) before reveal to block flood */
  const char *layout =
      "*..."
      "...."
      "...."
      "....";
  ASSERT_EQ(mines_from_layout(&b, 4, 4, layout), 0, "layout");
  mines_flag(&b, 2, 2);
  mines_reveal(&b, 3, 3);
  ASSERT_EQ(st(&b, 3, 3), MINES_OPEN, "start open");
  ASSERT_EQ(st(&b, 2, 2), MINES_FLAGGED, "flag not opened by flood");
  ASSERT_EQ(st(&b, 0, 0), MINES_CLOSED, "mine closed");
  /* number border next to mine should open if reached; (0,1) is adj=1 */
  ASSERT_EQ(st(&b, 1, 0), MINES_OPEN, "number border opened");
  ASSERT_TRUE(mines_at_c(&b, 1, 0)->adjacent >= 1, "border is number");
  /* cell beyond flag may stay closed if only path was through flag */
  ASSERT_EQ(b.status, MINES_PLAYING, "still playing");
}

/* (2) chord no-op when |F| ≠ N */
static void fixture2_chord_noop_flag_mismatch(void)
{
  MinesBoard b;
  const char *layout =
      "*.*"
      "..."
      "*.*";
  ASSERT_EQ(mines_from_layout(&b, 3, 3, layout), 0, "layout");
  mines_reveal(&b, 1, 1);
  ASSERT_EQ(mines_at_c(&b, 1, 1)->adjacent, 4, "N=4");
  mines_flag(&b, 0, 0);
  mines_flag(&b, 2, 0); /* |F|=2 ≠ 4 */
  mines_chord(&b, 1, 1);
  ASSERT_EQ(st(&b, 1, 0), MINES_CLOSED, "no-op leaves edges closed");
  ASSERT_EQ(b.status, MINES_PLAYING, "still playing");
}

/* (3) chord lose on misflag */
static void fixture3_chord_lose_on_misflag(void)
{
  MinesBoard b;
  const char *layout =
      "*.*"
      "..."
      "*.*";
  ASSERT_EQ(mines_from_layout(&b, 3, 3, layout), 0, "layout");
  mines_reveal(&b, 1, 1);
  mines_flag(&b, 0, 0);
  mines_flag(&b, 2, 0);
  mines_flag(&b, 0, 2);
  mines_flag(&b, 1, 0); /* false flag; real mine (2,2) unflagged */
  mines_chord(&b, 1, 1);
  ASSERT_EQ(b.status, MINES_LOST, "misflag chord loses");
  ASSERT_EQ(st(&b, 2, 2), MINES_OPEN, "unflagged mine opened");
  ASSERT_EQ(st(&b, 1, 0), MINES_FLAGGED, "chord never clears flags");
}

/* (4) chord into zero continues flood */
static void fixture4_chord_into_zero_continues_flood(void)
{
  MinesBoard b;
  const char *layout =
      "*.."
      "..."
      "...";
  ASSERT_EQ(mines_from_layout(&b, 3, 3, layout), 0, "layout");
  ASSERT_EQ(mines_at_c(&b, 1, 1)->adjacent, 1, "N=1");
  mines_reveal(&b, 1, 1); /* number only, no flood expand */
  ASSERT_EQ(st(&b, 1, 1), MINES_OPEN, "number open");
  ASSERT_EQ(st(&b, 2, 2), MINES_CLOSED, "zero still closed");
  mines_flag(&b, 0, 0);
  mines_chord(&b, 1, 1);
  ASSERT_EQ(st(&b, 2, 2), MINES_OPEN, "chord opened zero → flood");
  ASSERT_EQ(b.status, MINES_WON, "flood cleared remaining safes");
  ASSERT_EQ(st(&b, 0, 0), MINES_FLAGGED, "flags uncleared");
}

/* (5) corner/edge |adj| < 8 */
static void fixture5_corner_edge_neighbor_counts(void)
{
  MinesBoard b;
  ASSERT_EQ(mines_from_layout(&b, 3, 3, "........."), 0, "layout");
  ASSERT_EQ(mines_count_neighbors(&b, 0, 0), 3, "corner has 3 neighbors");
  ASSERT_EQ(mines_count_neighbors(&b, 1, 0), 5, "edge has 5 neighbors");
  ASSERT_EQ(mines_count_neighbors(&b, 1, 1), 8, "center has 8");
  /* N(v) for corner with one adjacent mine */
  ASSERT_EQ(mines_from_layout(&b, 2, 2, "*.*." ), 0, "mines corners");
  ASSERT_EQ(mines_at_c(&b, 1, 0)->adjacent, 2, "edge cell N with 2 mines");
  ASSERT_EQ(mines_count_neighbors(&b, 1, 0), 3, "still only 3 in-bounds nbrs");
}

/* (6) win = all safes open */
static void fixture6_win_all_safes_open(void)
{
  MinesBoard b;
  ASSERT_EQ(mines_from_layout(&b, 2, 2, "*..."), 0, "layout");
  mines_reveal(&b, 1, 0);
  mines_reveal(&b, 0, 1);
  ASSERT_EQ(b.status, MINES_PLAYING, "one safe left");
  mines_reveal(&b, 1, 1);
  ASSERT_EQ(b.status, MINES_WON, "all non-mines open => win");
  ASSERT_EQ(st(&b, 0, 0), MINES_CLOSED, "mine need not be flagged to win");
}

/* (7) flag toggle never opens */
static void fixture7_flag_toggle_never_opens(void)
{
  MinesBoard b;
  ASSERT_EQ(mines_from_layout(&b, 2, 2, "*.*." ), 0, "layout");
  mines_flag(&b, 0, 0);
  ASSERT_EQ(st(&b, 0, 0), MINES_FLAGGED, "flagged");
  mines_reveal(&b, 0, 0);
  ASSERT_EQ(st(&b, 0, 0), MINES_FLAGGED, "reveal on flag is no-op");
  ASSERT_EQ(b.status, MINES_PLAYING, "not lost");
  mines_flag(&b, 0, 0);
  ASSERT_EQ(st(&b, 0, 0), MINES_CLOSED, "unflag");
  mines_reveal(&b, 1, 0); /* safe open */
  ASSERT_EQ(st(&b, 1, 0), MINES_OPEN, "safe opened");
  mines_flag(&b, 1, 0);
  ASSERT_EQ(st(&b, 1, 0), MINES_OPEN, "flag toggle never opens; cannot flag open");
}


/* random board: exact mine_count + adjacent consistency */
static void fixture_random_board_mine_count_and_adjacent(void)
{
  MinesBoard b;
  int i, n, mines, x, y, k;
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  ASSERT_EQ(mines_init(&b, 9, 9), 0, "init 9x9");
  ASSERT_EQ(mines_place_random(&b, 10, 42u), 0, "place 10");
  ASSERT_EQ(b.mine_count, 10, "mine_count field");
  n = b.width * b.height;
  mines = 0;
  for (i = 0; i < n; ++i) {
    if (b.cells[i].is_mine)
      mines++;
    ASSERT_EQ(b.cells[i].state, MINES_CLOSED, "all closed");
  }
  ASSERT_EQ(mines, 10, "exact mine cells");

  for (y = 0; y < b.height; ++y) {
    for (x = 0; x < b.width; ++x) {
      const MinesCell *c = mines_at_c(&b, x, y);
      int expect = 0;
      if (c->is_mine) {
        ASSERT_EQ(c->adjacent, 0, "mine adjacent unused/0");
        continue;
      }
      for (k = 0; k < 8; ++k) {
        const MinesCell *nb = mines_at_c(&b, x + dx[k], y + dy[k]);
        if (nb && nb->is_mine)
          expect++;
      }
      ASSERT_EQ(c->adjacent, expect, "adjacent matches Moore mines");
    }
  }

  /* deterministic seed: same seed → same layout */
  {
    MinesBoard b2;
    ASSERT_EQ(mines_init(&b2, 9, 9), 0, "init2");
    ASSERT_EQ(mines_place_random(&b2, 10, 42u), 0, "place2");
    for (i = 0; i < n; ++i)
      ASSERT_EQ(b.cells[i].is_mine, b2.cells[i].is_mine, "seeded layout stable");
  }

  /* expert-sized portrait board */
  ASSERT_EQ(mines_init(&b, 16, 30), 0, "init expert");
  ASSERT_EQ(mines_place_random(&b, 99, 7u), 0, "place 99");
  ASSERT_EQ(b.mine_count, 99, "expert mine_count");
  mines = 0;
  n = b.width * b.height;
  for (i = 0; i < n; ++i)
    if (b.cells[i].is_mine)
      mines++;
  ASSERT_EQ(mines, 99, "expert exact mines");
}

static void camera_pick_smoke(void)
{
  BoardCamera cam;
  int x = -1, y = -1;
  board_camera_init(&cam, 9, 9);
  ASSERT_TRUE(board_camera_pick_cell(&cam, 0.5f, 0.5f, &x, &y), "pick");
  ASSERT_EQ(x, 0, "cell x");
  ASSERT_EQ(y, 0, "cell y");
  ASSERT_TRUE(!board_camera_pick_cell(&cam, -0.1f, 0.0f, &x, &y), "oob");
  board_camera_pan(&cam, 1.0f, 0.0f);
  board_camera_zoom(&cam, 2.0f);
  ASSERT_TRUE(cam.zoom > 1.0f, "zoomed");
}

int main(void)
{
  fixture1_zero_flood_stops_at_numbers_and_flags();
  fixture2_chord_noop_flag_mismatch();
  fixture3_chord_lose_on_misflag();
  fixture4_chord_into_zero_continues_flood();
  fixture5_corner_edge_neighbor_counts();
  fixture6_win_all_safes_open();
  fixture7_flag_toggle_never_opens();
  fixture_random_board_mine_count_and_adjacent();
  camera_pick_smoke();

  printf("mines tests: %d passed, %d failed\n", g_passed, g_failed);
  return g_failed ? 1 : 0;
}
