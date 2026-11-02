#ifndef SCREEN_VIEW_H
#define SCREEN_VIEW_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <algorithm>
#include <cmath>
#include <vector>

enum class ScreenFilter { None, ScanlinesLight, ScanlinesStrong, PixelGrid };

struct ScreenViewport {
  float x, y, scale;
};

// Presentation only: the original 320x240 ULA bitmap always stays unfiltered.
class ScreenView {
public:
  ScreenFilter filter = ScreenFilter::None;

  static ScreenViewport Viewport(int width, int height) {
    float scale = std::min(width / 320.0f, height / 240.0f);
    if (scale >= 1) scale = std::floor(scale);
    return {std::floor((width - 320 * scale) / 2),
            std::floor((height - 240 * scale) / 2), scale};
  }
  bool Fullscreen(ALLEGRO_DISPLAY *display) const {
    return (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW) != 0;
  }
  bool SetFullscreen(ALLEGRO_DISPLAY *display, bool enabled) {
    if (Fullscreen(display) == enabled) return true;
    if (enabled) {
      windowWidth = al_get_display_width(display);
      windowHeight = al_get_display_height(display);
    }
    // Keep the desktop's resolution and existing display/bitmap/audio resources.
    if (!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, enabled)) return false;
    return enabled || al_resize_display(display, windowWidth, windowHeight);
  }
  bool SetScale(ALLEGRO_DISPLAY *display, int scale) {
    if (scale != 2 && scale != 4) return false;
    if (!SetFullscreen(display, false)) return false;
    if (al_get_display_flags(display) & ALLEGRO_MAXIMIZED)
      if (!al_set_display_flag(display, ALLEGRO_MAXIMIZED, false)) return false;
    return al_resize_display(display, 320 * scale, 240 * scale);
  }
  bool IsScale(ALLEGRO_DISPLAY *display, int scale) const {
    return !Fullscreen(display) && al_get_display_width(display) == 320 * scale &&
        al_get_display_height(display) == 240 * scale;
  }
  void Draw(ALLEGRO_BITMAP *bitmap, int width, int height) const {
    if (width <= 0 || height <= 0) return;
    const auto v = Viewport(width, height);
    const float w = 320 * v.scale, h = 240 * v.scale;
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, v.x, v.y, w, h, 0);
    // At 1x or below there is no spare physical row for a readable pixel mask.
    if (filter == ScreenFilter::None || v.scale < 2) return;
    const bool grid = filter == ScreenFilter::PixelGrid;
    const int alpha = grid ? 48 : (filter == ScreenFilter::ScanlinesStrong ? 128 : 64);
    const ALLEGRO_COLOR color = al_map_rgba(0, 0, 0, alpha);
    const float thickness = grid ? 1 : v.scale / 2;
    std::vector<ALLEGRO_VERTEX> vertices;
    vertices.reserve(6 * (grid ? 560 : 240));
    auto rectangle = [&](float left, float top, float right, float bottom) {
      vertices.push_back({left, top, 0, 0, 0, color});
      vertices.push_back({right, top, 0, 0, 0, color});
      vertices.push_back({left, bottom, 0, 0, 0, color});
      vertices.push_back({right, top, 0, 0, 0, color});
      vertices.push_back({right, bottom, 0, 0, 0, color});
      vertices.push_back({left, bottom, 0, 0, 0, color});
    };
    for (int row = 0; row < 240; ++row) {
      float bottom = v.y + (row + 1) * v.scale;
      rectangle(v.x, bottom - thickness, v.x + w, bottom);
    }
    if (grid) for (int col = 0; col < 320; ++col) {
      float right = v.x + (col + 1) * v.scale;
      rectangle(right - thickness, v.y, right, v.y + h);
    }
    // A single draw call avoids hundreds of primitive submissions per refresh.
    al_draw_prim(vertices.data(), nullptr, nullptr, 0, vertices.size(), ALLEGRO_PRIM_TRIANGLE_LIST);
  }
private:
  int windowWidth = 640, windowHeight = 480;
};
#endif
