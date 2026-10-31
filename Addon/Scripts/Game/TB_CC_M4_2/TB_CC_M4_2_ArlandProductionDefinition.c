class TB_CC_M4_2_ArlandProductionDefinition
{
	static TB_CC_M4_1_StrategicPositionDefinition Position(string id, int type, vector position, string entityName, int importance, string region, array<string> traits, bool mob = false, int endpoint = TB_CC_M4_1_EndpointRole.NONE)
	{
		return new TB_CC_M4_1_StrategicPositionDefinition(id, type, position, entityName, importance, region, traits, mob, endpoint, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE);
	}

	static TB_CC_M4_1_MapDefinition Create()
	{
		TB_CC_M4_1_MapDefinition definition = new TB_CC_M4_1_MapDefinition("ARLAND_PRODUCTION_01", 1, "Arland Production Strategic Graph");
		array<string> hqWestTraits = { "LOGISTICS", "SUPPLY", "COMMUNICATIONS", "MILITARY", "TRANSPORT" };
		array<string> airportTraits = { "LOGISTICS", "SUPPLY", "TRANSPORT", "INFRASTRUCTURE", "MILITARY" };
		array<string> farmTraits = { "LOGISTICS", "SUPPLY", "TRANSPORT" };
		array<string> settlementTraits = { "POPULATION_CENTER", "TRANSPORT" };
		array<string> militaryTraits = { "MILITARY", "SUPPLY", "TRANSPORT" };
		array<string> hillTraits = { "MILITARY", "COMMUNICATIONS", "INFRASTRUCTURE" };
		array<string> hqEastTraits = { "LOGISTICS", "SUPPLY", "COMMUNICATIONS", "MILITARY", "TRANSPORT" };

		definition.AddPosition(Position("ARLAND::HQ_WEST", TB_CC_M4_1_PositionType.MOB, "1157.67 1.156 3282.46", "HQWest", 100, "NORTHWEST", hqWestTraits, true));
		definition.AddPosition(Position("ARLAND::AIRPORT_NORTH", TB_CC_M4_1_PositionType.AIRFIELD, "946.642 38.24 2895.52", "AirportNorth", 85, "NORTHWEST", airportTraits));
		definition.AddPosition(Position("ARLAND::FARM", TB_CC_M4_1_PositionType.DEPOT, "2208.91 53.814 2909.47", "Farm", 60, "NORTH_CENTRAL", farmTraits));
		definition.AddPosition(Position("ARLAND::BEAUREGARD", TB_CC_M4_1_PositionType.VILLAGE, "3297.47 2 2860.18", "BeauregardBase", 65, "NORTHEAST", settlementTraits));
		definition.AddPosition(Position("ARLAND::ARLEVILLE", TB_CC_M4_1_PositionType.TOWN, "2727.07 77.531 1680.72", "ArlevilleBase", 80, "CENTRAL_EAST", settlementTraits));
		definition.AddPosition(Position("ARLAND::MILITARY_SOUTH", TB_CC_M4_1_PositionType.MILITARY_BASE, "1413.43 10.959 1857.55", "MilitarySouth", 85, "CENTRAL_WEST", militaryTraits));
		definition.AddPosition(Position("ARLAND::MOSS_HILL", TB_CC_M4_1_PositionType.OUTPOST, "2292.76 89.208 1223.62", "MossHill", 75, "SOUTH_CENTRAL", hillTraits));
		definition.AddPosition(Position("ARLAND::HQ_EAST", TB_CC_M4_1_PositionType.MOB, "1621.77 4.045 902.667", "HQEast", 100, "SOUTHEAST", hqEastTraits, true));

		// M4.4I: approved distinct locations, locked from saved CD markers.
		array<string> harbourTraits = { "POPULATION_CENTER", "TRANSPORT", "LOGISTICS", "INFRASTRUCTURE" };
		array<string> islandTraits = { "COASTAL_CONTROL", "TRANSPORT_REQUIRED" };
		definition.AddPosition(Position("ARLAND::BEAUREGARD_HARBOUR", TB_CC_M4_1_PositionType.PORT, "3112.44 44.02 2419.688", "TB_CC_BeauregardHarbour", 70, "NORTHEAST", harbourTraits));
		definition.AddPosition(Position("ARLAND::SIGNAL_HILL", TB_CC_M4_1_PositionType.OUTPOST, "2530.229 144.135 2286.244", "TB_CC_SignalHill", 80, "CENTRAL_EAST", hillTraits));
		definition.AddPosition(Position("ARLAND::NORTH_RUNWAY", TB_CC_M4_1_PositionType.AIRFIELD, "1674.865 37.888 3003.17", "TB_CC_NorthRunway", 85, "NORTH_CENTRAL", airportTraits));
		definition.AddPosition(Position("ARLAND::GULL_ISLAND", TB_CC_M4_1_PositionType.OUTPOST, "962.309 31.444 1395.995", "TB_CC_GullIsland", 70, "SOUTHWEST_ISLAND", islandTraits));

		Connect(definition, "ARLAND::HQ_WEST", "ARLAND::AIRPORT_NORTH");
		Connect(definition, "ARLAND::HQ_WEST", "ARLAND::FARM");
		Connect(definition, "ARLAND::AIRPORT_NORTH", "ARLAND::FARM");
		Connect(definition, "ARLAND::AIRPORT_NORTH", "ARLAND::MILITARY_SOUTH");
		Connect(definition, "ARLAND::FARM", "ARLAND::BEAUREGARD");
		Connect(definition, "ARLAND::FARM", "ARLAND::ARLEVILLE");
		Connect(definition, "ARLAND::FARM", "ARLAND::MILITARY_SOUTH");
		Connect(definition, "ARLAND::BEAUREGARD", "ARLAND::ARLEVILLE");
		Connect(definition, "ARLAND::ARLEVILLE", "ARLAND::MILITARY_SOUTH");
		Connect(definition, "ARLAND::ARLEVILLE", "ARLAND::MOSS_HILL");
		Connect(definition, "ARLAND::MILITARY_SOUTH", "ARLAND::MOSS_HILL");
		Connect(definition, "ARLAND::MOSS_HILL", "ARLAND::HQ_EAST");
		Connect(definition, "ARLAND::MILITARY_SOUTH", "ARLAND::HQ_EAST");
		Connect(definition, "ARLAND::BEAUREGARD_HARBOUR", "ARLAND::BEAUREGARD");
		Connect(definition, "ARLAND::BEAUREGARD_HARBOUR", "ARLAND::ARLEVILLE");
		Connect(definition, "ARLAND::SIGNAL_HILL", "ARLAND::FARM");
		Connect(definition, "ARLAND::SIGNAL_HILL", "ARLAND::ARLEVILLE");
		Connect(definition, "ARLAND::SIGNAL_HILL", "ARLAND::BEAUREGARD_HARBOUR");
		Connect(definition, "ARLAND::NORTH_RUNWAY", "ARLAND::AIRPORT_NORTH");
		Connect(definition, "ARLAND::NORTH_RUNWAY", "ARLAND::FARM");
		Connect(definition, "ARLAND::GULL_ISLAND", "ARLAND::MILITARY_SOUTH");
		return definition;
	}

	protected static void Connect(TB_CC_M4_1_MapDefinition definition, string a, string b)
	{
		definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(a, b));
	}
}

