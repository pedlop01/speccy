#ifndef ROM_SETTINGS_H
#define ROM_SETTINGS_H

#include <allegro5/allegro.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <unistd.h>
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
    return result;
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
    // Create beside the destination so rename stays on the same filesystem.
    std::string pattern = path + ".XXXXXX";
    std::vector<char> temporaryNameBuffer(pattern.begin(), pattern.end());
    temporaryNameBuffer.push_back('\0');
    int descriptor = mkstemp(temporaryNameBuffer.data());
    if (descriptor < 0) { error = "Cannot write settings: " + path; return false; }
    std::string temporaryName = temporaryNameBuffer.data();
    ALLEGRO_FILE *temporary = al_fopen_fd(descriptor, "wb");
    if (!temporary) {
      close(descriptor);
      al_remove_filename(temporaryName.c_str());
      error = "Cannot write settings: " + path;
      return false;
    }
    bool written = al_save_config_file_f(temporary, config.get());
    written = al_fflush(temporary) && written;
    written = al_fclose(temporary) && written;
    if (!written || std::rename(temporaryName.c_str(), path.c_str()) != 0) {
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
