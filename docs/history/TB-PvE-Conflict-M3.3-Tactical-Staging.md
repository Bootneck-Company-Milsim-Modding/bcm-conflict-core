# TB PvE Conflict M3.3 — OPFOR Tactical Staging Location Policy

## Verdict

**M3.3 PASS.** A clean dedicated ServerDiag authority run proved the production staging contract, bounded failure path, coastal rejection/selection behavior, player-equivalent observer exclusions, physical AI movement, vanilla capture, M2 frontier reaction, fresh repeated staging and the M3.2 single-operation invariant.

Runtime evidence:

`TestProfiles/M3_3_Authority_15/logs/logs_2026-09-29_14-41-54/console.log`

The bounded proof switch is returned to `0`. The production M3.2 coordinator remains enabled. No M3.4 implementation was started.

## Service contract and responsibility boundary

`TB_PVE_M3_3_TacticalStagingPolicy.FindStagingPosition` accepts an operation faction, the target identifier and position already selected through the M2 public contract, plus current human-controlled observer positions. It returns either an accepted world position or a concrete failure reason, including `NO_VALID_STAGING_POSITION`.

The service does not decide whether an operation exists, choose a strategic target, select force composition, alter ownership or move the frontier. M3.1 still obtains the USSR legal objective only through `TB_PVE_M2_3_FrontierQuery`; M3.2 still owns operation timing and concurrency.

## Production configuration

- target annulus: 200–325 m;
- minimum human-player distance: 250 m;
- direct-visibility relevance distance: 900 m;
- target-to-infantry-navmesh anchor tolerance: 100 m;
- bounded candidate attempts: 64;
- detailed candidate diagnostics: disabled in production;
- proof switch: disabled after acceptance.

The annulus was reduced from the initial 350–500 m trial after runtime showed that range lay predominantly in water around Kermovan. The accepted 200 m coastal point remained well outside the objective seizing radius and required meaningful physical movement.

## Candidate generation and selection

The policy samples four distance bands across the configured annulus and sixteen bearings. Query serial rotation changes the ordering between calls. The first candidate passing every rejection rule is selected. This is deliberately simple tactical variation, not strategic scoring, and contains no per-objective coordinates.

Infantry navmesh tiles for the target approach and annulus are requested explicitly. Streaming is asynchronous: if usable target navigation is not yet available, the query fails safely and the existing coordinator cooldown/retry lifecycle applies.

## Validation policy

Each candidate is checked for:

- horizontal distance inside the configured annulus;
- water through `ChimeraWorldUtils.TryGetWaterSurfaceSimple`;
- a valid tile in the `Soldiers` `NavmeshWorldComponent`;
- a usable target-side infantry-navmesh anchor within tolerance;
- supporting world surface;
- surface normal of at least 0.82 to reject excessive slope;
- minimum distance from each actual human-controlled player entity;
- direct trace visibility from relevant observers within 900 m.

The server visibility rule is intentionally a direct line trace, not camera-frustum, stance, foliage-perception or cinematic visibility modelling. Roads and other walkable supporting surfaces are not rejected merely because the downward world trace reports an entity; navmesh, water and slope remain authoritative for infantry suitability.

Navigation validation does not calculate a complete tactical path. It establishes usable soldier navigation at the candidate and target approach, then leaves route finding to the ordinary AI waypoint system. The full Kermovan run additionally proved the selected candidate was operationally connected by physical navigation to capture range.

## Failure behavior and M3.1 integration

M3.1 no longer computes a fixed offset. Its flow is now:

`M2 target -> M3.3 query -> spawn ordinary group -> ordinary move waypoint`

On staging failure it enters `FAILED` before group creation. M3.2 disposes the terminal operation, clears its reference, enters cooldown and later performs a fresh M2 query. The proof forced zero candidate attempts and observed explicit failure, no active group, stable US ownership and revision 3. There was no target fallback, origin spawn or teleport.

## Dedicated runtime results

Route samples:

| Sample | Objective | Accepted position | Distance | Bearing | Attempts | Query time |
|---|---|---:|---:|---:|---:|---:|
| Early | OBJ_01_MAIDENS_BAY | `<5413.4,38.306,10778.4>` | 200 m | 67.5° | 1 | 0 ms |
| Coastal operation target | OBJ_02_KERMOVAN | `<6236.89,5.04103,9667.35>` | 200 m | 270° | 7 | 1 ms |
| Mid | OBJ_05_LARUNS | `<7326.65,124.582,5096.3>` | 200 m | 202.5° | 1 | 0 ms |
| Late | OBJ_08_VERNON | `<8928.48,88.3762,2292.27>` | 200 m | 270° | 1 | 0 ms |

