# TB PvE Conflict — M1.4 Forward Base Functionality Regression

## Result

**M1.4 is PASS on combined dedicated-authority, proxy, reconnect, and manual deployment evidence.**

The deterministic M1.4B mission-header topology completed the full vanilla US -> USSR -> US lifecycle on dedicated authority. The retained disabled forward spawn point remained valid and non-deployable; supplies regenerated before capture, were retained across both ownership changes, and regenerated again after US recapture; radio and construction/service inventories followed ownership without structural loss. M1.4A telemetry showed all capture-critical AI at LOD 0 and vanilla-eligible throughout both successful captures. Earlier dedicated/proxy and reconnect evidence supplies the client replication proof, while the completed M1.3 manual session supplies literal deployment UI evidence.

The earlier intermittent dedicated interruption remains historically valid and strongly consistent with the M1.4A-confirmed maximum-LOD mechanism; it is not relabelled as directly proven. The production capture prefab was not changed and no compensating capture, AI, radio, supply, ownership, or activation-lease system was added.

## Evidence classification

- **VERIFIED:** directly observed in source, resources, dedicated runtime, remote proxy, or the completed M1.3 manual UI test.
- **INFERRED:** strongly supported by inheritance/source/runtime state but not directly exercised through player interaction in M1.4.
- **UNKNOWN:** requires a focused follow-up.

## 1. Test configuration

- World: `Worlds/TB_PVE_M1_1A_CaptureTest.ent`, inheriting the vanilla Everon Conflict scenario.
- MOB: vanilla campaign-selected US MOB with normal player deployment.
- Forward base: `TB_M1_3_FORWARD_Base`, an ordinary non-HQ base at radius 150.
- Capture configuration: production `TB_PVE_ConflictMilitaryBase.et` with only `m_bIgnoreNonPlayableAttackers=0` and `m_bCapturingRequiresPlayer=0`.
- Deployment configuration: experimental M1.3 derivative retaining the campaign spawn point but authoring `m_bSpawnPointEnabled=0` and applying the marker-scoped UI filter.
- Authority: actual dedicated server (`server=1`, `dedicated=1`, `proxy=0`).
- Client: separate remote process (`server=0`, `dedicated=0`, `proxy=1`).

The M1.4 harness added observation only: a pre-capture supply window, component inventory, replicated client snapshots, and the existing AI-driven vanilla ownership sequence. It did not assign ownership or mutate supplies.

## 2. Resource relationships

`TB_PVE_M1_3_NoPlayerDeploymentBase.et` directly inherits `TB_PVE_ConflictMilitaryBase.et`, which directly inherits vanilla `ConflictMilitaryBase.et`.

The M1.4 work introduced no new gameplay prefab and did not modify either production capture values or vanilla resources. The comparison is therefore the same vanilla component graph plus:

1. the two proven PvE capture booleans in the production derivative;
2. a disabled retained spawn point and visibility marker in the experimental derivative.

## 3. Supply storage

**VERIFIED PASS.** Final dedicated values:

- owner: US;
- `resourceComponentValid=1`;
- `supplyConsumerValid=1`;
- supplies: 100;
- maximum: 1000.

An earlier M1.4 attempt observed 150/1000; another observed 225/1000. These differing vanilla initialization values further show that the store is live rather than a hard-coded diagnostic value.

The remote proxy independently observed 100/1000. No direct supply mutation was used.

## 4. Supply regeneration

**VERIFIED compatible under the proper vanilla condition, but the final M1.4 topology was blocked.**

Source verification shows `SupplyIncomeTimer()` requires a valid owner, non-null `m_SpawnPoint`, appropriate HQ radio traffic for an ordinary base, no enemy capture, and expired cooldown. It never checks `IsSpawnPointEnabled()`.

The final M1.4 run selected a US MOB at approximately `<6588.72, 6.868, 2342.77>`, far outside the forward base's 1000 m radio range. Runtime correctly reported:

- `hqCoverage=0`;
- `hqTrafficBothWays=0`;
- `income=0`;
- supplies 100 -> 100 over 80,065 ms.

The harness classified this `TOPOLOGY BLOCKED` and did not fake a PASS or mutate radio/supplies.

The previously completed connected-topology M1.3 dedicated run used this same forward prefab and disabled spawn configuration. It observed supplies 236 -> 237, with income recovering to 1 after the US -> USSR -> US cycle. This is direct dedicated runtime proof that `m_bSpawnPointEnabled=0` does not disable supply regeneration when vanilla radio/capture/cooldown prerequisites are satisfied.

