# TB PvE Conflict — M1.3 MOB-Only Player Deployment Proof

## Result

**M1.3 is PASS.**

The experimental forward base retains its vanilla `SCR_CampaignSpawnPointGroup` and the military base retains a non-null `m_SpawnPoint`. The spawn point is authored with `m_bSpawnPointEnabled = false`. Vanilla authority rejects reservations with `SCR_ESpawnResult.NOT_ALLOWED_SPAWNPOINT_DISABLED` (numeric result `3002` in the runtime log), and a marker-scoped UI override hides only this experimental base from the deploy list. A vanilla US MOB remained enabled.

The evidence is split across complementary tests. Dedicated-server authority plus a separate remote proxy verified the structural deployment contract, non-playable USSR AI capture, ownership replication, and US -> USSR -> US regression. A later manual local-authority session (`server=1`, `dedicated=0`, `proxy=0`) verified the literal player flow: joining US/BLUFOR, seeing only the MOB in deployment, deploying there, dying, opening the vanilla `RespawnSuperMenu`, and respawning at the MOB while the forward base remained unavailable. Screenshots were captured during that manual session. Together these tests satisfy M1.3 without claiming that the literal UI interaction occurred on the remote proxy run.

## 1. Vanilla deployment architecture

Classification below uses the current vanilla script source installed for Arma Reforger 1.8.0.13.

- **VERIFIED:** `SCR_SpawnPoint.EOnInit()` inserts every spawn point into the static `m_aSpawnPoints` collection and invokes `Event_SpawnPointAdded`. Registration is independent of `m_bSpawnPointEnabled`.
- **VERIFIED:** `SCR_SpawnPointRequestUIComponent.ShowAvailableSpawnPoints()` obtains faction spawn points with `SCR_SpawnPoint.GetSpawnPointsForFaction()` and filters them through `IsSpawnPointVisibleForPlayer()` before adding them to the selector.
- **VERIFIED:** vanilla `SCR_CampaignSpawnPointGroup` changes only the display name; its inherited `IsSpawnPointVisibleForPlayer()` returns true. Therefore `m_bSpawnPointEnabled = false` alone rejects spawning but does not hide the row from the stock selector.
- **VERIFIED:** `SCR_SpawnPoint.CanReserveFor_S()` returns false and `NOT_ALLOWED_SPAWNPOINT_DISABLED` when disabled. `PrepareSpawnedEntity_S()` also returns false, and `CanFinalizeSpawn_S()` returns the enabled state. The authoritative request cannot complete even if a stale or custom UI submits it.
- **VERIFIED:** `m_bSpawnPointEnabled` is replicated using `RplProp(onRplName: "OnSetEnabled")`. `SetSpawnPointEnabled_S()` bumps replication.
- **VERIFIED:** faction accessibility is separate. `SCR_CampaignMilitaryBaseComponent.HandleSpawnPointFaction()` assigns or clears the spawn point faction based on owner, HQ radio traffic, capture state, and respawn cooldown. It does not enable the point.
- **VERIFIED:** `SCR_GameModeCampaign.CanPlayerSpawn_S()` adds campaign-specific checks after the generic spawn path, including base-under-attack and loadout/service constraints.

## 2. Spawn-point registration path

1. The child `SCR_CampaignSpawnPointGroup` entity initializes.
2. `SCR_SpawnPoint.EOnInit()` initializes faction affiliation, inserts the entity in the global spawn-point array, and raises `Event_SpawnPointAdded`.
3. The deploy UI subscribes to spawn-point add/remove/faction events.
4. The deploy UI builds the faction list and calls `IsSpawnPointVisibleForPlayer()`.
5. Selection produces spawn-point spawn data.
6. Authority calls the generic reservation/preparation/finalization gates and then campaign-specific player-spawn validation.

Registration, visibility, enabled state, and faction accessibility are separate axes. The M1.3 implementation deliberately leaves registration intact, sets enabled false, and hides only marked experimental points.

## 3. Military-base/spawn-point relationship