At Kermovan, six water positions were rejected before valid navigable land was accepted. A known-ocean candidate independently returned `water`. At Maidens Bay an excessive-slope candidate was also rejected.

The player-distance test placed an observer at the accepted candidate and received `human-player-distance`. The visibility test used a server-side player-equivalent observer 270.2 m away, outside the distance exclusion but with unobstructed line of sight; it received `human-player-direct-visibility`. A later query rejected that visible point and accepted `<5428.62,39.2069,10701.9>` instead.

The observer evidence exercises the exact authority policy with controlled positions; it is not evidence of a newly authenticated retail client session.

## Autonomous operation, repetition and LOD

The coordinator selected OBJ_02_KERMOVAN at revision 3. M3.3 accepted `<6236.89,5.04103,9667.35>` and M3.1 spawned its ordinary three-man USSR defender group there. Natural group LOD was 0 while maximum LOD was 10; no activation lease was acquired or added.

The group moved from 200.064 m to a recorded minimum of 5.136 m, entered capture range, completed genuine vanilla ownership change `US -> USSR`, and M2 published revision 4. The operation then entered `SUCCEEDED`.

After cooldown, a fresh operation targeted OBJ_01_MAIDENS_BAY at revision 4 and selected `<5043.84,37.5359,10625.4>`, distinct from the first operation point. It demonstrated meaningful movement before the proof deliberately invalidated the campaign for cleanup. Maximum concurrent operations remained one.

Query work was bounded at 64 candidates and occurred only during operation creation. Observed successful queries were 0–1 ms; the earlier exhaustive 64-candidate coastal rejection trial completed in 14 ms.

## Acceptance A–T

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Production `TB_PVE_M3_3_TacticalStagingPolicy` exists and ran on dedicated authority. |
| B | PASS | M3.1 target remained the M2.3 public USSR legal objective. |
| C | PASS | Fixed offset removed; bearing/distance candidate matrix used. |
| D | PASS | Every accepted runtime sample was inside 200–325 m. |
| E | PASS | Accepted operations began at 200 m, outside seizing range. |
| F | PASS | Controlled near observer produced `human-player-distance`. |
| G | PASS | Clear relevant LOS produced `human-player-direct-visibility`; alternate selected. |
| H | PASS | Coastal and known-ocean water rejected; excessive slope also observed. |
| I | PASS | Candidate and target-approach soldier-navmesh checks passed; full operation physically navigated. |
| J | PASS | Forced no-candidate result failed before spawn and followed cooldown. |
| K | PASS | M3.1 consumes the staging service before materialization. |
| L | PASS | Ordinary USSR group moved 200.064 m to capture range. |
| M | PASS | Vanilla Kermovan capture and revision 3 -> 4 succeeded. |
| N | PASS | Early, coastal, mid and late objectives tested on the production route. |
| O | PASS | Repeated selections and operations used distinct positions/bearings. |
| P | PASS | Maximum coordinator concurrency remained one. |
| Q | PASS | No staging/operation direct ownership or frontier mutation; vanilla/M2 remained underneath. |
| R | PASS | Integration accepts USSR only; no FIA operation was introduced. |
| S | PASS | 64-attempt bound; measured successful queries 0–1 ms. |
| T | PASS | Natural operation LOD 0 versus max 10; no lease required. |

## Remaining unknowns

- The direct-visibility rule was authority-tested with controlled observer coordinates, not a retail remote player's live camera/stance.
- Early/mid/late samples validate both ends against soldier navigation, while complete end-to-end physical connectivity was runtime-proven specifically for Kermovan. No bespoke full-path solver was added.
- Navmesh streaming can cause a first query against a cold area to fail safely; the coordinator's existing retry lifecycle is the recovery mechanism.
- Exact terrain/foliage concealment quality will benefit from later human play observation, but does not block the narrow contract.

## Recommended M3.4 scope

Keep M3.4 narrow: define one production force package/assembly contract that consumes the already selected operation and staging point, while preserving the single-operation coordinator, vanilla capture and current LOD evidence. Do not add waves, reinforcements, vehicles, economy or strategic scoring in that slice.