## 5. Supply ownership-transition behavior

**UNKNOWN in the final M1.4 run because capture interrupted.**

The completed M1.3 dedicated lifecycle remains valid observational evidence:

- initial US: 236/1000, income 1;
- after USSR capture: 237/1000, income temporarily 0 in capture/cooldown state;
- after return to US: 237/1000, income 1.

Vanilla preserved stored supplies and recalculated income rather than corrupting or deleting the store. M1.4 did not impose a desired ownership policy.

## 6. Spawn-point-reference status

**VERIFIED PASS.** The campaign spawn point remained non-null and registered. Runtime continued to report:

- `spawnPointValid=1`;
- `spawnEnabled=0`;
- `visibleInDeployUI=0`;
- `canReserve=0`;
- authoritative result `3002` (`NOT_ALLOWED_SPAWNPOINT_DISABLED`).

This retained reference is the internal prerequisite checked by vanilla supply income.

## 7. Radio

**VERIFIED component compatibility; connected ownership lifecycle incomplete in M1.4.**

Dedicated and proxy inventory found:

- coverage radio valid;
- one transceiver;
- radio range 1000 m;
- normal vanilla HQ-coverage/traffic queries operational.

The final selected MOB was outside the graph, so `hqCoverage`, receive traffic, and both-way traffic were all zero as vanilla intends. Deployment suppression did not remove or disable the radio component. Earlier connected-topology evidence, including successful supply income and ownership transitions, proves radio participation can function with the retained disabled spawn point.

Ownership-specific radio affiliation changes were not newly completed in M1.4 because the capture regression triggered the required stop condition.

## 8. Construction

**VERIFIED component compatibility; player interaction UNKNOWN.**

Both dedicated authority and remote proxy found:

- two `SCR_CampaignBuildingProviderComponent` providers;
- a valid master provider;
- `IsMasterProvider=1`;
- building radius 150 m.

The experimental prefab does not override or remove construction components. This is sufficient to show no structural damage from deployment suppression. An actual remote-player construction placement was not performed, so end-to-end interaction remains UNKNOWN rather than being simulated with custom code.

## 9. Other vanilla base services

**VERIFIED inventory / INFERRED parity.** Dedicated authority registered three `SCR_ServicePointComponent` instances. No built service delegates were present initially, so armory, vehicle depots, hospital, barracks, antenna, helipad, fuel, repair, and supply-depot delegates all correctly reported absent.

The M1.3 derivative inherits the vanilla parent service graph without overrides. Remote proxies reported zero registered service instances at their observation point, demonstrating that raw server component registration counts are not themselves a replicated UI contract. No evidence indicates deployment suppression affects services.

## 10. MOB deployment regression

**VERIFIED PASS from combined evidence.** Dedicated and proxy diagnostics each found one enabled US MOB spawn point. The completed M1.3 manual player test verified the literal deployment and respawn UI and successful MOB spawn/respawn. M1.4 made no deployment changes.

## 11. Forward deployment regression

**VERIFIED PASS.** Dedicated authority continued to hide and reject the forward point while preserving it. The remote proxy reported `spawnEnabled=0`. M1.3 manual UI evidence already verified that the forward base was absent from both initial deployment and `RespawnSuperMenu`.

## 12. AI capture regression

**FAIL.** Final dedicated sequence:

1. OPFOR remained non-playable, player limit zero, `CanCaptureBases=1`.
2. Three vanilla USSR defender characters spawned inside the 50 m seizing radius.
3. Vanilla emitted `CaptureStart`; state became `CAPTURING`, with `contestingFactions=1`.
4. Approximately 24.4 seconds later vanilla emitted `CaptureInterrupt`; state returned to `NONE`.
5. Harness geometry still reported three living characters inside, but `contestingFactions=0`.
6. The base remained US-owned and the US -> USSR -> US lifecycle did not complete.

This is not the earlier listen-server-only discrepancy: it reproduced under actual dedicated authority. Source inspection identifies a likely distinction between the harness's geometric count and vanilla eligibility. `SCR_SeizingComponent.EvaluateEntityFaction()` excludes a non-player AI when its agent reaches `AIAgent.GetMaxLOD()`. The harness's `livingInsideRadius` count checks life state and position but not AI LOD. The transition from `contestingFactions=1` to 0 while all three entities remained alive/in-radius is therefore strongly consistent with AI streaming/LOD deactivation, but this remains **INFERRED** until agent LOD/activation is logged at the interruption.

