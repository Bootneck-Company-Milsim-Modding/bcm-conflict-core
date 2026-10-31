# M4.1 — Generic Map Definition Contract and Strategic Position Registry

Status: **PASS**

M4.1 adds the map-data/registry foundation only. It does not replace the M2 frontier, select objectives, replicate a graph, alter ownership, create deployment points, or implement the production Arland campaign.

## Implemented contract

`TB_CC_M4_1_MapDefinition` contains a stable map ID, schema version, debug/display name, strategic-position definitions, and undirected connection definitions. `TB_CC_M4_1_MapDefinitionCatalog` allows multiple definitions to coexist and rejects a duplicate map ID.

Each `TB_CC_M4_1_StrategicPositionDefinition` contains:

- map-scoped stable ID;
- semantic position type;
- authored world position and optional resolvable entity name;
- importance and optional region;
- copied strategic-trait strings;
- MOB candidacy and endpoint role;
- ownership-adapter type.

Supported semantic types are MOB, town, village, military base, outpost, depot, port, airfield, industrial, communications, infrastructure, crossroads, bridge, and other. Traits are extensible strings and have no gameplay effects in M4.1.

Definitions are builder inputs. The runtime registry copies its ordered ID view and exposes definitions and resolved positions only through getters. Mutable campaign state is not stored in the definition.

## Registry contract

`TB_CC_M4_1_StrategicPositionRegistry` provides:

- `Build`, `IsReady`, map ID/version/count, and deterministic validation failure;
- `FindPositionById`, `GetPositionIds`, and `GetPositions`;
- `GetConnections` with undirected adjacency;
- `GetMOBDefinitions` and `GetEndpointDefinitions`;
- position type, traits, authored position, importance, region, and metadata through the immutable definition getters;
- `TryGetCurrentOwner` through the resolved ownership adapter.

Ordering is lexicographic stable-ID order. Identity is never an array index, entity pointer, frontier index, or bit position.

## Ownership adapters

`TB_CC_M4_1_OwnershipAdapter` is the narrow read-only contract. `TB_CC_M4_1_VanillaBaseOwnershipAdapter` reads the faction from the existing `SCR_CampaignMilitaryBaseComponent`; it neither stores nor changes ownership.

`EXTERNAL_PROVIDER` reserves an explicit non-base path. Because no non-base authoritative provider exists in M4.1, selecting it fails closed with `external-provider-not-registered`. A future implementation can subclass the adapter without masquerading as a Conflict base or changing the definition identity model.

## Validation

The catalog rejects empty and duplicate map IDs. A runtime registry reaches READY only after validating schema 1, map identity/name, unique non-empty position IDs, non-negative importance, required entity resolution, vanilla component compatibility, ownership-adapter configuration, at least one MOB, unique endpoint roles, connection endpoints, prohibited self-edges, and duplicate undirected edges.

Failure clears all resolved positions and adjacency. Diagnostics consistently contain map, position, field, and problem. The proof observed these expected failures:

- duplicate position: `map=INVALID_DUPLICATE position=DUP field=id problem=duplicate`;
- missing target: `map=INVALID_EDGE position=MISSING field=connection problem=missing-target`;
- unregistered non-base adapter: `map=INVALID_ADAPTER position=MOB field=ownership.adapter problem=external-provider-not-registered`.

## Arland portability fixture

The fixture world inherits the real vanilla `worlds/MP/CTI_Campaign_Arland.ent`. It places three deliberately minimal fixture positions backed by real vanilla `SCR_CampaignMilitaryBaseComponent` ownership semantics:

| Stable ID | Semantic type | Initial vanilla owner | Purpose |
|---|---|---|---|
| `ARLAND::FIXTURE_MOB` | MOB | US | MOB/BLUFOR endpoint and ownership proof |
| `ARLAND::FIXTURE_CENTRAL` | TOWN | USSR | type, traits, lookup, and two-edge adjacency proof |
| `ARLAND::FIXTURE_PORT` | PORT | USSR | OPFOR endpoint and logistics/infrastructure metadata proof |

The graph is `MOB — CENTRAL — PORT`. These are portability-fixture entities on the real Arland campaign world, not a claim that they are the final production survey or graph. M4.2 must replace fixture choices with a deliberate whole-island survey.

