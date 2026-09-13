#include "mines.h"

#include <string.h>

static int idx(const MinesBoard *b, int x, int y)
{
  return y * b->width + x;
}

static int is_covered_openable(MinesCellState s)
{
  return s == MINES_CLOSED || s == MINES_QUESTION;
}

int mines_in_bounds(const MinesBoard *b, int x, int y)
{
  return b && x >= 0 && y >= 0 && x < b->width && y < b->height;
}

MinesCell *mines_at(MinesBoard *b, int x, int y)
{
  if (!mines_in_bounds(b, x, y))
    return NULL;
  return &b->cells[idx(b, x, y)];
}

const MinesCell *mines_at_c(const MinesBoard *b, int x, int y)
{
  if (!mines_in_bounds(b, x, y))
    return NULL;
  return &b->cells[idx(b, x, y)];
}

int mines_init(MinesBoard *b, int width, int height)
{
  if (!b || width < 1 || height < 1 || width > MINES_MAX_W || height > MINES_MAX_H)
    return -1;
  memset(b, 0, sizeof(*b));
  b->width = width;
  b->height = height;
  b->mine_count = 0;
  b->status = MINES_PLAYING;
  return 0;
}

static void compute_adjacent(MinesBoard *b)
{
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int x, y, k;

  for (y = 0; y < b->height; ++y) {
    for (x = 0; x < b->width; ++x) {
      MinesCell *c = mines_at(b, x, y);
      c->adjacent = 0;
      if (c->is_mine)
        continue;
      for (k = 0; k < 8; ++k) {
        const MinesCell *n = mines_at_c(b, x + dx[k], y + dy[k]);
        if (n && n->is_mine)
          c->adjacent++;
      }
    }
  }
}

int mines_load_layout(MinesBoard *b, const char *layout)
{
  int i, n;
  if (!b || !layout)
    return -1;
  n = b->width * b->height;
  b->mine_count = 0;
  for (i = 0; i < n; ++i) {
    char ch = layout[i];
    if (ch == '\0')
      return -1;
    b->cells[i].is_mine = (ch == '*' || ch == 'M' || ch == 'm') ? 1 : 0;
    b->cells[i].state = MINES_CLOSED;
    b->cells[i].adjacent = 0;
    if (b->cells[i].is_mine)
      b->mine_count++;
  }
  b->status = MINES_PLAYING;
  compute_adjacent(b);
  return 0;
}

int mines_from_layout(MinesBoard *b, int width, int height, const char *layout)
{
  if (mines_init(b, width, height) != 0)
    return -1;
  return mines_load_layout(b, layout);
}


int mines_place_random(MinesBoard *b, int mine_count, uint32_t seed)
{
  int n, i;
  uint32_t state;
  int indices[MINES_MAX_W * MINES_MAX_H];

  if (!b || mine_count < 0)
    return -1;
  n = b->width * b->height;
  if (mine_count > n)
    return -1;

  for (i = 0; i < n; ++i) {
    b->cells[i].is_mine = 0;
    b->cells[i].adjacent = 0;
    b->cells[i].state = MINES_CLOSED;
    indices[i] = i;
  }
  b->status = MINES_PLAYING;
  state = seed ? seed : 1u;

  /* Partial Fisher-Yates: first mine_count slots are mine positions */
  for (i = 0; i < mine_count; ++i) {
    int j, tmp;
    state = state * 1664525u + 1013904223u;
    j = i + (int)(state % (uint32_t)(n - i));
    tmp = indices[i];
    indices[i] = indices[j];
    indices[j] = tmp;
    b->cells[indices[i]].is_mine = 1;
  }
  b->mine_count = mine_count;
  compute_adjacent(b);
  return 0;
}


int mines_count_adj_flags(const MinesBoard *b, int x, int y)
{
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int k, count = 0;
  for (k = 0; k < 8; ++k) {
    const MinesCell *n = mines_at_c(b, x + dx[k], y + dy[k]);
    if (n && n->state == MINES_FLAGGED)
      count++;
  }
  return count;
}

