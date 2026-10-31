# TB PvE Conflict M2.3 — Frontier Query Contract and Multi-Client Robustness

## Verdict

**PASS. MULTI-CLIENT VERIFIED.** A stable read-only semantic query now hides the M2.1 controller, route indexes, replicated scalar fields and lock mask from future consumers. The same contract consumes authoritative snapshots on the server and the replicated projection on clients. One dedicated run admitted two simultaneous proxy clients; an existing client and a later JIP client reached the same revision 4 semantic state through different histories.

Primary logs:

- Authority: `TestProfiles/M2_3_Server_2/logs/logs_2026-09-22_14-01-34/script.log`
- Client A: `TestProfiles/M2_3_Client_A/logs/logs_2026-09-22_14-05-53/script.log`
- Client B: `TestProfiles/M2_3_Client_B/logs/logs_2026-09-22_14-05-52/script.log`

Workbench Script Editor validation also completed successfully after the runtime proof.

## 1. Public query contract

`TB_PVE_M2_3_FrontierQuery` exposes only read operations and a change signal:

- `GetReadiness()`
- `GetCampaignValidity()`
- `GetRevision()`
- `GetBLUFORLegalObjective()`
- `GetOPFORLegalObjective()`
- `GetObjectiveStrategicState(objectiveId)`
- `IsObjectiveStrategicallyLocked(objectiveId)`
- `CanFactionStrategicallyAttack(factionKey, objectiveId)`
- `GetOnStrategicStateChanged()`

Objective lookup uses stable project IDs (`OBJ_01`, `OBJ_02`, `OBJ_03`). Consumers do not receive setters, route arrays, raw replicated fields, frontier indexes or lock masks. `TB_M2_3_ConsumeProjection` is the replication/authority adapter entry point and is not the supported consumer surface.

`CanFactionStrategicallyAttack` is informational on a client. It never authorizes capture. The existing server-only `SCR_SeizingComponent` interception and authoritative M2.1 policy remain the enforcement path.

## 2. Strategic-state model

The minimal semantic model is:

| State | Meaning |
|---|---|
| `NOT_READY` | No complete strategic snapshot has been consumed. |
| `INVALID` | The campaign topology is invalid or the objective ID is not a route objective. |
| `FRIENDLY_REAR` | BLUFOR-owned objective behind the current BLUFOR frontline. |
| `FRIENDLY_FRONTLINE` | BLUFOR-held objective that OPFOR may legally attack. |
| `ENEMY_FRONTLINE` | OPFOR-held objective that BLUFOR may legally attack. |
| `ENEMY_REAR_LOCKED` | OPFOR-held objective beyond the legal BLUFOR objective. |

At retreat revision 4 the diagnostic consumer reported `OBJ_01=FRIENDLY_FRONTLINE`, `OBJ_02=ENEMY_FRONTLINE`, and `OBJ_03=ENEMY_REAR_LOCKED` (numeric values `3/4/5`).

## 3. Server and client sources

On authority, `SCR_GameModeCampaign.TB_M2_2_Publish` feeds each complete snapshot into the semantic query immediately after the M2.1 controller derives it from authored route plus vanilla ownership.

On a proxy, the existing revision callback defers until the replicated property batch is available, then feeds that complete replicated snapshot into the same semantic query. The client does not inspect ownership or reconstruct policy locally. The latest accepted complete revision is truth; callback ordering need not match the server.

## 4. NOT_READY behavior

Server and both client processes logged the consumer starting with `readiness=0 revision=0` before their first snapshot. During this state:

- readiness is explicitly `NOT_READY`;
- revision is `0`;
- legal-objective IDs are empty;
- objective state is `NOT_READY`;
- strategic attack queries return false.

This prevents an unsynchronized client from mistaking default integers for `OBJ_01` authorization.

## 5. Revision and change notification

Consumers subscribe to `GetOnStrategicStateChanged()`. Exactly one invocation occurs after each newer complete snapshot is accepted. The callback has no ordering-dependent payload: consumers query the latest cached semantic state and revision. Identical reconstruction at revisions 3, 4 and 6 remained stable and generated no new projection revision.

The authority lifecycle was:

| Revision | Validity/frontier | BLUFOR legal | OPFOR legal | Semantic objective states |
|---|---|---|---|---|
| 1 | valid, frontier 0 | OBJ_01 | none | `4/5/5` |
| 2 | valid, frontier 1 | OBJ_02 | OBJ_01 | `3/4/5` |
| 3 | valid, frontier 2 | OBJ_03 | OBJ_02 | `2/3/4` |
| 4 | valid, frontier 1 | OBJ_02 | OBJ_01 | `3/4/5` |
| 5 | invalid | none | none | `1/1/1` |
| 6 | valid reconstruction, frontier 1 | OBJ_02 | OBJ_01 | `3/4/5` |

The numeric semantic values correspond to the state table above.

## 6. Stale and duplicate revisions

After revision 4, the bounded diagnostic presented revision 3. The query logged `stale-revision-rejected current=4 incoming=3`; revision 4 remained current.

It then presented an identical revision 4. The query logged `duplicate-revision-ignored ... notification=0`, proving no false meaningful-change callback.

Finally it presented contradictory data tagged as revision 4. The query logged `ERROR inconsistent-duplicate-revision ... rejected=1`. The diagnostic error is the expected detection signal, not a runtime failure; the contradictory snapshot did not replace state or notify consumers.

## 7. Simultaneous multi-client result

