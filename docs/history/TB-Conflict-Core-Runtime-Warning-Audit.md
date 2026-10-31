# Runtime error and warning audit — 2026-10-03

Result: one BCM-owned missing resource reference corrected. Game source and the accepted chat/GM architecture are unchanged. Resource processing/runtime confirmation of the correction remains pending.

## Evidence boundary

Primary local Workbench evidence: `C:/Users/ruanb/Documents/My Games/ArmaReforgerWorkbench/logs/logs_2026-10-03_09-23-41/console.log`, specifically the successful post-correction session beginning at 10:02:32 (Europe/London). Earlier 09:29 entries in that same log precede the accepted chat repair and must not be treated as current failures.

Additional retail evidence: `C:/Users/ruanb/Documents/My Games/ArmaReforger/logs/logs_2026-10-03_10-06-24/console.log` and `script.log`. The loaded-addon block at console lines 801–804 names core, vanilla data and BCM only. Available-addon listings and mission-header discovery are not evidence that other mods were mounted. Workbench's loaded-addon block likewise identifies only those three projects. No new session was launched for this audit.

Chat is accepted from the user's confirmation and current logs: Workbench at 10:02:35.703 and retail at 10:06:54.345 report marker BCM_CHAT_CHAIN_03, getChat=1, panelManager=1, registeredPanels=1, chatHud=1, slotChat=1, panel=1, managerChat=1. Panel-open transitions are present in both. GM works by the Creative Director's manual confirmation. Neither architecture was reopened.

## A. Empty CreateInstanceFromPrefab reference

Classification: BCM-CAUSED configuration omission in a retained vanilla component.

Workbench console line 2264 at 10:02:34.911 reports the empty resource error in the GAME transition. Installed `scripts/Game/Caches/SCR_CacheManagerComponent.c:36–52` provides the matching startup path:

`OnGameModeStart -> server check -> GetSpawnRandomCaches -> CreateInstanceFromPrefab(m_sTerrainConfig, true)`.

The field is `SCR_CacheManagerComponent.m_sTerrainConfig`. Its attribute has no default resource. The inherited native component in `Prefabs/MP/Modes/Conflict/GameMode_Campaign.et:45–48` has ID `{691E6E472CBA6978}`, a valid cache prefab and `m_iAmount 0`, but supplies no terrain config. Vanilla Arland's world supplies the map-specific override. BCM's production layer enables `m_bSpawnRandomCaches 1` but omitted that override. The current local retail package independently has enabledCaches=True and terrainConfigPresent=False.

This is a source/configuration trace correlated to startup timing; the original error itself does not include a stack trace. Other native callers were inventoried. Arsenal/identity/notification loads occur during their earlier initialization and have their own failure messages; this single error at OnGameModeStart matches the retained cache component's empty field and server-only gate. No BCM script calls CreateInstanceFromPrefab.

Correction: override only `m_sTerrainConfig` on the existing component with:

```text
{8E573788A979EBAB}Configs/Campaign/CampaignCachePool/CampaignCachePool_Arland.conf
```

The target exists in installed vanilla data007.pak as SCR_CacheTerrainConfig. Its GUID was checked against the read-only installed resource database and the same binary reference in vanilla `worlds/MP/CTI_Campaign_Arland.ent` in worlds001.pak. No new GUID/config was invented.

Before correction the optional cache pool remains empty; its failed load returns null and the spawn method exits safely. It does not explain chat/GM failures or campaign ownership behavior. The existing inherited amount is zero and remains zero. Adding the valid pool therefore introduces no cache actors, count, supply or gameplay change. No global error suppression or custom lifecycle method was added. Runtime absence of this error after resource processing is not yet claimed.

## B. EditBoxFilterComponent warning

Classification: VANILLA.

Retail console lines 935–937 and Workbench lines 2265–2267 associate the warning with native ChatHud.layout creation. That layout inherits `ChatPanel.layout`; its valid `EditBoxWidgetClass`, named `m_MessageEditBox` (lines 156–169), contains EditBoxFilterComponent `{585D2E6C9661FDE8}`. These resources are in vanilla data010.pak. BCM has no replacement layout or EditBoxFilterComponent implementation.

