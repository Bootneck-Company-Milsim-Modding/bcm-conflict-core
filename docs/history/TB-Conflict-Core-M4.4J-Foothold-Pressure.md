# M4.4J — foothold history and pressure presentation

**M4.4J AUTHORITY ACCEPTANCE PASS.** Focused A–H, both terminal directions,
presentation-state projections and extended final production smoke PASS.
Genuine human visual acceptance and network N1–N5 remain PENDING. Overall
formal M4.4 is not closed while human criterion U and these debts remain.

## Root cause and historical evidence boundary

Two clean human sessions confirmed that initial USSR control of all ten
ordinary positions incorrectly began OPFOR pressure, suspended M3 and ended
the fresh campaign after 300 seconds. The previous Mode 10 proof demonstrated
pressure/cancellation/expiry mechanics but assumed the wrong entry condition.
Its logs remain preserved; its fresh-campaign prerequisite is superseded here.
The previous production smoke's immediate OPFOR pressure is a defect, not a
successful production-start acceptance result.

## Existing latch audit (before correction)

`m_bBLUFORAdvanced` is declared on the authoritative Arland campaign component.
The default bool value is false. Its only original write was `true` inside the
legal US ownership callback. It had no reads, no reset site, and no replication
or persistence. Reverse USSR ownership removed the current capture-stack entry
but did not clear the flag. Re-evaluation and reconstruction did not modify it.
Client components never subscribed to ownership changes or initialized authority.
The observable original semantics were a historical "US has advanced" latch,
distinct from the capture stack that tracks currently surviving US captures.
There is no recorded separate design rationale beyond these source semantics.

The same flag is reused, not replaced. An explicit fresh campaign initialization
sets it false. A completed legal non-HQ ownership transition from non-US to US
sets it true. Same-owner duplicate callbacks cannot establish it. It remains
true through retreat, cancellation, reevaluation, player lifecycle and terminal
state. Its read-only getter supports acceptance; its replicated projection
exposes the history fact without transferring authority to clients.

## Entry and timing contract

Fresh US ordinary=0 / USSR ordinary=required / foothold=false is ACTIVE, with
no pressure deadline, no siege hook and operational M3 consumers. OPFOR pressure
requires foothold=true AND US ordinary=0 AND USSR ordinary=required. Required
count is derived from the registry, excluding the permanent HQ anchors; no
ten-position constant is used in the gate. BLUFOR total-control pressure is
unchanged. Both production holds remain 300 seconds.

Authority owns `WorldTimestamp` start/deadline and the expiry callback. Remaining
seconds are ceiling-rounded from deadline minus current server timestamp.
Cancellation removes the expiry callback and clears start/deadline. A restart
creates a new full-duration deadline; terminal publication clears presentation
before the vanilla end-match bridge.

## Generic presentation and reconstruction

`TB_CC_VictoryPressurePresentation` has no Arland IDs or ownership/gameplay
writes. It creates one non-input-intercepting text overlay in graphical local
authority and client sessions; dedicated servers create no widgets. It displays
MOB UNDER SIEGE with the recapture instruction, or TOTAL TERRITORIAL CONTROL
with the hold instruction, plus MM:SS. It removes the overlay for ACTIVE,
INVALID, terminal and teardown states.

The vanilla replicated game mode carries outcome, configured duration,
foothold and remaining time. The server publishes current deadline-derived
remaining time every second and immediately on semantic transitions. Clients
do not start a five-minute timer or determine winners. Current RplProp state
is intended to initialize JIP with actual remaining time rather than 05:00.
Visual refresh polls at 250 ms and replication callbacks refresh immediately.
This is a bounded authoritative countdown sampling contract, not exact
subsecond client clock synchronization. Network latency/reordering/JIP behavior
still requires N1–N5 runtime acceptance; no network PASS is claimed here.

The public graph query's countdown getter reads this current timing projection
rather than the old frozen semantic-snapshot remaining value. The existing
graph projection/strategic revision is not incremented once per timer second.

## Persistence boundary

This is runtime campaign history and current-state reconstruction, NOT saved
campaign persistence. No partial save system was introduced. Future persistence
must jointly restore selected MOB/start state, ownership, revision, foothold,
active pressure and deadline/remaining semantics. A new server campaign starts
false; an eventual restored established campaign at US=0 must restore true.

## Historical M1 diagnostic

The missing `TB_M1_3_FORWARD_Base` assertion came from the legacy M1 fixture
harness, whose suppression recognized only the M2 linear controller. It now
returns when the authored M4 graph campaign system entity is present, including
before delayed graph initialization. No historical fixture was added, no
spawn-point restriction was removed and deployment enforcement is unchanged.

## Acceptance evidence

