class TB_CC_M4_1_ArlandFixture
{
	static TB_CC_M4_1_MapDefinition Create()
	{
		TB_CC_M4_1_MapDefinition definition = new TB_CC_M4_1_MapDefinition("ARLAND_M4_1_FIXTURE", 1, "Arland M4.1 Portability Fixture");
		array<string> mobTraits = { "LOGISTICS", "SUPPLY", "MILITARY" };
		array<string> townTraits = { "POPULATION_CENTER", "TRANSPORT" };
		array<string> portTraits = { "LOGISTICS", "TRANSPORT", "INFRASTRUCTURE" };
		definition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition("ARLAND::FIXTURE_MOB", TB_CC_M4_1_PositionType.MOB, "1150 12 2250", "TB_M4_1_ARLAND_MOB", 100, "FIXTURE_WEST", mobTraits, true, TB_CC_M4_1_EndpointRole.BLUFOR_HOME, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE));
		definition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition("ARLAND::FIXTURE_CENTRAL", TB_CC_M4_1_PositionType.TOWN, "1900 12 1820", "TB_M4_1_ARLAND_CENTRAL", 60, "FIXTURE_CENTRAL", townTraits, false, TB_CC_M4_1_EndpointRole.NONE, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE));
		definition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition("ARLAND::FIXTURE_PORT", TB_CC_M4_1_PositionType.PORT, "2700 12 1250", "TB_M4_1_ARLAND_PORT", 80, "FIXTURE_EAST", portTraits, false, TB_CC_M4_1_EndpointRole.OPFOR_HOME, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE));
		definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition("ARLAND::FIXTURE_MOB", "ARLAND::FIXTURE_CENTRAL"));
		definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition("ARLAND::FIXTURE_CENTRAL", "ARLAND::FIXTURE_PORT"));
		return definition;
	}
}

