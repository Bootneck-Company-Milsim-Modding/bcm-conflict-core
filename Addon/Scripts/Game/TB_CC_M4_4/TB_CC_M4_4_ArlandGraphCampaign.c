class TB_CC_M4_4_ArlandGraphCampaignComponentClass : ScriptComponentClass {}

// Arland-only production integration. Vanilla owns physical seizing and base
// ownership; this component owns only campaign initialization and strategic policy.
class TB_CC_M4_4_ArlandGraphCampaignComponent : ScriptComponent
{
	[Attribute(defvalue: "20", category: "TB Conflict Core M4.4")] protected int m_iCampaignSeed;
	[Attribute(defvalue: "1", category: "TB Conflict Core M4.4")] protected int m_iStartMode;
	[Attribute(defvalue: "ARLAND::HQ_EAST", category: "TB Conflict Core M4.4")] protected string m_sExplicitMobId;
	[Attribute(defvalue: "1", category: "TB Conflict Core M4.4")] protected bool m_bEnableM3Coordinator;
	[Attribute(defvalue: "1", category: "TB Conflict Core M4.4")] protected bool m_bEnableM3Garrison;
	[Attribute(defvalue: "0", category: "TB Conflict Core M4.4", desc: "Dedicated authority acceptance only; production must remain 0.")] protected int m_iAuthorityProofMode;
	[Attribute(defvalue: "300", category: "TB Conflict Core M4.4H Legacy", desc: "Historical graph scenarios/proofs only. Native campaigns use PressureDurationSeconds.")] protected int m_iBLUFORVictoryHoldSeconds;
	[Attribute(defvalue: "300", category: "TB Conflict Core M4.4H Legacy", desc: "Historical graph scenarios/proofs only. Native campaigns use PressureDurationSeconds.")] protected int m_iOPFORVictoryHoldSeconds;
	// Native campaigns use this one setting in both directions. The two legacy
	// holds above remain serialized for historical graph scenarios/proof modes.
	[Attribute(defvalue: "600", category: "BCM Campaign Pressure", desc: "Continuous ordinary-territory hold before native HQ vulnerability, in seconds.")] protected int m_iPressureDurationSeconds;

