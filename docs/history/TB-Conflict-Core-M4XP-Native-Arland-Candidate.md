# M4.XP — Native Arland production candidate

Current directive update (2026-10-02): THREE-FACTION FIRST IMPLEMENTATION /
WORKBENCH GATE REQUIRED. Two permanent physical conventional HQs plus ten
initial FIA ordinary positions supersede the previous eleven-ordinary model.
Pressure unlocks the enemy HQ; only genuine HQ capture wins. See the Section 98
report in the native foundation document. No runtime/publish/commit this pass.
Creative Director's verified native combat foundation remains accepted; the
new campaign policy and integration are not runtime accepted yet.

Current status (2026-10-02): **NATIVE FOUNDATION INTEGRATION / WORKBENCH GATE REQUIRED**.
Prior integrated dedicated PASS evidence below remains historical evidence for
the previous saved foundation, not acceptance of the newly authored stock-mode
world. See [native foundation audit and integration](TB-Conflict-Core-M4XP-Native-Foundation-Integration.md)
for the authoritative current inventory, changes and next gate. M3 real-human
combat remains HUMAN-PENDING. The old M4.4
world and the three-node M4.X fixture remain separate and unmodified. Do not
replace the production foundation on the basis of this draft.

## Resources and foundation

- World: `Worlds/Production/Arland/TB_CC_Arland.ent`, GUID `A4F3300000000000`.
- Mission: `Missions/Production/TB_CC_Arland.conf`, GUID `A4F3500000000000`.
- Layers: `Worlds/Production/Arland/TB_CC_Arland_Layers/A_Managers.layer`,
  `B_MOB.layer`, `C_CapturePoints.layer`; Creative Director's saved organization.
- Game mode: stock `Prefabs/MP/Modes/Conflict/GameMode_Campaign.et` /
  `SCR_GameModeCampaign`, with scenario-scoped BCM startup/victory/task seams.
  Old `TB_CC_M4XP_GameMode.et` is retained for compatibility, not selected by
  the current production layer. BCM's inherited campaign component still owns
  strategy, projection, pressure, M3 lifecycle and deployment policy.
- HQ candidates: `Prefabs/Systems/MilitaryBase/TB_CC_ArlandHQCandidate.et`,
  a narrow derivative of vanilla `ConflictBase_MOB.et`, with seizing enabled and
  spawn initially disabled. Exactly one gets US faction and `SetAsHQ(true)`
  during clean startup. The other remains USSR owned, seizable and nondeployable.
- Ordinary positions use the existing `TB_CC_StrategicBase.et`; the four M4.4I
  custom sites use their accepted `TB_CC_M4_4I_StrategicCapture.et` architecture.
- Standalone parent is `worlds/Arland/Arland.ent`. The complete
  `CTL_Campaign_Arland`/`CTI_Campaign_Arland.ent` scenario hierarchy is absent.
  The previously justified vanilla Arland AI navigation files retain CTI names;
  those are baked data assets, not scenario inheritance.
- World services: vanilla RadioManager, CampaignLoadoutManager,
  CampaignFactionManager, Arland AIWorld with soldier/vehicle navmesh, and
  inherited Arland terrain/environment/MapEntity. Game-mode components provide
  respawn, deployment, supply, tasks and replication. No duplicate manager is
  authored. The inherited map entity is known to warn about terrain-size
  fallback; rendered minimap/deployment behavior requires human inspection.

## Physical positions

Coordinates are saved authoring transforms. The first eight come from the
accepted original physical base positions also logged by M4.4I. Signal Hill,
North Airfield and Gull Island use the final accepted physical layer transforms.
Beauregard Harbour uses the Creative Director's explicit final moved coordinate.