**VERIFIED:** `SCR_CampaignMilitaryBaseComponent.OnInitialized()` scans child entities for the first `SCR_SpawnPoint`, stores it in `m_SpawnPoint`, sets its spawn range, and invokes `m_OnSpawnPointAssigned`. It does not require the point to be enabled.

**VERIFIED at runtime:** the M1.3 forward base repeatedly logged `spawnPointValid=1` while `spawnEnabled=0`, before capture, under USSR ownership, and after returning to US in the primary multiplayer run.

## 4. `m_SpawnPoint` and supply regeneration

**VERIFIED:** `SupplyIncomeTimer()` requires a non-null owner and non-null `m_SpawnPoint`, plus the usual base-type/radio/capture/cooldown conditions. It does not call `IsSpawnPointEnabled()` and does not require the spawn point to be absent from or present in the deploy UI.

**VERIFIED at runtime:** the point remained valid and supplies remained allocated. Primary run observations were 236/1000 initially, 237/1000 after USSR capture, and 237/1000 after return to US. Supply income was 1 initially/finally and temporarily 0 during the capture/cooldown state. The final assertion run observed 200/1000 and later income 1 after USSR capture. These are observations only; M1.4 remains the full supply test.

## 5. Candidate suppression mechanisms

| Candidate | Result |
| --- | --- |
| Delete the child spawn point | Rejected. It nulls the base reference and directly threatens the verified supply prerequisite. |
| Clear the spawn point faction | Rejected as the structural rule. Vanilla ownership/radio handling rewrites faction assignment, so it is state-dependent. |
| Disable the whole base or globally disable Conflict spawning | Rejected. It violates MOB and forward-base functionality requirements. |
| Custom spawn manager or forced player relocation | Rejected. Unnecessary and outside scope. |
| Keep the point, set `m_bSpawnPointEnabled = false` | Chosen authoritative gate. It preserves registration/reference but blocks reservation, preparation, and finalization. |
| Add a marker-scoped visibility override | Chosen UI complement. The stock UI otherwise lists disabled campaign points because inherited visibility always returns true. |

## 6. Chosen mechanism

`TB_PVE_M1_3_NoPlayerDeploymentBase.et` inherits the known-good M1.2 production base. It:

- retains the inherited child `SCR_CampaignSpawnPointGroup`;
- overrides only that child's `m_bSpawnPointEnabled` to `0`;
- adds `TB_PVE_M1_3_NoPlayerDeploymentMarker`.

A `modded SCR_CampaignSpawnPointGroup` override preserves vanilla visibility first, then hides the point only when its parent carries the M1.3 marker. The server-side vanilla enabled gate is still authoritative; the UI filter is not security or spawn authority.

The production `TB_PVE_ConflictMilitaryBase.et` was not changed.

## 7. Exact changes

Created:

- `Prefabs/Systems/MilitaryBase/TB_PVE_M1_3_NoPlayerDeploymentBase.et`
- `Prefabs/Systems/MilitaryBase/TB_PVE_M1_3_NoPlayerDeploymentBase.et.meta`
- `Scripts/Game/TB_PVE_M1_3/TB_PVE_M1_3_NoPlayerDeployment.c`
- this report

Modified:

- `Worlds/TB_PVE_M1_1A_CaptureTest_Layers/M1_1A_BLUFOR_Base.layer`: uses the experimental derivative and names the ordinary base `TB_M1_3_FORWARD_Base`.
- `Scripts/Game/TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c`: adds delayed testing, spawn/supply/MOB diagnostics, and AI-driven US -> USSR -> US regression.
- `resourceDatabase.rdb`: regenerated by Workbench for the new resources.

No vanilla resource was modified. No custom ownership, replication, teleport, supply, radio, or spawn manager was added.

## 8. MOB test result

**PASS:** the final dedicated assertion found exactly one enabled US HQ/MOB campaign spawn point: `enabledUSMobSpawnPoints=1`. Vanilla also logged a selected US MOB in both dedicated runs. This proves the mechanism is not global.

