enum TB_CC_M4_3_EvaluationState { INVALID, READY, NO_VALID_FRONTLINE_OBJECTIVE, NO_ENEMY_REMAINING, RADIO_WAIT }
enum TB_CC_M4_3_PositionState { UNKNOWN, FRIENDLY_CONNECTED, FRIENDLY_ISOLATED, ENEMY, OTHER }

class TB_CC_M4_3_OwnershipView
{
	bool TryGetOwner(string positionId, out FactionKey owner) { return false; }
}

class TB_CC_M4_3_VanillaOwnershipView : TB_CC_M4_3_OwnershipView
{
	protected TB_CC_M4_1_StrategicPositionRegistry m_Registry;
	void TB_CC_M4_3_VanillaOwnershipView(TB_CC_M4_1_StrategicPositionRegistry registry) { m_Registry = registry; }
	override bool TryGetOwner(string positionId, out FactionKey owner) { return m_Registry && m_Registry.TryGetCurrentOwner(positionId, owner); }
}

class TB_CC_M4_3_SyntheticOwnershipView : TB_CC_M4_3_OwnershipView
{
	protected ref map<string, FactionKey> m_mOwners = new map<string, FactionKey>();
	void SetOwner(string positionId, FactionKey owner) { m_mOwners.Set(positionId, owner); }
	override bool TryGetOwner(string positionId, out FactionKey owner)
	{
		if (!m_mOwners.Contains(positionId)) return false;
		owner = m_mOwners.Get(positionId); return true;
	}
}

// Optional operational gate. The accepted graph evaluator remains usable by
// non-native fixtures; the native campaign supplies its vanilla radio view.
class TB_CC_M4_3_RadioEligibilityView
{
	bool IsReachable(string positionId, out string reason) { reason = "no-radio-gate"; return true; }
}

class TB_CC_M4_3_CandidateDecision
{
	protected string m_sPositionId;
	protected int m_iBaseScore;
	protected int m_iTraitScore;
	protected int m_iConnectivityScore;
	protected int m_iEndpointScore;
	protected int m_iSeedVariation;
	protected int m_iFinalScore;
	protected string m_sEligibilityReason;
	protected int m_iProximityScore;
	protected int m_iConsolidationScore;
	void SetTerritorialScore(int importance, int proximity, int consolidation, int connectivity, int variation)
	{
		m_iBaseScore = importance; m_iProximityScore = proximity; m_iConsolidationScore = consolidation;
		m_iTraitScore = 0; m_iEndpointScore = 0; m_iConnectivityScore = connectivity; m_iSeedVariation = variation;
		// Variation breaks exact weighted ties only; it cannot reverse any
		// strategic/proximity/consolidation advantage, however small.
		m_iFinalScore = importance + proximity + consolidation + connectivity;
		m_sEligibilityReason = "ordinary hostile graph frontier; operational eligibility passed; no enemy-HQ attraction";
	}
	int GetProximityScore() { return m_iProximityScore; }
	int GetConsolidationScore() { return m_iConsolidationScore; }
	void TB_CC_M4_3_CandidateDecision(string id, int baseScore, int traitScore, int connectivityScore, int endpointScore, int seedVariation)
	{
		m_sPositionId = id; m_iBaseScore = baseScore; m_iTraitScore = traitScore; m_iConnectivityScore = connectivityScore;
		m_iEndpointScore = endpointScore; m_iSeedVariation = seedVariation;
		m_iFinalScore = baseScore + traitScore + connectivityScore + endpointScore + seedVariation;
		m_sEligibilityReason = "USSR-owned resolved adapter; adjacent to MOB-connected US component";
	}
	string GetPositionId() { return m_sPositionId; }
	int GetBaseScore() { return m_iBaseScore; }
	int GetTraitScore() { return m_iTraitScore; }
	int GetConnectivityScore() { return m_iConnectivityScore; }
	int GetEndpointScore() { return m_iEndpointScore; }
	int GetSeedVariation() { return m_iSeedVariation; }
	int GetFinalScore() { return m_iFinalScore; }
	string GetEligibilityReason() { return m_sEligibilityReason; }
}

