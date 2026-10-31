# TB PvE Conflict — M1.5 Human Faction Restriction

## Verdict

**M1.5 is PASS.**

The production Conflict faction manager and faction resources remain vanilla. A narrow server-authoritative policy closes the USSR human slot through the vanilla faction API while leaving its AI/gameplay configuration intact. On an actual dedicated server the resulting contract was:

- US: `playable=1`, `playerLimit=-1` (unlimited/default), `CanCaptureBases()=1`.
- USSR: `playable=0`, `playerLimit=0`, `CanCaptureBases()=1`.
- FIA: observed unchanged at its inherited scenario values: `playable=1`, `playerLimit=-1`, `CanCaptureBases()=0`.

Three ordinary USSR AI characters then completed the existing vanilla US -> USSR capture. Three ordinary US AI completed the USSR -> US recapture. Supply income resumed and the M1.3 forward deployment restriction remained intact.

**M1 FOUNDATION COMPLETE**

## 1. Exact implementation

Created `Scripts/Game/TB_PVE_M1_5/TB_PVE_M1_5_HumanFactionRestriction.c`.

On authoritative `SCR_GameModeCampaign.OnGameStart()`, it resolves the existing campaign OPFOR faction and calls:

```c
opfor.SetPlayerLimit(0);
opfor.SetIsPlayable(false);
opfor.InitFactionIsPlayable(false);
```

It does not replace the faction manager, faction-selection UI, or any faction resource. It does not change `CanCaptureBases`, catalogs, loadouts, defender prefabs, radio properties, ownership, capture, replication, or AI behavior. US and FIA are not mutated.

The explicit playable calls are necessary in the tested engine build. A discarded probe showed that `SetPlayerLimit(0)` changed the limit to zero but left runtime `IsPlayable()` true. The final policy therefore authors both parts of the required contract explicitly through vanilla APIs.

### Why this is not a faction-manager asset override

The production-style test world inherits `worlds/MP/CTI_Campaign_Eden.ent`, including its faction-manager entity. An attempted child-layer override was rejected during investigation because Enfusion treated it as an additional entity, producing `Multiple faction managers present!`; it was removed and is not part of the implementation.

Rebuilding the entire Conflict scenario from the terrain merely to own that embedded manager would be a much larger and more fragile change. The chosen policy is the smallest production-safe alternative: it retains ordinary Conflict selection and only configures the existing OPFOR faction on authority. No custom faction-selection system was introduced and no vanilla resource was modified.

## 2. Test scenario

Created:

- `Missions/TB_PVE_M1_5.conf`
- `Missions/TB_PVE_M1_5.conf.meta`

The M1.5 header uses the same production-style world and deterministic two-MOB/two-forward-base topology proven in M1.4B. It preserves the existing supply settings and base whitelist.

The disposable capture harness no longer mutates OPFOR faction state. It only logs the independently applied M1.5 contract and runs the previously proven vanilla capture/deployment lifecycle.

Final dedicated evidence:

- `TestProfiles/M1_5_Server_Final2/logs/logs_2026-09-22_10-28-42/script.log`

Workbench script/resource validation completed with exit code 0 after the final implementation.

## 3. Faction contract evidence

Dedicated authority logged:

```text
[TB_PVE_M1_5] ENFORCED key=USSR playable=0 playerLimit=0 canCaptureBases=1 server=1 dedicated=1 proxy=0
[TB_PVE_M1_5] BLUFOR key=US playable=1 playerLimit=-1 canCaptureBases=1 server=1 dedicated=1 proxy=0
[TB_PVE_M1_5] OPFOR key=USSR playable=0 playerLimit=0 canCaptureBases=1 server=1 dedicated=1 proxy=0
[TB_PVE_M1_5] FIA key=FIA playable=1 playerLimit=-1 canCaptureBases=0 unchanged=1 server=1 dedicated=1 proxy=0
```

`playerLimit=-1` is the inherited unlimited/default value for the playable factions in this scenario. Only USSR is changed.

## 4. US human playability and joinability

**PASS across combined evidence.** The M1.5 dedicated run reports US `playable=1` with its normal unlimited/default player limit. The earlier M1.3 manual acceptance used this same production-style world and verified that a human could select and join US/BLUFOR, deploy at the US MOB, die, and respawn at the MOB.

M1.5 changes only the resolved OPFOR faction. It does not touch US, the player controller, faction selection, loadouts, spawn handling, or deployment UI. Therefore the completed human US join test remains a direct regression baseline and the final server run confirms its prerequisite values remain unchanged.

## 5. USSR human restriction

**PASS.** Dedicated authority reports both required gates: `playable=0` and `playerLimit=0`. Vanilla faction-selection/respawn behavior excludes a faction whose playable state is false, while the zero player limit independently closes its player allocation.

A new literal faction-selection click was not completed in M1.5 because the bounded diagnostic client attempt failed before connecting (see section 9). The authoritative runtime properties that drive availability were nevertheless directly verified.

