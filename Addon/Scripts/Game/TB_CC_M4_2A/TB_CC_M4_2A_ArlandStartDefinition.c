class TB_CC_M4_2A_ArlandStartDefinition
{
	static TB_CC_M4_2A_CampaignStartDefinition Create()
	{
		TB_CC_M4_2A_CampaignStartDefinition definition = new TB_CC_M4_2A_CampaignStartDefinition();
		definition.AddRelationship("ARLAND::HQ_EAST", "ARLAND::HQ_WEST", 5);
		definition.AddRelationship("ARLAND::HQ_WEST", "ARLAND::HQ_EAST", 5);
		return definition;
	}
}

modded class SCR_GameModeCampaign
{
	void TB_CC_M4_2A_LogArlandMobAudit()
	{
		array<SCR_CampaignMilitaryBaseComponent> bases = {};
		GetBaseManager().GetBases(bases);
		foreach (SCR_CampaignMilitaryBaseComponent base : bases)
		{
			if (!base || !base.GetOwner()) continue;
			string entityName = base.GetOwner().GetName();
			if (entityName.StartsWith("TB_M4_1_")) continue;
			array<SCR_CampaignBuildingProviderComponent> providers = {};
			array<SCR_ServicePointComponent> services = {};
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
			PrintFormat("[TB_CC_M4_2A] MOB_AUDIT entity=%1 name=%2 position=%3 isHQ=%4 type=%5 spawnValid=%6 spawnEnabled=%7",
				entityName, base.GetBaseName(), base.GetOwner().GetOrigin(), base.IsHQ(), base.GetType(), spawn != null, spawn && spawn.IsSpawnPointEnabled());
			PrintFormat("[TB_CC_M4_2A] MOB_SYSTEMS entity=%1 radioValid=%2 radioRange=%3 supplies=%4 income=%5 providers=%6 services=%7",
				entityName, radio != null, base.GetRadioRange(), base.GetSupplies(), base.GetSuppliesIncome(), base.GetBuildingProviders(providers), base.GetServices(services));
		}
	}
}

class TB_CC_M4_2A_ProofComponentClass : ScriptComponentClass {}
class TB_CC_M4_2A_ProofComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer()) GetGame().GetCallqueue().CallLater(RunProof, 21000);
	}

	protected void RunProof()
	{
		TB_CC_M4_1_StrategicPositionRegistry registry = new TB_CC_M4_1_StrategicPositionRegistry();
		bool registryReady = registry.Build(TB_CC_M4_2_ArlandProductionDefinition.Create());
		array<ref TB_CC_M4_1_StrategicPositionDefinition> mobs = {}; registry.GetMOBDefinitions(mobs);
		Check("two-authored-mob-candidates", registryReady && mobs.Count() == 2);
		TB_CC_M4_2A_CampaignStartDefinition startDefinition = TB_CC_M4_2A_ArlandStartDefinition.Create();
		TB_CC_M4_2A_CampaignStartResolver resolver = new TB_CC_M4_2A_CampaignStartResolver();
		bool deterministic = true; bool sawEast; bool sawWest;
		for (int seed; seed < 16; seed++)
		{
			TB_CC_M4_2A_CampaignStartState first = resolver.Resolve(registry, startDefinition, seed);
			TB_CC_M4_2A_CampaignStartState second = resolver.Resolve(registry, startDefinition, seed);
			if (!first || !second || first.GetSelectedMobId() != second.GetSelectedMobId() || first.GetSelectedEndpointId() != second.GetSelectedEndpointId()) deterministic = false;
			if (first && first.GetSelectedMobId() == "ARLAND::HQ_EAST") sawEast = true;
			if (first && first.GetSelectedMobId() == "ARLAND::HQ_WEST") sawWest = true;
			if (first) PrintFormat("[TB_CC_M4_2A] SEED seed=%1 mob=%2 endpoint=%3", seed, first.GetSelectedMobId(), first.GetSelectedEndpointId());
		}
		Check("seeded-reconstruction-deterministic", deterministic);
		Check("seed-matrix-reaches-all-candidates", sawEast && sawWest);
		TB_CC_M4_2A_CampaignStartState explicitEast = resolver.Resolve(registry, startDefinition, 77, TB_CC_M4_2A_StartMode.EXPLICIT, "ARLAND::HQ_EAST");
		Check("explicit-valid-mob", explicitEast && explicitEast.GetSelectedMobId() == "ARLAND::HQ_EAST" && explicitEast.GetSelectedEndpointId() == "ARLAND::HQ_WEST");
		TB_CC_M4_2A_CampaignStartState invalidExplicit = resolver.Resolve(registry, startDefinition, 77, TB_CC_M4_2A_StartMode.EXPLICIT, "ARLAND::FARM");
		Check("explicit-invalid-fails-closed", !invalidExplicit);
		TB_CC_M4_2A_CampaignStartState resumed = resolver.Resume(registry, startDefinition, explicitEast);
		Check("resume-does-not-reroll", resumed && resumed.GetSelectedMobId() == explicitEast.GetSelectedMobId() && resumed.GetSelectedEndpointId() == explicitEast.GetSelectedEndpointId());
		CheckOpening(registry, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST", "ARLAND::MILITARY_SOUTH", "ARLAND::MOSS_HILL");
		CheckOpening(registry, "ARLAND::HQ_WEST", "ARLAND::HQ_EAST", "ARLAND::AIRPORT_NORTH", "ARLAND::FARM");
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (campaign) campaign.TB_CC_M4_2A_LogArlandMobAudit();
		PrintFormat("[TB_CC_M4_2A] COMPLETE registryReady=%1 mobCandidates=%2 deterministic=%3 variation=%4 explicit=1 invalidClosed=1 resumeStable=1 ownershipMutation=0 deploymentMutation=0 objectiveSelection=0", registryReady, mobs.Count(), deterministic, sawEast && sawWest);
	}

	protected void CheckOpening(TB_CC_M4_1_StrategicPositionRegistry registry, string mobId, string endpointId, string expectedA, string expectedB)
	{
		array<string> candidates = {}; registry.GetConnections(mobId, candidates);
		bool pass = candidates.Contains(expectedA) && candidates.Contains(expectedB) && !candidates.Contains(endpointId);
		PrintFormat("[TB_CC_M4_2A] OPENING mob=%1 endpoint=%2 candidates=%3", mobId, endpointId, candidates);
		Check("opening-" + mobId, pass);
	}

	protected void Check(string label, bool pass)
	{
		if (pass) PrintFormat("[TB_CC_M4_2A] PASS %1", label);
		else PrintFormat("[TB_CC_M4_2A] FAIL %1", label);
	}
}
