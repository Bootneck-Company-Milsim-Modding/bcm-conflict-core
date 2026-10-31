enum TB_CC_M4_2A_StartMode { SEEDED, EXPLICIT }

class TB_CC_M4_2A_MobEndpointDefinition
{
	protected string m_sMobId;
	protected string m_sEndpointId;
	protected int m_iMinimumConnectedOrdinaryPositions;
	void TB_CC_M4_2A_MobEndpointDefinition(string mobId, string endpointId, int minimumConnectedOrdinaryPositions = 0) { m_sMobId = mobId; m_sEndpointId = endpointId; m_iMinimumConnectedOrdinaryPositions = minimumConnectedOrdinaryPositions; }
	string GetMobId() { return m_sMobId; }
	string GetEndpointId() { return m_sEndpointId; }
	int GetMinimumConnectedOrdinaryPositions() { return m_iMinimumConnectedOrdinaryPositions; }
}

class TB_CC_M4_2A_CampaignStartDefinition
{
	protected ref array<ref TB_CC_M4_2A_MobEndpointDefinition> m_aRelationships = {};
	void AddRelationship(string mobId, string endpointId, int minimumConnectedOrdinaryPositions = 0) { m_aRelationships.Insert(new TB_CC_M4_2A_MobEndpointDefinition(mobId, endpointId, minimumConnectedOrdinaryPositions)); }
	void GetRelationships(out array<ref TB_CC_M4_2A_MobEndpointDefinition> relationships)
	{
		relationships.Clear();
		foreach (TB_CC_M4_2A_MobEndpointDefinition relationship : m_aRelationships) relationships.Insert(relationship);
	}
}

class TB_CC_M4_2A_CampaignStartState
{
	protected int m_iSeed;
	protected int m_iMode;
	protected string m_sSelectedMobId;
	protected string m_sSelectedEndpointId;
	protected int m_iMinimumConnectedOrdinaryPositions;
	void TB_CC_M4_2A_CampaignStartState(int seed, int mode, string mobId, string endpointId, int minimumConnectedOrdinaryPositions = 0) { m_iSeed = seed; m_iMode = mode; m_sSelectedMobId = mobId; m_sSelectedEndpointId = endpointId; m_iMinimumConnectedOrdinaryPositions = minimumConnectedOrdinaryPositions; }
	int GetSeed() { return m_iSeed; }
	int GetMode() { return m_iMode; }
	// Existing start definitions keep their physical endpoint contract.
	bool RequiresPhysicalOPFOREndpoint() { return true; }
	string GetSelectedMobId() { return m_sSelectedMobId; }
	string GetSelectedEndpointId() { return m_sSelectedEndpointId; }
	int GetMinimumConnectedOrdinaryPositions() { return m_iMinimumConnectedOrdinaryPositions; }
}

class TB_CC_M4_2A_CampaignStartResolver
{
	protected string m_sFailure;
	string GetFailure() { return m_sFailure; }