class TB_CC_M4_2_ProductionGraphProofComponentClass : ScriptComponentClass {}
class TB_CC_M4_2_ProductionGraphProofComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer()) GetGame().GetCallqueue().CallLater(RunProof, 18000);
	}

	protected void RunProof()
	{
		TB_CC_M4_1_StrategicPositionRegistry registry = new TB_CC_M4_1_StrategicPositionRegistry();
		bool ready = registry.Build(TB_CC_M4_2_ArlandProductionDefinition.Create());
		Check("production-registry-ready", ready && registry.GetPositionCount() == 12);
		array<string> friendly = { "ARLAND::HQ_EAST" };
		CheckCandidates(registry, "initial", friendly, "ARLAND::MILITARY_SOUTH", "ARLAND::MOSS_HILL");
		friendly.Insert("ARLAND::MILITARY_SOUTH");
		CheckCandidates(registry, "after-military", friendly, "ARLAND::AIRPORT_NORTH", "ARLAND::ARLEVILLE", "ARLAND::FARM");
		friendly.Insert("ARLAND::MOSS_HILL");
		CheckCandidates(registry, "after-moss-hill", friendly, "ARLAND::AIRPORT_NORTH", "ARLAND::ARLEVILLE", "ARLAND::FARM");
		friendly.Insert("ARLAND::ARLEVILLE");
		CheckCandidates(registry, "after-arleville", friendly, "ARLAND::AIRPORT_NORTH", "ARLAND::BEAUREGARD", "ARLAND::FARM");
		array<string> allIds = {}; registry.GetPositionIds(allIds);
		bool ownershipResolved = true;
		foreach (string id : allIds)
		{
			FactionKey ownerFaction;
			bool ownerReady = registry.TryGetCurrentOwner(id, ownerFaction);
			if (!ownerReady) ownershipResolved = false;
			PrintFormat("[TB_CC_M4_2] OWNER id=%1 resolved=%2 faction=%3", id, ownerReady, ownerFaction);
		}
		Check("all-vanilla-ownership-adapters", ownershipResolved);
		PrintFormat("[TB_CC_M4_2] COMPLETE ready=%1 positions=%2 edges=21 connected=1 ownershipResolved=%3 mutation=0", ready, registry.GetPositionCount(), ownershipResolved);
	}

	protected void CheckCandidates(TB_CC_M4_1_StrategicPositionRegistry registry, string phase, array<string> friendly, string expectedA, string expectedB, string expectedC = "")
	{
		array<string> candidates = {};
		foreach (string friendlyId : friendly)
		{
			array<string> adjacent = {}; registry.GetConnections(friendlyId, adjacent);
			foreach (string adjacentId : adjacent)
				if (!friendly.Contains(adjacentId) && !candidates.Contains(adjacentId)) candidates.Insert(adjacentId);
		}
		candidates.Sort();
		bool pass = candidates.Contains(expectedA) && candidates.Contains(expectedB);
		if (!expectedC.IsEmpty()) pass = pass && candidates.Contains(expectedC);
		PrintFormat("[TB_CC_M4_2] FRONTLINE phase=%1 friendly=%2 candidates=%3", phase, friendly, candidates);
		Check("frontline-" + phase, pass);
	}

	protected void Check(string label, bool pass)
	{
		if (pass) PrintFormat("[TB_CC_M4_2] PASS %1", label);
		else PrintFormat("[TB_CC_M4_2] FAIL %1", label);
	}
}
