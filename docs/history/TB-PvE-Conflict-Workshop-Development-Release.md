# BCM Conflict Core — Workshop Development Release

## Release status

**PUBLISHED DEVELOPMENT BUILD RESOLVES — M3.7 ADVERTISED SERVER PASS.**

This records packaging preparation and the first published-build server verification for the M3.7 retail-client playtest. It does not change gameplay.

## Identity and metadata

- Project ID: `BCMConflictCore`
- Addon GUID: `6A6A5973FA53A5B1` (preserve exactly)
- Workshop name: `BCM Conflict Core Development`
- Initial version: `0.1.0`
- Visibility: **Unlisted**
- Suggested category: the closest available gameplay/scenario category
- Suggested tags: `PvE Conflict Development Playtest`
- License: select only the license the owner has deliberately chosen; this preparation does not assume one
- Contributors: leave empty unless the owner deliberately adds a Bohemia-account email

The preferred string `BCM Conflict Core - Development` is 31 characters, while the current publishing interface documents a 30-character maximum. `BCM Conflict Core Development` preserves the meaning at 29 characters.

Suggested summary:

> Development build of BCM Conflict Core for controlled PvE Conflict playtesting. Includes the M3.7 ten-objective human combat playtest scenario.

Suggested description:

> Development/test release of BCM Conflict Core. This build provides a server-authoritative PvE Conflict campaign foundation, a ten-objective north-to-south route on Everon, autonomous OPFOR infantry operations, frontline garrisons, vanilla capture and ownership, campaign outcomes, US-only human play, and MOB-only deployment. It is published unlisted for controlled retail-client dedicated-server testing. Gameplay balance is not final.

Suggested change notes:

> Initial development upload for the M3.7 human combat playtest. Adds the M3.7 mission and accepted production route/configuration. All automated proof modes remain disabled.

## Dependency audit

`addon.gproj` declares exactly one dependency:

- `58D0FB3206B6F859` — vanilla Arma Reforger data.

No external Workshop addon dependency is declared or referenced by the project. Do not add a Workshop dependency for vanilla data.

`58D0FB3206B6F859` is an internal Workbench/project dependency on the base game's `ArmaReforger.gproj`; it is not a server-JSON mod and is not an external Workshop download. ServerDiag must be launched with its installation directory as the process working directory so its inherent `./addons/data` project is available. The M3.7 launcher enforces this independently of the absolute config/profile paths.

## Runtime package

The M3.7 entry point is:

`{A370000000000000}Missions/TB_PVE_M3_7_Playtest.conf`

Its runtime closure consists of:

- `addon.gproj` and the compiled resource database/package metadata;
- `Missions/TB_PVE_M3_7_Playtest.conf` and its metadata;
- `Worlds/TB_PVE_M2_4_ProductionRoute.ent`, metadata and `Worlds/TB_PVE_M2_4_ProductionRoute_Layers/M2_4_Route.layer`;
- production prefabs under `Prefabs/Systems`, including the production route, military-base capture derivative and disabled-forward-deployment derivative;
- all scripts under `Scripts/Game`.

The proof script files remain compile-time dependencies because the shared production component references their types. They must remain in the compiled script module, but all proof activation fields in the production route prefab are `0`.

## Files that must not be uploaded

Use Workbench **Publish Project**, which creates a bundled runtime build from registered/runtime resources. Do not upload or zip the source project directory. Before pressing Publish, inspect the separate Working Dir produced by the publisher and confirm it does not contain:

- `TestProfiles/`;
- `console.log` or `error.log`;
- `Tools/` or any `.ps1`, `.bat` or `.cmd` file;
- `ServerConfigs/`;
- `Documentation/`;
- `TestConfigs/`;
- screenshots, log dumps or temporary/generated test directories;
- local paths, credentials, authentication/session data or Steam material;
- any source/resources from `ConflictPVERemixedVanilla2` or another reference addon.

These items are non-runtime source-tree files and are not part of the registered M3.7 resource closure. They are intentionally retained in the development project. If any appears in the publisher Working Dir, cancel publishing and correct the bundle selection in Workbench; do not delete the development copy merely to make a package.