class TB_CC_M4_3_DecisionSnapshot
{
	protected int m_iCampaignSeed;
	protected string m_sSelectedMobId;
	protected string m_sSelectedEndpointId;
	protected ref array<string> m_aFriendlyComponent = {};
	protected ref array<string> m_aCandidateIds = {};
	protected ref array<ref TB_CC_M4_3_CandidateDecision> m_aCandidateDecisions = {};
	protected string m_sSelectedObjectiveId;
	protected string m_sSelectionReason;
	void Initialize(int seed, string mobId, string endpointId) { m_iCampaignSeed = seed; m_sSelectedMobId = mobId; m_sSelectedEndpointId = endpointId; }
	void AddFriendly(string id) { m_aFriendlyComponent.Insert(id); }
	void AddCandidate(string id) { m_aCandidateIds.Insert(id); }
	void AddDecision(TB_CC_M4_3_CandidateDecision decision) { m_aCandidateDecisions.Insert(decision); }
	void SetSelection(string objectiveId, string reason) { m_sSelectedObjectiveId = objectiveId; m_sSelectionReason = reason; }
	int GetCampaignSeed() { return m_iCampaignSeed; }
	string GetSelectedMobId() { return m_sSelectedMobId; }
	string GetSelectedEndpointId() { return m_sSelectedEndpointId; }
	string GetSelectedObjectiveId() { return m_sSelectedObjectiveId; }
	string GetSelectionReason() { return m_sSelectionReason; }
	void GetFriendlyComponent(out array<string> ids) { ids.Clear(); foreach (string id : m_aFriendlyComponent) ids.Insert(id); }
	void GetCandidateIds(out array<string> ids) { ids.Clear(); foreach (string id : m_aCandidateIds) ids.Insert(id); }
	void GetCandidateDecisions(out array<ref TB_CC_M4_3_CandidateDecision> decisions) { decisions.Clear(); foreach (TB_CC_M4_3_CandidateDecision decision : m_aCandidateDecisions) decisions.Insert(decision); }
}

class TB_CC_M4_3_StrategicEvaluator
{
	protected bool m_bReady;
	protected bool m_bValid;
	protected int m_iState;
	protected int m_iRevision;
	protected string m_sFailure;
	protected string m_sSelectedMobId;
	protected string m_sSelectedEndpointId;
	protected string m_sBLUFORObjectiveId;
	protected string m_sOPFORObjectiveId;
	protected string m_sLastSignature;
	protected bool m_bEndpointReady;
	protected int m_iConnectedOrdinaryPositions;
	protected int m_iEndpointRequirement;
	protected ref array<string> m_aFriendly = {};
	protected ref array<string> m_aCandidates = {};
	protected ref map<string, int> m_mPositionStates = new map<string, int>();
	protected ref TB_CC_M4_3_DecisionSnapshot m_Snapshot;
	protected int m_iRadioReachableCandidates;
	protected int m_iRadioRejectedCandidates;
	protected FactionKey m_sExpansionFaction = "US";
	protected bool m_bThreeFactionExpansion;
	protected int m_iImportanceWeight = 40;
	protected int m_iProximityWeight = 30;
	protected int m_iConsolidationWeight = 20;
	protected int m_iConnectivityWeight = 10;
	protected float m_fProximityScale = 1500;
	protected int m_iTieVariation = 2;
	protected float m_fObjectiveBandMeters = 1000;
	protected ref array<string> m_aUSTerritory = {};
	protected ref map<string, int> m_mUSGraphDepth = new map<string, int>();
	protected ref map<string, float> m_mUSDistance = new map<string, float>();
	protected string m_sLastUSScan;
	void ConfigureObjectiveBands(float bandMeters) { m_fObjectiveBandMeters = Math.Max(bandMeters, 1); }
	protected bool UsesUSTerritorialBands() { return m_bThreeFactionExpansion && m_sExpansionFaction == "US"; }
	void ConfigureTerritorialScoring(int importance, int proximity, int consolidation, int connectivity, float proximityScale, int variation)
	{
		m_iImportanceWeight = Math.Max(importance, 0); m_iProximityWeight = Math.Max(proximity, 0);
		m_iConsolidationWeight = Math.Max(consolidation, 0); m_iConnectivityWeight = Math.Max(connectivity, 0);
		m_fProximityScale = Math.Max(proximityScale, 1); m_iTieVariation = Math.Clamp(variation, 0, 3);
	}

