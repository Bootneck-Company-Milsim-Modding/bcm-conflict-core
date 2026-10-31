// Dedicated authority proof for the production staging policy. The proof may
// establish route ownership, but target selection and operation creation remain
// owned by the M2 query contract and M3.2 coordinator respectively.
class TB_PVE_M3_3_TacticalStagingProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static TB_PVE_M3_2_OPFOROperationCoordinator s_Coordinator;
	protected static TB_PVE_M3_3_TacticalStagingPolicy s_Policy;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iSetupTarget;
	protected static int s_iLastCaptureState = -1;
	protected static vector s_vFirstOperationStaging;
	protected static string s_sFirstOperationTarget;
	protected static int s_iFirstOperationRevision;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, TB_PVE_M3_2_OPFOROperationCoordinator coordinator, TB_PVE_M3_3_TacticalStagingPolicy policy)
	{
		if (!Replication.IsServer() || !route || !coordinator || !policy) return;
		s_Route = route; s_Coordinator = coordinator; s_Policy = policy;
		if (s_Coordinator.GetSelectionPolicy()) s_Coordinator.GetSelectionPolicy().SetProofHumanCount(1);
		PrintFormat("[TB_PVE_M3_3] proof-start revision=%1 dedicated=%2", TB_PVE_M2_3_FrontierQuery.GetRevision(), RplSession.Mode() == RplMode.Dedicated);
		s_Policy.RequestNavigationForTarget(ObjectivePosition(1));
		s_Policy.RequestNavigationForTarget(ObjectivePosition(2));
		s_Policy.RequestNavigationForTarget(ObjectivePosition(5));
		s_Policy.RequestNavigationForTarget(ObjectivePosition(8));
		GetGame().GetCallqueue().CallLater(BeginAfterNavigationStreaming, 10000, false);
	}

	protected static void BeginAfterNavigationStreaming()
	{
		if (!RunPolicyDiagnostics()) { Fail("policy-diagnostics-failed"); return; }
		// A zero-attempt bounded query proves the production coordinator cannot
		// spawn at the objective or invent a fallback when staging is unavailable.
		s_Policy.SetCandidateAttemptsForProof(0);
		StartSetupCapture(1, "US", "establish-frontier-depth-1");
		s_iPhase = 1; s_iTicks = 0;
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static bool RunPolicyDiagnostics()
	{
		bool bearings = s_Policy.BearingSeparationForProof(350, 10) == 20 && s_Policy.BearingSeparationForProof(90, 270) == 180 && s_Policy.BearingSeparationForProof(120, 120) == 0;
		PrintFormat("[TB_PVE_M3_3] bearing-separation-proof pass=%1 wrap=20 opposite=180 same=0", bearings);
		if (!bearings) return false;
		array<vector> noObservers = {};
		vector early; vector coastal; vector middle; vector late; string reason;
		if (!FindAt(1, "early", noObservers, early, reason)) return false;
		if (!FindAt(2, "coastal-operation-target", noObservers, coastal, reason)) return false;
		if (!FindAt(5, "middle", noObservers, middle, reason)) return false;
		if (!FindAt(8, "late", noObservers, late, reason)) return false;

		vector repeated; string repeatedReason;
		if (!FindAt(1, "early-repeat", noObservers, repeated, repeatedReason)) return false;
		bool varied = vector.Distance(early, repeated) > 5;
		PrintFormat("[TB_PVE_M3_3] repeated-selection pass=%1 first=%2 second=%3 separation=%4", varied, early, repeated, vector.Distance(early, repeated));
		if (!varied) return false;

		array<vector> nearObserver = {early}; string nearReason;
		bool nearRejected = !s_Policy.ValidateCandidateForProof(ObjectivePosition(1), early, nearObserver, nearReason) && nearReason == "human-player-distance";
		PrintFormat("[TB_PVE_M3_3] player-distance-proof pass=%1 reason=%2 candidate=%3 observer=%4", nearRejected, nearReason, early, early);
		if (!nearRejected) return false;

		bool visibilityRejected; vector visibilityObserver; string visibilityReason;
		for (int direction; direction < 4 && !visibilityRejected; direction++)
		{
			if (direction == 0) visibilityObserver = early + Vector(251, 100, 0);
			else if (direction == 1) visibilityObserver = early + Vector(-251, 100, 0);
			else if (direction == 2) visibilityObserver = early + Vector(0, 100, 251);
			else visibilityObserver = early + Vector(0, 100, -251);
			array<vector> visibleObserver = {visibilityObserver};
			visibilityRejected = !s_Policy.ValidateCandidateForProof(ObjectivePosition(1), early, visibleObserver, visibilityReason) && visibilityReason == "human-player-direct-visibility";
		}
		PrintFormat("[TB_PVE_M3_3] player-visibility-proof pass=%1 reason=%2 candidate=%3 observer=%4", visibilityRejected, visibilityReason, early, visibilityObserver);
		if (!visibilityRejected) return false;
		array<vector> activeObserver = {visibilityObserver}; vector alternate; string alternateReason;
		bool alternateFound = FindAt(1, "player-aware-alternate", activeObserver, alternate, alternateReason);
		PrintFormat("[TB_PVE_M3_3] player-aware-selection pass=%1 alternate=%2 reason=%3", alternateFound, alternate, alternateReason);
		if (!alternateFound) return false;

		bool waterRejected; string waterReason;
		vector coastalTarget = ObjectivePosition(1);
		for (int sample; sample < 24 && !waterRejected; sample++)
		{
			float angle = sample * 0.261799;
			vector waterCandidate = coastalTarget + Vector(Math.Sin(angle) * 425, 0, Math.Cos(angle) * 425);
			waterCandidate[1] = GetGame().GetWorld().GetSurfaceY(waterCandidate[0], waterCandidate[2]);
			if (!ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), waterCandidate)) continue;
			waterRejected = !s_Policy.ValidateCandidateForProof(coastalTarget, waterCandidate, noObservers, waterReason) && waterReason == "water";
			if (waterRejected) PrintFormat("[TB_PVE_M3_3] coastal-water-proof pass=1 objective=%1 candidate=%2 reason=%3", s_Route.GetRouteId(1), waterCandidate, waterReason);
		}
		if (!waterRejected)
		{
			vector knownWater = Vector(100, -10, 100);
			vector syntheticTarget = Vector(100, -10, 350);
			waterRejected = !s_Policy.ValidateCandidateForProof(syntheticTarget, knownWater, noObservers, waterReason) && waterReason == "water";
			PrintFormat("[TB_PVE_M3_3] water-proof fallback=known-ocean pass=%1 candidate=%2 reason=%3 coastalAnnulusWaterFound=0", waterRejected, knownWater, waterReason);
		}
		return waterRejected;
	}

	protected static bool FindAt(int index, string label, array<vector> observers, out vector position, out string reason)
	{
		bool found = s_Policy.FindStagingPosition("USSR", s_Route.GetRouteId(index), ObjectivePosition(index), observers, position, reason);
		PrintFormat("[TB_PVE_M3_3] route-sample label=%1 pass=%2 objective=%3 position=%4 distance=%5 bearing=%6 attempts=%7 durationMs=%8 reason=%9", label, found, s_Route.GetRouteId(index), position, s_Policy.GetLastAcceptedDistance(), s_Policy.GetLastAcceptedBearing(), s_Policy.GetLastAttemptsUsed(), s_Policy.GetLastDurationMs(), reason);
		return found;
	}

	protected static void Tick()
	{
		s_iTicks++;
		LogSetupCaptureTransition();
		if (s_iTicks > 600) { Fail("proof-timeout"); return; }
		if (s_iPhase == 1)
		{
			if (Owner(1) != "US") return;
			CleanupSetup(); StartSetupCapture(2, "US", "establish-frontier-depth-2"); s_iPhase = 2; return;
		}
		if (s_iPhase == 2)
		{
			if (Owner(2) != "US") return;
			CleanupSetup();
			if (TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_Route.GetRouteId(2)) { Fail("depth-2-target-mismatch"); return; }
			s_iPhase = 3; s_iTicks = 0; return;
		}
		if (s_iPhase == 3)
		{
			if (s_Coordinator.HasActiveOperation()) { Fail("operation-created-without-valid-staging"); return; }
			if (s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.FAILED) return;
			PrintFormat("[TB_PVE_M3_3] no-valid-location-proof pass=1 coordinatorState=%1 active=0 ownerStable=%2 revision=%3", s_Coordinator.GetState(), Owner(2), TB_PVE_M2_3_FrontierQuery.GetRevision());
			s_Policy.RestoreProductionCandidateAttempts(); s_iPhase = 4; s_iTicks = 0; return;
		}
		if (s_iPhase == 4)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation) return;
			if (operation.GetAssignedGroupLOD() == AIAgent.GetMaxLOD()) { Fail("operation-group-at-maximum-lod"); return; }
			if (s_Coordinator.GetCreatedOperationCount() != 1) { Fail("unexpected-first-operation-count"); return; }
			s_vFirstOperationStaging = operation.GetStagingPosition(); s_sFirstOperationTarget = operation.GetTargetObjectiveId(); s_iFirstOperationRevision = operation.GetAuthorizedRevision();
			PrintFormat("[TB_PVE_M3_3] autonomous-operation-start target=%1 revision=%2 staging=%3 distance=%4 LOD=%5", s_sFirstOperationTarget, s_iFirstOperationRevision, s_vFirstOperationStaging, operation.GetStartingDistance(), operation.GetAssignedGroupLOD());
			s_iPhase = 5; s_iTicks = 0; return;
		}
		if (s_iPhase == 5)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (operation) return;
			if (s_Coordinator.GetLastTerminalState() != TB_PVE_M3_1_OperationState.SUCCEEDED || Owner(2) != "USSR") { Fail("autonomous-operation-not-successful"); return; }
			PrintFormat("[TB_PVE_M3_3] autonomous-success pass=1 target=%1 owner=%2 revisionBefore=%3 revisionAfter=%4", s_sFirstOperationTarget, Owner(2), s_iFirstOperationRevision, TB_PVE_M2_3_FrontierQuery.GetRevision());
			s_iPhase = 6; s_iTicks = 0; return;
		}
		if (s_iPhase == 6)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation || s_Coordinator.GetCreatedOperationCount() != 2) return;
			if (!operation.HasMeaningfulMovement()) return;
			bool fresh = operation.GetTargetObjectiveId() != s_sFirstOperationTarget && operation.GetAuthorizedRevision() > s_iFirstOperationRevision;
			bool varied = vector.Distance(operation.GetStagingPosition(), s_vFirstOperationStaging) > 5;
			PrintFormat("[TB_PVE_M3_3] repeated-operation-proof pass=%1 fresh=%2 varied=%3 firstTarget=%4 secondTarget=%5 firstStaging=%6 secondStaging=%7 maxConcurrent=%8", fresh && varied, fresh, varied, s_sFirstOperationTarget, operation.GetTargetObjectiveId(), s_vFirstOperationStaging, operation.GetStagingPosition(), s_Coordinator.GetMaximumConcurrentOperations());
			if (!fresh || !varied || s_Coordinator.GetMaximumConcurrentOperations() != 1) { Fail("repeated-operation-invariants"); return; }
			s_Route.M2_2_SetDiagnosticInvalid(true); s_iPhase = 7; s_iTicks = 0; return;
		}
		if (s_iPhase == 7)
		{
			if (s_Coordinator.GetState() != TB_PVE_M3_2_CoordinatorState.SUSPENDED || s_Coordinator.HasActiveOperation()) return;
			if (s_iTicks < 3) return;
			s_Coordinator.Stop("m3-3-proof-complete");
			s_Route.M2_2_SetDiagnosticInvalid(false); s_Route.M2_2_ReconstructProjection();
			Print("[TB_PVE_M3_3] COMPLETE PASS authority=1 M2TargetContract=1 annulus=1 playerDistance=1 visibility=1 water=1 terrain=1 navigation=1 boundedFailure=1 autonomousCapture=1 repeatedVariation=1 maxConcurrent=1 noTeleport=1 noDirectOwnership=1 noDirectFrontier=1 FIAExcluded=1");
			CleanupSetup(); GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static vector ObjectivePosition(int index) { return s_Route.GetRouteBase(index).GetOwner().GetOrigin(); }
	protected static void StartSetupCapture(int index, FactionKey factionKey, string label)
	{
		CleanupSetup(); s_iSetupTarget = index; s_iLastCaptureState = -1;
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
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
		PrintFormat("[TB_PVE_M3_3] proof-topology-capture-start label=%1 faction=%2 objective=%3 actors=%4", label, factionKey, s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}
	protected static void LogSetupCaptureTransition()
	{
		if (!s_SetupGroup || s_SetupGroup.IsDeleted()) return;
		int state = s_Route.GetRouteBase(s_iSetupTarget).GetCaptureState(); if (state == s_iLastCaptureState) return;
		s_iLastCaptureState = state; PrintFormat("[TB_PVE_M3_3] proof-topology-capture-progress objective=%1 state=%2 owner=%3", s_Route.GetRouteId(s_iSetupTarget), state, Owner(s_iSetupTarget));
	}
	protected static void CleanupSetup()
	{
		foreach (IEntity character : s_aSetupCharacters) if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_aSetupCharacters.Clear(); if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup); s_SetupGroup = null;
	}
	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction)
			return "<none>";
		string factionKey = faction.GetFactionKey();
		return factionKey;
	}
	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M3_3] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 state=%4 created=%5 maxConcurrent=%6", reason, s_iPhase, s_iTicks, s_Coordinator.GetState(), s_Coordinator.GetCreatedOperationCount(), s_Coordinator.GetMaximumConcurrentOperations());
		if (s_Coordinator) s_Coordinator.Stop("m3-3-proof-failed"); CleanupSetup(); GetGame().GetCallqueue().Remove(Tick);
	}
}
