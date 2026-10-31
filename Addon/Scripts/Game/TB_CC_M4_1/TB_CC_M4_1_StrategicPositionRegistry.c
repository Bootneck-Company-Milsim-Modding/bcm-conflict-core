class TB_CC_M4_1_OwnershipAdapter
{
	bool TryGetCurrentOwner(out FactionKey owner) { return false; }
}

class TB_CC_M4_1_VanillaBaseOwnershipAdapter : TB_CC_M4_1_OwnershipAdapter
{
	protected SCR_CampaignMilitaryBaseComponent m_Base;
	void TB_CC_M4_1_VanillaBaseOwnershipAdapter(SCR_CampaignMilitaryBaseComponent base) { m_Base = base; }
	override bool TryGetCurrentOwner(out FactionKey owner)
	{
		if (!m_Base || !m_Base.GetFaction()) return false;
		owner = m_Base.GetFaction().GetFactionKey(); return !owner.IsEmpty();
	}
}

class TB_CC_M4_1_ResolvedPosition
{
	protected ref TB_CC_M4_1_StrategicPositionDefinition m_Definition;
	protected IEntity m_Entity;
	protected ref TB_CC_M4_1_OwnershipAdapter m_Ownership;
	void TB_CC_M4_1_ResolvedPosition(TB_CC_M4_1_StrategicPositionDefinition definition, IEntity entity, TB_CC_M4_1_OwnershipAdapter ownership) { m_Definition = definition; m_Entity = entity; m_Ownership = ownership; }
	TB_CC_M4_1_StrategicPositionDefinition GetDefinition() { return m_Definition; }
	IEntity GetResolvedEntity() { return m_Entity; }
	bool TryGetCurrentOwner(out FactionKey owner) { return m_Ownership && m_Ownership.TryGetCurrentOwner(owner); }
}

class TB_CC_M4_1_StrategicPositionRegistry
{
	static const int SUPPORTED_SCHEMA = 1;
	protected bool m_bReady;
	protected string m_sMapId;
	protected int m_iVersion;
	protected string m_sDisplayName;
	protected string m_sFailure;
	protected ref map<string, ref TB_CC_M4_1_ResolvedPosition> m_mPositions = new map<string, ref TB_CC_M4_1_ResolvedPosition>();
	protected ref map<string, ref array<string>> m_mConnections = new map<string, ref array<string>>();
	protected ref array<string> m_aOrderedIds = {};

	bool Build(TB_CC_M4_1_MapDefinition definition)
	{
		Reset();
		if (!definition) return Fail("<none>", "definition", "null-definition");
		m_sMapId = definition.GetMapId(); m_iVersion = definition.GetSchemaVersion(); m_sDisplayName = definition.GetDisplayName();
		if (m_sMapId.IsEmpty()) return Fail("<map>", "mapId", "empty");
		if (m_iVersion != SUPPORTED_SCHEMA) return Fail("<map>", "schemaVersion", "unsupported");
		if (m_sDisplayName.IsEmpty()) return Fail("<map>", "displayName", "empty");
		array<ref TB_CC_M4_1_StrategicPositionDefinition> positions = {}; definition.GetPositions(positions);
		int mobCount; int bluforEndpointCount; int opforEndpointCount;
		foreach (TB_CC_M4_1_StrategicPositionDefinition position : positions)
		{
			if (!position || position.GetId().IsEmpty()) return Fail("<position>", "id", "empty");
			string id = position.GetId();
			if (m_mPositions.Contains(id)) return Fail(id, "id", "duplicate");
			if (position.GetStrategicImportance() < 0) return Fail(id, "importance", "negative");
			IEntity entity; TB_CC_M4_1_OwnershipAdapter adapter;
			int adapterType = position.GetOwnershipAdapterType();
			if (adapterType == TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE)
			{
				if (position.GetEntityName().IsEmpty()) return Fail(id, "ownership.entityName", "empty");
				entity = GetGame().GetWorld().FindEntityByName(position.GetEntityName());
				if (!entity) return Fail(id, "ownership.entityName", "unresolved-" + position.GetEntityName());
				SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
				if (!base) return Fail(id, "ownership.adapter", "entity-is-not-vanilla-conflict-base");
				adapter = new TB_CC_M4_1_VanillaBaseOwnershipAdapter(base);
			}
			else if (adapterType == TB_CC_M4_1_OwnershipAdapterType.EXTERNAL_PROVIDER)
				return Fail(id, "ownership.adapter", "external-provider-not-registered");
			else return Fail(id, "ownership.adapter", "missing-or-unsupported");
			m_mPositions.Set(id, new TB_CC_M4_1_ResolvedPosition(position, entity, adapter)); m_aOrderedIds.Insert(id); m_mConnections.Set(id, new array<string>());
			if (position.IsMOBCandidate()) mobCount++;
			if (position.GetEndpointRole() == TB_CC_M4_1_EndpointRole.BLUFOR_HOME) bluforEndpointCount++;
			if (position.GetEndpointRole() == TB_CC_M4_1_EndpointRole.OPFOR_HOME) opforEndpointCount++;
		}
		if (mobCount < 1) return Fail("<map>", "mobCandidates", "missing");
		if (bluforEndpointCount > 1 || opforEndpointCount > 1) return Fail("<map>", "endpointRoles", "duplicate-role");
		array<ref TB_CC_M4_1_StrategicConnectionDefinition> connections = {}; definition.GetConnections(connections); map<string, bool> edgeKeys = new map<string, bool>();
		foreach (TB_CC_M4_1_StrategicConnectionDefinition connection : connections)
		{
			if (!connection) return Fail("<edge>", "connection", "null");
			string a = connection.GetA(); string b = connection.GetB();
			if (a.IsEmpty() || b.IsEmpty()) return Fail("<edge>", "endpoint", "empty");
			if (a == b) return Fail(a, "connection", "self-edge");
			if (!m_mPositions.Contains(a)) return Fail(a, "connection", "missing-source");
			if (!m_mPositions.Contains(b)) return Fail(b, "connection", "missing-target");
			string key = a + "|" + b; string reverseKey = b + "|" + a;
			if (edgeKeys.Contains(key) || edgeKeys.Contains(reverseKey)) return Fail(key, "connection", "duplicate-undirected-edge");
			edgeKeys.Set(key, true); m_mConnections.Get(a).Insert(b); m_mConnections.Get(b).Insert(a);
		}
		m_aOrderedIds.Sort(); foreach (string orderedId : m_aOrderedIds) m_mConnections.Get(orderedId).Sort();
		m_bReady = true;
		PrintFormat("[TB_CC_M4_1] READY map=%1 version=%2 name=%3 positions=%4 edges=%5 mobs=%6 deterministicOrder=stable-id", m_sMapId, m_iVersion, m_sDisplayName, m_aOrderedIds.Count(), connections.Count(), mobCount);
		return true;
	}

