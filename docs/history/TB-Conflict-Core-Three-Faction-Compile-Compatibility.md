# Three-faction compile compatibility correction — 2026-10-02

FOLLOW-UP WORKBENCH FAILURE: the subsequent real compile rejected the mixed
float/remainder staging expression. The override correction below is retained,
but its static PASS was not an Enforce compile PASS. See the latest
[Enforce operator/type-context correction](TB-Conflict-Core-Three-Faction-Enforce-Operators.md)
for the narrow source correction, whole-tree operator inventory, validator
regression and supported ScriptEditor/NET API compiler-feedback findings.

Status: REPORTED COMPILE FAILURE CORRECTED / REPOSITORY-WIDE STATIC CHECKS PASS /
WORKBENCH GATE REQUIRED. This is not an Enforce compile or runtime PASS.

## First causal failure and exact correction

Workbench reported `TB_CC_M4XP_IntegratedProof.c(11)`: overloaded function
ValidateCandidate not compatible.

Current production parent, TB_PVE_M3_3_TacticalStagingPolicy:

```c
protected bool ValidateCandidate(vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true)
```

Incompatible proof declaration before this correction:

```c
override protected bool ValidateCandidate(vector targetPosition, vector candidate, array<vector> observers, out string rejection)
```

Both return bool, are protected instance methods, and belong to ordinary
inheritance, not a modded class. The first four parameters match type, name and
order: vector targetPosition; vector candidate; array<vector> observers;
out string rejection. Production gained a FIFTH bool enforceAnnulus parameter
(default true) in the corrective FIA staging implementation.

A default preserves callers that omit the fifth argument; it does not make a
four-parameter override compatible with the five-parameter prototype. The prior
validator scanned selected production files and omitted IntegratedProof.
Its declaration parser could also mistake an in-body "return Method(...)" for
a declaration and overwrite a prototype. Both audit defects are corrected.

The proof now declares the identical five parameters/default and passes
enforceAnnulus to super.ValidateCandidate. Deny-all and sector-restriction
behaviour remains intact; accepted candidates still execute production
validation. Production validation was not weakened or edited.

Every ValidateCandidate declaration and caller was rechecked. The only
subclass override is the corrected IntegratedProof fixture. Production's
normal four-argument calls retain default annulus enforcement; FIA's explicit
false bypasses only the source-to-target annulus, not terrain/nav/human safety.
ValidateCandidateForProof retains four parameters and normal default validation.

## Other defects and conservative expression corrections

No other stale override/signature mismatch was found after resolving all
54 actual override declarations, including historical proof/test classes,
native campaign subclasses, synthetic ownership/radio adapters and modded
vanilla seams. Four other text occurrences of "override" are comments.

Three historical compiled diagnostics exceeded PrintFormat's nine value slots:

- TB_CC_M4_2_ArlandSurvey.c: BASE had ten values, with bool controlPoint in the
  position reserved for LogLevel; retain nine BASE fields and emit the
  control-point value in a second keyed diagnostic.
- TB_PVE_M2_5_CampaignOutcomeProof.c: COMPLETE had ten values; callback counts
  now appear together in a second winner-keyed diagnostic.
- TB_PVE_M3_1_AutonomousAttackProof.c: final-evidence had ten values;
  leaseReleased now appears in a second target-keyed diagnostic.

No diagnostic evidence or proof assertion was removed. These are static
API/format hazards discovered outside the previous audit, not claims of
additional Workbench-reported errors.

Two older M3.7 diagnostic snapshot concatenations had nine/twelve plus
operators. They are split into bounded append statements with the exact
same separators, field order and content. Participant/observer semantics,
change detection and gameplay behaviour are unchanged. This reduces known
expression-complexity risk; it does not establish that those old expressions
were independently rejected by Workbench.

## Contract/dependency scope

All 45 files in Scripts/Game are scanned, not just the corrective files.
All BCM inheritance chains are resolved locally; relevant engine parents are
read from installed vanilla archives. All overrides compare return type,
parameter count/type/name/order/default and out/ref tokens; static/instance
semantics and visibility compatibility are checked. Public widening of a
protected lifecycle callback remains allowed; access narrowing is rejected.

Changed existing contracts established from the preceding corrective work:

| Contract | Current signature / consumer result |
|---|---|
| TacticalStagingPolicy.ValidateCandidate | Fifth optional bool enforceAnnulus=true; proof corrected; normal four-value calls and FIA five-value call valid |
| ForceAssemblyService.SpawnGroup | Entry, vector position, vector targetPosition, int ordinal, FactionKey factionKey; its assembly caller supplies all five |
| ArlandNativeDefinition.Create | Optional array<ref TB_CC_TerritoryImportanceOverride> importanceOverrides=null; three zero-value consumers and one override-array consumer valid |

The garrison manager's independently scoped SpawnGroup still takes four
parameters, with MaterializationTrace as the fourth; it is NOT an override
of the assembly service and must not be mechanically changed to five.

The exhaustive current contract inventory below also covers new scoring,
FIA selection/recapture, occupation claim/adoption/defensive orders,
consolidation, ownership/depth/HQ accessors, owner-aware garrison configuration,
radio proof hooks, faction expansion, capture policy, human-presence consumers
and snapshot publication. All 137 direct calls in that inventory satisfy
current arity/default contracts. Class-scoped declarations are distinguished
from calls. No implicit inherited same-name declaration was found.

New constructors: occupation service takes one native policy; FIA coordinator
is zero-argument and Start takes policy plus cooldown. Native occupation uses
a no-argument TickTerritorialOccupation callback with matching removal;
FIA's Tick callback is likewise no-argument. Ownership callbacks retain the
base/previousFaction/newFaction prototype and existing event subscriptions.
GetOnDefenceEstablished invokes the documented id/faction pair; no consumer
subscription currently exists. New group/record/container field types and
critical typed caller arguments were reviewed in their owning classes.
No new RPC/signature was added.

Without a trustworthy pre-edit snapshot, no claim is made that static tooling
can reconstruct every earlier historical signature change. The report lists
the established recent changes and audits the CURRENT complete override tree
and expanded consumer surface instead.

## Installed API and FIA resources

Re-read actual installed SCR_AIGroup source:

- AddAgentFromControlledEntity(notnull IEntity controlledEntity) returns void.
- SetFaction(Faction faction) returns bool.
- GetFaction() returns Faction.

