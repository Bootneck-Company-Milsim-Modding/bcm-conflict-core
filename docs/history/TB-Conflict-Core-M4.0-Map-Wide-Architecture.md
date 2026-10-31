# M4.0 — Map-Wide Conflict Core Architecture

> **M4.4H product amendment:** terminal HQ capture is cancelled. HQs are permanent, non-capturable faction anchors. Campaign victory is now derived from sustained control of the map definition's required ordinary strategic territory, with authoritative BLUFOR/OPFOR victory-pressure states before immutable terminal outcome publication. Endpoint metadata remains useful for start anchoring and future siege geometry; it no longer implies an ordinary capture target.

## Status

**ARCHITECTURE COMPLETE — M4.1 NOT IMPLEMENTED.**

M1 established the production foundation, M2 established the deterministic reference campaign, and M3 proved the autonomous combat loop with a real human player. M4 converts the campaign core from one ordered Everon route into a map-independent strategic graph while preserving the proven semantic boundary consumed by M3.

## 1. Product definition

Conflict Core is a reusable, server-authoritative campaign and AI-war framework. It owns strategic policy, selects one authorised objective at a time, coordinates physical OPFOR forces, and projects an explainable campaign state. It does not own terrain-specific geography.

The existing `EVERON_NORTH_SOUTH_01` topology is the permanent reference/regression campaign. It is not the production map model. “Sequential” means one authorised BLUFOR strategic capture at a time; it does not mean that positions form a geographic line.

The production experience is a dense occupied battlespace. Enemy strategic territory remains enemy-owned until genuinely captured through the supported vanilla Conflict path. Players may raid or traverse other areas, but only the authorised objective advances the campaign.

## 2. Arland pilot-map decision

Arland is the first production consumer of the map-wide architecture. Its smaller extent makes whole-island authoring, population measurement, end-to-end conquest and community testing practical. It is also the portability test: the same core must run the Arland definition while the Everon reference topology continues to run unchanged.

Roles are explicit:

- **Everon:** deterministic M1–M3 regression topology.
- **Arland:** first complete map-wide production campaign.
- **Conflict Core:** terrain-independent mechanics and semantic APIs.
- **Future map packages:** authored definitions consuming the same contract.

No Arland name, entity name, coordinate or campaign branch belongs in core decision logic.

## 3. Sequential campaign progression

At any strategic revision, Conflict Core publishes at most one authorised BLUFOR objective. The sequence is dynamic:

```text
current ownership
→ derive connected friendly territory
→ derive enemy boundary candidates
→ filter eligibility
→ score eligible candidates
→ deterministically select one objective
→ vanilla capture changes ownership
→ publish a new revision
→ repeat
```

The next objective is not necessarily geographically “after” the previous objective and is not selected from a fixed index. Deterministic tie-breaking makes a given state/configuration reproducible without turning the map into a route.

## 4. Strategic-position model

An authored strategic position definition should contain:

| Field | Purpose |
|---|---|
| `positionId` | Stable, map-scoped identity; never derived from display text or array order. |
| `mapId` | Definition ownership and validation namespace. |
| `displayName` | Presentation only. |
| `positionType` | Town, village, military base, port, airfield, depot, industrial, communications, infrastructure, outpost, compound, crossroads, bridge, or extensible custom type. |
| `worldReference` | Preferred stable authored entity reference/name where a world entity exists. |
| `position` | Authored/fallback strategic coordinate. |
| `captureAdapter` | How ownership and vanilla capture are observed for this position. |
| `strategicImportance` | Base selection weight, not an eligibility bypass. |
| `regionId` | Optional grouping for regional policy and diagnostics. |
| `traits` | Extensible data such as logistics, communications, transport, supply, military, infrastructure, coastal, bridge or endpoint. |
| `initialOwner` | Validation/default intent; runtime ownership still comes from the authoritative ownership source. |
| `mobCandidate` | Whether the position may be selected as the authored BLUFOR MOB. |
| `endpointRole` | Optional victory role; data rather than array-boundary convention. |

Definitions are immutable campaign geography. Runtime state—owner, strategic state, selected objective, last-captured revision and materialised population—is stored separately.

Vanilla Conflict bases are valid position adapters, but not the only possible strategic positions. A non-base position must still have an explicitly supported capture/ownership adapter; coordinates alone cannot become magical territory.

## 5. Campaign graph

The graph is an undirected adjacency network unless an edge is explicitly authored with directional policy. Nodes are strategic positions. Edges mean meaningful military/campaign adjacency, not merely nearest-neighbour distance.

