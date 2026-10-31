// Adapter contract: one authority turn produces one coherent snapshot.
class TB_CC_StrategicPersistenceAdapter
{
	bool Capture(TB_CC_CampaignContainer snapshot) { return false; }
	string GetCaptureError() { return "strategic-adapter-rejected"; }
}

class TB_CC_StrategicPersistence
{
	static bool SameMap(map<string, string> a, map<string, string> b)
	{
		if (!a || !b || a.Count() != b.Count()) return false;
		for (int i = 0; i < a.Count(); i++)
		{
			string key = a.GetKey(i);
			if (!b.Contains(key) || a.Get(key) != b.Get(key)) return false;
		}
		return true;
	}
	static bool SamePayload(TB_CC_CampaignContainer a, TB_CC_CampaignContainer b)
	{
		if (!a || !b || a.snapshotPresent != b.snapshotPresent || a.strategicPositions.Count() != b.strategicPositions.Count()) return false;
		for (int i = 0; i < a.strategicPositions.Count(); i++) if (a.strategicPositions[i] != b.strategicPositions[i]) return false;
		if (!SameMap(a.positionOwners, b.positionOwners) || !SameMap(a.campaignState, b.campaignState)) return false;
		if (!SameMap(a.objectiveState, b.objectiveState) || !SameMap(a.hqState, b.hqState)) return false;
		if (!SameMap(a.pressureState, b.pressureState) || !SameMap(a.factionState, b.factionState)) return false;
		if (a.operations.Count() || b.operations.Count() || a.players.Count() != b.players.Count()) return false;
		for (int playerIndex = 0; playerIndex < a.players.Count(); playerIndex++) if (a.players[playerIndex] != b.players[playerIndex]) return false;
		return true;
	}
	static bool ReadInt(map<string, string> values, string key, int minimum, int maximum, out int result)
	{
		result = 0;
		if (!values || !values.Contains(key)) return false;
		string text = values.Get(key);
		result = text.ToInt();
		return result.ToString() == text && result >= minimum && result <= maximum;
	}
	static bool IsTerminal(int outcome)
	{
		return outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
	}
}
