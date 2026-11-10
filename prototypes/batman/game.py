#!/usr/bin/env python3
"""Playable native Python opening-room prototype. Run prepare.py once first."""
import argparse
import json
import os
from pathlib import Path
import sys

os.environ.setdefault('PYGAME_HIDE_SUPPORT_PROMPT', '1')
try:
    import pygame
except ImportError:
    raise SystemExit('Install Pygame first: python3 -m pip install -r prototypes/batman/requirements.txt')

from world import STEP, World, project

HERE = Path(__file__).resolve().parent
SIZE = (320, 240)
ROOM_ORIGIN = (32, 30)


class Game:
    def __init__(self, assets=HERE / 'assets'):
        self.room = json.loads((assets / 'room.json').read_text())
        if self.room.get('format') != 1 or self.room.get('room') != '8a40':
            raise ValueError('Unsupported asset format; run prepare.py again.')
        self.world = World(self.room)
        self.enhanced = True
        self.bases, self.heroes, self.layers = {}, {}, {}
        for enhanced in (False, True):
            suffix = '' if enhanced else '-original'
            self.bases[enhanced] = pygame.image.load(assets / f'room{suffix}.png')
            for pose in range(8):
                image = pygame.image.load(assets / f'hero-{pose}{suffix}.png')
                self.heroes[enhanced, pose, False] = image
                self.heroes[enhanced, pose, True] = pygame.transform.flip(image, True, False)
            for obj in self.room['objects']:
                self.layers[enhanced, obj['name']] = pygame.image.load(assets / f'{obj["name"]}{suffix}.png')
        self.room_surface = pygame.Surface((256, 160), pygame.SRCALPHA)
        self.font = pygame.font.Font(None, 16)
        self.small = pygame.font.Font(None, 12)
        self.accumulator = 0.0
        self.jump_queued = False

    def update(self, elapsed, horizontal=0, vertical=0, jump=False):
        self.accumulator += min(max(elapsed, 0), 0.1)
        # Retain taps that arrive between physics ticks.
        self.jump_queued |= jump
        while self.accumulator + 1e-12 >= STEP:
            self.world.step(horizontal, vertical, self.jump_queued)
            self.jump_queued = jump
            self.accumulator -= STEP

    def handle_event(self, event):
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_c:
                self.enhanced = not self.enhanced
            elif event.key == pygame.K_r:
                self.world.reset()
                self.accumulator = 0
                self.jump_queued = False
            elif event.key == pygame.K_SPACE:
                self.jump_queued = True

    def render_room(self):
        scene = self.room_surface
        scene.blit(self.bases[self.enhanced], (0, 0))
        w = self.world
        # Far-to-near painter order in world space; original opaque PAPER pixels
        # remain opaque. At equal ground depth, stack ceiling blocks bottom to top.
        entries = [(o['x'] + o['y'], o['z'], index, o)
                   for index, o in enumerate(self.room['objects'])]
        entries.append((w.x + w.y, 192 - w.height, -1, None))
        for _, _, _, obj in sorted(entries, key=lambda item: (-item[0], -item[1], item[2])):
            if obj is None:
                pose, mirrored = w.pose()
                px, py = project(w.x, w.y, w.height)
                scene.blit(self.heroes[self.enhanced, pose, mirrored], (round(px) - 12, round(py) - 28))
            else:
                scene.blit(self.layers[self.enhanced, obj['name']], (0, 0))
        return scene

    def draw(self, canvas):
        canvas.fill('#0b1320' if self.enhanced else '#000000')
        canvas.blit(self.font.render('BATMAN', True, '#e9c05c'), (16, 8))
        canvas.blit(self.small.render('PYTHON / FIRST ROOM', True, '#a2b7cf'), (190, 11))
        canvas.blit(self.render_room(), ROOM_ORIGIN)
        pygame.draw.line(canvas, '#34434a', (16, 197), (304, 197))
        mode = 'ENHANCED' if self.enhanced else 'MONOCHROME'
        canvas.blit(self.small.render(f'OPENING HALL  /  {mode}', True, '#c1c9cf'), (16, 203))
        canvas.blit(self.small.render('ARROWS / WASD  MOVE     SPACE  JUMP', True, '#a2b7cf'), (16, 216))
        canvas.blit(self.small.render('C  COLOURS    R  RESET    ESC  QUIT', True, '#a2b7cf'), (16, 228))


def present(window, canvas):
    width, height = window.get_size()
    scale = max(1, min(width // SIZE[0], height // SIZE[1]))
    image = pygame.transform.scale(canvas, (SIZE[0] * scale, SIZE[1] * scale))
    window.fill('#05090f')
    window.blit(image, ((width - image.get_width()) // 2, (height - image.get_height()) // 2))
    pygame.display.flip()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--scale', type=int, choices=range(1, 7), default=3)
    parser.add_argument('--frames', type=int, help='Exit after N frames (smoke testing).')
    parser.add_argument('--screenshot', type=Path, help='Save final native-resolution frame.')
    args = parser.parse_args(argv)
    pygame.display.init()
    pygame.font.init()
    try:
        game = Game()
    except (OSError, ValueError, KeyError, pygame.error) as error:
        pygame.quit()
        parser.exit(1, f'Cannot load assets: {error}\nRun python3 prototypes/batman/prepare.py first.\n')
    window = pygame.display.set_mode((SIZE[0] * args.scale, SIZE[1] * args.scale), pygame.RESIZABLE)
    pygame.display.set_caption('Batman — Python first-room prototype')
    canvas = pygame.Surface(SIZE)
    clock = pygame.time.Clock()
    running, frames, focused = True, 0, True
    while running:
        elapsed = clock.tick(60) / 1000
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False
            if event.type == pygame.WINDOWFOCUSLOST:
                focused = False
                game.accumulator = 0
                game.jump_queued = False
            elif event.type == pygame.WINDOWFOCUSGAINED:
                focused = True
            if focused:
                game.handle_event(event)
        keys = pygame.key.get_pressed()
        if focused:
            horizontal = int(keys[pygame.K_RIGHT] or keys[pygame.K_d]) - int(keys[pygame.K_LEFT] or keys[pygame.K_a])
            vertical = int(keys[pygame.K_DOWN] or keys[pygame.K_s]) - int(keys[pygame.K_UP] or keys[pygame.K_w])
            game.update(elapsed, horizontal, vertical, bool(keys[pygame.K_SPACE]))
        game.draw(canvas)
        present(window, canvas)
        frames += 1
        if args.frames is not None and frames >= args.frames:
            running = False
    if args.screenshot:
        pygame.image.save(canvas, args.screenshot)
    pygame.quit()
    return 0


if __name__ == '__main__':
    sys.exit(main())