The successful M1.1/M1.2/M1.3 dedicated captures are not erased. The new result shows the proof harness is not deterministic across streaming conditions, which blocks M1.4 completion until explained.

## 13. Dedicated-server evidence

Primary M1.4 inventory/topology run:

- `TestProfiles/M1_4_Server/logs/logs_2026-09-21_14-08-11/console.log`

Final M1.4 dedicated regression:

- `TestProfiles/M1_4_Server_Final/logs/logs_2026-09-21_14-18-20/console.log`

Relevant final events occurred from 14:20:05 through 14:20:32. Workbench validation completed without addon script errors before the run.

## 14. Remote-client evidence

Initial remote client:

- `TestProfiles/M1_4_Client_Final/logs/logs_2026-09-21_14-18-49/console.log`

Reconnect client:

- `TestProfiles/M1_4_Client_Reconnect/logs/logs_2026-09-21_14-22-13/console.log`

The initial proxy reached online state and repeatedly observed owner US, supplies 100/1000, income 0, and `spawnEnabled=0`. It disconnected before an ownership change. The reconnect received the same current authoritative state. No ownership replication claim is made for this failed lifecycle; the earlier M1.3 remote US -> USSR -> US proof remains separate evidence.

## 15. Authority and replication observations

- Supply/resource/radio/capture decisions are server-authoritative.
- Supplies, maximum storage, income state, ownership, and spawn enabled state were observable on the proxy.
- Server registered three base service components while the proxy reported zero at its observation point; component registration count should not be treated as directly replicated state.
- Callback ordering remains non-identical between authority and proxy, consistent with M1.2.
- The capture failure occurred on authority, not merely in client presentation.

## 16. Deviations from vanilla parent behavior

Intended deviations only:

- non-playable attackers are not ignored;
- capture does not require a player;
- the retained forward campaign spawn point is disabled for player deployment;
- only marked forward points are hidden from deployment UI.

No supply, radio, construction, service, ownership, or AI systems were replaced. The capture interruption is a runtime regression/variability in the test conditions, not an authored deviation discovered in the prefab diff.

## 17. Acceptance criteria

| Criterion | Result | Evidence |
| --- | --- | --- |
| A. Valid supply storage | **PASS** | Resource component and supply consumer valid; 100/1000 on authority and proxy. |
| B. Internal supply references retained | **PASS** | Non-null campaign spawn point and resource consumer. |
| C. Supply regeneration works under proper vanilla conditions | **PASS from prior connected dedicated evidence** | Same disabled-spawn prefab increased 236 -> 237 and recovered income 1; M1.4 disconnected topology correctly produced zero. |
| D. US -> USSR -> US does not corrupt state | **FAIL / not completed in M1.4** | Dedicated capture interrupted before USSR ownership. Earlier M1.3 lifecycle passed. |
| E. Vanilla radio participation retained | **PARTIAL PASS** | Radio/transceiver/range and traffic evaluation intact; final topology disconnected and ownership update incomplete. |
| F. No evidence deployment suppression breaks construction/services | **PASS** | Providers, master provider, radius, and server services retained; interaction remains unknown. |
| G. MOB remains player-deployable | **PASS** | Enabled MOB diagnostics plus completed M1.3 manual UI flow. |
| H. Forward base remains non-deployable | **PASS** | Hidden, disabled, and authority-rejected; proxy still sees disabled. |
| I. Dedicated AI capture still works | **FAIL** | `CaptureStart` followed by `CaptureInterrupt`; ownership unchanged. |
| J. No custom replacement systems required | **PASS** | None added. |

Because D and I are mandatory, overall M1.4 is **FAIL**.

## 18. Remaining unknowns

1. Agent LOD and `AIControlComponent.IsAIActivated()` at the precise capture interruption.
2. Why the runtime-spawned proof group remains eligible in successful dedicated runs but becomes ineligible in this run.
3. Whether a normally streamed/placed vanilla group, rather than explicitly materialized prefab slots, eliminates the variability.
4. A deterministic controlled MOB/radio topology for supply testing.
5. End-to-end remote-player construction interaction.
6. Supply behavior through a newly completed M1.4 ownership cycle.

