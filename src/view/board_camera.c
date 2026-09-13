#include "board_camera.h"

static float clampf(float v, float lo, float hi)
{
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

void board_camera_init(BoardCamera *cam, int board_w, int board_h)
{
  if (!cam)
    return;
  cam->board_w = board_w;
  cam->board_h = board_h;
  cam->cell_size = 1.0f;
  cam->zoom = 1.0f;
  cam->zoom_min = 0.5f;
  cam->zoom_max = 4.0f;
  cam->center_x = (board_w * 0.5f);
  cam->center_y = (board_h * 0.5f);
  cam->base_view_w = 9.0f;
  cam->base_view_h = 9.0f * (1280.0f / 720.0f);
  cam->view_half_w = cam->base_view_w * 0.5f;
  cam->view_half_h = cam->base_view_h * 0.5f;
  board_camera_clamp(cam);
}

void board_camera_set_base_view(BoardCamera *cam, float view_w, float view_h)
{
  if (!cam)
    return;
  if (view_w > 0.0f)
    cam->base_view_w = view_w;
  if (view_h > 0.0f)
    cam->base_view_h = view_h;
  cam->zoom = 1.0f;
  board_camera_frustum(cam, &view_w, &view_h);
  board_camera_set_view_half(cam, view_w * 0.5f, view_h * 0.5f);
}

void board_camera_set_zoom_limits(BoardCamera *cam, float zmin, float zmax)
{
  if (!cam)
    return;
  if (zmin > 0.0f)
    cam->zoom_min = zmin;
  if (zmax >= cam->zoom_min)
    cam->zoom_max = zmax;
  cam->zoom = clampf(cam->zoom, cam->zoom_min, cam->zoom_max);
}

void board_camera_frustum(const BoardCamera *cam, float *out_w, float *out_h)
{
  float z;
  if (!cam)
    return;
  z = cam->zoom > 0.0f ? cam->zoom : 1.0f;
  if (out_w)
    *out_w = cam->base_view_w / z;
  if (out_h)
    *out_h = cam->base_view_h / z;
}

void board_camera_set_view_half(BoardCamera *cam, float half_w, float half_h)
{
  if (!cam)
    return;
  if (half_w > 0.0f)
    cam->view_half_w = half_w;
  if (half_h > 0.0f)
    cam->view_half_h = half_h;
  board_camera_clamp(cam);
}

void board_camera_clamp(BoardCamera *cam)
{
  float bw, bh, min_x, max_x, min_y, max_y;
  if (!cam || cam->cell_size <= 0.0f)
    return;
  bw = cam->board_w * cam->cell_size;
  bh = cam->board_h * cam->cell_size;
  if (cam->view_half_w * 2.0f >= bw) {
    cam->center_x = bw * 0.5f;
  } else {
    min_x = cam->view_half_w;
    max_x = bw - cam->view_half_w;
    cam->center_x = clampf(cam->center_x, min_x, max_x);
  }
  if (cam->view_half_h * 2.0f >= bh) {
    cam->center_y = bh * 0.5f;
  } else {
    min_y = cam->view_half_h;
    max_y = bh - cam->view_half_h;
    cam->center_y = clampf(cam->center_y, min_y, max_y);
  }
}

void board_camera_pan(BoardCamera *cam, float dx, float dy)
{
  if (!cam)
    return;
  cam->center_x += dx;
  cam->center_y += dy;
  board_camera_clamp(cam);
}

void board_camera_zoom(BoardCamera *cam, float factor)
{
  float fw, fh;
  if (!cam || factor <= 0.0f)
    return;
  cam->zoom *= factor;
  cam->zoom = clampf(cam->zoom, cam->zoom_min, cam->zoom_max);
  board_camera_frustum(cam, &fw, &fh);
  board_camera_set_view_half(cam, fw * 0.5f, fh * 0.5f);
}

int board_camera_pick_cell(const BoardCamera *cam, float wx, float wy,
                           int *out_x, int *out_y)
{
  float cs;
  int x, y;
  if (!cam || !out_x || !out_y || cam->cell_size <= 0.0f)
    return 0;
  cs = cam->cell_size;
  if (wx < 0.0f || wy < 0.0f)
    return 0;
  x = (int)(wx / cs);
  y = (int)(wy / cs);
  if (x < 0 || y < 0 || x >= cam->board_w || y >= cam->board_h)
    return 0;
  *out_x = x;
  *out_y = y;
  return 1;
}

void board_camera_cell_center(const BoardCamera *cam, int x, int y,
                              float *out_wx, float *out_wy)
{
  float cs;
  if (!cam || !out_wx || !out_wy)
    return;
  cs = cam->cell_size;
  *out_wx = (x + 0.5f) * cs;
  *out_wy = (y + 0.5f) * cs;
}
