# Ecology, wildlife and environmental hazards

This is a deterministic, bounded ecological model for a small game world. It
models useful biological processes rather than cellular biology or a calibrated
real ecosystem. One simulated day is 240 seconds; biological rates and lifespans
are compressed for gameplay. The simulation has no external model dependency.

## Soil, plants and forests

Each land tile stores elevation, soil moisture, surface water, groundwater,
snow, nutrients, litter/organic matter, compaction, erosion, vegetation biomass,
plant age/health/disease, tree and shrub seeds, flower nectar, pollination and fire.
Trees and bushes retain their actual gatherable wood/fruit quantities.

Growth depends on soil moisture, temperature, fertility, health and compaction,
in addition to the existing biome/season multipliers. Dry soil, standing floods
and plant disease reduce health. Wet, dense vegetation develops disease;
improved conditions allow recovery. Trees shade their soil and deposit litter.
Litter decomposes into nutrients, harvest uses a little fertility, and grazing
removes actual biomass. This is a simplified nutrient cycle, not exact carbon or
nitrogen accounting. Plants use sunlight implicitly; no finite sunlight stock.

Mature trees disperse seeds to adjacent tiles daily. Healthy, pollinated shrubs
also disperse seeds; animal fruit consumption adds shrub seeds locally. Sufficient
seed accumulation, water, nutrients, open canopy and low compaction permit recruitment outside
the settlement clearing and building sites. New trees start as small seedlings;
deer browsing can damage them. Trees die from health loss or at 1,800 days of age;
remaining standing wood becomes a local loose cache, with litter returned to soil.
Depleted living trees coppice gradually, rather than instantly recreating wood.

## Weather and hydrology

Daily seeded weather chooses clear, rain, storm, snow or heatwave using biome and
season. Temperature and wind affect evaporation and wildlife. Six dry days marks
a drought; it changes growth through soil water rather than a scripted famine.

Rain enters surface water. Cold snow accumulates and later melts. Water infiltrates
soil more slowly under compacted paths, percolates to groundwater, and returns to
root-zone soil by capillary movement. Surface water flows to four adjacent tiles
according to elevation plus water depth. Groundwater diffuses between neighbors.
Two fixed transfer buffers apply these movements simultaneously and conserve water
within each pass. Permanent pond/river/sea tiles are explicit external reservoirs;
boundary drainage, reservoir drainage and capped overflow are counted as outflow.
Rain, evaporation and outflow counters explain the land water balance. Plant transpiration is included in evaporation, and animal drinking has a
separate water-used counter; the conservation regression
uses a laboratory world without wildlife. There is no river upstream catchment,
tide, hydraulic pressure solver or individual villager drinking requirement.

Flow erodes exposed ground, reduces elevation/fertility and changes visible wet
areas. Freeze/weather exposure weathers rock; high erosion can dislodge an actual
remaining stone into a loose cache, preserving material quantities. Walking adds
compaction and removes a little vegetation; soil recovers slowly. Worn trails give
a small movement benefit, while flooded tiles slow travel. Elevation changes never
turn a resident's tile into impassable deep water.

## Wildlife biology and learning

At most 48 wildlife entities coexist: rabbits, deer, foxes and bee colonies. The
initial world attempts to place 8/4/2/4 respectively outside the clearing, on land.
A bee entity represents mobile foragers from one colony, not a single bee or a
full queen/worker caste model. Colonies use the same simplified reproduction
framework as mammals; this abstraction is deliberately visible in the UI.

Each entity has an ID, maternal lineage, sex, age, body mass, hunger, thirst,
energy, health, body temperature, disease burden, inherited metabolism/immunity/
fertility, pregnancy and action state. Juveniles grow if fed. Metabolism consumes
energy/food/water; heat increases water needs, shade affects body temperature,
wet habitat and nearby infected animals raise disease exposure, and immunity supports recovery. Starvation,
dehydration, fire, illness, predation and species lifespan can cause death.
Carcasses return local organic matter and disappear; no magical village meat stock.

Rabbits graze and eat berries; deer browse plants and can damage young trees.
Foxes hunt actual nearby rabbits. Bees consume finite renewable nectar and improve
pollination, which helps fruit growth and shrub seeding. Drinking consumes puddle
water, plant moisture or water from a nearby permanent reservoir. Water seeking,
feeding, resting, fleeing, hunting and mating compete through need-based utilities.
Wildlife sees only a local four-tile habitat radius/seven-tile animal neighborhood,
uses bounded cardinal movement, and does not run whole-map path searches.

Healthy adults can mate when food/water needs are low and the population has room.
Parent/offspring and maternal sibling pairings are excluded; complete pedigrees
and paternal ancestry are not modeled. Gestation lasts 5/18/10/12 days for rabbits,
deer, foxes and colonies, with two rabbit offspring or one otherwise. Offspring
inherit both parents' biological traits with small mutations and some maternal
disease burden. Lifespans are 240/800/400/300 days. Breeding cooldowns and habitat
limit growth. Sparse recolonization occurs at suitable land boundaries, every
eight days when wildlife is low; islands do not spontaneously receive mammals.

