# BCM persistence — foundation and safe restore architecture

PHASE 1 — WORKBENCH PASS (Creative Director confirmation supplied with Phase 2 brief).

PHASE 2A — WORKBENCH PASS (Creative Director confirmation supplied with Phase 2B brief).

PHASE 2B — ordinary strategic restore WORKBENCH PASS (Creative Director confirmation in Phase 3 brief). Pressure/terminal restart gates remain unverified.

PHASE 3 — progression restoration and latest world-end save WORKBENCH confirmed by the user in the P5 brief. Dedicated acceptance remains pending.

PHASE 3.5 — NOT IMPLEMENTED (native construction boundary remains blocked).

PHASE 4 — NOT IMPLEMENTED. PHASE 5 — IMPLEMENTED / STATIC-API-MODEL PASS / WORKBENCH AND DEDICATED PENDING. See the P5 section at the end; earlier P5-pending statements are historical.

The P2B implementation section below is the current source status. Earlier P2A
audit/refusal descriptions are retained as historical evidence, not the current
startup behavior. The current P2B ordinary-restart result above supersedes its
implementation-time pending status; it is not pressure/terminal or dedicated
multiplayer acceptance. No P3 Workbench or runtime PASS is claimed.

## Historical Phase 2 audit — addressed by Phase 2A orchestration

The Phase 2 brief requires a transactional strategic restore before ordinary
campaign consumers start. The original Phase 1 hook ran after campaign
initialization, objective scheduling and M3 startup; it cannot be reused at
that location for strategic restore without changing lifecycle ordering.

Installed `SCR_CampaignMilitaryBaseComponent.SetFaction(SCR_CampaignFaction)`
delegates to faction affiliation. Its `OnFactionChanged` invokes the extended
faction-change event before performing native radio/supply/spawn updates,
capture messaging and remnants handling. XP is gated by `IsBeingCaptured`,
but that alone does not suppress the other restore side effects. A persistence
service gate alone is therefore insufficient. No ownership setter has been
called as part of this audit.

Current schema-v1 position storage is `array<string>` and future state sections
are string-to-string maps. Phase 1 explicitly rejects populated sections.
Phase 2 must define and validate its representation and an explicit compatible
empty-container upgrade before accepting populated saves; compatibility has
now uses the explicit v1/v2 compatibility described below. Full strategic
application remains intentionally unimplemented.

Additional state seams identified: US objective commitment, native HQ lock
states, gameplay pressure deadline/remaining seconds, response ownership epochs
and per-target USSR genuine-failure history. In-flight forces/operations remain
Phase 4 scope. These findings are source audit only, not strategic restore,
native-event suppression or restart acceptance evidence.

PHASE 3 PLAYER PROGRESSION — PENDING

PHASE 4 OPERATION RECONSTRUCTION — PENDING

PHASE 5 ADMIN/RECOVERY POLISH — PENDING

## Scope and architecture

`TB_CC_CampaignContainer` is the versioned JSON root. `TB_CC_PersistenceService`
owns server-only startup, validation, checkpoints and replacement. Its public
seams are `Start(mapIdentity, scenarioIdentity, configIdentity, interval)`,
`GetState`, `IsReady`, `GetCampaignId`, `GetCampaignRevision`, `MarkDirty(reason)`,
`SetCampaignRevision(revision, reason)`, `SaveNow(reason)` and `Stop`.

The existing native campaign adapter creates this service after native foundation,
registry and startup identity resolution, BEFORE objective evaluation and M3
consumers, and stops it before component cleanup.
Map identity comes from the existing registry. Scenario and configuration labels
are adapter attributes (`BCM_NATIVE_FULL_CAMPAIGN`, `THREE_FACTION_V1` defaults).
The configuration identity also includes campaign seed, start mode and selected
MOB ID. Authors of another scenario must assign appropriate distinct labels;
these are explicit configuration identities, not automatic resource hashes.

Reusable persistence source contains no map IDs, coordinates or production
world paths. No world/layer edits are necessary. One campaign store is permitted
per server profile; concurrently running authorities MUST use separate profiles.
Sharing a profile is not supported and there is no cross-process lock.

This is **container resume**, NOT playable strategic resume. The persisted
revision starts at zero and is independent of the live strategic revision in
Phase 1. No ownership callbacks, objectives, players, operations or endgame
state are saved/restored. Phase 2A adds a separate campaign-consumer gate;
container load failure or snapshot-without-P2B support blocks consumer startup.
Container readiness alone no longer authorizes campaign consumers.

## Format, installed API evidence and schema

Human-readable JSON via installed `JsonSaveContext.SaveToFile(string)` and
`JsonLoadContext.LoadFromFile(string)`. Every field uses checked
`SaveContext.WriteValue(string, void)` / `LoadContext.ReadValue(string, out void)`.
This avoids hand-assembled JSON and unchecked reflection/default-field loads.

Verified read-only in installed Reforger archives:

- `scripts/Game/generated/Plugins/Serialization/Presets/JsonSaveContext.c`
- `scripts/Game/generated/Plugins/Serialization/Presets/JsonLoadContext.c`
- `scripts/Game/generated/Plugins/Serialization/SaveContext.c` / `LoadContext.c`
- `scripts/Core/generated/System/FileIO.c`: `MakeDirectory`, `FileExists`, `CopyFile`
- `scripts/Core/generated/Types/UUID.c`: `GenV4`, `IsUUID`, `IsNull`
- `scripts/Core/generated/System/System.c`: `GetUnixTime` (UTC epoch seconds)

Current write schema version: **2**. Explicit read compatibility: empty Phase 1
**v1** and **v2**. Original required fields remain:

| Field | Phase 1 meaning |
| --- | --- |
| schemaVersion | integer 1 (legacy empty only) or 2 |
| campaignId | non-null native random UUID v4 |
| mapIdentity | registry map-definition identity |
| scenarioIdentity | adapter-supplied scenario label |
| campaignConfigIdentity | adapter configuration label + seed/start/MOB |
| campaignRevision | nonnegative container revision, initially 0 |
| createdTimestamp | nonnegative UTC epoch seconds |
| lastSavedTimestamp | UTC epoch seconds, never before creation/previous save |
| campaignState, objectiveState, hqState, pressureState, factionState | empty string-to-string maps reserved for future versioning |
| strategicPositions, operations, players | empty string arrays reserved for future versioning |

All original 16 fields must be read successfully. v2 additionally requires
`snapshotPresent` (boolean) and `positionOwners` (string-to-string map).
Reject invalid/null UUIDs, negative revision/timestamps, backwards save time
and missing containers. Unsupported versions other than 1/2 are rejected.
Timestamp storage uses installed signed integer epoch API;
long-term epoch-range handling is future schema work.

## Storage and campaign identity

Virtual root: **`$profile:BCMConflictCore/Persistence/`**.

Physical root: `<active server/Workbench runtime profile>/BCMConflictCore/Persistence/`.
Dedicated `-profile` determines that profile; Workbench uses its active game
profile. Actual engine resolution/permissions remain runtime-unverified.
No saves are written into addon source/resources.

- Primary: `campaign.save.json`
- Backup: `campaign.backup.save.json`
- Staging: `campaign.temp.save.json`
- Rotation safety copy: `campaign.rotation.save.json`
- Invalid-primary archive when repairing from backup: `campaign.rejected.<UUID>.json`

Native `UUID.GenV4()` creates the campaign ID only when no prior primary,
backup, temp or rotation artifact exists. Successful reload reuses the stored
ID; session/player identity is never used. Archive IDs are separate random
filenames, not new campaign IDs. Reset/admin commands are not implemented.

## Startup and failure policy

1. Authority enters RESTORING; validate runtime identities/interval and create directory.
2. Try primary and validate all fields.
3. Corrupt/unreadable primary: try backup. Missing primary also permits valid backup.
4. Valid backup resumes its campaign ID, with explicit recovery telemetry.
5. Wrong map/scenario/config or unsupported schema fails closed immediately;
   a matching backup does not authorize replacing a foreign primary.
6. Existing primary/backup both invalid: FAILED; preserve files, do not create
   a blank campaign. Orphan temp/rotation artifacts also prohibit fresh creation.
7. Only genuinely absent saves permit fresh container creation and initial save.
8. READY means container restored/created, not strategic ownership restored.
   Initial save failure changes the gate to FAILED. A checkpoint failure leaves
   dirty true and the loaded in-memory container available.

Rotation/temp files are diagnostic/recovery artifacts, not automatically
accepted as committed startup saves. Both invalid committed files still fail
loudly even if an auxiliary copy exists; manual recovery is a later phase.
Backup recovery does not immediately repair/overwrite the primary or mark the
container dirty. A later successful dirty save may repair it, after archiving
the invalid primary. No files are deleted by Phase 1.

## Recoverable replacement — atomicity boundary

Installed `FileIO` has copy/delete but **no rename/atomic-replace API**.
No invented API, shell helper or addon-source write is used. This is a
validated recoverable replacement strategy, **not a guaranteed atomic rename,
fsync or power-loss durability guarantee**.

1. Serialize to temp; wait for synchronous API return; reload/validate it and
   compare campaign ID, revision and timestamps with the intended snapshot.
2. Existing valid primary: validate its identity, copy it to rotation, verify
   rotation, copy rotation to backup, verify backup. Foreign existing backup
   stops replacement rather than being overwritten.
3. Existing invalid primary: archive it under a unique filename; require a
   valid backup before replacement. Never rotate corrupt primary over backup.
4. First save: create and verify backup from temp before creating primary.
5. Missing primary with backup: validate the recovery copy before replacement.
6. Copy temp to primary and verify primary. Clear dirty only after success.

Partial temp write leaves committed files untouched. Backup rotation failure
retains valid primary and/or verified rotation. Primary-copy failure retains
verified backup. Error leaves dirty true, logs ERROR, and preserves artifacts.
Same-profile concurrent writes, arbitrary external file edits, media failure,
process termination and OS buffering remain runtime/durability boundaries.

## Dirty/checkpoint/cleanup

`MarkDirty(reason)` sets dirty only in READY on authority. The log is emitted
on the clean-to-dirty edge, not each frame. Revision setter rejects decreases.
Default interval **60 seconds**, configurable through the adapter attribute and
passed into the service once. Clean checkpoints do nothing. Failed writes keep
dirty; successful writes clear it. There are no gameplay dirty hooks in Phase 1.

