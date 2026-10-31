# M3.7 — Human Combat Playtest

## Status

**TECHNICAL BUILD READY — HUMAN PLAYTEST PENDING.**

M3.7 intentionally makes no balance claim. The production-only mission compiles, resolves and starts on `ArmaReforgerServerDiag.exe`. The required 30–60 minute human session has not been simulated and remains the acceptance activity.

### Local interactive development topology

`Tools/Run-M3_7-LocalInteractive.ps1` prepares a local, interactive M3.7 gameplay session using the previously successful M2.3 diagnostic topology: one `ArmaReforgerServerDiag.exe` authority process and two independent visible `ArmaReforgerSteamDiag.exe` proxy clients, each with an isolated fresh profile. The clients connect directly to `127.0.0.1` on Reforger's implicit development port and load BCM Conflict Core from the local Workbench addon root.

The launcher reproduces the evidenced M2.3 timing: it waits for positive server readiness, holds the server until approximately 257.705 seconds of process age, launches Client B first, and launches Client A approximately 779 milliseconds later without waiting for Client B authentication. It uses no authentication bypass or undocumented Steam argument.

This session is **LOCAL INTERACTIVE M3.7 GAMEPLAY VALIDATION** only. It does not prove or close M2.4 external retail network criteria M/N, M2.5 external retail outcome projection/JIP verification, Workshop delivery, public-server reachability, or Community-browser admission.

Smoke evidence:

`TestProfiles/M3_7_Smoke_1/logs/logs_2026-09-30_06-43-00/console.log`

## Playtest setup

Mission resource:

`{A370000000000000}Missions/TB_PVE_M3_7_Playtest.conf`

The mission uses the unchanged production world:

`{A241000000000000}Worlds/TB_PVE_M2_4_ProductionRoute.ent`

It has a separate player-facing name, `TB PvE M3.7 Human Combat Playtest`, but deliberately retains the M2.4 route, base whitelist, supplies, radio ranges and all production M2/M3 components.

### Dedicated-server launch

The public-browser dedicated configuration is:

`ServerConfigs/M3_7_HumanPlaytest.json`

It advertises `TB PvE Conflict | M3.7 Playtest`, binds and advertises UDP game port 2105, uses standard A2S UDP port 17777, and has no password. The JSON owns the browser, mission and addon configuration. Addon `6A6A5973FA53A5B1` is declared as a required server mod.

From the `BCM Conflict Core` workspace root, run:

```powershell
.\Tools\Run-M3_7-HumanPlaytestServer.ps1
```

The launcher creates `TestProfiles/M3_7_Playtest_<timestamp>`, prints the name, actual port and profile/log path, starts the server from the JSON configuration, and returns while the server remains running. In a retail client, open Multiplayer → Community and search for the exact name `TB PvE Conflict | M3.7 Playtest`. Select US/BLUFOR, deploy only at the North MOB, and do not use a diagnostic client identity for this acceptance session.

The launcher deliberately starts the dedicated-server executable with its installation directory as the process working directory. Reforger resolves its inherent `./addons` root from that directory; using the addon workspace as the process working directory makes the built-in Arma Reforger data project (`58D0FB3206B6F859`) appear missing before the server configuration is read.

For the genuine retail-client human playtest, the launcher uses the matched retail dedicated-server binary `ArmaReforgerServer.exe`. `ArmaReforgerServerDiag.exe` remains appropriate for automated development proofs but is not the M3.7 human-playtest default. Binary validation and BattlEye remain enabled.

Public launch verification now passes. With ServerDiag launched from its installation directory, the inherent base-game project `58D0FB3206B6F859` resolved locally, Workshop addon `6A6A5973FA53A5B1` version `0.0.1` downloaded and mounted, the M3.7 mission loaded, RPL listened on UDP 2105, the backend registered the public address, and the server entered online game state. The earlier Missing Addon dialog was caused by the launcher's process working directory, not by the project or Workshop dependency declaration.

The audited first-upload metadata, runtime closure, bundle exclusion checklist and interactive publication steps are recorded in `Documentation/TB-PvE-Conflict-Workshop-Development-Release.md`.

## Production configuration audit

| System | Production setting |
|---|---|
| M2 route | Ten-node `EVERON_NORTH_SOUTH_01`, enabled through the production world |
| Campaign outcomes | Enabled; vanilla end-match bridge enabled |
| M3.2 coordinator | Enabled, one operation maximum, 45-second initial delay, 6-second cooldown |
| M3.3 staging | 200–325 m annulus, 250 m player exclusion, 900 m visibility relevance, quiet candidate diagnostics |
| M3.4 packages | LIGHT 3, STANDARD 6, REINFORCED 9 infantry |
| M3.5 selection | Real active controlled US entities; thresholds 1/2/4 and production depth/result modifiers |
| M3.6 garrison | Enabled, fixed STANDARD package, objective-local placement and visibility gating |
| M3.7 telemetry | Enabled, server-only, read-only, change-filtered ten-second sampling |

