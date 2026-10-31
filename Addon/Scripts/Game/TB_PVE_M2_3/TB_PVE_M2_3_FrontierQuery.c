enum TB_PVE_M2_3_QueryReadiness
{
	NOT_READY = 0,
	READY = 1
}

enum TB_PVE_M2_5_CampaignOutcome
{
	NOT_READY = 0,
	ACTIVE = 1,
	BLUFOR_VICTORY_PRESSURE = 2,
	OPFOR_VICTORY_PRESSURE = 3,
	BLUFOR_VICTORY = 4,
	OPFOR_VICTORY = 5,
	INVALID = 6
}

enum TB_PVE_M2_3_ObjectiveStrategicState
{
	NOT_READY = 0,
	INVALID = 1,
	FRIENDLY_REAR = 2,
	FRIENDLY_FRONTLINE = 3,
	ENEMY_FRONTLINE = 4,
	ENEMY_REAR_LOCKED = 5
}

// Stable read-only semantic boundary for future consumers. Raw projection
// ingestion is internal to the game-mode replication adapter below.
class TB_PVE_M2_3_FrontierQuery
{
	protected static ref ScriptInvoker s_OnStrategicStateChanged = new ScriptInvoker();
	protected static bool s_bReady;
	protected static int s_iRevision;
	protected static int s_iValidity;
	protected static int s_iFrontier = -1;
	protected static int s_iBluforLegal = -1;
	protected static int s_iOpforLegal = -1;
	protected static int s_iLockedMask;
	protected static int s_iOutcome = TB_PVE_M2_5_CampaignOutcome.NOT_READY;
	protected static ref array<string> s_aIds = {};
	protected static ref array<vector> s_aObjectivePositions = {};
	protected static int s_iGraphDepthPermille = -1;
	protected static ref array<int> s_aGraphStates = {};
	protected static ref array<string> s_aGraphCandidates = {};
	protected static string s_sSelectedMobId;
	protected static string s_sSelectedEndpointId;
	protected static int s_iVictoryPressureRemainingSeconds = -1;

	// Called by the authored route component on both authority and proxies before
	// the first snapshot. Consumer methods remain independent of representation.
	static void TB_M2_3_ConfigureRoute(array<string> objectiveIds)
	{
		s_aIds.Clear();
		if (!objectiveIds) return;
		foreach (string objectiveId : objectiveIds)
			s_aIds.Insert(objectiveId);
	}

	// Public read-only spatial resolution for consumers that have already
	// obtained an objective ID through the semantic targeting contract.
	static void TB_M2_3_ConfigureObjectivePositions(array<vector> objectivePositions)
	{
		s_aObjectivePositions.Clear();
		if (!objectivePositions) return;
		foreach (vector position : objectivePositions)
			s_aObjectivePositions.Insert(position);
	}

	static bool TryGetObjectiveWorldPosition(string objectiveId, out vector position)
	{
		int index = s_aIds.Find(objectiveId);
		if (index < 0 || index >= s_aObjectivePositions.Count()) return false;
		position = s_aObjectivePositions[index];
		return true;
	}

	static ScriptInvoker GetOnStrategicStateChanged() { return s_OnStrategicStateChanged; }
	static int GetRouteObjectiveCount() { return s_aIds.Count(); }
	// Public normalized conventional campaign depth. 0 is the BLUFOR endpoint;
	// 1000 is the OPFOR endpoint. Consumers never need route arrays or indices.
	static int GetCampaignDepthPermille()
	{
		if (s_bReady && s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_GRAPH) return s_iGraphDepthPermille;
		if (!s_bReady || s_iValidity != TB_PVE_M2_2_CampaignValidity.VALID_LINEAR_FRONTIER || s_aIds.Count() < 2 || s_iFrontier < 0) return -1;
		return (s_iFrontier * 1000) / (s_aIds.Count() - 1);
	}
	static bool IsCampaignValid()
	{
		return s_bReady && (s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_LINEAR_FRONTIER || s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_GRAPH);
	}
	static string GetSelectedMobId() { return s_sSelectedMobId; }
	static string GetSelectedEndpointId() { return s_sSelectedEndpointId; }
	static void GetBLUFORCandidateObjectives(out array<string> ids) { ids.Clear(); foreach (string id : s_aGraphCandidates) ids.Insert(id); }
	static string GetRouteObjectiveId(int index)
	{
		if (index < 0 || index >= s_aIds.Count()) return "";
		else return s_aIds[index];
	}
	static int GetReadiness()
	{
		if (s_bReady) return TB_PVE_M2_3_QueryReadiness.READY;
		else return TB_PVE_M2_3_QueryReadiness.NOT_READY;
	}
	static int GetCampaignValidity()
	{
		if (!s_bReady) return TB_PVE_M2_2_CampaignValidity.UNRESOLVED;
		else return s_iValidity;
	}
	static int GetRevision()
	{
		if (!s_bReady) return 0;
		else return s_iRevision;
	}
	static int GetCampaignOutcome()
	{
		if (!s_bReady) return TB_PVE_M2_5_CampaignOutcome.NOT_READY;
		return s_iOutcome;
	}
	static bool IsVictoryPressureActive()
	{
		return s_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE || s_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE;
	}
	static int GetVictoryPressureRemainingSeconds()
	{
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (mode && s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_GRAPH) return mode.TB_CC_GetPressureRemaining();
		return s_iVictoryPressureRemainingSeconds;
	}
	static string GetBLUFORLegalObjective()
	{
		if (!s_bReady || s_iBluforLegal < 0 || s_iBluforLegal >= s_aIds.Count()) return "";
		else return s_aIds[s_iBluforLegal];
	}
	static string GetOPFORLegalObjective()
	{
		if (!s_bReady || s_iOpforLegal < 0 || s_iOpforLegal >= s_aIds.Count()) return "";
		else return s_aIds[s_iOpforLegal];
	}