**VERIFIED.** Two separate `ArmaReforgerSteamDiag` processes ran concurrently. Dedicated-server replication recorded two connections and `Players connected: 2 / 2`, then the proof detected `remote-client-detected count=2` before starting the reverse capture.

Observed histories:

| Process | First semantic revision | Later semantic revisions | Role in proof |
|---|---:|---|---|
| Client A | 2 | 3, 4 | Existing client; observed live forward and reverse changes. |
| Client B | 3 | 4 | JIP client; initialized directly at the current advanced state, then observed the same reverse change. |

At revision 3 both reported BLUFOR `OBJ_03`, OPFOR `OBJ_02`, states `2/3/4`. At revision 4 both reported BLUFOR `OBJ_02`, OPFOR `OBJ_01`, states `3/4/5`. Client A received revision 3 at `14:06:15.531`, Client B initialized at revision 3 at `14:06:16.633`, and both received revision 4 at `14:06:36.050/055`.

This is actual simultaneous proxy-client evidence, not sequential-client inference. Client A began at revision 2 while Client B's first snapshot was revision 3, proving convergence despite different histories.

## 8. Invalid state and FIA

The established non-contiguous `US/USSR/US/USSR` diagnostic published revision 5. Through the public contract the consumer reported:

- validity `INVALID_NON_CONTIGUOUS` (`2`);
- no BLUFOR legal objective;
- no OPFOR legal objective;
- all route objective states `INVALID` (`1`);
- FIA attack authorization false.

Attack queries also return false for every faction while invalid. For valid snapshots only `US` and `USSR` can match their respective legal objectives; FIA and unknown factions always return false. No FIA behavior was modified.

## 9. Diagnostic consumer proof

`TB_PVE_M2_3_DiagnosticConsumer` subscribes to the public change signal and calls only public semantic getters. It does not access the game mode's raw projection, route arrays, frontier indexes, lock masks or controller state. Its matching server/client output proves that future UI, task and AI consumers can remain decoupled from representation and replication details. No UI or AI system was implemented.

## 10. Capture and vanilla-system regression

The dedicated proof again completed the full M2.1/M2.2 lifecycle:

- OBJ_02 and OBJ_03 stayed locked initially despite live attackers;
- FIA did not advance the conventional frontier;
- US physically captured OBJ_01 and OBJ_02 through vanilla capture;
- USSR physically recaptured OBJ_02;
- OPFOR MOB anti-back-cap held;
- the invalid state denied strategic progression and reconstructed cleanly;
- final ownership was `MOB=US, OBJ_01=US, OBJ_02=USSR, OBJ_03=USSR`;
- every forward spawn remained valid but disabled, hidden and non-reservable (`reserveResult=3002`);
- exactly one US MOB spawn remained enabled;
- objective supply/income, radio, two construction providers and three services remained present.

The final authority markers were M2.1 `COMPLETE PASS ... vanillaOwnershipOnly=1` and M2.2 `COMPLETE projection-lifecycle initial-forward-forward-reverse-invalid-reconstruct=1`. TB still vetoes ineligible actors only; it does not assign ownership or replace vanilla capture, radio, supply or construction.

## 11. Acceptance criteria A–N

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Narrow read-only semantic query and change signal exist. |
| B | PASS | Diagnostic consumer needs no raw replication, route, frontier or lock-mask internals. |
| C | PASS | Authority snapshots feed the same query directly from the server-derived state. |
| D | PASS | Two proxies produced matching semantic answers from replicated snapshots. |
| E | PASS | Server and clients logged explicit `NOT_READY`, revision 0 before synchronization. |
| F | PASS | Newer complete revisions invoke one semantic change notification. |
| G | PASS | Injected revision 3 could not replace current revision 4. |
| H | PASS | Identical revision 4 was ignored without notification; contradictory revision 4 was detected and rejected. |
| I | PASS | Invalid revision 5 returned no objectives, `INVALID` objective states and false authorization. |
| J | PASS | FIA attack query remained false in valid and invalid states. |
| K | PASS | Tiny consumer uses only the public contract and ran on authority plus both proxies. |
| L | PASS | Locked-objective, FIA and anti-back-cap eligibility tests passed. |
| M | PASS | Physical forward progression and reverse recapture passed; revisions 1–4 matched ownership. |
| N | PASS | Vanilla ownership/capture, supply, radio, construction and MOB-only deployment regressions passed. |

## 12. Files

Created:

- `Scripts/Game/TB_PVE_M2_3/TB_PVE_M2_3_FrontierQuery.c`
- `Documentation/TB-PvE-Conflict-M2.3-Frontier-Query-Multiclient.md`

Modified:

- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c`
- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_VerticalSliceProof.c`

No vanilla resource was edited and no commit was created.

## 13. Remaining unknowns

- The two local clients share the diagnostic Steam account name/zeroed development identity. The server nevertheless assigned separate player IDs and maintained two simultaneous replication connections for the tested window; distinct real Steam accounts and WAN latency remain untested.
- Diagnostic clients on this host remain short-lived, so long-duration multi-client soak, reconnect churn and packet-loss behavior are not proven.
- Revision wraparound and corrupted transport payloads are not endurance/fuzz tested. The targeted stale, duplicate and contradictory same-revision cases are proven.
- Restart persistence, campaign UI and non-linear/multiple-front semantics remain outside this milestone.

## 14. Recommended M2.4 scope

Keep M2.4 narrow and consumer-led: select one bounded read-only system that reacts to the semantic revision/change contract, with authority clearly separated from client presentation. Do not broaden it into AI Director, activation leases, persistence, economy, garrisons or branching fronts without a separate milestone definition.
