# TB PvE Conflict — M0 Technical Reconnaissance

**Target:** Arma Reforger / Enfusion Workbench  
**Local game build inspected:** `1.8.0.13`  
**Vanilla script snapshot:** Bohemia Interactive `Arma-Reforger-Script-Diff`, `main` at `3d77cc212d5cda9922daf5f45635c7300d2d4cce`  
**Project dependency:** vanilla data GUID `58D0FB3206B6F859`  
**Scope:** static reconnaissance only; no gameplay system was implemented.

## Evidence labels

- **VERIFIED** — directly present in the inspected vanilla script source, local project metadata, or installed executable metadata.
- **INFERRED** — a strong conclusion from those sources, but not exercised in play mode or on a dedicated server.
- **UNKNOWN** — requires resource inspection in Workbench or a runtime experiment.

## 1. Executive summary

Vanilla Conflict is reusable for the proposed PvE mode. Its base ownership, physical capture, radio graph, spawn-point faction assignment, supplies, flags, map feedback, and replication already form a coherent server-authoritative system. The future extension should constrain vanilla state transitions rather than replace them.

Acceptance-criteria answers:

| Question | Answer | Status |
|---|---|---|
| A. Can AI units physically capture vanilla Conflict bases? | **Yes in code.** `SCR_CampaignSeizingComponent` counts live `SCR_ChimeraCharacter` entities, including active AI, by faction. AI is rejected only at maximum AI LOD/dormancy, when the faction cannot capture, when non-playable attackers are explicitly ignored, or when the component is configured to require a player. | **VERIFIED statically; runtime smoke test still required** |
| B. How can ownership changes be detected? | Subscribe to `SCR_MilitaryBaseSystem.GetOnBaseFactionChanged()` for the general event, or `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended()` for Conflict base + previous/new faction. Capture start/interrupt/finish and capture-state invokers are also exposed. | **VERIFIED** |
| C. How can humans be restricted to BLUFOR? | Configure OPFOR `SCR_Faction.m_bIsPlayable = false` or player limit `0`; leave `m_bCanCaptureBases = true`. Non-playable factions do not appear in the respawn/faction menu, while capture ability is a separate flag. | **VERIFIED in code; configured resource must be checked in play mode** |
| D. How can MOB-only player spawning be enforced? | Prefer inherited forward-base prefabs/world entities with no child `SCR_SpawnPoint` (or a permanently disabled/unassigned spawn point). Keep the MOB’s spawn point. The base component discovers a child spawn point and continuously assigns/clears its faction according to ownership, radio, capture state, cooldown, and supplies. | **VERIFIED mechanism; exact inherited-prefab override requires Workbench validation** |
| E. How is the available objective determined/controlled? | Vanilla eligibility is radio reachability: `IsValidTarget()` requires enemy ownership plus `IsHQRadioTrafficPossible(faction)`. `SelectAndReturnPrimaryTarget()` uses the same radio-accessible set. A linear route can therefore be authored through radio geometry, but strict single-objective locking should use a very small server-side gate at the seizing/eligibility boundary if geometry alone proves ambiguous. | **VERIFIED mechanism; strict linear configuration is INFERRED** |
| F. What can stay vanilla? | Base ownership, character counting, timed capture, capture events, faction affiliation, radio coverage, base initialization, control-point accounting, flags/UI, supplies, spawn-point registration, and replication. | **VERIFIED** |
| G. What must M1 runtime-test? | AI-only capture, non-playable OPFOR capture, MOB-only spawning after capture/reconnect/JIP, radio-authored linear eligibility and rollback, ownership-event order on dedicated server, supplies across capture, and defender lifecycle/streaming. | **DEFINED below** |

The main caveat is that the official script-diff contains scripts, not all packed prefab/config values. The code paths are directly verified; the actual vanilla `ConflictMilitaryBase.et` values for `m_bCapturingRequiresPlayer`, non-playable attacker filtering, and embedded spawn-point layout must be confirmed in Workbench before implementation.

## 2. Relevant vanilla classes/components

### Game mode and campaign state

