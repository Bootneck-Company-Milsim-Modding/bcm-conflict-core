# Native chat and administrator editor reference match

## Result

Source/reference/static/API/model PASS; Workbench/resource processing and dedicated/client interaction PENDING. WORKBENCH GATE REQUIRED. No runtime, publication or commit performed.

The previous dedicated log explicitly proves ChatEntity was unavailable in that session. The CURRENT saved production layer now contains a manually added generic ScriptedChatEntity. No new log proving that saved/published revision failed was supplied. Its generic prefab is valid native chat, also used by the full GM mode; it cannot be called intrinsically broken. Why the earlier server package lacked chat remains unconfirmed (save/package/version state must be checked after processing). The correction matches Conflict's proven service rather than guessing at input bindings.

## Installed reference comparison

References read from the installed vanilla archives:

| Item | Conflict Arland | GM Arland | BCM |
| --- | --- | --- | --- |
| Header | SCR_MissionHeaderCampaign | SCR_MissionHeader | SCR_MissionHeaderCampaign retained |
| World | worlds/MP/CTI_Campaign_Arland.ent | worlds/GameMaster/GM_Arland.ent | Worlds/Production/Arland/TB_CC_Arland.ent |
| SystemsConfig | ConflictSystems.conf | GameMasterSystems.conf | Not serialized; unchanged, not an EDIT authorization flag |
| Game mode | SCR_GameModeCampaign | SCR_GameModeEditor | Campaign retained |
| Chat | CampaignMPChatEntity GUID 536D80EB494DDC43 | GameMode_Editor_Full has generic ScriptedChatEntity child | Corrected generic sibling to CampaignMPChatEntity sibling |
| Unlimited editor legal | false/default | true in GameMode_Editor.et | Corrected true to false |
| Editable/default game flags | 6 / 6 | omitted / omitted (0 / 0 defaults) | omitted / omitted (0 / 0 defaults), unchanged |

Vanilla world files are binary resources. The audit proves Conflict contains ScriptedChatEntity and the little-endian reference to 536D80EB494DDC43; it does not pretend to fully decode its authored layer/group names. GM Arland contains SCR_GameModeEditor; its installed game-mode prefab provides the chat child. No terrain or base topology was imported.

## Exact chat correction

In `Worlds/Production/Arland/TB_CC_Arland_Layers/A_Managers.layer`, changed the existing single entity reference from `{F69BC912AC8236F9}Prefabs/MP/ScriptedChatEntity.et` to `{536D80EB494DDC43}Prefabs/MP/Campaign/CampaignMPChatEntity.et`.

Preserved its authored coordinates 2044.812 45.207 2065.896 and top-level sibling hierarchy. No second entity was added. Native campaign prefab includes RplComponent with Streamable Disabled (non-streamable, not replication-disabled), Global/Faction/Group/Vehicle/Local/Whisper channels and Global default. No component/channel/enable overrides are authored. Installed ScriptedChatEntity sets INIT/FRAME events; BaseChatEntity supplies native engine lifecycle. No custom registration code is required. Source checks establish the resource chain, not actual dedicated initialization or client JIP delivery.

Exactly one explicit production-layer chat entity remains. The base Arland terrain inheritance and all existing managers remain untouched. Remote Workshop package contents were unavailable; packaged equality and successful initialization remain a runtime gate, not a source-only PASS.

## Manual editor additions: classification and changes

- SCR_EditorCameraDefaultsComponent: retained; native camera defaults, not a permission grant.
- SCR_EditorRespawnBriefingComponent with authored UI blocks: retained; GM-specific presentation, not proof of EDIT access. No lifecycle defect demonstrated.
- SCR_RestoreEditorModesComponent: removed only the new component with ID 6A866ECD63A5E2B8. Installed GM-mode implementation saves unlimited modes and restores them with AddEditorModes(BASE,...), independently of the current admin-role path. Not required by vanilla Conflict administrator access; avoids importing saved GM permissions into this role-authorized campaign.
- Inherited SCR_EditorSettingsEntity child ID 60DEAC2AD2AE44AD: retained at its native game-mode location. Corrected m_bIsUnlimitedEditorLegal 1 to 0, matching Conflict. This property describes editor use as normal gameplay; it is not the switch that grants admins EDIT.
- Existing empty m_SkipStreamingRules override: retained; does not authorize modes.
- No m_bOverrideBaseModes enablement exists. Earlier m_BaseModes 25 was no longer in the current saved layer. 25 means EDIT + PHOTO + ADMIN; enabling a global base-mode override would grant those modes to every connecting player and is deliberately not done.

