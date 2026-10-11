"""Pygame presentation for the Milestone A village."""
import argparse
import sys
from .simulation import World, SIZE

COLORS = ((83, 139, 79), (38, 91, 45), (119, 119, 115), (53, 104, 158), (61, 119, 57))


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fullscreen', action='store_true')
    parser.add_argument('--seed', type=int, default=20261011)
    args = parser.parse_args()
    try:
        import pygame
    except ImportError:
        print('Pygame is required for graphics. Run the headless test without it.', file=sys.stderr)
        return 2
    pygame.init()
    screen = pygame.display.set_mode((720, 480), pygame.FULLSCREEN if args.fullscreen else pygame.RESIZABLE)
    pygame.display.set_caption('Living Village')
    font = pygame.font.Font(None, 24)
    clock = pygame.time.Clock()
    pads = []
    if pygame.joystick.get_init():
        for i in range(pygame.joystick.get_count()):
            pad = pygame.joystick.Joystick(i)
            pad.init()
            pads.append(pad)
    world = World(args.seed)
    cx = cy = 32.0
    tile_size = 24
    paused = False
    running = True
    accumulator = 0.0
    while running:
        elapsed = min(clock.tick(30) / 1000, .1)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                elif event.key in (pygame.K_RETURN, pygame.K_SPACE):
                    paused = not paused
                elif event.key in (pygame.K_EQUALS, pygame.K_PLUS):
                    tile_size = min(48, tile_size + 4)
                elif event.key == pygame.K_MINUS:
                    tile_size = max(12, tile_size - 4)
            elif event.type == pygame.JOYBUTTONDOWN:
                if event.button == 7:
                    paused = not paused
                elif event.button == 4:
                    tile_size = max(12, tile_size - 4)
                elif event.button == 5:
                    tile_size = min(48, tile_size + 4)
            elif event.type == pygame.JOYDEVICEADDED:
                pad = pygame.joystick.Joystick(event.device_index)
                pad.init()
                pads.append(pad)
            elif event.type == pygame.JOYDEVICEREMOVED:
                pads = [pad for pad in pads if pad.get_instance_id() != event.instance_id]
        keys = pygame.key.get_pressed()
        dx = int(keys[pygame.K_RIGHT] or keys[pygame.K_d]) - int(keys[pygame.K_LEFT] or keys[pygame.K_a])
        dy = int(keys[pygame.K_DOWN] or keys[pygame.K_s]) - int(keys[pygame.K_UP] or keys[pygame.K_w])
        for pad in pads:
            if pad.get_numhats():
                hx, hy = pad.get_hat(0)
                dx += hx
                dy -= hy
            if pad.get_numaxes() >= 2:
                ax, ay = pad.get_axis(0), pad.get_axis(1)
                dx += (ax > .5) - (ax < -.5)
                dy += (ay > .5) - (ay < -.5)
        cx = max(0, min(SIZE - 1, cx + dx * elapsed * 9))
        cy = max(0, min(SIZE - 1, cy + dy * elapsed * 9))
        if not paused:
            accumulator += elapsed
            while accumulator >= .1:
                world.update(.1)
                accumulator -= .1
        screen.fill((17, 32, 38))
        sw, sh = screen.get_size()
        left = int(cx - sw / (2 * tile_size)) - 1
        top = int(cy - sh / (2 * tile_size)) - 1
        for y in range(max(0, top), min(SIZE, top + sh // tile_size + 4)):
            for x in range(max(0, left), min(SIZE, left + sw // tile_size + 4)):
                px = int(sw / 2 + (x - cx) * tile_size)
                py = int(sh / 2 + (y - cy) * tile_size)
                tile = world.tiles[y][x]
                pygame.draw.rect(screen, COLORS[tile], (px, py, tile_size + 1, tile_size + 1))
                if tile in (1, 2):
                    pygame.draw.circle(screen, (23, 63, 37) if tile == 1 else (178, 176, 162),
                                       (px + tile_size // 2, py + tile_size // 2), tile_size // (3 if tile == 1 else 5))
        for person in world.villagers:
            px = int(sw / 2 + (person.x - cx + .5) * tile_size)
            py = int(sh / 2 + (person.y - cy + .5) * tile_size)
            pygame.draw.circle(screen, (37, 35, 36), (px + 1, py + 2), max(4, tile_size // 4 + 1))
            pygame.draw.circle(screen, (245, 204, 120), (px, py), max(4, tile_size // 4))
        pygame.draw.rect(screen, (17, 27, 28), (0, 0, sw, 28))
        label = f'Living Village | {"PAUSED" if paused else "Day 1"} | villagers 6 | D-pad: move | Start: pause'
        screen.blit(font.render(label, True, (240, 238, 220)), (8, 5))
        pygame.display.flip()
    pygame.quit()
    return 0


if __name__ == '__main__':
    raise SystemExit(run())