- **VERIFIED:** `SCR_GameModeCampaign` — `scripts/Game/GameMode/SCR_GameModeCampaign.c`. Primary Conflict game mode. It owns `SCR_CampaignMilitaryBaseManager`, faction keys, victory/control-point state, supply settings, campaign start state, and radio spawn-point bookkeeping.
- **VERIFIED:** `SCR_CampaignMilitaryBaseManager` — `scripts/Game/Campaign/SCR_CampaignMilitaryBaseManager.c`. Maintains all Conflict bases and control points, chooses HQs, initializes bases/callsigns/supplies, recalculates coverage, evaluates control points, and offers targeting queries.
- **VERIFIED:** `SCR_MilitaryBaseSystem` — global base registry/event source used by the campaign manager (`GetBases`, base registration/unregistration, ownership and logic-component events).
- **VERIFIED:** `SCR_CampaignFactionManager` — `scripts/Game/GameMode/FactionManager/SCR_CampaignFactionManager.c`. Conflict-specific faction relationships and helpers.
- **VERIFIED:** `SCR_CampaignFaction` — `scripts/Game/Faction/SCR_CampaignFaction.c`. Stores main base, defender-group prefab, starting vehicles, campaign spawn/build settings, radio data, and control-point/victory state.

### Bases and capture

- **VERIFIED:** `SCR_MilitaryBaseComponent` — `scripts/Game/Components/Locations/SCR_MilitaryBaseComponent.c`. Generic base, requiring `FactionAffiliationComponent`; holds radius, logic components, service delegates, child capture points, and base ownership workflow.
- **VERIFIED:** `SCR_CampaignMilitaryBaseComponent` — `scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c`. Conflict specialization: base type, HQ/control-point state, spawn point, radio coverage, supplies, capture state, defender-group references, UI and persistence-facing state.
- **VERIFIED:** `SCR_SeizingComponent` — `scripts/Game/GameMode/Components/SCR_SeizingComponent.c`. Generic server-side character counting and timed seizure.
- **VERIFIED:** `SCR_CampaignSeizingComponent` — `scripts/Game/GameMode/Components/SCR_CampaignSeizingComponent.c`. Conflict-specific capture rules: `CanCaptureBases`, radio requirement, contest state, service/radio capture-time modifiers.
- **VERIFIED:** `SCR_FactionAffiliationComponent`. Authoritative storage for the base’s affiliated faction. `SCR_MilitaryBaseComponent.GetFaction()` reads it and `SetFaction()` writes it on authority.
- **VERIFIED:** `SCR_FlagComponent` and registered `SCR_MilitaryBaseLogicComponent` instances react to ownership changes.

### Radio, spawning, and supplies

- **VERIFIED:** `SCR_CoverageRadioComponent` and `SCR_RadioCoverageSystem` form the radio graph.
- **VERIFIED:** `SCR_SpawnPoint` — `scripts/Game/GameMode/Respawn/SCR_SpawnPoint.c`; globally registered spawn entity with replicated faction key, enable state, priority, and events.
- **VERIFIED:** `SCR_PlayerSpawnPoint`, `SCR_CampaignSpawnPointGroup`, and `SCR_PlayerRadioSpawnPointCampaign` participate in player deployment.
- **VERIFIED:** `SCR_ResourceComponent`, `SCR_ResourceConsumer`, `SCR_ResourceGenerator`, `SCR_ResourceConsumerMilitaryBase`, and `SCR_ResourceGeneratorMilitaryBase` supply storage/transfer behavior.
- **VERIFIED:** `SCR_CampaignSuppliesComponent` represents Conflict supply entities/depots.

### Important resource name

- **VERIFIED:** `{1391CE8C0E255636}Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et` is spawned when the campaign manager converts a generic military base into a Conflict base.
- **UNKNOWN:** The full component values of that packed prefab were not exported or modified during M0.

## 3. Conflict base lifecycle

