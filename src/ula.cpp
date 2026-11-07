#include "ula.h"
#include <algorithm>

ULA::ULA() {
  bitmap = NULL;
  Reset();
}

void ULA::Configure(MachineModel model) {
  timing = HardwareFor(model).timing;
  ay.Configure(timing.cpuClockHz, HardwareFor(model).devices.ayClockHz);
  Reset();
}

void ULA::Reset() {
  ay.Reset();
  displayMemory = data;
  FrameAudioCount = 0;
  dwFrameTStates = 0;
  dwScanLineTStates = 0;
  dwScanLine = 0;
  isDirty = true;
  blinkState = false;
  dwFrameCount = 0;
  dwBorderRGBColor = 0x0000000;
  memset(dwCurrentScanLineBackColor, 0xFF, sizeof(dwCurrentScanLineBackColor));

  memset(data, 0, sizeof(data));
  memset(keyMatrix, 0, sizeof(keyMatrix));

  memset(FrameAudio, 0, sizeof(FrameAudio));
  memset(pendingAudio, 0, sizeof(pendingAudio));
  memset(pixels, 0, sizeof(pixels));
  for (auto &row : pixels)
    for (auto &pixel : row) pixel[3] = 255;
  earInput = true;
  tapeMonitor = false;
  ULAIOData = 0;
  audioPhase = audioIndex = 0;
  audioSum = 0;
  dcAverage = 0;
}

ULA::~ULA() {

}

void ULA::UpdateChar(unsigned int nChar) {
  unsigned int pmemOffset;
  unsigned char attr;
  unsigned int ink;
  unsigned int paper;
  unsigned int row;
  unsigned int column;

  pmemOffset = ((nChar & 0x300) << 3) | (nChar & 0x0FF);
  attr = displayMemory[nChar + 0x1800];

  row = (nChar >> 5) << 3;
  column = (nChar & 0x1F) << 3;

  if((attr & 0x80) && blinkState) {
    paper = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
    ink = dwColorTable[(attr & 0x78) >> 3];
  } else {
    paper = dwColorTable[(attr & 0x78) >> 3];
    ink   = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
  }

  isDirty = true;
  // Redraw 8x8 pixels
  for (int y = 0; y < 8; y++) {
    unsigned char pixelData = displayMemory[pmemOffset];
    for (int x = 0; x < 8; x++) {
      bool is_ink = (pixelData & (0x01 << (7 - x)));

      PutPixel(column + x + 32, row + y + 24, is_ink ? ink : paper);
    }
    pmemOffset += 256;
  }

}

void ULA::AddCycles(unsigned int cycles, bool& IRQ) {
  IRQ = false;
  // Split at scanlines so frame snapshots precede any samples of the next frame.
  while (cycles) {
    unsigned int step = std::min(cycles,
        static_cast<unsigned int>(timing.tstatesPerLine - dwScanLineTStates));
    step = std::min(step, ay.UntilTick());
    unsigned int units = step * AUDIO_SAMPLE_RATE;
    const int signal = ((ULAIOData & 0x10) ? 16384 : 0) +
                       ((tapeMonitor && earInput) ? 8192 : 0) + ay.Output();
    // Integrate each level over time, including transitions within a sample.
    while (units) {
      unsigned int part = std::min(units, timing.cpuClockHz - audioPhase);
      audioSum += static_cast<std::int64_t>(signal) * part;
      audioPhase += part;
      units -= part;
      if (audioPhase == timing.cpuClockHz) {
        double sample = static_cast<double>(audioSum) / timing.cpuClockHz;
        dcAverage += 0.005 * (sample - dcAverage);
        pendingAudio[audioIndex++] = static_cast<short>(std::max(-32768.0, std::min(32767.0, sample - dcAverage)));
        audioSum = 0;
        audioPhase = 0;
      }
    }
    ay.Advance(step);
    RenderScreen(dwFrameTStates, dwFrameTStates + step);
    dwFrameTStates += step;
    dwScanLineTStates += step;
    cycles -= step;
    if (dwScanLineTStates >= timing.tstatesPerLine)
      ScanLine(IRQ);
  }
}

