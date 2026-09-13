#include "gesture.h"

#include <math.h>

static float dist2(float ax, float ay, float bx, float by)
{
  float dx = ax - bx, dy = ay - by;
  return dx * dx + dy * dy;
}

static float dist(float ax, float ay, float bx, float by)
{
  return sqrtf(dist2(ax, ay, bx, by));
}

static void pick(GestureSM *g, float wx, float wy, int *cx, int *cy, int *ok)
{
  *ok = board_camera_pick_cell(g->cam, wx, wy, cx, cy);
}

static void try_chord_lr(GestureSM *g)
{
  int cx, cy, ok;
  if (!g || !g->board || g->chorded)
    return;
  if (!(g->left_down && g->right_down))
    return;
  pick(g, g->last_wx, g->last_wy, &cx, &cy, &ok);
  if (!ok)
    return;
  mines_chord(g->board, cx, cy);
  g->chorded = 1;
}

static void apply_mode_tap(GestureSM *g, int cx, int cy)
{
  const MinesCell *c = mines_at_c(g->board, cx, cy);
  if (!c)
    return;
  /* Chord always on revealed number, regardless of HUD mode. */
  if (c->state == MINES_OPEN && c->adjacent >= 1) {
    mines_chord(g->board, cx, cy);
    return;
  }
  switch (g->mode) {
  case GESTURE_MODE_FLAG:
    mines_flag(g->board, cx, cy);
    break;
  case GESTURE_MODE_QUESTION:
    mines_question(g->board, cx, cy);
    break;
  case GESTURE_MODE_REVEAL:
  default:
    mines_reveal(g->board, cx, cy);
    break;
  }
}

static void primary_tap(GestureSM *g, float wx, float wy)
{
  int cx, cy, ok;
  pick(g, wx, wy, &cx, &cy, &ok);
  if (!ok || !g->board)
    return;
  apply_mode_tap(g, cx, cy);
}

void gesture_init(GestureSM *g, MinesBoard *board, BoardCamera *cam)
{
  if (!g)
    return;
  g->board = board;
  g->cam = cam;
  g->slop = 0.35f;
  g->mode = GESTURE_MODE_REVEAL;
  g->left_down = 0;
  g->right_down = 0;
  g->press_wx = g->press_wy = 0.0f;
  g->last_wx = g->last_wy = 0.0f;
  g->press_cell_x = g->press_cell_y = 0;
  g->has_press_cell = 0;
  g->panning = 0;
  g->chorded = 0;
  g->touch_count = 0;
  g->t0x = g->t0y = g->t1x = g->t1y = 0.0f;
  g->pinch_dist0 = 0.0f;
  g->pinch_active = 0;
}

void gesture_set_mode(GestureSM *g, GestureMode mode)
{
  if (!g)
    return;
  g->mode = mode;
}

void gesture_cycle_mode(GestureSM *g)
{
  if (!g)
    return;
  g->mode = (GestureMode)(((int)g->mode + 1) % 3);
}

GestureMode gesture_get_mode(const GestureSM *g)
{
  return g ? g->mode : GESTURE_MODE_REVEAL;
}

void gesture_button_down(GestureSM *g, int button, float wx, float wy)
{
  int cx, cy, ok;
  if (!g)
    return;
  g->last_wx = wx;
  g->last_wy = wy;
  if (button == GESTURE_BTN_LEFT) {
    g->left_down = 1;
    g->press_wx = wx;
    g->press_wy = wy;
    g->panning = 0;
    pick(g, wx, wy, &cx, &cy, &ok);
    g->has_press_cell = ok;
    g->press_cell_x = cx;
    g->press_cell_y = cy;
    if (g->right_down)
      try_chord_lr(g);
  } else if (button == GESTURE_BTN_RIGHT) {
    g->right_down = 1;
    if (g->left_down)
      try_chord_lr(g);
  }
}