Installed `SCR_BaseGameMode.OnGameEnd()` is a match-end event, not guaranteed
process shutdown. The existing component `OnDelete(IEntity owner)` provides a
best-effort cleanup save if dirty and removes the scheduled checkpoint.
It is NOT guaranteed for crashes, force termination, or every server shutdown.
Periodic checkpoints are the normal safeguard; shutdown is not the sole save.

Restore states: UNINITIALIZED -> RESTORING -> READY or FAILED. No automatic
reset/retry to a new campaign after FAILED. Public state getters form the
future authoritative restore seam; no client save/write path is exposed.

## Telemetry and validation gate

`[TB_CC_PERSIST]` actions: STATE, LOAD (PRIMARY/BACKUP and validation stages),
RECOVERY, DIRTY, SAVE (ID/revision/reason/result/dirty). No per-frame logging.
Loader errors contain the exact mismatched field and expected/saved identity.

Run `Tools/Test-PersistenceFoundation.py` with the available Python runtime.
It checks field read/write coverage, core portability, balanced expressions,
installed API signatures and an independent fault-injection disk model. Cases
cover fresh creation/reload/ID retention, backup selection, missing primary,
both invalid, wrong identities, schema/type/missing fields, clean checkpoint,
dirty success/failure and partial temp/backup/primary writes.

These tests do not execute Enforce or native JSON IO. Workbench must compile
the new code and process unchanged resources. Subsequent separately authorized
runtime validation must establish empty-container serialization, exact native
type rejection, profile-path resolution, copy-over-existing behavior,
readback integrity, lifecycle scheduling and restart ID retention.
Phase 1 Workbench compile PASS is human-confirmed. No Phase 2A Workbench,
dedicated, durability or gameplay-resume PASS is claimed.

## Phase 2A lifecycle and consumer gating

Before: native campaign -> default-state objectives/M3 -> persistence load.

After: native foundation -> registry/start identity -> CONTAINER_LOADING ->
Phase 1 IO load/validation -> STRATEGIC_RESTORE -> no-snapshot authored-state
validation -> DERIVED_REBUILD -> READY -> normal objectives/M3/FIA/USSR.

`TB_CC_RestoreTransaction` centralizes UNINITIALIZED, CONTAINER_LOADING,
STRATEGIC_RESTORE, DERIVED_REBUILD, READY and FAILED. It is separate from the
unchanged Phase 1 IO state machine. `IsRestoring` and
`IsReadyForCampaignConsumers` expose this boundary. Illegal/out-of-order
transitions refuse progress. Failure is latched; no default-state retry occurs.

The native adapter uses `PersistenceAllowsCampaignConsumers` at objective
access/publication, evaluator/radio callback, pressure/HQ evaluation,
ownership gameplay callback, capture authorization, M3 startup and territorial
tick boundaries. `IsWarOperational` also requires this gate, covering FIA,
USSR, response, defence and physical/virtual consumers via existing adapters.
No scoring, balance, radio policy, capture rules or UI layout is rewritten.
Initial projections/tasks are produced only after readiness, not during loading.
Client reads continue to consume authoritative projections; this is an
authority startup gate, not completed multiplayer/JIP restore handling.

Fresh and legacy-empty containers perform no historical ownership application.
Their existing native state passes the derived-rebuild seam, and normal native
radio observer/projection/task startup follows. A valid populated snapshot
enters STRATEGIC_RESTORE then FAILED with `strategic-snapshot-requires-P2B`:
it is preserved, not applied, not converted to an empty campaign.

## Quiet ownership seam and installed source evidence

`TB_CC_QuietOwnershipRestore.Apply(base, faction, transaction)` is a future
state-application seam, NOT called by Phase 2A startup and NOT an admin API.
It requires authority, STRATEGIC_RESTORE, an initialized non-proxy base,
no active capture and its real SCR faction-affiliation component.

The exact entity is scoped temporarily. Native `SetFaction` performs the actual
engine affiliation assignment; a BCM extension of
`SCR_FactionAffiliationComponent.OnFactionChanged(Faction previous, Faction current)`
returns before the vanilla script invokers only for that scoped entity on authority.
All unscoped changes call the original implementation. Subscribers are never
cleared, detached or reordered. Readback must match; failure closes the transaction.

Installed data007.pak source:

- `scripts/Game/generated/Components/FactionAffiliationComponent.c`: setters
  have no silent/notify argument; `SetAffiliatedFaction(Faction)` and
  `SetAffiliatedFactionByKey(FactionKey)` both use the normal faction mechanism.
- `scripts/Game/Components/SCR_FactionAffiliationComponent.c:17-24`: script
  callback invokes faction-update and faction-change subscribers. Suppression
  is placed before that fanout, not after the native base has emitted history.
- `scripts/Game/Components/Locations/SCR_MilitaryBaseComponent.c:534-557`:
  base callback notifies systems, changes flags and can notify nearby players.
- `scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c:1550-1668`:
  extended faction event, capture XP, radio/supply/spawn updates, capture HQ
  message, remnants elimination/orders and autosave are tied to the callback.
- Same campaign component at 2212-2224: `SetFaction` delegates to its real
  faction-affiliation component rather than a separate silent owner variable.
- Installed array API confirms `RemoveItem(T)` for closing the exact scope.

Thus quiet assignment does not replay those SCRIPT subscriber paths on authority.
This deliberately also suppresses their derived native updates. **It is not a
complete native restore by itself.** Phase 2B must reconcile radio, native base
systems/registration, supply/spawn/flag/map presentation and projections from
current state without invoking historical callbacks. The adapter's current
`CompletePersistenceDerivedRebuild` refuses READY after any quiet ownership write;
only the no-write/no-snapshot path is implemented in Phase 2A.

Native engine behavior, proxy callback fanout, replication, UI notifications and
all preserved subscriber behavior need runtime verification. No claim of complete
multiplayer historical-event suppression is made from source/model tests.

## v1/v2 compatibility and strategic placeholder representation

Version 2 is intentional: v1 validation explicitly prohibited populated sections.
Reinterpreting a populated v1 save would violate that existing semantic contract.

- v1: all future sections empty; accepted as no snapshot, campaign ID retained.
  No migration write occurs just for loading. Next dirty save writes v2; the
  previous valid v1 primary is preserved through the normal backup rotation.
- v2 `snapshotPresent=false`: all reserved sections and `positionOwners` empty.
- v2 `snapshotPresent=true`: `strategicPositions` is a nonempty list of unique,
  nonempty stable IDs. `positionOwners` has exactly those IDs as keys, with
  US/USSR/FIA values. Other campaign/objective/HQ/pressure/faction sections remain
  string-to-string maps reserved for P2B semantics. Operations/players must remain
  empty. Type/read errors, mismatched counts, duplicates or unsupported factions
  are invalid. This is a structural schema seam, not full cross-state validation.
- Any recognized populated snapshot is fail-closed at the adapter until P2B
  adds registry resolution, cross-state validation and complete application.
- Unsupported schema, foreign identity and corrupt-file policies remain intact.

Phase 2A writes no live strategic ownership/objective/HQ/pressure/faction state.
New containers write v2 with no snapshot. No active operations are reconstructed.

## Phase 2A validation and next gate

`Tools/Test-PersistenceRestoreArchitecture.py`: 81 source/API/model cases covering
startup ordering, gate transitions/failure, v1/v2 structures, scoped vs normal
callback models, snapshot refusal and portability. It also runs the existing
58-case Phase 1 disk-failure model and audits 18 Phase 1 non-void methods.
The whole-tree installed-prototype validator now includes the actual SCR faction
affiliation source, without exempting its new override signature from checking.

Full static/model regression and fixed-array validation are required separately
from Workbench. Models do not execute native callbacks. Human Workbench compile,
native JSON v1/v2 round-trip, fresh-start readiness/failure, subscriber suppression,
replication and changed-owner native reconciliation remain unverified.

P2B/P3/P4/P5 remain PENDING. The Phase 2A Workbench gate is now human-confirmed;
any subsequent Phase 2B source changes will require their own Workbench gate.

## Phase 2B reconciliation audit — implementation still pending

Phase 2A's compile gate is now human-confirmed. It does not establish runtime
quiet-assignment or native-reconciliation acceptance. No Phase 2B ownership
application, strategic snapshot writer or operation reconstruction has been
enabled by this audit.

Additional installed-source findings:

- `SCR_MilitaryBaseSystem.OnBaseFactionChanged(Faction, SCR_MilitaryBaseComponent)`
  (lines 75-78) is event fanout, not a silent cache-rebuild API. Calling it during
  restore would reintroduce historical subscribers.
- `SCR_MilitaryBaseComponent.OnFactionChanged` (lines 534-557) propagates faction
  to registered base logic, notifies the global system, changes flags and can
  notify players. Replaying it is not acceptable native reconciliation.
- `SCR_MilitaryBaseLogicComponent.OnBaseFactionChanged` (lines 80-87) assigns the
  logic owner's faction. Registered derived logic may override that method;
  reconciliation must not assume arbitrary overrides are silent.
- `SCR_CampaignMilitaryBaseComponent.OnFactionChanged` (lines 1550-1694) also
  updates supply regeneration, spawn faction/respawn timing, radio settings,
  previous-owner state and map information, alongside historical messages,
  remnants processing, rewards and autosave. These service updates are skipped
  by the accepted quiet-assignment seam and still need explicit reconciliation.

Strategic data identified for the P2B implementation audit:

- Campaign revision, US objective commitment, HQ lock enums, pressure duration
  and remaining gameplay time, campaign outcome.
- FIA conventional strength, former-territory membership, loss serial/order,
  and ownership epochs. Pending recapture opportunities and in-flight forces
  are separate Phase 4 reconstruction work, not ownership-history replay.
- USSR per-target genuine-failure counts used by package escalation. Active
  operations and their reconstruction remain Phase 4 scope.

No strategic-runtime PASS or Phase 2B implementation claim is supported yet.

## Phase 2B implementation — 2026-10-13 (supersedes historical refusal above)

PHASE 1 — WORKBENCH PASS. PHASE 2A — WORKBENCH PASS.
PHASE 2B — IMPLEMENTED / WORKBENCH PENDING.
PHASE 3 — PLAYER PROGRESSION PENDING.
PHASE 4 — OPERATION RECONSTRUCTION PENDING.
PHASE 5 — ADMIN/RECOVERY POLISH PENDING.

