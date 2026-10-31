# TB PvE Conflict M2.4 — Production-Scale Authored Linear Campaign Route

## Verdict

**M2.4 IMPLEMENTATION COMPLETE**

**M2.4 AUTHORITY ACCEPTANCE PASS**

**M2.4 NETWORK ACCEPTANCE PENDING**

Criterion M: **UNVERIFIED**  
Criterion N: **UNVERIFIED**

Overall formal milestone status: **INCOMPLETE** until M and N receive production-route runtime proof.

The ten-node production route passed initialization, the complete physical forward walk, three consecutive reverse captures, deep-lock and anti-back-cap checks, reconstruction, invalid-state handling, semantic-query checks, endpoint behavior, and vanilla-system regressions. However, no remote client was admitted during the M2.4 run. Acceptance M and N therefore cannot be marked PASS.

Outstanding acceptance item:

> Run prepared M2.4A external retail-client acceptance when a suitable second authenticated client/account is available.

Primary authority evidence:

`TestProfiles/M2_4_Server_9/logs/logs_2026-09-22_17-03-02/script.log`

Radio investigation evidence:

- 2.5 km failure: `TestProfiles/M2_4_Server_5/logs/logs_2026-09-22_16-42-07/script.log`
- 5 km failure: `TestProfiles/M2_4_Server_7/logs/logs_2026-09-22_16-50-23/script.log`

Rejected network attempts:

- `TestProfiles/M2_4_Client_A2`
- `TestProfiles/M2_4_Client_JIP`
- `TestProfiles/M2_4_Client_JIP_RDB`
- `TestProfiles/M2_4A_Client_A` (diagnostic client against retail server; binary-class mismatch)
- `TestProfiles/M2_4A_Client_A2` (matched diagnostic client/server; backend identity rejection)
- `TestProfiles/M2_4A_Client_Retail_A` (matched retail client/server; backend identity rejection)
- `TestProfiles/M2_4A_Runner_2026-09-29_08-58-51` (final recovered-topology authentication experiment; terminal local result)

Final Workbench script validation completed successfully.

## 1. Route configuration architecture

The route component now authors:

- stable route ID;
- ordered entity names;
- parallel stable objective IDs;
- required MOB ID at index zero;
- required terminal ID at the final index;
- proof selection.

`EVERON_NORTH_SOUTH_01` is data in `TB_PVE_M2_4_ProductionRoute.et`. Reordering nodes changes prefab data and the mission whitelist, not frontier logic. The controller does not contain coordinates, entity GUID routing, objective names, or a fixed route length.

The world uses the proven project Conflict base prefab at selected real Everon Conflict-base locations. This retains vanilla base, capture, supply, radio, construction and service functionality while applying the established AI-capture configuration and no-player-deployment forward-base configuration. The route is deliberately authored; it is not sorted by distance at runtime.

## 2. Initialization validation and fail-closed behavior

Initialization validates:

- non-empty route identity;
- parallel entity/ID arrays;
- 3–31 nodes;
- non-empty and unique objective IDs;
- non-empty and unique entity references;
- every entity resolves to an initialized campaign base;
- every node has a seizing component and spawn point;
- index zero matches the authored MOB ID and is an HQ;
- the final index matches the authored terminal ID;
- forward nodes are non-HQ and have the marker plus disabled spawn policy;
- route size fits the replicated mask.

A failed route never becomes resolved. While an authored route component exists but is unresolved, the capture eligibility bridge denies strategic capture rather than silently permitting it.

The first layout experiment placed custom endpoints directly over vanilla MainBase entities. Conflict HQ initialization overwrote their factions and the controller correctly published an invalid topology. The endpoints were separated from the vanilla HQ entities rather than adding ownership-repair code.

## 3. Representation limit

The existing replicated lock representation is a signed integer bitmask. M2.4 makes its operational ceiling explicit: at most 31 total nodes, using indices 0–30 and avoiding the sign bit. The selected route uses ten nodes and bits 1–9 (`lockedMask` maximum 1020), so it fits safely.

The public M2.3 query remains representation-independent. Its objective-ID table is now configured from authored route data on both server and client rather than hard-coded to three objectives.

