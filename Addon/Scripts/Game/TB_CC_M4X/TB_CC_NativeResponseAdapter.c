// Map binding only. Queue/epochs/timing contain no Arland IDs or coordinates.
class TB_CC_NativeResponseAdapter : TB_CC_ResponseAdapter
{
	protected TB_CC_M4XP_NativeArlandComponent m_Policy;
	void TB_CC_NativeResponseAdapter(TB_CC_M4XP_NativeArlandComponent policy) { m_Policy = policy; }
	override bool IsOperational() { return m_Policy && m_Policy.IsWarOperational(); }
	override bool IsOrdinary(string id) { return TB_CC_M4XP_CampaignRoles.GetRole(id) == TB_CC_StrategicRole.ORDINARY; }
	override FactionKey Owner(string id) { return m_Policy.GetTerritorialOwner(id); }
	override bool FinalDefence() { return m_Policy.GetHQState(m_Policy.GetSelectedEndpointId()) == TB_CC_HQLockState.VULNERABLE; }
	override int PressureRemaining()
	{
		if (m_Policy.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) return -1;
		return m_Policy.GetPressureRemainingSeconds();
	}
	override bool CanRespond(FactionKey faction, string id, bool reinforcement)
	{
		if (!IsOperational() || !m_Policy.IsFootTerritory(id)) return false;
		if (faction == "FIA" && !m_Policy.HasFIAResponseCapability()) return false;
		if (reinforcement && Owner(id) != faction) return false;
		if (!reinforcement && Owner(id) != "US") return false;
		string source; vector position;
		return m_Policy.WarSource(faction, id, source, position);
	}
	override string RecoveryTarget()
	{
		// Rank the currently valid Soviet graph frontier, not a random coordinate.
		// Recency first, then nearest source distance and stable registry order.
		array<string> ids = {}; m_Policy.GetResponsePositionIds(ids);
		string selected; int bestRecency = -1; float bestDistance = float.MAX;
		foreach (string id : ids)
		{
			if (!CanRespond("USSR", id, false)) continue;
			string source; vector origin; vector position;
			if (!m_Policy.WarSource("USSR", id, source, origin) || !TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, position)) continue;
			int recency = m_Policy.GetResponseLossRevision(id);
			float distance = vector.Distance(origin, position);
			if (recency > bestRecency || (recency == bestRecency && distance < bestDistance))
			{ selected = id; bestRecency = recency; bestDistance = distance; }
		}
		return selected;
	}
	override string Package(TB_CC_ResponseRequest request)
	{
		if (request.m_sFaction == "USSR")
		{
			if (request.m_iPhase == TB_CC_ResponsePhase.CONTACT) return "REINFORCED";
			return "HEAVY";
		}
		// Existing FIA capability/depth/human policy remains the ceiling.
		string selected = m_Policy.SelectFIAPackage(request.m_sTarget);
		if (request.m_iPhase == TB_CC_ResponsePhase.CONTACT && selected == "FIA_STRONGPOINT") return "FIA_STANDARD";
		return selected;
	}
	override TB_PVE_M3_1_Operation Launch(TB_CC_ResponseRequest request, string packageId)
	{
		return m_Policy.LaunchResponse(request, packageId);
	}
}

class TB_CC_NativeDefenceAdapter : TB_CC_DefenceAdapter
{
	protected TB_CC_M4XP_NativeArlandComponent m_Policy;
	void TB_CC_NativeDefenceAdapter(TB_CC_M4XP_NativeArlandComponent policy) { m_Policy = policy; }
	override bool IsOperational() { return m_Policy && m_Policy.IsWarOperational(); }
	override FactionKey Owner(string id) { return m_Policy.GetTerritorialOwner(id); }
	override bool IsFinalHQ(string id) { return id == m_Policy.GetSelectedEndpointId() && m_Policy.GetHQState(id) == TB_CC_HQLockState.VULNERABLE; }
	override bool Approach(string id, out vector position)
	{
		vector centre; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, centre)) return false;
		array<string> ids = {}; m_Policy.GetResponsePositionIds(ids);
		float nearest = float.MAX; bool found;
		foreach (string source : ids)
		{
			if (m_Policy.GetTerritorialOwner(source) != "US") continue;
			vector candidate; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(source, candidate)) continue;
			float distance = vector.Distance(centre, candidate);
			if (distance < nearest) { nearest = distance; position = candidate; found = true; }
		}
		return found;
	}
}
