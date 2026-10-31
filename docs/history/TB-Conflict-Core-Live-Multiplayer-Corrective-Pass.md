# Live multiplayer corrective pass — in progress

Five-human Arland report: USSR Military South defenders existed but did not
shoot, the next BLUFOR objective was outside the intended radio network, and
the first counterattack failed bounded staging before a later three-group
attack approached from effectively one axis. The report confirms genuine
Military South USSR→US capture, graph progression, next-task creation, and
physical OPFOR response. The five-player logs are
`C:/Users/ruanb/Downloads/console (1).log` and
`C:/Users/ruanb/Downloads/AMPLOG_2026-10-01 00-00-21.log`. They predate
the source corrections below and do not establish their runtime acceptance.

## Five-player log findings (19:17–19:32 session)

- At 19:17:22 all six directed US/USSR/FIA relationship checks report
  `enemy=1 friendly=0`. A global faction-matrix change is not justified.
- At 19:18:26 M3.6 created two three-unit Military South defender groups
  with `LOD=0` and `DefendSmall`, then reported ACTIVE with six units. At
  19:27:52 it reported ENGAGED. In the current implementation that means a
  human was nearby; it does not prove target perception or firing. The log
  has no per-unit fire/perception evidence identifying the non-firing cause.
  An initial LOD failure is not supported by this run.
- At 19:28:57 genuine vanilla ownership changed Military South USSR→US.
  M4 published revision 2/depth 90 and scored Airport North 123 above the
  other candidates, publishing it as the next US task. The existing scorer
  did not test vanilla radio reachability. The logs do not print Airport
  North's radio state; inability to capture it is a human observation.
- At 19:29:44 five humans selected the old REINFORCED three-group package.
  The first staging search exhausted 12 candidates. At 19:30:36 a second
  attempt staged three groups from effectively the west axis: anchor
  `(1063.43, 1857.55)`, then `(1023.43, 1863.55)` and
  `(1028.43, 1863.55)`. The latter two are about five metres apart.
- Other Workshop mods were loaded, but the logs do not implicate one in
  the defender behavior. That is a test confounder, not a diagnosis.

## Narrow source corrections awaiting normal Workbench compile

- Native M4 candidate selection now applies the vanilla
  `SCR_CampaignMilitaryBaseComponent.IsHQRadioTrafficPossible(US)` predicate
  to graph-legal candidates before the existing score winner is selected.
  A graph boundary with no reachable candidate remains a valid ACTIVE
  campaign in `RADIO_WAIT`, with no impossible objective published. A
  single 10-second authority retry is rescheduled while waiting; an
  ownership callback also immediately re-evaluates. A radio transition
  changes the normal semantic signature/revision and task projection.
  Non-native M4 fixtures retain the prior graph-only default.
- The existing M3.5 selector retains its depth/result strength band and
  scales the upper band by active humans: 5–7 selects four groups, 8–11
  five, and 12+ six, bounded at six. Existing LIGHT/STANDARD/REINFORCED
  package meanings for smaller teams remain unchanged. The current USSR
  defenders group has shown three units in runtime logs, so these packages
  target 12/15/18 units respectively; the assembly log remains the actual
  runtime count.
- M3.3 staging now interleaves radius and bearing within bounded attempts
  rather than concentrating the first 12 attempts in one radius band. M4
  production allows 24 bounded attempts. Additional groups search around
  the target for ≥80° separation first, then ≥45°, then a valid one-axis
  fallback. Every candidate still passes existing water, terrain, navmesh,
  player-distance and visibility checks; a failed search remains atomic.
- M3.6 custom group materialization now explicitly confirms USSR group and
  character affiliation, requires full unit/agent creation, preserves the
  engine-selected non-maximal LOD, and logs a one-time combat-readiness snapshot
  after DefendSmall assignment. The global faction matrix is unchanged.
  This closes an unvalidated custom-spawn invariant, **not yet a confirmed
  explanation of the reported lack of fire**.