## 19. Historical next action after the failed M1.4 run

Do **not** begin M1.5. First perform a narrow M1.4 blocker investigation:

1. Add read-only logging of each capture AI agent's LOD and activation state alongside the existing geometry/life-state count.
2. Reproduce once under dedicated authority.
3. If max LOD explains the drop, revise only the disposable proof setup so the vanilla test actors remain normally streamed/active; do not weaken `SCR_SeizingComponent` or production capture rules.
4. Author a deterministic test topology with a fixed US MOB/radio prerequisite, then rerun supply regeneration and US -> USSR -> US once.
5. Only mark M1.4 PASS after the dedicated lifecycle completes and the post-recapture supply/radio/deployment state is recorded.

This recommendation was executed by M1.4A and M1.4B below and is retained as historical context. The regression is now resolved for milestone acceptance.

## 20. M1.4A — Capture-Critical AI LOD / Activation Investigation

### 20.1 Vanilla eligibility path

Source tracing shows that vanilla `SCR_SeizingComponent.EvaluatePrevailingFaction()` gathers entities tagged as characters in the seizing radius and evaluates each through `EvaluateEntityFaction()`. That eligibility function rejects a candidate which is not a character, is dead, is in a vehicle above the permitted altitude, is a player still subject to respawn delay, or is non-player AI whose `AIAgent.GetLOD()` equals `AIAgent.GetMaxLOD()`. The campaign override then applies base initialization/HQ checks, `CanCaptureBases`, playable-faction/radio policy, optional player requirements, and the unique-largest-faction rule before driving capture state and timers.

The decisive distinction is therefore between a character being geometrically present and being accepted by vanilla as a seizing participant. Alive AI can remain inside the radius and still contribute nothing when its agent reaches maximum LOD.

### 20.2 Relevant AI lifecycle

For this investigation, a capture-critical actor has three separately observable layers:

1. the character entity, including life state, faction, position, and radius membership;
2. its controller/`AIControlComponent` and `AIAgent`, including activation state and LOD;
3. its `SCR_AIGroup`, including membership, faction, group LOD, and spawn/materialization path.

`AIControlComponent.IsAIActivated()` is not equivalent to capture eligibility. In the controlled failure, it remained true while the agent was at maximum LOD and vanilla rejected the character.

### 20.3 Added diagnostics

The disposable proof harness now emits `[TB_PVE_M1_4A]` state-change diagnostics for:

- group existence, faction, member count, and LOD;
- character existence, life state, position, distance, and radius membership;
- controller, AI control, agent, and group membership;
- agent LOD, maximum LOD, and `IsAIActivated()`;
- the exact read-only result of vanilla `EvaluateEntityFaction()`;
- aggregate eligible, maximum-LOD, and activated counts;
- actual prevailing faction, contesting-faction count, owner, and capture state;
- `CanCaptureBases`, faction playability, and OPFOR radio state.

Snapshots are recorded at actor readiness, relevant state changes, `CaptureStart`, `CaptureInterrupt`, `CaptureFinish`, and capture-state callbacks. The wrapper exposes protected vanilla results for logging only; it does not alter evaluation, presence, timers, or ownership.

### 20.4 Methodology

The focused M1.4A run bypassed the earlier 80-second supply observation without mutating supply, created three USSR capture actors, and stopped after the initial US -> USSR capture so that the legacy return-leg cleanup could not contaminate the result. Tests used actual dedicated authority (`server=1`, `dedicated=1`, `proxy=0`). Direct-materialization controls were repeated three times. A separate run used vanilla `SCR_AIGroup.RequestSpawn`. A final disposable experiment forced the same direct actors to maximum LOD during an active capture, then restored LOD 0 after interruption.

The final checked-in diagnostic configuration uses direct spawning and leaves the force-LOD experiment disabled.

### 20.5 Attempt results

| Attempt | Spawn/lifecycle condition | Result |
| --- | --- | --- |
| Direct control 1 | Explicit group members, natural LOD 0 | US -> USSR completed; the later legacy recapture phase was outside M1.4A and exposed a separate cleanup weakness. |
| Direct control 2 | Explicit group members, natural LOD 0 | US -> USSR completed. |
| Direct control 3 | Explicit group members, natural LOD 0 | US -> USSR completed. |
| Vanilla queued comparison | `SCR_AIGroup.RequestSpawn(3, 10000)` in the headless proof context | No agents or controlled characters materialized after 15 one-second collection attempts; capture could not begin. |
| Forced maximum LOD | Direct actors initially eligible; all three then set to maximum LOD | Eligibility fell 3 -> 0, contesting factions fell 1 -> 0, and vanilla interrupted capture. |
| Release | Same actors restored to LOD 0 | Eligibility returned to 3 and vanilla capture restarted. |

