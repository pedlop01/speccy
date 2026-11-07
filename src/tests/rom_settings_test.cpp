#include <cassert>
#include <cstdio>
#include <unistd.h>
#include "../rom_settings.h"

static void Write(const std::string &path, const std::vector<unsigned char> &bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  assert(file.good());
}

int main() {
  assert(al_init());
  al_set_org_name("gomas-tests"); al_set_app_name("gomas-tests");
  auto standard = AbsoluteROMPath(FindROM("48.rom"));
  assert(!standard.empty());
  auto executable = ROMDirectory(standard) + "../bin/gomas";
  char temporary[] = "/tmp/gomas-settings-XXXXXX";
  assert(mkdtemp(temporary));
  std::string folder = std::string(temporary) + "/", config = folder + "config/settings.ini";
  const char *oldEnv = getenv("GOMAS_CONFIG");
  bool hadEnv = oldEnv != nullptr;
  std::string savedEnv = oldEnv ? oldEnv : "";
  setenv("GOMAS_CONFIG", config.c_str(), 1);
  assert(ROMSettingsPath() == AbsoluteROMPath(config));
  auto preferences = ROMPreferences::Read(config);
  assert(preferences.restoreLast && preferences.lastROM.empty());
  SpectrumMachine machine;
  std::string error, notice;
  assert(LoadStartupROM(machine, preferences, "", false, executable, error, notice));
  assert(machine.CanFastLoad() && notice.empty());
  std::vector<unsigned char> custom(16384, 0);
  custom[0] = 0x76;
  std::string alternative = folder + "custom ROM.rom";
  Write(alternative, custom);
  preferences.Remember(alternative, true);
  assert(preferences.Save(config, error));
  auto restored = ROMPreferences::Read(config);
  assert(restored.restoreLast && restored.lastCustom48 && restored.lastROM == alternative);
  // Full frontend configuration survives an atomic save/read round trip.
  restored.windowWidth = 1280; restored.windowHeight = 960;
  restored.filter = 3; restored.fullscreen = true; restored.maximized = true;
  restored.playbackSpeed = 0.25; restored.cpuMultiplier = 4; restored.fastLoad = false;
  assert(restored.Save(config, error));
  auto view = ROMPreferences::Read(config);
  assert(view.windowWidth == 1280 && view.windowHeight == 960 && view.filter == 3);
  assert(view.fullscreen && view.maximized && !view.fastLoad);
  assert(view.playbackSpeed == 0.25 && view.cpuMultiplier == 4);
  // Invalid or partially written values use safe defaults, not arbitrary sizes
  // or unsupported clock/speed values. Old ROM-only files remain compatible.
  std::string malformed = folder + "bad.ini";
  { std::ofstream out(malformed);
    out << "[view]\nwidth=999999999999999999999\nheight=-5\nfilter=4\nfullscreen=yes\n"
        << "[emulation]\nspeed=nan\ncpu_multiplier=3\nfast_load=2\n"; }
  auto bad = ROMPreferences::Read(malformed);
  assert(bad.windowWidth == 640 && bad.windowHeight == 480 && bad.filter == 0 && !bad.fullscreen);
  assert(bad.playbackSpeed == 1 && bad.cpuMultiplier == 1 && bad.fastLoad);
  { std::ofstream out(malformed); out << "[view]\nwidth=640\nheight=451\n"; }
  auto legacy = ROMPreferences::Read(malformed);
  assert(legacy.windowWidth == 640 && legacy.windowHeight == 480);
  { std::ofstream out(malformed); out << "[view]\nwidth=1280\nheight=931\n"; }
  assert(ROMPreferences::Read(malformed).windowHeight == 960);
  { std::ofstream out(malformed); out << "[view]\nsize_version=1\nwidth=640\nheight=451\n"; }
  assert(ROMPreferences::Read(malformed).windowHeight == 451);
  unlink(malformed.c_str());
  char *cwd = al_get_current_directory(); assert(cwd);
  std::string originalDirectory = cwd; al_free(cwd);
  assert(al_change_directory("/tmp"));
  assert(LoadStartupROM(machine, restored, "", false, executable, error, notice));
  assert(machine.systemROM.source == alternative && !machine.CanFastLoad());
  restored.restoreLast = false;
  assert(restored.Save(config, error));
  auto fixed = ROMPreferences::Read(config);
  assert(!fixed.restoreLast && fixed.lastROM == alternative);
  assert(LoadStartupROM(machine, fixed, "", false, executable, error, notice));
  assert(machine.CanFastLoad() && notice.empty());
  // Explicit choices override both startup policies; invalid explicit choices
  // fail rather than silently executing another ROM.
  assert(LoadStartupROM(machine, fixed, alternative, true, executable, error, notice));
  assert(!machine.CanFastLoad());
  machine.cpu.regs.PC = 0x9000; machine.bus.Write(0x8000, 0x42);
  assert(!LoadStartupROM(machine, fixed, folder + "missing.rom", false, executable, error, notice));
  assert(machine.cpu.regs.PC == 0x9000 && machine.bus.Read(0x8000) == 0x42);
  restored.restoreLast = true;
  restored.lastROM = folder + "missing.rom";
  assert(LoadStartupROM(machine, restored, "", false, executable, error, notice));
  assert(machine.CanFastLoad() && !notice.empty());
  assert(restored.lastROM == folder + "missing.rom");
  restored.lastROM = ROMDirectory(standard) + "128-0.rom";
  restored.lastCustom48 = false;
  assert(LoadStartupROM(machine, restored, "", false, executable, error, notice));
  assert(machine.CanFastLoad() && notice.empty());
  assert(machine.Configuration().model == MachineModel::Spectrum128);
  // A failed save leaves the existing preferences intact, and temporary files
  // are removed even when the destination happens to be a directory.
  assert(!restored.Save(folder + "config", error));
  assert(ROMPreferences::Read(config).lastROM == alternative);
  assert(restored.Save("", error));
  setenv("GOMAS_CONFIG", "", 1); assert(ROMSettingsPath().empty());

  // Also support the user's 48k.rom naming convention in a portable layout.
  std::string portable = folder + "portable/roms/";
  assert(al_make_directory(portable.c_str()));
  ROMSet image; assert(LoadROMSet(standard, image, error));
  Write(portable + "48k.rom", image.pages[0]);
  assert(al_make_directory((folder + "portable/bin").c_str()));
  auto alternateExecutable = folder + "portable/bin/gomas";
  auto defaultPath = Default48ROM(alternateExecutable);
  assert(defaultPath.find("48k.rom") != std::string::npos);
  assert(LoadROMSet(defaultPath, image, error));
  assert(al_change_directory(originalDirectory.c_str()));
  unlink((portable + "48k.rom").c_str()); rmdir(portable.c_str());
  rmdir((folder + "portable/bin").c_str());
  rmdir((folder + "portable").c_str());
  unlink(alternative.c_str()); unlink(config.c_str());
  rmdir((folder + "config").c_str()); rmdir(temporary);
  if (hadEnv) setenv("GOMAS_CONFIG", savedEnv.c_str(), 1); else unsetenv("GOMAS_CONFIG");
  al_uninstall_system();
  puts("Startup ROM: persistence, custom ROMs, priorities, 48K fallback, paths and save failures passed");
}
