# TB PvE Conflict Engine — M1.2 Production Base and Multiplayer Verification

## Result

**PASS.** A clean production PvE ordinary-base resource now directly inherits the vanilla Conflict military base and changes only the two AI-exclusion settings proven by M1.1A/M1.1B. An actual `ArmaReforgerServerDiag.exe` dedicated process completed a vanilla `US → USSR` capture, a separate network client process observed the active capture and replicated ownership change, and a fresh reconnecting client initialized with the current USSR ownership. No custom capture, ownership, or replication code was used.

## Production resource architecture

Production resource:

`Prefabs/Systems/MilitaryBase/TB_PVE_ConflictMilitaryBase.et`

Direct parent:

`{1391CE8C0E255636}Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et`

The production resource contains exactly these property overrides on the inherited vanilla `SCR_CampaignSeizingComponent`:

```text
m_bIgnoreNonPlayableAttackers 0
m_bCapturingRequiresPlayer 0
```

It does not override capture radius, timing, character count, faction affiliation, ownership, replication, radio, supply, notifications, building, spawning, or any other base behavior.

The experimental proof resource remains unchanged and separate:

`Prefabs/Systems/MilitaryBase/TB_PVE_M1_1B_AICaptureBase.et`

The production prefab derives directly from vanilla, not through the M1.1B asset. No mass rollout was performed.

## Controlled multiplayer test world

The existing controlled world remains:

`Worlds/TB_PVE_M1_1A_CaptureTest.ent`

Its single added test-base layer now instantiates the production resource as `TB_M1_2_BLUFOR_Base` with these scene-level test properties:

- Initial faction: US.
- Type: ordinary `BASE` (`type=0`).
- HQ: false.
- Base radius: 150 m.
- Effective inherited seizing radius: 50 m.
- Base name: `TB M1.2 BLUFOR Base`.

The harness configures the vanilla USSR campaign faction as non-playable with player limit zero while preserving `CanCaptureBases()`. On dedicated authority it spawns three vanilla USSR defender characters at the base centre. On proxy clients it performs read-only base lookup, subscribes to the same vanilla events, and periodically observes replicated state and ownership.

The harness never calls `SetFaction`, `SetAffiliatedFaction`, `BeginCapture`, `EndCapture`, an ownership RPC, or a custom timer. Client observation adds no custom replication.

## Dedicated-server procedure

Server executable:

`D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe`

Launch form:

```text
ArmaReforgerServerDiag.exe
  -server Worlds/TB_PVE_M1_1A_CaptureTest.ent
  -addonsDir "<dedicated-server addons>,<Workbench addons>"
  -addons 6A6A5973FA53A5B1
  -profile "<workspace>/TestProfiles/M1_2_Server_Live"
  -maxFPS 60
  -logLevel debug
```

Separate client executable:

`D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe`

Client launch form:

```text
ArmaReforgerSteamDiag.exe
  -client 127.0.0.1
  -addonsDir "<game addons>,<Workbench addons>"
  -addons 6A6A5973FA53A5B1
  -profile "<workspace>/TestProfiles/M1_2_Client_Live"
```

This produced two distinct processes, worlds, profiles, and logs connected over the local network stack. The client reported world role `Proxy`; it was not local/listen authority.

## Dedicated-server runtime evidence

Authoritative log:

`TestProfiles/M1_2_Server_Live/logs/logs_2026-09-21_09-15-38/console.log`

