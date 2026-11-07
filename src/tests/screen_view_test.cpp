#include <cassert>
#include <cstdio>
#include "../screen_view.h"

static int red(ALLEGRO_BITMAP *bitmap, int x, int y) {
  unsigned char r, g, b;
  al_unmap_rgb(al_get_pixel(bitmap, x, y), &r, &g, &b);
  assert(r == g && g == b);
  return r;
}
int main() {
  assert(al_init() && al_init_primitives_addon());
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  ALLEGRO_BITMAP *source = al_create_bitmap(320, 240);
  ALLEGRO_BITMAP *target = al_create_bitmap(1280, 1000);
  assert(source && target);
  al_set_target_bitmap(source); al_clear_to_color(al_map_rgb(255, 255, 255));
  ScreenView view;
  for (int scale : {2, 4}) {
    const int width = 320 * scale, height = 240 * scale;
    al_set_target_bitmap(target);
    view.filter = ScreenFilter::None; view.Draw(source, width, height);
    assert(red(target, 0, 0) == 255 && red(target, width - 1, height - 1) == 255);
    view.filter = ScreenFilter::ScanlinesLight; view.Draw(source, width, height);
    assert(red(target, 0, 0) == 255);
    int light = red(target, 0, scale - 1); assert(light >= 190 && light <= 192);
    assert(red(target, 0, scale) == 255);
    view.filter = ScreenFilter::ScanlinesStrong; view.Draw(source, width, height);
    assert(red(target, 0, 0) == 255);
    int strong = red(target, 0, scale - 1); assert(strong >= 126 && strong <= 128 && strong < light);
    view.filter = ScreenFilter::PixelGrid; view.Draw(source, width, height);
    assert(red(target, 0, 0) == 255 && red(target, scale - 1, 0) < 255);
    assert(red(target, 0, scale - 1) < 255);
    assert(red(target, scale - 1, scale - 1) < red(target, scale - 1, 0));
    assert(red(source, 0, 0) == 255 && red(source, 319, 239) == 255);
  }
  view.filter = ScreenFilter::ScanlinesStrong;
  view.Draw(source, 1000, 750); // Integer 3x, centered black bands.
  auto viewport = ScreenView::Viewport(1000, 750);
  assert(viewport.scale == 3 && viewport.x == 20 && viewport.y == 15);
  assert(red(target, 19, 20) == 0 && red(target, 20, 15) == 255);
  assert(red(target, 20, 17) < 255 && red(target, 980, 20) == 0);
  view.Draw(source, 319, 239); // Sub-1x still fits, with no unreadable pixel mask.
  auto small = ScreenView::Viewport(319, 239);
  assert(small.scale > 0 && small.scale < 1 && red(target, 100, 100) == 255);
  view.Draw(source, 0, 0); // Minimized/zero-area frames are harmless.
  view.Draw(source, 640, 480);
  view.DrawRewind(source, 640, 480, 1.25);
  assert(red(source, 0, 0) == 255 && red(source, 319, 239) == 255);
  view.DrawRewind(source, 0, 0, 2);
  al_destroy_bitmap(target); al_destroy_bitmap(source);
  al_shutdown_primitives_addon(); al_uninstall_system();
  puts("View: 2x/4x, integer viewport, scanline strengths, pixel grid, letterboxing and intact source passed");
}
