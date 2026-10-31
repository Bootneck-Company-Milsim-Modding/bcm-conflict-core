enum TB_PVE_M3_5_PlayerBand { NONE = 0, LOW = 1, MEDIUM = 2, HIGH = 3 }
enum TB_PVE_M3_5_DepthBand { INVALID = 0, EARLY = 1, MID = 2, LATE = 3 }

class TB_PVE_M3_5_SelectionDecision
{
	bool m_bSelected;
	string m_sPackageId;
	string m_sReason;
	int m_iHumanBluforCount;
	int m_iPlayerBand;
	int m_iDepthPermille;
	int m_iDepthBand;
	int m_iPreviousResult;
	int m_iBaseStrength;
	int m_iDepthModifier;
	int m_iResultModifier;
	int m_iFinalStrength;
	int m_iRevision;
}

// Server-authoritative, deterministic package selection only. This service
// neither creates operations nor materializes groups.
class TB_PVE_M3_5_ForceSelectionPolicy
{
	protected int m_iMediumPlayers = 2;
	protected int m_iHighPlayers = 4;
	protected int m_iEarlyMaximumPermille = 334;
	protected int m_iMidMaximumPermille = 667;
	protected int m_iMidDepthModifier = 1;
	protected int m_iLateDepthModifier = 2;
	protected int m_iFailedResultModifier = 1;
	protected string m_sLightPackage = "LIGHT";
	protected string m_sStandardPackage = "STANDARD";
	protected string m_sReinforcedPackage = "REINFORCED";
	protected int m_iProofHumanCount = -1;
	protected string m_sProofForcedPackage;

	void Configure(int mediumPlayers, int highPlayers, int earlyMaximumPermille, int midMaximumPermille, int midDepthModifier, int lateDepthModifier, int failedResultModifier, string lightPackage, string standardPackage, string reinforcedPackage)
	{
		m_iMediumPlayers = mediumPlayers; m_iHighPlayers = highPlayers;
		m_iEarlyMaximumPermille = earlyMaximumPermille; m_iMidMaximumPermille = midMaximumPermille;
		m_iMidDepthModifier = midDepthModifier; m_iLateDepthModifier = lateDepthModifier; m_iFailedResultModifier = failedResultModifier;
		m_sLightPackage = lightPackage; m_sStandardPackage = standardPackage; m_sReinforcedPackage = reinforcedPackage;
	}

	int CountActiveHumanBLUFOR()
	{
		if (m_iProofHumanCount >= 0) return m_iProofHumanCount;
		return TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US");
	}