void ULA::ScanLine(bool& IRQ) {
  unsigned int bitmapLine;

  dwScanLineTStates %= timing.tstatesPerLine;
  dwScanLine++;

  if (dwScanLine >= timing.linesPerFrame) {
    FrameAudioCount = audioIndex;
    memcpy(FrameAudio, pendingAudio, FrameAudioCount * sizeof(short));
    std::fill(FrameAudio + FrameAudioCount, FrameAudio + SAMPLES_PER_FRAME, 0);
    audioIndex = 0;
    // Frame complete - trigger IRQ
    IRQ = true;
    dwFrameTStates %= timing.TStatesPerFrame();
    dwScanLine = 0;
    dwFrameCount++;
    if (dwFrameCount >= 16) {    // each 16 full frames
      dwFrameCount = 0;         // reset frame counter
      blinkState = !blinkState; // invert blink
      // The new FLASH phase is sampled by the next frame's screen fetches.
    }
  }

  // Skip non-visible lines
  // Position the 24-line top crop relative to the model's first display line.
  const unsigned firstLine = (timing.firstDisplayTstate + timing.tstatesPerLine - 1) / timing.tstatesPerLine;
  const unsigned cropLine = firstLine - 24;
  if((dwScanLine < cropLine) || (dwScanLine >= cropLine + 240))
    return;

  // Check if current background color for scanline is
  // different from the required one
  bitmapLine = dwScanLine - cropLine;
  if (dwCurrentScanLineBackColor[bitmapLine] == dwBorderRGBColor)
    return;

  // redraw current scanline with proper color
  if ((bitmapLine < 24) || (bitmapLine > 24+191)) {
    // top/bottom border    
    for (unsigned int x = 0; x < 320; x++)
      PutPixel(x, bitmapLine, dwBorderRGBColor);
  } else {
    for (unsigned int x = 0; x < 32; x++) {
      // screen contents
      PutPixel(x, bitmapLine, dwBorderRGBColor);
      PutPixel(x + 288, bitmapLine, dwBorderRGBColor);
    }
  }

  // Save border color for line
  dwCurrentScanLineBackColor[bitmapLine] = dwBorderRGBColor;
  isDirty = true;
}

void ULA::UpdateBlink() {
  for (unsigned int nChar = 0; nChar < 32*24; nChar++)
    if (displayMemory[nChar + 0x1800] & 0x80)
      UpdateChar(nChar);
}

void ULA::SelectScreen(const unsigned char *memory) {
  const unsigned char *selected = memory ? memory : data;
  if (displayMemory == selected) return;
  displayMemory = selected;
}

// RAM writes only change the source for future ULA fetches. Repainting here
// exposes temporary sprite erasure and retroactively changes scanned pixels.
void ULA::MemoryWrite(unsigned int, unsigned char) {}
void ULA::DisplayWrite(unsigned int, unsigned char) {}

void ULA::RenderByte(unsigned y, unsigned column) {
  const unsigned offset = ((y & 0xC0) << 5) | ((y & 7) << 8) |
                          ((y & 0x38) << 2) | column;
  const unsigned char bits = displayMemory[offset];
  const unsigned char attr = displayMemory[0x1800 + (y / 8) * 32 + column];
  unsigned ink = dwColorTable[((attr & 0x40) >> 3) | (attr & 7)];
  unsigned paper = dwColorTable[(attr & 0x78) >> 3];
  if ((attr & 0x80) && blinkState) std::swap(ink, paper);
  for (unsigned x = 0; x < 8; ++x)
    PutPixel(32 + column * 8 + x, 24 + y, (bits & (0x80 >> x)) ? ink : paper);
  isDirty = true;
}

