#ifndef MACHINE_CONFIG_H
#define MACHINE_CONFIG_H

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>

// Hardware identity is independent of ROM language, filenames and fingerprints.
enum class MachineModel { Spectrum48, Spectrum128, Plus2, Plus3 };
enum class PagingKind { Flat, Spectrum128, Plus3 };
enum class FloatingBusKind { ScreenFetch, Idle };

constexpr unsigned SPECTRUM_BANK_BYTES = 16384;

struct MachineTiming {
  unsigned cpuClockHz;
  unsigned tstatesPerLine;
  unsigned linesPerFrame;
  unsigned firstDisplayTstate;
  unsigned interruptPulseTstates;
  unsigned firstFloatingFetchTstate;
  FloatingBusKind floatingBus;

  constexpr unsigned TStatesPerFrame() const { return tstatesPerLine * linesPerFrame; }
};

// Hardware timing facts. Audio samples are clocked independently at 48 kHz.
// References: https://github.com/fuse-emulator/libspectrum/blob/master/timings.c
//             https://github.com/fuse-emulator/fuse/blob/master/spectrum.c
namespace MachineTimings {
  constexpr MachineTiming PAL48 = {3500000, 224, 312, 14336, 32, 14338, FloatingBusKind::ScreenFetch};
  constexpr MachineTiming PAL128 = {3546900, 228, 311, 14362, 36, 14364, FloatingBusKind::ScreenFetch};
  constexpr MachineTiming Plus3 = {3546900, 228, 311, 14365, 32, 0, FloatingBusKind::Idle};
}

struct MemoryConfiguration {
  unsigned ramBanks;
  unsigned romPages;
  unsigned bootROMPage;
  // Banks exposed at 4000, 8000 and C000 on reset. In 48K these are the three
  // physical 16 KB regions, numbered locally rather than as 128K bank numbers.
  std::array<unsigned, 3> bootRAMBanks;
  std::array<unsigned, 2> screenBanks;
  PagingKind paging;

  unsigned RAMBytes() const { return ramBanks * SPECTRUM_BANK_BYTES; }
  unsigned ROMBytes() const { return romPages * SPECTRUM_BANK_BYTES; }
  bool HasShadowScreen() const { return screenBanks[0] != screenBanks[1]; }
};

struct DeviceConfiguration {
  unsigned ayClockHz; // Zero means AY is absent.
  bool floppyController;
  bool HasAY() const { return ayClockHz != 0; }
};

struct MachineConfiguration {
  MachineModel model;
  const char *name;
  MemoryConfiguration memory;
  MachineTiming timing;
  DeviceConfiguration devices;
};

inline const MachineConfiguration &HardwareFor(MachineModel model) {
  static const MachineConfiguration spectrum48 = {
    MachineModel::Spectrum48, "ZX Spectrum 48K",
    {3, 1, 0, {{0, 1, 2}}, {{0, 0}}, PagingKind::Flat}, MachineTimings::PAL48, {0, false}
  };
  static const MachineConfiguration spectrum128 = {
    MachineModel::Spectrum128, "ZX Spectrum 128K",
    {8, 2, 0, {{5, 2, 0}}, {{5, 7}}, PagingKind::Spectrum128}, MachineTimings::PAL128, {1773400, false}
  };
  static const MachineConfiguration plus2 = {
    MachineModel::Plus2, "ZX Spectrum +2",
    {8, 2, 0, {{5, 2, 0}}, {{5, 7}}, PagingKind::Spectrum128}, MachineTimings::PAL128, {1773400, false}
  };
  static const MachineConfiguration plus3 = {
    MachineModel::Plus3, "ZX Spectrum +3",
    {8, 4, 0, {{5, 2, 0}}, {{5, 7}}, PagingKind::Plus3}, MachineTimings::Plus3, {1773400, true}
  };
  switch (model) {
    case MachineModel::Spectrum48: return spectrum48;
    case MachineModel::Spectrum128: return spectrum128;
    case MachineModel::Plus2: return plus2;
    case MachineModel::Plus3: return plus3;
  }
  throw std::invalid_argument("Unknown Spectrum hardware model");
}

// Keep implementation availability out of the hardware facts. Future tasks
// enable capabilities here only once their memory/video/sound implementation is ready.
enum HardwareFeature : unsigned {
  FeatureBankedRAM = 1u << 0,
  FeatureROMPaging = 1u << 1,
  FeatureModelTiming = 1u << 2,
  FeatureShadowScreen = 1u << 3,
  FeatureAY = 1u << 4,
  FeaturePlus3Paging = 1u << 5,
  FeatureFloppy = 1u << 6
};

inline unsigned RequiredHardwareFeatures(const MachineConfiguration &configuration) {
  unsigned features = 0;
  if (configuration.memory.paging != PagingKind::Flat) features |= FeatureBankedRAM;
  if (configuration.memory.romPages > 1) features |= FeatureROMPaging;
  if (configuration.model != MachineModel::Spectrum48) features |= FeatureModelTiming;
  if (configuration.memory.HasShadowScreen()) features |= FeatureShadowScreen;
  if (configuration.devices.HasAY()) features |= FeatureAY;
  if (configuration.memory.paging == PagingKind::Plus3) features |= FeaturePlus3Paging;
  if (configuration.devices.floppyController) features |= FeatureFloppy;
  return features;
}

inline unsigned ImplementedHardwareFeatures() { return FeatureBankedRAM | FeatureROMPaging | FeatureModelTiming | FeatureShadowScreen | FeatureAY | FeaturePlus3Paging | FeatureFloppy; }

inline std::string MissingHardwareFeatures(const MachineConfiguration &configuration) {
  unsigned missing = RequiredHardwareFeatures(configuration) & ~ImplementedHardwareFeatures();
  std::string result;
  const struct { unsigned feature; const char *name; } names[] = {
    {FeatureBankedRAM, "banked RAM"}, {FeatureROMPaging, "ROM paging"},
    {FeatureModelTiming, "model-specific video/audio timing"},
    {FeatureShadowScreen, "shadow screen"}, {FeatureAY, "AY sound"},
    {FeaturePlus3Paging, "+3 paging modes"}, {FeatureFloppy, "floppy controller"}
  };
  for (const auto &entry : names) {
    if (!(missing & entry.feature)) continue;
    if (!result.empty()) result += ", ";
    result += entry.name;
  }
  return result;
}

inline bool SupportsHardware(MachineModel model) {
  return (RequiredHardwareFeatures(HardwareFor(model)) & ~ImplementedHardwareFeatures()) == 0;
}

#endif
