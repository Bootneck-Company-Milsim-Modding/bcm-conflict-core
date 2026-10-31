# TB PvE Conflict Engine — M1.1 AI Capture Runtime Proof

## Result

**FAIL — the acceptance criteria are not met.**

The runtime test verified that vanilla Conflict detects live AI faction presence inside a real base capture radius, but it did not verify an AI-only ownership transition. During the decisive run, three vanilla BLUFOR AI remained alive inside the target radius for more than five minutes. The base changed from capture state `0` to `1`, but its owner stayed `FIA`; `CaptureStart`, `CaptureFinish`, and both ownership-change callbacks never fired.

No PASS is claimed for M1.1.

## Runtime test configuration

- Game: Arma Reforger diagnostic client `1.8.0.13`.
- World: vanilla `$ArmaReforger:worlds/MP/CTI_Campaign_Eden.ent`.
- Addon: `BCM Conflict Core`, GUID `6A6A5973FA53A5B1`.
- Authority: offline/listen authority (`server=1`, `dedicated=0`, `proxy=0`).
- Target: vanilla Airbase, callsign `34`, initial owner `FIA`, capture radius `150 m`.
- Bootstrap approach: because a fresh Eden Conflict world has no non-HQ BLUFOR-owned base, the harness first attempted to let vanilla BLUFOR AI capture an INDFOR base. Only after that vanilla ownership transition would it remove BLUFOR and spawn non-playable OPFOR AI.
- OPFOR setup: vanilla USSR campaign faction with `SetPlayerLimit(0)`; `CanCaptureBases()` remained true. No custom faction was introduced.
- AI: three characters from the vanilla `{D4A1576757665D02}Prefabs/Groups/Campaign/Group_US_Defenders.et` unit slots, registered with a vanilla `SCR_AIGroup`.
- Capture logic: untouched `SCR_CampaignSeizingComponent`; the harness never calls `SetFaction`, `BeginCapture`, `EndCapture`, or any ownership RPC.

## Instrumentation added

Disposable harness:

`Scripts/Game/TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c`

It:

1. Runs only on the authority server.
2. Logs faction playability/player limit and `CanCaptureBases()`.
3. Selects an initialized non-HQ BLUFOR base, or bootstraps a neutral target through vanilla BLUFOR AI capture.
4. Subscribes to vanilla capture-start, capture-interrupt, capture-finish, capture-state, and ownership-change invokers.
5. Spawns vanilla faction AI physically at the base and counts living agents inside the real capture radius.
6. Emits PASS only after the base's actual faction becomes OPFOR.
7. Emits FAIL after a bounded 120-second phase timeout if vanilla ownership does not transition.

All harness messages use the `[TB_PVE_M1_1]` prefix.

## Observed runtime trace

Source log:

`C:\Users\ruanb\Documents\My Games\ArmaReforger\logs\logs_2026-09-21_07-12-19\console.log`

Relevant lines:

```text
07:12:38.698 [TB_PVE_M1_1] OnGameStart server=1 dedicated=0 proxy=0
07:12:41.108 [TB_PVE_M1_1] OPFOR key=USSR playable=1 playerLimit=0 canCaptureBases=1
07:12:41.108 [TB_PVE_M1_1] Target base=#AR-Campaign_MapLocation_Airbase callsign=34 owner=FIA radius=150 opforHQRadioTraffic=0 bootstrapWithVanillaBLUFORCapture=1
07:12:41.203 [TB_PVE_M1_1] AI group spawned phase=BLUFOR bootstrap ... faction=US unitSlots=3 characters=3 ...
07:12:42.107 [TB_PVE_M1_1] CaptureStateChanged state=1 server=1 dedicated=0 proxy=0
07:12:44.109 [TB_PVE_M1_1] Presence agents=3 livingInsideRadius=3 captureState=1 owner=FIA server=1
...
07:18:24.579 [TB_PVE_M1_1] Presence agents=3 livingInsideRadius=3 captureState=1 owner=FIA server=1
```

