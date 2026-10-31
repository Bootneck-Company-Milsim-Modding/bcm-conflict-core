# M4.4I — Resource hygiene gate (2026-10-01)

**Project duplicate-GUID pollution corrected; startup registry verified; fully clean resource acceptance NOT claimed. Physical acceptance remains gated.**

**Subsequent resolution:** The Creative Director manually inspected/saved/processed the cleaned world, accepted inherited vanilla sign warnings, confirmed the four actual capture placements and retired old markers. That closes this hygiene gate. Dedicated physical proofs and final production smoke subsequently passed; see `TB-Conflict-Core-M4.4I-Runtime-Acceptance.md`. The blocked language below records the earlier investigation, not current status.

## Preserved generated copies

Moved exactly 48 BCM addon-cache directories (458 files, 14,433,222 bytes) from:
`TestProfiles/<profile>/addons/BCMConflictCore_6A6A5973FA53A5B1`
to:
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/ResourceHygiene_2026-10-01/<profile>/addons/BCMConflictCore_6A6A5973FA53A5B1`.

Every move was constrained to the exact addon directory below TestProfiles, with pre/post file count and byte-sum verification. No evidence was deleted. Logs, profiles, transcripts and other artifacts remain at their original paths. Archived package contents remain recoverable at the matching external path. Do not copy archived packages back below the live addon. Future acceptance profiles must remain outside the addon resource tree to avoid recreating this condition. Existing launchers were not altered.

Moved profile names:

- M3_7_Playtest_2026-09-30_16-08-55
- M3_7_Playtest_2026-09-30_16-25-32
- M4_1_ArlandFixture_2026-09-30_19-29-24
- M4_1_ArlandFixture_2026-09-30_19-31-10
- M4_1_ArlandFixture_2026-09-30_19-32-26
- M4_2_ArlandSurvey_2026-09-30_19-49-40
- M4_2_ArlandSurvey_2026-09-30_19-50-59
- M4_2_ArlandSurvey_2026-09-30_19-52-13
- M4_2_ProductionGraph_2026-09-30_19-54-51
- M4_2_ProductionGraph_2026-09-30_19-56-30
- M4_2A_CampaignStart_2026-09-30_20-04-26
- M4_2A_CampaignStart_2026-09-30_20-05-52
- M4_2A_CampaignStart_2026-09-30_20-07-13
- M4_2A_Final_2026-09-30_20-09-41
- M4_3_Final_2026-09-30_20-20-19
- M4_3_StrategicSelection_2026-09-30_20-17-16
- M4_3_StrategicSelection_2026-09-30_20-18-30
- M4_4_AuthorityGate
- M4_4_AuthorityGate_Final
- M4_4A_East_Physical
- M4_4A_East_Physical_2
- M4_4A_East_Physical_3
- M4_4A_East_Physical_4
- M4_4A_East_Physical_5
- M4_4A_East_Physical_6
- M4_4A_East_Physical_7
- M4_4B_Causal_PlayerRequirement
- M4_4B_Causal_PlayerRequirement_2
- M4_4B_Telemetry_1
- M4_4B_Telemetry_2
- M4_4B_Telemetry_3
- M4_4C_Causal_BothGates
- M4_4D_AdmissionTelemetry
- M4_4D_Causal_HQRadioRange
- M4_4D_Causal_PlayableRadioBranch
- M4_4D_Causal_RadioBranch
- M4_4E_StrategicConnectivity
- M4_4E_StrategicConnectivity_2
- M4_4F_EastFull
- M4_4F_EastFull_2
- M4_4F_Invalid
- M4_4F_OPFORTerminal
- M4_4F_ProductionSmoke
- M4_4F_West
- M4_4G_East
- M4_4G_ProductionSmoke
- M4_4H_BackendControl_Unrestricted_2026-10-01_09-28-48
- M4_4H_BLUFOR

After cleanup: zero matching BCM addon-cache directories and zero .meta files remain under TestProfiles.

## Metafile correction

Workbench manual log `C:/Users/ruanb/Documents/My Games/ArmaReforgerWorkbench/logs/logs_2026-10-01_11-07-02/console.log`, lines 1085–1087, rejects ConfigLoadResource inside MetaFileClass. That block is not valid metafile configuration. Its original author/generation provenance cannot be established from surviving files; do not attribute it to normal Workbench output.

Repaired M4.4 world metadata to the ENTResourceClass Configurations schema used by the existing processed M2.1 world metafile, preserving `A441000000000000`. Corrected the identical invalid block in the M4.1 fixture metadata, preserving `A411000000000000`. No world identity was changed.

Normal Workbench resource indexing was then invoked with -createDB. At 11:18:35.549 it saved the BCM database (188,126 bytes), with zero duplicate-GUID warnings and no ConfigLoadResource error. Workbench later crashed during Create Game (exit 0xC0000005) with shader errors; thus database scan/save succeeded, but complete interactive metadata regeneration/world-editor validation is not claimed.

The campaign layer had again serialized the stale TestProfiles prefab path. Its reference was corrected to `{A440000000000000}Prefabs/Systems/TB_CC_M4_4_ArlandGraphCampaign.et`, keeping the same GUID. This is resource-reference hygiene, not a proof/configuration change.

## Exact hierarchy-error identities

The manual Workbench log and subsequent dedicated startup identify all four as instances of vanilla:
`{2233B598B6DE5C3A}Prefabs/Structures/Signs/Military/Supplies/Sign_SupplyStorage_Wall_01_Supplies_USSR.et`.

| Runtime entity ID | Position X Y Z |
| --- | --- |
| 2305843009213694665 | 1369.078003 39.151001 2345.188965 |
| 2305843009213694666 | 1385.224976 37.777000 2404.989990 |
| 2305843009213694667 | 1241.339966 39.123001 2931.794922 |
| 2305843009213694668 | 1330.571045 38.570000 2377.497070 |

They are inherited vanilla supply signs, not any TB_CC capture entity and not the authoring markers. No duplicate Hierarchy component is authored in the BCM capture prefab/layer. No vanilla prefab or strategic placement was changed. The exact duplicate-component source within the packed inherited vanilla composition remains unresolved.

## Proof-off dedicated startup

Evidence:
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/M4_4I_HygieneStartup_2026-10-01/logs/logs_2026-10-01_11-19-38/console.log`.

