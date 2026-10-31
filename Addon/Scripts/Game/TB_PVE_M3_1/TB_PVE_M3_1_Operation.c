enum TB_PVE_M3_1_OperationState
{
	CREATED = 0,
	ASSEMBLING = 1,
	MOVING = 2,
	ASSAULTING = 3,
	CAPTURING = 4,
	SUCCEEDED = 5,
	FAILED = 6,
	CANCELLED = 7,
	CONSOLIDATING = 8
}

// One-operation lease. It touches only its registered group and restores the
// group's observed natural LOD when the operation terminates.
class TB_PVE_M3_1_ActivationLease
{
	protected SCR_AIGroup m_Group;
	protected int m_iOriginalLOD;
	protected bool m_bHeld;

	bool Acquire(SCR_AIGroup group, string reason)
	{
		if (!Replication.IsServer() || m_bHeld || !group || group.IsDeleted()) return false;
		m_Group = group;
		m_iOriginalLOD = group.GetLOD();
		m_Group.SetLOD(0);
		m_bHeld = true;
		PrintFormat("[TB_PVE_M3_1] activation-lease acquired=1 reason=%1 originalLOD=%2 forcedLOD=%3 scopedGroup=%4", reason, m_iOriginalLOD, m_Group.GetLOD(), m_Group);
		return true;
	}

	void Release(string reason)
	{
		if (!m_bHeld) return;
		if (m_Group && !m_Group.IsDeleted()) m_Group.SetLOD(m_iOriginalLOD);
		PrintFormat("[TB_PVE_M3_1] activation-lease released=1 reason=%1 restoredLOD=%2", reason, m_iOriginalLOD);
		m_Group = null;
		m_bHeld = false;
	}

	bool IsHeld() { return m_bHeld; }
}

