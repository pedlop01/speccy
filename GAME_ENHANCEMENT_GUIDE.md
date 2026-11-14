# Game enhancement guide

Use this guide when adding or extending optional colour, tile or background
enhancements in Gomas. The goal is to enhance the original game while its
unmodified Spectrum program continues to determine gameplay.

This is a development workflow, not an implemented enhancement-pack format.
Manic Miner is the first code-based adapter. New games still need analysis.

## Non-negotiable rules

1. **Presentation only.** Do not patch game RAM, ROM, CPU registers, loaders,
   input, collision detection, timing or audio to produce enhanced graphics.
   Test fixtures may initialise game state; production adapters must not.
2. **Original remains available.** Keep the Original / Enhanced toggle. An
   unsupported version, scene or effect must use original video. Never guess
   game identity from the filename alone.
3. **Keep game knowledge outside the engine.** Game names, memory addresses,
   sprite layouts and room exceptions belong in adapters. CPU/ULA/machine
   interfaces may gain generic observation facilities when necessary, without
   game-specific branches. Do not change shared contracts just to fit one game.
4. **Preserve gameplay readability.** Original silhouettes, playable geometry,
   hazards, collectibles, exits and HUD must remain recognisable. Decorative
   scenery must not look like a solid platform or interactive object.
5. **Deterministic rendering.** The same observed state and enhancement settings
   produce the same pixels. No wall-clock animation, random colours or live AI
   inference in the rendering loop.
6. **Be precise about scope.** Distinguish new palettes, colour-clash separation,
   illustrated backgrounds and replacement artwork. Supporting many scenes does
   not mean each has new artwork. Tests do not imply a complete playthrough or
   visual acceptance.

## 1. Identify and analyse the game

Before coding, record:

- Exact media version, SHA-256, Spectrum model, loading steps and dependencies.
- Where code and graphics reside after loading; packed data, bank switches,
  alternate screens and self-modifying code that matter to observation.
- The actual drawing process: background construction, sprite masks, OR/XOR
  drawing, attribute updates and the final screen transfer.
- Actor coordinates, coordinate units, animation frames and direction encoding.
- Tile definitions and game roles; dynamic tiles, items, portals and HUD regions.
- Scene/mode indicators and special effects needing fallback.

Use disassemblies as evidence, then verify the relevant instructions and data
against the local version. Keep links and address notes with the adapter. Do
not assume similar-looking releases share the same memory layout.

Choose a completed game drawing boundary. A hardware frame boundary alone does
not ensure that a game's position variables and finished screen agree.

## 2. Implement a small, isolated adapter

Start with one scene and one actor. Demonstrate correct movement and layering
before expanding. Reuse the existing host presentation path and toggle.

Conceptually, each adapter needs:

- Identification of supported media and machine model.
- A bounded, read-only capture of a coherent game drawing update.
- A description of supported scenes and effects.
- Rendering into a host image, with an explicit fallback when unavailable.
- Invalidation after reset, media change, restore, rewind or scene change.

These are design responsibilities, not a requirement to introduce a framework
or scripting language before a second game demonstrates a shared need. The
current machine observer supports two PCs with one context/callback; it is not a general
multi-adapter registry. The callback and its context must remain alive while
registered. Callbacks must not retain references into temporary machine state.

Do not identify and hash media on every pixel/frame. Cache identification per
session and invalidate appropriately. Bound observation work and measure actual
frontend responsiveness when rendering becomes more complex.

## 3. Compose the enhanced image

Use the game's source graphics and semantic state when possible. The final
Spectrum bitmap has already merged objects and is insufficient for reliable
object separation in general.

Decide and document the layer order for the particular game. A typical order is:

1. Decorative background, restricted to identified empty scenery.
2. Playable tiles and dynamic scenery.
3. Items and exits, respecting their real draw order and effects.
4. Actors, preserving game-specific overlap/occlusion rules.
5. HUD and overlays.

Colour only an actor's actual set/masked pixels, not its bounding rectangle or
attribute cells. XOR sprites may require reconstructing the background rather
than treating the final screen bits as an intact sprite. Test that explicitly.
Distinguish source zero bits from true black: Spectrum zero bits display PAPER,
which can change with the underlying tile. An opaque enhanced sprite must not
borrow that tile colour for its face, outlines or clothing. Reconstruct the
foreground using its mask, authored materials and explicit feature colours.
Only pixels outside its silhouette are transparent. Check the same foreground
over at least two different PAPER colours. Preserve intended invisible graphics,
flashing effects and coloured PAPER in unenhanced layers unless their replacement
has been deliberately implemented and checked.

