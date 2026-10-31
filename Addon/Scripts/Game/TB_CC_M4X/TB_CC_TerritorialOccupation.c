// Campaign-scoped infantry occupation. Owns handed-off survivors, not bases.
// Future construction/static-weapon/logistics policies can subscribe here.
class TB_CC_TerritorialOccupationService
{
	protected ref map<string, ref TB_PVE_M3_6_FrontlineGarrison> m_mDefences = new map<string, ref TB_PVE_M3_6_FrontlineGarrison>();
	protected ref map<string, string> m_mRetirementReasons = new map<string, string>();
	protected ref map<string, int> m_mOrderRetries = new map<string, int>();
	protected ref array<ref TB_PVE_M3_6_FrontlineGarrison> m_aRetiring = {};
	protected ref map<string, FactionKey> m_mConsolidationClaims = new map<string, FactionKey>();
	protected TB_CC_M4XP_NativeArlandComponent m_Policy;
	protected ref ScriptInvoker m_OnDefenceEstablished = new ScriptInvoker();
	void TB_CC_TerritorialOccupationService(TB_CC_M4XP_NativeArlandComponent policy) { m_Policy = policy; }
	ScriptInvoker GetOnDefenceEstablished() { return m_OnDefenceEstablished; }
	bool HasDefence(string id)
	{
		TB_PVE_M3_6_FrontlineGarrison defence = m_mDefences.Get(id);
		return defence && defence.m_iState == TB_PVE_M3_6_GarrisonState.ACTIVE && m_Policy.GetTerritorialOwner(id) == defence.m_sFaction && TB_PVE_M3_4_ForceAssemblyService.CountUnits(defence.m_aGroups) > 0;
	}
	bool HasClaim(string id) { return m_mDefences.Contains(id) || m_mConsolidationClaims.Contains(id); }
	void ReserveConsolidation(string id, FactionKey faction)
	{
		if (Replication.IsServer() && m_Policy.GetTerritorialOwner(id) == faction) m_mConsolidationClaims.Set(id, faction);
	}
	void ReleaseConsolidation(string id, FactionKey faction)
	{
		if (m_mConsolidationClaims.Get(id) == faction) m_mConsolidationClaims.Remove(id);
	}
	static bool SetDefensiveOrder(TB_PVE_M3_4_AssembledGroup record, vector position)
	{
		if (!record || !record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() == 0) return false;
		if (record.m_Waypoint && !record.m_Waypoint.IsDeleted())
		{
			record.m_Group.RemoveWaypoint(record.m_Waypoint);
			SCR_EntityHelper.DeleteEntityAndChildren(record.m_Waypoint);
		}
		record.m_Waypoint = null;
		record.m_bDefensiveOrder = false;
		Resource resource = Resource.Load(TB_PVE_M3_6_OPFORFrontlineGarrisonManager.DEFEND_WAYPOINT);
		if (!resource || !resource.IsValid()) return false;
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = position;
		record.m_Waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!record.m_Waypoint) return false;
		record.m_Group.AddWaypoint(record.m_Waypoint);
		record.m_bDefensiveOrder = true;
		return true;
	}
	bool Adopt(string id, FactionKey faction, array<ref TB_PVE_M3_4_AssembledGroup> survivors, bool requireOrders = true)
	{
		if (!Replication.IsServer() || !m_Policy || m_Policy.GetTerritorialOwner(id) != faction || TB_CC_M4XP_CampaignRoles.GetRole(id) != TB_CC_StrategicRole.ORDINARY) return false;
		if (survivors.IsEmpty()) return false;
		TB_PVE_M3_6_FrontlineGarrison defence = m_mDefences.Get(id);
		if (defence && defence.m_sFaction != faction)
		{
			// A second physical recapture may occur before safe old-owner cleanup.
			// Preserve both ownership records, rather than orphaning the winners.
			m_aRetiring.Insert(defence); m_mDefences.Remove(id); defence = null;
			m_mRetirementReasons.Remove(id); m_mOrderRetries.Remove(id);
		}
		vector position; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, position)) return false;
		bool ordersReady = true;
		foreach (TB_PVE_M3_4_AssembledGroup record : survivors)
		{
			if (!record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() == 0) continue;
			Faction owner = record.m_Group.GetFaction();
			if (!owner || owner.GetFactionKey() != faction) return false;
			if ((!record.m_bDefensiveOrder || !record.m_Waypoint || record.m_Waypoint.IsDeleted()) && !SetDefensiveOrder(record, position)) ordersReady = false;
		}
		if (!ordersReady && requireOrders) return false;
		if (!defence)
		{
			defence = new TB_PVE_M3_6_FrontlineGarrison(); defence.m_sId = "OCCUPATION_" + id;
			defence.m_sObjectiveId = id; defence.m_sFaction = faction; defence.m_iState = TB_PVE_M3_6_GarrisonState.ACTIVE;
			defence.m_iCreationRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
			m_mDefences.Set(id, defence);
		}
		foreach (TB_PVE_M3_4_AssembledGroup record : survivors)
		{
			if (!record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() == 0) { TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record); continue; }
			TB_CC_PhysicalWarService.SetRole(record, "occupation", false);
			if (!defence.m_aGroups.Contains(record)) defence.m_aGroups.Insert(record);
		}
		survivors.Clear(); // Ownership transfer: NEVER delete the handed-off groups.
		ReleaseConsolidation(id, faction);
		if (!ordersReady) defence.m_iState = TB_PVE_M3_6_GarrisonState.PENDING_ACTIVATION;
		PrintFormat("[TB_CC_OCCUPATION] handoff base=%1 faction=%2 groups=%3 units=%4 ready=%5 persistent=1 replacementSpawn=0", id, faction, defence.m_aGroups.Count(), TB_PVE_M3_4_ForceAssemblyService.CountUnits(defence.m_aGroups), ordersReady);
		if (ordersReady) m_OnDefenceEstablished.Invoke(id, faction);
		return true;
	}
	void Tick()
	{
		if (!Replication.IsServer()) return;
		array<string> retired = {};
		array<string> staleClaims = {};
		foreach (string claimId, FactionKey claimFaction : m_mConsolidationClaims)
			if (m_Policy.GetTerritorialOwner(claimId) != claimFaction) staleClaims.Insert(claimId);
		foreach (string claimId : staleClaims) m_mConsolidationClaims.Remove(claimId);
		array<vector> observers = {}; TB_PVE_M3_7_HumanPresenceQuery.GetActiveHumanObservers("US", observers);
		foreach (string id, TB_PVE_M3_6_FrontlineGarrison defence : m_mDefences)
		{
			bool ownerChanged = m_Policy.GetTerritorialOwner(id) != defence.m_sFaction;
			bool exhausted = TB_PVE_M3_4_ForceAssemblyService.CountUnits(defence.m_aGroups) == 0;
			vector position; TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, position);
			if (!ownerChanged && !exhausted)
			{
				if (defence.m_iState == TB_PVE_M3_6_GarrisonState.PENDING_ACTIVATION && m_mOrderRetries.Get(id) < 3)
				{
					bool ordersReady = true;
					foreach (TB_PVE_M3_4_AssembledGroup record : defence.m_aGroups)
					{
						if (!record.m_Group || record.m_Group.IsDeleted() || record.m_Group.GetAgentsCount() == 0) continue;
						if ((!record.m_bDefensiveOrder || !record.m_Waypoint || record.m_Waypoint.IsDeleted()) && !SetDefensiveOrder(record, position)) ordersReady = false;
					}
					m_mOrderRetries.Set(id, m_mOrderRetries.Get(id) + 1);
					if (ordersReady) defence.m_iState = TB_PVE_M3_6_GarrisonState.ACTIVE;
					PrintFormat("[TB_CC_OCCUPATION] order-retry base=%1 ready=%2 attempts=%3 survivorsRetained=1", id, ordersReady, m_mOrderRetries.Get(id));
				}
				continue;
			}
			bool nearHuman;
			foreach (vector observer : observers) if (vector.Distance(observer, position) < 250) nearHuman = true;
			if (nearHuman && !exhausted)
			{
				if (!m_mRetirementReasons.Contains(id)) PrintFormat("[TB_CC_OCCUPATION] retirement-delayed base=%1 ownerChanged=1 nearbyHuman=1", id);
				m_mRetirementReasons.Set(id, "owner-changed"); continue;
			}
			foreach (TB_PVE_M3_4_AssembledGroup record : defence.m_aGroups) TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record);
			defence.m_aGroups.Clear(); retired.Insert(id);
			PrintFormat("[TB_CC_OCCUPATION] retired base=%1 ownerChanged=%2 exhausted=%3", id, ownerChanged, exhausted);
		}
		foreach (string id : retired) { m_mDefences.Remove(id); m_mRetirementReasons.Remove(id); m_mOrderRetries.Remove(id); }
		for (int index = m_aRetiring.Count() - 1; index >= 0; index--)
		{
			TB_PVE_M3_6_FrontlineGarrison obsolete = m_aRetiring[index];
			vector position; TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(obsolete.m_sObjectiveId, position);
			bool nearHuman;
			foreach (vector observer : observers) if (vector.Distance(observer, position) < 250) nearHuman = true;
			if (nearHuman && TB_PVE_M3_4_ForceAssemblyService.CountUnits(obsolete.m_aGroups) > 0)
			{
				if (obsolete.m_iState != TB_PVE_M3_6_GarrisonState.RETIRING) PrintFormat("[TB_CC_OCCUPATION] obsolete-retirement-delayed base=%1 faction=%2 nearbyHuman=1", obsolete.m_sObjectiveId, obsolete.m_sFaction);
				obsolete.m_iState = TB_PVE_M3_6_GarrisonState.RETIRING; continue;
			}
			foreach (TB_PVE_M3_4_AssembledGroup record : obsolete.m_aGroups) TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record);
			obsolete.m_aGroups.Clear(); m_aRetiring.Remove(index);
			PrintFormat("[TB_CC_OCCUPATION] obsolete-retired base=%1 faction=%2", obsolete.m_sObjectiveId, obsolete.m_sFaction);
		}
	}
	void Stop()
	{
		foreach (string id, TB_PVE_M3_6_FrontlineGarrison defence : m_mDefences)
			foreach (TB_PVE_M3_4_AssembledGroup record : defence.m_aGroups) TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record);
		foreach (TB_PVE_M3_6_FrontlineGarrison obsolete : m_aRetiring)
			foreach (TB_PVE_M3_4_AssembledGroup record : obsolete.m_aGroups) TB_PVE_M3_4_ForceAssemblyService.DeleteRecord(record);
		m_aRetiring.Clear();
		m_mDefences.Clear(); m_mRetirementReasons.Clear(); m_mOrderRetries.Clear(); m_mConsolidationClaims.Clear();
	}
}

