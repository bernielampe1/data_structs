#pragma once

#include "types.h"

// drawLine(x0, y0, x1, y1, color, width, height, img): rasterizes the
// segment between (x0, y0) and (x1, y1) into img -- a row-major u8
// buffer of the given width x height -- drawing every visited pixel in
// color. Both endpoints are drawn when inside the image; pixels beyond
// either axis (or with negative coordinates) are skipped, so callers
// never need to clip coordinates themselves.
//
// Classic Bresenham: integer arithmetic only, one advance per step.
inline void drawLine(const s32 x0, const s32 y0, const s32 x1, const s32 y1,
                     const u8 color, const u32 width, const u32 height,
                     u8 *img) {
  const s32 W = s32(width);
  const s32 H = s32(height);

  s32 x = x0, y = y0;
  s32 dx = ABS(x1 - x0), sx = (x0 < x1) ? 1 : -1;
  s32 dy = -ABS(y1 - y0), sy = (y0 < y1) ? 1 : -1;
  s32 err = dx + dy;

  for (;;) {
    if (x >= 0 && x < W && y >= 0 && y < H)
      img[y * W + x] = color;

    if (x == x1 && y == y1)
      break;

    s32 e2 = 2 * err;
    if (e2 >= dy) { err += dy; x += sx; }
    if (e2 <= dx) { err += dx; y += sy; }
  }
}