An edge definition contains stable endpoint IDs and optional metadata such as movement class, crossing type, logistics relevance, blocked/default-enabled state or future traversal cost. Edge metadata must be additive; M4.1 needs only identity, endpoints and validation.

Graph validation requires:

- unique position IDs;
- no dangling edge endpoints;
- no self-edges or duplicate equivalent edges;
- at least one valid MOB;
- all campaign-required positions reachable under the initial graph policy;
- explicit endpoint/victory semantics;
- resolvable ownership adapters;
- deterministic ordering for diagnostics and tie-breaking.

No single frontier integer can represent graph state.

## 6. Strategic frontline definition

Friendly strategic territory is the set of friendly-owned nodes connected through friendly-owned nodes to the active BLUFOR MOB. This prevents an isolated friendly pocket or invalid ownership mutation from automatically becoming an advance origin.

A frontline edge connects a node in connected friendly territory to an enemy-owned adjacent node. The enemy endpoint of such an edge is a BLUFOR frontline candidate. A friendly node adjacent to enemy territory is a friendly boundary/defensive position.

Disconnected ownership pockets remain visible in validity diagnostics but do not grant campaign reach. Policy must explicitly decide whether a pocket is legal, invalid, or merely non-projecting; M4.2 must choose and test that rule.

## 7. Candidate eligibility

Eligibility precedes scoring. An enemy position is eligible only when all required rules pass:

1. Campaign is READY, valid and ACTIVE.
2. Position exists and has a functioning capture/ownership adapter.
3. Position is enemy-owned.
4. At least one edge connects it to connected friendly territory.
5. Position is not disabled by authored campaign policy.
6. Capturing faction is allowed to capture it.
7. Endpoint restrictions, if any, are satisfied.

A high score cannot make an ineligible deep node selectable. All rejection reasons should be enumerable and logged so objective choice remains explainable.

## 8. Strategic objective selection

Selection operates only on the eligible frontier set. The first production selector should be deliberately small and deterministic:

```text
score = strategic importance
      + authored type/trait weights
      + connectivity/unlocking value
      + optional regional context
```

M4.3 should initially implement only the minimum accepted factors and stable tie-breaking by `positionId`. It should publish a score breakdown and rejection/selection reason. Logistics, communications, supply and regional effects remain data hooks until separately accepted.

Selection is server-authoritative. Clients receive the selected ID and semantic state, not authority to rerun policy.

## 9. Territory model

Territory is derived from authoritative ownership plus graph connectivity. It is not painted from player proximity, kills, patrol destruction or ambient population.

Required semantic sets are:

- connected BLUFOR territory;
- connected OPFOR territory where needed;
- BLUFOR boundary positions;
- enemy frontline candidates;
- isolated friendly/enemy pockets;
- selected BLUFOR objective;
- legal OPFOR counterattack objective;
- locked enemy territory;
- terminal positions/outcome state.

Ownership remains underneath strategy. For vanilla-base-backed nodes, the vanilla base faction is truth. Additional position types need a reviewed adapter that preserves genuine physical capture and emits ownership changes; they must not introduce scripted ownership shortcuts.

## 10. BLUFOR objective semantics

Exactly one eligible enemy position is published as `GetBLUFORLegalObjective()`. Other enemy positions remain physically accessible but strategically locked. Capture authorization must deny strategic ownership progression for non-selected positions without preventing ordinary travel or combat.

Semantic objective states should remain conceptually stable:

- friendly rear;
- friendly boundary/recently captured;
- selected enemy frontline;
- eligible but not selected frontline;
- enemy rear locked;
- invalid/unresolved;
- terminal where useful.

The existing enum may be extended or replaced internally, but consumers should ask semantic questions rather than compare graph structure.

## 11. OPFOR counterattack semantics

After a successful BLUFOR capture, the newly captured position normally becomes the legal OPFOR counterattack objective if it remains the valid friendly boundary created by that advance. The strategic engine publishes that ID through `GetOPFORLegalObjective()` and `CanFactionStrategicallyAttack()`.

M3.2 continues asking only:

> What is the legal OPFOR objective now?

It must not traverse edges, calculate connected components or score nodes. If ownership changes again, the strategic revision invalidates or completes the operation exactly as in M3.

## 12. MOB-only deployment invariant

The BLUFOR MOB is the only player deployment and respawn location. Captured towns, bases, depots, ports, airfields and objectives never become player spawn points.