The unchanged state was observed continuously from `07:12:44` through `07:18:24` (about 5 minutes 40 seconds after the first presence sample).

## Evidence classification

### VERIFIED

- The addon and vanilla dependency loaded in a real Conflict world.
- The harness executed on authority (`server=1`, `proxy=0`).
- Setting the USSR player limit to zero did not change `CanCaptureBases()`, which logged true.
- A vanilla faction group prefab supplied three vanilla character slots.
- Three living AI agents physically existed inside the target's reported 150 m radius.
- AI presence caused the vanilla base capture state to change from `0` to `1`.
- The target retained FIA ownership for more than five minutes.
- No capture-start, capture-finish, or ownership-change event was observed.
- Workbench script validation succeeded after the final harness change.

### INFERRED

- Capture state `1` represents presence/contestation rather than an actively progressing ownership timer in this configuration. This is inferred from the state transition combined with the absence of `CaptureStart`, timer completion, and ownership change.
- Vanilla Conflict's `SCR_SeizingComponent` exposes a protected `m_bCapturingRequiresPlayer` setting. The observed stall is consistent with that gate, but the effective prefab value was not directly readable through the public runtime API, so it is not marked verified.
- Radio eligibility is not sufficient by itself to explain or solve the result. The selected base logged no OPFOR-to-HQ traffic, but the stalled phase used BLUFOR AI and was intended only to create the BLUFOR-owned target through vanilla mechanics.

### UNKNOWN

- Whether a preconfigured non-HQ BLUFOR target would allow player-disabled OPFOR AI to advance beyond capture state `1`.
- The exact serialized values of `m_bCapturingRequiresPlayer`, `m_bIgnoreNonPlayableAttackers`, and related seizing options on the current vanilla Conflict prefab.
- Dedicated-server behavior; the observed run was listen/offline authority, not a dedicated process.
- Client propagation of a completed AI-only ownership change, because no ownership change occurred.
- Radio-network changes after AI-only capture, because no ownership change occurred.

## Failure criteria observed

| Criterion | Observation | Result |
|---|---|---|
| AI physically enters/exists in capture radius | 3 agents alive inside 150 m | PASS |
| Vanilla capture reacts to AI | state `0 -> 1` | PASS |
| Vanilla capture timer starts | no `CaptureStart`; owner unchanged | FAIL |
| Base ownership becomes OPFOR | never reached OPFOR phase because vanilla AI bootstrap could not own the target | FAIL |
| Server owns transition | authority instrumentation worked, but no transition occurred | UNKNOWN |
| Client observes transition | no transition to observe | UNKNOWN |
| Radio/network updates | no transition to trigger update | UNKNOWN |

## Smallest next action

Create one tiny M1.1 test-world override that starts a normal non-HQ vanilla Conflict base as BLUFOR-owned, without changing `SCR_CampaignSeizingComponent`. Run the existing harness directly against that base so player-disabled USSR AI is the only force in its capture radius.

If that run again reaches state `1` without `CaptureStart`, inspect the inherited seizing-component prefab values in Workbench. The likely minimum product change would then be a prefab/config override that disables the player-required gate while retaining vanilla seizing, timing, ownership, replication, and radio code. That would be a deliberate behavior change and should be tested as M1.2, not retroactively counted as an M1.1 PASS.

---

## M1.1A — Controlled BLUFOR Base AI Capture Test

### Result

**FAIL — Case B.** A real, ordinary, initialized, non-HQ BLUFOR base was tested with exactly three living non-playable OPFOR AI at its centre. Vanilla presence detection changed the base from state `0` (`NONE`) to state `1` (`CONTESTED`), but never to state `2` (`CAPTURING`). Ownership remained `US`; no vanilla capture clock or ownership callback started.

