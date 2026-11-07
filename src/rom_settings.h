#ifndef ROM_SETTINGS_H
#define ROM_SETTINGS_H

#include <allegro5/allegro.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include "platform.h"
#include "emulator.h"

inline std::string AbsoluteROMPath(const std::string &path) {
  if (path.empty()) return "";
  std::unique_ptr<ALLEGRO_FS_ENTRY, decltype(&al_destroy_fs_entry)> entry(
      al_create_fs_entry(path.c_str()), al_destroy_fs_entry);
  const char *absolute = entry ? al_get_fs_entry_name(entry.get()) : nullptr;
  return absolute ? absolute : path;
}

inline std::string ROMSettingsPath() {
  // Optional override lets tests and portable launches keep preferences isolated.
  const char *overridePath = std::getenv("GOMAS_CONFIG");
  if (overridePath) return AbsoluteROMPath(overridePath); // Empty disables saving.
  std::unique_ptr<ALLEGRO_PATH, decltype(&al_destroy_path)> directory(
      al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH), al_destroy_path);
  if (!directory) return "";
  al_set_path_filename(directory.get(), "settings.ini");
  return al_path_cstr(directory.get(), ALLEGRO_NATIVE_PATH_SEP);
}

struct ROMPreferences {
  bool restoreLast = true;
  std::string lastROM;
  bool lastCustom48 = false;
  int windowWidth = 640, windowHeight = 480, filter = 0;
  bool fullscreen = false, maximized = false, fastLoad = true;
  double playbackSpeed = 1;
  unsigned cpuMultiplier = 1;

  static int Integer(ALLEGRO_CONFIG *config, const char *section, const char *key,
      int fallback, int minimum, int maximum) {
    const char *value = al_get_config_value(config, section, key);
    if (!value || !*value) return fallback;
    char *end = nullptr; errno = 0;
    long parsed = std::strtol(value, &end, 10);
    return errno || *end || parsed < minimum || parsed > maximum ? fallback : int(parsed);
  }

  static ROMPreferences Read(const std::string &path) {
    ROMPreferences result;
    if (path.empty()) return result;
    std::unique_ptr<ALLEGRO_CONFIG, decltype(&al_destroy_config)> config(
        al_load_config_file(path.c_str()), al_destroy_config);
    if (!config) return result;
    const char *mode = al_get_config_value(config.get(), "startup", "rom");
    result.restoreLast = !mode || std::string(mode) != "48k";
    const char *rom = al_get_config_value(config.get(), "startup", "last_rom");
    if (rom) result.lastROM = rom;
    const char *custom = al_get_config_value(config.get(), "startup", "last_custom48");
    result.lastCustom48 = custom && std::string(custom) == "1";
    result.windowWidth = Integer(config.get(), "view", "width", 640, 320, 7680);
    result.windowHeight = Integer(config.get(), "view", "height", 480, 240, 4320);
    // Older settings captured GTK's menu-reduced initial height. Repair only
    // the known 2x/4x widths, once; arbitrary sizes from the new schema stay exact.
    if (Integer(config.get(), "view", "size_version", 0, 0, 2) < 2) {
      int height = result.windowWidth / 320 * 240;
      if ((result.windowWidth == 640 || result.windowWidth == 1280) &&
          result.windowHeight < height && result.windowHeight >= height - 64)
        result.windowHeight = height;
    }
    result.filter = Integer(config.get(), "view", "filter", 0, 0, 3);
    result.fullscreen = Integer(config.get(), "view", "fullscreen", 0, 0, 1);
    result.maximized = Integer(config.get(), "view", "maximized", 0, 0, 1);
    result.fastLoad = Integer(config.get(), "emulation", "fast_load", 1, 0, 1);
    int clock = Integer(config.get(), "emulation", "cpu_multiplier", 1, 1, 4);
    if (clock == 1 || clock == 2 || clock == 4) result.cpuMultiplier = clock;
    const char *speed = al_get_config_value(config.get(), "emulation", "speed");
    for (double allowed : {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0}) {
      if (speed && std::string(speed) == SpeedValue(allowed)) result.playbackSpeed = allowed;
    }
    return result;
  }