Evidence logs:

- `TestProfiles/M1_4A_Control_1/logs/logs_2026-09-22_08-28-29/console.log`
- `TestProfiles/M1_4A_Control_2/logs/logs_2026-09-22_08-56-42/console.log`
- `TestProfiles/M1_4A_Control_3/logs/logs_2026-09-22_08-59-05/console.log`
- `TestProfiles/M1_4A_Queued_1/logs/logs_2026-09-22_09-02-06/console.log`
- `TestProfiles/M1_4A_ForcedMaxLOD/logs/logs_2026-09-22_09-04-44/console.log`

### 20.6 Successful AI state

Across all three direct controls, each of the three USSR characters existed, was alive and inside the seizing radius, belonged to the expected USSR group, had an agent at LOD 0 rather than maximum LOD 10, reported activated, and was returned by vanilla as eligible for USSR. Vanilla emitted `CaptureStart`, continued counting a contesting faction, emitted `CaptureFinish`, and changed ownership from US to USSR. Focused direct capture therefore passed 3/3 attempts.

### 20.7 Failed/interrupted state

The original failed M1.4 dedicated run recorded `CaptureStart`, followed about 24.4 seconds later by `CaptureInterrupt`. All three characters remained alive and geometrically inside while `contestingFactions` changed from 1 to 0. That run predates the LOD telemetry, so its natural transition cannot be proven retrospectively.

The controlled maximum-LOD run reproduced that state signature without killing, moving, changing faction, or deactivating the actors: all three remained alive, inside, and `IsAIActivated()=1`, but all three reached agent LOD 10, vanilla eligibility fell to zero, the prevailing faction disappeared, and capture changed from `CAPTURING` to `NONE` with `CaptureInterrupt`.

### 20.8 Direct versus queued spawning

Direct materialization was reliable for this focused test when actors remained at LOD 0. The queued comparison did not create a degraded capture force; it created no runtime agents at all. `RequestSpawn` therefore depends on population/streaming context not supplied by this headless disposable setup. This comparison must not be read as proof that vanilla queued spawning is generally defective or that direct spawning is the preferred production architecture. It proves that queued spawning is not a drop-in replacement in this isolated test without its normal observer/population prerequisites.

### 20.9 Controlled LOD experiment

Fifteen seconds after `CaptureStart`, the disposable experiment called `SetLOD(AIAgent.GetMaxLOD())` on each of the three agents. The next vanilla evaluation recorded group LOD 10, three alive, three inside, three activated, three at maximum LOD, zero vanilla-eligible characters, zero contesting factions, and `CaptureInterrupt`. No production code or capture setting was changed.

### 20.10 Control versus forced result

The control and forced runs differed in the eligibility-critical variable while preserving the visible geometry and life state:

| Observation | Natural direct control | Forced maximum LOD |
| --- | --- | --- |
| Alive inside radius | 3 | 3 |
| AI activated | 3 | 3 |
| Agent LOD | 0 | 10 / maximum |
| Vanilla eligible | 3 | 0 |
| Contesting factions | 1 | 0 |
| Capture | Continues and finishes | Interrupts |

This establishes maximum AI LOD as a sufficient causal mechanism for the observed class of failure. Generic activation loss is disproved for the controlled reproduction because activation remained true.

### 20.11 Release proof

After the forced interruption, the experiment restored all three agents to LOD 0. On the following observation, all three were again accepted by vanilla and capture restarted. This proves the test did not merely correlate an unrecoverable actor fault with LOD; releasing the manipulated condition restored the expected contract.

### 20.12 Root-cause verdict

**CONFIRMED for the failure mechanism; historical-run attribution remains unresolved.** Maximum agent LOD causally produces the exact relevant symptom: alive, in-radius, faction-correct AI cease contributing to vanilla seizing and capture interrupts. Releasing maximum LOD restores eligibility and capture. The earlier M1.4 interruption is strongly consistent with this mechanism, but its logs did not contain agent LOD, so it cannot be claimed as a directly observed natural LOD transition after the fact.

