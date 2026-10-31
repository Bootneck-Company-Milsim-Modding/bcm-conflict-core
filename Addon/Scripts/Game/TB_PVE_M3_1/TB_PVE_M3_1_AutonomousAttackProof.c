// Dedicated authority proof. Topology setup uses genuine vanilla capture; the
// operation never receives a route index or authored target ID from this class.
class TB_PVE_M3_1_AutonomousAttackProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static ref TB_PVE_M3_1_Operation s_Operation;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iOperationSeconds;
	protected static int s_iSetupTarget;
	protected static int s_iLastCaptureState = -1;
	protected static int s_iSuccessRevisionBefore;
	protected static string s_sSuccessTarget;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route)
	{
		if (!Replication.IsServer() || !route || !route.IsReady()) return;
		s_Route = route;
		if (TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE)
		{
			Fail("campaign-not-active-at-proof-start"); return;
		}
		PrintFormat("[TB_PVE_M3_1] proof-start route=%1 revision=%2 initialOPFORLegal=%3 dedicated=%4", route.GetRouteIdentity(), TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), RplSession.Mode() == RplMode.Dedicated);
		s_iPhase = 1;
		StartSetupCapture(1, "US", "establish-valid-opfor-objective");
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_iTicks++;
		LogSetupCaptureTransition();
		if (s_Operation && !s_Operation.IsTerminal())
		{
			s_iOperationSeconds += 2;
			s_Operation.Tick(s_iOperationSeconds);
		}

		if (s_iPhase == 1)
		{
			if (Owner(1) != "US") { if (s_iTicks > 60) Fail("initial-topology-capture-timeout"); return; }
			CleanupSetup();
			bool topology = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() == s_Route.GetRouteId(1) && TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", s_Route.GetRouteId(1));
			PrintFormat("[TB_PVE_M3_1] topology-ready pass=%1 revision=%2 BLUFORLegal=%3 OPFORLegal=%4", topology, TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
			if (!topology) { Fail("setup-topology-semantic-mismatch"); return; }
			StartOperation("M3_1_CANCEL");
			s_iPhase = 2; s_iTicks = 0;
			return;
		}

		if (s_iPhase == 2)
		{
			if (s_iTicks == 4)
				StartSetupCapture(2, "US", "legitimate-frontier-change-for-cancellation");
			if (!s_Operation.IsTerminal()) { if (s_iTicks > 65) Fail("cancellation-frontier-change-timeout"); return; }
			if (s_Operation.GetState() != TB_PVE_M3_1_OperationState.CANCELLED || Owner(1) != "US") { Fail("operation-did-not-cancel-safely"); return; }
			PrintFormat("[TB_PVE_M3_1] PASS cancellation target=%1 targetOwner=%2 revision=%3 leaseReleased=%4 m2Authoritative=1", s_Operation.GetTargetObjectiveId(), Owner(1), TB_PVE_M2_3_FrontierQuery.GetRevision(), !s_Operation.IsLeaseHeld());
			s_Operation.CleanupAssignedForce("post-cancellation-proof");
			s_Operation = null;
			CleanupSetup();
			StartSetupCapture(2, "USSR", "restore-opfor-legal-objective");
			s_iPhase = 3; s_iTicks = 0;
			return;
		}

		if (s_iPhase == 3)
		{
			if (Owner(2) != "USSR") { if (s_iTicks > 60) Fail("topology-restore-timeout"); return; }
			CleanupSetup();
			if (TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_Route.GetRouteId(1)) { Fail("opfor-objective-not-restored"); return; }
			StartOperation("M3_1_DESTROY");
			s_iPhase = 4; s_iTicks = 0;
			return;
		}

		if (s_iPhase == 4)
		{
			if (s_iTicks == 5) s_Operation.DestroyAssignedForceForProof();
			if (!s_Operation.IsTerminal()) { if (s_iTicks > 15) Fail("destroyed-group-not-detected"); return; }
			if (s_Operation.GetState() != TB_PVE_M3_1_OperationState.FAILED || Owner(1) != "US") { Fail("destruction-regression-mismatch"); return; }
			PrintFormat("[TB_PVE_M3_1] PASS destruction state=FAILED ownerStable=%1 frontierRevision=%2 leaseReleased=%3", Owner(1), TB_PVE_M2_3_FrontierQuery.GetRevision(), !s_Operation.IsLeaseHeld());
			s_Operation.CleanupAssignedForce("post-destruction-proof");
			s_Operation = null;
			s_iSuccessRevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
			StartOperation("M3_1_SUCCESS");
			s_sSuccessTarget = s_Operation.GetTargetObjectiveId();
			s_iPhase = 5; s_iTicks = 0; s_iLastCaptureState = -1;
			return;
		}

		if (s_iPhase == 5)
		{
			int targetIndex = FindRouteIndex(s_sSuccessTarget);
			if (targetIndex < 0) { Fail("success-target-not-in-route"); return; }
			int captureState = s_Route.GetRouteBase(targetIndex).GetCaptureState();
			if (captureState != s_iLastCaptureState)
			{
				PrintFormat("[TB_PVE_M3_1] vanilla-capture-observed target=%1 captureState=%2 owner=%3 revision=%4", s_sSuccessTarget, captureState, Owner(targetIndex), TB_PVE_M2_3_FrontierQuery.GetRevision());
				s_iLastCaptureState = captureState;
			}
			if (!s_Operation.IsTerminal()) { if (s_iTicks > 160) Fail("autonomous-operation-timeout"); return; }
			bool success = s_Operation.GetState() == TB_PVE_M3_1_OperationState.SUCCEEDED;
			bool ownerChanged = Owner(targetIndex) == "USSR";
			bool frontierRetreated = TB_PVE_M2_3_FrontierQuery.GetRevision() > s_iSuccessRevisionBefore && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_sSuccessTarget;
			bool movement = s_Operation.HasMeaningfulMovement() && s_Operation.GetMinimumDistance() < s_Operation.GetStartingDistance() - 25;
			bool leaseReleased = !s_Operation.IsLeaseHeld();
			PrintFormat("[TB_PVE_M3_1] final-evidence operationSuccess=%1 targetFromPublicQuery=%2 startDistance=%3 minimumDistance=%4 physicalMovement=%5 vanillaOwner=%6 revisionBefore=%7 revisionAfter=%8 frontierRetreated=%9 FIAOperation=0", success, s_sSuccessTarget, s_Operation.GetStartingDistance(), s_Operation.GetMinimumDistance(), movement, Owner(targetIndex), s_iSuccessRevisionBefore, TB_PVE_M2_3_FrontierQuery.GetRevision(), frontierRetreated);
			PrintFormat("[TB_PVE_M3_1] final-evidence-cleanup target=%1 leaseReleased=%2", s_sSuccessTarget, leaseReleased);
			if (!success || !ownerChanged || !frontierRetreated || !movement || !leaseReleased) { Fail("final-acceptance-mismatch"); return; }
			s_Operation.CleanupAssignedForce("proof-complete");
			Print("[TB_PVE_M3_1] COMPLETE PASS oneOperation=1 normalAIWaypoint=1 teleport=0 directOwnership=0 directFrontier=0 vanillaCapture=1 FIAExcluded=1");
			GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static void StartOperation(string operationId)
	{
		s_Operation = new TB_PVE_M3_1_Operation();
		s_iOperationSeconds = 0;
		if (!s_Operation.Create(operationId, s_Route.GetM3_3StagingPolicy(), s_Route.GetM3_4AssemblyService(), "LIGHT"))
			Fail("operation-create-failed-" + operationId);
	}

	protected static void StartSetupCapture(int index, FactionKey factionKey, string label)
	{
		CleanupSetup();
		s_iSetupTarget = index; s_iLastCaptureState = -1;
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		if (!faction) { Fail("setup-faction-unavailable"); return; }
		ResourceName prefab = faction.GetDefendersGroupPrefab();
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		vector origin = s_Route.GetRouteBase(index).GetOwner().GetOrigin(); params.Transform[3] = origin;
		s_SetupGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params));
		if (!s_SetupGroup) { Fail("setup-group-spawn-failed"); return; }
		int slot;
		for (int pass; pass < 2; pass++)
		{
			foreach (ResourceName unitPrefab : s_SetupGroup.m_aUnitPrefabSlots)
			{
				int row = slot / 4;
				int column = slot - (row * 4);
				vector position = origin + Vector(column * 2, 0, row * 2);
				position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.5;
				params.Transform[3] = position;
				IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
				if (character) { s_SetupGroup.AddAgentFromControlledEntity(character); s_aSetupCharacters.Insert(character); }
				slot++;
			}
		}
		s_SetupGroup.SetLOD(0);
		PrintFormat("[TB_PVE_M3_1] proof-topology-capture-start label=%1 faction=%2 objective=%3 actors=%4 mechanism=vanilla-seizing", label, factionKey, s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}

	protected static void LogSetupCaptureTransition()
	{
		if (!s_SetupGroup || s_SetupGroup.IsDeleted()) return;
		int state = s_Route.GetRouteBase(s_iSetupTarget).GetCaptureState();
		if (state == s_iLastCaptureState) return;
		s_iLastCaptureState = state;
		PrintFormat("[TB_PVE_M3_1] proof-topology-capture-progress objective=%1 state=%2 owner=%3", s_Route.GetRouteId(s_iSetupTarget), state, Owner(s_iSetupTarget));
	}

	protected static void CleanupSetup()
	{
		foreach (IEntity character : s_aSetupCharacters)
			if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_aSetupCharacters.Clear();
		if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup);
		s_SetupGroup = null;
	}

	protected static int FindRouteIndex(string objectiveId)
	{
		for (int i; i < s_Route.GetRouteCount(); i++)
			if (s_Route.GetRouteId(i) == objectiveId) return i;
		return -1;
	}

	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		return faction.GetFactionKey();
	}

	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M3_1] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 revision=%4 OPFORLegal=%5", reason, s_iPhase, s_iTicks, TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
		if (s_Operation) s_Operation.CleanupAssignedForce("proof-failed");
		CleanupSetup();
		GetGame().GetCallqueue().Remove(Tick);
	}
}
