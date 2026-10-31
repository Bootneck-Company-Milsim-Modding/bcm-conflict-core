# FIA native group lifecycle corrective pass — 2026-10-02

Status: SOURCE LIFECYCLE DIFFERENCE CONFIRMED / NARROW CORRECTION IMPLEMENTED /
STATIC PREFLIGHT PASS / WORKBENCH GATE REQUIRED / HUMAN FIA COMBAT PENDING.

The Creative Director confirmed the preceding normal Workbench compile passed,
but the real human FIA encounter failed: six FIA defenders stood inert. The
supplied directive records correct faction, membership, group LOD and assigned
DefendSmall; those facts do not establish character AI activation or execution.
No server, client or gameplay test was launched during this pass. The new code
has NOT passed a normal Workbench compile or a human combat test yet.

## 1. Previous BCM FIA materialization

M3.6 CreateGarrison selects the owner-aware package and validated local group
positions. FIA_LIGHT remains two Group_FIA_Remnants groups. SpawnGroup:

1. Resource.Load the package group; SpawnEntityPrefab at the local position.
2. Read/verify group faction against the owner-selected FIA faction, using the
   existing group-only SetFaction fallback if needed.
3. Read m_aUnitPrefabSlots from the instantiated group.
4. Manually Resource.Load and SpawnEntityPrefab each character, using BCM's
   row/column offsets and terrain height.
5. Read effective affiliation; reject a contradictory explicit faction.
6. Call AddAgentFromControlledEntity; record the character; verify count.
7. Read natural group LOD; reject maximum LOD.
8. Spawn AIWaypoint_DefendSmall and call group.AddWaypoint.
9. Set the manager's ACTIVE bookkeeping state.

There was no BCM call to the native group member factory, no native member
completion callback, and no explicit character control activation. Character
entity creation/component initialization and group components still ran, but
that is not equivalent to the native complete-member lifecycle.

## 2. Working USSR differential

The human-proven USSR attack path is M3.4 ForceAssemblyService.SpawnGroup,
consumed by M3.2/M3.1. Its manual character/agent construction has the same
essential shape as previous M3.6. M3.6 USSR defense shares that assembly shape.
This pass leaves BOTH USSR construction paths unchanged.

| Step | USSR | Previous FIA M3.6 | Difference / significance |
|---|---|---|---|
| Group | Group_USSR_Defenders -> Group_USSR -> Group_Base | Group_FIA_Remnants -> Group_FIA_Remnants_Base -> Group_Remnants_Base -> Group_Base | Both are complete SCR_AIGroup resources, not bare GenericEntity shells |
| Immediate members | m_bSpawnImmediately 0 | m_bSpawnImmediately 0 | Both deliberately await external member spawning |
| Character | Campaign_USSR_AI, three slots | FIA Rifleman/Scout/MG, three slots | Different native prefab initialization defaults |
| AI control default | Campaign_USSR_AI explicitly overrides EnableAI 1 | FIA chain inherits Character_Base EnableAI 0 | FIRST decisive character-lifecycle difference; manual membership does not enable AI |
| Member creation | BCM SpawnEntityPrefab | BCM SpawnEntityPrefab | Neither manual path calls native SpawnGroupMember |
| Registration | AddAgentFromControlledEntity | AddAgentFromControlledEntity | Adds existing agent, does not activate character control |
| Brain/formation | Inherited native character/group components | Same common Group_Base infrastructure | Presence/count does not prove execution |
| Order | Attack waypoint via operation; defense uses DefendSmall | DefendSmall via AddWaypoint | Different policy roles, not evidence the defend order is broken |
| Replication | Native prefab Rpl components | Native prefab Rpl components | No demonstrated replication mismatch |
| Commander | No explicit BCM registration repair | No explicit BCM registration repair | Native factory does not require a new BCM faction-commander repair |

## 3. Installed vanilla lifecycle and exact evidence

Read-only inspection used the installed Arma Reforger data007.pak. Source and
resource line references below refer to the decompressed installed text, not
guessed web APIs or modified vanilla files.

- Prefabs/Groups/Campaign/Group_FIA_Remnants.et(1–6): complete SCR_AIGroup;
  slots are Rifleman, Scout, MG. GUID F22EDFBEFC193357 unchanged.