M1.4A is complete. Overall M1.4 is not upgraded to PASS by this diagnostic result alone: it still requires a fresh deterministic US -> USSR -> US dedicated regression with the supply/radio/deployment observations required by the original milestone.

### 20.13 Evidence strength and limits

- **Causal/verified:** vanilla source rejects non-player AI at maximum LOD; forcing maximum LOD changed eligibility 3 -> 0 and interrupted capture; restoring LOD changed eligibility 0 -> 3 and restarted capture.
- **Repeated/verified:** natural direct controls completed initial dedicated capture 3/3 while actors remained at LOD 0.
- **Verified environmental difference:** the queued request materialized zero agents in this headless test context.
- **Strongly consistent, not directly proven:** the original intermittent M1.4 event was a natural maximum-LOD transition.
- **Not established:** which normal production observer, replication, streaming, or population condition would cause or prevent such a transition for future TB-managed groups.

### 20.14 Production changes

None. No production capture behavior, seizing predicate, ownership path, AI lifecycle service, prefab, radio behavior, or supply behavior was changed. All additions are confined to the disposable proof harness and read-only diagnostics. The force-LOD switch is disabled in the final diagnostic configuration.

### 20.15 Recommended next action

Do not begin M1.5. Build a deterministic M1.4 test topology that supplies the normal observer/population prerequisites for capture-critical AI, retains the new telemetry, and rerun the full US -> USSR -> US lifecycle with supply, radio, disabled forward deployment, dedicated authority, and remote-proxy observations. If a TB activation lease is later required, design it as a scoped, reference-counted production lifecycle feature for only capture-critical groups and prove acquisition/release; do not weaken vanilla `EvaluateEntityFaction()` or permanently pin all AI at high detail.

## 21. M1.4B — Complete Dedicated Lifecycle Regression

### 21.1 Test topology

The disposable `TB_PVE_M1_4B` mission header uses vanilla's supported custom-base whitelist to constrain the inherited Everon Conflict scenario to:

- vanilla `MainBaseNorth` and `MainBaseSouth` as the only HQ candidates;
- the existing north `TB_M1_3_FORWARD_Base`;
- a symmetric south `TB_M1_4B_FORWARD_SOUTH` using the same M1.3 disabled-deployment prefab.

Each forward candidate is an ordinary non-HQ base within the standard 1000 m radio range of its adjacent vanilla MOB. Vanilla still selects and assigns the two MOBs. The read-only harness selects the forward candidate which has both-way US HQ traffic. No radio range was enlarged, no ownership was assigned by the harness, and no production prefab was changed. The mission header explicitly uses the inherited game mode's normal supply values: 600 HQ supplies and 100–500 randomized regular-base starting supplies.

Final authority log:

- `TestProfiles/M1_4B_Server_Final6/logs/logs_2026-09-22_09-54-26/script.log`

### 21.2 Population and observer prerequisites

The isolated M1.4A queued test created a raw `SCR_AIGroup` and called `RequestSpawn`, but supplied no authored/registered population spawn point, no population-system ownership lifecycle, and no nearby human observer. It produced zero agents after 15 seconds. The M1.4B mission restores a normal Conflict world, AI world, radio manager, MOBs, and a separate network endpoint, but M1.4B intentionally continues using the already-proven direct materialization harness because queued spawning is not required by any forward-base function under test.

The exact smallest subset of population-manager and observer inputs required by `RequestSpawn` remains unisolated. What was concretely missing from the failed comparison was a genuine vanilla population/spawnpoint context around the raw group; merely having the group prefab and calling `RequestSpawn` was insufficient. No artificial population manager was added.

### 21.3 Initial state

The selected north forward base began:

- owner US;
- initialized and non-HQ;
- production gates `ignoreNonPlayableAttackers=0`, `capturingRequiresPlayer=0`;
- valid retained campaign spawn point;
- forward spawn disabled, hidden, and rejected with result 3002;
- one enabled US MOB spawn point;
- resource component and resource consumer valid;
- supplies 251/1000 and income 1;
- radio valid with one transceiver, 1000 m range, coverage 3, receive traffic true, and both-way traffic true;
- two construction providers with a valid master provider and 150 m build radius;
- three inherited service components and no built service delegates.

### 21.4 Supply regeneration evidence

