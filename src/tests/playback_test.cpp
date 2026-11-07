#include <cassert>
#include <cstdio>
#include <vector>
#include "../playback.h"

static void Loop(SpectrumMachine &m) {
  m.cpu = Z80(); m.cpu.DataBus = &m.bus; m.cpu.IOBus = &m.io;
  m.cpu.regs.PC = 0x8000;
  m.bus.Write(0x8000, 0x03); // INC BC
  m.bus.Write(0x8001, 0x3E); m.bus.Write(0x8002, 0); // LD A,0 (7 cycles).
  m.bus.Write(0x8003, 0xC3); m.bus.Write(0x8004, 0); m.bus.Write(0x8005, 0x80);
}
static void Same(const SpectrumMachine::State &a, const SpectrumMachine::State &b) {
  assert(a.frame == b.frame && a.multiplier == b.multiplier && a.clockPhase == b.clockPhase);
  assert(a.tstates == b.tstates && a.audioOffset == b.audioOffset && a.audioAvailable == b.audioAvailable);
  assert(a.memory.ram == b.memory.ram && a.memory.paging == b.memory.paging);
#define R(field) assert(a.registers.field == b.registers.field)
  R(PC); R(SP); R(AF); R(BC); R(DE); R(HL); R(IX); R(IY); R(XX); R(IR);
  R(altAF); R(altBC); R(altDE); R(altHL); R(iff1A); R(iff1B); R(IM);
#undef R
  assert(a.tapePos == b.tapePos && a.fastLoad == b.fastLoad);
  assert(a.deck.current == b.deck.current && a.deck.pulse == b.deck.pulse);
  assert(a.deck.remaining == b.deck.remaining && a.deck.elapsed == b.deck.elapsed);
  assert(a.deck.clockPhase == b.deck.clockPhase && a.deck.state == b.deck.state && a.deck.level == b.deck.level);
  assert(a.ula.frameTStates == b.ula.frameTStates && a.ula.lineTStates == b.ula.lineTStates && a.ula.line == b.ula.line);
  assert(a.ula.audioPhase == b.ula.audioPhase && a.ula.audioSum == b.ula.audioSum);
  assert(a.ula.audioIndex == b.ula.audioIndex && a.ula.dcAverage == b.ula.dcAverage);
  assert(a.ula.blinkState == b.ula.blinkState && a.ula.frameCount == b.ula.frameCount);
  assert(!memcmp(a.ula.pendingAudio, b.ula.pendingAudio, sizeof(a.ula.pendingAudio)));
  assert(!memcmp(a.ula.frameAudio, b.ula.frameAudio, sizeof(a.ula.frameAudio)));
}
static unsigned Pixels(ALLEGRO_BITMAP *bitmap) {
  auto *lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READONLY);
  assert(lock); unsigned hash = 0;
  for (unsigned y = 0; y < 240; ++y)
    for (unsigned x = 0; x < 1280; ++x)
      hash = hash * 33 + reinterpret_cast<unsigned char *>(lock->data)[y * lock->pitch + x];
  al_unlock_bitmap(bitmap); return hash;
}
int main() {
  assert(al_init()); al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  std::string error;
  // Hardware overclock: more Z80 instructions, identical device-time/audio.
  for (const char *rom : {"48.rom", "128-0.rom"}) {
    SpectrumMachine normal, turbo;
    assert(normal.LoadROM(FindROM(rom), error)); assert(turbo.LoadROM(FindROM(rom), error));
    Loop(normal); Loop(turbo); assert(turbo.SetCPUMultiplier(2));
    normal.Frame(); turbo.Frame();
    assert(std::abs(int(turbo.cpu.regs.BC) - int(normal.cpu.regs.BC) * 2) <= 2);
    assert(normal.ula.FrameAudioCount == turbo.ula.FrameAudioCount);
    assert(std::abs(int(normal.CaptureState().ula.frameTStates) - int(turbo.CaptureState().ula.frameTStates)) <= 10);
    assert(!turbo.SetCPUMultiplier(3) && turbo.CPUMultiplier() == 2);

    normal.Reset(); turbo.Reset(); Loop(normal); Loop(turbo);
    assert(normal.Open("cobra.tap", error)); assert(turbo.Open("cobra.tap", error));
    Loop(normal); Loop(turbo); normal.SetFastLoad(false); turbo.SetFastLoad(false);
    assert(normal.PlayTape() && turbo.PlayTape());
    Playback a(normal), b(turbo);
    std::vector<short> audioA(48000), audioB(48000);
    a.ReadAudio(audioA.data(), audioA.size()); b.ReadAudio(audioB.data(), audioB.size());
    assert(normal.FrameCounter() == turbo.FrameCounter());
    assert(std::abs(normal.deck.Seconds() - turbo.deck.Seconds()) < 0.00001);
    assert(audioA == audioB);
    b.paused = true;
    auto stopped = turbo.CaptureState();
    std::fill(audioB.begin(), audioB.end(), 123);
    b.ReadAudio(audioB.data(), audioB.size());
    Same(stopped, turbo.CaptureState());
    for (short sample : audioB) assert(sample == 0);
    b.paused = false; assert(b.SetSpeed(2));
    auto before = turbo.FrameCounter();
    b.ReadAudio(audioB.data(), audioB.size());
    assert(turbo.FrameCounter() - before >= 99 && turbo.FrameCounter() - before <= 102);
    assert(turbo.CPUMultiplier() == 2); // Playback speed never changes the CPU setting.
  }

  // The multiplier saturates at both ends; normal speed is a direct action.
  SpectrumMachine speedMachine;
  assert(speedMachine.LoadROM(FindROM("48.rom"), error));
  Playback speedControl(speedMachine);
  assert(speedControl.Speed() == 1);
  assert(speedControl.DecreaseSpeed() && speedControl.Speed() == 0.5);
  assert(speedControl.DecreaseSpeed() && speedControl.Speed() == 0.25);
  assert(!speedControl.DecreaseSpeed());
  assert(speedControl.IncreaseSpeed() && speedControl.Speed() == 0.5);
  assert(speedControl.IncreaseSpeed() && speedControl.Speed() == 1);
  for (unsigned expected : {2u, 4u, 8u, 16u}) {
    assert(speedControl.IncreaseSpeed() && speedControl.Speed() == expected);
  }
  assert(!speedControl.IncreaseSpeed() && speedControl.Speed() == 16);
  for (double expected : {8.0, 4.0, 2.0, 1.0, 0.5, 0.25}) {
    assert(speedControl.DecreaseSpeed() && speedControl.Speed() == expected);
  }
  assert(!speedControl.SetSpeed(0) && !speedControl.SetSpeed(3) && !speedControl.SetSpeed(32));
  assert(speedControl.SetSpeed(16) && speedControl.NormalSpeed() && speedControl.Speed() == 1);
  assert(!speedControl.NormalSpeed());
  for (double speed : {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0}) {
    speedMachine.Reset(); Loop(speedMachine); assert(speedControl.SetSpeed(speed));
    std::vector<short> audio(4800);
    speedControl.ReadAudio(audio.data(), audio.size());
    auto frames = speedMachine.FrameCounter();
    assert(frames >= speed * 5 && frames <= speed * 5 + 1);
    assert(speedMachine.CPUMultiplier() == 1);
  }

  // A real ROM pulse loader must decode the same tape at every global speed.
  // Count host audio fragments: playback scales them without overclocking.
  const double speeds[] = {0.25, 0.5, 1, 2, 4, 8, 16};
  unsigned fragments[7] = {};
  for (unsigned fast = 0; fast < 7; ++fast) {
    SpectrumMachine loader;
    assert(loader.LoadROM(FindROM("48.rom"), error));
    loader.tape = {34, 0, 255};
    unsigned char checksum = 255;
    for (unsigned i = 0; i < 32; ++i) { loader.tape.push_back(i * 7); checksum ^= i * 7; }
    loader.tape.push_back(checksum); assert(loader.deck.Mount(loader.tape));
    loader.SetFastLoad(false);
    loader.cpu.regs.PC = 0x0556; loader.cpu.regs.SP = 0xBF00;
    loader.cpu.regs.A = 255; loader.cpu.regs.CF = 1;
    loader.cpu.regs.IX = 0x8000; loader.cpu.regs.DE = 32;
    loader.bus.Write(0xBF00, 0); loader.bus.Write(0xBF01, 0x90); loader.bus.Write(0x9000, 0xF3); loader.bus.Write(0x9001, 0x18); loader.bus.Write(0x9002, 0xFD);
    assert(loader.PlayTape());
    Playback playback(loader); assert(playback.SetSpeed(speeds[fast]));
    short fragment[SAMPLES_PER_FRAME];
    while ((loader.cpu.regs.PC < 0x9000 || loader.cpu.regs.PC > 0x9002) && fragments[fast] < 800) {
      playback.ReadAudio(fragment, SAMPLES_PER_FRAME); ++fragments[fast];
    }
    assert(loader.cpu.regs.PC >= 0x9000 && loader.cpu.regs.PC <= 0x9002 && loader.cpu.regs.CF && !loader.cpu.regs.DE);
    for (unsigned i = 0; i < 32; ++i) assert(loader.bus.Read(0x8000 + i) == i * 7);
    assert(loader.CPUMultiplier() == 1);
  }
  for (unsigned i = 0; i < 7; ++i) {
    double equivalent = fragments[i] * speeds[i];
    assert(std::abs(equivalent - fragments[2]) <= std::max(1.0, speeds[i]));
  }

  // Fractional playback carries the held sample across fragment boundaries,
  // pauses and resumes; rewinding flushes the old branch's repeated sample.
  SpectrumMachine slowA, slowB;
  assert(slowA.LoadROM(FindROM("48.rom"), error)); assert(slowB.LoadROM(FindROM("48.rom"), error));
  Loop(slowA); Loop(slowB);
  slowA.ula.IOWrite(0xFE, 0x10); slowB.ula.IOWrite(0xFE, 0x10);
  auto initial = slowA.CaptureState();
  Playback pa(slowA), pb(slowB); assert(pa.SetSpeed(0.25) && pb.SetSpeed(0.25));
  std::vector<short> whole(4099), pieces(whole.size());
  pa.ReadAudio(whole.data(), whole.size());
  for (size_t offset = 0; offset < pieces.size();) {
    size_t n = std::min(size_t(137), pieces.size() - offset);
    pb.ReadAudio(pieces.data() + offset, n); offset += n;
  }
  assert(whole == pieces && whole[0] > 1000); Same(slowA.CaptureState(), slowB.CaptureState());
  pa.paused = true; short silence[7]; pa.ReadAudio(silence, 7);
  for (auto sample : silence) assert(sample == 0);
  pa.paused = false; short nextA[60], nextB[60]; pa.ReadAudio(nextA, 60); pb.ReadAudio(nextB, 60);
  assert(!memcmp(nextA, nextB, sizeof(nextA)));
  assert(slowA.RestoreState(initial)); pa.DiscardAudio();
  std::vector<short> again(whole.size()); pa.ReadAudio(again.data(), again.size()); assert(again == whole);

  // Restore a partially consumed audio frame with 128K paging, AY, FLASH,
  // a playing pulse tape and fractional 4x CPU cycles. Replay must be exact.
  SpectrumMachine m;
  assert(m.LoadROM(FindROM("128-0.rom"), error));
  assert(m.Open("cobra.tap", error)); Loop(m);
  m.SetFastLoad(false); assert(m.PlayTape()); assert(m.SetCPUMultiplier(4));
  m.io.Write(0x7FFD, 0x0F);
  m.bus.Write(0xD800, 0x8A); m.bus.Write(0xC000, 0x80);
  m.io.Write(0xFFFD, 0); m.io.Write(0xBFFD, 31);
  m.io.Write(0xFFFD, 7); m.io.Write(0xBFFD, 0x3E);
  m.io.Write(0xFFFD, 8); m.io.Write(0xBFFD, 15);
  ALLEGRO_BITMAP *bitmap = al_create_bitmap(320, 240); assert(bitmap); m.ula.SetBitmap(bitmap);
  std::vector<short> first(19207), future(12345), replay(future.size());
  m.ReadAudio(first.data(), first.size());
  auto saved = m.CaptureState();
  assert(m.ula.Present()); unsigned image = Pixels(bitmap);
  m.ReadAudio(future.data(), future.size()); auto later = m.CaptureState();
  m.io.Write(0x7FFD, 0x20); m.bus.Write(0x4000, 0xFF);
  assert(m.RestoreState(saved)); assert(m.ula.Present()); assert(Pixels(bitmap) == image);
  assert(m.cpu.DataBus == &m.bus && m.cpu.IOBus == &m.io && m.memory.ScreenBank() == 7);
  m.ReadAudio(replay.data(), replay.size());
  assert(future == replay); Same(later, m.CaptureState());

  // Bounded history, repeated rewind, branch truncation, media/reset boundary.
  m.Reset(); Loop(m); m.EnableRewind();
  assert(!m.CanRewind());
  for (unsigned i = 0; i < 550; ++i) m.Frame();
  assert(m.HistorySize() <= 51 && m.CanRewind());
  auto present = m.FrameCounter();
  assert(m.RewindOneSecond() && m.FrameCounter() == present - 50);
  for (unsigned i = 0; i < 25; ++i) m.Frame();
  present = m.FrameCounter();
  assert(m.RewindOneSecond() && m.FrameCounter() <= present - 50);
  unsigned rewinds = 0;
  while (m.RewindOneSecond()) { assert(++rewinds <= 10); }
  assert(m.FrameCounter() >= 50 && !m.CanRewind());
  auto old = m.CaptureState();
  m.Reset(); assert(!m.CanRewind() && !m.RestoreState(old));
  m.Frame(); assert(m.CanRewind());
  assert(!m.Open("missing-tape-for-rewind.tap", error) && m.CanRewind());
  m.Close(); assert(!m.CanRewind());
  m.EnableRewind(false); m.Frame(); assert(m.HistorySize() == 0);
  al_destroy_bitmap(bitmap); al_uninstall_system();
  // Held rewind silences audio, keeps pause/speed, and cannot run forward
  // at the history limit. Release resumes from a new branch.
  m.Reset(); Loop(m); m.EnableRewind();
  for (unsigned i = 0; i < 600; ++i) m.Frame();
  Playback reverse(m); reverse.paused = true; reverse.SetSpeed(2);
  assert(reverse.BeginRewind());
  const auto reverseStart = m.FrameCounter();
  short silent[128]; std::fill(silent, silent + 128, 123);
  reverse.ReadAudio(silent, 128);
  for (short sample : silent) assert(sample == 0);
  assert(m.FrameCounter() == reverseStart && !reverse.BeginRewind());
  assert(!reverse.TickRewind(0.02));
  assert(reverse.TickRewind(0.03) && m.FrameCounter() == reverseStart - 10);
  for (int i = 0; i < 100; ++i) reverse.TickRewind(0.05);
  assert(!m.CanRewind() && reverse.Rewinding());
  const auto oldest = m.FrameCounter();
  reverse.TickRewind(10); reverse.ReadAudio(silent, 128);
  assert(m.FrameCounter() == oldest && m.HistorySize() <= 51);
  assert(reverse.EndRewind() && !reverse.EndRewind());
  assert(reverse.paused && reverse.Speed() == 2);
  reverse.paused = false; reverse.ReadAudio(silent, 128);
  assert(m.FrameCounter() > oldest);
  m.Reset(); assert(!reverse.BeginRewind());

  puts("Playback: pause, 0.25x-16x ladder vs CPU overclock, exact restore of RAM/AY/video/tape/audio and bounded rewind passed");
}
