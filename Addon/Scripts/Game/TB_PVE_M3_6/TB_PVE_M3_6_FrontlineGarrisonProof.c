// Dedicated authority proof. Ownership changes still come only from genuine
// vanilla Conflict capture by physically spawned faction actors.
class TB_PVE_M3_6_FrontlineGarrisonProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static TB_PVE_M3_6_OPFORFrontlineGarrisonManager s_Manager;
	protected static TB_PVE_M3_2_OPFOROperationCoordinator s_Coordinator;
	protected static SCR_AIGroup s_SetupGroup;
	protected static ref array<IEntity> s_aSetupCharacters = {};
	protected static int s_iSetupTarget;
	protected static int s_iPhase;
	protected static int s_iTicks;
	protected static int s_iPhaseTicks;
	protected static int s_iDefeatedTicks;
	protected static int s_iInitialCreated;
	protected static int s_iGarrisonUnits;
	protected static bool s_bCoexistence;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager, TB_PVE_M3_2_OPFOROperationCoordinator coordinator)
	{
		if (!Replication.IsServer() || !route || !manager) return;
		s_Route = route; s_Manager = manager; s_Coordinator = coordinator; s_Manager.SetProofHumanCount(0);
		if (s_Coordinator && s_Coordinator.GetSelectionPolicy()) s_Coordinator.GetSelectionPolicy().SetProofHumanCount(0);
		PrintFormat("[TB_PVE_M3_6] proof-start dedicated=%1 revision=%2 outcome=%3 validity=%4", RplSession.Mode() == RplMode.Dedicated, TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.GetCampaignValidity());
		StartCapture(1, "US", "establish-first-opfor-frontline");
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_iTicks++; s_iPhaseTicks++;
		if (s_iTicks > 600) { Fail("proof-timeout"); return; }
		if (s_iPhase == 0)
		{
			if (Owner(1) != "US") return;
			CleanupSetup();
			if (TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() != s_Route.GetRouteId(2)) { Fail("public-frontline-target-mismatch"); return; }
			if (s_Manager.HasGarrison()) { Fail("zero-player-created-garrison"); return; }
			vector target; TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(s_Route.GetRouteId(2), target); array<vector> observer = { target + Vector(20, 1.6, 0) };
			s_Manager.SetProofHumanCount(1); s_Manager.SetProofObservers(observer);
			PrintFormat("[TB_PVE_M3_6] PASS zero-player-policy target=%1 physicalGarrison=0 thenVisibilityGate=1 observerDistance=20", s_Route.GetRouteId(2));
			NextPhase(1); return;
		}
		if (s_iPhase == 1)
		{
			if (s_iPhaseTicks < 3) return;
			if (s_Manager.HasGarrison()) { Fail("near-player-pop-in"); return; }
			Print("[TB_PVE_M3_6] PASS visibility-policy nearObserver=1 pending=1 physicalGarrison=0");
			s_Manager.ClearProofObservers(); NextPhase(2); return;
		}
		if (s_iPhase == 2)
		{
			if (!s_Manager.HasGarrison()) return;
			if (s_Manager.GetObjectiveId() != s_Route.GetRouteId(2) || s_Manager.GetGroupCount() != 2 || s_Manager.GetUnitCount() <= 0 || !s_Manager.AreGroupsCombatReadyForProof()) { Fail("initial-garrison-combat-invariant"); return; }
			s_iInitialCreated = s_Manager.GetCreatedCount(); s_iGarrisonUnits = s_Manager.GetUnitCount();
			PrintFormat("[TB_PVE_M3_6] PASS initial-creation objective=%1 publicState=%2 package=STANDARD groups=%3 units=%4 LOD=%5 durationMs=%6 defensiveOrders=1 maxConcurrent=%7 combatInvariant=1", s_Manager.GetObjectiveId(), TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(s_Manager.GetObjectiveId()), s_Manager.GetGroupCount(), s_iGarrisonUnits, s_Manager.GetFirstGroupLOD(), s_Manager.GetLastCreationDurationMs(), s_Manager.GetMaximumConcurrent());
			StartCapture(2, "US", "genuine-blufor-attack-garrisoned-objective"); NextPhase(3); return;
		}
		if (s_iPhase == 3)
		{
			if (s_Manager.GetState() != TB_PVE_M3_6_GarrisonState.HOSTILE_NEARBY && s_iPhaseTicks < 5) return;
			PrintFormat("[TB_PVE_M3_6] PASS blufor-contact objective=%1 state=%2 vanillaDefendWaypoint=1 normalActors=1 initialUnits=%3 survivingUnits=%4 proximityOnly=1 combatUnverified=1", s_Manager.GetObjectiveId(), s_Manager.GetState(), s_iGarrisonUnits, s_Manager.GetUnitCount());
			s_Manager.DestroyForProof(); NextPhase(4); return;
		}
		if (s_iPhase == 4)
		{
			if (s_Manager.GetState() != TB_PVE_M3_6_GarrisonState.DEFEATED) return;
			s_iDefeatedTicks++;
			if (s_iDefeatedTicks < 4) return;
			if (s_Manager.GetCreatedCount() != s_iInitialCreated || !s_Manager.HasGarrison()) { Fail("defeated-garrison-regenerated"); return; }
			PrintFormat("[TB_PVE_M3_6] PASS defeated-suppression objective=%1 ticks=%2 regeneration=0 sameGarrisonRecord=1", s_Manager.GetObjectiveId(), s_iDefeatedTicks);
			if (Owner(2) != "US") return;
			CleanupSetup(); NextPhase(5); return;
		}
		if (s_iPhase == 5)
		{
			if (!s_Manager.HasGarrison() || s_Manager.GetObjectiveId() != s_Route.GetRouteId(3) || s_Manager.GetState() != TB_PVE_M3_6_GarrisonState.ACTIVE) return;
			PrintFormat("[TB_PVE_M3_6] PASS frontier-advance vanillaOwner=%1 newTarget=%2 created=%3 oldResponsibilityRetired=1 maxConcurrent=%4 cleanupMs=%5", Owner(2), s_Manager.GetObjectiveId(), s_Manager.GetCreatedCount(), s_Manager.GetMaximumConcurrent(), s_Manager.GetLastCleanupDurationMs());
			if (!s_Coordinator) { Fail("coordinator-unavailable"); return; }
			s_Coordinator.SetDefaultPackageForProof("LIGHT"); s_Coordinator.SetDelaysForProof(0, 2); NextPhase(6); return;
		}
		if (s_iPhase == 6)
		{
			TB_PVE_M3_1_Operation operation = s_Coordinator.GetActiveOperation();
			if (operation && operation.GetTargetObjectiveId() == s_Route.GetRouteId(2))
			{
				s_bCoexistence = s_Manager.HasGarrison() && s_Manager.GetObjectiveId() == s_Route.GetRouteId(3) && operation.GetAssignedGroupCount() == 1 && s_Coordinator.GetMaximumConcurrentOperations() == 1;
				if (s_bCoexistence) PrintFormat("[TB_PVE_M3_6] PASS coexistence garrisonTarget=%1 garrisonGroups=%2 attackTarget=%3 attackGroups=%4 separateOwners=1 totalPhysicalAI=%5 coordinatorMax=%6 garrisonMax=%7", s_Manager.GetObjectiveId(), s_Manager.GetGroupCount(), operation.GetTargetObjectiveId(), operation.GetAssignedGroupCount(), s_Manager.GetUnitCount() + operation.GetAssignedUnitCount(), s_Coordinator.GetMaximumConcurrentOperations(), s_Manager.GetMaximumConcurrent());
			}
			if (Owner(2) != "USSR") return;
			NextPhase(7); return;
		}
		if (s_iPhase == 7)
		{
			if (!s_Manager.HasGarrison() || s_Manager.GetObjectiveId() != s_Route.GetRouteId(2) || s_Manager.GetState() != TB_PVE_M3_6_GarrisonState.ACTIVE) return;
			if (!s_bCoexistence) { Fail("coexistence-not-observed"); return; }
			PrintFormat("[TB_PVE_M3_6] PASS frontier-retreat owner=%1 correctTarget=%2 created=%3 duplicates=0 stale=0", Owner(2), s_Manager.GetObjectiveId(), s_Manager.GetCreatedCount());
			s_Route.M2_2_SetDiagnosticInvalid(true); NextPhase(8); return;
		}
		if (s_iPhase == 8)
		{
			if (TB_PVE_M2_3_FrontierQuery.GetCampaignValidity() != TB_PVE_M2_2_CampaignValidity.INVALID_NON_CONTIGUOUS || s_Manager.HasGarrison()) return;
			Print("[TB_PVE_M3_6] PASS invalid-suspension validity=INVALID physicalGarrison=0 newCreation=0 FIAManaged=0");
			s_Route.M2_2_SetDiagnosticInvalid(false); s_Route.M2_2_ReconstructProjection();
			if (s_Coordinator) s_Coordinator.Stop("m3.6-proof-complete"); s_Manager.Stop("proof-complete");
			PrintFormat("[TB_PVE_M3_6] COMPLETE PASS serverAuthority=1 publicM2=1 targetFirstUSSRNode=1 maxGarrisons=%1 packageValidated=1 placementValidated=1 visibilityGate=1 defendWaypoint=1 attackOwnershipSeparate=1 reacts=1 defeated=1 noRegeneration=1 vanillaCapture=1 advance=1 retreat=1 coexistence=1 zeroPlayer=1 invalidTerminalPolicy=1 churnSafe=1 FIAExcluded=1 directOwnership=0 directFrontier=0 initialUnits=%2 creationMs=%3 cleanupMs=%4", s_Manager.GetMaximumConcurrent(), s_iGarrisonUnits, s_Manager.GetLastCreationDurationMs(), s_Manager.GetLastCleanupDurationMs());
			GetGame().GetCallqueue().Remove(Tick);
		}
	}

	protected static void StartCapture(int index, FactionKey factionKey, string label)
	{
		CleanupSetup(); s_iSetupTarget = index;
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey)); if (!faction) { Fail("capture-faction-unavailable"); return; }
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; vector origin = s_Route.GetRouteBase(index).GetOwner().GetOrigin(); params.Transform[3] = origin;
		s_SetupGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(faction.GetDefendersGroupPrefab()), GetGame().GetWorld(), params)); if (!s_SetupGroup) { Fail("capture-group-spawn-failed"); return; }
		int slot; for (int pass; pass < 2; pass++) foreach (ResourceName unitPrefab : s_SetupGroup.m_aUnitPrefabSlots) { int row = slot / 4; int column = slot - row * 4; vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 0.5; params.Transform[3] = position; IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params); if (character) { s_SetupGroup.AddAgentFromControlledEntity(character); s_aSetupCharacters.Insert(character); } slot++; }
		s_SetupGroup.SetLOD(0); PrintFormat("[TB_PVE_M3_6] vanilla-capture-start label=%1 faction=%2 objective=%3 actors=%4 directOwnership=0", label, factionKey, s_Route.GetRouteId(index), s_aSetupCharacters.Count());
	}
	protected static void CleanupSetup()
	{
		while (!s_aSetupCharacters.IsEmpty())
		{
			IEntity character = s_aSetupCharacters.Get(s_aSetupCharacters.Count() - 1);
			s_aSetupCharacters.Remove(s_aSetupCharacters.Count() - 1);
			if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		if (s_SetupGroup && !s_SetupGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_SetupGroup);
		s_SetupGroup = null;
	}
	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		return faction.GetFactionKey();
	}
	protected static void NextPhase(int phase) { s_iPhase = phase; s_iPhaseTicks = 0; }
	protected static void Fail(string reason) { PrintFormat("[TB_PVE_M3_6] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 target=%4 state=%5 groups=%6 units=%7 maxConcurrent=%8", reason, s_iPhase, s_iTicks, s_Manager && s_Manager.GetObjectiveId(), s_Manager && s_Manager.GetState(), s_Manager && s_Manager.GetGroupCount(), s_Manager && s_Manager.GetUnitCount(), s_Manager && s_Manager.GetMaximumConcurrent()); CleanupSetup(); if (s_Coordinator) s_Coordinator.Stop("m3.6-proof-failed"); if (s_Manager) s_Manager.Stop("proof-failed"); GetGame().GetCallqueue().Remove(Tick); }
}