1. **VERIFIED:** `SCR_GameModeCampaign` constructs its `SCR_CampaignMilitaryBaseManager` and starts campaign setup from `Start()` / `InitalBaseSetup()`.
2. **VERIFIED:** The manager reads registered bases from `SCR_MilitaryBaseSystem`, filters `SCR_CampaignMilitaryBaseComponent`, builds `m_aBases` and `m_aControlPoints`, and chooses/assigns HQs.
3. **VERIFIED:** `InitializeBases()` applies a default faction (or INDFOR if unset), assigns callsigns, initializes supplies, and leaves base logic registered with the global system.
4. **VERIFIED:** `SCR_CampaignMilitaryBaseComponent.Initialize()` powers its coverage radio, sets replicated `m_bInitialized`, bumps replication, and runs `OnInitialized()`.
5. **VERIFIED:** `OnAllBasesInitialized()` updates lists, fires `GetOnAllBasesInitialized()`, recalculates BLUFOR/OPFOR coverage, evaluates control points, and associates ambient remnants.
6. **VERIFIED:** Capture completion changes `SCR_FactionAffiliationComponent`; its faction-change event calls `SCR_MilitaryBaseComponent.OnFactionChanged()`, then the Conflict override.
7. **VERIFIED:** Ownership change clears capture state, resets defender reward state and supply regeneration, changes radio settings, recalculates coverage, starts the forward-base respawn cooldown, updates spawn ownership, sends messages, redirects surviving remnants, and requests an autosave.

Useful lifecycle hooks:

- `SCR_MilitaryBaseSystem.GetOnBaseRegistered()` / `GetOnBaseUnregistered()`.
- `SCR_CampaignMilitaryBaseManager.GetOnAllBasesInitialized()`.
- `SCR_CampaignMilitaryBaseManager.GetOnSignalChanged()`.
- `SCR_MilitaryBaseSystem.GetOnBaseFactionChanged()`.
- `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended()`.
- `SCR_CampaignMilitaryBaseComponent.GetOnBaseUnderAttack()` / `GetOnBaseAttackEnd()`.
- `SCR_CampaignMilitaryBaseComponent.GetOnBaseCaptured()` — documented as base + player ID, but this is explicitly invoked by relay capture; do not treat it as the sole generic ownership event.

## 4. Capture mechanics

### Presence collection

- **VERIFIED:** `SCR_SeizingComponent.EvaluatePrevailingFaction()` and the Conflict override query `TagSystem.GetTagsInRange(..., ETagCategory.Character)` around the base origin and configured radius.
- **VERIFIED:** `EvaluateEntityFaction()` accepts only `SCR_ChimeraCharacter`, excludes characters above the configured altitude while in a vehicle, excludes non-alive characters, and obtains faction from `char.GetFaction()`.
- **VERIFIED:** AI characters are explicitly considered. For non-player entities the method inspects `AIControlComponent`; an agent at maximum LOD is ignored. Active/local AI is not rejected.
- **VERIFIED:** Each faction’s raw presence is counted. Conflict selects a unique maximum-presence attacker; ties do not capture.
- **VERIFIED:** Effective seizing strength is `min((2 * attackerPresence) - totalPresence, maximumSeizingCharacters)`. Thus defenders and third parties reduce effective strength.

### Eligibility gates

An attacking faction is ignored if any applicable gate fails:

- **VERIFIED:** It is the current defender.
- **VERIFIED:** `SCR_Faction.CanCaptureBases()` is false.
- **VERIFIED:** `m_bIgnoreNonPlayableAttackers` is true and `faction.IsPlayable()` is false.
- **VERIFIED:** `m_bCapturingRequiresPlayer` is true and no player-controlled character of that faction is present.
- **VERIFIED:** It lacks a unique superior presence.
- **VERIFIED:** For a playable `SCR_CampaignFaction`, the target does not have receive-path radio traffic to that faction’s HQ. The code deliberately bypasses this radio gate for non-playable campaign factions: `!cFaction.IsPlayable() || m_Base.IsHQRadioTrafficPossible(cFaction)`.
- **VERIFIED:** HQs and uninitialized bases return no faction from `SCR_CampaignSeizingComponent.EvaluateEntityFaction()` and cannot be captured through this normal area path.

### Timer and completion

- **VERIFIED:** Capture time interpolates between maximum and minimum by effective character count.
- **VERIFIED:** Conflict increases duration for built services and defending-side radio connections.
- **VERIFIED:** Capture timestamps are replicated. When the server timestamp reaches the end, a reliable broadcast RPC fires capture-finish callbacks and authority changes the affiliated faction.
- **VERIFIED:** Generic capture invokers: `GetOnCaptureStart()`, `GetOnCaptureInterrupt()`, `GetOnCaptureFinish()`, `GetOnTimerChange()`.
- **VERIFIED:** Conflict adds `GetOnCaptureStateChanged()` with `NONE`, `CONTESTED`, and `CAPTURING`.