This independently reproduces the original stall against a correctly BLUFOR-owned target. The earlier FIA bootstrap target was therefore not the cause.

### Test setup and difference from the original M1.1 run

- World: project-owned subscene `Worlds/TB_PVE_M1_1A_CaptureTest.ent`, parented to the vanilla `worlds/MP/CTI_Campaign_Eden.ent` Conflict world.
- Target: `TB_M1_1A_BLUFOR_Base`, an inherited instance of the vanilla `{1391CE8C0E255636}Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et` prefab.
- Target properties at runtime: initialized, owner `US`, `isHQ=0`, type `0` (`BASE`), base radius `150 m`, vanilla seizing radius `50 m`.
- OPFOR: the vanilla `USSR` campaign faction with player limit `0`, runtime `playable=0`, and `CanCaptureBases()=1`.
- Attackers: three characters from vanilla `{359353A0A5A52216}Prefabs/Groups/Campaign/Group_USSR_Defenders.et`, registered to an `SCR_AIGroup` and placed at the target centre.
- Difference from M1.1: no FIA target and no BLUFOR bootstrap. Scene configuration starts the target BLUFOR-owned; the harness performs no runtime ownership assignment.
- Authority: offline/listen authority (`server=1`, `dedicated=0`, `proxy=0`).
- Observation window: 120 seconds, sampled every 3 seconds.

Test resources:

- `Worlds/TB_PVE_M1_1A_CaptureTest.ent`
- `Worlds/TB_PVE_M1_1A_CaptureTest.ent.meta`
- `Worlds/TB_PVE_M1_1A_CaptureTest_Layers/M1_1A_BLUFOR_Base.layer`
- `Scripts/Game/TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c`

The harness only observes vanilla state, exposes two protected configuration booleans through read-only getters, and spawns test AI. It does not call `SetFaction`, `BeginCapture`, `EndCapture`, any ownership RPC, or any custom timer.

### Runtime evidence

Decisive full-run log:

`C:\Users\ruanb\Documents\My Games\ArmaReforger\logs\logs_2026-09-21_08-27-27\console.log`

Read-only gate-confirmation log:

`C:\Users\ruanb\Documents\My Games\ArmaReforger\logs\logs_2026-09-21_08-33-58\console.log`

```text
08:27:49.545 [TB_PVE_M1_1] OPFOR key=USSR playable=0 playerLimit=0 canCaptureBases=1
08:27:49.545 [TB_PVE_M1_1] M1.1A initial proof base=TB M1.1A BLUFOR Base entity=TB_M1_1A_BLUFOR_Base owner=US initialized=1 isHQ=0 type=0 baseRadius=150 seizingRadius=50
08:27:49.648 [TB_PVE_M1_1] AI group spawned phase=M1.1A OPFOR proof ... faction=USSR unitSlots=3 characters=3 ...
08:27:50.645 [TB_PVE_M1_1] CaptureStateChanged state=1 server=1 dedicated=0 proxy=0
08:27:52.550 [TB_PVE_M1_1] Observation tick=1 agents=3 livingInsideRadius=3 aiFaction=USSR captureState=1 owner=US contestingFactions=1 server=1
08:27:52.550 [TB_PVE_M1_1] CaptureClock ... start=0 end=0x0000000000000000 {}
...
08:29:50.002 [TB_PVE_M1_1] Observation tick=40 agents=3 livingInsideRadius=3 aiFaction=USSR captureState=1 owner=US contestingFactions=1 server=1
08:29:50.002 [TB_PVE_M1_1] FAIL M1.1A timeout initialOwner=US finalOwner=US captureState=1 agents=3 livingInsideRadius=3 captureStart=0 captureEnd=0x0000000000000000 {}; vanilla ownership did not transition

08:34:23.336 [TB_PVE_M1_1] M1.1A vanilla gates ignoreNonPlayableAttackers=1 capturingRequiresPlayer=1 opforHQRadioTraffic=0
```