Tie palettes to stable actor identity, local sprite coordinates and tile roles.
Do not let colours change when the actor crosses a screen cell. For detailed
figures, author explicit material masks for each animation drawing: face/hands,
hair, clothing and footwear. Check reflected frames against the original art.
Preserve original facial features, expressions and clothing patterns during
colouring; do not silently replace them with newly drawn features.
Do not infer anatomical parts from shared horizontal bands or stretch a standing
palette over a crouching pose. If simple row bands suit a particular sprite,
label that treatment accurately and inspect its result on every relevant pose.
Export an enlarged original/coloured sprite sheet, inspect it visually and check
it in the real game. Non-Spectrum RGB values and a passing decoder test prove
neither sensible material placement nor visual quality. Keep unchecked actors
on original colours instead of assigning guessed human palettes to objects.

For decorative backgrounds, use subdued contrast, stable spatial placement and
clear depth cues. Draw them only behind identified background pixels. Do not
cover tile holes, items or animation merely because a pixel looks black. Give
special rooms their own artwork or keep their existing background; do not
silently apply inappropriate scenery everywhere.

## 4. Validate before calling a block complete

Run checks appropriate to the changed adapter and any shared code it touches.

- Boot the real local media through the ROM/loader and enter gameplay.
- Exercise movement, jumps, animation, overlap and background restoration across
  multiple updates. A coloured screenshot alone does not prove correct layering.
- Compare execution from the same checkpoint with and without observation:
  RAM, CPU registers, original ULA pixels and audio must agree. Include transport
  and other devices when the observation path could affect them.
- Verify palette/layer behaviour using controlled shared-cell fixtures as well
  as real scenes. Check that scenery stays put when actors leave an area.
- Test unsupported media/model/scene fallback, reset, toggle, pause, death,
  scene transitions, save/load and rewind as applicable. Avoid stale enhanced
  images after a discontinuity; either invalidate or recapture coherently.
- Exercise both memory and **GPU bitmaps**. Allegro locks may have negative
  pitch: use signed row-offset arithmetic and respect the requested pixel format.
  Read back pixels; a successful upload alone is insufficient.
- Compare Original / Enhanced screenshots of the same state. Inspect background
  visibility, object contrast, hazards, exits and HUD at actual play scale.
- If shared machine/CPU/ULA code changes, run the relevant existing regressions
  and the full suite where appropriate. Report what actually ran.

Record exclusions. Initialising every room and running a short segment proves
much less than completing every room. Leave visual judgement to the user and
incorporate their observations into the next bounded block.

## 5. Document and hand off

For each adapter keep version identification, memory-map sources, capture
boundary, coordinate conversions, draw order, palette/artwork rules, supported
scenes, fallback conditions, tests and known gaps. Link the instructions from
README.txt. Keep user-supplied media out of unrelated changes.

Summarise each completed block with what visibly changed, how to try it, checks
run and limitations. Do not commit unless the user authorises that commit.

## Worked example: current Manic Miner prototype

- Adapter: `src/colour_enhancement.h`, `ManicColourEnhancement`.
- Media: `bin/manic.tap`, SHA-256
  `0229ca09bb87c58024d68b05a6a5ecd8ac93fb4932d937883722467f86277f4e`.
- Machine: 48K. Toggle: View > Experimental colour enhancement, or Ctrl+E.
- Capture: PC 34771, after the game's bitmap and attribute transfers.
- Reference: https://skoolkit.ca/disassemblies/manic_miner/asm/34574.html
- Tile definitions: eight nine-byte records at 32800; background attributes at
  24064 and background bitmap at 28672.
- Willy: doubled pixel y at 32872, animation at 32873, direction flags at 32874,
  attribute-buffer location at 32876, graphics at 33280.
- Horizontal guardians: records at 32958; vertical guardians: 32989; shared
  guardian graphics at 33024. Decode each family's coordinates separately.
- Coloured actors/tiles: 15 caverns. Original-video fallback: Eugene's Lair,
  both Kong rooms, Skylab Landing Bay and Solar Power Generator; also demos,
  title screens, flashes, unsupported models/media and invalid captures.
- Illustrated backdrop: **Central Cavern only**. Other supported caverns have
  a tint. Guardian palettes are a shared row-based treatment.
- HUD and save thumbnails use original video. Current adapters are code-based.