## 5. AI capture feasibility

### Static conclusion

**VERIFIED:** AI-controlled OPFOR characters can contribute to—and can be the entire force performing—a vanilla Conflict capture, provided:

1. The OPFOR faction has `Can Capture Bases` enabled.
2. The Conflict seizing component does not have `Capturing Requires Player` enabled.
3. If OPFOR is non-playable, `Ignore Non Playable Attackers` is disabled.
4. The AI agents are alive, inside the radius/altitude limits, tagged as characters, and active rather than culled at maximum LOD.
5. The target is initialized and not an HQ.
6. OPFOR has uniquely superior presence.

An important favorable detail is **VERIFIED:** non-playable `SCR_CampaignFaction` attackers do not need the HQ-radio check in `SCR_CampaignSeizingComponent`. Making OPFOR non-playable for humans therefore does not inherently prevent its AI from capturing.

### Restrictions and caveats

- **UNKNOWN:** Exact vanilla `ConflictMilitaryBase.et` defaults for `m_bCapturingRequiresPlayer` and `m_bIgnoreNonPlayableAttackers` must be checked in Workbench.
- **UNKNOWN:** Whether distant AI directed to a target remains at an eligible LOD on a headless/dedicated server under the intended streaming budget.
- **VERIFIED:** The ordinary area-capture path excludes HQs. The final OPFOR HQ and BLUFOR-loss-at-MOB conditions therefore cannot be assumed to work as ordinary base seizures. M1 must decide whether the world setup uses non-HQ terminal objectives, relay interaction, or a minimal explicit terminal rule.
- **INFERRED:** No custom capture system is justified. At most, an inherited prefab setting or a narrow server-side eligibility hook is needed.

### Smallest runtime experiment

Run a dedicated-server test with one neutral/BLUFOR non-HQ `ConflictMilitaryBase.et` and one OPFOR AI fireteam:

1. Set OPFOR non-playable but `Can Capture Bases = true`.
2. Confirm the seizing component settings above.
3. Move/waypoint the AI into the radius with no human OPFOR player.
4. Log start, state, finish, faction-change, and resulting `GetFaction()`.
5. Repeat with the AI streamed/dormant and then activated.

Pass condition: ownership changes to OPFOR using only vanilla capture logic.

## 6. Base connection/progression mechanics

Vanilla Conflict does not maintain a separate authored adjacency list in the inspected scripts. Connectivity is a dynamic radio graph.

- **VERIFIED:** `SCR_CampaignMilitaryBaseComponent.GetHQRadioCoverage(faction)` gets the faction HQ coverage status via matching radio encryption.
- **VERIFIED:** `IsHQRadioTrafficPossible(faction, direction)` supports `RECEIVE`, `SEND`, and `BOTH_WAYS`.
- **VERIFIED:** `CanReachByRadio(entity)` tests whether another coverage radio is in the connected-radio set.
- **VERIFIED:** `IsValidTarget(faction)` means “not already owned” and “can receive HQ traffic for this faction.”
- **VERIFIED:** `SCR_CampaignMilitaryBaseManager.SelectAndReturnPrimaryTarget()` selects from initialized, non-HQ, enemy bases reachable through the faction’s HQ radio network, preferring control points and proximity.
- **VERIFIED:** Capture itself uses HQ receive coverage for playable factions.
- **VERIFIED:** Captures and radio-range changes recalculate coverage; losing a relay/base can therefore remove downstream reachability.

### Linear campaign fit

- **INFERRED:** A chain of deliberately spaced/ranged radios can naturally produce `MOB -> OBJ_01 -> OBJ_02...`: each captured objective adopts the faction radio encryption and extends its network, while loss can break downstream coverage.
- **RISK:** Radio is range/graph based, not an explicit ordered edge list. Terrain placement or overlapping ranges may expose two objectives, allow a skip, or keep a later base connected after a rollback.
- **Recommended extension point:** first attempt world/entity and inherited-prefab radio configuration. If it cannot guarantee exactly one objective, add one server-authoritative “is this base currently eligible?” policy at the Conflict capture boundary. Do not replace the radio or capture systems. The policy should consume vanilla ownership/radio events and only allow/deny progress.
- **UNKNOWN:** Whether protected fields/method dispatch in the current script API permit a clean `modded class SCR_CampaignSeizingComponent` gate without copying its evaluation method. Validate compile-time extension options in M1 before choosing composition versus override.

