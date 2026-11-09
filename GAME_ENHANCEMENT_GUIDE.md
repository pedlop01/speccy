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
current machine observer supports one PC/context/callback; it is not a general
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

## Starting a future task

A useful request template:

> Read GAME_ENHANCEMENT_GUIDE.md. Analyse [media path/version]. First identify
> its rendering process and propose a bounded enhancement for [scene/actors].
> Keep gameplay unchanged, provide Original / Enhanced comparison, validate
> memory and GPU paths, and document supported scenes and fallback conditions.
> Use existing facilities where possible; do not assume Manic Miner's memory
> layout or introduce a general pack format without a demonstrated need.
