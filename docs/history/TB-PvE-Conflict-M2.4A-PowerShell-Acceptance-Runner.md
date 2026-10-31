# TB PvE Conflict M2.4A — PowerShell Network Acceptance Runner

## Purpose

`Tools/Run-M2_4A-NetworkAcceptance.ps1` reproduces the proven-compatible M2.3 diagnostic executable pairing while running the bounded M2.4A production-route acceptance harness. It uses positive server, admission, world-initialization and query-contract log evidence. Process existence alone never counts as a connection.

The runner does not disable authentication, alter credentials, edit Steam or vanilla resources, change firewall/router configuration, or modify gameplay data.

## Reconstructed M2.3 baseline

The successful M2.3 evidence is:

- server: `TestProfiles/M2_3_Server_2/logs/logs_2026-09-22_14-01-34/console.log`;
- Client A: `TestProfiles/M2_3_Client_A/logs/logs_2026-09-22_14-05-53/console.log`;
- Client B: `TestProfiles/M2_3_Client_B/logs/logs_2026-09-22_14-05-52/console.log`.

The server's script defines contain `ENF_WB` and not `ENABLE_DIAG`, matching `ArmaReforgerServerDiag.exe`. Both clients contain `ENF_WB` and `ENABLE_DIAG`, matching `ArmaReforgerSteamDiag.exe`. The exact logged command lines used:

```text
-server {A212000000000000}Missions/TB_PVE_M2_1.conf
-addonsDir D:\SteamLibrary\steamapps\common\Arma Reforger Server\addons,<Workbench addons>
-addons 6A6A5973FA53A5B1
-profile <M2_3_Server_2>
-maxFPS 60
-logLevel debug
```

```text
-client 127.0.0.1
-addonsDir D:\SteamLibrary\steamapps\common\Arma Reforger\addons,<Workbench addons>
-addons 6A6A5973FA53A5B1
-profile <M2_3_Client_A-or-B>
-logLevel debug
```

Port `2001` was implicit. No explicit backend, Steam, bind/public-port, password, authentication-bypass, Steam AppID, or environment-variable argument was supplied by the recovered launch commands.

The original Codex session archive has now supplied the literal launch automation. All three commands ran from the BCM Conflict Core workspace root. The server was invoked directly through PowerShell's call operator (`&`). Approximately 4 minutes 18 seconds after the server engine started, Client A and Client B were submitted concurrently through two independent direct PowerShell invocations. Client B's engine happened to start about 0.779 seconds before Client A. Both first connected with a provisional identity, remained pending for approximately eighteen seconds, entered online world state, received Steam web-API tickets and were created as players. The server then logged `Players connected: 2 / 2`.

## Differences from failed M2.4A attempts

- One failed retry paired retail `ArmaReforgerServer.exe` with `ArmaReforgerSteamDiag.exe`; the server explicitly rejected it because `isDevBinary` differed.
- The later diagnostic/diagnostic attempt used the same binary classes as successful M2.3, but identity zero was rejected almost immediately with reason 3 instead of being allowed to complete the earlier approximately eighteen-second initialization sequence.
- The M2.4 mission, route, script checksum and fresh profile paths necessarily differ from M2.3. Native protocol version, addon ID, addon roots, client address, implicit port and normal authentication policy match.
- The successful M2.3 launch used direct `&` invocation from the workspace root. The failed M2.4A attempt used `Start-Process -WindowStyle Hidden -PassThru` and each executable's installation directory as its working directory. The runner now preserves `Start-Process` ownership/cleanup but uses the recovered workspace-root working directory.
- M2.3 submitted both clients concurrently. The M2.4A runner starts Client A alone and deliberately does not start Client B until Client A has authenticated and the server has reached frontier 4.
- M2.3 allowed approximately 4 minutes 18 seconds between server engine startup and the clients. The failed M2.4A run started Client A approximately 39 seconds after server startup and about one second after `SERVER_INITIAL_READY`.
- No evidenced special environment variable or omitted authentication argument explains the reason-3 behavior.

## Prerequisites

- Steam running and signed in normally.
- Arma Reforger Tools/server/client versions mutually compatible.
- The local addon resource database current and the M2.4A harness validated.
- TCP/UDP localhost networking available on the normal Reforger port.
- No unrelated `ArmaReforgerServerDiag.exe` or `ArmaReforgerSteamDiag.exe` process running.

