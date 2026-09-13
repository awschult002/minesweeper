#ifndef BOARD_CAMERA_H
#define BOARD_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BoardCamera {
  float center_x;
  float center_y;
  float zoom;       /* user factor; 1 = default playable view */
  float zoom_min;
  float zoom_max;
  float cell_size;
  int board_w;
  int board_h;
  float view_half_w;
  float view_half_h;
  /* World frustum size at zoom == 1 (portrait playfield, not stretched). */
  float base_view_w;
  float base_view_h;
} BoardCamera;

void board_camera_init(BoardCamera *cam, int board_w, int board_h);
/** Set default world view at zoom=1 (e.g. ~9 cells wide, portrait aspect). */
void board_camera_set_base_view(BoardCamera *cam, float view_w, float view_h);
void board_camera_set_zoom_limits(BoardCamera *cam, float zmin, float zmax);
void board_camera_set_view_half(BoardCamera *cam, float half_w, float half_h);
void board_camera_clamp(BoardCamera *cam);
void board_camera_pan(BoardCamera *cam, float dx, float dy);
void board_camera_zoom(BoardCamera *cam, float factor);
/** Effective world frustum size for current zoom. */
void board_camera_frustum(const BoardCamera *cam, float *out_w, float *out_h);
int board_camera_pick_cell(const BoardCamera *cam, float wx, float wy,
                           int *out_x, int *out_y);
void board_camera_cell_center(const BoardCamera *cam, int x, int y,
                              float *out_wx, float *out_wy);

#ifdef __cplusplus
}
#endif

#endif
