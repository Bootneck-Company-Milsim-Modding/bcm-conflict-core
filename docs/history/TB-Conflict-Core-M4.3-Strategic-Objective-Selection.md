# M4.3 — Eligibility-First Strategic Objective Selection

> **M4.4H product amendment:** candidate selection now excludes both selected HQ anchors unconditionally. Endpoint-readiness metadata from M4.4G is retained for compatibility/history but no longer admits an HQ into the production objective set. When no enemy ordinary position remains, the campaign enters victory pressure instead of selecting an HQ.

## Verdict

**PASS**

M4.3 implements and runtime-proves a generic, read-only strategic evaluator for the Arland production pilot. It consumes the M4.1 registry, M4.2 graph, and the already-resolved M4.2A campaign-start state. It does not reroll the campaign start, mutate vanilla ownership, or migrate any M2/M3 gameplay consumer.

Final dedicated proof log: `TestProfiles/M4_3_Final_2026-09-30_20-20-19/logs/logs_2026-09-30_20-20-19/console.log`.

## Implementation

Created:

- `Scripts/Game/TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c`
- `Scripts/Game/TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluatorProof.c`

Modified:

- `Prefabs/Systems/TB_CC_M4_1_ArlandFixture.et` — added the M4.3 proof component only.

No M2/M3 gameplay code, Everon topology, mission gameplay, population, or UI was changed.

## Input and ownership contract

The evaluator requires a valid `TB_CC_M4_2A_CampaignStartState` and consumes its campaign seed, selected MOB stable ID, and selected endpoint stable ID unchanged. An invalid start state fails closed; M4.3 never chooses substitutes.

Ownership is accessed through `TB_CC_M4_3_OwnershipView`. Production evaluation uses `TB_CC_M4_3_VanillaOwnershipView`, which delegates to the M4.1 registry ownership adapter. Proofs use `TB_CC_M4_3_SyntheticOwnershipView`, allowing deterministic read-only ownership scenarios without modifying production vanilla ownership.

## Friendly-component traversal

The evaluator performs a breadth-first traversal beginning at the selected MOB. An edge is traversed only when the adjacent position is currently US-owned. The resulting stable-ID-sorted set is the MOB-connected friendly component.

A US-owned node outside this component is marked `FRIENDLY_ISOLATED`. It neither extends the frontline nor unlocks objectives adjacent to that isolated pocket.

## Eligibility before preference

Candidate construction completes before any scoring occurs. A BLUFOR candidate must be:

- USSR-owned;
- directly adjacent to at least one member of the MOB-connected US component;
- resolvable through the M4.1 registry and its real ownership adapter;
- consistent with the valid campaign start and graph state.

Candidate IDs are sorted by stable strategic-position ID before evaluation. Strategic importance, endpoint status, traits, connectivity, and seed variation cannot make a non-candidate selectable. The selected MOB is never a BLUFOR objective. The selected endpoint receives no eligibility exemption and must reach the ordinary graph boundary.

If USSR positions remain but the boundary yields no valid candidates, the evaluator returns `NO_VALID_FRONTLINE_OBJECTIVE`, exposes no legal objective, and denies strategic attack authorization. If no USSR position remains it reports `NO_ENEMY_REMAINING`; M4.3 deliberately does not derive campaign victory.

## Preference scoring

Only eligible candidates are scored:

`final = strategic importance + trait contribution + (graph degree * 2) + endpoint contribution + seeded variation`

Trait weights:

| Trait | Contribution |
|---|---:|
| Military | 8 |
| Logistics | 6 |
| Supply | 5 |
| Transport | 4 |
| Population center | 4 |
| Communications | 3 |
| Infrastructure | 3 |

The endpoint contribution is `+10`, but only after the endpoint independently qualifies as a boundary candidate. This is preference, not eligibility.

Seeded variation is bounded to `0..6`:

`abs(((campaignSeed % 997) * 31 + stableCandidateOrdinal * 17) % 7)`

No runtime `Random()` call is used. Identical definition, ownership, campaign start, and seed reconstruct identically. Because the current Arland opening scores are decisive, seeds 0–23 do not change the winning East or West opening; the proof therefore establishes bounded eligible-only variation, not fabricated route variety. Exact final-score ties resolve to the lexically lowest stable position ID because candidates are stable-ID sorted and an equal score never replaces the current winner.

