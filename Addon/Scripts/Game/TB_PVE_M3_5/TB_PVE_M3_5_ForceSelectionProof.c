class TB_PVE_M3_5_ForceSelectionProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static TB_PVE_M3_2_OPFOROperationCoordinator s_Coordinator;
	protected static TB_PVE_M3_5_ForceSelectionPolicy s_Selector;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iSetupTarget;
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iCreatedBeforeZero;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, TB_PVE_M3_2_OPFOROperationCoordinator coordinator, TB_PVE_M3_5_ForceSelectionPolicy selector)
	{
		if (!Replication.IsServer() || !route || !coordinator || !selector) return;
		s_Route = route; s_Coordinator = coordinator; s_Selector = selector;
		s_Coordinator.SetDelaysForProof(2, 2); s_Selector.SetProofHumanCount(0);
		if (!RunSelectionMatrix()) { Fail("selection-matrix"); return; }
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(1));
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(2));
		s_Route.GetM3_3StagingPolicy().RequestNavigationForTarget(ObjectivePosition(3));
		PrintFormat("[TB_PVE_M3_5] proof-start dedicated=%1 revision=%2", RplSession.Mode() == RplMode.Dedicated, TB_PVE_M2_3_FrontierQuery.GetRevision());
		GetGame().GetCallqueue().CallLater(Begin, 10000, false);
	}

	protected static bool RunSelectionMatrix()
	{
		TB_PVE_M3_5_SelectionDecision zero = s_Selector.SelectFromContextForProof(0, 100, -1);
		TB_PVE_M3_5_SelectionDecision early = s_Selector.SelectFromContextForProof(1, 100, -1);
		TB_PVE_M3_5_SelectionDecision mid = s_Selector.SelectFromContextForProof(1, 500, -1);
		TB_PVE_M3_5_SelectionDecision late = s_Selector.SelectFromContextForProof(1, 800, -1);
		TB_PVE_M3_5_SelectionDecision failed = s_Selector.SelectFromContextForProof(1, 100, TB_PVE_M3_1_OperationState.FAILED);
		TB_PVE_M3_5_SelectionDecision failedClamped = s_Selector.SelectFromContextForProof(4, 800, TB_PVE_M3_1_OperationState.FAILED);
		TB_PVE_M3_5_SelectionDecision cancelled = s_Selector.SelectFromContextForProof(1, 100, TB_PVE_M3_1_OperationState.CANCELLED);
		TB_PVE_M3_5_SelectionDecision succeeded = s_Selector.SelectFromContextForProof(1, 100, TB_PVE_M3_1_OperationState.SUCCEEDED);
		TB_PVE_M3_5_SelectionDecision two = s_Selector.SelectFromContextForProof(2, 100, -1);
		TB_PVE_M3_5_SelectionDecision five = s_Selector.SelectFromContextForProof(5, 100, -1);
		TB_PVE_M3_5_SelectionDecision eight = s_Selector.SelectFromContextForProof(8, 100, -1);
		TB_PVE_M3_5_SelectionDecision twelve = s_Selector.SelectFromContextForProof(12, 100, -1);
		TB_PVE_M3_5_SelectionDecision sixteen = s_Selector.SelectFromContextForProof(16, 100, -1);
		TB_PVE_M3_5_ForceSelectionPolicy invalid = new TB_PVE_M3_5_ForceSelectionPolicy(); invalid.SetInvalidConfigurationForProof();
		TB_PVE_M3_5_SelectionDecision invalidResult = invalid.SelectFromContextForProof(1, 100, -1);
		bool pass = !zero.m_bSelected && early.m_sPackageId == "LIGHT" && two.m_sPackageId == "STANDARD" && mid.m_sPackageId == "STANDARD" && late.m_sPackageId == "REINFORCED" && failed.m_sPackageId == "STANDARD" && failedClamped.m_sPackageId == "REINFORCED" && cancelled.m_sPackageId == "LIGHT" && succeeded.m_sPackageId == "LIGHT" && five.m_sPackageId == "ASSAULT" && eight.m_sPackageId == "HEAVY" && twelve.m_sPackageId == "SURGE" && sixteen.m_sPackageId == "SURGE" && !invalidResult.m_bSelected;
		PrintFormat("[TB_PVE_M3_5] matrix pass=%1 zero=%2 early=%3 mid=%4 late=%5 failed=%6 failedClamp=%7 cancelled=%8 succeeded=%9", pass, zero.m_sReason, early.m_sPackageId, mid.m_sPackageId, late.m_sPackageId, failed.m_sPackageId, failedClamped.m_sPackageId, cancelled.m_sPackageId, succeeded.m_sPackageId);
		PrintFormat("[TB_PVE_M3_5] expanded-matrix two=%1 five=%2 eight=%3 twelve=%4 sixteen=%5 groups=configured", two.m_sPackageId, five.m_sPackageId, eight.m_sPackageId, twelve.m_sPackageId, sixteen.m_sPackageId);
		PrintFormat("[TB_PVE_M3_5] invalid-config-proof pass=%1 reason=%2", !invalidResult.m_bSelected, invalidResult.m_sReason);
		return pass;
	}

	protected static void Begin()
	{
		StartSetupCapture(1); s_iPhase = 1; GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}
	protected static void Tick()
	{
		s_iTicks++; if (s_iTicks > 700) { Fail("timeout"); return; }
		if (s_iPhase == 1) { if (Owner(1) != "US") return; CleanupSetup(); StartSetupCapture(2); s_iPhase = 2; return; }
		if (s_iPhase == 2) { if (Owner(2) != "US") return; CleanupSetup(); StartSetupCapture(3); s_iPhase = 3; return; }
		if (s_iPhase == 3)
		{
			if (Owner(3) != "US") return; CleanupSetup();
			if (TB_PVE_M2_3_FrontierQuery.GetCampaignDepthPermille() > 334) { Fail("actual-early-depth"); return; }
			s_Selector.SetProofHumanCount(1); PrintFormat("[TB_PVE_M3_5] actual-depth band=EARLY permille=%1 pass=1", TB_PVE_M2_3_FrontierQuery.GetCampaignDepthPermille()); s_iPhase = 4; return;
		}
		if (s_iPhase == 4)
		{
			TB_PVE_M3_1_Operation op = s_Coordinator.GetActiveOperation(); if (!op || op.GetPackageId() != "LIGHT") return;
			if (!op.HasMeaningfulMovement()) return;
			s_Selector.SetProofHumanCount(4);
			if (op.GetPackageId() != "LIGHT" || op.GetSelectionHumanCount() != 1) { Fail("active-package-mutated"); return; }
			Print("[TB_PVE_M3_5] active-snapshot-proof pass=1 createdHumans=1 currentHumans=4 package=LIGHT unchanged=1");
			s_Selector.SetProofHumanCount(1); op.DestroyAssignedForceForProof(); s_iPhase = 5; return;
		}
		if (s_iPhase == 5)
		{
			TB_PVE_M3_1_Operation op = s_Coordinator.GetActiveOperation();
			if (!op) return;
			if (op.GetPackageId() == "LIGHT") return;
			if (op.GetPackageId() != "STANDARD" || op.GetAssignedGroupCount() != TB_PVE_M3_4_ForcePackageRegistry.GetPackage("STANDARD").GetExpectedGroupCount()) { Fail("failed-modifier-not-standard"); return; }
			if (!op.DidEveryViableGroupMove()) return;
			Print("[TB_PVE_M3_5] failed-result-e2e pass=1 previous=FAILED package=STANDARD groups=configured moved=1");
			s_Route.M2_2_SetDiagnosticInvalid(true); s_iPhase = 6; return;
		}
		if (s_iPhase == 6)
		{
			if (s_Coordinator.HasActiveOperation() || s_Coordinator.GetState() != TB_PVE_M3_2_CoordinatorState.SUSPENDED) return;
			s_Route.M2_2_SetDiagnosticInvalid(false); s_Route.M2_2_ReconstructProjection(); s_Selector.SetProofHumanCount(4); s_iPhase = 7; return;
		}
		if (s_iPhase == 7)
		{
			TB_PVE_M3_1_Operation op = s_Coordinator.GetActiveOperation(); if (!op) return;
			if (op.GetPackageId() != "REINFORCED" || op.GetAssignedGroupCount() != TB_PVE_M3_4_ForcePackageRegistry.GetPackage("REINFORCED").GetExpectedGroupCount()) { Fail("cancel-neutral-high-not-reinforced"); return; }
			if (!op.DidEveryViableGroupMove()) return;
			Print("[TB_PVE_M3_5] cancelled-neutral-e2e pass=1 previous=CANCELLED humans=4 package=REINFORCED groups=configured moved=1");
			s_Route.M2_2_SetDiagnosticInvalid(true); s_iPhase = 8; return;
		}
		if (s_iPhase == 8)
		{
			if (s_Coordinator.HasActiveOperation() || s_Coordinator.GetState() != TB_PVE_M3_2_CoordinatorState.SUSPENDED) return;
			s_Route.M2_2_SetDiagnosticInvalid(false); s_Route.M2_2_ReconstructProjection(); s_Selector.SetProofHumanCount(0); s_iCreatedBeforeZero = s_Coordinator.GetCreatedOperationCount(); s_iTicks = 0; s_iPhase = 9; return;
		}
		if (s_iPhase == 9)
		{
			if (s_iTicks < 4) return;
			if (s_Coordinator.HasActiveOperation() || s_Coordinator.GetCreatedOperationCount() != s_iCreatedBeforeZero) { Fail("zero-player-created-operation"); return; }
			PrintFormat("[TB_PVE_M3_5] zero-player-e2e pass=1 createdStable=%1 frontier=%2", s_iCreatedBeforeZero, s_Route.GetFrontierIndex());
			s_Coordinator.Stop("m3-5-depth-band-proof"); s_Selector.SetProofHumanCount(1); StartSetupCapture(4); s_iPhase = 10; return;
		}
		if (s_iPhase == 10)
		{
			if (Owner(4) != "US") return; CleanupSetup(); TB_PVE_M3_5_SelectionDecision mid = s_Selector.SelectAttackPackage(TB_PVE_M3_1_OperationState.CANCELLED);
			if (mid.m_iDepthBand != TB_PVE_M3_5_DepthBand.MID || mid.m_sPackageId != "STANDARD") { Fail("actual-mid-depth"); return; }
			PrintFormat("[TB_PVE_M3_5] actual-depth band=MID permille=%1 package=%2 pass=1", mid.m_iDepthPermille, mid.m_sPackageId); StartSetupCapture(5); s_iPhase = 11; return;
		}
		if (s_iPhase == 11) { if (Owner(5) != "US") return; CleanupSetup(); StartSetupCapture(6); s_iPhase = 12; return; }
		if (s_iPhase == 12) { if (Owner(6) != "US") return; CleanupSetup(); StartSetupCapture(7); s_iPhase = 13; return; }
		if (s_iPhase == 13)
		{
			if (Owner(7) != "US") return; CleanupSetup(); TB_PVE_M3_5_SelectionDecision late = s_Selector.SelectAttackPackage(TB_PVE_M3_1_OperationState.SUCCEEDED);
			if (late.m_iDepthBand != TB_PVE_M3_5_DepthBand.LATE || late.m_sPackageId != "REINFORCED") { Fail("actual-late-depth"); return; }
			PrintFormat("[TB_PVE_M3_5] actual-depth band=LATE permille=%1 package=%2 pass=1", late.m_iDepthPermille, late.m_sPackageId);
			PrintFormat("[TB_PVE_M3_5] COMPLETE PASS authority=1 deterministic=1 zeroPlayers=1 activeSnapshot=1 nextOperationReevaluated=1 earlyMidLate=1 failedModifier=1 cancelledNeutral=1 succeededNeutral=1 lightE2E=1 standardE2E=1 reinforcedE2E=1 invalidSafe=1 maxConcurrent=%1", s_Coordinator.GetMaximumConcurrentOperations()); GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static vector ObjectivePosition(int index) { return s_Route.GetRouteBase(index).GetOwner().GetOrigin(); }
	protected static void StartSetupCapture(int index)
	{
		CleanupSetup(); s_iSetupTarget = index; SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US")); if (!faction) { Fail("setup-faction"); return; }
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; vector origin = ObjectivePosition(index); params.Transform[3] = origin;
		s_SetupGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(faction.GetDefendersGroupPrefab()), GetGame().GetWorld(), params)); if (!s_SetupGroup) { Fail("setup-group"); return; }
		int slot; for (int pass; pass < 2; pass++) foreach (ResourceName unitPrefab : s_SetupGroup.m_aUnitPrefabSlots)
		{
			int row = slot / 4; int column = slot - row * 4; vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.5;
			params.Transform[3] = position; IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params); if (character) { s_SetupGroup.AddAgentFromControlledEntity(character); s_aSetupCharacters.Insert(character); } slot++;
		}
		s_SetupGroup.SetLOD(0); PrintFormat("[TB_PVE_M3_5] setup-capture objective=%1 actors=%2", s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}
	protected static void CleanupSetup()
	{
		while (s_aSetupCharacters.Count() > 0)
		{
			int last = s_aSetupCharacters.Count() - 1; IEntity character = s_aSetupCharacters.Get(last); s_aSetupCharacters.Remove(last);
			if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup); s_SetupGroup = null;
	}
	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		string factionKey = faction.GetFactionKey();
		return factionKey;
	}
	protected static void Fail(string reason) { int created; if (s_Coordinator) created = s_Coordinator.GetCreatedOperationCount(); PrintFormat("[TB_PVE_M3_5] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 created=%4", reason, s_iPhase, s_iTicks, created); if (s_Coordinator) s_Coordinator.Stop("m3-5-proof-failed"); CleanupSetup(); GetGame().GetCallqueue().Remove(Tick); }
}
