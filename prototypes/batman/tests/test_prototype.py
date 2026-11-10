"""Native physics and rendering regressions; asset checks need prepare.py first."""
import os
from pathlib import Path
import sys
import unittest

os.environ['SDL_VIDEODRIVER'] = 'dummy'
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE))
from world import STEP, RADIUS, Solid, World, project


ROOM = {'bounds': [32, 8, 48, 72], 'spawn': [40, 64], 'objects': []}


class PhysicsTests(unittest.TestCase):
    def test_room_boundaries_in_every_direction(self):
        for h, v in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1)):
            w = World(ROOM)
            for _ in range(3000):
                w.step(h, v)
            self.assertGreaterEqual(w.x, 32 + RADIUS)
            self.assertLessEqual(w.x, 48 - RADIUS)
            self.assertGreaterEqual(w.y, 8 + RADIUS)
            self.assertLessEqual(w.y, 72 - RADIUS)

    def test_jump_lands_and_held_key_does_not_autojump(self):
        w = World(ROOM)
        heights = []
        for _ in range(240):
            w.step(jump=True)
            heights.append(w.height)
        self.assertGreater(max(heights), 4)
        self.assertEqual(w.height, 0)
        self.assertEqual(sum(a == 0 and b > 0 for a, b in zip([0] + heights, heights)), 1)
        w.step(jump=False)
        w.step(jump=True)
        self.assertGreater(w.height, 0)

    def test_solid_stops_motion_but_allows_sliding(self):
        w = World(ROOM)
        w.x, w.y = 40, 40
        w.solids = [Solid(43, 8, 45, 72)]
        for _ in range(120):
            w.step(1, 1)
        self.assertLessEqual(w.x + RADIUS, 43)
        self.assertLess(w.y, 32)
        self.assertFalse(w.solids[0].overlaps(w.x, w.y, w.height))

    def test_projection_and_sprite_directions(self):
        self.assertEqual(project(40, 64), (80, 79))
        self.assertEqual(project(40, 64, 5), (80, 69))
        # Cardinal controls follow floor edges, with four distinct orientations.
        directions = [(1, 0, 1, -1, True, True), (-1, 0, -1, 1, False, True),
                      (0, -1, -1, -1, True, False), (0, 1, 1, 1, False, False)]
        for h, v, sx, sy, rear, mirror in directions:
            with self.subTest(horizontal=h, vertical=v):
                w = World(ROOM)
                before = project(w.x, w.y)
                w.step(h, v)
                after = project(w.x, w.y)
                self.assertGreater((after[0] - before[0]) * sx, 0)
                self.assertGreater((after[1] - before[1]) * sy, 0)
                self.assertAlmostEqual(abs(after[0] - before[0]), 2 * abs(after[1] - before[1]))
                pose, reflected = w.pose()
                self.assertIn(pose, (3, 4) if rear else (0, 1, 2))
                self.assertEqual(reflected, mirror)

    def test_walking_never_uses_falling_pose(self):
        for h, v, expected in ((0, -1, {3, 4}), (1, 0, {3, 4}),
                               (0, 1, {0, 1, 2}), (-1, 0, {0, 1, 2})):
            w = World(ROOM)
            seen = set()
            for _ in range(60):
                w.step(h, v)
                seen.add(w.pose()[0])
            self.assertEqual(seen, expected)

    def test_falling_pose_only_on_descent_and_returns_to_walk(self):
        for h, v, falling in ((0, -1, 6), (0, 1, 5), (1, 0, 6), (-1, 0, 5)):
            with self.subTest(horizontal=h, vertical=v):
                w = World(ROOM)
                w.step(h, v, jump=True)
                ascent = 4 if falling == 6 else 1
                self.assertEqual(w.pose()[0], ascent)
                # Releasing movement mid-jump must keep a raised-leg pose.
                w.step()
                self.assertEqual(w.pose()[0], ascent)
                seen_descent = False
                for _ in range(150):
                    w.step(h, v)
                    if w.height > 0 and w.vertical_speed < 0:
                        seen_descent = True
                        self.assertEqual(w.pose()[0], falling)
                    elif w.height > 0:
                        self.assertEqual(w.pose()[0], ascent)
                    else:
                        self.assertNotIn(w.pose()[0], (5, 6))
                self.assertTrue(seen_descent)
                self.assertTrue(w.grounded)

    def test_standing_jump_uses_active_pose_and_preserves_facing(self):
        for facing, ascent, mirrored in (((0, 1), 4, False), ((1, 0), 4, True),
                                          ((0, -1), 1, False), ((-1, 0), 1, True)):
            with self.subTest(facing=facing):
                w = World(ROOM)
                w.facing = facing
                idle = w.pose()[0]
                w.step(jump=True)
                self.assertFalse(w.moving)
                self.assertNotEqual(w.pose()[0], idle)
                self.assertEqual(w.pose(), (ascent, mirrored))