Build and run its regression from the repository root:

```sh
make -C bin -j2 gomas colour-enhancement-tests
(cd bin && ./colour-enhancement-tests)
# Requires a graphical session; opens a temporary window:
(cd bin && ./colour-enhancement-tests --video)
```

The regression boots the real tape, walks/jumps, exercises all 20 cavern
initialisation/drawing loops, checks the 15 supported rooms and five fallbacks,
compares observed/unobserved execution, tests shared-cell layers and reads back
GPU pixels. It exports previews under `/tmp/manic-*.png`. This is **not** a full
playthrough, comprehensive coverage of every lifecycle event, or proof that all
visual edge cases are solved. Extend coverage when a change exposes a gap.

For a native Game Boy adaptation, read
[MANIC_MINER_GAMEBOY_HANDOFF.md](MANIC_MINER_GAMEBOY_HANDOFF.md). It covers source
extraction, Game Boy constraints, unresolved gameplay research and a first-room
implementation plan; the colour adapter itself does not implement game logic.

## Starting a future task

A useful request template:

> Read GAME_ENHANCEMENT_GUIDE.md. Analyse [media path/version]. First identify
> its rendering process and propose a bounded enhancement for [scene/actors].
> Keep gameplay unchanged, provide Original / Enhanced comparison, validate
> memory and GPU paths, and document supported scenes and fallback conditions.
> Use existing facilities where possible; do not assume Manic Miner's memory
> layout or introduce a general pack format without a demonstrated need.

## Army Moves: opening jeep prototype

- Adapter: `src/army_colour_enhancement.h`; selected by
  `src/game_colour_enhancement.h` using exact media contents.
- Media: `bin/army.tap`, SHA-256
  `9cce91fa703d74f31a809218a9162b26ea0c445de382ad2308c831e4ec7500ec`.
- 48K only. Load with `LOAD ""`, then select **3—Play**. The existing Ctrl+E /
  View toggle enables the enhancement. The feature starts disabled.
- Scope: opening jeep stage, original wheel frames at 0x7F0C / 0x7FFE.
  Olive body/canvas, dark tyres, metal hubs, blue windscreen; muted bridge,
  mountain and stone-column colours and a dark sky. Preserve original source art.
  Enemy helicopters (six rotor/tilt drawings) have blue-grey metal, cyan glass
  and amber markings; enemy trucks (two wheel drawings) have rust-red bodywork,
  tan canvas and dark tyres. Other vehicle drawings, projectiles, explosions
  and HUD retain their original colours. Helicopter/later player stages and death/transition effects fall
  back to original video. This is not a full-game enhancement.
- Addresses were established by instruction tracing the local tape. The masked
  sprite header entry is 0xBB6B; FF14 is signed y, FF15 signed x in four-pixel
  units, FF16 points to source art. Header bytes give height and doubled byte
  width; each row stores ink bytes then mask bytes. Zero mask bits are opaque.
- Capture at 0xCF56, after F2C7's completed screen transfer and before F2A5
  restores the next clean background. The playfield is 240x104 at screen (8,16).
  Clean background: 0x6B90. Composed bitmap: 0x5DC0. Snapshot that composed
  buffer before its first sprite: the clean buffer scrolls after drawing. Attributes: 0x69F0,
  32-column stride. Sprite source selection uses 0xA238 offsets from 0x7960.
- Replay bounded observed sprite masks in their original order. Verify the
  reconstructed monochrome scene equals every byte of the actual composed
  bitmap before accepting a capture. Unrecognised/incomplete rendering falls
  back, rather than showing a guessed reconstruction. Preserve opaque black
  source details rather than letting the sky show through vehicles.
- The shared read-only observer now accepts two PC boundaries, for sprite
  observation and completed scene capture. Manic Miner still uses one. Adapter
  selection and identification are refreshed on session changes; switching back
  to original rendering forces a complete upload to erase enhanced pixels.
- Test: `make -C bin army-colour-tests`, then `(cd bin && ./army-colour-tests)`.
  Add `--video` in a graphical session to exercise GPU locks and pixel readback.
  Exports `/tmp/army-original.png`, `/tmp/army-enhanced.png` and the enlarged
  sprite sheet `/tmp/army-jeep-comparison.png`. Checks real
  loading/starting, keyboard movement/jump/fire and wheel poses, ordered reconstruction, original
  machine state, observed/unobserved execution, media selection and fallbacks.
  The unattended opening segment includes deaths; it does not complete a level.

