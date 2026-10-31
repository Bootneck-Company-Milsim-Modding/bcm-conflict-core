Canonical handoff status: [docs/CURRENT_STATE.md](docs/CURRENT_STATE.md). Read this before choosing work; older documents are supporting history.

# bcm-conflict-core

Private Bootneck Company Milsim Modding source repository for **BCM Conflict Core**. This is a preserved development baseline, **not a qualified Workshop release or dedicated-server runtime PASS**.

## Continue on another PC

For Codex, start with the [dedicated-server handover prompt](docs/CODEX_HANDOVER.md).

1. Sign into GitHub with an account that can access this private organisation repository.
2. Install Git, Python 3.10+ and matching stable Arma Reforger / Arma Reforger Tools (target API: 1.8). Native game data is external, supplied by your own installation.
3. Clone the repository into a folder **outside** any active addon or publication folder:

   ```powershell
   git clone https://github.com/Bootneck-Company-Milsim-Modding/bcm-conflict-core.git
   cd bcm-conflict-core
   python -B tools/verify_source.py
   ```

4. Open **`Addon/addon.gproj`** in Workbench. If registering it in Workbench's addon search directory, follow [SETUP.md](docs/SETUP.md); do not load another checkout of the same GUID simultaneously.
5. Run the portable source/API/model checks against **your** installation:

   ```powershell
   python -B tools/run_static_checks.py --reforger-dir "D:\SteamLibrary\steamapps\common\Arma Reforger" --workbench-dir "D:\SteamLibrary\steamapps\common\Arma Reforger Tools"
   ```

   Replace both example paths. These checks never launch the game or execute a campaign reset. They do not replace Workbench or dedicated acceptance.

6. Read [CURRENT_STATE.md](docs/CURRENT_STATE.md), [VALIDATION.md](docs/VALIDATION.md), and [DEDICATED_TEST_PLAN.md](docs/DEDICATED_TEST_PLAN.md) before changing or packaging anything.

## Identity — preserve across computers

| Item | Value |
| --- | --- |
| Project ID | `BCMConflictCore` |
| Mod GUID | `6A6A5973FA53A5B1` |
| Dependency | `58D0FB3206B6F859` (vanilla Arma Reforger) |
| Production scenario | `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf` |
| Production world | `{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent` |
| Current topology | 12 managed positions: 2 HQs + 10 ordinary |

Do not create a new addon project or regenerate `.meta` identities to move PCs.

## Repository layout

- `Addon/`: native project, scripts, prefabs, missions, worlds/layers, layout and resource metadata. The original runtime source is preserved byte-for-byte.
- `docs/`: current handoff, setup, validation and deployment guidance.
- `docs/history/`: original project documentation, retained as historical evidence; old paths/status are not current acceptance.
- `tools/`: portable verification entry points; `tools/legacy/` retains the established tests/runners. Python test path wiring is adapted for this layout; assertions are retained.
- `manifests/SourceBaseline.json`: SHA-256 inventory of the 180 preserved native project files.
- `examples/`: placeholder-only server configuration; never store real credentials here.

Tools, documentation, local profiles and server secrets are outside the Workbench addon root. Keep them out of Workshop packages too: `.gitignore` does **not** control Publisher packaging.

## Current boundaries

Strategic persistence, native XP/rank restoration, HQ pressure/victory and campaign administration are implemented at source level. Current-build Workbench/headless, native admin/RCON, Linux filesystem and multiplayer acceptance remain explicit gates. Workbench-only administrator bootstrap is unresolved and must not block dedicated testing.

Feature freeze: no P3.5 construction persistence, P4 active-operation reconstruction, helicopter AI, combined arms or new gameplay/UI systems. No Workshop publication, deployment, reset, or balance change is authorised by cloning this repository.

This repository intentionally does **not** transfer test profiles, campaign saves, credentials, logs/dumps, generated resource databases or old `data.pak` builds. See [SETUP.md](docs/SETUP.md) for separate private save transfer and [OWNERSHIP.md](docs/OWNERSHIP.md) for asset/licence boundaries. No open-source licence is granted.
