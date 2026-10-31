# TB PvE Conflict M2.1 — Linear Frontier Vertical Slice

## Verdict

**PASS.** A production-style dedicated-server run completed the reversible route `MOB -> OBJ_01 -> OBJ_02 -> OBJ_03` using real physical AI actors and vanilla Conflict capture/ownership transitions. TB supplies only a server-authoritative eligibility veto and derives its frontier from current vanilla ownership. No TB code assigns ownership or drives a capture timer.

Final proof log: `TestProfiles/M2_1_Server_19/logs/logs_2026-09-22_12-21-33/script.log` (run timestamps 12:22–12:26 local). The run reports `server=1`, `dedicated=1` and ends with:

```text
[TB_PVE_M2_1] COMPLETE PASS sequence=MOB-US,OBJ_01-US,OBJ_02-USSR,OBJ_03-USSR reversible=1 vanillaOwnershipOnly=1
```

## 1. Architecture

`TB_PVE_M2_1_LinearFrontierComponent` owns only route identity, derived frontier state, and the `CanFactionCaptureBase(faction, base)` policy. Vanilla `SCR_SeizingComponent` continues to own actor validation, prevailing-faction evaluation, capture state/timing/interruption, and the ownership transition. `SCR_CampaignMilitaryBaseComponent` remains the source of ownership events and current ownership.

The narrow interception is an override of `SCR_SeizingComponent.EvaluateEntityFaction(IEntity ent)`. It calls `super` first. For a vanilla-valid physical actor at a route base, TB may return `null` when policy denies that faction/base pair; otherwise it returns vanilla's result unchanged. There is no copied `EvaluatePrevailingFaction`, timer mutation, or ownership setter.

## 2. Route representation and objective identity

The route is authored as two parallel prefab attributes:

```text
entities: TB_M2_1_MOB, TB_M2_1_OBJ_01, TB_M2_1_OBJ_02, TB_M2_1_OBJ_03
IDs:      MOB,          OBJ_01,          OBJ_02,          OBJ_03
```

These project-owned stable IDs and ordered entity references are explicit data. Logic neither infers order from geography nor routes by prefab GUID. Reordering/replacing the three references requires data editing, not frontier-code changes. The scenario authors a US MOB plus three forward bases and uses the established no-player-deployment forward-base prefab.

## 3. Frontier derivation and authority

On the server, route resolution and every route ownership callback trigger an idempotent scan of current vanilla ownership. A valid state is a contiguous US prefix followed by a USSR suffix, with the MOB US-owned. The final US member is the frontier index.

- BLUFOR legal objective: the first USSR base immediately after the US prefix.
- OPFOR legal objective: the final US objective immediately before that boundary, excluding the MOB when no objective has yet been taken.
- FIA/other factions are not conventional frontier participants.

No duplicated persisted campaign truth is maintained. Repeating the calculation over identical ownership produces identical eligibility.

## 4. Eligibility contract

For route bases:

- US is allowed only at the derived BLUFOR legal index.
- USSR is allowed only at the derived OPFOR legal index.
- FIA and other factions are denied by the M2.1 strategic policy.
- An invalid/non-contiguous route denies every strategic capture.

Non-route bases are left to vanilla. The policy executes only on the authority; proxy/client callback order is not used as truth.

## 5. Scenario configuration and initial state

The dedicated mission uses a custom base whitelist containing the project MOB, the USSR HQ (`MainBaseSouth`), and the three objectives. Initial runtime ownership was:

```text
MOB=US, OBJ_01=USSR, OBJ_02=USSR, OBJ_03=USSR
frontierIndex=0, bluforLegal=OBJ_01, opforLegal=<none>
```

The sparse proof layout configures `m_fRadioRange=15000` through the ordinary Conflict mission header for its five whitelisted bases. This is test-scenario topology configuration: vanilla radio coverage remains active and was required for capture. Earlier runs with the inherited 1 km range produced 30 live, vanilla-eligible actors at OBJ_02 but `hqReceive=0`, `captureState=1`, and no prevailing faction; no capture occurred. With connected vanilla coverage, `hqReceive=1`, `hqBothWays=1`, and the same capture path completed. No production radio code was bypassed.

