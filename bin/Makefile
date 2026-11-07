CXX ?= g++
CXXFLAGS ?= -O2 -std=c++11
PACKAGES = allegro-5 allegro_audio-5 allegro_dialog-5 allegro_font-5 allegro_primitives-5 allegro_image-5 libarchive zlib libcurl libcrypto nlohmann_json expat
CPPFLAGS += $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES)) -pthread -lgif
# Monitor refresh discovery on the native Linux/X11 frontend only.
CPPFLAGS += $(shell pkg-config --cflags xrandr x11)
LDLIBS += $(shell pkg-config --libs xrandr x11)
SOURCES = ../src/main.cpp ../src/z80.cpp ../src/ula.cpp ../src/monitor_refresh.cpp
HEADERS = $(wildcard ../src/*.h ../src/*.hpp)

.PHONY: all clean test test-ui
all: gomas

gomas: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES) $(LDLIBS)

test: monitor-sync-tests raster-tests catalog-sources-tests joystick-tests ula-tests floating-bus-tests machine-tests tape-tests tape-game-tests rom-loader-tests rom-settings-tests machine-config-tests paging-tests video128-tests ay-tests tape128-tests compat128-tests z80-index-flags-tests capitan-tests z80-block-compare-tests bombjack-tests screen-view-tests playback-tests save-state-tests tape-zip-tests formats-tests new-media-tests catalog-tests debug-tests
	./monitor-sync-tests
	./raster-tests
	./catalog-sources-tests
	./joystick-tests
	./ula-tests
	./floating-bus-tests
	./machine-tests
	./tape-tests
	./tape-game-tests
	./rom-loader-tests
	./rom-settings-tests
	./machine-config-tests
	./paging-tests
	./video128-tests
	./ay-tests
	./tape128-tests
	./compat128-tests
	./z80-index-flags-tests
	./capitan-tests
	./z80-block-compare-tests
	./bombjack-tests
	./screen-view-tests
	./playback-tests
	./save-state-tests
	./tape-zip-tests
	./formats-tests
	./new-media-tests
	./catalog-tests
	./debug-tests

ula-tests: ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

floating-bus-tests: ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

machine-tests: ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape-tests: ../src/tests/tape_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape-game-tests: ../src/tests/tape_game_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_game_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

rom-loader-tests: ../src/tests/rom_loader_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/rom_loader_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

rom-settings-tests: ../src/tests/rom_settings_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/rom_settings_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

machine-config-tests: ../src/tests/machine_config_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/machine_config_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

test-ui: tape-ui-tests
	./tape-ui-tests

tape-ui-tests: ../src/monitor_refresh.cpp ../src/tests/tape_ui_test.cpp ../src/tests/catalog_art_fixture.h ../src/main.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_ui_test.cpp ../src/ula.cpp ../src/z80.cpp ../src/monitor_refresh.cpp $(LDLIBS) $(shell pkg-config --libs x11 xtst) -pthread

clean: clean-windows
	rm -f *.o gomas monitor-sync-tests raster-tests topcat-tests catalog-sources-tests joystick-tests ula-tests floating-bus-tests machine-tests tape-tests tape-game-tests tape-ui-tests rom-loader-tests rom-settings-tests machine-config-tests paging-tests video128-tests ay-tests tape128-tests compat128-tests z80-index-flags-tests capitan-tests z80-block-compare-tests bombjack-tests screen-view-tests playback-tests save-state-tests tape-zip-tests formats-tests new-media-tests catalog-tests debug-tests

paging-tests: ../src/tests/paging_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/paging_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

video128-tests: ../src/tests/video128_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/video128_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

ay-tests: ../src/tests/ay_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/ay_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape128-tests: ../src/tests/tape128_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape128_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

compat128-tests: ../src/tests/compat128_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/compat128_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

z80-index-flags-tests: ../src/tests/z80_index_flags_test.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/z80_index_flags_test.cpp ../src/z80.cpp $(LDLIBS)

capitan-tests: ../src/tests/capitan_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/capitan_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

z80-block-compare-tests: ../src/tests/z80_block_compare_test.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/z80_block_compare_test.cpp ../src/z80.cpp $(LDLIBS)

bombjack-tests: ../src/tests/bombjack_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/bombjack_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

screen-view-tests: ../src/tests/screen_view_test.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/screen_view_test.cpp $(LDLIBS)

playback-tests: ../src/tests/playback_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/playback_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

save-state-tests: ../src/tests/save_state_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/save_state_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape-zip-tests: ../src/tests/tape_zip_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_zip_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

formats-tests: ../src/tests/formats_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/formats_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

new-media-tests: ../src/tests/new_media_test.cpp ../src/tests/formats_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/new_media_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

catalog-tests: ../src/tests/catalog_test.cpp ../src/tests/catalog_art_fixture.h ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/catalog_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

debug-tests: ../src/tests/debug_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/debug_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

# Linux -> Windows x64. The sysroot must contain WINDOWS builds of all packages;
# never fall back to Linux pkg-config paths. Native targets above are unchanged.
WIN_PREFIX ?= x86_64-w64-mingw32
WIN_CXX ?= $(WIN_PREFIX)-g++
WIN_SYSROOT ?= $(if $(wildcard ../.windows-sdk/prefix/gomas-sdk.json),$(abspath ../.windows-sdk/prefix),/usr/$(WIN_PREFIX))
WIN_PKG_CONFIG ?= pkg-config
WIN_PACKAGES ?= $(PACKAGES)
WIN_PKG_ENV = env PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$(WIN_SYSROOT)/lib/pkgconfig:$(WIN_SYSROOT)/share/pkgconfig" PKG_CONFIG_SYSROOT_DIR="$(WIN_SYSROOT)"
WIN_GCC_VERSION = $(notdir $(wildcard $(WIN_SYSROOT)/include/c++/*))
WIN_ABI_FLAGS ?= $(if $(wildcard $(WIN_SYSROOT)/gomas-sdk.json),-mcrtdll=msvcrt-os -B"$(WIN_SYSROOT)/lib/" -isystem "$(WIN_SYSROOT)/include/c++/$(WIN_GCC_VERSION)" -isystem "$(WIN_SYSROOT)/include/c++/$(WIN_GCC_VERSION)/x86_64-w64-mingw32",)
WIN_CXXFLAGS ?= $(CXXFLAGS) $(WIN_ABI_FLAGS)
WIN_CPPFLAGS ?= -DNOMINMAX
WIN_LDFLAGS ?=
WIN_LDLIBS ?= -pthread -lgif -lshell32
WIN_OUTPUT = windows/gomas.exe

.PHONY: windows windows-check clean-windows
windows: $(WIN_OUTPUT)

windows-check:
	@command -v "$(WIN_CXX)" >/dev/null || { echo "Missing Windows compiler: $(WIN_CXX). Install a MinGW-w64 POSIX-thread toolchain."; exit 1; }
	@"$(WIN_CXX)" -dumpmachine | grep -q mingw || { echo "WIN_CXX must target Windows (MinGW-w64)."; exit 1; }
	@if test -f "$(WIN_SYSROOT)/gomas-sdk.json"; then test -f "$(WIN_SYSROOT)/lib/libstdc++.a" || { echo "Matching C++ runtime missing; rerun make windows-deps."; exit 1; }; fi
	@if test -f "$(WIN_SYSROOT)/gomas-sdk.json"; then test "$$($(WIN_CXX) -dumpfullversion)" = "$(WIN_GCC_VERSION)" || { echo "Compiler and SDK GCC runtime versions differ; update the toolchain and rerun windows-deps."; exit 1; }; fi
	@$(WIN_PKG_ENV) $(WIN_PKG_CONFIG) --print-errors --exists $(WIN_PACKAGES) || { echo "Missing Windows libraries in WIN_SYSROOT=$(WIN_SYSROOT). See BUILD.md."; exit 1; }
	@printf '\043include <gif_lib.h>\n' | "$(WIN_CXX)" $(WIN_CPPFLAGS) -isystem "$(WIN_SYSROOT)/include" -x c++ -fsyntax-only - || { echo "Missing Windows giflib headers."; exit 1; }

$(WIN_OUTPUT): $(SOURCES) $(HEADERS) Makefile $(wildcard $(WIN_SYSROOT)/gomas-sdk.json) | windows-check
	mkdir -p windows
	$(WIN_CXX) $(WIN_CPPFLAGS) $(WIN_CXXFLAGS) -isystem "$(WIN_SYSROOT)/include" -o $@ $(SOURCES) -L"$(WIN_SYSROOT)/lib" -L"$(WIN_SYSROOT)/lib/gcc/$(WIN_PREFIX)/$(WIN_GCC_VERSION)" $(WIN_LDFLAGS) \
	  $$($(WIN_PKG_ENV) $(WIN_PKG_CONFIG) --cflags --libs $(WIN_PACKAGES)) $(WIN_LDLIBS)

clean-windows:
	rm -f windows/*.exe windows/*.dll windows/*.crt windows-static/*.exe windows-static/*.dll windows-static/*.crt
	rm -rf windows/licenses windows-static/licenses
	@rmdir windows windows-static 2>/dev/null || true

.PHONY: windows-deps
windows-deps:
	python3 ../tools/windows_deps.py

.PHONY: windows-bundle
windows-bundle: windows
	python3 ../tools/windows_bundle.py --sdk "$(WIN_SYSROOT)" --exe "$(WIN_OUTPUT)" --prefix "$(WIN_PREFIX)"

# Full static application/dependency link. Only Windows system DLLs may remain.
WIN_STATIC_OUTPUT = windows-static/gomas.exe
WIN_STATIC_ALLEGRO = -lallegro_audio-static -lallegro_dialog-static -lallegro_font-static -lallegro_primitives-static -lallegro_image-static -lallegro-static
WIN_STATIC_SYSTEM = -lwinmm -luser32 -lgdi32 -lopengl32 -ld3d9 -ldinput8 -ldsound -ldxguid -lcomdlg32 -lshell32 -lole32 -loleaut32 -luuid -lshlwapi -lcomctl32 -lpsapi -lgdiplus
.PHONY: windows-static windows-static-check
windows-static-check: windows-check
	@test -f "$(WIN_SYSROOT)/allegro-deps/lib/libpng16.a" || { echo "Static Allegro dependencies missing. Run make windows-deps again."; exit 1; }

$(WIN_STATIC_OUTPUT): $(SOURCES) $(HEADERS) Makefile $(wildcard $(WIN_SYSROOT)/gomas-sdk.json) $(WIN_SYSROOT)/static/lib/libarchive.a | windows-static-check
	mkdir -p windows-static
	$(WIN_CXX) $(WIN_CPPFLAGS) -DALLEGRO_STATICLINK -DCURL_STATICLIB -DLIBARCHIVE_STATIC $(WIN_CXXFLAGS) \
	  -isystem "$(WIN_SYSROOT)/include" -o $@ $(SOURCES) $(WIN_LDFLAGS) -static -static-libgcc -static-libstdc++ \
	  -L"$(WIN_SYSROOT)/static/lib" -L"$(WIN_SYSROOT)/lib" -L"$(WIN_SYSROOT)/lib/gcc/$(WIN_PREFIX)/$(WIN_GCC_VERSION)" -L"$(WIN_SYSROOT)/allegro-deps/lib" -Wl,--start-group \
	  $(WIN_STATIC_ALLEGRO) -lpng16 -ljpeg -lwebp -lsharpyuv \
	  $$($(WIN_PKG_ENV) $(WIN_PKG_CONFIG) --cflags --libs --static $(filter-out allegro%,$(WIN_PACKAGES))) \
	  $(WIN_LDLIBS) $(WIN_STATIC_SYSTEM) -Wl,--end-group

windows-static: $(WIN_STATIC_OUTPUT)
	python3 ../tools/windows_bundle.py --sdk "$(WIN_SYSROOT)" --exe "$(WIN_STATIC_OUTPUT)" --prefix "$(WIN_PREFIX)" --assert-static

$(WIN_SYSROOT)/static/lib/libarchive.a: ../tools/windows_static_deps.py $(wildcard $(WIN_SYSROOT)/gomas-sdk.json)
	python3 ../tools/windows_static_deps.py --sdk "$(WIN_SYSROOT)" --prefix "$(WIN_PREFIX)"

.PHONY: test-windows-build
test-windows-build:
	python3 ../src/tests/windows_build_test.py

joystick-tests: ../src/tests/joystick_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/joystick_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

catalog-sources-tests: ../src/tests/catalog_sources_test.cpp ../src/tests/catalog_art_fixture.h ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/catalog_sources_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

raster-tests: ../src/tests/raster_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/raster_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

# Optional fixture test: run with the downloaded ZXArt 48K Top Cat TAP path.
topcat-tests: ../src/tests/topcat_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/topcat_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

monitor-sync-tests: ../src/tests/monitor_sync_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/monitor_sync_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)
