# Multi-PC setup and source ownership

## Requirements

- Git and authorised access to this private organisation repository.
- Python 3.10+ (stdlib only; no pip dependencies required by the verification tools).
- Your installed stable Arma Reforger and Arma Reforger Tools. The source/API baseline targets 1.8; record actual tool/client/server patch versions before acceptance.
- Sufficient local disk space for native imported/processed resources. Installed game assets and purchased assets from other BCM repositories are not supplied here.

## Clone and register

Clone outside an addon root, for example `C:\Dev\bcm-conflict-core`. Open the cloned `Addon\addon.gproj` in Workbench and register that existing project. Do not use New Project and do not change its GUID.

If your Workbench setup requires the default addon search directory, create a Windows directory junction **after checking the destination does not exist**:

```powershell
$checkout = 'C:\Dev\bcm-conflict-core'
$destination = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'My Games\ArmaReforgerWorkbench\addons\BCM Conflict Core'
if (Test-Path -LiteralPath $destination) { throw 'An existing project is present. Preserve it and resolve duplicate GUID ownership before proceeding.' }
New-Item -ItemType Junction -Path $destination -Target (Join-Path $checkout 'Addon')
```

Only one copy of GUID `6A6A5973FA53A5B1` should be active in the search paths. Preserve any existing folder until its changes have been compared. Never recursively delete/clean a junction or original addon to solve a duplicate. A normal fresh clone has real directories, not a junction.

Rebuild the local resource database through Workbench. The generated `resourceDatabase.rdb` is intentionally not transferred; `.meta` files **are** transferred and must remain intact.

## Test paths

The supported runner accepts `--reforger-dir`, `--workbench-dir` and optional `--package`. These can also be configured for direct legacy Python test execution:

```powershell
$env:BCM_REFORGER_DIR = 'E:\SteamLibrary\steamapps\common\Arma Reforger'
$env:BCM_REFORGER_TOOLS_DIR = 'E:\SteamLibrary\steamapps\common\Arma Reforger Tools'
python -B tools/run_static_checks.py --suite all
```

No username or drive letter is required by the new entry points. Do not copy the old PC's drive layout to satisfy a test. Native archive names/API assertions remain version-specific and deliberately fail if the installed data no longer matches.

## Two PCs and Git

Before switching PCs: review changes, run appropriate checks, commit and push the intended branch. On the other PC, start with `git status`, then `git pull --ff-only`. Do not overwrite local modifications with reset/clean. Use a separate branch for unfinished work rather than editing stale copies simultaneously.

The first repository import is a snapshot of previously unversioned source, not reconstructed development history. The original source folder on the first PC was not moved or changed by the handoff. Future edits in a clone do not automatically update that original folder; choose one authoritative checkout per PC.

`tools/verify_source.py` compares Addon files against the initial handoff manifest. Intentional future edits will correctly report drift until explicitly reviewed; do not automatically regenerate the manifest to obtain a PASS.

## Campaign saves and server credentials — separate private transfer

Git transfers development source, not an active campaign. If continuing an actual saved campaign, gracefully stop the owning instance, take a full private backup of its effective profile, and securely transfer the complete `BCMConflictCore/Persistence` directory. Preserve primary/backup/reset-journal/archive relationships together. Never put those files in Git, edit their UUIDs, delete reset markers, or run two instances against one profile.

Keep map/scenario/config identity, seed, start mode and selected MOB unchanged when restoring. Changing them can deliberately fail closed. Prefer a separate disposable profile for verification. Source can be cloned without moving any save.

Configure AMP passwords/RCON credentials on the target instance, not in tracked files. The example JSON contains placeholders only. If a secret was reused from a historically distributed test configuration, rotate it privately; the source handoff itself does not rotate credentials or alter a live server.

## Legacy evidence and launchers

Historical documents retain old absolute paths and old test results. They are not portable operational commands. The PowerShell runners in `tools/legacy/` may depend on old test configs/profiles which are intentionally excluded, and can write files or launch servers/clients. Keep them as reference; use current explicit parameters/configuration only after reviewing the script.