	void ConfigureExpansion(FactionKey faction, bool threeFaction)
	{
		m_sExpansionFaction = faction;
		m_bThreeFactionExpansion = threeFaction;
	}

	protected bool IsHostileTerritorialOwner(FactionKey owner)
	{
		if (!m_bThreeFactionExpansion) return owner == "USSR";
		return owner != m_sExpansionFaction && (owner == "US" || owner == "USSR" || owner == "FIA");
	}

	bool Evaluate(TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_2A_CampaignStartState startState, TB_CC_M4_3_OwnershipView ownership, string lastCapturedObjectiveId = "", TB_CC_M4_3_RadioEligibilityView radio = null)
	{
		m_bReady = false; m_bValid = false; m_iState = TB_CC_M4_3_EvaluationState.INVALID; m_sFailure = "";
		m_iRadioReachableCandidates = 0; m_iRadioRejectedCandidates = 0;
		m_sBLUFORObjectiveId = ""; m_sOPFORObjectiveId = ""; m_aFriendly.Clear(); m_aCandidates.Clear(); m_mPositionStates.Clear(); m_Snapshot = null;
		if (!registry || !registry.IsReady()) return Fail("registry-not-ready");
		if (!startState) return Fail("campaign-start-null");
		if (!ownership) return Fail("ownership-view-null");
		m_sSelectedMobId = startState.GetSelectedMobId(); m_sSelectedEndpointId = startState.GetSelectedEndpointId();
		m_iEndpointRequirement = startState.GetMinimumConnectedOrdinaryPositions();
		if (m_sSelectedMobId.IsEmpty()) return Fail("campaign-start-invalid");
		if (startState.RequiresPhysicalOPFOREndpoint() && (m_sSelectedEndpointId.IsEmpty() || m_sSelectedMobId == m_sSelectedEndpointId)) return Fail("campaign-start-invalid");
		TB_CC_M4_1_ResolvedPosition mob = registry.FindPositionById(m_sSelectedMobId);
		TB_CC_M4_1_ResolvedPosition endpoint = registry.FindPositionById(m_sSelectedEndpointId);
		if (!mob || !mob.GetDefinition().IsMOBCandidate()) return Fail("campaign-start-unresolved");
		if (startState.RequiresPhysicalOPFOREndpoint() && !endpoint) return Fail("campaign-start-unresolved");
		array<string> allIds = {}; registry.GetPositionIds(allIds); map<string, FactionKey> owners = new map<string, FactionKey>();
		foreach (string positionId : allIds)
		{
			FactionKey owner;
			if (!ownership.TryGetOwner(positionId, owner)) return Fail("owner-unresolved-" + positionId);
			owners.Set(positionId, owner);
		}
		if (owners.Get(m_sSelectedMobId) != m_sExpansionFaction) return Fail("origin-owner-mismatch");
		BuildFriendlyComponent(registry, owners);
		m_iConnectedOrdinaryPositions = 0;
		foreach (string friendlyPositionId : m_aFriendly) if (friendlyPositionId != m_sSelectedMobId && friendlyPositionId != m_sSelectedEndpointId) m_iConnectedOrdinaryPositions++;
		m_bEndpointReady = m_iConnectedOrdinaryPositions >= m_iEndpointRequirement;
		foreach (string positionId : allIds)
		{
			FactionKey owner = owners.Get(positionId);
			if (m_aFriendly.Contains(positionId)) m_mPositionStates.Set(positionId, TB_CC_M4_3_PositionState.FRIENDLY_CONNECTED);
			else if (owner == m_sExpansionFaction) m_mPositionStates.Set(positionId, TB_CC_M4_3_PositionState.FRIENDLY_ISOLATED);
			else if (IsHostileTerritorialOwner(owner)) m_mPositionStates.Set(positionId, TB_CC_M4_3_PositionState.ENEMY);
			else m_mPositionStates.Set(positionId, TB_CC_M4_3_PositionState.OTHER);
		}
		BuildCandidates(registry, owners);
		m_Snapshot = new TB_CC_M4_3_DecisionSnapshot(); m_Snapshot.Initialize(startState.GetSeed(), m_sSelectedMobId, m_sSelectedEndpointId);
		foreach (string friendlyId : m_aFriendly) m_Snapshot.AddFriendly(friendlyId);
		foreach (string candidateId : m_aCandidates) m_Snapshot.AddCandidate(candidateId);
		SelectObjective(registry, startState.GetSeed(), radio);
		if (UsesUSTerritorialBands()) TraceUSTerritorialScan(allIds, owners, radio);
		if (!m_bThreeFactionExpansion) DeriveOPFORResponse(registry, owners, lastCapturedObjectiveId);
		bool enemyRemains;
		foreach (string positionId : allIds)
		{
			if (positionId == m_sSelectedMobId || positionId == m_sSelectedEndpointId) continue;
			if (IsHostileTerritorialOwner(owners.Get(positionId))) { enemyRemains = true; break; }
		}
		if (m_sBLUFORObjectiveId.IsEmpty())
		{
			if (enemyRemains && radio && !m_aCandidates.IsEmpty() && m_iRadioReachableCandidates == 0) { m_iState = TB_CC_M4_3_EvaluationState.RADIO_WAIT; m_Snapshot.SetSelection("", "graph boundary exists; waiting for vanilla US HQ radio reachability"); }
			else if (enemyRemains) { m_iState = TB_CC_M4_3_EvaluationState.NO_VALID_FRONTLINE_OBJECTIVE; m_Snapshot.SetSelection("", "enemy remains but no eligible boundary candidate"); }
			else { m_iState = TB_CC_M4_3_EvaluationState.NO_ENEMY_REMAINING; m_Snapshot.SetSelection("", "no hostile-owned strategic position remains; outcome not inferred"); }
		}
		else m_iState = TB_CC_M4_3_EvaluationState.READY;
		m_bReady = true; m_bValid = m_iState == TB_CC_M4_3_EvaluationState.READY || m_iState == TB_CC_M4_3_EvaluationState.RADIO_WAIT;
		if (m_bThreeFactionExpansion) m_bValid = true; // A disconnected/no-target front is valid, not an invalid ownership topology.
		string signature = BuildSignature(allIds, owners);
		if (signature != m_sLastSignature) { m_iRevision++; m_sLastSignature = signature; }
		return true;
	}