	static int GetObjectiveStrategicState(string objectiveId)
	{
		if (!s_bReady) return TB_PVE_M2_3_ObjectiveStrategicState.NOT_READY;
		int index = s_aIds.Find(objectiveId);
		if (index < 0 || !IsCampaignValid())
			return TB_PVE_M2_3_ObjectiveStrategicState.INVALID;
		if (s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_GRAPH && index < s_aGraphStates.Count()) return s_aGraphStates[index];
		if (index == s_iBluforLegal) return TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_FRONTLINE;
		if (index == s_iOpforLegal) return TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_FRONTLINE;
		if (index <= s_iFrontier) return TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_REAR;
		return TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_REAR_LOCKED;
	}

	static bool IsObjectiveStrategicallyLocked(string objectiveId)
	{
		if (!IsCampaignValid()) return true;
		int index = s_aIds.Find(objectiveId);
		if (index < 0) return true;
		if (s_iValidity == TB_PVE_M2_2_CampaignValidity.VALID_GRAPH) return index != s_iBluforLegal && index != s_iOpforLegal;
		return (s_iLockedMask & (1 << index)) != 0;
	}

	static bool CanFactionStrategicallyAttack(FactionKey factionKey, string objectiveId)
	{
		if (!IsCampaignValid()) return false;
		int index = s_aIds.Find(objectiveId);
		if (factionKey == "US") return (s_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || s_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) && index == s_iBluforLegal;
		if (factionKey == "USSR") return (s_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || s_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) && index == s_iOpforLegal;
		return false;
	}

	static bool TB_M4_4_ConsumeGraphProjection(int revision, int validity, int outcome, string mobId, string endpointId, string bluforLegalId, string opforLegalId, int depthPermille, int pressureRemainingSeconds, array<string> ids, array<vector> positions, array<int> states, array<string> candidates, string source)
	{
		if (s_bReady && revision < s_iRevision) return false;
		if (s_bReady && revision == s_iRevision && s_iValidity == validity && s_iOutcome == outcome && GetBLUFORLegalObjective() == bluforLegalId && GetOPFORLegalObjective() == opforLegalId) return false;
		s_aIds.Clear(); s_aObjectivePositions.Clear(); s_aGraphStates.Clear(); s_aGraphCandidates.Clear();
		foreach (string id : ids) s_aIds.Insert(id);
		foreach (vector position : positions) s_aObjectivePositions.Insert(position);
		foreach (int state : states) s_aGraphStates.Insert(state);
		foreach (string candidate : candidates) s_aGraphCandidates.Insert(candidate);
		s_iRevision = revision; s_iValidity = validity; s_iOutcome = outcome; s_iGraphDepthPermille = depthPermille; s_iVictoryPressureRemainingSeconds = pressureRemainingSeconds;
		s_sSelectedMobId = mobId; s_sSelectedEndpointId = endpointId;
		s_iBluforLegal = s_aIds.Find(bluforLegalId); s_iOpforLegal = s_aIds.Find(opforLegalId); s_iFrontier = -1; s_iLockedMask = 0; s_bReady = true;
		PrintFormat("[TB_CC_M4_4] projection-accepted source=%1 revision=%2 validity=%3 outcome=%4 mob=%5 endpoint=%6 BLUFOR=%7 OPFOR=%8 depth=%9", source, revision, validity, outcome, mobId, endpointId, bluforLegalId, opforLegalId, depthPermille);
		PrintFormat("[TB_CC_M4_4H] projection-pressure remainingSeconds=%1 candidateCount=%2", pressureRemainingSeconds, candidates.Count());
		s_OnStrategicStateChanged.Invoke();
		return true;
	}