The opening bridge uses a material palette independent of Spectrum attributes,
including the broken ends (0x963C), trusses (0x9458) and arch (0x955A). The
0x963C drawing depicts broken bridge girders, not palms. Column drawings at
0x971E and 0x8E88 use one stone palette, including opaque shadow detail; their
attribute cells must not recolour bridge pixels behind them.

Enemy material maps live in `src/army_enemy_colours.h`. Helicopter source
addresses are 0x7960, 0x7A52, 0x7B44 (level) and 0x7C36, 0x7D28, 0x7E1A
(tilted); each pose has its own authored 40x24 material map, shared only across
its three rotor frames. Trucks at 0x80F0 and 0x81E2 share a body/canvas/glass/tyre
map across their two wheel drawings. Every opaque zero bit receives an explicit
material shadow, preserving source detail without borrowing background PAPER.
The regression composes all eight drawings over red and white attribute cells,
checks their opaque pixels, and exports `/tmp/army-enemies-comparison.png`
(original/coloured pairs) and `/tmp/army-enemies-gameplay.png`. Real opening
play records the encountered enemy drawings; fixture coverage of all frames is
not a claim that the unattended run encounters every enemy or completes a level.

The opening-stage backdrop adds a static sky gradient and two distant mountain
ranges with muted blue faces and haze. It fills only identified empty background
and the opaque zero bits of bridge openings; original bridge ink, columns,
vehicles and other sprite masks retain their drawing order and colours. The
mountains remain fixed in screen space to suggest distance; there is no
parallax animation or gameplay geometry. Regression checks cover scenery
through broken bridge masks and deterministic output under attribute changes.

## Batman: six checked isometric rooms

- Exact media: `bin/batman.tap`, SHA-256
  `c0619e7f31047563bfb91ba504e002b1ef09c9955a1147b5bb4e10ff5951c427`.
- 48K only. Checked rooms: 0x8A40 (opening hall), 0x8940, 0x9A40,
  0x8840, 0x9940 and 0x8A30. The opening hall has X=32..48, Y=8..72;
  other checked rooms use their own validated bounds and subdued palettes.
  Load normally, choose keyboard controls and Play with ENTER, and dismiss the
  equipment introduction. Ctrl+E uses the existing Original / Enhanced toggle.
- Scope: Batman's eight drawings and their reflections, existing floor and wall
  patterns, shared arches/columns, stone blocks, metal and raised platforms in
  the checked rooms. Blue cape/cowl/gloves/boots,
  grey suit, gold belt and skin tones on the existing exposed lower face.
  `src/batman_materials.h` contains authored source-local material regions.
  The sprite's original mask and ink bits, facial features and animation remain.
- Unchecked rooms, incompatible media/model, death and invalid captures use
  original video. Equipment icons overlap the room's screen rectangle; their
  original pixels are explicitly excluded from the overlay, as is the lower HUD.
  There is no new background art, furniture or gameplay geometry.
- Addresses established by instruction tracing this local release:
  0x8E1D dispatches a prepared, clipped and shifted masked sprite blit into the
  six-byte-row A000 scratch buffer. Alternate BC is the destination, alternate
  DE the ink and alternate HL the mask; B gives height and 9691 source stride.
  The dispatch table identifies the actual copied byte width. 8CD1/8CD2 are
  right/left bounds in two-pixel units; 8CD3/8CD4 are bottom/top bounds, with
  screen origin (64,72). 8DAA points to the current object's record plus two.
- 0x8D9A finishes the room region before 82EA transfers it to VRAM. Replay the
  observed blits in their original order into a host colour buffer and a
  monochrome check buffer; compare the latter byte-for-byte with actual A000
  before publishing the region. Keep persistent colour pixels outside dirty
  regions, including restoration of backgrounds after movement. Do not sort
  actors again or put Batman unconditionally above the room's furniture.
- Room identity at 658B; room dimensions at 6A41..6A44. Batman's two half-sprite
  records start at 8F18/8F2A. Drawings 10..17 occupy 24x32 ink+mask pairs starting
  at C530, 192 bytes per drawing. Option bit 1 selects its upper/lower 16 rows;
  drawing bit 7 selects reflection. Coordinate conversion uses the real source
  position, half selection and reflection, including clipped/shifted drawings.