## 4. Selected route and topology

| Index | Stable ID | Topology location | Position | Spacing from prior |
|---:|---|---|---|---:|
| 0 | `MOB_NORTH` | North MOB / St Philippe corridor | 4400, 11200 | — |
| 1 | `OBJ_01_MAIDENS_BAY` | Maidens Bay | 5229, 10702 | 0.97 km |
| 2 | `OBJ_02_KERMOVAN` | Kermovan | 6437, 9667 | 1.59 km |
| 3 | `OBJ_03_ANDRES_BEACON` | Andre's Beacon | 6853, 8200 | 1.53 km |
| 4 | `OBJ_04_CHOTAIN_COASTAL` | Coastal Chotain | 7473, 6698 | 1.63 km |
| 5 | `OBJ_05_LARUNS` | Laruns | 7403, 5281 | 1.42 km |
| 6 | `OBJ_06_QUARRY` | Quarry | 8793, 3897 | 1.96 km |
| 7 | `OBJ_07_DURRAS` | Durras | 8896, 2709 | 1.20 km |
| 8 | `OBJ_08_VERNON` | Vernon | 9128, 2292 | 0.48 km |
| 9 | `OPFOR_ENDPOINT_SOUTH` | St Pierre / south endpoint corridor | 9950, 1750 | 0.99 km |

There are eight ordinary objectives between the two explicit endpoints. Terrain and the central highlands, rather than Euclidean spacing alone, are the main radio concern.

## 5. Initial frontier

The final run resolved:

```text
MOB_NORTH=US
all nine forward nodes=USSR
frontier=0
BLUFOR legal=OBJ_01_MAIDENS_BAY
OPFOR legal=none
validity=VALID_LINEAR_FRONTIER
```

Revision 1 had lock mask 1020: only the first objective was BLUFOR-legal and every deeper node was locked.

## 6. Full forward walk

All nine forward nodes changed from USSR to US through physical LOD-0 US actors and vanilla capture/ownership callbacks. Revisions advanced monotonically from 1 to 10. After every transition the public query reported the next authored ID as BLUFOR-legal, the captured node as the OPFOR-facing friendly frontline, and deeper nodes as locked.

The sequence was:

`Maidens Bay → Kermovan → Andre's Beacon → Coastal Chotain → Laruns → Quarry → Durras → Vernon → OPFOR endpoint`.

No ownership setter or capture timer manipulation was introduced.

## 7. Deep-lock and FIA tests

At the initial frontier, 30 LOD-0 US actors remained at OBJ_03 and separately at OBJ_06 for 20 seconds. Both retained USSR ownership, inactive capture state and stable revision 1.

At frontier 4, OBJ_05 was legal while 30 US actors attempted OBJ_06. OBJ_06 remained USSR-owned with stable revision 5 and inactive capture state.

Fifty LOD-0 FIA actors at the initial legal objective produced no ownership change or revision. The public contract returned false FIA authorization for every production-route objective.

## 8. Reverse progression and anti-back-cap

After revision 10 reached the endpoint, physical USSR forces captured three consecutive legal frontier nodes:

- endpoint: frontier 9 → 8, revision 11;
- Vernon: frontier 8 → 7, revision 12;
- Durras: frontier 7 → 6, revision 13.

Each step changed ownership through vanilla capture, advanced the revision, updated both legal objectives, and relocked the deeper BLUFOR objective.

Afterward, USSR's legal objective was Quarry. Thirty USSR actors instead attempted deeper US rear node Laruns for 20 seconds. Ownership, capture state and revision remained unchanged.

## 9. Mid-route reconstruction

At depths 2, 4 and 7 the proof explicitly recalculated from ordered route plus current vanilla ownership. Each returned the same frontier and legal objectives and logged `revisionStable=1`. No capture history or persistence state was consulted.

## 10. Invalid production state

A ten-node synthetic ownership pattern containing a USSR gap followed by a deeper US node was detected as non-contiguous. The bounded invalid projection reported:

- invalid campaign validity;
- no BLUFOR objective;
- no OPFOR objective;
- false strategic attack authorization;
- `INVALID` objective state.