@unittest.skipUnless((HERE / 'assets/room.json').exists(), 'Run prepare.py for asset tests')
class GameTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import pygame
        pygame.display.init()
        pygame.display.set_mode((320, 240))
        pygame.font.init()
        cls.pygame = pygame
        from game import Game
        cls.Game = Game

    @classmethod
    def tearDownClass(cls):
        cls.pygame.quit()

    def test_update_is_independent_of_render_rate(self):
        positions = []
        for fps in (30, 60, 144):
            game = self.Game()
            for _ in range(fps):
                game.update(1 / fps, 1, 0, True)
            positions.append((game.world.x, game.world.y, game.world.height))
        for position in positions[1:]:
            for a, b in zip(positions[0], position):
                self.assertAlmostEqual(a, b, places=7)

    def test_static_scene_matches_original_tape_pixels(self):
        game = self.Game()
        game.enhanced = False
        # Compare scenery independently of the original engine's moving,
        # separately animated half-sprites. The extractor records every actually
        # painted hero pixel; it does not alter the original program.
        transparent = self.pygame.Surface((24, 32), self.pygame.SRCALPHA)
        for key in game.heroes:
            game.heroes[key] = transparent
        hero_mask = self.pygame.image.load(HERE / 'assets/reference-hero-mask.png')
        expected = self.pygame.image.load(HERE / 'assets/reference-original.png')
        actual = game.render_room()
        # HUD equipment at the bottom-right belongs to the original UI, not room.
        mismatches = []
        checked = 0
        for y in range(136):
            for x in range(232):
                if expected.get_at((x, y)).a == 255 and hero_mask.get_at((x, y)).a == 0:
                    checked += 1
                    if expected.get_at((x, y))[:3] != actual.get_at((x, y))[:3]:
                        mismatches.append((x, y))
        self.assertGreater(checked, 10000)
        self.assertEqual(mismatches, [], f'{len(mismatches)} incorrect source pixels; first {mismatches[:10]}')

    def test_colour_toggle_reset_and_short_jump_event(self):
        pg = self.pygame
        game = self.Game()
        game.handle_event(pg.event.Event(pg.KEYDOWN, key=pg.K_c))
        self.assertFalse(game.enhanced)
        game.handle_event(pg.event.Event(pg.KEYDOWN, key=pg.K_SPACE))
        game.update(STEP / 2)
        self.assertEqual(game.world.height, 0)
        game.update(STEP / 2)
        self.assertGreater(game.world.height, 0)
        game.update(0.1, 0, 1)
        game.handle_event(pg.event.Event(pg.KEYDOWN, key=pg.K_r))
        self.assertEqual((game.world.x, game.world.y, game.world.height), (40, 64, 0))

    def test_opaque_paper_and_column_occlusion(self):
        game = self.Game()
        pg = self.pygame
        # Use a distinctive full hero rectangle to identify painter order without
        # depending on a particular pose's holes. Exercise either side of a column.
        marker = pg.Surface((24, 32), pg.SRCALPHA)
        marker.fill((255, 0, 255, 255))
        for key in game.heroes:
            game.heroes[key] = marker
        column = next(o for o in game.room['objects'] if o['x'] == 52 and o['y'] == 36)
        mask = game.layers[True, column['name']]
        found = False
        for x in range(256):
            for y in range(120):
                colour = mask.get_at((x, y))
                if colour.a != 255 or colour[:3] != (0x39, 0x42, 0x4b):
                    continue
                # Put the marker on this same pixel from either depth side.
                for depth, visible in ((90, False), (86, True)):
                    w = game.world
                    w.x = (depth + (x - 128) / 2) / 2
                    w.y = depth - w.x
                    w.height = (183 - depth - (y + 10)) / 2
                    actual = game.render_room().get_at((x, y))[:3]
                    self.assertEqual(actual == (255, 0, 255), visible)
                found = True
                break
            if found:
                break
        self.assertTrue(found, 'No opaque column PAPER pixel tested')


if __name__ == '__main__':
    unittest.main()
