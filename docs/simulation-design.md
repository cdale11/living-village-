# Village development simulation

Villagers have names/IDs, ages, hunger, energy, health, curiosity, cooperation,
loneliness, food/wood/stone inventories, gathering/building/farming skills,
relationships, goals, paths, decision utility scores/reasons, resource memories
and six recent significant experiences. Households, inherited traits, child care,
ageing and migration are documented in [population rules](population.md).

## Decisions and cooperation

Utility selection and action execution are separate. Decisions are reevaluated
on arrival, after completed actions and approximately every five seconds,
staggered by ID. Movement runs at 1.6 tiles per simulated second; each update
carries leftover movement across tile boundaries using a bounded loop. Water
blocks navigation; trees, stone, crops and buildings remain walkable.

Candidates use current needs, shared stock, known resources, skills and
cooperation. Examples:

- Explore: `25 + curiosity * 15`.
- Forage: `(45 if no carried food) + hunger * 0.5 + (25 if shared food < 12)`.
- Eat: `hunger * 1.5`.
- Rest: `(100 - energy) * 1.6`, with an extra preference for available shelter.
- Deliver site materials: `90 + cooperation * 35 + affinity with initiator * 0.1`.
- Collect shared construction supplies: `65 + cooperation * 30`.
- Gather for construction: approximately `58 + cooperation * 30 + gathering skill * 8`.
- Build: `58 + cooperation * 30 + building skill * 15`.
- Socialize: `loneliness * 0.8 + cooperation * 15 + affinity * 0.05`.

Hunger 75+ adds 55 to food acquisition/eating scores. At hunger 85+, an agent
carrying a meal can interrupt a long trip to eat. The inspector shows the
actual winning score and reason; its detail pages show skills, personality,
relationships and recent experiences. Scores are not fixed daily schedules. A personal online-trained MLP adds a
bounded learned adjustment to each valid candidate, using actual outcomes. See
[neural learning](neural-learning.md) for architecture and reward rules.

Unfinished building plans expose actual wood/stone deficits to all villagers.
Agents choose to collect known resources, withdraw available materials at camp,
deliver their own inventory to the site, or contribute work once supplied.
The first interested worker is recorded as the project initiator. Useful
material deliveries improve mutual relationships with that villager and record
experiences. Construction records the IDs of contributors. No scene scripts
or predetermined helpers are used.

A person carries up to four food and six combined wood/stone. Gathering checks
availability at execution, preventing two agents harvesting the same unit.
Deliveries subtract from inventory and add only the missing amount at the
actual site, retaining extra supplies. Gathering takes `2 / (1 + skill)`
seconds per unit; skills improve through completed gathering/building/harvest.
Excess food is carried back to camp, retaining one personal meal. Shared
materials are physically collected at camp before transport to a site.

## Buildings and player interventions

| Building | Wood | Stone | Work units | Effect when complete |
| --- | ---: | ---: | ---: | --- |
| Shelter | 12 | 4 | 24 | Four resting places; faster energy recovery |
| Food storage | 10 | 2 | 18 | Adds 40 to central food capacity |
| Stockpile | 8 | 4 | 16 | Adds 20 to each central wood/stone capacity |
| Farm plot | 6 | 2 | 12 | Renewable crops, harvested into villager inventory |
| Campfire | 4 | 2 | 8 | Fuelled warmth and better social comfort nearby |

Construction work cannot advance until all required resources arrive. Workers
contribute `0.75 + building skill` work units per second. Completion consumes
the delivered materials; the site's delivered counters then record investment,
not available inventory. Each new village starts with a small stockpile and
campfire, 18 food, no stored materials and no completed shelter.

Every five seconds, shared needs can request a plan: missing shelter capacity,
farms during low shared food, or food storage when existing capacity is nearly
full. At most two automatically requested projects are pending at once. Plans
use reachable clear land near camp. Farm demand scales with living population;
an abundant berry settlement may prefer storage rather than crops. The player
can disable automatic *new* plans in the overview; existing plans still receive
agent work. When population growth is enabled and food is sufficient, shelter
planning anticipates up to four extra residents, capped at 20.

