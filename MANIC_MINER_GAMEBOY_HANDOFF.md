# Manic Miner → Game Boy: agent handoff

Prepared 2026-11-10 from the local `color_enhancements` checkout and linked
hardware documentation. This is a development handoff, not a completed port.

## Objective and first deliverable

Build a native Game Boy homebrew version of Manic Miner. Start with a playable
**Central Cavern**: walking, jumping, collision, crumbling platforms, conveyors,
guardian movement, collecting items, unlocking the exit, air, death and restart.
Deliver a reproducibly built `.gb` ROM and a short controls/build README.
Then extend the engine to all 20 caverns, with explicit special-room handlers.

Recommended defaults, subject to visual review:

- C with GBDK-2020, original monochrome Game Boy (DMG) first; optional Game Boy
  Color palettes later. Python may generate assets and tests on the host.
- Preserve the 256×128 world, 8×8 cells and 16×16 actor silhouettes. Use a
  160×128 horizontal viewport plus a 16-pixel bottom HUD on the 160×144 LCD.
- Start with original artwork and simple readable shades. The owner preferred
  colouring existing scenery to invented landscape backgrounds in recent work.
- First milestone is one genuinely completable room, not 20 static screenshots.

These are recommendations, not recorded user decisions about camera or console
variant. Present a playable first-room result for review before investing in
all-room visual adaptation. A whole-room, redrawn layout is a legitimate
alternative if the owner dislikes scrolling; it changes the composition and
needs its own jump/puzzle validation. Do not silently squash the original room.

## What this repository already knows

| Local source | Useful evidence |
| --- | --- |
| `bin/manic.tap` | Exact reference tape; six TAP blocks, final data block loads 32768 bytes at address 32768 |
| `src/colour_enhancement.h` | Sprite decoding, tile palettes, coordinate conversions, completed-frame capture and guarded room support |
| `src/tests/colour_enhancement_test.cpp` | Real ROM/tape boot, input sequence, all-room initialisation, observation isolation and preview generation |
| `src/emulator.h` | `SpectrumMachine`, `StepInstruction()`, instruction hooks, `CaptureState()` / `RestoreState()` |
| `GAME_ENHANCEMENT_GUIDE.md` | Existing evidence, validation conventions and limits |
| `README.txt` | How to load/play the reference game and run enhancement checks |

The enhancement **does not implement game logic**. Spectrum code still handles
physics, collision, scores, timing and sound. Its RGB canvas is a host display
layer; neither that canvas nor the Allegro frontend belongs in a Game Boy ROM.
The Sharp SM83 CPU is not a drop-in Z80. Write a native engine rather than
copying Z80 instructions or putting this Spectrum emulator inside the cartridge.

The adapter colours 15 rooms and falls back to original video in five rooms.
That is a limitation of its visual coverage, not proof that the other rooms
lack data or that generic guardian logic handles their special mechanics.
Existing regression coverage is bounded: boot, movement/jump, room entry and
short runs, not a complete playthrough or a validated replacement physics model.

## Reproduce the reference

From the repository root:

```sh
sha256sum bin/manic.tap
make -C bin -j2 gomas colour-enhancement-tests
(cd bin && ./colour-enhancement-tests)
(cd bin && ./colour-enhancement-tests --video) # graphical session
```

Expected tape SHA-256:
`0229ca09bb87c58024d68b05a6a5ecd8ac93fb4932d937883722467f86277f4e`.

The test loads the 48K ROM and `manic.tap`, issues `LOAD ""`, waits for loading,
then presses Enter. Original controls: O/P left/right, Space jump. Its exact
keyboard-matrix schedule is in the test; reuse that instead of inventing a new
loader automation. Previews: `/tmp/manic-original.png`,
`/tmp/manic-enhanced.png`, and supported-room `/tmp/manic-cavern-NN.png` images.
Temporary previews must be regenerated on another machine.

Use original video as the visual/behaviour reference. Enhancement palettes are
optional art guidance, and its guardian colours are shared row bands, not
individually authored material masks.

## Source data: locally verified addresses

Addresses below are decimal Spectrum addresses for this exact tape. Little-endian
words are `ram[a] | ram[a+1]<<8`. These are extraction/reference addresses,
not addresses to retain in the Game Boy engine.