Restoring the diagnostic override reconstructed the real frontier-6 ownership as valid revision 15.

## 11. Public semantic contract

The M2.3 route-ID table is no longer fixed to `OBJ_01/02/03`. It is configured from the authored route before snapshot consumption. The diagnostic consumer enumerated all nine forward IDs and reported semantic state, lock state and FIA authorization using only public methods.

At endpoint revision 10, BLUFOR had no next legal objective and the endpoint was the OPFOR-facing friendly frontline. After reversal, the same contract returned the appropriate legal authored IDs and rear/frontline/locked states.

## 12. Replication and JIP

**UNVERIFIED for M2.4.** Several separate `ArmaReforgerSteamDiag` processes attempted to connect. The dedicated server repeatedly authenticated identity `0x00000000` and immediately disconnected it with reason 3 while logging `Trying to register connection of possibly unauthorized client`. Clients never reached world/script initialization.

A focused post-lifecycle JIP retry also failed. Disabling only diagnostic resource-database validation did not change admission. Server and client script protocol checksums matched. This points to the local diagnostic Steam/backend identity rather than a demonstrated projection failure, but it is not runtime evidence of success.

M2.3's simultaneous two-client proof remains valid for the unchanged replication architecture, but it is not substituted for production-route M2.4 evidence.

## 12A. M2.4A — Network Acceptance

### Scope and configuration comparison

M2.4A did not change route representation, frontier derivation, capture eligibility, revision rules, public-query semantics, ownership, capture, radio, supply, deployment, or FIA behavior. It used fresh profiles and the same mission/addon arguments as the earlier tests. Backend authentication and replication validation remained enabled.

The known-good M2.3 launch and the original failing M2.4 diagnostic launch used the same substantive command-line shape: dedicated `-server` or remote `-client 127.0.0.1`, the stock-data plus local-workspace addon roots, addon `6A6A5973FA53A5B1`, a fresh profile, and debug logging. Mission player count is eight in both `TB_PVE_M2_1.conf` and `TB_PVE_M2_4.conf`; neither mission declares a password. Native protocol checksums matched, and each matched M2.4A pair used the same script checksum.

An initially hidden difference was positively identified in the first clean retry:

```text
RplConnection::ValidationError isDevBinary value does not match! local=0 remote=1
```

That retry paired retail `ArmaReforgerServer.exe` with development `ArmaReforgerSteamDiag.exe`, producing disconnect group 4/reason 6. This is a real binary-class incompatibility, but it does not explain the original matched-diagnostic reason-3 failures.

Two legitimate matched configurations were then tested:

- `ArmaReforgerServerDiag.exe` with `ArmaReforgerSteamDiag.exe`;
- `ArmaReforgerServer.exe` with `ArmaReforgerSteam.exe`.

Both server runs entered online game state and published the valid production-route projection. Both clients initialized Steamworks, loaded saved session data, reported the signed-in Steam user, and matched the server's native/script protocol. Nevertheless, each connection remained identity `0x00000000`, the server logged `Trying to register connection of possibly unauthorized client`, and the server rejected it almost immediately with group 4/reason 3. No client reached world/script initialization and no Steam web-API ticket response appeared. The earlier successful M2.3 clients, by contrast, remained pending for approximately eighteen seconds, produced Steam ticket responses, and were then admitted. No mission-header, player-limit, password, addon-list, checksum, bind-port, or frontier-code difference was found that accounts for that backend authorization behavior.

### Evidence runs

| Purpose | Server profile | Client profile | Result |
|---|---|---|---|
| Restricted-sandbox diagnostic | `M2_4A_Server_3` | — | Backend unreachable; invalid as a network acceptance run. |
| Binary-class diagnosis | `M2_4A_Server_4` | `M2_4A_Client_A` | Rejected reason 6; explicit retail/dev `isDevBinary` mismatch. |
| Matched diagnostic | `M2_4A_Server_Diag_1` | `M2_4A_Client_A2` | Server online; client signed in; rejected reason 3 at identity 0 before world initialization. |
| Matched retail | `M2_4A_Server_Retail_1` | `M2_4A_Client_Retail_A` | Server online; client signed in; rejected reason 3 at identity 0 before world initialization. |