The runner only terminates stale processes whose command line identifies a profile under `TestProfiles\M2_4A_Runner_*`. It refuses to kill another Reforger process.

The normal-run preflight now refuses to start the server if the Steam process is absent.

## Configuration

Machine-specific defaults are parameters at the top of the script:

- `ServerExecutable`
- `ClientExecutable`
- `ServerAddonsDirectory`
- `ClientAddonsDirectory`
- `AddonId`
- `Mission`
- `ServerAddress`
- `ServerSettleSeconds` (default 45 seconds after positive readiness)
- timeout values

Override any path on invocation rather than editing vanilla files. Example:

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1 -ServerExecutable 'E:\Arma Reforger Server\ArmaReforgerServerDiag.exe'
```

## Dry run

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1 -DryRun
```

Dry-run mode prints resolved executables, arguments, working directories through the commands, profiles and launch order. It creates no directories and starts no processes.

## Normal run

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1
```

The order is deterministic:

1. start the diagnostic dedicated server;
2. wait for `[TB_PVE_M2_4A] SERVER_INITIAL_READY` at revision 1/frontier 0;
3. preserve a bounded post-ready settle interval (45 seconds by default, matching the recovered M2.3 late-hold interval);
4. start Client A;
5. require server player admission, client online-world initialization and a parsed `CLIENT_READY` snapshot;
6. verify Client A's revision-1 projection;
7. verify a later live snapshot;
8. wait for authoritative `MID_ROUTE_READY` at frontier 4;
9. start fresh Client B;
10. require Client B admission and direct current-state initialization;
11. compare the latest A/B snapshots when Client A remains alive;
12. stop only the processes started by this run.

## Logs

Each run preserves isolated profiles and logs under:

```text
TestProfiles/M2_4A_Runner_<timestamp>/Server
TestProfiles/M2_4A_Runner_<timestamp>/Client_A
TestProfiles/M2_4A_Runner_<timestamp>/Client_B
```

The terminal summary prints the exact run root.

## Expected output

The runner reports independently:

```text
SERVER START: PASS/FAIL
CLIENT A AUTH: PASS/FAIL
CLIENT A INITIAL PROJECTION: PASS/FAIL
CLIENT A LIVE SYNC: PASS/FAIL
FRONTIER 4 REACHED: PASS/FAIL
CLIENT B AUTH: PASS/FAIL
CLIENT B JIP: PASS/FAIL
A/B CONVERGENCE: PASS/FAIL/NOT TESTED
```

It then reports a classification and independent criterion M/N results.

## Authentication blocked

An identity-zero group-4/reason-3 rejection is classified as `AUTHENTICATION BLOCKED`. The runner stops the acceptance run, preserves all logs, does not retry, and leaves M/N unverified unless an earlier criterion was already fully proven. This is not reported as a frontier-replication failure.

If a client is admitted and enters the world but its required M2.4A projection is missing or wrong, the classification is `FRONTIER REPLICATION FAILED`. That is a genuine networking-path failure suitable for projection/JIP investigation.

## Criterion rules

- Criterion M passes only when Client A is admitted, reports the correct initial revision-1 production projection, and receives a later valid frontier revision without reconnecting.
- Criterion N passes only when fresh Client B is admitted after authoritative frontier 4 exists and initializes directly at that same non-revision-1 state.
- A/B convergence is additional evidence and is not required for M or N.

Do not mark M2.4 PASS from process existence, transport connection, identity zero, or prior M2.3 results alone.

## Runner validation on 2026-09-29

Dry-run validation completed successfully: commands, paths, isolated profiles and launch order resolved without creating directories or processes.

One normal attempt was made and preserved under `TestProfiles/M2_4A_Runner_2026-09-29_06-45-00`. The server reached the positive revision-1/frontier-0 marker. Client A then logged `SteamAPI_Init failed. Is Steam running?` and exited before transport connection because Steam was not running on the host. This is an authentication/prerequisite block, not a frontier-replication failure, and it provides no M/N evidence. No retry was made. The runner was subsequently hardened to preflight Steam and to classify a flushed Steam initialization failure after early client exit.

## Focused authentication comparison — Steam-running attempt

The runner was subsequently executed with Steam running. The preserved evidence is under `TestProfiles/M2_4A_Runner_2026-09-29_06-54-58`. Server start passed, but Client A was rejected before world initialization. The runner correctly classified the run as `AUTHENTICATION BLOCKED`; M and N remain unverified.

### Successful M2.3 chronology

Both successful clients used the logged Steam user `TacticalBootneck`, the same executable mode and an overlapping Steam-authentication period. Their two ticket-response payload hashes match across clients. The logs do not expose a Steam session identifier, so they prove the same account/authentication context but not a formally identifiable session ID.

Client B started first:

1. 14:05:53.617 — `Steamworks: StartUp()` followed by country `GB`.
2. 14:05:55.370–55.670 — online game config attempted, settings and sessions containers loaded, and user `TacticalBootneck` signed in.
3. 14:05:56.071–56.169 — multiplayer/RPL client started; native and script checksums logged; connection request, cookie and payload exchange completed.
4. 14:05:56.267 — transport connected with provisional `identity=0x00000000`.
5. 14:05:56.169–56.197 on the server — the corresponding identity-zero connection authenticated, connected, and produced `Trying to register connection of possibly unauthorized client`.
6. The server did not disconnect it. The connection remained pending for about 19.6 seconds.
7. 14:06:15.905–16.004 — player connection/creation began and Client B entered online game state.
8. 14:06:16.108 — two Steam web-API ticket response events were received.
9. 14:06:16.634 — the player was created as `TacticalBootneck`; the server subsequently reported two connected players.

Client A followed the same sequence:

1. 14:05:54.427 — Steamworks startup succeeded.
2. 14:05:56.090–56.288 — online config, settings/session load and `TacticalBootneck` sign-in succeeded.
3. 14:05:56.887–56.987 — RPL negotiation completed and transport connected. The server assigned transport identity `0x00000001` because identity zero was already occupied by Client B.
4. 14:05:56.898 — the server emitted the same possibly-unauthorized registration warning.
5. The server again did not disconnect the client and allowed about 17.9 seconds for admission.
6. 14:06:14.764–14.914 — server player connection began and Client A entered online game state.
7. 14:06:15.021 — two Steam web-API ticket response events were received.
8. 14:06:15.492 — player 1 was created as `TacticalBootneck`.

No explicit `GetAuthTicket` or equivalent ticket-request line exists in either successful log. Ticket receipt is confirmed; the exact request time and requester are not exposed by these logs.

### Failed M2.4A chronology

1. 06:55:38.124–38.125 — `Steamworks: StartUp()` succeeded and country `GB` was returned. This confirms SteamAPI initialized in this run.
2. 06:55:39.524–39.823 — online game config was attempted, settings and sessions containers loaded, and the same user name, `TacticalBootneck`, signed in.
3. 06:55:39.825–40.324 — multiplayer/RPL startup, matching client/server native and script checksums, connection request, cookie and payload exchange all completed.
4. 06:55:40.323–40.422 — the server authenticated and connected the transport as provisional `identity=0x00000000`; the client reported the same identity.
5. 06:55:40.355 — the server logged `Trying to register connection of possibly unauthorized client`.
6. 06:55:40.523 — only about 0.168 seconds after that warning, the server disconnected identity zero with `group=4, reason=3`.
7. 06:55:40.622 — the client observed disconnection before player creation or world initialization.
8. The client began one engine-level reconnect attempt; the runner detected the reason-3 block and stopped the acceptance run without an operator retry.

No Steam web-API ticket response appears before termination. No explicit ticket-request event appears either. Therefore ticket receipt is confirmed absent in the failed window, while whether a ticket was requested is unknown from available logging.

### Last identical event and first divergence

The last materially identical sequence is:

```text
Steam startup succeeds
sessions container loads
TacticalBootneck signs in
RPL request/cookie/payload succeeds
transport connects with a provisional identity
server logs possibly-unauthorized registration warning
```

The first divergence occurs immediately after that server warning:

- successful M2.3: connection remains pending and is admitted approximately 18–20 seconds later;
- failed M2.4A: server disconnects provisional identity zero after approximately 0.2 seconds with group 4/reason 3.

The warning itself is not the failure because it is present in both successful and failed runs.

### Evidence classification

| Potential cause or observation | Classification | Log evidence |
|---|---|---|
| SteamAPI failed in the Steam-running attempt | **UNSUPPORTED** | Startup and country lookup succeeded; `TacticalBootneck` signed in. |
| Client remained identity `0x00000000` at failed authorization | **CONFIRMED** | Both client and server log identity zero immediately before reason 3. |
| Identity zero alone is invalid | **UNSUPPORTED** | Successful M2.3 Client B initially used identity zero and was admitted. |
| Server made an immediate reason-3 authorization rejection | **CONFIRMED** | Group 4/reason 3 followed the warning by about 0.168 seconds. |
| Failed client received a Steam web-API ticket | **UNSUPPORTED** | No ticket response appears before termination. |
| Failed client requested a Steam ticket | **UNKNOWN** | Neither successful nor failed logs expose an explicit request event. |
| Backend/service error immediately preceded rejection | **UNSUPPORTED** | No curl, HTTP, REST, backend or platform error precedes the failure. |
| Protocol/binary-class mismatch caused this run | **UNSUPPORTED** | Diagnostic classes match and client/server checksums match; no validation mismatch is logged. |
| Different Steam account caused the failure | **UNSUPPORTED** | Successful and failed clients all log `TacticalBootneck`. |
| Successful M2.3 clients used the same logged Steam account | **CONFIRMED** | Both log `TacticalBootneck`; their ticket-response hashes match; server reports 2/2. |
| Successful M2.3 clients used the same formal Steam session ID | **UNKNOWN** | The logs expose loaded session containers but no comparable Steam session identifier. |
| Simultaneous ServerDiag/SteamDiag under one Steam session inherently conflicts | **UNSUPPORTED** | That exact mode successfully admitted two M2.3 clients concurrently. |
| Profile paths differ | **CONFIRMED** | M2.3 and M2.4A use separate named profile roots. |
| Profile/session contents caused rejection | **UNKNOWN** | All clients report successful settings/session-container load; no corrupt/stale-session diagnostic is logged. |
| Stale Reforger process caused rejection | **UNSUPPORTED** | The runner found no unrelated Reforger process and used isolated profiles. |
| Stale Steam/backend or diagnostic identity state caused rejection | **UNKNOWN** | The logs contain no lifecycle/state diagnostic capable of proving or excluding it. |
| Project frontier/gameplay code caused pre-world rejection | **UNSUPPORTED** | Rejection occurs before world/script initialization and before any client query snapshot. |
| Exact lower-level root cause | **UNKNOWN** | The logs identify the authorization boundary and timing but not why the server changes from pending to immediate rejection. |

## Recovery of Previously Working Local Multi-Client Setup

### Sources searched

The workspace root, `Tools`, `TestProfiles`, `Documentation`, milestone artifacts, command/script files and surviving profiles were searched for M2.2/M2.3, proxy, two-client and Reforger launch material. The accessible PSReadLine history was also inspected without modification.

No old `.ps1`, `.bat` or `.cmd` M2.3 launcher exists in the workspace. The actual automation was recovered instead from the original Codex session archive:

```text
C:\Users\ruanb\.codex\sessions\2026\09\21\rollout-2026-09-21T06-40-38-01a0c27b-0952-7212-9a56-1e5dffad029a.jsonl
```

This is a primary record of the literal tool calls, not an equivalent-looking reconstruction from engine logs.

### Exact recovered procedure

The server was launched by direct PowerShell invocation from the workspace root:

```powershell
& 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe' `
  -server '{A212000000000000}Missions/TB_PVE_M2_1.conf' `
  -addonsDir 'D:\SteamLibrary\steamapps\common\Arma Reforger Server\addons,C:\Users\ruanb\Documents\My Games\ArmaReforgerWorkbench\addons' `
  -addons 6A6A5973FA53A5B1 `
  -profile 'C:\Users\ruanb\Documents\My Games\ArmaReforgerWorkbench\addons\BCM Conflict Core\TestProfiles\M2_3_Server_2' `
  -maxFPS 60 `
  -logLevel debug
```

