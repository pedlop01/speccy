# Windows cross-build

The same Makefile has separate Linux and Windows targets:

```sh
make -C bin                      # Linux: bin/gomas
make -C bin windows              # Windows x64: bin/windows/gomas.exe
make -C bin clean-windows        # Remove generated files from both Windows builds
```

## Quick start on Arch Linux

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

## Static build and cleaning

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

### C++ runtime ABI

Arch's current compiler defaults to UCRT, whereas this SDK uses MSVCRT. Mixing
its C++ import library with MSYS2's runtime DLL caused the missing entry point
`std::istream::seekg` (`fpos<_Mbstatet>` versus `fpos<int>`).

The local SDK now supplies matching C++ headers, static/import libraries and CRT
files. `WIN_ABI_FLAGS` selects MSVCRT and those files consistently. The compiler
version must match the SDK GCC runtime version; `windows-check` verifies that.
The shared bundler additionally checks mangled C++ imports against the supplied
DLL exports, so this mismatch fails during packaging rather than at launch.

## Using a different dependency prefix

The compiler defaults to `x86_64-w64-mingw32-g++`. Without a prepared local SDK,
the dependency prefix defaults to `/usr/x86_64-w64-mingw32`. It must contain
Windows headers, libraries, and pkg-config files for Allegro core/audio/native
dialogs/fonts/primitives/images, libarchive, zlib, libcurl, OpenSSL libcrypto and
nlohmann/json. giflib needs a header and library but no pkg-config file.

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

## Trying the Windows build

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

## Verification

`src/platform.h` handles temporary files, flushing, replacing existing files,
directories, executable discovery and date formatting. Windows replacement uses
MoveFileExW rather than the CRT's non-overwriting rename.

Both shared and static executables have been cross-compiled as Windows x64
(PE32+). Shared C++ imports are checked against bundled DLL exports, and the
static EXE is checked to import only Windows system DLLs. Cleanup and C++ import
parsing are covered by `make -C bin test-windows-build`.
**It has not been run on Windows or Wine yet.** Native Windows validation should
cover menus/dialogs, audio, resizing/fullscreen, ROM lookup, repeated save/load,
settings updates and catalog HTTPS/install/uninstall.

`test` and `test-ui` remain native Linux tests; the latter requires X11/XTest.

The catalog also uses Expat for the public itch.io RSS feed. The dependency
helper includes it in the Windows SDK; if reusing an older SDK, rerun
`python3 tools/windows_deps.py` before building. The source selector and
unified installed library work in both Linux and Windows builds. itch.io
opens the creator page in the default browser; ZXArt/ZXInfo downloads use
the existing HTTPS cache.
