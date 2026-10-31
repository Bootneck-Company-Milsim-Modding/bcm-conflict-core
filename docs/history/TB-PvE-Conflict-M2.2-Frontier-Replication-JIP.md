# TB PvE Conflict M2.2 — Frontier Replication and JIP

## Verdict

**PASS.** Dedicated authority published a compact, read-only strategic snapshot through the globally replicated `SCR_GameModeCampaign`. Separate proxy clients proved initial synchronization, live forward movement, live reverse movement, advanced-state JIP, retreat-state JIP, and invalid-state synchronization. Clients do not participate in capture authorization.

Primary completed authority run:

`TestProfiles/M2_2_Server_9/logs/logs_2026-09-22_13-27-57/script.log`

## 1. Replicated-state contract

The authoritative frontier controller derives state from the authored route plus current vanilla ownership, then publishes six integer scalars on the existing globally replicated Conflict game-mode entity:

- monotonically increasing `revision`;
- validity: `UNRESOLVED`, `VALID_LINEAR_FRONTIER`, or `INVALID_NON_CONTIGUOUS`;
- frontier index;
- BLUFOR legal objective index;
- OPFOR legal objective index;
- BLUFOR objective-lock bitmask.

Route IDs remain authored deterministic data and are not redundantly replicated. Index `1/2/3` maps to `OBJ_01/OBJ_02/OBJ_03`. A set lock bit means that objective is not the current BLUFOR offensive objective. Invalid state publishes frontier/legal indexes as `-1` and locks all three objectives (`mask=14`).

The game-mode entity was selected because campaign-wide state must not depend on geographic relevance. A standalone world replication node was rejected during testing because its proxy existed but its state remained outside replication relevance.

## 2. Authority model and negative evidence

Only `TB_PVE_M2_1_LinearFrontierComponent` on dedicated authority reads vanilla ownership, derives the frontier, evaluates capture eligibility, and calls the game mode's publish method. `SCR_GameModeCampaign` is a projection container, not a policy authority.

The client policy component logs `authority=0 enforcement=disabled`. The narrow `SCR_SeizingComponent.EvaluateEntityFaction` interception returns vanilla's result immediately when `Replication.IsServer()` is false. Replicated fields have getters but no client mutation/RPC path. A stale or modified proxy snapshot therefore cannot authorize a capture; the server independently evaluates current ownership and policy.

## 3. Revision behaviour

The final lifecycle produced:

| Revision | Validity | Ownership/frontier meaning | US legal | USSR legal | Lock mask |
|---|---|---|---|---|---|
| 1 | Valid | `US/USSR/USSR/USSR`, frontier 0 | OBJ_01 | none | 12 |
| 2 | Valid | `US/US/USSR/USSR`, frontier 1 | OBJ_02 | OBJ_01 | 10 |
| 3 | Valid | `US/US/US/USSR`, frontier 2 | OBJ_03 | OBJ_02 | 6 |
| 4 | Valid | `US/US/USSR/USSR`, frontier 1 | OBJ_02 | OBJ_01 | 10 |
| 5 | Invalid | diagnostic non-contiguous safe state | none | none | 14 |
| 6 | Valid | reconstructed retreat state | OBJ_02 | OBJ_01 | 10 |

Repeated derivation with identical strategic data logs `reconstruction-stable` and does not increment revision. No per-frame updates occur.

## 4. Initial remote synchronization

Client log:

`TestProfiles/M2_2_Client_Live_4/logs/logs_2026-09-22_12-48-32/script.log`

The separate process reported `server=0`, `dedicated=0`, `proxy=1` and received:

```text
revision=1 validity=1 frontier=0 USLegal=1 USSRLegal=-1
lockedMask=12 OBJ_02Locked=1 OBJ_03Locked=1 readOnly=1
```

This verifies the TB projection itself, not merely replicated base ownership.

## 5. Live forward progression

Authority log:

`TestProfiles/M2_2_Server_7/logs/logs_2026-09-22_13-10-41/script.log`

Connected client log:

`TestProfiles/M2_2_Client_Live_7/logs/logs_2026-09-22_13-14-47/script.log`

One uninterrupted proxy received:

```text
13:15:11.392 revision=1 frontier=0 USLegal=1 OBJ_02Locked=1 OBJ_03Locked=1
13:15:17.063 revision=2 frontier=1 USLegal=2 OBJ_02Locked=0 OBJ_03Locked=1
13:15:28.550 revision=3 frontier=2 USLegal=3 OBJ_02Locked=1 OBJ_03Locked=0
```

Server revisions 2 and 3 were published at `13:15:17.021` and `13:15:28.504`. The live proxy therefore received forward changes without reconnecting, approximately 42 ms and 46 ms later.

## 6. Live reverse progression

Authority log:

`TestProfiles/M2_2_Server_9/logs/logs_2026-09-22_13-27-57/script.log`

Connected client log:

`TestProfiles/M2_2_Client_Live_Reverse_9/logs/logs_2026-09-22_13-33-16/script.log`

The proxy first received advanced revision 3:

```text
revision=3 frontier=2 USLegal=3 USSRLegal=2 OBJ_03Locked=0
```

After the same connected session's vanilla USSR recapture of OBJ_02, it received:

```text
revision=4 frontier=1 USLegal=2 USSRLegal=1 OBJ_03Locked=1
```

The server published revision 4 at `13:33:57.542`; the proxy logged it at `13:33:57.583`, approximately 41 ms later.