int mines_count_neighbors(const MinesBoard *b, int x, int y)
{
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int k, count = 0;
  if (!mines_in_bounds(b, x, y))
    return 0;
  for (k = 0; k < 8; ++k) {
    if (mines_in_bounds(b, x + dx[k], y + dy[k]))
      count++;
  }
  return count;
}

void mines_check_win(MinesBoard *b)
{
  int i, n;
  if (!b || b->status != MINES_PLAYING)
    return;
  n = b->width * b->height;
  for (i = 0; i < n; ++i) {
    if (!b->cells[i].is_mine && b->cells[i].state != MINES_OPEN)
      return;
  }
  b->status = MINES_WON;
}

static void flood_reveal(MinesBoard *b, int x, int y)
{
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  int stack_x[MINES_MAX_W * MINES_MAX_H];
  int stack_y[MINES_MAX_W * MINES_MAX_H];
  int sp = 0;
  MinesCell *start = mines_at(b, x, y);
  if (!start || !is_covered_openable(start->state) || start->is_mine)
    return;

  stack_x[sp] = x;
  stack_y[sp] = y;
  sp++;

  while (sp > 0) {
    int cx, cy, k;
    MinesCell *c;
    sp--;
    cx = stack_x[sp];
    cy = stack_y[sp];
    c = mines_at(b, cx, cy);
    if (!c || !is_covered_openable(c->state) || c->is_mine)
      continue;
    c->state = MINES_OPEN;
    if (c->adjacent != 0)
      continue;
    for (k = 0; k < 8; ++k) {
      int nx = cx + dx[k];
      int ny = cy + dy[k];
      MinesCell *n = mines_at(b, nx, ny);
      if (n && is_covered_openable(n->state) && !n->is_mine) {
        stack_x[sp] = nx;
        stack_y[sp] = ny;
        sp++;
      }
    }
  }
}

void mines_reveal(MinesBoard *b, int x, int y)
{
  MinesCell *c;
  if (!b || b->status != MINES_PLAYING)
    return;
  c = mines_at(b, x, y);
  if (!c)
    return;
  if (c->state == MINES_OPEN || c->state == MINES_FLAGGED)
    return;

  if (c->is_mine) {
    c->state = MINES_OPEN;
    b->status = MINES_LOST;
    return;
  }

  flood_reveal(b, x, y);
  mines_check_win(b);
}

void mines_flag(MinesBoard *b, int x, int y)
{
  MinesCell *c;
  if (!b || b->status != MINES_PLAYING)
    return;
  c = mines_at(b, x, y);
  if (!c)
    return;
  if (c->state == MINES_OPEN)
    return;
  if (c->state == MINES_FLAGGED)
    c->state = MINES_CLOSED;
  else
    c->state = MINES_FLAGGED; /* closed or question → flag */
}

void mines_question(MinesBoard *b, int x, int y)
{
  MinesCell *c;
  if (!b || b->status != MINES_PLAYING)
    return;
  c = mines_at(b, x, y);
  if (!c)
    return;
  if (c->state == MINES_OPEN)
    return;
  if (c->state == MINES_QUESTION)
    c->state = MINES_CLOSED;
  else
    c->state = MINES_QUESTION; /* closed or flagged → ? */
}

void mines_chord(MinesBoard *b, int x, int y)
{
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
  MinesCell *c;
  int flags, k;

  if (!b || b->status != MINES_PLAYING)
    return;
  c = mines_at(b, x, y);
  if (!c)
    return;
  if (c->state != MINES_OPEN || c->adjacent == 0)
    return;

  flags = mines_count_adj_flags(b, x, y);
  if (flags != (int)c->adjacent)
    return;

  for (k = 0; k < 8; ++k) {
    int nx = x + dx[k];
    int ny = y + dy[k];
    MinesCell *n = mines_at(b, nx, ny);
    if (!n)
      continue;
    if (n->state == MINES_FLAGGED)
      continue;
    if (is_covered_openable(n->state)) {
      mines_reveal(b, nx, ny);
      if (b->status == MINES_LOST)
        return;
    }
  }
  mines_check_win(b);
}
