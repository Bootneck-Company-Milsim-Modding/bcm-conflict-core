# Current handoff state — 2026-10-31

This is the authoritative continuation summary. The source is preserved from the BCM Conflict Core working project, not taken from a downloaded Workshop package. No runtime acceptance is implied by this source transfer.

## Architecture present

- Three-faction US/USSR/FIA Conflict campaign with 12 managed positions, two HQs and ten ordinary bases.
- Native capture and radio; expanding 1,000 m objective bands/frontier scoring, recapture eligibility and objective commitment.
- US MOB-only player deployment; FIA/USSR operations and physical/virtual force simulation.
- Strategic persistence, native player XP/rank persistence, pressure/HQ vulnerability/victory.
- P5 native `bcmcampaign` admin/RCON transport and restart-based campaign reset implementation.
- All seven persistence/admin source files are included under `Addon/Scripts/Game/TB_CC_Persistence/`.

The reusable core and existing Arland map adapter remain distinct concerns. This handoff does not redesign either or claim the adapter is map-agnostic.

## Last release-readiness result

The 20 October audit blocked deployment of the locally downloaded package labelled 0.0.10: it lacked seven current persistence/admin scripts, differed from six other current scripts, and contained development/profile material. That is historical evidence about that local artifact, **not** verification of the latest remotely published version. Do not deploy an old `data.pak` to continue this source.

The repository places only native content in `Addon/`; docs and tools are outside the package root. Proof missions/worlds remain preserved native resources in Addon for historical tests, so supported build-presence exclusions and staged-package inspection are still required before release. Git exclusions are not Publisher exclusions. Do not delete proof resources or shared classes to make packaging appear clean.

## Validation status

Previously established source/API/independent-model checks cover persistence phases 1/2A/2B/3, snapshot repair, P5 administration, objective continuity, native radio, FIA/USSR, physical/virtual war, chat/GM, responses and pressure. Re-run on each installed SDK; see VALIDATION.md.

Earlier user reports accepted Workbench milestones. They do not constitute a current-build Linux dedicated PASS. Remaining gates include current Workbench/resource compilation, headless loading, actual native admin/RCON dispatch, backend identity/XP replication, two-player/JIP, pressure/terminal restart and Linux recovery/durability.

## P5 authority detail

Both command classes implement all eight installed native API events, including synchronous `OnUpdate` completion. Main `#bcmcampaign` retains native ADMINISTRATOR chat and ADMIN RCON gates. Local `#bcmcampaignlocal` rejects multiplayer/dedicated/RCON use and requires a registered local controller plus SESSION_ADMINISTRATOR.

The `#ifdef WORKBENCH` helper `TB_CC_WorkbenchCampaignAdmin` supplies explicit offline-only diagnostics/native role assignment. Its interaction/transport is unresolved. It is not a production privilege path and must not block dedicated testing. Do not lower native permissions, grant multiplayer privileges from GM/editor access, or introduce unauthenticated RPCs to work around it.

## Persistence boundaries

- Schema 2; campaign UUID plus map/scenario/config compatibility checks.
- Scoped quiet ownership restore followed by explicit native reconciliation before consumers start. Do not replay historical capture/reward callbacks.
- Account-audited backend UUIDs and absolute native XP/rank reconstruction; no invented/offline-name identity fallback.
- Dirty 60-second checkpoints and startup/disconnect/admin/shutdown paths. Shutdown is best effort; readback copies do not establish fsync/atomic-rename durability.
- Profile-local primary/backup/recovery/reset state. Restarting unchanged must retain the same campaign UUID unless an explicit reset was authorised and staged.
- Construction persistence (P3.5) and active-operation reconstruction (P4) are intentionally absent. Physical battlefield/casualty/supply persistence must not be advertised.

## Next work, in order

1. Verify the handoff baseline and rerun static/API/model suites against the target PC installation.
2. Compile/process the production project with stable tools; inspect resolved native components and headless resources.
3. Create and inspect a fresh package with private development content and unnecessary proof resources excluded.
4. Obtain explicit publication/deployment authority, then use an isolated passworded Linux AMP instance and persistent profile for two-player acceptance.
5. Confirm ordinary restart, XP/ownership/objective restoration and backup recovery without executing a campaign reset.

Feature freeze remains: no P3.5/P4, helicopter AI, combined arms, new gameplay/UI or admin quality-of-life expansion. Do not retune balance/AI/radio/capture/objectives without evidence and authorisation. See DEDICATED_TEST_PLAN.md.