Every proof setting is `0`: M2.1 vertical slice, M2.4 production proof, M2.5 outcome proof, and M3.1 through M3.6 proofs. No proof code is called. Human-count override fields remain at their constructor default `-1`; no forced package, delay, observer, ownership, capture, defeat or invalid-state seam is activated.

## Intended initial state and gameplay loop

The smoke run initialized an ACTIVE, valid campaign at revision 1:

- `MOB_NORTH=US`;
- every forward objective and the southern endpoint `USSR`;
- BLUFOR legal objective `OBJ_01_MAIDENS_BAY`;
- no initial OPFOR counterattack objective because BLUFOR owns only its MOB;
- coordinator `IDLE`;
- no physical garrison while human BLUFOR count is zero.

When the real player spawns as US, the M3.6 manager should create the first STANDARD garrison at Maidens Bay when its visibility policy finds safe conditions. The intended loop is MOB deployment → physical travel → garrison fight → vanilla capture → frontier advance → new garrison → coordinator wait/selection/staging → physical OPFOR counterattack → unscripted hold or retreat.

The deployment contract remains the accepted M1.3/M1.5 contract: US is the human faction, the North MOB is deployable, forward-base player deployment is disabled, USSR is AI-only, and USSR capture remains enabled.

## M3.7A — Production Human Presence Semantics

### Triggering runtime evidence

The completed local-interactive session admitted one genuine SteamDiag proxy. The player joined US, deployed at North MOB, travelled physically to Maidens Bay, completed an approximately 60-second vanilla capture, died, entered the normal respawn UI and later disconnected. Vanilla ownership changed `USSR -> US`; M2 advanced frontier `0 -> 1` and revision `1 -> 2`; BLUFOR legal became `OBJ_02_KERMOVAN`, OPFOR legal became `OBJ_01_MAIDENS_BAY`, and Client A received revision 2 live.

Throughout that lifecycle, `[TB_PVE_M3_7] humansUS=0`. Consequently, no Maidens Bay or Kermovan M3.6 garrison was created, M3.5 selected no package, and M3.2 remained idle. The old implementation enumerated `PlayerManager` entries but counted a player only when `GetPlayerControlledEntity(playerId)` existed and that entity carried an `SCR_FactionAffiliationComponent` whose affiliated faction was `US`. It therefore conflated durable faction participation with the transient representation and components of the controlled entity. The surviving log proves that this predicate returned false for the admitted US player; it does not retain enough per-player state to distinguish a missing entity from a missing/wrong entity affiliation component. M3.7A adds that missing trace for the next run rather than inventing a retrospective cause.

### Corrected contracts

`TB_PVE_M3_7_HumanPresenceQuery` is the single read-only production query:

- **Strategic participation:** enumerate admitted `PlayerManager` player IDs and use vanilla `SCR_FactionManager.SGetPlayerFaction(playerId)`. This is vanilla's authoritative player-faction mapping and is independent of possession of a live character. M3.5 selection, M3.2's participation gate and M3.6's conventional-war creation gate consume this count.
- **Physical observers:** start from the same vanilla faction assignment, then require a controlled `SCR_ChimeraCharacter` with a character controller in `ECharacterLifeState.ALIVE`. Only that character's real world origin is returned. Death, deployment UI or absence of a controlled character cannot fabricate an observer position. M3.3 operation staging and M3.6 creation/cleanup safety consume these positions.

The prior M3.1 observer enumeration included every controlled entity regardless of faction. It now uses the same US observer contract. The M3.6 proof-only human count and observer seams remain intact and default off; no runtime fallback, synthetic participant or test-only override was added.

Change-filtered `[TB_PVE_M3_7A] human-presence` telemetry records player ID/name, vanilla player faction, player-controller presence, controlled-character presence/life state, controlled-entity faction, participant count and observer count. This instrumentation is read-only and retains no player-faction registry.

### Validation and acceptance status

The official Workbench script validator passes after the source change. A fresh admitted-client lifecycle run is still required; source inspection does not satisfy runtime criteria.

| Criterion | Status | Evidence / remaining proof |
|---|---|---|
| A. Admitted US human counted strategically | PENDING RUNTIME | Query now uses vanilla player-faction state. |
| B. Count survives character lifecycle | PENDING RUNTIME | Must observe death, deployment UI and respawn. |
| C. Disconnect removes participant | PENDING RUNTIME | PlayerManager/faction-manager removal must be observed. |
| D. AI/FIA/USSR do not contaminate US count | PENDING RUNTIME | Contract filters admitted player IDs by exact `US`; runtime regression remains. |
| E. Participants and observers are distinct | IMPLEMENTED, RUNTIME PENDING | Separate public query methods and consumers. |
| F. Alive US player has an observer | PENDING RUNTIME | Requires admitted deployed client. |
| G. Non-world participant has no fabricated observer | PENDING RUNTIME | Requires death/deployment observation. |
| H. M3.5 consumes participant count | IMPLEMENTED, RUNTIME PENDING | Selection policy routes through shared query. |
| I. M3.6 uses participation plus observer safety | IMPLEMENTED, RUNTIME PENDING | Count and positions are separate; distances/visibility unchanged. |
| J. No test-only override | PASS BY INSPECTION | Existing proof seams unchanged and default off; no new override. |
| K. M2 ownership/frontier unchanged | PASS FOR PRE-FIX SESSION; POST-FIX REGRESSION PENDING | Completed session proved vanilla capture and M2 revision 2; M3.7A does not edit M2. |