	protected void BuildFriendlyComponent(TB_CC_M4_1_StrategicPositionRegistry registry, map<string, FactionKey> owners)
	{
		array<string> pending = { m_sSelectedMobId };
		while (!pending.IsEmpty())
		{
			string current = pending[0]; pending.Remove(0);
			if (m_aFriendly.Contains(current) || owners.Get(current) != m_sExpansionFaction) continue;
			m_aFriendly.Insert(current);
			array<string> adjacent = {}; registry.GetConnections(current, adjacent);
			foreach (string nextId : adjacent) if (!m_aFriendly.Contains(nextId) && owners.Get(nextId) == m_sExpansionFaction) pending.Insert(nextId);
		}
		m_aFriendly.Sort();
	}

	protected void BuildCandidates(TB_CC_M4_1_StrategicPositionRegistry registry, map<string, FactionKey> owners)
	{
		if (UsesUSTerritorialBands()) { BuildUSTerritorialCandidates(registry, owners); return; }
		foreach (string friendlyId : m_aFriendly)
		{
			array<string> adjacent = {}; registry.GetConnections(friendlyId, adjacent);
			foreach (string adjacentId : adjacent)
			{
				if (!IsHostileTerritorialOwner(owners.Get(adjacentId)) || m_aCandidates.Contains(adjacentId)) continue;
				// HQs are permanent anchors, never ordinary graph objectives.
				if (adjacentId == m_sSelectedMobId || adjacentId == m_sSelectedEndpointId) continue;
				TB_CC_M4_1_ResolvedPosition position = registry.FindPositionById(adjacentId); FactionKey adapterOwner;
				if (!position || !position.TryGetCurrentOwner(adapterOwner)) continue;
				m_aCandidates.Insert(adjacentId);
			}
		}
		m_aCandidates.Sort();
	}