The first normal Workbench compile rejected `SCR_AIGroup.ActivateAllMembers`
at M3.6 line 268: that call is not exposed by the installed 1.8.0.13 Game
API. The explicit activation and forced `SetLOD(0)` were removed. Existing
M3.4 group assembly and the pre-correction five-player M3.6 run spawn
agents through `AddAgentFromControlledEntity` without explicit activation;
the latter naturally logged LOD 0. The source now rejects only the
engine's maximum/inactive LOD, matching M3.4's established guard. The
non-firing cause remains unknown. The Creative Director subsequently reported
normal stable Workbench Game-module compile PASS after this correction.

Synthetic/proof checks were added for unreachable higher-score rejection,
radio-wait/recovery, 2/5/8/12/16-player package selection, expanded package
shapes, bearing wraparound, and garrison affiliation/LOD/order invariants.
They have passed the reported normal Workbench compile but have not been run.
An isolated final-candidate ServerDiag smoke on 2026-10-01 compiled Game
scripts and validated `M4XP_FinalSmoke.json`, then failed before mission load
at backend `ENV:[invalid] / Could not connect to server` while requesting
`GameConfig/List`. The owned process was stopped and its log preserved at
`C:/Users/ruanb/AppData/Local/Temp/BCM_M4XP_CorrectiveSmoke_20261001_210811`.
No mission runtime or JIP regression claim is made.

## Acceptance still required

### Unrestricted Mode 7 result (2026-10-02)

### Single pre-rollback diagnostic run (2026-10-02)

The one authorized unrestricted Mode 7 diagnostic ran in
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/M4XP_Mode7_Diagnostic_2026-10-02_06-42-59`.
It reached online state, then genuine Military South capture selected Moss Hill.
At 06:44:09, M3.6 reported `firstStep=character-affiliation-component-slot-0`:
the group resource and entity succeeded, its faction was USSR before and
after verification, one of three character entities spawned, but the new
character-local `SCR_FactionAffiliationComponent` lookup was null. The code
rolled the spawned entities back before `AddAgentFromControlledEntity`;
therefore agent count was 0, natural LOD was not sampled, and DefendSmall
was not created or assigned. This run cannot establish combat readiness.
`GM Budget got clamped at some point!` appeared immediately afterward and
on retries; it did not prevent the observed group/character creation, and
its precise source remains unproven. The owned diagnostic server was stopped
after the first failure had been preserved. No ProductionSmoke or other mode
was run.

This is a corrective-pass M3.6 invariant defect, not evidence that the
Mode 7 fixture's player-count boundary prevented spawning: the strict
character-local component requirement was added after the earlier live
six-defender spawn, while the existing M3.4 assembly path registers spawned
characters directly with their group. M3.6 now treats an absent local
component as unknown diagnostic data rather than a fatal mismatch, retains
the verified USSR group faction, checks any *present* explicit affiliation
for conflict, and retains expected agent count/natural LOD/order checks.
It no longer writes character affiliation. This narrow source edit is
**not Workbench-compiled or runtime-verified**; a normal Workbench gate is
required before further testing. M3.6 real-human firing remains
**HUMAN-PENDING**.

### Post-Workbench focused acceptance (2026-10-02)

The Creative Director confirmed the corrected Game module compiled in normal
Workbench. One unrestricted Mode 7 run then passed, followed by the gated
production native-Arland smoke. Evidence is preserved under
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/M4XP_Corrective_2026-10-02_06-48-54`.

Mode 7 reported `garrison-invariants pass=1` and `COMPLETE result=PASS`.
At Moss Hill, the STANDARD garrison remained ACTIVE with two USSR-faction
groups, three characters/controlled agents per group, six units total,
natural LOD 0, and DefendSmall present/assigned for both groups. All six
character-local affiliation components were absent and correctly reported
as UNKNOWN (`unknownUnits=3` per group); no explicit affiliation contradicted
USSR. No materialization rollback occurred. This proves materialization,
agent, LOD, and order invariants, **not** autonomous firing at a human.