## 6. Locked-objective tests

Before OBJ_01 changed owner, 30 ordinary US AI characters were physically materialized at each locked objective, attached to a vanilla group, and held at LOD 0. Each test ran for 60 seconds, longer than the observed eligible capture duration:

```text
PASS locked-test=OBJ_02-initial owner=USSR captureState=0 ticks=30 aliveAtEnd=30
PASS locked-test=OBJ_03-initial owner=USSR captureState=0 ticks=30 aliveAtEnd=27
```

The actors were not teleported away, killed, hidden only in UI, or followed by an ownership correction. Surviving valid actors remained inside the radius and the server denied their faction at the eligibility gate, so vanilla capture never progressed.

## 7. Forward progression

Thirty LOD-0 US actors physically captured OBJ_01 through vanilla. The observed ownership callback was `USSR -> US`, after which the derived frontier advanced from 0 to 1 and OBJ_02 became the US legal objective while OBJ_03 remained locked.

The next physical US force at OBJ_02 was logged `ALLOW`; vanilla changed ownership `USSR -> US`. Recalculation advanced the frontier from 1 to 2:

```text
ownership=MOB=US,OBJ_01=US,OBJ_02=US,OBJ_03=USSR
bluforLegal=OBJ_03 opforLegal=OBJ_02
PASS OBJ_02 captured OBJ_03-unlocked=1 frontier=2
```

## 8. Reverse progression and relock

Thirty LOD-0 USSR actors were then physically present at OBJ_02. Policy allowed the frontier objective, vanilla reported `captureState=2` and `prevailing=USSR`, and vanilla changed ownership `US -> USSR`. The callback-derived frontier retreated from 2 to 1:

```text
ownership=MOB=US,OBJ_01=US,OBJ_02=USSR,OBJ_03=USSR
bluforLegal=OBJ_02 opforLegal=OBJ_01
PASS reverse-capture ... OBJ_03-relocked=1
```

Thus BLUFOR must retake OBJ_02 before OBJ_03 can be attacked again.

## 9. OPFOR anti-back-cap

In the post-retreat state, OPFOR's legal objective was OBJ_01. Thirty live USSR actors were held at the US MOB for 60 seconds. Ownership remained US and capture stayed inactive:

```text
PASS locked-test=OPFOR-MOB-anti-backcap owner=US captureState=0 ticks=30 aliveAtEnd=30
```

The same derived frontier policy therefore constrains both conventional factions.

## 10. FIA interaction

Fifty live vanilla FIA fire-team actors occupied OBJ_01 for 60 seconds. The policy denied FIA, ownership stayed USSR, capture remained inactive, and frontier index stayed 0:

```text
PASS locked-test=FIA-does-not-advance ... aliveAtEnd=50
PASS FIA interaction owner=USSR frontier=0
```

This is only an exclusion proof. M2.1 does not design human FIA access or a FIA campaign.

## 11. Non-contiguous-state policy

The controller detects a US base after the USSR suffix (and any non-US/USSR owner) as invalid. Its deterministic safe policy is `DENY_ALL_STRATEGIC_CAPTURES`; it logs the complete ownership snapshot and does not repair or rewrite ownership. The runtime diagnostic evaluated `US/USSR/US/USSR`, returned invalid, and logged the safe policy.

## 12. MOB-only deployment regression

Initial and final authority checks both found exactly one enabled US MOB spawn. Every forward base retained a valid spawn-point reference but reported:

```text
spawnEnabled=0 visible=0 canReserve=0 reserveResult=3002
```

This remained true after the complete forward/reverse lifecycle. M1.3's manual deploy/respawn UI proof remains the literal player-facing evidence; M2.1 neither changes nor duplicates deployment code.

## 13. Supply, radio, construction and services regression