	protected void Reset() { m_bReady = false; m_sMapId = ""; m_iVersion = 0; m_sDisplayName = ""; m_sFailure = ""; m_mPositions.Clear(); m_mConnections.Clear(); m_aOrderedIds.Clear(); }
	protected bool Fail(string position, string field, string problem) { m_sFailure = string.Format("map=%1 position=%2 field=%3 problem=%4", m_sMapId, position, field, problem); PrintFormat("[TB_CC_M4_1] INVALID %1", m_sFailure); ResetDataKeepFailure(); return false; }
	protected void ResetDataKeepFailure() { m_bReady = false; m_mPositions.Clear(); m_mConnections.Clear(); m_aOrderedIds.Clear(); }
	bool IsReady() { return m_bReady; }
	string GetMapDefinitionId() { return m_sMapId; }
	int GetDefinitionVersion() { return m_iVersion; }
	int GetPositionCount() { return m_aOrderedIds.Count(); }
	string GetValidationFailure() { return m_sFailure; }
	TB_CC_M4_1_ResolvedPosition FindPositionById(string positionId)
	{
		if (!m_bReady || !m_mPositions.Contains(positionId))
			return null;
		return m_mPositions.Get(positionId);
	}
	void GetPositionIds(out array<string> ids)
	{
		ids.Clear();
		if (!m_bReady)
			return;
		foreach (string orderedPositionId : m_aOrderedIds)
			ids.Insert(orderedPositionId);
	}
	void GetPositions(out array<ref TB_CC_M4_1_ResolvedPosition> positions)
	{
		positions.Clear();
		if (!m_bReady)
			return;
		foreach (string positionId : m_aOrderedIds)
			positions.Insert(m_mPositions.Get(positionId));
	}
	void GetConnections(string positionId, out array<string> ids)
	{
		ids.Clear();
		if (!m_bReady || !m_mConnections.Contains(positionId))
			return;
		foreach (string adjacent : m_mConnections.Get(positionId))
			ids.Insert(adjacent);
	}
	void GetMOBDefinitions(out array<ref TB_CC_M4_1_StrategicPositionDefinition> positions)
	{
		positions.Clear();
		if (!m_bReady)
			return;
		foreach (string mobPositionId : m_aOrderedIds)
		{
			TB_CC_M4_1_StrategicPositionDefinition definition = m_mPositions.Get(mobPositionId).GetDefinition();
			if (definition.IsMOBCandidate())
				positions.Insert(definition);
		}
	}
	void GetEndpointDefinitions(out array<ref TB_CC_M4_1_StrategicPositionDefinition> positions)
	{
		positions.Clear();
		if (!m_bReady)
			return;
		foreach (string endpointPositionId : m_aOrderedIds)
		{
			TB_CC_M4_1_StrategicPositionDefinition definition = m_mPositions.Get(endpointPositionId).GetDefinition();
			if (definition.GetEndpointRole() != TB_CC_M4_1_EndpointRole.NONE)
				positions.Insert(definition);
		}
	}
	bool TryGetCurrentOwner(string id, out FactionKey owner) { TB_CC_M4_1_ResolvedPosition position = FindPositionById(id); return position && position.TryGetCurrentOwner(owner); }
}