The manual local-authority acceptance session then verified that the deployment UI offered the Main Operating Base and that the player could deploy there successfully.

## 9. Forward-base deployment result

**Server/mechanism PASS:** final dedicated evidence:

`spawnPointValid=1 spawnEnabled=0 visibleInDeployUI=0 canReserve=0 reserveResult=3002`

The same result persisted after USSR captured the base. In the primary run the point remained disabled after the base returned to US.

**Manual UI PASS:** in the local-authority acceptance session the forward base was absent from the deployment UI while the US MOB remained offered. This is literal UI evidence, not a conclusion inferred solely from logs. Dedicated authority separately proved that the vanilla spawn request was rejected.

## 10. Respawn result

**PASS:** during the manual local-authority session the player was killed, the vanilla `RespawnSuperMenu` opened, the MOB remained available, the forward base remained unavailable, and the player respawned through the MOB.

## 11. Ownership-transition regression

**PASS in the primary dedicated + remote-client run.**

- Server: `PASS M1.3 AI-only OPFOR capture initialOwner=US intermediateOwner=USSR`.
- Server: `PASS M1.3 ownership regression sequence=US->USSR->US finalOwner=US`.
- Remote proxy: observed USSR and then `PASS M1.3 remote client observed authoritative regression finalOwner=US`.
- After return to US: `spawnPointValid=1 spawnEnabled=0`, supplies 237/1000, supply income 1.

The final assertion-only rerun selected a different random MOB/radio topology. US recapture did not start because the base was outside the returning faction's valid radio eligibility; that run was stopped rather than treated as a deployment failure. This demonstrates why the first completed regression is the relevant smallest practical equivalent and why future deterministic worlds should fix MOB/radio placement.

## 12. AI-capture regression

**PASS.** OPFOR was non-playable, had player limit zero, retained `CanCaptureBases=1`, and three vanilla USSR defender units completed vanilla capture. The harness never assigned ownership. Capture callbacks, ownership events, faction replication, and later US AI recapture all came through vanilla systems.

The later manual local-authority session produced a discrepant diagnostic: three USSR AI were reported inside the radius, but capture remained `NONE` and timed out. That session used `server=1`, `dedicated=0`, `proxy=0`. It does not overturn the M1.2 dedicated proof or the completed M1.3 dedicated capture/regression because:

- the production capture prefab was unchanged;
- the same M1.3 build completed AI capture under actual dedicated authority;
- the discrepancy is isolated to a different authority/session topology;
- “inside radius” alone does not prove every vanilla seizing prerequisite was satisfied.

Plausible causes include a local player or surviving BLUFOR entity contesting the base, faction/radio eligibility, initialization/order differences in a listen-server session, or a harness timing/state difference. Without the local session log and exact player position/entity state, the cause remains **UNKNOWN**. This is a non-blocking test-environment discrepancy, not evidence of a production capture regression. Production capture behavior must not be modified to accommodate it without reproducing and tracing the failed prerequisite.

## 13. Supply observations

- `m_SpawnPoint` was never null.
- Disabling it did not enter the supply timer's rejection condition by itself.
- Supply storage remained valid across ownership changes.
- Income followed radio/capture/cooldown state and recovered to 1 in the completed primary regression.
- No supply workaround was implemented.

Conclusion: no obvious supply prerequisite was destroyed. Full package timing, delivery, and edge cases remain M1.4 work.

## 14. AI-spawn observations

**VERIFIED:** the AI capture harness spawned vanilla campaign defender group resources and their vanilla unit slots successfully while the forward player spawn point was disabled.

**INFERRED:** base defenders/reinforcements are not registered through the player deploy selector. Source searches found no campaign AI path consuming `SCR_CampaignSpawnPointGroup.IsSpawnPointEnabled()`.

**UNKNOWN:** every future vanilla garrison/reinforcement variant has not been exercised. M1.3 did not build a garrison system.

## 15. Dedicated-server evidence

Primary completed run:

- `TestProfiles/M1_3_Server_Live/logs/logs_2026-09-21_09-32-09/console.log`

