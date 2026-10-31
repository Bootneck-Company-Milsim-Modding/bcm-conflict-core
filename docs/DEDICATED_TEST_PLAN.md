# Dedicated Linux / AMP candidate plan

This is a plan, not deployment approval or runtime acceptance. Target stable Arma Reforger 1.8 with matching tools/client/server patches. The Workbench-only bootstrap is not required on dedicated servers.

## Package and instance

1. Freeze a reviewed source commit and pass source/API checks plus the native compile/resource gate.
2. Package only the Addon project, with supported exclusions for unnecessary proof missions/worlds. Inspect the staged archive; exclude private settings, profiles, secrets and old build artifacts. Shared script classes/resources stay where needed by references.
3. The owner must separately authorise any stable Workshop update. Keep GUID 6A6A5973FA53A5B1, verify the current remote version before choosing a higher version, and verify test-client/server downloadability. Do not assume a fresh Workshop build exists.
4. Use a separate passworded two-player instance with the verified candidate pinned. Only BCM plus base data initially. Never reuse a live profile for destructive/fault-injection tests.
5. Review `examples/serverconfig.example.json` and replace every placeholder. Prefer AMP's managed configuration; inspect its generated JSON and effective launch arguments rather than assuming a manually edited file persists.
6. Verify the durable, writable effective `-profile` directory. BCM saves resolve to `<profile>/BCMConflictCore/Persistence`. Use distinct profiles per server instance.
7. Match allocated game/query/RCON UDP ports across AMP, OS and OVH firewall rules. Keep administrator RCON private/firewalled. Loopback is valid only if AMP can reach the same network namespace. Preserve accepted AI limits and ensure AI is not globally disabled.
8. Verify mounted addon/version, production scenario, registry READY, restore READY, one US MOB, objective publication and successful checkpoint. AMP process readiness alone is insufficient.

Scenario: `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf` (not the world GUID). Native base-game dependency 58D0FB3206B6F859 is installed server data, not an extra Workshop download.

## Two-player acceptance

- One logged-in administrator, one ordinary player. Both join US and deploy at the MOB, not forward spawns.
- Same authoritative ownership/task/pressure/HQ state on both clients; reconnect/JIP reconstructs it.
- Distinct audited backend identity bindings, separate XP and correct native ranks on spawn/respawn/rejoin.
- Native capture superiority, one committed active objective, task completion/replacement, recapture eligibility and explicit radio waiting.
- FIA/USSR operations, physical movement/combat and physical/virtual transitions without duplicated forces; separate player bubbles exercise concurrency.
- Normal native administrator chat/RCON `bcmcampaign status` and `save` dispatch; ordinary, GM-only and monitor-RCON rejection. `bcmcampaignlocal` must be denied on dedicated.
- Native GM permission behavior stays native; editor access is not BCM administrator authority.
- In a disposable campaign, verify continuous 600-second pressure, cancellation, HQ unlock, terminal victory and JIP presentation. Observe CPU/RAM, server tick/FPS, placement deferrals, query/log volume and save hitches before proposing tuning.

## Persistence acceptance — NO reset required

1. Record campaign UUID, ownership, committed objective, pressure and two distinct XP/rank values.
2. Earn/capture normally; issue an authorised explicit save and require successful coherent readback.
3. Stop gracefully through AMP, preserve logs and a private profile backup, restart with identical mod/map/config/seed/MOB and profile.
4. Require the same campaign UUID, restored ownership/objective/XP/ranks and no duplicated historical capture/reward events.
5. Test active-pressure and terminal restarts separately. Active operations/actors/casualties/buildings are not restored; do not expect frozen-world continuity.
6. Only in disposable copied profiles, test abrupt termination, missing/torn primary with a valid backup, and complete/foreign/mismatched corruption. Recover valid state or fail closed, never silently reset. Backup recovery can roll back progress; no fsync/atomic-rename guarantee is claimed.
7. Repeat restart/rejoin to check timer/subscription cleanup and identity/session-ID reuse. Do not run campaign new/confirm or delete reset markers during this plan.

Native RCON/game admin credentials are separate from join passwords; use unique values. P5 RCON audit context is shared admin/session=0, not an invented per-operator identity. Reset functionality remains a separately authorised runtime gate.

Official operator references (verify against installed versions):

- https://community.bistudio.com/wiki/Arma_Reforger:Server_Config
- https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters
- https://community.bistudio.com/wiki/Arma_Reforger:Mod_Publishing_Process
- https://github.com/CubeCoders/AMPTemplates/blob/main/arma-reforger.kvp
