"""Deterministic display-independent prototype simulation."""
from dataclasses import dataclass
import random

SIZE = 64
NAMES = ('Mira', 'Arin', 'Tala', 'Bram', 'Neri', 'Oren')


@dataclass
class Villager:
    identifier: int
    name: str
    x: float
    y: float
    target_x: int
    target_y: int
    moving: bool = False


class World:
    def __init__(self, seed=20261011):
        self.seed = seed
        self.rng = random.Random(seed)
        self.tiles = []
        for y in range(SIZE):
            row = []
            for x in range(SIZE):
                roll = self.rng.random()
                if abs(x - 32) + abs(y - 32) < 8:
                    tile = 0
                elif roll < .045:
                    tile = 3  # water
                elif roll < .14:
                    tile = 1  # tree
                elif roll < .19:
                    tile = 2  # stone
                elif roll < .24:
                    tile = 4  # bush
                else:
                    tile = 0
                row.append(tile)
            self.tiles.append(row)
        self.villagers = []
        for i, name in enumerate(NAMES):
            x, y = 31 + i % 3, 31 + i // 3
            self.villagers.append(Villager(i, name, float(x), float(y), x, y))
        self.ticks = 0

    def passable(self, x, y):
        return 0 <= x < SIZE and 0 <= y < SIZE and self.tiles[y][x] != 3

    def update(self, dt):
        self.ticks += 1
        for person in self.villagers:
            if not person.moving:
                x, y = round(person.x), round(person.y)
                choices = [(x + dx, y + dy) for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1))]
                person.target_x, person.target_y = self.rng.choice([(a, b) for a, b in choices if self.passable(a, b)])
                person.moving = True
            dx, dy = person.target_x - person.x, person.target_y - person.y
            distance = abs(dx) + abs(dy)
            amount = min(distance, dt * 1.7)
            if distance:
                person.x += dx / distance * amount
                person.y += dy / distance * amount
            if distance <= dt * 1.7 + 1e-8:
                person.x, person.y = float(person.target_x), float(person.target_y)
                person.moving = False