### Actual strategic schema and authority

Schema remains **2**. Existing identity/header fields and profile-local verified
primary/backup/rotation handling are retained. `snapshotPresent=true` identifies
the complete P2B payload; v1 and v2 empty foundation containers remain readable.
The campaign UUID and creation time are preserved across upgrade/restart.

| Section | Exact fields |
| --- | --- |
| `strategicPositions` | Complete stable registry ID array, in deterministic registry order |
| `positionOwners` | Every ID -> current authoritative vanilla faction key (`US`, `USSR`, `FIA`) |
| `campaignState` | `outcome`, `foothold`, `USDepth`, `USSRDepth` |
| `objectiveState` | `committedUS` (no task handles or completion blacklist) |
| `hqState` | `US`, `USSR`: PROTECTED=0, PRESSURED=1, VULNERABLE=2, CAPTURED=3 |
| `pressureState` | `active`, `remaining`, `duration`, `starts` (side follows `outcome`) |
| `factionState` global | `FIA-strength`, `FIA-lossSerial`, `FIA-cooldown`, `FIA-operationSerial`, `USSR-cooldown` |
| `factionState` per ID | `FIA-former:<id>`, `FIA-loss:<id>`, `FIA-epoch:<id>`, `FIA-pending:<id>`, `FIA-attempts:<id>`, `FIA-revision:<id>`, `FIA-queueOrder:<id>`, `USSR-failures:<id>` |
| `operations`, `players` | Empty; later phases are not implemented |

Enums, flags and counters use canonical decimal strings in the existing typed
string maps. Presence, exact section counts, canonical integers/ranges, complete
ID set, supported owners, authority/non-proxy base availability, initialized base
roles, HQ owner/outcome consistency, pressure side/dominance consistency and
contiguous unique FIFO queue orders are checked before **any ownership write**.
Unknown/incomplete P2A placeholder snapshots cannot masquerade as P2B saves.
Objective IDs may become structurally invalid and are reconciled rather than
used as ownership write targets.

Ownership is read from the live vanilla faction of each registry-resolved base,
not from task state, UI or a historical callback log. Depth is rebuilt from the
evaluators for live non-terminal campaigns. Saved bounded depth provides terminal
presentation where the captured HQ no longer satisfies a live evaluator origin.

### Restore order and explicit native reconciliation

1. Load and validate header/schema/identity; enter STRATEGIC_RESTORE before consumers.
2. Resolve/check the entire strategic snapshot and all native faction/base targets.
3. Apply all owner changes through the accepted P2A scoped affiliation seam;
   verify each readback. No historical `OnFactionChanged`/`OnBaseFactionChanged`
   handler is invoked as a restore mechanism.
4. Restore revision, HQ/outcome/foothold, pressure metadata and FIA/USSR strategic
   counters. Quietly import unlaunched FIA intent in saved FIFO order.
5. Enter DERIVED_REBUILD. For every native base explicitly reconcile registered
   logic-owner affiliations under the quiet scope, previous-owner and defender
   caches, flags, radio encryption/settings, spawn faction, supply regeneration
   and supply arrival deadline. Refresh local native map descriptors where present.
6. Rebuild native radio coverage using installed `SCR_RadioCoverageSystem.UpdateAll(true)`
   after all ownership/radio settings are reconciled. This is the synchronous
   native traversal, not a captured-base event. Native winner/control-point event
   fanout is not used as a substitute for BCM campaign reconstruction.
7. Evaluate live US/USSR derived caches; revalidate saved commitment. Retain a
   structurally valid hostile objective even if radio is temporarily absent.
   Only absent/removed/US-owned/terminal objectives need replacement; selection
   uses the already radio-filtered evaluator once. An unavailable replacement
   remains withheld (RADIO_WAIT); the existing radio observer/retry remains.
8. Restore pressure deadline from remaining gameplay seconds, and verify owner
   readbacks before READY. Restoring never calls `EnterVictoryPressure`.
9. Open READY; prime the semantic publication signature at the saved revision,
   publish current projections and reconcile the single native objective task
   through the existing stable-ID task guard. No historical completion message.
10. Resume eligible M3 consumers and import escalation/cooldown state before their
    first queued tick. Terminal/INVALID restores do not initialize normal war.

The deliberate ordering adjustment is that strategic scalar fields precede
native derived reconciliation, while deadline scheduling and task publication
follow READY. This allows native services to read the final state while the
consumer gate is closed, and prevents task/expiry callbacks against partial state.

`SupplyIncomeTimer(true)` only resets the native arrival deadline (installed
source 1874–1879); it does not grant supplies. The reconciliation never calls XP,
base-capture messaging, historical remnants handling, native autosave/capture
events, response-wave generation or end-match replay. Native script-side service
behavior and replication must still be demonstrated at runtime.

### Objective, HQ, pressure and terminal semantics

- Saved commitment is restored quietly, without `Resolve` completion/invalidation
  history. Existing single-task/stable task ID wiring remains responsible for
  duplicate prevention; no saved task entity is reconstructed.
- VULNERABLE is the permanent latch in the existing enum; no separate bit is
  invented. Restored vulnerability does not restart a hold.
- Pressure uses saved remaining gameplay seconds. Offline wall-clock time is
  ignored. A bounded saved duration (1..86400 seconds) is retained while that
  restored hold is active, even if today's default differs. Subsequent genuinely
  new holds use current configuration. Production default is unchanged (600).
- Inactive pressure remains inactive during restore, including a 10/10 snapshot.
  Normal ownership/radio reevaluation may subsequently start pressure after READY.
- Replicated restore-presentation baseline primes client pressure/HQ latches;
  restored active pressure resumes the compact countdown rather than the large
  eight-second announcement. The next genuine pressure start clears this baseline
  flag. No UI layout/design resource was changed. Replicated arrival ordering and
  JIP presentation remain runtime-unverified.
- Terminal snapshots keep winner/outcome, clear legal objectives, retain the
  terminal capture-denial contract, do not initialize FIA/USSR war, and mark the
  historical end-match bridge already signalled. Native POSTGAME/menu behavior
  on a restored terminal snapshot remains a specific runtime acceptance item;
  no replay of historical terminal capture/end-match events is claimed.

### Minimal faction continuation and Phase 4 boundary

FIA retains capability strength, former-territory membership, loss chronology,
ownership epochs, unlaunched opportunity attempts/revision/FIFO order and admission
cooldown/serial. USSR retains per-target genuine-failure escalation and remaining
strategic cooldown. No factions are assigned to AI and no AI is forced active.

FIA in-flight leases and active CONTACT/COLLAPSE/POST_CAPTURE/PRESSURE_RECOVERY,
USSR/FIA operations, actors/vehicles, battle results and operation positions are
**not reconstructed**. Pending unlaunched intent is not an active operation.
Responses are initialized from current owners, not replayed capture history.
Native supply inventory, constructed facilities and physical radio buildings are
not a Phase 2B persistence payload; radio coverage is rebuilt from the resources
present in the loaded world. Runtime radio availability can therefore differ
after restart if later-phase physical state is absent.

### Save, dirty tracking, consistency and failure

The service's bound adapter captures a new complete document in one synchronous
authority invocation at SaveNow, before touching the temp file. Header identity,
UUID and creation timestamp are retained; live revision is copied with live state.
The initial fresh identity-only save is an internal creation flag, not a caller
supplied reason-string bypass. Every subsequent bound save requires gameplay READY.

Strategic publication marks dirty; a one-second bounded observation additionally
detects pressure time, FIA ledger/counters/cooldowns and USSR escalation/cooldown
changes. The periodic checkpoint remains 60 seconds. Observation is not disk IO.
Teardown observes/saves before consumers are destroyed. Payload equality now checks
all maps and ordered IDs when verifying temp/backup/commit, not just the header.

An incomplete/capturing/native-unavailable snapshot is refused, retains dirty and
does not replace the prior save. Restore errors latch FAILED before consumers,
and the adapter refuses shutdown/checkpoint saves of default/partial state. An
unexpected write/readback failure can leave partial live ownership in the failed
world; no rollback events or default-save overwrite are attempted. Restart/diagnosis
is required. Existing primary/backup/rejected evidence is preserved. The verified
copy-based writer is recoverable, **not a claimed fsync/atomic rename transaction**.

### Files and validation gate

New: `Scripts/Game/TB_CC_Persistence/TB_CC_StrategicPersistence.c`,
`Tools/Test-PersistenceStrategicRestore.py`.

Changed: persistence service/restore transaction; native campaign adapter;
objective commitment; FIA opportunity ledger; FIA coordinator's strategic
admission seam in `TB_CC_TerritorialOccupation.c`; USSR coordinator's strategic
failure/cooldown seam; pressure presentation's restore baseline;
P1/P2A regression assertions; this documentation.

World/layer changes: NONE. UI layout/design changes: NONE. No third-party code
copied. Reusable persistence/transaction code has no Arland IDs or coordinates;
scenario-specific registry/role resolution stays in the existing native adapter.
P3/P4/P5 were not implemented. Nothing published, deployed or committed.

Static/installed-API/model checks:

- `Tools/Test-PersistenceStrategicRestore.py`: P1 58 + P2A 81 + P2B 179 independent
  model cases, source wiring/order checks and inspected installed native API checks.
- `Tools/Test-EndgamePressure.py`: existing chained regression suite, including
  1024 ownership subsets, 20480 transitions, 29518 radio masks, ten-objective route,
  recapture/commitment, FIA/USSR/physical-virtual/chat/GM and HQ pressure regressions.
- Whole-tree textual/fixed-array/API readiness checks are not an Enforce compiler.

Required post-Workbench runtime matrix: fresh/v1/v2-empty campaign ID stability;
partial/full ownership restart; valid and invalid commitment with radio wait;
pressure both sides/inactive/vulnerable with offline pause; terminal both winners;
FIA FIFO/epochs and USSR escalation/cooldown; corrupt/foreign/incomplete snapshot
fail-closed; exactly one task; no historical XP/messages/remnants/waves; native
spawn/supply/radio/map agreement; restart during active operations clearly Phase 4.
Use isolated acceptance profiles, preserve existing evidence, and obtain explicit
authorization before running runtime tests. **WORKBENCH GATE REQUIRED.**

## Phase 3 — campaign-scoped player XP/rank (2026-10-13)

