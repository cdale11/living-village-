# Simulation rules

Each villager has ID, name, age, position, curiosity, hunger, energy, health, inventory, goal, reason, path and limited resource memory. Age is initialized but does not advance yet. Relationships and construction are not implemented.

Utility scores choose among actions:

- Explore: `25 + curiosity * 15`.
- Forage: `45 if no carried food + hunger * 0.5 + 25 if camp food < 12`, when known berries exist and inventory has space.
- Eat: `hunger * 1.5`, when food is carried or available at camp.
- Rest: `(100 - energy) * 1.6`.

Choices are evaluated on arrival and roughly every five seconds, staggered by ID. Decisions set a path and reason. Execution separately moves, eats, gathers or rests. Berry locations are discovered within five tiles and retained up to 48 locations. The inspector shows the real latest decision reason. There are no fixed work schedules.

Villagers move at 1.6 tiles per simulated second; routes avoid water. Trees, stone and bushes are walkable in this release. Hunger rises by 0.12 per second and energy falls by 0.065. Eating takes one second, consumes one food and reduces hunger by 45. Gathering takes two seconds per unit, checking availability at execution to prevent duplication. Carry capacity is four. Rest increases energy by 2.5 per second at camp, or 0.8 elsewhere. On arrival at camp, excess carried food may enter shared stock, retaining one unit and stopping at stock 30. This is opportunistic transport, not a complete cooperative task/request system.

Bushes hold at most six food, regrowing one every 80 simulated seconds when below capacity. Renewable berries permit survival under seeded starting conditions. Hunger 99+ reduces health by 0.25 per second; otherwise health recovers by 0.02. Health zero causes death and an event. Exhausting all food can collapse the village.

A day is 240 simulated seconds (2400 steps). A sine curve controls the night tint, without imposing agent schedules. Weather and seasons are not modeled yet. Tests cover 300 days with six survivors and an exhausted-food world that collapses.

Determinism applies within an equivalent build/execution environment, seed, fixed steps and actions. Saved PRNG state enables identical continuation. Floating-point differences across architectures/libraries are not promised to produce identical histories.
