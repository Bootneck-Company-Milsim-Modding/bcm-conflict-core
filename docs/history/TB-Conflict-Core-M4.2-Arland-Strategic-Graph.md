# M4.2 — Arland Strategic Survey and Production Graph

Status: **PASS — amended by M4.2A multiple-MOB campaign-start contract**

M4.2A supersedes the fixed HQ East start described below. Both HQ East and HQ West are valid MOB candidates; the campaign seed or a validated explicit override chooses one, and the opposite HQ becomes its endpoint. See `TB-Conflict-Core-M4.2A-Campaign-Start-MOB-Selection.md`.

## Evidence and survey boundary

The survey used the real vanilla `worlds/MP/CTI_Campaign_Arland.ent` authority runtime, the campaign base manager, stable world entity names, authored coordinates, vanilla ownership components, and the terrain's known settlement/airbase context. Arland is 4096 m × 4096 m; the eight selected bases span northwest, northeast, central, and southern territory rather than a corridor.

Runtime inventory found 11 registered bases: eight vanilla Arland bases plus the three explicitly labelled M4.1 fixture bases. Fixture bases are excluded from production. The eight vanilla bases were `HQEast`, `HQWest`, `Farm`, `MossHill`, `BeauregardBase`, `ArlevilleBase`, `AirportNorth`, and `MilitarySouth`.

The vanilla scenario assigned both HQ factions differently across two clean launches. One run reported HQ East US/HQ West USSR; the final run reported HQ East USSR/HQ West US. The production definition therefore does not claim that vanilla's unmodified randomized start is the production initial state. It explicitly selects HQ East as the intended BLUFOR MOB and HQ West as the OPFOR terminal; the later production-scenario integration must establish that initial ownership deterministically through supported scenario configuration.

## Vanilla-base inventory and production classification

| Vanilla entity | Position | Vanilla role | Core ID/type | Importance | Traits | Decision |
|---|---:|---|---|---:|---|---|
| `HQEast` | 1622, 903 | HQ | `ARLAND::HQ_EAST`, MOB | 100 | LOGISTICS, SUPPLY, COMMUNICATIONS, MILITARY, TRANSPORT | Strategic; intended BLUFOR MOB/home endpoint |
| `MossHill` | 2293, 1224 | regular base/high ground | `ARLAND::MOSS_HILL`, OUTPOST | 75 | MILITARY, COMMUNICATIONS, INFRASTRUCTURE | Strategic southern observation/communications position |
| `MilitarySouth` | 1413, 1858 | military base | `ARLAND::MILITARY_SOUTH`, MILITARY_BASE | 85 | MILITARY, SUPPLY, TRANSPORT | Strategic military route anchor |
| `ArlevilleBase` | 2727, 1681 | settlement base | `ARLAND::ARLEVILLE`, TOWN | 80 | POPULATION_CENTER, TRANSPORT | Strategic major community and eastern route hub |
| `BeauregardBase` | 3297, 2860 | settlement base | `ARLAND::BEAUREGARD`, VILLAGE | 65 | POPULATION_CENTER, TRANSPORT | Strategic northeastern community/branch |
| `Farm` | 2209, 2909 | farm base | `ARLAND::FARM`, DEPOT | 60 | LOGISTICS, SUPPLY, TRANSPORT | Strategic north-central logistics junction |
| `AirportNorth` | 947, 2896 | airfield base | `ARLAND::AIRPORT_NORTH`, AIRFIELD | 85 | LOGISTICS, SUPPLY, TRANSPORT, INFRASTRUCTURE, MILITARY | Strategic aviation/logistics installation |
| `HQWest` | 1158, 3282 | HQ | `ARLAND::HQ_WEST`, MILITARY_BASE | 100 | LOGISTICS, SUPPLY, COMMUNICATIONS, MILITARY, TRANSPORT | Strategic OPFOR terminal/home endpoint |

All eight use `VANILLA_CONFLICT_BASE`. Runtime resolved every adapter and read an actual faction. No TB ownership state or capture system was introduced.

## Non-base and ambient assessment

The survey deliberately assessed six non-base categories present or relevant to the island: coastal quays/landing points, ordinary road junctions, minor crossings, isolated farm/utility compounds, additional hills/observation sites, and power/communications infrastructure. None is promoted to production strategic ownership in M4.2:

- existing vanilla bases already cover both communities, the airfield, the military installation, logistics/farm territory, high ground, and both campaign ends;
- no additional candidate has a verified stable runtime ownership provider;
- adding an unverified harbour, bridge, or utility node would create fictional capture semantics rather than improve this compact 6.46 km² terrain's strategic model.

These six categories remain **ambient/local candidates** for M4.6–M4.8. Minor buildings, ponds, shoreline names, and purely descriptive terrain features are currently **not relevant**. No authored production node is marked `FUTURE_NON_BASE_PROVIDER`; consequently M4.2 has no hidden non-capturable strategic requirement.

## MOB and endpoint

`ARLAND::HQ_EAST` is the sole authored MOB and BLUFOR home endpoint. It has direct access to two independent southern boundary positions—Military South and Moss Hill—plus existing HQ logistics/radio semantics. MOB metadata remains descriptive and creates no spawn.

`ARLAND::HQ_WEST` is the OPFOR terminal position. Victory will eventually mean a legal vanilla capture of this final authored HQ after M4.3 eligibility and later endpoint policy permit it. M4.2 does not authorize it, mutate it, or end a match.

## Undirected production graph

There are eight nodes and thirteen reviewed edges:

```text
                         HQ WEST (OPFOR END)
                         /                 \
                AIRPORT NORTH ----------- FARM -------- BEAUREGARD
                    |       \              |  \          /
                    |        \             |   \        /
              MILITARY SOUTH -------------+---- ARLEVILLE
                    |  \                         /
                    |   \                       /
                    +---- MOSS HILL -----------+
                       \       /
                        HQ EAST (BLUFOR MOB)
```

The exact machine-readable artifact is [M4.2-Arland-Strategic-Graph.dot](Artifacts/M4.2-Arland-Strategic-Graph.dot). Coordinates are embedded in the DOT nodes so it can be rendered with Graphviz using a position-preserving `neato` layout.

The graph contains two opening branches, several cross-links, and reconvergence around Farm/Arleville. It is not a predetermined route or fully connected graph. Every node is reachable from the MOB.

## Offline frontline sanity proof

The proof treats the listed friendly set as a synthetic ownership snapshot and never changes vanilla ownership.

| Phase | Friendly additions | Enemy boundary candidates |
|---|---|---|
| Initial | HQ East | Military South, Moss Hill |
| After Military South | + Military South | Airport North, Arleville, Farm, Moss Hill |
| After Moss Hill | + Moss Hill | Airport North, Arleville, Farm |
| After Arleville | + Arleville | Airport North, Beauregard, Farm |

This demonstrates branching and reconvergence. Beauregard cannot appear until Farm or Arleville is connected. HQ West cannot appear merely because importance is 100; it requires a connected captured neighbour at Airport North or Farm. Importance is metadata and is never consulted by the boundary derivation.

## Runtime and resource validation

Final evidence:

`TestProfiles/M4_2_ProductionGraph_2026-09-30_19-56-30/logs/logs_2026-09-30_19-56-30/console.log`

- Workbench processed the resource database against installed vanilla data.
- Game scripts compiled and the real Arland world entered online game state.
- `ARLAND_PRODUCTION_01` reached READY with 8 positions and 13 edges.
- All four synthetic candidate transitions passed.
- All eight real vanilla entity references resolved through the ownership adapter.
- Completion: `ready=1 positions=8 edges=13 connected=1 ownershipResolved=1 mutation=0`.

The read-only survey/proof does not alter ownership. Existing M1–M3/Everon resources were not modified; M2 remains the active Everon strategic implementation.

## Acceptance A–U

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Entire 4096 m island represented by eight distributed real strategic bases. |
| B | PASS | All eight vanilla bases inventoried; three fixture bases explicitly excluded. |
| C | PASS | Six non-base categories assessed; none promoted without a provider. |
| D | PASS | Eight strategic positions; six ambient categories; minor descriptive terrain excluded. |
| E | PASS | All use stable `ARLAND::` IDs. |
| F | PASS | Types correspond to actual HQ, airfield, communities, farm/logistics, military site, and high ground. |
| G | PASS | Importance and evidence-backed traits recorded above. |
| H | PASS | HQ East explicitly authored as the sole BLUFOR MOB. |
| I | PASS | HQ West explicitly authored as OPFOR home/terminal. |
| J | PASS | Thirteen reviewed undirected edges authored. |
| K | PASS | Two initial choices and later branch/reconvergence sets. |
| L | PASS | M4.1 validation reached READY; every edge resolved. |
| M | PASS | Runtime registry and graph traversal reach all eight nodes. |
| N | PASS | Initial candidates are Military South and Moss Hill. |
| O | PASS | Three subsequent capture-state transitions passed at runtime. |
| P | PASS | Boundary analysis ignores importance; deep nodes require adjacency. |
| Q | PASS | All eight vanilla ownership adapters resolved at runtime. |
| R | PASS | No non-base production node is falsely represented; future categories remain ambient. |
| S | PASS | Coordinate table, ASCII overview, adjacency explanation, and DOT artifact supplied. |
| T | PASS | Everon/M1–M3 resources and consumers unchanged. |
| U | PASS | Workbench resource processing, script compile, Arland load, online state, and proof passed. |

## Remaining uncertainties

### M4.4I expansion decision (2026-10-01)

The Creative Director approved four distinct capture locations and manually inspected/saved/processed the final entities. Production now contains ten ordinary positions plus two permanent non-capturable HQs and twenty-one edges. This supersedes the earlier ambient-only treatment of non-base candidates. The preceding eight-node tables/proofs describe historical M4.2 evidence. Current positions and architecture are recorded in `TB-Conflict-Core-M4.4I-Arland-Strategic-Expansion.md`; the DOT artifact reflects the expanded source. Final-dataset physical authority proofs and production startup PASS are recorded in `TB-Conflict-Core-M4.4I-Runtime-Acceptance.md`, including remaining overall M4.4 human criterion U and network N1–N5 debt. Retired authoring markers are not dependencies.

- Vanilla Arland HQ faction assignment varied between clean launches. Production integration must deterministically configure HQ East as US and HQ West as USSR before this becomes a playable campaign.
- M4.3 must decide endpoint eligibility; graph adjacency alone intentionally does not make importance or endpoint status an eligibility bypass.
- Road-level traversal and AI staging suitability remain runtime concerns for later objective/staging acceptance, not reasons to change strategic adjacency without evidence.
- Ambient candidate coordinates and density are intentionally deferred until the population-budget milestones.

## Exact M4.3 handoff

Implement a read-only, deterministic, eligibility-first selector over `ARLAND_PRODUCTION_01`: derive the MOB-connected friendly component, derive enemy boundary candidates, apply explicit endpoint/ownership/adapter eligibility, score only eligible candidates using authored metadata, use stable-ID tie-breaking, and publish one explainable selected objective. Add synthetic ownership snapshots and runtime proofs, but do not migrate M3, mutate vanilla ownership, add non-base capture, or implement ambient population.