## Validation performed on 2026-09-30

- Addon GUID: **PASS**, `6A6A5973FA53A5B1` in `addon.gproj`.
- Workbench script validation: **PASS**, SteamDiag Script Editor `-validate` exited successfully.
- Resource database processing: **PASS**, SteamDiag `-createDB` exited successfully.
- M3.7 mission resolution: **PASS**, ServerDiag loaded `{A370000000000000}Missions/TB_PVE_M3_7_Playtest.conf` and `$BCMConflictCore:Worlds/TB_PVE_M2_4_ProductionRoute.ent`.
- Runtime initialization: **PASS**, ServerDiag logged `Game successfully created` and `Entered online game state`.
- Route initialization: **PASS**, ten-node `EVERON_NORTH_SOUTH_01` validated and revision 1 published.
- Script errors: **PASS**, no script error/fatal was logged.
- Proof switches: **PASS**, M2.1, M2.4, M2.5 and M3.1–M3.6 proof fields are all `0`; M3.7 telemetry reports `proofActions=0`.

The smoke log contains stock-data/editor-only wrong-GUID warnings for `system/wbdata` resources and empty-resource messages paired with vanilla Everon pathfinding tile warnings. They did not prevent world loading, online state, route initialization or script startup, and no missing BCM resource was identified.

## Interactive publication procedure

1. Start the **stable** Arma Reforger Tools/Workbench, not Experimental Tools.
2. Open the existing `BCM Conflict Core` project. Confirm its project GUID is `6A6A5973FA53A5B1`; do not create a new project.
3. Allow Workbench to finish resource processing. Recompile scripts and stop if Workbench reports a real error.
4. Select **Workbench → Publish Project**.
5. If prompted, use Workbench's normal account-link flow. Do not share credentials with scripts or Codex.
6. Set Project Name to `BCM Conflict Core Development`.
7. Leave Working Dir at its separate default publish location. It must not be the addon source directory.
8. Set Version to `0.1.0`.
9. Set Visibility to **Unlisted**. Private is unsuitable because another retail account cannot download it; Unlisted is the most restrictive documented setting that remains usable by other players and dedicated servers.
10. Select an accurate gameplay/scenario category and use the summary, description, tags and change notes above as desired.
11. Choose the license deliberately. Do not select a license merely to complete this checklist.
12. Bundle the project. Before publishing, inspect the Working Dir and apply the exclusion checklist above. The bundle should contain runtime package output, not the development source tree.
13. Confirm the displayed addon identity remains `6A6A5973FA53A5B1` and that the only dependency is vanilla Arma Reforger data.
14. Press **Publish** and complete the interactive confirmation.
15. After success, open the Workshop item and verify the GUID is still `6A6A5973FA53A5B1`, visibility is Unlisted, and the intended retail account can resolve/download it.
16. Report that confirmation before rerunning the public M3.7 dedicated-server launcher.

After confirmation, run:

```powershell
.\Tools\Run-M3_7-HumanPlaytestServer.ps1
```

The expected retail path is **Multiplayer → Community**, searching for `TB PvE Conflict`.

## Published-build server verification

On 2026-09-30, ServerDiag resolved Workshop addon `6A6A5973FA53A5B1` as `BCM Conflict Core`, downloaded version `0.0.1`, mounted its packed project and retained the internal vanilla-data dependency `58D0FB3206B6F859`. The dedicated config validated, the M3.7 mission loaded, the RPL server listened on `0.0.0.0:2105`, the backend registered `185.214.223.17:2105`, and the process entered online game state. The configured public name was `TB PvE Conflict | M3.7 Playtest` with `visible: true`.

The initial post-publication Missing Addon dialog did not come from generated Workshop metadata. ServerDiag was launched with the addon workspace as its process working directory, causing its inherent relative `./addons` lookup to miss the installation's base-game data. Changing only the launcher working directory to `D:\SteamLibrary\steamapps\common\Arma Reforger Server` fixed discovery. The Workbench dependency remains correctly declared and no replacement Workshop version is required for this issue.
