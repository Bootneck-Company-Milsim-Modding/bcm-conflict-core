# TB PvE Conflict M3.1 — Autonomous OPFOR Attack

## Verdict

**M3.1 PASS.** A clean production-route dedicated ServerDiag session proved a server-authoritative USSR operation selected its legal target through the M2.3 public query, created one ordinary three-man infantry group 300.006 m outside the objective, issued a vanilla move waypoint, physically navigated into the seizing area, completed vanilla Conflict capture, and observed the resulting M2 frontier retreat before entering `SUCCEEDED`.

Final evidence is preserved at:

`TestProfiles/M3_1_Authority_Final/logs/logs_2026-09-29_12-07-03/console.log`

The bounded proof switch is returned to `0`. No M3.2 implementation was started. M2.4 production-route network acceptance and M2.5 outcome/JIP network verification remain the previously documented external-client debts and were not reopened.

## Operation representation and state machine

`TB_PVE_M3_1_Operation` is the first deliberately small production-shaped operation object. It stores an operation ID, `USSR` faction key, `OPFOR_ATTACK` type, target objective ID, authorization revision, assigned `SCR_AIGroup`, waypoint, state, elapsed time, terminal reason, target/staging positions, movement distances and activation-lease ownership.

The explicit observable lifecycle is:

`CREATED -> ASSEMBLING -> MOVING -> ASSAULTING -> CAPTURING -> SUCCEEDED`

Terminal alternatives are `FAILED` and `CANCELLED`. There is no scheduler, scoring, retry, wave, reinforcement, vehicle, economy, persistence, garrison or general Director layer.

## M2 public-query boundary

The operation obtains strategic intent only by calling:

- `GetCampaignOutcome()` and requiring `ACTIVE`;
- `GetOPFORLegalObjective()` for its target ID;
- `CanFactionStrategicallyAttack("USSR", target)` for authorization;
- `GetRevision()` to record and validate the authorization snapshot;
- `GetObjectiveStrategicState()` to recognize the post-capture campaign result.

The operation does not read a frontier index, lock mask, route array, route component, base component, or hard-coded target ID. The M2.3 contract gained `TryGetObjectiveWorldPosition(objectiveId, out position)`, a read-only public spatial resolver. It converts an already selected semantic ID to a destination without exposing the route representation or deriving strategy.

At creation the final run returned `OBJ_01_MAIDENS_BAY` at revision 4 with authorization true. Before every tick/state decision the operation rechecked campaign outcome, the recorded target against the current public OPFOR objective, and public authorization. It never retargets.

## Force creation, staging and AI order

The force prefab was the faction-configured ordinary defenders group:

`{359353A0A5A52216}Prefabs/Groups/Campaign/Group_USSR_Defenders.et`

Exactly one group with three USSR infantry characters was materialized. The deterministic proof staging offset is authored on the route prefab as world offset `0 0 300`; the resolved spawn was `<5228.62,36.3392,11001.9>` and target was `<5228.62,34.428,10701.9>`, producing a 300.006 m starting distance. No vehicle or wave was used by the operation.

The operation spawned `{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et` at the resolved objective and assigned it through `SCR_AIGroup.AddWaypoint`. It performs no teleport, incremental repositioning, ownership mutation or frontier mutation.

## Dedicated physical movement and capture evidence

The final clean run reported representative leader distances:

- 300.005 m in `MOVING`;
- 249.516 m;
- 197.912 m;
- 141.603 m;
- 85.7193 m followed by `MOVING -> ASSAULTING`;
- 34.2961 m after `ASSAULTING -> CAPTURING`;
- minimum observed distance 4.42207 m.

All three agents remained assigned. This is meaningful physical navigation from outside the capture radius, not spawn-in-radius evidence.

Vanilla capture state changed from `0` to `2` while owner remained US. Vanilla then emitted the base faction change `US -> USSR`. M2 independently logged `ownership-change`, recalculated frontier `old=1 new=0`, changed the next public OPFOR legal objective to `MOB_NORTH`, and published revision 5. Only after observing revision 5 and the public semantic change did the operation transition `CAPTURING -> SUCCEEDED`.

## AI LOD and activation lease

The natural group LOD at every spawn in the final dedicated run was `0`; `AIAgent.GetMaxLOD()` was `10`. The success group remained LOD 0 throughout movement and capture. Therefore no activation lease was acquired, and M3.1 did not force or globally alter AI LOD.

A narrowly scoped one-group lease implementation exists for the evidenced maximum-LOD case: it records the group's natural LOD, applies LOD 0 only to the registered operation group, and restores the recorded LOD on success, failure or cancellation. It was not exercised or required in the accepted run; all logs report `lease=0` and terminal cleanup reports release/no leak.

## Cancellation regression