## 7. Advanced-frontier JIP

Client log:

`TestProfiles/M2_2_Client_Live_5/logs/logs_2026-09-22_12-59-21/script.log`

A fresh process joined after both forward ownership transitions. Its first TB snapshot was current revision 3:

```text
revision=3 validity=1 frontier=2 USLegal=3 USSRLegal=2
OBJ_02Locked=1 OBJ_03Locked=0
```

It did not receive or replay revisions 1–2 and never assumed OBJ_01 was current.

## 8. JIP after retreat

Client log:

`TestProfiles/M2_2_Client_JIP_Advanced/logs/logs_2026-09-22_12-52-19/script.log`

Despite its exploratory profile name, this client completed loading after the reverse transition. Its first snapshot was current revision 4:

```text
revision=4 validity=1 frontier=1 USLegal=2 USSRLegal=1
OBJ_02Locked=0 OBJ_03Locked=1
```

Again, no historical projection events were replayed.

## 9. Invalid-topology replication

The M2.1 synthetic `US/USSR/US/USSR` diagnostic activates the established server-side deny-all override without assigning or repairing vanilla ownership. Authority published revision 5:

```text
validity=2 frontier=-1 USLegal=-1 USSRLegal=-1 lockedMask=14
```

Client evidence:

`TestProfiles/M2_2_Client_JIP_Retreat_5/logs/logs_2026-09-22_13-02-20/script.log`

Its first snapshot was revision 5 with `OBJ_02Locked=1` and `OBJ_03Locked=1`. There is no client repair behavior. When the bounded diagnostic ended, authority reconstructed current real ownership and published valid revision 6.

## 10. Callback and ordering observations

RplProp member application order is not treated as a cross-machine event protocol. The revision callback schedules snapshot consumption on the next call-queue turn, after the replicated batch has been applied. Consumers read one latest revision-tagged snapshot and never depend on vanilla ownership callbacks arriving in the same server/client order. JIP correctness comes from the current mandatory replicated values on the game mode, not event history.

## 11. Reconstruction

At advanced revision 3 and retreat revision 4, the proof explicitly called authoritative reconstruction from route plus current vanilla ownership. Both logged `reconstruction-stable` with identical validity, frontier, legal objectives and lock mask, and neither incremented revision. After invalid diagnostic revision 5, the same reconstruction restored the real retreat snapshot as revision 6. No capture history is required.

## 12. FIA isolation

FIA remains outside the conventional projection. The projection contains only US/USSR frontier indexes and validity. The underlying lifecycle again held 50 live FIA actors at OBJ_01 for 60 seconds without ownership/frontier change; no projection revision was generated.

## 13. Deployment and vanilla-system regression

The completed authority run ended with the original M2.1 `COMPLETE PASS`. It retained one enabled US MOB spawn. Every forward objective remained valid but disabled, hidden and non-reservable (`reserveResult=3002`). Final objectives retained vanilla supplies/income, radio, two construction providers and three services.

All forward and reverse ownership changes came from physical vanilla capture. TB still does not assign ownership, manipulate capture timers, replace radio/supply, or implement custom ownership replication.

## 14. Acceptance criteria A–M

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | One authority-derived source publishes through the replicated game mode. |
| B | PASS | Separate proxy received revision 1 initial state. |
| C | PASS | One proxy received revisions 1 → 2 → 3 live. |
| D | PASS | One proxy received revisions 3 → 4 live. |
| E | PASS | Fresh advanced JIP received revision 3 directly. |
| F | PASS | Validity enum is part of every snapshot. |
| G | PASS | Fresh proxy received invalid revision 5 with all objectives locked. |
| H | PASS | Client policy enforcement disabled; no mutation/RPC path; eligibility requires server authority. |
| I | PASS | Explicit non-initial recalculations reproduced revisions 3 and 4 without change. |
| J | PASS | JIP clients received current revision directly without historical replay. |
| K | PASS | FIA generated no conventional frontier transition or revision. |
| L | PASS | One MOB enabled; all forward deployment points disabled/hidden/rejected. |
| M | PASS | Dedicated physical forward/reverse lifecycle and vanilla supply/radio/services regression passed. |

## 15. Files

Created:

- `Documentation/TB-PvE-Conflict-M2.2-Frontier-Replication-JIP.md`

Modified:

- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c`
- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_VerticalSliceProof.c`

The existing M2.1 prefab, world and mission remain the runtime host; no new campaign gameplay system was added.

## 16. Remaining unknowns

- SteamDiag clients on this host disconnect about 31 seconds after completing load because the local diagnostic Steam identity is not fully authorized. Deterministic proof windows/client detection allowed all required live and JIP observations before disconnect, but long-duration and WAN behavior remain untested.
- Multiple simultaneous remote-client consumption was not tested.
- Full server shutdown/restart persistence remains out of scope. Reconstruction from live vanilla ownership was proven, not persistence storage.
- The projection is intentionally code-facing; no campaign UI consumer exists yet.
- Revision integer wraparound is not operationally relevant to this vertical slice and has not been endurance-tested.

## 17. Recommended M2.3 scope

M2.3 should remain a read-only consumer slice: define a small objective/frontier query service or debug presentation consuming the revisioned snapshot, and verify two simultaneous clients plus stale-revision rejection in the consumer. Do not introduce AI Director, attacks, garrisons, activation leases, economy, persistence, or campaign map UI without a separate milestone decision.
