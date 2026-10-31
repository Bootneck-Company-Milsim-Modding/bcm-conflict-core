# BCM Conflict Core endgame pressure timing and presentation

Native campaigns now use one 600-second territorial-control hold before HQ
vulnerability. Loss of an ordinary position cancels the deadline; restoring full
control starts a new full hold. The existing recovery, defence and HQ capture
lifecycle remains in place. Source/model validation passes. Normal Workbench
compile, layout processing and human visual acceptance remain pending.

## Runtime baseline

1. The Creative Director reports a complete campaign through ten ordinary
   positions, native radio progression, FIA/USSR warfare, pressure recovery and
   HQ vulnerability. The current brief quotes `coordinatorRetained=1`,
   `recoveryMode=1 remaining=299`, and HQ West `PREPARED package=HEAVY strength=15`.
   The full log referenced by this brief was not attached with it; this baseline
   relies on those supplied observations and quotations. It does not establish
   runtime acceptance of the new 600-second setting or layout.

## Authoritative timing

2. The previous native duration came from inherited
   `m_iBLUFORVictoryHoldSeconds` and `m_iOPFORVictoryHoldSeconds`, both explicitly
   serialized as 300 in `A_Managers.layer`, with matching script defaults.
3. `m_iPressureDurationSeconds` is the single current native setting, default
   600. `GetConfiguredPressureDuration(int outcome)` resolves it in both native
   directions. The shared publication method uses the same resolver.
4. Both native pressure-entry arguments and replicated duration publication
   now use that resolver. Four M4XP launchers no longer require serialized
   legacy `VictoryHoldSeconds 300`; their shared configuration guard resolves
   the current attribute default or explicit native layer override. The old
   continuity test assertion was updated to follow this setting.
   Historical graph/prototype holds, compressed proofs and historical evidence
   remain unchanged. Their 300 values are not current native duration inputs.
   Other 300 values are placement radii, operation timeouts or test samples.
5. Existing entry creates a deadline from the current server timestamp plus
   the configured duration. Cancellation clears it. Restoration creates a new
   deadline with the full 600 seconds; no elapsed-time carry-over is introduced.
   The current world has no explicit override of the new setting, and the
   configuration guard resolves 600. There is no pressure persistence system.

## Recovery and final defence

6. A pending/admitted recovery request prevents another recovery request.
   The queue marks a terminal operation resolved and removes its request;
   a subsequent tick may enqueue another while pressure remains active and
   at least 30 seconds remain. Existing retry pacing is 15 seconds and blocked
   admission retries after two seconds.
7. No retry correction was required.
8. Existing coordinator capacity, operation admission, source/target eligibility,
   AI budgets, physical/virtual materialization and staging remain responsible
   for each force. Recovery ends on lost pressure or HQ vulnerability. Only
   response telemetry was extended with remaining time and admitted operation ID.

## Native presentation references

9. Installed vanilla resources reviewed:
   `scripts/Game/UI/HUD/SCR_PopupNotification.c`,
   `UI/layouts/Common/PopupUI.layout`,
   `scripts/Game/UI/HUD/SCR_CampaignInfoDisplay.c`, and
   `scripts/Game/UI/HUD/SCR_CampaignMapInfoDisplay.c`.
   `PopupUI.layout` uses centred title/body text at y=70/116, 25/20-point
   typography, and native shadows. Its default title is orange and body grey;
   `SCR_PopUpNotification` fades over one second. The campaign display formats
   authoritative timestamp differences and sets its running countdown white.
10. Selected family: native Roboto Condensed, Bold for titles/labels and Regular
    for body/countdown digits.
11. Font paths: `UI/Fonts/RobotoCondensed/RobotoCondensed_Bold.fnt` and
    `UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt`.
12. Exact GUIDs: Bold `EABA4FE9D014CCEF`; Regular `3E7733BAC8C831F6`.
13. Both exact references occur in installed `PopupUI.layout`; both GUID/path
    pairs match the installed vanilla resource database.
14. The BCM-owned `TB_CC_VictoryPressure.layout` replaces dynamic debug text
    and the shared vanilla popup call. It follows the native centred typography
    and shadow conventions, with one-second alpha fades within an eight-second
    announcement. Afterwards a 320x70 countdown frame contains only label and
    digits. All widgets ignore cursor/focus. Authority remains on the server;
    local presentation timing affects only announcement visibility.
15. Title is declared and set to RGBA 255/255/255/255.
16. Body is declared and set to RGBA 255/255/255/255.
17. Countdown label is declared and set to RGBA 255/255/255/255.
18. Countdown digits are declared and set to RGBA 255/255/255/255.
19. Pressure-broken title/body use those same white widgets; pressure loss hides
    the countdown immediately.
20. HQ-vulnerable title/body use those same white widgets; HQ unlock hides the
    countdown. Notifications are triggered by state edges, not every tick.
21. Format cases passed: 600=10:00, 599=09:59, 301=05:01, 300=05:00,
    61=01:01, 60=01:00, 1=00:01, 0=00:00. Negative input clamps to 00:00.

