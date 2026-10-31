# TB PvE Conflict — Local Multiplayer Testing

## Decision

Use a matched diagnostic topology for M2.4 acceptance:

```text
ArmaReforgerServerDiag.exe
  + SteamDiag Client A at revision 1
  + fresh SteamDiag Client B after authoritative frontier 4
```

This is the only investigated local topology that supplies two independent, script-running proxy worlds and allows Client B to be created after strategic state already exists. It is also the topology that produced the successful M2.3 `Players connected: 2 / 2` evidence on this machine.

Workbench Peer Tool remains useful for interactive multiplayer development. Virtual connections remain useful for replication load profiling. Neither replaces the two real proxy clients required by M2.4 criteria M and N.

## Evidence and scope

Primary official references:

- [Startup Parameters](https://community.bohemia.net/wiki/Arma_Reforger:Startup_Parameters)
- [Development Executables](https://community.bohemia.net/wiki/Arma_Reforger:Development_Executables)
- [Multiplayer Scripting](https://community.bohemia.net/wiki/Arma_Reforger:Multiplayer_Scripting)
- [Server Hosting](https://community.bohemia.net/wiki/Arma_Reforger:Server_Hosting)
- [World Editor](https://community.bohemia.net/wiki/Arma_Reforger:World_Editor)

Installed evidence was also inspected under:

```text
D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\docs
D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe
```

The public installed Enfusion API documents `Replication.SetNumVirtualConnections(int)`. The installed Workbench executable contains the corresponding `rpl-vcons` parser and PeerTool mode labels. Existing M2.3 and M2.4A logs and the recovered original M2.3 launch record provide project-specific runtime evidence.

No undocumented authentication bypass was used or added. No gameplay resource was changed.

## Official local multiplayer facilities

### `-server`

The official startup-parameter documentation defines `-server <world-or-mission>` as a local server launch. It can be combined with `-addonsDir` and `-addons`, which is the appropriate workflow for testing an unpacked Workbench addon before Workshop publication. When `-server` is used, a server JSON configuration is ignored.

The test harness uses:

```text
ArmaReforgerServerDiag.exe
-server {A242000000000000}Missions/TB_PVE_M2_4.conf
-addonsDir <server-addons>,<Workbench-addons>
-addons 6A6A5973FA53A5B1
-profile <isolated-server-profile>
-maxFPS 60
-logLevel debug
```

### `-client 127.0.0.1`

The official documentation defines `-client <IP>` as starting an RPL session in local-client mode. `127.0.0.1` is its explicit local-server example.

The test harness uses a separate profile and process for each client:

```text
ArmaReforgerSteamDiag.exe
-client 127.0.0.1
-addonsDir <client-addons>,<Workbench-addons>
-addons 6A6A5973FA53A5B1
-profile <isolated-client-profile>
-logLevel debug
```

These are real client processes. Each has its own world, RPL session, script execution and client log. M2.3 proved that two such processes can be admitted simultaneously on this machine.

### Diagnostic executable compatibility

Official Development Executables documentation states that Diag and non-Diag environments are not normally compatible. `ArmaReforgerSteamDiag.exe` is the diagnostic client and `ArmaReforgerServerDiag.exe` is the diagnostic dedicated server. This matches the successful M2.3 topology.

Mixing retail `ArmaReforgerServer.exe` with SteamDiag already produced a concrete developer-binary validation rejection in earlier M2.4A investigation. The runner therefore keeps ServerDiag and SteamDiag matched and does not disable developer-binary, version, script or resource-database validation.

## Virtual connections (`-rpl-vcons`)

### Proven behaviour

The official startup documentation states:

- `-rpl-vcons <N>` is Diag-only;
- supported range is 0 through 200;
- it sets the number of virtual connections.

The installed Enfusion Script API is more specific. `Replication.SetNumVirtualConnections(int)` is documented as setting virtual connections that are **just generating load for the master**, with an explicit warning that it is an internal profiling feature. The installed diagnostic executable also contains the matching command-line parser and logs the selected virtual-connection count.

Therefore the parameter belongs on the Diag process acting as the replication master/server. Its supported purpose is to make the master build/process replication traffic for synthetic connections and profile server load.

### Capability matrix

| Capability | Finding | Evidence level |
|---|---|---|
| Create server-side replication connection load | Yes | Explicit installed API documentation |
| Cause the master to produce replicated streams/state | Yes, for load generation | Explicit purpose plus replication-master terminology |
| Create a separate client OS process | No | Virtual connections exist inside the master process |
| Create an independent client world | Not evidenced; do not treat as present | API says load-only; no client-world API or log surface documented |
| Run a separate copy of game scripts per virtual connection | Not evidenced | No independent script VM/world is documented |
| Instantiate normal player controllers | Not evidenced | Load-generation API makes no player/controller guarantee |
| Provide authenticated player identities | Not evidenced | No backend/authentication identity contract documented |
| Provide client-side public-query output | No usable independent output surface was found | No separate client log or world |
| Control an individual connection's later join time | Not documented | CLI sets a count; no per-connection lifecycle interface found |
| Prove fresh JIP initialization | No | No independently created late client consumer |
| Exercise Steam/backend player admission | No evidence | Profiling feature, not an authenticated player-client facility |

### M2.4 suitability

Virtual connections can support load/performance regression work: stream production, replication scheduling pressure and master-side scalability. They cannot, on current evidence, prove that a real proxy world consumed the frontier projection, ran the M2.3 query contract, or initialized directly from a pre-existing frontier.

Consequently:

- Criterion M: **insufficient evidence**.
- Criterion N: **insufficient evidence**.

The harness does not substitute virtual connections for Client A or Client B.

## Workbench Peer Tool

The official World Editor documentation exposes two relevant play modes:

- **Server localhost** — starts the game with a background server;
- **Server localhost + PeerTool** — starts a background server and local clients created from PeerTools settings and connects them to that server.

The Development Executables documentation separately identifies Workbench Game and Peer Tool as debug targets. The installed Workbench binary contains `Server localhost + PeerTool` and `PeerTool` labels, confirming that the facility is present in this installed version.

### Capabilities

Peer Tool is an official interactive way to create local client peers under World Editor control. Unlike `-rpl-vcons`, the documentation calls them clients rather than load-only virtual connections. This makes Peer Tool appropriate for rapid locality checks, RPC/replication debugging and interactive multi-peer development of a world loaded in Workbench.

### Limitations for this acceptance

No official or installed documentation found during this investigation establishes that Peer Tool:

- launches the production dedicated-server executable and scenario invocation used by M2.4;
- exposes a stable command-line automation contract for the current PowerShell runner;
- can delay one selected peer until frontier 4 while another remains connected;
- creates a fresh peer on demand after the server has already advanced;
- avoids all Steam/backend identity behaviour relevant to normal player admission;
- emits per-peer isolated logs containing the existing `[TB_PVE_M2_4A]` query snapshots.

The World Editor mode describes clients being created from PeerTools settings when the combined play mode starts. That is not proof of controllable late JIP. Workbench's own binary also states that using Workbench itself as a replication client is unsupported; Peer Tool must therefore be treated as its dedicated editor facility, not as evidence that the editor host is interchangeable with a standalone SteamDiag client.

The current desktop automation environment could inventory running applications but could not launch native apps, so the Workbench settings UI was not altered or experimentally executed. The documentation and installed-binary findings are sufficient to establish existence, but not a stronger JIP contract.

Peer Tool is therefore recommended for developer-facing smoke tests, not as the sole M2.4 M/N acceptance mechanism.

## Multiple SteamDiag instances

### Supported elements

The supported primitives are official:

- local server through `-server`;
- loopback RPL client through `-client 127.0.0.1`;
- matched diagnostic server/client binaries;
- independent `-profile` paths;
- local addon roots through `-addonsDir` and `-addons`.

M2.3 supplies direct runtime proof beyond the generic documentation:

- one `ArmaReforgerServerDiag.exe`;
- two `ArmaReforgerSteamDiag.exe` processes;
- two separate fresh profiles;
- two simultaneous proxy/player admissions;
- independent client query snapshots;
- server result `Players connected: 2 / 2`.

The recovered launch record shows that every process used the BCM Conflict Core workspace as working directory. The two clients were submitted concurrently, rather than waiting for Client A to authenticate first. Both used the same logged Steam account context.

### Authentication implications

The official documentation found no test-identity, offline player-identity or duplicate-local-client authentication switch. `-noBackend` exists, but it disables backend HTTP communication; it is not documented as an authenticated multiplayer-test identity mechanism and is not used. Validation-disable flags likewise do not create legitimate identities and are not used.

The M2.4A Steam-running failure is therefore still classified correctly as `AUTHENTICATION BLOCKED`, not frontier replication failure. It does not disprove the local topology, because M2.3 admitted two identical binary-class clients on this host. It does show that local duplicate diagnostic admission can depend on engine/backend launch state that is not fully explained by available logs.

## Chosen topology

For automated M2.4 proof, retain:

1. one ServerDiag production-route server;
2. one fresh SteamDiag Client A launched after positive server readiness and a bounded settle interval;
3. Client A must be admitted, enter its proxy world and report the revision-1 query-contract snapshot;
4. one legitimate live frontier transition must be observed by Client A;
5. the authoritative harness advances to and holds frontier 4;
6. only then launch a fresh SteamDiag Client B;
7. Client B must initialize directly at the current revision/frontier;
8. compare A/B semantic snapshots if A remains connected.

This uses real proxy worlds for both consumers. It preserves the distinction between authentication failure and project-network failure.

## PowerShell architecture

The command remains:

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1
```

Dry run remains:

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1 -DryRun
```

The runner now reflects the recovered successful M2.3 process context:

- matched ServerDiag/SteamDiag defaults;
- workspace root as the working directory for every process;
- isolated server, Client A and Client B profiles;
- configurable `ServerSettleSeconds`, default 45 seconds, after the positive server-ready marker;
- real admission/world/query markers rather than process existence;
- Client B launched only after authoritative frontier 4;
- owned-process cleanup only;
- preserved logs and independent result fields;
- immediate `AUTHENTICATION BLOCKED` classification for reason 3, identity-zero rejection or SteamAPI initialization failure;
- no authentication bypass, firewall change or credential manipulation.

The 45-second default reproduces the observed interval between the comparable late M2.3 server hold marker and the successful client launch. It is bounded and configurable; it is not claimed as a proven authentication fix.

## What the harness proves

When it passes, the harness proves:

- a production-style dedicated authority started the production route;
- a real remote/proxy client world initialized at revision 1;
- Client A consumed correct legal-objective/frontier state through the public M2.3 query contract;
- Client A received a live authoritative revision change;
- the server reached an authoritative frontier-4 state;
- a separately created fresh Client B joined after that state existed;
- Client B initialized directly from current state without historical replay;
- A/B convergence, when both remain connected.

## What it cannot prove

Even a passing one-PC run does not prove:

- WAN routing, NAT or internet latency behaviour;
- cross-machine installation/Workshop distribution;
- different-account backend policy;
- retail/non-Diag compatibility, because the development environment intentionally uses matched Diag binaries;
- deployment UI interaction by a human;
- general load scalability beyond two clients.

Virtual connections may supplement the final item, but not replace the two proxy clients.

## M2.4 acceptance mapping

### Criterion M

PASS only when Client A:

- is genuinely admitted and enters online world state;
- reports `READY`, `VALID`, revision 1 and the correct initial frontier/legal objectives;
- receives a later valid frontier revision without reconnecting.

A virtual connection, process existence or transport-only identity is insufficient.

### Criterion N

PASS only when fresh Client B:

- is launched after authoritative frontier 4 already exists;
- is genuinely admitted and enters online world state;
- initializes directly at the current non-revision-1 state with matching legal objectives.

An editor peer created at server startup or a load-only virtual connection is insufficient.

## Long-term regression workflow

Use three layers:

1. **Fast editor development:** Workbench Game/Peer Tool for interactive locality, RPC and obvious replication defects.
2. **Load profiling:** ServerDiag with `-rpl-vcons` for master-side replication cost and stream-pressure measurements, with no player/JIP claims.
3. **Acceptance regression:** the PowerShell ServerDiag + SteamDiag A + delayed SteamDiag B harness, requiring real admission and semantic query snapshots.

If local SteamDiag authentication is blocked before world entry, preserve logs and leave M/N `UNVERIFIED`. Do not modify gameplay or disable authentication. A retail second-machine/account run is then a fallback for that blocked acceptance execution, not the default test architecture.

## Final recovered-topology authentication experiment

Exactly one authentication-only experiment was run on 2026-09-29 through:

```powershell
.\Tools\Run-M2_4A-NetworkAcceptance.ps1 -AuthTopologyTest
```

This was not M2.4 acceptance. It used the same ServerDiag mission/addon configuration, fresh separate SteamDiag profiles, workspace-root launch context and recovered M2.3 timing. Logs and the generated summary are preserved under:

```text
TestProfiles\M2_4A_Runner_2026-09-29_08-58-51
```

### Reproduced timing

- server launch UTC: `2026-09-29T07:58:51.4588567Z`;
- target server age: 257.705 seconds;
- actual server age at Client B launch: 257.709 seconds;
- Client B launch UTC: `2026-09-29T08:03:09.1680944Z`;
- Client A launch UTC: `2026-09-29T08:03:09.9642627Z`;
- actual B-to-A process-launch separation: 796.168 milliseconds.

The timing therefore reproduced the known-good 257.705-second server age to within 0.004 seconds and the observed 779-millisecond M2.3 engine-start separation to within approximately 17.2 milliseconds. B was launched first and A was launched without waiting for B authentication.

### Client B result

1. 09:03:10.468 — Steamworks startup logged successfully.
2. 09:03:12.367 — account `TacticalBootneck` signed in.
3. 09:03:12.868 — server authenticated/connected the first transport as provisional identity zero.
4. 09:03:12.898 — server logged the possibly-unauthorized warning.
5. 09:03:12.965 — Client B logged transport connection as identity zero.
6. 09:03:13.065 — server rejected identity zero with group 4/reason 3, 0.167 seconds after the warning.
7. 09:03:13.166 — Client B observed disconnection.

No Steam ticket response, player creation or online-world initialization was logged. Engine-level reconnects began automatically, but no operator retry was made and no reconnect progressed beyond provisional authorization.

### Client A result

1. 09:03:11.261 — Steamworks startup logged successfully.
2. 09:03:13.160 — account `TacticalBootneck` signed in.
3. During overlapping reconnects, the server held Client B as provisional identity zero and assigned Client A provisional identity one at 09:03:13.668.
4. 09:03:13.698 — server logged the possibly-unauthorized warning for the second simultaneous slot.
5. 09:03:13.759 — Client A logged transport connection; its local log reports provisional identity zero.
6. 09:03:13.867 — the server rejected provisional identity one with group 4/reason 3, 0.169 seconds after the warning.
7. 09:03:13.958 — Client A observed disconnection.

No Steam ticket response, player creation or online-world initialization was logged.

### Server result and classification

The server reached two simultaneous provisional slots during the reconnect overlap (`identity=0` and `identity=1`), proving that both client processes were active concurrently. Every observed connection produced the possibly-unauthorized warning and was rejected approximately 0.16–0.20 seconds later with group 4/reason 3.

No `Players connecting` or `Players connected` progression was logged. Neither client progressed beyond provisional identity.

```text
AUTH TOPOLOGY RESULT: AUTHENTICATION_BLOCKED
```

Compared with successful M2.3, server age, launch order, close launch spacing, matched binaries, fresh profiles and workspace-root context were reproduced. The decisive difference remained after the same warning: M2.3 held both connections pending for approximately 18–20 seconds, received ticket responses and admitted both players; this run rejected both in under 0.2 seconds and received no ticket response.

The recovered launch topology therefore does not restore authentication in the current environment. Stop further local-authentication experiments. Leave M2.4 M/N `UNVERIFIED` and use an external retail client/machine/account for acceptance, or explicitly retain M/N as pending.