P1 WORKBENCH PASS; P2A WORKBENCH PASS; P2B ordinary strategic restart WORKBENCH
PASS as supplied in the P3 brief. P2B pressure/terminal restart scenarios remain
separate UNVERIFIED gates. P3 IMPLEMENTED / WORKBENCH PENDING. P4/P5 PENDING.

### Installed native authority and reconstruction evidence

Read from the installed vanilla `addons/data/data007.pak`, not external API guesses:

- `scripts/Game/Components/SCR_PlayerXPHandlerComponent.c`: controller-local,
  protected `m_iPlayerXP = 0`, owner-only `RplProp` (13–14); `GetPlayerXP()` (33);
  `GetPlayerRankByXP()` (81) uses the faction rank container; typed
  `GetOnPlayerXPChanged()` (107) invokes `(int playerId, int currentXP,
  int XPToAdd, SCR_EXPRewards rewardId)` after authoritative XP mutation.
- `AddPlayerXP(SCR_EXPRewards rewardID, float multiplier = 1.0,
  bool volunteer = false, int addDirectly = 0)` (437) is an AWARD path. It can
  process reward cooldowns, invoke XP events, update rank and send an owner reward
  popup. Even `addDirectly` (464–474) sends that popup. It is NOT used for restore.
- Native `SCR_PlayerXPHandlerComponentSerializer.c` (38) restores through that
  additive path. BCM deliberately uses a narrow modded protected-field setter
  for absolute reconstruction, then `Replication.BumpMe()` and native derived
  rank. It does not replay capture/kill/veterancy rewards or XP popup RPCs.
- `scripts/Game/Ranks/SCR_RankContainer.c:GetRankByXP()` (52–68) selects the
  highest eligible native threshold. `SCR_FactionManager.c:GetFactionRanks()`
  (221) resolves the player's faction or default ranks. No custom rank table.
- Installed `Configs/Ranks/BaseMilitaryRanks.conf`: default thresholds are
  renegade -51, private -50, corporal 150, sergeant 600, lieutenant 1200,
  captain 2100, major 3300. These are audit/model evidence only: production code
  uses the native faction container and does NOT hardcode them.
- `SCR_XPHandlerComponent.c:OnPlayerSpawnFinalize_S()` (91–115) updates rank
  silently on the existing controller. Death resets single-life XP (126 in the
  player component), not campaign XP. BCM leaves those native actions unchanged.
- `SCR_CharacterRankComponent.c:SetCharacterRank()`/`RpcDoSetCharacterRank()`
  retain native rank replication, consumers, permissions and special-rank logic.
  Native rank changes invoke consumers even when silent. The campaign listener
  honors silent; `SCR_DataCollectorAchievementsModule.c:RankedUp()` (479–490)
  does NOT honor it and can unlock MAJOR_PROMOTION during reconstruction.
  BCM therefore uses an authority-only reconstruction RPC, calls the native
  rank application under an entity-scoped guard on each receiver, and excludes
  only that historical achievement callback. Genuine promotions still use the
  unchanged native reward path. Restored-controller respawns reuse this silent
  path. This receiver-side guard requires real multiplayer acceptance.
- `SCR_XPInfoDisplay.c` (294, 320) reads authoritative/replicated native XP when
  showing/updating rank progress. No fake HUD rank or UI/layout modifications.

### Stable identity — no offline/name fallback

`SCR_PlayerIdentityUtils.c` (7–45) calls
`GetGame().GetBackendApi().GetPlayerIdentityId(playerId)`, with documented
server/after-audit lifecycle; it also synthesizes a name-derived UUID when the
backend ID is empty in a non-dedicated session (23–35). That polyfill violates
this persistence contract and is intentionally NOT used.

BCM calls the backend identity API only after native `OnPlayerAuditSuccess` or
`OnPlayerAuditRevived`, validates a non-null UUID, and canonicalizes it to lower
case. Audit failure/timeout/disconnect removes eligibility. Callback prototypes
come from installed `SCR_BaseGameModeComponent.c` (50/59/65/71/83).

Unavailable identity or audit defers binding and creates no player record.
Routine diagnostics contain only session player ID, XP/rank and defer reason;
no account UUID, name or secret. Session ID is diagnostic/routing only, never a
persistence key. The private JSON necessarily contains the account identifier;
protect profile storage as account-associated data.

**Workbench limitation:** an offline F5/PeerTool session without a real backend
identity cannot honestly prove progression persistence. It defers while native
gameplay continues. No guessed fallback is added. Authenticated Workbench and
dedicated identity availability must be verified at runtime. XP earned while
identity cannot be established is not attributed to a guessed account; a saved
record is authoritative once the verified account can bind.

### Compatible schema and coherent save flow

Root schema remains v2; empty v1 and existing v2 strategic saves stay loadable.
The reserved `players: array<string>` now contains canonical records:

`1|<backend-account-UUID>|<signed-native-XP>`

Record version 1; campaign scope comes from the root campaignId. No independently
stored rank, career, equipment, medals or unlock authority. Negative native XP
is allowed. Invalid version/UUID/noncanonical integer/duplicate identity rejects
the complete container before use. Future fields need explicit record migration.

P3 starts only after P2B derived reconciliation and the final strategic startup
contract. A one-second authority poll binds audited players when their native
controller/progression/rank infrastructure exists. Existing records set absolute
XP exactly once per controller lifecycle; absent records capture native default
progression, not a made-up starting value. Before a native award, the small XP
override resolves pending audited bindings, then delegates the original award
unchanged. Bound native XP-change events re-read the authority component rather
than trusting callback values, cache XP and mark the existing service dirty.

Disconnect captures and unsubscribes BEFORE native XP handler cleanup, then asks
the existing writer for a prompt checkpoint. Controller replacement flushes the
previous component and restores its successor. A repeated bind does not reapply
XP. Duplicate live identities defer until the previous owner is released.
Ordinary character respawn retains the controller's XP and only reconciles its
new character rank. Periodic polling also catches non-event changes; stop flushes
cached authoritative XP and removes poll/subscriptions before the service's
best-effort shutdown checkpoint.

Every write builds fresh strategic data and captures current player cache in
the SAME authority turn before selecting the candidate root. Disconnected
records stay cached. Player events never serialize an old strategic root;
strategic saves cannot erase player records. Full snapshot comparison includes
the sorted player section. Before P3 starts, saves preserve loaded records.
There is no separate save manager or client XP submission endpoint.

Storage remains `$profile:BCMConflictCore/Persistence/`, with 60-second dirty
checkpoints and verified temporary/primary/backup/recovery copies. Failed writes
retain dirty state and cached newer XP; prior recoverable files remain subject
to the existing P1 recovery behavior. Primary/backup recovery selects ONE whole
campaign container, including its player records; no cross-file player merge.
This is recoverable replacement, NOT guaranteed filesystem atomic rename.

### Future P5 isolation seam — not an admin reset implementation

`TB_CC_PersistenceService.ResetPlayersForAdoptedCampaign(newCampaignId)` requires
an explicitly adopted, valid, DIFFERENT root campaignId matching its argument.
It rejects ordinary restart/current-ID requests before touching XP. It validates
all tracked native components before resetting online XP to native zero and
single-life/survival counters to zero, clears old player records, changes only
the player cache's generation, and marks dirty. Disconnected accounts are absent
from the new generation and start normally on join. The old generation is never
searched for missing records.

Future P5 must explicitly archive/adopt the new root and handle storage lineage
before calling this seam, then checkpoint. No full campaign adoption, archival
workflow, admin command/UI or multi-save management is implemented here.

### P3 source/model gates and required engine acceptance

`Tools/Test-PersistencePlayerProgression.py`: 115 independent lifecycle/storage/
rank-boundary model cases plus source and installed API assertions. P1 58, P2A
81, P2B 179 retained. These Python models do not execute Enforce callbacks, JSON
IO, actual disconnect ordering, native permissions, rank RPCs or HUD rendering.

Full `Tools/Test-EndgamePressure.py` regression chain PASS: whole-tree syntax/
override/API readiness, fixed arrays (62 Game scripts/16 fixtures), 1024 ownership
states, 20480 transitions, 29518 radio masks, route/recapture/commitment/radio,
FIA/USSR, response phases, physical/virtual, HQ/endgame/UI, chat and GM.
Installed prototype checks now explicitly include the five P3 native classes;
no previous assertions were removed or relaxed. P1 return method count rises
from 19 to 21 for the two new service seams.

Required A–T engine acceptance after Workbench compile/resource PASS:

- A/B/C/D/E/F/G: default player, genuine XP award, disconnect/reconnect, clean
  same-profile restart/same campaignId, changed display name/session ID, and at
  least two authenticated account IDs with different XP. Check native rank/HUD.
- H/I/J/K: death/respawn retains XP without reaward; repeated binding is a no-op;
  no backend ID creates no record; native threshold/rank agreement including
  faction selection before/after join and silent native permission updates.
- L/M/N/O/P: real dirty checkpoint, isolated induced write failure/recovery,
  unchanged strategic fields, both dirty directions, empty v1/v2 compatibility
  with campaignId retained. Never perform corruption tests on real player saves.
- Q/R/S/T: explicitly adopted new-generation preparation rejects old-ID reset,
  resets online/absent players, join/disconnect/rejoin races, shutdown flush, and
  no historical XP/capture/kill/promotion achievement on any RPC receiver.

P3 changed files: new `TB_CC_PlayerProgression.c`, new
`Tools/Test-PersistencePlayerProgression.py`; modified `TB_CC_CampaignContainer.c`,
`TB_CC_PersistenceService.c`, `TB_CC_StrategicPersistence.c`, the native campaign
P3 startup/record-validation integration, `Tools/Test-PersistenceFoundation.py`,
`Tools/Test-ThreeFactionCorrectiveStatic.py`, and this document.
World/layer/UI resources: NONE. Locked gameplay rules, radio, AI, graph,
pressure/defence, chat and GM implementations: unchanged. P4/P5 unimplemented.
No server/client runtime, publish, deploy or commit performed.

**WORKBENCH GATE REQUIRED.**

## P3 live-snapshot repair and P3.5 native integration boundary (2026-10-13)

### Status and supplied-log attribution

P3 repair IMPLEMENTED; source/installed-API/independent-model checks PASS.
Workbench compile, actual repaired save/restore and dedicated acceptance NOT RUN.
P3.5 construction persistence NOT IMPLEMENTED: blocked at the native inline
deserialization completion/error contract described below. No visual-only
construction fallback or second storage manager was introduced.

