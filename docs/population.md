# Households and generations

This milestone adds ageing, household bonds, children, care, and immigration.
The population is limited to 20 living residents. Residents keep permanent IDs;
retiring a dead record never reuses its identity. Up to 32 recent deceased
residents remain available to history, and parent-name snapshots survive older
records being retired. Construction keeps stable contributor IDs and an owner
name, so an old project is not attributed to a new resident.

## Household formation

Successful social interactions can form a household when two unrelated adults
are at least 18, both have mutual affinity of at least 45, both have family drive
at least 0.3, and a completed shelter can hold them. Existing partnerships are
exclusive until a partner dies. Parent/child, sibling and close shared ancestry
within two generations prevent a reproductive partnership. There are no
scripted couples, gender requirements or preassigned families.

Unpartnered adults can prefer companionship with peers within ten years of
age; family drive modifies that utility. Household formation follows actual
social interaction and relationship state. Partners share a home and can move
into spare shelter together before raising children. Dependent children are
not abandoned to make room for another birth. Grown children may move to spare
homes. A shelter houses at most four assigned residents.

## Birth opportunities

Once per simulated day, the world considers actual eligible households, with
a seeded opportunity chance modified by the parents' family drives. A birth
requires both parents aged 18–45, health at least 75, hunger no greater than 50,
matching shelter/home, an available home space, spare village shelter capacity,
room under the population limit, and shared food of at least
`max(12, (population + 1) * 2)`. Parents have an 80-day spacing between births;
there is also a four-day village-wide birth interval. The initial opportunity
starts on day 8. These are simulation constraints, not promised event dates.

A birth consumes two shared food and eight energy from each parent. The newborn
starts in the family home with no resources. Cooperation, curiosity and family
drive inherit the parents' average with small seeded variation; learned skills
start independently. Both parents and the child gain family links and affinity.
The birth is logged and recorded as a real experience. Pregnancy/gestation,
biological sex and physical reproduction are abstracted in this release.

## Childhood and care

Children under six stay at home. They rest, play or wait for care according to
energy and hunger. Ages six through fifteen can play near home, gather food,
harvest farms, eat, rest and socialize. Heavy material gathering, construction
and fuel delivery begin at sixteen. Play teaches a small amount of gathering
skill without reducing skills already learned through work.

Adults choose care from a child's actual hunger, their cooperative tendency,
parent role and available information. Parents know their child's needs;
others can help a child in their household or nearby. The adult uses carried
food or first withdraws a meal at camp, then physically walks to the child.
Feeding consumes one carried food, reduces hunger by 45 and improves the
child's trust. Execution checks current need, so two caregivers do not consume
two meals for an already satisfied child. Care competes with the caregiver's
own survival, work and social goals.

Young children gain 0.06 hunger per second; older children 0.09; adults 0.12.
Child energy declines more slowly. A village with many dependent children has
fewer workers and more care duties, so food and shelter still matter.

## Arrivals and housing

A healthy settlement with spare beds and at least
`max(12, (population + 1) * 3)` shared food can attract a newcomer. Hunger above
65 or health below 60 among current residents prevents new arrivals. The first
possible arrival is day 12, subsequent arrivals have a twelve-day interval,
and the seeded daily opportunity is one in four. Full or collapsed villages
receive no newcomers. The population inspector explains current readiness.

Newcomers enter at a reachable map edge and walk to camp. They bring no free
food/materials; their age, personality and modest experience are seeded. New
arrivals prefer shelter outside a family with paired residents when room is
available. They join ordinary utility-based work after reaching the village.

Automatic construction anticipates up to four spare beds while the village has
food reserves and growth enabled, up to the 20-resident limit. Housing and food
production therefore respond to growth rather than a scripted expansion scene.

## Ageing, death and bounded history

One game year is forty simulated days, matching the four-season cycle. Birthday
progress is saved. Children reach working age at sixteen. Old-age mortality
has a small daily chance after seventy and a maximum lifespan of ninety game
years. Hunger can kill at any age. The inspector and log show the actual cause.
Losing a partner or parent increases loneliness and records bereavement;
widowed residents can later form a new household.

On death, carried food/wood/stone move once into a loose supply cache on the last
tile. Nearby villagers can discover and physically recover those supplies.
Caches survive retirement of the old character record. Recent deaths have map
markers; older markers are hidden unless selected, keeping the village readable.
Individual memories, relationships (32 contacts), experiences and event logs
remain bounded. Contributor history is limited to 64 identities per building.

## Controls and saves

X opens villager inspection; left/right reaches the household/family page.
Y opens village overview; left/right reaches population and history. A on the
population page pauses/resumes *new* births and arrivals. Ageing, existing
children and care continue. Pausing the game stops all simulation time.

The pause menu includes New village with an explicit confirmation. The current
world is archived as `saves/previous-village.lvsave`. Switch previous village
swaps the saved world with the current one. Both retain normal atomic writes
and backups. Keep the whole saves folder when upgrading or synchronizing.

Version 5 saves retain demographics, stable IDs, homes, parents, names, partners,
age progress, death causes, loose supplies, sparse relationships, and growth
preferences. Versions 1–4 upgrade automatically with existing time, goals,
inventories, resource changes and PRNG intact. Existing ages are preserved and
start accumulating birthday progress after upgrade. Initial ancestry is unknown;
no family history is invented for old residents. Newly upgraded worlds delay
arrival opportunities rather than immediately adding a crowd. Old binaries
cannot read version 5 saves. There is no powered-off progression.

Each starting resident, newborn and arrival also receives an independent
[online MLP](neural-learning.md), whose weights and pending experience are saved.