Re-read installed generated AIGroup/AIAgent/FactionAffiliationComponent:

- GetAgents(notnull out array<AIAgent> outAgents) returns int;
  GetAgentsCount() returns int.
- AddWaypoint(AIWaypoint w), RemoveWaypoint(AIWaypoint w) return void.
- GetLOD() returns int; static GetMaxLOD() returns int.
- GetAffiliatedFaction() returns Faction.

Uses supply initialized arrays/entities and the correct class/record types.
The prior resource/spawn, queue/invoker, native terrain/nav and container
contracts are retained. No invented activation API, forced targeting,
additional LOD manipulation or new native dependency was added by this fix.

Registered FIA identities remain:
F22EDFBEFC193357 (Remnants), 8D2035F179B9E08F (MG),
079F822A6E7FEA30 (AT). All five compiled FIA resource references are non-zero,
registered and resolve to installed prefab resources. Their character slots
and FIA inheritance were revalidated from installed data. No resource DB,
world, manager or vanilla file was modified.

## Validation result and boundaries

Read-only command:

```powershell
& 'C:/Users/ruanb/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' .\Tools\Test-ThreeFactionCorrectiveStatic.py
```

Exit code 0. Results: 45 source files; 479 PrintFormat calls; 94 unique BCM
class definitions; 54 complete override comparisons; 137 audited direct
calls; 371 local enum references; unchanged 13-field snapshot; valid FIA
GUIDs/inheritance; capture-superiority boundary/model checks; no ternary
expressions or single-line string formulas above the conservative six-plus
threshold. Full inventory follows.

The tool is NOT an Enforce parser/type checker. Same-name methods on unrelated
classes are recorded separately and their call-arity checks accept applicable
candidate signatures; critical receiver/argument types were manually reviewed.
It cannot certify arbitrary expression budgets, all engine implicit conversions,
resource-generated code, overload resolution, runtime initialization or AI combat.

Official [startup documentation](https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters)
documents -wbsilent as a Workbench initialization/script-compilation path.
The installed Tools directory has ArmaReforgerWorkbenchSteamDiag.exe, not a
separate standalone Enforce compiler. Existing project evidence records
automated Workbench startup failing with 0xc0000142 while normal interactive
Workbench works. No new evidence establishes that automation environment is
repaired. Therefore no known-broken invocation was repeated and no actual
Enforce compilation was invoked this pass. Normal Workbench remains final
compile authority in the established environment. No ServerDiag/gameplay,
publishing or commit was performed.

## Complete files changed in this compatibility pass

1. Scripts/Game/TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c
2. Scripts/Game/TB_CC_M4_2/TB_CC_M4_2_ArlandSurvey.c
3. Scripts/Game/TB_PVE_M2_5/TB_PVE_M2_5_CampaignOutcomeProof.c
4. Scripts/Game/TB_PVE_M3_1/TB_PVE_M3_1_AutonomousAttackProof.c
5. Scripts/Game/TB_PVE_M3_7/TB_PVE_M3_7_HumanPresenceQuery.c
6. Scripts/Game/TB_PVE_M3_7/TB_PVE_M3_7_PlaytestTelemetry.c
7. Tools/Test-ThreeFactionCorrectiveStatic.py
8. Documentation/TB-Conflict-Core-Three-Faction-Compile-Compatibility.md
9. Documentation/TB-Conflict-Core-Three-Faction-Corrective-Pass.md

All changed files were reread after correction. No production scoring,
recapture, faction model, occupation policy, graph, world hierarchy,
RadioManager, AIWorld or PerceptionManager redesign was made. Accepted runtime
logs remain unmodified; this compile-time defect is not reclassified as a
gameplay failure.

WORKBENCH GATE REQUIRED: compile normally and return the compiler result.
Do not infer readiness to publish or run gameplay from this static pass.

## Full source / override / contract / caller validation inventory

