class TB_PVE_M2_1_LinearFrontierComponentClass : ScriptComponentClass {}

enum TB_PVE_M2_2_CampaignValidity
{
	UNRESOLVED = 0,
	VALID_LINEAR_FRONTIER = 1,
	INVALID_NON_CONTIGUOUS = 2,
	VALID_GRAPH = 3,
	INVALID_GRAPH = 4
}

modded class SCR_GameModeCampaign
{
	[RplProp()]
	protected int m_iTBM2_2Validity;
	[RplProp()]
	protected int m_iTBM2_2Frontier = -1;
	[RplProp()]
	protected int m_iTBM2_2BluforLegal = -1;
	[RplProp()]
	protected int m_iTBM2_2OpforLegal = -1;
	[RplProp()]
	protected int m_iTBM2_2LockedMask;
	[RplProp()]
	protected int m_iTBM2_5Outcome;
	[RplProp(onRplName: "TB_M2_2_OnRevision")]
	protected int m_iTBM2_2Revision;

	void TB_M2_2_Publish(int validity, int frontier, int bluforLegal, int opforLegal, int lockedMask, int outcome, string reason)
	{
		if (!Replication.IsServer()) return;
		m_iTBM2_2Validity = validity;
		m_iTBM2_2Frontier = frontier;
		m_iTBM2_2BluforLegal = bluforLegal;
		m_iTBM2_2OpforLegal = opforLegal;
		m_iTBM2_2LockedMask = lockedMask;
		m_iTBM2_5Outcome = outcome;
		m_iTBM2_2Revision++;
		Replication.BumpMe();
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConsumeProjection(m_iTBM2_2Revision, validity, frontier, bluforLegal, opforLegal, lockedMask, outcome, "authority");
		PrintFormat("[TB_PVE_M2_5][SERVER] projection-published revision=%1 reason=%2 validity=%3 outcome=%4 frontier=%5 USLegal=%6 USSRLegal=%7 lockedMask=%8 authority=1", m_iTBM2_2Revision, reason, validity, outcome, frontier, bluforLegal, opforLegal, lockedMask);
	}

	int TB_M2_2_GetRevision() { return m_iTBM2_2Revision; }
	int TB_M2_2_GetValidity() { return m_iTBM2_2Validity; }
	int TB_M2_2_GetFrontier() { return m_iTBM2_2Frontier; }
	int TB_M2_2_GetBluforLegal() { return m_iTBM2_2BluforLegal; }
	int TB_M2_2_GetOpforLegal() { return m_iTBM2_2OpforLegal; }
	int TB_M2_5_GetOutcome() { return m_iTBM2_5Outcome; }
	bool TB_M2_2_IsLocked(int index) { return (m_iTBM2_2LockedMask & (1 << index)) != 0; }

	// RPC_DoEndMatch is protected in vanilla. Keep the call inside the modded
	// game-mode class and expose only the narrow M2.5 consequence boundary.
	void TB_M2_5_EndMatch(int winningFactionId)
	{
		RPC_DoEndMatch(winningFactionId);
	}


	protected void TB_M2_2_OnRevision()
	{
		if (Replication.IsServer()) return;
		GetGame().GetCallqueue().CallLater(TB_M2_2_LogSnapshot, 0);
	}

	protected void TB_M2_2_LogSnapshot()
	{
		TB_PVE_M2_3_DiagnosticConsumer.Start();
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConsumeProjection(m_iTBM2_2Revision, m_iTBM2_2Validity, m_iTBM2_2Frontier, m_iTBM2_2BluforLegal, m_iTBM2_2OpforLegal, m_iTBM2_2LockedMask, m_iTBM2_5Outcome, "replicated-client");
		PrintFormat("[TB_PVE_M2_2][CLIENT] projection-received revision=%1 validity=%2 frontier=%3 USLegal=%4 USSRLegal=%5 lockedMask=%6 OBJ_02Locked=%7 OBJ_03Locked=%8 proxy=1 readOnly=1", m_iTBM2_2Revision, m_iTBM2_2Validity, m_iTBM2_2Frontier, m_iTBM2_2BluforLegal, m_iTBM2_2OpforLegal, m_iTBM2_2LockedMask, TB_M2_2_IsLocked(2), TB_M2_2_IsLocked(3));
	}
}

// One deliberately small linear campaign-policy slice. Route order and stable
// objective IDs are authored data; current ownership remains vanilla truth.
class TB_PVE_M2_1_LinearFrontierComponent : ScriptComponent
{
	[Attribute(desc: "Ordered world entity names: MOB followed by objectives.", category: "TB PvE M2.1")]
	protected ref array<string> m_aRouteEntityNames;

	[Attribute(desc: "Stable project-owned IDs parallel to route entity names.", category: "TB PvE M2.1")]
	protected ref array<string> m_aRouteObjectiveIds;

	[Attribute(defvalue: "TB_LINEAR_ROUTE", desc: "Stable authored route identity.", category: "TB PvE M2.4")]
	protected string m_sRouteId;

	[Attribute(defvalue: "MOB", desc: "Explicit authored BLUFOR campaign endpoint ID.", category: "TB PvE M2.5")]
	protected string m_sMobObjectiveId;

