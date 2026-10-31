enum TB_PVE_M3_2_CoordinatorState
{
	NOT_READY = 0,
	IDLE = 1,
	WAITING = 2,
	OPERATION_ACTIVE = 3,
	COOLDOWN = 4,
	SUSPENDED = 5
}

// Authority-only owner of zero or one conventional OPFOR operation. Strategic
// answers come exclusively from the M2.3 public semantic contract.
class TB_PVE_M3_2_OPFOROperationCoordinator
{
	protected int m_iState = TB_PVE_M3_2_CoordinatorState.NOT_READY;
	protected ref TB_PVE_M3_1_Operation m_ActiveOperation;
	protected TB_PVE_M3_3_TacticalStagingPolicy m_StagingPolicy;
	protected TB_PVE_M3_4_ForceAssemblyService m_AssemblyService;
	protected TB_PVE_M3_5_ForceSelectionPolicy m_SelectionPolicy;
	protected int m_iInitialDelaySeconds;
	protected int m_iCooldownSeconds;
	protected int m_iRemainingSeconds;
	protected int m_iOperationElapsedSeconds;
	protected int m_iOperationSerial;
	protected int m_iCreatedOperations;
	protected int m_iConcurrentOperations;
	protected int m_iMaximumConcurrentOperations;
	protected int m_iLastTerminalState = -1;
	protected string m_sWaitingTarget;
	protected int m_iWaitingRevision;
	protected bool m_bRunning;
	protected ref map<string, int> m_mTargetFailures = new map<string, int>();
	// Strategic escalation only. Active operations/actors are not persisted here.
	void ExportStrategicFailures(map<string, string> values, array<string> ids)
	{
		foreach (string id : ids) values.Set("USSR-failures:" + id, m_mTargetFailures.Get(id).ToString());
		int cooldown;
		if (m_iState == TB_PVE_M3_2_CoordinatorState.COOLDOWN) cooldown = Math.Max(m_iRemainingSeconds, 0);
		values.Set("USSR-cooldown", cooldown.ToString());
	}
	void RestoreStrategicFailures(map<string, string> values, array<string> ids)
	{
		m_mTargetFailures.Clear();
		foreach (string id : ids) m_mTargetFailures.Set(id, values.Get("USSR-failures:" + id).ToInt());
		int cooldown = values.Get("USSR-cooldown").ToInt();
		if (cooldown > 0) { m_iRemainingSeconds = cooldown; m_iState = TB_PVE_M3_2_CoordinatorState.COOLDOWN; }
	}
	protected ref array<ref TB_CC_ConcurrentOperationSlot> m_aAdditionalOperations = {};
	protected int m_iNextAdditionalAttemptMs;
	protected int Capacity()
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy) return policy.GetMaximumConcurrentUSSROperations();
		return 1;
	}

	void Start(TB_PVE_M3_3_TacticalStagingPolicy stagingPolicy, TB_PVE_M3_4_ForceAssemblyService assemblyService, TB_PVE_M3_5_ForceSelectionPolicy selectionPolicy, int initialDelaySeconds, int cooldownSeconds)
	{
		if (!Replication.IsServer() || m_bRunning) return;
		m_StagingPolicy = stagingPolicy;
		m_AssemblyService = assemblyService;
		m_SelectionPolicy = selectionPolicy;
		m_iInitialDelaySeconds = Math.Max(initialDelaySeconds, 0);
		m_iCooldownSeconds = Math.Max(cooldownSeconds, 0);
		m_bRunning = true;
		PrintFormat("[TB_PVE_M3_2] initialized server=1 faction=USSR maxOperations=%1 initialDelay=%2 cooldown=%3 stagingPolicy=%4 selector=%5 assemblyService=%6", Capacity(), m_iInitialDelaySeconds, m_iCooldownSeconds, m_StagingPolicy != null, m_SelectionPolicy != null, m_AssemblyService != null);
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	void Stop(string reason)
	{
		if (!m_bRunning) return;
		m_bRunning = false;
		GetGame().GetCallqueue().Remove(Tick);
		foreach (TB_CC_ConcurrentOperationSlot slot : m_aAdditionalOperations) slot.m_Operation.CleanupAssignedForce("coordinator-stop-" + reason);
		m_aAdditionalOperations.Clear();
		if (m_ActiveOperation)
		{
			m_ActiveOperation.CleanupAssignedForce("coordinator-stop-" + reason);
			m_ActiveOperation = null;
		}
		m_iConcurrentOperations = 0;
		PrintFormat("[TB_PVE_M3_2] cleanup reason=%1 state=%2 activeReference=0 maximumConcurrent=%3 created=%4", reason, m_iState, m_iMaximumConcurrentOperations, m_iCreatedOperations);
	}

	protected void Tick()
	{
		if (!m_bRunning) return;
		if (TB_PVE_M2_3_FrontierQuery.GetReadiness() != TB_PVE_M2_3_QueryReadiness.READY)
		{
			Transition(TB_PVE_M3_2_CoordinatorState.NOT_READY, "campaign-query-not-ready");
			return;
		}
		TickAdditionalOperations();

		int outcome = TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome();
		int validity = TB_PVE_M2_3_FrontierQuery.GetCampaignValidity();
		TB_CC_M4XP_NativeArlandComponent campaignPolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		bool operational = outcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		if (campaignPolicy) operational = campaignPolicy.IsWarOperational();
		if (!operational || !TB_PVE_M2_3_FrontierQuery.IsCampaignValid())
		{
			if (m_ActiveOperation)
			{
				m_iOperationElapsedSeconds += 2;
				m_ActiveOperation.Tick(m_iOperationElapsedSeconds);
				if (m_ActiveOperation.IsTerminal()) DisposeTerminalOperation("campaign-suspended");
			}
			Transition(TB_PVE_M3_2_CoordinatorState.SUSPENDED, "campaign-terminal-or-invalid");
			return;
		}

		if (m_ActiveOperation)
		{
			m_iOperationElapsedSeconds += 2;
			m_ActiveOperation.Tick(m_iOperationElapsedSeconds);
			if (m_ActiveOperation.IsTerminal())
			{
				DisposeTerminalOperation("operation-terminal");
				BeginCooldown();
			}
			else if ((!campaignPolicy || !campaignPolicy.IsSovietEmergency()) && Capacity() > m_aAdditionalOperations.Count() + 1 && System.GetTickCount() >= m_iNextAdditionalAttemptMs)
			{
				m_iNextAdditionalAttemptMs = System.GetTickCount() + Math.Max(m_iCooldownSeconds, 10) * 1000;
				CreateOperation();
			}
			return;
		}

		// Emergency queue dispatch runs in the native campaign tick. Keep existing
		// operations ticking, but do not consume its capacity with ordinary delay work.
		if (campaignPolicy && campaignPolicy.IsSovietEmergency()) return;
		if (m_iState == TB_PVE_M3_2_CoordinatorState.COOLDOWN)
		{
			m_iRemainingSeconds -= 2;
			if (m_iRemainingSeconds > 0) return;
			PrintFormat("[TB_PVE_M3_2] cooldown-complete revision=%1 requery=1", TB_PVE_M2_3_FrontierQuery.GetRevision());
			Transition(TB_PVE_M3_2_CoordinatorState.IDLE, "cooldown-complete");
		}

		string target = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective();
		bool authorized = !target.IsEmpty() && TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", target);
		if (!authorized)
		{
			m_sWaitingTarget = "";
			m_iRemainingSeconds = 0;
			Transition(TB_PVE_M3_2_CoordinatorState.IDLE, "active-campaign-no-legal-opfor-target");
			return;
		}
		TB_CC_M4XP_NativeArlandComponent nativePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		bool background = TB_CC_WarSimulation.Get() && nativePolicy && nativePolicy.GetTerritorialOwner(target) == "FIA";
		if (!m_SelectionPolicy || (m_SelectionPolicy.CountActiveHumanBLUFOR() <= 0 && !background))
		{
			m_sWaitingTarget = ""; m_iRemainingSeconds = 0;
			Transition(TB_PVE_M3_2_CoordinatorState.IDLE, "no-active-human-blufor");
			return;
		}

		if (m_iState != TB_PVE_M3_2_CoordinatorState.WAITING || target != m_sWaitingTarget)
		{
			m_sWaitingTarget = target;
			m_iWaitingRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
			m_iRemainingSeconds = m_iInitialDelaySeconds;
			Transition(TB_PVE_M3_2_CoordinatorState.WAITING, "legal-target-observed");
			PrintFormat("[TB_PVE_M3_2] waiting-start target=%1 revision=%2 delay=%3", m_sWaitingTarget, m_iWaitingRevision, m_iRemainingSeconds);
			return;
		}

		m_iRemainingSeconds -= 2;
		if (m_iRemainingSeconds > 0) return;
		CreateOperation();
	}

	protected void CreateOperation()
	{
		TB_CC_M4XP_NativeArlandComponent responsePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (responsePolicy && responsePolicy.GetResponses() && responsePolicy.GetResponses().HasDue("USSR", System.GetTickCount() + 2000)) return;
		string currentTarget = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective();
		int currentRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		if (TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE || currentTarget.IsEmpty() || !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", currentTarget))
		{
			Transition(TB_PVE_M3_2_CoordinatorState.IDLE, "delay-expired-target-invalid");
			return;
		}
		if (m_ActiveOperation && m_aAdditionalOperations.Count() + 1 >= Capacity())
		{
			Print("[TB_PVE_M3_2] ERROR duplicate-operation-prevented activeReference=1");
			return;
		}

		m_iOperationSerial++;
		int failures = m_mTargetFailures.Get(currentTarget);
		int previousResult = TB_PVE_M3_1_OperationState.CREATED;
		if (failures > 0) previousResult = TB_PVE_M3_1_OperationState.FAILED;
		TB_PVE_M3_5_SelectionDecision selection = m_SelectionPolicy.SelectAttackPackage(previousResult);
		if (selection && selection.m_bSelected && failures > 0)
		{
			string minimumPackage = "STANDARD";
			if (failures >= 3) minimumPackage = "HEAVY";
			else if (failures >= 2) minimumPackage = "REINFORCED";
			TB_PVE_M3_4_ForcePackage minimum = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(minimumPackage);
			TB_PVE_M3_4_ForcePackage selected = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(selection.m_sPackageId);
			if (selected && minimum && selected.GetExpectedGroupCount() < minimum.GetExpectedGroupCount())
			{
				selection.m_sPackageId = minimumPackage;
				selection.m_iFinalStrength = 1;
				if (failures >= 2) selection.m_iFinalStrength = 2;
				selection.m_sReason = "same-target-genuine-failure-escalation";
			}
			PrintFormat("[TB_CC_PHYSICAL_WAR] escalation target=%1 genuineFailures=%2 package=%3", currentTarget, failures, selection.m_sPackageId);
		}
		if (!selection || !selection.m_bSelected)
		{
			PrintFormat("[TB_PVE_M3_5] coordinator-create-denied reason=%1 revision=%2", selection && selection.m_sReason, currentRevision);
			Transition(TB_PVE_M3_2_CoordinatorState.IDLE, "selector-returned-no-package");
			return;
		}
		TB_PVE_M3_1_Operation operation = new TB_PVE_M3_1_Operation();
		string operationId = "M3_2_OP_" + m_iOperationSerial.ToString();
		if (!operation.Create(operationId, m_StagingPolicy, m_AssemblyService, selection.m_sPackageId))
		{
			operation.CleanupAssignedForce("coordinator-create-failed");
			m_iLastTerminalState = TB_PVE_M3_1_OperationState.FAILED;
			// Resource/staging/authorization/assembly failures are not combat losses.
			if (!m_ActiveOperation) BeginCooldown();
			return;
		}
		operation.SetSelectionSnapshot(selection);
		if (m_ActiveOperation)
		{
			TB_CC_ConcurrentOperationSlot slot = new TB_CC_ConcurrentOperationSlot(); slot.m_Operation = operation; m_aAdditionalOperations.Insert(slot);
		}
		else { m_ActiveOperation = operation; m_iOperationElapsedSeconds = 0; }
		m_iCreatedOperations++;
		m_iConcurrentOperations++;
		if (m_iConcurrentOperations > m_iMaximumConcurrentOperations) m_iMaximumConcurrentOperations = m_iConcurrentOperations;
		m_iNextAdditionalAttemptMs = System.GetTickCount() + Math.Max(m_iInitialDelaySeconds, 10) * 1000;
		Transition(TB_PVE_M3_2_CoordinatorState.OPERATION_ACTIVE, "operation-created");
		PrintFormat("[TB_PVE_M3_2] operation-created id=%1 serial=%2 target=%3 operationTarget=%4 revision=%5 operationRevision=%6 activeCount=%7 maximumConcurrent=%8 freshQuery=1", operationId, m_iOperationSerial, currentTarget, operation.GetTargetObjectiveId(), currentRevision, operation.GetAuthorizedRevision(), m_iConcurrentOperations, m_iMaximumConcurrentOperations);
	}

	protected void DisposeTerminalOperation(string reason)
	{
		if (!m_ActiveOperation) return;
		m_iLastTerminalState = m_ActiveOperation.GetState();
		string target = m_ActiveOperation.GetTargetObjectiveId();
		RecordTargetResult(m_ActiveOperation);
		PrintFormat("[TB_PVE_M3_2] operation-terminal id=%1 state=%2 target=%3 reason=%4 leaseHeld=%5", m_ActiveOperation.GetOperationId(), m_iLastTerminalState, m_ActiveOperation.GetTargetObjectiveId(), reason, m_ActiveOperation.IsLeaseHeld());
		m_ActiveOperation.CleanupAssignedForce("coordinator-dispose-terminal");
		m_ActiveOperation = null;
		m_iConcurrentOperations = m_aAdditionalOperations.Count();
		PrintFormat("[TB_PVE_M3_2] primary-reference-cleared remainingPoolCount=%1", m_iConcurrentOperations);
	}
	protected void TickAdditionalOperations()
	{
		for (int index = m_aAdditionalOperations.Count() - 1; index >= 0; index--)
		{
			TB_CC_ConcurrentOperationSlot slot = m_aAdditionalOperations[index];
			slot.m_iElapsedSeconds += 2; slot.m_Operation.Tick(slot.m_iElapsedSeconds);
			if (!slot.m_Operation.IsTerminal()) continue;
			string target = slot.m_Operation.GetTargetObjectiveId();
			RecordTargetResult(slot.m_Operation);
			slot.m_Operation.CleanupAssignedForce("additional-terminal"); m_aAdditionalOperations.Remove(index);
		}
		// Keep the compatibility primary accessor populated without dropping slots.
		if (!m_ActiveOperation && !m_aAdditionalOperations.IsEmpty())
		{
			TB_CC_ConcurrentOperationSlot promoted = m_aAdditionalOperations[0];
			m_ActiveOperation = promoted.m_Operation; m_iOperationElapsedSeconds = promoted.m_iElapsedSeconds; m_aAdditionalOperations.Remove(0);
		}
		m_iConcurrentOperations = m_aAdditionalOperations.Count(); if (m_ActiveOperation) m_iConcurrentOperations++;
	}

	void ResetTargetFailureHistory(string target)
	{
		m_mTargetFailures.Remove(target);
	}
	TB_PVE_M3_1_Operation TryLaunchResponse(TB_CC_ResponseRequest request, string packageId)
	{
		if (!m_bRunning || !Replication.IsServer() || m_iConcurrentOperations >= Capacity()) return null;
		TB_PVE_M3_5_SelectionDecision selection;
		if (m_SelectionPolicy) selection = m_SelectionPolicy.SelectAttackPackage(TB_PVE_M3_1_OperationState.CREATED);
		// Emergency profile is a floor, not a replacement for stronger selection.
		TB_PVE_M3_4_ForcePackage minimum = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(packageId);
		if (!minimum) return null;
		if (selection && selection.m_bSelected)
		{
			TB_PVE_M3_4_ForcePackage selected = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(selection.m_sPackageId);
			if (selected && selected.GetExpectedGroupCount() > minimum.GetExpectedGroupCount()) packageId = selection.m_sPackageId;
		}
		TB_PVE_M3_1_Operation operation = new TB_PVE_M3_1_Operation(); operation.ConfigureResponse(request);
		m_iOperationSerial++;
		if (!operation.Create("M3_RESPONSE_" + m_iOperationSerial.ToString(), m_StagingPolicy, m_AssemblyService, packageId))
		{ operation.CleanupAssignedForce("response-admission-deferred"); return null; }
		if (selection && selection.m_bSelected) { selection.m_sPackageId = packageId; operation.SetSelectionSnapshot(selection); }
		if (m_ActiveOperation)
		{
			TB_CC_ConcurrentOperationSlot slot = new TB_CC_ConcurrentOperationSlot(); slot.m_Operation = operation; m_aAdditionalOperations.Insert(slot);
		}
		else { m_ActiveOperation = operation; m_iOperationElapsedSeconds = 0; }
		m_iConcurrentOperations++; m_iCreatedOperations++;
		m_iMaximumConcurrentOperations = Math.Max(m_iMaximumConcurrentOperations, m_iConcurrentOperations);
		Transition(TB_PVE_M3_2_CoordinatorState.OPERATION_ACTIVE, "phased-response-admitted");
		return operation;
	}
	protected void RecordTargetResult(TB_PVE_M3_1_Operation operation)
	{
		string target = operation.GetTargetObjectiveId();
		if (operation.GetState() == TB_PVE_M3_1_OperationState.SUCCEEDED) { ResetTargetFailureHistory(target); return; }
		if (!operation.HasPhysicalFailureForEscalation()) return;
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && policy.GetTerritorialOwner(target) != operation.GetInitialTargetOwner()) return;
		m_mTargetFailures.Set(target, m_mTargetFailures.Get(target) + 1);
	}

	protected void BeginCooldown()
	{
		m_iRemainingSeconds = m_iCooldownSeconds;
		Transition(TB_PVE_M3_2_CoordinatorState.COOLDOWN, "terminal-operation-disposed");
		PrintFormat("[TB_PVE_M3_2] cooldown-start duration=%1 lastTerminalState=%2", m_iCooldownSeconds, m_iLastTerminalState);
	}

	protected void Transition(int nextState, string reason)
	{
		if (m_iState == nextState) return;
		PrintFormat("[TB_PVE_M3_2] state-transition from=%1 to=%2 reason=%3 revision=%4", m_iState, nextState, reason, TB_PVE_M2_3_FrontierQuery.GetRevision());
		m_iState = nextState;
	}

	int GetState() { return m_iState; }
	TB_PVE_M3_1_Operation GetActiveOperation() { return m_ActiveOperation; }
	bool HasActiveOperation() { return m_ActiveOperation != null; }
	int GetCreatedOperationCount() { return m_iCreatedOperations; }
	int GetMaximumConcurrentOperations() { return m_iMaximumConcurrentOperations; }
	int GetLastTerminalState() { return m_iLastTerminalState; }
	string GetWaitingTarget() { return m_sWaitingTarget; }
	void SetDefaultPackageForProof(string packageId) { if (m_SelectionPolicy) { m_SelectionPolicy.SetProofHumanCount(1); m_SelectionPolicy.SetForcedPackageForProof(packageId); } PrintFormat("[TB_PVE_M3_4] proof-default-package=%1", packageId); }
	void SetDelaysForProof(int initialDelaySeconds, int cooldownSeconds) { m_iInitialDelaySeconds = Math.Max(initialDelaySeconds, 0); m_iCooldownSeconds = Math.Max(cooldownSeconds, 0); PrintFormat("[TB_PVE_M3_4] proof-delays initial=%1 cooldown=%2", m_iInitialDelaySeconds, m_iCooldownSeconds); }
	TB_PVE_M3_4_ForceAssemblyService GetAssemblyService() { return m_AssemblyService; }
	TB_PVE_M3_5_ForceSelectionPolicy GetSelectionPolicy() { return m_SelectionPolicy; }
}