	protected static TB_CC_M4_4_ArlandGraphCampaignComponent s_Instance;
	protected ref TB_CC_M4_1_StrategicPositionRegistry m_Registry;
	protected ref TB_CC_M4_2A_CampaignStartState m_Start;
	protected ref TB_CC_M4_3_StrategicEvaluator m_Evaluator;
	protected ref map<SCR_CampaignMilitaryBaseComponent, string> m_mIdsByBase = new map<SCR_CampaignMilitaryBaseComponent, string>();
	protected ref array<string> m_aIds = {};
	protected ref array<vector> m_aPositions = {};
	protected string m_sLastCapturedBLUFORObjective;
	protected ref array<string> m_aBLUFORCaptureHistory = {};
	// Campaign-history latch: a legitimate ordinary US ownership transition
	// has completed. Never cleared by retreat, reevaluation or player lifecycle.
	// A new component/new campaign starts false; no restart persistence exists.
	protected bool m_bBLUFORAdvanced;
	protected string m_sSemanticSignature;
	protected int m_iRevision;
	protected int m_iOutcome = TB_PVE_M2_5_CampaignOutcome.NOT_READY;
	protected int m_iDepthPermille = -1;
	protected bool m_bReady;
	protected bool m_bInitializing;
	protected bool m_bVanillaEndSignalled;
	protected bool m_bPressureTimerActive;
	protected WorldTimestamp m_PressureStarted;
	protected WorldTimestamp m_PressureDeadline;
	protected int m_iPressureStarts;
	protected int m_iLastEvaluationDurationMs;
	protected int m_iLastOwnershipEventDurationMs;
	protected int m_iLastProjectionDurationMs;
	protected ref TB_PVE_M3_2_OPFOROperationCoordinator m_M3Coordinator;
	protected ref TB_PVE_M3_3_TacticalStagingPolicy m_M3Staging;
	protected ref TB_PVE_M3_4_ForceAssemblyService m_M3Assembly;
	protected ref TB_PVE_M3_5_ForceSelectionPolicy m_M3Selection;
	protected ref TB_PVE_M3_6_OPFORFrontlineGarrisonManager m_M3Garrison;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		TB_CC_VictoryPressurePresentation.Start();
		if (!Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(InitializeClientDefinition, 12000);
			return;
		}
		SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Insert(OnBaseFactionChanged);
		GetGame().GetCallqueue().CallLater(InitializeAuthority, 15000);
	}

	override void OnDelete(IEntity owner)
	{
		SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Remove(OnBaseFactionChanged);
		if (m_M3Coordinator) m_M3Coordinator.Stop("M4.4-component-deleted");
		if (m_M3Garrison) m_M3Garrison.Stop("M4.4-component-deleted");
		GetGame().GetCallqueue().Remove(CheckVictoryPressure);
		GetGame().GetCallqueue().Remove(PublishPressureClock);
		GetGame().GetCallqueue().Remove(RetryRadioObjective);
		TB_CC_VictoryPressurePresentation.Stop();
		if (s_Instance == this) s_Instance = null;
		super.OnDelete(owner);
	}

	protected void InitializeClientDefinition()
	{
		BuildRegistryAndSpatialQuery();
		Print("[TB_CC_M4_4][CLIENT] definition-ready; admitted-client projection acceptance pending");
	}

	protected bool BuildRegistryAndSpatialQuery()
	{
		m_Registry = new TB_CC_M4_1_StrategicPositionRegistry();
		if (!m_Registry.Build(TB_CC_M4_2_ArlandProductionDefinition.Create())) return false;
		m_Registry.GetPositionIds(m_aIds); m_aPositions.Clear(); m_mIdsByBase.Clear();
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			m_aPositions.Insert(resolved.GetResolvedEntity().GetOrigin());
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (base) m_mIdsByBase.Set(base, id);
		}
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureRoute(m_aIds);
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureObjectivePositions(m_aPositions);
		return true;
	}

	protected void InitializeAuthority()
	{
		if (m_bReady || !BuildRegistryAndSpatialQuery()) return;
		TB_CC_M4_2A_CampaignStartResolver resolver = new TB_CC_M4_2A_CampaignStartResolver();
		m_Start = resolver.Resolve(m_Registry, TB_CC_M4_2A_ArlandStartDefinition.Create(), m_iCampaignSeed, m_iStartMode, m_sExplicitMobId);
		if (!m_Start) { FailClosed("campaign-start-" + resolver.GetFailure()); return; }
		m_bInitializing = true;
		m_bBLUFORAdvanced = false; // Explicit NEW campaign boundary only.
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		SCR_CampaignFaction ussr = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("USSR"));
		if (!us || !ussr) { m_bInitializing = false; FailClosed("required-faction-unavailable"); return; }
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base || !base.IsInitialized()) { m_bInitializing = false; FailClosed("base-uninitialized-" + id); return; }
			SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
			// HQs remain permanent anchors. Only ordinary positions participate
			// in the M4 strategic capture lifecycle.
			if (seizing && !base.IsHQ()) seizing.TB_CC_M4_4_ConfigurePvECaptureRules();
			SCR_CampaignFaction desired = ussr;
			if (id == m_Start.GetSelectedMobId()) desired = us;
			if (base.GetFaction() != desired) base.SetFaction(desired);
		}
		m_bInitializing = false;
		ApplyDeploymentPolicy();
		m_Evaluator = new TB_CC_M4_3_StrategicEvaluator();
		m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		m_bReady = true;
		if (m_iAuthorityProofMode == 9 || m_iAuthorityProofMode == 10) { m_iBLUFORVictoryHoldSeconds = 60; m_iOPFORVictoryHoldSeconds = 60; }
		PrintFormat("[TB_CC_M4_4] campaign-start initializedOnce=1 seed=%1 mode=%2 mob=%3 endpoint=%4 ownershipProfile=selected-MOB-US_all-others-USSR", m_Start.GetSeed(), m_Start.GetMode(), m_Start.GetSelectedMobId(), m_Start.GetSelectedEndpointId());
		Reevaluate("initialization");
		GetGame().GetCallqueue().CallLater(PublishPressureClock, 1000, true);
		if (m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE) StartM3Consumers();
		if (m_iAuthorityProofMode > 0 && m_iAuthorityProofMode < 4) GetGame().GetCallqueue().CallLater(TB_CC_M4_4_PhysicalAuthorityProof.Start, 1500, false, this, m_iAuthorityProofMode);
		if (m_iAuthorityProofMode >= 4 && m_iAuthorityProofMode < 9) GetGame().GetCallqueue().CallLater(TB_CC_M4_4_RemainingAuthorityProof.Start, 1500, false, this, m_iAuthorityProofMode);
		if (m_iAuthorityProofMode == 9 || m_iAuthorityProofMode == 10) GetGame().GetCallqueue().CallLater(TB_CC_M4_4H_VictoryPressureProof.Start, 1500, false, this, m_iAuthorityProofMode);
	}

	protected void ApplyDeploymentPolicy()
	{
		if (!m_Start) return;
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			if (spawn) spawn.SetSpawnPointEnabled_S(id == m_Start.GetSelectedMobId());
		}
		PrintFormat("[TB_CC_M4_4] deployment-policy selected=%1 enabledUSMobSpawnPoints=%2 allForwardDisabled=1 referencesPreserved=1", m_Start.GetSelectedMobId(), CountEnabledSpawns());
	}

	protected int CountEnabledSpawns()
	{
		int count; foreach (string id : m_aIds) { SCR_CampaignMilitaryBaseComponent base = GetBase(id); if (base && base.GetSpawnPoint() && base.GetSpawnPoint().IsSpawnPointEnabled()) count++; } return count;
	}

	protected void OnBaseFactionChanged(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)
	{
		if (!m_bReady || m_bInitializing || !m_mIdsByBase.Contains(base)) return;
		int startMs = System.GetTickCount(); string id = m_mIdsByBase.Get(base); FactionKey oldKey = "<none>"; FactionKey newKey = "<none>";
		if (previousFaction) oldKey = previousFaction.GetFactionKey(); if (newFaction) newKey = newFaction.GetFactionKey();
		bool wasBLUFORLegal = id == GetBLUFORLegalObjective();
		bool wasOPFORLegal = id == GetOPFORLegalObjective();
		if (newKey == "US" && oldKey != "US" && wasBLUFORLegal && !base.IsHQ())
		{
			if (!m_bBLUFORAdvanced) PrintFormat("[TB_CC_M4_4J] foothold-established id=%1 previous=%2 new=%3 ownershipCompleted=1", id, oldKey, newKey);
			m_bBLUFORAdvanced = true;
			m_aBLUFORCaptureHistory.RemoveItem(id);
			m_aBLUFORCaptureHistory.Insert(id);
		}
		else if (newKey == "USSR")
			m_aBLUFORCaptureHistory.RemoveItem(id);
		m_sLastCapturedBLUFORObjective = FindLatestCapturedBLUFORObjective();
		PrintFormat("[TB_CC_M4_4] vanilla-ownership-callback id=%1 previous=%2 new=%3 wasBLUFORLegal=%4 wasOPFORLegal=%5", id, oldKey, newKey, wasBLUFORLegal, wasOPFORLegal);
		ApplyDeploymentPolicy();
		Reevaluate("vanilla-ownership-callback-" + id);
		m_iLastOwnershipEventDurationMs = System.GetTickCount() - startMs;
		PrintFormat("[TB_CC_M4_4] ownership-event-complete id=%1 durationMs=%2 revision=%3", id, m_iLastOwnershipEventDurationMs, m_iRevision);
	}

	protected void Reevaluate(string reason)
	{
		int startMs = System.GetTickCount();
		TB_CC_M4_3_VanillaOwnershipView owners = new TB_CC_M4_3_VanillaOwnershipView(m_Registry);
		TB_CC_M4_3_RadioEligibilityView radio = CreateRadioEligibilityView();
		bool evaluated = m_Evaluator.Evaluate(m_Registry, m_Start, owners, m_sLastCapturedBLUFORObjective, radio);
		GetGame().GetCallqueue().Remove(RetryRadioObjective);
		if (evaluated && m_Evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.RADIO_WAIT)
		{
			Print("[TB_CC_M4_3_RADIO] objective-withheld reason=no-radio-reachable-graph-candidate retrySeconds=10");
			GetGame().GetCallqueue().CallLater(RetryRadioObjective, 10000, false);
		}
		m_iLastEvaluationDurationMs = System.GetTickCount() - startMs;
		int usOrdinary; int ussrOrdinary; int requiredOrdinary; bool ownersReady = CountRequiredTerritory(usOrdinary, ussrOrdinary, requiredOrdinary);
		if (m_iOutcome != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY && m_iOutcome != TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
		{
			if (!evaluated || !ownersReady || requiredOrdinary < 1 || (!m_Evaluator.IsValid() && m_Evaluator.GetEvaluationState() != TB_CC_M4_3_EvaluationState.NO_ENEMY_REMAINING))
			{
				CancelVictoryPressure("invalid");
				m_iOutcome = TB_PVE_M2_5_CampaignOutcome.INVALID;
			}
			else if (usOrdinary == requiredOrdinary)
				EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE, m_iBLUFORVictoryHoldSeconds, reason);
			else if (m_bBLUFORAdvanced && usOrdinary == 0 && ussrOrdinary == requiredOrdinary)
				EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE, m_iOPFORVictoryHoldSeconds, reason);
			else
			{
				bool wasPressure = IsPressure();
				CancelVictoryPressure("territorial-condition-broken");
				m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
				if (wasPressure) StartM3Consumers();
			}
		}
		m_iDepthPermille = CalculateDepthPermille();
		PublishIfChanged(reason);
		PrintFormat("[TB_CC_M4_4] evaluation reason=%1 durationMs=%2 state=%3 valid=%4 depth=%5 candidates=%6 BLUFOR=%7 OPFOR=%8 outcome=%9", reason, m_iLastEvaluationDurationMs, m_Evaluator.GetEvaluationState(), m_Evaluator.IsValid(), m_iDepthPermille, CandidateCsv(), GetBLUFORLegalObjective(), GetOPFORLegalObjective(), m_iOutcome);
		PrintFormat("[TB_CC_M4_4H] territory ordinaryUS=%1 ordinaryUSSR=%2 required=%3 pressureRemainingSeconds=%4", usOrdinary, ussrOrdinary, requiredOrdinary, GetPressureRemainingSeconds());
		PrintFormat("[TB_CC_M4_4J] history foothold=%1 outcome=%2 pressureActive=%3 deadlinePresent=%4 siegeHook=%5", m_bBLUFORAdvanced, m_iOutcome, IsVictoryPressureActive(), m_bPressureTimerActive, m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE && m_bPressureTimerActive);
	}

	protected TB_CC_M4_3_RadioEligibilityView CreateRadioEligibilityView() { return null; }

	protected void RetryRadioObjective()
	{
		if (!Replication.IsServer() || !m_bReady || IsTerminal()) return;
		Reevaluate("radio-reachability-retry");
	}

	protected bool CountRequiredTerritory(out int usCount, out int ussrCount, out int requiredCount)
	{
		usCount = 0; ussrCount = 0; requiredCount = 0;
		if (!m_Start || !m_Registry) return false;
		foreach (string id : m_aIds)
		{
			if (id == m_Start.GetSelectedMobId() || id == m_Start.GetSelectedEndpointId()) continue;
			FactionKey owner; if (!m_Registry.TryGetCurrentOwner(id, owner)) return false;
			requiredCount++;
			if (owner == "US") usCount++;
			else if (owner == "USSR") ussrCount++;
		}
		return true;
	}

	protected bool IsPressure()
	{
		return m_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE;
	}

	protected void EnterVictoryPressure(int pressureOutcome, int durationSeconds, string reason)
	{
		if (m_iOutcome == pressureOutcome && m_bPressureTimerActive) return;
		if (IsPressure()) CancelVictoryPressure("pressure-side-changed");
		m_iOutcome = pressureOutcome;
		m_bPressureTimerActive = true;
		m_iPressureStarts++;
		m_PressureStarted = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetServerTimestamp();
		m_PressureDeadline = m_PressureStarted.PlusSeconds(Math.Max(durationSeconds, 1));
		GetGame().GetCallqueue().Remove(CheckVictoryPressure);
		GetGame().GetCallqueue().CallLater(CheckVictoryPressure, Math.Max(durationSeconds, 1) * 1000, false);
		StopM3Consumers("M4.4H-victory-pressure");
		PrintFormat("[TB_CC_M4_4H] pressure-enter outcome=%1 reason=%2 start=%3 deadline=%4 durationSeconds=%5 startCount=%6 siegeHook=%7", pressureOutcome, reason, m_PressureStarted, m_PressureDeadline, durationSeconds, m_iPressureStarts, pressureOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE);
		PrintFormat("[TB_CC_PRESSURE] state=STARTED duration=%1 remaining=%2 outcome=%3", durationSeconds, GetPressureRemainingSeconds(), pressureOutcome);
	}

	protected void CancelVictoryPressure(string reason)
	{
		if (!m_bPressureTimerActive) return;
		int previousOutcome = m_iOutcome;
		int remainingSeconds = GetPressureRemainingSeconds();
		GetGame().GetCallqueue().Remove(CheckVictoryPressure);
		m_bPressureTimerActive = false;
		WorldTimestamp cleared;
		m_PressureStarted = cleared;
		m_PressureDeadline = cleared;
		PrintFormat("[TB_CC_M4_4H] pressure-cancel outcome=%1 reason=%2 remainingSeconds=%3", previousOutcome, reason, remainingSeconds);
	}

	protected void CheckVictoryPressure()
	{
		if (!Replication.IsServer() || !m_bPressureTimerActive || !IsPressure() || IsTerminal()) return;
		int expectedPressure = m_iOutcome; int usCount; int ussrCount; int requiredCount;
		if (!CountRequiredTerritory(usCount, ussrCount, requiredCount)) { CancelVictoryPressure("expiry-owner-unresolved"); m_iOutcome = TB_PVE_M2_5_CampaignOutcome.INVALID; PublishIfChanged("pressure-expiry-invalid"); return; }
		bool stillHeld = expectedPressure == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE && usCount == requiredCount;
		if (expectedPressure == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) stillHeld = m_bBLUFORAdvanced && usCount == 0 && ussrCount == requiredCount;
		if (!stillHeld) { Reevaluate("pressure-expiry-condition-broken"); return; }
		m_bPressureTimerActive = false;
		WorldTimestamp cleared;
		m_PressureStarted = cleared;
		m_PressureDeadline = cleared;
		if (expectedPressure == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) m_iOutcome = TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY;
		else m_iOutcome = TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
		PrintFormat("[TB_CC_M4_4H] pressure-expired previous=%1 terminalOutcome=%2 ordinaryUS=%3 ordinaryUSSR=%4 required=%5", expectedPressure, m_iOutcome, usCount, ussrCount, requiredCount);
		PublishIfChanged("pressure-expired");
	}

	// M4.3 deliberately models one counterattack response. M4.4 retains the
	// legal BLUFOR capture stack so successive vanilla OPFOR recaptures can
	// retreat deterministically toward the selected MOB without inventing a
	// second graph evaluator or allowing arbitrary enemy-base selection.
	protected string FindLatestCapturedBLUFORObjective()
	{
		for (int i = m_aBLUFORCaptureHistory.Count() - 1; i >= 0; i--)
		{
			string id = m_aBLUFORCaptureHistory[i]; FactionKey owner;
			if (m_Registry.TryGetCurrentOwner(id, owner) && owner == "US") return id;
		}
		return "";
	}

	protected int CalculateDepthPermille()
	{
		int usCount; int ussrCount; int requiredCount;
		if (!CountRequiredTerritory(usCount, ussrCount, requiredCount) || requiredCount < 1) return -1;
		return Math.Clamp((usCount * 1000) / requiredCount, 0, 1000);
	}

	protected void PublishIfChanged(string reason)
	{
		string blufor = GetBLUFORLegalObjective(); string opfor = GetOPFORLegalObjective();
		string signature = m_iOutcome.ToString();
		signature += "|" + blufor;
		signature += "|" + opfor;
		signature += "|" + CandidateCsv();
		signature += "|" + StateCsv();
		signature += "|" + m_iDepthPermille.ToString();
		signature += "|" + m_iPressureStarts.ToString();
		signature += AdditionalSemanticSignature();
		if (signature == m_sSemanticSignature) { PrintFormat("[TB_CC_M4_4] reconstruction-stable revision=%1 reason=%2", m_iRevision, reason); return; }
		m_sSemanticSignature = signature; m_iRevision++;
		int startMs = System.GetTickCount();
		PrintFormat("[TB_CC_M4_4][SERVER] projection-published revision=%1 reason=%2 representation=stable-id authority=1 networkAcceptance=pending bytes=%3", m_iRevision, reason, IdCsv().Length() + StateCsv().Length() + CandidateCsv().Length());
		m_iLastProjectionDurationMs = System.GetTickCount() - startMs;
		ConsumeProjection("authority");
		PublishAdditionalProjection();
		if (IsTerminal()) ApplyVanillaEndState();
	}
	protected void PublishAdditionalProjection() {}
	protected string AdditionalSemanticSignature() { return ""; }

	protected void ConsumeProjection(string source)
	{
		PublishPressureClock();
		array<int> states = {}; foreach (string id : m_aIds) states.Insert(GetProjectedState(id));
		array<string> candidates = {}; m_Evaluator.GetBLUFORCandidateObjectives(candidates);
		TB_PVE_M2_3_FrontierQuery.TB_M4_4_ConsumeGraphProjection(m_iRevision, GetValidity(), m_iOutcome, m_Start.GetSelectedMobId(), m_Start.GetSelectedEndpointId(), GetBLUFORLegalObjective(), GetOPFORLegalObjective(), m_iDepthPermille, GetPressureRemainingSeconds(), m_aIds, m_aPositions, states, candidates, source);
	}

	static void ConsumeReplicated(int revision, int validity, int outcome, string mobId, string endpointId, string bluforId, string opforId, int depth, int pressureRemainingSeconds, string idsCsv, string statesCsv, string candidatesCsv)
	{
		if (!s_Instance) return;
		array<string> ids = {}; array<string> stateText = {}; array<string> candidates = {}; idsCsv.Split(",", ids, true); statesCsv.Split(",", stateText, true); if (!candidatesCsv.IsEmpty()) candidatesCsv.Split(",", candidates, true);
		array<int> states = {}; foreach (string value : stateText) states.Insert(value.ToInt());
		array<vector> positions = {};
		foreach (string id : ids)
		{
			TB_CC_M4_1_ResolvedPosition resolved;
			if (s_Instance.m_Registry) resolved = s_Instance.m_Registry.FindPositionById(id);
			if (resolved) positions.Insert(resolved.GetResolvedEntity().GetOrigin());
			else positions.Insert("0 0 0");
		}
		TB_PVE_M2_3_FrontierQuery.TB_M4_4_ConsumeGraphProjection(revision, validity, outcome, mobId, endpointId, bluforId, opforId, depth, pressureRemainingSeconds, ids, positions, states, candidates, "replicated-client");
	}

	protected string IdCsv() { string result; foreach (string id : m_aIds) { if (!result.IsEmpty()) result += ","; result += id; } return result; }
	protected string CandidateCsv() { array<string> ids = {}; if (m_Evaluator) m_Evaluator.GetBLUFORCandidateObjectives(ids); string result; foreach (string id : ids) { if (!result.IsEmpty()) result += ","; result += id; } return result; }
	protected string StateCsv() { string result; foreach (string id : m_aIds) { if (!result.IsEmpty()) result += ","; result += GetProjectedState(id).ToString(); } return result; }
	protected int GetProjectedState(string id)
	{
		if (!m_Evaluator || !m_Evaluator.IsReady()) return TB_PVE_M2_3_ObjectiveStrategicState.NOT_READY;
		int state = m_Evaluator.GetStrategicState(id);
		if (state == TB_CC_M4_3_PositionState.FRIENDLY_CONNECTED)
		{
			if (id == GetOPFORLegalObjective()) return TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_FRONTLINE;
			return TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_REAR;
		}
		if (state == TB_CC_M4_3_PositionState.ENEMY)
		{
			if (id == GetBLUFORLegalObjective()) return TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_FRONTLINE;
			return TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_REAR_LOCKED;
		}
		return TB_PVE_M2_3_ObjectiveStrategicState.INVALID;
	}

	protected void StartM3Consumers()
	{
		if (m_M3Coordinator || m_M3Garrison || IsPressure() || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return;
		m_M3Staging = new TB_PVE_M3_3_TacticalStagingPolicy(); m_M3Staging.Configure(350, 500, 250, 900, 20, 24, false);
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			if (resolved && resolved.GetDefinition().HasTrait("TRANSPORT_REQUIRED")) m_M3Staging.AddUnsupportedFootTarget(id);
		}
		m_M3Assembly = new TB_PVE_M3_4_ForceAssemblyService();
		m_M3Selection = new TB_PVE_M3_5_ForceSelectionPolicy(); m_M3Selection.Configure(2, 4, 334, 667, 1, 2, 1, "LIGHT", "STANDARD", "REINFORCED");
		if (m_bEnableM3Coordinator) { m_M3Coordinator = new TB_PVE_M3_2_OPFOROperationCoordinator(); m_M3Coordinator.Start(m_M3Staging, m_M3Assembly, m_M3Selection, 45, 6); }
		if (m_bEnableM3Garrison) { m_M3Garrison = new TB_PVE_M3_6_OPFORFrontlineGarrisonManager(); m_M3Garrison.Configure("STANDARD", 250, 900, 250, 55, 32); m_M3Garrison.Start(); }
		PrintFormat("[TB_CC_M4_4] M3-migration coordinator=%1 staging=1 packages=1 depth=%2 garrison=%3 graphKnowledge=0", m_bEnableM3Coordinator, m_iDepthPermille, m_bEnableM3Garrison);
	}

	protected void StopM3Consumers(string reason)
	{
		if (m_M3Coordinator) { m_M3Coordinator.Stop(reason); m_M3Coordinator = null; }
		if (m_M3Garrison) { m_M3Garrison.Stop(reason); m_M3Garrison = null; }
		PrintFormat("[TB_CC_M4_4H] M3-suspended reason=%1 outcome=%2", reason, m_iOutcome);
	}

	protected void ApplyVanillaEndState()
	{
		if (m_bVanillaEndSignalled) return;
		FactionKey winner = "US"; if (m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY) winner = "USSR";
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(winner); SCR_GameModeCampaign gameMode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!faction || !gameMode) return; m_bVanillaEndSignalled = true; gameMode.TB_M2_5_EndMatch(GetGame().GetFactionManager().GetFactionIndex(faction));
		PrintFormat("[TB_CC_M4_4] vanilla-end-state outcome=%1 winner=%2 count=1", m_iOutcome, winner);
	}

	protected void FailClosed(string reason) { CancelVictoryPressure("fail-closed"); m_bReady = false; m_iOutcome = TB_PVE_M2_5_CampaignOutcome.INVALID; StopM3Consumers("M4.4-fail-closed"); PrintFormat("[TB_CC_M4_4] INVALID reason=%1 captures=DENY_ALL operations=0 garrison=0", reason); }
	protected bool IsTerminal() { return m_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY; }
	protected int GetValidity()
	{
		int result = TB_PVE_M2_2_CampaignValidity.INVALID_GRAPH;
		if (m_Evaluator && m_Evaluator.IsReady() && (m_Evaluator.IsValid() || m_Evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.NO_ENEMY_REMAINING) && m_iOutcome != TB_PVE_M2_5_CampaignOutcome.INVALID) result = TB_PVE_M2_2_CampaignValidity.VALID_GRAPH;
		return result;
	}
	string GetBLUFORLegalObjective()
	{
		string result;
		if ((m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) && m_Evaluator) result = m_Evaluator.GetBLUFORLegalObjective();
		return result;
	}
	string GetOPFORLegalObjective()
	{
		string result;
		if ((m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) && m_Evaluator)
		{
			result = m_Evaluator.GetOPFORLegalObjective();
		}
		return result;
	}
	SCR_CampaignMilitaryBaseComponent GetBase(string id)
	{
		TB_CC_M4_1_ResolvedPosition resolved;
		if (m_Registry) resolved = m_Registry.FindPositionById(id);
		if (!resolved) return null;
		return SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
	}
	string GetId(SCR_CampaignMilitaryBaseComponent base)
	{
		string result;
		if (m_mIdsByBase.Contains(base)) result = m_mIdsByBase.Get(base);
		return result;
	}

	static bool IsGraphCampaignActive() { return s_Instance && s_Instance.m_bReady; }
	static bool CanBypassPlayableRadioGate(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		if (!Replication.IsServer() || !s_Instance || !s_Instance.m_bReady || !faction || !base || !base.IsInitialized()) return false;
		string id = s_Instance.GetId(base);
		if (id.IsEmpty() || !s_Instance.m_Evaluator || !s_Instance.m_Evaluator.IsReady()) return false;
		// CanFactionCaptureBase is the authoritative public graph contract and
		// admits only a configured conventional campaign faction at its current
		// legal objective. FIA and all unknown factions fail closed there.
		return CanFactionCaptureBase(faction, base);
	}
	static bool CanFactionCaptureBase(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		TB_CC_M4XP_NativeArlandComponent nativePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (nativePolicy) return nativePolicy.AuthorizeCapture(faction, base);
		if (!s_Instance || !s_Instance.m_bReady || !faction || !base) return false;
		string id = s_Instance.GetId(base); if (id.IsEmpty()) return true;
		if (id == s_Instance.m_Start.GetSelectedMobId() || id == s_Instance.m_Start.GetSelectedEndpointId() || s_Instance.IsTerminal() || s_Instance.m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return false;
		if (faction.GetFactionKey() == "US") return (s_Instance.m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || s_Instance.m_iOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) && id == s_Instance.GetBLUFORLegalObjective();
		if (faction.GetFactionKey() == "USSR") return (s_Instance.m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE || s_Instance.m_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) && id == s_Instance.GetOPFORLegalObjective();
		return false;
	}
	static bool CanFactionSeizeTerminalHQ(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		TB_CC_M4XP_NativeArlandComponent nativePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (nativePolicy) return nativePolicy.AuthorizeCapture(faction, base);
		// M4 HQs are permanent faction anchors. Everon keeps its separate M2.5 policy.
		return false;
	}
	static bool IsPlayerDeploymentAllowed(IEntity spawnParent)
	{
		if (!s_Instance || !s_Instance.m_bReady || !spawnParent) return true;
		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(spawnParent.FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (!base) return true;
		string id = s_Instance.GetId(base);
		if (id.IsEmpty()) return true;
		return id == s_Instance.m_Start.GetSelectedMobId();
	}

	bool IsReady() { return m_bReady; }
	int GetRevision() { return m_iRevision; }
	int GetOutcome() { return m_iOutcome; }
	int GetDepthPermille() { return m_iDepthPermille; }
	bool IsVictoryPressureActive() { return IsPressure() && m_bPressureTimerActive; }
	bool HasBLUFORFoothold() { return m_bBLUFORAdvanced; }
	int GetPressureStartCount() { return m_iPressureStarts; }
	int GetPressureRemainingSeconds()
	{
		if (!m_bPressureTimerActive) return -1;
		int remainingMs = m_PressureDeadline.DiffMilliseconds(ChimeraWorld.CastFrom(GetGame().GetWorld()).GetServerTimestamp());
		return Math.Max((remainingMs + 999) / 1000, 0);
	}
	protected int GetConfiguredPressureDuration(int outcome)
	{
		// Historical graph acceptance retains its separate proof/hold settings.
		if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) return m_iOPFORVictoryHoldSeconds;
		return m_iBLUFORVictoryHoldSeconds;
	}
	protected void PublishPressureClock()
	{
		if (!Replication.IsServer() || !m_bReady) return;
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!mode) return;
		int duration = GetConfiguredPressureDuration(m_iOutcome);
		mode.TB_CC_PublishPressure(m_iOutcome, GetPressureRemainingSeconds(), duration, m_bBLUFORAdvanced);
	}
	int GetRequiredOrdinaryPositionCount() { int usCount; int ussrCount; int requiredCount; CountRequiredTerritory(usCount, ussrCount, requiredCount); return requiredCount; }
	string GetSelectedMobId() { string result; if (m_Start) result = m_Start.GetSelectedMobId(); return result; }
	string GetSelectedEndpointId() { string result; if (m_Start) result = m_Start.GetSelectedEndpointId(); return result; }
	TB_CC_M4_3_StrategicEvaluator GetEvaluator() { return m_Evaluator; }
	bool IsEndpointReady() { return m_Evaluator && m_Evaluator.IsEndpointReady(); }
	TB_PVE_M3_2_OPFOROperationCoordinator GetCoordinator() { return m_M3Coordinator; }
	TB_PVE_M3_3_TacticalStagingPolicy GetStagingPolicy() { return m_M3Staging; }
	TB_PVE_M3_5_ForceSelectionPolicy GetSelectionPolicy() { return m_M3Selection; }
	TB_PVE_M3_6_OPFORFrontlineGarrisonManager GetGarrisonManager() { return m_M3Garrison; }
	int GetLastEvaluationDurationMs() { return m_iLastEvaluationDurationMs; }
	int GetLastOwnershipEventDurationMs() { return m_iLastOwnershipEventDurationMs; }
	int GetLastProjectionDurationMs() { return m_iLastProjectionDurationMs; }
	void TB_CC_M4_4_InjectInvalidForProof()
	{
		if (!Replication.IsServer() || m_iAuthorityProofMode != 8) return;
		CancelVictoryPressure("proof-invalid");
		m_iOutcome = TB_PVE_M2_5_CampaignOutcome.INVALID;
		PublishIfChanged("proof-invalid-state");
		if (m_M3Coordinator) m_M3Coordinator.Stop("M4.4F-invalid-proof");
		if (m_M3Garrison) m_M3Garrison.Stop("M4.4F-invalid-proof");
		Print("[TB_CC_M4_4F] diagnostic-invalid injected=1 ownershipMutation=0");
	}
}