## 7. Player spawn architecture

- **VERIFIED:** `SCR_SpawnPoint` registers globally on initialization and broadcasts `Event_SpawnPointAdded`, `Event_SpawnPointRemoved`, and `Event_SpawnPointFactionAssigned`.
- **VERIFIED:** Its faction key and enabled flag are replicated. `SetFactionKey()` updates the key, invokes `OnSetFactionKey()`, updates the faction object/event, and bumps replication.
- **VERIFIED:** `SCR_CampaignMilitaryBaseComponent` searches its children for the first `SCR_SpawnPoint`, stores it as `m_SpawnPoint`, configures its spawn radius, and exposes `GetOnSpawnPointAssigned()`.
- **VERIFIED:** `HandleSpawnPointFaction()` assigns that child point to the base owner only when vanilla conditions permit. Non-HQ bases lose spawning while disconnected both ways from HQ, captured/contested, or in post-capture cooldown. Supply/resource changes also cause reevaluation.
- **VERIFIED:** Ownership changes normally call `HandleSpawnPointFaction()`, which is why captured bases become spawn locations.

### Least-invasive MOB-only strategy

1. Create inherited TB base prefabs/resources; never edit vanilla.
2. Keep the child spawn point only on the BLUFOR MOB prefab.
3. Remove/disable the child `SCR_SpawnPoint` in every forward-objective variant while retaining `SCR_CampaignMilitaryBaseComponent`, `SCR_CampaignSeizingComponent`, radio, resources, services, and flags.
4. If prefab inheritance cannot remove the child safely, use a disabled spawn-point variant or a tiny server-side policy that permanently clears/disables only forward spawn points after `GetOnSpawnPointAssigned()`.

**INFERRED:** No child `SCR_SpawnPoint` is the cleanest option because base capture, radio, supplies, services, and ownership do not require `m_SpawnPoint`; however, vanilla supply auto-regeneration currently checks `!m_SpawnPoint` and returns early. Therefore removal may also disable automatic supply income at forward bases. This interaction makes “present but disabled/unassigned” the safer first M1 experiment.

**UNKNOWN:** Whether setting `Spawn Point Enabled = false` in an inherited prefab remains untouched by base `HandleSpawnPointFaction()` and is correctly excluded from deploy UI/server spawn validation after JIP. Test it.

## 8. Faction/playability architecture

- **VERIFIED:** `SCR_Faction` separates `m_bIsPlayable` from `m_bCanCaptureBases`.
- **VERIFIED:** Its documentation states that non-playable factions do not appear in the respawn menu.
- **VERIFIED:** A player limit of `0` forces the faction non-playable; `SetPlayerLimit()` also updates playability.
- **VERIFIED:** `SCR_CampaignFaction` remains a complete AI/gameplay faction when non-playable: it still has a main base, defender prefab, starting vehicles, radio properties, capture permission, building data, and entity catalogs.
- **Recommendation:** inherit the OPFOR faction config, set `Is Playable = false` (and preferably player limit `0` as defense in depth), retain `Can Capture Bases = true`, and leave BLUFOR playable.
- **UNKNOWN:** Confirm the scenario mission header/server `playerLimits` does not override the intended inherited faction values. Test faction selection and reconnect on dedicated server.

## 9. AI defender architecture