ProductionSmoke reached online state with ARLAND_NATIVE_FULL_01 READY,
12 positions/21 edges, 11 USSR ordinary positions, selected HQ East as the
sole enabled US MOB spawn, ACTIVE with depth 0 and foothold false, no startup
pressure, one initial Military South US objective task, M3.2/M3.6 enabled,
and no proof actor or BCM script/runtime error. Production serialization
remained proof mode 0 and BLUFOR/OPFOR holds 300/300 seconds. Both owned
server processes were stopped by the runner after their evidence checks.

**Verdict: READY TO REPUBLISH UNLISTED TEST BUILD.** Publishing was not
performed. M3.6 autonomous combat against genuine human US players remains
**HUMAN-PENDING**.

The Creative Director's unrestricted Mode 7 run is preserved under
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/M4XP_Corrective_2026-10-02_06-13-51/Mode7`.
Genuine Military South capture and radio filtering passed: Airport North,
Arleville and Farm were graph-legal but vanilla-radio-unreachable; Gull Island
and Moss Hill were reachable; Moss Hill won. Task replacement, sole-MOB
deployment, proof-only no-reachable wait and 10-second recovery passed. The
1/2/5/8/12/16 selector matrix, three-axis staging, bounded single-sector
fallback and 24-attempt total failure passed. None of those systems is being
changed in response to this run.

Mode 7 failed at M3.6 Moss Hill materialization. Placement succeeded at
`<2292.76,90,1241.62>`, then the old combined
`materialization-or-order` wrapper returned false before `group-created`.
The log cannot distinguish group spawn, faction/character/agent checks,
natural LOD, or waypoint creation. The subsequent GM Budget warning is
correlated with rollback attempts, not proven causal. The five-player live
Military South run had previously created two three-unit USSR groups at
natural LOD 0; the current Mode 7 proof has a test-only participant count
but no physical observer. A newly added, transition-only M3.6 trace records
the first failing step and pre-rollback group/character/agent/faction/LOD/
waypoint state. It does not force AI activation, LOD or combat. This latest
Game-script edit requires a new normal Workbench compile before rerunning
Mode 7; the production smoke did not run after the Mode 7 FAIL.

The Creative Director confirmed a normal Workbench Game compile and resource
processing PASS for the initial Mode 7 child world/mission. Mode 7 has since
been extended with a **test-only** dedicated radio view (guarded by the named
corrective proof entity), 10-second wait/recovery assertions, actual native
candidate/task/deployment logging, the selector matrix, bounded multi-axis
staging probes, and M3.6 affiliation/order/LOD invariant checks using a
clearly logged test-only participant count. These latest Game-script changes
have **not** passed the next Workbench compile. They never assign base
ownership, alter production radio ranges, or claim real human combat.

The Creative Director subsequently confirmed normal Workbench Game compile
PASS and the Mode 7 world loads. The unrestricted runner is now
`Tools/Run-M4XP-CorrectiveAcceptance.ps1`. It has passed PowerShell parsing
and `-DryRun` validation but has **not** been executed by Codex. Leon should
run it from normal unrestricted Windows PowerShell at the addon root:

```powershell
.\Tools\Run-M4XP-CorrectiveAcceptance.ps1
```

It preflights UDP 2230/17810 for Mode 7 and 2231/17811 for the production
smoke, writes two isolated profiles under
`Documents/BCM-Conflict-Core-TestEvidence/M4XP_Corrective_<timestamp>`,
uses the dedicated-server installation as working directory, and never
stops processes it did not start. BCM is declared only in `game.mods`; the
command line has `-config` and `-addonsDir`, not `-addons`. Any known
`ENV:[invalid]` backend failure is a STOP with logs preserved, not a BCM
gameplay verdict. The runner executes no historical Modes 1–6.

The proof-only human count used to materialize M3.6 defenders does not
establish real-player target acquisition or firing. The known restricted
backend result is not under investigation.

1. Dedicated authority and synthetic/proof runs for radio wait/recovery,
   next objective, group counts, multi-axis staging, lifecycle and cleanup.
2. A genuine human dedicated session to observe USSR defender target
   acquisition/firing and the next radio-reachable objective; client/JIP
   projection must be checked separately.

The project is **not ready to republish** while those gates are open.