Map definitions may identify MOB candidates and the active MOB, but may not opt ordinary strategic positions into forward player deployment. Future reinsertion systems must move players physically. They must not bypass campaign depth through forward spawning.

This invariant requires an acceptance check on every map package.

## 13. Strategic positions versus ambient enemy presence

Strategic positions own campaign meaning: ownership, adjacency, eligibility, progression and victory.

Ambient/local presence owns lived experience: patrols, checkpoints, roadblocks, observation posts, occupied compounds, local defenders, convoys and reaction forces. An ambient encounter has no strategic capture authority unless the map definition explicitly promotes its location to a strategic position with a capture adapter.

This separation prevents destroying a random patrol from changing territory and prevents strategic selection from becoming dependent on transient AI population.

## 14. Living-world population architecture

A future population manager consumes, but does not define, territory. Its inputs are semantic territory, player observers, map population rules and a global performance budget. It owns materialisation leases and population lifecycle for ambient systems.

Conceptual layers:

```text
Strategic engine: ownership, graph, legal objectives
        ↓ semantic territory snapshot
Population planner: desired presence by territory/region/type
        ↓ budget allocation and priority
Materialisation services: garrisons, checkpoints, patrols, later QRF/convoys
        ↓
Physical vanilla AI and normal combat/capture
```

Population must be deterministic enough to diagnose, spatially safe around human observers, bounded, authority-owned and disposable without changing strategic ownership.

## 15. Checkpoint architecture

The future checkpoint system belongs below the population manager. Map data supplies allowed roads/areas, exclusions, density and optional authored anchors. Runtime policy selects enemy-territory road positions, validates road geometry and safe materialisation, requests budget, and owns spawned groups/props.

Checkpoint identity and lifecycle are independent of strategic position identity. A checkpoint becomes strategic only when authored separately as a strategic position. No checkpoint implementation belongs in M4.0–M4.5.

## 16. Patrol architecture

The future patrol system also consumes territory and population budget. Initial patrols are infantry with plausible route/area constraints, density configuration, observer-safe materialisation and explicit cleanup ownership. Motorised patrols, convoys and reaction forces are later extensions sharing the same budget and territory interfaces.

Patrol outcomes do not directly affect strategic selection or ownership.

## 17. Generic map-definition contract

The recommended contract is a versioned authored resource containing:

- schema version;
- stable `mapId` and display metadata;
- strategic-position definitions;
- graph edges;
- one or more MOB candidates and one selected/default MOB policy;
- endpoint/victory definitions;
- initial ownership expectations;
- scoring weights/traits where map-specific;
- optional regions;
- future population-rule references for checkpoints, patrols and ambient density;
- future map population/performance defaults.

Core loads the resource through a generic registry/resolver. Runtime world references are resolved and validated once on authority. The registry exposes ID lookup, resolved position, ownership adapter, neighbours, traits and map identity. It does not select objectives.

Map packages contain data/resources and any generic adapter instances required by the world. They do not subclass or fork campaign policy merely to name terrain locations.

## 18. Arland map-definition approach

M4.1 should create the contract and a deliberately minimal Arland consumer fixture sufficient to prove loading and portability—not the final graph. M4.2 then performs full-island discovery and authoring.

Arland discovery must inventory:

- every vanilla Conflict base and its capture/ownership components;
- towns, villages, military installations, port/coastal facilities, industrial/storage areas, communications sites, important compounds, crossroads and bridges;
- viable BLUFOR MOB candidates and deployment implications;
- candidate OPFOR terminal/victory semantics;
- meaningful adjacency based on roads, crossings, terrain and military movement;
- positions needing non-base capture adapters;
- map edges/coast/water/navmesh constraints;
- sensible regions for diagnostics and future population;
- road areas and exclusions for future checkpoints;
- patrol-compatible areas and exclusions;
- expected density and AI-budget pressure.

The final Arland graph must cover the whole strategically relevant island rather than a demonstration corridor. The vanilla base list is input, not presumed completeness.

## 19. Map portability requirements

1. Core contains no map-name conditionals, Arland coordinates or Everon entity names.
2. Stable IDs are defined by each map package and namespaced by `mapId`.
3. Every map passes the same structural validator.
4. All M3 consumers operate through semantic campaign queries.
5. Deployment validation enforces MOB-only player spawning on every map.
6. Core supports base-backed and explicitly registered non-base capture adapters.
7. Replication is based on stable IDs, not local array order.
8. Everon regression resources remain runnable throughout migration.
9. A core change must be tested against both the Everon reference definition and the Arland pilot definition once both exist.