```text
SOURCE CHECK PASS TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c
SOURCE CHECK PASS TB_CC_M4X/TB_CC_M4XP_NativeArland.c
SOURCE CHECK PASS TB_CC_M4X/TB_CC_M4X_DependencyAudit.c
SOURCE CHECK PASS TB_CC_M4X/TB_CC_M4X_NativeCampaign.c
SOURCE CHECK PASS TB_CC_M4X/TB_CC_M4X_PhysicalProof.c
SOURCE CHECK PASS TB_CC_M4X/TB_CC_TerritorialOccupation.c
SOURCE CHECK PASS TB_CC_M4_1/TB_CC_M4_1_ArlandFixture.c
SOURCE CHECK PASS TB_CC_M4_1/TB_CC_M4_1_MapDefinition.c
SOURCE CHECK PASS TB_CC_M4_1/TB_CC_M4_1_StrategicPositionRegistry.c
SOURCE CHECK PASS TB_CC_M4_2/TB_CC_M4_2_ArlandProductionDefinition.c
SOURCE CHECK PASS TB_CC_M4_2/TB_CC_M4_2_ArlandSurvey.c
SOURCE CHECK PASS TB_CC_M4_2A/TB_CC_M4_2A_ArlandStartDefinition.c
SOURCE CHECK PASS TB_CC_M4_2A/TB_CC_M4_2A_CampaignStart.c
SOURCE CHECK PASS TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c
SOURCE CHECK PASS TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluatorProof.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4H_VictoryPressureProof.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4I_PlacementProbe.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4_PhysicalAuthorityProof.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4_RemainingAuthorityProof.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_M4_4_StrategicCaptureConnectivity.c
SOURCE CHECK PASS TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c
SOURCE CHECK PASS TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c
SOURCE CHECK PASS TB_PVE_M1_3/TB_PVE_M1_3_NoPlayerDeployment.c
SOURCE CHECK PASS TB_PVE_M1_5/TB_PVE_M1_5_HumanFactionRestriction.c
SOURCE CHECK PASS TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c
SOURCE CHECK PASS TB_PVE_M2_1/TB_PVE_M2_1_VerticalSliceProof.c
SOURCE CHECK PASS TB_PVE_M2_3/TB_PVE_M2_3_FrontierQuery.c
SOURCE CHECK PASS TB_PVE_M2_4/TB_PVE_M2_4_ProductionRouteProof.c
SOURCE CHECK PASS TB_PVE_M2_5/TB_PVE_M2_5_CampaignOutcomeProof.c
SOURCE CHECK PASS TB_PVE_M2_5/TB_PVE_M2_5_TerminalHQCapturePolicy.c
SOURCE CHECK PASS TB_PVE_M3_1/TB_PVE_M3_1_AutonomousAttackProof.c
SOURCE CHECK PASS TB_PVE_M3_1/TB_PVE_M3_1_Operation.c
SOURCE CHECK PASS TB_PVE_M3_2/TB_PVE_M3_2_OPFORCoordinatorProof.c
SOURCE CHECK PASS TB_PVE_M3_2/TB_PVE_M3_2_OPFOROperationCoordinator.c
SOURCE CHECK PASS TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c
SOURCE CHECK PASS TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingProof.c
SOURCE CHECK PASS TB_PVE_M3_4/TB_PVE_M3_4_ForcePackageProof.c
SOURCE CHECK PASS TB_PVE_M3_4/TB_PVE_M3_4_ForcePackages.c
SOURCE CHECK PASS TB_PVE_M3_5/TB_PVE_M3_5_ForceSelection.c
SOURCE CHECK PASS TB_PVE_M3_5/TB_PVE_M3_5_ForceSelectionProof.c
SOURCE CHECK PASS TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrison.c
SOURCE CHECK PASS TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrisonProof.c
SOURCE CHECK PASS TB_PVE_M3_7/TB_PVE_M3_7_HumanPresenceQuery.c
SOURCE CHECK PASS TB_PVE_M3_7/TB_PVE_M3_7_PlaytestTelemetry.c
PrintFormat calls checked: 479
Unique BCM class definitions: 94
13-field snapshot / invalid activation reference checks PASS
Capture-superiority boundary checks PASS
FIA RESOURCE PASS F22EDFBEFC193357 Prefabs/Groups/Campaign/Group_FIA_Remnants.et slots= 3
FIA RESOURCE PASS 8D2035F179B9E08F Prefabs/Groups/Campaign/Group_FIA_Remnants_MG.et slots= 2
FIA RESOURCE PASS 079F822A6E7FEA30 Prefabs/Groups/Campaign/Group_FIA_Remnants_AT.et slots= 2
RANKING US START (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::MILITARY_SOUTH score= 58 distance= 977.3 importance/proximity/consolidation/connectivity/variation= (34, 10, 6, 8, 2)
  ARLAND::MOSS_HILL score= 54 distance= 743.8 importance/proximity/consolidation/connectivity/variation= (30, 15, 6, 3, 1)
RANKING US AFTER MILITARY SOUTH (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::MOSS_HILL score= 52 distance= 1084.0 importance/proximity/consolidation/connectivity/variation= (30, 8, 13, 1, 1)
  ARLAND::GULL_ISLAND score= 51 distance= 645.4 importance/proximity/consolidation/connectivity/variation= (28, 17, 6, 0, 2)
  ARLAND::AIRPORT_NORTH score= 50 distance= 1138.1 importance/proximity/consolidation/connectivity/variation= (34, 7, 6, 3, 2)
  ARLAND::ARLEVILLE score= 49 distance= 1325.5 importance/proximity/consolidation/connectivity/variation= (32, 3, 6, 8, 1)
  ARLAND::FARM score= 42 distance= 1318.8 importance/proximity/consolidation/connectivity/variation= (24, 4, 6, 8, 0)
RANKING US TWO NEARBY ORDINARY POSITIONS (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::ARLEVILLE score= 68 distance= 630.5 importance/proximity/consolidation/connectivity/variation= (32, 17, 13, 6, 1)
  ARLAND::GULL_ISLAND score= 51 distance= 645.4 importance/proximity/consolidation/connectivity/variation= (28, 17, 6, 0, 2)
  ARLAND::AIRPORT_NORTH score= 50 distance= 1138.1 importance/proximity/consolidation/connectivity/variation= (34, 7, 6, 3, 2)
  ARLAND::FARM score= 42 distance= 1318.8 importance/proximity/consolidation/connectivity/variation= (24, 4, 6, 8, 0)
RANKING US CONSOLIDATED ARLEVILLE (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::SIGNAL_HILL score= 58 distance= 636.7 importance/proximity/consolidation/connectivity/variation= (32, 17, 6, 3, 0)
  ARLAND::GULL_ISLAND score= 51 distance= 645.4 importance/proximity/consolidation/connectivity/variation= (28, 17, 6, 0, 1)
  ARLAND::AIRPORT_NORTH score= 50 distance= 1138.1 importance/proximity/consolidation/connectivity/variation= (34, 7, 6, 3, 2)
  ARLAND::BEAUREGARD_HARBOUR score= 50 distance= 833.4 importance/proximity/consolidation/connectivity/variation= (28, 13, 6, 3, 0)
  ARLAND::FARM score= 47 distance= 1318.8 importance/proximity/consolidation/connectivity/variation= (24, 4, 13, 6, 2)
  ARLAND::BEAUREGARD score= 39 distance= 1310.1 importance/proximity/consolidation/connectivity/variation= (26, 4, 6, 3, 1)
RANKING USSR START (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::AIRPORT_NORTH score= 66 distance= 440.7 importance/proximity/consolidation/connectivity/variation= (34, 21, 6, 5, 2)
  ARLAND::FARM score= 48 distance= 1115.4 importance/proximity/consolidation/connectivity/variation= (24, 8, 6, 10, 1)
RANKING USSR AFTER AIRPORT NORTH (all graph candidates assumed radio-eligible; not runtime radio proof)
  ARLAND::NORTH_RUNWAY score= 56 distance= 736.1 importance/proximity/consolidation/connectivity/variation= (34, 15, 6, 1, 0)
  ARLAND::MILITARY_SOUTH score= 53 distance= 1138.1 importance/proximity/consolidation/connectivity/variation= (34, 7, 6, 6, 1)
  ARLAND::FARM score= 50 distance= 1262.3 importance/proximity/consolidation/connectivity/variation= (24, 5, 13, 8, 2)
STATIC MODEL CHECKS PASS; WORKBENCH COMPILE AND RUNTIME ACCEPTANCE STILL REQUIRED
OVERRIDE TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c TB_CC_M4XP_CorrectiveStagingPolicy.ValidateCandidate ('bool', 'vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true', ['override', 'protected']) PARENT TB_PVE_M3_3_TacticalStagingPolicy ('bool', 'vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c TB_CC_M4XP_IntegratedProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c TB_CC_M4XP_IntegratedProofComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_VanillaRadioEligibilityView.IsReachable ('bool', 'string positionId, out string reason', ['override']) PARENT TB_CC_M4_3_RadioEligibilityView ('bool', 'string positionId, out string reason', [])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_GameMode.InitalBaseSetup ('void', '', ['override', 'protected']) PARENT TB_CC_M4X_GameMode ('void', '', ['override', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c SCR_GameModeCampaign.InitalBaseSetup ('void', '', ['override', 'protected']) PARENT SCR_GameModeCampaign ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c SCR_GameModeCampaign.CheckForWinner ('void', '', ['override', 'protected']) PARENT SCR_GameModeCampaign ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c SCR_CampaignFactionCommanderHandlerComponent.CreateSeizeTask ('void', 'notnull SCR_CampaignFaction faction, SCR_CampaignMilitaryBaseComponent targetBase, int playerid', ['override']) PARENT SCR_CampaignFactionCommanderHandlerComponent ('void', 'notnull SCR_CampaignFaction faction, SCR_CampaignMilitaryBaseComponent targetBase, int playerid', [])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c SCR_CampaignFactionCommanderHandlerComponent.CheckSeizeTasks ('void', 'notnull SCR_CampaignFaction faction, int playerid', ['override', 'protected']) PARENT SCR_CampaignFactionCommanderHandlerComponent ('void', 'notnull SCR_CampaignFaction faction, int playerid', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.StartM3Consumers ('void', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.StopM3Consumers ('void', 'string reason', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', 'string reason', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.AdditionalSemanticSignature ('string', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('string', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.GetBLUFORLegalObjective ('string', '', ['override']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('string', '', [])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.GetOPFORLegalObjective ('string', '', ['override']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('string', '', [])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.Reevaluate ('void', 'string reason', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', 'string reason', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.CheckVictoryPressure ('void', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.OnBaseFactionChanged ('void', 'SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', 'SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', 'IEntity owner', ['override'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.CreateRadioEligibilityView ('TB_CC_M4_3_RadioEligibilityView', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('TB_CC_M4_3_RadioEligibilityView', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.PublishAdditionalProjection ('void', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.InitializeClientDefinition ('void', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.BuildRegistryAndSpatialQuery ('bool', '', ['override', 'protected']) PARENT TB_CC_M4X_NativeCampaignComponent ('bool', '', ['override', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.InitializeAuthority ('void', '', ['override', 'protected']) PARENT TB_CC_M4X_NativeCampaignComponent ('void', '', ['override', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4XP_NativeArland.c TB_CC_M4XP_NativeArlandComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT TB_CC_M4X_NativeCampaignComponent ('void', 'IEntity owner', ['override'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_DependencyAudit.c TB_CC_M4X_DependencyAuditComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_GameMode.InitalBaseSetup ('void', '', ['override', 'protected']) PARENT SCR_GameModeCampaign ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_GameMode.CheckForWinner ('void', '', ['override', 'protected']) PARENT SCR_GameModeCampaign ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_GameMode.TB_PVE_M1_1_Begin ('void', '', ['override', 'protected']) PARENT SCR_GameModeCampaign ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_OffMapStartState.RequiresPhysicalOPFOREndpoint ('bool', '', ['override']) PARENT TB_CC_M4_2A_CampaignStartState ('bool', '', [])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_NativeCampaignComponent.BuildRegistryAndSpatialQuery ('bool', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('bool', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_NativeCampaignComponent.InitializeAuthority ('void', '', ['override', 'protected']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_NativeCampaign.c TB_CC_M4X_NativeCampaignComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT TB_CC_M4_4_ArlandGraphCampaignComponent ('void', 'IEntity owner', ['override'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_PhysicalProof.c TB_CC_M4X_PhysicalProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4X/TB_CC_M4X_PhysicalProof.c TB_CC_M4X_PhysicalProofComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_1/TB_CC_M4_1_ArlandFixture.c TB_CC_M4_1_ProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_1/TB_CC_M4_1_StrategicPositionRegistry.c TB_CC_M4_1_VanillaBaseOwnershipAdapter.TryGetCurrentOwner ('bool', 'out FactionKey owner', ['override']) PARENT TB_CC_M4_1_OwnershipAdapter ('bool', 'out FactionKey owner', [])
OVERRIDE TB_CC_M4_2/TB_CC_M4_2_ArlandProductionDefinition.c TB_CC_M4_2_ProductionGraphProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_2/TB_CC_M4_2_ArlandSurvey.c TB_CC_M4_2_ArlandSurveyComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_2A/TB_CC_M4_2A_ArlandStartDefinition.c TB_CC_M4_2A_ProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c TB_CC_M4_3_VanillaOwnershipView.TryGetOwner ('bool', 'string positionId, out FactionKey owner', ['override']) PARENT TB_CC_M4_3_OwnershipView ('bool', 'string positionId, out FactionKey owner', [])
OVERRIDE TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c TB_CC_M4_3_SyntheticOwnershipView.TryGetOwner ('bool', 'string positionId, out FactionKey owner', ['override']) PARENT TB_CC_M4_3_OwnershipView ('bool', 'string positionId, out FactionKey owner', [])
OVERRIDE TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluatorProof.c TB_CC_M4_3_SyntheticRadioView.IsReachable ('bool', 'string positionId, out string reason', ['override']) PARENT TB_CC_M4_3_RadioEligibilityView ('bool', 'string positionId, out string reason', [])
OVERRIDE TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluatorProof.c TB_CC_M4_3_StrategicEvaluatorProofComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c TB_CC_M4_4_ArlandGraphCampaignComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c TB_CC_M4_4_ArlandGraphCampaignComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_CC_M4_4/TB_CC_M4_4_StrategicCaptureConnectivity.c SCR_CampaignSeizingComponent.OnPrevailingFactionChanged ('void', '', ['override', 'protected']) PARENT SCR_SeizingComponent ('void', '', ['protected'])
OVERRIDE TB_CC_M4_4/TB_CC_M4_4_StrategicCaptureConnectivity.c SCR_CampaignSeizingComponent.EvaluatePrevailingFaction ('void', '', ['override', 'protected']) PARENT SCR_CampaignSeizingComponent ('void', '', ['override', 'protected'])
OVERRIDE TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c SCR_GameModeCampaign.OnGameStart ('void', '', ['override']) PARENT SCR_GameModeCampaign ('void', '', ['override'])
OVERRIDE TB_PVE_M1_3/TB_PVE_M1_3_NoPlayerDeployment.c SCR_CampaignSpawnPointGroup.IsSpawnPointVisibleForPlayer ('bool', 'int pid', ['override']) PARENT SCR_SpawnPoint ('bool', 'int pid', [])
OVERRIDE TB_PVE_M1_5/TB_PVE_M1_5_HumanFactionRestriction.c SCR_GameModeCampaign.OnGameStart ('void', '', ['override']) PARENT SCR_GameModeCampaign ('void', '', ['override'])
OVERRIDE TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c TB_PVE_M2_1_LinearFrontierComponent.OnPostInit ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c TB_PVE_M2_1_LinearFrontierComponent.OnDelete ('void', 'IEntity owner', ['override']) PARENT ScriptComponent ('void', 'IEntity owner', ['event', 'protected'])
OVERRIDE TB_PVE_M2_1/TB_PVE_M2_1_LinearFrontier.c SCR_SeizingComponent.EvaluateEntityFaction ('SCR_Faction', 'IEntity ent', ['override', 'protected']) PARENT SCR_SeizingComponent ('SCR_Faction', 'IEntity ent', ['protected'])
OVERRIDE TB_PVE_M2_5/TB_PVE_M2_5_TerminalHQCapturePolicy.c SCR_CampaignSeizingComponent.EvaluateEntityFaction ('SCR_Faction', 'IEntity ent', ['override', 'protected']) PARENT SCR_CampaignSeizingComponent ('SCR_Faction', 'IEntity ent', ['protected', 'override'])
Complete override return/type/name/order checks PASS: 54
FIA selector tier reachability / strength caps PASS
FIA recency / ordinary-only / zero-territory / unsupported-foot-target model PASS
No gameplay ownership writes / string-formula checks PASS
CONTRACT TB_CC_M4XP_CorrectiveStagingPolicy.ValidateCandidate ('bool', 'vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true', ['override', 'protected'])
CONTRACT SCR_GameModeCampaign.TB_CC_SetNativePressureDeadline ('void', 'WorldTimestamp deadline', [])
CONTRACT SCR_GameModeCampaign.TB_CC_PublishNativeSnapshot ('void', 'string snapshot', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetOccupationService ('TB_CC_TerritorialOccupationService', '', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetConsolidationSeconds ('int', '', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetTerritorialNeighbours ('void', 'string id, out array<string> neighbours', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.IsFootTerritory ('bool', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.CanFIARecapture ('bool', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.SelectFIARecaptureTarget ('string', '', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.SelectFIAPackage ('string', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.StartM3Consumers ('void', '', ['override', 'protected'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.StopM3Consumers ('void', 'string reason', ['override', 'protected'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.OwnerCsv ('string', '', ['protected'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.ConsumeNativeSnapshot ('void', 'string snapshot', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetHQState ('int', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetTerritorialOwner ('FactionKey', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetFactionDepth ('int', 'FactionKey faction', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetPositionImportance ('int', 'string id', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.AuthorizeCapture ('bool', 'SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetBLUFORLegalObjective ('string', '', ['override'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetOPFORLegalObjective ('string', '', ['override'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.OnBaseFactionChanged ('void', 'SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction', ['override', 'protected'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.AllowsNativeSeizeTask ('bool', 'Faction faction, SCR_CampaignMilitaryBaseComponent base', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.GetPresentedTaskId ('string', 'string legal, int revision', ['protected'])
CONTRACT TB_CC_M4XP_NativeArlandComponent.SetCorrectiveProofRadioView ('bool', 'TB_CC_M4_3_RadioEligibilityView view', [])
CONTRACT TB_CC_M4XP_NativeArlandComponent.CorrectiveProofTaskMatchesLegal ('bool', '', [])
CONTRACT TB_CC_TerritorialOccupationService.GetOnDefenceEstablished ('ScriptInvoker', '', [])
CONTRACT TB_CC_TerritorialOccupationService.HasDefence ('bool', 'string id', [])
CONTRACT TB_CC_TerritorialOccupationService.HasClaim ('bool', 'string id', [])
CONTRACT TB_CC_TerritorialOccupationService.ReserveConsolidation ('void', 'string id, FactionKey faction', [])
CONTRACT TB_CC_TerritorialOccupationService.ReleaseConsolidation ('void', 'string id, FactionKey faction', [])
CONTRACT TB_CC_TerritorialOccupationService.SetDefensiveOrder ('bool', 'TB_PVE_M3_4_AssembledGroup record, vector position', ['static'])
CONTRACT TB_CC_TerritorialOccupationService.Adopt ('bool', 'string id, FactionKey faction, array<ref TB_PVE_M3_4_AssembledGroup> survivors, bool requireOrders = true', [])
CONTRACT TB_CC_FIARecaptureCoordinator.IsAttacking ('bool', 'string id', [])
CONTRACT TB_CC_M4_3_CandidateDecision.SetTerritorialScore ('void', 'int importance, int proximity, int consolidation, int connectivity, int variation', [])
CONTRACT TB_CC_M4_3_CandidateDecision.GetProximityScore ('int', '', [])
CONTRACT TB_CC_M4_3_CandidateDecision.GetConsolidationScore ('int', '', [])
CONTRACT TB_CC_M4_3_StrategicEvaluator.ConfigureTerritorialScoring ('void', 'int importance, int proximity, int consolidation, int connectivity, float proximityScale, int variation', [])
CONTRACT TB_CC_M4_3_StrategicEvaluator.ConfigureExpansion ('void', 'FactionKey faction, bool threeFaction', [])
CONTRACT TB_CC_M4_3_StrategicEvaluator.IsHostileTerritorialOwner ('bool', 'FactionKey owner', ['protected'])
CONTRACT TB_CC_M4_3_StrategicEvaluator.Evaluate ('bool', 'TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_2A_CampaignStartState startState, TB_CC_M4_3_OwnershipView ownership, string lastCapturedObjectiveId = "", TB_CC_M4_3_RadioEligibilityView radio = null', [])
CONTRACT TB_CC_M4_3_StrategicEvaluator.ScoreTerritorialCandidate ('void', 'TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_1_StrategicPositionDefinition definition, array<string> adjacent, int seed, int index, TB_CC_M4_3_CandidateDecision decision', ['protected'])
CONTRACT TB_CC_M4_3_StrategicEvaluator.GetBLUFORLegalObjective ('string', '', [])
CONTRACT TB_CC_M4_3_StrategicEvaluator.GetOPFORLegalObjective ('string', '', [])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.OnBaseFactionChanged ('void', 'SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction', ['protected'])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.StartM3Consumers ('void', '', ['protected'])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.StopM3Consumers ('void', 'string reason', ['protected'])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.GetBLUFORLegalObjective ('string', '', [])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.GetOPFORLegalObjective ('string', '', [])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.GetEvaluator ('TB_CC_M4_3_StrategicEvaluator', '', [])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.GetCoordinator ('TB_PVE_M3_2_OPFOROperationCoordinator', '', [])
CONTRACT TB_CC_M4_4_ArlandGraphCampaignComponent.GetGarrisonManager ('TB_PVE_M3_6_OPFORFrontlineGarrisonManager', '', [])
CONTRACT SCR_CampaignSeizingComponent.TB_CC_UseThreeFactionCapture ('bool', '', ['protected'])
CONTRACT SCR_CampaignSeizingComponent.TB_CC_M4_4_ConfigurePvECaptureRules ('void', '', [])
CONTRACT TB_PVE_M2_1_LinearFrontierComponent.OnBaseFactionChanged ('void', 'SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction', ['protected'])
CONTRACT TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective ('string', '', ['static'])
CONTRACT TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective ('string', '', ['static'])
CONTRACT TB_PVE_M3_1_Operation.ConfigureTerritorialAttack ('void', 'FactionKey faction, string target', [])
CONTRACT TB_PVE_M3_1_Operation.IsTargetAuthorized ('bool', '', ['protected'])
CONTRACT TB_PVE_M3_1_Operation.BeginConsolidation ('void', 'TB_CC_M4XP_NativeArlandComponent policy', ['protected'])
CONTRACT TB_PVE_M3_1_Operation.TickConsolidation ('void', 'TB_CC_M4XP_NativeArlandComponent policy', ['protected'])
CONTRACT TB_PVE_M3_3_TacticalStagingPolicy.FindStagingPosition ('bool', 'FactionKey factionKey, string targetObjectiveId, vector targetPosition, array<vector> observerPositions, out vector stagingPosition, out string failureReason', [])
CONTRACT TB_PVE_M3_3_TacticalStagingPolicy.ValidateCandidate ('bool', 'vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true', ['protected'])
CONTRACT TB_PVE_M3_3_TacticalStagingPolicy.FindFIATerritorialStaging ('bool', 'string targetId, vector targetPosition, array<vector> observers, array<vector> used, int ordinal, out vector result, out string failure', ['protected'])
CONTRACT TB_PVE_M3_3_TacticalStagingPolicy.ValidateCandidateForProof ('bool', 'vector targetPosition, vector candidate, array<vector> observers, out string rejection', [])
CONTRACT TB_PVE_M3_3_TacticalStagingPolicy.FindAdditionalGroupPosition ('bool', 'FactionKey factionKey, string targetObjectiveId, vector targetPosition, array<vector> usedPositions, int groupOrdinal, array<vector> observerPositions, out vector stagingPosition, out string failureReason', [])
CONTRACT TB_PVE_M3_4_ForcePackageRegistry.ConfigureFIAPackages ('void', 'int light, int rifles, int mg, int at', ['static'])
CONTRACT TB_PVE_M3_4_ForceAssemblyService.AssemblePackage ('bool', 'string packageId, FactionKey factionKey, string targetId, vector targetPosition, TB_PVE_M3_3_TacticalStagingPolicy stagingPolicy, array<vector> observers, out array<ref TB_PVE_M3_4_AssembledGroup> assembledGroups, out string failureReason', [])
CONTRACT TB_PVE_M3_4_ForceAssemblyService.SpawnGroup ('TB_PVE_M3_4_AssembledGroup', 'TB_PVE_M3_4_ForcePackageEntry entry, vector position, vector targetPosition, int ordinal, FactionKey factionKey', ['protected'])
CONTRACT TB_PVE_M3_5_ForceSelectionPolicy.CountActiveHumanBLUFOR ('int', '', [])
CONTRACT TB_PVE_M3_6_OPFORFrontlineGarrisonManager.SetOPFORFront ('void', 'bool enabled', [])
CONTRACT TB_PVE_M3_6_OPFORFrontlineGarrisonManager.ConfigureOwnerAware ('void', 'string light, string standard, string reinforced, string strongpoint, int maximumGroups, int conventionalStrength', [])
CONTRACT TB_PVE_M3_6_OPFORFrontlineGarrisonManager.SpawnGroup ('TB_PVE_M3_4_AssembledGroup', 'TB_PVE_M3_4_ForcePackageEntry entry, vector position, vector objectivePosition, TB_PVE_M3_6_MaterializationTrace trace', ['protected'])
CONTRACT TB_PVE_M3_6_OPFORFrontlineGarrisonManager.FindLocalPosition ('bool', 'vector objectivePosition, int ordinal, array<vector> observers, out vector result, out string failure', ['protected'])
CONTRACT TB_PVE_M3_6_OPFORFrontlineGarrisonManager.CountActiveHumanBLUFOR ('int', '', ['protected'])
CONTRACT TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount ('int', 'FactionKey factionKey', ['static'])
CONTRACT TB_PVE_M3_7_HumanPresenceQuery.GetActiveHumanObservers ('int', 'FactionKey factionKey, out array<vector> observerPositions', ['static'])
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 15 ValidateCandidate ['targetPosition', 'candidate', 'observers', 'rejection', 'enforceAnnulus']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 186 SetCorrectiveProofRadioView ['null']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 320 SetCorrectiveProofRadioView ['m_CorrectiveRadio']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 278 FindStagingPosition ['"USSR"', '"ARLAND::MILITARY_SOUTH"', 'target', 'observers', 'anchor', 'failure']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 300 FindStagingPosition ['"USSR"', '"ARLAND::MILITARY_SOUTH"', 'target', 'observers', 'impossible', 'failure']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 364 FindStagingPosition ['"USSR"', '"ARLAND::GULL_ISLAND"', 'objective', 'observers', 'staging', 'reason']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 283 ValidateCandidateForProof ['target', 'second', 'observers', 'validity']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 282 FindAdditionalGroupPosition ['"USSR"', '"ARLAND::MILITARY_SOUTH"', 'target', 'used', '1', 'observers', 'second', 'failure']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 290 FindAdditionalGroupPosition ['"USSR"', '"ARLAND::MILITARY_SOUTH"', 'target', 'used', '2', 'observers', 'third', 'failure']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 295 FindAdditionalGroupPosition ['"USSR"', '"ARLAND::MILITARY_SOUTH"', 'target', 'singleSectorUsed', '1', 'observers', 'degraded', 'failure']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_IntegratedProof.c 348 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 621 TB_CC_SetNativePressureDeadline ['m_PressureDeadline']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 622 TB_CC_PublishNativeSnapshot ['packet']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 271 GetTerritorialNeighbours ['id', 'neighbours']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 268 IsFootTerritory ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 272 IsFootTerritory ['neighbour']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 280 CanFIARecapture ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 322 StopM3Consumers ['reason']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 506 StopM3Consumers ['"terminal-HQ-capture"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 820 StopM3Consumers ['"native-component-deleted"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 104 ConsumeNativeSnapshot ['m_sTBCCNativeSnapshot']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 502 GetHQState ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 269 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 272 GetTerritorialOwner ['neighbour']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 332 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 401 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 404 GetTerritorialOwner ['neighbour']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 443 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 281 GetPositionImportance ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 291 GetPositionImportance ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 522 OnBaseFactionChanged ['base', 'previousFaction', 'newFaction']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 210 AllowsNativeSeizeTask ['faction', 'targetBase']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 583 GetPresentedTaskId ['legal', 'm_iPresentedRevision']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 650 GetPresentedTaskId ['legalId', 'm_iRevision']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 703 GetPresentedTaskId ['desired', 'revision']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 515 ReserveConsolidation ['id', '"USSR"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 516 ReserveConsolidation ['id', '"FIA"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 516 IsAttacking ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 776 ConfigureTerritorialScoring ['m_iTerritoryImportanceWeight', 'm_iTerritoryProximityWeight', 'm_iTerritoryConsolidationWeight', 'm_iTerritoryConnectivityWeight', 'm_fTerritoryProximityScale', 'm_iTerritoryVariation']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 779 ConfigureTerritorialScoring ['m_iTerritoryImportanceWeight', 'm_iTerritoryProximityWeight', 'm_iTerritoryConsolidationWeight', 'm_iTerritoryConnectivityWeight', 'm_fTerritoryProximityScale', 'm_iTerritoryVariation']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 775 ConfigureExpansion ['"US"', 'true']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 778 ConfigureExpansion ['"USSR"', 'true']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 434 Evaluate ['m_Registry', 'm_Start', 'owners', '""', 'CreateRadioEligibilityView()']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 435 Evaluate ['m_Registry', 'opforStart', 'owners']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 304 ConfigureFIAPackages ['m_iFIALightGroups', 'm_iFIARifleGroups', 'm_iFIAMGGroups', 'm_iFIAATGroups']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 315 SetOPFORFront ['true']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 310 ConfigureOwnerAware ['m_sFIALightPackage', 'm_sFIAStandardPackage', 'm_sFIAReinforcedPackage', 'm_sFIAStrongpointPackage', 'm_iMaximumGarrisonGroups', 'm_iFIAConventionalStrength']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 314 ConfigureOwnerAware ['m_sFIALightPackage', 'm_sFIAStandardPackage', 'm_sFIAReinforcedPackage', 'm_sFIAStrongpointPackage', 'm_iMaximumGarrisonGroups', 'm_iFIAConventionalStrength']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 289 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4XP_NativeArland.c 814 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4X_NativeCampaign.c 89 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_M4X_PhysicalProof.c 63 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 198 SelectFIAPackage ['target']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 17 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 22 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 49 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 94 GetTerritorialOwner ['claimId']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 99 GetTerritorialOwner ['id']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 82 ReleaseConsolidation ['id', 'faction']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 66 SetDefensiveOrder ['record', 'position']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 110 SetDefensiveOrder ['record', 'position']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 196 ConfigureTerritorialAttack ['"FIA"', 'target']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 192 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_CC_M4X\TB_CC_TerritorialOccupation.c 96 GetActiveHumanObservers ['"US"', 'observers']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluator.c 297 SetTerritorialScore ['importance', 'proximity', 'consolidation', 'connectivity', 'variation']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluator.c 180 IsHostileTerritorialOwner ['owner']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluator.c 193 IsHostileTerritorialOwner ['owners.Get(positionId)']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluator.c 230 IsHostileTerritorialOwner ['owners.Get(adjacentId)']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluator.c 258 ScoreTerritorialCandidate ['registry', 'definition', 'adjacent', 'seed', 'i', 'decision']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 45 Evaluate ['registry', 'start', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 50 Evaluate ['registry', 'start', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 53 Evaluate ['registry', 'Start(seed, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST")', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 61 Evaluate ['registry', 'start', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 65 Evaluate ['registry', 'Start(seed, "ARLAND::HQ_WEST", "ARLAND::HQ_EAST")', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 73 Evaluate ['registry', 'Start(30, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST")', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 83 Evaluate ['registry', 'start', 'owners', '"ARLAND::MILITARY_SOUTH"']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 86 Evaluate ['registry', 'start', 'owners', '"ARLAND::ARLEVILLE"']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 89 Evaluate ['registry', 'start', 'reverseOwners', '"ARLAND::MILITARY_SOUTH"']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 90 Evaluate ['registry', 'start', 'reverseOwners', '"ARLAND::MILITARY_SOUTH"']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 100 Evaluate ['registry', 'Start(50, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST")', 'owners']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 111 Evaluate ['registry', 'start', 'owners', '""', 'radio']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 114 Evaluate ['registry', 'start', 'owners', '""', 'none']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 117 Evaluate ['registry', 'start', 'owners', '""', 'none']
CALL Scripts\Game\TB_CC_M4_3\TB_CC_M4_3_StrategicEvaluatorProof.c 126 Evaluate ['registry', 'Start(5, "SCALE::P0", "SCALE::P127")', 'owners']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4H_VictoryPressureProof.c 40 FindStagingPosition ['"USSR"', '"ARLAND::GULL_ISLAND"', 'islandPosition', 'observers', 'staging', 'reason']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4I_PlacementProbe.c 8 FindLocalPosition ['target', '0', 'observers', 'result', 'reason']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c 259 StopM3Consumers ['"M4.4H-victory-pressure"']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c 414 StopM3Consumers ['"M4.4-fail-closed"']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c 465 AuthorizeCapture ['faction', 'base']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c 476 AuthorizeCapture ['faction', 'base']
CALL Scripts\Game\TB_CC_M4_4\TB_CC_M4_4_ArlandGraphCampaign.c 185 Evaluate ['m_Registry', 'm_Start', 'owners', 'm_sLastCapturedBLUFORObjective', 'radio']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 85 CanFIARecapture ['m_sTargetObjectiveId']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 132 GetTerritorialOwner ['m_sTargetObjectiveId']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 147 GetTerritorialOwner ['m_sTargetObjectiveId']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 186 GetTerritorialOwner ['m_sTargetObjectiveId']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 308 GetTerritorialOwner ['m_sTargetObjectiveId']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 177 ReserveConsolidation ['m_sTargetObjectiveId', 'm_sFactionKey']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 318 ReleaseConsolidation ['m_sTargetObjectiveId', 'm_sFactionKey']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 180 SetDefensiveOrder ['record', 'm_vTargetPosition']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 190 Adopt ['m_sTargetObjectiveId', 'm_sFactionKey', 'm_aForceGroups']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 200 Adopt ['m_sTargetObjectiveId', 'm_sFactionKey', 'm_aForceGroups', 'false']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 311 Adopt ['m_sTargetObjectiveId', 'm_sFactionKey', 'm_aForceGroups', 'false']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 134 BeginConsolidation ['policy']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 130 TickConsolidation ['policy']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 109 AssemblePackage ['m_sPackageId', 'm_sFactionKey', 'm_sTargetObjectiveId', 'm_vTargetPosition', 'stagingPolicy', 'observerPositions', 'm_aForceGroups', 'assemblyFailure']
CALL Scripts\Game\TB_PVE_M3_1\TB_PVE_M3_1_Operation.c 122 GetActiveHumanObservers ['"US"', 'observerPositions']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 78 ValidateCandidate ['targetPosition', 'candidate', 'observerPositions', 'rejection']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 200 ValidateCandidate ['targetPosition', 'candidate', 'observers', 'rejection', 'false']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 275 ValidateCandidate ['targetPosition', 'candidate', 'observers', 'rejection']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 310 ValidateCandidate ['targetPosition', 'candidate', 'observerPositions', 'rejection']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 182 GetTerritorialNeighbours ['targetId', 'neighbours']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 184 IsFootTerritory ['neighbour']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 180 CanFIARecapture ['targetId']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 184 GetTerritorialOwner ['neighbour']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 62 FindFIATerritorialStaging ['targetObjectiveId', 'targetPosition', 'observerPositions', 'none', '0', 'stagingPosition', 'failureReason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingPolicy.c 283 FindFIATerritorialStaging ['targetObjectiveId', 'targetPosition', 'observerPositions', 'usedPositions', 'groupOrdinal', 'stagingPosition', 'failureReason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingProof.c 106 FindStagingPosition ['"USSR"', 's_Route.GetRouteId(index)', 'ObjectivePosition(index)', 'observers', 'position', 'reason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingProof.c 62 ValidateCandidateForProof ['ObjectivePosition(1)', 'early', 'nearObserver', 'nearReason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingProof.c 74 ValidateCandidateForProof ['ObjectivePosition(1)', 'early', 'visibleObserver', 'visibilityReason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingProof.c 91 ValidateCandidateForProof ['coastalTarget', 'waterCandidate', 'noObservers', 'waterReason']
CALL Scripts\Game\TB_PVE_M3_3\TB_PVE_M3_3_TacticalStagingProof.c 98 ValidateCandidateForProof ['syntheticTarget', 'knownWater', 'noObservers', 'waterReason']
CALL Scripts\Game\TB_PVE_M3_4\TB_PVE_M3_4_ForcePackages.c 138 FindStagingPosition ['factionKey', 'targetId', 'targetPosition', 'observers', 'anchor', 'stagingFailure']
CALL Scripts\Game\TB_PVE_M3_4\TB_PVE_M3_4_ForcePackages.c 155 FindAdditionalGroupPosition ['factionKey', 'targetId', 'targetPosition', 'usedPositions', 'groupOrdinal', 'observers', 'groupPosition', 'stagingFailure']
CALL Scripts\Game\TB_PVE_M3_4\TB_PVE_M3_4_ForcePackages.c 159 SpawnGroup ['entry', 'groupPosition', 'targetPosition', 'groupOrdinal', 'factionKey']
CALL Scripts\Game\TB_PVE_M3_5\TB_PVE_M3_5_ForceSelection.c 56 GetTerritorialOwner ['TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective()']
CALL Scripts\Game\TB_PVE_M3_5\TB_PVE_M3_5_ForceSelection.c 56 GetFactionDepth ['"USSR"']
CALL Scripts\Game\TB_PVE_M3_5\TB_PVE_M3_5_ForceSelection.c 49 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 244 SelectFIAPackage ['target']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 147 GetTerritorialOwner ['target']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 149 GetTerritorialOwner ['m_Garrison.m_sObjectiveId']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 201 HasClaim ['target']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 269 SpawnGroup ['entry', 'position', 'objectivePosition', 'trace']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 267 FindLocalPosition ['objectivePosition', 'ordinal', 'observers', 'position', 'failure']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 480 GetParticipatingHumanCount ['"US"']
CALL Scripts\Game\TB_PVE_M3_6\TB_PVE_M3_6_FrontlineGarrison.c 474 GetActiveHumanObservers ['"US"', 'observers']
CALL Scripts\Game\TB_PVE_M3_7\TB_PVE_M3_7_HumanPresenceQuery.c 82 GetParticipatingHumanCount ['relevantFactionKey']
CALL Scripts\Game\TB_PVE_M3_7\TB_PVE_M3_7_HumanPresenceQuery.c 81 GetActiveHumanObservers ['relevantFactionKey', 'observers']
Whole-Game changed/new contract call-arity checks PASS: 137
Whole-Game local enum member checks PASS: 371
New constructor / factory / occupation callback contracts PASS
Implicit inherited same-name declaration checks PASS: 0
Whole-Game FIA reference GUID/resource-type checks PASS: 5
Whole-Game conditional / oversized string expression checks PASS
Repository-wide static checks complete: 45 Game source files; NOT Enforce compile PASS
```
