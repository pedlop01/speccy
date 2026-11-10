# Batman: native Python first-room prototype

A standalone playable reconstruction of the opening hall from the local
`bin/batman.tap`. Python handles movement, jumping, animation, collision and
rendering. The emulator is used **only during asset preparation**; the running
Python game needs neither the emulator, tape nor ROM.

## Run

From the repository root, using Python 3.10 or newer:

```sh
python3 -m venv prototypes/batman/.venv
prototypes/batman/.venv/bin/python -m pip install -r prototypes/batman/requirements.txt
python3 prototypes/batman/prepare.py
prototypes/batman/.venv/bin/python prototypes/batman/game.py
```

If your Python already has Pygame installed, simply run:

```sh
python3 prototypes/batman/prepare.py
python3 prototypes/batman/game.py
```

Preparation requires the existing Linux/WSL [build dependencies](../../BUILD.md),
a C++11 compiler, `pkg-config`, and the repository's 48K ROM. It compiles its own
small extractor; no separate emulator build is needed. Compilation and the
headless tape boot take a few seconds. Preparation never changes the tape or
ROM. Its detailed log is `.build/extraction.log`.

Run preparation again after changing the extractor or Batman material maps.
Extracted assets and build outputs are ignored by Git. The extractor accepts
only the checked tape SHA-256
`c0619e7f31047563bfb91ba504e002b1ef09c9955a1147b5bb4e10ff5951c427`;
use `prepare.py --tape /path/to/batman.tap` for another location of that release.

Once prepared, `game.py`, `world.py` and `assets/` can be copied together to
another machine with Python/Pygame. Runtime code uses portable Python/Pygame;
asset preparation has been tested on Linux/WSL. Windows/macOS runtime has not
been tested here.

## Controls

| Key | Action |
| --- | --- |
| Arrow keys / WASD | Move along the isometric floor axes |
| Space | Jump; release before jumping again |
| C | Toggle enhanced colours / original monochrome art |
| R | Reset Batman to the starting position |
| Esc | Quit |

Left moves down-left, Right up-right, Up up-left, and Down down-right,
following the floor edges. Each direction keeps its own front/rear and reflected
orientation. Jump ascent holds a directional lifted-leg stride (also when jumping from
standstill); descent uses the original falling pose, and landing restores the walking animation.

The resizable window uses integer nearest-neighbour scaling. `--scale 4` starts
at 1280×960. The game pauses when its window loses focus.

## What this version includes

- The original opening hall, extracted as a scenery base and 12 masked layers.
- Eight original 24×32 Batman drawings, with reflected variants. Original front/rear
  walking tables use five poses; separate front/rear falling poses appear only
  during jump descent. Ascent uses a lifted-leg walking drawing because the
  extracted set has no separate ascent drawing. All eight source drawings are
  available as assets.
- The existing enhancement's authored material colours, preserving the original
  sprite silhouettes, eyes, masks, and opaque dark pixels.
- Native movement and jumping at a fixed 120 Hz physics step, with normalized
  movement, room boundaries and column footprints. Rendering runs at 60 Hz.
- Far-to-near scenery/Batman composition in world coordinates.

This is a first-room prototype, not a full port of the original game. Movement
speed, jump timing, collision footprints and animation selection are new,
explicit approximations. The original engine animates two half-sprites
independently; this prototype uses full-frame poses. Doorways stop at room
boundaries. There are no connected rooms, enemies, inventory, puzzles, deaths,
sound or save games yet. The colour toggle displays monochrome source art,
not an emulation of Spectrum attribute clash.

## Files and extension points

- `prepare.py`: reproducible local preparation and exact tape identity check.
- `extract_assets.cpp`: boots the tape using the existing machine, enters room
  8A40 through the original room-entry routine, and observes prepared blits at
  8E1D / completed regions at 8D9A. It records procedural backgrounds separately
  from scenery objects and converts their original ink/mask pixels to PNGs.
  Fixture setup writes are confined to this extraction machine. The observation
  callback reads state without patching execution or source art.
- `batman_materials.h`: local authored sprite material maps, copied from the
  colour enhancement branch so preparation does not depend on that branch.
- `world.py`: pure Python projection and physics, with no Pygame/emulator imports.
- `game.py`: input, frame accumulator, animation, layer ordering and UI.
- `assets/room.json`: extracted room bounds, spawn and source-object positions.
  Object PNGs retain their room-space placement, including clipped architecture.
- `tests/test_prototype.py`: native physics, timing, input and rendering checks.

The floor projection is `(128 + 2*(X-Y), 183-X-Y-2*height)`. A full Batman
sprite is placed 12 pixels left and 28 pixels above its projected foot position.
Original Z=192 is the base floor. Source graphics are sometimes reflected in
place by the original game's cache; extraction undoes that reflection before
writing canonical Batman poses. Enhanced and monochrome PNGs share alpha masks.

Future rooms should supply their own background, scenery layers, spawn,
collision volumes and transitions. Raised platforms need support/landing logic
before they become playable; a different PNG alone is insufficient. More
complex overlapping objects may require volume-based ordering rather than the
opening hall's X+Y painter order (with Z breaking ties).

## Verification

```sh
python3 -m unittest discover -s prototypes/batman/tests -v
SDL_VIDEODRIVER=dummy python3 prototypes/batman/game.py --frames 3 --screenshot /tmp/batman-python.png
```

Run preparation first so asset/rendering tests run rather than skip. Tests check
room boundaries, wall sliding, jump landing and key release, equal simulation
results at 30/60/144 Hz presentation rates, colour/reset/jump events, and column
occlusion including opaque PAPER pixels. A scenery comparison checks original
room pixels against the original engine's completed dirty-region pixels, excluding the
original HUD and the recorded pixels of its independently moving Batman.
These checks do not establish exact original gameplay physics.