## Read-only public contract

`TB_CC_M4_3_StrategicEvaluator` exposes:

- `IsReady()` and `IsValid()`;
- evaluation state and semantic revision;
- selected MOB and endpoint IDs;
- copied friendly-component and candidate-ID arrays;
- BLUFOR and OPFOR legal objective IDs;
- per-position strategic state;
- `CanBLUFORAttack(positionId)` and `CanOPFORAttack(positionId)`;
- a read-only decision snapshot with copied collections.

The decision snapshot records the seed, start IDs, friendly component, candidate set, per-candidate score breakdown and eligibility reason, selected objective, and selection reason. Revision increments only when the public semantic signature changes. Re-evaluating identical state does not increment it.

## Runtime results

### East start

Selected MOB: `ARLAND::HQ_EAST`.

Eligible candidates were exactly:

- `ARLAND::MILITARY_SOUTH` — base 85, traits 17, connectivity 10, endpoint 0, seed variation 4, final 116;
- `ARLAND::MOSS_HILL` — base 75, traits 14, connectivity 6, endpoint 0, seed variation 0, final 95.

Selected objective: `ARLAND::MILITARY_SOUTH`. Deeper nodes were denied.

### West start

Selected MOB: `ARLAND::HQ_WEST`.

Eligible candidates were exactly:

- `ARLAND::AIRPORT_NORTH` — base 85, traits 26, connectivity 6, endpoint 0, seed variation 0, final 117;
- `ARLAND::FARM` — base 60, traits 15, connectivity 10, endpoint 0, seed variation 3, final 88.

Selected objective: `ARLAND::AIRPORT_NORTH`. Deeper nodes were denied. East and West campaign starts therefore produce different opening geometry.

### Topology changes

- Isolated-pocket proof: a disconnected US-owned `ARLAND::BEAUREGARD` did not enter the friendly component and did not unlock Farm or Arleville.
- Branch proof: after a legitimate synthetic capture of Military South, the candidates recalculated to Airport North, Arleville, Farm, and Moss Hill.
- OPFOR response: the most recently captured authorised objective, Military South, became the OPFOR response objective because it remained US-owned, non-MOB, and adjacent to USSR territory.
- Reconvergence: after Military South, Moss Hill, and Arleville became connected US territory, candidates were Airport North, Beauregard, and Farm; already-friendly Arleville was not reselected.
- Reverse capture: when OPFOR recaptured Military South, the connected component and candidates recalculated, the previous response target cleared, and no stale BLUFOR objective survived.
- No-candidate proof: a disconnected topology with enemy territory remaining returned `NO_VALID_FRONTLINE_OBJECTIVE` and denied attacks rather than inventing an objective or declaring victory.

## OPFOR response semantics

The evaluator accepts the stable ID of the most recently captured authorised BLUFOR objective as strategic history. It exposes that position as the OPFOR legal response only while it remains US-owned, is not the selected MOB, is adjacent to at least one USSR position, and the topology remains valid and non-terminal. M3.2 is not migrated and need not understand graph edges.

## Performance

The Arland East evaluation completed in 1 ms at the available timer resolution. A synthetic chain of 128 positions and 127 edges evaluated in 0 ms at that resolution and selected only its immediate eligible boundary. Evaluation is explicit/event-driven; no per-frame graph analysis was introduced.

## Acceptance A–Z