## Native authorization and tools

REQUIRED EDITOR CORE: SCR_EditorManagerCore creates per-player SCR_EditorManagerEntity, retains native RplComponent and gives replication ownership to the player. It creates modes server-side. EDIT and ADMIN are separate mode flags; native admin-role changes select ADMIN-flagged core modes. Workbench local access instead uses WORKBENCH. No authored manager/core duplicate is required.

CONFLICT-SPECIFIC: GameMode_Campaign's inherited SCR_EditorSettingsEntity keeps unlimited editor use outside normal campaign rules while allowing administrators. Native camera/browser/placing/context delete/command-waypoint resources live in EditorModeEdit and its action/registry references, not a custom BCM catalog. Source checks confirm standard placeable registries remain referenced; actual browser population/spawn/delete still need runtime proof.

GM-GAMEMODE-SPECIFIC: unrestricted GM game rules, editor respawn presentation and saved-mode restoration. BCM did not import GameMode_Editor or its mission systems configuration.

`BackendApi.IsListedServerAdmin` reports listing/login eligibility. Native logged-in ADMINISTRATOR or SESSION_ADMINISTRATOR roles satisfy SCR_Global.IsAdminRole and select EDIT/ADMIN. A listed player is not proven logged in merely by membership in game.admins. Existing read-only [TB_CC_GM] diagnostics retain role/listing/mode/manager/opening/budget evidence. They do not assert per-mode client UI availability from the headless host's current CanOpen value.

## Header flags are not permissions

Installed scripts/Game/game.c defines EGameFlags: Metabolism=1, SpawnVehicles=2, SpawnAI=4. SCR_MissionHeader defaults both editable/default masks to zero. Conflict's 6 permits/configures SpawnVehicles + SpawnAI; it is NOT EDIT/ADMIN authorization. BCM masks remain zero/default, preserving gameplay. No speculative header change was made to obtain GM access.

## Hosted server fragment

Merge into the existing `game` object, using private real values on the host only:

```json
{
  "game": {
    "scenarioId": "{A4F3500000000000}Missions/Production/TB_CC_Arland.conf",
    "admins": ["<authorized-platform-or-identity-ID>"],
    "passwordAdmin": "<private-admin-password>"
  }
}
```

This is an illustrative fragment, not a complete server configuration. No missionHeader override is required for native GM authorization. Listing permits passwordless native #login; configured passwordAdmin supports #login <password>. Then #roles reports roles, not proof that every editor UI/tool is operational. Do not commit identities/passwords. Actual AMP config and custom admin-mod configs remain unavailable.

Official references: [server configuration](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config), [server management](https://community.bistudio.com/wiki/Arma_Reforger:Server_Management).

## Mod conflict and budget boundary

No new evidence demonstrates a GameMasterEnhanced, GMTools, DisableGameMasterBudgets or ServerAdminTools conflict. Prior cached-source audit is documented in TB-Conflict-Core-Dedicated-Game-Master.md; exact hosted versions/configs remain unknown. No third-party file was modified. EDIT access must be established before testing budget-dependent placement. No AI/global budget changes.

## Portable map-services baseline

Each BCM multiplayer map must author a single native campaign chat service with replication, alongside required native campaign/faction/loadout/AI/perception/radio services. Use native Campaign game mode/editor settings and dynamically created per-player editor managers. Administrator authorization belongs in hosted config/native roles, not map-specific source. Do not add unconditional EDIT/ADMIN base modes, duplicated editor managers or GM game-mode systems merely for admin testing.

## Validation and next gate

`Tools/Test-NativeChatGMReference.py` runs the existing whole-tree static/API/FIA continuity chain and checks installed Conflict binary chat GUID, campaign replication/channels, single production chat, manager/core duplicates, production mission/world/meta GUID chain, installed header enum/defaults, and standard native registries. PASS. No Enforce compiler, package processing or interaction result is inferred.

Files changed this pass: A_Managers.layer; new Test-NativeChatGMReference.py; this report. World modifications are only the chat reference, removed restore-modes component, and unlimited-editor-legal property. RadioManager, other managers, campaign scripts/FIA/radio/graph/gameplay/header flags are unchanged.

Next: normal Workbench compile, open/save production world and process resources. Then updated dedicated/client acceptance: native chat opens/message -> #login/#roles -> EDIT/ADMIN modes -> browser -> AI/vehicle placement -> delete/waypoints -> exit/re-enter -> non-admin exclusion. If chat remains missing, compare the actual served package/resource revision with this saved layer before further edits. Do not publish automatically.