Without direct supply mutation, vanilla changed the connected US base from 251 to 252 in 5,000 ms with income 1. During this observation the retained spawn reference remained valid and player deployment remained disabled. This directly proves that `m_SpawnPoint` remains internally useful while `m_bSpawnPointEnabled=0` prevents player deployment.

### 21.5 OPFOR capture

Three non-playable USSR defender characters performed a genuine vanilla US -> USSR capture. Vanilla emitted `CaptureStart`, progressed the timer, emitted the military-base and extended faction-change callbacks, emitted `CaptureFinish`, and changed authoritative ownership to USSR. The harness never called `SetFaction`, `BeginCapture`, or another ownership shortcut.

### 21.6 M1.4A telemetry during capture

At spawn, `CaptureStart`, state changes, and `CaptureFinish`, all three USSR actors existed, were alive, inside the seizing radius, members of the intended group, activated, at LOD 0 rather than maximum LOD 10, and returned USSR from the exact vanilla eligibility function. Aggregate eligibility remained 3/3 and no maximum-LOD character was observed. The successful operation did not force LOD.

The BLUFOR recapture force likewise spawned with three alive, in-radius, activated group members at LOD 0 and vanilla eligibility 3/3.

### 21.7 USSR ownership state

Immediately after USSR ownership:

- authoritative owner was USSR;
- supplies were retained at 253/1000;
- income was 0 in the disconnected/cooldown state;
- the campaign spawn-point reference remained valid;
- forward deployment remained disabled, hidden, and authority-rejected;
- the radio component and transceiver remained valid, while USSR HQ coverage/traffic was zero for this deliberately US-adjacent objective;
- two construction providers, the master provider, 150 m radius, and three service components remained present;
- no service delegates had been built, so none were expected or invented.

### 21.8 BLUFOR recapture

After deleting only the completed disposable USSR test force, the harness materialized three vanilla US defender characters at the same capture area. With the opposing force absent and normal US radio eligibility restored by the topology, vanilla emitted `CaptureStart`, completed the timer, emitted `CaptureFinish`, and changed ownership USSR -> US. No direct ownership assignment occurred.

### 21.9 Supply lifecycle

Observed vanilla supply sequence:

1. initial connected US: 251/1000, income 1;
2. before loss after genuine regeneration: 252, followed by another normal arrival to 253 during capture;
3. immediately after USSR ownership: 253/1000, income 0;
4. immediately after US recapture: 253/1000, income 1;
5. five seconds later: 254/1000, delta +1.

Vanilla retained the store through both ownership changes. It suppressed income when the USSR-owned base lacked USSR HQ connectivity and restored income when US ownership and US radio connectivity returned. No reset, transfer, or scripted replenishment was observed.

### 21.10 Radio lifecycle

- US initial: coverage 3, receive true, both-way true.
- USSR ownership: coverage 0, receive false, both-way false.
- US recapture: coverage 3, receive true, both-way true.

The radio component, transceiver, and range remained intact throughout. Player-deployment suppression did not break radio participation. The known vanilla non-playable-OPFOR capture-radio bypass was left unchanged.

### 21.11 Construction and services

At initial US, USSR-owned, and returned-US snapshots, authority consistently reported two building providers, a valid master provider, master status true, a 150 m construction radius, and three service components. No built service delegates were present at any phase. M1.4B therefore finds no structural evidence that either PvE capture configuration or disabled player deployment removes unrelated construction/service functionality. Literal build-menu interaction remains outside this automated regression.

### 21.12 Spawn-point validity

The forward campaign spawn-point reference remained non-null before capture, after USSR ownership, and after US recapture. It remained disabled throughout. Its faction assignment followed vanilla state, but its enabled state did not change.

### 21.13 MOB deployment status

Authority found one enabled US MOB spawn point in M1.4B. Literal deployment and post-death respawn at the MOB were already manually completed in M1.3 and M1.4B made no MOB deployment change.

### 21.14 Forward deployment status

At every ownership snapshot the forward point reported `spawnPointValid=1`, `spawnEnabled=0`, `visibleInDeployUI=0`, `canReserve=0`, and authority result 3002. The earlier M1.3 manual UI test independently verified that this base class is absent from both initial deployment and respawn menus.

### 21.15 Remote-client replication

The final M1.4B authority run was started with a separate game-client process, but that process failed before networking because local Steam platform initialization was unavailable. A second headless-server binary reached the network endpoint but was rejected with identity zero and is not counted as a proxy proof.