	[Attribute(defvalue: "OPFOR_ENDPOINT", desc: "Explicit authored OPFOR campaign endpoint ID.", category: "TB PvE M2.5")]
	protected string m_sTerminalObjectiveId;

	[Attribute(defvalue: "USSR", desc: "Conventional OPFOR faction permitted to seize the legally exposed BLUFOR terminal HQ.", category: "TB PvE M2.5")]
	protected FactionKey m_sOpforFactionKey;

	[Attribute(defvalue: "1", desc: "Run the bounded M2.1 dedicated proof harness.", category: "TB PvE M2.1")]
	protected bool m_bRunVerticalSliceProof;

	[Attribute(defvalue: "0", desc: "Run the bounded M2.4 production-route proof.", category: "TB PvE M2.4")]
	protected bool m_bRunProductionScaleProof;

	[Attribute(defvalue: "0", desc: "M2.5 authority proof: 0 off, 1 BLUFOR endpoint, 2 OPFOR endpoint.", category: "TB PvE M2.5")]
	protected int m_iCampaignOutcomeProofMode;

	[Attribute(defvalue: "1", desc: "Forward an established TB terminal outcome to vanilla Conflict RPC_DoEndMatch. Disable only for bounded automated proof.", category: "TB PvE M2.5")]
	protected bool m_bInvokeVanillaGameEnd;

	[Attribute(defvalue: "0", desc: "Run the bounded M3.1 autonomous OPFOR attack proof.", category: "TB PvE M3.1")]
	protected bool m_bRunM3_1Proof;

	[Attribute(defvalue: "0", desc: "Enable the authority-only single-operation OPFOR coordinator.", category: "TB PvE M3.2")]
	protected bool m_bEnableM3_2Coordinator;

	[Attribute(defvalue: "45", desc: "Seconds a stable legal OPFOR target must remain available before operation creation.", category: "TB PvE M3.2")]
	protected int m_iM3_2InitialAttackDelaySeconds;

	[Attribute(defvalue: "6", desc: "Seconds after terminal operation disposal before the coordinator re-queries M2.", category: "TB PvE M3.2")]
	protected int m_iM3_2AttackCooldownSeconds;

	[Attribute(defvalue: "0", desc: "Run the bounded M3.2 coordinator authority proof.", category: "TB PvE M3.2")]
	protected bool m_bRunM3_2Proof;

	[Attribute(defvalue: "350", desc: "Minimum operation staging distance from the current target.", category: "TB PvE M3.3")]
	protected float m_fM3_3MinimumTargetDistance;

	[Attribute(defvalue: "500", desc: "Maximum operation staging distance from the current target.", category: "TB PvE M3.3")]
	protected float m_fM3_3MaximumTargetDistance;

	[Attribute(defvalue: "250", desc: "Minimum staging distance from human-controlled player characters.", category: "TB PvE M3.3")]
	protected float m_fM3_3MinimumPlayerDistance;

	[Attribute(defvalue: "900", desc: "Human observer range within which direct line of sight rejects a candidate.", category: "TB PvE M3.3")]
	protected float m_fM3_3VisibilityRelevanceDistance;

	[Attribute(defvalue: "20", desc: "Local infantry navigation tolerance used by staging validation.", category: "TB PvE M3.3")]
	protected float m_fM3_3NavigationTolerance;

	[Attribute(defvalue: "12", desc: "Maximum staging candidates evaluated per operation creation.", category: "TB PvE M3.3")]
	protected int m_iM3_3CandidateAttempts;

	[Attribute(defvalue: "0", desc: "Emit per-candidate M3.3 staging diagnostics.", category: "TB PvE M3.3")]
	protected bool m_bM3_3DetailedDiagnostics;

	[Attribute(defvalue: "0", desc: "Run the bounded M3.3 tactical-staging authority proof.", category: "TB PvE M3.3")]
	protected bool m_bRunM3_3Proof;

	[Attribute(defvalue: "0", desc: "Run the bounded M3.4 force-package authority proof.", category: "TB PvE M3.4")]
	protected bool m_bRunM3_4Proof;

	[Attribute(defvalue: "2", desc: "Minimum active human BLUFOR players for MEDIUM band.", category: "TB PvE M3.5")]
	protected int m_iM3_5MediumPlayers;
	[Attribute(defvalue: "4", desc: "Minimum active human BLUFOR players for HIGH band.", category: "TB PvE M3.5")]
	protected int m_iM3_5HighPlayers;
	[Attribute(defvalue: "334", desc: "Maximum normalized campaign depth (permille) classified EARLY.", category: "TB PvE M3.5")]
	protected int m_iM3_5EarlyMaximumPermille;
	[Attribute(defvalue: "667", desc: "Maximum normalized campaign depth (permille) classified MID.", category: "TB PvE M3.5")]
	protected int m_iM3_5MidMaximumPermille;
	[Attribute(defvalue: "1", desc: "Strength steps added in MID campaign depth.", category: "TB PvE M3.5")]
	protected int m_iM3_5MidDepthModifier;
	[Attribute(defvalue: "2", desc: "Strength steps added in LATE campaign depth.", category: "TB PvE M3.5")]
	protected int m_iM3_5LateDepthModifier;
	[Attribute(defvalue: "1", desc: "One-operation strength modifier following FAILED.", category: "TB PvE M3.5")]
	protected int m_iM3_5FailedResultModifier;
	[Attribute(defvalue: "LIGHT", category: "TB PvE M3.5")] protected string m_sM3_5LightPackage;
	[Attribute(defvalue: "STANDARD", category: "TB PvE M3.5")] protected string m_sM3_5StandardPackage;
	[Attribute(defvalue: "REINFORCED", category: "TB PvE M3.5")] protected string m_sM3_5ReinforcedPackage;
	[Attribute(defvalue: "0", desc: "Run the bounded M3.5 strategic force-selection proof.", category: "TB PvE M3.5")]
	protected bool m_bRunM3_5Proof;