| Address / range | Meaning |
| --- | --- |
| `45056 + room*1024`, room 0..19 | 1024-byte cavern definition |
| Definition `+0..511` | 32×16 row-major **attribute bytes**, not portable tile IDs |
| Definition `+512..543` | 32-byte room name |
| Definition `+544..615` | Eight records: attribute byte followed by eight 1bpp tile rows |
| Definition `+616..1023` | Initial state, items/exit, guardians and room graphics; parse using the references below |
| `24064..24575` | Live background attribute buffer, including mutable scenery |
| `28672..32767` | Background bitmap; Spectrum scanline order, not linear pixel rows |
| `23552..24063`, `24576..28671` | Composited attribute and bitmap buffers |
| `32800 + type*9` | Current room's eight tile definitions |
| `32872`, `32873`, `32874` | Willy Y×2, animation frame, direction/movement flags |
| `32875`, `32876..32877`, `32878` | Airborne state, attribute-buffer location, jump counter |
| `32879..32882` | Conveyor direction, bitmap-buffer location word, length |
| `32885..32910` | Item definitions, including terminator |
| `32911..32947` | Portal attribute, 32-byte picture and two location words |
| `32948..32955`, `32956`, `32957` | Item picture, initial air, game clock |
| `32958 + slot*7`, four slots | Horizontal guardian records |
| `32989 + slot*7`, four slots | Vertical guardian records; not generic actors in every room |
| `33024..33279` | Eight current-room 16×16 guardian drawings, 32 bytes each |
| `33280..33535` | Eight Willy drawings: four per direction, 32 bytes each |
| `33799` | Current room index, zero based |
| `33879`, `33880`, `33882` | Lives, flash counter, game-mode indicator |
| `34449` | Original room initialisation entry used by local fixtures |
| `34771` | After playfield bitmap AND attributes have been copied to display |