	static bool TB_M2_3_ConsumeProjection(int revision, int validity, int frontier, int bluforLegal, int opforLegal, int lockedMask, int outcome, string source)
	{
		if (s_bReady && revision < s_iRevision)
		{
			PrintFormat("[TB_PVE_M2_3] stale-revision-rejected current=%1 incoming=%2 source=%3", s_iRevision, revision, source);
			return false;
		}
		if (s_bReady && revision == s_iRevision)
		{
			bool identical = s_iValidity == validity && s_iFrontier == frontier && s_iBluforLegal == bluforLegal && s_iOpforLegal == opforLegal && s_iLockedMask == lockedMask && s_iOutcome == outcome;
			if (identical)
				PrintFormat("[TB_PVE_M2_3] duplicate-revision-ignored revision=%1 source=%2 notification=0", revision, source);
			else
				PrintFormat("[TB_PVE_M2_3] ERROR inconsistent-duplicate-revision revision=%1 source=%2 rejected=1", revision, source);
			return false;
		}

		s_bReady = true;
		s_iRevision = revision;
		s_iValidity = validity;
		s_iFrontier = frontier;
		s_iBluforLegal = bluforLegal;
		s_iOpforLegal = opforLegal;
		s_iLockedMask = lockedMask;
		s_iOutcome = outcome;
		PrintFormat("[TB_PVE_M2_3] snapshot-accepted revision=%1 source=%2 validity=%3 outcome=%4 USLegal=%5 USSRLegal=%6", revision, source, validity, outcome, GetBLUFORLegalObjective(), GetOPFORLegalObjective());
		s_OnStrategicStateChanged.Invoke();
		return true;
	}

	static void TB_M2_3_RunRevisionDiagnostics()
	{
		if (!s_bReady || s_iRevision < 2) return;
		TB_M2_3_ConsumeProjection(s_iRevision - 1, s_iValidity, s_iFrontier, s_iBluforLegal, s_iOpforLegal, s_iLockedMask, s_iOutcome, "diagnostic-stale");
		TB_M2_3_ConsumeProjection(s_iRevision, s_iValidity, s_iFrontier, s_iBluforLegal, s_iOpforLegal, s_iLockedMask, s_iOutcome, "diagnostic-duplicate");
		TB_M2_3_ConsumeProjection(s_iRevision, s_iValidity, s_iFrontier + 1, s_iBluforLegal, s_iOpforLegal, s_iLockedMask, s_iOutcome, "diagnostic-inconsistent-duplicate");
	}
}

// Proof consumer: depends only on the public semantic contract above.
class TB_PVE_M2_3_DiagnosticConsumer
{
	protected static bool s_bStarted;

	static void Start()
	{
		if (s_bStarted) return;
		s_bStarted = true;
		TB_PVE_M2_3_FrontierQuery.GetOnStrategicStateChanged().Insert(OnStrategicStateChanged);
		PrintFormat("[TB_PVE_M2_3][CONSUMER] started readiness=%1 revision=%2", TB_PVE_M2_3_FrontierQuery.GetReadiness(), TB_PVE_M2_3_FrontierQuery.GetRevision());
	}

	protected static void OnStrategicStateChanged()
	{
		PrintFormat("[TB_PVE_M2_3][CONSUMER] revision=%1 validity=%2 outcome=%3 BLUFOR=%4 OPFOR=%5", TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetCampaignValidity(), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
		if (!Replication.IsServer())
		{
			int frontier;
			for (int routeIndex = 1; routeIndex < TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveCount(); routeIndex++)
			{
				int state = TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveId(routeIndex));
				if (state == TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_REAR || state == TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_FRONTLINE)
					frontier = routeIndex;
			}
			Print("[TB_PVE_M2_4A] CLIENT_READY");
			PrintFormat("[TB_PVE_M2_4A] revision=%1", TB_PVE_M2_3_FrontierQuery.GetRevision());
			PrintFormat("[TB_PVE_M2_4A] frontier=%1", frontier);
			PrintFormat("[TB_PVE_M2_4A] USLegal=%1", TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective());
			PrintFormat("[TB_PVE_M2_4A] USSRLegal=%1", TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
			if (TB_PVE_M2_3_FrontierQuery.GetCampaignValidity() == TB_PVE_M2_2_CampaignValidity.VALID_LINEAR_FRONTIER)
				Print("[TB_PVE_M2_4A] validity=VALID");
			else
				PrintFormat("[TB_PVE_M2_4A] validity=INVALID value=%1", TB_PVE_M2_3_FrontierQuery.GetCampaignValidity());
		}
		for (int i = 1; i < TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveCount(); i++)
		{
			string objectiveId = TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveId(i);
			PrintFormat("[TB_PVE_M2_3][CONSUMER] id=%1 state=%2 locked=%3 FIAAttack=%4", objectiveId, TB_PVE_M2_3_FrontierQuery.GetObjectiveStrategicState(objectiveId), TB_PVE_M2_3_FrontierQuery.IsObjectiveStrategicallyLocked(objectiveId), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("FIA", objectiveId));
		}
	}
}