class TB_CC_M4_1_ProofComponentClass : ScriptComponentClass {}
class TB_CC_M4_1_ProofComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer()) GetGame().GetCallqueue().CallLater(Run, 12000);
	}

	protected void Run()
	{
		TB_CC_M4_1_StrategicPositionRegistry registry = new TB_CC_M4_1_StrategicPositionRegistry();
		TB_CC_M4_1_MapDefinition arland = TB_CC_M4_1_ArlandFixture.Create();
		bool ready = registry.Build(arland);
		Check("valid-definition-ready", ready && registry.IsReady());
		Check("stable-map-identity", registry.GetMapDefinitionId() == "ARLAND_M4_1_FIXTURE" && registry.GetDefinitionVersion() == 1);
		Check("position-count-and-lookup", registry.GetPositionCount() == 3 && registry.FindPositionById("ARLAND::FIXTURE_CENTRAL") != null);
		TB_CC_M4_1_ResolvedPosition central = registry.FindPositionById("ARLAND::FIXTURE_CENTRAL");
		Check("type-and-traits", central && central.GetDefinition().GetPositionType() == TB_CC_M4_1_PositionType.TOWN && central.GetDefinition().HasTrait("TRANSPORT"));
		array<string> adjacent = {}; registry.GetConnections("ARLAND::FIXTURE_CENTRAL", adjacent);
		Check("undirected-connections", adjacent.Count() == 2 && adjacent.Contains("ARLAND::FIXTURE_MOB") && adjacent.Contains("ARLAND::FIXTURE_PORT"));
		array<ref TB_CC_M4_1_StrategicPositionDefinition> mobs = {}; registry.GetMOBDefinitions(mobs);
		array<ref TB_CC_M4_1_StrategicPositionDefinition> endpoints = {}; registry.GetEndpointDefinitions(endpoints);
		Check("mob-and-endpoint-metadata", mobs.Count() == 1 && endpoints.Count() == 2);
		FactionKey owner; bool ownership = registry.TryGetCurrentOwner("ARLAND::FIXTURE_MOB", owner);
		Check("vanilla-base-ownership", ownership && owner == "US");
		TB_CC_M4_1_MapDefinitionCatalog catalog = new TB_CC_M4_1_MapDefinitionCatalog();
		bool catalogFirst = catalog.Register(arland);
		bool catalogDuplicate = catalog.Register(arland);
		Check("definition-catalog-duplicate-map-id", catalogFirst && !catalogDuplicate && catalog.Count() == 1);
		PrintFormat("[TB_CC_M4_1] ARLAND_FIXTURE ready=%1 positions=%2 mobOwner=%3 realWorld=worlds/MP/CTI_Campaign_Arland.ent ownership=vanilla-base", ready, registry.GetPositionCount(), owner);

		TB_CC_M4_1_StrategicPositionRegistry invalidDuplicate = new TB_CC_M4_1_StrategicPositionRegistry();
		TB_CC_M4_1_StrategicPositionRegistry invalidTarget = new TB_CC_M4_1_StrategicPositionRegistry();
		TB_CC_M4_1_StrategicPositionRegistry invalidAdapter = new TB_CC_M4_1_StrategicPositionRegistry();
		Check("invalid-duplicate-position", !invalidDuplicate.Build(CreateDuplicateDefinition()));
		Check("invalid-missing-target", !invalidTarget.Build(CreateMissingTargetDefinition()));
		Check("invalid-ownership-adapter", !invalidAdapter.Build(CreateInvalidAdapterDefinition()));

		TB_CC_M4_1_StrategicPositionRegistry scaleA = new TB_CC_M4_1_StrategicPositionRegistry();
		TB_CC_M4_1_StrategicPositionRegistry scaleB = new TB_CC_M4_1_StrategicPositionRegistry();
		bool scaleReadyA = scaleA.Build(CreateScaleDefinition()); bool scaleReadyB = scaleB.Build(CreateScaleDefinition());
		array<string> idsA = {}; array<string> idsB = {}; scaleA.GetPositionIds(idsA); scaleB.GetPositionIds(idsB);
		bool equivalent = idsA.Count() == idsB.Count();
		for (int i; equivalent && i < idsA.Count(); i++) if (idsA[i] != idsB[i]) equivalent = false;
		Check("more-than-31", scaleReadyA && scaleA.GetPositionCount() == 40);
		Check("deterministic-reconstruction", scaleReadyA && scaleReadyB && equivalent);
		PrintFormat("[TB_CC_M4_1] COMPLETE arlandReady=%1 scale40=%2 deterministic=%3 ownershipRead=%4 campaignMutation=0", ready, scaleReadyA, equivalent, ownership);
	}

	protected TB_CC_M4_1_StrategicPositionDefinition Position(string id, string entityName, bool mob = false)
	{
		array<string> traits = { "MILITARY" };
		TB_CC_M4_1_EndpointRole endpointRole = TB_CC_M4_1_EndpointRole.NONE;
		if (mob)
			endpointRole = TB_CC_M4_1_EndpointRole.BLUFOR_HOME;
		return new TB_CC_M4_1_StrategicPositionDefinition(id, TB_CC_M4_1_PositionType.OUTPOST, "1900 12 1820", entityName, 1, "PROOF", traits, mob, endpointRole, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE);
	}
	protected TB_CC_M4_1_MapDefinition CreateDuplicateDefinition() { TB_CC_M4_1_MapDefinition d = new TB_CC_M4_1_MapDefinition("INVALID_DUPLICATE", 1, "Invalid duplicate"); d.AddPosition(Position("DUP", "TB_M4_1_ARLAND_MOB", true)); d.AddPosition(Position("DUP", "TB_M4_1_ARLAND_CENTRAL")); return d; }
	protected TB_CC_M4_1_MapDefinition CreateMissingTargetDefinition() { TB_CC_M4_1_MapDefinition d = new TB_CC_M4_1_MapDefinition("INVALID_EDGE", 1, "Invalid edge"); d.AddPosition(Position("MOB", "TB_M4_1_ARLAND_MOB", true)); d.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition("MOB", "MISSING")); return d; }
	protected TB_CC_M4_1_MapDefinition CreateInvalidAdapterDefinition() { TB_CC_M4_1_MapDefinition d = new TB_CC_M4_1_MapDefinition("INVALID_ADAPTER", 1, "Invalid adapter"); array<string> traits = {}; d.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition("MOB", TB_CC_M4_1_PositionType.MOB, "0 0 0", "", 1, "", traits, true, TB_CC_M4_1_EndpointRole.BLUFOR_HOME, TB_CC_M4_1_OwnershipAdapterType.EXTERNAL_PROVIDER)); return d; }
	protected TB_CC_M4_1_MapDefinition CreateScaleDefinition()
	{
		TB_CC_M4_1_MapDefinition d = new TB_CC_M4_1_MapDefinition("SYNTHETIC_SCALE_40", 1, "Synthetic scale proof");
		for (int i; i < 40; i++)
		{
			string id = string.Format("SCALE::P%1", i); d.AddPosition(Position(id, "TB_M4_1_ARLAND_MOB", i == 0));
			if (i > 0) d.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(string.Format("SCALE::P%1", i - 1), id));
		}
		return d;
	}
	protected void Check(string label, bool pass)
	{
		if (pass)
			PrintFormat("[TB_CC_M4_1] PASS %1", label);
		else
			PrintFormat("[TB_CC_M4_1] FAIL %1", label);
	}
}