- Test: `make -C bin batman-colour-tests`, then
  `(cd bin && ./batman-colour-tests)`; add `--video` for a temporary GPU window.
  The regression boots the real tape, moves/turns/jumps in the opening hall,
  checks incremental reconstruction and presentation/execution isolation,
  preserves HUD/border pixels, and exercises both overlap orders with opaque
  column pixels. Each of the six rooms is initialised through original code and
  compared with unobserved execution. The original exit dispatcher is exercised
  from 8A40 to 8940 and back. These are controlled fixtures, not a full playthrough. It exports `/tmp/batman-original.png`,
  `/tmp/batman-enhanced.png` and `/tmp/batman-sprite-comparison.png`.
  Fixture coverage does not imply a complete game playthrough or visual
  acceptance. Further rooms need their own material/scene analysis before support.

Room palettes and bounds live in `BatmanColourEnhancement::FindRoom`. Floor
classification inverts the game's projection against those bounds instead of
reusing the opening hall's diamond. It adds no geometry. Stone, metal and raised
platform materials follow source drawing IDs and opaque masks independently of
their screen cells. Unchecked enemies/items keep their original Spectrum
INK/PAPER. On a room change, availability immediately rejects the previous
room's cache, even for rooms with identical dimensions; the first new region
starts a fresh canvas from the actual display. Room previews are exported as
`/tmp/batman-room-XXXX-original.png` and `...-enhanced.png`.

## Agent handoff: continuing this work

Read this guide **and the relevant adapter/test source** before changing a game.
The checked-in code and local media are authoritative; do not depend on a prior
agent's conversation, temporary scripts or `/tmp` files. The game memory maps
are version-specific research, not a portable interface shared by all games.

### Where to work

| File | Responsibility |
| --- | --- |
| `src/game_colour_enhancement.h` | Exact-content adapter selection, session refresh and original-video restoration |
| `src/colour_enhancement.h` | Manic Miner capture and rendering |
| `src/army_colour_enhancement.h` | Army Moves capture, ordered mask replay, scenery and backdrop |
| `src/army_enemy_colours.h` | Authored helicopter/truck material maps |
| `src/batman_colour_enhancement.h` | Batman's dirty-region replay, room bounds/palettes and shared scenery materials |
| `src/batman_materials.h` | Batman's eight authored material maps |
| `src/rick_colour_enhancement.h` | Rick Dangerous II player masks, all-actor background protection, Hyde Park scene guards and scenery capture |
| `src/rick_materials.h` | Rick's fourteen authored pose-local material maps |
| `src/rick_scenery.h` | Authored palettes for identified solid and background tiles across Hyde Park |
| `src/tests/*colour*test.cpp` | Real-media boot, fixtures, execution comparisons and reproducible previews |
| `src/main.cpp` | Existing Ctrl+E/View toggle and presentation integration |
| `src/emulator.h` | Generic read-only instruction observation, not game-specific rendering |

### Adding another Batman room

1. Start from `src/tests/batman_colour_test.cpp`. Boot the supplied tape and
   obtain a genuine gameplay checkpoint. Inspect the target room's data and
   original graphics before allowing its ID in `FindRoom`.
2. The existing controlled fixture writes the room word at 658B, releases keys,
   chooses an idle spawn inside the actual bounds, and enters original code at
   PC 65B3 with SP FFF4. Those writes belong **only in research/tests**. Runtime
   enhancements must observe normal play without patching the machine.
3. Record the resulting room ID, bounds at 6A41..6A44, original floor pattern,
   encountered source drawings and any special effects. Invalid/empty room
   coordinates can fail to initialise correctly. Do not infer that every ID
   adjacent numerically to a valid room is playable.
4. Add only the checked room's bounds/palette to `FindRoom`. Reuse `Scenery`'s
   inverse floor projection when applicable. Verify raised floors, holes and
   unusual wall geometry before applying that model to them. Keep unsupported
   layouts on original video rather than relaxing `Scene` checks blindly.
5. Inspect unfamiliar source art before extending `Material`. Material colours
   follow drawing IDs and masks, not screen positions or current attributes.
   Preserve the existing original-colour fallback for unchecked enemies/items.
6. Extend the room fixtures, compare observed/unobserved execution and inspect
   original/enhanced images of the same checkpoint. Exercise real movement and
   overlap as well as controlled room initialisation. Check leaving and returning
   to the room; identical dimensions do not mean the same cached image is valid.
7. Add the room to this guide's scope and report exactly what was tested. A
   fixture entering a room is not a claim of reaching it during a playthrough.

