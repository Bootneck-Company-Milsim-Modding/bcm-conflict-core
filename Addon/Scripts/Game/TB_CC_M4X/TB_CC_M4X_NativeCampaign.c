class TB_CC_M4X_GameModeClass : SCR_GameModeCampaignClass {}

// Service inheritance, not vanilla campaign policy inheritance. These
// overrides apply only to the isolated M4.X game-mode resource.
class TB_CC_M4X_GameMode : SCR_GameModeCampaign
{
	override protected void InitalBaseSetup()
	{
		array<string> names = { "TB_CC_PROTO_MOB", "TB_CC_PROTO_OBJ_01", "TB_CC_PROTO_OBJ_02" };
		foreach (string name : names)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(name);
			if (!entity) { PrintFormat("[TB_CC_M4X] INIT_FAILED entity=%1", name); return; }
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base) { PrintFormat("[TB_CC_M4X] INIT_FAILED base=%1", name); return; }
			base.Initialize();
			if (name == "TB_CC_PROTO_MOB") base.SetAsHQ(true);
		}
		Print("[TB_CC_M4X] service-start bases=3 physicalEnemyHQ=0 initialOwnershipWrites=0");
	}
	override protected void CheckForWinner() {} // BCM pressure owns outcomes.
	override protected void TB_PVE_M1_1_Begin() {} // Historical fixture never runs here.
}

class TB_CC_M4X_OffMapStartState : TB_CC_M4_2A_CampaignStartState
{
	override bool RequiresPhysicalOPFOREndpoint() { return false; }
	string GetOPFOROriginId() { return "PROTO::OFF_MAP_OPFOR"; }
}

class TB_CC_M4X_NativeCampaignComponentClass : TB_CC_M4_4_ArlandGraphCampaignComponentClass {}

// Transitional map adapter: reuse the tested authority/projection/pressure/M3
// lifecycle rather than fork it. The inherited class name is historical;
// the production Arland definition and world are never loaded by this adapter.
class TB_CC_M4X_NativeCampaignComponent : TB_CC_M4_4_ArlandGraphCampaignComponent
{
	override protected bool BuildRegistryAndSpatialQuery()
	{
		TB_CC_M4_1_MapDefinition definition = new TB_CC_M4_1_MapDefinition("ARLAND_NATIVE_PROTO_01", 1, "BCM native prototype");
		array<string> ids = { "PROTO::MOB", "PROTO::OBJ_01", "PROTO::OBJ_02" };
		array<string> names = { "TB_CC_PROTO_MOB", "TB_CC_PROTO_OBJ_01", "TB_CC_PROTO_OBJ_02" };
		for (int i; i < ids.Count(); i++)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(names[i]);
			if (!entity) return false;
			array<string> traits = {};
			int type = TB_CC_M4_1_PositionType.OUTPOST;
			int role = TB_CC_M4_1_EndpointRole.NONE;
			if (i == 0) { type = TB_CC_M4_1_PositionType.MOB; role = TB_CC_M4_1_EndpointRole.BLUFOR_HOME; }
			definition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition(ids[i], type, entity.GetOrigin(), names[i], 50, "PROTOTYPE", traits, i == 0, role, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE));
		}
		definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(ids[0], ids[1]));
		definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(ids[1], ids[2]));
		m_Registry = new TB_CC_M4_1_StrategicPositionRegistry();
		if (!m_Registry.Build(definition)) return false;
		m_Registry.GetPositionIds(m_aIds); m_aPositions.Clear(); m_mIdsByBase.Clear();
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			m_aPositions.Insert(resolved.GetResolvedEntity().GetOrigin());
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
			m_mIdsByBase.Set(base, id);
		}
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureRoute(m_aIds);
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureObjectivePositions(m_aPositions);
		return true;
	}
	override protected void InitializeAuthority()
	{
		if (m_bReady || !BuildRegistryAndSpatialQuery()) return;
		m_Start = new TB_CC_M4X_OffMapStartState(1, TB_CC_M4_2A_StartMode.EXPLICIT, "PROTO::MOB", "", 2);
		foreach (string id : m_aIds)
		{
			SCR_CampaignMilitaryBaseComponent base = GetBase(id);
			if (!base || !base.IsInitialized()) { FailClosed("native-base-uninitialized-" + id); return; }
		}
		m_iAuthorityProofMode = 0;
		m_bBLUFORAdvanced = false;
		m_Evaluator = new TB_CC_M4_3_StrategicEvaluator();
		m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE; m_bReady = true;
		ApplyDeploymentPolicy(); Reevaluate("native-initialization");
		StartM3Consumers();
		GetGame().GetCallqueue().CallLater(PublishPressureClock, 1000, true);
		GetGame().GetCallqueue().CallLater(ReportNativeState, 5000, true);
	}
	protected void ReportNativeState()
	{
		PrintFormat("[TB_CC_M4X] state ready=%1 revision=%2 USLegal=%3 USSRLegal=%4 depth=%5 outcome=%6 humansUS=%7 origin=PROTO::OFF_MAP_OPFOR originGraphNode=0", m_bReady, m_iRevision, GetBLUFORLegalObjective(), GetOPFORLegalObjective(), m_iDepthPermille, m_iOutcome, TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US"));
		foreach (string id : m_aIds)
		{
			SCR_CampaignMilitaryBaseComponent base = GetBase(id);
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			PrintFormat("[TB_CC_M4X] base id=%1 owner=%2 initialized=%3 HQ=%4 spawnPresent=%5 spawnEnabled=%6", id, base.GetFaction().GetFactionKey(), base.IsInitialized(), base.IsHQ(), spawn != null, spawn && spawn.IsSpawnPointEnabled());
			if (spawn && id != "PROTO::MOB")
			{
				SCR_ESpawnResult result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
				bool canReserve = spawn.CanReserveFor_S(1, result);
				PrintFormat("[TB_CC_M4X] forward-spawn-readonly id=%1 canReserve=%2 result=%3 diagnosticPlayerId=1 admittedHuman=0", id, canReserve, result);
			}
		}
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(ReportNativeState);
		super.OnDelete(owner);
	}
}
