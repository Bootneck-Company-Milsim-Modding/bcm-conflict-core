# Native chat restoration — BCM_CHAT_CHAIN_03

2026-10-03. SOURCE CORRECTION COMPLETE; WORKBENCH / RUNTIME GATE REQUIRED.
This report supersedes the previous chat report's unresolved first-failure finding.

## First broken link and correction

The supplied current Workbench log establishes this order:

1. Game scripts create/initialize the game.
2. `{A9ACCDC72AD70995}Configs/Core/CoresManager.conf` fails: `Unknown class 'SCR_EditorManagerCore' at offset 349(0x15d)`.
3. Native Campaign player controller and HUD root are instantiated.
4. `SCR_ChatHud`, along with numerous other native displays, fails in `SCR_InfoDisplayExtended.OnStartDraw`, line 203: null `Event_OnEditorManagerInitOwner`.

The same core-load failure occurs in the available retail console log `logs_2026-10-03_07-52-13` at 07:52:35.162 and 07:58:28.510; its script log contains the corresponding player-controller/HUD exceptions. Those retail segments used older local BCM packages (0.0.7/0.0.8). The newly supplied Workbench evidence independently demonstrates the failure in current authoring use. It is not a new dedicated-server acceptance result.

Installed vanilla `scripts/Game/Editor/Core/SCR_EditorManagerCore.c:4-5` declares:

```c
[BaseContainerProps(configRoot: true)]
class SCR_EditorManagerCore : SCR_GameCoreBase
```

BCM's diagnostic extension omitted the container attribute. Added `[BaseContainerProps(configRoot: true)]` immediately above its existing `modded class SCR_EditorManagerCore`. No permissions, editor mode selection, native methods or world managers were replaced. This restores the native declaration's config-container metadata rather than hiding the downstream null exception. The observed broken link is proven; successful construction after this correction must still be verified in Workbench.

The root configuration contains both editor core and chat-panel manager. Failure loading that configuration can prevent native cores from being constructed; the HUD also directly requires the editor core. Restoring this dependency addresses visible HUD initialization, not merely presence of a world chat entity.

## Complete installed native chain

| Item | Vanilla Conflict and current BCM |
| --- | --- |
| Chat world service | `{536D80EB494DDC43}Prefabs/MP/Campaign/CampaignMPChatEntity.et`; exactly one BCM layer instance |
| Game-mode resource | `{3B230AAC5252108F}Prefabs/MP/Modes/Conflict/GameMode_Campaign.et`; BCM campaign extensions remain unchanged |
| Player controller | `{D39FCAB93FC4FB02}Prefabs/Characters/Core/DefaultPlayerControllerMP_Campaign.et` |
| Inheritance | Campaign -> MP_Factions -> MP -> DefaultPlayerController |
| Multiplayer chat component | Native `SCR_ChatComponent` on MP controller |
| HUD manager | `SCR_HUDManagerComponent` on base controller |
| HUD root | `{3C9DCDD76AB83157}UI/layouts/HUD/HUDLayouts/HUDManager_Root.layout` |
| Chat display | One inherited `SCR_ChatHud`, show mask 119, setting `m_bShowChat` |
| Display layout | `{D89BBA18F99053D6}UI/layouts/HUD/Chat/ChatHud.layout` |
| Panel | `{28C54D576A8997CF}UI/layouts/HUD/Chat/ChatPanel.layout`, widget `ChatPanel`, native `SCR_ChatPanel` handler |
| Manager | `CoresManager.conf` -> `SCR_ChatPanelManager.OnGameStart` -> `GetGame().GetChat()` |
| Registration | Panel initialization calls `chatPanelMgr.Register(this)` |
| Update | `SCR_ChatHud.DisplayUpdate` drives `m_ChatPanel.OnUpdateChat(timeSlice)` |
| Open | `DisplayStartDraw` adds `ChatToggle`, DOWN, `Callback_OnToggleAction`; manager opens native panel |
| Send | `ChatSendMessage` -> panel callback/`SendMessage` -> `SCR_ChatComponent.SendMessage(message, channelId)` |

BCM does not override these controller or layout resources. The first difference is the BCM core declaration, not a missing chat slot or a different player controller.

### Engine registration boundary

The installed `GetChat()` accessor is native (`proto external`). ScriptedChatEntity inherits BaseChatEntity and sets INIT/FRAME events. Its installed script does not expose a registration setter or the precise native C++ registration instant. Do not invent an EOnInit registration call. CampaignMPChatEntity retains vanilla non-streamable RplComponent and channels (Global, Faction, Group, Vehicle, Local, Whisper), authored before player initialization as in Conflict. Current source is expected to register through the engine's normal lifecycle; a non-null runtime GetChat remains the required proof. GameMode_Campaign does not need a BCM registration shim.

### Slot and input boundary