| Stable ID | Physical centre (x, y, z) | Native entity |
| --- | --- | --- |
| ARLAND::HQ_EAST | 1621.770, 4.045, 902.667 | TB_CC_NATIVE_HQEast |
| ARLAND::HQ_WEST | 1157.670, 1.156, 3282.460 | TB_CC_NATIVE_HQWest |
| ARLAND::MOSS_HILL | 2292.760, 89.208, 1223.620 | TB_CC_NATIVE_MossHill |
| ARLAND::MILITARY_SOUTH | 1413.430, 10.959, 1857.550 | TB_CC_NATIVE_MilitarySouth |
| ARLAND::ARLEVILLE | 2727.070, 77.531, 1680.720 | TB_CC_NATIVE_ArlevilleBase |
| ARLAND::BEAUREGARD | 3297.470, 2.000, 2860.180 | TB_CC_NATIVE_BeauregardBase |
| ARLAND::FARM | 2208.910, 53.814, 2909.470 | TB_CC_NATIVE_Farm |
| ARLAND::AIRPORT_NORTH | 946.642, 38.240, 2895.520 | TB_CC_NATIVE_AirportNorth |
| ARLAND::BEAUREGARD_HARBOUR | 3112.440, 44.020, 2419.688 | TB_CC_NATIVE_TB_CC_BeauregardHarbour |
| ARLAND::SIGNAL_HILL | 2527.110, 144.445, 2286.730 | TB_CC_NATIVE_TB_CC_SignalHill |
| ARLAND::NORTH_RUNWAY | 1675.290, 37.915, 3017.612 | TB_CC_NATIVE_TB_CC_NorthRunway |
| ARLAND::GULL_ISLAND | 962.309, 31.444, 1395.995 | TB_CC_NATIVE_TB_CC_GullIsland |

## Reused campaign policy

`TB_CC_M4XP_ArlandNativeDefinition.Create()` copies the accepted M4.2
definition's 12 stable IDs, types, importance, traits and **all 21 existing
edges**, changing only map identity and native entity names. Edge changes: **0**.
The accepted undirected edges are:

1. HQ West—Airport North
2. HQ West—Farm
3. Airport North—Farm
4. Airport North—Military South
5. Farm—Beauregard
6. Farm—Arleville
7. Farm—Military South
8. Beauregard—Arleville
9. Arleville—Military South
10. Arleville—Moss Hill
11. Military South—Moss Hill
12. Moss Hill—HQ East
13. Military South—HQ East
14. Beauregard Harbour—Beauregard
15. Beauregard Harbour—Arleville
16. Signal Hill—Farm
17. Signal Hill—Arleville
18. Signal Hill—Beauregard Harbour
19. North Airfield—Airport North
20. North Airfield—Farm
21. Gull Island—Military South

The same M4.2A resolver chooses HQ East/West from the campaign seed or explicit
selection. Its physical endpoint relationship is used only to choose a
candidate; the resulting native start state has an empty physical endpoint and
the metadata-only origin `ARLAND::OFF_MAP_OPFOR`. The old five-connected-base
endpoint readiness threshold belongs to the former protected enemy HQ and is
not applied to this now-ordinary HQ; normal adjacency/eligibility still gates
its capture. The unselected HQ therefore
counts as one of **11 ordinary positions**. The origin contributes no node,
ownership, deployment or depth.

The inherited M4.3 evaluator, M4.4 projection/deployment, M4.4H/J pressure and
history, M3.1–M3.6 systems, and M3.7A human query remain shared. Fresh startup
sets the history latch false. One startup ownership assignment selects the US
MOB before strategic readiness; gameplay ownership remains vanilla capture.
All other physical bases start USSR owned. The existing integer depth formula
`(US ordinary * 1000) / required ordinary` will use required=11. No depth
table, player count or force package is hard-coded.

The M4.4E seizing seam now routes bases named `TB_CC_NATIVE_*` through the
same registered graph capture authorization for **both** factions. This is
required because the selected HQ candidate retains a seizing component:
otherwise vanilla's nonplayable USSR radio path could capture it despite graph
denial. Other scenario entity names keep the previous M4.4E path. Physical
presence, prevailing faction, timer, interruption, CaptureFinish and owner
change remain in the same vanilla seizing lifecycle. The startup checks fail
closed unless exactly one spawn is enabled, the selected HQ is US/HQ, all
other positions are USSR/non-HQ, required ordinary count is 11, depth is 0 and
outcome is ACTIVE.

Both candidate HQs inherit a disabled spawn at authoring time. On startup the
shared deployment policy enables only the selected MOB; every other spawn
stays disabled, including the unselected HQ after capture. The six ordinary
base prefab and four custom capture prefab also retain the no-player-deployment
resource. Gull Island keeps its `TRANSPORT_REQUIRED` trait, which the shared
M3 staging policy uses to suppress unsupported foot operations safely.

The unchanged pressure system therefore uses 11/11 for BLUFOR pressure and
foothold-true 0/11 for OPFOR pressure; production holds stay 300/300 seconds.
The existing client overlay and future siege/final-stand semantic hook remain
in place. No siege or other new force is authored.