Installed `scripts/Game/UI/Components/EditBoxFilterComponent.c:68–75` casts to EditBoxWidget, then to MultilineEditBoxWidget only if the first cast fails. Its guard is:

```c
if (!m_wEditBox || !m_wMultilineEditBox)
```

A valid single-line widget has a null multiline pointer and therefore triggers the warning. This is a native boolean-guard defect, not evidence that BCM authored an invalid chat widget. Initialization of blacklist/whitelist arrays after the guard is skipped; filtering semantics may be affected, although current chat interaction works. Do not label all effects harmless merely because it is a warning.

The same warning occurs in this retail log at 10:06:36.662 while only vanilla/core are loaded, before BCM is mounted at 10:06:45.239. No separate vanilla Conflict play session was performed, so a clean Conflict runtime reproduction is not claimed; the unchanged native chat resource and script establish the same conditional path. No BCM fix is appropriate under the requested ownership boundary.

## C. ToolMenu unknown `name`

Classification: VANILLA resource error.

Owner: `UI/layouts/Map/ToolMenu.layout`, vanilla data010.pak, resource `{2F8AEE78FF5A8104}`. BCM has no override. At line 80, the `Action` RichTextWidget's inline gamepad image text contains an unescaped quote after the imageset path:

```text
Text "<image set=\"{F7FD1672FECA05E8}UI/Textures/Icons/icons_gamepad_64.imageset" name=\"DPAD_horizontal\" scale=\"1\"/>"
```

That quote ends the serialized text early, leaving `name` for the resource parser to read as a keyword. This directly explains the logged parser error. An engine/addon version mismatch is not established as its cause; the malformed text exists in installed native content used by this build.

Retail lines 1042–1049 show RespawnMenu -> MapMenu -> ToolMenu -> error, followed by continuing RespawnMenu widget creation. The affected resource includes the map tool strip and controller action hint. Successful continuation does not prove every map tool/hint works; detailed map interaction remains unverified. No vanilla replacement was created.

## D. Duplicate admin commands and notifications

`ban`/`kick`: VANILLA registrations / UNKNOWN exact competing registrant.

Workbench records these messages at 10:02:32.283 and again after reload at 10:02:57.969. Installed `scripts/Game/Network/ServerCommands/BanCommands.c` declares BanCommand:GetKeyword() returning `ban` at lines 432–434 and KickCommand:GetKeyword() returning `kick` at lines 579–581. BCM declares no ScrServerCommand subclass, keyword implementation or command registration. Neither ServerAdminTools, GMTools nor GameMasterEnhanced is mounted in this inspected session. The warnings do not occur in the inspected retail log.

The error text is generated by native registration; installed script source and the log do not name the already-registered competing handler. A native built-in overlap or Workbench registration/reload issue is possible, but neither is proven. No exact two-handler pair can responsibly be reported from this evidence. No BCM command modification was made.

Duplicate editor notification keys: VANILLA, exact duplicate owners identified in a single config.

Both pairs exist in installed `Configs/Notifications/Notifications.conf` itself:

| Key | First entry | Second entry |
| --- | --- | --- |
| EDITOR_PERCEIVED_FACTION_PUNISHMENT_KILLING_SET | line 2427, perceived-faction punishment entry | line 2463, entry whose UI name is EDITOR_VEHICLE_SALVAGE_ENABLED |
| EDITOR_PERCEIVED_FACTION_TYPE_DISABLED | line 2382, perceived-faction disabled entry | line 2472, entry whose UI name is EDITOR_VEHICLE_SALVAGE_DISABLED |

The second pair's identifiers appear incorrectly reused for vehicle-salvage notifications. Both are within the same native addon; no cross-mod pair is required to produce the warning. BCM defines none of these entries. Notification presentation may be ambiguous; leave native data unchanged.

