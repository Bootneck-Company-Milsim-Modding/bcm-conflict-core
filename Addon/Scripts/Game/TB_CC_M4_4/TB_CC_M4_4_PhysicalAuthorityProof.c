// Dedicated-only M4.4A acceptance driver. It materializes ordinary faction
// actors and observes vanilla Conflict capture; it never writes ownership,
// revision, graph state, or capture timers.
class TB_CC_M4_4_PhysicalAuthorityProof
{
	protected static TB_CC_M4_4_ArlandGraphCampaignComponent s_Campaign;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Characters = {};
	protected static int s_Mode;
	protected static int s_Phase;
	protected static int s_Ticks;
	protected static int s_RevisionBefore;
	protected static int s_MaxActors;
	protected static bool s_InterruptionStarted;
	protected static string s_Selected;
	protected static string s_Alternate;

	static void Start(TB_CC_M4_4_ArlandGraphCampaignComponent campaign, int mode)
	{
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated || !campaign || !campaign.IsReady()) return;
		s_Campaign = campaign; s_Mode = mode;
		s_InterruptionStarted = false;
		s_US = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		if (!s_US) { Fail("US-faction-missing"); return; }
		bool east = campaign.GetSelectedMobId() == "ARLAND::HQ_EAST";
		s_Selected = "ARLAND::MILITARY_SOUTH"; s_Alternate = "ARLAND::MOSS_HILL";
		if (!east) { s_Selected = "ARLAND::AIRPORT_NORTH"; s_Alternate = "ARLAND::FARM"; }
		PrintFormat("[TB_CC_M4_4A] START mode=%1 direction=%2 mob=%3 endpoint=%4 revision=%5 depth=%6 directOwnership=0 vanillaCapture=1", mode, east, campaign.GetSelectedMobId(), campaign.GetSelectedEndpointId(), campaign.GetRevision(), campaign.GetDepthPermille());
		LogSemantic("initial"); LogDeployment("initial");
		if (TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() != s_Selected || !TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty()) { Fail("initial-objective-mismatch"); return; }
		s_RevisionBefore = campaign.GetRevision();
		if (mode == 3) { SpawnAt(s_Selected, s_US, "legal-opening-capture-resume"); s_Phase = 2; }
		else { SpawnAt(s_Alternate, s_US, "alternate-candidate-lock"); s_Phase = 1; }
		s_Ticks = 0; GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_Ticks++;
		if (s_Ticks % 5 == 0) LogPhysical();
		if (s_Phase == 1 && s_Ticks >= 35)
		{
			SCR_CampaignMilitaryBaseComponent alternate = s_Campaign.GetBase(s_Alternate);
			bool pass = Owner(s_Alternate) == "USSR" && alternate.GetCaptureState() != SCR_EBaseCaptureState.CAPTURING && s_Campaign.GetRevision() == s_RevisionBefore && AliveCount() > 0 && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Alternate);
			PrintFormat("[TB_CC_M4_4A] %1 alternate-lock id=%2 owner=%3 captureState=%4 alive=%5 entityFactionAccepted=%6 authorized=%7 revisionStable=%8 durationSeconds=%9", Result(pass), s_Alternate, Owner(s_Alternate), alternate.GetCaptureState(), AliveCount(), EntityFactionAcceptedCount(s_Alternate), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Alternate), s_Campaign.GetRevision() == s_RevisionBefore, s_Ticks * 2);
			if (!pass) { Fail("alternate-candidate-progressed"); return; }
			Cleanup(); s_RevisionBefore = s_Campaign.GetRevision(); SpawnAt(s_Selected, s_US, "legal-opening-capture"); s_Phase = 2; s_Ticks = 0; return;
		}
		if (s_Phase == 2)
		{
			SCR_CampaignMilitaryBaseComponent selectedBase = s_Campaign.GetBase(s_Selected);
			SCR_CampaignSeizingComponent selectedSeizing = SCR_CampaignSeizingComponent.Cast(selectedBase.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
			if (s_Mode == 1 && !s_InterruptionStarted && selectedBase.GetCaptureState() == SCR_EBaseCaptureState.CAPTURING && selectedSeizing && selectedSeizing.GetSeizingStartTimestamp() != 0)
			{
				s_InterruptionStarted = true;
				PrintFormat("[TB_CC_M4_4E] interruption-start id=%1 owner=%2 prevailing=US seizingCharacters=%3 start=%4 end=%5 revision=%6", s_Selected, Owner(s_Selected), selectedSeizing.TB_CC_M4_4B_GetSeizingCharacters(), selectedSeizing.GetSeizingStartTimestamp(), selectedSeizing.GetSeizingEndTimestamp(), s_Campaign.GetRevision());
				Cleanup(); s_Phase = 21; s_Ticks = 0; return;
			}
			if (Owner(s_Selected) == "US")
			{
				Cleanup(); bool pass = s_Campaign.GetRevision() > s_RevisionBefore && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() == s_Selected;
				PrintFormat("[TB_CC_M4_4A] %1 vanilla-capture id=%2 previous=USSR new=US revisionBefore=%3 revisionAfter=%4 callback=1", Result(pass), s_Selected, s_RevisionBefore, s_Campaign.GetRevision()); LogSemantic("after-opening-capture"); LogDeployment("after-opening-capture");
				if (!pass) { Fail("opening-transition-mismatch"); return; }
				if (s_Mode == 2) { Complete("west-opening-physical"); return; }
				TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrison = s_Campaign.GetGarrisonManager(); if (garrison) { garrison.SetProofHumanCount(1); array<vector> observers = {}; garrison.SetProofObservers(observers); }
				TB_PVE_M3_2_OPFOROperationCoordinator coordinator = s_Campaign.GetCoordinator(); if (coordinator) { coordinator.SetDefaultPackageForProof("LIGHT"); coordinator.SetDelaysForProof(2, 6); }
				s_Phase = 3; s_Ticks = 0; return;
			}
			if (s_Ticks > 240) { Fail("opening-vanilla-capture-timeout"); return; }
		}
		if (s_Phase == 21)
		{
			SCR_CampaignMilitaryBaseComponent interruptedBase = s_Campaign.GetBase(s_Selected);
			SCR_CampaignSeizingComponent interruptedSeizing = SCR_CampaignSeizingComponent.Cast(interruptedBase.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
			if (interruptedBase.GetCaptureState() == SCR_EBaseCaptureState.NONE)
			{
				bool pass = Owner(s_Selected) == "USSR" && s_Campaign.GetRevision() == s_RevisionBefore && interruptedSeizing && !interruptedSeizing.TB_PVE_M1_4A_GetPrevailingFaction();
				PrintFormat("[TB_CC_M4_4E] %1 interruption id=%2 owner=%3 captureState=%4 prevailing=none revisionStable=%5", Result(pass), s_Selected, Owner(s_Selected), interruptedBase.GetCaptureState(), s_Campaign.GetRevision() == s_RevisionBefore);
				if (!pass) { Fail("vanilla-interruption-mismatch"); return; }
				SpawnAt(s_Selected, s_US, "legal-opening-capture-after-interruption"); s_Phase = 2; s_Ticks = 0; return;
			}
			if (s_Ticks > 20) { Fail("vanilla-interruption-timeout"); return; }
		}
		if (s_Phase == 3)
		{
			TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrison = s_Campaign.GetGarrisonManager();
			if (garrison && garrison.HasGarrison())
			{
				PrintFormat("[TB_CC_M4_4A] PASS graph-garrison objective=%1 state=%2 groups=%3 units=%4 LOD=%5 creationMs=%6", garrison.GetObjectiveId(), garrison.GetState(), garrison.GetGroupCount(), garrison.GetUnitCount(), garrison.GetFirstGroupLOD(), garrison.GetLastCreationDurationMs());
				s_Phase = 4; s_Ticks = 0; return;
			}
			if (s_Ticks > 30) { Fail("graph-garrison-timeout"); return; }
		}
		if (s_Phase == 4)
		{
			TB_PVE_M3_2_OPFOROperationCoordinator coordinator = s_Campaign.GetCoordinator();
			if (Owner(s_Selected) == "USSR")
			{
				int operationState = -1; int operations; int maximumConcurrent;
				if (coordinator) { operationState = coordinator.GetLastTerminalState(); operations = coordinator.GetCreatedOperationCount(); maximumConcurrent = coordinator.GetMaximumConcurrentOperations(); }
				bool success = operationState == TB_PVE_M3_1_OperationState.SUCCEEDED;
				PrintFormat("[TB_CC_M4_4A] %1 autonomous-recapture id=%2 owner=USSR operationState=%3 operations=%4 maxConcurrent=%5", Result(success), s_Selected, operationState, operations, maximumConcurrent);
				LogSemantic("after-opfor-retreat"); LogDeployment("after-opfor-retreat");
				if (!success) { Fail("ownership-retreated-before-operation-success-observed"); return; }
				Complete("east-opening-lock-forward-garrison-autonomous-retreat"); return;
			}
			if (s_Ticks > 180) { Fail("autonomous-opfor-recapture-timeout"); return; }
		}
	}

	protected static void SpawnAt(string id, SCR_CampaignFaction faction, string label)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); Resource resource = Resource.Load(faction.GetDefendersGroupPrefab()); EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; base.GetOwner().GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params)); if (!s_Group) { Fail("group-spawn-failed-" + id); return; }
		vector origin = base.GetOwner().GetOrigin(); int row; int column;
		for (int wave; wave < 10; wave++) foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
		{
			vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1; params.Transform[3] = position;
			IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params); if (character) { s_Group.AddAgentFromControlledEntity(character); s_Characters.Insert(character); }
			column++; if (column == 5) { column = 0; row++; }
		}
		s_MaxActors = Math.Max(s_MaxActors, s_Characters.Count()); PrintFormat("[TB_CC_M4_4A] proof-force label=%1 faction=%2 id=%3 actors=%4 alive=%5 entityFactionAccepted=%6 distanceToCenter=0 LOD=%7", label, faction.GetFactionKey(), id, s_Characters.Count(), AliveCount(), EntityFactionAcceptedCount(id), s_Group.GetLOD());
	}

	protected static int EntityFactionAcceptedCount(string id)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent)); int count;
		foreach (IEntity character : s_Characters) if (character && seizing && seizing.TB_PVE_M1_4A_EvaluateEntityFaction(character)) count++;
		return count;
	}
	protected static int AliveCount()
	{
		int count;
		foreach (IEntity character : s_Characters)
		{
			if (character)
			{
				CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
				if (controller && controller.GetLifeState() == ECharacterLifeState.ALIVE) count++;
			}
		}
		return count;
	}
	protected static string Owner(string id)
	{
		string result = "<none>";
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id);
		if (base && base.GetFaction()) result = base.GetFaction().GetFactionKey();
		return result;
	}
	protected static void LogPhysical()
	{
		string id = s_Selected; if (s_Phase == 1) id = s_Alternate;
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id);
		PrintFormat("[TB_CC_M4_4A] physical phase=%1 tick=%2 id=%3 owner=%4 captureState=%5 alive=%6 entityFactionAccepted=%7 authorized=%8 revision=%9", s_Phase, s_Ticks, id, Owner(id), base.GetCaptureState(), AliveCount(), EntityFactionAcceptedCount(id), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", id), s_Campaign.GetRevision());
		SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		SCR_Faction prevailing; if (seizing) prevailing = seizing.TB_PVE_M1_4A_GetPrevailingFaction(); FactionKey prevailingKey = "<none>"; if (prevailing) prevailingKey = prevailing.GetFactionKey();
		ChimeraWorld world = GetGame().GetWorld();
		if (seizing)
		{
			PrintFormat("[TB_CC_M4_4B] capture-clock id=%1 now=%2 start=%3 end=%4 currentTime=%5 minTime=%6 maxTime=%7 seizingCharacters=%8 maxCharacters=%9", id, world.GetServerTimestamp(), seizing.GetSeizingStartTimestamp(), seizing.GetSeizingEndTimestamp(), seizing.TB_CC_M4_4B_GetCurrentSeizingTime(), seizing.TB_PVE_M1_1_GetMinimumSeizingTime(), seizing.TB_PVE_M1_1_GetMaximumSeizingTime(), seizing.TB_CC_M4_4B_GetSeizingCharacters(), seizing.TB_PVE_M1_1_GetMaximumSeizingCharacters());
			PrintFormat("[TB_CC_M4_4D] capture-admission id=%1 prevailing=%2 contesting=%3 minimumCharactersForContest=%4 seizingCharacters=%5 requiresPlayer=%6 ignoreNonPlayable=%7 interruptedDuration=%8", id, prevailingKey, seizing.GetContestingFactionsCount(), seizing.TB_CC_M4_4B_GetMinimumCharactersForContest(), seizing.TB_CC_M4_4B_GetSeizingCharacters(), seizing.TB_PVE_M1_1_GetCapturingRequiresPlayer(), seizing.TB_PVE_M1_1_GetIgnoreNonPlayableAttackers(), seizing.TB_CC_M4_4B_GetInterruptedCaptureDuration());
		}
		array<SCR_CampaignBuildingProviderComponent> providers = {}; array<SCR_ServicePointComponent> services = {}; SCR_SpawnPoint spawn = base.GetSpawnPoint();
		int radius; float extraRadio; float extraService; if (seizing) { radius = seizing.GetRadius(); extraRadio = seizing.TB_CC_M4_4B_GetExtraTimePerRadioConnection(); extraService = seizing.TB_CC_M4_4B_GetExtraTimePerService(); }
		PrintFormat("[TB_CC_M4_4B] base-state id=%1 entity=%2 initialized=%3 isHQ=%4 radius=%5 supplies=%6 income=%7 spawnValid=%8", id, base.GetOwner().GetName(), base.IsInitialized(), base.IsHQ(), radius, base.GetSupplies(), base.GetSuppliesIncome(), spawn != null);
		PrintFormat("[TB_CC_M4_4B] base-services id=%1 providers=%2 services=%3 radioRange=%4", id, base.GetBuildingProviders(providers), base.GetServices(services), base.GetRadioRange());
		int coverage; bool receive; bool send; bool both; if (us) { coverage = base.GetHQRadioCoverage(us); receive = base.IsHQRadioTrafficPossible(us); send = base.IsHQRadioTrafficPossible(us, SCR_ERadioCoverageStatus.SEND); both = base.IsHQRadioTrafficPossible(us, SCR_ERadioCoverageStatus.BOTH_WAYS); }
		PrintFormat("[TB_CC_M4_4B] radio-state id=%1 USCoverage=%2 USReceive=%3 USSend=%4 USBoth=%5 extraRadioTime=%6 extraServiceTime=%7", id, coverage, receive, send, both, extraRadio, extraService);
	}
	protected static void LogSemantic(string phase) { array<string> candidates = {}; TB_PVE_M2_3_FrontierQuery.GetBLUFORCandidateObjectives(candidates); array<string> friendly = {}; s_Campaign.GetEvaluator().GetFriendlyComponent(friendly); PrintFormat("[TB_CC_M4_4A] semantic phase=%1 revision=%2 friendly=%3 candidates=%4 BLUFOR=%5 OPFOR=%6 depth=%7 outcome=%8", phase, s_Campaign.GetRevision(), friendly, candidates, TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_Campaign.GetDepthPermille(), s_Campaign.GetOutcome()); }
	protected static void LogDeployment(string phase) { array<string> ids = {"ARLAND::HQ_EAST","ARLAND::HQ_WEST","ARLAND::AIRPORT_NORTH","ARLAND::FARM","ARLAND::BEAUREGARD","ARLAND::ARLEVILLE","ARLAND::MILITARY_SOUTH","ARLAND::MOSS_HILL"}; foreach (string id : ids) { SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); SCR_SpawnPoint spawn = base.GetSpawnPoint(); bool enabled; if (spawn) enabled = spawn.IsSpawnPointEnabled(); PrintFormat("[TB_CC_M4_4A] deployment phase=%1 id=%2 owner=%3 spawnValid=%4 enabled=%5 selected=%6", phase, id, Owner(id), spawn != null, enabled, id == s_Campaign.GetSelectedMobId()); } }
	protected static string Result(bool pass) { string result = "FAIL"; if (pass) result = "PASS"; return result; }
	protected static void Cleanup()
	{
		while (!s_Characters.IsEmpty())
		{
			IEntity character = s_Characters.Get(s_Characters.Count() - 1);
			s_Characters.Remove(s_Characters.Count() - 1);
			if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		if (s_Group && !s_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_Group);
		s_Group = null;
	}
	protected static void Complete(string scope) { Cleanup(); GetGame().GetCallqueue().Remove(Tick); PrintFormat("[TB_CC_M4_4A] COMPLETE PASS scope=%1 mode=%2 maxProofActors=%3 directOwnership=0", scope, s_Mode, s_MaxActors); }
	protected static void Fail(string reason) { Cleanup(); GetGame().GetCallqueue().Remove(Tick); int revision = -1; if (s_Campaign) revision = s_Campaign.GetRevision(); PrintFormat("[TB_CC_M4_4A] COMPLETE FAIL reason=%1 phase=%2 ticks=%3 revision=%4 maxProofActors=%5 directOwnership=0", reason, s_Phase, s_Ticks, revision, s_MaxActors); }
}
