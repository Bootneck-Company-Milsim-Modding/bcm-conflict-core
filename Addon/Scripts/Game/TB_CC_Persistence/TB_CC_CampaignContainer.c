// Versioned campaign root. Player progression uses the compatible v2 reserved section.
class TB_CC_CampaignContainer
{
	static const int SCHEMA_VERSION = 2;
	int schemaVersion = -1;
	string campaignId;
	string mapIdentity;
	string scenarioIdentity;
	string campaignConfigIdentity;
	int campaignRevision = -1;
	int createdTimestamp = -1;
	int lastSavedTimestamp = -1;
	ref map<string, string> campaignState = new map<string, string>();
	ref array<string> strategicPositions = {};
	ref map<string, string> objectiveState = new map<string, string>();
	ref map<string, string> hqState = new map<string, string>();
	ref map<string, string> pressureState = new map<string, string>();
	ref map<string, string> factionState = new map<string, string>();
	ref array<string> operations = {};
	ref array<string> players = {};
	bool snapshotPresent;
	ref map<string, string> positionOwners = new map<string, string>();

	bool Write(string path)
	{
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("schemaVersion", schemaVersion)) return false;
		if (!context.WriteValue("campaignId", campaignId)) return false;
		if (!context.WriteValue("mapIdentity", mapIdentity)) return false;
		if (!context.WriteValue("scenarioIdentity", scenarioIdentity)) return false;
		if (!context.WriteValue("campaignConfigIdentity", campaignConfigIdentity)) return false;
		if (!context.WriteValue("campaignRevision", campaignRevision)) return false;
		if (!context.WriteValue("createdTimestamp", createdTimestamp)) return false;
		if (!context.WriteValue("lastSavedTimestamp", lastSavedTimestamp)) return false;
		if (!context.WriteValue("campaignState", campaignState)) return false;
		if (!context.WriteValue("strategicPositions", strategicPositions)) return false;
		if (!context.WriteValue("objectiveState", objectiveState)) return false;
		if (!context.WriteValue("hqState", hqState)) return false;
		if (!context.WriteValue("pressureState", pressureState)) return false;
		if (!context.WriteValue("factionState", factionState)) return false;
		if (!context.WriteValue("operations", operations)) return false;
		if (!context.WriteValue("players", players)) return false;
		if (!context.WriteValue("snapshotPresent", snapshotPresent)) return false;
		if (!context.WriteValue("positionOwners", positionOwners)) return false;
		return context.SaveToFile(path);
	}

	bool Read(string path)
	{
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path)) return false;
		if (!context.ReadValue("schemaVersion", schemaVersion)) return false;
		if (!context.ReadValue("campaignId", campaignId)) return false;
		if (!context.ReadValue("mapIdentity", mapIdentity)) return false;
		if (!context.ReadValue("scenarioIdentity", scenarioIdentity)) return false;
		if (!context.ReadValue("campaignConfigIdentity", campaignConfigIdentity)) return false;
		if (!context.ReadValue("campaignRevision", campaignRevision)) return false;
		if (!context.ReadValue("createdTimestamp", createdTimestamp)) return false;
		if (!context.ReadValue("lastSavedTimestamp", lastSavedTimestamp)) return false;
		if (!context.ReadValue("campaignState", campaignState)) return false;
		if (!context.ReadValue("strategicPositions", strategicPositions)) return false;
		if (!context.ReadValue("objectiveState", objectiveState)) return false;
		if (!context.ReadValue("hqState", hqState)) return false;
		if (!context.ReadValue("pressureState", pressureState)) return false;
		if (!context.ReadValue("factionState", factionState)) return false;
		if (!context.ReadValue("operations", operations)) return false;
		if (!context.ReadValue("players", players)) return false;
		if (schemaVersion == SCHEMA_VERSION)
		{
			if (!context.ReadValue("snapshotPresent", snapshotPresent)) return false;
			if (!context.ReadValue("positionOwners", positionOwners)) return false;
		}
		return true;
	}

	string Validate(string expectedMap, string expectedScenario, string expectedConfig)
	{
		if (schemaVersion != 1 && schemaVersion != SCHEMA_VERSION) return string.Format("unsupported-schema saved=%1 expected=1-or-%2", schemaVersion, SCHEMA_VERSION);
		UUID id = campaignId;
		if (!UUID.IsUUID(campaignId) || id.IsNull()) return "invalid-campaign-id";
		if (mapIdentity != expectedMap) return string.Format("map-mismatch saved=%1 expected=%2", mapIdentity, expectedMap);
		if (scenarioIdentity != expectedScenario) return string.Format("scenario-mismatch saved=%1 expected=%2", scenarioIdentity, expectedScenario);
		if (campaignConfigIdentity != expectedConfig) return string.Format("config-mismatch saved=%1 expected=%2", campaignConfigIdentity, expectedConfig);
		if (campaignRevision < 0 || createdTimestamp < 0 || lastSavedTimestamp < createdTimestamp) return "invalid-revision-or-timestamps";
		if (!campaignState || !strategicPositions || !objectiveState || !hqState || !pressureState || !factionState || !operations || !players) return "missing-reserved-container";
		if (!positionOwners) return "missing-position-owners";
		// v1 had an explicit empty-only contract; only that legacy form is valid.
		if (schemaVersion == 1 || !snapshotPresent)
		{
			if (campaignState.Count() || strategicPositions.Count() || objectiveState.Count() || hqState.Count() || pressureState.Count() || factionState.Count() || operations.Count() || players.Count() || positionOwners.Count() || snapshotPresent) return "invalid-no-snapshot-payload";
			return "";
		}
		if (operations.Count()) return "later-phase-payload-not-supported";
		if (!TB_CC_PlayerProgression.Validate(players)) return "invalid-player-progression";
		if (strategicPositions.IsEmpty() || positionOwners.Count() != strategicPositions.Count()) return "invalid-strategic-position-count";
		array<string> seen = {};
		foreach (string stableId : strategicPositions)
		{
			if (stableId.IsEmpty() || seen.Contains(stableId) || !positionOwners.Contains(stableId)) return "invalid-or-duplicate-strategic-id";
			seen.Insert(stableId);
			string faction = positionOwners.Get(stableId);
			if (faction != "US" && faction != "USSR" && faction != "FIA") return "unsupported-strategic-owner";
		}
		return "";
	}
}