- **VERIFIED:** `SCR_CampaignFaction.m_DefendersGroupPrefab` is a faction-configured “Defenders group prefab.” This is the primary faction-level reuse point.
- **VERIFIED:** Entities spawned through Free Roam Building reach `SCR_GameModeCampaign.OnEntityRequested()`. AI groups are passed to `SCR_CampaignMilitaryBaseManager.OnDefenderGroupSpawned()`, which associates them with relevant bases through `SetDefendersGroup()`.
- **VERIFIED:** The campaign manager also processes `SCR_AmbientPatrolSpawnPointComponent` remnants near bases. After a playable faction captures a non-HQ base, surviving remnants may receive a timed search-and-destroy waypoint back at that base; dormant/not-yet-spawned remnants are eliminated instead.
- **VERIFIED:** Service/catalog components (`SCR_CatalogEntitySpawnerComponent`) are registered through the base/service system and use faction catalogs.
- **UNKNOWN:** Packed prefab/config values determine how many defenders exist, when the Free Roam Building spawner requests them, and exact despawn/streaming persistence. The scripts do not establish that every base has a permanently resident garrison.
- **Recommendation:** reuse the faction defender-group prefab and vanilla ambient/service spawners for the first experiment. Do not build a garrison manager until actual lifecycle behavior is measured.

## 10. Radio mechanics

- **VERIFIED:** Base ownership changes update encryption key and frequency on the base radio, call `SCR_RadioCoverageSystem.UpdateAll()`, and refresh affected seizing timers.
- **VERIFIED:** Antenna services can enlarge the base radio range; the base copies the maximum antenna range to its own coverage transceivers and powers the separate antenna radio down for performance.
- **VERIFIED:** Coverage status directly affects capture eligibility for playable factions, spawn availability, control-point scoring, supply regeneration, map visibility/feedback, and capture duration.
- **VERIFIED:** INDFOR is special-cased as always having traffic for several checks.
- **INFERRED:** Linear progression can coexist with vanilla radio and should use it as the default connectivity signal.
- **Risk:** Making OPFOR non-playable bypasses the radio requirement for OPFOR capture. If OPFOR must obey the same frontline chain, the future eligibility policy must explicitly enforce the desired route for OPFOR, or OPFOR must remain technically playable but unavailable to humans through another tested server-side join rule. This is a core M1 decision.

## 11. Supply mechanics

- **VERIFIED:** `SCR_CampaignMilitaryBaseComponent` wraps base supply access through `SCR_ResourceComponent` consumer/generator APIs.
- **VERIFIED:** Initial supplies are assigned during campaign initialization, optionally randomized by distance/order.
- **VERIFIED:** Periodic income is server-driven. It requires an owning faction, a stored spawn point, valid two-way HQ radio (except source bases), no hostile capturing faction, and expiration of the post-capture respawn cooldown.
- **VERIFIED:** HQ/supply-hub and ordinary-base income differ. Optional base autoregeneration can add extra income for connected friendly bases, capped by `BASES_IN_RANGE_RESUPPLY_THRESHOLD`.
- **VERIFIED:** On ownership change, the code resets the reinforcement timer and recalculates regeneration for the new faction. It does **not** explicitly zero or transfer the stored supply quantity in `OnFactionChanged()`.
- **INFERRED:** Stored supplies remain in the same base resource storage across capture, while future income follows the new owner and connectivity.
- **UNKNOWN:** Confirm exact visible amount and usable ownership immediately after capture in runtime, including service-provider ownership and any resource callbacks outside the inspected method.
- **Risk:** Physically removing forward spawn points may suppress vanilla periodic supply income because `SupplyIncomeTimer()` requires `m_SpawnPoint`. Prefer disabling player deployment without removing the internal spawn-point entity until this is tested.

## 12. Authority and replication findings

### Server-authoritative work

- **VERIFIED:** `SCR_SeizingComponent.OnPostInit()` explicitly says all functionality is server-side and returns on proxies before scheduling presence evaluation.
- **VERIFIED:** Presence counting, timer calculation, capture completion, ownership mutation, spawn-point faction decisions, supply income, base initialization, radio coverage evaluation, and control-point evaluation are authority paths guarded by `IsProxy()`, `RplRole.Authority`, or `Replication.IsServer()`.
- **VERIFIED:** `SCR_MilitaryBaseComponent.SetFaction()` and the Conflict overload return on proxies.
- **Rule:** Future progression eligibility and OPFOR win/loss decisions must run on authority. Clients may display replicated state but must not decide ownership or unlocks.

### Replicated state/callbacks

