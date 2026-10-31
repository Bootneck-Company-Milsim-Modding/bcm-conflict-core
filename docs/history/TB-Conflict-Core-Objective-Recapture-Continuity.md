# Objective recapture and campaign continuity — 2026-10-07

Status: SOURCE / STATIC / MODEL validation only. **WORKBENCH GATE REQUIRED.** No runtime launched, world edited, upload or commit performed. Real campaign recovery remains unverified.

## Compile correction — 7 October 2026

The first manual Workbench compile rejected line 371: `int hostile; int owned;`.
The installed Workbench `EnfusionScriptAPI/html/Page_Enforce_Syntax_Keywords.html:272–274`
documents `owned` as a value/return modifier keyword. It cannot be the counter's
identifier. The semicolons were present; this was not a missing terminator.
Renamed only that counter to `usOwnedCount`, including its increment and diagnostic
argument, and separated the two declarations. No selector algorithm was reverted.

Reviewed the entire recent US-territory scan, breadth-first frontier expansion,
distance bands, radio filtering, diagnostics, band configuration and active-task
guard. No additional concrete syntax/API mismatch was found. Installed API evidence:

- `int.MAX`, `float.MAX`, map Get/Set/Contains/Clear and array operations exist in
  the installed Enfusion API; project registry signatures match their new calls.
- `IEntity.IsDeleted()` returns bool. Resolved entity and authored-position accessors
  exist in the BCM registry/definition.
- `Math.Floor(float)` returns float; implicit assignment to int is used in 24
  installed vanilla declarations, including CampaignMilitaryBaseManager line 653.
- `SCR_TaskSystem.GetTaskState(notnull SCR_Task task)` returns SCR_ETaskState
  (installed Game source line 904), matching the tightened guard.
- New diagnostics stay within the native formatting argument limits; no ternary,
  lambda, foreign cast or string-interpolation construct was introduced.

The whole-Game static scanner now checks reserved identifiers against the installed
keyword reference, with negative fixtures proving the original `int owned;` and
parameter/array variants are rejected. Legal `owned string` return modifiers and
comments/string contents are not mistaken for identifiers. This lexical guard is
not a full Enforce parser or type checker.

Re-ran `Tools/Test-ObjectiveTerritorialContinuity.py`: exit 0. Its whole-tree static/API
chain and commitment, radio, HQ, FIA, USSR, physical/virtual, chat and GM checks pass;
the model retains 1024 ownership states, 20,480 transitions, 29,518 radio masks,
the ten-objective route and repeated recapture cycles. These checks cannot establish
absence of every possible engine compile error. Normal Workbench compile remains
the authoritative gate and is NOT claimed PASS.

Files changed in this compile-only follow-up: the evaluator, both
`Tools/Test-ThreeFactionCorrectiveStatic.py` and
`Tools/Test-ObjectiveTerritorialContinuity.py`, and this report. No task-guard source
change was necessary. World entities changed: NONE. Locked systems unchanged.

## Newly supplied dedicated evidence

Read-only inputs: `C:/Users/ruanb/Downloads/console (4).log` and `console (5).log`. Times below are literal log times, not inferred dates. These are packaged dedicated-server sessions, not execution of the corrected local source.

| console (4) evidence | Observation |
|---|---|
| 12:57:08.912, line 539 | US HQ East: source=1, powered=1, encryptionMatches=0, selectedSource=0. |
| 12:57:10.652, lines 591–593 | Military South committed; zero ordinary US territory. |
| 13:57:24.532–.782, lines 24524–24548 | Still zero US ordinary territory; HQ East receive/transmit coverage becomes zero. Radio loss precedes the first US completion. |
| 20:09:35.973, lines 198340–198347 | Military South completed; revision 9; Airport North task successfully created and published. US owns one ordinary position. Moss Hill and other candidates fail native HQ radio traffic. |
| 20:12:24.698, lines 199522–199542 | Military South changes US -> FIA. It immediately appears as graphLegal=1, radioReachable=0. Airport North remains committed, correctly avoiding rear-loss whiplash. |
| 20:28:56.559, lines 205987–206009 | Airport North capture produces revision 11. Military South and Moss Hill are the two candidates, both radio-rejected. Objective clears into WAITING_FOR_RADIO_EXTENSION, with nine hostile ordinary positions remaining. US owns isolated Airport North, but connected depth is zero. |
| 20:28:58.546, line 206011 | Airport North owner=US, encryptionMatches=1; its correct faction key does not establish an operational US HQ chain. |