void ULA::RenderScreen(unsigned from, unsigned to) {
  // Eight pixels take four ULA T-states. Sample each byte/attribute pair at
  // its display position, using the selected bank at that instant. CPU bus
  // accesses are still instruction-granular; this does not add contention.
  const unsigned first = timing.firstDisplayTstate;
  const unsigned end = first + 191 * timing.tstatesPerLine + 128;
  if (to <= first || from >= end) return;
  from = std::max(from, first) - first;
  to = std::min(to, end) - first;
  const unsigned lastRow = (to - 1) / timing.tstatesPerLine;
  for (unsigned y = from / timing.tstatesPerLine; y <= lastRow; ++y) {
    const unsigned row = y * timing.tstatesPerLine;
    const unsigned x = from > row ? from - row : 0;
    for (unsigned column = (x + 3) / 4; column < 32 && row + column * 4 < to; ++column)
      RenderByte(y, column);
  }
}

void ULA::SetBitmap(ALLEGRO_BITMAP* _bitmap) {
  bitmap = _bitmap;
  isDirty = true;
}

void ULA::IOWrite(unsigned int address, unsigned char value) {
  // ULA is selected when reading an even address
  if (address & 0x01)
    return;
  
  // save a copy of ULA's state
  ULAIOData = value;

  // Update border color
  dwBorderRGBColor = dwColorTable[value & 0x07];
}

unsigned char ULA::IORead(unsigned int address) {
  // ULA is selected by A0 being low
  if ((address & 0x01) == 0) {
    // Get the scancodes
    unsigned char kData = 0xFF;   // Pull ups
    unsigned char row = (address >> 8) ^ 0xFF;
    for (int i = 0; i < 8; i++) {
      if (row & (1 << i)) {       // travers all selected rows
        kData &= ~keyMatrix[i];   // pull down bits representing pressed key
      }
    }
    if (!earInput) kData &= ~0x40;
    return kData;
  }
  // Unattached ports expose the ULA video fetches (48K floating bus).
  // The profile defines the first fetch; four fetches then four idle cycles.
  // See https://github.com/fuse-emulator/fuse/blob/master/spectrum.c
  if (timing.floatingBus == FloatingBusKind::Idle) return 0xFF;
  const unsigned int firstFetch = timing.firstFloatingFetchTstate;
  if (dwFrameTStates < firstFetch) return 0xFF;
  unsigned int elapsed = dwFrameTStates - firstFetch;
  unsigned int y = elapsed / timing.tstatesPerLine;
  unsigned int x = elapsed % timing.tstatesPerLine;
  if (y >= 192 || x >= 128 || (x & 7) >= 4) return 0xFF;
  unsigned int column = (x / 8) * 2 + ((x & 2) >> 1);
  unsigned int offset;
  if (x & 1)
    offset = 0x1800 + (y / 8) * 32 + column;
  else
    offset = ((y & 0xC0) << 5) | ((y & 7) << 8) |
             ((y & 0x38) << 2) | column;
  return displayMemory[offset];
}

void ULA::PressKey(unsigned int keyRow, unsigned int keyCol, bool down) {

  if (keyCol > 9)
    return;
  if (keyRow > 3)
    return;

  int rowNdx;
  int bitMask;
  if (keyCol < 5) {  // Left bank
    rowNdx  = 3 - keyRow;
    bitMask = 0x01 << keyCol;
  } else {
    rowNdx  = 4 + keyRow;
    bitMask = 0x01 << (9 - keyCol);
  }

  if (down)
    keyMatrix[rowNdx] |= bitMask;
  else
    keyMatrix[rowNdx] &= ~bitMask;
}

bool ULA::GetIsDirty() {
  return isDirty;
}

void ULA::PutPixel(unsigned int x, unsigned int y, unsigned int color) {
  pixels[y][x][0] = color >> 24;
  pixels[y][x][1] = color >> 16;
  pixels[y][x][2] = color >> 8;
  pixels[y][x][3] = 255;
}