| Criterion | Result | Runtime evidence |
|---|---|---|
| A. Consume campaign start | PASS | Valid M4.2A seed/MOB/endpoint consumed unchanged. |
| B. MOB-connected US component | PASS | BFS component derived from selected MOB through US nodes only. |
| C. Isolated pocket denial | PASS | `isolated-pocket-excluded` and `isolated-pocket-no-deep-unlock`. |
| D. Graph-adjacent boundary | PASS | East/West exact boundary candidate sets. |
| E. Eligibility before scoring | PASS | Only constructed eligible candidates received score records. |
| F. Deep-node denial | PASS | `east-deep-denial` and `west-deep-denial`. |
| G. Exactly one BLUFOR objective | PASS | `east-one-objective`; one selected ID from each non-empty candidate set. |
| H. Other enemies unauthorised | PASS | Attack query allowed only the selected stable ID. |
| I. Deterministic/explainable score | PASS | Runtime per-candidate score breakdowns and selection reason. |
| J. Seed only among eligible nodes | PASS | East/West seeds 0–23: `seed-matrix-eligible-only`. |
| K. Same seed/state reconstruction | PASS | `east-repeat-deterministic`; revision remained stable. |
| L. Stable-ID final tie-break | PASS | Stable-ID sort plus retain-first equality rule. |
| M. Endpoint ordinary adjacency | PASS | Endpoint bonus applied only after candidate eligibility; deep endpoint denied. |
| N. Fail-closed no-candidate state | PASS | `no-candidate-fails-closed`. |
| O. Correct East opening | PASS | Military South and Moss Hill only. |
| P. Correct West opening | PASS | Airport North and Farm only. |
| Q. Different start geometry | PASS | East and West candidate sets and winners differ. |
| R. Branch recalculation | PASS | `branch-recalculated`. |
| S. Reconvergence | PASS | `reconvergence`; friendly node excluded and enemy boundary remained reachable. |
| T. Semantic OPFOR response | PASS | `opfor-response`. |
| U. Reverse capture recalculation | PASS | `reverse-component-recalculated`. |
| V. No stale objective | PASS | `reverse-no-stale-response` and `reverse-no-stale-objective`. |
| W. Public read-only query | PASS | `publicQuery=1`; copied collections and semantic accessors. |
| X. No M2/M3 migration | PASS | `gameplayMigration=0`; no M2/M3 source changed. |
| Y. Everon unchanged | PASS | No Everon topology/resource changed. |
| Z. Workbench/resource/runtime proof | PASS | Workbench `-createDB` completed and dedicated runtime entered online game state with all M4.3 proof checks passing. |

## Remaining limitations and unknowns

- M4.3 proves selection against real Arland metadata and synthetic read-only ownership transitions. Production ownership initialization and subsequent vanilla capture events are not yet wired to drive this evaluator; that is M4.4 work.
- The current ownership-adapter contract proves that a candidate resolves and its owner can be read. If future strategic-position adapters represent non-base capturable objects, the adapter API may need an explicit strategic-capturability predicate rather than inferring availability from successful resolution.
- Replication, client projection, physical capture enforcement, legacy M2/M3 consumer migration, and deployment effects are intentionally absent.
- Campaign outcome derivation after `NO_ENEMY_REMAINING`, endpoint victory integration, and persistence remain later work.
- Seed variation is operational and bounded, but current Arland opening score gaps are too large for the tested seeds to change either winner. No metadata was distorted to manufacture variety.

## Exact M4.4 migration contract

M4.4 should integrate this evaluator without changing its eligibility rules:

1. Instantiate the evaluator on authoritative server initialization only after M4.1 registry, M4.2 graph, and M4.2A campaign-start state are valid.
2. Feed it authoritative vanilla ownership through the M4.1 adapter view and re-evaluate on meaningful ownership/start-state changes, not every frame.
3. Project its semantic revision, evaluation state, selected start IDs, candidate set, and BLUFOR/OPFOR legal objective IDs to clients with join-in-progress reconstruction.
4. Replace legacy M2 frontier-index/lock-bitmask consumers at a narrow boundary: capture authorization, legal-objective presentation, and M3 strategic target queries consume the M4.3 public contract rather than graph internals.
5. Enforce that only `CanBLUFORAttack`/`CanOPFORAttack` objectives can advance strategic ownership while retaining vanilla physical capture and vanilla ownership underneath.
6. Pass the last legitimately captured authorised BLUFOR objective back as response history so the evaluator derives the OPFOR response target. Do not teach M3.2 graph traversal.
7. Preserve the selected MOB deployment contract and route endpoint/outcome handling; do not make the MOB a normal objective.
8. Add dedicated authority, remote-client live-sync, mid-state JIP, branch, reverse-capture, no-candidate, and regression acceptance. Keep the existing Everon reference campaign unchanged until its consumers are deliberately switched and proven.

M4.4 must not add population simulation, UI redesign, persistence, or broader AI strategy. Its scope is authoritative projection, enforcement, and controlled migration of existing semantic consumers to the proven graph-selection contract.