## Remaining classifications

| Message | Classification | Evidence / effect boundary |
| --- | --- | --- |
| SlidingTrackMaterial | VANILLA | Installed M151A2.et line 548 contains the field; Workbench rejects it during native prefab load. No BCM override. Underlying schema/field support issue; skid-material behavior not verified. |
| Vehicle `Parent` keywords | VANILLA | Installed BRDM2_base.et lines 1044/1068 and BTR70_Base.et lines 1273/1307/1340/1373 contain differential Parent fields matching the logged loads. No BCM vehicle override. Ignored fields may affect vehicle behavior; not tested here. |
| Image-set texture-path warning | VANILLA caller/config path; exact runtime caller UNKNOWN | SCR_UIInfo.GetIconPath emits it for imagesets. Native deploy UI uses LoadoutIcons.imageset. No BCM GetIconPath call was found. A full caller stack is absent; possible missing icon presentation, not a proven gameplay failure. |
| SocialComponent::IsRestricted invalid player ID | UNKNOWN exact caller; native API/call sites identified | No BCM direct call found. Vanilla SCR_AIGroup getters and map-marker/group UI perform these checks. Log lacks the offending ID/caller stack, so BCM-created group involvement cannot be ruled in or out. No permission or player-identity mutation is justified. |
| ArlandRasterized.edds / Arland.edds fallback | VANILLA terrain presentation dependency | Inherited Arland terrain requests these defaults; native MapEntity remains intact. Raster fallback is acknowledged. Actual minimap appearance remains a human presentation check. No replacement textures added. |
| Slot_* Has no Content | EXPECTED initialization in the observed healthy chain | Native root slots are created before their display content; slot handler then creates layouts. The subsequent panel=1/registeredPanels=1 and open transitions prove Slot_Chat was populated in this session. Other slots' final state is not inferred from chat alone. |
| Journal config path is empty | UNKNOWN exact selected component; BCM-added briefing is a supported contributor | SCR_MapJournalUI.Init calls SCR_RespawnBriefingComponent.GetInstance().LoadJournalConfig; empty path returns false and the optional journal UI exits. Native Campaign's base briefing has JournalConflict.conf, while BCM's additional EditorRespawnBriefingComponent (a subclass) has no journal path. FindComponent selects by base type; without runtime component identity, exact selection is not proved. Journal functionality is unavailable on that path; no journal requirement or campaign failure is established. Left unchanged rather than removing accepted editor briefing or suppressing the warning. |
| native-component-deleted / M3-suspended during reload | EXPECTED/TEARDOWN | Correlates with Workbench Reload Game and entity destruction; normal cleanup, not an operation failure. |

No third-party-caused issue is established in these loaded sessions. Cached or available mod names are not used to assign blame.

## Change and validation

Files changed:

- `Worlds/Production/Arland/TB_CC_Arland_Layers/A_Managers.layer`: one resource field on the existing GameMode_Campaign cache component.
- `Tools/Test-NativeChatFullChain.py`: validate the Arland cache resource, installed world GUID, matching native component ID and retained zero count, alongside the existing full-chain suite.
- `Documentation/TB-Conflict-Core-Runtime-Warning-Audit.md`: this report.

World entities added/deleted/moved: zero. One existing entity's inherited component resource is configured. No Game script, chat/GM component, faction, graph, RadioManager, capture, spawn policy, war, objective or FIA behavior was edited.

Whole-tree static/API checks, resource-reference checks, chained campaign regressions, FIA continuity, chat chain and GM validation: PASS. These are source/model checks. Existing manual compile PASS remains accepted; no fresh Workbench compile or resource processing was performed by Codex. No game process, publishing or commit was initiated.

WORKBENCH GATE REQUIRED because the production layer resource changed. Process/save the production world normally and initialize it once. Verify the empty CreateInstanceFromPrefab error is absent and the accepted chat/GM and campaign behavior remain healthy. Remaining vanilla/unknown warnings are documented above; their disappearance is not a pass criterion for this BCM correction.