void gesture_button_up(GestureSM *g, int button, float wx, float wy)
{
  float d2;
  if (!g)
    return;
  g->last_wx = wx;
  g->last_wy = wy;

  if (button == GESTURE_BTN_LEFT) {
    if (g->left_down && !g->panning && !g->chorded) {
      d2 = dist2(wx, wy, g->press_wx, g->press_wy);
      if (d2 <= g->slop * g->slop)
        primary_tap(g, wx, wy);
    }
    g->left_down = 0;
    g->panning = 0;
    if (!g->right_down)
      g->chorded = 0;
  } else if (button == GESTURE_BTN_RIGHT) {
    /* Desktop shortcut: R-click flags regardless of HUD mode. */
    if (g->right_down && !g->left_down && !g->chorded) {
      int cx, cy, ok;
      pick(g, wx, wy, &cx, &cy, &ok);
      if (ok)
        mines_flag(g->board, cx, cy);
    }
    g->right_down = 0;
    if (!g->left_down)
      g->chorded = 0;
  }
}

void gesture_pointer_move(GestureSM *g, float wx, float wy)
{
  float dx, dy, d2;
  if (!g || !g->cam)
    return;
  if (g->left_down && !g->right_down) {
    d2 = dist2(wx, wy, g->press_wx, g->press_wy);
    if (!g->panning && d2 > g->slop * g->slop)
      g->panning = 1;
    if (g->panning) {
      dx = g->last_wx - wx;
      dy = g->last_wy - wy;
      board_camera_pan(g->cam, dx, dy);
    }
  }
  g->last_wx = wx;
  g->last_wy = wy;
  if (g->left_down && g->right_down)
    try_chord_lr(g);
}

void gesture_wheel(GestureSM *g, float factor)
{
  if (!g || !g->cam || factor <= 0.0f)
    return;
  board_camera_zoom(g->cam, factor);
}

void gesture_touches(GestureSM *g, int count, float x0, float y0, float x1,
                     float y1)
{
  float d, factor;
  if (!g)
    return;
  if (count < 0)
    count = 0;
  if (count > 2)
    count = 2;

  if (count == 0) {
    if (g->touch_count == 1 && g->left_down)
      gesture_button_up(g, GESTURE_BTN_LEFT, g->t0x, g->t0y);
    /* 2-finger tap no longer flags — pinch-only multi-touch. */
    g->touch_count = 0;
    g->pinch_active = 0;
    return;
  }

  if (count == 1) {
    if (g->touch_count == 2) {
      g->left_down = 0;
      g->panning = 0;
      g->chorded = 1;
      g->touch_count = 1;
      g->t0x = x0;
      g->t0y = y0;
      g->pinch_active = 0;
      return;
    }
    if (g->touch_count == 0)
      gesture_button_down(g, GESTURE_BTN_LEFT, x0, y0);
    else
      gesture_pointer_move(g, x0, y0);
    g->t0x = x0;
    g->t0y = y0;
    g->touch_count = 1;
    return;
  }

  if (g->touch_count < 2) {
    if (g->left_down) {
      g->left_down = 0;
      g->panning = 0;
      g->chorded = 1;
    }
    g->t0x = x0;
    g->t0y = y0;
    g->t1x = x1;
    g->t1y = y1;
    g->pinch_dist0 = dist(x0, y0, x1, y1);
    if (g->pinch_dist0 < 1e-3f)
      g->pinch_dist0 = 1e-3f;
    g->pinch_active = 0;
    g->touch_count = 2;
    return;
  }

  d = dist(x0, y0, x1, y1);
  if (d < 1e-3f)
    d = 1e-3f;
  factor = d / g->pinch_dist0;
  if (factor < 0.92f || factor > 1.08f) {
    g->pinch_active = 1;
    board_camera_zoom(g->cam, factor);
    g->pinch_dist0 = d;
  }
  {
    float mx0 = 0.5f * (g->t0x + g->t1x);
    float my0 = 0.5f * (g->t0y + g->t1y);
    float mx1 = 0.5f * (x0 + x1);
    float my1 = 0.5f * (y0 + y1);
    board_camera_pan(g->cam, mx0 - mx1, my0 - my1);
  }
  g->t0x = x0;
  g->t0y = y0;
  g->t1x = x1;
  g->t1y = y1;
}
