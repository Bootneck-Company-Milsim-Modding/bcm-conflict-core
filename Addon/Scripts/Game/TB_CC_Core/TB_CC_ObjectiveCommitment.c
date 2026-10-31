// Map-independent operational commitment, distinct from candidate scoring.
// The campaign adapter supplies current ownership and narrow invalidation facts.
class TB_CC_ObjectiveCommitment
{
	protected string m_sCurrentId;
	protected string m_sPreviousId;
	protected string m_sEndReason;
	protected bool m_bCompleted;
	protected bool m_bChanged;
	protected int m_iLastRetainedRevision = -1;
	string GetCurrentId() { return m_sCurrentId; }
	// Reconstitution is not completion, invalidation or a historical retask.
	void RestoreQuiet(string id, int revision)
	{
		m_sCurrentId = id; m_sPreviousId = ""; m_sEndReason = "";
		m_bCompleted = false; m_bChanged = false; m_iLastRetainedRevision = revision;
	}
	bool NeedsSelection(FactionKey currentOwner, bool terminal, string invalidation)
	{
		return m_sCurrentId.IsEmpty() || currentOwner == "US" || terminal || !invalidation.IsEmpty();
	}

	void Resolve(string recommendation, FactionKey currentOwner, bool terminal, string invalidation)
	{
		m_sPreviousId = m_sCurrentId; m_sEndReason = ""; m_bCompleted = false; m_bChanged = false;
		if (!m_sCurrentId.IsEmpty())
		{
			if (currentOwner == "US") { m_bCompleted = true; m_sEndReason = "owned-by-US"; }
			else if (terminal) m_sEndReason = "campaign-terminal";
			else if (!invalidation.IsEmpty()) m_sEndReason = invalidation;
			else return; // Ordinary ownership/frontier/ranking changes cannot retask.
			m_sCurrentId = "";
		}
		if (!terminal && !recommendation.IsEmpty()) m_sCurrentId = recommendation;
		m_bChanged = m_sPreviousId != m_sCurrentId;
	}

	// Called after publication, so diagnostics use the actual campaign revision.
	void Trace(int revision)
	{
		if (!m_sEndReason.IsEmpty())
		{
			if (m_bCompleted) PrintFormat("[TB_CC_OBJECTIVE] objective-completed id=%1 revision=%2", m_sPreviousId, revision);
			else PrintFormat("[TB_CC_OBJECTIVE] objective-invalidated id=%1 reason=%2 revision=%3", m_sPreviousId, m_sEndReason, revision);
		}
		if (m_bChanged)
		{
			if (!m_sCurrentId.IsEmpty()) PrintFormat("[TB_CC_OBJECTIVE] objective-locked id=%1 revision=%2", m_sCurrentId, revision);
			PrintFormat("[TB_CC_OBJECTIVE] objective-reselected previous=%1 new=%2 revision=%3", m_sPreviousId, m_sCurrentId, revision);
		}
		else if (!m_sCurrentId.IsEmpty() && revision != m_iLastRetainedRevision)
			PrintFormat("[TB_CC_OBJECTIVE] objective-retained id=%1 reason=frontline-changed-elsewhere revision=%2", m_sCurrentId, revision);
		m_iLastRetainedRevision = revision;
	}

	static string StructuralInvalidation(TB_CC_M4_1_StrategicPositionRegistry registry, string id, string origin)
	{
		if (id.IsEmpty()) return "";
		if (!registry || !registry.IsReady()) return "registry-unavailable";
		TB_CC_M4_1_ResolvedPosition position = registry.FindPositionById(id);
		if (!position) return "position-removed";
		IEntity entity = position.GetResolvedEntity();
		if (!entity || entity.IsDeleted()) return "authoritative-entity-unavailable";
		FactionKey owner;
		if (!position.TryGetCurrentOwner(owner)) return "ownership-unavailable";
		if (owner != "US" && owner != "USSR" && owner != "FIA") return "unsupported-owner";
		// Structural reachability ignores ownership. Losing a friendly connecting
		// position does not invalidate a committed assault across that frontier.
		if (!registry.FindPositionById(origin)) return "campaign-origin-unavailable";
		array<string> pending = {origin}; array<string> visited = {};
		while (!pending.IsEmpty())
		{
			string current = pending[0]; pending.Remove(0);
			if (current == id) return "";
			if (visited.Contains(current)) continue;
			visited.Insert(current);
			array<string> adjacent = {}; registry.GetConnections(current, adjacent);
			foreach (string next : adjacent) if (!visited.Contains(next)) pending.Insert(next);
		}
		return "structural-graph-path-unavailable";
	}
}
