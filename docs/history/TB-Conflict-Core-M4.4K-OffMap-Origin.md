# M4.4K — off-map OPFOR origin and eleven-position territory

## Current status

**M4.4K IMPLEMENTATION/ACCEPTANCE PENDING — HQ RESOURCE AUTHORING GATE.**
The new Creative Director topology is approved design, not yet the serialized
production implementation. M4.4J's accepted implementation remains intact.
No gameplay, resource identity, placement, graph edge, duration, proof mode,
launcher, network configuration or vanilla file was changed during this audit.
No test server/client was launched. No M4.5 implementation or commit occurred.

## Approved model (supersedes the old design, not its historical evidence)

Physical strategic territory is not the OPFOR force origin. Runtime roles:

| Start | Selected protected/deployable US root | Other HQ: ordinary/capturable/non-deployable |
| --- | --- | --- |
| East | ARLAND::HQ_EAST | ARLAND::HQ_WEST |
| West | ARLAND::HQ_WEST | ARLAND::HQ_EAST |

Each orientation has eleven required ordinary positions, twelve physical graph
nodes and one abstract/non-territorial OPFOR origin. The origin must not be a
thirteenth graph node, a physical base, an ownership adapter or a player spawn.
The four Creative Director capture resources remain unchanged.

## Audit findings

1. `TB_CC_M4_2_ArlandProductionDefinition` already contains the twelve required
   physical locations and twenty-one undirected edges. HQ West connects to
   Airport North/Farm; HQ East connects to Military South/Moss Hill. Both roots
   and the other HQ are connected through the existing graph. No additional
   edge is presently indicated; runtime eligibility/progression must still be
   proved after role/resource changes.
2. The generic start resolver currently requires a resolved physical endpoint
   for every MOB relationship. The evaluator requires that endpoint, excludes
   it from ordinary objectives/counting/OPFOR response and treats both anchors
   as protected. These are actual code changes required by the new contract.
3. Campaign initialization, victory counting, authorization and the foothold
   callback also retain two-HQ assumptions. In particular, the foothold write
   uses `!base.IsHQ()` and must instead honor the required ordinary campaign
   role after dynamic physical HQ demotion.
4. Existing scoring's endpoint bonus is already zero. The unselected HQ must
   pass the same connectivity eligibility and score path as any other ordinary
   position; no "final endpoint" bonus or ordering rule should be added.
5. Both inherited Arland HQs lack `SCR_CampaignSeizingComponent`, as established
   in prior M4.4G authority evidence and explicitly confirmed by this brief.
   The current BCM world inherits `worlds/MP/CTI_Campaign_Arland.ent`; its saved
   M4_4_Campaign.layer contains the campaign system and four new capture entities,
   but no authored HQ instance override/component additions.
6. `IEntity` exposes component lookup, not runtime component creation. The
   installed Workbench API has source-level `WorldEditorAPI.CreateComponent`
   and undoable editor actions. Installed vanilla API also exposes
   `SCR_CampaignMilitaryBaseComponent.SetAsHQ(bool)`. Changing HQ status alone
   cannot supply the absent physical capture component.
7. Native desktop/Workbench UI control is unavailable in this tool session.
   There is no verified editable vanilla HQ source or authored source identity
   available in the workspace from which to safely serialize an instance
   override by hand. No packed vanilla extraction, invented entity ID, global
   HQ prefab override, duplicate overlapping capture base or resource-database
   fabrication was used as a substitute.

## Smallest required authoring step

This is resource preparation, NOT final human gameplay/visual acceptance.

Open BCM's `Worlds/TB_CC_M4_4_ArlandGraphCampaign.ent` normally in Workbench.
Using the supported addon-local instance override mechanism, author the missing
vanilla-compatible capture component on the existing `HQEast` and `HQWest`
entities. Preserve both entity identities, transforms and existing components;
do not add a second HQ/base or a second Hierarchy component.

For each existing HQ instance:

- Ensure exactly one `SCR_CampaignSeizingComponent`, using the accepted ordinary
  Conflict capture configuration represented by BCM's
  `TB_PVE_ConflictMilitaryBase.et` / M4.4I strategic capture architecture.
