// Bounded dedicated proof. It may establish ownership and destroy a proof
// force, but it never supplies a target, creates an operation or issues orders.
class TB_PVE_M3_2_OPFORCoordinatorProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static TB_PVE_M3_2_OPFOROperationCoordinator s_Coordinator;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iSetupTarget;
	protected static int s_iLastCaptureState = -1;
	protected static bool s_bActionIssued;
	protected static string s_sFirstSuccessfulTarget;
	protected static int s_iFirstSuccessRevision;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, TB_PVE_M3_2_OPFOROperationCoordinator coordinator)
	{
		if (!Replication.IsServer() || !route || !coordinator) return;
		s_Route = route; s_Coordinator = coordinator;
		if (s_Coordinator.GetSelectionPolicy()) s_Coordinator.GetSelectionPolicy().SetProofHumanCount(1);
		s_iPhase = 0; s_iTicks = 0;
		PrintFormat("[TB_PVE_M3_2] proof-start revision=%1 outcome=%2 legalTarget=%3 dedicated=%4", TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), RplSession.Mode() == RplMode.Dedicated);
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_iTicks++;
		LogSetupCaptureTransition();
		if (s_iTicks > 500) { Fail("proof-timeout"); return; }

		if (s_iPhase == 0)
		{
			if (s_iTicks < 2) return;
			bool noTargetSafe = TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty() && !s_Coordinator.HasActiveOperation() && s_Coordinator.GetState() == TB_PVE_M3_2_CoordinatorState.IDLE;
			PrintFormat("[TB_PVE_M3_2] no-target-proof pass=%1 state=%2 active=%3 arbitraryAttack=0", noTargetSafe, s_Coordinator.GetState(), s_Coordinator.HasActiveOperation());
			if (!noTargetSafe) { Fail("initial-no-target-not-safe"); return; }
			StartSetupCapture(1, "US", "establish-frontier-depth-1");
			s_iPhase = 1; return;
		}

		if (s_iPhase == 1)
		{
			if (Owner(1) != "US") return;
			CleanupSetup();
			StartSetupCapture(2, "US", "establish-frontier-depth-2");
			s_iPhase = 2; return;
		}

		if (s_iPhase == 2)
		{
			if (Owner(2) != "US") return;
			CleanupSetup();
			if (TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_Route.GetRouteId(2)) { Fail("depth-2-target-mismatch"); return; }
			PrintFormat("[TB_PVE_M3_2] topology-ready revision=%1 OPFORLegal=%2 coordinatorState=%3", TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_Coordinator.GetState());
			s_iPhase = 3; s_iTicks = 0; return;
		}

		if (s_iPhase == 3)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation)
			{
				if (s_bActionIssued && s_Coordinator.GetLastTerminalState() == TB_PVE_M3_1_OperationState.FAILED)
				{
					PrintFormat("[TB_PVE_M3_2] PASS failure-recovery coordinatorState=%1 referenceCleared=1 maximumConcurrent=%2", s_Coordinator.GetState(), s_Coordinator.GetMaximumConcurrentOperations());
					s_iPhase = 4; s_iTicks = 0; s_bActionIssued = false;
				}
				return;
			}
			if (s_Coordinator.GetCreatedOperationCount() != 1) { Fail("unexpected-first-operation-count"); return; }
			if (!s_bActionIssued && operation.HasMeaningfulMovement())
			{
				PrintFormat("[TB_PVE_M3_2] failure-injection id=%1 target=%2 revision=%3 LOD=%4 units=%5", operation.GetOperationId(), operation.GetTargetObjectiveId(), operation.GetAuthorizedRevision(), operation.GetAssignedGroupLOD(), operation.GetAssignedUnitCount());
				operation.DestroyAssignedForceForProof(); s_bActionIssued = true;
			}
			if (!s_bActionIssued) return;
			if (operation.IsTerminal()) return;
			if (s_iTicks > 120) Fail("failure-operation-timeout");
			return;
		}

		if (s_iPhase == 4)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation)
			{
				if (s_bActionIssued && Owner(3) == "US" && s_Coordinator.GetLastTerminalState() == TB_PVE_M3_1_OperationState.CANCELLED)
				{
					CleanupSetup();
					if (TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_Route.GetRouteId(3)) { Fail("new-target-after-cancellation-mismatch"); return; }
					PrintFormat("[TB_PVE_M3_2] PASS cancellation-recovery newTarget=%1 revision=%2 operationRetargeted=0", TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetRevision());
					s_iPhase = 5; s_iTicks = 0; s_bActionIssued = false;
				}
				return;
			}
			if (s_Coordinator.GetCreatedOperationCount() < 2) return;
			if (!s_bActionIssued)
			{
				PrintFormat("[TB_PVE_M3_2] cancellation-operation id=%1 target=%2 revision=%3 freshGroupUnits=%4", operation.GetOperationId(), operation.GetTargetObjectiveId(), operation.GetAuthorizedRevision(), operation.GetAssignedUnitCount());
				StartSetupCapture(3, "US", "legitimate-target-change-for-cancellation");
				s_bActionIssued = true;
			}
			if (Owner(3) != "US") return;
			return;
		}

		if (s_iPhase == 5)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation)
			{
				if (s_bActionIssued && s_Coordinator.GetLastTerminalState() == TB_PVE_M3_1_OperationState.SUCCEEDED)
				{
					PrintFormat("[TB_PVE_M3_2] PASS first-success target=%1 revisionBefore=%2 revisionAfter=%3 owner=%4", s_sFirstSuccessfulTarget, s_iFirstSuccessRevision, TB_PVE_M2_3_FrontierQuery.GetRevision(), Owner(3));
					s_iPhase = 6; s_iTicks = 0; s_bActionIssued = false;
				}
				return;
			}
			if (s_Coordinator.GetCreatedOperationCount() < 3) return;
			if (!s_bActionIssued)
			{
				if (operation.GetTargetObjectiveId() != s_Route.GetRouteId(3)) { Fail("fresh-post-cancel-target-mismatch"); return; }
				s_sFirstSuccessfulTarget = operation.GetTargetObjectiveId();
				s_iFirstSuccessRevision = operation.GetAuthorizedRevision();
				PrintFormat("[TB_PVE_M3_2] success-operation-start id=%1 target=%2 revision=%3 LOD=%4", operation.GetOperationId(), s_sFirstSuccessfulTarget, s_iFirstSuccessRevision, operation.GetAssignedGroupLOD());
				s_bActionIssued = true;
			}
			return;
		}

		if (s_iPhase == 6)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (!operation || s_Coordinator.GetCreatedOperationCount() < 4) return;
			if (operation.GetTargetObjectiveId() == s_sFirstSuccessfulTarget || operation.GetAuthorizedRevision() <= s_iFirstSuccessRevision) { Fail("second-operation-stale-state"); return; }
			if (!operation.HasMeaningfulMovement()) return;
			PrintFormat("[TB_PVE_M3_2] PASS second-operation id=%1 oldTarget=%2 newTarget=%3 oldRevision=%4 newRevision=%5 units=%6 LOD=%7 movement=1", operation.GetOperationId(), s_sFirstSuccessfulTarget, operation.GetTargetObjectiveId(), s_iFirstSuccessRevision, operation.GetAuthorizedRevision(), operation.GetAssignedUnitCount(), operation.GetAssignedGroupLOD());
			s_Route.M2_2_SetDiagnosticInvalid(true);
			s_iPhase = 7; s_iTicks = 0; return;
		}

		if (s_iPhase == 7)
		{
			if (s_Coordinator.GetState() != TB_PVE_M3_2_CoordinatorState.SUSPENDED || s_Coordinator.HasActiveOperation()) return;
			if (s_iTicks < 3) return;
			bool pass = s_Coordinator.GetMaximumConcurrentOperations() == 1 && s_Coordinator.GetCreatedOperationCount() == 4 && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty();
			PrintFormat("[TB_PVE_M3_2] suspension-proof pass=%1 state=%2 active=0 validity=%3 outcome=%4 noNewOperation=1", pass, s_Coordinator.GetState(), TB_PVE_M2_3_FrontierQuery.GetCampaignValidity(), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome());
			if (!pass) { Fail("final-coordinator-invariants"); return; }
			s_Coordinator.Stop("proof-complete");
			s_Route.M2_2_SetDiagnosticInvalid(false);
			s_Route.M2_2_ReconstructProjection();
			Print("[TB_PVE_M3_2] COMPLETE PASS coordinatorAuthority=1 publicM2Only=1 maxConcurrent=1 delay=1 cooldown=1 failureRecovery=1 cancellationRecovery=1 autonomousSuccess=1 secondFreshOperation=1 suspendedInvalid=1 noTargetSafe=1 FIAExcluded=1 directOwnership=0 directFrontier=0");
			CleanupSetup();
			GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static void StartSetupCapture(int index, FactionKey factionKey, string label)
	{
		CleanupSetup(); s_iSetupTarget = index; s_iLastCaptureState = -1;
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		if (!faction) { Fail("setup-faction-unavailable"); return; }
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		vector origin = s_Route.GetRouteBase(index).GetOwner().GetOrigin(); params.Transform[3] = origin;
		s_SetupGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(faction.GetDefendersGroupPrefab()), GetGame().GetWorld(), params));
		if (!s_SetupGroup) { Fail("setup-group-spawn-failed"); return; }
		int slot;
		for (int pass; pass < 2; pass++)
		{
			foreach (ResourceName unitPrefab : s_SetupGroup.m_aUnitPrefabSlots)
			{
				int row = slot / 4; int column = slot - (row * 4);
				vector position = origin + Vector(column * 2, 0, row * 2);
				position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.5;
				params.Transform[3] = position;
				IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
				if (character) { s_SetupGroup.AddAgentFromControlledEntity(character); s_aSetupCharacters.Insert(character); }
				slot++;
			}
		}
		s_SetupGroup.SetLOD(0);
		PrintFormat("[TB_PVE_M3_2] proof-topology-capture-start label=%1 faction=%2 objective=%3 actors=%4", label, factionKey, s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}

	protected static void LogSetupCaptureTransition()
	{
		if (!s_SetupGroup || s_SetupGroup.IsDeleted()) return;
		int state = s_Route.GetRouteBase(s_iSetupTarget).GetCaptureState();
		if (state == s_iLastCaptureState) return;
		s_iLastCaptureState = state;
		PrintFormat("[TB_PVE_M3_2] proof-topology-capture-progress objective=%1 state=%2 owner=%3", s_Route.GetRouteId(s_iSetupTarget), state, Owner(s_iSetupTarget));
	}

	protected static void CleanupSetup()
	{
		foreach (IEntity character : s_aSetupCharacters)
			if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_aSetupCharacters.Clear();
		if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup);
		s_SetupGroup = null;
	}

	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		return faction.GetFactionKey();
	}

	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M3_2] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 coordinatorState=%4 created=%5 maxConcurrent=%6", reason, s_iPhase, s_iTicks, s_Coordinator.GetState(), s_Coordinator.GetCreatedOperationCount(), s_Coordinator.GetMaximumConcurrentOperations());
		if (s_Coordinator) s_Coordinator.Stop("proof-failed");
		CleanupSetup();
		GetGame().GetCallqueue().Remove(Tick);
	}
}