Player plans can be placed on reachable empty grass using A at the camera
reticle. Up to 64 buildings/plans are supported. A on a tree, bush or stone tile
can designate/cancel a 5×5 gathering area. Up to 16 areas publish resource
locations to the village and increase gathering utility while shared stock has
space. Survival and other competing goals still participate in selection.

Storage buildings expand the shared depot represented at camp. Individual
warehouses do not yet maintain separate inventories; all shared pickup/deposit
happens at camp. Site delivery, resource harvesting and crop transport remain
physical trips. Completed shelters are used as rest destinations, with at most
four agents assigned to each shelter at a time.

## Survival, ecology and society

Adult hunger rises by 0.12 and energy falls by 0.065 per second. Children
have lower rates, described in the population rules. A one-second eating
action consumes one food and reduces hunger by 45. Rest recovers 1.8 energy per
second at camp, 3.2 inside completed shelter or 0.8 elsewhere, with 0.3 extra
near a fuelled fire. Hunger 99+ reduces health by 0.25 per second; otherwise
health recovers by 0.02. Death is persistent and logged. Severe shortages can
collapse the settlement.

Bushes hold six food and regrow one every 80 seconds. Trees hold six wood and
regrow one every 240 seconds; depleted trees display as stumps. Stone deposits
hold eight units and do not regenerate. Farms hold twelve ripe food and grow
two every 45 seconds, requiring a three-second harvesting action modified by
farming skill. Crop and wild plant growth both use seasonal/weather rates.

A day is 240 seconds (2400 fixed steps). Seasons each last ten days; growth
multipliers are spring 1.0, summer 1.2, autumn 0.9, winter 0.5. Deterministic
seed/day weather makes roughly two days in seven rainy; rain multiplies growth
by 1.15. Day/night tint remains independent of agent work schedules. Villagers
collect fuel at camp or use carried wood and physically deliver it to a low
campfire. Refuelling consumes one carried wood and adds 240 seconds of fuel.
No fuel means no fire comfort.

Loneliness rises slowly. Nearby villagers can socialize based on loneliness,
cooperation and existing affinity. A successful interaction lowers both
villagers' loneliness and improves both relationships; a fuelled campfire adds
comfort. Crossing a friendship threshold logs a real relationship event.
Relationships are bounded and each villager keeps six significant experiences.

## Persistence and bounds

Version 6 saves record all resources, buildings, invested materials, progress,
contributors, crops, fuel, gathering areas, skills, relationships, experiences,
goals/targets, paths, memories, settings and PRNG state. Version 1 saves upgrade
on load, preserving old villagers/time/inventories/terrain/goals/PRNG and adding
new resource quantities to existing trees/stone, plus the camp structures.
Version 2 saves preserve village development state and acquire stable identity
and household metadata. Version 3 introduced family state, growth settings,
birth/arrival/death counters and recoverable supplies left at death locations.
Version 4 adds neural weights and in-flight learning experience; version 3
worlds upgrade with fresh per-resident models. The existing checksum, atomic rename/flush and validated backup remain.

Memories cap at 48 locations per resource per villager; shared events at 24;
buildings at 64; gathering areas at 16; population at 20 living plus 32 archived deceased.
Tests cover deterministic continuation, version 1 migration, resource budgets,
real cooperative delivery, unsupplied construction, building effects,
designations, seasons, controller planning, 20 active villagers and 300 days
of six-agent survival. Cross-architecture floating-point histories are not
promised to be identical. There is no powered-off progression or game network.

Version 5 retains replay, local neural RNG, occupations, technology, staged
upgrade supplies, social counters, bounded chronicle/statistics and feedback
settings. The workshop and social/economy rules are in [society.md](society.md).

Version 6 adds named-world and generation metadata without regenerating old
terrain. Climate multipliers and resource distributions for the four biomes,
and water/mineral geometry for the five layouts, are documented in
[world-management.md](world-management.md).
