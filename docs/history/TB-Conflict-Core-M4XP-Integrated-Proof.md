# M4.XP full-native integrated authority proof

Status: **INTEGRATED DEDICATED AUTHORITY PASS WITH M3 HUMAN-PENDING SUBCRITERIA**.
Phase 1 is closed/PASS from the Creative Director's post-map-cleanup
unrestricted dedicated smoke. Do not rerun it as a proof prerequisite.

## Isolation and ownership contract

The production world and mission remain
`Worlds/Production/Arland/TB_CC_Arland.ent` and
`Missions/Production/TB_CC_Arland.conf`. Production remains proof mode 0,
300/300-second holds, M3.2/M3.6 enabled, with no proof actor. Six thin proof
worlds under `Worlds/Test/M4XP` inherit the production world and add exactly
one `TB_CC_M4XP_IntegratedProofComponent`; six proof missions under
`Missions/Test/M4XP` select those worlds. The single controller is
`Scripts/Game/TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c`.

All proof transitions use physical faction actors and the vanilla seizing
lifecycle. The controller has no `SetFaction` or direct capture-completion
call. It counts `CaptureFinish` events, physical captures and maximum proof
actors, checks revision/depth/deployment, and cleans up its actors. Terminal
`PASS` still requires checking the campaign's separate
`[TB_CC_M4_4] vanilla-end-state ... count=1` line exactly once; the proof
controller cannot read that private bridge latch.

| Mode | Scope | Expected terminal classification |
| --- | --- | --- |
| 1 | Alternate lock, legal ordinary capture, timer interruption/retry, revision +1 and 1/11 depth | PASS/FAIL |
| 2 | Physically progress to dynamically selected unselected HQ and capture it as ordinary territory | PASS/FAIL |
| 3 | Physically progress to Gull Island, capture it, then test transport-sensitive foot-staging denial | PASS/FAIL |
| 4 | Genuine first US capture, M3 response target, garrison/coordinator observation | May be BLOCKED by real-human observer gate; no participant override |
| 5 | Physical 11/11, EARLY/MID/LATE depth, BLUFOR pressure/cancel/restart/expiry | PASS/FAIL |
| 6 | Fresh 0/11 gate, genuine foothold/retreat, OPFOR pressure/cancel/restart/expiry | PASS/FAIL |

The pressure modes use the production 300-second hold, not a shortened hold.
This avoids a proof-only mutation of the production campaign component.
Mode 4 deliberately does not fabricate human participants: if M3.6/M3.2
cannot create a force without a real observer, the missing subcriterion is
HUMAN-PENDING rather than a synthetic PASS.

## Mandatory Workbench gate before runtime

Open BCM Conflict Core in normal Workbench, compile Game scripts, and open/save
each of the six thin worlds
`Worlds/Test/M4XP/TB_CC_M4XP_Proof_1.ent` through
`TB_CC_M4XP_Proof_6.ent`. Confirm each inherits the full native production
world and has one proof controller with the matching `m_iMode`. Process the
six mission resources in `Missions/Test/M4XP`. Do not infer a compile or
resource pass from PowerShell syntax/static checks.

After that gate, run one mode at a time in a normal unrestricted PowerShell
session, for example:

```powershell
& 'C:\Users\ruanb\Documents\My Games\ArmaReforgerWorkbench\addons\BCM Conflict Core\Tools\Run-M4XP-IntegratedProof.ps1' -Mode 1
```

The runner clones the validated M4.XP JSON into a timestamped evidence
directory outside the addon tree, changes only the proof mission and isolated
ports, then launches ServerDiag from its install directory with `-config`
and `-addonsDir` (never CLI `-addons`). It stops only its own PID at terminal
proof output or timeout. `-DryRun` creates no profile, config or process.
Do not execute modes until the Workbench gate is confirmed.

## Acceptance state

The Creative Director completed the real Workbench compile/resource gate for
all six proof worlds and missions. Each mode then ran on a fresh unrestricted
ServerDiag profile under
`C:\Users\ruanb\Documents\BCM-Conflict-Core-TestEvidence`.
No restricted-backend launch, duplicate profile or second client was used.

