# Chat package versus runtime investigation

## Findings and limits

The reported NEW live chat failure is accepted. Its first failing lifecycle step is NOT yet established because a fresh dedicated/client log for that revision was not supplied. The old missing-ChatEntity warning is not reused as proof of the new failure. No world/prefab/editor change was made in this pass.

## Source production chain

Mission metadata maps A4F3500000000000 to Missions/Production/TB_CC_Arland.conf. Its World points to `{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent`. World metadata preserves A4F3300000000000. The world SubScene inherits worlds/Arland/Arland.ent; authored child layers are in its corresponding TB_CC_Arland_Layers directory. A_Managers.layer is the relevant serialized production manager layer (and is present at that exact path in the inspected package).

Exact source record:

```text
ScriptedChatEntity : "{536D80EB494DDC43}Prefabs/MP/Campaign/CampaignMPChatEntity.et" {
 coords 2044.812 45.207 2065.896
}
```

Exactly one explicit production-layer chat record exists. The installed GUID is also referenced by vanilla Conflict Arland's binary world. The resolved prefab has RplComponent with Streamable Disabled and native channels. No disabled/editor-only/deletion/component overrides are authored on this record. No source-layer exclusion was found; no base/derived chat deletion override is demonstrated. Source alone does not prove processed resource inclusion or actual initialization.

## Actual accessible package

Read-only inspection of `C:\Users\ruanb\Documents\My Games\ArmaReforger\addons\BCMConflictCore_6A6A5973FA53A5B1\data.pak`, cached version 0.0.8, found:

| Resource | Comparison |
| --- | --- |
| Production mission | Exact byte match |
| Production .ent entry | Exact byte match |
| B_MOB.layer | Exact byte match |
| C_CapturePoints.layer | Exact byte match |
| A_Managers.layer | DIFFERENT |

Packaged A_Managers has one generic `{F69BC912AC8236F9}Prefabs/MP/ScriptedChatEntity.et`, plus earlier unlimited-editor/m_BaseModes settings. It does NOT contain the intended CampaignMPChatEntity reference. The package includes the layer, so omission of that layer is rejected for THIS archive. Mission/world-name equality alone is insufficient evidence of manager-layer freshness.

Source A_Managers SHA256: `97d2d4423169c9d25b6316084ccab2cf358185a72de562ef39cb8b9f4bd88117`.
Package A_Managers SHA256: `71dce7aad0014a5ab310756f28058036e9fee17523b9e6d22be6d13906b9417d`.

This proves the local cache differs from current source. It does NOT prove the hosted server uses this exact cache/revision. The generic prefab is itself valid native chat; this mismatch is not proof that it caused absent UI. The hosted resource database/package was not available and was not fabricated/rebuilt. No package/evidence was deleted or overwritten.

## Native registration and client path

CampaignMPChatEntity is a resource instantiated as ScriptedChatEntity, deriving from generated native BaseChatEntity. ScriptedChatEntity sets INIT/FRAME events. Installed script has no custom registration call or mandatory name/tag; its vanilla Conflict instance is unnamed. The native engine exposes active service through `ChimeraGame.GetChat(): BaseChatEntity`. Registration/arbitration internals are engine-owned and unavailable in Game script source; timing requirements beyond normal native entity initialization cannot be invented.

SCR_ChatPanelManager.OnGameStart caches `ScriptedChatEntity.Cast(GetGame().GetChat())`; SCR_ChatPanel initializes the same service reference. Its opening gate also rejects an unavailable/disabled HUD parent; it depends on chat channels, SCR_ChatComponent and registered input listeners. Thus a registered authority service is not proof of client replication, HUD initialization or input success. If server registration passes but client does not, inspect replication/client initialization. If client registration passes, inspect ChatPanelManager/ChatPanel/HUD/component/channel/opening gates before adding any world entity.

No BCM startup deletion/replacement of chat was found. Lifecycle instrumentation below will expose construction/destruction rather than assume service persistence.

## Text-chat controls and mods

Installed SCR_DSConfig and current official server configuration expose VONDisableUI and VONDisableDirectSpeechUI for voice UI, not a general text-chat-disable JSON field. No text-chat-disable field was found in that schema; do not import Arma 3 disableChannels settings. The Reforger diagnostic menu has a separate Disable chat control, but its current client state is unknown; no evidence establishes it as this failure's cause.