Initial ownership was `US`; final ownership was `US`. Capture state progressed `0 → 1` and remained `1`. Capture timestamps remained unset throughout. `CaptureStart`, `CaptureInterrupt`, `CaptureFinish`, `SCR_MilitaryBaseSystem.GetOnBaseFactionChanged`, and `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended` produced no event.

### Source-level blocking path

Vanilla source was traced in Bohemia Interactive's `Arma-Reforger-Script-Diff`, specifically `SCR_SeizingComponent.c` and `SCR_CampaignSeizingComponent.c` under `scripts/Game/GameMode/Components/`.

`SCR_CampaignSeizingComponent.EvaluatePrevailingFaction()` can mark a base contested from enemy presence before selecting a prevailing attacker. Its relevant gates occur in this order:

1. `m_bIgnoreNonPlayableAttackers && !faction.IsPlayable()` — skip the faction.
2. `m_bCapturingRequiresPlayer && !factionsPlayerPresence.Get(faction)` — skip a faction with no player present.
3. Only after those gates pass are presence superiority and radio eligibility evaluated and the faction allowed to become prevailing/`CAPTURING`.

The effective target values were `m_bIgnoreNonPlayableAttackers=true` and `m_bCapturingRequiresPlayer=true`. The first exact blocker was therefore `m_bIgnoreNonPlayableAttackers`; even if it alone were disabled, `m_bCapturingRequiresPlayer` would independently reject this AI-only attacker. This accounts for `CONTESTED` with no prevailing capture clock.

Although `opforHQRadioTraffic=0`, radio is not the blocker for this non-playable faction: the vanilla condition accepts either a non-playable capturing faction or valid HQ radio traffic (`!cFaction.IsPlayable() || IsHQRadioTrafficPossible(...)`). Base links/radio reachability therefore do not explain this stall.

The narrowest extension point is the existing vanilla `SCR_CampaignSeizingComponent` configuration on an inherited project-owned test-base prefab: override only `m_bIgnoreNonPlayableAttackers=false` and `m_bCapturingRequiresPlayer=false`, then repeat this exact harness. This two-flag prefab experiment retains vanilla capture, timing, ownership, replication, and radio code. It is intentionally **not implemented in M1.1A**.

### Evidence classification

#### VERIFIED

- The target started and finished BLUFOR-owned (`US → US`), initialized, ordinary type `BASE`, and non-HQ.
- OPFOR was the real vanilla USSR faction, non-playable with player limit zero, while `CanCaptureBases()` remained true.
- Three living OPFOR AI remained inside the effective vanilla seizing radius for the complete 120-second observation.
- Vanilla detection registered one contesting faction and transitioned state `0 → 1`.
- Vanilla capture never entered state `2`; timestamps remained unset and ownership never changed.
- No capture lifecycle or ownership callback fired.
- The effective seizing configuration had both `ignoreNonPlayableAttackers=1` and `capturingRequiresPlayer=1`.
- Source order makes `m_bIgnoreNonPlayableAttackers` the first rejection and `m_bCapturingRequiresPlayer` an independent second blocker.
- The final diagnostic script compiled and ran in the game; the confirmation run reproduced state `1` with three living OPFOR AI.

#### INFERRED

- The original FIA result and this BLUFOR result share this attacker-selection cause because both stop at the same boundary. The controlled result strongly supports this, but the earlier target's flags were not logged directly.
- Disabling only the two flags should allow evaluation to reach later vanilla conditions. Completion of ownership transfer remains a hypothesis for the next experiment.

#### UNKNOWN

- Whether another downstream vanilla gate appears after those two flags are disabled.
- Whether a resulting ownership transfer would replicate correctly to dedicated-server clients; this test used listen authority only.
- Capture duration and downstream notifications under the hypothetical two-flag configuration, since M1.1A deliberately made no such behavior change.

### Verdict and next experiment