```text
09:15:59.125 [TB_PVE_M1_1] OnGameStart server=1 dedicated=1 proxy=0
09:16:01.445 [TB_PVE_M1_1] M1.2 OPFOR key=USSR playable=0 playerLimit=0 canCaptureBases=1 server=1 dedicated=1 proxy=0
09:16:01.445 [TB_PVE_M1_1] M1.2 initial proof base=TB M1.2 BLUFOR Base entity=TB_M1_2_BLUFOR_Base owner=US initialized=1 isHQ=0 type=0 baseRadius=150 seizingRadius=50
09:16:01.445 [TB_PVE_M1_1] M1.2 role server=1 dedicated=1 proxy=0
09:16:01.445 [TB_PVE_M1_1] M1.2 production gates ignoreNonPlayableAttackers=0 capturingRequiresPlayer=0 opforHQRadioTraffic=0
09:16:01.483 [TB_PVE_M1_1] AI group spawned phase=M1.2 dedicated OPFOR proof ... faction=USSR unitSlots=3 characters=3 ...
09:16:02.783 [TB_PVE_M1_1] CaptureStateChanged state=2 server=1 dedicated=1 proxy=0
09:16:02.784 [TB_PVE_M1_1] CaptureStart faction=USSR server=1 dedicated=1 proxy=0
09:16:04.433 [TB_PVE_M1_1] Observation tick=1 agents=3 livingInsideRadius=3 aiFaction=USSR captureState=2 owner=US contestingFactions=1 server=1
09:16:52.800 [TB_PVE_M1_1] MilitaryBaseSystem.OnBaseFactionChanged newOwner=USSR server=1 dedicated=1 proxy=0
09:16:52.800 [TB_PVE_M1_1] CampaignMilitaryBase.OnFactionChangedExtended previous=US new=USSR server=1 dedicated=1 proxy=0
09:16:52.801 [TB_PVE_M1_1] CaptureFinish faction=USSR server=1 dedicated=1 proxy=0
09:16:53.516 [TB_PVE_M1_1] CaptureStateChanged state=0 server=1 dedicated=1 proxy=0
09:16:55.499 [TB_PVE_M1_1] Observation tick=18 ... captureState=0 owner=USSR ...
09:16:55.499 [TB_PVE_M1_1] PASS M1.2 vanilla base ownership transitioned initialOwner=US finalOwner=USSR ...
```

The dedicated server was the authority throughout. Vanilla entered `CAPTURING`, completed the inherited 50-second timer, changed ownership, invoked both ownership callbacks, invoked `CaptureFinish`, and returned the capture state to `NONE`.

## Remote-client runtime evidence

Live-transition client log:

`TestProfiles/M1_2_Client_Live/logs/logs_2026-09-21_09-15-51/console.log`

```text
09:16:18.726 [TB_PVE_M1_1] OnGameStart server=0 dedicated=0 proxy=1
09:16:21.857 [TB_PVE_M1_1] M1.2 initial proof ... owner=US initialized=1 isHQ=0 type=0 baseRadius=150 seizingRadius=50
09:16:21.857 [TB_PVE_M1_1] M1.2 role server=0 dedicated=0 proxy=1
09:16:22.857 [TB_PVE_M1_1] M1.2 remote observation tick=1 captureState=2 owner=US server=0 dedicated=0 proxy=1
09:16:51.979 [TB_PVE_M1_1] M1.2 remote observation tick=30 captureState=2 owner=US server=0 dedicated=0 proxy=1
09:16:52.829 [TB_PVE_M1_1] CaptureFinish faction=USSR server=0 dedicated=0 proxy=1
09:16:52.829 [TB_PVE_M1_1] MilitaryBaseSystem.OnBaseFactionChanged newOwner=USSR server=0 dedicated=0 proxy=1
09:16:52.829 [TB_PVE_M1_1] CampaignMilitaryBase.OnFactionChangedExtended previous=US new=USSR server=0 dedicated=0 proxy=1
09:16:52.974 [TB_PVE_M1_1] M1.2 remote observation tick=31 captureState=2 owner=USSR server=0 dedicated=0 proxy=1
09:16:52.974 [TB_PVE_M1_1] PASS M1.2 remote client observed authoritative owner=USSR captureState=2 proxy=1
09:16:53.563 [TB_PVE_M1_1] CaptureStateChanged state=0 server=0 dedicated=0 proxy=1
```

The client finished loading after the server's `CaptureStart`, so it did not receive that earlier transition callback. It nevertheless initialized while capture was active with owner US and state `CAPTURING`, then received `CaptureFinish`, both ownership callbacks, owner USSR, and the later reset to state `NONE`.

No replication inconsistency was observed. Comparing log clocks, the client received its completion/ownership callbacks about 29 ms after the server emitted them and observed USSR ownership about 174 ms after the server callbacks.

### Callback ordering difference

Server order at completion:

1. MilitaryBaseSystem ownership callback.
2. Extended faction-change callback (`US → USSR`).
3. `CaptureFinish`.
4. Capture state returns to `NONE`.

Client order at completion:

1. `CaptureFinish`.
2. MilitaryBaseSystem ownership callback.
3. Extended faction-change callback (`US → USSR`).
4. Periodic observation sees owner USSR while state is still `CAPTURING`.
5. Replicated capture state returns to `NONE`.

Consumers must therefore treat authoritative replicated ownership as state and must not depend on identical cross-machine callback ordering.

## Reconnect regression

After the live client was disconnected, a fresh client process connected to the still-running dedicated server using a separate profile.

Reconnect log:

`TestProfiles/M1_2_Reconnect/logs/logs_2026-09-21_09-18-00/console.log`

```text
09:18:26.064 [TB_PVE_M1_1] OnGameStart server=0 dedicated=0 proxy=1
09:18:29.134 [TB_PVE_M1_1] M1.2 initial proof ... owner=USSR initialized=1 isHQ=0 type=0 baseRadius=150 seizingRadius=50
09:18:30.135 [TB_PVE_M1_1] M1.2 remote observation tick=1 captureState=0 owner=USSR server=0 dedicated=0 proxy=1
09:18:30.135 [TB_PVE_M1_1] PASS M1.2 remote client observed authoritative owner=USSR captureState=0 proxy=1
```

The reconnecting client received current USSR ownership directly and did not rely on replaying the original transition callbacks.

## Acceptance criteria

| Criterion | Evidence | Result |
|---|---|---|
| A. Clean production ordinary-base resource | `TB_PVE_ConflictMilitaryBase.et` exists | PASS |
| B. Direct vanilla inheritance | Direct parent is vanilla `ConflictMilitaryBase.et` | PASS |
| C. Only required PvE capture configuration changed | Exactly two serialized boolean overrides | PASS |
| D. M1.1B experiment remains separate | Experimental prefab remains present and unchanged | PASS |
| E. Actual dedicated server | `ArmaReforgerServerDiag.exe`; runtime `dedicated=1` | PASS |
| F. Non-playable USSR AI trigger vanilla capture | Three USSR AI; state 2 and `CaptureStart` | PASS |
| G. Dedicated authority completes US → USSR | Server ownership callbacks and final USSR owner | PASS |
| H. No custom/direct ownership assignment | No assignment, ownership RPC, or custom timer in harness | PASS |
| I. Separate client observes USSR through replication | Proxy observed US/state 2, callbacks, then USSR | PASS |
| J. No custom replication required | Vanilla replicated state/events used exclusively | PASS |

**Overall M1.2: PASS.**

## Evidence classification

### VERIFIED

- The production prefab directly inherits vanilla and overrides only the two AI-exclusion booleans.
- The M1.1B experimental prefab remains separate.
- The tested process was the actual dedicated-server executable and reported `dedicated=1`.
- Non-playable USSR retained `CanCaptureBases()=1`.
- Three living USSR AI occupied the seizing area on authority.
- Dedicated vanilla capture completed `US → USSR` with capture and ownership callbacks.
- A separate proxy client observed initial US ownership during active capture and final USSR ownership.
- The proxy received `CaptureFinish`, MilitaryBaseSystem ownership, extended faction-change, and final capture-state replication.
- A fresh reconnecting client initialized directly with owner USSR and state `NONE`.
- No custom capture, ownership, or replication implementation was required.
- Final Workbench script validation succeeded.

### INFERRED

- The approximately 29–174 ms server/client timing differences are ordinary localhost replication and scheduling latency. They are measured observations, but their exact causes were not instrumented below the engine replication layer.
- The absence of client `CaptureStart` was caused by joining after the server emitted it; the client still received the active timer/state snapshot.

### UNKNOWN

- Latency and callback ordering over a real wide-area network or under packet loss.
- Multiple simultaneous remote-client behavior.
- Persistence across a complete dedicated-server shutdown/restart; reconnect was tested against the same running server.
- Production application strategy across the eventual campaign route, HQ, and MOB resources.
- The future server-authoritative frontline eligibility layer and its client-facing state.

## Recommended M1.3 scope

Do not mass-convert bases yet. M1.3 should define the narrow server-authoritative campaign eligibility/frontline contract that decides which ordinary bases are currently eligible for capture while leaving the production prefab's vanilla seizing, ownership, radio, supply, and replication mechanisms intact. Any production rollout should remain explicit and separately reviewable.