	protected void SelectObjective(TB_CC_M4_1_StrategicPositionRegistry registry, int seed, TB_CC_M4_3_RadioEligibilityView radio)
	{
		int bestScore = -2147483647;
		int bestVariation = -1;
		int bestBand = int.MAX;
		for (int i; i < m_aCandidates.Count(); i++)
		{
			string candidateId = m_aCandidates[i]; TB_CC_M4_1_StrategicPositionDefinition definition = registry.FindPositionById(candidateId).GetDefinition();
			string radioReason = "no-radio-gate";
			bool radioReachable = true;
			if (radio) radioReachable = radio.IsReachable(candidateId, radioReason);
			PrintFormat("[TB_CC_M4_3_RADIO] candidate=%1 graphLegal=1 radioReachable=%2 reason=%3", candidateId, radioReachable, radioReason);
			if (!radioReachable) { m_iRadioRejectedCandidates++; continue; }
			m_iRadioReachableCandidates++;
			int traitScore = TraitScore(definition); array<string> adjacent = {}; registry.GetConnections(candidateId, adjacent);
			int connectivityScore = adjacent.Count() * 2; int endpointScore;
			int variation = ((seed % 997) * 31 + i * 17) % 7; if (variation < 0) variation = -variation;
			TB_CC_M4_3_CandidateDecision decision = new TB_CC_M4_3_CandidateDecision(candidateId, definition.GetStrategicImportance(), traitScore, connectivityScore, endpointScore, variation);
			if (m_bThreeFactionExpansion) ScoreTerritorialCandidate(registry, definition, adjacent, seed, i, decision);
			m_Snapshot.AddDecision(decision);
			if (m_bThreeFactionExpansion) PrintFormat("[TB_CC_M4_3] SCORE id=%1 faction=%2 importance=%3 proximity=%4 consolidation=%5 connectivity=%6 HQPull=0 variation=%7 final=%8", candidateId, m_sExpansionFaction, decision.GetBaseScore(), decision.GetProximityScore(), decision.GetConsolidationScore(), decision.GetConnectivityScore(), decision.GetSeedVariation(), decision.GetFinalScore());
			else PrintFormat("[TB_CC_M4_3] SCORE id=%1 base=%2 traits=%3 connectivity=%4 endpoint=%5 seedVariation=%6 final=%7 eligible=1", candidateId, decision.GetBaseScore(), traitScore, connectivityScore, endpointScore, variation, decision.GetFinalScore());
			bool better = decision.GetFinalScore() > bestScore;
			if (m_bThreeFactionExpansion && decision.GetFinalScore() == bestScore && decision.GetSeedVariation() > bestVariation) better = true;
			if (UsesUSTerritorialBands())
			{
				int band = Math.Floor(m_mUSDistance.Get(candidateId) / m_fObjectiveBandMeters);
				if (band < bestBand) better = true;
				else if (band > bestBand) better = false;
				if (better) bestBand = band;
			}
			if (better) { bestScore = decision.GetFinalScore(); bestVariation = decision.GetSeedVariation(); m_sBLUFORObjectiveId = candidateId; }
		}
		PrintFormat("[TB_CC_M4_3_RADIO] selected=%1 reachableCandidates=%2 rejectedUnreachable=%3", m_sBLUFORObjectiveId, m_iRadioReachableCandidates, m_iRadioRejectedCandidates);
		if (!m_sBLUFORObjectiveId.IsEmpty()) m_Snapshot.SetSelection(m_sBLUFORObjectiveId, string.Format("highest eligible final score=%1; territorial seed variation breaks weighted ties, then stable-ID order", bestScore));
	}