The six pre-M4.4I ordinary positions use one generic BCM strategic-base
prefab on the native world. This preserves the common seizing/ownership
architecture, but the earlier CTI hierarchy may have used specialized base
prefab variants with different capture radii, services or supply fixtures.
Exact parity of those physical details is **not yet established**. Inspect the
representative base at runtime before declaring the full campaign equivalent;
if a difference affects accepted behavior, adapt that position's BCM prefab
narrowly from the corresponding installed vanilla source. Do not infer parity
from matching strategic IDs or coordinates alone.

## Validation and outstanding gates

Static authoring checks: 12 physical native entity definitions; every accepted
M4.2 ID maps to one native entity; copied edge set has 21 entries; one proof-zero
component; no `CTL_Campaign_Arland` world parent or diagnostic actor. Candidate
resource GUIDs are distinct within the authored files. This is **not** a
Workbench compile or a runtime acceptance claim.

Automated Workbench `-validate` previously failed at process startup with Windows
`0xc0000142`, an invocation/environment failure. The Creative Director then
opened this **full native candidate** in normal interactive Workbench on
2026-10-01: Game scripts compiled (5703 files / 11213 classes), the game
initialized, and `TB_CC_Arland.ent` loaded with terrain and road networks.
The corrected BCM `PrintFormat` error and downstream vanilla `Tuple2` errors
were absent. The Arland minimap texture fallback remains a human presentation
check, not a compile failure.

An existing user-owned ServerDiag process (PID 29232 at inspection) was left
untouched. One isolated full-candidate ServerDiag startup used
`ServerConfigs/M4XP_IntegratedAuthority.json`, UDP 2215/A2S 17789 and a fresh
profile at
`C:\Users\ruanb\AppData\Local\Temp\BCM_M4XP_Integrated_20261001_152804`.
Its `console.log` is under that profile's `logs` directory. The process loaded
vanilla data and the BCM addon, compiled Game scripts (5703 files / 11135
classes in ServerDiag), validated the JSON, and created the game. It did
**not** load the mission or reach online state. The first blocking event was
`[RestApi] TYPE:[GameConfig/List] ENV:[invalid]` with `Curl error=Could not
connect to server`, repeated before scenario startup. This is a backend
startup gate, not evidence of a BCM gameplay defect. The exact owned test
process (PID 5496) was stopped; its logs were preserved. No client or
authentication experiment was run. No later acceptance phase was attempted.

Current integrated results:

| Gate | Status |
| --- | --- |
| Phase 1 compile/resource/online, 12-node graph, one MOB, 0/11 ACTIVE | PASS in unrestricted ServerDiag, including post-cleanup smoke |
| Phase 2 representative physical capture | PASS: vanilla interruption/retry, USSR→US, revision 2, depth 90 |
| Phase 3 unselected HQ, Gull Island and custom-location checks | PASS: HQ West ordinary capture; Gull Island capture and foot-staging denial; custom registrations |
| Phase 4 semantic M3 garrison/coordinator/staging/movement | HUMAN-PENDING: response/frontline derived; physical forces observer-gated at humansUS=0 |
| Phase 5 depth and both 300-second pressure interruption paths | PASS: separate BLUFOR/OPFOR terminal proofs, one bridge each |
| Map/deployment presentation and one real human session | PENDING |
| Network N1–N5 | PENDING separate debt |

**Replacement decision: NO-GO pending final human acceptance.** The old
production M4 world, mission, proof settings and historical three-node
regression fixture remain available. No production migration or M4.5 work
has begun.

## 2026-10-01 continuation: authored map and unrestricted Phase 1

The Creative Director added one `SCR_MapEntity` to the saved native candidate
layer. Static inspection finds exactly one authored map entity at
`2032.622, 34.532, 2029.859`. It references vanilla Arland topology
`worlds/Arland/Arland.topo` and the Arland rasterized satellite background.
The world still directly inherits `worlds/Arland/Arland.ent`, not
`CTL_Campaign_Arland`; the layer still contains 12 native strategic base
entities. The map entity adds no spawn-point component or graph entry. Its
visual/deployment usability remains a human runtime check, including the
known minimap texture fallback.

