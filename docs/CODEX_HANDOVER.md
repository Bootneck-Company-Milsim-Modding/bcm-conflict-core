# Codex handover — dedicated server test build

Prepared 2026-10-31. Use the prompt below to resume on another PC. Current operational status remains in [CURRENT_STATE.md](CURRENT_STATE.md); this handover does not authorise Workshop publication or deployment.

## Handover prompt

Continue BCM Conflict Core from its private GitHub repository:

https://github.com/Bootneck-Company-Milsim-Modding/bcm-conflict-core

Objective: prepare the current campaign for an isolated two-player Arma Reforger 1.8 Linux dedicated-server test using AMP on OVHcloud.

First:

1. Clone the repository into a suitable development directory, outside an active addon or publication folder. If already cloned, inspect its status and preserve local changes before updating.
2. Read AGENTS.md, README.md, then:
   - docs/CURRENT_STATE.md
   - docs/SETUP.md
   - docs/VALIDATION.md
   - docs/DEDICATED_TEST_PLAN.md
   - docs/OWNERSHIP.md
3. Treat docs/history/ as supporting history, not current runtime acceptance.
4. Run:

   ```powershell
   python -B tools/verify_source.py
   ```

5. Locate this PC's actual Reforger and Workbench installations, then run:

   ```powershell
   python -B tools/run_static_checks.py --reforger-dir "<GAME_INSTALL>" --workbench-dir "<TOOLS_INSTALL>" --suite all
   ```

Project identity:

- Open Addon/addon.gproj, not the repository root.
- Mod GUID: 6A6A5973FA53A5B1
- Sole dependency: vanilla Arma Reforger, 58D0FB3206B6F859
- Production scenario: `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf`
- Preserve all resource GUIDs. Avoid loading duplicate copies of the project.

Where we left off:

- The repository was created and pushed on main.
- All 180 native project files were preserved unchanged and hash-verified.
- Static/API/model suites and fresh-clone source verification passed on the original PC.
- Current-build Workbench/resource compilation, headless loading, Linux IO and two-player multiplayer acceptance remain outstanding.
- The previously audited downloaded package was stale, lacked persistence/admin scripts and included development/profile material. Do not reuse an old data.pak or assume the latest Workshop build matches this source.
- The unresolved Workbench-only administrator bootstrap must not block dedicated testing. Preserve normal native administrator/RCON permissions.

Next work:

1. Verify this checkout and installed versions.
2. Perform the final native Workbench compile/resource gate.
3. Prepare a fresh staged package from Addon only.
4. Configure supported build-presence exclusions for unnecessary proof missions/worlds, preserving shared dependencies.
5. Inspect the actual package for current runtime content and absence of profiles, credentials, logs, saves and development material. Gitignore does not control Workshop packaging.
6. Report the candidate's commit, validation results, package contents/hash and remaining gates.
7. Stop before Workshop upload or deployment and request explicit approval.

Once deployment is separately authorised, follow the dedicated test plan using a passworded, isolated instance and a separate persistent profile. Test two-player authority/JIP, native admin access, ownership/objectives/radio, XP/rank, AI transitions, pressure/HQ and ordinary restart/backup recovery.

Scope remains frozen:

- No P3.5 construction persistence or P4 active-operation reconstruction.
- No helicopter AI, combined arms, new gameplay/UI or admin conveniences.
- Do not change gameplay balance, radio, capture, objectives, AI tuning or world/layers incidentally.
- Do not weaken permissions or treat GM/editor access as administrator authority.
- Do not execute a campaign reset, delete recovery markers, modify live saves or overwrite existing user work.
- Do not commit/push additional changes without approval.
- Keep static/API/model, native compilation, resource processing and actual runtime/network/Linux results explicitly separate.

Make progress toward the test build rather than repeating the entire historical audit. If a gate fails, identify the exact cause and propose the smallest scoped correction.