The native root slot starts without display content. `SCR_InfoDisplaySlotHandler.OnStart` creates the display layout in the slot and initializes it. Therefore an early `Slot_Chat Has no Content!` is not independently proof of a different BCM layout. No clean vanilla runtime comparison was executed here. In this failing session the subsequent explicit SCR_ChatHud exception proves initialization is broken; the warning is not dismissed as a healthy final state.

`SCR_ChatPanel.Internal_Open` respects HUD-parent enable state. Native interface settings can remove the listener when chat is hidden; show mask 119 excludes pause-menu display. The installed ChatToggle input source defaults to `keyboard:KC_SLASH` and a gamepad combination; local user bindings may override it. No rebind is proposed and no input settings were changed. Missing core/HUD initialization is upstream of these gates.

Conflict mission flags 6/6 represent SpawnVehicles (2) and SpawnAI (4); GM and BCM omit them/default 0. They are not chat enable flags. Neither mission flags nor systems configuration was changed.

## Other mod evidence

Read-only installed/cache audit found concrete overlapping code/resources: SH-CORE modifies the inherited DefaultPlayerController chat show mask to 86; ArmaConflictCore extends SCR_ChatPanel.SendMessage and SCR_ChatComponent.OnNewMessage; GMTools extends OnNewMessage. These can affect final merged behavior when loaded and should be compared against an actual hosted load list, not assumed active merely because their gproj files were discovered.

The inspected retail failure segments mounted BCM, vanilla data and core. No third-party chat override is established as the cause of those failures. BCM's own SCR_EditorManagerCore extension is the relevant demonstrated declaration difference. An installed REAPER AiHelicopters extension of another config-root core preserves the same BaseContainerProps annotation, consistent with the native declaration. No external mod was edited or removed. Hosted multi-mod behavior remains unverified.

## Diagnostics and package identity

Marker: `BCM_CHAT_CHAIN_03`. Server snapshots report GetChat, concrete class, RPL role and channel count. Interactive clients separately report manager existence, its registered panel count, HUD/slot/panel existence, panel-open state and action-context activity. A read-only, correctly annotated SCR_ChatPanelManager extension exposes only its existing array count; no native method override is added.

`inputListener=NOT_EXPOSED` is deliberate: native public input APIs do not expose whether this specific callback is registered. IsActionActive is reported separately and is not misrepresented as listener proof. Actual opening/sending remains human-pending.

Snapshots poll at five-second intervals, printing only the initial snapshot or changed state; no world scan or per-frame output. Startup/end use the existing core lifecycle; if core loading fails again, missing diagnostics must be interpreted alongside CoresManager errors, not as proof of absent chat entity.

The inspected local 0.0.8 package remains stale: its manager layer differs, it lacks the chat diagnostic source, and editor-core source differs after this fix. This does not establish the current hosted version. addon.gproj has no package-version field; no unsupported field was invented. After gates pass, publishing must select a NEW unique Workshop version greater than the latest published version and preserve addon ID `6A6A5973FA53A5B1`. No package upload or version publication was performed.

## Validation and machine-readable comparison

`Tools/Test-NativeChatFullChain.py --json-only` emits a JSON comparison for all chat-critical links, the first failure, pre/post declaration metadata and runtime boundaries. Without that switch it also prints the complete underlying validation results. It reads installed archives and project files; it does not launch anything.

Validation executed: whole-tree source/override checks, 54 Game sources, 18 modded targets resolved to installed script-defined classes, installed native chat/core/HUD/input references, GUID/path references, duplicate source-chat/display checks, full mission/world/controller/layout/panel/input/send chain, and chained FIA/war/objective/radio/GM source/API/model regressions. PASS is static/source/model only. These checks are not an Enforce compiler, resource processor or runtime chat test.

Changed files in this pass:

- Scripts/Game/TB_CC_Core/TB_CC_DedicatedGMDiagnostics.c — native config-root attribute; low-noise snapshot scheduling.
- Scripts/Game/TB_CC_Core/TB_CC_ChatRuntimeDiagnostics.c — marker 03 and separate server/client read-only snapshots; native panel-count accessor.
- Tools/Test-ChatPackageRuntimeStatic.py — updated diagnostic contract and editor-core package comparison.
- Tools/Test-NativeChatFullChain.py — full installed-chain validation and machine-readable JSON output.
- Documentation/TB-Conflict-Core-Native-Chat-Restoration.md — this report.
- Documentation/TB-Conflict-Core-Chat-Package-Runtime.md — superseding-result pointer.

World resources/entities changed: NONE. Locked campaign, FIA, radio, ownership, M3, graph and RadioManager systems changed: NONE. No runtime, publication or commit performed.

## Required next gate

WORKBENCH GATE REQUIRED. Recompile normally and initialize the production world. Require absence of `Unknown class 'SCR_EditorManagerCore'` and the downstream `Event_OnEditorManagerInitOwner` HUD exceptions. Require new marker 03 and a created client panel. Then verify visible native chat and message delivery on a matching new package before accepting dedicated chat or #login/#roles. Compilation alone does not establish this runtime repair.