The matched runs independently reconfirmed that the authority could initialize revision 1 and advance legitimate physical captures. Those captures are not claimed as a new authority-lifecycle proof and do not replace the already accepted M2.4 run.

### Required network observations

- Successful client admission: **PENDING / not obtained** because local authentication was blocked before world entry.
- Initial remote state (`READY`, `VALID`, revision/frontier/legal objectives/semantic states): **UNVERIFIED** because no proxy reached world initialization.
- Live transition on an already-connected client: **UNVERIFIED**.
- Mid-route authoritative state: the server reached valid production-route revisions during the bounded attempts, but no state is claimed for criterion N without an admitted JIP client.
- Fresh mid-route JIP projection: **UNVERIFIED**.
- Existing/JIP convergence: **NOT TESTED**, because neither client role could be admitted.

### M2.4A verdict

Criterion M remains **UNVERIFIED** and criterion N remains **UNVERIFIED**.

```text
M2.4 IMPLEMENTATION COMPLETE
M2.4 AUTHORITY ACCEPTANCE PASS
M2.4 NETWORK ACCEPTANCE PENDING
Overall formal milestone status: INCOMPLETE
```

The evidence supports a test-host Steam/backend authorization problem before world initialization, not an M2 frontier replication failure. It does not yet establish the lower-level cause of the missing/late authenticated identity. Authentication was not disabled and no production security setting was weakened.

### Terminal local-authentication finding

The final controlled `-AuthTopologyTest` reproduced the recovered successful M2.3 launch topology as closely as practical while retaining the M2.4 mission/addon:

- matched ServerDiag plus two independent SteamDiag clients;
- fresh separate profiles;
- workspace-root launch context;
- Client B launched first at server age 257.709 seconds (target 257.705);
- Client A launched 796.168 milliseconds after B without waiting for B authentication;
- both clients initialized Steam, signed in and reached transport;
- the server held simultaneous provisional identities zero and one during overlapping reconnects.

Both clients were nevertheless rejected with group 4/reason 3 approximately 0.16–0.20 seconds after the possibly-unauthorized warning. Neither received a logged Steam ticket response, created a player or initialized a world. The server logged no `Players connecting` or `Players connected` progression.

```text
AUTH TOPOLOGY RESULT: AUTHENTICATION_BLOCKED
```

This is the **terminal local-authentication finding**. The recovered M2.3 timing/topology differences were reproduced without restoring admission. Do not perform further local-authentication experiments unless new external evidence materially changes the situation. Do not modify gameplay or networking implementation in response to this pre-world test-environment failure.

Preserve the normal M2.4A acceptance runner, its `-AuthTopologyTest` diagnostic/reference mode, all successful M2.3 evidence, all M2.4 authority evidence and the final authentication logs.

## 13. Radio topology

Radio remained a separate vanilla prerequisite from TB strategic eligibility.

- At 2.5 km, Maidens Bay captured, but Kermovan reported `receive=0 bothWays=0` despite 1.59 km spacing.
- At 5 km, progression reached Andre's Beacon, but Coastal Chotain reported no HQ traffic after a six-second relay settle.
- At 15 km, every forward node and all three reverse targets reported `receive=1 bothWays=1`; the complete lifecycle succeeded.

The production corridor therefore needs the currently authored 15 km scenario range under vanilla terrain-aware radio behavior. No TB radio bypass or runtime range mutation was added. A future map/topology pass could add relay bases and retest a shorter range.

## 14. Supply, deployment and services

Representative final checks at Maidens Bay, Laruns and the OPFOR endpoint reported:

- valid, nonzero supplies and vanilla income;
- valid spawn points, disabled and invisible;
- two construction providers;
- three services.

The selected US north MOB was logged by vanilla `MOBsSelected`, while all forward nodes used the established disabled/hidden/non-reservable policy. MOB-only deployment remains intact.

## 15. AI LOD observations

