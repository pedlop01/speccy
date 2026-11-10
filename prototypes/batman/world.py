"""Native fixed-step movement and isometric projection, independent of Pygame/Z80."""
from dataclasses import dataclass
from math import hypot

STEP = 1 / 120
SPEED = 15.0
GRAVITY = 75.0
JUMP_SPEED = 28.0
RADIUS = 1.8
BODY_HEIGHT = 12.0


def project(x, y, height=0):
    """Original room's projection; positive height is above the base floor."""
    return 128 + 2 * (x - y), 183 - x - y - 2 * height


@dataclass(frozen=True)
class Solid:
    xmin: float
    ymin: float
    xmax: float
    ymax: float
    bottom: float = 0
    top: float = 32

    def overlaps(self, x, y, height):
        return (x + RADIUS > self.xmin and x - RADIUS < self.xmax
                and y + RADIUS > self.ymin and y - RADIUS < self.ymax
                and height < self.top and height + BODY_HEIGHT > self.bottom)


class World:
    def __init__(self, room):
        self.bounds = room['bounds']
        self.spawn = room['spawn']
        # Paired column drawings sit just outside the room edges. Treat their
        # feet as small solid footprints, not their tall screen-space rectangles.
        # Ceiling masonry (z<192) is decorative in this first-room prototype.
        self.solids = [Solid(o['x'] - 2, o['y'] - 2, o['x'] + 2, o['y'] + 2)
                       for o in room['objects'] if o['drawing'] in (10, 11)]
        self.reset()

    def reset(self):
        self.x, self.y = map(float, self.spawn)
        self.height = self.vertical_speed = self.walk_time = 0.0
        self.facing = (0, -1)
        self.moving = False
        self._jump_down = False

    @property
    def grounded(self):
        return self.height == 0

    def _free(self, x, y):
        return not any(s.overlaps(x, y, self.height) for s in self.solids)

    def step(self, horizontal=0, vertical=0, jump=False):
        """Room axes: right=+X (screen up-right), up=+Y (up-left)."""
        if jump and not self._jump_down and self.grounded:
            self.vertical_speed = JUMP_SPEED
        self._jump_down = jump
        self.height += self.vertical_speed * STEP
        self.vertical_speed -= GRAVITY * STEP
        if self.height <= 0:
            self.height = self.vertical_speed = 0.0
        dx, dy = horizontal, -vertical
        length = hypot(dx, dy)
        self.moving = bool(length)
        if not length:
            return
        self.facing = (dx, dy)
        dx, dy = dx / length * SPEED * STEP, dy / length * SPEED * STEP
        xmin, ymin, xmax, ymax = self.bounds
        x = max(xmin + RADIUS, min(xmax - RADIUS, self.x + dx))
        y = max(ymin + RADIUS, min(ymax - RADIUS, self.y + dy))
        old = self.x, self.y
        if self._free(x, self.y):
            self.x = x
        if self._free(self.x, y):
            self.y = y
        self.moving = old != (self.x, self.y)
        if self.moving:
            self.walk_time += STEP

    def pose(self):
        """Original walking tables; falling drawings are descent-only."""
        dx, dy = self.facing
        rear = dx + dy > 0
        # Canonical rear art faces the opposite horizontal direction to the
        # canonical front art; reflection must reverse for the rear poses.
        mirrored = dx > dy if rear else dx < dy
        # Drawings 15/16 are the front/rear falling poses, not walk frames.
        if not self.grounded and self.vertical_speed < 0:
            return (6 if rear else 5), mirrored
        # Use a lifted-leg stride on ascent, including a standing jump. There
        # is no separate ascent drawing in the extracted set; reserve the wide
        # cape falling drawings above for descent.
        if not self.grounded:
            return (4 if rear else 1), mirrored
        # On the floor the source tables at 8F3C/8F42 contain
        # 11,10,12,10 and 14,14,13,13.
        if not self.moving:
            return (3 if rear else 0), mirrored
        frame = int(self.walk_time * 9) % 4
        return ((4, 4, 3, 3) if rear else (1, 0, 2, 0))[frame], mirrored