## 20. Existing M2/M3 audit

| System/concept | Classification | M4 treatment |
|---|---|---|
| M2 route ID | **ADAPT** | Generalise to stable map-definition/campaign-definition identity. Keep `EVERON_NORTH_SOUTH_01` as reference data. |
| Ordered route array | **REFERENCE-ONLY** | Retain for Everon tests; production registry is ID lookup plus graph adjacency. |
| Frontier index | **REFERENCE-ONLY / REMOVE EVENTUALLY from production API** | Cannot express graph boundaries. Optional only in the Everon adapter/diagnostics. |
| Ownership derivation | **KEEP + ADAPT** | Keep authoritative derivation from vanilla ownership; generalise through per-position capture adapters and connectivity. |
| BLUFOR legal objective query | **KEEP** | Preserve ID-returning semantic contract. Back with graph selection. |
| OPFOR legal objective query | **KEEP** | Preserve ID-returning semantic contract. Back with recent-capture/boundary policy. |
| Semantic objective states | **KEEP + ADAPT** | Preserve consumer intent; extend for candidate/non-selected boundary and pockets if required. |
| Revision system | **KEEP** | Monotonic authoritative semantic revisions, idempotent duplicate handling and JIP reconstruction remain mandatory. |
| Validity | **KEEP + ADAPT** | Replace “valid linear frontier” with graph-definition and runtime-territory validity; retain fail-closed behavior. |
| Campaign outcome | **KEEP + ADAPT** | Preserve terminal, immutable outcomes; derive from authored graph endpoint/victory semantics. |
| Replicated strategic projection | **KEEP + ADAPT** | Replace frontier index and 31-bit lock mask with ID-based graph snapshot/delta representation. Preserve live sync/JIP semantics. |
| M2.5 endpoints | **ADAPT** | Replace first/last array boundaries with authored endpoint roles and victory rules. Keep vanilla end-match bridge/idempotence. |
| M3.1 target query | **KEEP** | Already asks for legal target, position, revision and authorization. Remove any residual route naming only if encountered. |
| M3.2 coordinator | **KEEP** | It already treats the strategic layer as a semantic authority. Validity enum dependency must become topology-neutral. |
| M3.3 staging | **KEEP** | Geometry/observer/navmesh policy is map-independent; map-specific constraints remain data. |
| M3.4 force packages | **KEEP + later configure** | Assembly is map-independent. Prefab/package catalogues can later become faction/server data. |
| M3.5 selection | **KEEP + ADAPT** | Human and previous-result logic stays. Campaign depth must become graph/campaign progress rather than frontier index. |
| M3.6 garrisons | **KEEP + ADAPT** | Consume selected enemy frontline/defensive semantics, never graph internals. Later decide whether more than one boundary garrison is population-managed. |
| M3.7 human presence | **KEEP** | Map-independent participant and observer contracts. |

Nothing is deleted during M4 migration. “Remove eventually” means deprecate from the production implementation only after equivalent semantic tests pass.

## 21. Migration from the Everon reference route

Migration is adapter-first, not rewrite-in-place:

1. Freeze the current Everon world, mission and proof resources as regression assets.
2. Introduce the generic map-definition and registry beside the linear component.
3. Represent the Everon route through an adapter or equivalent graph definition without deleting the old evaluator.
4. Run both evaluators against identical Everon ownership states and compare legal objectives, authorization, outcomes and revisions.
5. Build the independent Arland definition to prove core portability.
6. Introduce a topology-neutral strategic query/projection behind the existing semantic method names.
7. Migrate M3 consumers without exposing adjacency or scores to them.
8. Retain the old linear proofs until graph parity, replication and JIP are accepted.

The current 31-node validation limit and 32-bit `lockedMask` are hard blockers for dozens/hundreds of positions. Production projection must use stable-ID collections or a bounded encoded snapshot with explicit schema/versioning. Array index may be a wire encoding optimisation only if the authoritative map-definition identity/version guarantees identical mapping and clients never treat it as strategic meaning.

## 22. Performance and population implications

Graph calculation is cheap relative to physical AI if recomputed on ownership/configuration events rather than every frame. Cache adjacency and connected-component results per strategic revision. Selection should evaluate only the boundary candidate set.

Replication must avoid broadcasting full map state on every tick. Publish only on semantic revision, support a complete current-state snapshot for JIP, and consider compact deltas only after correctness is proven.