- Group_FIA_Remnants_Base.et(1–4): FIA faction; inherits Group_Remnants_Base.
- Group_Remnants_Base.et(4–5): m_bSpawnImmediately 0, member delay 250.
- Prefabs/Characters/Core/Character_Base.et(250–299): ChimeraAIControlComponent,
  full SCR_ChimeraAIAgent template and soldier behavior tree, EnableAI 0.
- FIA Rifleman/Scout/MG -> Character_FIA_BaseLoadout -> Character_FIA_base ->
  Character_Base: no EnableAI 1 override in that chain.
- Campaign/Final/OPFOR/USSR_Army/Campaign_USSR_AI.et(4–11): AI component and
  perception enabled, EnableAI 1 explicitly overrides the generic character.
- scripts/Game/Entities/SCR_AIGroup.c(1132–1148):
  AddAgentFromControlledEntity finds AIControlComponent, obtains GetAIAgent,
  AddAgent, and notifies group membership. NO ActivateAI call.
- SCR_AIGroup.c(1927–1942): AddAIEntityToGroup gets GetControlAIAgent,
  calls control.ActivateAI(), then adds the agent if it has no parent group.
- SCR_AIGroup.c(1664–1771): native SpawnGroupMember derives formation/world
  transform, terrain/navmesh position, calls SpawnEntityPrefabEx(res,true,...),
  calls AddAIEntityToGroup, applies group affiliation, and invokes group init
  on member completion. BCM does not reproduce these steps manually now.
- SCR_AIGroup.c(2684–2718): RequestSpawn(int slotsWanted=-1,
  float observerRange=0) queues via ChimeraAIWorld.EnqueueSpawnRequest; uses
  native SpawnMembers fallback in a non-Chimera world.
- SCR_AIGroup.c(2720–2764): ExpandOneMember delegates to SpawnGroupMember,
  completes Event_OnAllDelayedEntitySpawned and InvokeEventOnInit at capacity.
- SCR_AIGroup.c(2578–2608): runtime EOnInit auto-requests members ONLY when
  m_bSpawnImmediately is true. FIA Remnants therefore needs a spawn request.

Vanilla working production example located: the FIA ambient-patrol service.
Prefabs/Systems/AmbientPatrol/AmbientPatrolSpawnpoint_FIA.et declares FIA.
SCR_AmbientPatrolSpawnPointComponent.Update(282 onward) resolves the faction's
GROUP entity catalog. SpawnPatrol(407–460) instantiates its selected complete
group resource, configures its lifecycle, adds the native waypoint and calls
RequestSpawn. It does not loop over character prefabs adding membership by hand.
This is a concrete installed native production path, not a claim that a new
vanilla human combat test was conducted here. No narrower direct Conflict-base
FIA factory was established by the selected source search.

Common Group_Base.et has native Group.bt, Wedge formation, Soldiers pathfinding,
DEFEND reaction, group info/movement/utility and runtime RplComponent. No new
brain, formation, perception/world manager or replication component is needed.
SCR_AIGroupUtilityComponent.EOnInit(828 onward) subscribes to native agent and
waypoint events. OnCurrentWaypointChanged(672–694) creates/selects waypoint
state; the native tree executes it. We preserve this infrastructure.

## 4. Root-cause classification

CONFIRMED in source: manual BCM assembly bypassed the native control activation
step; FIA character defaults differ from the working USSR campaign character.
SUPPORTED explanation of human inertness: EnableAI 0 + membership-only assembly.
UNKNOWN in the supplied prior runtime evidence: actual control/agent activation
flags, current waypoint recognition and native activity execution. Group LOD 0
and three controlled agents do not answer those questions.
UNVERIFIED: whether the corrected lifecycle restores human combat.

No unsupported conclusion is drawn about hostility, GUIDs, managers, budget,
targeting or firing. Those systems were not changed.

## 5. Misleading ENGAGED diagnostic

ObserveExisting previously entered ENGAGED solely when HasEnemyNear found a
hostile-affiliated dynamic entity within 180 metres while state was ACTIVE.
It did not inspect targets, shooting or native combat activity.