- **VERIFIED:** `SCR_GameModeCampaign` replicates started state, victory timestamps/winning faction, and callsign offset using `RplProp` callbacks.
- **VERIFIED:** `SCR_SeizingComponent` replicates start/end timestamps with `OnSeizingTimestampChanged`.
- **VERIFIED:** Capture start, interrupt, and finish are reliable broadcast RPCs; Conflict capture-state changes also use a reliable broadcast RPC.
- **VERIFIED:** `SCR_CampaignMilitaryBaseComponent` replicates initialized/HQ flags, capture state, capturing and attacking faction indices, radio range, respawn timestamp, supply-control values, built state/faction, enemy presence, supply arrival time, and regeneration amount.
- **VERIFIED:** `SCR_SpawnPoint` replicates faction key, enabled state, priority, name/time values.
- **VERIFIED:** Most authority mutations call `Replication.BumpMe()` after state changes.

### Preferred observation points

- Global ownership: `SCR_MilitaryBaseSystem.GetOnBaseFactionChanged()`.
- Conflict ownership with old/new factions: `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended()`.
- Radio topology: `SCR_CampaignMilitaryBaseManager.GetOnSignalChanged()`.
- Capture activity: the seizing component start/interrupt/finish and capture-state invokers.
- Base initialization: `GetOnAllBasesInitialized()`.
- Spawn registration/faction: `SCR_SpawnPoint` static events.

**UNKNOWN:** Exact callback order and duplicate local/server invocation behavior for every event on a dedicated server with JIP must be recorded once before building stateful listeners.

## 13. Recommended extension points

### Prefer configuration/resources

- Inherited scenario game-mode prefab based on the vanilla Conflict game mode.
- Inherited `ConflictMilitaryBase.et` variants for MOB and forward objectives.
- World placement, HQ candidacy/type/control-point flags, initialization/default faction, radio range, and antenna setup.
- Inherited BLUFOR/OPFOR faction configs: OPFOR non-playable but capture-capable; reuse defender group and catalogs.
- Spawn-point component configuration or a forward-base spawn variant that stays present for supplies but is not player-deployable.

### Prefer event-driven script extension only where configuration is insufficient

- Observe ownership through the existing events.
- Add a small, authority-only objective-order policy only if radio geometry cannot guarantee strict linearity and backward relocking.
- Keep the policy data-only: ordered base identifiers/callsigns/entity names and current valid index; do not own capture, faction, supplies, spawn, defenders, or radio.
- Gate the actual vanilla capture start/evaluation, rather than reverting ownership after a forbidden capture.

### Do not replace

- `SCR_CampaignSeizingComponent` character counting and timer.
- `SCR_FactionAffiliationComponent` ownership.
- `SCR_MilitaryBaseSystem` registration/events.
- `SCR_RadioCoverageSystem` graph.
- `SCR_SpawnPoint` registration/replication.
- Resource/supply components.
- Vanilla map/flag/feedback updates.

## 14. Known risks/unknowns

1. **Prefab defaults:** packed `ConflictMilitaryBase.et` values for player-required and non-playable capture filters are not yet verified.
2. **MOB/HQ capture:** ordinary Conflict area capture rejects HQs. Terminal victory/loss needs an explicit, minimal design choice.
3. **OPFOR radio bypass:** non-playable campaign factions bypass the vanilla capture radio gate, so strict OPFOR rollback progression needs its own eligibility check if this remains true in runtime.
4. **Spawn versus supplies coupling:** absence of `m_SpawnPoint` prevents periodic supply regeneration.
5. **Strict topology:** dynamic radio ranges may create unintended alternate paths.
6. **Defender persistence:** exact prefab-controlled activation, streaming, respawn, and deletion behavior remains unmeasured.
7. **Dedicated event order/JIP:** replicated callbacks and static invokers need a single instrumentation pass.
8. **Source/build parity:** the installed executable is `1.8.0.13`; the official current script-diff snapshot was inspected, but exact symbol-for-symbol correspondence to the installed packed data was not independently proven.
9. **Persistence:** serializers exist for the campaign game mode and military-base component, but a custom ordered-front state would require an explicit persistence decision only if it cannot be derived from current ownership.

## 15. Proposed M1 scope

M1 should be a disposable, minimal validation scenario—not production architecture.

