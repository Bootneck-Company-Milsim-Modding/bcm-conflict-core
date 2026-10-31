// Dedicated diagnostic only, invoked by explicit mode 8. Reuses production
// placement predicates without starting a manager, faking humans or spawning AI.
class TB_CC_M4_4I_GarrisonPlacementProbe : TB_PVE_M3_6_OPFORFrontlineGarrisonManager
{
	bool Probe(vector target, out vector result, out string reason)
	{
		array<vector> observers = {};
		return FindLocalPosition(target, 0, observers, result, reason);
	}
}

class TB_CC_M4_4I_PlacementProbe
{
	protected static int s_Attempts;
	protected static ref array<string> s_Completed = {};
	static void Run()
	{
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated) { GetGame().GetCallqueue().Remove(Run); return; }
		s_Attempts++;
		TB_CC_M4_4I_GarrisonPlacementProbe garrison = new TB_CC_M4_4I_GarrisonPlacementProbe();
		garrison.Configure("STANDARD", 250, 900, 250, 55, 32);
		array<string> ids = {"ARLAND::BEAUREGARD_HARBOUR", "ARLAND::SIGNAL_HILL", "ARLAND::NORTH_RUNWAY", "ARLAND::GULL_ISLAND"};
		foreach (string id : ids)
		{
			if (s_Completed.Contains(id)) continue;
			vector target; vector placement; string failure;
			if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, target)) { PrintFormat("[TB_CC_M4_4I_PROBE] FAIL semantic-position id=%1", id); s_Completed.Insert(id); continue; }
			bool valid = garrison.Probe(target, placement, failure);
			if (valid || s_Attempts >= 5)
			{
				string result = "UNSUPPORTED"; if (valid) result = "PASS";
				PrintFormat("[TB_CC_M4_4I_PROBE] %1 local-garrison-placement id=%2 position=%3 reason=%4 attempts=%5 actorsCreated=0 humanOverride=0", result, id, placement, failure, s_Attempts);
				s_Completed.Insert(id);
			}
		}
		if (s_Completed.Count() == ids.Count()) { GetGame().GetCallqueue().Remove(Run); Print("[TB_CC_M4_4I_PROBE] COMPLETE readOnly=1"); }
	}
}