The room loader's copies establish the mapping: definition bytes 0..511 go to
24064; bytes 512..1023 go to 32768. Thus an initial-state address `a` in
32768..33279 maps to definition offset `512 + (a-32768)`.
See [original room entry](https://skoolkit.ca/disassemblies/manic_miner/asm/34436.html).

Tile role order is background, floor, crumble, wall, conveyor, nasty 1,
nasty 2, extra. Extra is **room-specific**: floor in some rooms, spider silk
in The Menagerie, a switch in the Kong rooms. Do not infer collision from an
RGB colour or declare every extra tile solid.
See [tile roles](https://skoolkit.ca/disassemblies/manic_miner/asm/32800.html).

**Important extraction trap:** attribute values can repeat among the eight
records. Unused definitions often share background's attribute. Preserve raw
attributes and candidate matches until the original semantics are resolved.
The original drawing routine searches the entire 72-byte definition area, not
just eight attribute slots. A first-match lookup is only a convenient adapter
classification, not a universal format specification.

**The Final Barrier is exceptional even though its enhancement is enabled.**
Local static extraction finds 15 distinct layout attribute values outside its
eight role attributes. Its upper layout also serves the title screen, and the
original draw routine replaces the upper 2048 bitmap bytes from address 40960.
Do not label these bytes corrupt, replace unmatched cells with empty tiles or
assume a simple eight-tile renderer recreates this room. Preserve raw records,
export its original rendered image, and implement an explicit visual/collision
mapping. See [cavern drawing](https://skoolkit.ca/disassemblies/manic_miner/asm/35445.html)
and [Final Barrier data](https://skoolkit.ca/disassemblies/manic_miner/asm/64512.html).

## Sprite coordinates and conversion

For Willy, the existing renderer uses:

```text
x = (word_at_32876 & 31) * 8
y = byte_at_32872 / 2
frame_address = 33280 + (byte_at_32874 & 1)*128 + (byte_at_32873 & 3)*32
source_bit(x_local,y_local) =
    ram[frame_address + y_local*2 + x_local/8] & (128 >> (x_local & 7))
```

This is a **rendering decoder**, not a continuous movement model. The drawings
contain different poses and horizontal placement within their 16×16 canvas;
do not replace animation-dependent movement with an assumed constant velocity.
The eight source frames are stored explicitly; verify before replacing them
with four mirrored frames. See [Willy graphics](https://skoolkit.ca/disassemblies/manic_miner/asm/33280.html).

Horizontal guardian record: attribute/speed flags, location word, bitmap-address
high byte, animation, left/right path bounds. Vertical record: attribute,
animation, Y, X field, Y increment, minimum/maximum Y. The current renderer
converts a horizontal location relative to 23552 into 32-column cells; vertical
X uses `(field & 31)*8` and Y uses `field & 127`. Animation-bank selection also
varies by room; preserve the rules visible in the adapter and trace original
movement. Do not apply the horizontal format to vertical actors.
Sources: [horizontal records](https://skoolkit.ca/disassemblies/manic_miner/asm/32958.html),
[vertical records](https://skoolkit.ca/disassemblies/manic_miner/asm/32989.html).

Game Boy conversion: each 8×8 tile is 16 bytes, two bitplane bytes per row.
A simple 1bpp-to-darkest-shade conversion uses `low=row_bits; high=row_bits`.
For 16×16 actors, convert both 8-pixel columns; each column's top/bottom tiles
must be consecutive with an even top ID in 8×16 OBJ mode. Source zero bits map
to OBJ colour 0 (transparent); deliberate dark details need nonzero indices.
The original sprites are a starting mask, not the emulator's RGB screenshot.
See [tile format](https://raw.githubusercontent.com/gbdev/pandocs/master/src/Tile_Data.md)
and [OBJ layout](https://raw.githubusercontent.com/gbdev/pandocs/master/src/OAM.md).

## All 20 room identities

Names and base addresses below were read from the local tape, not guessed.

| ID | Base (hex) | Name | Planned engine work |
| --- | --- | --- | --- |
| 0 | B000 | Central Cavern | First playable milestone |
| 1 | B400 | The Cold Room | Standard engine validation |
| 2 | B800 | The Menagerie | Extra tile / spider silk semantics |
| 3 | BC00 | Abandoned Uranium Workings | Standard engine validation |
| 4 | C000 | Eugene's Lair | Dedicated Eugene state machine |
| 5 | C400 | Processing Plant | Standard engine validation |
| 6 | C800 | The Vat | Standard engine validation |
| 7 | CC00 | Miner Willy meets the Kong Beast | Kong and switches |
| 8 | D000 | Wacky Amoebatrons | Vertical guardians |
| 9 | D400 | The Endorian Forest | Guardian animation-bank exception |
| 10 | D800 | Attack of the Mutant Telephones | Standard engine validation |
| 11 | DC00 | Return of the Alien Kong Beast | Kong and switches |
| 12 | E000 | Ore Refinery | Standard engine validation |
| 13 | E400 | Skylab Landing Bay | Falling/recycling Skylab actors |
| 14 | E800 | The Bank | Standard engine validation |
| 15 | EC00 | The Sixteenth Cavern | Guardian animation-bank exception |
| 16 | F000 | The Warehouse | Standard engine validation |
| 17 | F400 | Amoebatrons' Revenge | Standard engine validation |
| 18 | F800 | Solar Power Generator | Beam tracing and air consequences |
| 19 | FC00 | The Final Barrier | Special upper-half picture/attributes |

“Standard validation” does not assert that a room has no other special rules.
The current colour fallbacks are IDs **4,7,11,13,18**. Final Barrier's data
exception is a separate issue; do not use that five-room list as the only
special-case inventory.

## Game Boy implementation choices

The hardware constraints below come from [Pan Docs specifications](https://raw.githubusercontent.com/gbdev/pandocs/master/src/Specifications.md),
[scrolling](https://raw.githubusercontent.com/gbdev/pandocs/master/src/Scrolling.md),
[OAM](https://raw.githubusercontent.com/gbdev/pandocs/master/src/OAM.md) and
[VRAM access](https://raw.githubusercontent.com/gbdev/pandocs/master/src/Accessing_VRAM_and_OAM.md).

- **Viewport:** the 32×32-tile hardware map can hold the full 32×16 room.
  Draw the 128-pixel playfield at the top and use the window for a bottom HUD
  (`WY=128`, appropriate `WX`, normally 7). Clamp camera X to 0..96, keep Y=0,
  and convert actor positions with the same camera. Give the player a stable
  horizontal dead zone. Test jumps and guardians approaching from off screen;
  scrolling changes visibility and requires playability review even when world
  geometry is unchanged.
- **Sprites:** 40 hardware OBJs total, at most 10 on a scanline. A 16×16 actor
  needs two OBJs in 8×16 mode: five such actors on one row already consume ten.
  Budget actors and effects explicitly. Cull hidden actors with off-screen Y,
  not just X. Keep collision active in world coordinates for off-screen actors.
  Do not treat disappearance/flicker as acceptable proof that a room works.
- **Colour:** DMG has four background shades and two OBJ palettes, each with
  three visible colours plus transparency. CGB adds per-tile/OBJ palette choices;
  it still does not display the host adapter's arbitrary RGB palette per pixel.
  Start with readable silhouettes. Using four 8×8 OBJs per actor for more
  colour segmentation doubles scanline cost versus the recommended layout.
- **Memory:** DMG has 8 KiB work RAM and 8 KiB VRAM. Keep room data/art in ROM,
  copy only mutable current-room state, and use explicit bounded arrays. A
  32×16 one-byte collision map costs 512 bytes. Separate tile roles from visual
  tile IDs, track crumble stages independently, and dirty-update changed cells.
  Do not allocate a 256×128 framebuffer (even 2bpp would consume 8 KiB).
- **VRAM/OAM:** use GBDK's supported sprite shadow/OAM path. Batch small tile
  changes into a bounded safe update phase; load bulk room art with the display
  disabled using the SDK's normal sequence. Do not redraw a complete room into
  VRAM every frame or perform unchecked writes while the PPU is drawing.
- **Banks:** a prototype may fit a 32 KiB unbanked ROM. For the full game budget
  code/art first, then use MBC5/autobanking if required; record bank ownership
  of data and keep interrupt-critical code accessible. Follow the
  [GBDK banking guide](https://gbdk.org/docs/api/docs_rombanking_mbcs.html).
- **Controls:** D-pad left/right, A jump, Start pause, B initially unused.
  Define simultaneous opposite directions and held-jump/re-jump behaviour from
  a trace, rather than adding modern controls silently.
- **Audio:** adapt effects/music to Game Boy channels after movement is stable.
  Original beeper busy loops are not a timing strategy for the port.

Use an installed, pinned GBDK release and its matching template. Verify its
example builds on the actual host (including CPU architecture) before promising
a working toolchain. No GBDK installation or Game Boy ROM has been validated by
this handoff. Setup: [GBDK Getting Started](https://gbdk.org/docs/api/docs_getting_started.html).

## Behaviour discovery still required

The colour adapter identifies drawings, not exact replacement gameplay rules.
Before implementing physics, trace the original through deterministic input
sequences at completed game updates, recording room, Willy fields, guardians,
item state, mutable floor state, air, lives and scores.

Use a separate host tool with `observationPC=34771` and a custom observer; the
machine has one observer context, so do not silently replace the enhancement's
hook and expect both callbacks to run. Use `StepInstruction()` and accumulate
its returned emulated cycles for timestamps, or record frame index and ULA
phase. `cpu.tStates` is per instruction, not an absolute clock.

Do **not** equate one Spectrum video frame, one game-loop update and one Game
Boy VBlank. The source loop includes music/effect delays and variable work.
Measure game-update spacing, including music on/off. Render at the Game Boy
refresh rate with a fixed simulation scheduler calibrated from those traces;
document any chosen timing approximation and latch input edges between ticks.

Willy's jump routine uses a discrete counter, with terrain-dependent early
landing/ceiling paths. Inspect and trace it before choosing gravity constants.
Reference: [jump/fall routine](https://skoolkit.ca/disassemblies/manic_miner/asm/35515.html).
Measure take-off, apex, landing, ceiling hits, walking off ledges, fatal falls,
conveyor influence, crumbling timing, enemy reversal and item/exit interactions.
Bounding boxes may be an early placeholder; compare original hazard/guardian
collision rules before claiming faithful gameplay.

## Suggested project structure and completion gates

Use a separate `ports/manic_gameboy/` project or a separate repository. The
current workspace contains unrelated, uncommitted Rick work and generated
Python prototype assets. Preserve them; do not stage, revert or refactor them
as part of this port. No branch switch or commit is required by this handoff.

Suggested modules: `src/main.c`, `simulation.c`, `collision.c`, `guardians.c`,
`render_gb.c`, `audio.c`; `include/`; `tools/` for extraction/conversion;
`assets/source/`, ignored `assets/generated/`, `tests/`, `build/` and `Makefile`.
Keep simulation free of Game Boy registers so a host test runner can replay
input traces against it. Treat emulator state fixtures as host research only.

1. **Reference bundle:** hash-checked local extraction, 20 names/raw records,
   sprite sheets, room previews, unresolved attribute/role report and first-room
   movement traces. No silent defaults for unknown bytes.
2. **Hardware smoke test:** reproducible `.gb`, Central Cavern background and
   Willy, HUD, safe tile uploads, bounded OAM use, working D-pad/A/Start.
3. **Playable first room:** collect every item, activate/use the exit, die and
   restart, expire air, crumble tiles and ride conveyors. Keep geometry stable
   while camera moves. Record a complete run and compare jump/collision cases
   with the reference. Placeholder movement is not a completed milestone.
4. **Room families:** add ordinary rooms and vertical guardians, then dedicated
   Eugene/Kong/Skylab/solar-beam and Final Barrier handlers. Test every room for
   reachability, progression and restart-state restoration.
5. **Hardware/resource pass:** run a hardware-accurate emulator and a second
   emulator, check scanline OBJ load, stack/RAM/banks and VRAM timing. Validate
   on actual hardware if available; otherwise state that it remains untested.

Each milestone should include exact build commands, pinned tools, the ROM,
known deviations, evidence of what was played/tested, and refreshed handoff
notes. Do not claim all-room completion from room initialisation alone.

## Local extraction bootstrap

This read-only Python snippet was exercised against the local tape. It checks
TAP checksums and extracts the final CODE block without requiring Allegro or a
booted emulator. The resulting file is **not** a snapshot or a bootable machine:
its lower 32 KiB are zero padding. Only addresses 32768..65535 contain source
payload. Raw data still needs the decoding and exceptional-room handling above.

```sh
python3 - <<'PY'
from functools import reduce
from hashlib import sha256
from pathlib import Path
source = Path('bin/manic.tap').read_bytes()
assert sha256(source).hexdigest() == '0229ca09bb87c58024d68b05a6a5ecd8ac93fb4932d937883722467f86277f4e'
blocks, pos = [], 0
while pos < len(source):
    assert pos + 2 <= len(source)
    size = int.from_bytes(source[pos:pos+2], 'little')
    block = source[pos+2:pos+2+size]
    assert size >= 2 and len(block) == size
    assert reduce(int.__xor__, block) == 0
    blocks.append(block)
    pos += size + 2
assert pos == len(source) and len(blocks) == 6
header, data = blocks[4], blocks[5]
assert len(header) == 19 and header[:2] == bytes([0, 3])
assert int.from_bytes(header[12:14], 'little') == 32768
assert int.from_bytes(header[14:16], 'little') == 32768
assert data[0] == 255 and len(data) == 32770
ram = bytes(32768) + data[1:-1]
Path('/tmp/manic-source-high-ram.bin').write_bytes(ram)
for room in range(20):
    base = 45056 + room * 1024
    name = ram[base+512:base+544].decode('ascii').strip()
    attributes = ram[base+544:base+616:9]
    unresolved = sorted(set(ram[base:base+512]) - set(attributes))
    print(room, hex(base), name, 'unmatched role attributes:', unresolved)
PY
```

Expected: 20 room names as above; unmatched role-attribute list empty for IDs
0..18, and 15 values for ID 19. Duplicate role attributes also occur, so this
report is not a final collision-map decoder. Next export raw 1bpp assets and
compare each decoded sheet/room with the original renderer.

## Paste into the next agent's task

> Read MANIC_MINER_GAMEBOY_HANDOFF.md, GAME_ENHANCEMENT_GUIDE.md and the referenced
> local Manic Miner sources. Develop a native Game Boy version using the
> handoff's recommended DMG/GBDK baseline. Begin with reproducible extraction,
> reference movement traces and a genuinely playable Central Cavern ROM. Keep
> original geometry and sprites, use a horizontal camera and a compact HUD,
> and preserve unrelated emulator changes. Treat uncertain physics, timing and
> special-room formats as research tasks, not established facts. Validate the
> ROM and report build/run instructions, tested behaviour and known deviations.
> Present the first-room result before expanding to all 20 rooms.