## Scope and cleanup

22. Response epochs, waves, packages, force strength, staging, recovery retry
    and HEAVY HQ defence were preserved. The supplied strength=15 remains the
    runtime baseline; this pass does not normalize or rebuild packages.
23. Objective selection/commitment/publication, radio, capture superiority,
    factions, physical/virtual war, fortifications, chat, GM and spawn policy
    were preserved.
24. HQ protection, cancellation, permanent vulnerability and physical-capture
    terminal rules are unchanged; current native hold duration is now 600.
25. Changed scripts: graph campaign component, native campaign component,
    pressure presentation, and response event telemetry. Changed launch guards:
    M4XP human, integrated proof, corrective acceptance and Phase 1 unrestricted.
    Added shared `Assert-NativePressureConfiguration.ps1`. Updated continuity
    and strategic-response test expectations; added `Test-EndgamePressure.py`.
    This report records the current contract without rewriting historical proof
    documentation.
26. New BCM resource:
    `{A4F3600000000001}UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout`,
    with matching `LayoutResourceClass` platform metadata. Its GUID is unique in
    project metadata. Processing/loading it must be confirmed in Workbench.
27. World and layer files changed: NONE.
28. Cleanup removed the old generated text-widget path, default28 font reference,
    gold colour, and shared popup notification call from BCM pressure presentation.
    No existing file or historical evidence was deleted; no orphaned earlier
    pressure layout was present.
29. Duration and presentation introduce no Arland coordinates or IDs. The map
    adapter retains the existing HQ/state binding. Native campaigns derive both
    pressure directions from the reusable duration setting.

## Validation and next gate

30. `Tools/Test-EndgamePressure.py` passes the existing chain: 1,024 ownership
    states, 20,480 transitions, 29,518 radio masks, ten-objective route, repeated
    recapture/commitment and HQ/FIA/USSR/physical/virtual/chat/GM regressions,
    4,000 delay cases, 11 queue/epoch scenarios, 24 pressure cases and 128 placement
    filter cases. New checks pass 36 timer/reset/HQ cases, 45 bounded recovery
    cases, nine countdown formats and four notification edges. Five installed
    fortification/font resource identities pass, including both native fonts.
31. Fixed-array lint passes across 57 scripts with 16 negative/positive fixtures.
    Whole-tree source checks and installed widget API review pass. PowerShell
    parsing passes across Tools; the read-only guard resolves 600 seconds.
    Layout braces/widget names, font provenance, white declarations, local GUID
    uniqueness and matching metadata pass source checks. These results do not
    establish engine compilation, resource processing, multiplayer presentation
    or human visual acceptance.
32. WORKBENCH GATE REQUIRED. Compile normally, process the new layout and load
    the production world. Then verify white native typography, one eight-second
    announcement, the compact countdown, pressure cancellation/reset and HQ unlock
    during the next authorized human session. No server/client session, Workshop
    publication, dedicated deployment or commit was performed in this pass.

## Changed files

| File | Change |
| --- | --- |
| `Scripts/Game/TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c` | Native duration attribute, shared duration resolver/publication, start telemetry and legacy field descriptions. |
| `Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c` | Resolve native duration from one setting, use it at entry and log completion. |
| `Scripts/Game/TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c` | BCM white layout, native fonts, fades, announcement/countdown lifecycle. |
| `Scripts/Game/TB_CC_Core/TB_CC_StrategicResponse.c` | Remaining-time and admitted-operation telemetry only. |
| `UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout` | New white announcement and countdown layout. |
| `UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout.meta` | New unique layout identity and platform configurations. |
| `Tools/Assert-NativePressureConfiguration.ps1` | Read-only shared effective-duration guard. |
| `Tools/Run-M4XP-HumanAcceptance.ps1` | Replace legacy hold assertions with current duration guard. |
| `Tools/Run-M4XP-IntegratedProof.ps1` | Replace legacy hold assertions with current duration guard. |
| `Tools/Run-M4XP-CorrectiveAcceptance.ps1` | Replace legacy hold assertions with current duration guard. |
| `Tools/Run-M4XP-Phase1-Unrestricted.ps1` | Replace legacy hold assertions with current duration guard. |
| `Tools/Test-ObjectiveTerritorialContinuity.py` | Bind the existing duration assertion to the new resolver. |
| `Tools/Test-StrategicResponse.py` | Validate layout-driven presentation and native fonts, retaining existing cases. |
| `Tools/Test-EndgamePressure.py` | Add timer/reset, recovery, formatting, UI and resource checks. |
| `Documentation/TB-Conflict-Core-Endgame-Pressure-600.md` | Current contract, evidence boundaries and required report. |

Before/after SHA256 comparison confirms no changes to world/layer, prefab,
mission or server-config files. Locked selector, radio, capture, faction,
coordinator, package, staging, physical/virtual, fortification, chat, GM and
spawn source files also remain unchanged. No files were removed.
