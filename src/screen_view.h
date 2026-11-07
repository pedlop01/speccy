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
  void RememberWindow(ALLEGRO_DISPLAY *display) {
    if (Fullscreen(display)) return;
    windowMaximized = (al_get_display_flags(display) & ALLEGRO_MAXIMIZED) != 0;
    if (!windowMaximized) {
      windowWidth = al_get_display_width(display);
      windowHeight = al_get_display_height(display);
    }
  }
  int WindowWidth() const { return windowWidth; }
  int WindowHeight() const { return windowHeight; }
  bool WindowMaximized() const { return windowMaximized; }
  void SetWindowDefaults(int width, int height, bool maximized) {
    windowWidth = width; windowHeight = height; windowMaximized = maximized;
  }
  bool Fullscreen(ALLEGRO_DISPLAY *display) const {
    return (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW) != 0;
  }
  bool SetFullscreen(ALLEGRO_DISPLAY *display, bool enabled) {
    if (Fullscreen(display) == enabled) return true;
    if (enabled) {
      RememberWindow(display);
    }
    // Keep the desktop's resolution and existing display/bitmap/audio resources.
    if (!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, enabled)) return false;
    if (enabled) return true;
    if (windowMaximized) return al_set_display_flag(display, ALLEGRO_MAXIMIZED, true);
    return al_resize_display(display, windowWidth, windowHeight);
  }
  bool SetScale(ALLEGRO_DISPLAY *display, int scale) {
    if (scale != 2 && scale != 4) return false;
    if (!SetFullscreen(display, false)) return false;
    if (al_get_display_flags(display) & ALLEGRO_MAXIMIZED)
      if (!al_set_display_flag(display, ALLEGRO_MAXIMIZED, false)) return false;
    bool resized = al_resize_display(display, 320 * scale, 240 * scale);
    if (resized) { windowWidth = 320 * scale; windowHeight = 240 * scale; windowMaximized = false; }
    return resized;
  }
  bool IsScale(ALLEGRO_DISPLAY *display, int scale) const {
    return !Fullscreen(display) && al_get_display_width(display) == 320 * scale &&
        al_get_display_height(display) == 240 * scale;
  }
  // VHS tracking bands are presentation only; never alter saved ULA pixels.
  void DrawRewind(ALLEGRO_BITMAP *bitmap, int width, int height, double time) const {
    if (width <= 0 || height <= 0) return;
    const auto v = Viewport(width, height);
    for (int band = 0; band < 4; ++band) {
      float row = std::fmod(time * 85 + band * 67, 240.0);
      float rows = std::min(6.0f, 240 - row);
      float shift = std::sin(time * 43 + band * 9) * 9;
      float left = std::max(0.0f, shift), right = std::min(320.0f, 320 + shift);
      al_draw_scaled_bitmap(bitmap, left - shift, row, right - left, rows,
          v.x + left * v.scale, v.y + row * v.scale,
          (right - left) * v.scale, rows * v.scale, 0);
      al_draw_filled_rectangle(v.x, v.y + row * v.scale, v.x + 320 * v.scale,
          v.y + (row + 1) * v.scale, al_map_rgba(220, 235, 255, 100));
      for (int line = 2; line < rows; line += 2)
        al_draw_filled_rectangle(v.x, v.y + (row + line) * v.scale,
            v.x + 320 * v.scale, v.y + (row + line + 0.5f) * v.scale,
            al_map_rgba(0, 0, 0, 120));
    }
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
  bool windowMaximized = false;
};
#endif