	TB_PVE_M3_5_SelectionDecision SelectAttackPackage(int previousResult)
	{
		int depth = TB_PVE_M2_3_FrontierQuery.GetCampaignDepthPermille();
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && policy.GetTerritorialOwner(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective()) == "FIA") depth = policy.GetFactionDepth("USSR");
		// USSR expansion uses its own progress; responses against US retain the
		// accepted US-penetration strength signal, rather than conflating fronts.
		// A native remote USSR-vs-FIA operation may use the baseline at zero real
		// humans. Do not fabricate a participant or change human-team selection.
		bool background = TB_CC_WarSimulation.Get() && policy && policy.GetTerritorialOwner(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective()) == "FIA";
		return SelectFromContext(CountActiveHumanBLUFOR(), depth, previousResult, TB_PVE_M2_3_FrontierQuery.GetRevision(), true, background);
	}

	TB_PVE_M3_5_SelectionDecision SelectFromContextForProof(int humanCount, int depthPermille, int previousResult)
	{
		return SelectFromContext(humanCount, depthPermille, previousResult, TB_PVE_M2_3_FrontierQuery.GetRevision(), true);
	}

	protected TB_PVE_M3_5_SelectionDecision SelectFromContext(int humanCount, int depthPermille, int previousResult, int revision, bool emitLog, bool background = false)
	{
		TB_PVE_M3_5_SelectionDecision decision = new TB_PVE_M3_5_SelectionDecision();
		decision.m_iHumanBluforCount = humanCount; decision.m_iDepthPermille = depthPermille; decision.m_iPreviousResult = previousResult; decision.m_iRevision = revision;
		if (!Replication.IsServer()) return Reject(decision, "authority-required", emitLog);
		if (!ConfigurationValid()) return Reject(decision, "invalid-configuration", emitLog);
		if (humanCount <= 0 && !background) return Reject(decision, "no-active-human-blufor", emitLog);
		if (depthPermille < 0 || depthPermille > 1000) return Reject(decision, "strategic-depth-not-ready", emitLog);

		if (humanCount >= m_iHighPlayers) { decision.m_iPlayerBand = TB_PVE_M3_5_PlayerBand.HIGH; decision.m_iBaseStrength = 2; }
		else if (humanCount >= m_iMediumPlayers) { decision.m_iPlayerBand = TB_PVE_M3_5_PlayerBand.MEDIUM; decision.m_iBaseStrength = 1; }
		else { decision.m_iPlayerBand = TB_PVE_M3_5_PlayerBand.LOW; decision.m_iBaseStrength = 0; }
		if (humanCount <= 0) decision.m_iPlayerBand = TB_PVE_M3_5_PlayerBand.NONE;

		if (depthPermille <= m_iEarlyMaximumPermille) decision.m_iDepthBand = TB_PVE_M3_5_DepthBand.EARLY;
		else if (depthPermille <= m_iMidMaximumPermille) { decision.m_iDepthBand = TB_PVE_M3_5_DepthBand.MID; decision.m_iDepthModifier = m_iMidDepthModifier; }
		else { decision.m_iDepthBand = TB_PVE_M3_5_DepthBand.LATE; decision.m_iDepthModifier = m_iLateDepthModifier; }
		if (previousResult == TB_PVE_M3_1_OperationState.FAILED) decision.m_iResultModifier = m_iFailedResultModifier;
		decision.m_iFinalStrength = Math.Clamp(decision.m_iBaseStrength + decision.m_iDepthModifier + decision.m_iResultModifier, 0, 2);
		if (decision.m_iFinalStrength == 0) decision.m_sPackageId = m_sLightPackage;
		else if (decision.m_iFinalStrength == 1) decision.m_sPackageId = m_sStandardPackage;
		else decision.m_sPackageId = m_sReinforcedPackage;
		// Extend the existing high-strength band for larger human teams. The
		// depth/result calculation still determines whether this band is reached;
		// human count selects ASSAULT/HEAVY/SURGE within configured package bounds.
		if (decision.m_iFinalStrength == 2 && humanCount >= 12) decision.m_sPackageId = "SURGE";
		else if (decision.m_iFinalStrength == 2 && humanCount >= 8) decision.m_sPackageId = "HEAVY";
		else if (decision.m_iFinalStrength == 2 && humanCount >= 5) decision.m_sPackageId = "ASSAULT";
		if (!m_sProofForcedPackage.IsEmpty()) decision.m_sPackageId = m_sProofForcedPackage;
		TB_PVE_M3_4_ForcePackage package = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(decision.m_sPackageId);
		if (!package || package.m_sFactionKey != "USSR" || package.GetExpectedGroupCount() < 1 || package.GetExpectedGroupCount() > 8) return Reject(decision, "selected-package-invalid", emitLog);
		decision.m_bSelected = true; decision.m_sReason = "deterministic-strength-selection";
		if (emitLog) { PrintDecision(decision); PrintFormat("[TB_PVE_M3_5] selected-force humans=%1 depth=%2 previous=%3 package=%4 groups=%5 expectedUnitsAtThreeSlots=%6", humanCount, depthPermille, previousResult, decision.m_sPackageId, package.GetExpectedGroupCount(), package.GetExpectedGroupCount() * 3); }
		return decision;
	}

	protected bool ConfigurationValid()
	{
		return m_iMediumPlayers >= 1 && m_iHighPlayers > m_iMediumPlayers && m_iEarlyMaximumPermille >= 0 && m_iMidMaximumPermille > m_iEarlyMaximumPermille && m_iMidMaximumPermille <= 1000 && m_iMidDepthModifier >= 0 && m_iLateDepthModifier >= m_iMidDepthModifier && m_iFailedResultModifier >= 0;
	}
	protected TB_PVE_M3_5_SelectionDecision Reject(TB_PVE_M3_5_SelectionDecision decision, string reason, bool emitLog)
	{
		decision.m_bSelected = false; decision.m_sPackageId = ""; decision.m_sReason = reason;
		if (emitLog) PrintDecision(decision); return decision;
	}
	protected void PrintDecision(TB_PVE_M3_5_SelectionDecision decision)
	{
		PrintFormat("[TB_PVE_M3_5] selection selected=%1 humans=%2 playerBand=%3 depthPermille=%4 depthBand=%5 previous=%6 base=%7 depthModifier=%8 resultModifier=%9", decision.m_bSelected, decision.m_iHumanBluforCount, decision.m_iPlayerBand, decision.m_iDepthPermille, decision.m_iDepthBand, decision.m_iPreviousResult, decision.m_iBaseStrength, decision.m_iDepthModifier, decision.m_iResultModifier);
		PrintFormat("[TB_PVE_M3_5] selection-result package=%1 strength=%2 revision=%3 reason=%4", decision.m_sPackageId, decision.m_iFinalStrength, decision.m_iRevision, decision.m_sReason);
	}

	void SetProofHumanCount(int count) { m_iProofHumanCount = count; }
	void ClearProofHumanCount() { m_iProofHumanCount = -1; }
	void SetForcedPackageForProof(string packageId) { m_sProofForcedPackage = packageId; }
	void ClearForcedPackageForProof() { m_sProofForcedPackage = ""; }
	void SetInvalidConfigurationForProof() { m_iHighPlayers = m_iMediumPlayers; }
}
