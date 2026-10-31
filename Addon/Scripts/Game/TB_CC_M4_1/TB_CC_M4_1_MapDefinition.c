enum TB_CC_M4_1_PositionType
{
	MOB, TOWN, VILLAGE, MILITARY_BASE, OUTPOST, DEPOT, PORT, AIRFIELD,
	INDUSTRIAL, COMMUNICATIONS, INFRASTRUCTURE, CROSSROADS, BRIDGE, OTHER
}

enum TB_CC_M4_1_EndpointRole { NONE, BLUFOR_HOME, OPFOR_HOME }
enum TB_CC_M4_1_OwnershipAdapterType { NONE, VANILLA_CONFLICT_BASE, EXTERNAL_PROVIDER }

class TB_CC_M4_1_StrategicPositionDefinition
{
	protected string m_sId;
	protected int m_iType;
	protected vector m_vPosition;
	protected string m_sEntityName;
	protected int m_iImportance;
	protected string m_sRegion;
	protected ref array<string> m_aTraits = {};
	protected bool m_bMobCandidate;
	protected int m_iEndpointRole;
	protected int m_iOwnershipAdapterType;

	void TB_CC_M4_1_StrategicPositionDefinition(string id, int type, vector position, string entityName, int importance, string region, array<string> traits, bool mobCandidate, int endpointRole, int adapterType)
	{
		m_sId = id; m_iType = type; m_vPosition = position; m_sEntityName = entityName; m_iImportance = importance; m_sRegion = region;
		if (traits) foreach (string trait : traits) m_aTraits.Insert(trait);
		m_bMobCandidate = mobCandidate; m_iEndpointRole = endpointRole; m_iOwnershipAdapterType = adapterType;
	}

	string GetId() { return m_sId; }
	int GetPositionType() { return m_iType; }
	vector GetAuthoredPosition() { return m_vPosition; }
	string GetEntityName() { return m_sEntityName; }
	int GetStrategicImportance() { return m_iImportance; }
	string GetRegion() { return m_sRegion; }
	bool IsMOBCandidate() { return m_bMobCandidate; }
	int GetEndpointRole() { return m_iEndpointRole; }
	int GetOwnershipAdapterType() { return m_iOwnershipAdapterType; }
	void GetTraits(out array<string> traits) { traits.Clear(); foreach (string trait : m_aTraits) traits.Insert(trait); }
	bool HasTrait(string trait) { return m_aTraits.Contains(trait); }
}

class TB_CC_M4_1_StrategicConnectionDefinition
{
	protected string m_sA;
	protected string m_sB;
	void TB_CC_M4_1_StrategicConnectionDefinition(string a, string b) { m_sA = a; m_sB = b; }
	string GetA() { return m_sA; }
	string GetB() { return m_sB; }
}

class TB_CC_M4_1_MapDefinition
{
	protected string m_sMapId;
	protected int m_iSchemaVersion;
	protected string m_sDisplayName;
	protected ref array<ref TB_CC_M4_1_StrategicPositionDefinition> m_aPositions = {};
	protected ref array<ref TB_CC_M4_1_StrategicConnectionDefinition> m_aConnections = {};

	void TB_CC_M4_1_MapDefinition(string mapId, int schemaVersion, string displayName) { m_sMapId = mapId; m_iSchemaVersion = schemaVersion; m_sDisplayName = displayName; }
	string GetMapId() { return m_sMapId; }
	int GetSchemaVersion() { return m_iSchemaVersion; }
	string GetDisplayName() { return m_sDisplayName; }
	void AddPosition(TB_CC_M4_1_StrategicPositionDefinition position) { m_aPositions.Insert(position); }
	void AddConnection(TB_CC_M4_1_StrategicConnectionDefinition connection) { m_aConnections.Insert(connection); }
	void GetPositions(out array<ref TB_CC_M4_1_StrategicPositionDefinition> positions) { positions.Clear(); foreach (TB_CC_M4_1_StrategicPositionDefinition position : m_aPositions) positions.Insert(position); }
	void GetConnections(out array<ref TB_CC_M4_1_StrategicConnectionDefinition> connections) { connections.Clear(); foreach (TB_CC_M4_1_StrategicConnectionDefinition connection : m_aConnections) connections.Insert(connection); }
}

// Addon-level definition catalog. It validates map identity independently of
// any one world's runtime registry so multiple map definitions can coexist.
class TB_CC_M4_1_MapDefinitionCatalog
{
	protected ref map<string, ref TB_CC_M4_1_MapDefinition> m_mDefinitions = new map<string, ref TB_CC_M4_1_MapDefinition>();
	protected string m_sFailure;

	bool Register(TB_CC_M4_1_MapDefinition definition)
	{
		if (!definition || definition.GetMapId().IsEmpty())
		{
			m_sFailure = "map=<none> position=<none> field=mapId problem=empty";
			return false;
		}
		string mapId = definition.GetMapId();
		if (m_mDefinitions.Contains(mapId))
		{
			m_sFailure = string.Format("map=%1 position=<none> field=mapId problem=duplicate", mapId);
			return false;
		}
		m_mDefinitions.Set(mapId, definition);
		return true;
	}

	TB_CC_M4_1_MapDefinition Find(string mapId)
	{
		if (!m_mDefinitions.Contains(mapId))
			return null;
		return m_mDefinitions.Get(mapId);
	}
	int Count() { return m_mDefinitions.Count(); }
	string GetFailure() { return m_sFailure; }
}