All profiles below are beneath
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/`, outside the addon.

| Run | Result / evidence |
| --- | --- |
| M4_4J_CompileSmoke_2026-10-01 | Initial new-code compile failed on modulo applied to the float result of `Math.Max`; root corrected by clamping the integer before modulo. Cascade errors were not patched. Failed log preserved. |
| M4_4J_CompileSmoke_Fixed_2026-10-01/logs/logs_2026-10-01_12-31-33/console.log | Native Game compilation/startup PASS. 12:31:58 ACTIVE, foothold=0, remaining=-1, deadline absent, siegeHook=0, M3 operational. |
| M4_4J_OPFOR_Mode10_2026-10-01/logs/logs_2026-10-01_12-32-10/console.log | Corrected physical Mode 10 PASS: four captures, pressure starts twice, OPFOR expiry once, bridge once, terminal revision 6 stable under duplicate callbacks. |
| M4_4J_BLUFOR_Mode9_2026-10-01/logs/logs_2026-10-01_12-35-19/console.log | Physical Mode 9 PASS: twelve captures, pressure starts twice, BLUFOR expiry once, bridge once, terminal revision 14 stable. Final presentation assertions prove visible pressure semantics and cleared cancellation/terminal semantics. |
| M4_4J_OPFOR_FinalProjection_2026-10-01/logs/logs_2026-10-01_12-40-14/console.log | Final Mode 10 PASS: four captures, both OPFOR pressure projections, breakout clearing, terminal clearing, two starts, one expiry/bridge, stable revision 6. COMPLETE PASS 12:42:49.943. |
| M4_4J_FinalProductionSmoke_2026-10-01/logs/logs_2026-10-01_12-44-15/console.log | PASS. Production proof=0, holds=300/300, M3 enabled. READY 12/21 at 12:44:40.259; ACTIVE/foothold=0/no deadline/siegeHook=0. Verified alive at 12:50:46, over six minutes after initialization: zero pressure entries, POSTGAME events, proof starts, SCRIPT(E), duplicate GUID warnings or metafile errors during the active session. Task-owned PID 20712 stopped afterward. |

### Focused A–H

| Criterion | Evidence |
| --- | --- |
| A fresh start | PASS in every clean run: ACTIVE, foothold=0, no timer/start count, both M3 consumer instances present. |
| B first capture | PASS: genuine USSR→US Military South ownership completed; latch set once; US=1, depth=100, ACTIVE. |
| C retreat | PASS: genuine US→USSR completion; latch remains true; US=0; one OPFOR deadline/start and siegeHook=1. |
| D breakout | PASS: genuine USSR→US; ACTIVE, remaining=-1/deadline cleared, latch true, M3 resumed. Presentation projection not visible. |
| E second retreat | PASS: genuine US→USSR; second start, new 60-second proof deadline. Production duration untouched. |
| F BLUFOR total control | PASS: all ten genuine US captures; depth=1000; BLUFOR deadline/title projection. |
| G BLUFOR interruption | PASS: genuine USSR Gull Island recapture; immediate ACTIVE/cancel and hidden presentation projection. |
| H BLUFOR restoration | PASS: genuine US Gull Island recapture; new full-duration deadline and visible BLUFOR projection. |

Mode 9 reached first pressure at 12:38:29.832, cancellation at
12:38:42.982, fresh pressure at 12:38:49.914, expiry at 12:39:49.931,
and COMPLETE PASS at 12:39:51.916. The countdown projection was 00:59,
then 00:50/00:40/…; terminal projection was hidden with remaining=-1.
OPFOR first run likewise sampled 50/40/30/20/10 from its deadline; expiry
at 12:34:44.877 published hidden terminal projection before the single bridge.

Final OPFOR projection run: fresh 12:40:43.609; first foothold capture
12:41:01.651; retreat pressure 12:41:17.159; breakout cancellation
12:41:32.892; new full deadline 12:41:48.941; expiry 12:42:48.957;
terminal projection at 12:42:49.940 and stable reconstruction at 12:42:49.942.
Each run had one terminal expiry and one bridge, with both HQ owners unchanged
and legal objectives empty/captures frozen. All ordinary spawns remained off
and only selected HQ East MOB remained deployment-enabled.

Two `SCR_BaseResupplySupportStationComponent needs a entity catalog manager!`
SCRIPT(E) lines occur in the final OPFOR log at 12:44:04.425/.430 during world
destruction, after COMPLETE PASS and while the task-owned process was being
stopped. They are not compiler errors or active-proof BCM errors. They are
preserved and reported, not silently counted as a globally error-free log or
"fixed" by modifying vanilla content. Initial compile-failure logs and both
successful OPFOR runs are likewise retained.

### Overall M4.4 evidence reconciliation

The existing A–AE matrix is retained with historical evidence explicitly
bounded. A/AE production-start interpretation is superseded by the fresh
foothold gate and final smoke here; Z terminal-pressure evidence now includes
the corrected full Mode 10 prerequisite. Prior physical graph/capture, HQ,
deployment, INVALID and mainland M3 evidence is not erased. Criterion U
remains PENDING HUMAN; this task adds a separate genuine-human pressure-visual
check. N1, N2, N3, N4 and N5 remain individually PENDING. Therefore this is
focused M4.4J authority acceptance, not formal human/network milestone closure.

The final source is compiled by native ServerDiag, not claimed to be a new
manual Workbench compile confirmation. Installed Enfusion API documentation
was checked for widget creation, font/text APIs and timestamp arithmetic.
The vanilla default28 font exists in the installed resource database; no
custom UI resource or new GUID was introduced. Source brace checks pass.

Final production serialization was checked: proof 0, holds 300/300, coordinator
1, garrison 1, selected HQ East; graph and strategic placements untouched.
Only task-owned diagnostic servers were stopped. No human client, SteamDiag
authentication attempt, Workbench automation, persistence system, siege AI,
balance change, M4.5 implementation or commit was performed.

Server-side presentation projection is not genuine human visual acceptance.
The Creative Director must recompile/open the world normally and check actual
overlay readability, countdown, cancellation, terminal clearing and clean
fresh-start absence. Network N1–N5 remain individually PENDING; no client was
launched and no authentication investigation was performed.