	TB_CC_M4_2A_CampaignStartState Resolve(TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_2A_CampaignStartDefinition startDefinition, int seed, int mode = TB_CC_M4_2A_StartMode.SEEDED, string explicitMobId = "")
	{
		m_sFailure = "";
		if (!registry || !registry.IsReady()) return Fail("registry-not-ready");
		if (!startDefinition) return Fail("start-definition-null");
		array<ref TB_CC_M4_2A_MobEndpointDefinition> relationships = {}; startDefinition.GetRelationships(relationships);
		if (relationships.IsEmpty()) return Fail("no-mob-endpoint-relationships");
		array<string> candidateIds = {};
		foreach (TB_CC_M4_2A_MobEndpointDefinition relationship : relationships)
		{
			if (!relationship || relationship.GetMobId().IsEmpty() || relationship.GetEndpointId().IsEmpty()) return Fail("malformed-relationship");
			TB_CC_M4_1_ResolvedPosition mob = registry.FindPositionById(relationship.GetMobId());
			TB_CC_M4_1_ResolvedPosition endpoint = registry.FindPositionById(relationship.GetEndpointId());
			if (!mob || !endpoint) return Fail("relationship-position-unresolved");
			if (!mob.GetDefinition().IsMOBCandidate()) return Fail("relationship-mob-not-candidate-" + relationship.GetMobId());
			if (relationship.GetMobId() == relationship.GetEndpointId()) return Fail("mob-equals-endpoint");
			if (!IsReachable(registry, relationship.GetMobId(), relationship.GetEndpointId())) return Fail("endpoint-unreachable-" + relationship.GetMobId());
			if (candidateIds.Contains(relationship.GetMobId())) return Fail("duplicate-mob-relationship-" + relationship.GetMobId());
			candidateIds.Insert(relationship.GetMobId());
		}
		candidateIds.Sort();
		string selectedMobId;
		if (mode == TB_CC_M4_2A_StartMode.EXPLICIT)
		{
			if (!candidateIds.Contains(explicitMobId)) return Fail("explicit-mob-invalid-" + explicitMobId);
			selectedMobId = explicitMobId;
		}
		else
		{
			int index = seed % candidateIds.Count();
			if (index < 0) index = -index;
			selectedMobId = candidateIds[index];
		}
		string endpointId = FindEndpoint(relationships, selectedMobId);
		if (endpointId.IsEmpty()) return Fail("selected-endpoint-missing");
		int minimumConnectedOrdinaryPositions;
		foreach (TB_CC_M4_2A_MobEndpointDefinition relationship : relationships) if (relationship.GetMobId() == selectedMobId) minimumConnectedOrdinaryPositions = relationship.GetMinimumConnectedOrdinaryPositions();
		return new TB_CC_M4_2A_CampaignStartState(seed, mode, selectedMobId, endpointId, minimumConnectedOrdinaryPositions);
	}

	TB_CC_M4_2A_CampaignStartState Resume(TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_2A_CampaignStartDefinition definition, TB_CC_M4_2A_CampaignStartState storedState)
	{
		if (!storedState) return Fail("stored-state-null");
		TB_CC_M4_2A_CampaignStartState validated = Resolve(registry, definition, storedState.GetSeed(), TB_CC_M4_2A_StartMode.EXPLICIT, storedState.GetSelectedMobId());
		if (!validated) return null;
		if (validated.GetSelectedEndpointId() != storedState.GetSelectedEndpointId()) return Fail("stored-endpoint-mismatch");
		return new TB_CC_M4_2A_CampaignStartState(storedState.GetSeed(), storedState.GetMode(), storedState.GetSelectedMobId(), storedState.GetSelectedEndpointId(), validated.GetMinimumConnectedOrdinaryPositions());
	}

	protected string FindEndpoint(array<ref TB_CC_M4_2A_MobEndpointDefinition> relationships, string mobId)
	{
		foreach (TB_CC_M4_2A_MobEndpointDefinition relationship : relationships)
			if (relationship.GetMobId() == mobId) return relationship.GetEndpointId();
		return "";
	}

	protected bool IsReachable(TB_CC_M4_1_StrategicPositionRegistry registry, string startId, string targetId)
	{
		array<string> pending = { startId }; array<string> visited = {};
		while (!pending.IsEmpty())
		{
			string current = pending[0]; pending.Remove(0);
			if (current == targetId) return true;
			if (visited.Contains(current)) continue;
			visited.Insert(current);
			array<string> adjacent = {}; registry.GetConnections(current, adjacent);
			foreach (string nextId : adjacent) if (!visited.Contains(nextId) && !pending.Contains(nextId)) pending.Insert(nextId);
		}
		return false;
	}

	protected TB_CC_M4_2A_CampaignStartState Fail(string problem)
	{
		m_sFailure = "field=campaignStart problem=" + problem;
		PrintFormat("[TB_CC_M4_2A] INVALID %1", m_sFailure);
		return null;
	}
}
