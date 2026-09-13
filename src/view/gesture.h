/**
 * Lean gesture \u2192 camera / mines_* router.
 * HUD mode: Reveal | Flag | Question (cycle). Tap-on-open-N always mines_chord.
 * No 2-finger flag. Pinch = zoom only. Drag = pan (clamped).
 */
#ifndef GESTURE_H
#define GESTURE_H

#include "board_camera.h"
#include "mines.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  GESTURE_BTN_LEFT  = 0,
  GESTURE_BTN_RIGHT = 1
};

typedef enum GestureMode {
  GESTURE_MODE_REVEAL = 0,
  GESTURE_MODE_FLAG = 1,
  GESTURE_MODE_QUESTION = 2
} GestureMode;

typedef struct GestureSM {
  MinesBoard *board;
  BoardCamera *cam;
  float slop;
  GestureMode mode;

  int left_down;
  int right_down;
  float press_wx, press_wy;
  float last_wx, last_wy;
  int press_cell_x, press_cell_y;
  int has_press_cell;
  int panning;
  int chorded;

  int touch_count;
  float t0x, t0y, t1x, t1y;
  float pinch_dist0;
  int pinch_active;
} GestureSM;

void gesture_init(GestureSM *g, MinesBoard *board, BoardCamera *cam);
void gesture_set_mode(GestureSM *g, GestureMode mode);
void gesture_cycle_mode(GestureSM *g);
GestureMode gesture_get_mode(const GestureSM *g);

void gesture_button_down(GestureSM *g, int button, float wx, float wy);
void gesture_button_up(GestureSM *g, int button, float wx, float wy);
void gesture_pointer_move(GestureSM *g, float wx, float wy);
void gesture_wheel(GestureSM *g, float factor);
void gesture_touches(GestureSM *g, int count, float x0, float y0, float x1,
                     float y1);

#ifdef __cplusplus
}
#endif

#endif