// First production-shaped autonomous operation. Strategic target selection is
// intentionally restricted to the M2.3 public semantic query.
class TB_PVE_M3_1_Operation
{
	protected string m_sOperationId;
	protected FactionKey m_sFactionKey = "USSR";
	protected string m_sOperationType = "OPFOR_ATTACK";
	protected string m_sTargetObjectiveId;
	protected int m_iAuthorizedRevision;
	protected int m_iState = TB_PVE_M3_1_OperationState.CREATED;
	protected int m_iElapsedSeconds;
	protected string m_sTerminalReason;
	protected string m_sPackageId;
	protected TB_PVE_M3_4_ForceAssemblyService m_AssemblyService;
	protected ref array<ref TB_PVE_M3_4_AssembledGroup> m_aForceGroups = {};
	protected ref TB_CC_WarForceLease m_WarForce;
	protected vector m_vTargetPosition;
	protected vector m_vStagingPosition;
	protected float m_fStartingDistance;
	protected float m_fMinimumDistance;
	protected float m_fLastLoggedDistance = -1;
	protected bool m_bMeaningfulMovement;
	protected bool m_bPhysicalForceOrdered;
	protected bool m_bVirtualForceEngaged;
	protected string m_sInitialTargetOwner;
	protected int m_iSelectionHumanCount;
	protected int m_iSelectionPlayerBand;
	protected int m_iSelectionDepthPermille;
	protected int m_iSelectionDepthBand;
	protected int m_iSelectionPreviousResult;
	protected int m_iSelectionStrength;
	protected int m_iConsolidationUntil;
	protected int m_iHandoffAttempts;
	protected int m_iNextHandoffSecond;
	protected int m_iLastProgressSecond;
	protected int m_iStallRecoveries;
	protected int m_iPreviousLiving = -1;
	protected int m_iNextPhysicalReportSecond;
	protected ref map<SCR_AIGroup, vector> m_mLastPositions = new map<SCR_AIGroup, vector>();
	protected ref map<BaseMagazineComponent, int> m_mPreviousAmmo = new map<BaseMagazineComponent, int>();
	void ConfigureTerritorialAttack(FactionKey faction, string target)
	{
		if (m_iState != TB_PVE_M3_1_OperationState.CREATED) return;
		m_sFactionKey = faction; m_sTargetObjectiveId = target; m_sOperationType = "FIA_RECAPTURE";
	}
	protected TB_CC_ResponseRequest m_Response;
	void ConfigureResponse(TB_CC_ResponseRequest request)
	{
		m_Response = request; m_sFactionKey = request.m_sFaction; m_sTargetObjectiveId = request.m_sTarget;
		m_sOperationType = "STRATEGIC_RESPONSE";
	}
	protected bool CampaignOperational()
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy) return policy.IsWarOperational();
		return TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE;
	}
	protected bool IsTargetAuthorized()
	{
		if (m_Response)
		{
			TB_CC_M4XP_NativeArlandComponent responsePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
			return responsePolicy && responsePolicy.GetResponses() && responsePolicy.GetResponses().IsCurrent(m_Response);
		}
		if (m_sFactionKey == "FIA")
		{
			TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
			return policy && policy.CanFIARecapture(m_sTargetObjectiveId);
		}
		return TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() == m_sTargetObjectiveId && TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(m_sFactionKey, m_sTargetObjectiveId);
	}

	bool Create(string operationId, TB_PVE_M3_3_TacticalStagingPolicy stagingPolicy, TB_PVE_M3_4_ForceAssemblyService assemblyService, string packageId)
	{
		if (!Replication.IsServer() || (m_sFactionKey != "USSR" && m_sFactionKey != "FIA")) return false;
		m_sOperationId = operationId;
		if (!CampaignOperational())
			return Fail("campaign-not-active");
		if (m_sFactionKey == "USSR" && !m_Response) m_sTargetObjectiveId = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective();
		m_iAuthorizedRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		if (m_sTargetObjectiveId.IsEmpty()) return Fail("no-legal-opfor-objective");
		if (!IsTargetAuthorized())
			return Fail("public-query-denied-target");
		if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(m_sTargetObjectiveId, m_vTargetPosition))
			return Fail("target-position-unresolved");
		if (!stagingPolicy) return Fail("staging-policy-unavailable");
		if (!assemblyService) return Fail("assembly-service-unavailable");
		m_AssemblyService = assemblyService; m_sPackageId = packageId;
		TB_CC_WarSimulation war = TB_CC_WarSimulation.Get();
		if (m_sFactionKey == "FIA" && !war) return Fail("strategic-war-service-unavailable");
		if (war)
		{
			m_WarForce = war.Operation(operationId, m_sFactionKey, packageId, m_sTargetObjectiveId);
			if (!m_WarForce) return Fail("strategic-force-registration-denied");
			m_sInitialTargetOwner = m_WarForce.m_State.m_sExpectedOwner;
			// FIA always enters the shared strategic lease/materializer. Do not let
			// the legacy assembly target-anchor check discard its opportunity.
			if (m_sFactionKey == "FIA" || !war.WantsPhysical(m_WarForce) || !war.PreparePhysicalDefence(m_WarForce))
			{
				m_aForceGroups = m_WarForce.m_aPhysical;
				m_vStagingPosition = m_WarForce.m_State.m_vPosition;
				m_fStartingDistance = vector.Distance(m_vStagingPosition, m_vTargetPosition); m_fMinimumDistance = m_fStartingDistance;
				Transition(TB_PVE_M3_1_OperationState.MOVING, "strategic-virtual-or-admission-pending");
				TB_CC_PhysicalWarService.RegisterOperation(this); return true;
			}
		}
		array<vector> observerPositions = {};
		CollectHumanObserverPositions(observerPositions);
		string assemblyFailure;
		if (!m_AssemblyService.AssemblePackage(m_sPackageId, m_sFactionKey, m_sTargetObjectiveId, m_vTargetPosition, stagingPolicy, observerPositions, m_aForceGroups, assemblyFailure))
		{
			if (war && m_WarForce) { war.Abandon(m_WarForce); m_WarForce = null; }
			return Fail("assembly-failed-" + assemblyFailure);
		}
		if (war && m_WarForce) war.AttachPhysical(m_WarForce, m_aForceGroups);
		m_vStagingPosition = m_aForceGroups.Get(0).m_vStartPosition;
		m_fStartingDistance = vector.Distance(m_vStagingPosition, m_vTargetPosition);
		m_fMinimumDistance = m_fStartingDistance;
		PrintFormat("[TB_PVE_M3_1] operation-created id=%1 type=%2 faction=%3 package=%4 groups=%5 revision=%6 target=%7 authorized=%8", m_sOperationId, m_sOperationType, m_sFactionKey, m_sPackageId, m_aForceGroups.Count(), m_iAuthorizedRevision, m_sTargetObjectiveId, IsTargetAuthorized());
		PrintFormat("[TB_PVE_M3_1] operation-geometry id=%1 staging=%2 targetPosition=%3 distance=%4", m_sOperationId, m_vStagingPosition, m_vTargetPosition, m_fStartingDistance);
		Transition(TB_PVE_M3_1_OperationState.ASSEMBLING, "target-authorized");
		TB_CC_PhysicalWarService.RegisterOperation(this);
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups) if (record.m_bNativeMembersPending) return true;
		return IssueMovementOrders();
	}

	protected void CollectHumanObserverPositions(out array<vector> observerPositions)
	{
		TB_PVE_M3_7_HumanPresenceQuery.GetActiveHumanObservers("US", observerPositions);
	}

	void Tick(int elapsedSeconds)
	{
		if (IsTerminal()) return;
		m_iElapsedSeconds = elapsedSeconds;
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && m_iState == TB_PVE_M3_1_OperationState.CONSOLIDATING) { TickConsolidation(policy); return; }
		// Observe a genuine ownership callback before pressure can cancel M3.
		bool reinforcement = m_Response && m_Response.m_iPhase <= TB_CC_ResponsePhase.COLLAPSE;
		if (reinforcement && !IsTargetAuthorized()) { Cancel("reinforcement-epoch-ended"); return; }
		if (reinforcement && m_WarForce && m_WarForce.m_State.m_iStrength > 0 && policy && vector.Distance(m_WarForce.m_State.m_vPosition, m_vTargetPosition) <= 80)
		{
			BeginConsolidation(policy); return;
		}
		if (!reinforcement && policy && TB_CC_M4XP_CampaignRoles.GetRole(m_sTargetObjectiveId) == TB_CC_StrategicRole.ORDINARY && TB_PVE_M2_3_FrontierQuery.GetRevision() > m_iAuthorizedRevision && policy.GetTerritorialOwner(m_sTargetObjectiveId) == m_sFactionKey)
		{
			BeginConsolidation(policy); return;
		}
		if (!CampaignOperational())
		{
			Cancel("campaign-left-active"); return;
		}

		int currentRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		bool stillAuthorized = IsTargetAuthorized();
		if (!stillAuthorized)
		{
			int state = TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(m_sTargetObjectiveId);
			bool capturedByOperationFaction = state == TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_FRONTLINE || state == TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_REAR_LOCKED;
			if (policy) capturedByOperationFaction = policy.GetTerritorialOwner(m_sTargetObjectiveId) == m_sFactionKey;
			if (currentRevision > m_iAuthorizedRevision && capturedByOperationFaction)
			{
				Transition(TB_PVE_M3_1_OperationState.SUCCEEDED, "observed-vanilla-ownership-and-frontier-reaction");
				Terminate("success");
			}
			else
				Cancel("target-strategically-invalidated");
			return;
		}

		if (m_iState == TB_PVE_M3_1_OperationState.ASSEMBLING)
		{
			bool complete = true;
			foreach (TB_PVE_M3_4_AssembledGroup pending : m_aForceGroups)
			{
				string failure;
				if (!TB_PVE_M3_4_ForceAssemblyService.ObserveNativeMembers(pending, failure)) complete = false;
				if (!failure.IsEmpty()) { Fail(failure); return; }
			}
			if (!complete) return;
			m_iLastProgressSecond = elapsedSeconds; IssueMovementOrders(); return;
		}
		if (m_WarForce)
		{
			TB_CC_WarForceState warState = m_WarForce.m_State;
			warState.m_iEscalationContext = m_iSelectionPreviousResult;
			if (warState.m_bCompleted && warState.m_iOperationState == TB_PVE_M3_1_OperationState.FAILED)
			{ m_bVirtualForceEngaged = warState.m_fCombatSeconds > 0; Fail("all-assigned-groups-destroyed"); return; }
			if (m_sFactionKey == "FIA" && warState.m_bCompleted && warState.m_iOperationState == TB_PVE_M3_1_OperationState.CANCELLED)
			{ Cancel("strategic-war-capability-or-owner-invalidated"); return; }
			if (warState.m_iRepresentation != TB_CC_WarRepresentation.PHYSICAL)
			{
				m_fMinimumDistance = Math.Min(m_fMinimumDistance, vector.Distance(warState.m_vPosition, m_vTargetPosition));
				m_bMeaningfulMovement = warState.m_fProgress > 0;
				m_iLastProgressSecond = elapsedSeconds;
				return; // Virtual clock/queue owns this force; no physical advancement.
			}
			warState.m_iOperationState = m_iState;
			m_bPhysicalForceOrdered = !m_aForceGroups.IsEmpty();
		}
		int viableGroups = GetViableGroupCount();
		if (viableGroups == 0)
		{
			// Incremental representation may still have surviving unleased reserve.
			// Absence of ready physical groups is not destruction of that reserve.
			if (m_WarForce && m_WarForce.m_State.m_iStrength > 0) { m_iLastProgressSecond = elapsedSeconds; return; }
			Fail("all-assigned-groups-destroyed"); return;
		}
		if (!policy && m_iElapsedSeconds > 300)
		{
			Fail("movement-capture-timeout"); return;
		}

		float distance = UpdateForceMovement();
		if (distance <= 100 && m_iState == TB_PVE_M3_1_OperationState.MOVING)
			Transition(TB_PVE_M3_1_OperationState.ASSAULTING, "within-assault-distance");
		if (distance <= 50 && m_iState == TB_PVE_M3_1_OperationState.ASSAULTING)
			Transition(TB_PVE_M3_1_OperationState.CAPTURING, "physically-entered-objective-area");
		if (policy) ObservePhysicalProgress();
	}

	protected void ObservePhysicalProgress()
	{
		int living; bool moved; int visibleTargets; int ammoConsumed;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (!record.m_Group || record.m_Group.IsDeleted()) continue;
			living += TB_CC_PhysicalWarService.LivingCount(record);
			vector position = GetGroupPosition(record);
			if (!m_mLastPositions.Contains(record.m_Group) || vector.Distance(position, m_mLastPositions.Get(record.m_Group)) >= 5)
			{ moved = true; m_mLastPositions.Set(record.m_Group, position); }
			array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.IsAIActivated()) continue;
				SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(agent.FindComponent(SCR_AICombatComponent));
				BaseTarget target; if (combat) target = combat.GetCurrentTarget();
				if (target && combat.IsTargetVisible(target)) visibleTargets++;
				if (combat)
				{
					BaseWeaponComponent weapon; int muzzle; BaseMagazineComponent magazine;
					combat.GetSelectedWeaponAndMagazine(weapon, muzzle, magazine);
					if (magazine)
					{
						int ammo = magazine.GetAmmoCount();
						if (m_mPreviousAmmo.Contains(magazine) && ammo < m_mPreviousAmmo.Get(magazine)) ammoConsumed++;
						m_mPreviousAmmo.Set(magazine, ammo);
					}
				}
			}
		}
		if (moved || visibleTargets > 0 || ammoConsumed > 0 || living != m_iPreviousLiving) m_iLastProgressSecond = m_iElapsedSeconds;
		m_iPreviousLiving = living;
		if (m_iElapsedSeconds >= m_iNextPhysicalReportSecond)
		{
			m_iNextPhysicalReportSecond = m_iElapsedSeconds + 30;
			foreach (TB_PVE_M3_4_AssembledGroup observed : m_aForceGroups)
			{
				if (!observed.m_Group || observed.m_Group.IsDeleted()) continue;
				array<AIAgent> members = {}; observed.m_Group.GetAgents(members); int active;
				foreach (AIAgent member : members) if (member && member.IsAIActivated()) active++;
				PrintFormat("[TB_CC_PHYSICAL_WAR] operation-observation id=%1 group=%2 startDistance=%3 minimumDistance=%4 displacement=%5 activeAgents=%6 agents=%7 LOD=%8 currentWaypoint=%9", m_sOperationId, observed.m_Group, observed.m_fStartDistance, observed.m_fMinimumDistance, vector.Distance(observed.m_vStartPosition, GetGroupPosition(observed)), active, members.Count(), observed.m_Group.GetLOD(), observed.m_Group.GetCurrentWaypoint());
			}
			PrintFormat("[TB_CC_PHYSICAL_WAR] operation-progress id=%1 state=%2 living=%3 visibleTargets=%4 movedThisSample=%5 lastProgressSecond=%6 elapsedSecond=%7 ammoConsumedMagazines=%8", m_sOperationId, m_iState, living, visibleTargets, moved, m_iLastProgressSecond, m_iElapsedSeconds, ammoConsumed);
		}
		int stallLimit = 90;
		if (m_iState == TB_PVE_M3_1_OperationState.ASSAULTING || m_iState == TB_PVE_M3_1_OperationState.CAPTURING) stallLimit = 180;
		if (m_iElapsedSeconds - m_iLastProgressSecond < stallLimit) return;
		PrintFormat("[TB_CC_PHYSICAL_WAR] operation-stall id=%1 state=%2 living=%3 visibleTargets=%4 inactiveSeconds=%5 recovery=%6", m_sOperationId, m_iState, living, visibleTargets, m_iElapsedSeconds - m_iLastProgressSecond, m_iStallRecoveries);
		if (m_iStallRecoveries >= 3) { Fail("physical-progress-stalled-after-recovery"); return; }
		m_iStallRecoveries++; m_iLastProgressSecond = m_iElapsedSeconds;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (!record.m_Group || record.m_Group.IsDeleted()) continue;
			TB_CC_PhysicalWarService.SetRole(record, "offensive", true);
			if (record.m_Waypoint && !record.m_Waypoint.IsDeleted())
			{ record.m_Group.RemoveWaypoint(record.m_Waypoint); record.m_Group.AddWaypoint(record.m_Waypoint); }
			else CreateMoveOrder(record);
		}
	}

	protected void BeginConsolidation(TB_CC_M4XP_NativeArlandComponent policy)
	{
		if (policy.GetOccupationService()) policy.GetOccupationService().ReserveConsolidation(m_sTargetObjectiveId, m_sFactionKey);
		Terminate("captured-transition-to-consolidation"); // Clear offensive waypoints only.
		if (m_WarForce && TB_CC_WarSimulation.Get()) TB_CC_WarSimulation.Get().EndOperation(m_WarForce, true);
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (record.m_Group && !record.m_Group.IsDeleted() && record.m_Group.GetAgentsCount() > 0) TB_CC_TerritorialOccupationService.SetDefensiveOrder(record, m_vTargetPosition);
			TB_CC_PhysicalWarService.SetRole(record, "consolidation", false);
		}
		m_iConsolidationUntil = m_iElapsedSeconds + policy.GetConsolidationSeconds();
		Transition(TB_PVE_M3_1_OperationState.CONSOLIDATING, "genuine-capture-survivors-defend");
	}
	protected void TickConsolidation(TB_CC_M4XP_NativeArlandComponent policy)
	{
		if (policy.GetTerritorialOwner(m_sTargetObjectiveId) != m_sFactionKey) { Cancel("consolidation-owner-lost"); return; }
		if (m_WarForce)
		{
			if (m_WarForce.m_State.m_iStrength <= 0) { Fail("consolidation-survivors-lost"); return; }
			if (m_iElapsedSeconds < m_iConsolidationUntil) return;
			Transition(TB_PVE_M3_1_OperationState.SUCCEEDED, "strategic-survivor-defence-handoff-confirmed"); return;
		}
		if (GetViableGroupCount() == 0) { Fail("consolidation-survivors-lost"); return; }
		if (m_iElapsedSeconds < m_iConsolidationUntil || m_iElapsedSeconds < m_iNextHandoffSecond) return;
		TB_CC_TerritorialOccupationService occupation = policy.GetOccupationService();
		if (occupation && occupation.Adopt(m_sTargetObjectiveId, m_sFactionKey, m_aForceGroups))
		{
			Transition(TB_PVE_M3_1_OperationState.SUCCEEDED, "persistent-defence-handoff-confirmed"); Terminate("success-handoff"); return;
		}
		m_iHandoffAttempts++; m_iNextHandoffSecond = m_iElapsedSeconds + 10;
		PrintFormat("[TB_CC_OCCUPATION] handoff-pending operation=%1 base=%2 attempt=%3 survivors=%4", m_sOperationId, m_sTargetObjectiveId, m_iHandoffAttempts, GetViableGroupCount());
		if (m_iHandoffAttempts >= 3)
		{
			// Fail safely: the occupation service owns retained infantry even if
			// defensive-resource initialization is pending. Never erase winners.
			if (occupation) occupation.Adopt(m_sTargetObjectiveId, m_sFactionKey, m_aForceGroups, false);
			Fail("defence-handoff-not-confirmed");
		}
	}

	protected bool IssueMovementOrders()
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (!CreateMoveOrder(record)) return Fail("move-waypoint-spawn-failed");
			TB_CC_PhysicalWarService.SetRole(record, "offensive", true);
			PrintFormat("[TB_PVE_M3_4] movement-issued operation=%1 package=%2 group=%3 role=%4 start=%5 target=%6 distance=%7 units=%8 LOD=%9", m_sOperationId, m_sPackageId, record.m_Group, record.m_sEntryRole, record.m_vStartPosition, m_vTargetPosition, record.m_fStartDistance, record.m_Group.GetAgentsCount(), record.m_iNaturalLOD);
		}
		// Evidence for escalation only; no change to movement/lifecycle decisions.
		m_bPhysicalForceOrdered = !m_aForceGroups.IsEmpty();
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && m_sInitialTargetOwner.IsEmpty()) m_sInitialTargetOwner = policy.GetTerritorialOwner(m_sTargetObjectiveId);
		Transition(TB_PVE_M3_1_OperationState.MOVING, "normal-ai-waypoint-issued");
		return true;
	}
	protected bool CreateMoveOrder(TB_PVE_M3_4_AssembledGroup record)
	{
		if (!record.m_Group || record.m_Group.IsDeleted()) return false;
		Resource resource = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!resource || !resource.IsValid()) return false;
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = m_vTargetPosition;
		record.m_Waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!record.m_Waypoint) return false;
		record.m_Group.AddWaypoint(record.m_Waypoint); return true;
	}

	protected vector GetGroupPosition(TB_PVE_M3_4_AssembledGroup record)
	{
		array<AIAgent> agents = {};
		record.m_Group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (agent && agent.GetControlledEntity()) return agent.GetControlledEntity().GetOrigin();
		}
		return record.m_Group.GetOrigin();
	}

	protected float UpdateForceMovement()
	{
		float closest = 1000000;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (!record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() == 0) continue;
			vector position = GetGroupPosition(record); float distance = vector.Distance(position, m_vTargetPosition);
			if (distance < record.m_fMinimumDistance) record.m_fMinimumDistance = distance;
			if (record.m_fStartDistance - distance >= 25) record.m_bMeaningfulMovement = true;
			if (record.m_bMeaningfulMovement) m_bMeaningfulMovement = true;
			if (distance < closest) closest = distance;
			if (distance < m_fMinimumDistance) m_fMinimumDistance = distance;
			if (record.m_fLastLoggedDistance < 0 || Math.AbsFloat(distance - record.m_fLastLoggedDistance) >= 50)
			{
				record.m_fLastLoggedDistance = distance;
				PrintFormat("[TB_PVE_M3_4] group-movement operation=%1 group=%2 position=%3 distance=%4 alive=%5 LOD=%6", m_sOperationId, record.m_Group, position, distance, record.m_Group.GetAgentsCount(), record.m_Group.GetLOD());
			}
		}
		return closest;
	}

	protected int GetViableGroupCount()
	{
		int result; foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups) if (record.m_Group && !record.m_Group.IsDeleted() && record.m_Group.GetAgentsCount() > 0) result++;
		return result;
	}

	protected void Transition(int nextState, string reason)
	{
		PrintFormat("[TB_PVE_M3_1] state-transition id=%1 from=%2 to=%3 reason=%4 revision=%5 target=%6", m_sOperationId, m_iState, nextState, reason, TB_PVE_M2_3_FrontierQuery.GetRevision(), m_sTargetObjectiveId);
		m_iState = nextState;
		if (nextState == TB_PVE_M3_1_OperationState.FAILED || nextState == TB_PVE_M3_1_OperationState.CANCELLED) m_sTerminalReason = reason;
	}

	protected bool Fail(string reason)
	{
		Transition(TB_PVE_M3_1_OperationState.FAILED, reason);
		Terminate("failure");
		return false;
	}

	protected void Cancel(string reason)
	{
		Transition(TB_PVE_M3_1_OperationState.CANCELLED, reason);
		Terminate("cancellation");
	}

	protected void Terminate(string reason)
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups)
		{
			if (record.m_Group && record.m_Waypoint && !record.m_Waypoint.IsDeleted()) record.m_Group.RemoveWaypoint(record.m_Waypoint);
			if (record.m_Waypoint && !record.m_Waypoint.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(record.m_Waypoint);
			record.m_Waypoint = null;
		}
		PrintFormat("[TB_PVE_M3_1] operation-terminal id=%1 package=%2 groups=%3 state=%4 reason=%5 startDistance=%6 minimumDistance=%7 moved=%8 revision=%9 cleanup=group-waypoints", m_sOperationId, m_sPackageId, m_aForceGroups.Count(), m_iState, reason, m_fStartingDistance, m_fMinimumDistance, m_bMeaningfulMovement, TB_PVE_M2_3_FrontierQuery.GetRevision());
	}

	void DestroyAssignedForceForProof()
	{
		for (int i; i < m_aForceGroups.Count(); i++) DestroyGroupForProof(i);
	}

	void DestroyGroupForProof(int index)
	{
		if (index < 0 || index >= m_aForceGroups.Count()) return;
		TB_PVE_M3_4_AssembledGroup record = m_aForceGroups.Get(index);
		foreach (IEntity character : record.m_aCharacters) if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		record.m_aCharacters.Clear();
		PrintFormat("[TB_PVE_M3_4] proof-group-destroyed operation=%1 index=%2 viableRemaining=%3", m_sOperationId, index, GetViableGroupCount());
	}

	void CleanupAssignedForce(string reason)
	{
		TB_CC_PhysicalWarService.ReleaseOperation(this);
		if (m_WarForce && TB_CC_WarSimulation.Get())
		{
			bool won = TB_CC_WarSimulation.Get().Owner(m_sTargetObjectiveId) == m_sFactionKey;
			TB_CC_WarSimulation.Get().EndOperation(m_WarForce, won);
			// The persistent representation service owns the shared physical array.
			m_aForceGroups = new array<ref TB_PVE_M3_4_AssembledGroup>(); m_WarForce = null;
			return;
		}
		int startMs = System.GetTickCount(); int groups = m_aForceGroups.Count();
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && !m_aForceGroups.IsEmpty() && TB_CC_M4XP_CampaignRoles.GetRole(m_sTargetObjectiveId) == TB_CC_StrategicRole.ORDINARY && policy.GetTerritorialOwner(m_sTargetObjectiveId) == m_sFactionKey)
		{
			TB_CC_TerritorialOccupationService occupation = policy.GetOccupationService();
			if (occupation && occupation.Adopt(m_sTargetObjectiveId, m_sFactionKey, m_aForceGroups, false))
			{
				PrintFormat("[TB_CC_OCCUPATION] cleanup-handoff operation=%1 base=%2 reason=%3 attackersDeleted=0", m_sOperationId, m_sTargetObjectiveId, reason); return;
			}
			PrintFormat("[TB_CC_OCCUPATION] cleanup-blocked operation=%1 base=%2 reason=handoff-unavailable survivorsRetained=1", m_sOperationId, m_sTargetObjectiveId); return;
		}
		if (m_AssemblyService) m_AssemblyService.Rollback(m_aForceGroups, "operation-cleanup-" + reason);
		if (policy && policy.GetOccupationService()) policy.GetOccupationService().ReleaseConsolidation(m_sTargetObjectiveId, m_sFactionKey);
		PrintFormat("[TB_PVE_M3_4] package-cleanup operation=%1 package=%2 groups=%3 reason=%4 durationMs=%5", m_sOperationId, m_sPackageId, groups, reason, System.GetTickCount() - startMs);
	}

	void SetSelectionSnapshot(TB_PVE_M3_5_SelectionDecision decision)
	{
		if (!decision) return;
		m_iSelectionHumanCount = decision.m_iHumanBluforCount; m_iSelectionPlayerBand = decision.m_iPlayerBand;
		m_iSelectionDepthPermille = decision.m_iDepthPermille; m_iSelectionDepthBand = decision.m_iDepthBand;
		m_iSelectionPreviousResult = decision.m_iPreviousResult; m_iSelectionStrength = decision.m_iFinalStrength;
		PrintFormat("[TB_PVE_M3_5] operation-snapshot id=%1 package=%2 humans=%3 playerBand=%4 depthPermille=%5 depthBand=%6 previous=%7 strength=%8 revision=%9", m_sOperationId, m_sPackageId, m_iSelectionHumanCount, m_iSelectionPlayerBand, m_iSelectionDepthPermille, m_iSelectionDepthBand, m_iSelectionPreviousResult, m_iSelectionStrength, m_iAuthorizedRevision);
	}
	int GetSelectionHumanCount() { return m_iSelectionHumanCount; }
	int GetSelectionDepthBand() { return m_iSelectionDepthBand; }

	bool IsTerminal() { return m_iState == TB_PVE_M3_1_OperationState.SUCCEEDED || m_iState == TB_PVE_M3_1_OperationState.FAILED || m_iState == TB_PVE_M3_1_OperationState.CANCELLED; }
	int GetState() { return m_iState; }
	string GetTerminalReason() { return m_sTerminalReason; }
	string GetTargetObjectiveId() { return m_sTargetObjectiveId; }
	int GetAuthorizedRevision() { return m_iAuthorizedRevision; }
	bool HasMeaningfulMovement() { return m_bMeaningfulMovement; }
	bool HasPhysicalFailureForEscalation()
	{
		// Legacy API name retained for coordinator/proof callers. A measured
		// background attrition defeat also feeds the SAME escalation thresholds,
		// without pretending that a virtual force had physical movement orders.
		if ((!m_bPhysicalForceOrdered && !m_bVirtualForceEngaged) || m_iState != TB_PVE_M3_1_OperationState.FAILED) return false;
		return m_sTerminalReason == "all-assigned-groups-destroyed" || m_sTerminalReason == "physical-progress-stalled-after-recovery" || m_sTerminalReason == "movement-capture-timeout";
	}
	string GetInitialTargetOwner() { return m_sInitialTargetOwner; }
	float GetStartingDistance() { return m_fStartingDistance; }
	float GetMinimumDistance() { return m_fMinimumDistance; }
	vector GetStagingPosition() { return m_vStagingPosition; }
	bool IsLeaseHeld() { return false; }
	string GetPackageId() { return m_sPackageId; }
	int GetAssignedGroupCount() { return m_aForceGroups.Count(); }
	int GetViableAssignedGroupCount() { return GetViableGroupCount(); }
	bool DidEveryViableGroupMove()
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : m_aForceGroups) if (record.m_Group && !record.m_Group.IsDeleted() && record.m_Group.GetAgentsCount() > 0 && !record.m_bMeaningfulMovement) return false;
		return true;
	}
	string GetOperationId() { return m_sOperationId; }
	FactionKey GetOperationFaction() { return m_sFactionKey; }
	int GetAssignedGroupLOD()
	{
		if (m_aForceGroups.Count() == 0 || !m_aForceGroups.Get(0).m_Group || m_aForceGroups.Get(0).m_Group.IsDeleted()) return -1;
		return m_aForceGroups.Get(0).m_Group.GetLOD();
	}
	int GetAssignedGroupLODAt(int index)
	{
		if (index < 0 || index >= m_aForceGroups.Count() || !m_aForceGroups.Get(index).m_Group || m_aForceGroups.Get(index).m_Group.IsDeleted()) return -1;
		return m_aForceGroups.Get(index).m_Group.GetLOD();
	}
	int GetAssignedUnitCount()
	{
		return TB_PVE_M3_4_ForceAssemblyService.CountUnits(m_aForceGroups);
	}
}
