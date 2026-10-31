# TB PvE Conflict M3.4 — Data-Driven Infantry Force Packages

## Verdict

**M3.4 PASS.** A clean dedicated ServerDiag authority run proved the package registry, server-authoritative atomic assembly, individually validated multi-group staging, operation ownership, movement, vanilla capture, partial- and total-force-loss behavior, cleanup, natural LOD and the existing single-operation invariant.

Runtime evidence:

`TestProfiles/M3_4_Authority_7/logs/logs_2026-09-29_16-17-53/console.log`

The bounded proof switch is returned to `0`. The production coordinator remains enabled with `STANDARD` as its configured default. No M3.5 work was started.

## Package model and lookup

`TB_PVE_M3_4_ForcePackage` contains a stable ID, faction key, package role and an array of entries. Each `TB_PVE_M3_4_ForcePackageEntry` contains a role, real group resource and quantity. It contains no objective, coordinate, strategic score or timing policy.

`TB_PVE_M3_4_ForcePackageRegistry.GetPackage(packageId)` is the narrow read-only lookup boundary. M3.1 receives only a package ID and assembly service; it no longer chooses or hard-codes a group prefab. M3.2 uses the authored `m_sM3_4DefaultAttackPackage` and does not randomly score or scale it.

## Authored USSR packages

All entries use the verified vanilla campaign-compatible resource:

`{359353A0A5A52216}Prefabs/Groups/Campaign/Group_USSR_Defenders.et`

| Package | Entries | Groups | Runtime units |
|---|---|---:|---:|
| LIGHT | RIFLE × 1 | 1 | 3 |
| STANDARD | RIFLE × 2 | 2 | 6 |
| REINFORCED | RIFLE × 2; SUPPORT_RIFLE × 1 | 3 | 9 |

The support role deliberately uses the same proven campaign infantry archetype. A six-man vanilla rifle-squad candidate was tested during development: it materialized and received a waypoint but did not participate in movement alongside the campaign groups. It was rejected rather than hidden behind custom AI or an invented GUID. Strength still increases meaningfully as `3 < 6 < 9` conventional infantry.

## Assembly service

`TB_PVE_M3_4_ForceAssemblyService.AssemblePackage` runs only on server authority. It validates the lookup result, USSR faction and bounded group count; asks M3.3 for an anchor plus independently valid nearby positions; creates each group and its ordinary prefab units; records operation ownership; and returns the complete force.

If any position or materialization fails, it deletes all already-created characters/groups and returns failure. The deterministic proof permitted one of three requested groups, then rejected the second: `groupsCreated=1`, `groupsRemoved=1`, no active operation, no ownership change, no downgrade. The coordinator followed its existing FAILED → COOLDOWN lifecycle.

Assembly is bounded to three groups. M3.3 remains bounded to 64 anchor candidates and 16 nearby offsets per additional group.

## Multi-group staging

M3.3 still chooses the coherent assembly anchor. Additional positions use small outward/tangential offsets around it, then independently run the complete established candidate validation: target annulus, player exclusion, direct visibility, water, slope, supporting surface, soldier navmesh and target approach. Runtime STANDARD positions were distinct by approximately 40 m; REINFORCED produced three distinct positions in the same local assembly area. No group was blindly overlaid on the anchor.

## Ownership, orders and outcome

M3.1 owns the package ID, every assembled group, each waypoint and cleanup. M3.2 owns only the operation reference and lifecycle. Every viable group receives an ordinary move waypoint toward the same objective selected through the M2.3 public query. Diagnostics record its start, distance, progress, alive count and LOD.

Success did not change: vanilla capture changed ownership, M2 published the new frontier/revision, then the operation observed that semantic result and entered `SUCCEEDED`. There is no direct ownership or frontier mutation.

## Loss and cleanup policy

An operation continues while at least one owned group has living agents. During STANDARD, both groups moved; one group was destroyed; the operation remained active with one viable group; the survivor completed vanilla capture and the operation succeeded. When all three REINFORCED groups were destroyed after movement, the operation entered `FAILED`, the coordinator disposed it and began cooldown.

The existing immediate authoritative disposal policy is retained for M3.4: waypoints, surviving characters and group entities are removed when the coordinator disposes `SUCCEEDED`, `FAILED` or `CANCELLED`. This is deterministic and leak-free, but can be visually abrupt after success. A delayed cleanup or garrison handoff is intentionally deferred until lifecycle/visibility requirements justify it.