Physical AI requires a global authority-owned budget with categories and priority. Reserved capacity should protect capture-critical operations and the visible frontline. Ambient checkpoints/patrols consume remaining capacity and dematerialise safely. Budget inputs include maximum agents/groups, relevant observers, distance, current objective, active operations, region pressure and server performance signals.

No map-wide population feature should precede the initial budget contract and telemetry.

## 23. Difficulty and configuration philosophy

Configuration establishes legitimate physical capability, not guaranteed balance. Player count supplies a baseline; server/admin multipliers and authored map density may increase it; performance is the hard ceiling.

Future configuration may expose garrison strength, checkpoint/patrol density, attack and reinforcement strength, waves, delays, cooldowns and maximum physical AI. Selection and budget decisions must log their inputs. Difficulty must never come from teleporting near players, magical ownership, omniscient AI or bypassing vanilla capture.

No configuration UI is part of M4.0.

## 24. Revised M4 milestone sequence

The proposed sequence is refined to put the hard performance ceiling before dense population:

1. **M4.0 — Map-Wide Conflict Core Architecture**: this document.
2. **M4.1 — Generic Map Definition Contract and Strategic Position Registry**: schemas, resolver, validation, minimal Arland consumer fixture, Everon coexistence.
3. **M4.2 — Full Arland Strategic Graph and Territory Ownership**: whole-island authoring, connectivity, ownership adapters and territory derivation.
4. **M4.3 — One-Objective-at-a-Time Strategic Selection**: eligibility-first deterministic selector and explainability.
5. **M4.4 — M3 Migration to Graph Semantic Contract**: topology-neutral projection/query and unchanged M3 consumers.
6. **M4.5 — Population and Performance Budget Foundation**: global budget, priorities, telemetry and safe allocation contract.
7. **M4.6 — Arland Map-Wide Strategic and Ambient Population Foundation**: occupied-territory planning under budget; no checkpoint/patrol specialisation yet.
8. **M4.7 — Dynamic Road Checkpoints**.
9. **M4.8 — Dynamic Infantry Patrols**.
10. **M4.9 — Full Arland Campaign Acceptance**.

This ordering prevents M4.6–M4.8 from creating unbounded AI first and retrofitting performance control later. Vehicles, convoys, QRF and reinforcement waves remain later milestones.

## 25. Major risks

- **Capture representation:** strategically useful Arland locations may lack vanilla Conflict base/capture components. Each needs an explicit, tested adapter without magical ownership.
- **Projection scale:** the existing 31-bit mask/index projection cannot scale and must change without regressing live replication or JIP.
- **Graph validity:** isolated pockets, multiple MOB candidates, asymmetric ownership and terminal rules need explicit fail-closed semantics.
- **Selection quality:** a technically valid but militarily absurd graph or scoring model will produce poor campaigns. Explainability and authored adjacency are essential.
- **M3 coupling:** validity names and campaign-depth calculation still encode a linear topology even though target queries are already semantic.
- **Population cost:** whole-island occupation can exceed AI/network budgets quickly. Budgeting must precede density.
- **Materialisation visibility:** dense systems increase pop-in and cleanup risks; M3.7A observer semantics must remain central.
- **Authored-data volume:** full-island coverage creates validation and maintenance burden. Tooling/diagnostics should detect duplicate IDs, missing references, bad edges and unreachable nodes.
- **Regression erosion:** modifying the Everon world in place would destroy proven evidence. New architecture must coexist until parity is demonstrated.
- **Endpoint pressure:** an OPFOR legal objective at the MOB requires a navigable staging/capture model; the M3.7 session already exposed a `target-navigation-anchor-unavailable` case that graph migration must not hide.

## 26. Explicit non-goals

M4.0 does not implement graph code, an Arland graph, objective scoring, territory code, AI, checkpoints, patrols, vehicles, reinforcements, persistence, campaign UI, FIA PvP, balance changes or forward deployment. It does not modify M2/M3, the Everon reference topology, missions, worlds, proof modes or runtime resources.

## 27. Recommended exact M4.1 scope and acceptance criteria

> **M4.2A locked campaign-start amendment:** a map may author multiple valid MOB candidates. A new/reset campaign selects one by stable ID from an explicit campaign seed, or by a validated explicit administrator override. Resume reuses the stored seed and selected MOB and never rerolls. The selected MOB determines its authored opposing endpoint and is the only player-deployable strategic position for that campaign.

### Scope