Rename to HOSTILE_NEARBY, retaining numeric state 3 and proximity behaviour.
The log explicitly states proximityOnly=1 and combatUnverified=1. The proof
consumer follows the renamed enum; its contact log no longer claims physical
opposition merely from proximity. No synthetic combat acceptance is added.
The combat-ready group log is now materialized-group. The existing proof
method AreGroupsCombatReadyForProof remains an interface-compatible structural
check, NOT evidence of firing.

## 6. Exact correction / bounded asynchronous completion

Only FIA M3.6 spawning branches into RequestSpawn(expectedSlots), once per group.
The complete existing FIA prefab remains authoritative for member types/count.
Native Enfusion/vanilla code performs member creation, AI control activation,
registration, formation placement and member-init events. No direct BCM
ActivateAI, forced LOD, target or firing call is introduced.

Because the native queue is asynchronous:

- Keep the manager record PENDING_ACTIVATION while native members materialize;
  zero members during that interval must not be called DEFEATED.
- Retain existing defensive waypoint creation/AddWaypoint. An assigned waypoint
  can exist while members are pending, as in the native patrol sequence.
- At existing two-second manager ticks, collect native members from GetAgents,
  check complete expected membership and non-contradictory affiliations.
- Publish ACTIVE bookkeeping only after all expected controlled members exist.
  ACTIVE still means materialized garrison, NOT combat engagement.
- Bound incomplete assembly to 30 seconds; on failure log exact reason and
  pre-rollback diagnostics, clean partial members and apply existing bounded
  retry policy (three failures per observed revision, 30-second retry delay).
- Existing target/ownership/terminal cleanup still owns the group; rollback
  discovers any members created between ticks so partial queued work is not
  orphaned. No AIWorld budgets or global queue policies are changed.

FIA_LIGHT remains two groups x three slots = six defenders. No package file,
character prefab, waypoint resource, placement policy or balance was modified.
FIA recapture coordination and M3.4's shared attack assembly were not changed;
this corrective gate concerns the human-observed M3.6 FIA defenders.

## 7. Representative AI-control and waypoint diagnostics

Read-only diagnostics run at native member completion, pre-rollback, and the
first hostile-proximity transition, not every tick:

- native-order: group; assigned waypoint; GetCurrentWaypoint result;
  currentIsDefend; presence in GetWaypoints; agent count; natural group LOD.
- native-control: representative character; AIControlComponent presence;
  AIAgent identity; GetControlAIAgent identity; sameAgent; membership;
  GetParentGroup; IsAIActivated.
- native-character: effective faction; alive via installed GetLifeState;
  weapon count via BaseWeaponManagerComponent.GetWeaponsList;
  control.IsAIActivated; parentMatches; natural agent LOD.

All introduced method signatures were verified in installed generated Game
scripts. The old human log proves six entities/controlled agents and FIA
affiliation; it does NOT contain these new readouts. Their runtime values remain
UNKNOWN until the next approved test. Current waypoint equality establishes
native recognition, not movement, target acquisition or firing by itself.

DefendSmall is an installed SCR_DefendWaypoint derivative; common Group_Base
supplies the DEFEND reaction and utility machinery. No evidence justified
replacing it. No synthetic targets or combat-state acceptance inference.

## 8. Static preflight and untouched scope

Complete Tools/Test-ThreeFactionCorrectiveStatic.py exit 0 across 45 Game scripts:
delimiter/operator screen, PrintFormat limits, override/caller contracts,
enums, duplicate classes, GUIDs, serializer/parser and earlier model checks.
Added installed native API/prefab evidence assertions and pending/partial/full/
timeout completion model tests. Model tests are NOT runtime proof.

Untouched: capture superiority; US scoring; USSR expansion/consolidation;
FIA recapture policy; HQ state machine; faction relationships; RadioManager;
graph; hierarchy; native managers; packages; resources; M3.2/M3.3; production
holds/proof switches. No ServerDiag, publishing or commit.

Files changed in this pass:

1. Scripts/Game/TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrison.c
2. Scripts/Game/TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrisonProof.c
3. Tools/Test-ThreeFactionCorrectiveStatic.py
4. Documentation/TB-Conflict-Core-FIA-Native-Group-Lifecycle.md

Next action: normal Workbench Game-module compile. Then an approved genuine
human FIA encounter must verify movement, reaction and fighting, using the
new transition readouts to locate the first remaining failure if any.

WORKBENCH GATE REQUIRED. Do not interpret static exit 0 as compile/combat PASS.
