/**
 * Maps multi-touch slots → gesture_touches (no Orx).
 * Orx shell feeds screen→world points; all chords still go through mines_chord.
 */
#ifndef TOUCH_BRIDGE_H
#define TOUCH_BRIDGE_H

#include "gesture.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TouchBridge {
  GestureSM *gesture;
  uint32_t id0, id1;
  float x0, y0, x1, y1;
  int has0, has1;
  int used; /* set on first begin — Tester wiring gate */
} TouchBridge;

void touch_bridge_init(TouchBridge *t, GestureSM *gesture);
void touch_bridge_begin(TouchBridge *t, uint32_t id, float wx, float wy);
void touch_bridge_move(TouchBridge *t, uint32_t id, float wx, float wy);
void touch_bridge_end(TouchBridge *t, uint32_t id, float wx, float wy);

/** Non-zero once any begin has been seen (Tester: wiring live). */
int touch_bridge_was_used(const TouchBridge *t);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_BRIDGE_H */