M4.1 creates only the versioned generic definition types, registry/resolver, structural validator, ownership-adapter interface and read-only registry query. It adds a minimal Arland definition fixture with enough real positions/edges to prove the contract and a minimal Everon representation/adapter to prove coexistence. It does not calculate territory, choose objectives, replace M2 projection, migrate M3, or author the full Arland island.

### Acceptance criteria

**A. Schema identity** — A versioned map definition exposes a non-empty stable `mapId`, schema version and definition identity.

**B. Stable positions** — Position definitions support stable ID, type, coordinate/reference, importance, optional region/traits, MOB candidacy, endpoint role and capture-adapter reference without array-order semantics.

**C. Graph data** — Edge definitions resolve two stable position IDs and allow future metadata without core map conditionals.

**D. Registry resolution** — On authority, the registry resolves every fixture position by ID and provides position, type, traits, neighbours and ownership adapter through a topology-neutral read-only API.

**E. Fail-closed validation** — Duplicate/empty IDs, dangling/self/duplicate edges, missing MOB, unresolved required world references/adapters, invalid endpoint declarations and unreachable required nodes produce explicit validation failures and no READY state.

**F. Vanilla ownership source** — At least one Arland fixture position backed by a vanilla Conflict base reports its actual vanilla owner through the generic adapter. No ownership mutation is introduced.

**G. Non-base extensibility** — The schema can describe a non-base strategic position, but it cannot become runtime-valid without an explicitly registered supported ownership/capture adapter.

**H. Minimal Arland consumer** — A small, clearly non-production-complete Arland fixture loads through the generic contract with real resolvable references sufficient to demonstrate portability. It is labelled fixture, not the M4.2 full graph.

**I. Everon coexistence** — `EVERON_NORTH_SOUTH_01`, its world/mission resources and existing M1–M3 proofs remain unchanged and resolvable while the generic registry exists.

**J. No map conditionals** — Core registry and validation code contain no `Arland`/`Everon` branching or terrain coordinates.

**K. MOB-only invariant represented** — The definition identifies MOB candidates/active MOB while ordinary positions have no player-deployment capability. M4.1 does not enable any spawn point.

**L. Deterministic diagnostics** — Registry output uses stable ID ordering and records map identity, counts, resolved positions, edges, adapters and precise validation failures.

**M. Scale contract** — Validation accepts a fixture exceeding 31 positions or a bounded synthetic definition, proving that the data model itself has no 31-node/bitmask limit. This is not replication proof.

**N. No strategic behavior change** — M2 remains the active Everon evaluator; M3 target selection, capture authorization, outcomes and deployment behavior are unchanged.

**O. Compile/resource validation** — Real Workbench compile and resource processing pass for both the preserved Everon resources and the minimal Arland fixture.

Runtime criteria must not be marked PASS from source inspection. M4.1 ends when the generic contract/registry and portability fixture are proven; full Arland discovery and graph authoring begin only in M4.2.

## 2026-10-01 supplement — M4.X native-scenario portability evidence

This supplements, rather than rewrites, the historical architecture and
milestone evidence. The Creative Director's native-scenario direction is now
being tested in a separate three-node Arland world; production M4.4J resources
have not been migrated.

The installed vanilla campaign-base initialization depends concretely on
SCR_GameModeCampaign for spawn, services and capture hooks. The prototype
therefore subclasses that service parent, overriding initial campaign setup
and winner selection, without inheriting the CTI/CTL campaign world hierarchy.
It reuses the generic registry/evaluator/public projection and existing M3
services, with an explicit metadata-only OPFOR-origin start adapter.

Runtime proves three-node/two-edge READY, US MOB/USSR objectives, disabled
forward deployment, genuine vanilla US capture, objective advancement, existing
M3.1 physical waypoint movement and USSR recapture/graph retreat. It also proves
fresh ACTIVE, foothold-triggered OPFOR pressure and US 2/2 BLUFOR pressure.
No fake human count or direct gameplay ownership mutation was used.

Do not extrapolate that to completed human/coordinator acceptance: no human
client was admitted in these tests. The direct-operation driver also retains
its failed strict SUCCEEDED assertion because M3 returns CANCELLED when the
last-territory recapture legitimately enters pressure. Detailed evidence,
the migration survival classifications, remaining boundaries and NO-GO-for-now
gate are in `TB-Conflict-Core-M4X-Native-Scenario-Prototype.md`.

Network N1–N5 remain pending. No production migration or M4.5 is authorized.