The restricted ServerDiag backend failure is the previously established
M4.4H execution-containment problem; it is not being rediagnosed or retried
inside restricted automation. `Tools/Run-M4XP-Phase1-Unrestricted.ps1` now
prepares an outside-Codex PowerShell launch using the previously successful
server-install working directory, BCM addon, native mission, fresh profile
outside the addon tree, and reserved UDP game/query ports 2215/17789. It
checks production proof/M3/hold settings and refuses ports already in use;
it never stops unrelated processes. PowerShell syntax and `-DryRun` both pass.
At this preparation point, the real unrestricted command had not been
executed by Codex. The launcher does not start a client or physical proof
actor. The subsequent Creative Director run and Phase-1 result are recorded
below.

The first manual unrestricted invocation stopped before backend or mission
initialization with `-config cannot be used together with addons!`. The
server JSON already declares BCM under `game.mods` with modId
`6A6A5973FA53A5B1`, so no JSON change was needed. The Phase-1 launcher was
corrected to omit CLI `-addons`; `-config` remains the mission/server/mod
authority. CLI `-addonsDir` remains solely as an additional search path for
the local Workbench addon source, consistent with the successful M4.4G
dedicated launch. PowerShell parsing and dry-run command construction were
validated after the correction. No second ServerDiag run was performed by
Codex; the subsequent Creative Director run and Phase-1 result are recorded
below.

## Unrestricted Phase-1 result and inherited MapEntity correction

The Creative Director's next unrestricted ServerDiag run is preserved at
`C:\Users\ruanb\Documents\BCM-Conflict-Core-TestEvidence\M4XP_Phase1_2026-10-01_15-52-36\Server`.
Its log shows the native mission online at 15:52:56, registry READY for
`ARLAND_NATIVE_FULL_01`, 12 positions and 21 edges, selected MOB HQ East,
11 ordinary positions, and gameplay ownership writes 0. Twelve base lines
show HQ East US/HQ/deployable; every other base (including HQ West) USSR,
non-HQ and non-deployable. State is revision 1, ACTIVE, depth 0,
ordinaryUS=0, ordinaryUSSR=11, required=11, foothold=0, pressure=0. The
legal US objective is Military South, with no USSR objective. Deployment
policy reports one enabled US MOB spawn; M3 coordinator/garrison initialized.
M3.6 correctly has no active human observer in this headless run. These are
**Phase-1 strategic startup PASS** results, not physical-capture results.

The same log identifies a resource/presentation duplicate at 15:52:54:
the inherited `MapEntity1` from vanilla
`Prefabs/World/Game/Arland/MapEntity_Arland.et` and a second BCM-authored
`SCR_MapEntity1`. Only the BCM-authored four-line map entity was removed
from `NativeCampaign.layer`. The world continues to inherit base Arland;
12 native strategic entity definitions and all production proof/M3/hold
settings remain unchanged. The Phase-1 launcher preflight now requires zero
authored map entities. Static checks pass for 12 strategic entities and
PowerShell parsing. The inherited map entity's presence is established by
the runtime log, but a fresh Workbench resource pass and unrestricted
startup smoke must confirm the duplicate warning is gone. No follow-on
capture/M3/pressure acceptance is claimed before that smoke.
## Post-cleanup Phase-1 closure and proof-harness boundary

The Creative Director processed and saved the map cleanup, then ran the
unrestricted dedicated smoke at
`C:\Users\ruanb\Documents\BCM-Conflict-Core-TestEvidence\M4XP_Phase1_2026-10-01_16-57-12\Server`.
The server entered online game state at 16:57:31 and reported registry READY,
12 positions/21 edges, one selected US MOB, all ordinary spawns disabled,
ordinaryUS=0/11, depth=0, foothold=0, ACTIVE outcome, pressure/deadline/siege
hook absent, and M3 coordinator/garrison enabled. The prior duplicate-map
error is absent. **Phase 1 is closed/PASS** and must not be rerun for the
remaining acceptance work.

The current M4.XP candidate deliberately sets `m_iAuthorityProofMode=0`
during initialization. Its existing M4.4 physical and pressure proof drivers
are not directly reusable: they assume a protected physical OPFOR endpoint,
whereas M4.XP has an off-map origin and an unselected HQ counted among 11
ordinary positions. The three-node M4.X driver hard-codes `PROTO::*` IDs.
The missing bounded M4.XP fixture for EARLY/MID/LATE and 11/11 state was
subsequently authored under `Worlds/Test/M4XP` and `Missions/Test/M4XP`.
The Creative Director passed its normal Workbench compile/resource gate,
and the six dedicated modes ran. Results and the explicit M3 human-presence
boundary are in `Documentation/TB-Conflict-Core-M4XP-Integrated-Proof.md`.
Historical M4.4 and three-node results remain regression context, not
full-native acceptance evidence. No direct ownership assignment is used to
skip captures.

