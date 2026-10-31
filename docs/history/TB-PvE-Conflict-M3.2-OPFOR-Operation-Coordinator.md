# TB PvE Conflict M3.2 — Single-Operation OPFOR Coordinator

## Verdict

**M3.2 PASS.** A clean dedicated ServerDiag authority session proved a single server-owned coordinator can wait for a legal OPFOR objective, respect configured delay, create and own at most one M3.1 operation, recover through failure and cancellation cooldowns, observe an autonomous vanilla capture success, and create a later operation from fresh M2 state. Invalid campaign state cancelled the active operation and suspended further creation.

Runtime evidence:

`TestProfiles/M3_2_Authority_1/logs/logs_2026-09-29_13-25-35/console.log`

The bounded proof switch is returned to `0`. The production coordinator remains enabled. M2.4 external-client route acceptance and M2.5 outcome/JIP network verification remain the previously documented debts and were not reopened.

## Coordinator responsibility and states

`TB_PVE_M3_2_OPFOROperationCoordinator` is authority-only and owns:

- the decision that zero or one conventional USSR operation should exist;
- initial waiting delay and post-terminal cooldown;
- creation and reference ownership for one `TB_PVE_M3_1_Operation`;
- observation of terminal state;
- requesting final operation disposal and clearing the reference;
- re-querying current M2 state before every later operation.

It does not select among targets, inspect route data, issue movement orders, evaluate capture, assign ownership, or move the frontier.

The coordinator states are:

`NOT_READY`, `IDLE`, `WAITING`, `OPERATION_ACTIVE`, `COOLDOWN`, and `SUSPENDED`.

Observed lifecycle included:

`IDLE -> WAITING -> OPERATION_ACTIVE -> COOLDOWN -> IDLE -> WAITING`

and, when the campaign became invalid:

`OPERATION_ACTIVE -> SUSPENDED`.

## Coordinator/operation ownership boundary

The coordinator creates the operation, retains the sole active reference, observes its terminal state, calls the operation's disposal method, and clears its reference. It never deletes an operation waypoint, group, character, or lease directly.

The reused M3.1 operation owns target acquisition/validation, authorization revision, assigned group and characters, waypoint, physical movement, capture-result observation, terminal transition, waypoint/lease cleanup, and force disposal. This preserves a single resource owner while allowing the coordinator to request disposal after a terminal result.

## Public M2 contract

The coordinator consumes only:

- `GetReadiness()`;
- `GetCampaignValidity()`;
- `GetCampaignOutcome()`;
- `GetRevision()`;
- `GetOPFORLegalObjective()`;
- `CanFactionStrategicallyAttack("USSR", target)`.

It reads no raw replicated field, lock mask, frontier index, route array, base component or route controller state. At delay expiry it re-queries outcome, target and authorization, then constructs M3.1 without passing a target. M3.1 independently acquires and records its target through the same public contract.

## Configuration

The production route exposes only:

- `m_bEnableM3_2Coordinator = 1`;
- `m_iM3_2InitialAttackDelaySeconds = 45`;
- `m_iM3_2AttackCooldownSeconds = 6`;
- the already required M3.1 staging offset, `0 0 300`.

`m_bRunM3_2Proof` is a bounded acceptance switch and is returned to `0`. No probability, budget, wave, difficulty, player-scaling, vehicle or economy configuration was added.

## No-target and waiting behavior

At revision 1 the campaign was ACTIVE but had no legal OPFOR objective. The coordinator entered `IDLE`, created no operation, and the proof logged `no-target-proof pass=1 ... arbitraryAttack=0`.

Vanilla setup capture first exposed OBJ_01. The coordinator began a 45-second wait. Before it expired, a second legitimate capture advanced the public target to OBJ_02 at revision 3. The coordinator discarded the pending OBJ_01 authorization and restarted the full delay for OBJ_02.

For the later successful leg, WAITING began for OBJ_03 at `13:30:03.276`; operation creation occurred at `13:30:49.331`, approximately 46.1 seconds later with the two-second coordinator tick resolution. The configured 45 seconds was therefore respected.

## Failure recovery

The coordinator created `M3_2_OP_1` against public target OBJ_02 at revision 3. It owned one new three-unit group at natural LOD 0 and began physical movement. The proof then removed the assigned group. M3.1 detected zero agents and entered `FAILED`.

The coordinator observed terminal state 6, requested operation cleanup, cleared the active reference, set active count to zero and entered the six-second cooldown. It did not reuse the failed operation or group. Cooldown completion re-queried revision 3 and began a new waiting cycle.

## Cancellation recovery

After cooldown and a fresh delay, `M3_2_OP_2` was created independently against OBJ_02 at revision 3 with a new group. The proof caused a legitimate vanilla US capture of OBJ_03. M2 published revision 4 and changed the legal OPFOR objective to OBJ_03.

The existing operation did not retarget. M3.1 entered `CANCELLED`, the coordinator observed state 7, disposed it, cleared its reference and entered cooldown. After cooldown it queried the new public target and waited for OBJ_03.

## Autonomous success

`M3_2_OP_3` was coordinator-created at revision 4 with target OBJ_03. The proof did not supply the target, create the operation, issue its order or cause its capture.

The one three-unit USSR group spawned 300.134 m away at natural LOD 0 and received the vanilla move waypoint. Recorded distances were 300.141, 246.241, 190.585, 138.748, 88.2661 and 37.1779 m, with a final minimum of 4.04559 m. The operation transitioned through MOVING, ASSAULTING and CAPTURING.