- Set `m_bIgnoreNonPlayableAttackers=0` and `m_bCapturingRequiresPlayer=0` on
  that component; preserve vanilla prevailing-faction, timer, interruption and
  ownership-completion behavior. Do not invent a TB capture timer.
- Inspect the existing `SCR_CampaignMilitaryBaseComponent`'s
  `m_bDisableWhenUnusedAsHQ` setting. The future unselected ordinary role must
  not disappear/disable when no longer an HQ; retain a supported addon-local
  override for this if the inherited value is true.
- Do not manually choose permanent HQ roles, change faction ownership or
  enable forward deployment. Campaign initialization will select/demote roles.
- Do not Apply to the vanilla prefab or edit the vanilla parent world. Save
  the overrides inside BCM, preserve world GUID A441000000000000, allow normal
  processing and confirm real Workbench compilation/load.

If the editor cannot represent these changes as local instance overrides,
stop and provide its exact limitation; do not duplicate the HQ as a workaround.
After saving, the authored overrides/source identities can be inspected and
the runtime implementation/proofs can proceed safely.

Bohemia's prefab documentation distinguishes local instance changes from
applying them to a prefab: [Prefabs Basics](https://community.bohemia.net/wiki/Arma_Reforger:Prefabs_Basics).
That is the required locality boundary, not permission to globally override
vanilla HQ content.

## Narrow implementation plan after the authoring gate

1. Introduce generic off-map origin definition metadata and a MOB-candidate
   start contract that does not require a physical enemy endpoint. Retain
   compatibility for historical fixture start definitions where appropriate.
2. Derive runtime campaign roles from the selected root: selected MOB protected,
   all other physical positions ordinary and required. Do not put Arland IDs
   into generic role/evaluator logic or ownership-counting code.
3. Configure the two existing authored HQ instances through supported vanilla
   HQ/capture initialization APIs. Selected capture surface stays dormant and
   unauthorized; unselected surface uses genuine vanilla ordinary capture.
   Validate component/base readiness and fail closed on missing resources.
4. Update evaluator candidates, response derivation, territory count and depth
   to use campaign roles. Keep the existing twenty-one edges unless proof
   identifies a concrete reachability defect. Keep singleton authorization.
5. Preserve the M4.4J latch and pressure presentation. Required count is derived;
   deterministic depth is `(ordinaryUS * 1000) / requiredOrdinary`.
   M3 EARLY/MID/LATE bands remain unchanged.
6. Expose semantic NORMAL, OPFOR_FINAL_STAND, and OPFOR_FINAL_PUSH/MOB_SIEGE
   pressure profiles only. No materialization, extra AI, vehicles or multipliers.
7. Adapt clean East/West physical authority proofs for eleven ordinary positions,
   including unselected-HQ authorization/presence/timer/interruption/restart/
   CaptureFinish/ownership/revision/deployment. Run both pressure directions,
   restore proof=0/holds=300/300/M3 enabled and perform final production smoke.

## Acceptance/report status

East/West physical HQ capture, eleven-position depth, final-dataset pressure
regressions, semantic hooks and production smoke are all **NOT RUN/UNVERIFIED**.
The current source still implements the accepted ten-ordinary/two-protected-HQ
M4.4J model; do not report the new model as implemented or PASS.

M4.4K authority acceptance remains PENDING. Historical M4.4J PASS is retained
for its original topology, not transferred automatically to this design.
Genuine human visual acceptance remains deferred. N1, N2, N3, N4 and N5 remain
individually PENDING; no authentication investigation is reopened. Persistence
is not implemented; future saves must include selected root, dynamic other-HQ
role, all eleven ordinary owners, history/pressure and off-map origin metadata.

## Recommended M4.5 scope (proposal only)

After M4.4K closes, define and runtime-measure population/performance budgeting:
global AI group/unit ceilings, ordinary garrison/operation/reserve allocations,
human-presence/distance activation policy, deterministic admission/cleanup,
and measured server frame-time/memory/network costs. Establish budgets and
consumer contracts for NORMAL/FINAL_STAND/FINAL_PUSH before any force
materialization. Vehicles, ingress implementation, siege forces, patrols and
checkpoints remain out of this preparation task.