Each animal has its own C++ 37→12→1 MLP, trained from changes in hunger, thirst,
energy and health. It adjusts valid utility candidates by at most 10 points and
keeps 32 bounded replay samples. The animal model uses seven animal action flags
and animal/habitat observations; weights are never mixed with villager models.
The village learning toggle freezes animal weights and replay too. Learned
preferences do not create new species, rewrite rules or bypass physical resources.

## Disasters and maintenance

Storm runoff can accumulate in low ground and flood vegetation, slow travel,
damage crops and structures, and harm residents. Dry fuel can ignite during storms
or extended drought, then fire consumes biomass, damages plants/buildings/animals
and spreads to dry neighboring vegetation. Wet soil and rain extinguish it. These
hazards arise from environmental state, with seeded lightning/ignition sampling;
there are no scheduled scripted disaster scenes. Earthquakes and landslides are
not part of this release.

Completed structures have persistent condition. Ordinary exposure, rain, storms,
floods, fire and shelter use cause wear. Below 85%, residents can gather, withdraw,
physically deliver and work on maintenance using their existing material/build
skills and learned goals. Each repair consumes one wood, requires eight work units and restores 25
condition. Severe structural damage below 50% also needs one stone, except for
farms/campfires. Routine patching uses renewable wood so ordinary upkeep does not
inevitably exhaust finite quarries; disasters can still make recovery costly.
Unsafe buildings at 20% stop supplying beds/storage; excess storage contents spill
into the actual communal ground cache. Farms and shelter rest efficiency scale
with condition. Buildings stay at stable IDs when ruined so families and saved
plans do not acquire invalid references; repairs can restore them.

## Inspection, saves and tests

Y → left/right reaches Ecology/weather and Wildlife after the sound page.
L1/R1 cycles animals on Wildlife. Inspect a tile with A for soil/plant/compaction
state or building condition/maintenance. Map sprites show wildlife, seedlings,
trails, standing water, snow and fires. Weather and meaningful ecology events
appear in the village log. Original procedural art uses no external assets.

Save version 7 includes all tile states, weather, ecology PRNG, counters, animal
biology, genetics, pregnancy, intentions, neural weights/replay and maintenance.
Versions 1–6 migrate by initializing ecology around the saved terrain while
preserving residents, existing neural models, buildings and resources. Saves are bounded to 8 MiB, including maximum populations, replay and routes. Older
executables cannot read version 7. Keep the whole saves folder when upgrading.

`make test` includes water balance/runoff, infiltration/traffic, snowmelt, drought,
forest recruitment/mortality, fire, grazing, finite nectar/pollination, predation,
pregnancy/inheritance/caps, repair costs, flooding, learning/freeze, deterministic
save continuation, maximum-sized save budgets and controller inspection. `make world-soak` surveys climates.
Ecology runs every ten fixed ticks (one second); plant/water updates scan 4,096
tiles, animals are capped, all history/replay remain bounded. Physical handheld
FPS/memory/battery results still require measurement.

Y’s existing CSV export also writes `ecology.csv` (a 4,096-tile habitat snapshot)
and `wildlife.csv` (living animals and their biology/intentions) alongside village
history in each world’s export directory. These are snapshots, not unbounded
time-series files.

## Desktop verification

The completed 3,500-day default-world regression retained 20 living residents,
four births, 26 arrivals, 16 deaths and full shared food storage. Residents
completed 2,210 repairs, and the founding generation was replaced. Routine
wood-based maintenance avoids a permanent demand on finite quarry stone.
This is one tested seed, not a guarantee against difficult climates or disasters.

Focused ecology tests pass with GCC and the same libc++ runtime used by the ARM64
build; address/undefined-behavior sanitizer checks also pass. Save tests include
full living/deceased/wildlife populations with maximum replay and route buffers.
Controller/rendering and save-management smoke tests pass. A short desktop
20-resident/wildlife run measured 30.19 FPS at the 30 FPS cap, with 8,704 KiB peak
process RSS. These figures do not establish physical handheld performance.

## Wildlife visibility and regional dispersal

New worlds place two rabbits and two deer near the initial camera in passable
habitat, with remaining wildlife distributed across the map. The header reports
the real animal count. Y → Wildlife → A follows an animal; enlarged original
sprites distinguish deer, rabbits, foxes and colonies. D-pad releases the camera.
Existing saves retain actual wildlife; zero animals is reported honestly.

Once per simulated day, under nonstorm/nonheatwave weather, a species below its
regional target (four herbivores/colonies, two foxes) has a one-in-four dispersal
opportunity. At most sixteen boundary habitats are checked per species. New
animals require passable land, healthy plants, moisture, no fire, and biomass
(herbivores), nectar (bees) or at least four prey animals (foxes). The total
population remains capped at 48. This models an external regional population;
an entirely unsuitable or water-only boundary cannot be recolonized this way.
Births and arrivals are distinct, and arrivals are logged from actual conditions.