Criterion K is instead supported by the existing M1.3 dedicated/proxy regression using the same production capture prefab, same disabled-deployment prefab, and same vanilla ownership/replication path. That separate proxy observed US -> USSR -> US, and M1.2 independently observed active capture plus replicated ownership. M1.4B introduced no ownership or replication modification. This is combined evidence, not a claim that the Final6 proxy observed the new run.

### 21.16 Reconnect result

A new reconnect could not be collected in Final6 for the same local Steam initialization reason. The prior M1.2 reconnect initialized with current USSR ownership, and the prior M1.4 reconnect initialized with current authoritative base state. No M1.4B replication change or regression was observed on authority. Per the milestone instruction, reconnect is non-blocking where prior behavior remains applicable.

### 21.17 Overall acceptance A–K

| Criterion | Result | Evidence |
| --- | --- | --- |
| A. Valid supply storage | **PASS** | Valid resource/consumer; 251/1000 initially; 254/1000 finally. |
| B. Internal spawn-point/reference retained | **PASS** | Non-null across US -> USSR -> US. |
| C. Regeneration while deployment disabled | **PASS** | 251 -> 252 before capture and 253 -> 254 after recapture; spawn remained disabled. |
| D. Lifecycle does not corrupt supply/base state | **PASS** | Store retained at 253 across ownership changes; components remained valid. |
| E. Expected vanilla radio participation | **PASS** | US 3/true/true -> USSR 0/false/false -> US 3/true/true. |
| F. Construction/services unaffected | **PASS** | Providers 2, master valid, radius 150, services 3 throughout; no built delegates expected. |
| G. MOB player-deployable | **PASS** | One enabled US MOB in M1.4B plus completed M1.3 manual deployment/respawn. |
| H. Forward base non-player-deployable | **PASS** | Disabled, hidden, reservation rejected across all phases; manual UI proof retained. |
| I. Dedicated vanilla AI capture with eligible AI | **PASS** | Both captures completed; 3/3 alive, activated, LOD 0, vanilla-eligible. |
| J. No custom replacement systems | **PASS** | Vanilla capture, ownership, supply, radio, construction, and replication retained. |
| K. Remote US -> USSR -> US ownership | **PASS from prior dedicated/proxy evidence** | M1.3 separate proxy observed the complete sequence through the unchanged path; Final6 client launch was environment-blocked and is not misreported. |

### 21.18 M1.4 verdict

**PASS.** All acceptance criteria are supported across the deterministic M1.4B authority regression, retained M1.4A causal/telemetry evidence, prior dedicated/proxy ownership proof, prior reconnect proof, and manual M1.3 deployment/respawn proof. Evidence boundaries are stated explicitly.

### 21.19 Activation-lease recommendation

A future scoped activation lease is **recommended**, but not implemented. Representative M1.4B actors remained naturally at LOD 0 for both operations, so the current small connected topology did not require intervention. Nevertheless, M1.4A proved that maximum LOD immediately removes otherwise valid AI from capture, and future operations are expected to occur away from human observers.

The high-level contract should remain only:

- `Acquire(group, reason)` — keep a registered capture-critical group simulation-eligible while at least one strategic reason is active;
- `Release(group, reason)` — remove that reason and return the group to normal engine LOD when no reasons remain.

It must be scoped, reference-counted by reason, authority-owned, automatically cleaned up when groups are destroyed/operation state ends/server state resets, and must never disable vanilla's maximum-LOD eligibility rule globally.

### 21.20 Remaining unknowns

1. The exact minimum vanilla population/spawnpoint/observer subset required for raw `SCR_AIGroup.RequestSpawn` materialization.
2. Literal remote construction-menu interaction and actual construction placement at the disabled-deployment forward base.
3. A fresh proxy/reconnect observation of the new symmetric M1.4B topology once Steam platform initialization is available.
4. How often distant production operations naturally reach maximum LOD under realistic player counts and AI budgets.

### 21.21 Recommended M1.5 scope

M1.4 is complete, but do not begin M1.5 in this task. The next milestone should start with the already proposed three-objective vertical slice and explicit server-authoritative frontier/objective policy, while preserving vanilla physical capture. Activation leasing should enter only when a production capture-critical group lifecycle exists to own `Acquire(group, reason)` and `Release(group, reason)` correctly; it should not be built as a free-standing global AI patch.
