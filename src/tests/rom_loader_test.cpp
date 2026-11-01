#include <cassert>
#include <cstdio>
#include <unistd.h>
#include "../emulator.h"

static void WriteFile(const std::string &path, const std::vector<unsigned char> &bytes) {
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
  assert(out.good());
}

int main() {
  std::string path = FindROM("48.rom");
  assert(!path.empty());
  std::string directory = ROMDirectory(path), error;
  ROMSet loaded;
  for (const auto &profile : ROMProfiles()) {
    for (size_t selected = 0; selected < profile.files.size(); ++selected) {
      assert(LoadROMSet(directory + profile.files[selected], loaded, error));
      assert(loaded.model == profile.model && loaded.name == profile.name);
      assert(loaded.pages.size() == profile.files.size());
      for (size_t i = 0; i < loaded.pages.size(); ++i)
        assert(ROMFingerprint(loaded.pages[i].data(), loaded.pages[i].size()) == profile.fingerprints[i]);
    }
  }
  char temp[] = "/tmp/gomas-rom-XXXXXX";
  char *created = mkdtemp(temp);
  assert(created);
  std::string folder = std::string(created) + "/", file = folder + "renamed.rom";
  assert(LoadROMSet(path, loaded, error));
  WriteFile(file, loaded.pages[0]);
  assert(LoadROMSet(file, loaded, error) && loaded.model == MachineModel::Spectrum48);
  // A misleading file name never changes the model inferred from the bytes.
  WriteFile(folder + "128-0.rom", loaded.pages[0]);
  assert(LoadROMSet(folder + "128-0.rom", loaded, error) && loaded.model == MachineModel::Spectrum48);
  assert(LoadROMSet(directory + "128-1.rom", loaded, error));
  WriteFile(file, loaded.pages[1]);
  unlink((folder + "128-0.rom").c_str());
  assert(!LoadROMSet(file, loaded, error)); // Missing sibling page, output stays intact.
  assert(loaded.model == MachineModel::Spectrum128 && loaded.pages.size() == 2);
  WriteFile(folder + "128-0.rom", loaded.pages[0]);
  assert(LoadROMSet(file, loaded, error) && loaded.pages.size() == 2);
  // Combined sets are accepted only in their defined page order.
  std::vector<unsigned char> combined = loaded.pages[0];
  combined.insert(combined.end(), loaded.pages[1].begin(), loaded.pages[1].end());
  WriteFile(file, combined);
  assert(LoadROMSet(file, loaded, error));
  std::rotate(combined.begin(), combined.begin() + 16384, combined.end());
  WriteFile(file, combined);
  assert(!LoadROMSet(file, loaded, error));
  assert(LoadROMSet(directory + "plus3-3.rom", loaded, error));
  combined.clear();
  for (const auto &page : loaded.pages) combined.insert(combined.end(), page.begin(), page.end());
  WriteFile(file, combined);
  assert(LoadROMSet(file, loaded, error) && loaded.pages.size() == 4);
  assert(!LoadROMSet(directory + "if1-2.rom", loaded, error));

  SpectrumMachine machine;
  assert(machine.LoadROM(path, error));
  assert(machine.CanFastLoad());
  assert(machine.Open("cobra.tap", error));
  machine.bus.Write(0x8000, 0x42);
  machine.cpu.regs.PC = 0x9000;
  machine.tapePos = 21;
  auto checkPreserved = [&]() {
    assert(machine.systemROM.model == MachineModel::Spectrum48 && machine.CanFastLoad());
    assert(machine.bus.Read(0x8000) == 0x42 && machine.cpu.regs.PC == 0x9000);
    assert(machine.tapePos == 21 && machine.tapePath == "cobra.tap");
  };
  assert(!machine.LoadROM(directory + "128-0.rom", error));
  assert(error.find("not implemented") != std::string::npos);
  checkPreserved();
  assert(!machine.LoadROM(directory + "plus2-0.rom", error)); checkPreserved();
  assert(!machine.LoadROM(directory + "plus3-2.rom", error)); checkPreserved();
  std::vector<unsigned char> custom(16384, 0);
  custom[0] = 0x76;
  WriteFile(file, custom);
  assert(!machine.LoadROM(file, error)); checkPreserved();
  assert(machine.LoadROM(file, error, true));
  assert(!machine.CanFastLoad() && !machine.fastLoad);
  assert(machine.bus.Read(0) == 0x76 && machine.bus.Read(0x8000) == 0);
  assert(machine.cpu.regs.PC == 0 && machine.tapePos == 0 && machine.tapePath == "cobra.tap");
  machine.SetFastLoad(true); assert(!machine.fastLoad);
  machine.Reset(); assert(machine.bus.Read(0) == 0x76);
  assert(!machine.LoadROM(directory + "128-0.rom", error, true));
  custom.push_back(0); WriteFile(file, custom);
  assert(!machine.LoadROM(file, error, true) && machine.bus.Read(0) == 0x76);
  custom.resize(16383); WriteFile(file, custom);
  assert(!machine.LoadROM(file, error, true) && machine.bus.Read(0) == 0x76);
  // The low-level ROM device is transactional and rejects oversized files too.
  ROM<0, 16384> device;
  assert(device.Load(path.c_str()) == 0);
  auto first = device.Read(0);
  custom.resize(16385); WriteFile(file, custom);
  assert(device.Load(file.c_str()) == -1 && device.Read(0) == first);
  custom.resize(16383); WriteFile(file, custom);
  assert(device.Load(file.c_str()) == -1 && device.Read(0) == first);
  unlink(file.c_str()); unlink((folder + "128-0.rom").c_str()); rmdir(created);
  // Search relative to the executable remains valid outside the project cwd.
  char cwd[4096]; assert(getcwd(cwd, sizeof(cwd)));
  char absoluteROM[4096]; assert(realpath(path.c_str(), absoluteROM));
  std::string executable = ROMDirectory(absoluteROM) + "../bin/gomas";
  assert(chdir("/tmp") == 0);
  assert(!FindROM("48.rom", executable).empty());
  assert(LoadROMSet(FindROM("48.rom", executable), loaded, error));
  assert(chdir(cwd) == 0);
  puts("ROMs: profiles, page sets, identification, custom ROMs, atomic failure and paths passed");
}