The supplied session log establishes successful initial and periodic saves,
audited player binding at native XP 150/rank 2, and genuine XP dirty events.
The first periodic rejection follows Moss Hill reaching capture superiority,
but precedes its FIA-to-US ownership callback. That callback subsequently
commits revision 3 and advances the objective; the game continues normally.
The shutdown rejection occurs later. The old `snapshot-incomplete` boolean
discarded the failing invariant, so neither exact rejection was directly
identified by that old log. No raw player account ID is reproduced here.

**VERIFIED source defect:** `CaptureStrategicSnapshot()` reused the restore
validator, which rejected `base.IsBeingCaptured()` at every base. A live
unfinished capture does not invalidate the existing committed owner, revision,
objective or XP. The old guard rejected a valid live checkpoint solely because
historical ownership could not safely be APPLIED in that current world state.
The independent regression reproduces that specific false rejection.

**Strongly supported session attribution, not direct invariant telemetry:** the
first supplied periodic failure matches this defect's timing. Other invariants
could also have failed, because the old message erased them. The shutdown
failure's exact native dependency/order is UNKNOWN; the log alone does not prove
that a base or player component was already destroyed. The repair records the
actual section and reason for the next failure instead of assuming one cause.

### Exact repair and preserved integrity

- `ValidateStrategicSnapshot(snapshot, bool forRestore = true)` retains every
  payload/owner/HQ/pressure/FIA/authority/native-dependency invariant. Only the
  transient capture-in-progress restore precondition is conditional on
  `forRestore`. Live export explicitly passes false. Historical restore still
  uses true and rejects before any ownership write. No partial capture progress
  is fabricated or persisted; actual committed ownership remains authoritative.
- Export rejection reports section=strategic with its invariant or stable base
  ID, section=players with readiness/generation/record failure, or
  section=root-or-storage with container validation or write/rotation/readback
  stage. Routine logs never print backend player identities.
- Required export failures return before file IO, leave dirty set, and do not
  adopt the candidate. Successful whole-root readback clears dirty. Storage
  failures restore the previous in-memory root; existing verified recovery
  copies and P1 invalid-primary archival remain unchanged. This is recoverable
  replacement, not a newly claimed atomic filesystem transaction.
- Strategy plus player records are still captured in one synchronous authority
  turn, with no yield between sections. The same campaign UUID and full live
  committed strategy are retained. Current XP is observed from registered native
  components; missing account identity continues to defer, not invent a record.
- Player progression subscribes to `SCR_BaseGameMode.GetOnGameEnd()`. Installed
  `SCR_BaseGameMode.c:823-830` invokes `m_OnGameEnd` BEFORE calling additional
  game-mode components' `OnGameEnd`. This earliest exposed notification attempts
  a fresh coherent checkpoint before those component callbacks, without
  depending on their enumeration order. Stop removes this subscription, the
  poll and XP subscriptions. This does NOT prove all entities survive until
  world-end notification; actual Workbench F5/shutdown ordering is still a gate.
- If final fresh export fails, the store explicitly reports UNSAVED_CHANGES.
  It never labels a stale cached root current or overwrites disk using one.
  Periodic checkpoints work independently of shutdown. Abrupt termination can
  still lose changes since the last successful save.

Native XP/rank thresholds, award flow, quiet restoration, account audit,
disconnect/respawn handling, campaign-generation reset seam and replication are
unchanged. No new reward or player-submit RPC exists.

### P3.5 installed native construction evidence

All following evidence was read directly from installed Arma Reforger
`addons/data/data007.pak`; no third-party source was used or copied. Line numbers
refer to the installed resources, not BCM scripts.

| Native resource | Verified contract |
|---|---|
| `SCR_CampaignBuildingManagerComponent.c:369-416,517-546` | Config/catalog resource lookup and authority native BUILDING/provider placement; `GetOnEntitySpawnedByProvider()` at 893 is a provenance seam, not completion. |
| Same resource `784-816,825-855,863-911` | Native radius-based base registration, composition completion subscription, unregister event, base-scoped `GetBuildingCompositions(...)`. A composition can associate with multiple native bases; do not guess nearest. |
| `SCR_CampaignBuildingCompositionComponent.c:60-191,196-268,325-386,597-604` | Preview/site versus completed linked composition, native registration, replicated completion, destruction and root deletion/unregister, prefab/provider access. `SetIsCompositionSpawned()` is not a generic state restore API. |
| `SCR_CampaignBuildingLayoutComponent.c:216-229,360` | Native linked composition spawn and build-progress setter; building progress/reward callbacks must not be replayed as historical construction. |
| `SCR_EditorLinkComponent.c:50,76-81,100-134` | Native spawn suppression/deferred linked children and `GetLinkedChildren()`. Root presence alone does not establish child completion. |
| `SCR_ServicePointComponent.c:126-164,183-197` | Service registers with `SCR_MilitaryBaseSystem.RegisterLogicComponent`, takes native faction and subscribes to faction changes; linked-children event drives registration. Completion/base association/online state must be checked, not manually registered twice. |
| `Configs/Systems/Persistence/Configuration/Building/BuildableComposition.conf` | Existing native `BuildableEntitySerializer`, composition-progress and editor-link serializers. Native construction persistence genuinely exists. |
| `SCR_CampaignBuildingCompositionComponentSerializer.c:10-37,41-64` | Serializes native progress; restore sets building value or spawns linked composition. This alone does not serialize every child facility, supply or damage state. |
| `SCR_EditorLinkComponentSerializer.c:16-85,108-162` | Requires matching authored link-entry count; skips living untracked children; serializes changed tracked children and removed children; applies nested load/removal. |
| `SCR_ResourceComponentSerializer.c:21-119,123-166` | Native resource containers/defaults and restored values; skips encapsulated containers. Shared base supplies cannot be treated as independent per-building balances. |

Native building membership/service logic is the functional route for barracks,
maintenance, arsenal, storage and radio facilities, not a BCM parallel service.
Actual facility-specific interaction, damage/ruin reconstruction, supply
encapsulation ownership and JIP remain UNVERIFIED. No claim is made that a
spawned prefab, online flag or registered root proves those facilities work.
Native network RplSave/RplLoad is not a disk save contract.

### Specific native API limitation under BCM's single-root contract

`scripts/Game/generated/Plugins/Persistence/System/PersistenceSystem.c` exposes:

```c
proto external ESerializeResult Serialize(notnull Managed entityOrState, notnull SaveContext context, SerializerDefaultSpawnData defaultData = null);
proto external ref Managed DeserializeSpawn(notnull LoadContext context);
proto external void RequestSpawn(notnull PersistenceSpawnRequest spawnRequest, PersistenceResultCallback callback = null);
```

Serialize requires tracked/configured entities (70-71). DeserializeSpawn has
NO callback overload (77-78). Deferred-deserialization completion tasks are
ignored without an original request callback (80-85). `WhenAvailable` explicitly
does NOT guarantee all deferred tasks completed (89-97). Default serializer
failure handling is IGNORE in `PersistenceSerializerBase.c:16-23`, whose
constructor is private/internal. Therefore a returned Managed/root or availability
callback is not a verified complete child-deserialization result.

The callback-bearing RequestSpawn fetches from the native system's save-data
(20-21). `PersistenceSpawnRequest.c:12-19` accepts collection, include/exclude
UUIDs and paging, NOT an inline LoadContext/root JSON payload.
`PersistenceLoadRequest.c:12-15` accepts instances, likewise not inline data.
`PersistenceResultCallback.c:12-26` gives request results but cannot be passed
to DeserializeSpawn. Using RequestSpawn as-is would introduce a separate native
storage lineage rather than atomically validate strategy/XP/buildings in BCM's
existing root. That change is outside the accepted storage contract.

**Blocking invariant:** complete all mandatory nested facility/damage/removal/
supply restoration with an observable success/failure boundary BEFORE strategic
READY and before replacing a recoverable unified save. The available inline
bridge does not expose that completion/error boundary; native child omission and
default tolerated errors make root-return/prefab-presence checks insufficient.
Bounded polling of composition completion/base membership would not itself
detect all those nested failures. Direct serializer construction, a fictional
DeserializeSpawn callback, or prefab-only reconstruction would be unsupported or
fail the requested contract. No such shortcut was implemented.

This is a limitation of the verified public bridge under the current contract,
NOT a claim that Reforger cannot persist buildings. Native system availability,
tracking selection and facility completeness in this scenario still require
an isolated engine test; no production configuration was silently changed.

**Smallest safe next step:** authorize an isolated native-persistence integration
fixture/profile using the installed buildable/composition/resource/damage
configuration, one completed barracks and one maintenance point. Verify linked
child tracking, deletion/damage, supply ownership, callback completion and
native functional registration. To proceed with BCM's unified JSON root, obtain
or establish a supported strict inline completion/error contract (including
nested failures); otherwise explicitly approve a redesigned native-backed
single authoritative storage transaction. A native sidecar writer is not the
current implementation. Production world edits require separate approval.

### Construction contract decisions — deferred, not shipped functionality

Intended provenance policy is native server-validated provider placement plus
actual completion. Exclude authored infrastructure, BCM procedural defence,
previews/helpers and incomplete sites. Track genuine removal/root destruction;
persist a repairable ruin only through a verified complete native damage
contract. Never infer construction eligibility from prefab names or faction
alone. Native faction/base associations and capture rules must remain native.

No construction record format, export cache, restore registry, reward suppression,
generation-reset seam or JIP reconstruction was shipped. The minimum proposed
future record is a versioned structure UUID plus catalog resource, native
association identity and complete validated native payload, scoped to the root's
campaign/map. Transform, damage and independently owned supply state must be
validated in that payload. Runtime pointers/Rpl IDs/names are not persistence
keys. This is a proposal, not a reader/writer contract.

Inspection confirms root v2 has NO reserved structures section. Schema remains
v2, existing v1/v2 compatibility is unchanged; no premature v3 migration or
fabricated empty construction defaults were added. Construction would require
an explicit next-schema migration retaining campaign UUID and old records.
P3's existing adopted-generation reset affects players only, not buildings.
No unified construction+XP+territory snapshot, removal/reset or multiplayer/JIP
construction guarantee is claimed. P4/P5 remain unimplemented.