1. Create one inherited Conflict game-mode prefab and three inherited base variants: BLUFOR MOB, ordinary objective, and OPFOR terminal candidate.
2. Configure BLUFOR playable; OPFOR non-playable, player limit `0`, and capture-capable.
3. Instrument only existing invokers: base initialized, signal changed, capture start/state/finish, faction changed, and spawn-point faction/enabled state.
4. Perform an AI-only OPFOR capture on a dedicated server.
5. Test forward spawn-point strategies in this order:
   - present but disabled;
   - present but permanently faction-unassigned through a narrow authority policy;
   - absent, while checking the supply-income side effect.
6. Place MOB, OBJ_01, OBJ_02 and tune radio ranges. Verify OBJ_02 cannot be captured before OBJ_01, becomes valid afterward, and locks again after OBJ_01 is lost—for both factions.
7. Measure supplies before/during/after capture and after reconnect/JIP.
8. Measure faction defender/remnant activation and cleanup at near/far player distances.
9. Decide the terminal HQ rule only after confirming that ordinary HQs remain uncapturable.

**Recommended first M1 task:** build the smallest dedicated-server AI-capture harness around one non-HQ base, with OPFOR non-playable and all vanilla capture events logged. This resolves the highest-risk premise without committing to campaign architecture.

## Files inspected

### Local project/environment

- `addon.gproj`
- `resourceDatabase.rdb` (identity/header only; binary database was not modified)
- Installed `ArmaReforgerSteam.exe` version metadata
- Installed Workbench public script API index under `Workbench/docs/ArmaReforgerScriptAPIPublic/html`

### Official Bohemia Interactive script snapshot

- `scripts/Game/GameMode/SCR_GameModeCampaign.c`
- `scripts/Game/Campaign/SCR_CampaignMilitaryBaseManager.c`
- `scripts/Game/Components/Locations/SCR_MilitaryBaseComponent.c`
- `scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c`
- `scripts/Game/GameMode/Components/SCR_SeizingComponent.c`
- `scripts/Game/GameMode/Components/SCR_CampaignSeizingComponent.c`
- `scripts/Game/GameMode/Components/SCR_MilitaryBaseLogicComponent.c`
- `scripts/Game/GameMode/FactionManager/SCR_CampaignFactionManager.c`
- `scripts/Game/GameMode/FactionManager/SCR_FactionManager.c`
- `scripts/Game/Faction/SCR_Faction.c`
- `scripts/Game/Faction/SCR_CampaignFaction.c`
- `scripts/Game/GameMode/Respawn/SCR_SpawnPoint.c`
- `scripts/Game/GameMode/Respawn/SCR_PlayerSpawnPoint.c`
- `scripts/Game/GameMode/Respawn/SCR_PlayerRadioSpawnPointCampaign.c`
- `scripts/Game/Campaign/SCR_CampaignSpawnPointGroup.c`
- `scripts/Game/Campaign/SCR_CampaignSuppliesComponent.c`
- `scripts/Game/Campaign/SCR_CampaignBuildingManagerComponent.c`
- `scripts/Game/Campaign/SCR_FreeRoamBuildingBaseTriggerEntity.c`
- `scripts/Game/Campaign/SCR_FreeRoamBuildingClientTriggerEntity.c`
- `scripts/Game/Components/Locations/SCR_AmbientPatrolSpawnPointComponent.c`
- `scripts/Game/GameMode/Components/Resources/Consumer/SCR_ResourceConsumerMilitaryBase.c`
- `scripts/Game/GameMode/Components/Resources/Generator/SCR_ResourceGeneratorMilitaryBase.c`
- `scripts/Game/Plugins/Persistence/System/Serializers/Entities/SCR_GameModeCampaignSerializer.c`
- `scripts/Game/Plugins/Persistence/System/Serializers/Components/GameMode/Conflict/SCR_CampaignMilitaryBaseComponentSerializer.c`

## Files created/modified in M0

- Created `Documentation/TB-PvE-Conflict-M0-Technical-Reconnaissance.md`.
- No scripts, prefabs, configs, worlds, or vanilla resources were created or modified.

## Source provenance

The code evidence above comes from Bohemia Interactive’s official [Arma Reforger Script Diff repository](https://github.com/BohemiaInteractive/Arma-Reforger-Script-Diff). The repository is a script snapshot, not a substitute for inspecting packed prefab values in Workbench; those values are deliberately marked **UNKNOWN** where relevant.
