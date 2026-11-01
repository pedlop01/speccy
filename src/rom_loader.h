#ifndef ROM_LOADER_H
#define ROM_LOADER_H

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include "machine_config.h"


struct ROMProfile {
  MachineModel model;
  const char *name;
  std::vector<const char *> files;
  std::vector<std::uint64_t> fingerprints;
};

inline const std::vector<ROMProfile> &ROMProfiles() {
  static const std::vector<ROMProfile> profiles = {
    {MachineModel::Spectrum48, "ZX Spectrum 48K", {"48.rom"}, {0x0a927c36965e6e82ULL}},
    {MachineModel::Spectrum128, "ZX Spectrum 128K (English)", {"128-0.rom", "128-1.rom"},
      {0x167a39561d9acbe6ULL, 0x42501740662a4075ULL}},
    {MachineModel::Spectrum128, "ZX Spectrum 128K (Spanish)", {"128-spanish-0.rom", "128-spanish-1.rom"},
      {0x6ba7bfb8918d3c26ULL, 0xfbfbfb846a66ce46ULL}},
    {MachineModel::Plus2, "ZX Spectrum +2", {"plus2-0.rom", "plus2-1.rom"},
      {0xc375c3cc0d78ed32ULL, 0xa13be2977ca8a985ULL}},
    {MachineModel::Plus3, "ZX Spectrum +3", {"plus3-0.rom", "plus3-1.rom", "plus3-2.rom", "plus3-3.rom"},
      {0x5cb98a43949c0901ULL, 0xe892611fcbed4aa1ULL, 0x4857bc4f38f2881fULL, 0x0e9ecad2d59e8131ULL}}
  };
  return profiles;
}

inline std::uint64_t ROMFingerprint(const unsigned char *bytes, size_t count) {
  std::uint64_t value = 14695981039346656037ULL; // FNV-1a; identification, not authentication.
  for (size_t i = 0; i < count; ++i) value = (value ^ bytes[i]) * 1099511628211ULL;
  return value;
}

inline std::string ROMDirectory(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  return slash == std::string::npos ? "./" : path.substr(0, slash + 1);
}

inline bool ReadROMFile(const std::string &path, std::vector<unsigned char> &out, std::string &error) {
  std::ifstream file(path, std::ios::binary);
  if (!file) { error = "Cannot open ROM: " + path; return false; }
  std::vector<unsigned char> candidate(65537);
  file.read(reinterpret_cast<char *>(candidate.data()), candidate.size());
  candidate.resize(static_cast<size_t>(file.gcount()));
  if (file.bad() || (candidate.size() != 16384 && candidate.size() != 32768 && candidate.size() != 65536)) {
    error = "Invalid system ROM size: " + path + ". Expected 16, 32 or 64 KB. Interface 1 ROMs are not system ROMs.";
    return false;
  }
  out.swap(candidate);
  error.clear();
  return true;
}

struct ROMSet {
  MachineModel model = MachineModel::Spectrum48;
  std::string name, source;
  std::vector<std::vector<unsigned char>> pages;
  bool fastLoadCompatible = false;
  // Page whose recognized LD-BYTES routine may be trapped; -1 for custom ROMs.
  int fastLoadPage = -1;
};

inline bool ValidateROMHardware(const ROMSet &rom, std::string &error) {
  const auto &hardware = HardwareFor(rom.model);
  if (rom.pages.size() != hardware.memory.romPages) {
    error = "Wrong ROM page count for " + std::string(hardware.name);
    return false;
  }
  for (const auto &page : rom.pages) {
    if (page.size() != SPECTRUM_BANK_BYTES) {
      error = "Wrong ROM page size for " + std::string(hardware.name);
      return false;
    }
  }
  error.clear();
  return true;
}

// A known page determines its complete model/profile. File names only locate
// companion pages: their contents are checked too, before any state is changed.
inline bool LoadROMSet(const std::string &path, ROMSet &out, std::string &error, bool custom48 = false) {
  std::vector<unsigned char> bytes;
  if (!ReadROMFile(path, bytes, error)) return false;
  const size_t pageSize = 16384;
  std::vector<std::uint64_t> hashes;
  for (size_t offset = 0; offset < bytes.size(); offset += pageSize)
    hashes.push_back(ROMFingerprint(bytes.data() + offset, pageSize));
  for (const auto &profile : ROMProfiles()) {
    auto match = std::find(profile.fingerprints.begin(), profile.fingerprints.end(), hashes.front());
    if (match == profile.fingerprints.end()) continue;
    if (custom48 && profile.model != MachineModel::Spectrum48) {
      error = "This is a " + std::string(profile.name) + " ROM, not a custom 48K ROM.";
      return false;
    }
    ROMSet candidate;
    candidate.model = profile.model;
    candidate.name = profile.name;
    candidate.source = path;
    candidate.fastLoadCompatible = true;
    candidate.fastLoadPage = profile.model == MachineModel::Spectrum48 ? 0 :
        (profile.model == MachineModel::Plus3 ? 3 : 1);
    if (hashes.size() > 1) {
      if (hashes != profile.fingerprints) {
        error = "Wrong page order or incomplete ROM set for " + candidate.name;
        return false;
      }
      for (size_t offset = 0; offset < bytes.size(); offset += pageSize)
        candidate.pages.emplace_back(bytes.begin() + offset, bytes.begin() + offset + pageSize);
    } else {
      size_t selected = static_cast<size_t>(match - profile.fingerprints.begin());
      for (size_t i = 0; i < profile.files.size(); ++i) {
        std::vector<unsigned char> page;
        if (i == selected) page = bytes;
        else if (!ReadROMFile(ROMDirectory(path) + profile.files[i], page, error)) return false;
        if (page.size() != pageSize || ROMFingerprint(page.data(), page.size()) != profile.fingerprints[i]) {
          error = "Wrong companion ROM page: " + ROMDirectory(path) + profile.files[i];
          return false;
        }
        candidate.pages.push_back(std::move(page));
      }
    }
    if (!ValidateROMHardware(candidate, error)) return false;
    out = std::move(candidate);
    error.clear();
    return true;
  }
  if (custom48 && bytes.size() == pageSize) {
    ROMSet candidate;
    candidate.name = "ZX Spectrum 48K (custom ROM)";
    candidate.source = path;
    candidate.pages.push_back(std::move(bytes));
    if (!ValidateROMHardware(candidate, error)) return false;
    out = std::move(candidate);
    error.clear();
    return true;
  }
  error = "Unrecognized ROM/model: " + path +
      ". For an alternative 16 KB 48K ROM, use Settings > ROM > Open custom 48K ROM.";
  return false;
}

inline std::string FindROM(const std::string &name, const std::string &executable = "") {
  std::vector<std::string> candidates;
  if (!executable.empty()) {
    auto directory = ROMDirectory(executable);
    candidates.push_back(directory + "../roms/" + name);
    candidates.push_back(directory + "roms/" + name);
  }
  candidates.push_back("roms/" + name);
  candidates.push_back("../roms/" + name);
  for (const auto &path : candidates) {
    std::ifstream file(path, std::ios::binary);
    if (file) return path;
  }
  return "";
}

#endif
