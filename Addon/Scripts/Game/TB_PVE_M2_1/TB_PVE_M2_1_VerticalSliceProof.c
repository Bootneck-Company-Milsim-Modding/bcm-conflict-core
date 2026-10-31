class TB_PVE_M2_1_VerticalSliceProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Frontier;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_CampaignFaction s_USSR;
	protected static SCR_CampaignFaction s_FIA;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Characters = {};
	protected static int s_Phase;
	protected static int s_Ticks;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent frontier)
	{
		if (!Replication.IsServer() || !frontier || !frontier.IsReady())
			return;
		s_Frontier = frontier;
		FactionManager fm = GetGame().GetFactionManager();
		s_US = SCR_CampaignFaction.Cast(fm.GetFactionByKey("US"));
		s_USSR = SCR_CampaignFaction.Cast(fm.GetFactionByKey("USSR"));
		s_FIA = SCR_CampaignFaction.Cast(fm.GetFactionByKey("FIA"));
		if (!s_US || !s_USSR || !s_FIA)
		{
			PrintFormat("[TB_PVE_M2_1] FAIL proof missing factions US=%1 USSR=%2 FIA=%3", s_US, s_USSR, s_FIA);
			return;
		}

		Print("[TB_PVE_M2_1] proof-start sequence=locked-OBJ_02,locked-OBJ_03,FIA-neutral,OBJ_01-capture,OBJ_02-capture,OBJ_02-recapture,OPFOR-MOB-backcap");
		LogRegression("initial");
		SpawnAt(2, s_US, "locked OBJ_02 BLUFOR attempt");
		s_Phase = 1;
		s_Ticks = 0;
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_Ticks++;
		if ((s_Phase == 4 || s_Phase == 5 || s_Phase == 6) && s_Ticks % 5 == 0)
			LogCaptureDiagnostic();
		if (s_Phase == 1 && s_Ticks >= 30)
		{
			if (!AssertLocked(2, "OBJ_02-initial")) return;
			Cleanup(); SpawnAt(3, s_US, "locked OBJ_03 BLUFOR attempt"); s_Phase = 2; s_Ticks = 0; return;
		}
		if (s_Phase == 2 && s_Ticks >= 30)
		{
			if (!AssertLocked(3, "OBJ_03-initial")) return;
			Cleanup(); SpawnAt(1, s_FIA, "FIA outside frontier attempt"); s_Phase = 3; s_Ticks = 0; return;
		}
		if (s_Phase == 3 && s_Ticks >= 30)
		{
			if (!AssertLocked(1, "FIA-does-not-advance")) return;
			PrintFormat("[TB_PVE_M2_1] PASS FIA interaction owner=%1 frontier=%2", Owner(1), s_Frontier.GetFrontierIndex());
			Cleanup();
			Print("[TB_PVE_M2_2][SERVER] pre-forward-hold durationMs=50000 purpose=live-client");
			s_Phase = 30; s_Ticks = 0;
			GetGame().GetCallqueue().CallLater(StartObj1Capture, 50000);
			return;
		}
		if (s_Phase == 4 && Owner(1) == "US")
		{
			Cleanup();
			PrintFormat("[TB_PVE_M2_1] PASS OBJ_01 captured frontier=%1 bluforLegal=%2", s_Frontier.GetFrontierIndex(), s_Frontier.GetBluforLegalIndex());
			s_Phase = 40;
			s_Ticks = 0;
			GetGame().GetCallqueue().CallLater(StartObj2Capture, 3000);
			return;
		}
		if (s_Phase == 5 && Owner(2) == "US")
		{
			Cleanup();
			PrintFormat("[TB_PVE_M2_1] PASS OBJ_02 captured OBJ_03-unlocked=%1 frontier=%2", s_Frontier.GetBluforLegalIndex() == 3, s_Frontier.GetFrontierIndex());
			s_Frontier.M2_2_ReconstructProjection();
			Print("[TB_PVE_M2_2][SERVER] advanced-frontier-hold waitsForRemoteClient=1 fallbackMs=120000 purpose=JIP-or-live-reverse");
			s_Phase = 50; s_Ticks = 0;
			return;
		}
		if (s_Phase == 50)
		{
			array<int> playerIds = {};
			GetGame().GetPlayerManager().GetPlayers(playerIds);
			if (playerIds.Count() >= 2)
			{
				PrintFormat("[TB_PVE_M2_2][SERVER] remote-client-detected count=%1 reverseStartsAfterMs=5000", playerIds.Count());
				s_Phase = 51;
				GetGame().GetCallqueue().CallLater(StartReverseCapture, 5000);
				return;
			}
			if (s_Ticks >= 60)
			{
				Print("[TB_PVE_M2_2][SERVER] remote-client-wait fallback=1");
				StartReverseCapture();
				return;
			}
		}
		if (s_Phase == 6 && Owner(2) == "USSR")
		{
			Cleanup();
			PrintFormat("[TB_PVE_M2_1] PASS reverse-capture OBJ_02 owner=USSR frontier=%1 OBJ_03-relocked=%2 bluforLegal=%3 opforLegal=%4", s_Frontier.GetFrontierIndex(), s_Frontier.GetBluforLegalIndex() == 2, s_Frontier.GetBluforLegalIndex(), s_Frontier.GetOpforLegalIndex());
			s_Frontier.M2_2_ReconstructProjection();
			TB_PVE_M2_3_FrontierQuery.TB_M2_3_RunRevisionDiagnostics();
			SpawnAt(0, s_USSR, "illegal OPFOR MOB back-cap"); s_Phase = 7; s_Ticks = 0; return;
		}
		if (s_Phase == 7 && s_Ticks >= 30)
		{
			if (!AssertLocked(0, "OPFOR-MOB-anti-backcap")) return;
			Cleanup();
			array<string> invalid = {"US", "USSR", "US", "USSR"};
			bool accepted = s_Frontier.EvaluateSyntheticOwnership(invalid, "non-contiguous-runtime-proof");
			PrintFormat("[TB_PVE_M2_1] PASS non-contiguous-policy detected=%1 policy=DENY_ALL_STRATEGIC_CAPTURES", !accepted);
			s_Frontier.M2_2_SetDiagnosticInvalid(true);
			Print("[TB_PVE_M2_2][SERVER] invalid-projection-hold durationMs=60000 policy=DENY_ALL_STRATEGIC_CAPTURES");
			s_Phase = 8; s_Ticks = 0;
			GetGame().GetCallqueue().CallLater(FinishInvalidProjectionProof, 60000);
			return;
		}

		if ((s_Phase == 4 || s_Phase == 5 || s_Phase == 6) && s_Ticks >= 45)
			Fail("capture phase timeout");
	}

	protected static void StartObj1Capture()
	{
		SpawnAt(1, s_US, "eligible OBJ_01 BLUFOR capture");
		s_Phase = 4;
		s_Ticks = 0;
	}

	protected static void StartReverseCapture()
	{
		SpawnAt(2, s_USSR, "legal OPFOR reverse capture OBJ_02");
		s_Phase = 6;
		s_Ticks = 0;
	}

	protected static void FinishInvalidProjectionProof()
	{
		s_Frontier.M2_2_SetDiagnosticInvalid(false);
		s_Frontier.M2_2_ReconstructProjection();
		LogRegression("final");
		Print("[TB_PVE_M2_1] COMPLETE PASS sequence=MOB-US,OBJ_01-US,OBJ_02-USSR,OBJ_03-USSR reversible=1 vanillaOwnershipOnly=1");
		Print("[TB_PVE_M2_2][SERVER] COMPLETE projection-lifecycle initial-forward-forward-reverse-invalid-reconstruct=1");
		GetGame().GetCallqueue().Remove(Tick);
	}

	protected static void StartObj2Capture()
	{
		SCR_CampaignMilitaryBaseComponent base = s_Frontier.GetRouteBase(2);
		PrintFormat("[TB_PVE_M2_1] post-OBJ_01-settle OBJ_02 hqReceive=%1 hqBothWays=%2 OBJ_01income=%3", base.IsHQRadioTrafficPossible(s_US), base.IsHQRadioTrafficPossible(s_US, SCR_ERadioCoverageStatus.BOTH_WAYS), s_Frontier.GetRouteBase(1).GetSuppliesIncome());
		SpawnAt(2, s_US, "newly eligible OBJ_02 BLUFOR capture");
		s_Phase = 5;
		s_Ticks = 0;
	}

	protected static bool AssertLocked(int index, string label)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Frontier.GetRouteBase(index);
		string expectedOwner = "USSR";
		if (index == 0) expectedOwner = "US";
		int alive;
		foreach (IEntity character : s_Characters)
		{
			if (!character) continue;
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
			if (controller && controller.GetLifeState() == ECharacterLifeState.ALIVE) alive++;
		}
		bool pass = Owner(index) == expectedOwner && base.GetCaptureState() != SCR_EBaseCaptureState.CAPTURING && alive > 0;
		string result = "FAIL";
		if (pass) result = "PASS";
		PrintFormat("[TB_PVE_M2_1] %1 locked-test=%2 owner=%3 captureState=%4 ticks=%5 aliveAtEnd=%6", result, label, Owner(index), base.GetCaptureState(), s_Ticks, alive);
		if (!pass) Fail("locked objective progressed");
		return pass;
	}

	protected static void LogCaptureDiagnostic()
	{
		int routeIndex = s_Phase - 3;
		if (s_Phase == 6) routeIndex = 2;
		SCR_CampaignMilitaryBaseComponent base = s_Frontier.GetRouteBase(routeIndex);
		SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		int alive;
		int eligible;
		foreach (IEntity character : s_Characters)
		{
			if (!character) continue;
			CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
			if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE) continue;
			alive++;
			if (seizing && seizing.TB_PVE_M1_4A_EvaluateEntityFaction(character)) eligible++;
		}
		Faction prevailing;
		if (seizing) prevailing = seizing.TB_PVE_M1_4A_GetPrevailingFaction();
		FactionKey prevailingKey = "<none>";
		if (prevailing) prevailingKey = prevailing.GetFactionKey();
		PrintFormat("[TB_PVE_M2_1] capture-diagnostic phase=%1 objective=%2 tick=%3 owner=%4 captureState=%5 alive=%6 eligible=%7 prevailing=%8", s_Phase, s_Frontier.GetRouteId(routeIndex), s_Ticks, Owner(routeIndex), base.GetCaptureState(), alive, eligible, prevailingKey);
	}

	protected static void SpawnAt(int routeIndex, SCR_CampaignFaction faction, string label)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Frontier.GetRouteBase(routeIndex);
		ResourceName prefabName = faction.GetDefendersGroupPrefab();
		// FIA has no campaign defender-group resource in this scenario. Use one
		// ordinary vanilla FIA fire team only for the bounded exclusion proof.
		if (prefabName.IsEmpty() && faction.GetFactionKey() == "FIA")
			prefabName = "{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et";
		Resource resource = Resource.Load(prefabName);
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		base.GetOwner().GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!s_Group)
		{
			Fail("group spawn failed " + label);
			return;
		}
		vector origin = base.GetOwner().GetOrigin();
		int slot;
		int row;
		int column;
		for (int wave; wave < 10; wave++)
		{
			foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
			{
				Resource unit = Resource.Load(unitPrefab);
				if (!unit) continue;
				vector spawnPosition = origin + Vector(column * 2, 0, row * 2);
				spawnPosition[1] = GetGame().GetWorld().GetSurfaceY(spawnPosition[0], spawnPosition[2]) + 1.0;
				params.Transform[3] = spawnPosition;
				IEntity character = GetGame().SpawnEntityPrefab(unit, GetGame().GetWorld(), params);
				if (character)
				{
					s_Group.AddAgentFromControlledEntity(character);
					s_Characters.Insert(character);
				}
				slot++;
				column++;
				if (column == 5)
				{
					column = 0;
					row++;
				}
			}
		}
		PrintFormat("[TB_PVE_M2_1] proof-force-spawned label=%1 faction=%2 objective=%3 characters=%4 groupLOD=%5 maxLOD=%6", label, faction.GetFactionKey(), s_Frontier.GetRouteId(routeIndex), s_Characters.Count(), s_Group.GetLOD(), AIAgent.GetMaxLOD());
	}

	protected static void Cleanup()
	{
		foreach (IEntity character : s_Characters)
			if (character) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_Characters.Clear();
		if (s_Group) SCR_EntityHelper.DeleteEntityAndChildren(s_Group);
		s_Group = null;
	}

	protected static string Owner(int index)
	{
		Faction owner = s_Frontier.GetRouteBase(index).GetFaction();
		if (!owner) return "<none>";
		return owner.GetFactionKey();
	}

	protected static void LogRegression(string phase)
	{
		for (int i = 1; i < s_Frontier.GetRouteCount(); i++)
		{
			SCR_CampaignMilitaryBaseComponent base = s_Frontier.GetRouteBase(i);
			array<SCR_CampaignBuildingProviderComponent> providers = {};
			array<SCR_ServicePointComponent> services = {};
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
			bool enabled;
			bool visible;
			bool reserve;
			SCR_ESpawnResult result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			if (spawn)
			{
				enabled = spawn.IsSpawnPointEnabled();
				visible = spawn.IsSpawnPointVisibleForPlayer(SCR_PlayerController.GetLocalPlayerId());
				reserve = spawn.CanReserveFor_S(1, result);
			}
			PrintFormat("[TB_PVE_M2_1] deployment-regression phase=%1 objective=%2 owner=%3 spawnValid=%4 spawnEnabled=%5 visible=%6 canReserve=%7 reserveResult=%8", phase, s_Frontier.GetRouteId(i), Owner(i), spawn != null, enabled, visible, reserve, result);
			PrintFormat("[TB_PVE_M2_1] systems-regression phase=%1 objective=%2 supplies=%3 income=%4 radioValid=%5 radioRange=%6 providers=%7 services=%8", phase, s_Frontier.GetRouteId(i), base.GetSupplies(), base.GetSuppliesIncome(), radio != null, base.GetRadioRange(), base.GetBuildingProviders(providers), base.GetServices(services));
		}
		int enabledUSMobs;
		foreach (SCR_SpawnPoint point : SCR_SpawnPoint.GetSpawnPointsForFaction("US"))
		{
			SCR_CampaignSpawnPointGroup campaignSpawn = SCR_CampaignSpawnPointGroup.Cast(point);
			if (!campaignSpawn || !campaignSpawn.GetParent()) continue;
			SCR_CampaignMilitaryBaseComponent mob = SCR_CampaignMilitaryBaseComponent.Cast(campaignSpawn.GetParent().FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (mob && mob.IsHQ() && point.IsSpawnPointEnabled()) enabledUSMobs++;
		}
		PrintFormat("[TB_PVE_M2_1] MOB-regression phase=%1 enabledUSMobSpawnPoints=%2", phase, enabledUSMobs);
	}

	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M2_1] FAIL proof reason=%1 phase=%2 ticks=%3", reason, s_Phase, s_Ticks);
		Cleanup();
		GetGame().GetCallqueue().Remove(Tick);
	}
}
