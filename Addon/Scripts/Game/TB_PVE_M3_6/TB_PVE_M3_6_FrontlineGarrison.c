enum TB_PVE_M3_6_GarrisonState
{
	NONE = 0,
	PENDING_ACTIVATION = 1,
	ACTIVE = 2,
	HOSTILE_NEARBY = 3,
	DEFEATED = 4,
	RETIRING = 5,
	SUSPENDED = 6,
	VIRTUAL = 7
}

// One authority-owned physical garrison. It owns only its defensive groups;
// vanilla Conflict remains the sole owner of base capture and ownership.
class TB_PVE_M3_6_FrontlineGarrison
{
	string m_sId;
	FactionKey m_sFaction = "USSR";
	string m_sObjectiveId;
	string m_sPackageId;
	int m_iCreationRevision;
	int m_iState = TB_PVE_M3_6_GarrisonState.NONE;
	ref array<ref TB_PVE_M3_4_AssembledGroup> m_aGroups = {};
}

class TB_PVE_M3_6_MaterializationTrace
{
	string m_sFirstFailure;
	bool m_bGroupCreated;
	string m_sGroupFactionBefore = "<none>";
	string m_sGroupFactionAfter = "<none>";
	int m_iExpectedSlots;
	int m_iCharactersCreated;
	int m_iAgents;
	int m_iUSSRAffiliations;
	int m_iFIAAffiliations;
	int m_iUSAffiliations;
	int m_iOtherAffiliations;
	int m_iMissingAffiliationComponents;
	int m_iNaturalLOD = -1;
	bool m_bWaypointCreated;
	bool m_bWaypointAssignmentIssued;
}