### Validation and Workbench acceptance

`Tools/Test-PersistenceSnapshotRepair.py` PASS: source/API assertions plus all
16 requested P3 regression categories in independent models. It reruns P1 58,
P2A 81, P2B 179 and P3 115 cases. It reproduces the old capture-only checkpoint
rejection and confirms live save versus restore isolation, unchanged coherent
payload, failed-file preservation/dirty state, successful clearing, two accounts,
disconnect/shutdown/revision/generation cases. Models do NOT execute Enforce or
prove real native rank/rewards/shutdown/disk IO.

`Tools/Test-ConstructionPersistenceNativeBoundary.py` verifies 24 installed-source
contracts. This is API-boundary evidence, NOT P3.5 model/runtime PASS. The requested
24 functional construction acceptance cases were NOT RUN: implementation is
blocked, not silently substituted with prefab-presence tests.

Full `Tools/Test-EndgamePressure.py` chain PASS: existing whole-tree source/API
readiness, fixed-array 62 Game scripts/16 fixtures, 1024 ownership states,
20480 transitions, 29518 radio masks, route/recapture/objective commitment,
native radio/capture, FIA/USSR/response, physical/virtual/fortifications,
HQ/endgame/600-second pressure/UI, chat and GM. No existing assertions weakened.
These are conservative static checks, not an exhaustive Enforce compiler.

Workbench plan (use a backed-up isolated profile; never corrupt a real save):

1. Compile/resource gate first. Test A: earn legitimate XP during active capture,
   wait for SAVE SUCCESS, restart same campaign/profile, verify latest saved
   XP/native rank, ownership/revision/committed objective. Record actual failure
   section/reason if any. Check pre-capture and post-callback checkpoints.
2. Test C's territory+XP part: capture and earn XP together, save/restart and
   verify both. Construction portion is BLOCKED, not an acceptance-ready feature.
3. Test shutdown twice: immediately after XP, then after capture. Verify the
   world-end checkpoint if emitted and inspect explicit UNSAVED_CHANGES; compare
   recovered state to latest successful save, not unpersisted live values.
4. Test E's progression part: two authenticated dedicated accounts with distinct
   XP, reconnect/respawn/JIP, no repeated awards. Construction replication part
   remains blocked. Offline name identity is not an accepted substitute.
5. Test F: on separate copies/profile only, test primary/backup recovery; verify
   one whole root and no campaign/XP/strategy mixing. Preserve latest known-good
   originals. Existing pressure-countdown/terminal P2B restart gates still pending.
6. Tests B/D and construction portions of C/E require the native integration
   blocker to be resolved; do not advertise them as implemented tests to pass.

This pass modified four scripts: `TB_CC_StrategicPersistence.c`,
`TB_CC_PersistenceService.c`, `TB_CC_PlayerProgression.c`, and
`TB_CC_M4XP_NativeArland.c`. Created the two test tools above; updated this document.
No world/layer/UI/config/prefab edits, gameplay tuning, radio/AI/capture/HQ/chat/GM
implementation changes, real save edits, publishing, deployment or commits.

**P3 repair: WORKBENCH GATE REQUIRED. P3.5: BLOCKED — NATIVE API LIMITATION.**

## P5 campaign administration and restart reset (2026-10-13)

IMPLEMENTED — SOURCE/INSTALLED API/INDEPENDENT MODEL PASS.
P5 Workbench compilation succeeded per user report; initial native registration
failed for both commands (missing OnUpdate). The registration repair below is
STATIC/API checked; Workbench command registration RETEST and interaction/IO,
dedicated Windows/Linux/AMP acceptance remain pending.
The P5 brief reports successful P3 progression restoration and world-end save;
that is user-supplied acceptance, not a new runtime test performed in this pass.
P3.5 construction and P4 operation reconstruction remain NOT IMPLEMENTED.

### Administrative interface and exact authority evidence

The minimal **BCM Campaign Administration** interface uses the existing native
administrator command transport, not a replacement admin menu or custom
client-to-server RPC. No proven menu-extension/resource seam was selected; the
installed server-command surface supplies both administrator chat feedback and
RCON support without editing native menus or production resources.

Installed `scripts/Game/generated/ScrServerCommand.c:9-24` defines keyword,
server-side routing, protected chat/RCON execution callbacks and separate native
permission requirements. Installed `BanCommands.c:432-476` and `579-644` provide
the exact working native pattern: ADMINISTRATOR chat role, PERMISSIONS_ADMIN
RCON role, server execution, client forwarding and ScrServerCmdResult feedback.
`Global/Functions.c:1918-1921` defines SCR_Global.IsAdmin using server-side native
ADMINISTRATOR or SESSION_ADMINISTRATOR roles. Generated `PlayerManager.c:61-64`
provides HasPlayerRole/GetPlayerRoles. Listed-admin status alone is NOT accepted;
`SCR_PlayerListedAdminManagerComponent.c:66-69` expressly distinguishes listed
players from administrators who have logged in.

- `bcmcampaign`: native ADMINISTRATOR chat gate and native ADMIN RCON gate.
  Server chat callback independently rechecks SCR_Global.IsAdmin, and the
  service rechecks it again at each request, including confirmation. Context
  playerId comes from the engine callback, never argv/client-provided flags.
- `bcmcampaignlocal`: native SESSION_ADMINISTRATOR chat gate for Workbench/local
  offline play ONLY (permission repair below restricts ALL multiplayer modes);
  denied on listen/dedicated/client sessions and denied through RCON.
  It uses the same independent server role checks. No debug/editor/GM fallback,
  guessed player ID, auto-granted role or unauthenticated console backdoor.
- RCON has no per-client player/role identity in its callback prototype. Its
  protected engine callback is gated by RequiredRCONPermission for EVERY request.
  The service source is a constant supplied by that callback, not an argument.
  Audit records say RCON_ADMIN/session=0; they do not invent an operator identity.
  Protect RCON access as shared administrator authority.

Authoritative store registration starts after container startup and is removed
on Stop. No active store or unavailable strategic/player source rejects mutation.
Clients have no BCM administrative request RPC. The only new replicated field
is a fixed reset-pending notification on native SCR_GameModeCampaign. Native
`SCR_HintManagerComponent.c:380` renders it, with a bounded readiness retry and
world-end cleanup. It contains no paths, account records, nonce or admin control.