	protected void ScoreTerritorialCandidate(TB_CC_M4_1_StrategicPositionRegistry registry, TB_CC_M4_1_StrategicPositionDefinition definition, array<string> adjacent, int seed, int index, TB_CC_M4_3_CandidateDecision decision)
	{
		// Once ordinary territory exists, use that footprint rather than either HQ.
		bool hasOrdinaryFootprint;
		foreach (string friendly : m_aFriendly)
			if (friendly != m_sSelectedMobId && friendly != m_sSelectedEndpointId) hasOrdinaryFootprint = true;
		float nearest = 1000000;
		foreach (string friendly : m_aFriendly)
		{
			if (hasOrdinaryFootprint && (friendly == m_sSelectedMobId || friendly == m_sSelectedEndpointId)) continue;
			vector origin = registry.FindPositionById(friendly).GetDefinition().GetAuthoredPosition();
			vector target = definition.GetAuthoredPosition();
			float dx = origin[0] - target[0]; float dz = origin[2] - target[2];
			nearest = Math.Min(nearest, Math.Sqrt(dx * dx + dz * dz));
		}
		int friendlyNeighbours; int newConnections;
		if (UsesUSTerritorialBands()) nearest = m_mUSDistance.Get(definition.GetId());
		foreach (string neighbour : adjacent)
		{
			bool friendlyNeighbour = m_aFriendly.Contains(neighbour);
			if (UsesUSTerritorialBands()) friendlyNeighbour = m_aUSTerritory.Contains(neighbour);
			if (friendlyNeighbour) friendlyNeighbours++;
			else if (neighbour != m_sSelectedMobId && neighbour != m_sSelectedEndpointId && GetStrategicState(neighbour) == TB_CC_M4_3_PositionState.ENEMY) newConnections++;
		}
		int importance = (Math.Clamp(definition.GetStrategicImportance(), 0, 100) * m_iImportanceWeight) / 100;
		int proximity = Math.Round(m_iProximityWeight * (1 - Math.Clamp(nearest / m_fProximityScale, 0, 1)));
		int consolidation = (Math.Min(friendlyNeighbours, 3) * m_iConsolidationWeight) / 3;
		int connectivity = (Math.Min(newConnections, 6) * m_iConnectivityWeight) / 6;
		int variation = ((seed % 997) * 31 + index * 17) % (m_iTieVariation + 1);
		if (variation < 0) variation = -variation;
		decision.SetTerritorialScore(importance, proximity, consolidation, connectivity, variation);
	}

	// US selection only. Connected-component depth/HQ pressure and the USSR
	// evaluator retain their existing semantics. No capture history is consulted.
	protected void BuildUSTerritorialCandidates(TB_CC_M4_1_StrategicPositionRegistry registry, map<string, FactionKey> owners)
	{
		m_aUSTerritory.Clear(); m_mUSGraphDepth.Clear(); m_mUSDistance.Clear();
		array<string> ids = {}; registry.GetPositionIds(ids);
		foreach (string id : ids)
			if (id != m_sSelectedEndpointId && owners.Get(id) == "US") m_aUSTerritory.Insert(id);
		array<string> pending = {};
		foreach (string source : m_aUSTerritory) { pending.Insert(source); m_mUSGraphDepth.Set(source, 0); }
		// Multi-source breadth-first search. An empty first frontier does not
		// terminate enumeration; protected enemy HQ is never a transit shortcut.
		for (int cursor; cursor < pending.Count(); cursor++)
		{
			string current = pending[cursor];
			array<string> adjacent = {}; registry.GetConnections(current, adjacent);
			foreach (string next : adjacent)
			{
				if (next == m_sSelectedEndpointId || m_mUSGraphDepth.Contains(next)) continue;
				m_mUSGraphDepth.Set(next, m_mUSGraphDepth.Get(current) + 1);
				pending.Insert(next);
			}
		}
		int frontierDepth = int.MAX;
		foreach (string id : ids)
		{
			if (id == m_sSelectedMobId || id == m_sSelectedEndpointId || !IsHostileTerritorialOwner(owners.Get(id))) continue;
			TB_CC_M4_1_ResolvedPosition position = registry.FindPositionById(id);
			if (!position || !position.GetResolvedEntity() || position.GetResolvedEntity().IsDeleted()) continue;
			float nearest = float.MAX;
			vector target = position.GetDefinition().GetAuthoredPosition();
			foreach (string source : m_aUSTerritory)
			{
				vector origin = registry.FindPositionById(source).GetDefinition().GetAuthoredPosition();
				float dx = target[0] - origin[0]; float dz = target[2] - origin[2];
				nearest = Math.Min(nearest, Math.Sqrt(dx * dx + dz * dz));
			}
			m_mUSDistance.Set(id, nearest);
			if (m_mUSGraphDepth.Contains(id)) frontierDepth = Math.Min(frontierDepth, m_mUSGraphDepth.Get(id));
		}
		foreach (string id : ids)
			if (m_mUSDistance.Contains(id) && m_mUSGraphDepth.Contains(id) && m_mUSGraphDepth.Get(id) == frontierDepth) m_aCandidates.Insert(id);
		m_aCandidates.Sort();
	}

