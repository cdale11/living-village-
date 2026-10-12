# Work, cooperation, exchange and technology

## Occupations from actual work

Seven bounded practice scores record successful foraging, woodcutting, stone
collection, farming, building, caregiving and delivery. Gathering earns one point
per real resource; harvesting/delivery earns the number moved; building earns
half a point per second worked; care earns one point per supplied meal.
Failed actions earn nothing. Scores cap at 100 and decay 2% per simulated day.

A resident becomes a specialist at eight points. A new specialty must exceed the
current specialty by 35%, and a current specialty is retained until its practice
falls below four. This avoids rapidly alternating labels. Role changes come
from work and are recorded in history. Practice adds up to six utility points to
matching work; learned skills and the personal MLP also affect decisions.
No person is locked into a job, and survival can override specialization.
Children can earn age-appropriate practice; newborns start without work history.

## Useful social behavior

Adults can seek a nearby hungry resident when they have spare carried meals.
During a completed interaction within two tiles, a donor with more than one meal,
hunger below 70 and sufficient cooperation/trust gives one meal to a hungry
resident with inventory space. The recipient must still eat it through the normal
food action. Sharing improves relationships and gives actual learning feedback.

Two nearby residents can barter **two surplus meals for one carried wood/stone**
when one lacks food and the village needs materials. Both inventories and carrying
limits are checked. Materials are then used through normal transport/construction;
there is no external market, currency, resource creation or instantaneous depot
transfer. The exchange rate is fixed for this small release.

A hungry resident with low cooperation may resent another nearby harvester when
a resource is depleted and that person has obtained some of it. This lowers mutual
trust, increases loneliness, records the claim dispute and is bounded to once per
resident per day. There is no scripted conflict scene or combat. Healthy,
cooperative residents can seek reconciliation; successful nearby interactions
repair trust. Ordinary socialization and bereavement continue to affect relationships.

## Workshop and two technologies

The sixth construction recipe is a **workshop: 12 wood, 8 stone, 28 work**. It is
built through the same physical deliveries and construction as other buildings.
When food/housing are adequate and communal knowledge is sufficient, automatic
planning can request one. Player plans remain available through A.

Successful practice adds `0.02 * work units` communal knowledge, capped at 30.
A completed workshop offers a technology project when knowledge reaches a
threshold. Workers gather/withdraw supplies, carry them to the workshop, then
work there. Resources and time are required for every upgrade:

| Upgrade | Knowledge threshold | Wood | Stone | Work | Effect |
| --- | ---: | ---: | ---: | ---: | --- |
| Improved hand tools | 8 | 4 | 2 | 20 | Gathering and construction 10% faster |
| Farm irrigation | 18 | 6 | 4 | 30 | Farm crop growth 15% faster |

Knowledge is a prerequisite, not consumed inventory. Upgrade supplies are
consumed on completion. Multiple workshops cannot duplicate the communal upgrade;
other sites' staged materials become recoverable ground supplies. Progression is
limited to these two technologies and does not unlock an unbounded tech tree.

## History and exports

The quick event log holds 24 entries. Saves also retain the latest 512 meaningful
chronicle entries and 128 daily snapshots. Snapshots contain population, children,
food/wood/stone, completed buildings, technology and cumulative birth/death/trade
counts. This is bounded recent history, not an unlimited lifetime journal.

**Y → economy/history → A** exports:

- `saves/exports/village-history.csv`
- `saves/exports/village-statistics.csv`
- `saves/exports/residents.csv`

Files include the world seed, use escaped CSV text, and are each flushed and
atomically replaced. Re-exporting replaces the previous export; copy exports
elsewhere to preserve separate reports. Exports describe retained records and
residents, including up to 32 deceased residents. Old recent logs are imported
with an explicit `Imported log:` prefix at the upgrade tick; earlier chronological
records and historical occupation data are not invented.

Save version 5 retains all these systems, replay, exploration state and
sound/rumble preferences. Versions 1–4 upgrade automatically; version 4 trained
weights and pending experiences remain intact. Back up the whole saves directory
before upgrading. Older executables cannot load version 5.