Every proof force logged group LOD 0 with engine maximum LOD 10. No natural maximum-LOD transition occurred during these bounded captures. The M1.4 rule remains unchanged: maximum-LOD non-player AI are not valid vanilla seizing actors. No activation lease or population system was implemented.

## 16. Endpoint behavior

The route explicitly identifies `MOB_NORTH` and `OPFOR_ENDPOINT_SOUTH`. Capturing the terminal logged:

`FRONTIER_AT_OPFOR_ENDPOINT ... gameContinues=1`

No victory, defeat, restart or mission termination was implemented. Reverse capture remained possible immediately afterward.

## 17. Acceptance A–R

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Ten-node authored production-style route exists. |
| B | PASS | Order and IDs are prefab data; frontier logic is count-agnostic. |
| C | PASS | References, uniqueness, endpoints, components, deployment policy and size validate; unresolved route denies. |
| D | PASS | Ten nodes fit the explicit 31-node signed-mask ceiling. |
| E | PASS | Revision 1 was valid with only Maidens Bay BLUFOR-legal. |
| F | PASS | Nine consecutive legitimate vanilla forward captures reached the endpoint. |
| G | PASS | Initial OBJ_03/06 and mid-route OBJ_06 attempts were denied. |
| H | PASS | Three consecutive legitimate USSR reverse captures passed. |
| I | PASS | USSR attempt against deeper US rear Laruns was denied. |
| J | PASS | Depths 2, 4 and 7 reconstructed without revision change or event history. |
| K | PASS | Ten-node non-contiguous diagnostic failed closed. |
| L | PASS | Dynamic authored IDs and semantic states worked across revisions 1–15. |
| M | **UNVERIFIED** | Diagnostic proxy was not admitted during M2.4. |
| N | **UNVERIFIED** | Fresh production-route JIP client was not admitted. |
| O | PASS | US MOB selected; all representative forward spawns disabled/hidden. |
| P | PASS | Vanilla physical capture/ownership, radio, supply, construction and services remained underneath. |
| Q | PASS | Physical FIA presence produced no capture/revision; public authorization false. |
| R | PASS | Both endpoints explicit; endpoint signal emitted without victory/defeat. |

## 18. Files

Created:

- `Scripts/Game/TB_PVE_M2_4/TB_PVE_M2_4_ProductionRouteProof.c`
- `Prefabs/Systems/TB_PVE_M2_4_ProductionRoute.et` and metadata
- `Worlds/TB_PVE_M2_4_ProductionRoute.ent` and metadata
- `Worlds/TB_PVE_M2_4_ProductionRoute_Layers/M2_4_Route.layer`
- `Missions/TB_PVE_M2_4.conf` and metadata
- this document

Modified:

- `Scripts/Game/TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c`
- `Scripts/Game/TB_PVE_M2_3/TB_PVE_M2_3_FrontierQuery.c`

No vanilla resource was edited. No commit was created.

## 19. Remaining unknowns

- Production-route proxy projection and JIP remain unverified because no authenticated production-route client has entered the world.
- The reason the same host admitted the earlier M2.3 clients after an approximately eighteen-second authentication delay remains unknown. Local investigation is closed after the terminal controlled `AUTHENTICATION_BLOCKED` result.
- Whether 15 km should remain the production radio range or be replaced by additional authored relay topology needs a design decision and map playtest.
- Real-player travel pacing, force balance and objective suitability were not evaluated.
- Long-duration AI LOD behavior across the full geographic route remains for the future activation/population milestone.
- The 31-node mask ceiling is explicit but has not been tested near its upper bound.

## 20. Recommended next action

Outstanding acceptance item: **Run prepared M2.4A external retail-client acceptance when a suitable second authenticated client/account is available.**

That session should repeat only the pending network slice: connect Client A at revision 1, observe one live transition, advance to approximately frontier 4, and connect fresh Client B for direct JIP convergence. Leave backend authentication enabled. If both succeed, update M/N and the overall verdict to PASS without rerunning or redesigning the already-passed authority lifecycle.

No further authentication investigation is authorized as part of the current development path. M2.5 is not started. Its scope proposal is recorded separately in `Documentation/TB-PvE-Conflict-M2.5-Scope-Proposal.md`.