The post-death `Client data is null` warning, later `UpdateClientData` failure and disconnect group 1/reason 9 remain secondary local SteamDiag observations. Current evidence does not connect them to the old counting predicate: the human count was already wrong while the player was alive, deployed and successfully capturing. No networking or duplicate-identity change is included in M3.7A.

## Telemetry

`[TB_PVE_M3_7]` emits a snapshot only when its semantic state changes, sampled every ten seconds. Its three correlated lines contain:

- real active US human count, readiness, validity, outcome and revision;
- BLUFOR/OPFOR legal objectives;
- garrison objective, state, groups, units and LOD;
- coordinator state;
- operation ID, state, target, selected package, groups, units and LOD.

Existing production diagnostics additionally record staging positions and duration, package assembly duration, movement distances, capture eligibility/state, vanilla ownership changes, frontier movement, cleanup and garrison creation duration. Per-candidate M3.3 diagnostics remain disabled.

The session log will be below the fresh profile supplied through `-profile`, normally:

`TestProfiles/M3_7_Playtest_<timestamp>/logs/logs_<timestamp>/console.log`

Server FPS should be taken from the running server/diagnostic performance display when available. Record coarse visible hitching alongside the timestamp because no gameplay code was added solely to calculate FPS.

## Human playtest checklist

Run for 30–60 minutes. Aim for at least one genuine BLUFOR capture, one autonomous counterattack, one terminal operation result and the beginning of a subsequent coordinator cycle.

### Start and deployment

- [ ] Server starts from the M3.7 mission with ACTIVE revision 1.
- [ ] Join US/BLUFOR successfully.
- [ ] North MOB is offered and usable for deployment.
- [ ] No forward objective is offered for deployment.
- [ ] Record human count while connected, deployed, dead, in deployment UI and respawned.

### First attack

- [ ] Record travel time from MOB to Maidens Bay.
- [ ] Note whether the first garrison appears naturally or visibly pops in.
- [ ] Assess the positioning of both three-man groups.
- [ ] Confirm defenders react and fight without artificial intervention.
- [ ] Note whether defenders remain meaningfully associated with the objective.
- [ ] Rate six defenders as too weak, reasonable or too strong; do not alter the package.
- [ ] Capture through normal Conflict gameplay without killing actors through tools.

### Frontier transition

- [ ] Match vanilla ownership change to the M2 revision/frontier update.
- [ ] Confirm the old garrison retires without visible deletion nearby.
- [ ] Confirm the next OPFOR frontline is correct.
- [ ] Observe whether and when its replacement garrison appears.
- [ ] Confirm the recently captured objective becomes OPFOR's legal counterattack target.

### Counterattack

- [ ] Observe the coordinator's waiting period.
- [ ] Record real human count, depth, prior result and selected package.
- [ ] Record staging position/distance and whether it feels plausible.
- [ ] Confirm the attack group moves physically rather than appearing at the objective.
- [ ] Record warning/predictability, arrival time and dead time between fights.
- [ ] Fight normally and allow either BLUFOR hold or OPFOR recapture.
- [ ] Confirm the operation reaches SUCCEEDED, FAILED or CANCELLED and enters cooldown.
- [ ] Confirm the next coordinator cycle uses fresh strategic state.

### Player experience and performance

- [ ] Respawn/reinsert burden.
- [ ] Garrison difficulty and positioning quality.
- [ ] AI combat behavior, stuck/lost AI or implausible wandering.
- [ ] Counterattack timing, strength and staging plausibility.
- [ ] Whether objectives feel occupied and the battle direction is understandable.
- [ ] Whether MOB-only respawn feels appropriate.
- [ ] Noticeable radio effects.
- [ ] Obvious exploits.
- [ ] Server FPS where available and obvious hitching.
- [ ] Garrison, attack and combined physical AI counts at major events.
- [ ] Cleanup behavior after capture or operation termination.

## Post-session template

Do not fill this from synthetic actors. After human feedback is supplied, add:

- chronological player/system timeline with log timestamps;
- deployment and human-count transitions;
- qualitative observations from the checklist;
- performance baseline;
- blockers and bugs;
- balance concerns that were observed but not automatically fixed;
- UX concerns;
- narrowly recommended next changes.
