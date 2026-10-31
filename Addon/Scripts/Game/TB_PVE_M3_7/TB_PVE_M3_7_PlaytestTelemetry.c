// Read-only, low-frequency production telemetry for the manual M3.7 session.
// It creates no actors and changes no campaign, operation or player state.
class TB_PVE_M3_7_PlaytestTelemetry
{
	protected TB_PVE_M3_2_OPFOROperationCoordinator m_Coordinator;
	protected TB_PVE_M3_5_ForceSelectionPolicy m_SelectionPolicy;
	protected TB_PVE_M3_6_OPFORFrontlineGarrisonManager m_GarrisonManager;
	protected bool m_bRunning;
	protected string m_sLastSnapshot;

	void Start(TB_PVE_M3_2_OPFOROperationCoordinator coordinator, TB_PVE_M3_5_ForceSelectionPolicy selectionPolicy, TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrisonManager)
	{
		if (!Replication.IsServer() || m_bRunning) return;
		m_Coordinator = coordinator; m_SelectionPolicy = selectionPolicy; m_GarrisonManager = garrisonManager; m_bRunning = true;
		Print("[TB_PVE_M3_7] telemetry-start authority=1 readOnly=1 intervalSeconds=10 proofActions=0");
		GetGame().GetCallqueue().CallLater(Tick, 10000, true);
		Tick();
	}

	void Stop()
	{
		if (!m_bRunning) return;
		m_bRunning = false; GetGame().GetCallqueue().Remove(Tick);
		Print("[TB_PVE_M3_7] telemetry-stop");
	}

	protected void Tick()
	{
		if (!m_bRunning) return;
		TB_PVE_M3_7_HumanPresenceQuery.TraceAuthorityState("US");
		int humans;
		if (m_SelectionPolicy) humans = m_SelectionPolicy.CountActiveHumanBLUFOR();
		int coordinatorState = -1; int operationState = -1; int attackGroups; int attackUnits; int attackLOD = -1; string packageId; string operationId; string attackTarget;
		if (m_Coordinator)
		{
			coordinatorState = m_Coordinator.GetState();
			TB_PVE_M3_1_Operation operation = m_Coordinator.GetActiveOperation();
			if (operation)
			{
				operationId = operation.GetOperationId(); attackTarget = operation.GetTargetObjectiveId(); packageId = operation.GetPackageId(); operationState = operation.GetState();
				attackGroups = operation.GetViableAssignedGroupCount(); attackUnits = operation.GetAssignedUnitCount(); attackLOD = operation.GetAssignedGroupLOD();
			}
		}
		string garrisonObjective; int garrisonState; int garrisonGroups; int garrisonUnits; int garrisonLOD = -1;
		if (m_GarrisonManager)
		{
			garrisonObjective = m_GarrisonManager.GetObjectiveId(); garrisonState = m_GarrisonManager.GetState(); garrisonGroups = m_GarrisonManager.GetGroupCount(); garrisonUnits = m_GarrisonManager.GetUnitCount(); garrisonLOD = m_GarrisonManager.GetFirstGroupLOD();
		}
		string snapshot = humans.ToString() + ":" + TB_PVE_M2_3_FrontierQuery.GetRevision().ToString();
		snapshot += ":" + coordinatorState.ToString() + ":" + operationState.ToString();
		snapshot += ":" + garrisonObjective + ":" + garrisonState.ToString() + ":" + garrisonUnits.ToString();
		if (snapshot == m_sLastSnapshot) return;
		m_sLastSnapshot = snapshot;
		PrintFormat("[TB_PVE_M3_7] snapshot humansUS=%1 readiness=%2 validity=%3 outcome=%4 revision=%5 BLUFORLegal=%6 OPFORLegal=%7", humans, TB_PVE_M2_3_FrontierQuery.GetReadiness(), TB_PVE_M2_3_FrontierQuery.GetCampaignValidity(), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
		PrintFormat("[TB_PVE_M3_7] garrison objective=%1 state=%2 groups=%3 units=%4 LOD=%5 coordinatorState=%6", garrisonObjective, garrisonState, garrisonGroups, garrisonUnits, garrisonLOD, coordinatorState);
		PrintFormat("[TB_PVE_M3_7] operation id=%1 state=%2 target=%3 package=%4 groups=%5 units=%6 LOD=%7", operationId, operationState, attackTarget, packageId, attackGroups, attackUnits, attackLOD);
	}
}
