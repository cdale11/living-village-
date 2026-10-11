import unittest
from living_village.simulation import World, SIZE


class SmokeTest(unittest.TestCase):
    def test_headless_determinism(self):
        a, b = World(42), World(42)
        for _ in range(600):
            a.update(.1)
            b.update(.1)
        self.assertEqual(a.tiles, b.tiles)
        self.assertEqual(a.villagers, b.villagers)
        self.assertEqual(len(a.villagers), 6)
        for person in a.villagers:
            self.assertTrue(0 <= person.x < SIZE and 0 <= person.y < SIZE)