bool ULA::Present() {
  if (!bitmap) return false;
  if (!isDirty) return true;
  ALLEGRO_LOCKED_REGION *lock = al_lock_bitmap(bitmap,
      ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
  if (!lock) return false;
  for (int y = 0; y < 240; ++y)
    memcpy(static_cast<char *>(lock->data) + y * lock->pitch,
           pixels[y], sizeof(pixels[y]));
  al_unlock_bitmap(bitmap);
  isDirty = false;
  return true;
}

ULA::State ULA::CaptureState() const {
  State state{};
  state.timing = timing;
  state.ay = ay;
  state.blinkState = blinkState;
  state.earInput = earInput;
  state.tapeMonitor = tapeMonitor;
  state.borderColor = dwBorderRGBColor;
  state.frameTStates = dwFrameTStates;
  state.lineTStates = dwScanLineTStates;
  state.line = dwScanLine;
  state.frameCount = dwFrameCount;
  state.ioData = ULAIOData;
  state.frameAudioCount = FrameAudioCount;
  state.audioPhase = audioPhase;
  state.audioIndex = audioIndex;
  state.audioSum = audioSum;
  state.dcAverage = dcAverage;
  memcpy(state.lineColors, dwCurrentScanLineBackColor, sizeof(dwCurrentScanLineBackColor));
  memcpy(state.keys, keyMatrix, sizeof(keyMatrix));
  memcpy(state.frameAudio, FrameAudio, sizeof(FrameAudio));
  memcpy(state.pendingAudio, pendingAudio, sizeof(pendingAudio));
  state.screenRGB = CaptureRGB();
  return state;
}

void ULA::RestoreState(const State &state) {
  timing = state.timing;
  ay = state.ay;
  blinkState = state.blinkState;
  earInput = state.earInput;
  tapeMonitor = state.tapeMonitor;
  dwBorderRGBColor = state.borderColor;
  dwFrameTStates = state.frameTStates;
  dwScanLineTStates = state.lineTStates;
  dwScanLine = state.line;
  dwFrameCount = state.frameCount;
  ULAIOData = state.ioData;
  FrameAudioCount = state.frameAudioCount;
  audioPhase = state.audioPhase;
  audioIndex = state.audioIndex;
  audioSum = state.audioSum;
  dcAverage = state.dcAverage;
  memcpy(dwCurrentScanLineBackColor, state.lineColors, sizeof(dwCurrentScanLineBackColor));
  memcpy(keyMatrix, state.keys, sizeof(keyMatrix));
  memcpy(FrameAudio, state.frameAudio, sizeof(FrameAudio));
  memcpy(pendingAudio, state.pendingAudio, sizeof(pendingAudio));
  if (state.screenRGB.size() == 320 * 240 * 3) {
    // RAM may already hold the next sprite update; keep what the ULA saw.
    for (unsigned y = 0; y < 240; ++y)
      for (unsigned x = 0; x < 320; ++x) {
        const auto *rgb = &state.screenRGB[(y * 320 + x) * 3];
        PutPixel(x, y, unsigned(rgb[0]) << 24 | unsigned(rgb[1]) << 16 | unsigned(rgb[2]) << 8);
      }
  } else {
    // External snapshots and old saves have no raster. Seed them from RAM.
    for (unsigned y = 0; y < 240; ++y)
      for (unsigned x = 0; x < 320; ++x)
        PutPixel(x, y, dwCurrentScanLineBackColor[y] == 0xFFFFFFFF ? 0 : dwCurrentScanLineBackColor[y]);
    for (unsigned ch = 0; ch < 32 * 24; ++ch) UpdateChar(ch);
  }
  isDirty = true;
}

std::vector<unsigned char> ULA::CaptureRGB() const {
  std::vector<unsigned char> image;
  image.reserve(320 * 240 * 3);
  for (const auto &row : pixels)
    for (const auto &pixel : row)
      image.insert(image.end(), pixel, pixel + 3);
  return image;
}