  static std::string SpeedValue(double value) {
    if (value == 0.25) return "0.25";
    if (value == 0.5) return "0.5";
    return std::to_string(static_cast<unsigned>(value));
  }

  void Remember(const std::string &path, bool custom48) {
    lastROM = AbsoluteROMPath(path);
    lastCustom48 = custom48;
  }

  bool Save(const std::string &path, std::string &error) const {
    error.clear();
    if (path.empty()) return true;
    if (!al_make_directory(ROMDirectory(path).c_str())) {
      error = "Cannot create settings directory: " + ROMDirectory(path);
      return false;
    }
    std::unique_ptr<ALLEGRO_CONFIG, decltype(&al_destroy_config)> config(
        al_load_config_file(path.c_str()), al_destroy_config);
    if (!config) config.reset(al_create_config());
    if (!config) { error = "Cannot allocate settings"; return false; }
    al_set_config_value(config.get(), "startup", "rom", restoreLast ? "last" : "48k");
    al_set_config_value(config.get(), "startup", "last_rom", lastROM.c_str());
    al_set_config_value(config.get(), "startup", "last_custom48", lastCustom48 ? "1" : "0");
    auto number = [&](const char *section, const char *key, int value) {
      al_set_config_value(config.get(), section, key, std::to_string(value).c_str());
    };
    number("view", "size_version", 2);
    number("view", "width", windowWidth); number("view", "height", windowHeight);
    number("view", "filter", filter); number("view", "fullscreen", fullscreen);
    number("view", "maximized", maximized);
    number("emulation", "fast_load", fastLoad);
    number("emulation", "cpu_multiplier", cpuMultiplier);
    al_set_config_value(config.get(), "emulation", "speed", SpeedValue(playbackSpeed).c_str());
    // Create beside the destination so rename stays on the same filesystem.
    std::string pattern = path + ".XXXXXX";
    std::vector<char> temporaryNameBuffer(pattern.begin(), pattern.end());
    temporaryNameBuffer.push_back('\0');
    int descriptor = Platform::Temporary(temporaryNameBuffer.data());
    if (descriptor < 0) { error = "Cannot write settings: " + path; return false; }
    std::string temporaryName = temporaryNameBuffer.data();
    ALLEGRO_FILE *temporary = al_fopen_fd(descriptor, "wb");
    if (!temporary) {
      Platform::Close(descriptor);
      al_remove_filename(temporaryName.c_str());
      error = "Cannot write settings: " + path;
      return false;
    }
    bool written = al_save_config_file_f(temporary, config.get());
    written = al_fflush(temporary) && written;
    written = al_fclose(temporary) && written;
    if (!written || Platform::Replace(temporaryName, path) != 0) {
      al_remove_filename(temporaryName.c_str());
      error = "Cannot save settings: " + path;
      return false;
    }
    return true;
  }
};

inline std::string Default48ROM(const std::string &executable) {
  auto path = FindROM("48.rom", executable);
  return path.empty() ? FindROM("48k.rom", executable) : path;
}

// An explicit command-line choice wins. A missing/unsupported remembered ROM
// falls back to 48K without discarding the remembered path (e.g. an external disk).
inline bool LoadStartupROM(SpectrumMachine &machine, const ROMPreferences &preferences,
    const std::string &explicitROM, bool custom48, const std::string &executable,
    std::string &error, std::string &notice) {
  notice.clear();
  if (!explicitROM.empty()) return machine.LoadROM(explicitROM, error, custom48);
  if (preferences.restoreLast && !preferences.lastROM.empty()) {
    if (machine.LoadROM(preferences.lastROM, error, preferences.lastCustom48)) return true;
    notice = "Cannot restore the last ROM. Using the default 48K ROM. " + error;
  }
  auto path = Default48ROM(executable);
  if (path.empty()) { error = "Cannot find roms/48.rom (or 48k.rom). Use --rom to specify a system ROM."; return false; }
  return machine.LoadROM(path, error);
}

#endif