	protected void TraceUSTerritorialScan(array<string> ids, map<string, FactionKey> owners, TB_CC_M4_3_RadioEligibilityView radio)
	{
		string signature = m_sBLUFORObjectiveId;
		int selectedBand = -1;
		if (!m_sBLUFORObjectiveId.IsEmpty()) selectedBand = Math.Floor(m_mUSDistance.Get(m_sBLUFORObjectiveId) / m_fObjectiveBandMeters);
		array<string> lines = {};
		int hostile;
		int usOwnedCount;
		foreach (string id : ids)
		{
			if (id == m_sSelectedMobId || id == m_sSelectedEndpointId) continue;
			FactionKey owner = owners.Get(id);
			if (owner == "US") { usOwnedCount++; continue; }
			hostile++;
			bool frontier = m_aCandidates.Contains(id);
			bool graphLegal = m_mUSGraphDepth.Contains(id) && m_mUSDistance.Contains(id);
			string reason = "structural-path-or-entity-unavailable";
			string radioReason = "no-radio-gate";
			bool reachable = true;
			if (radio) reachable = radio.IsReachable(id, radioReason);
			int depth = -1; int band = -1; float distance = -1;
			if (m_mUSGraphDepth.Contains(id)) depth = m_mUSGraphDepth.Get(id);
			if (m_mUSDistance.Contains(id))
			{
				distance = m_mUSDistance.Get(id);
				band = Math.Floor(distance / m_fObjectiveBandMeters);
			}
			bool eligible = frontier && reachable && band == selectedBand;
			if (graphLegal) reason = "deeper-than-current-graph-frontier";
			if (frontier) reason = radioReason;
			if (frontier && reachable && band != selectedBand) reason = "farther-than-first-radio-valid-band";
			string line = string.Format("id=%1 owner=%2 distanceFromUSTerritory=%3 graphDepth=%4 graphLegal=%5 frontierLegal=%6 radioReachable=%7", id, owner, distance, depth, graphLegal, frontier, reachable);
			line += string.Format(" historyExcluded=0 depthExcluded=0 candidateBand=%1 finalEligible=%2 reason=%3", band, eligible, reason);
			lines.Insert(line); signature += "|" + line;
		}
		if (signature == m_sLastUSScan) return;
		m_sLastUSScan = signature;
		PrintFormat("[TB_CC_OBJECTIVE_SCAN] hostileOrdinary=%1 usOwnedOrdinary=%2 bandMeters=%3 bandIndex=%4 candidates=%5 radioReachable=%6", hostile, usOwnedCount, m_fObjectiveBandMeters, selectedBand, m_aCandidates.Count(), m_iRadioReachableCandidates);
		foreach (string line : lines) PrintFormat("[TB_CC_OBJECTIVE_CANDIDATE] %1", line);
		string selectionReason = "nearest-radio-valid-band-then-score";
		if (hostile == 0) selectionReason = "all-ordinary-currently-US";
		else if (m_sBLUFORObjectiveId.IsEmpty()) selectionReason = "no-eligible-frontier";
		PrintFormat("[TB_CC_OBJECTIVE_SELECT] selected=%1 band=%2 remainingHostile=%3 reason=%4 recommendationOnly=1", m_sBLUFORObjectiveId, selectedBand, hostile, selectionReason);
		if (hostile > 0 && !m_aCandidates.IsEmpty() && m_iRadioReachableCandidates == 0)
			PrintFormat("[TB_CC_OBJECTIVE_WAIT] state=RADIO_WAIT remainingHostile=%1 graphLegal=%2 radioReachable=0", hostile, m_aCandidates.Count());
	}