Operator login/RCON permission configuration follows the official
[Reforger server management](https://community.bistudio.com/wiki/Arma_Reforger:Server_Management)
and [server configuration](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config)
documentation. These links supplement, not replace, installed 1.8 prototypes.
Native command registration/transport and Workbench host role availability must
be verified at the engine gate; no live administrative interaction was run here.

### Usage (no administration has been executed by Codex)

In native chat, log in through normal server administration, then:

```text
#bcmcampaign status
#bcmcampaign save
#bcmcampaign save <requestUUID>
#bcmcampaign new <currentCampaignUUID> <mapIdentity>
#bcmcampaign confirm <currentCampaignUUID> <mapIdentity> <returnedToken>
```

For locally authorised offline Workbench testing, use the SAME subcommands with
`#bcmcampaignlocal`. A local player must genuinely have the native session-admin
role. Opening GM/editor is insufficient. `#roles` helps inspect the native role.

For authenticated native Reforger administrator RCON, send `#bcmcampaign` with
the same arguments through the RCON client's command field. This is an engine
script/server command, NOT the unrelated `@logout` RCON protocol command. An AMP
terminal that merely writes process stdin is not a verified command transport;
use authenticated native RCON or a logged-in in-game admin. Confirm actual
argument forwarding in the dedicated acceptance test before destructive use.

Status reports campaign/map, saved and observed revision, dirty/state, owner
counts and US-owned stable IDs (explicitly INCLUDING HQ), and whether values are
live or from the last container. No player identities/save paths are displayed.
Save uses the existing coherent writer; success is returned only after verified
readback. Explicit requestUUID deduplicates the last 32 requests per store,
keyed by authority source/session/campaign/request. Duplicate failure returns
the original failure; use a new requestUUID to retry. Plain `save` is a fresh
checkpoint each time; neither form changes gameplay or awards XP.

New only previews; it does not generate a new campaign or touch disk. The
warning explicitly covers all territory and every account's campaign XP/rank,
retained archive, restart requirement and absent construction persistence.
One global confirmation challenge is bound to the requesting source/session,
campaign, map, revision, committed owners and exact sorted XP records. It expires
after 120 seconds. Another admin's proposal replaces it. Changed territory/XP,
wrong actor/map/token or expired consent rejects and consumes the challenge.
Clock/cooldown passage alone is not a changed destructive scope. Confirmation
is re-authorised and accepted only once. Repeated confirmations while staged
are rejected; they do not create additional campaign UUIDs.

### Versioned restart transaction and storage ordering

Campaign root schema remains v2; existing v1/v2 readers/migration are unchanged.
The independent control journal uses version 1 and mandatory fields:
`phase, oldCampaignId, newCampaignId, mapIdentity, scenarioIdentity, configIdentity,
requestedAt, committedAt, archivedRevision, authoritySource, authoritySession`.
All identity/config fields must match the current adapter. UUIDs are non-null,
old/new differ, timestamps/revision are bounded valid integers, and source/session
must match the native chat/RCON audit convention. No arbitrary path is accepted.

1. After valid confirmation, force a coherent current-generation checkpoint.
   Any strategic/player/root/write failure prevents staging and retains dirty.
2. Freeze persistence writes, not the running battle. Copy the validated primary
   to `$profile:BCMConflictCore/Persistence/archives/`
   `<oldUUID>.<timestamp>.<newUUID>.save.json`. Name collisions reject, never
   overwrite an archive. Re-read and compare the complete root. Write/readback
   its `.reset.json` audit companion before any pending intent.
3. Write/readback `campaign.reset.pending.prepared.json`; copy/readback to
   `campaign.reset.pending.json`, then `.backup.json`. Only after all copies
   verify, report RESET_PENDING_RESTART and notify players. Current UUID/XP/
   ownership remain unchanged until restart. Old periodic, disconnect and
   world-end saves are blocked. Progress after staging is deliberately unsaved.
4. A pending-write failure leaves the session write-blocked and reports recovery
   required. A valid prepared copy is already a resumable authorised intent;
   restart MAY activate it even if the command returned staging-incomplete.
   If all journal copies are unusable, startup fails closed and preserves files.
   Archive/audit failure BEFORE journal writes leaves the old campaign usable;
   partial/orphan archive attempts are retained, not automatically deleted.
5. On normal authoritative startup, inspect journals BEFORE loading a historical
   root. Select identical verified copies or a strict old-to-new lineage when a
   newer prepared intent coexists with older copies. Wrong map/config/version,
   unrelated transactions or inconsistent receipt times fail closed. Verify the
   old full archive and its exact audit companion before activation.
6. An uncommitted intent creates an EMPTY new-UUID container IN MEMORY only. It
   does NOT load old ownership, XP, objective/HQ/pressure or histories. The
   existing map adapter runs its ordinary authored-world baseline initialization
   and all normal native ownership/role/spawn/objective readiness checks. No live
   setters, historical capture callbacks or guessed baseline data are replayed.
   Baseline ownership failure fails closed before writing a new campaign.
7. After strategic readiness and player-service startup, capture/validate a FULL
   fresh baseline snapshot. Copy/readback it to a transaction-specific activation
   recovery file, then primary, backup and rotation. The immutable old archive
   protects every replacement step; no filesystem atomic rename is assumed.
8. Only after the full new root is durably saved/read back, write/readback the
   matching versioned COMMITTED journal using prepared/primary/backup copies.
   Clear activation state and report ACTIVATED. Only then allow M3/war consumers;
   the indirect call from initial reevaluation is also gated while activating.
   A save/receipt failure latches startup FAILED; it does not start the war.
9. Journals remain as a lineage fence, not deleted markers. On subsequent normal
   restart, a matching committed receipt requires primary/backup recovery to
   resolve ONLY the new UUID. Never fall back to archived old progress. Native
   ordinary restore then retains the new campaign's subsequent progress. A later
   explicitly confirmed reset creates the next linked transaction, not a new UUID
   on every startup. Archive files/audit companions remain immutable history.

File primitives are installed Core `generated/System/FileIO.c:32-38`:
FileExists, MakeDirectory and CopyFile into $profile. JsonSaveContext/LoadContext
write/readback uses existing engine APIs. Forward-slash virtual profile paths
are platform-independent; no Windows-only shell/rename assumption exists.
Readback proves recoverable content, not fsync/power-loss atomicity. Linux/AMP
profile permissions, persistent mounts and native IO still require runtime tests.
No save file, journal or real campaign has been modified during this coding pass.

### State coverage, player isolation and portability

Restart creates new native component/service instances: authored base owners,
normal initial radio-valid committed objective, initial native revision/history,
protected HQs, no pressure/vulnerability/victory, seeded fresh FIA queues/epochs/
responses and fresh USSR failures/escalation/admission. Native graph, objective
commitment, hostility, capture superiority, spawn restrictions, 600-second hold,
AI composition/budgets and physical/virtual policies are NOT retuned.

Player cache starts empty under the new UUID. For explicit reset activation,
the player service latches a new-generation first-bind policy: each available
audited account with no new record quietly resets native XP and single-life/
survival counters to zero, including a controller retained by scenario restart.
Rank/HUD reconciliation uses the existing silent native P3 path; no historical
award/achievement/capture reward. Repeated binds use the now-cached new record,
not repeated zeroing. Missing audit/identity/controller/rank readiness defers;
unverified activity before binding is not attributed to a guessed account.
Disconnected accounts have no old record in the new root and receive native
default progression on joining. Old account records exist only in the archive.

ResetPlayersForAdoptedCampaign remains available for a future supported live
adoption, but is NOT misused to reset online players in the old staged session.
Restart adoption starts a new cache rather than editing an active battle in
place. No construction generation-removal seam is claimed. P3.5/P4 are absent,
reserved operation records remain empty, and future unsupported schemas are
rejected rather than silently discarded. Authored infrastructure is unchanged;
unpersisted runtime facilities follow ordinary scenario restart behavior.

Core administration contains no map-specific IDs/coordinates. Identity/config
comes from the existing adapter; baseline and initial objective come from its
normal startup, not core hardcoded owners. Future adapters must call the same
completion hook after their own authored baseline is coherent and before war
consumers start. Session-local Rpl IDs, names and raw backend account IDs are not
administration identities or public logs. Private archives remain sensitive
account-associated data and must retain profile storage access controls.

### Checks and remaining runtime gates

`Tools/Test-CampaignAdministration.py` PASS: 24 categories / 95 independent
authority, stale-consent, crash/copy/readback, recovery and future-map cases.
Includes logged/local/RCON authority and forged/GM/editor/monitor denial,
duplicate save/confirmation, archive and journal failures, generation adoption,
authored ownership/objective/XP/FIA/USSR/HQ/pressure/terminal reset, old-write
blocking, activation replay, torn markers, unrelated/wrong-map and normal restart.
Source assertions tie ordering to actual code and installed callback/role/IO
contracts; models are NOT native execution, real files or facility functionality.

Retained P1 58, P2A 81, P2B 179, P3 115 and P3-repair 16-category checks PASS.
Full EndgamePressure regression chain PASS: 64 Game scripts/fixed-array 16
fixtures, native override prototypes, 1024 ownership states, 20480 transitions,
29518 radio masks, route/recapture/objective commitment, radio/capture/FIA/USSR,
response/physical-virtual/fortifications/endgame/pressure/UI/chat/GM. Administration
is consolidated into the existing persistence service; no local-class modding
is required and the installed-only modded-target audit remains unchanged.
Added native ScrServerCommand to exact override-prototype coverage and extended
the exact return-readiness count from 21 to 37 for 16 P5 service contracts.
No assertions weakened.

Workbench acceptance (isolated copied profile; destructive tests never on the
latest only known-good campaign):

1. Compile/resource gate. Verify both command keywords register exactly once.
   Obtain legitimate native role; prove non-admin and editor-only denial. Check
   local session-admin command, and deny logged-out/role-revoked confirmation.
2. Change XP/ownership, issue status/save, verify SAVE SUCCESS and restore the
   same UUID/XP/rank/strategy on ordinary restart. Replay an explicit save nonce
   without a second writer transaction; a failed nonce needs a new nonce to retry.
3. Request new with exact UUID/map. Observe warning/token. Test wrong map/token,
   expired/changed revision/XP, another caller and duplicate confirmation denial.
4. Confirm intentionally on that test profile. Verify full archive/audit and
   prepared/primary/backup journals, player hint, RESET_PENDING_RESTART and no
   further periodic/disconnect/world-end primary replacement.
5. Restart/F5 normally. Verify ACTIVATED only after full save and receipt,
   DIFFERENT UUID, authored owners, initial valid objective, fresh history,
   protected HQ/no pressure/terminal, XP zero/native rank/HUD for retained players.
   Earn new XP; disconnect/rejoin; ordinary restart must retain this new progress.
6. Repeat from separate pressured and terminal test campaigns. Induce copy/write
   failures only in disposable profile copies; require old recovery/archive or
   validated resumable intent. Corrupt all markers/wrong-map => fail closed.
   Never manually remove reset markers just to make startup succeed.

Dedicated Linux/AMP acceptance, SEPARATE from Workbench:

1. Use an isolated instance/profile with durable mounted profile storage and
   correct filesystem permissions. Back up originals. Authenticate native RCON
   with administrator permission; permit bcmcampaign in the command filter if
   a whitelist is configured. Verify native command routing/argv and monitor
   denial; do not assume AMP's process console routes RCON commands.
2. Two authenticated accounts: distinct XP; logged administrator vs non-admin
   and GM-only player; RCON admin vs monitor. Check source/session audit metadata
   without account IDs or secrets. Try confirmation after admin logout/revoke.
3. Repeat save/ordinary restart/new warning/confirmation/staging. Verify all
   clients receive the generic pending notice, including a late join, and no old
   generation checkpoint overwrites files after staging.
4. Restart with AMP's authorised instance controls while preserving exactly the
   same profile mount/map/config. BCM does not restart the process. Verify UUID,
   baseline/history/HQ/pressure/XP/rank, retained-client scenario restart, fresh
   process reconnect and JIP; no duplicate rewards. Main local command is denied
   on dedicated servers. Follow with another ordinary restart and another reset.
5. In disposable copies only, terminate between journal/root/receipt copies,
   induce permission failure/torn JSON/missing primary/bad backup and wrong-map
   markers. Verify strict recovery/lineage and no old XP/territory resurrection.
   Record actual Linux IO, networking and AMP results independently.

Limits: no custom button menu, automatic process restart, archive-pruning policy,
individual RCON-client identity, fsync/atomic rename guarantee, P3.5 facilities or
P4 actors. Explicit save nonce deduplication is bounded to the last 32 requests
in the current store. Confirmations are one global 120-second proposal. Native
role availability, command routing, retained-controller reset, hints/JIP and all
real IO/recovery still require the gates above.

Changed files this pass: NEW `TB_CC_CampaignAdministration.c`, NEW
`TB_CC_CampaignAdminCommand.c`, NEW `Tools/Test-CampaignAdministration.py`;
MODIFIED `TB_CC_PersistenceService.c`, `TB_CC_PlayerProgression.c`, native map
adapter startup/M3 activation gate, `Tools/Test-ThreeFactionCorrectiveStatic.py`,
`Tools/Test-PersistenceFoundation.py`, and this document. World/layer/layout/
prefab/config resources: NONE. No publish, deploy, commit or live administration.

**IMPLEMENTED — STATIC/API/MODEL PASS — WORKBENCH GATE REQUIRED.**

### P5 command registration repair (2026-10-13)

User-reported engine errors: both TB_CC_CampaignAdminCommand and
TB_CC_LocalCampaignAdminCommand were missing the required OnUpdate API definition.
Compilation alone did not detect this registration failure. The BCM base command
omitted OnUpdate, so its local subclass had no implementation to inherit either.

Installed 1.8 evidence re-read directly from vanilla `data007.pak`:

- `scripts/Game/generated/ScrServerCommand.c:9-24`: eight event definitions,
  including `event protected ref ScrServerCmdResult OnUpdate();` at line 12.
  Line 11 says `Update PENDING request.` It takes NO arguments and returns a
  reference-qualified ScrServerCmdResult, not void/bool or a result enum.
- `scripts/Game/generated/ScrServerCmdResult.c:9-15`: the constructor takes
  `(string sResponse, EServerCmdResultType eResultType)` and stores both fields.
- `scripts/Game/Network/ServerCommands/BanCommands.c:575-654`: native KickCommand
  implements all eight events, GetKeyword returns `kick` without `#`, server-side
  routing is true, chat requires ADMINISTRATOR and RCON requires PERMISSIONS_ADMIN.
  Its synchronous execution returns final OK/ERR/PARAMETERS; OnUpdate at 649-652
  returns `ScrServerCmdResult(string.Empty, EServerCmdResultType.OK)`.
- The same resource's BanCommand returns PENDING for asynchronous backend work
  (186/217/249). OnUpdate at 561-572 polls callback state, returning PENDING while
  waiting and OK/ERR when complete. It does not rerun the original request.
- Both native examples are ordinary global classes derived from ScrServerCommand;
  no command registration attribute or separate per-command resource is present
  in these implementations. The closed native registrar itself is not script
  source; actual discovery/binding remains an engine retest, not a static claim.
- `scripts/Game/Global/Functions.c:1918-1921`: server role recheck accepts native
  ADMINISTRATOR/SESSION_ADMINISTRATOR, not GM/editor permission.

Smallest source correction: main command adds the exact native synchronous
OnUpdate return above; local command explicitly overrides the same signature and
delegates to `super.OnUpdate()`. All BCM execution paths already return final
OK/ERR. Neither update callback executes commands, repeats writes, polls/reset
transactions or returns PENDING. No lifecycle state or deferred processing was
introduced. The journal phase string PENDING is unrelated to command-result
PENDING and is unchanged.

Registration audit checks all eight installed signatures (including ref and
argument declarations) on both concrete classes through their inheritance chain.
Local inherited server/client/RCON-permission methods resolve to the BCM base;
its keyword, session-admin gate, dedicated denial and RCON rejection remain
explicit. GetKeyword names are unique in the local Game tree and exclude `#`;
native chat uses `#bcmcampaign`/`#bcmcampaignlocal`. Client forwarding, protected
native role requirements (public override widening follows native Kick), server
role rechecks, constant authority context and unchanged argv[1] dispatch for
status/save/new/confirm are verified at source level. No further registration
API omission/signature hazard was found. No native engine authorization was run.

Validation: P5 24 categories/95 independent authority/stale/crash/recovery cases;
new registration tests cover 2 x 8 native event bindings and 18 rejection fixtures.
P1/P2A/P2B/P3/snapshot-repair suites and the endgame/response/objective/native
radio/FIA/USSR/physical-virtual/chat/GM/static regression chain rerun successfully.
These are installed-source/static checks and independent models, NOT Enforce
compilation, engine registration, real command transport, real save/reset IO or
runtime acceptance.

This repair changes ONLY `TB_CC_CampaignAdminCommand.c` (both OnUpdate methods),
`Tools/Test-CampaignAdministration.py` and this document. Schema/reset transaction,
worlds/layers/resources, capture/objective/radio/XP/AI/HQ/GM/chat are unchanged.
No administration command or campaign reset was executed; no publish/deploy/commit.
Retest both native registrations after recompile/reload, then check permitted
status/save routes and denial for non-admin/GM-only callers; local must reject
dedicated/RCON and monitor RCON must not execute main. New/confirm dispatch is
source/model checked only; do NOT reset a live campaign to test this repair.

**STATIC/API PASS — WORKBENCH COMMAND REGISTRATION RETEST REQUIRED**

### P5 Workbench administrator permission repair (2026-10-13)

User reports native registration now succeeds, but `#bcmcampaignlocal status`
returns `You are not allowed to execute that command 'bcmcampaignlocal status'`.
That response is not emitted anywhere in the BCM command/store. BCM rejection
strings explicitly mention native admin/local controller/offline/store status.
The observed failure is native pre-dispatch permission enforcement, not BCM's
AdministrationRequest parser. The local keyword requires SESSION_ADMINISTRATOR;
Workbench/editor availability alone does not supply that role to the caller.
The precise native caller role snapshot was not supplied, and the C++ registrar/
permission-enforcement bodies are not exposed by the installed generated script
API. The new diagnostics confirm the exact boundary and native role booleans at
the runtime retest; no claim is made about an inspected C++ implementation.

Verified installed API/resource evidence, in vanilla data007.pak unless noted:

- `scripts/Game/generated/ScrServerCommand.c:15-24`: native RCON/chat permission
  callbacks, client-first execution and server-side forwarding. Existing complete
  eight-event registration surface and both OnUpdate bindings are retained.
- `scripts/Game/Network/ServerCommands/BanCommands.c:593-637`: native Kick uses
  PERMISSIONS_ADMIN RCON, ADMINISTRATOR chat and OK-returning client forwarding.
- `scripts/Game/generated/Network/PlayerManager.c:55-74`: GetPlayerController,
  IsPlayerConnected, HasPlayerRole and authority-only
  `proto external bool GivePlayerRole(int iPlayerId, EPlayerRole role);` (69).
  GetPlayers(out notnull array<int>) is at 30.
- `scripts/Game/Components/Voting/SCR_VotingAdmin.c:13-16`: native voting grants
  the real SESSION_ADMINISTRATOR role using GivePlayerRole. No invented role API.
- `scripts/Game/generated/ChimeraGame.c:16-18` and
  `generated/Player/PlayerController.c:52-55`: engine-owned local controller,
  player manager, GetPlayerId and HasRole.
- core/data.pak `scripts/GameLib/generated/Game.c:102-105`: InPlayMode;
  `RplSession.c:19-20`: Mode; `RplMode.c:8-14`: None/Client/Listen/Dedicated.
  `SCR_CameraLimitedEditorComponent.c:1-3,106` uses WORKBENCH compile guards and
  explicitly classifies every mode except None as multiplayer.
- `scripts/Game/Global/Functions.c:1918-1921`: SCR_Global.IsAdmin rechecks actual
  ADMINISTRATOR or SESSION_ADMINISTRATOR roles, not GM/editor access.

Smallest safe correction avoids lowering either native command permission gate
or introducing a new privileged dispatch channel. Added
TB_CC_WorkbenchCampaignAdmin inside `#ifdef WORKBENCH`, with two explicit static
console methods: DiagnoseLocalHost (read-only) and AuthorizeLocalHost. The grant
requires running offline play, a live world, server authority, RplMode.None, a
positive engine-owned local-controller ID, exactly one connected player matching
that controller, and matching manager/controller references. It grants ONLY the
native session-admin role through GivePlayerRole and verifies both native manager
and controller role checks. No ID/role/flag parameters, RPC, automatic startup
grant, store dispatch or editor/GM privilege test exists. No retail/dedicated
build helper survives the WORKBENCH preprocessor boundary. Workbench attached to
any client/listen/dedicated session rejects the grant. Reinvoke deliberately for
each fresh offline test session; do not infer any saved/persistent admin role.

The local command additionally rejects all multiplayer modes even for native
admins, requires offline play/world, and validates the engine callback playerId
against the registered local controller plus native SESSION_ADMINISTRATOR before
delegating through the existing server and service role checks. Production normal
bcmcampaign remains ADMINISTRATOR chat/PERMISSIONS_ADMIN RCON; authenticated remote
admins use that normal route. Local RCON remains denied. Status/save/new/confirm
parser, confirmation actor binding, archive/reset/schema semantics are untouched.

Diagnostics `[TB_CC_P5_AUTH]` identify native requirement queries, chat client
entry, local/server role checks, RCON entry/rejection, dispatch availability/result,
Workbench environment/native-role booleans and grant outcome. No command argv,
account/player IDs, save paths, replies or confirmation tokens are logged. A
NATIVE_CHAT_REQUIREMENT marker alone is NOT handler entry or a proof of denial;
the engine may query it during registration. Native denial with no CLIENT_ENTER
means rejection before BCM execution; CLIENT_ENTER without SERVER_ENTER indicates
rejection/forwarding failure before server execution; explicit *_CHECK DENIED
means BCM rejection. DISPATCH failure can be store readiness/IO, not permission.

Workbench acceptance (NOT executed by Codex): compile/reload, start normal OFFLINE
local play with one player, open Script Editor Remote Console targeting the
running **Game**, not the Workbench tool module or any network server. This
console supports executing scripts while in play mode: see official
[Enfusion World Systems](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/Page_WorldSystems.html)
and [Reforger Script Editor](https://community.bistudio.com/wiki/Arma_Reforger:Script_Editor).

```c
TB_CC_WorkbenchCampaignAdmin.DiagnoseLocalHost();
TB_CC_WorkbenchCampaignAdmin.AuthorizeLocalHost();
TB_CC_WorkbenchCampaignAdmin.DiagnoseLocalHost();
```

Require `mode=0` (installed RplMode.None), authority/play true, connectedCount=1,
localRegistered true, and nativeSessionAdminVerified=true. Then use native chat:

```text
#bcmcampaignlocal status
#bcmcampaignlocal save
```

Do not execute a reset for this retest. New/confirm dispatch remains covered by
the existing independent source/model tests. If grant returns false or native
chat still denies, retain the preflight/entry markers and failure text; do not
lower native permissions or enable multiplayer fallback. Check unauthorised,
GM-only and monitor-RCON denial, normal authenticated admin chat/RCON status, and
local rejection on listen/dedicated servers in separate authorised test sessions.

Validation rerun: P5 24 categories/95 cases; eight installed event signatures for
each class with 18 rejection fixtures; new source/API/independent permission
models cover 2,048 native-gate/handler route cases, 1,024 Workbench bootstrap
cases and eight role-completion/idempotency cases. Retail preprocessor view
excludes the helper/GivePlayerRole while keeping
both command APIs. P1 58/P2A 81/P2B 179/P3 115 and 16 snapshot-repair categories
pass, as does the full established objective/radio/FIA/USSR/physical-virtual/chat/
GM/response/endgame static/model regression chain. Static/source readiness is NOT
new Workbench compilation, actual role assignment, native permission transport,
network isolation, real save IO or runtime acceptance.

Changed files for THIS permission repair only: TB_CC_CampaignAdminCommand.c,
Tools/Test-CampaignAdministration.py and this document. No changes to persistence
service/schema/reset semantics, XP, gameplay, radio, AI, world/layers or resources.
No native role grants, campaign commands/reset, publish, deploy or commit executed.

**STATIC/API PASS — WORKBENCH PERMISSION RETEST REQUIRED.**