| Mode | Result | Physical captures | Final revision/depth | Evidence directory | Distinct evidence |
| --- | --- | ---: | --- | --- | --- |
| 1 | PASS | 1 | 2 / 90 | `M4XP_Proof_Mode1_2026-10-01_17-34-52` | Moss Hill locked; Military South timer started, interrupted with ownership/revision stable, retry `CaptureFinish`, USSR→US callback, foothold true, next legal Airport North |
| 2 | PASS | 3 | 4 / 272 | `M4XP_Proof_Mode2_2026-10-01_17-37-20` | Military South → Airport North → dynamically unselected HQ West; HQ West captured as ordinary, no terminal result |
| 3 | PASS | 11 | 12 / 1000 | `M4XP_Proof_Mode3_2026-10-01_17-38-47` | Gull Island physical `CaptureFinish` and callback; transport trait true; foot staging denied `unsupported-transport-requirement`, no teleported attacker |
| 4 | HUMAN-PENDING | 1 | 2 / 90 | `M4XP_Proof_Mode4_2026-10-01_17-41-05` | Genuine Military South capture produced OPFOR response target and new frontline; `humansUS=0`, so M3.6 logged `no-active-human-blufor` and no physical garrison/operation was fabricated |
| 5 | PASS | 13 | 15 / 1000 | `M4XP_Proof_Mode5_2026-10-01_17-43-06` | Early 90, mid 545, late 909, full 1000; BLUFOR pressure → genuine USSR cancel → genuine US restoration → fresh 300-second timer → BLUFOR victory |
| 6 | PASS | 4 | 6 / 0 | `M4XP_Proof_Mode6_2026-10-01_17-50-50` | Fresh 0/11 no pressure; US capture sets foothold; USSR recapture starts OPFOR pressure; US breakout cancels; USSR restoration starts fresh 300-second timer; OPFOR victory |

Every terminal proof line reports `directGameplayOwnershipMutations=0` and
`maximumProofActors=30`. The combined physical-capture count is 33 across
six **independent clean sessions**, not one campaign. All recorded ownership
changes came through vanilla `CaptureFinish` and campaign callbacks. Each
capture also passed the proof's deployment check: only the selected MOB
remained enabled. Custom Beauregard Harbour, Signal Hill, North Runway and
Gull Island all resolved with USSR owner, seizing component and disabled
deployment in the proof world's initial state.

Mode 5's revision path was 1→12 at 11/11, 13 after USSR recapture, 14 after
US restoration, 15 at terminal expiry. Mode 6 was 1→2→3→4→5→6 with depth
0→90→0→90→0. Both pressure modes logged exactly one
`[TB_CC_M4_4] vanilla-end-state ... count=1` bridge in their separate logs,
and duplicate terminal callbacks left their revisions/outcomes stable. Both
selected MOB ownerships remained US; no MOB capture was required. The proof
logs contain zero script-error or strategic INVALID lines.

Mode 4 is a deliberate **HUMAN-PENDING** boundary, not an M3 failure. M3.2
initialized and the legal OPFOR response target became Military South.
M3.6 observed Airport North as the new frontline, but `humansUS=0` and
`observers=0` correctly deferred garrison creation. Consequently physical
garrison package/groups and coordinator selection/staging/assembly/movement
remain for the genuine one-client session; no participant override was used.

Production was verified unchanged: proof mode 0, 300/300 holds, coordinator
and garrison enabled, no proof actor or diagnostic ownership state. A single
final production smoke used the separate free port pair UDP 2222/17796,
profile `M4XP_Phase1_2026-10-01_17-58-48`, and the production mission. It
entered online game state with 12 positions/21 edges, one enabled HQ East
MOB, 0/11 US ordinary control, foothold false, ACTIVE, no pressure/deadline,
and M3 initialized. The preceding port-2215 preflight refused an occupied
port **without launching** a server. The owned final-smoke PID 29988 was
stopped afterward; unrelated server PIDs were untouched.

The full-native candidate is ready for the final **human** architectural
gate, not yet for replacing the old production foundation. The prepared
`Tools/Run-M4XP-HumanAcceptance.ps1` uses one ServerDiag authority and one
visible SteamDiag client, fresh separate profiles, the production mission,
and the previously accepted local launch timing. Its syntax and dry-run
passed. It has **not** been executed. Network N1–N5 remains separate debt.