M1.1A is **FAIL / Case B**. AI presence is detected, but this ordinary vanilla base configuration explicitly excludes a non-playable, playerless attacker from becoming the prevailing capturing faction.

The smallest justified next experiment is a disposable inherited version of the same vanilla base with only the two confirmed seizing booleans disabled. Keep the same position, BLUFOR ownership, USSR faction, three AI, bounded observation, and callback logging. PASS only if vanilla logic produces `US → USSR`; otherwise trace the next reached vanilla gate. Do not add custom ownership or duplicate capture logic.

---

## M1.1B — AI Capture Gate Override Proof

### Result

**PASS.** With only the two verified AI-exclusion flags disabled on a project-owned inherited base prefab, three non-playable USSR AI completed a genuine vanilla `US → USSR` ownership transition. Vanilla calculated and completed its own timer, invoked its capture lifecycle event, changed the affiliated faction, and emitted both observed ownership callbacks. No custom timer or ownership assignment was used.

### Inherited experimental resource

`Prefabs/Systems/MilitaryBase/TB_PVE_M1_1B_AICaptureBase.et` inherits directly from:

`{1391CE8C0E255636}Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et`

Its only component-property overrides are:

```text
SCR_CampaignSeizingComponent "{5C66967235FBEEA3}" {
 m_bIgnoreNonPlayableAttackers 0
 m_bCapturingRequiresPlayer 0
}
```

No radius, timing, ownership, faction, replication, radio, supply, notification, or other capture property is overridden in the inherited prefab. The controlled test layer now instantiates this experimental resource as `TB_M1_1B_BLUFOR_Base`; the existing scene-level initial-US ownership and descriptive test-base properties remain unchanged.

The resource database was rebuilt with the project and base-game addon directories, and the final scripts passed Workbench validation for all configurations.

### Controlled runtime setup

- World: `Worlds/TB_PVE_M1_1A_CaptureTest.ent`, reused from M1.1A.
- Target: initialized, ordinary type `0` (`BASE`), non-HQ `TB_M1_1B_BLUFOR_Base`.
- Initial owner: `US`.
- Base radius: `150 m`; effective vanilla seizing radius: `50 m`.
- OPFOR: vanilla USSR campaign faction, `playable=0`, `playerLimit=0`, `CanCaptureBases()=1`.
- Attackers: three living vanilla USSR defender-group characters at the target centre.
- Defenders inside the seizing area: none.
- Authority: local-server authority (`server=1`, `dedicated=0`, `proxy=0`).
- Launch form: `ArmaReforgerSteamDiag.exe -server Worlds/TB_PVE_M1_1A_CaptureTest.ent`, with the base-game/project addon directories and addon GUID `6A6A5973FA53A5B1`.

An earlier offline-world attempt was discarded as a completion proof after added timing instrumentation showed its server timestamp frozen at `100`. It nevertheless reached `CAPTURING`; the decisive run used local-server mode, where the timestamp advanced normally.

### Primary capture evidence

Decisive log:

`C:\Users\ruanb\Documents\My Games\ArmaReforger\logs\logs_2026-09-21_08-59-19\console.log`

