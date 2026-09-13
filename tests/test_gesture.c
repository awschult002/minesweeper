/**
 * Gesture / camera / touch_bridge gates (post S22 remap).
 */
#include "mines.h"
#include "board_camera.h"
#include "gesture.h"
#include "touch_bridge.h"

#include <stdio.h>

static int g_failed = 0;
static int g_passed = 0;

#define ASSERT_TRUE(cond, msg) do { \
  if (!(cond)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); g_failed++; } \
  else { g_passed++; } \
} while (0)

#define ASSERT_EQ(a, b, msg) do { \
  long _a = (long)(a), _b = (long)(b); \
  if (_a != _b) { \
    fprintf(stderr, "FAIL %s:%d: %s (got %ld expected %ld)\n", \
            __FILE__, __LINE__, msg, _a, _b); \
    g_failed++; \
  } else { g_passed++; } \
} while (0)

static float cell_wx(int x) { return x + 0.5f; }
static float cell_wy(int y) { return y + 0.5f; }

static void tap_reveal_closed(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  ASSERT_EQ(mines_at_c(&b, 0, 0)->state, MINES_OPEN, "reveal mode opens");
}

static void tap_open_number_chords_ok(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  const char *layout = "*.." "..." "...";
  mines_from_layout(&b, 3, 3, layout);
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  mines_reveal(&b, 1, 1);
  mines_flag(&b, 0, 0);
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(1), cell_wy(1));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(1), cell_wy(1));
  ASSERT_EQ(mines_at_c(&b, 2, 2)->state, MINES_OPEN, "tap-N chords");
}

static void mode_flag_tap(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  gesture_set_mode(&g, GESTURE_MODE_FLAG);
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(1), cell_wy(1));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(1), cell_wy(1));
  ASSERT_EQ(mines_at_c(&b, 1, 1)->state, MINES_FLAGGED, "flag mode");
}

static void mode_question_cycle(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  gesture_set_mode(&g, GESTURE_MODE_QUESTION);
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  ASSERT_EQ(mines_at_c(&b, 0, 0)->state, MINES_QUESTION, "question set");
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(0), cell_wy(0));
  ASSERT_EQ(mines_at_c(&b, 0, 0)->state, MINES_CLOSED, "question clears");
}

static void question_not_chord_count(void)
{
  MinesBoard b;
  const char *layout = "*.." "..." "...";
  mines_from_layout(&b, 3, 3, layout);
  mines_reveal(&b, 1, 1);
  mines_question(&b, 0, 0);
  mines_chord(&b, 1, 1);
  ASSERT_EQ(mines_at_c(&b, 2, 2)->state, MINES_CLOSED, "? does not satisfy chord N");
  ASSERT_EQ(b.status, MINES_PLAYING, "still playing");
}

static void question_opens_on_reveal(void)
{
  MinesBoard b;
  mines_from_layout(&b, 3, 3, ".........");
  mines_question(&b, 1, 1);
  mines_reveal(&b, 1, 1);
  ASSERT_EQ(mines_at_c(&b, 1, 1)->state, MINES_OPEN, "reveal opens ?");
}

static void two_finger_no_longer_flags(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  gesture_touches(&g, 2, cell_wx(0), cell_wy(0), cell_wx(2), cell_wy(0));
  gesture_touches(&g, 0, 0, 0, 0, 0);
  ASSERT_EQ(mines_at_c(&b, 0, 0)->state, MINES_CLOSED, "2-finger must not flag");
}

static void bridge_two_finger_no_flag(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g; TouchBridge tb;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  touch_bridge_init(&tb, &g);
  touch_bridge_begin(&tb, 1, cell_wx(0), cell_wy(0));
  touch_bridge_begin(&tb, 2, cell_wx(2), cell_wy(0));
  touch_bridge_end(&tb, 1, cell_wx(0), cell_wy(0));
  touch_bridge_end(&tb, 2, cell_wx(2), cell_wy(0));
  ASSERT_TRUE(touch_bridge_was_used(&tb), "bridge used");
  ASSERT_EQ(mines_at_c(&b, 0, 0)->state, MINES_CLOSED, "bridge 2-finger no flag");
}

