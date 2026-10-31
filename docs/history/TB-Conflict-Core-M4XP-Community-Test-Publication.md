# M4.XP unlisted community-test publication preflight

Status: **SOURCE/RESOURCE PREFLIGHT PASS; WORKSHOP BUNDLE INSPECTION PENDING. DO NOT UPLOAD YET.** This preparation changes no gameplay or production resource.

## Exact identity and server fields

- Existing project/addon GUID: `6A6A5973FA53A5B1` (`addon.gproj`). Reuse this published project; do not create a second addon.
- Production mission metafile: `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf`. This **16-hex-digit** GUID is the server-ready `scenarioId`; the shortened `{A4F3500000000}` spelling in the request is not the resource identity.
- World metafile: `{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent`; the mission points to exactly this world.
- AMP/server `modId`: `6A6A5973FA53A5B1`.
- AMP/server `scenarioId`: `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf`.
- No other Workshop mods are required by the current `addon.gproj`. Its sole declared project dependency, `58D0FB3206B6F859`, is vanilla Arma Reforger data, supplied by the game/server installation; do not add it to the server's downloadable `mods` list.

The earlier M3.7 Workshop upload is evidenced by the 2026-09-30 dedicated log/documentation: the server downloaded and mounted this same addon GUID at version `0.0.1`. That establishes **previously published**, not the current remote version or visibility. `addon.gproj` has no version field. Confirm the live current version and visibility in the stable Workbench Publisher before choosing an update version. Proposed display name: `BCM Conflict Core [TEST]`; proposed visibility: **Unlisted**. Do not silently treat the historical `0.1.0` release-plan text as the actual published version.

## Current production-content audit

The full native candidate is `Missions/Production/TB_CC_Arland.conf` → `Worlds/Production/Arland/TB_CC_Arland.ent` → `NativeCampaign.layer`, not the M4XP proof missions or three-node prototype. The world inherits vanilla `worlds/Arland/Arland.ent`; no `CTL_Campaign_Arland` world parent is present. Vanilla Arland navmesh references are terrain services, not a reintroduced Conflict campaign topology. The layer explicitly instantiates vanilla `RadioManager.et` and `CampaignLoadoutManager.et`, plus the vanilla campaign faction manager and Arland AI world. It instantiates 12 named native strategic base entities, the native game mode, and the native campaign component. The four custom locations are actual capture entities, not authoring markers.

Production layer values inspected on 2026-10-01: `m_iAuthorityProofMode 0`, BLUFOR and OPFOR holds `300` seconds each, `m_bEnableM3Coordinator 1`, `m_bEnableM3Garrison 1`. No proof actors are authored in this world; no absolute local filesystem path, TestProfiles path, or proof-world reference appears in the production mission/world/component resource text checked. The final unrestricted dedicated smoke loaded this mission and reached online state with registry READY (12 positions, 21 edges, 11 ordinary), exactly one US MOB, M3 enabled, zero proof mode, and no BCM script errors. The Creative Director also confirmed normal Workbench compile and resource processing before integrated authority proofs. These results establish the **processed local project**; they do not prove a newly published Workshop package.

## Bundle boundary and exclusion gate

Runtime resources must include the production mission/world/layer, `TB_CC_M4XP_GameMode.et`, native campaign scripts, the referenced strategic-base prefabs and their dependencies. Vanilla Arland terrain, radio/loadout managers, faction manager, AI world and navmesh come from base-game data. The installed server must provide the vanilla game project; it must not rely on this PC's Workbench source path.

Development-only source-tree items to exclude from the distributed bundle: `TestProfiles/`, `console.log`, `error.log`, `Documentation/`, `Tools/`, `ServerConfigs/`, `TestConfigs/`, screenshots, captured logs, temporary profiles, credentials/session material and local-machine configuration. The historical `TestProfiles/addons` duplicate-resource pollution was cleaned; do not regenerate those copies inside the addon source tree. The registered `Missions/Test/M4XP/` proof missions and `Worlds/Test/M4XP/` proof worlds also need explicit bundle inspection: unlike plain documents/logs, they are valid resources and **may be packaged** by a whole-project publish. Their presence is unnecessary for the community server. Proof script classes may remain in the compiled Game module where referenced by shared code, but proof activation stays off. Do not remove/relocate proof resources or change metadata merely on an untested assumption about the Publisher's resource-selection behavior.

**Stop gate:** inspect the Publisher's separate Working Dir/package preview before final upload. If any private material or proof mission/world is present, cancel and use supported Workbench resource build-presence/build-tag controls to exclude the identified resources, then reprocess and inspect again. Do not delete historical evidence or source development files. The exact packed-file inventory has not yet been verified, so publication readiness remains conditional rather than an unconditional PASS.

## Manual stable-Workbench update procedure — stop before upload

1. Launch **stable** Arma Reforger Tools, open the existing BCM Conflict Core project, and verify GUID `6A6A5973FA53A5B1` and a clean compile/resource state. Do not create another project.
2. Open **Workbench → Publish Project**. Because the GUID was previously published, this should open the update path under the original owning account; if ownership is not recognized, stop. Do not enter credentials into project files/scripts.
3. Read the current Workshop version and visibility in Publisher. Set Project Name to `BCM Conflict Core [TEST]` (or preserve the existing name if changing it would disrupt the established item), set **Visibility: Unlisted**, and select a new version greater than the actual current published version. The historical verified `0.0.1` is not proof of the latest version. Use change notes such as: `Unlisted M4.XP full native Arland community test candidate; 12-position campaign; proof modes off; human M3 evaluation pending.`
4. Keep the Publisher Working Dir at its separate default, **never the addon source directory**. Select an accurate category/license and an owner-approved preview image if prompted. Do not invent contributors or dependencies.
5. Inspect any bundle preview/Working Dir and the upload confirmation before proceeding. Confirm the production mission/world and dependencies are present, and the exclusions above are absent. If the Publisher cannot show enough contents before its upload confirmation, stop and inspect the staged bundle by supported tooling before approving upload.
6. **Stop here for this task.** The Creative Director must explicitly authorize/perform the final Publisher upload/confirmation. No Workshop action has been performed by Codex.

After publication (not now): confirm the Workshop item retains GUID `6A6A5973FA53A5B1`, verify the new version and Unlisted visibility/downloadability, configure the separate community server with that one mod and the exact `scenarioId` above, start the full native Arland scenario, then preserve retail-client/server logs for human and network acceptance. Do not use M4XP proof worlds/missions or the three-node prototype. N1–N5 and physical human M3 behavior remain separate acceptance debt.

Official configuration/publishing references: [Reforger Server Config](https://community.bistudio.com/wiki/Arma_Reforger:Server_Config) and [Mod Publishing Process](https://community.bistudio.com/wiki/Arma_Reforger:Mod_Publishing_Process).
