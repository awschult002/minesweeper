#include "touch_bridge.h"

static void emit(TouchBridge *t)
{
  int n = (t->has0 ? 1 : 0) + (t->has1 ? 1 : 0);
  if (n == 0)
    gesture_touches(t->gesture, 0, 0, 0, 0, 0);
  else if (n == 1) {
    if (t->has0)
      gesture_touches(t->gesture, 1, t->x0, t->y0, 0, 0);
    else
      gesture_touches(t->gesture, 1, t->x1, t->y1, 0, 0);
  } else
    gesture_touches(t->gesture, 2, t->x0, t->y0, t->x1, t->y1);
}

void touch_bridge_init(TouchBridge *t, GestureSM *gesture)
{
  if (!t)
    return;
  t->gesture = gesture;
  t->id0 = t->id1 = 0;
  t->x0 = t->y0 = t->x1 = t->y1 = 0.0f;
  t->has0 = t->has1 = 0;
  t->used = 0;
}

int touch_bridge_was_used(const TouchBridge *t)
{
  return t ? t->used : 0;
}

void touch_bridge_begin(TouchBridge *t, uint32_t id, float wx, float wy)
{
  if (!t || !t->gesture)
    return;
  t->used = 1;
  if (!t->has0) {
    t->has0 = 1;
    t->id0 = id;
    t->x0 = wx;
    t->y0 = wy;
  } else if (!t->has1 && id != t->id0) {
    t->has1 = 1;
    t->id1 = id;
    t->x1 = wx;
    t->y1 = wy;
  } else if (t->has0 && id == t->id0) {
    t->x0 = wx;
    t->y0 = wy;
  } else if (t->has1 && id == t->id1) {
    t->x1 = wx;
    t->y1 = wy;
  }
  emit(t);
}

void touch_bridge_move(TouchBridge *t, uint32_t id, float wx, float wy)
{
  if (!t || !t->gesture)
    return;
  if (t->has0 && id == t->id0) {
    t->x0 = wx;
    t->y0 = wy;
  } else if (t->has1 && id == t->id1) {
    t->x1 = wx;
    t->y1 = wy;
  } else
    return;
  emit(t);
}

void touch_bridge_end(TouchBridge *t, uint32_t id, float wx, float wy)
{
  int was_two;
  if (!t || !t->gesture)
    return;
  was_two = t->has0 && t->has1;
  if (t->has0 && id == t->id0) {
    t->x0 = wx;
    t->y0 = wy;
    t->has0 = 0;
    if (t->has1) {
      t->has0 = 1;
      t->id0 = t->id1;
      t->x0 = t->x1;
      t->y0 = t->y1;
      t->has1 = 0;
    }
  } else if (t->has1 && id == t->id1) {
    t->x1 = wx;
    t->y1 = wy;
    t->has1 = 0;
  } else
    return;
  /* 2→1: stay quiet so gesture still sees a clean 2→0 two-finger tap.
   * Remaining finger moves will emit via touch_bridge_move. */
  if (was_two && (t->has0 || t->has1))
    return;
  emit(t);
}