For the original exit-dispatcher fixture, PC 6603 reads the exit selection at
9B05. The current test uses selections 2 and 4 to go from 8A40 to 8940 and back.
This exercises the actual dispatcher after fixture setup; it is not a keyboard
walkthrough of the doorway.

### Details that are easy to miss

- Batman mutates cached source drawings when reflecting them. The cache bits at
  A840 track current reflection: byte `drawing / 8`, bit `0x80 >> (drawing & 7)`.
  When exporting a canonical sprite sheet, undo that cached reflection before
  applying the desired display reflection. See the atlas export in the test.
- `Reset` must also clear capture timestamps and room identity. After restoring
  an earlier checkpoint, an old timestamp must not repeatedly discard new layers.
- Batman's scratch buffer holds up to 42 six-byte rows in one 256-byte page.
  Prepared blitters advance the destination low byte by six per row; use the
  observed dispatch width/stride and validate the replay against the real buffer.
- The overlay excludes original equipment-icon rectangles at screen (232,136)
  and (200,152), besides the lower HUD. Preserve those exclusions when extending
  room support, and verify any new layout that reaches into them.
- On fallback, the selector forces a full original ULA upload. Incremental video
  updates alone may leave unchanged enhanced pixels on the host bitmap.
- Avoid hashing media or copying the whole ULA state inside a pixel loop. Cache
  identity per session and capture frame-wide properties once per update.

### Reproducible verification

From the repository root:

```sh
make -C bin -j2 gomas batman-colour-tests army-colour-tests colour-enhancement-tests
(cd bin && ./batman-colour-tests)
(cd bin && ./army-colour-tests)
(cd bin && ./colour-enhancement-tests)
# Requires a graphical session; verifies real video bitmap uploads/readback:
(cd bin && ./batman-colour-tests --video)
```

Use the other adapters' `--video` runs when changing their presentation paths.
Run `make -C bin test` when shared machine/CPU/ULA changes warrant the full suite.
The tests regenerate the documented per-game previews; `/tmp` artifacts are
outputs, never required inputs. Inspect them visually before declaring an art
change ready. Keep changes uncommitted until the user requests a commit.

## Rick Dangerous II: Hyde Park player and scenery enhancement

- Adapter: `src/rick_colour_enhancement.h`; material maps:
  `src/rick_materials.h`. Selected by exact tape contents, not filename.
- Local `bin/rick2.tap`, SHA-256
  `4592e610385098122800c50651af7bff82b605a945ba1e185601a984f8896be8`.
  48K only. Load with `LOAD ""`, press a key at the loading picture, then
  **1 Keyboard**, **0 Start Game**, **1 Hyde Park London**. Ctrl+E or the
  existing View menu toggles the enhancement; it starts disabled.
- Scope is **Rick in all eight populated Hyde Park rooms**, IDs 0..7, with
  64 checked viewport positions in total. Level 7EF1=1; room is 9B93 and scroll
  is 88CF, in eight-tile/64-pixel increments. Maximum scroll offsets by room:
  0,112,112,0,104,104,8,8. The room's actual maximum at 88D2 must match.
  Death flag 9B9C and bike state 8A24 must be zero; player active 9BAB must be
  one. Other levels, bike and death effects retain original video. This is not
  whole-game support or a claim of completing Hyde Park.
- Fourteen checked drawings: C26D..C7ED spaced 128 bytes apart (standing,
  five walking drawings, two crouch/crawl drawings, jump, raygun, punch and
  detonator action), plus C8ED/C96D for the two rear-facing climbing poses.
  C86D and C9ED/CA6D are excluded effects, not extra walking/climbing poses.
  Blond hair, warm face/hands, red jacket, blue trousers,
  brown boots/belt and a steel raygun. The head boundary uses blond hair above
  skin below, with the original dark eye details preserved; no cyan face stripe.
  Pose-local authored material maps colour only the original source silhouette. Opaque zero bits keep explicit dark
  outlines/details rather than borrowing the underlying cell's PAPER colour.
  Neither source pixels nor facial features are redrawn. Other actors, objects,
  unchecked hazards, border and HUD retain their original colours. Identified
  scenery is coloured in all eight rooms and all 64 scrolling viewports.

### Hyde Park scenery