```text
08:59:43.341 [TB_PVE_M1_1] OPFOR key=USSR playable=0 playerLimit=0 canCaptureBases=1
08:59:43.341 [TB_PVE_M1_1] M1.1B initial proof base=TB M1.1B BLUFOR Base entity=TB_M1_1B_BLUFOR_Base owner=US initialized=1 isHQ=0 type=0 baseRadius=150 seizingRadius=50
08:59:43.341 [TB_PVE_M1_1] M1.1B vanilla gates ignoreNonPlayableAttackers=0 capturingRequiresPlayer=0 opforHQRadioTraffic=0
08:59:43.341 [TB_PVE_M1_1] M1.1B vanilla timing maximumSeconds=60 minimumSeconds=5 maximumCharacters=12
08:59:43.423 [TB_PVE_M1_1] AI group spawned phase=M1.1B OPFOR proof ... faction=USSR unitSlots=3 characters=3 ...
08:59:44.341 [TB_PVE_M1_1] CaptureStateChanged state=2 server=1 dedicated=0 proxy=0
08:59:44.341 [TB_PVE_M1_1] CaptureStart faction=USSR server=1 dedicated=0 proxy=0
08:59:46.325 [TB_PVE_M1_1] Observation tick=1 agents=3 livingInsideRadius=3 aiFaction=USSR captureState=2 owner=US contestingFactions=1 server=1
08:59:46.325 [TB_PVE_M1_1] CaptureClock ... now=4685 start=2703 end=0x000000000000CDDF {}
09:00:13.358 [TB_PVE_M1_1] CaptureClock ... now=31720 start=2703 end=0x000000000000CDDF {}
09:00:34.358 [TB_PVE_M1_1] MilitaryBaseSystem.OnBaseFactionChanged newOwner=USSR server=1 dedicated=0 proxy=0
09:00:34.358 [TB_PVE_M1_1] CampaignMilitaryBase.OnFactionChangedExtended previous=US new=USSR server=1 dedicated=0 proxy=0
09:00:34.358 [TB_PVE_M1_1] CaptureFinish faction=USSR server=1 dedicated=0 proxy=0
09:00:34.408 [TB_PVE_M1_1] CaptureStateChanged state=0 server=1 dedicated=0 proxy=0
09:00:34.408 [TB_PVE_M1_1] Observation tick=17 agents=3 livingInsideRadius=3 aiFaction=USSR captureState=0 owner=USSR contestingFactions=1 server=1
09:00:34.408 [TB_PVE_M1_1] PASS M1.1B vanilla base ownership transitioned initialOwner=US finalOwner=USSR ...
```

The timer was entirely vanilla. With inherited settings `maximumSeconds=60`, `minimumSeconds=5`, `maximumCharacters=12`, vanilla calculated a 50-second capture for three effective attackers: start timestamp `2703`, end timestamp `52703` (`0xCDDF`). Ownership callbacks and `CaptureFinish` occurred when server time reached that endpoint.

The emitted state transition was `NONE (0) → CAPTURING (2) → NONE (0)`, not a separately observable `CONTESTED (1)` step. This is valid vanilla behavior for an uncontested superior attacker: `CONTESTED` was the terminal state in M1.1/M1.1A only because no prevailing faction passed the two gates. M1.1B demonstrably progressed beyond that boundary into active capture and ownership completion.

### Callback and authority observations

- `CaptureStart`: observed for USSR on authority.
- `CaptureInterrupt`: not emitted in the successful primary run, as expected.
- `CaptureFinish`: observed for USSR on authority.
- `SCR_MilitaryBaseSystem.GetOnBaseFactionChanged`: observed with new owner USSR.
- `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended`: observed with `previous=US`, `new=USSR`.
- Final owner: USSR.
- All observed callbacks reported `server=1`, `dedicated=0`, `proxy=0`.
- The callback order observed at completion was MilitaryBaseSystem ownership change, extended faction change, then CaptureFinish, followed by capture state returning to NONE.

### Radio behavior

`opforHQRadioTraffic=0` remained unchanged. Capture nevertheless started and completed because vanilla `SCR_CampaignSeizingComponent` permits a non-playable prevailing faction to bypass the radio-traffic branch. M1.1B did not alter radio configuration or implement the future campaign/frontline eligibility restriction.

### Interruption regression

A separate run enabled the harness's optional diagnostic switch, waited until vanilla state `CAPTURING`, and moved all three USSR characters from the base centre to 150 m away—100 m beyond the effective 50 m seizing radius.

Regression log:

`C:\Users\ruanb\Documents\My Games\ArmaReforger\logs\logs_2026-09-21_09-02-22\console.log`