`console (5)` starts with the same HQ mismatch (lines 541/635). At 10:04:24.815 its HQ receive/transmit coverage is zero (24648). At 11:11:59.966 an online antenna extends range to 2000 but encryptionMatches remains zero and selectedSource remains zero (485868). Increasing range did not establish correct HQ participation. No objective-completed or waiting-for-radio event appears in this file; it does not reproduce the claimed three-completion sequence.

Neither supplied log contains the newer `hqEncryptionMatches` or `TB_CC_RADIO_TRANSITION` diagnostics. This supports a package/source-version difference, but absence of markers alone is not a verified package hash or definitive publishing history. Current local startup already calls native `ChangeRadioSettings(radioFaction)`; this pass does not modify that correction. See the existing Radio-Progression-Root-Cause report.

### Causal verdict

- CONFIRMED: Military South is not permanently excluded after FIA recapture; the log explicitly evaluates and radio-rejects it.
- CONFIRMED: the observed empty objective after Airport North is caused by both enumerated candidates failing native HQ radio eligibility. This is not campaign completion.
- CONFIRMED source limitation: the old US candidate frontier comes only from the HQ-connected US component. Isolated current US territory cannot contribute its neighbours; the revision-11 snapshot demonstrates this context.
- SUPPORTED underlying radio cause: incorrectly initialized US HQ encryption, consistent with the previously identified startup lifecycle defect. These logs do not prove the corrected local startup was deployed.
- NOT FOUND: permanent completion blacklist, three-objective cap, maximum selector depth, or native task historical-ID blacklist.
- UNKNOWN: the exact reported Military South -> Moss Hill -> Arleville session and any literal attempted recapture denial in that session. These supplied logs establish a different sequence. They cannot prove that every reported symptom has been fixed.

## Narrow implementation

US three-faction selection now collects the selected US origin and **all currently US-owned ordinary positions**, including isolated territory. The protected enemy HQ is excluded from ordinary traversal/selection. Reusable core code has no Arland IDs or coordinates.

Multi-source graph traversal establishes the nearest available hostile graph frontier. Direct hostile neighbours are normally depth 1; there is no fixed search-depth cap. Deeper structural fallback applies only when nearer nodes are not valid resolved candidates. Native radio remains mandatory: no synthetic coverage, connections or forced capture authorization.

For each frontier candidate, distance is minimum horizontal XZ distance from its registered position to any current US territorial position. Default configurable band width is 1000 m: [0,1000), [1000,2000), [2000,3000), etc., with no maximum band. The first band containing a radio-valid frontier candidate wins; an empty or wholly radio-rejected nearer band does not block a farther valid band. Existing weighted scoring and deterministic tie-breaking operate within that band. Graph legality remains a prerequisite; this does not select arbitrary geographically close non-frontier bases.

USSR selection and operations remain unchanged. Existing connected-depth telemetry/endgame calculations are not replaced by the new all-territory candidate footprint. Depth is not an exclusion criterion. Objective commitment still owns the active target: the evaluator returns a recommendation, not permission to switch on every rear loss. Radio event handling and the existing 10-second reevaluation remain intact.

Diagnostics enumerate hostile ordinary nodes with current owner, distance, graph depth, structural/frontier legality, radio result, band, final eligibility and rejection reason. They explicitly report historyExcluded=0 and depthExcluded=0. Repeated identical snapshots are suppressed. `RADIO_WAIT` requires proven rejection of existing frontier candidates; a missing resource/disconnected invalid graph remains a structural failure, not falsely labelled a radio issue.