`src/rick_scenery.h` assigns materials to exact tile IDs throughout level 1:
174 is the original diagonal exterior hatching (subdued blue); 175..180 are
steel roof/wall sections; 27..30 and 245..246 are bronze cross-braced supports
and capitals; 181..183 are muted grass edges; 184..186 are earthy root/soil
textures. Interior panels (35..48) use muted plum/steel; brickwork (73..77) uses
blue-grey with dark mortar; vents (78..83), doors (84..93) and the framed wall
display (61..72) have complementary metal tones. Safe pipe sections
(118..125,128..131,134..135,138..139,142..143) use pale oxidised metal; ladder
rungs (160..161) use warm brass. Identified patterned platforms, bracing and
grilles (49..60,102..117,150..151,156..159,195..197,231..234,255) use steel;
platform end caps (164..169) use warm bronze. The palette is shared by tile
identity across rooms and scroll positions. Heated pipe/ladder variants and
other unchecked tiles/effects keep their original colours. Both original INK
and PAPER receive explicit colours, preserving the original 8x8 pixel shapes.

Outdoor backgrounds keep their **original artwork and empty pixels**. No
procedural sky, roofline, hedges or tree crowns are drawn. `OutdoorDetail`
identifies the existing artwork in rooms 0/3 and room 2's outdoor return at
scroll 112 below screen Y=72. Existing tree/foliage tiles 228..230 get green INK,
small background structures 187/188 get warm stone INK, and fine texture tiles
243/244/249 get muted grey-green INK. Their zero bits receive no overlay, so
original PAPER shows through every gap. Empty tile 0 is never coloured, in any
room. Exterior tile 174 retains its original diagonal pattern with the subdued
blue palette. Existing building walls, brickwork, doors and platforms retain
all previously checked material palettes. Actor masks and completed-screen
validation protect sprites and original effects throughout.

Capture at 7D91 accepts identified solid and non-solid tiles. Accumulate an
opaque mask for **every** draw at 7329, including uncoloured/unknown actors and
both source INK and opaque zero bits. Routine 85FA draws 9BB4 **before** player
9BAB, then 9BBD and its object lists; do not clear the mask when Rick is drawn.
Clear it after each completed upload and on scene/generation/rewind changes.
Malformed source geometry disables scenery for that capture. Foreground tiles
(A800+ID bit 7) still suppress actor masks, matching the original blitter.

Source bytes at A900+ID*8 must match the completed Spectrum bitmap at every
pixel not covered by an actor. Any unexplained differing bit rejects the whole
tile. Covered pixels never receive scenery colour, even when their original
bits happen to equal the background. This preserves complete actor silhouettes
without toggling whole background cells as actors move. Unrecognised tiles,
including objects and unchecked hazards, stay original. Fine exterior texture
is recoloured only in the explicitly identified open-park areas.
The scenery buffer is separate from player pixels, is refreshed on each
successful capture and uses the same scene/generation/frame validity checks.
Presentation restores original ULA video, applies scenery, then player pixels.

Tests check per-pixel scenery materials in all eight rooms and all 64 views,
with full bitmap readback on memory and GPU bitmaps. Fixtures draw actors both
before and after Rick over real non-solid brickwork and original foliage, using both reflections
and PAPER colours. They check exact exclusion of INK and opaque zero bits,
visible uncovered background, and whole-tile fallback on an unexplained changed
finished-screen bit. These fixtures initialise rooms through original code;
they are not a full level playthrough.

### Renderer and room layout

Addresses and layout were established by disassembling and tracing this exact
local tape, not by borrowing the first Rick Dangerous game's memory map:

- PC 7329 enters the ordered actor blitter. IX points to an actor record:
  +1 X in pixels, +2/+3 world Y little endian, +4/+5 source pointer,
  +6 bit 7 horizontal reflection. Player record is 9BAB.
- Source header is vertical offset, row count, then six bytes per row.
  In column order 0,1,2, INK is at row offsets 2,1,0 and MASK at 3,5,4.
  Output is `(background & mask) | ink`. A set mask bit is transparent only
  when its INK bit is zero. Source-local reflection flips the full 24 pixels.
  The crouching drawings have offset 5 and height 16; standing drawings have
  offset 0 and height 21. Do not stretch a standing material map over a crouch.
- Screen Y is world Y minus 64. The playable viewport starts at screen Y=8;
  screen row 0 is the original HUD. Tile IDs at 6B00 use a 32-column world
  grid. A800+tileID bit 7 marks foreground tiles whose actor uploads are
  suppressed by the original renderer. These tiles must also suppress player colour.
- PC 7D91 is immediately after the completed 76D4 screen transfer in the normal
  gameplay loop. Pending player colour pixels are masked away by later actors,
  preserving their actual drawing order. Every remaining player pixel's INK bit
  is checked against the completed Spectrum bitmap before a capture is accepted.
  Failed or incomplete captures use original video.