class TB_PVE_M3_6_OPFORFrontlineGarrisonManager
{
	protected static ref map<string, TB_PVE_M3_6_OPFORFrontlineGarrisonManager> s_mOwnerGarrisonClaims = new map<string, TB_PVE_M3_6_OPFORFrontlineGarrisonManager>();
	static const ResourceName DEFEND_WAYPOINT = "{2FCBE5C76E285A7B}Prefabs/AI/Waypoints/AIWaypoint_DefendSmall.et";
	protected string m_sPackageId = "STANDARD";
	protected float m_fMinimumPlayerDistance = 250;
	protected float m_fVisibilityRange = 900;
	protected float m_fCleanupPlayerDistance = 250;
	protected float m_fPlacementRadius = 55;
	protected int m_iCandidateAttempts = 32;
	protected NavmeshWorldComponent m_Navmesh;
	protected ref TB_PVE_M3_6_FrontlineGarrison m_Garrison;
	protected ref TB_CC_WarForceLease m_WarDefence;
	protected bool m_bRunning;
	protected bool m_bOwnerAware;
	protected bool m_bOPFORFront;
	protected int m_iRetryAfterMs;
	protected int m_iCreationFailures;
	protected string m_sTerritorialTarget;
	protected bool m_bPreferHiddenPlacement;
	protected bool m_bCreationSuspended;
	void SuspendCreation(bool suspended) { m_bCreationSuspended = suspended; }
	void SetTerritorialTarget(string id) { m_sTerritorialTarget = id; }
	string GetTerritorialTarget() { return m_sTerritorialTarget; }
	protected int RetryMilliseconds()
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy) return policy.GetActiveGarrisonRetrySeconds() * 1000;
		return 30000;
	}
	void SetOPFORFront(bool enabled) { m_bOPFORFront = enabled; }
	protected string GetObservedObjective()
	{
		if (!m_sTerritorialTarget.IsEmpty()) return m_sTerritorialTarget;
		if (m_bOPFORFront) return TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective();
		return TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective();
	}
	protected FactionKey m_sDefenderFaction = "USSR";
	protected string m_sFIALight = "FIA_LIGHT";
	protected string m_sFIAStandard = "FIA_STANDARD";
	protected string m_sFIAReinforced = "FIA_REINFORCED";
	protected string m_sFIAStrongpoint = "FIA_STRONGPOINT";
	protected int m_iMaximumGarrisonGroups = 3;
	protected int m_iFIAConventionalStrength = 1000;
	void ConfigureOwnerAware(string light, string standard, string reinforced, string strongpoint, int maximumGroups, int conventionalStrength)
	{
		m_bOwnerAware = true; m_sFIALight = light; m_sFIAStandard = standard;
		m_sFIAReinforced = reinforced; m_sFIAStrongpoint = strongpoint;
		m_iMaximumGarrisonGroups = Math.Max(maximumGroups, 1);
		m_iFIAConventionalStrength = Math.Clamp(conventionalStrength, 0, 1000);
	}
	protected string m_sObservedTarget;
	protected int m_iObservedRevision;
	protected int m_iSerial;
	protected int m_iMaximumConcurrent;
	protected int m_iCreated;
	protected int m_iProofHumanCount = -1;
	protected ref array<vector> m_aProofObservers;
	protected int m_iLastAliveGroups = -1;
	protected int m_iLastCreationDurationMs;
	protected int m_iNativeSpawnStartMs;
	protected int m_iLastCleanupDurationMs;
	protected ref array<IEntity> m_aNearbyQuery = {};
	protected string m_sLastPendingKey;
	protected string m_sLastMaterializationFailureKey;
	protected bool m_bSuspensionLogged;
	protected int m_iLastParticipantCount = -1;
	protected int m_iLastObserverCount = -1;
	protected string m_sRetirementDelayKey;

	void Configure(string packageId, float minimumPlayerDistance, float visibilityRange, float cleanupPlayerDistance, float placementRadius, int candidateAttempts)
	{
		m_sPackageId = packageId;
		m_fMinimumPlayerDistance = Math.Max(minimumPlayerDistance, 0);
		m_fVisibilityRange = Math.Max(visibilityRange, m_fMinimumPlayerDistance);
		m_fCleanupPlayerDistance = Math.Max(cleanupPlayerDistance, 0);
		m_fPlacementRadius = Math.Max(placementRadius, 15);
		m_iCandidateAttempts = Math.Max(candidateAttempts, 8);
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld) m_Navmesh = aiWorld.GetNavmeshWorldComponent("Soldiers");
		PrintFormat("[TB_PVE_M3_6] configured package=%1 playerExclusion=%2 visibilityRange=%3 cleanupDistance=%4 placementRadius=%5 attempts=%6 navmesh=%7", m_sPackageId, m_fMinimumPlayerDistance, m_fVisibilityRange, m_fCleanupPlayerDistance, m_fPlacementRadius, m_iCandidateAttempts, m_Navmesh != null);
	}

	void Start()
	{
		if (!Replication.IsServer() || m_bRunning) return;
		m_bRunning = true;
		PrintFormat("[TB_PVE_M3_6] manager-start server=1 authority=1 perManagerCapacity=1 territorialTarget=%1 ownerAware=%2", m_sTerritorialTarget, m_bOwnerAware);
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	void Stop(string reason)
	{
		if (!m_bRunning) return;
		m_bRunning = false;
		GetGame().GetCallqueue().Remove(Tick);
		Cleanup("manager-stop-" + reason, true);
		PrintFormat("[TB_PVE_M3_6] manager-stop reason=%1 created=%2 maxConcurrent=%3", reason, m_iCreated, m_iMaximumConcurrent);
	}

	protected void Tick()
	{
		if (!m_bRunning) return;
		if (m_bCreationSuspended) return; // Physical defence retained; no new spawn.
		if (TB_PVE_M2_3_FrontierQuery.GetReadiness() != TB_PVE_M2_3_QueryReadiness.READY || !TB_PVE_M2_3_FrontierQuery.IsCampaignValid() || TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE)
		{
			if (m_Garrison && !SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy()) { m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.SUSPENDED; Cleanup("campaign-invalid-or-terminal", false); }
			if (!m_bSuspensionLogged) Print("[TB_PVE_M3_6] suspended reason=campaign-not-active-valid newCreation=0");
			m_bSuspensionLogged = true;
			return;
		}
		m_bSuspensionLogged = false;

		// From BLUFOR-oriented M2 semantics ENEMY_FRONTLINE is the first USSR-held
		// objective and therefore the conventional OPFOR frontline to defend.
		string target = GetObservedObjective();
		TB_CC_M4XP_NativeArlandComponent ownerPolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		TB_CC_WarSimulation war = TB_CC_WarSimulation.Get();
		if (m_bOwnerAware && ownerPolicy && war)
		{
			FactionKey currentOwner = ownerPolicy.GetTerritorialOwner(target);
			if (currentOwner != "FIA" && currentOwner != "USSR") { m_Garrison = null; m_WarDefence = null; return; }
			string packageId = m_sPackageId; if (currentOwner == "FIA") packageId = ownerPolicy.SelectFIAPackage(target);
			m_WarDefence = war.Defence(target, currentOwner, packageId);
			if (!m_WarDefence) { LogPending(target, TB_PVE_M2_3_FrontierQuery.GetRevision(), "strategic-defence-resource-unavailable", 0); return; }
			if (!m_Garrison || m_Garrison.m_sId != m_WarDefence.m_State.m_sForceId)
			{
				m_Garrison = new TB_PVE_M3_6_FrontlineGarrison(); m_iCreated++; m_iMaximumConcurrent = 1;
				m_Garrison.m_sId = m_WarDefence.m_State.m_sForceId; m_Garrison.m_sObjectiveId = target;
				m_Garrison.m_sFaction = currentOwner; m_Garrison.m_sPackageId = m_WarDefence.m_State.m_sPackageId;
				m_Garrison.m_iCreationRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
			}
			m_Garrison.m_aGroups = m_WarDefence.m_aPhysical;
			m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.PENDING_ACTIVATION;
			if (m_WarDefence.m_State.m_iRepresentation == TB_CC_WarRepresentation.VIRTUAL) m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.VIRTUAL;
			if (m_WarDefence.m_State.m_iRepresentation == TB_CC_WarRepresentation.PHYSICAL) m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.ACTIVE;
			if (m_WarDefence.m_State.m_iStrength <= 0) m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.DEFEATED;
			return; // One logical defence, never an extra fresh physical garrison.
		}
		if (m_bOwnerAware && ownerPolicy)
		{
			FactionKey owner = ownerPolicy.GetTerritorialOwner(target);
			if (owner != "FIA" && owner != "USSR") target = "";
			if (m_Garrison && ownerPolicy.GetTerritorialOwner(m_Garrison.m_sObjectiveId) != m_Garrison.m_sFaction)
			{
				if (!Cleanup("owner-changed", false)) return;
			}
			m_sDefenderFaction = owner;
		}
		int revision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		bool targetValid = !target.IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(target) == TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_FRONTLINE;
		if (m_bOwnerAware && ownerPolicy) targetValid = !target.IsEmpty() && TB_CC_M4XP_CampaignRoles.GetRole(target) == TB_CC_StrategicRole.ORDINARY && (m_sDefenderFaction == "FIA" || m_sDefenderFaction == "USSR");
		if (!targetValid)
		{
			if (m_Garrison) Cleanup("no-opfor-frontline", false);
			return;
		}
		if (target != m_sObservedTarget || revision != m_iObservedRevision)
		{
			PrintFormat("[TB_PVE_M3_6] frontline-observed objective=%1 revision=%2 previous=%3 publicM2=1 state=ENEMY_FRONTLINE", target, revision, m_sObservedTarget);
			m_sObservedTarget = target; m_iObservedRevision = revision;
			m_iCreationFailures = 0; m_iRetryAfterMs = 0;
		}

		if (m_Garrison && m_Garrison.m_sObjectiveId != target)
		{
			m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.RETIRING;
			if (!Cleanup("frontier-changed", false)) return;
		}
		if (m_Garrison)
		{
			ObserveExisting(target);
			return;
		}
		int participants = CountActiveHumanBLUFOR();
		array<vector> observers = {}; CollectObservers(observers);
		if (participants != m_iLastParticipantCount || observers.Count() != m_iLastObserverCount)
		{
			PrintFormat("[TB_PVE_M3_6] human-gate-transition objective=%1 revision=%2 participants=%3 observers=%4 garrisonPresent=%5", target, revision, participants, observers.Count(), m_Garrison != null);
			m_iLastParticipantCount = participants;
			m_iLastObserverCount = observers.Count();
		}
		if (participants <= 0 && !ownerPolicy)
		{
			LogPending(target, revision, "no-active-human-blufor", observers.Count());
			return;
		}
		vector objectivePosition;
		if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(target, objectivePosition)) return;
		if (!ownerPolicy && ObjectiveUnsafeForCreation(objectivePosition, observers))
		{
			LogPending(target, revision, "near-or-observing-human", observers.Count());
			return;
		}
		if (m_bOwnerAware && System.GetTickCount() < m_iRetryAfterMs) return;
		if (m_bOwnerAware && ownerPolicy && ownerPolicy.GetOccupationService() && ownerPolicy.GetOccupationService().HasClaim(target)) { LogPending(target, revision, "territorial-occupation-already-owned", observers.Count()); return; }
		if (m_bOwnerAware && s_mOwnerGarrisonClaims.Contains(target) && s_mOwnerGarrisonClaims.Get(target) != this) { LogPending(target, revision, "territorial-garrison-already-owned", observers.Count()); return; }
		if (!CreateGarrison(target, revision, objectivePosition, observers) && m_bOwnerAware)
		{
			m_iCreationFailures++; m_iRetryAfterMs = System.GetTickCount() + RetryMilliseconds();
			PrintFormat("[TB_CC_GARRISON] creation-deferred base=%1 failures=%2 retryAfterMs=%3 retryIntervalMs=%4", target, m_iCreationFailures, m_iRetryAfterMs, RetryMilliseconds());
		}
	}

	protected void ObserveExisting(string target)
	{
		if (!m_Garrison) return;
		if (m_Garrison.m_iState == TB_PVE_M3_6_GarrisonState.PENDING_ACTIVATION)
		{
			ObserveNativeMaterialization(target);
			return;
		}
		int aliveGroups; int units;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_Garrison.m_aGroups)
		{
			if (!record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() <= 0) continue;
			aliveGroups++; units += record.m_Group.GetAgentsCount();
		}
		if (m_iLastAliveGroups >= 0 && aliveGroups < m_iLastAliveGroups)
			PrintFormat("[TB_PVE_M3_6] group-loss garrison=%1 objective=%2 aliveGroups=%3 previous=%4 units=%5", m_Garrison.m_sId, target, aliveGroups, m_iLastAliveGroups, units);
		m_iLastAliveGroups = aliveGroups;
		if (aliveGroups == 0 && m_Garrison.m_iState != TB_PVE_M3_6_GarrisonState.DEFEATED)
		{
			m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.DEFEATED;
			PrintFormat("[TB_PVE_M3_6] DEFEATED garrison=%1 objective=%2 revision=%3 regeneration=0 untilFrontierChange=1", m_Garrison.m_sId, target, TB_PVE_M2_3_FrontierQuery.GetRevision());
		}
		if (m_Garrison.m_iState == TB_PVE_M3_6_GarrisonState.DEFEATED) return;
		vector objectivePosition; TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(target, objectivePosition);
		if (HasEnemyNear(objectivePosition, 180) && m_Garrison.m_iState == TB_PVE_M3_6_GarrisonState.ACTIVE)
		{
			m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.HOSTILE_NEARBY;
			PrintFormat("[TB_PVE_M3_6] HOSTILE_NEARBY garrison=%1 objective=%2 units=%3 proximityOnly=1 combatUnverified=1", m_Garrison.m_sId, target, units);
			foreach (TB_PVE_M3_4_AssembledGroup observedRecord : m_Garrison.m_aGroups) LogRepresentativeControl(observedRecord, "hostile-nearby");
		}
	}

	// FIA Remnants prefabs deliberately disable immediate member spawning.
	// Let SCR_AIGroup's native queue create/init/register members, rather than
	// treating an empty shell as a failed or defeated combat group.
	protected void ObserveNativeMaterialization(string target)
	{
		bool complete = true;
		string failure;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_Garrison.m_aGroups)
		{
			if (!record.m_Group || record.m_Group.IsDeleted()) { failure = "native-group-deleted"; break; }
			array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
			record.m_aCharacters.Clear();
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity()) { complete = false; continue; }
				IEntity character = agent.GetControlledEntity(); record.m_aCharacters.Insert(character);
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
				Faction faction; if (affiliation) faction = affiliation.GetAffiliatedFaction();
				if (faction && faction.GetFactionKey() != m_Garrison.m_sFaction) failure = "native-character-faction-mismatch";
			}
			Faction groupFaction = record.m_Group.GetFaction();
			if (!groupFaction || groupFaction.GetFactionKey() != m_Garrison.m_sFaction) failure = "native-group-faction-mismatch";
			if (agents.Count() != record.m_Group.m_aUnitPrefabSlots.Count() || record.m_aCharacters.Count() != agents.Count()) complete = false;
			record.m_iNaturalLOD = record.m_Group.GetLOD();
		}
		int elapsed = System.GetTickCount() - m_iNativeSpawnStartMs;
		if (failure.IsEmpty() && !complete && elapsed < 30000) return;
		if (!complete && failure.IsEmpty()) failure = "native-member-spawn-timeout";
		if (!failure.IsEmpty())
		{
			PrintFormat("[TB_PVE_M3_6] native-materialization-failed objective=%1 reason=%2 elapsedMs=%3", target, failure, elapsed);
			foreach (TB_PVE_M3_4_AssembledGroup failedRecord : m_Garrison.m_aGroups) LogRepresentativeControl(failedRecord, "pre-rollback");
			Cleanup("native-materialization-failed", true);
			m_iCreationFailures++; m_iRetryAfterMs = System.GetTickCount() + RetryMilliseconds();
			return;
		}
		m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.ACTIVE;
		foreach (TB_PVE_M3_4_AssembledGroup materialized : m_Garrison.m_aGroups) materialized.m_bNativeMembersPending = false;
		m_iCreated++; m_iLastAliveGroups = m_Garrison.m_aGroups.Count(); m_iLastCreationDurationMs = elapsed;
		int ordinal;
		foreach (TB_PVE_M3_4_AssembledGroup completedRecord : m_Garrison.m_aGroups)
		{
			LogOperationalGroup(m_Garrison.m_sId, ordinal, completedRecord);
			LogRepresentativeControl(completedRecord, "native-members-complete"); ordinal++;
		}
		PrintFormat("[TB_PVE_M3_6] ACTIVE garrison=%1 objective=%2 package=%3 groups=%4 units=%5 nativeMembers=1 durationMs=%6 combatUnverified=1", m_Garrison.m_sId, target, m_Garrison.m_sPackageId, GetGroupCount(), GetUnitCount(), elapsed);
	}

	protected void LogRepresentativeControl(TB_PVE_M3_4_AssembledGroup record, string transition)
	{
		if (!record || !record.m_Group || record.m_Group.IsDeleted()) return;
		array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
		AIWaypoint current = record.m_Group.GetCurrentWaypoint();
		array<AIWaypoint> waypoints = {}; record.m_Group.GetWaypoints(waypoints);
		PrintFormat("[TB_PVE_M3_6] native-order transition=%1 group=%2 assigned=%3 current=%4 currentIsDefend=%5 listed=%6 agents=%7 LOD=%8", transition, record.m_Group, record.m_Waypoint, current, current == record.m_Waypoint && current != null, waypoints.Contains(record.m_Waypoint), agents.Count(), record.m_Group.GetLOD());
		if (agents.IsEmpty()) return;
		AIAgent agent = agents[0]; if (!agent) return;
		IEntity character = agent.GetControlledEntity(); if (!character) return;
		AIControlComponent control = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		AIAgent controllingAgent; if (control) controllingAgent = control.GetControlAIAgent();
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		BaseWeaponManagerComponent weapons = BaseWeaponManagerComponent.Cast(character.FindComponent(BaseWeaponManagerComponent));
		array<IEntity> weaponEntities = {}; if (weapons) weapons.GetWeaponsList(weaponEntities);
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
		Faction faction; if (affiliation) faction = affiliation.GetAffiliatedFaction();
		string factionKey = "UNKNOWN"; if (faction) factionKey = faction.GetFactionKey();
		PrintFormat("[TB_PVE_M3_6] native-control transition=%1 character=%2 control=%3 agent=%4 controllingAgent=%5 sameAgent=%6 member=%7 parentGroup=%8 active=%9", transition, character, control != null, agent, controllingAgent, agent == controllingAgent, agents.Contains(controllingAgent), agent.GetParentGroup(), agent.IsAIActivated());
		PrintFormat("[TB_PVE_M3_6] native-character faction=%1 alive=%2 weapons=%3 controlActive=%4 parentMatches=%5 agentLOD=%6 combatUnverified=1", factionKey, controller && controller.GetLifeState() == ECharacterLifeState.ALIVE, weaponEntities.Count(), control && control.IsAIActivated(), agent.GetParentGroup() == record.m_Group, agent.GetLOD());
	}

	protected bool CreateGarrison(string target, int revision, vector objectivePosition, array<vector> observers)
	{
		int startMs = System.GetTickCount();
		string selectedPackage = m_sPackageId;
		if (m_bOwnerAware && m_sDefenderFaction == "FIA")
		{
			if (m_iFIAConventionalStrength <= 0) { LogPending(target, revision, "FIA-conventional-strength-exhausted", observers.Count()); return false; }
			TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
			if (policy) selectedPackage = policy.SelectFIAPackage(target);
			else
			{
				int humans = CountActiveHumanBLUFOR(); selectedPackage = m_sFIAStandard;
				if (humans <= 1 || m_iFIAConventionalStrength < 334) selectedPackage = m_sFIALight;
				if (humans >= 6 && m_iFIAConventionalStrength >= 667) selectedPackage = m_sFIAReinforced;
			}
		}
		// Keep defensive quantities unchanged by USSR offensive strength tuning.
		TB_PVE_M3_4_ForcePackage package = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(selectedPackage, false);
		if (!package || package.m_sFactionKey != m_sDefenderFaction || package.GetExpectedGroupCount() < 1 || package.GetExpectedGroupCount() > m_iMaximumGarrisonGroups)
		{
			PrintFormat("[TB_PVE_M3_6] creation-failed objective=%1 reason=invalid-package package=%2", target, selectedPackage); return false;
		}
		PrintFormat("[TB_PVE_M3_6] package-selected objective=%1 package=%2 groups=%3 separateAttackPolicy=1", target, package.m_sPackageId, package.GetExpectedGroupCount());
		TB_PVE_M3_6_FrontlineGarrison candidate = new TB_PVE_M3_6_FrontlineGarrison();
		candidate.m_sId = "M3_6_GARRISON_" + (++m_iSerial).ToString(); candidate.m_sObjectiveId = target; candidate.m_sPackageId = package.m_sPackageId; candidate.m_iCreationRevision = revision; candidate.m_sFaction = m_sDefenderFaction;
		if (m_bOwnerAware) s_mOwnerGarrisonClaims.Set(target, this);
		int ordinal;
		foreach (TB_PVE_M3_4_ForcePackageEntry entry : package.m_aEntries)
		{
			for (int quantity; quantity < entry.m_iQuantity; quantity++)
			{
				vector position; string failure;
				if (!FindLocalPosition(objectivePosition, ordinal, observers, position, failure)) { Rollback(candidate); PrintFormat("[TB_PVE_M3_6] creation-failed objective=%1 reason=%2 ordinal=%3", target, failure, ordinal); return false; }
				TB_PVE_M3_6_MaterializationTrace trace = new TB_PVE_M3_6_MaterializationTrace();
				TB_PVE_M3_4_AssembledGroup record = SpawnGroup(entry, position, objectivePosition, trace);
				if (record && !IssueDefendOrder(record, objectivePosition, trace)) { TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); record = null; }
				if (!record) { Rollback(candidate); LogMaterializationFailure(target, ordinal, trace); return false; }
				candidate.m_aGroups.Insert(record);
				TB_CC_PhysicalWarService.SetRole(record, "garrison", false);
				m_sLastMaterializationFailureKey = "";
				PrintFormat("[TB_PVE_M3_6] group-created garrison=%1 ordinal=%2 group=%3 role=%4 units=%5 position=%6 LOD=%7 defensiveOrder=DefendSmall", candidate.m_sId, ordinal, record.m_Group, entry.m_sRole, record.m_Group.GetAgentsCount(), position, record.m_iNaturalLOD);
				if (candidate.m_sFaction != "FIA") LogOperationalGroup(candidate.m_sId, ordinal, record);
				ordinal++;
			}
		}
		if (target != GetObservedObjective() || revision != TB_PVE_M2_3_FrontierQuery.GetRevision()) { Rollback(candidate); Print("[TB_PVE_M3_6] stale-creation-rolled-back revisionOrTargetChanged=1"); return false; }
		if (candidate.m_sFaction == "FIA" || SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy())
		{
			candidate.m_iState = TB_PVE_M3_6_GarrisonState.PENDING_ACTIVATION;
			m_Garrison = candidate; m_iNativeSpawnStartMs = startMs; m_iLastAliveGroups = -1;
			m_iMaximumConcurrent = Math.Max(m_iMaximumConcurrent, 1); m_sLastPendingKey = "";
			PrintFormat("[TB_PVE_M3_6] native-materialization-pending objective=%1 package=%2 groups=%3", target, candidate.m_sPackageId, candidate.m_aGroups.Count());
			return true;
		}
		candidate.m_iState = TB_PVE_M3_6_GarrisonState.ACTIVE; m_Garrison = candidate; m_iCreated++; m_iLastAliveGroups = candidate.m_aGroups.Count(); m_iMaximumConcurrent = Math.Max(m_iMaximumConcurrent, 1); m_iLastCreationDurationMs = System.GetTickCount() - startMs; m_sLastPendingKey = "";
		PrintFormat("[TB_PVE_M3_6] ACTIVE garrison=%1 objective=%2 revision=%3 package=%4 groups=%5 units=%6 durationMs=%7 maxConcurrent=%8", candidate.m_sId, target, revision, candidate.m_sPackageId, candidate.m_aGroups.Count(), GetUnitCount(), m_iLastCreationDurationMs, m_iMaximumConcurrent);
		return true;
	}

	protected void LogMaterializationFailure(string target, int ordinal, TB_PVE_M3_6_MaterializationTrace trace)
	{
		string key = target + ":" + ordinal.ToString() + ":" + trace.m_sFirstFailure;
		if (key == m_sLastMaterializationFailureKey) return;
		m_sLastMaterializationFailureKey = key;
		PrintFormat("[TB_PVE_M3_6] materialization-failed objective=%1 ordinal=%2 firstStep=%3 groupCreated=%4 groupFactionBefore=%5 groupFactionAfter=%6 slots=%7 charactersCreated=%8 agents=%9", target, ordinal, trace.m_sFirstFailure, trace.m_bGroupCreated, trace.m_sGroupFactionBefore, trace.m_sGroupFactionAfter, trace.m_iExpectedSlots, trace.m_iCharactersCreated, trace.m_iAgents);
		PrintFormat("[TB_PVE_M3_6] materialization-detail FIA=%1 USSR=%2 US=%3 other=%4 unknown=%5 naturalLOD=%6 maxLOD=%7 waypointCreated=%8 assigned=%9 rollback=1", trace.m_iFIAAffiliations, trace.m_iUSSRAffiliations, trace.m_iUSAffiliations, trace.m_iOtherAffiliations, trace.m_iMissingAffiliationComponents, trace.m_iNaturalLOD, AIAgent.GetMaxLOD(), trace.m_bWaypointCreated, trace.m_bWaypointAssignmentIssued);
	}

	protected void LogOperationalGroup(string garrisonId, int ordinal, TB_PVE_M3_4_AssembledGroup record)
	{
		Faction groupFaction = record.m_Group.GetFaction();
		Faction us = GetGame().GetFactionManager().GetFactionByKey("US");
		string groupKey = "<none>"; if (groupFaction) groupKey = groupFaction.GetFactionKey();
		int fiaUnits; int ussrUnits; int usUnits; int otherUnits; int unknownUnits;
		foreach (IEntity character : record.m_aCharacters)
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
			Faction effectiveFaction; if (affiliation) effectiveFaction = affiliation.GetAffiliatedFaction();
			if (!effectiveFaction) unknownUnits++;
			else if (effectiveFaction.GetFactionKey() == "FIA") fiaUnits++;
			else if (effectiveFaction.GetFactionKey() == "USSR") ussrUnits++;
			else if (effectiveFaction.GetFactionKey() == "US") usUnits++;
			else otherUnits++;
		}
		array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
		int controlledAgents;
		foreach (AIAgent agent : agents) if (agent && agent.GetControlledEntity()) controlledAgents++;
		PrintFormat("[TB_PVE_M3_6] materialized-group garrison=%1 ordinal=%2 groupFaction=%3 units=%4 FIAUnits=%5 USSRUnits=%6 USUnits=%7 unknownUnits=%8 otherUnits=%9", garrisonId, ordinal, groupKey, record.m_Group.GetAgentsCount(), fiaUnits, ussrUnits, usUnits, unknownUnits, otherUnits);
		Faction ussr = GetGame().GetFactionManager().GetFactionByKey("USSR");
		PrintFormat("[TB_PVE_M3_6] combat-order garrison=%1 ordinal=%2 defendSmall=%3 USHostile=%4 USSRHostile=%5 controlledAgents=%6 LOD=%7", garrisonId, ordinal, record.m_Waypoint != null, groupFaction && us && groupFaction.IsFactionEnemy(us), groupFaction && ussr && groupFaction.IsFactionEnemy(ussr), controlledAgents, record.m_Group.GetLOD());
	}

	bool AreGroupsCombatReadyForProof()
	{
		if (!m_Garrison || m_Garrison.m_aGroups.IsEmpty()) return false;
		Faction us = GetGame().GetFactionManager().GetFactionByKey("US");
		Faction ussr = GetGame().GetFactionManager().GetFactionByKey(m_sDefenderFaction);
		if (!us || !ussr || !ussr.IsFactionEnemy(us)) return false;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_Garrison.m_aGroups)
		{
			if (!record.m_Group || record.m_Group.GetFaction() != ussr || !record.m_Waypoint || record.m_Group.GetLOD() == AIAgent.GetMaxLOD()) return false;
			if (record.m_Group.GetAgentsCount() != record.m_aCharacters.Count()) return false;
			foreach (IEntity character : record.m_aCharacters)
			{
				if (!character) return false;
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
				if (affiliation && affiliation.GetAffiliatedFaction() && affiliation.GetAffiliatedFaction() != ussr) return false;
			}
		}
		return true;
	}

	protected TB_PVE_M3_4_AssembledGroup SpawnGroup(TB_PVE_M3_4_ForcePackageEntry entry, vector position, vector objectivePosition, TB_PVE_M3_6_MaterializationTrace trace)
	{
		Resource resource = Resource.Load(entry.m_sGroupPrefab);
		if (!resource || !resource.IsValid()) { trace.m_sFirstFailure = "group-prefab-resource"; return null; }
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = position;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!group) { trace.m_sFirstFailure = "group-entity-spawn"; return null; }
		trace.m_bGroupCreated = true;
		TB_PVE_M3_4_AssembledGroup record = new TB_PVE_M3_4_AssembledGroup(); record.m_Group = group; record.m_vStartPosition = position; record.m_fStartDistance = vector.Distance(position, objectivePosition); record.m_sEntryRole = entry.m_sRole;
		Faction ussr = GetGame().GetFactionManager().GetFactionByKey(m_sDefenderFaction);
		if (!ussr) { trace.m_sFirstFailure = "ussr-faction-unavailable"; TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
		Faction beforeFaction = group.GetFaction(); if (beforeFaction) trace.m_sGroupFactionBefore = beforeFaction.GetFactionKey();
		if (group.GetFaction() != ussr) group.SetFaction(ussr);
		Faction afterFaction = group.GetFaction(); if (afterFaction) trace.m_sGroupFactionAfter = afterFaction.GetFactionKey();
		if (afterFaction != ussr) { trace.m_sFirstFailure = "group-faction-verification"; TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
		trace.m_iExpectedSlots = group.m_aUnitPrefabSlots.Count();
		if (m_sDefenderFaction == "FIA" || SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy())
		{
			if (trace.m_iExpectedSlots <= 0 || group.GetSpawnImmediately()) { trace.m_sFirstFailure = "unexpected-fia-native-spawn-contract"; TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
			// Same group-owned queue used by vanilla ambient FIA patrols. Native
			// SpawnGroupMember -> AddAIEntityToGroup initializes AI control and
			// fires group member-completion callbacks. No BCM activation repair.
			group.RequestSpawn(trace.m_iExpectedSlots);
			record.m_bNativeMembersPending = true; record.m_iNativeSpawnStartMs = System.GetTickCount();
			return record;
		}
		int slot;
		foreach (ResourceName unitPrefab : group.m_aUnitPrefabSlots)
		{
			int row = slot / 4; int column = slot - row * 4; vector unitPosition = position + Vector(column * 1.5, 0, row * 1.5); unitPosition[1] = GetGame().GetWorld().GetSurfaceY(unitPosition[0], unitPosition[2]) + 0.25; params.Transform[3] = unitPosition;
			Resource unitResource = Resource.Load(unitPrefab);
			if (!unitResource || !unitResource.IsValid()) { trace.m_sFirstFailure = "character-prefab-resource-slot-" + slot.ToString(); TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
			IEntity character = GetGame().SpawnEntityPrefab(unitResource, GetGame().GetWorld(), params);
			if (character)
			{
				trace.m_iCharactersCreated++;
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
				Faction effectiveFaction; if (affiliation) effectiveFaction = affiliation.GetAffiliatedFaction();
				string characterKey = "UNKNOWN"; if (effectiveFaction) characterKey = effectiveFaction.GetFactionKey();
				PrintFormat("[TB_PVE_M3_6] unit-created slot=%1 prefab=%2 effectiveFaction=%3 groupFaction=%4", slot, unitPrefab, characterKey, m_sDefenderFaction);
				if (!effectiveFaction) trace.m_iMissingAffiliationComponents++;
				else if (characterKey == "FIA") trace.m_iFIAAffiliations++;
				else if (characterKey == "USSR") trace.m_iUSSRAffiliations++;
				else if (characterKey == "US") trace.m_iUSAffiliations++;
				else trace.m_iOtherAffiliations++;
				if (effectiveFaction && effectiveFaction != ussr) { trace.m_sFirstFailure = "character-affiliation-mismatch-slot-" + slot.ToString(); SCR_EntityHelper.DeleteEntityAndChildren(character); TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
				int agentsBefore = group.GetAgentsCount(); group.AddAgentFromControlledEntity(character); record.m_aCharacters.Insert(character);
				trace.m_iAgents = group.GetAgentsCount();
				if (trace.m_iAgents <= agentsBefore && trace.m_sFirstFailure.IsEmpty()) trace.m_sFirstFailure = "add-agent-slot-" + slot.ToString();
			}
			else { trace.m_sFirstFailure = "character-entity-spawn-slot-" + slot.ToString(); TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
			slot++;
		}
		trace.m_iAgents = group.GetAgentsCount();
		if (trace.m_iAgents == 0 || trace.m_iAgents != trace.m_iExpectedSlots) { if (trace.m_sFirstFailure.IsEmpty()) trace.m_sFirstFailure = "expected-agent-count"; TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
		record.m_iNaturalLOD = group.GetLOD();
		trace.m_iNaturalLOD = record.m_iNaturalLOD;
		if (record.m_iNaturalLOD == AIAgent.GetMaxLOD()) { trace.m_sFirstFailure = "natural-max-lod"; TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); return null; }
		return record;
	}

	protected bool IssueDefendOrder(TB_PVE_M3_4_AssembledGroup record, vector objectivePosition, TB_PVE_M3_6_MaterializationTrace trace)
	{
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = objectivePosition;
		Resource waypointResource = Resource.Load(DEFEND_WAYPOINT);
		if (!waypointResource || !waypointResource.IsValid()) { trace.m_sFirstFailure = "defend-waypoint-resource"; return false; }
		record.m_Waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(waypointResource, GetGame().GetWorld(), params));
		if (!record.m_Waypoint) { trace.m_sFirstFailure = "defend-waypoint-spawn"; return false; }
		trace.m_bWaypointCreated = true;
			record.m_Group.AddWaypoint(record.m_Waypoint); record.m_bDefensiveOrder = true; trace.m_bWaypointAssignmentIssued = true; return true;
	}

	protected bool FindLocalPosition(vector objectivePosition, int ordinal, array<vector> observers, out vector result, out string failure)
	{
		if (!m_Navmesh) { failure = "navmesh-unavailable"; return false; }
		// First exhaust hidden/distant candidates, then use a valid local point
		// outside immediate character overlap. Proximity is not a defence veto.
		int passes = 1; if (m_bOwnerAware) passes = 2;
		for (int pass; pass < passes; pass++)
		{
		m_bPreferHiddenPlacement = pass == 0;
		for (int attempt; attempt < m_iCandidateAttempts; attempt++)
		{
			int index = ordinal * 7 + attempt;
			int band = (index / 12); band = band - (band / 3) * 3;
			int direction = index - (index / 12) * 12;
			float radius = 18 + band * ((m_fPlacementRadius - 18) / 2); float angle = direction * 0.523599;
			vector candidate = objectivePosition + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius); candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
			if (!ValidateLocalCandidate(candidate, observers, failure)) continue;
			result = candidate; PrintFormat("[TB_PVE_M3_6] placement-valid ordinal=%1 position=%2 distance=%3 attempt=%4 land=1 nav=1 support=1", ordinal, candidate, vector.Distance(candidate, objectivePosition), attempt + 1); return true;
		}
		}
		failure = "no-valid-objective-local-position"; return false;
	}

	protected bool ValidateLocalCandidate(vector candidate, array<vector> observers, out string failure)
	{
		if (ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), candidate)) { failure = "water"; return false; }
		if (!m_Navmesh.IsTileValid(candidate)) { m_Navmesh.LoadTileIn(candidate); failure = "navigation-invalid"; return false; }
		TraceParam trace = MakeTraceParam(candidate + Vector(0, 4, 0), candidate - Vector(0, 8, 0), TraceFlags.ENTS | TraceFlags.WORLD); float fraction = GetGame().GetWorld().TraceMove(trace, FilterTraceHit);
		if (fraction >= 1 || trace.TraceNorm[1] < 0.82) { failure = "support-or-slope"; return false; }
		foreach (vector observer : observers)
		{
			float distance = vector.Distance(observer, candidate);
			if (!m_bOwnerAware && distance < m_fMinimumPlayerDistance) { failure = "human-distance"; return false; }
			if (m_bOwnerAware && distance < 15) { failure = "human-overlap"; return false; }
			if (m_bOwnerAware && m_bPreferHiddenPlacement && distance < m_fVisibilityRange && HasDirectVisibility(observer, candidate)) { failure = "prefer-hidden"; return false; }
		}
		return true;
	}

	protected bool ObjectiveUnsafeForCreation(vector objectivePosition, array<vector> observers)
	{
		foreach (vector observer : observers)
		{
			float distance = vector.Distance(observer, objectivePosition);
			if (distance < m_fMinimumPlayerDistance) return true;
			if (distance <= m_fVisibilityRange && HasDirectVisibility(observer, objectivePosition)) return true;
		}
		return false;
	}

	protected void LogPending(string target, int revision, string reason, int observers)
	{
		string key = target + ":" + reason;
		if (key == m_sLastPendingKey) return;
		m_sLastPendingKey = key;
		PrintFormat("[TB_PVE_M3_6] creation-pending objective=%1 revision=%2 reason=%3 observers=%4", target, revision, reason, observers);
	}

	protected bool Cleanup(string reason, bool force)
	{
		if (m_WarDefence) { m_WarDefence = null; m_Garrison = null; return true; }
		if (!m_Garrison) return true;
		if (!force && !SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy())
		{
			vector position; array<vector> observers = {}; CollectObservers(observers);
			TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(m_Garrison.m_sObjectiveId, position);
			foreach (vector observer : observers) if (vector.Distance(observer, position) < m_fCleanupPlayerDistance)
			{
				m_Garrison.m_iState = TB_PVE_M3_6_GarrisonState.RETIRING;
				string key = m_Garrison.m_sId + ":" + reason;
				if (key != m_sRetirementDelayKey) PrintFormat("[TB_PVE_M3_6] retirement-delayed garrison=%1 reason=%2 nearbyHuman=1", m_Garrison.m_sId, reason);
				m_sRetirementDelayKey = key; return false;
			}
		}
		int startMs = System.GetTickCount(); int groups = m_Garrison.m_aGroups.Count(); int units = GetUnitCount(); string id = m_Garrison.m_sId; string objective = m_Garrison.m_sObjectiveId;
		Rollback(m_Garrison); m_Garrison = null; m_iLastAliveGroups = -1; m_iLastCleanupDurationMs = System.GetTickCount() - startMs; m_sRetirementDelayKey = "";
		PrintFormat("[TB_PVE_M3_6] cleanup garrison=%1 objective=%2 groups=%3 units=%4 reason=%5 durationMs=%6 visibleDeletionPrevented=1", id, objective, groups, units, reason, m_iLastCleanupDurationMs); return true;
	}

	protected void Rollback(TB_PVE_M3_6_FrontlineGarrison garrison)
	{
		if (garrison && s_mOwnerGarrisonClaims.Get(garrison.m_sObjectiveId) == this) s_mOwnerGarrisonClaims.Remove(garrison.m_sObjectiveId);
		if (!garrison) return;
		foreach (TB_PVE_M3_4_AssembledGroup record : garrison.m_aGroups)
		{
			// Native queued members may appear between manager ticks. Include
			// them in existing cleanup ownership even during partial assembly.
			if (garrison.m_sFaction == "FIA" && record.m_Group && !record.m_Group.IsDeleted())
			{
				array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
				foreach (AIAgent agent : agents)
				{
					if (agent && agent.GetControlledEntity() && !record.m_aCharacters.Contains(agent.GetControlledEntity())) record.m_aCharacters.Insert(agent.GetControlledEntity());
				}
			}
			TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record);
		}
		garrison.m_aGroups.Clear();
	}

	protected void CollectObservers(out array<vector> observers)
	{
		if (m_aProofObservers) { foreach (vector proofObserver : m_aProofObservers) observers.Insert(proofObserver); return; }
		TB_PVE_M3_7_HumanPresenceQuery.GetActiveHumanObservers("US", observers);
	}

	protected int CountActiveHumanBLUFOR()
	{
		if (m_iProofHumanCount >= 0) return m_iProofHumanCount;
		return TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US");
	}

	protected bool HasEnemyNear(vector position, float radius)
	{
		m_aNearbyQuery.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(position, radius, CollectNearbyEntity, null, EQueryEntitiesFlags.DYNAMIC);
		Faction defender = GetGame().GetFactionManager().GetFactionByKey(m_sDefenderFaction);
		foreach (IEntity entity : m_aNearbyQuery)
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (affiliation && affiliation.GetAffiliatedFaction() && defender && defender.IsFactionEnemy(affiliation.GetAffiliatedFaction())) return true;
		}
		return false;
	}
	protected bool CollectNearbyEntity(IEntity entity) { m_aNearbyQuery.Insert(entity); return true; }
	protected bool HasDirectVisibility(vector observer, vector destination) { TraceParam trace = MakeTraceParam(observer, destination + Vector(0, 1, 0), TraceFlags.ENTS | TraceFlags.WORLD); return GetGame().GetWorld().TraceMove(trace, FilterTraceHit) >= 0.98; }
	protected bool FilterTraceHit(notnull IEntity target, vector traceStart = "0 0 0", vector traceDirection = "0 0 0") { return SCR_Global.FilterCallback_IgnoreCharactersWithChildren(target); }

	void SetProofHumanCount(int count) { m_iProofHumanCount = count; }
	void SetProofObservers(array<vector> observers) { m_aProofObservers = observers; }
	void ClearProofObservers() { m_aProofObservers = null; }
	void DestroyForProof()
	{
		if (!m_Garrison) return;
		foreach (TB_PVE_M3_4_AssembledGroup record : m_Garrison.m_aGroups)
		{
			while (!record.m_aCharacters.IsEmpty())
			{
				IEntity character = record.m_aCharacters.Get(record.m_aCharacters.Count() - 1);
				record.m_aCharacters.Remove(record.m_aCharacters.Count() - 1);
				if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
			}
			if (record.m_Group && !record.m_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(record.m_Group);
		}
		PrintFormat("[TB_PVE_M3_6] proof-force-destroyed garrison=%1 groups=%2", m_Garrison.m_sId, m_Garrison.m_aGroups.Count());
	}
	bool HasGarrison() { return m_Garrison != null; }
	int GetState()
	{
		if (!m_Garrison) return TB_PVE_M3_6_GarrisonState.NONE;
		return m_Garrison.m_iState;
	}
	string GetObjectiveId()
	{
		if (!m_Garrison) return "";
		return m_Garrison.m_sObjectiveId;
	}
	int GetGroupCount()
	{
		if (!m_Garrison) return 0;
		return m_Garrison.m_aGroups.Count();
	}
	int GetUnitCount()
	{
		if (!m_Garrison) return 0;
		return TB_PVE_M3_4_ForceAssemblyService.CountUnits(m_Garrison.m_aGroups);
	}
	int GetMaximumConcurrent() { return m_iMaximumConcurrent; }
	int GetCreatedCount() { return m_iCreated; }
	int GetLastCreationDurationMs() { return m_iLastCreationDurationMs; }
	int GetLastCleanupDurationMs() { return m_iLastCleanupDurationMs; }
	int GetFirstGroupLOD()
	{
		if (!m_Garrison || m_Garrison.m_aGroups.IsEmpty() || !m_Garrison.m_aGroups[0].m_Group) return -1;
		return m_Garrison.m_aGroups[0].m_Group.GetLOD();
	}
}
