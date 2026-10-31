# BCM Conflict Core continuation rules

Read `docs/CURRENT_STATE.md` and `docs/VALIDATION.md` first. `docs/history/` is historical, not new runtime acceptance.

- `Addon/` is the native project root; preserve its project GUID, resource GUIDs and native resource paths. Open `Addon/addon.gproj`, not the repository root.
- At initial handoff all 180 Addon files are an exact preservation baseline. Do not normalise line endings or regenerate metadata. Verify changes with `tools/verify_source.py` and review intentional baseline drift explicitly.
- Freeze new features unless the user authorises them. Do not implement P3.5/P4, helicopter AI, combined arms or new UI/admin conveniences as incidental work.
- Preserve native authority, radio, capture, objective commitment, US MOB-only deployment, AI policy, XP/rank and HQ behavior. Do not treat GM/editor access as multiplayer administrator authority.
- Never replay historical capture/reward events during restore. Never invent player identities or client-submitted privilege flags.
- Do not publish to Workshop, deploy, execute a reset, change live server/profile data, or commit/push changes unless the current user request authorises that action. A source handoff is not Workshop publication approval.
- Keep secrets, real server configs, profiles, saves, credentials, logs and build outputs out of Git. Do not delete original evidence or user changes to clean a repository.
- Run `python -B tools/verify_source.py`; use `tools/run_static_checks.py` with the machine's installed Reforger/Tools paths for API/model checks. Do not weaken tests to conceal an installed-version mismatch.
- The old PowerShell launchers under `tools/legacy/` are historical, machine-specific operational tools. Do not run them blindly: they can launch games and write test profiles.
- Report source/static/API/model, native compilation, resource processing, UI, multiplayer/JIP, real IO and Linux/runtime results separately. Never promote one gate into another.
- No agent should update the user's external Codex memory unless explicitly requested.