References: [server schema](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config), [Reforger diagnostic menu](https://community.bistudio.com/wiki/Arma_Reforger:Diag_Menu).

Read-only scan of 32,023 cached .c/.et/.conf resources found concrete chat overrides:

- GMTools: SCR_ChatComponent.OnNewMessage calls super first, then message forwarding/logging. No chat-service removal or open-UI suppression demonstrated.
- ArmaConflict_Core: overrides SCR_ChatComponent.OnNewMessage and SCR_ChatPanel.SendMessage, including custom command handling. It is an installed cache; its presence in the inspected old server log was not found. Loading in the NEW server remains unknown. The override is evidence of modified message behavior, not evidence of failure to open chat.

No matching chat entity/panel overrides were found in the cached ServerAdminTools/GameMasterEnhanced resources under the scan's specified class/resource patterns. This bounded scan is not proof that every indirect UI/input override is absent. No third-party source was changed and no mod is accused of causing this failure.

## Added diagnostics

The first version was rejected by real Workbench: `Engine class 'BaseChatEntity' cannot be modded.` Its native/generated declaration does not expose permission to modify engine implementation; a declaration's presence is not proof of moddability. The invalid modded class and its constructor/destructor hooks have been removed.

`TB_CC_ChatRuntimeDiagnostics.c` now uses a BCM-owned observer. Installed BaseWorld.GetBoundBox and QueryEntitiesByAABB with ALL flags collect entities castable to BaseChatEntity. The native registered ChimeraGame.GetChat reference is included separately even if absent from spatial results. Count is explicitly SPATIAL_PLUS_REGISTERED_LOWER_BOUND: unregistered non-spatial entities can be missed; zero is not exhaustive proof that no entity exists anywhere. It reports server/console context, world/name, verified BaseChatEntity/ScriptedChatEntity type family, RplComponent and Role(), channel count and registration identity. Full derived-class names are not claimed. Enabled remains NOT_EXPOSED; consoleApp is not a dedicated-session assertion. No custom registration or engine-class extension remains.

Existing native core OnGameStart schedules bounded snapshots at 1 and 10 seconds on server AND client; OnGameEnd removes pending callbacks. Construction/destruction instrumentation is DROPPED. Snapshot differences can show observed presence/registration changes, not exact destruction timing or cause. No per-frame polling, credentials, message text, ownership/RPC/spawn/registration setters.

Manually controlled marker: `[TB_CC_BUILD] buildTag=BCM_CHAT_CHAIN_02 chatFixRevision=3 world=...`. It proves the corrected observer script revision when present; it does NOT by itself prove the world/layer package matches. Correlate with entity/registration diagnostics and package hashes.

Reasons:

- CHAT_SERVICE_MISSING: no engine registered reference; compare tracked count.
- CHAT_ENTITY_EXISTS_REGISTRATION_MISSING: native entity constructed but engine lookup absent.
- CHAT_SERVICE_REGISTERED_UI_NOT_VERIFIED: service registered; move to client/HUD checks, not another prefab.

## Validation and acceptance

`Tools/Test-ChatPackageRuntimeStatic.py` chains whole-tree/native/FIA checks, verifies installed diagnostic APIs and prints deterministic mission/world/layer package SHA256 comparisons. Optional `--package <path-to-data.pak>` permits inspection of an actual supplied server archive. It never launches, publishes or writes packages.

Static/API/reference validation PASS. Package comparison correctly reports DIFFERENT rather than claiming freshness. Not an Enforce compiler.

| Criterion | Verdict |
| --- | --- |
| A source correct campaign chat | PASS, source only |
| B package correct campaign chat | FAIL for inspected local 0.0.8; actual hosted revision UNVERIFIED |
| C dedicated startup count=1 | UNVERIFIED |
| D client usable service | UNVERIFIED |
| E native chat UI opens | User reports failure; first lifecycle cause UNVERIFIED |
| F normal message | UNVERIFIED |
| G #login | BLOCKED by chat |
| H #roles | BLOCKED by chat |

Files changed this pass: new TB_CC_ChatRuntimeDiagnostics.c; existing TB_CC_DedicatedGMDiagnostics.c only to schedule/cancel chat snapshots; new Test-ChatPackageRuntimeStatic.py; this report. World entities/properties changed: NONE. GM architecture/permissions and locked campaign systems remain unchanged. FIA continuity/static models PASS.

Correction validation: whole-tree override/API/static/FIA/native-reference chain PASS. All 17 remaining modded declarations resolve to installed script-defined rather than generated-only engine classes. No other demonstrated engine-class extension was found. Added conservative regression check rejects unresolved/generated-only targets; it is not a substitute for the compiler's hidden rules. New observer introduces no overrides/callbacks beyond the verified QueryEntitiesCallback `bool (IEntity)` signature and existing compiled startup integration. Package comparison continues to show the same local cache mismatch, not a runtime PASS.

Files changed in this compile-correction pass: TB_CC_ChatRuntimeDiagnostics.c, Test-ChatPackageRuntimeStatic.py, this report. World/GM/campaign resources changed: NONE.

Historical gate above used BCM_CHAT_CHAIN_02. Superseded on 2026-10-03 by `TB-Conflict-Core-Native-Chat-Restoration.md`: current Workbench evidence proves CoresManager.conf fails on SCR_EditorManagerCore before the chat HUD can initialize. The narrow native config-root declaration correction and BCM_CHAT_CHAIN_03 now require a new WORKBENCH GATE. See that report and Test-NativeChatFullChain.py for full-chain evidence; do not treat older package/diagnostic results as current runtime acceptance. No automatic runtime, publication or commit performed.