At both ends of the run every objective retained its vanilla radio component, two building providers, three services, and valid supply storage. Final observations showed nonzero supplies and income (`OBJ_01 income=1`; USSR-held OBJ_02/OBJ_03 income=2). Radio range was the authored 15000 and OBJ_02 reported two-way HQ traffic before its eligible US capture. The successful ownership changes themselves used vanilla capture and ownership callbacks.

Replication remains the vanilla underlying mechanism and was previously proven with an actual remote client in M1.2. This M2.1 run was a dedicated authority lifecycle test; it did not add a second remote client observation.

## 14. Acceptance criteria A–O

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Explicit data route `MOB -> OBJ_01 -> OBJ_02 -> OBJ_03`. |
| B | PASS | Dedicated authority derives one frontier from ordered route plus current vanilla ownership on resolution/callback. |
| C | PASS | Initial snapshot: only OBJ_01 US-eligible; OBJ_02/03 denied. |
| D | PASS | 30 live US actors at OBJ_02 for 60 s; owner USSR, capture state 0. |
| E | PASS | Vanilla OBJ_01 `USSR -> US` callback advanced frontier 0 -> 1. |
| F | PASS | OBJ_02 changed from denied to allowed and then captured. |
| G | PASS | Vanilla OBJ_02 `USSR -> US` advanced frontier to 2; OBJ_03 became US-legal. |
| H | PASS | Vanilla USSR capture of OBJ_02 changed `US -> USSR`; frontier retreated 2 -> 1. |
| I | PASS | Post-retreat `bluforLegal=OBJ_02`; OBJ_03 relocked. |
| J | PASS | 30 live USSR actors at MOB for 60 s; owner US, capture state 0. |
| K | PASS | No TB ownership assignment; progression came only from observed vanilla ownership changes. |
| L | PASS | Vanilla actor evaluation, prevailing faction, timer, ownership, radio, supplies and replication remain underneath; dedicated lifecycle passed, with replication inherited from M1.2 remote-client proof. |
| M | PASS | All forward spawn points stayed disabled, hidden and non-reservable; one US MOB remained enabled. |
| N | PASS | 50 live FIA actors for 60 s did not capture or move the frontier. |
| O | PASS | Runtime synthetic non-contiguous state detected; deterministic deny-all policy logged. |

## 15. Files

Created:

- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c`
- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_VerticalSliceProof.c`
- `Prefabs/Systems/TB_PVE_M2_1_LinearFrontier.et` and `.meta`
- `Worlds/TB_PVE_M2_1_LinearFrontier.ent` and `.meta`
- `Worlds/TB_PVE_M2_1_LinearFrontier_Layers/M2_1_Route.layer`
- `Missions/TB_PVE_M2_1.conf` and `.meta`
- this document

Modified:

- `Scripts/Game/TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c` — suppresses the legacy M1 harness while an M2.1 frontier controller is active.

## 16. Remaining unknowns

- M2.1 did not repeat M1.2's remote-client replication observation. It relies on the unchanged vanilla ownership/replication path plus that existing proof.
- The mission-header radio range is intentionally large for this sparse vertical-slice topology. A production terrain route needs normal per-objective coverage tuning and validation; M2.1 does not define final map spacing.
- The runtime harness uses deliberately oversized physical groups to dominate ambient defenders deterministically. It is proof tooling, not an AI-force-balance or Director design.
- The invalid-state check is a bounded runtime policy diagnostic; no external system forced the authoritative world itself into a non-contiguous state because M2.1 intentionally does not rewrite ownership for testing.
- Persistence/restart reconstruction has not been tested, although frontier state is derivable and recalculation is idempotent.

## 17. Recommended M2.2 scope

Proceed only after review to a narrowly scoped authoritative objective-state projection for campaign consumers: expose stable route/objective IDs, current owners, frontier index, and legal US/USSR objective IDs, with join-in-progress/reconnect verification against vanilla replicated ownership. Keep AI Director, attack selection, activation leases, garrisons, economy and UI out of that slice.