	protected int TraitScore(TB_CC_M4_1_StrategicPositionDefinition definition)
	{
		int score;
		if (definition.HasTrait("MILITARY")) score += 8;
		if (definition.HasTrait("LOGISTICS")) score += 6;
		if (definition.HasTrait("SUPPLY")) score += 5;
		if (definition.HasTrait("TRANSPORT")) score += 4;
		if (definition.HasTrait("POPULATION_CENTER")) score += 4;
		if (definition.HasTrait("COMMUNICATIONS")) score += 3;
		if (definition.HasTrait("INFRASTRUCTURE")) score += 3;
		return score;
	}

	protected void DeriveOPFORResponse(TB_CC_M4_1_StrategicPositionRegistry registry, map<string, FactionKey> owners, string lastCapturedObjectiveId)
	{
		if (lastCapturedObjectiveId.IsEmpty() || lastCapturedObjectiveId == m_sSelectedMobId || lastCapturedObjectiveId == m_sSelectedEndpointId || !owners.Contains(lastCapturedObjectiveId) || owners.Get(lastCapturedObjectiveId) != "US") return;
		array<string> adjacent = {}; registry.GetConnections(lastCapturedObjectiveId, adjacent);
		foreach (string adjacentId : adjacent) if (owners.Get(adjacentId) == "USSR") { m_sOPFORObjectiveId = lastCapturedObjectiveId; return; }
		// At 6/6 ordinary US control, only the protected USSR HQ may remain.
		// Preserve the last legitimately captured ordinary position as the one
		// bounded countercapture that can cancel BLUFOR victory pressure.
		bool enemyOrdinaryRemains;
		foreach (string id, FactionKey owner : owners)
		{
			if (id == m_sSelectedMobId || id == m_sSelectedEndpointId) continue;
			if (owner == "USSR") { enemyOrdinaryRemains = true; break; }
		}
		if (!enemyOrdinaryRemains) m_sOPFORObjectiveId = lastCapturedObjectiveId;
	}

	protected string BuildSignature(array<string> ids, map<string, FactionKey> owners)
	{
		string signature = m_sSelectedMobId + ">" + m_sSelectedEndpointId + "|" + m_sBLUFORObjectiveId + "|" + m_sOPFORObjectiveId;
		foreach (string id : ids) signature += "|" + id + "=" + owners.Get(id);
		return signature;
	}

	protected bool Fail(string problem) { m_sFailure = "field=strategicEvaluation problem=" + problem; PrintFormat("[TB_CC_M4_3] INVALID %1", m_sFailure); return false; }
	bool IsReady() { return m_bReady; }
	bool IsValid() { return m_bValid; }
	int GetEvaluationState() { return m_iState; }
	int GetRevision() { return m_iRevision; }
	string GetFailure() { return m_sFailure; }
	string GetSelectedMobId() { return m_sSelectedMobId; }
	string GetSelectedEndpointId() { return m_sSelectedEndpointId; }
	void GetFriendlyComponent(out array<string> ids) { ids.Clear(); foreach (string id : m_aFriendly) ids.Insert(id); }
	bool GetFriendlyComponentContains(string positionId) { return m_aFriendly.Contains(positionId); }
	void GetBLUFORCandidateObjectives(out array<string> ids) { ids.Clear(); foreach (string id : m_aCandidates) ids.Insert(id); }
	string GetBLUFORLegalObjective() { return m_sBLUFORObjectiveId; }
	string GetOPFORLegalObjective() { return m_sOPFORObjectiveId; }
	int GetStrategicState(string positionId)
	{
		if (!m_mPositionStates.Contains(positionId))
			return TB_CC_M4_3_PositionState.UNKNOWN;
		return m_mPositionStates.Get(positionId);
	}
	bool CanBLUFORAttack(string positionId) { return m_bValid && positionId == m_sBLUFORObjectiveId; }
	bool CanOPFORAttack(string positionId) { return !m_sOPFORObjectiveId.IsEmpty() && positionId == m_sOPFORObjectiveId; }
	bool IsEndpointReady() { return m_bEndpointReady; }
	int GetConnectedOrdinaryPositionCount() { return m_iConnectedOrdinaryPositions; }
	int GetEndpointRequirement() { return m_iEndpointRequirement; }
	TB_CC_M4_3_DecisionSnapshot GetDecisionSnapshot() { return m_Snapshot; }
}