## 6. USSR AI functionality and capture regression

**PASS.** After the restriction was active:

- the vanilla USSR defender group prefab materialized normally;
- three USSR AI characters existed, were alive, had controllers/AI control/agents, and were members of the USSR group;
- all three evaluated as vanilla-eligible USSR capture entities;
- group and character LOD remained engine-active at `0`; no forced maximum-LOD workaround was enabled;
- vanilla capture started and finished;
- ownership changed from US to USSR;
- US AI subsequently recaptured the base through the same vanilla path.

Key results:

```text
PASS M1.3 AI-only OPFOR capture initialOwner=US intermediateOwner=USSR
PASS M1.3 ownership regression sequence=US->USSR->US finalOwner=US
[TB_PVE_M1_4B] COMPLETE sequence=US->USSR->US postRecaptureSupplyDelta=1 observationTicks=1
```

The harness never assigns ownership. This confirms no regression to the M1.2/M1.4 AI-capture configuration.

## 7. MOB-only deployment regression

**PASS.** The final dedicated run found one enabled US MOB spawn point and retained the forward-base contract:

```text
enabledUSMobSpawnPoints=1
spawnPointValid=1 spawnEnabled=0 visibleInDeployUI=0 canReserve=0 reserveResult=3002
```

After the full US -> USSR -> US lifecycle, the forward point was still valid but disabled/hidden and authority still rejected it with vanilla result `3002`. Supply income had returned to `1`, and supplies advanced from `178` to `179` after recapture.

The literal MOB-only deploy and respawn UI flow was manually accepted during M1.3. M1.5 does not modify any spawn/deployment code or resource.

## 8. Dedicated-server result

**PASS.** The final run used `ArmaReforgerServerDiag.exe` and reported `server=1`, `dedicated=1`, `proxy=0`. It proved in one session:

1. the final US/USSR/FIA faction values;
2. normal USSR AI creation;
3. non-playable USSR AI capture;
4. US AI recapture;
5. forward spawn rejection throughout ownership changes;
6. an enabled US MOB;
7. supply recovery after recapture.

No duplicate faction manager or M1.5 script error was present in the final run.

## 9. Client-facing verification

The existing manual M1.3 player acceptance provides the successful human path: select/join US, deploy at the MOB, and respawn at the MOB. The prior M1.3 remote proxy also proved the same scenario's ownership replication.

A fresh M1.5 remote-client attempt was practical and was made, but `ArmaReforgerSteamDiag.exe` terminated before connecting with:

```text
Steamworks: SteamAPI_Init failed. Is Steam running?
```

Attempt log:

- `TestProfiles/M1_5_Client_Final/logs/logs_2026-09-22_10-33-56/error.log`

Accordingly, M1.5 does not claim a new literal remote faction-menu interaction. Its client-facing conclusion combines the already completed manual US UI/join test with directly verified final authoritative playability/limit values and the fact that M1.5 changes only OPFOR.

## 10. Acceptance matrix

| Requirement | Result | Evidence |
| --- | --- | --- |
| 1. US available to a human player | **PASS** | Dedicated `US playable=1`, default/unlimited limit; prior manual selection UI offered US. |
| 2. Player can join US | **PASS** | Completed M1.3 manual US join on the same production-style world; M1.5 leaves US unchanged. |
| 3. USSR not available as a human faction | **PASS** | Dedicated authority `USSR playable=0`; this is the vanilla availability gate. |
| 4. USSR `playerLimit=0` | **PASS** | Dedicated authority logged `playerLimit=0`. |
| 5. USSR `CanCaptureBases()=true` | **PASS** | Dedicated authority logged `canCaptureBases=1`. |
| 6. USSR AI can exist normally | **PASS** | Vanilla USSR group plus three living, controlled, eligible AI characters materialized. |
| 7. No AI-capture regression | **PASS** | Vanilla US -> USSR -> US capture/recapture completed with no forced-LOD workaround. |
| 8. No MOB-only deployment regression | **PASS** | One enabled US MOB; forward spawn remained hidden/disabled/rejected before and after lifecycle. |

## 11. Remaining unknowns

1. A fresh literal USSR faction-selection screen check from a remote M1.5 client remains unrecorded because the diagnostic game executable could not initialize SteamAPI.
2. The cause of that local SteamAPI failure remains external to the addon; it also affected the late M1.4B client attempt.
3. The authority policy has been verified at initial scenario start. A future persistence/hot-reload test may additionally confirm the contract after unusual admin-driven faction reinitialization, although normal dedicated start and play are covered.
4. FIA's inherited values were observed, not redesigned. Whether a future production scenario wants different FIA semantics is intentionally outside M1.5.

## 12. Recommended next action

Proceed to **M2.1: a three-objective linear-frontline vertical slice**. Keep vanilla physical capture, radio, ownership, supply, AI faction behavior, and the completed human-faction contract as the foundation. Do not introduce AI Director or activation-lease scope until the vertical slice identifies a concrete lifecycle owner and need.
