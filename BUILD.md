# Building Gomas

Gomas uses one Makefile, `bin/Makefile`, for native Linux builds and separate
Windows x64 cross-builds. Run the commands below from the repository root.

- [Linux dependencies](#linux-dependencies)
- [Build and run on Linux](#build-and-run-on-linux)
- [Tests](#tests)
- [Cleaning](#cleaning)
- [Windows cross-build from Linux](#windows-cross-build-from-linux)

## Linux dependencies

Use a C++11 compiler, GNU make and pkg-config. The native frontend needs a
working graphical session (X11 or XWayland), OpenGL and an audio device.

| Dependency | Purpose / pkg-config modules |
| --- | --- |
| Allegro 5 core, audio, native dialogs, font, primitives and image addons | `allegro-5`, `allegro_audio-5`, `allegro_dialog-5`, `allegro_font-5`, `allegro_primitives-5`, `allegro_image-5` |
| giflib | GIF catalog artwork; linked as `-lgif` |
| libarchive and zlib | Compressed games and snapshots; `libarchive`, `zlib` |
| [libcurl 7.85 or newer](https://curl.se/libcurl/c/CURLOPT_PROTOCOLS_STR.html#Availability) | HTTPS catalog access; `libcurl` |
| OpenSSL libcrypto | Download checksums; `libcrypto` |
| nlohmann/json | Catalog metadata; `nlohmann_json` |
| Expat | itch.io discovery feed; `expat` |
| X11 and XRandR | Current monitor refresh detection; `x11`, `xrandr` |

The Makefile adds pthread support. Allegro's acodec, TTF, video and PhysFS addons
are not required. CA certificates are needed for catalog HTTPS; `xdg-open`
(from `xdg-utils`) opens source pages in the default browser.

### Arch Linux

Arch packages include both runtime libraries and development headers:

```sh
sudo pacman -Syu --needed base-devel git pkgconf allegro giflib libarchive \
  zlib curl openssl nlohmann-json expat libx11 libxrandr ca-certificates xdg-utils
```

The `allegro` package includes all required addons and pulls in GTK for the
native menus/dialogs. On a Wayland desktop, install XWayland if it is missing:

```sh
sudo pacman -S --needed xorg-xwayland
```

### Ubuntu (24.04 LTS or newer)

Install the development packages, including the separate Allegro addons:

```sh
sudo apt update
sudo apt install build-essential git pkg-config \
  liballegro5-dev liballegro-audio5-dev liballegro-dialog5-dev liballegro-image5-dev \
  libgif-dev libarchive-dev zlib1g-dev libcurl4-openssl-dev libssl-dev \
  nlohmann-json3-dev libexpat1-dev libx11-dev libxrandr-dev ca-certificates xdg-utils
```

Allegro is in Ubuntu's **Universe** repository. If apt cannot locate it, enable
Universe, then repeat the update and installation:

```sh
sudo apt install software-properties-common
sudo add-apt-repository universe
sudo apt update
```

For a Wayland session, install XWayland if necessary:

```sh
sudo apt install xwayland
```

These package names were checked against Ubuntu 24.04's package index; the build
has been exercised on Arch Linux. Ubuntu 22.04's stock libcurl is older than the
API used by the catalog; use Ubuntu 24.04 or newer for these instructions.

Package references: [Arch Allegro](https://archlinux.org/packages/extra/x86_64/allegro/),
[Ubuntu Allegro](https://packages.ubuntu.com/noble/liballegro5-dev),
[Ubuntu audio addon](https://packages.ubuntu.com/noble/liballegro-audio5-dev),
[Ubuntu dialog addon](https://packages.ubuntu.com/noble/liballegro-dialog5-dev),
[Ubuntu image addon](https://packages.ubuntu.com/noble/liballegro-image5-dev).

## Build and run on Linux

If you have not cloned the repository yet:

```sh
git clone https://github.com/pedlop01/speccy.git
cd speccy
```

Build and launch:

```sh
make -C bin -j2
./bin/gomas
```

The executable is `bin/gomas`; no installation step or sudo is needed to build
or run it. The default compiler flags are `-O2 -std=c++11`. Adjust the parallel
job count to suit the available memory, or omit `-j2` for a serial build.

Keep the `roms/` directory alongside `bin/`. Startup restores the last selected
ROM when possible, otherwise it looks for `roms/48.rom` (or `48k.rom`). Multi-page
ROM sets must keep their sibling pages together. An explicit ROM can also be
selected on the command line:

```sh
./bin/gomas --rom roms/48.rom
./bin/gomas bin/cobra.tap --rom roms/48.rom
```

A tape opened this way is inserted for manual loading; File > Game catalog
provides its own automatic loading flow. See [src/README.txt](src/README.txt)
for ROM layouts, keyboard mappings, media formats and menu options.

On Wayland, the frontend selects `GDK_BACKEND=x11` for Allegro's native menus.
XWayland and a valid `DISPLAY` must be available; this is a graphical application,
not a terminal-only executable.

### Checking dependencies

This read-only check lists missing pkg-config modules:

```sh
pkg-config --print-errors --exists \
  allegro-5 allegro_audio-5 allegro_dialog-5 allegro_font-5 \
  allegro_primitives-5 allegro_image-5 libarchive zlib libcurl libcrypto \
  nlohmann_json expat x11 xrandr
```

It prints nothing and returns success when all modules are available. giflib
has no required pkg-config entry; install its headers/library using the package
listed above. If a module is missing, install its native development package.
Windows libraries and a Windows sysroot cannot substitute for Linux libraries.

## Tests

The native regression suite builds its own executables and runs from `bin/`:

```sh
make -C bin -j2 test
```

Keep the repository's ROMs and tape fixtures in place. Some tests create Allegro
memory bitmaps but do not need a visible window; the standard suite does not
query live game repositories. The optional `--live` catalog checks require
Internet access and are not part of `make test`.

The UI integration test additionally needs XTest development files:

```sh
# Arch Linux
sudo pacman -S --needed libxtst

# Ubuntu
sudo apt install libxtst-dev

make -C bin test-ui
```

Run `test-ui` in an X11/XWayland graphical session with a working `DISPLAY` and
audio device. It opens windows and injects mouse/keyboard input; use a dedicated
test session where possible. It isolates its settings, save files and catalog
from the user's normal configuration.

Windows packaging helpers have a separate native Python test target (Python 3
is required):

```sh
make -C bin test-windows-build
```

## Cleaning

```sh
make -C bin clean
```

This removes native build/test outputs and generated Windows release artifacts.
ROMs, game files, configuration, saved games and `.windows-sdk` are preserved.
Use `make -C bin clean-windows` to clean only the Windows release directories.

## Windows cross-build from Linux

The same Makefile has separate Linux and Windows targets:

```sh
make -C bin                      # Linux: bin/gomas
make -C bin windows              # Windows x64: bin/windows/gomas.exe
make -C bin clean-windows        # Remove generated files from both Windows builds
```

### Windows toolchain (Arch Linux)

After installing the native build tools above, install the cross compiler and
SDK preparation tools:

```sh
sudo pacman -S --needed mingw-w64-gcc python libarchive cmake
```

The project-local SDK workflow is verified on Arch Linux. Other Linux hosts
need a MinGW-w64 POSIX-thread compiler whose GCC version matches the SDK runtime
version selected by the helper; the stock Ubuntu cross compiler may not match.
`windows-check` reports a mismatch before compilation.

With the MinGW-w64 POSIX-thread compiler installed:

```sh
make -C bin windows-deps
make -C bin windows-check
make -C bin windows-bundle
```

`windows-deps` needs Python 3 and `bsdtar` (Arch's `libarchive` package). It
prepares `.windows-sdk/prefix` inside the project, without sudo or installing
Windows packages into the host system. The Makefile selects this SDK automatically.
The initial download is about 140 MB; the SDK uses about 730 MB before static build intermediates.
Repeat runs reuse downloads whose checksums still match the repository metadata.

The SDK combines the official Allegro 5.2.10.1 Windows binary release, its static dependency archive, and matching GCC runtime/header files with
MSYS2 **MINGW64 / MSVCRT** libraries (not UCRT64). Allegro's archive is pinned by
SHA-256; MSYS2 packages are checked against hashes in its HTTPS repository
database. This is not pacman's GPG signature verification. No package install
scripts or Windows executables are run. A manifest records package versions and
hashes at `.windows-sdk/prefix/gomas-sdk.json`. Re-running setup refreshes MSYS2
metadata, so future runs can select newer dependency versions.

The Allegro download's name includes `static` for bundled third-party libraries.
Our generated pkg-config entries link Allegro's **DLL import libraries**, and the
Allegro DLLs are included by `windows-bundle`.

Sources:
- https://github.com/liballeg/allegro5/releases/tag/5.2.10.1
- https://repo.msys2.org/mingw/mingw64/

### Static build and cleaning

```sh
make -C bin clean-windows       # Clear shared and static release artifacts
make -C bin windows-deps        # Refresh SDK, including matching C++ runtime
make -C bin windows-static      # bin/windows-static/gomas.exe
```

The static target links Allegro, the C++ runtime, threading and the media/network
libraries into the EXE. It automatically builds libarchive from the pinned
3.8.11 source with static zlib references; the MSYS2 archive otherwise retains
some references to zlib's DLL import table. This step additionally needs CMake,
GNU make and the cross C compiler. Sources and incremental build files are kept
inside `.windows-sdk/static-work`.

An import-table check rejects third-party DLL dependencies in the static EXE.
Windows system DLLs (graphics, audio, networking and the Windows C runtime) are
still used. Windows HTTPS uses the native trusted certificate store. The bundled
`curl-ca-bundle.crt` is optional additional trust material; ROMs remain separate. Copy the `windows-static` folder to Windows; do not reuse an
older shared-build folder with stale DLLs.

`make -C bin clean` cleans Linux outputs AND generated files in both Windows
release directories. `clean-windows` cleans only the Windows releases. Both
remove generated EXEs, DLLs, certificates and license directories, but preserve
ROM directories, personal files and the SDK/download cache. Nonempty release
directories are therefore intentionally retained.

#### C++ runtime ABI

Arch's current compiler defaults to UCRT, whereas this SDK uses MSVCRT. Mixing
its C++ import library with MSYS2's runtime DLL caused the missing entry point
`std::istream::seekg` (`fpos<_Mbstatet>` versus `fpos<int>`).

The local SDK now supplies matching C++ headers, static/import libraries and CRT
files. `WIN_ABI_FLAGS` selects MSVCRT and those files consistently. The compiler
version must match the SDK GCC runtime version; `windows-check` verifies that.
The shared bundler additionally checks mangled C++ imports against the supplied
DLL exports, so this mismatch fails during packaging rather than at launch.

### Using a different dependency prefix

The compiler defaults to `x86_64-w64-mingw32-g++`. Without a prepared local SDK,
the dependency prefix defaults to `/usr/x86_64-w64-mingw32`. It must contain
Windows headers, libraries, and pkg-config files for Allegro core/audio/native
dialogs/fonts/primitives/images, libarchive, zlib, libcurl, OpenSSL libcrypto and
nlohmann/json and Expat. giflib needs a header and library but no pkg-config file.

```sh
make -C bin windows-check WIN_SYSROOT=/opt/speccy-win64
make -C bin windows WIN_SYSROOT=/opt/speccy-win64
```

Other overrides: `WIN_PREFIX`, `WIN_CXX`, `WIN_PKG_CONFIG`, `WIN_PACKAGES`,
`WIN_CPPFLAGS`, `WIN_CXXFLAGS`, `WIN_ABI_FLAGS`, `WIN_LDFLAGS`, `WIN_LDLIBS`.
Custom SDKs must match their compiler/CRT ABI. The automated static dependency
build is configured for the project-local MSVCRT SDK.
Windows pkg-config lookup is isolated from Linux dependencies. Metadata should
use target-relative prefixes (e.g. `/`), which `PKG_CONFIG_SYSROOT_DIR` maps into
the SDK. A static Allegro build needs different metadata and link options; the
default target uses shared-library metadata.

### Trying the Windows build

`windows-bundle` follows DLL imports recursively with the cross-toolchain's
`objdump`, copying required dependency DLLs beside `bin/windows/gomas.exe`.
It also copies dependency license files available in the SDK and a public CA
bundle from the host. The Windows catalog uses the Windows trusted certificate
store, with the optional bundle added when readable. It reads that bundle using
Allegro's UTF-8 file API and passes its contents to curl, supporting accented
installation paths. Certificate and hostname verification remain enabled.
Connection errors are displayed in the empty game list as well as the footer.

Copy the **whole** `bin/windows` directory to a Windows computer, then add your
ROMs in a `roms` directory beside the EXE:

```text
gomas.exe
<required DLLs>
curl-ca-bundle.crt
licenses/
roms/
```

ROMs and games are not copied by the build target. Settings and saved games use
Allegro's Windows user directories. GOMAS_CONFIG, GOMAS_SAVE_DIR and
GOMAS_GAMES_DIR overrides remain available. Launching the EXE directly on Linux
requires Wine, which is not installed automatically.

### Verification

`src/platform.h` handles temporary files, flushing, replacing existing files,
directories, executable discovery and date formatting. Windows replacement uses
MoveFileExW rather than the CRT's non-overwriting rename.

Both shared and static executables have been cross-compiled as Windows x64
(PE32+). Shared C++ imports are checked against bundled DLL exports, and the
static EXE is checked to import only Windows system DLLs. Cleanup and C++ import
parsing are covered by `make -C bin test-windows-build`.
These build checks do not execute the EXE. Smoke-test new Windows builds for
menus/dialogs, audio, resizing/fullscreen, ROM lookup, repeated save/load,
settings updates and catalog HTTPS/install/uninstall.

`test` and `test-ui` remain native Linux tests; the latter requires X11/XTest.

The catalog also uses Expat for the public itch.io RSS feed. The dependency
helper includes it in the Windows SDK; if reusing an older SDK, rerun
`python3 tools/windows_deps.py` before building. The source selector and
unified installed library work in both Linux and Windows builds. itch.io
opens the creator page in the default browser; ZXArt/ZXInfo downloads use
the existing HTTPS cache.

Monitor synchronization uses the Windows monitor containing the emulator window
and the active mode reported by EnumDisplaySettingsExW. It needs no additional
Windows SDK packages. The Linux build additionally uses XRandR/X11 for refresh
discovery; those libraries are excluded from the Windows cross-build.