The Codex command yielded after one second while the engine remained active. The engine started at 14:01:34.759. The two clients were submitted together at 13:05:42 UTC through a `Promise.all` of two independent command executions. Each command was direct PowerShell invocation from the workspace root:

```powershell
& 'D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteamDiag.exe' `
  -client 127.0.0.1 `
  -addonsDir 'D:\SteamLibrary\steamapps\common\Arma Reforger\addons,C:\Users\ruanb\Documents\My Games\ArmaReforgerWorkbench\addons' `
  -addons 6A6A5973FA53A5B1 `
  -profile '<M2_3_Client_A or M2_3_Client_B absolute path>' `
  -logLevel debug
```

Client B's engine started at 14:05:52.464 and Client A's at 14:05:53.243. Therefore:

- server-to-first-client engine delay: approximately 4 minutes 17.705 seconds;
- Client A versus Client B submission delay: none intentionally; they were submitted concurrently;
- observed engine-start separation: approximately 0.779 seconds, with B first;
- Client A was not allowed to authenticate before Client B launched;
- both clients were already in overlapping authentication/transport initialization before either entered the world.

There was no dedicated-server JSON configuration. The scenario resource was supplied directly with `-server`. No explicit port, backend/auth switch, password, bind address, Steam AppID setting or environment-variable assignment was present. Addon ID and addon-root order match the later runner. The old launch did not use Workbench as a launcher and did not launch SteamDiag indirectly through Workbench.

The record shows Codex requested escalated execution for the launches, but it does not record the Windows token/elevation level inside each process. No evidence establishes a privilege difference from the later run.

### Profile comparison

The surviving successful profiles were separate and fresh:

```text
TestProfiles\M2_3_Client_A
TestProfiles\M2_3_Client_B
```

For each successful client, `.save` and its settings files were created only after that client's engine started. Each contains the same small generated structure:

```text
.save\app1874880_user<redacted>\settings\ReforgerEngineSettings.conf
.save\app1874880_user<redacted>\settings\ReforgerGameSettings.conf
.save\settings\customInputConfigs
```

The failed profile is:

```text
TestProfiles\M2_4A_Runner_2026-09-29_06-54-58\Client_A
```

Its `.save` directory was likewise created at runtime. It has the same app-user directory, filenames, sizes and default settings structure as the successful profiles. Generated object identifiers differ, as expected for newly written Enfusion settings. It lacks `.save\settings\customInputConfigs`, which the successful clients created only after world admission. No authentication secret, Steam ticket, identity file, backend login cache or reusable service-session file is present in any compared profile.

`Client_B` does not exist in the failed run because the runner correctly stopped after Client A's authentication block and never launched B.

Consequently, fresh-profile creation is **UNSUPPORTED** as the differentiating cause: the successful M2.3 clients also began with fresh, separate profile roots. The log statement that the `sessions` container loaded does not expose a comparable private session artifact inside these profiles.

### PowerShell history

The accessible history file was inspected:

```text
C:\Users\ruanb\AppData\Roaming\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt
```

It contains older unrelated Reforger commands and the current M2.4A runner invocations, but not the successful M2.3 commands. That is consistent with M2.3 having been launched through Codex command sessions rather than an interactive PSReadLine prompt. The equivalent read-only manual search is:

```powershell
Select-String -Path (Get-PSReadLineOption).HistorySavePath -Pattern 'ArmaReforger|ServerDiag|SteamDiag|M2_2|M2_3|-client|-server'
```

### Material differences and implications

| Item | Successful M2.3 | Failed M2.4A | Implication |
|---|---|---|---|
| Old launch artifact | Literal commands recovered from Codex session archive; no saved launcher script | Current runner script | Exact M2.3 procedure is now recoverable. |
| Process launch method | Direct `&` invocation in independent PowerShell command sessions | Failed run used `Start-Process -WindowStyle Hidden -PassThru`; runner still uses it for reliable process ownership | **CONFIRMED difference**; causal significance **UNKNOWN**. |
| Working directory | BCM Conflict Core workspace root for server and clients | Failed run used executable installation directories; runner now uses workspace root | Historical difference removed from the next run. |
| Client timing | A and B submitted concurrently; engine starts 0.779 seconds apart | A alone; B gated on A success and frontier 4 | **CONFIRMED difference**. The successful run did not prove that a lone identity-zero diagnostic client authenticates in the same way. |
| Server warm-up | About 4 minutes 18 seconds before clients | About 39 seconds; A began about one second after `SERVER_INITIAL_READY` | **CONFIRMED difference**; causal significance **UNKNOWN**. |
| Profiles | Fresh, separate A/B roots | Fresh, separate roots; B never launched | Freshness is not a material difference. |
| Executables | ServerDiag plus SteamDiag | Same binary classes | No difference. |
| Addon ID/order and address | Same addon ID/order; `127.0.0.1`; implicit port 2001 | Same | No difference. |
| Mission | M2.1 scenario | M2.4 production route/harness | **CONFIRMED expected difference**; no pre-world log evidence implicates gameplay code. |
| Steam already running | Not recorded at launch; successful SteamAPI startup proves it was available by client initialization | Explicitly running | Prior pre-launch state **UNKNOWN**. |
| Retail Reforger launched first | Not recorded | Not required by runner | **UNKNOWN**. |
| Workbench running | Not recorded; it was not the process launcher | Not recorded | **UNKNOWN**. |
| Server relative to Steam startup | Server launched first in the recorded procedure; Steam's pre-existing process state was not inventoried | Steam was confirmed running before server preflight | Exact prior ordering **UNKNOWN**. |
| Environment/AppID handling | No explicit environment assignment or AppID switch | No explicit authentication override | No evidenced difference; inherited environment values were not recorded. |
| Privilege/elevation | Codex escalation requested; OS token not logged | Codex escalation used for the run | Concrete elevation difference **UNKNOWN**. |

These findings do not establish the root cause. They narrow it to test-process launch context, client launch topology/timing, server warm-up or an unlogged external Steam/backend state. They do not implicate frontier or other project gameplay code because rejection still occurs before world initialization.

### Smallest controlled next experiment

Do not change production code and do not run this automatically. The smallest evidence-producing retry is an authentication-only reproduction that changes one variable: **client launch topology**. Keep the current runner's executables, arguments, profile freshness, working directories, server sequence and authentication policy unchanged, but submit two separate SteamDiag clients concurrently after `SERVER_INITIAL_READY`, as M2.3 actually did, rather than launching Client A alone.

This is not an M2.4 criterion-M/N acceptance run because Client B would not be a frontier-4 JIP client. Its sole purpose is to determine whether the old two-client concurrent topology changes the identity-zero authorization outcome. Stop after admission or the first reason-3 rejection and preserve both logs. If it still fails, do not iterate through the other differences locally; use the external second-machine/account retail-client acceptance path.

### Decision

The recovered primary record identifies concrete launch differences but does not prove which, if any, caused the authorization outcome. No project-code change is indicated.

The single controlled authentication-only retry described above has now been run and did not restore admission. Do not perform another local-authentication experiment. The smallest subsequent action is to run the prepared M2.4A acceptance with a genuine retail client on a second machine/account, or explicitly leave M/N pending. If an external client is admitted but the query projection fails, only then investigate the frontier replication/JIP path.

## Final auth-topology experiment — 2026-09-29

The one authorized retry was completed with `-AuthTopologyTest`; it must not be repeated as another local-authentication experiment. The normal M2.4A workflow was not executed and remains unchanged.

Preserved evidence:

```text
TestProfiles\M2_4A_Runner_2026-09-29_08-58-51
TestProfiles\M2_4A_Runner_2026-09-29_08-58-51\AuthTopologyResult.txt
```

Recovered topology reproduction:

- Client B launched at server age 257.709 seconds (target 257.705);
- Client B launch UTC `2026-09-29T08:03:09.1680944Z`;
- Client A launch UTC `2026-09-29T08:03:09.9642627Z`;
- B-to-A launch separation 796.168 milliseconds;
- matched ServerDiag/SteamDiag binaries, same mission/addon, fresh separate profiles and workspace-root context;
- B first, A launched without waiting for B authentication.

Both clients initialized Steam, signed in as the same logged account and reached transport. The server initially saw identity zero and, during overlapping reconnects, simultaneously saw provisional identities zero and one. Every connection produced the same possibly-unauthorized warning and was rejected with group 4/reason 3 approximately 0.16–0.20 seconds later.

Neither client received a logged Steam ticket response, created a player or initialized a world. The server logged no `Players connecting` or `Players connected` progression.

```text
AUTH TOPOLOGY RESULT: AUTHENTICATION_BLOCKED
M2.4 Criterion M: UNVERIFIED
M2.4 Criterion N: UNVERIFIED
```

The known M2.3 topology and timing differences were reproduced without restoring the earlier 18–20-second pending authorization or ticket/admission path. No project gameplay code is implicated. Further local-authentication experimentation is closed; use an external retail client/machine/account for M2.4 acceptance or leave M/N pending.
