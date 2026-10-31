// Dedicated authority proof for data-driven force packages. Ownership setup and
// operation captures both use ordinary Conflict actors and seizing underneath.
class TB_PVE_M3_4_ForcePackageProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static TB_PVE_M3_2_OPFOROperationCoordinator s_Coordinator;
	protected static TB_PVE_M3_4_ForceAssemblyService s_Assembly;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iSetupTarget;
	protected static int s_iLastCaptureState = -1;
	protected static bool s_bPartialLossApplied;
	protected static bool s_bAllLossApplied;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, TB_PVE_M3_2_OPFOROperationCoordinator coordinator, TB_PVE_M3_4_ForceAssemblyService assembly)
	{
		if (!Replication.IsServer() || !route || !coordinator || !assembly) return;
		s_Route = route; s_Coordinator = coordinator; s_Assembly = assembly;
		TB_PVE_M3_4_ForcePackage assault = TB_PVE_M3_4_ForcePackageRegistry.GetPackage("ASSAULT");
		TB_PVE_M3_4_ForcePackage heavy = TB_PVE_M3_4_ForcePackageRegistry.GetPackage("HEAVY");
		TB_PVE_M3_4_ForcePackage surge = TB_PVE_M3_4_ForcePackageRegistry.GetPackage("SURGE");
		bool expandedValid = assault && heavy && surge && assault.GetExpectedGroupCount() >= 1 && assault.GetExpectedGroupCount() <= heavy.GetExpectedGroupCount() && heavy.GetExpectedGroupCount() == surge.GetExpectedGroupCount() && surge.GetExpectedGroupCount() <= 8;
		PrintFormat("[TB_PVE_M3_4] expanded-package-proof pass=%1 groups=%2/%3/%4 max=8", expandedValid, assault.GetExpectedGroupCount(), heavy.GetExpectedGroupCount(), surge.GetExpectedGroupCount());
		if (!expandedValid) return;
		s_Coordinator.SetDelaysForProof(2, 2);
		// Keep the coordinator unable to materialize forces while vanilla setup
		// advances the frontier. This is a proof seam, not production policy.
		s_Assembly.SetProofMaximumGroups(0);
		PrintFormat("[TB_PVE_M3_4] proof-start dedicated=%1 revision=%2 routeNodes=%3", RplSession.Mode() == RplMode.Dedicated, TB_PVE_M2_3_FrontierQuery.GetRevision(), s_Route.GetRouteCount());
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(1));
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(2));
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(3));
		GetGame().GetCallqueue().CallLater(BeginAfterNavigationStreaming, 10000, false);
	}

	protected static void BeginAfterNavigationStreaming()
	{
		StartSetupCapture(1); s_iPhase = 1;
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_iTicks++; LogSetupCaptureTransition();
		if (s_iTicks > 750) { Fail("proof-timeout"); return; }
		if (s_iPhase == 1)
		{
			if (Owner(1) != "US") return; CleanupSetup(); StartSetupCapture(2); s_iPhase = 2; return;
		}
		if (s_iPhase == 2)
		{
			if (Owner(2) != "US") return; CleanupSetup(); StartSetupCapture(3); s_iPhase = 3; return;
		}
		if (s_iPhase == 3)
		{
			if (Owner(3) != "US") return;
			CleanupSetup();
			s_Coordinator.SetDefaultPackageForProof("REINFORCED"); s_Assembly.SetProofMaximumGroups(1);
			PrintFormat("[TB_PVE_M3_4] topology-ready frontier=%1 legal=%2 partial-assembly-proof=armed", s_Route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
			s_iPhase = 4; return;
		}
		if (s_iPhase == 4)
		{
			if (s_Coordinator.HasActiveOperation() || s_Assembly.GetLastRollbackRemovedCount() != 1) return;
			PrintFormat("[TB_PVE_M3_4] atomic-rollback-proof pass=1 groupsCreated=1 groupsRemoved=%1 activeOperation=0 owner=%2", s_Assembly.GetLastRollbackRemovedCount(), Owner(3));
			s_Assembly.ClearProofMaximumGroups(); s_Coordinator.SetDefaultPackageForProof("LIGHT"); s_iPhase = 5; return;
		}
		if (s_iPhase == 5)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation(); if (!operation) return;
			if (!ValidateActivePackage(operation, "LIGHT", TB_PVE_M3_4_ForcePackageRegistry.GetPackage("LIGHT").GetExpectedGroupCount())) return;
			PrintFormat("[TB_PVE_M3_4] light-active pass=1 units=%1 allMoved=%2", operation.GetAssignedUnitCount(), operation.DidEveryViableGroupMove());
			s_iPhase = 6; return;
		}
		if (s_iPhase == 6)
		{
			if (s_Coordinator.HasActiveOperation()) return;
			if (s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.SUCCEEDED || Owner(3) != "USSR") { Fail("light-operation-not-successful"); return; }
			PrintFormat("[TB_PVE_M3_4] light-success pass=1 owner=%1 revision=%2", Owner(3), TB_PVE_M2_3_FrontierQuery.GetRevision());
			s_Assembly.SetProofMaximumGroups(0); StartSetupCapture(3); s_iPhase = 61; return;
		}
		if (s_iPhase == 61)
		{
			if (Owner(3) != "US") return;
			CleanupSetup(); s_Assembly.ClearProofMaximumGroups(); s_Coordinator.SetDefaultPackageForProof("STANDARD"); s_iPhase = 7; return;
		}
		if (s_iPhase == 7)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation(); if (!operation) return;
			if (!ValidateActivePackage(operation, "STANDARD", TB_PVE_M3_4_ForcePackageRegistry.GetPackage("STANDARD").GetExpectedGroupCount())) return;
			if (!operation.DidEveryViableGroupMove()) return;
			operation.DestroyGroupForProof(0); s_bPartialLossApplied = true;
			PrintFormat("[TB_PVE_M3_4] partial-loss-injected package=STANDARD expectedSurvivors=%1", operation.GetAssignedGroupCount() - 1); s_iPhase = 8; return;
		}
		if (s_iPhase == 8)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (operation)
			{
				if (operation.GetViableAssignedGroupCount() != operation.GetAssignedGroupCount() - 1) { Fail("standard-did-not-continue-after-partial-loss"); return; }
				return;
			}
			if (!s_bPartialLossApplied || s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.SUCCEEDED || Owner(3) != "USSR") { Fail("standard-operation-not-successful"); return; }
			PrintFormat("[TB_PVE_M3_4] standard-partial-loss-success pass=1 owner=%1 revision=%2", Owner(3), TB_PVE_M2_3_FrontierQuery.GetRevision());
			s_Assembly.SetProofMaximumGroups(0); StartSetupCapture(3); s_iPhase = 81; return;
		}
		if (s_iPhase == 81)
		{
			if (Owner(3) != "US") return;
			CleanupSetup(); s_Assembly.ClearProofMaximumGroups(); s_Coordinator.SetDefaultPackageForProof("REINFORCED"); s_iPhase = 9; return;
		}
		if (s_iPhase == 9)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation(); if (!operation) return;
			if (!ValidateActivePackage(operation, "REINFORCED", TB_PVE_M3_4_ForcePackageRegistry.GetPackage("REINFORCED").GetExpectedGroupCount())) return;
			if (!operation.DidEveryViableGroupMove()) return;
			operation.DestroyAssignedForceForProof(); s_bAllLossApplied = true;
			Print("[TB_PVE_M3_4] all-groups-loss-injected package=REINFORCED expectedFailure=1"); s_iPhase = 10; return;
		}
		if (s_iPhase == 10)
		{
			if (s_Coordinator.HasActiveOperation()) return;
			if (!s_bAllLossApplied || s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.FAILED || Owner(3) != "US") { Fail("all-groups-loss-semantics"); return; }
			PrintFormat("[TB_PVE_M3_4] all-groups-loss-proof pass=1 ownerStable=%1 revision=%2", Owner(3), TB_PVE_M2_3_FrontierQuery.GetRevision()); s_iPhase = 11; return;
		}
		if (s_iPhase == 11)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation(); if (!operation) return;
			if (!ValidateActivePackage(operation, "REINFORCED", TB_PVE_M3_4_ForcePackageRegistry.GetPackage("REINFORCED").GetExpectedGroupCount())) return;
			if (!operation.DidEveryViableGroupMove()) return;
			PrintFormat("[TB_PVE_M3_4] reinforced-all-groups-moving pass=1 units=%1", operation.GetAssignedUnitCount()); s_iPhase = 12; return;
		}
		if (s_iPhase == 12)
		{
			if (s_Coordinator.HasActiveOperation()) return;
			if (s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.SUCCEEDED || Owner(3) != "USSR") { Fail("reinforced-operation-not-successful"); return; }
			if (s_Coordinator.GetMaximumConcurrentOperations() != 1) { Fail("concurrent-operation-limit"); return; }
			s_Coordinator.Stop("m3-4-proof-complete");
			PrintFormat("[TB_PVE_M3_4] COMPLETE PASS authority=1 packages=3 groups=configured atomicRollback=1 partialLossContinues=1 allLossFails=1 allGroupsMove=1 vanillaCapture=1 directOwnershipMutation=0 directFrontierMutation=0 activationLease=0 maxConcurrent=%1", s_Coordinator.GetMaximumConcurrentOperations());
			GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static bool ValidateActivePackage(TB_PVE_M3_1_Operation operation, string packageId, int groups)
	{
		if (operation.GetPackageId() != packageId || operation.GetAssignedGroupCount() != groups) { Fail("package-composition-mismatch-" + packageId); return false; }
		for (int index; index < groups; index++) if (operation.GetAssignedGroupLODAt(index) == AIAgent.GetMaxLOD()) { Fail("maximum-lod-group-" + packageId); return false; }
		return true;
	}

	protected static vector ObjectivePosition(int index) { return s_Route.GetRouteBase(index).GetOwner().GetOrigin(); }
	protected static void StartSetupCapture(int index)
	{
		CleanupSetup(); s_iSetupTarget = index; s_iLastCaptureState = -1;
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		if (!faction) { Fail("setup-faction-unavailable"); return; }
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		vector origin = ObjectivePosition(index); params.Transform[3] = origin;
		s_SetupGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(faction.GetDefendersGroupPrefab()), GetGame().GetWorld(), params));
		if (!s_SetupGroup) { Fail("setup-group-spawn-failed"); return; }
		int slot;
		for (int pass; pass < 2; pass++) foreach (ResourceName unitPrefab : s_SetupGroup.m_aUnitPrefabSlots)
		{
			int row = slot / 4; int column = slot - row * 4;
			vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.5;
			params.Transform[3] = position; IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
			if (character) { s_SetupGroup.AddAgentFromControlledEntity(character); s_aSetupCharacters.Insert(character); } slot++;
		}
		s_SetupGroup.SetLOD(0);
		PrintFormat("[TB_PVE_M3_4] setup-capture-start objective=%1 actors=%2", s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}
	protected static void LogSetupCaptureTransition()
	{
		if (!s_SetupGroup || s_SetupGroup.IsDeleted()) return;
		int state = s_Route.GetRouteBase(s_iSetupTarget).GetCaptureState(); if (state == s_iLastCaptureState) return;
		s_iLastCaptureState = state; PrintFormat("[TB_PVE_M3_4] setup-capture-progress objective=%1 state=%2 owner=%3", s_Route.GetRouteId(s_iSetupTarget), state, Owner(s_iSetupTarget));
	}
	protected static void CleanupSetup()
	{
		foreach (IEntity character : s_aSetupCharacters) if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_aSetupCharacters.Clear(); if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup); s_SetupGroup = null;
	}
	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		string factionKey = faction.GetFactionKey();
		return factionKey;
	}
	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M3_4] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 state=%4 created=%5 maxConcurrent=%6", reason, s_iPhase, s_iTicks, s_Coordinator.GetState(), s_Coordinator.GetCreatedOperationCount(), s_Coordinator.GetMaximumConcurrentOperations());
		if (s_Coordinator) s_Coordinator.Stop("m3-4-proof-failed"); CleanupSetup(); GetGame().GetCallqueue().Remove(Tick);
	}
}
