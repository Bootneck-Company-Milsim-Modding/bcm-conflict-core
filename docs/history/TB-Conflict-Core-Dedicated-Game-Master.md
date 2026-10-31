# Dedicated Game Master access audit

## Verdict and boundary

Installed-source/API/static/model validation PASS. Real Workbench compile and dedicated GM interaction remain PENDING. WORKBENCH GATE REQUIRED.

The exact hosted failure is not established by the available log. The confirmed Workbench/dedicated permission difference is native: Workbench grants EDIT through the WORKBENCH flag; a dedicated player needs an active ADMINISTRATOR or SESSION_ADMINISTRATOR role (or explicit delegated EDIT permission). Being listed as an admin is login eligibility, not proof of an active role. No speculative permission grant, budget change or scenario change was made.

## Runtime evidence

Audited `C:\Users\ruanb\Downloads\console (3).log` (3 October session). The hosted configuration is `/home/amp/.ampdata/instances/BCMCommunity01/arma-reforger/1874900/Configs/serverconfig.json`; its contents and custom-mod profile configurations were not supplied.

- 19:57:08.691: JSON loader warns that array `admins` has no adequate script declaration. It does not identify the originating file. This cannot prove the native server admin array is invalid.
- 19:58:26.292: player 1 opens PHOTO; 19:58:58.659: BUILDING. Vehicle maintenance and M151A2 placement follow. No full EDIT/ADMIN session or native admin-login confirmation was found.
- BCM reports GM budget UNAVAILABLE with `no-local-editor-mode`. A headless host has no local player editor; this does not demonstrate missing per-player editor replication or exhausted budget.
- Native faction AI maximum is reported separately as -1. It is not an EDIT permission flag.

Local diagnostic JSONs have empty admins and admin passwords, but are not the hosted configuration. No hosted identity, admin membership or login state is confirmed.

## Installed native contracts

Examined installed retail data `D:\SteamLibrary\steamapps\common\Arma Reforger\addons\data\data007.pak`:

- `Configs/Core/EditorManagerCore.conf`: EDIT flags 62 include ADMIN and WORKBENCH, not DEFAULT. PHOTO includes DEFAULT.
- `Scripts/Game/Editor/Core/SCR_EditorManagerCore.c`: `OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)` maps native admin roles to ROLE modes. It creates per-player editor managers dynamically and assigns replication ownership. No authored per-player editor entity is needed.
- `Scripts/Game/Global/Functions.c`: `SCR_Global.IsAdmin` recognizes ADMINISTRATOR and SESSION_ADMINISTRATOR.
- `SCR_PlayerListedAdminManagerComponent.c`: listing permits login even before the player has logged in.
- `SCR_EditorSettingsEntity.c`: unlimited editor can be illegal for normal Conflict gameplay while admins can still access it. Do not enable unrestricted scenario EDIT for everyone.
- Native `EditorModeEdit.et` supplies free camera, content browser and placement; `Configs/Editor/ActionLists/Context/TempEdit.conf` supplies `SCR_DeleteSelectedContextAction`. Native editor lifecycle remains responsible for enter/exit; POSTGAME can block opening.

Production mission `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf` uses SCR_MissionHeaderCampaign and the native production world. A_Managers uses vanilla GameMode_Campaign.et, including its editor settings child. No missing editor service or BCM scenario-disable override was demonstrated. No world edit is required.

## Mod and budget audit

Cached sources were inspected read-only; their byte/version identity with the remote server is not proven.

| Mod | Finding |
| --- | --- |
| GameMasterEnhanced | Extends EDIT attributes/placement; no demonstrated native role-assignment override. |
| GMTools | Commands check privileged roles; does not automatically authorize every player. |
| DisableGameMasterBudgets | Runtime toggle changes placement budget enforcement/caps, not native role authorization or engine AI limits. Loading it alone does not prove the toggle disables budgets. |
| ServerAdminTools | Reads `$profile:ServerAdminTools_Config.json`; custom `admins`/`gameMasters` are maps, not the native server `game.admins` array. Configured admins can receive roles; configured GMs can receive EDIT. Actual hosted config/component activation is unknown. |
| SH-AutoAdmin (also loaded) | Uses its own profile config and role fallback. GAME_MASTER alone is not the native IsAdminRole test. Actual grant state is unknown. |

No evidenced conflict was found. The JSON `admins` warning could require examining a custom loader/config, but its source is UNKNOWN. Do not copy the native array schema into a custom map configuration or vice versa. Do not modify third-party mods.

BCM physical AI and engine/faction limits remain unchanged. GM placement budgets are separate; no budget cause for inability to obtain EDIT is demonstrated.

## Smallest manual server action

On the actual hosted server, privately verify `game.admins` contains the intended account's valid identity/platform identifier using the native array schema. Do not put identities or passwords in repository source. If listed, use native `#login`, then `#roles`, and inspect the new diagnostics before changing anything else. Alternatively a privately configured `game.passwordAdmin` permits `#login <password>`; do not share that password in logs or this repository. Configuration changes may require the host's normal restart workflow.

Official references: [server configuration](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config), [native admin commands](https://community.bistudio.com/wiki/Arma_Reforger:Server_Management).

If adminDetected=0, resolve the native login/role configuration first. If adminDetected=1 but editorPermission=0, use reason/adminEditConfigured/editorAvailable to localize the next failure rather than globally granting EDIT. If EDIT exists but opening fails, inspect current opening gates and client evidence. No runtime correction is claimed yet.

## BCM changes

`Scripts/Game/TB_CC_Core/TB_CC_DedicatedGMDiagnostics.c` adds an authority-only, five-second, changed-state diagnostic on native SCR_EditorManagerCore. It looks up each player's editor rather than the headless host's local instance. It reports numeric player ID, active/listed admin status, role flags, EDIT/ADMIN modes, settings, opening gates and player EDIT AI budget when available. No identity/ticket/password logging, permission setters, RPC, spawning or gameplay mutation. Native OnGameStart/OnGameEnd remain called; polling is removed at game end.

`Tools/Test-ThreeFactionCorrectiveStatic.py` adds installed SCR_EditorManagerCore to override-prototype inventory. `Tools/Test-DedicatedGMStatic.py` validates installed contracts, read-only behavior, permissions model, native camera/placement/delete wiring and locked-file hashes, and runs the existing FIA continuity validation.

## Acceptance matrix

| Case | Available result | Runtime boundary |
| --- | --- | --- |
| A admin access | Native permission model PASS | Dedicated interaction PENDING |
| B non-admin exclusion | Native permission model PASS; no automatic BCM grant | Dedicated interaction PENDING |
| C AI/vehicle spawn | Native tooling source verified | Placement and budget interaction PENDING |
| D delete | Native delete action resolves | Interaction PENDING |
| E enter/exit campaign | No BCM lifecycle/permission mutation | Campaign continuation PENDING |
| F Workbench retained | WORKBENCH native flag path unchanged | New compile/interaction PENDING |
| G FIA continuity | Existing static/API/model suite PASS; locked source/resource hashes unchanged through validation | Earlier changes still require Workbench/runtime gates |

Whole-tree validator chain PASS; FIA queue model PASS; campaign model covers 12 nodes/21 edges/10 ordinary positions, 390 ownership subsets and 1516 transitions. This is not an Enforce compiler or real GM session.

World entities changed: NONE. Missions/gameplay/RadioManager/FIA/radio/war/graph settings changed by this pass: NONE. No Workbench, dedicated run, publishing or commit performed.