## Repeated capture and native tasks

Installed vanilla `data007.pak` was inspected:

- `scripts/Game/Tasks/Campaign/SCR_CampaignMilitaryBaseTaskEntity.c:104–110`: matching capture completes and deletes the native task.
- `scripts/Game/Tasks/Campaign/SCR_SeizeCampaignMilitaryBaseTask.c:29–60`: lost HQ signal can cancel/delete the task.
- `scripts/Game/FactionCommander/Components/SCR_CampaignFactionCommanderHandlerComponent.c:157–202`: native seize creation uses faction/callsign task ID; its active lookup uses CREATED/ASSIGNED states.
- `scripts/Game/Tasks/SCR_TaskSystem.c:1594–1605,1673–1711`: duplicate prevention queries current tasks, not a permanent history of IDs; deletion removes the existing task entity.

No ownership epoch is needed. Native stable IDs can be reused after deletion. BCM's same-objective early-return now requires CREATED or ASSIGNED, not mere registry presence. A terminal registered task follows existing cleanup/recreation. This is source-supported lifecycle hardening, **not a demonstrated cause of the supplied session stall**. Current ownership remains authoritative for recapture; prior completion adds no exclusion.

## Validation and limits

`Tools/Test-ObjectiveTerritorialContinuity.py` runs the existing full-chain static suite, installed-task API checks, and an independent ownership/selection model:

- 1024 US/not-US ordinary ownership subsets.
- 20,480 faction-labelled single-position transitions (each of 1024 subsets x 10 nodes x FIA/USSR alternative). Not an exhaustive 3^10 mixed-faction campaign simulation.
- 29,518 frontier radio masks. Every valid hostile topology selects a radio-valid target or explicitly waits when all its frontier candidates are radio-rejected.
- Ten-target model route, with radio eligibility supplied as true: Military South -> Moss Hill -> Arleville -> Signal Hill -> Farm -> North Runway -> Airport North -> Beauregard Harbour -> Beauregard -> Gull Island. This is selection-model evidence, not proof that native radio can reach every step without construction.
- Five repeated recapture cycles per hostile faction, multiple rear losses, isolated territory, nearest-band priority, expansion beyond 2 km, and stable native task-ID reuse model.
- Existing objective-commitment regression suite remains included. Native task recreation and HQ pressure tests are source/model checks, not engine execution.
- 12-node / 21-edge registry retained. Existing FIA/USSR, physical/virtual, radio, HQ, chat and GM static regressions run through the full-chain entry point.

Requested A–P scenarios are covered at model/source level: startup; first/second/third and all-ten selection; recapture by either hostile faction; repeat recapture; five-cycle repetition; simultaneous rear losses; nearest cluster; 1 km and >2 km expansion; current-count all-US HQ-pressure eligibility; pressure cancellation on loss; explicit radio wait. No synthetic firing/capture/runtime PASS is claimed.

HQ pressure and timers were not edited: all ten current US ordinary owners remain the dominance input, and losing dominance still cancels pressure. World entities, graph resources, radio implementation, physical/virtual war, operation systems, packages, factions, capture superiority, spawn policy, chat and GM were not changed.

## Files and next gate

This pass changes:

1. `Scripts/Game/TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c`: US all-territory frontier, expanding distance bands, bounded diagnostics.
2. `Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c`: configurable US band width; native task active-state guard.
3. `Tools/Test-ObjectiveTerritorialContinuity.py`: static/API and exhaustive model checks.
4. This report.

World entities changed: **none**. No publication, commit or server run.

**WORKBENCH GATE REQUIRED.** Compile/process normally before runtime acceptance. Then verify the actually shipped package includes the existing native HQ radio initialization correction as well as this selector change. The supplied logs do not validate that package. A later authorized dedicated test must prove native radio progression, real repeat capture and task recreation; static/model PASS is not that gate.