```text
09:02:51.920 [TB_PVE_M1_1] CaptureStateChanged state=2 server=1 dedicated=0 proxy=0
09:02:51.920 [TB_PVE_M1_1] CaptureStart faction=USSR server=1 dedicated=0 proxy=0
09:02:59.769 [TB_PVE_M1_1] M1.1B interruption regression movedCharacters=3 outsidePosition=<4830,28.281,11947.7> seizingRadius=50
09:03:00.753 [TB_PVE_M1_1] CaptureStateChanged state=0 server=1 dedicated=0 proxy=0
09:03:00.753 [TB_PVE_M1_1] CaptureInterrupt faction=USSR server=1 dedicated=0 proxy=0
09:03:00.753 [TB_PVE_M1_1] PASS M1.1B interruption regression vanilla capture interrupted after AI left the seizing radius
```

The regression therefore confirms that the vanilla active-capture interruption path remains intact. The optional regression switch is disabled in the final harness so its default behavior remains the primary ownership proof.

### Acceptance criteria

| Criterion | Evidence | Result |
|---|---|---|
| A. Starts BLUFOR/US-owned | Initial runtime owner `US` | PASS |
| B. OPFOR non-human-playable | `playable=0`, `playerLimit=0` | PASS |
| C. OPFOR may capture bases | `CanCaptureBases()=1` | PASS |
| D. Only confirmed gates disabled | Runtime `ignoreNonPlayableAttackers=0`, `capturingRequiresPlayer=0`; inherited prefab contains only those overrides | PASS |
| E. Living USSR AI in vanilla area | 3 agents, 3 living inside 50 m | PASS |
| F. Vanilla enters active capture | state `2`, `CaptureStart`, advancing timestamps | PASS |
| G. Vanilla capture completes | timer endpoint reached; `CaptureFinish` | PASS |
| H. Ownership changes US → USSR | both ownership callbacks plus final owner USSR | PASS |
| I. No custom ownership assignment | harness contains no faction assignment, ownership RPC, or custom timer | PASS |
| J. Vanilla callbacks observed | capture-start/finish and both ownership callbacks observed | PASS |

**Overall M1.1B: PASS.**

### Evidence classification

#### VERIFIED

- The experimental prefab inherits the vanilla Conflict base and overrides only the two named seizing booleans.
- Runtime values for both booleans were false.
- OPFOR remained non-playable, playerless, and allowed to capture bases.
- Three living USSR AI occupied the effective vanilla seizing radius.
- Vanilla entered `CAPTURING`, ran its computed 50-second timer, completed, and changed ownership `US → USSR`.
- `CaptureStart`, `CaptureFinish`, MilitaryBaseSystem ownership, and extended previous/new faction callbacks fired on authority.
- Vanilla interruption still occurred when all attackers left during active capture.
- Radio traffic remained unavailable and was not required for the non-playable attacker.
- Final Workbench script validation succeeded.

#### INFERRED

- The two gates identified in M1.1A were jointly sufficient to explain that test's failure because changing only those values allowed the same controlled scenario to complete. This is a causal inference supported by the one-resource/two-property controlled change.

#### UNKNOWN

- Remote-client replication and client-side callback observation; no separate client joined this local-server proof.
- Dedicated executable behavior (`RplMode.Dedicated`); authority was verified, but this run reported `dedicated=0`.
- Production rollout strategy across campaign bases, deliberately deferred by the milestone.
- Future frontline/linear eligibility policy and how it will constrain the currently preserved non-playable radio bypass.

### Recommended next milestone

Treat the inherited prefab as an experimental proof asset only; do not apply it globally yet. The next milestone should decide the narrow production delivery mechanism for PvE-compatible base configuration and test it on the intended campaign topology, including a dedicated server plus remote client. Campaign/frontline eligibility should remain a separate server-authoritative layer rather than being folded into or replacing vanilla seizing logic.