	[Attribute(defvalue: "1", desc: "Enable the authority-only OPFOR frontline garrison manager.", category: "TB PvE M3.6")]
	protected bool m_bEnableM3_6Garrison;
	[Attribute(defvalue: "STANDARD", desc: "Fixed defensive package; independent of M3.5 attack selection.", category: "TB PvE M3.6")]
	protected string m_sM3_6GarrisonPackage;
	[Attribute(defvalue: "250", category: "TB PvE M3.6")] protected float m_fM3_6MinimumPlayerDistance;
	[Attribute(defvalue: "900", category: "TB PvE M3.6")] protected float m_fM3_6VisibilityRange;
	[Attribute(defvalue: "250", category: "TB PvE M3.6")] protected float m_fM3_6CleanupPlayerDistance;
	[Attribute(defvalue: "55", category: "TB PvE M3.6")] protected float m_fM3_6PlacementRadius;
	[Attribute(defvalue: "32", category: "TB PvE M3.6")] protected int m_iM3_6CandidateAttempts;
	[Attribute(defvalue: "0", desc: "Run bounded M3.6 authority proof.", category: "TB PvE M3.6")]
	protected bool m_bRunM3_6Proof;
	[Attribute(defvalue: "1", desc: "Enable low-frequency read-only human playtest telemetry.", category: "TB PvE M3.7")]
	protected bool m_bEnableM3_7PlaytestTelemetry;

	protected static TB_PVE_M2_1_LinearFrontierComponent s_Instance;
	protected ref array<SCR_CampaignMilitaryBaseComponent> m_aRouteBases = {};
	protected ref map<string, bool> m_mLastEligibility = new map<string, bool>();
	protected bool m_bResolved;
	protected bool m_bContiguous;
	protected int m_iFrontierIndex = -1;
	protected int m_iBluforLegalIndex = -1;
	protected int m_iOpforLegalIndex = -1;
	protected string m_sLastSnapshot;
	protected bool m_bM2_2DiagnosticInvalidOverride;
	protected bool m_bBluforAdvancedBeyondEndpoint;
	protected int m_iBluforEndpointIndex = -1;
	protected int m_iOpforEndpointIndex = -1;
	protected int m_iCampaignOutcome = TB_PVE_M2_5_CampaignOutcome.NOT_READY;
	protected bool m_bVanillaEndSignalled;
	protected ref TB_PVE_M3_2_OPFOROperationCoordinator m_M3_2Coordinator;
	protected ref TB_PVE_M3_3_TacticalStagingPolicy m_M3_3StagingPolicy;
	protected ref TB_PVE_M3_4_ForceAssemblyService m_M3_4AssemblyService;
	protected ref TB_PVE_M3_5_ForceSelectionPolicy m_M3_5SelectionPolicy;
	protected ref TB_PVE_M3_6_OPFORFrontlineGarrisonManager m_M3_6GarrisonManager;
	protected ref TB_PVE_M3_7_PlaytestTelemetry m_M3_7PlaytestTelemetry;

	protected int m_iProjectedValidity;
	protected int m_iProjectedFrontier = -1;
	protected int m_iProjectedBluforLegal = -1;
	protected int m_iProjectedOpforLegal = -1;
	protected int m_iProjectedLockedMask;
	protected int m_iProjectedOutcome = TB_PVE_M2_5_CampaignOutcome.NOT_READY;
	protected int m_iProjectedRevision;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureRoute(m_aRouteObjectiveIds);
		if (!Replication.IsServer())
		{
			TB_PVE_M2_3_DiagnosticConsumer.Start();
			Print("[TB_PVE_M2_2][CLIENT] frontier-policy-component authority=0 enforcement=disabled projectionOwner=SCR_GameModeCampaign");
			return;
		}

		SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Insert(OnBaseFactionChanged);
		TB_PVE_M2_3_DiagnosticConsumer.Start();
		GetGame().GetCallqueue().CallLater(ResolveRoute, 15000);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_M3_2Coordinator)
			m_M3_2Coordinator.Stop("route-component-deleted");
		if (m_M3_6GarrisonManager)
			m_M3_6GarrisonManager.Stop("route-component-deleted");
		if (m_M3_7PlaytestTelemetry)
			m_M3_7PlaytestTelemetry.Stop();
		if (s_Instance == this)
			s_Instance = null;
		SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Remove(OnBaseFactionChanged);
		super.OnDelete(owner);
	}

	static bool IsFrontierActive()
	{
		return s_Instance != null;
	}

	static TB_PVE_M2_1_LinearFrontierComponent GetInstance()
	{
		return s_Instance;
	}

	static bool CanFactionCaptureBase(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		if (TB_CC_M4_4_ArlandGraphCampaignComponent.IsGraphCampaignActive())
			return TB_CC_M4_4_ArlandGraphCampaignComponent.CanFactionCaptureBase(faction, base);
		if (!s_Instance)
			return true;
		if (!s_Instance.m_bResolved)
			return false;
		return s_Instance.EvaluateEligibility(faction, base);
	}

	// Production terminal-HQ exception policy. This answers only whether the
	// campaign-specific HQ rejection may be lifted; it does not evaluate the
	// actor or mutate capture/ownership state.
	static bool CanFactionSeizeTerminalHQ(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		if (TB_CC_M4_4_ArlandGraphCampaignComponent.IsGraphCampaignActive())
			return TB_CC_M4_4_ArlandGraphCampaignComponent.CanFactionSeizeTerminalHQ(faction, base);
		if (!Replication.IsServer() || !s_Instance || !s_Instance.m_bResolved || !faction || !base)
			return false;
		if (!base.IsInitialized() || !base.IsHQ() || s_Instance.m_iCampaignOutcome != TB_PVE_M2_5_CampaignOutcome.ACTIVE)
			return false;
		if (faction.GetFactionKey() != s_Instance.m_sOpforFactionKey || s_Instance.m_iBluforEndpointIndex < 0)
			return false;
		if (base != s_Instance.m_aRouteBases.Get(s_Instance.m_iBluforEndpointIndex))
			return false;
		if (s_Instance.m_iOpforLegalIndex != s_Instance.m_iBluforEndpointIndex)
			return false;
		string endpointId = s_Instance.m_aRouteObjectiveIds.Get(s_Instance.m_iBluforEndpointIndex);
		if (TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE || TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() != endpointId)
			return false;
		return s_Instance.EvaluateEligibility(faction, base);
	}

	bool IsReady() { return m_bResolved; }
	bool IsContiguous() { return m_bContiguous; }
	int GetFrontierIndex() { return m_iFrontierIndex; }
	int GetBluforLegalIndex() { return m_iBluforLegalIndex; }
	int GetOpforLegalIndex() { return m_iOpforLegalIndex; }
	int GetRouteCount() { return m_aRouteBases.Count(); }
	SCR_CampaignMilitaryBaseComponent GetRouteBase(int index) { return m_aRouteBases.Get(index); }
	string GetRouteId(int index) { return m_aRouteObjectiveIds.Get(index); }
	bool ShouldRunProof() { return m_bRunVerticalSliceProof; }
	int GetCampaignOutcomeProofMode() { return m_iCampaignOutcomeProofMode; }
	string GetRouteIdentity() { return m_sRouteId; }
	int GetProjectedRevision() { return m_iProjectedRevision; }
	int GetProjectedValidity() { return m_iProjectedValidity; }
	int GetProjectedFrontier() { return m_iProjectedFrontier; }
	int GetProjectedBluforLegal() { return m_iProjectedBluforLegal; }
	int GetProjectedOpforLegal() { return m_iProjectedOpforLegal; }
	int GetCampaignOutcome() { return m_iCampaignOutcome; }
	int GetBluforEndpointIndex() { return m_iBluforEndpointIndex; }
	int GetOpforEndpointIndex() { return m_iOpforEndpointIndex; }
	bool IsProjectedObjectiveLocked(int index) { return (m_iProjectedLockedMask & (1 << index)) != 0; }
	TB_PVE_M3_2_OPFOROperationCoordinator GetM3_2Coordinator() { return m_M3_2Coordinator; }
	TB_PVE_M3_3_TacticalStagingPolicy GetM3_3StagingPolicy() { return m_M3_3StagingPolicy; }
	TB_PVE_M3_4_ForceAssemblyService GetM3_4AssemblyService() { return m_M3_4AssemblyService; }
	TB_PVE_M3_5_ForceSelectionPolicy GetM3_5SelectionPolicy() { return m_M3_5SelectionPolicy; }
	TB_PVE_M3_6_OPFORFrontlineGarrisonManager GetM3_6GarrisonManager() { return m_M3_6GarrisonManager; }

	protected void ResolveRoute()
	{
		m_aRouteBases.Clear();
		m_bResolved = false;
		if (!m_aRouteEntityNames || !m_aRouteObjectiveIds || m_aRouteEntityNames.Count() != m_aRouteObjectiveIds.Count() || m_aRouteEntityNames.Count() < 3 || m_aRouteEntityNames.Count() > 31)
		{
			PrintFormat("[TB_PVE_M2_4] FAIL route-size route=%1 entities=%2 ids=%3 required=3..31", m_sRouteId, m_aRouteEntityNames.Count(), m_aRouteObjectiveIds.Count());
			return;
		}
		m_iBluforEndpointIndex = m_aRouteObjectiveIds.Find(m_sMobObjectiveId);
		m_iOpforEndpointIndex = m_aRouteObjectiveIds.Find(m_sTerminalObjectiveId);
		if (m_sRouteId.IsEmpty() || m_sMobObjectiveId.IsEmpty() || m_sTerminalObjectiveId.IsEmpty() || m_sMobObjectiveId == m_sTerminalObjectiveId || m_iBluforEndpointIndex != 0 || m_iOpforEndpointIndex != m_aRouteObjectiveIds.Count() - 1)
		{
			PrintFormat("[TB_PVE_M2_5] FAIL endpoint-contract route=%1 BLUFOR=%2 resolvedIndex=%3 requiredBoundary=0 OPFOR=%4 resolvedIndex=%5 requiredBoundary=%6", m_sRouteId, m_sMobObjectiveId, m_iBluforEndpointIndex, m_sTerminalObjectiveId, m_iOpforEndpointIndex, m_aRouteObjectiveIds.Count() - 1);
			return;
		}

		map<string, bool> uniqueIds = new map<string, bool>();
		map<string, bool> uniqueEntities = new map<string, bool>();

		for (int i; i < m_aRouteEntityNames.Count(); i++)
		{
			if (m_aRouteObjectiveIds[i].IsEmpty() || m_aRouteEntityNames[i].IsEmpty() || uniqueIds.Contains(m_aRouteObjectiveIds[i]) || uniqueEntities.Contains(m_aRouteEntityNames[i]))
			{
				PrintFormat("[TB_PVE_M2_4] FAIL duplicate-or-empty-route-entry index=%1 id=%2 entity=%3", i, m_aRouteObjectiveIds[i], m_aRouteEntityNames[i]);
				return;
			}
			uniqueIds.Set(m_aRouteObjectiveIds[i], true);
			uniqueEntities.Set(m_aRouteEntityNames[i], true);
			IEntity entity = GetGame().GetWorld().FindEntityByName(m_aRouteEntityNames[i]);
			SCR_CampaignMilitaryBaseComponent base;
			if (entity)
				base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base || !base.IsInitialized())
			{
				PrintFormat("[TB_PVE_M2_1] FAIL route unresolved index=%1 id=%2 entityName=%3 entity=%4 base=%5 initialized=%6", i, m_aRouteObjectiveIds[i], m_aRouteEntityNames[i], entity, base, base && base.IsInitialized());
				GetGame().GetCallqueue().CallLater(ResolveRoute, 1000);
				return;
			}
			SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(entity.FindComponent(SCR_CampaignSeizingComponent));
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			bool isMob = i == m_iBluforEndpointIndex;
			bool deploymentPolicy = isMob || (entity.FindComponent(TB_PVE_M1_3_NoPlayerDeploymentMarker) && spawn && !spawn.IsSpawnPointEnabled());
			if (!seizing || !spawn || !deploymentPolicy || (isMob && !base.IsHQ()) || (!isMob && base.IsHQ()))
			{
				PrintFormat("[TB_PVE_M2_4] FAIL base-contract index=%1 id=%2 seizing=%3 spawn=%4 deploymentPolicy=%5 isHQ=%6 expectedMOB=%7", i, m_aRouteObjectiveIds[i], seizing != null, spawn != null, deploymentPolicy, base.IsHQ(), isMob);
				return;
			}
			m_aRouteBases.Insert(base);
			PrintFormat("[TB_PVE_M2_4] route-node route=%1 index=%2 id=%3 entity=%4 baseName=%5 position=%6 endpoint=%7", m_sRouteId, i, m_aRouteObjectiveIds[i], m_aRouteEntityNames[i], base.GetBaseName(), entity.GetOrigin(), i == 0 || i == m_aRouteEntityNames.Count() - 1);
		}

		m_bResolved = true;
		array<vector> objectivePositions = {};
		foreach (SCR_CampaignMilitaryBaseComponent routeBase : m_aRouteBases)
			objectivePositions.Insert(routeBase.GetOwner().GetOrigin());
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureObjectivePositions(objectivePositions);
		PrintFormat("[TB_PVE_M2_5] endpoint-contract-valid route=%1 BLUFOR=%2 index=%3 OPFOR=%4 index=%5 distinct=1 boundaries=1", m_sRouteId, m_sMobObjectiveId, m_iBluforEndpointIndex, m_sTerminalObjectiveId, m_iOpforEndpointIndex);
		PrintFormat("[TB_PVE_M2_4] route-valid route=%1 nodes=%2 ordinary=%3 representationLimit=31 server=%4 dedicated=%5", m_sRouteId, m_aRouteBases.Count(), m_aRouteBases.Count() - 2, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated);
		RecalculateFrontier("route-resolved");

		if (m_bRunVerticalSliceProof)
			GetGame().GetCallqueue().CallLater(TB_PVE_M2_1_VerticalSliceProof.Start, 1000, false, this);
		else if (m_bRunProductionScaleProof)
			GetGame().GetCallqueue().CallLater(TB_PVE_M2_4_ProductionRouteProof.Start, 1000, false, this);
		else if (m_iCampaignOutcomeProofMode > 0)
			GetGame().GetCallqueue().CallLater(TB_PVE_M2_5_CampaignOutcomeProof.Start, 1000, false, this, m_iCampaignOutcomeProofMode);
		else if (m_bRunM3_1Proof)
			GetGame().GetCallqueue().CallLater(TB_PVE_M3_1_AutonomousAttackProof.Start, 1000, false, this);

		if (m_bEnableM3_2Coordinator)
		{
			m_M3_3StagingPolicy = new TB_PVE_M3_3_TacticalStagingPolicy();
			m_M3_3StagingPolicy.Configure(m_fM3_3MinimumTargetDistance, m_fM3_3MaximumTargetDistance, m_fM3_3MinimumPlayerDistance, m_fM3_3VisibilityRelevanceDistance, m_fM3_3NavigationTolerance, m_iM3_3CandidateAttempts, m_bM3_3DetailedDiagnostics || m_bRunM3_3Proof);
			m_M3_4AssemblyService = new TB_PVE_M3_4_ForceAssemblyService();
			m_M3_5SelectionPolicy = new TB_PVE_M3_5_ForceSelectionPolicy();
			m_M3_5SelectionPolicy.Configure(m_iM3_5MediumPlayers, m_iM3_5HighPlayers, m_iM3_5EarlyMaximumPermille, m_iM3_5MidMaximumPermille, m_iM3_5MidDepthModifier, m_iM3_5LateDepthModifier, m_iM3_5FailedResultModifier, m_sM3_5LightPackage, m_sM3_5StandardPackage, m_sM3_5ReinforcedPackage);
			m_M3_2Coordinator = new TB_PVE_M3_2_OPFOROperationCoordinator();
			m_M3_2Coordinator.Start(m_M3_3StagingPolicy, m_M3_4AssemblyService, m_M3_5SelectionPolicy, m_iM3_2InitialAttackDelaySeconds, m_iM3_2AttackCooldownSeconds);
			if (m_bRunM3_2Proof)
				GetGame().GetCallqueue().CallLater(TB_PVE_M3_2_OPFORCoordinatorProof.Start, 1000, false, this, m_M3_2Coordinator);
			else if (m_bRunM3_3Proof)
				GetGame().GetCallqueue().CallLater(TB_PVE_M3_3_TacticalStagingProof.Start, 1000, false, this, m_M3_2Coordinator, m_M3_3StagingPolicy);
			else if (m_bRunM3_4Proof)
				GetGame().GetCallqueue().CallLater(TB_PVE_M3_4_ForcePackageProof.Start, 1000, false, this, m_M3_2Coordinator, m_M3_4AssemblyService);
			else if (m_bRunM3_5Proof)
				GetGame().GetCallqueue().CallLater(TB_PVE_M3_5_ForceSelectionProof.Start, 1000, false, this, m_M3_2Coordinator, m_M3_5SelectionPolicy);
		}

		if (m_bEnableM3_6Garrison)
		{
			m_M3_6GarrisonManager = new TB_PVE_M3_6_OPFORFrontlineGarrisonManager();
			m_M3_6GarrisonManager.Configure(m_sM3_6GarrisonPackage, m_fM3_6MinimumPlayerDistance, m_fM3_6VisibilityRange, m_fM3_6CleanupPlayerDistance, m_fM3_6PlacementRadius, m_iM3_6CandidateAttempts);
			m_M3_6GarrisonManager.Start();
			if (m_bRunM3_6Proof)
				GetGame().GetCallqueue().CallLater(TB_PVE_M3_6_FrontlineGarrisonProof.Start, 1000, false, this, m_M3_6GarrisonManager, m_M3_2Coordinator);
		}

		if (m_bEnableM3_7PlaytestTelemetry)
		{
			m_M3_7PlaytestTelemetry = new TB_PVE_M3_7_PlaytestTelemetry();
			m_M3_7PlaytestTelemetry.Start(m_M3_2Coordinator, m_M3_5SelectionPolicy, m_M3_6GarrisonManager);
		}
	}

	protected void OnBaseFactionChanged(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)
	{
		if (!m_bResolved || m_aRouteBases.Find(base) == -1)
			return;
		FactionKey previousKey = "<none>";
		FactionKey newKey = "<none>";
		if (previousFaction) previousKey = previousFaction.GetFactionKey();
		if (newFaction) newKey = newFaction.GetFactionKey();
		PrintFormat("[TB_PVE_M2_1] ownership-change-observed id=%1 previous=%2 new=%3 server=%4", GetIdForBase(base), previousKey, newKey, Replication.IsServer());
		GetGame().GetCallqueue().CallLater(RecalculateFrontier, 0, false, "ownership-change");
	}

	void RecalculateFrontier(string reason)
	{
		if (!m_bResolved)
			return;

		int oldFrontier = m_iFrontierIndex;
		m_iFrontierIndex = -1;
		m_iBluforLegalIndex = -1;
		m_iOpforLegalIndex = -1;
		m_bContiguous = true;
		bool foundNonUS;
		string snapshot;

		for (int i; i < m_aRouteBases.Count(); i++)
		{
			Faction owner = m_aRouteBases[i].GetFaction();
			FactionKey key = "<none>";
			if (owner) key = owner.GetFactionKey();
			if (!snapshot.IsEmpty()) snapshot += ",";
			snapshot += m_aRouteObjectiveIds[i] + "=" + key;

			if (key == "US")
			{
				if (foundNonUS)
					m_bContiguous = false;
				else
					m_iFrontierIndex = i;
			}
			else if (key == "USSR")
			{
				foundNonUS = true;
			}
			else
			{
				m_bContiguous = false;
				foundNonUS = true;
			}
		}

		if (m_bContiguous)
		{
			if (m_iFrontierIndex > m_iBluforEndpointIndex)
				m_bBluforAdvancedBeyondEndpoint = true;
			if (m_iFrontierIndex + 1 < m_aRouteBases.Count())
				m_iBluforLegalIndex = m_iFrontierIndex + 1;
			if (m_iFrontierIndex > m_iBluforEndpointIndex || (m_iFrontierIndex == m_iBluforEndpointIndex && m_bBluforAdvancedBeyondEndpoint))
				m_iOpforLegalIndex = m_iFrontierIndex;
		}

		int derivedOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		if (!m_bContiguous)
			derivedOutcome = TB_PVE_M2_5_CampaignOutcome.INVALID;
		else if (m_iFrontierIndex == m_iOpforEndpointIndex && m_aRouteBases[m_iOpforEndpointIndex].GetFaction().GetFactionKey() == "US")
			derivedOutcome = TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY;
		else if (m_iFrontierIndex < m_iBluforEndpointIndex && m_aRouteBases[m_iBluforEndpointIndex].GetFaction().GetFactionKey() == "USSR")
			derivedOutcome = TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
		if (m_iCampaignOutcome != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY && m_iCampaignOutcome != TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
			m_iCampaignOutcome = derivedOutcome;
		if (m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
		{
			m_iBluforLegalIndex = -1;
			m_iOpforLegalIndex = -1;
		}

		m_mLastEligibility.Clear();
		string usLegal = IndexToId(m_iBluforLegalIndex);
		string ussrLegal = IndexToId(m_iOpforLegalIndex);
		if (!m_bContiguous)
			PrintFormat("[TB_PVE_M2_1] INVALID-NON-CONTIGUOUS ownership=%1 policy=DENY_ALL_STRATEGIC_CAPTURES reason=%2", snapshot, reason);
		else
			PrintFormat("[TB_PVE_M2_5] ownership-snapshot reason=%1 ownership=%2 frontierIndex=%3 outcome=%4 bluforLegal=%5 opforLegal=%6 advanced=%7", reason, snapshot, m_iFrontierIndex, m_iCampaignOutcome, usLegal, ussrLegal, m_bBluforAdvancedBeyondEndpoint);

		if (oldFrontier >= 0 && m_iFrontierIndex > oldFrontier)
			PrintFormat("[TB_PVE_M2_1] frontier-moved-forward old=%1 new=%2 bluforLegal=%3 opforLegal=%4", oldFrontier, m_iFrontierIndex, usLegal, ussrLegal);
		else if (oldFrontier >= 0 && m_iFrontierIndex < oldFrontier)
			PrintFormat("[TB_PVE_M2_1] frontier-moved-backward old=%1 new=%2 bluforLegal=%3 opforLegal=%4", oldFrontier, m_iFrontierIndex, usLegal, ussrLegal);
		m_sLastSnapshot = snapshot;
		PublishProjection(reason);
	}

	protected void PublishProjection(string reason)
	{
		int validity = TB_PVE_M2_2_CampaignValidity.VALID_LINEAR_FRONTIER;
		int frontier = m_iFrontierIndex;
		int bluforLegal = m_iBluforLegalIndex;
		int opforLegal = m_iOpforLegalIndex;
		int outcome = m_iCampaignOutcome;
		bool terminal = m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
		if ((!m_bContiguous || m_bM2_2DiagnosticInvalidOverride) && !terminal)
		{
			validity = TB_PVE_M2_2_CampaignValidity.INVALID_NON_CONTIGUOUS;
			frontier = -1;
			bluforLegal = -1;
			opforLegal = -1;
			outcome = TB_PVE_M2_5_CampaignOutcome.INVALID;
		}

		int lockedMask;
		for (int i = 1; i < m_aRouteBases.Count(); i++)
			if (validity != TB_PVE_M2_2_CampaignValidity.VALID_LINEAR_FRONTIER || i != bluforLegal) lockedMask |= 1 << i;

		if (m_iProjectedRevision > 0 && m_iProjectedValidity == validity && m_iProjectedFrontier == frontier && m_iProjectedBluforLegal == bluforLegal && m_iProjectedOpforLegal == opforLegal && m_iProjectedLockedMask == lockedMask && m_iProjectedOutcome == outcome)
		{
			PrintFormat("[TB_PVE_M2_2][SERVER] reconstruction-stable revision=%1 reason=%2 validity=%3 frontier=%4 USLegal=%5 USSRLegal=%6 lockedMask=%7", m_iProjectedRevision, reason, validity, frontier, bluforLegal, opforLegal, lockedMask);
			return;
		}

		m_iProjectedValidity = validity;
		m_iProjectedFrontier = frontier;
		m_iProjectedBluforLegal = bluforLegal;
		m_iProjectedOpforLegal = opforLegal;
		m_iProjectedLockedMask = lockedMask;
		m_iProjectedOutcome = outcome;
		SCR_GameModeCampaign gameMode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			Print("[TB_PVE_M2_2][SERVER] FAIL projection game mode unavailable");
			return;
		}
		gameMode.TB_M2_2_Publish(validity, frontier, bluforLegal, opforLegal, lockedMask, outcome, reason);
		m_iProjectedRevision = gameMode.TB_M2_2_GetRevision();
		ApplyVanillaEndState(gameMode, outcome);
	}

	protected void ApplyVanillaEndState(SCR_GameModeCampaign gameMode, int outcome)
	{
		if (!m_bInvokeVanillaGameEnd || m_bVanillaEndSignalled)
			return;
		FactionKey winnerKey;
		if (outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY)
			winnerKey = "US";
		else if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
			winnerKey = "USSR";
		else
			return;
		FactionManager factionManager = GetGame().GetFactionManager();
		Faction winner = factionManager.GetFactionByKey(winnerKey);
		if (!winner)
		{
			PrintFormat("[TB_PVE_M2_5] ERROR vanilla-end-state winner faction unavailable key=%1", winnerKey);
			return;
		}
		m_bVanillaEndSignalled = true;
		int winnerId = factionManager.GetFactionIndex(winner);
		PrintFormat("[TB_PVE_M2_5] vanilla-end-state outcome=%1 winner=%2 factionId=%3 mechanism=SCR_GameModeCampaign.RPC_DoEndMatch", outcome, winnerKey, winnerId);
		gameMode.TB_M2_5_EndMatch(winnerId);
	}

	void M2_2_ReconstructProjection()
	{
		if (!Replication.IsServer()) return;
		RecalculateFrontier("M2.2-explicit-reconstruction");
	}

	void M2_2_SetDiagnosticInvalid(bool enabled)
	{
		if (!Replication.IsServer()) return;
		m_bM2_2DiagnosticInvalidOverride = enabled;
		PublishProjection("M2.2-invalid-diagnostic");
	}

	bool EvaluateSyntheticOwnership(array<string> ownershipKeys, string label)
	{
		bool foundNonUS;
		bool valid = ownershipKeys && ownershipKeys.Count() == m_aRouteObjectiveIds.Count();
		for (int i; valid && i < ownershipKeys.Count(); i++)
		{
			if (ownershipKeys[i] == "US")
			{
				if (foundNonUS) valid = false;
			}
			else if (ownershipKeys[i] == "USSR")
				foundNonUS = true;
			else
				valid = false;
		}
		string safePolicy = "DENY_ALL_STRATEGIC_CAPTURES";
		if (valid) safePolicy = "DERIVE_FRONTIER";
		PrintFormat("[TB_PVE_M2_1] synthetic-contiguity-test label=%1 nodes=%2 valid=%3 safePolicy=%4", label, ownershipKeys.Count(), valid, safePolicy);
		return valid;
	}

	protected bool EvaluateEligibility(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		int index = m_aRouteBases.Find(base);
		if (index == -1)
			return true;
		FactionKey key = faction.GetFactionKey();
		bool allowed;
		if (m_bM2_2DiagnosticInvalidOverride || m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || m_iCampaignOutcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
			allowed = false;
		else if (m_bContiguous && key == "US")
			allowed = index == m_iBluforLegalIndex;
		else if (m_bContiguous && key == "USSR")
			allowed = index == m_iOpforLegalIndex;

		string requestKey = key + ":" + m_aRouteObjectiveIds[index];
		bool previous;
		if (!m_mLastEligibility.Find(requestKey, previous) || previous != allowed)
		{
			m_mLastEligibility.Set(requestKey, allowed);
			string result = "DENY";
			if (allowed) result = "ALLOW";
			PrintFormat("[TB_PVE_M2_1] eligibility faction=%1 objective=%2 index=%3 result=%4 frontier=%5 contiguous=%6", key, m_aRouteObjectiveIds[index], index, result, m_iFrontierIndex, m_bContiguous);
		}
		return allowed;
	}

	protected string GetIdForBase(SCR_CampaignMilitaryBaseComponent base)
	{
		return IndexToId(m_aRouteBases.Find(base));
	}

	protected string IndexToId(int index)
	{
		if (index < 0 || index >= m_aRouteObjectiveIds.Count())
			return "<none>";
		return m_aRouteObjectiveIds[index];
	}
}

// The smallest safe interception available in the current API: vanilla first
// validates each physical actor; TB can only reject that valid faction for a
// route-marked base. The vanilla prevailing evaluator and all downstream state
// transitions remain untouched.
modded class SCR_SeizingComponent
{
	// Calls the original generic vanilla actor evaluator directly. The M2.5
	// campaign-HQ seam uses this only after the campaign override returned null,
	// preserving vanilla entity, life, vehicle/altitude, respawn and AI-LOD rules.
	SCR_Faction TB_PVE_M2_5_EvaluateVanillaActor(IEntity ent)
	{
		return super.EvaluateEntityFaction(ent);
	}

	override protected SCR_Faction EvaluateEntityFaction(IEntity ent)
	{
		SCR_Faction faction = super.EvaluateEntityFaction(ent);
		if (!faction || !Replication.IsServer())
			return faction;

		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(GetOwner().FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (!base || TB_PVE_M2_1_LinearFrontierComponent.CanFactionCaptureBase(faction, base))
			return faction;
		return null;
	}
}