## Retail community symptom correction — manual Workbench gate pending

The unlisted full-native community run admitted a retail player and showed
the selected MOB, enemy bases, and radio lines, but lacked a single current
objective indicator and visible defenders. This is a real player-facing defect;
the old dedicated proofs did not close human M3 acceptance.

Differential audit: the inherited Conflict foundation logged vanilla
`SCR_TaskSystem` creation of `US_SeizeCampaignMilitaryBase_*` tasks. The native
world has `SCR_TaskSystem`, but its dedicated startup logged no created seize
task. Vanilla faction-commander task generation targets radio-eligible bases,
not the single M4-authorized graph objective, so restoring the whole old
campaign layer or enabling its broad task generation would give misleading
targets. The native component now bridges the M4 legal US stable ID to one
vanilla `SeizeCampaignMilitaryBaseTaskEntity.et`, sets its faction visibility
to US, and deletes/replaces it when the legal objective changes. It clears
the task when no legal objective exists (including BLUFOR pressure, terminal,
or invalid state). A two-second readiness reconciliation is idempotent and
does not alter capture authorization or ownership. The intended initial task
is Military South; Moss Hill remains a candidate, not a presented task.

For M3.6, the suspected missing retry trigger is **disproved in current
source**: the manager already ticks every two seconds, re-queries the M2/M4
legal objective and live US count, and reattempts a deferred creation without
requiring a revision change. The previous M3.7 human session proves this path
once transitioned 0→1 and created two groups/six units after a transient
placement failure. The current native headless log proves only startup
`no-active-human-blufor`; no community server/client log was supplied to
establish whether the retail player became a US participant/observer,
whether visibility/placement blocked creation, whether groups spawned, or
whether replication failed. The native component now activates existing
M3.7A human-presence transition diagnostics, and M3.6 logs distinct
participant/observer gate transitions. No speculative M3 gameplay policy
change was made. M3.2 already rechecks every two seconds and the previous
human session demonstrated its wait/operation response after genuine capture;
full-native physical response remains human-pending.

Focused service audit: the native startup already has the inherited Arland
MapEntity, RespawnSystem, SCR_MilitaryBaseSystem and SCR_TaskSystem, plus
authored RadioManager, CampaignLoadoutManager, faction manager and AI world.
Retail observation supports faction selection, deployment and base/map
presentation at least through entry. Victory-pressure presentation is a
separate replicated overlay in current code. Capture-progress UI, task visual
visibility/JIP, M3 group replication, respawn and end-state still require
genuine retail observation; no other missing world service is established by
available evidence.

The automated Workbench validator failed to launch with Windows status
`0xc0000142`, the same previously documented automated-invocation issue;
this is **not** a script compile result. Next manual gate: open the existing
BCM project in stable Workbench; compile Game scripts; open/save
`Worlds/Production/Arland/TB_CC_Arland.ent`; allow resource processing;
confirm `Missions/Production/TB_CC_Arland.conf` resolves. Do not republish
until that passes and the corrected seams receive focused dedicated checks.
The existing unlisted Workshop item needs an update after validation; Codex
has not published or deployed it.

### Retail evidence after first objective bridge; bridge lifecycle follow-up

The Creative Director reports a retail session **after** the first bridge
was processed: US participants progressed to 1, physical observers to 1,
and M3.6 created a STANDARD Military South garrison of two 3-unit rifle
groups after one placement retry. **M3.6 human garrison path PASS; do not
change M3.6 gameplay.** M3.2 natural post-capture response remains pending.

The same client still saw no objective despite authority reporting Military
South legal and Moss Hill only a candidate. The first bridge used the vanilla
seize-task prefab and `SCR_TaskSystem.CreateTask`, but omitted the vanilla
`SCR_GroupTaskManagerComponent` creation/registration step. Vanilla's
`SCR_CampaignFactionCommanderHandlerComponent.CreateSeizeTask` verifies
`CanCreateNewTaskWithResourceName`, sets task ownership to EXECUTOR, adds the
faction and calls `SetGroupTask(task, 0)`. The bridge now performs that
group-task lifecycle while keeping visibility scoped to US. This is a
source-level lifecycle difference, **not yet proof** that the omitted step
was the first failing runtime step: the actual community server log was not
available in this workspace. Transition-only bridge logs now record desired
ID, base/callsign resolution, creation attempt, task creation/registration,
task state and pending reason. Proxy logs query the corresponding task ID
after M4 projection readiness; genuine map/HUD rendering remains a manual
retail check. No candidate receives its own task.