## Dedicated proof results

- **LIGHT:** resolved one group/three units; staged through M3.3; moved from roughly 200 m; completed vanilla capture; ownership and M2 revision changed; operation succeeded and cleaned up.
- **STANDARD:** resolved two groups/six units at distinct safe positions; both moved toward the same target; one was removed; the survivor completed vanilla capture; cleanup removed the remaining owned force.
- **REINFORCED:** resolved three groups/nine units with RIFLE and SUPPORT_RIFLE roles; every group staged distinctly, remained natural LOD 0, received a waypoint and moved; a clean attempt completed vanilla capture and succeeded.
- **Total loss:** all three moving REINFORCED groups were destroyed; operation failed, ownership stayed US, cooldown ran and all owned records were disposed.
- **Atomic failure:** one group was created for a three-group request, the proof capacity seam rejected continuation, and rollback removed exactly one. No active force leaked and package identity remained REINFORCED.
- **Concurrency:** maximum concurrent operations remained `1`.
- **FIA:** packages require faction `USSR`; FIA cannot request them through the conventional coordinator.

The comparison cases reused Andrés Beacon by restoring it with genuine vanilla US capture actors between operations. Each subsequent operation therefore re-queried fresh M2 state/revision; no direct ownership shortcut was used.

## LOD and performance

Every created group logged natural `LOD=0` during assembly and movement. No maximum-LOD group was observed, so no activation lease was added.

Successful assembly was non-pathological in the diagnostic run: LIGHT created one group/three units, STANDARD two/six and REINFORCED three/nine. Logged assembly work was in the tens to low hundreds of milliseconds; staging queries were normally 0–1 ms. Rollback/cleanup was generally around 0–tens of milliseconds. These are coarse diagnostic timings, not a load benchmark.

## Acceptance A–V

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Package and entry data models exist. |
| B | PASS | M3.1 requests a package ID; prefab lookup is outside the operation. |
| C | PASS | LIGHT, STANDARD and REINFORCED provide 1/2/3 groups and 3/6/9 units. |
| D | PASS | Clean dedicated runtime loaded and spawned the real USSR campaign group resource. |
| E | PASS | Assembly requires `Replication.IsServer()` and ran on dedicated authority. |
| F | PASS | Each runtime group received a distinct position accepted by full M3.3 validation. |
| G | PASS | Operation records and cleans every package group and waypoint. |
| H | PASS | Coordinator owns only the operation reference. |
| I | PASS | Runtime logged one movement order per group toward the same M2 target. |
| J | PASS | All successful cases followed vanilla ownership → M2 revision → operation success. |
| K | PASS | STANDARD remained active after one group loss and its survivor captured. |
| L | PASS | Destruction of all three groups produced FAILED. |
| M | PASS | Deterministic partial assembly removed its one created group atomically. |
| N | PASS | REINFORCED failure did not become STANDARD/LIGHT. |
| O | PASS | LIGHT completed an autonomous coordinator-driven operation. |
| P | PASS | STANDARD assembled/moved two groups and succeeded after partial loss. |
| Q | PASS | REINFORCED assembled/moved all three groups and completed vanilla capture. |
| R | PASS | Dedicated maximum concurrent operation count was one. |
| S | PASS | Registry/service enforce USSR; FIA remains outside the coordinator. |
| T | PASS | Runtime ownership came from Conflict capture; source contains no direct mutation. |
| U | PASS | Every assembled/moving group logged LOD 0; no lease was introduced. |
| V | PASS | Candidate/group bounds and coarse assembly, staging and cleanup timings are logged. |

## Remaining unknowns

- Larger-player-count and long-duration server cost is not measured by this bounded proof.
- No authenticated remote-client replication claim is added; the documented local SteamDiag authentication limitation remains separate.
- Some locally valid M3.3 points can still produce vanilla pathfinding stalls across terrain; the existing operation timeout fails safely. Full-route tactical path validation remains intentionally outside M3.4.
- Immediate success cleanup may be visually abrupt to a nearby player.
- A truly distinct support/weapons archetype still needs compatibility proof before replacing the conservative SUPPORT_RIFLE entry.

## Recommended M3.5 scope

M3.5 should define the narrow strategic package-selection policy that chooses among these proven package IDs from campaign context. It should not add waves, reinforcements, vehicles, economy or multiple simultaneous operations until selection behavior itself is proven.