static void camera_clamp_keeps_board(void)
{
  BoardCamera cam;
  board_camera_init(&cam, 9, 9);
  board_camera_set_view_half(&cam, 2.0f, 2.0f);
  board_camera_pan(&cam, 100.0f, 100.0f);
  ASSERT_TRUE(cam.center_x <= 9.0f - 2.0f + 0.01f, "clamp max x");
  ASSERT_TRUE(cam.center_y <= 9.0f - 2.0f + 0.01f, "clamp max y");
  board_camera_pan(&cam, -1000.0f, -1000.0f);
  ASSERT_TRUE(cam.center_x >= 2.0f - 0.01f, "clamp min x");
  ASSERT_TRUE(cam.center_y >= 2.0f - 0.01f, "clamp min y");
}

static void drag_pans_no_reveal(void)
{
  MinesBoard b; BoardCamera cam; GestureSM g;
  float cx0;
  mines_from_layout(&b, 9, 9,
    "........."
    "........."
    "........."
    "........."
    "........."
    "........."
    "........."
    "........."
    ".........");
  board_camera_init(&cam, 9, 9);
  board_camera_set_view_half(&cam, 2.0f, 2.0f);
  cx0 = cam.center_x;
  gesture_init(&g, &b, &cam);
  gesture_button_down(&g, GESTURE_BTN_LEFT, cell_wx(4), cell_wy(4));
  gesture_pointer_move(&g, cell_wx(4) + 2.0f, cell_wy(4));
  gesture_button_up(&g, GESTURE_BTN_LEFT, cell_wx(4) + 2.0f, cell_wy(4));
  ASSERT_EQ(mines_at_c(&b, 4, 4)->state, MINES_CLOSED, "drag no reveal");
  ASSERT_TRUE(cam.center_x != cx0, "drag pans");
}

static void zoom_in_from_default(void)
{
  BoardCamera cam;
  float z0, fw0, fh0, fw1, fh1;
  board_camera_init(&cam, 16, 30);
  board_camera_set_base_view(&cam, 9.0f, 9.0f * (1280.0f/720.0f));
  board_camera_set_zoom_limits(&cam, 0.5f, 4.0f);
  z0 = cam.zoom;
  board_camera_frustum(&cam, &fw0, &fh0);
  board_camera_zoom(&cam, 1.25f);
  board_camera_frustum(&cam, &fw1, &fh1);
  ASSERT_TRUE(cam.zoom > z0, "zoom in increases factor");
  ASSERT_TRUE(fw1 < fw0 && fh1 < fh0, "zoom in shrinks frustum");
  ASSERT_TRUE(fw0 / fh0 > 0.99f * (fw1 / fh1) && fw0 / fh0 < 1.01f * (fw1 / fh1),
              "portrait aspect preserved");
}

static void mode_cycle(void)
{
  GestureSM g;
  MinesBoard b; BoardCamera cam;
  mines_from_layout(&b, 3, 3, ".........");
  board_camera_init(&cam, 3, 3);
  gesture_init(&g, &b, &cam);
  ASSERT_EQ(gesture_get_mode(&g), GESTURE_MODE_REVEAL, "start reveal");
  gesture_cycle_mode(&g);
  ASSERT_EQ(gesture_get_mode(&g), GESTURE_MODE_FLAG, "→ flag");
  gesture_cycle_mode(&g);
  ASSERT_EQ(gesture_get_mode(&g), GESTURE_MODE_QUESTION, "→ question");
  gesture_cycle_mode(&g);
  ASSERT_EQ(gesture_get_mode(&g), GESTURE_MODE_REVEAL, "wrap");
}

int main(void)
{
  tap_reveal_closed();
  tap_open_number_chords_ok();
  mode_flag_tap();
  mode_question_cycle();
  question_not_chord_count();
  question_opens_on_reveal();
  two_finger_no_longer_flags();
  bridge_two_finger_no_flag();
  camera_clamp_keeps_board();
  zoom_in_from_default();
  drag_pans_no_reveal();
  mode_cycle();
  printf("gesture: %d passed, %d failed\n", g_passed, g_failed);
  return g_failed ? 1 : 0;
}