The reported vanilla GUI `Unknown keyword/data 'name'` and `m_Outline`
mask errors have no established causal link to this task lifecycle;
normal RespawnSuperMenu and MapMenu open. No vanilla UI file was modified.
The manual Workbench compile/resource gate and a focused dedicated bridge
smoke (initial task and one genuine capture/replacement) are required before
the existing unlisted Workshop item is updated. Do not rerun integrated
Modes 1–6. If the bridge telemetry reaches `registeredWithTaskSystem=1`
but the proxy reports `taskQueryResult=0`, investigate task replication;
if both are 1 but the marker is absent, investigate task UI presentation.

### Focused pre-republish run after Workbench bridge compile

The Creative Director confirmed a normal Workbench Game compile and native
world load before this run. The unrestricted dedicated ServerDiag run at
`C:\Users\ruanb\Documents\BCM-Conflict-Core-TestEvidence\M4XP_Phase1_2026-10-01_20-03-57\Server\logs\logs_2026-10-01_20-03-57\console.log`
loaded the full native mission and reached online state. M4 selected
`ARLAND::MILITARY_SOUTH` with `ARLAND::MOSS_HILL` as alternate. The first
objective-bridge result was `taskCreated=0 reason=base-callsign-unavailable`
for Military South. This is a base-startup contract failure, before task
manager lookup or task creation; it is not evidence that either manager is
missing. No active objective task was created in this run.

The native single-physical-HQ startup calls `base.Initialize()` but does
not invoke vanilla `SCR_CampaignMilitaryBaseManager.InitializeBases`, which
normally assigns callsign indices as part of its two-HQ initialization.
The native game-mode startup now assigns and verifies a callsign index for
each of its 12 bases in stable registry order, without invoking that
two-HQ topology or altering strategic ownership. This is a new Game script
edit and **requires another normal Workbench compile/resource gate before
any further ServerDiag test**. Callsign resolution, task creation, client
projection, one-capture replacement, actual three-faction relationships,
and FIA provenance all remain unverified in the corrected build. No faction
or M3.6 change was made. M3.6's separate genuine-human path remains PASS;
M3.2 remains human-pending. Not ready to republish.

### Focused callsign-gate rerun

After the Creative Director confirmed the callsign edit compiled in normal
Workbench, the focused unrestricted ServerDiag run at
`C:\Users\ruanb\Documents\BCM-Conflict-Core-TestEvidence\M4XP_Phase1_2026-10-01_20-13-28\Server\logs\logs_2026-10-01_20-13-29\console.log`
reached online state. Startup reported `bases=12 callsigns=12` without an
`INIT_FAILED`. Military South resolved with callsign 5. Revision 1 retained
Military South as the legal BLUFOR objective and Moss Hill only as an
alternate candidate. The bridge passed its TaskSystem and GroupTaskManager
guards, requested `TB_CC_US_Seize_1`, and vanilla TaskSystem logged creation,
EXECUTOR ownership, FACTION visibility, US faction assignment and group
assignment. The bridge logged `registeredWithTaskSystem=1`, `taskState=1`,
and `groupTaskRegistered=1`. Only the Military South task ID was created in
this run; no Moss Hill task creation appeared. This is authority-side task
publication evidence, not a client active-task query or visual UI proof.

No human client or authorized capture actor was in this smoke. Accordingly,
the requested genuine Military South capture and post-capture task replacement
remain unverified; historical Mode 1–6 proof was not rerun. The run also did
not log the actual directed faction relationship values or identify the FIA
actors' provenance. To permit an evidence-based audit, native startup now
has read-only diagnostics for US↔USSR, US↔FIA and USSR↔FIA. This is a new
Game script edit: **stop for a fresh normal Workbench compile/resource gate
before another dedicated run**. No hostility setting, FIA actor, M3.6 code,
graph, proof setting or gameplay ownership was changed. Not ready to republish.
