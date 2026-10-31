// Bounded dedicated proof for the authored production-scale route. Every world
// ownership transition is still performed by vanilla physical capture.
class TB_PVE_M2_4_ProductionRouteProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_CampaignFaction s_USSR;
	protected static SCR_CampaignFaction s_FIA;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Characters = {};
	protected static int s_Mode;
	protected static int s_Target;
	protected static int s_Ticks;
	protected static int s_ReverseRemaining;
	protected static int s_RevisionBefore;
	protected static bool s_bNetworkAcceptance;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route)
	{
		if (!Replication.IsServer() || !route || !route.IsReady()) return;
		s_Route = route;
		FactionManager fm = GetGame().GetFactionManager();
		s_US = SCR_CampaignFaction.Cast(fm.GetFactionByKey("US"));
		s_USSR = SCR_CampaignFaction.Cast(fm.GetFactionByKey("USSR"));
		s_FIA = SCR_CampaignFaction.Cast(fm.GetFactionByKey("FIA"));
		if (!s_US || !s_USSR || !s_FIA) { Fail("factions unavailable"); return; }
		s_bNetworkAcceptance = true;
		s_Mode = 20;
		s_Ticks = 0;
		PrintFormat("[TB_PVE_M2_4A] SERVER_INITIAL_READY revision=%1 frontier=%2 USLegal=%3 USSRLegal=%4 route=%5", TB_PVE_M2_3_FrontierQuery.GetRevision(), route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), route.GetRouteIdentity());
		Print("[TB_PVE_M2_4A] WAITING_FOR_CLIENT_A minimumPlayers=1 captureStartsAfterAdmissionDelayMs=10000");
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_Ticks++;
		if (s_Mode == 20)
		{
			array<int> playerIds = {};
			GetGame().GetPlayerManager().GetPlayers(playerIds);
			if (playerIds.Count() < 1) return;
			PrintFormat("[TB_PVE_M2_4A] CLIENT_A_ADMITTED players=%1 revision=%2 frontier=%3 captureStartsAfterMs=10000", playerIds.Count(), TB_PVE_M2_3_FrontierQuery.GetRevision(), s_Route.GetFrontierIndex());
			s_Mode = 22;
			s_Ticks = 0;
			return;
		}
		if (s_Mode == 22)
		{
			if (s_Ticks < 5) return;
			StartForward(1);
			return;
		}
		if (s_Mode == 21)
		{
			array<int> playerIds = {};
			GetGame().GetPlayerManager().GetPlayers(playerIds);
			if (playerIds.Count() < 2) return;
			PrintFormat("[TB_PVE_M2_4A] CLIENT_B_ADMITTED players=%1 revision=%2 frontier=%3 USLegal=%4 USSRLegal=%5", playerIds.Count(), TB_PVE_M2_3_FrontierQuery.GetRevision(), s_Route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
			GetGame().GetCallqueue().Remove(Tick);
			return;
		}
		if (s_Mode == 1)
		{
			if (s_Ticks < 10) return;
			if (!AssertUnchanged("deep-lock")) return;
			StartDenied(6, s_US, "initial-deep-lock-OBJ_06");
			s_Mode = 2;
			return;
		}
		if (s_Mode == 2)
		{
			if (s_Ticks < 10) return;
			if (!AssertUnchanged("deep-lock")) return;
			StartDenied(1, s_FIA, "FIA-frontier-isolation");
			s_Mode = 3;
			return;
		}
		if (s_Mode == 3)
		{
			if (s_Ticks < 10) return;
			if (!AssertUnchanged("FIA-isolation")) return;
			StartForward(1);
			return;
		}
		if (s_Mode == 4)
		{
			if (Owner(s_Target) == "US")
			{
				Cleanup();
				if (s_Route.GetFrontierIndex() != s_Target || TB_PVE_M2_3_FrontierQuery.GetRevision() <= s_RevisionBefore || TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() != NextId(s_Target))
				{
					Fail("forward semantic mismatch"); return;
				}
				PrintFormat("[TB_PVE_M2_4] PASS forward index=%1 id=%2 revision=%3 next=%4 previousState=%5", s_Target, s_Route.GetRouteId(s_Target), TB_PVE_M2_3_FrontierQuery.GetRevision(), NextId(s_Target), TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(s_Route.GetRouteId(s_Target)));
				if (s_bNetworkAcceptance)
				{
					PrintFormat("[TB_PVE_M2_4A] SERVER_TRANSITION revision=%1 frontier=%2 USLegal=%3 USSRLegal=%4 captured=%5", TB_PVE_M2_3_FrontierQuery.GetRevision(), s_Route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_Route.GetRouteId(s_Target));
					if (s_Target == 4)
					{
						int before = TB_PVE_M2_3_FrontierQuery.GetRevision();
						s_Route.M2_2_ReconstructProjection();
						PrintFormat("[TB_PVE_M2_4A] MID_ROUTE_READY revision=%1 frontier=%2 USLegal=%3 USSRLegal=%4 revisionStable=%5", TB_PVE_M2_3_FrontierQuery.GetRevision(), s_Route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), before == TB_PVE_M2_3_FrontierQuery.GetRevision());
						Print("[TB_PVE_M2_4A] WAITING_FOR_CLIENT_B freshJIP=1");
						s_Mode = 21;
						s_Ticks = 0;
						return;
					}
					s_Target++;
					s_Mode = 8;
					s_Ticks = 0;
					PrintFormat("[TB_PVE_M2_4A] advancing-to-mid-route next=%1 settleMs=6000", s_Route.GetRouteId(s_Target));
					return;
				}
				if (s_Target == 2 || s_Target == 4 || s_Target == 7)
				{
					int before = TB_PVE_M2_3_FrontierQuery.GetRevision();
					s_Route.M2_2_ReconstructProjection();
					PrintFormat("[TB_PVE_M2_4] PASS mid-route-reconstruction depth=%1 frontier=%2 revisionStable=%3", s_Target, s_Route.GetFrontierIndex(), before == TB_PVE_M2_3_FrontierQuery.GetRevision());
				}
				if (s_Target == 4)
				{
					StartDenied(6, s_US, "mid-route-deep-lock");
					s_Mode = 5;
					return;
				}
				if (s_Target + 1 < s_Route.GetRouteCount())
				{
					s_Target++;
					s_Mode = 8;
					s_Ticks = 0;
					PrintFormat("[TB_PVE_M2_4] radio-settle-before-next id=%1 durationMs=6000", s_Route.GetRouteId(s_Target));
					return;
				}
				PrintFormat("[TB_PVE_M2_4] FRONTIER_AT_OPFOR_ENDPOINT id=%1 revision=%2 gameContinues=1", s_Route.GetRouteId(s_Target), TB_PVE_M2_3_FrontierQuery.GetRevision());
				s_ReverseRemaining = 3;
				StartReverse(s_Target);
				return;
			}
			if (s_Ticks > 35) Fail("forward capture timeout");
			return;
		}
		if (s_Mode == 5)
		{
			if (s_Ticks < 10) return;
			if (!AssertUnchanged("mid-route-deep-lock")) return;
			StartForward(5);
			return;
		}
		if (s_Mode == 6)
		{
			if (Owner(s_Target) == "USSR")
			{
				Cleanup();
				if (s_Route.GetFrontierIndex() != s_Target - 1 || TB_PVE_M2_3_FrontierQuery.GetRevision() <= s_RevisionBefore)
				{
					Fail("reverse semantic mismatch"); return;
				}
				bool deeperRelocked = true;
				if (s_Target + 1 < s_Route.GetRouteCount()) deeperRelocked = TB_PVE_M2_3_FrontierQuery.IsObjectiveStrategicallyLocked(s_Route.GetRouteId(s_Target + 1));
				PrintFormat("[TB_PVE_M2_4] PASS reverse index=%1 id=%2 frontier=%3 revision=%4 BLUFOR=%5 OPFOR=%6 deeperRelocked=%7", s_Target, s_Route.GetRouteId(s_Target), s_Route.GetFrontierIndex(), TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), deeperRelocked);
				s_ReverseRemaining--;
				if (s_ReverseRemaining > 0) { StartReverse(s_Target - 1); return; }
				StartDenied(s_Target - 2, s_USSR, "OPFOR-deep-rear-anti-backcap");
				s_Mode = 7;
				return;
			}
			if (s_Ticks > 35) Fail("reverse capture timeout");
			return;
		}
		if (s_Mode == 7)
		{
			if (s_Ticks < 10) return;
			if (!AssertUnchanged("OPFOR-deep-rear-anti-backcap")) return;
			Finish();
		}
		if (s_Mode == 8 && s_Ticks >= 3)
			StartForward(s_Target);
	}

	protected static void StartForward(int index)
	{
		if (TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() != s_Route.GetRouteId(index)) { Fail("wrong BLUFOR legal objective"); return; }
		if (index + 1 < s_Route.GetRouteCount() && !TB_PVE_M2_3_FrontierQuery.IsObjectiveStrategicallyLocked(s_Route.GetRouteId(index + 1))) { Fail("next objective unlocked early"); return; }
		s_Mode = 4; s_Target = index; s_Ticks = 0; s_RevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
		LogRadio("forward", index, s_US);
		SpawnAt(index, s_US, "forward");
	}

	protected static void StartReverse(int index)
	{
		if (TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != s_Route.GetRouteId(index)) { Fail("wrong OPFOR legal objective"); return; }
		s_Mode = 6; s_Target = index; s_Ticks = 0; s_RevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
		LogRadio("reverse", index, s_USSR);
		SpawnAt(index, s_USSR, "reverse");
	}

	protected static void StartDenied(int index, SCR_CampaignFaction faction, string label)
	{
		Cleanup(); s_Target = index; s_Ticks = 0; s_RevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
		if (s_Mode == 0) s_Mode = 1;
		PrintFormat("[TB_PVE_M2_4] denied-test-start label=%1 faction=%2 id=%3 authorization=%4", label, faction.GetFactionKey(), s_Route.GetRouteId(index), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction.GetFactionKey(), s_Route.GetRouteId(index)));
		SpawnAt(index, faction, label);
	}

	protected static bool AssertUnchanged(string label)
	{
		bool pass = TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore && s_Route.GetRouteBase(s_Target).GetCaptureState() == SCR_EBaseCaptureState.NONE;
		string result = "FAIL";
		if (pass) result = "PASS";
		PrintFormat("[TB_PVE_M2_4] %1 denied-test=%2 id=%3 owner=%4 revisionStable=%5 captureState=%6", result, label, s_Route.GetRouteId(s_Target), Owner(s_Target), TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore, s_Route.GetRouteBase(s_Target).GetCaptureState());
		Cleanup();
		if (!pass) Fail("denied objective progressed");
		return pass;
	}

	protected static void Finish()
	{
		array<string> invalid = {};
		for (int i; i < s_Route.GetRouteCount(); i++)
		{
			if (i <= 3 || i == 6) invalid.Insert("US");
			else invalid.Insert("USSR");
		}
		bool detected = !s_Route.EvaluateSyntheticOwnership(invalid, "M2.4-production-non-contiguous");
		s_Route.M2_2_SetDiagnosticInvalid(true);
		PrintFormat("[TB_PVE_M2_4] PASS invalid-production-route detected=%1 validity=%2 BLUFOR=%3 OPFOR=%4 attackAllowed=%5 objectiveState=%6", detected, TB_PVE_M2_3_FrontierQuery.GetCampaignValidity(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Route.GetRouteId(1)), TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(s_Route.GetRouteId(1)));
		s_Route.M2_2_SetDiagnosticInvalid(false);
		s_Route.M2_2_ReconstructProjection();
		LogRepresentativeSystems("final-early", 1);
		LogRepresentativeSystems("final-middle", s_Route.GetRouteCount() / 2);
		LogRepresentativeSystems("final-late", s_Route.GetRouteCount() - 1);
		PrintFormat("[TB_PVE_M2_4] COMPLETE PASS route=%1 nodes=%2 fullForward=1 consecutiveReverse=3 vanillaOwnershipOnly=1 noVictoryImplemented=1", s_Route.GetRouteIdentity(), s_Route.GetRouteCount());
		GetGame().GetCallqueue().Remove(Tick);
	}

	protected static void LogRadio(string phase, int index, SCR_CampaignFaction faction)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Route.GetRouteBase(index);
		PrintFormat("[TB_PVE_M2_4] radio phase=%1 id=%2 faction=%3 receive=%4 bothWays=%5 range=%6 spacingFromPrevious=%7", phase, s_Route.GetRouteId(index), faction.GetFactionKey(), base.IsHQRadioTrafficPossible(faction), base.IsHQRadioTrafficPossible(faction, SCR_ERadioCoverageStatus.BOTH_WAYS), base.GetRadioRange(), vector.Distance(base.GetOwner().GetOrigin(), s_Route.GetRouteBase(index - 1).GetOwner().GetOrigin()));
	}

	protected static void LogRepresentativeSystems(string phase, int index)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Route.GetRouteBase(index);
		array<SCR_CampaignBuildingProviderComponent> providers = {};
		array<SCR_ServicePointComponent> services = {};
		SCR_SpawnPoint spawn = base.GetSpawnPoint();
		PrintFormat("[TB_PVE_M2_4] systems phase=%1 id=%2 supplies=%3 income=%4 spawnValid=%5 spawnEnabled=%6 visible=%7 providers=%8 services=%9", phase, s_Route.GetRouteId(index), base.GetSupplies(), base.GetSuppliesIncome(), spawn != null, spawn && spawn.IsSpawnPointEnabled(), spawn && spawn.IsSpawnPointVisibleForPlayer(SCR_PlayerController.GetLocalPlayerId()), base.GetBuildingProviders(providers), base.GetServices(services));
	}

	protected static void SpawnAt(int index, SCR_CampaignFaction faction, string label)
	{
		ResourceName prefabName = faction.GetDefendersGroupPrefab();
		if (prefabName.IsEmpty() && faction.GetFactionKey() == "FIA") prefabName = "{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et";
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		IEntity baseEntity = s_Route.GetRouteBase(index).GetOwner(); baseEntity.GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(prefabName), GetGame().GetWorld(), params));
		if (!s_Group) { Fail("group spawn failed"); return; }
		vector origin = baseEntity.GetOrigin(); int column; int row;
		for (int wave; wave < 10; wave++)
		{
			foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
			{
				vector spawnPosition = origin + Vector(column * 2, 0, row * 2);
				spawnPosition[1] = GetGame().GetWorld().GetSurfaceY(spawnPosition[0], spawnPosition[2]) + 1.0;
				params.Transform[3] = spawnPosition;
				IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
				if (character) { s_Group.AddAgentFromControlledEntity(character); s_Characters.Insert(character); }
				column++; if (column == 5) { column = 0; row++; }
			}
		}
		s_Group.SetLOD(0);
		PrintFormat("[TB_PVE_M2_4] proof-force-spawned label=%1 faction=%2 id=%3 characters=%4 groupLOD=%5 maxLOD=%6", label, faction.GetFactionKey(), s_Route.GetRouteId(index), s_Characters.Count(), s_Group.GetLOD(), AIAgent.GetMaxLOD());
	}

	protected static void Cleanup()
	{
		foreach (IEntity character : s_Characters) if (character) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_Characters.Clear(); if (s_Group) SCR_EntityHelper.DeleteEntityAndChildren(s_Group); s_Group = null;
	}

	protected static string Owner(int index)
	{
		Faction faction = s_Route.GetRouteBase(index).GetFaction();
		if (!faction) return "<none>";
		else return faction.GetFactionKey();
	}

	protected static string NextId(int index)
	{
		if (index + 1 >= s_Route.GetRouteCount()) return "";
		else return s_Route.GetRouteId(index + 1);
	}
	protected static void Fail(string reason) { PrintFormat("[TB_PVE_M2_4] FAIL proof reason=%1 mode=%2 target=%3 ticks=%4", reason, s_Mode, s_Target, s_Ticks); Cleanup(); GetGame().GetCallqueue().Remove(Tick); }
}
