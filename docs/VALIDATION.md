# Verification and acceptance gates

## Portable baseline check — no game installation needed

```powershell
python -B tools/verify_source.py
```

Checks the preserved Addon file inventory/SHA-256s, project identity, production mission reference and absence of private/generated content in the source package root. A PASS is source preservation, not compilation or runtime qualification.

## Source/API/model regression suites

```powershell
python -B tools/run_static_checks.py --reforger-dir "E:\SteamLibrary\steamapps\common\Arma Reforger" --workbench-dir "E:\SteamLibrary\steamapps\common\Arma Reforger Tools" --suite all
```

`--suite persistence` runs Test-CampaignAdministration.py and its nested persistence chain. `--suite campaign` runs Test-EndgamePressure.py and the established nested campaign chain. `all` runs both plus the read-only native construction boundary check. Python uses the standard library only.

To compare an actual newly staged package, add `--package "E:\BCM-Staging\data.pak"`. Without this flag, the legacy package check reports PACKAGE UNAVAILABLE; that is an intentionally unperformed package gate, not a package PASS. A package comparison is evidence only about that exact local file, not Workshop content.

Covered checks retain the original assertions: P1/P2A/P2B/P3/snapshot repair; native command API/permission/transaction models; 1,024 ownership subsets, 20,480 transitions, 29,518 radio masks; route/recapture/commitment; FIA/USSR and physical/virtual warfare; chat/GM; response/pressure and fonts/layout contracts.

Native archive names, signatures and documentation assertions are version-specific. If an updated game installation fails them, investigate the installed source and report the version mismatch. Do not silently skip assertions or substitute a model PASS for engine validation.

## Workbench/resource gate

Compile and process Addon/addon.gproj using the matching stable tools. Check script errors, native command registration, production scenario discovery, resolved campaign controller/task/XP/radio/respawn/editor components, and required PC/headless resources. Proof mode stays zero. Workbench bootstrap interaction is separate and is not a dedicated deployment prerequisite.

## Runtime/network/IO gates

Dedicated loading, actual identity audits, player XP/rank/HUD replication, native commands/RCON, two-player capture and tasks, JIP, native GM permission, AI movement/combat, virtualisation, pressure/HQ and real save/restore all require observed runtime evidence. Follow DEDICATED_TEST_PLAN.md. Record version, source commit, exact package/version/hash, profile location and logs privately.

Never label a compilation, static scan, mathematical model, or package inventory as proof of interactive/network/Linux behavior. Keep secrets, full profiles, tokens and account records out of repository test output.

## Initial handoff verification (2026-10-31)

The 180-file SHA-256 source transfer check passed. The relocated `--suite all` runner passed against the original PC's installed game/Tools resources with the existing assertions retained: persistence/admin, full campaign regression chain and the 24-assertion construction boundary check. No actual package was selected for that run; PACKAGE UNAVAILABLE is intentional. No Workbench compile, game/server launch, native administration, real save IO, campaign reset or Linux test was performed during repository preparation.