Final explicit spawn/MOB assertion run:

- `TestProfiles/M1_3_Server_Final/logs/logs_2026-09-21_09-38-38/console.log`

Workbench validation completed with no addon script compile errors after the final correction.

## 16. Remote-client evidence

- `TestProfiles/M1_3_Client_UI/logs/logs_2026-09-21_09-33-33/console.log`
- Separate client reached online state as `proxy=1`.
- It replicated US -> USSR -> US and logged the final regression PASS.
- The native window was not exposed to the available computer-control API, so this run supplies replication evidence rather than literal UI evidence.

Manual local-authority UI evidence:

- joined US/BLUFOR successfully;
- deployment UI offered the MOB and omitted the forward base;
- deployed at the MOB;
- after death, vanilla `RespawnSuperMenu` again offered the MOB and omitted the forward base;
- respawned at the MOB;
- screenshots captured the MOB-only deployment state.

Evidence provenance is therefore explicit: the literal UI interaction was manually verified locally, while the deployment contract, authoritative rejection, dedicated ownership regression, and proxy replication were verified separately.

## 17. Remaining unknowns

1. Root cause of the local-authority AI capture timeout despite three reported USSR AI inside the radius.
2. A literal UI interaction on a separate remote client. This is no longer blocking because the UI path was manually verified and dedicated/proxy tests separately verified the authoritative and replicated contract.
3. An attempted direct/stale forward-base request from a real client, although authority already returns `NOT_ALLOWED_SPAWNPOINT_DISABLED`.
4. Full supply delivery/regeneration behavior (reserved for M1.4).
5. Broader garrison/reinforcement behavior with this flag.
6. Whether the project ultimately prefers the small visibility override or accepts a visible-but-disabled row; authoritative suppression does not depend on that choice.

## 18. Recommended M1.4 scope / next action

Do **not** start M1.4 automatically. Before M1.4 implementation, preserve and inspect the local-authority timeout log and compare its seizing prerequisites against the successful dedicated run: contesting factions/entities, playable-character presence, faction eligibility, radio state, initialization timing, and capture callbacks. Reproduce it once in a controlled local session if practical. Do not alter `TB_PVE_ConflictMilitaryBase.et` unless that investigation demonstrates a real production regression.

After that discrepancy is classified, M1.4 should test supply package timing and regeneration across ownership/radio/cooldown states without adding a workaround pre-emptively.

## Acceptance matrix

| Criterion | Result | Evidence |
| --- | --- | --- |
| A. MOB remains valid player deployment | **PASS** | Dedicated assertion found one enabled US MOB; manual UI offered it and deployment succeeded. |
| B. US forward base not usable | **PASS** | Dedicated assertion showed hidden/disabled and reservation rejected; manual UI omitted it. |
| C. Player can spawn/respawn at MOB | **PASS** | Manual deployment and post-death `RespawnSuperMenu` flow both succeeded. |
| D. Player cannot spawn/respawn at forward base | **PASS** | Manual deployment and respawn UI omitted it; authority rejected reservation (`3002`). |
| E. Restriction survives return to BLUFOR | **PASS** | Dedicated US -> USSR -> US run retained a valid, disabled spawn point after return. |
| F. Forward base remains functional Conflict base | **PASS** | Ownership, capture, faction handling, supplies, and AI presence remained functional in dedicated testing. |
| G. AI-only OPFOR vanilla capture works | **PASS** | Non-playable USSR AI completed vanilla capture. |
| H. Critical spawn-point state not deleted/nulled | **PASS** | `spawnPointValid=1` throughout. |
| I. No obvious supply prerequisite destroyed | **PASS** | Valid reference, supplies retained, income recovered in completed run. |
| J. Dedicated + remote client behavior confirms restriction | **PASS across combined evidence** | Dedicated authority proved rejection and ownership durability; remote proxy proved replication; manual local authority proved literal UI/deploy/respawn behavior. |

All M1.3 acceptance criteria are supported across the combined dedicated multiplayer and manual local-authority evidence. M1.3 is **PASS**.