Vanilla changed OBJ_03 ownership US to USSR. M2 independently retreated the frontier, published revision 5 and exposed OBJ_02 as the current OPFOR objective. M3.1 observed that campaign result and entered `SUCCEEDED`. The coordinator observed terminal state 5, cleaned the reference and entered cooldown.

The success cooldown began at `13:34:47.615` and completed at `13:34:53.608`, approximately 6.0 seconds later.

## Fresh second operation after success

After cooldown the coordinator re-queried revision 5, observed current target OBJ_02 and waited another full delay. It created `M3_2_OP_4` with:

- target OBJ_02 rather than previous OBJ_03;
- authorization revision 5 rather than revision 4;
- a new three-unit group;
- natural LOD 0;
- fresh physical movement from 300.185 m.

This proves lifecycle recycling without stale target, revision, group or state. The second post-success operation did not need to complete capture for M3.2.

## No-duplicate proof

Every creation log reported `activeCount=1 maximumConcurrent=1`. Update ticks, ownership revisions, terminal handling, cooldown callbacks and proof observations never created a concurrent operation. Four sequential operations were created across the complete recovery/success proof; maximum simultaneous coordinator ownership remained exactly one.

## Invalid/terminal suspension

While the fourth operation was moving, the established M2 diagnostic published revision 6 with validity `INVALID_NON_CONTIGUOUS`, outcome `INVALID`, empty legal objectives and false attack authorization. The active M3.1 operation cancelled; the coordinator disposed it, cleared the reference and transitioned to `SUSPENDED`.

The proof held invalid state for multiple ticks and recorded no new operation. The coordinator was then stopped, the diagnostic override removed, and M2 reconstructed its real ownership projection. Terminal victory outcomes follow the same `outcome != ACTIVE` branch, so no cooldown can create another operation while BLUFOR_VICTORY, OPFOR_VICTORY or INVALID is current.

## AI LOD, FIA and cleanup

All four coordinator-created groups naturally spawned at LOD 0 while maximum LOD was 10. No activation lease was acquired or required. The existing scoped M3.1 fallback was not exercised or expanded.

The coordinator hard-codes only the conventional operation faction key `USSR` when checking authorization. FIA is never an owner, target-driving faction or fallback, and public diagnostics continued to show FIA authorization false.

Terminal operation disposal logged waypoint/lease termination, group cleanup and active-reference clearing. Final coordinator cleanup reported `activeReference=0`, `maximumConcurrent=1`, `created=4`.

## Acceptance A–S

| Criterion | Result | Dedicated runtime evidence |
|---|---|---|
| A. Server-authoritative coordinator | **PASS** | Initialized only on authority; dedicated session reported server coordinator startup. |
| B. Public M2 contract only | **PASS** | All observed decisions used readiness, validity, outcome, revision, public legal target and authorization. |
| C. Never more than one operation | **PASS** | Maximum concurrent count remained 1 across four sequential operations. |
| D. Valid campaign enters WAITING | **PASS** | Legal OBJ_01/02/03 targets each caused `WAITING`. |
| E. Configured delay respected | **PASS** | 45-second setting produced approximately 46.1 seconds with two-second tick resolution. |
| F. Creates M3.1 without harness target | **PASS** | Harness supplied no target or creation call; operation and coordinator independently queried M2. |
| G. Autonomous attack | **PASS** | OP_3 moved 300.134 m, entered capture and completed vanilla ownership change. |
| H. Terminal state observed | **PASS** | Coordinator observed FAILED, CANCELLED and SUCCEEDED states. |
| I. Cooldown entered | **PASS** | Every terminal result produced `COOLDOWN`; success cooldown measured approximately 6.0 seconds. |
| J. Active reference cleaned | **PASS** | Every terminal disposal logged `active-reference-cleared activeCount=0`. |
| K. Current M2 state re-queried | **PASS** | Cooldown completion logs include current revision and `requery=1`. |
| L. Fresh second operation | **PASS** | After success, OP_4 used OBJ_02/revision 5/new group instead of OBJ_03/revision 4. |
| M. Failure recovery | **PASS** | Destroyed OP_1 failed, cleaned, cooled down and was followed by fresh OP_2. |
| N. Cancellation recovery | **PASS** | OP_2 cancelled after legitimate revision 4 target change; fresh OP_3 used OBJ_03. |
| O. Terminal/invalid suspension | **PASS** | INVALID revision 6 cancelled OP_4, cleared it and held `SUSPENDED` without creation. |
| P. No-target safety | **PASS** | ACTIVE revision 1 with no OPFOR target remained IDLE and created nothing. |
| Q. FIA excluded | **PASS** | Only USSR is queried/created; FIA authorization remained false. |
| R. No ownership/frontier mutation | **PASS** | Coordinator contains no ownership/frontier setter; success was waypoint -> vanilla capture -> M2 reaction. |
| S. Natural LOD recorded | **PASS** | All coordinator groups reported natural LOD 0, max LOD 10, lease unused. |

## Remaining unknowns

- Operation LOD under player presence, denser populations and longer route distances remains untested; no lease requirement appeared here.
- M3.2 is authority-only. It does not add replicated coordinator UI or reopen the existing external-client networking debt.
- The fixed staging offset is intentionally still M3.1/M3.2 proof-scale and is not intelligent spawn selection.
- Long-term scheduling balance, force availability and strategic prioritization remain outside this milestone.

## Recommended M3.3 scope

Add a narrow operation spawn/staging policy that resolves a safe, navigable, non-visible staging location for the current public target while retaining the single-operation coordinator and existing M3.1 lifecycle. Do not add multiple operations, waves, budgets, vehicles or strategic scoring in that slice.