The cancellation operation recorded `OBJ_01_MAIDENS_BAY` at revision 2 and physically moved from 300.006 m to 168.893 m. The proof setup then legitimately captured OBJ_02 for US through vanilla seizing, publishing revision 3 and changing the public OPFOR legal objective to OBJ_02. The operation detected that its recorded target was no longer authorized and entered `CANCELLED`; it did not retarget or capture the now-illegal OBJ_01. The waypoint and operation resources were cleaned, the lease was not held, and M2 remained authoritative.

## Group-destruction regression

A separate clean operation instance was created at revision 4. The proof removed its assigned characters before capture. On the next observation the operation detected zero agents, entered `FAILED` with `assigned-group-destroyed`, removed its waypoint, released/no-leaked lease state, and cleaned the group. OBJ_01 remained US-owned and the frontier revision remained 4.

## Cleanup and system regression

On every terminal path the waypoint is removed and deleted and any lease is released. The bounded proof additionally deletes its temporary groups/characters. The success trace ends with `cleanup ... reason=proof-complete` and `COMPLETE PASS`.

Vanilla Conflict remains underneath actor eligibility, capture state/timer and ownership. M2 remains the only frontier authority. The operation contains no ownership or frontier setter. FIA authorization remained false in the M2 diagnostics and M3 hard-restricts this operation type to USSR. Existing deployment, radio, supply, base and faction policies were not replaced.

## Acceptance A–Q

| Criterion | Result | Runtime evidence |
|---|---|---|
| A. Target from M2 public query | **PASS** | Revision 4 returned and authorized `OBJ_01_MAIDENS_BAY`. |
| B. Target not hard-coded | **PASS** | Operation stores the value returned by `GetOPFORLegalObjective`; harness supplies only staging offset. |
| C. One ordinary USSR infantry group | **PASS** | Faction defenders prefab, one group, three units. |
| D. Begins outside capture radius | **PASS** | Starting distance 300.006 m; initial capture state 0. |
| E. Physically navigates | **PASS** | Distance series 300.005 -> 249.516 -> 197.912 -> 141.603 -> 85.7193 -> 34.2961 -> 4.42207 m. |
| F. No teleport/reposition simulation | **PASS** | Single vanilla move waypoint; diagnostic explicitly reports `teleport=0 repositionLoop=0`. |
| G. Strategic validation maintained | **PASS** | Campaign, target and authorization checked throughout; revision-change regression cancelled safely. |
| H. AI enters seizing area | **PASS** | Entered `CAPTURING`, reached 34.2961 m and then 4.42207 m. |
| I. Vanilla Conflict starts capture | **PASS** | Capture state changed 0 -> 2. |
| J. Ownership US -> USSR | **PASS** | Vanilla faction-change callback recorded exactly that transition. |
| K. M2 frontier retreats | **PASS** | M2 logged frontier 1 -> 0 and published revision 5. |
| L. Operation observes result and succeeds | **PASS** | `CAPTURING -> SUCCEEDED` occurred only after revision 5/public semantic change. |
| M. No direct M3 ownership/frontier assignment | **PASS** | Runtime path is waypoint -> vanilla capture -> faction callback -> M2 publication; implementation contains neither mutation. |
| N. Strategic-invalidation cancellation | **PASS** | Legitimate OBJ_02 capture changed target at revision 3; operation cancelled without retarget. |
| O. Group destruction cleanup | **PASS** | Zero agents produced `FAILED`, stable owner/revision, and cleanup. |
| P. Scoped/released activation lease | **PASS (not acquired)** | Natural LOD was 0, so no lease was needed; terminal state shows none held. The fallback is group-scoped and releases on all terminal paths. |
| Q. FIA excluded | **PASS** | Operation creation is USSR-only; FIA public attack authorization remained false. |

## Validation history and remaining unknowns

The actual ServerDiag script compiler accepted the final source. One initial pre-world parser error and one failed staging serialization were corrected before acceptance. A subsequent run proved the complete runtime path but exposed a proof-only assertion that incorrectly expected no OPFOR objective after retreat; production correctly exposed the MOB. The final clean run used the corrected semantic expectation and passed.

Unknown beyond M3.1: natural LOD may differ with players, longer distances, larger populations or other server conditions. The scoped lease branch consequently remains implemented but not runtime-exercised because the accepted group naturally remained at LOD 0. Network observation of M3 operations was not in scope, and the existing M2 external-client debts remain unchanged.

## Recommended M3.2 scope

Define a minimal server-authoritative operation coordinator that owns at most one active conventional operation, creates a new operation only from the current public frontier contract, and handles lifecycle registration/cleanup. Keep scheduling, scoring, waves, vehicles, economy and persistence out until that ownership boundary is proven.