- Game script compilation completed: zero SCRIPT (E).
- World loaded; online state entered at 11:19:47.054.
- All four new bases report initialized=1 and isHQ=0 at the intended current positions.
- 11:20:01.948: registry READY, positions=12, edges=21, mobs=2.
- 11:20:01.950: selected HQ East, enabledUSMobSpawnPoints=1, allForwardDisabled=1.
- 11:20:01.951: valid=1, depth=0, expected East boundary.
- required ordinary=10, USSR ordinary=10, remaining production timer=300.
- Zero duplicate-GUID/metafile errors, zero proof-harness starts.
- The four inherited sign hierarchy errors persist. Also logged six unknown Parent fields, SlidingTrackMaterial and a Workbench Gizmo sphere resource error during inherited/resource loading. These are not a clean all-resource validation result.
- Temporary validation server PID 24896 was stopped explicitly after evidence collection. It exposed no closeable window, so process termination was used; not represented as graceful shutdown. No other process was stopped.

No capture proof, authentication test or gameplay change was performed.

## Authoring-reference discrepancy / next action

The currently saved campaign layer contains the production system and four capture entities, but no TB_MARKER_* entities. This absence was observed during source inspection; this cleanup did not delete markers or change capture coordinates. Cause is not established; stale editor state is a possibility, not a confirmed cause. Do not silently recreate/move authoring references from obsolete snapshots.

Reopen the cleaned project in normal Workbench, compile and reload/save the current M4.4 world to confirm metadata and current live prefab references without cached TestProfiles paths. Inspect whether authoring markers exist in an unsaved editor state before saving. Confirm intended marker preservation/restoration if missing.

For remaining sign errors, inspect the inherited vanilla composition or reproduce in an unmodified vanilla Arland world before any scoped project override. Do not modify vanilla files or capture prefab composition to conceal unrelated errors. Physical M4.4I acceptance stays pending until the clean-resource gate is resolved. No M4.5 or commit.