Runtime resource: `{A412000000000000}Missions/TB_CC_M4_1_ArlandFixture.conf`.

## Validation evidence

Final runtime log:

`TestProfiles/M4_1_ArlandFixture_2026-09-30_19-32-26/logs/logs_2026-09-30_19-32-26/console.log`

Evidence:

- Game scripts compiled and the server entered online game state at `19:32:45.317`.
- Registry READY: map `ARLAND_M4_1_FIXTURE`, schema 1, 3 positions, 2 edges, 1 MOB.
- Real Arland entity resolution and vanilla ownership read passed; MOB owner was `US`.
- Catalog duplicate-map-ID rejection passed.
- Duplicate-position, missing-target, and invalid-adapter fail-closed tests passed.
- Two independent reconstructions of a 40-position/39-edge definition both reached READY and produced equivalent stable-ID order.
- Completion reported `arlandReady=1 scale40=1 deterministic=1 ownershipRead=1 campaignMutation=0`.

Workbench resource processing was run with the installed base-game add-on directory supplied. It updated `resourceDatabase.rdb` at 19:32 and resolved the new world, prefab, and mission GUIDs. The earlier command lacking that base-game search path did not process resources and was discarded. Final runtime showed no BCM M4.1 script or resource error. A vanilla Workbench-only gizmo reference warning (`system/wbdata/Gizmo3D/sphere.xob`) appeared under headless ServerDiag; it did not prevent world initialization, online state, or the proof.

## Everon coexistence and deployment regression

No M1, M2, M3, Everon world/mission, deployment, faction, capture, or production-route file was modified. The generic registry has no production consumer and contains no Arland conditional. Arland-specific construction exists only in `TB_CC_M4_1_ArlandFixture` and its fixture resources.

The accepted Everon M3.7 local-interactive evidence remains in `TestProfiles/M3_7_LocalInteractive_2026-09-30_17-58-15`. It records the existing M2 revision/frontier transitions, M3.2 operation lifecycle, M3.6 garrison cleanup/recreation, and read-only M3.7 telemetry. M4.1 is additive and does not initialize in that campaign. MOB metadata is descriptive only and creates no spawn point; MOB-only deployment behavior is therefore unchanged.

## Acceptance A–O

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Schema-1 generic map definition implemented. |
| B | PASS | Stable map ID and duplicate-ID catalog validation. |
| C | PASS | Map-scoped IDs such as `ARLAND::FIXTURE_CENTRAL`; no index identity. |
| D | PASS | Extensible type enum, traits, region, position, and importance metadata. |
| E | PASS | Undirected edges authored and resolved by stable IDs. |
| F | PASS | Full-build validation; failures clear the registry and never expose READY. |
| G | PASS | Runtime read `US` through the vanilla-base ownership adapter without mutation. |
| H | PASS | Separate adapter contract and explicit unregistered external-provider path. |
| I | PASS | Runtime 40-position/39-edge proof; no bitmask or index identity. |
| J | PASS | Real Arland world loaded; three entities and ownership resolved through generic code. |
| K | PASS | Generic definition/registry files contain no Arland branch or conditional. |
| L | PASS | M1–M3 resources/code unchanged; accepted Everon runtime remains operational and registry is not wired into it. |
| M | PASS | Metadata-only MOB description; no deployment/spawn mutation or new production spawn. |
| N | PASS | Two runtime reconstructions produced equivalent sorted ID registries. |
| O | PASS | Workbench processing, real script compilation, resource load, online state, and runtime proof passed. |

## Known limitations and M4.2 handoff

- The Arland fixture is intentionally not a production campaign definition.
- No non-base ownership provider or capture mechanism is implemented.
- The registry is authority-local and not replicated.
- No strategic consumer has migrated to it.
- No frontier, objective selection, connectivity, ambient population, patrol, checkpoint, reinforcement, vehicle, persistence, or UI logic is included.

M4.2 should perform the production Arland whole-island survey: identify and document stable strategic IDs, actual position categories/traits, vanilla-backed versus future non-base candidates, regions, MOB/endpoint candidates, and a reviewed undirected adjacency graph. It should author that map data against this contract and validate it, without yet implementing campaign progression or migrating M2/M3 consumers.