class TB_CC_ConcurrentOperationSlot
{
	ref TB_PVE_M3_1_Operation m_Operation;
	int m_iElapsedSeconds;
}

class TB_CC_FIAOperationSlot : TB_CC_ConcurrentOperationSlot
{
	ref TB_CC_FIARecaptureOpportunity m_Opportunity;
	bool m_bPhysicalReported;
}

// Bounded authority-only FIA pool; baseline capacity remains one. Ownership
// opportunities live in the policy ledger, not in the physical staging path.
class TB_CC_FIARecaptureCoordinator
{
	int GetStrategicCooldownSeconds() { return Math.Max((m_iNextAttemptMs - System.GetTickCount() + 999) / 1000, 0); }
	int GetStrategicOperationSerial() { return m_iSerial; }
	void RestoreStrategicAdmission(int remainingSeconds, int serial)
	{
		m_iNextAttemptMs = System.GetTickCount() + remainingSeconds * 1000; m_iSerial = serial;
	}
	protected TB_CC_M4XP_NativeArlandComponent m_Policy;
	protected ref array<ref TB_CC_FIAOperationSlot> m_aOperations = {};
	protected TB_CC_FIARecaptureLedger m_Ledger;
	protected ref TB_PVE_M3_3_TacticalStagingPolicy m_Staging;
	protected ref TB_PVE_M3_4_ForceAssemblyService m_Assembly;
	protected int m_iCooldownSeconds;
	protected int m_iNextAttemptMs;
	protected int m_iSerial;
	protected bool m_bRunning;
	bool IsAttacking(string id)
	{
		foreach (TB_CC_FIAOperationSlot slot : m_aOperations) if (!slot.m_Operation.IsTerminal() && slot.m_Operation.GetTargetObjectiveId() == id) return true;
		return false;
	}
	void Start(TB_CC_M4XP_NativeArlandComponent policy, int cooldownSeconds)
	{
		if (!Replication.IsServer() || m_bRunning) return;
		m_Policy = policy; m_iCooldownSeconds = Math.Max(cooldownSeconds, 10);
		m_Ledger = policy.GetFIARecaptureLedger();
		m_Staging = new TB_PVE_M3_3_TacticalStagingPolicy(); m_Staging.Configure(350, 500, 250, 900, 20, 24, false);
		m_Assembly = new TB_PVE_M3_4_ForceAssemblyService();
		m_iNextAttemptMs = System.GetTickCount() + m_iCooldownSeconds * 1000; m_bRunning = true;
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
		PrintFormat("[TB_CC_FIA] coordinator-start maxOperations=%1 cooldownSeconds=%2 HQTargets=0", m_Policy.GetMaximumConcurrentFIAOperations(), m_iCooldownSeconds);
	}
	protected void Tick()
	{
		if (!m_bRunning) return;
		// Pressure is operational; only terminal/invalid state stops consumers.
		if (!m_Policy.IsWarOperational() || !TB_PVE_M2_3_FrontierQuery.IsCampaignValid()) return;
		for (int index = m_aOperations.Count() - 1; index >= 0; index--)
		{
			TB_CC_FIAOperationSlot slot = m_aOperations[index];
			if (!slot.m_Opportunity)
			{
				slot.m_iElapsedSeconds += 2; slot.m_Operation.Tick(slot.m_iElapsedSeconds);
				if (slot.m_Operation.IsTerminal()) { slot.m_Operation.CleanupAssignedForce("FIA-response-terminal"); m_aOperations.Remove(index); }
				continue;
			}
			string owner = m_Policy.GetTerritorialOwner(slot.m_Opportunity.m_sTarget);
			if (owner != "FIA" && (!m_Ledger.IsCurrent(slot.m_Opportunity) || !m_Policy.CanFIARecapture(slot.m_Opportunity.m_sTarget)))
			{
				m_Ledger.Complete(slot.m_Opportunity, "CANCELLED", "ownership-epoch-or-conventional-capability-lost");
				slot.m_Operation.CleanupAssignedForce("FIA-opportunity-invalid"); m_aOperations.Remove(index); continue;
			}
			slot.m_iElapsedSeconds += 2; slot.m_Operation.Tick(slot.m_iElapsedSeconds);
			if (!slot.m_bPhysicalReported && slot.m_Operation.GetViableAssignedGroupCount() > 0)
			{
				slot.m_bPhysicalReported = true;
				PrintFormat("[TB_CC_FIA_RECAPTURE] started target=%1 mode=PHYSICAL attempt=%2 epoch=%3", slot.m_Opportunity.m_sTarget, slot.m_Opportunity.m_iAttempts, slot.m_Opportunity.m_iEpoch);
			}
			if (!slot.m_Operation.IsTerminal()) continue;
			string result = "FAILURE";
			if (slot.m_Operation.GetState() == TB_PVE_M3_1_OperationState.SUCCEEDED) result = "SUCCESS";
			else if (slot.m_Operation.GetState() == TB_PVE_M3_1_OperationState.CANCELLED) result = "CANCELLED";
			string terminalReason = slot.m_Operation.GetTerminalReason();
			if (terminalReason.IsEmpty()) terminalReason = "ownership-capture-and-consolidation-complete";
			m_Ledger.Complete(slot.m_Opportunity, result, terminalReason);
			slot.m_Operation.CleanupAssignedForce("FIA-terminal"); m_aOperations.Remove(index);
			m_iNextAttemptMs = System.GetTickCount() + m_iCooldownSeconds * 1000;
		}
		if (m_aOperations.Count() >= m_Policy.GetMaximumConcurrentFIAOperations()) return;
		if (m_Policy.GetResponses() && m_Policy.GetResponses().HasDue("FIA", System.GetTickCount() + 2000)) return;
		TB_CC_FIARecaptureOpportunity opportunity = m_Ledger.Peek();
		while (opportunity && (!m_Ledger.IsCurrent(opportunity) || !m_Policy.CanFIARecapture(opportunity.m_sTarget)))
		{
			m_Ledger.Complete(opportunity, "CANCELLED", "ownership-epoch-or-conventional-capability-lost");
			opportunity = m_Ledger.Peek();
		}
		if (!opportunity || System.GetTickCount() < m_iNextAttemptMs) return;
		string target = opportunity.m_sTarget;
		PrintFormat("[TB_CC_FIA] recapture-target base=%1 authoritativeOwner=%2 targetIsNotOwnership=1 mapWrite=0", target, m_Policy.GetTerritorialOwner(target));
		m_iNextAttemptMs = System.GetTickCount() + 2000;
		TB_PVE_M3_1_Operation operation = new TB_PVE_M3_1_Operation(); operation.ConfigureTerritorialAttack("FIA", target);
		m_iSerial++; opportunity.m_iAttempts++;
		string packageId = m_Policy.SelectFIAPackage(target);
		if (!operation.Create("FIA_RECAPTURE_" + m_iSerial.ToString(), m_Staging, m_Assembly, packageId))
		{
			// Admission/resource failure cannot consume the strategic opportunity.
			string failure = operation.GetTerminalReason();
			if (failure != opportunity.m_sDeferredReason || System.GetTickCount() >= opportunity.m_iNextLogMs)
			{
				opportunity.m_sDeferredReason = failure; opportunity.m_iNextLogMs = System.GetTickCount() + 10000;
				PrintFormat("[TB_CC_FIA_RECAPTURE] physical-staging-deferred target=%1 reason=%2 retryIn=2", target, failure);
			}
			operation.CleanupAssignedForce("FIA-create-deferred"); m_Ledger.DeferAdmission(opportunity); return;
		}
		m_Ledger.Started(opportunity);
		m_iNextAttemptMs = System.GetTickCount() + m_iCooldownSeconds * 1000;
		TB_CC_FIAOperationSlot added = new TB_CC_FIAOperationSlot(); added.m_Operation = operation; added.m_Opportunity = opportunity; m_aOperations.Insert(added);
	}
	void Stop(string reason)
	{
		if (!m_bRunning) return;
		m_bRunning = false; GetGame().GetCallqueue().Remove(Tick);
		foreach (TB_CC_FIAOperationSlot slot : m_aOperations)
		{
			if (slot.m_Opportunity) m_Ledger.Complete(slot.m_Opportunity, "CANCELLED", reason);
			slot.m_Operation.CleanupAssignedForce("FIA-stop-" + reason);
		}
		m_Ledger.CancelPending(reason);
		m_aOperations.Clear();
	}
	TB_PVE_M3_1_Operation TryLaunchResponse(TB_CC_ResponseRequest request, string packageId)
	{
		if (!m_bRunning || !Replication.IsServer() || m_aOperations.Count() >= m_Policy.GetMaximumConcurrentFIAOperations()) return null;
		TB_PVE_M3_1_Operation operation = new TB_PVE_M3_1_Operation(); operation.ConfigureResponse(request); m_iSerial++;
		if (!operation.Create("FIA_RESPONSE_" + m_iSerial.ToString(), m_Staging, m_Assembly, packageId))
		{ operation.CleanupAssignedForce("FIA-response-admission-deferred"); return null; }
		TB_CC_FIAOperationSlot slot = new TB_CC_FIAOperationSlot(); slot.m_Operation = operation; m_aOperations.Insert(slot);
		return operation;
	}
}