- Only identified player/scenery pixels are uploaded. Restore the full original ULA image before
  each sparse colour overlay: unchanged black/white bits can otherwise retain
  coloured trails. Use signed GPU pitch arithmetic. The common selector also
  restores original video on fallback. Scene, frame age, generation and rewind
  checks prevent use of an old viewport's pixels. Both pending and published
  captures carry level/room/scroll identity: changing to another room with the
  same dimensions still invalidates the old canvas immediately. Backward frame
  changes clear pending pixels/timestamps even without reattaching the adapter.
- The original scroll routines at 78A5 (down) / 7905 (up) rebuild the visible
  tile grid at 6B00, adjust 88CF by +/-8, and call 9E31 to adjust actor Y by
  -/+64. Actor Y remains relative to this tile grid, so screen Y stays Y-64;
  do not subtract the cumulative scroll a second time.
- Original room entry at 8A6D receives A=room ID, C=scroll, E=player X and
  HL=player Y. The room table at Word(C202), eight bytes per room, supplies
  background data, maximum scroll (byte 3), exits and object lists. Initialising
  these registers belongs only in tests/research, never in the runtime adapter.
  In this local release rooms 8..16 are empty placeholders: do not infer that
  all room table entries describe playable rooms.
- The original exit dispatcher at 8993 reads direction 88D3 and the current
  room's exit records. The regression uses right=2 from room 0 into room 2 at
  scroll 112, then left=1 back into room 0 at scroll 0. It executes original
  destination selection; fixture positioning is not a keyboard walkthrough.

Controls from the original manual: **O** jump/up, **K** crouch/down,
**Z/X** left/right and **Space/Enter** fire. Fire modifies directional actions.
Reference: [original Spectrum manual](https://www.gamesdatabase.org/Media/SYSTEM/Sinclair_ZX_Spectrum/Manual/formated/Rick_Dangerous_II_-_1990_-_MicroStyle.htm).
Runtime addresses and source formats are verified against the local tape.

Build and verify from the repository root:

```sh
make -C bin -j2 gomas rick-colour-tests
(cd bin && ./rick-colour-tests)
(cd bin && ./rick-colour-tests --video) # graphical session required
```

The test boots the real tape, enters Hyde Park, walks, jumps, crouches and
reverses direction. It checks execution with/without observation, original
RAM/registers/ULA/audio preservation, all fourteen source drawings at all eight
pixel alignments, both reflections and two PAPER colours using the actual
original blitter/upload routines. Readback verifies the overlay and every
unchanged HUD/background/border pixel. Controlled foreground-tile and later-actor
fixtures check exact occlusion masks; a poisoned host-image fixture checks full
background restoration. It checks scene/death/media/reset/toggle
fallbacks and selector changes. Each of the eight rooms is entered through
original code and run for 60 frames with/without observation. Every one of the
64 viewport positions is initialised through original entry and checked using
the real blitter/upload with climbing poses, including full GPU readback. These
viewport fixtures prove rendering, not survival at arbitrary spawn locations.
Keyboard O at the real room-1 ladder (fixture entry X=40,Y=184,scroll=0)
exercises both climbing drawings through original movement logic. Additional
fixtures exercise the original exit dispatcher, scroll routines in both
directions, original death/respawn and backward state restoration without
reattaching the observer. These are bounded gameplay and fixture checks, not a
complete playthrough or validation of all special effects.

Reproducible previews:
`/tmp/rick-original.png`, `/tmp/rick-enhanced.png`, and
`/tmp/rick-player-comparison.png` (fourteen original/coloured pairs, normal then
reflected). Per-room previews are `/tmp/rick-room-N-original.png` and
`/tmp/rick-room-N-enhanced.png`, where N=0..7. Inspect the atlas and real gameplay
when refining material regions.
All-view scenery previews are `/tmp/rick-viewport-N-S-enhanced.png`, with room N
and scroll S from its checked 8-tile increments. These use a controlled climbing
source-mask fixture in the real viewport, not keyboard traversal of the level.

Rick's real keyboard-climbing previews are `/tmp/rick-climbing-original.png`
and `/tmp/rick-climbing-enhanced.png`. Do not confuse the gold vertical room-1
scenery at other positions with a usable ladder spawn: the climbable test enters
at X=40,Y=184 and then presses O, without writing the climbing state flag.
