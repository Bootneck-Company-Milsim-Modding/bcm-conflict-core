// Full native Arland candidate. The accepted graph and campaign consumers stay
// shared; this file adapts only physical entity names, startup roles and origin.
[BaseContainerProps()]
class TB_CC_TerritoryImportanceOverride
{
	[Attribute(category: "BCM Territorial Scoring")] string m_sPositionId;
	[Attribute(defvalue: "70", category: "BCM Territorial Scoring")] int m_iImportance;
}

class TB_CC_M4XP_ArlandNativeDefinition
{
	static TB_CC_M4_1_MapDefinition Create(array<ref TB_CC_TerritoryImportanceOverride> importanceOverrides = null)
	{
		TB_CC_M4_1_MapDefinition accepted = TB_CC_M4_2_ArlandProductionDefinition.Create();
		TB_CC_M4_1_MapDefinition nativeDefinition = new TB_CC_M4_1_MapDefinition("ARLAND_NATIVE_FULL_01", 1, "BCM Native Arland Campaign");
		array<ref TB_CC_M4_1_StrategicPositionDefinition> positions = {};
		accepted.GetPositions(positions);
		foreach (TB_CC_M4_1_StrategicPositionDefinition position : positions)
		{
			array<string> traits = {};
			position.GetTraits(traits);
			int importance = position.GetStrategicImportance();
			if (importanceOverrides)
				foreach (TB_CC_TerritoryImportanceOverride importanceOverride : importanceOverrides)
					if (importanceOverride && importanceOverride.m_sPositionId == position.GetId()) importance = Math.Clamp(importanceOverride.m_iImportance, 0, 100);
			nativeDefinition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition(
				position.GetId(), position.GetPositionType(), position.GetAuthoredPosition(),
				"TB_CC_NATIVE_" + position.GetEntityName(), importance,
				position.GetRegion(), traits, position.IsMOBCandidate(),
				position.GetEndpointRole(), position.GetOwnershipAdapterType()));
		}
		array<ref TB_CC_M4_1_StrategicConnectionDefinition> connections = {};
		accepted.GetConnections(connections);
		foreach (TB_CC_M4_1_StrategicConnectionDefinition connection : connections)
			nativeDefinition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(connection.GetA(), connection.GetB()));
		return nativeDefinition;
	}
}

enum TB_CC_StrategicRole { ORDINARY, HQ_US, HQ_USSR }
enum TB_CC_HQLockState { PROTECTED, PRESSURED, VULNERABLE, CAPTURED }

class TB_CC_M4XP_CampaignRoles
{
	static int GetRole(string id)
	{
		if (id == "ARLAND::HQ_EAST") return TB_CC_StrategicRole.HQ_US;
		if (id == "ARLAND::HQ_WEST") return TB_CC_StrategicRole.HQ_USSR;
		return TB_CC_StrategicRole.ORDINARY;
	}
}

class TB_CC_M4XP_VanillaRadioEligibilityView : TB_CC_M4_3_RadioEligibilityView
{
	protected TB_CC_M4_1_StrategicPositionRegistry m_Registry;
	protected SCR_CampaignFaction m_US;
	void TB_CC_M4XP_VanillaRadioEligibilityView(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		m_Registry = registry;
		m_US = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
	}
	override bool IsReachable(string positionId, out string reason)
	{
		if (!m_Registry || !m_US) { reason = "registry-or-US-faction-unavailable"; return false; }
		TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(positionId);
		if (!resolved) { reason = "position-unresolved"; return false; }
		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (!base || !base.IsInitialized()) { reason = "base-uninitialized"; return false; }
		bool reachable = base.IsHQRadioTrafficPossible(m_US);
		if (reachable) reason = "vanilla-US-HQ-radio-traffic";
		else reason = "no-US-HQ-radio-traffic";
		return reachable;
	}
}

class TB_CC_M4XP_GameModeClass : TB_CC_M4X_GameModeClass {}
class TB_CC_M4XP_GameMode : TB_CC_M4X_GameMode
{
	override protected void InitalBaseSetup()
	{
		TB_CC_M4XP_InitializeBasePolicy();
	}
}

// Scoped adapter for the Creative Director's stock GameMode_Campaign entity.
// Native Start/OnStarted, base manager, replication and respawn stay untouched.
modded class SCR_GameModeCampaign
{
	[RplProp(onRplName: "TB_CC_ApplyNativeSnapshot")]
	protected string m_sTBCCNativeSnapshot;
	[RplProp()] protected WorldTimestamp m_TBCCNativePressureDeadline;
	void TB_CC_SetNativePressureDeadline(WorldTimestamp deadline) { m_TBCCNativePressureDeadline = deadline; }
	WorldTimestamp TB_CC_GetNativePressureDeadline() { return m_TBCCNativePressureDeadline; }
	void TB_CC_PublishNativeSnapshot(string snapshot)
	{
		if (!Replication.IsServer() || snapshot == m_sTBCCNativeSnapshot) return;
		m_sTBCCNativeSnapshot = snapshot;
		Replication.BumpMe();
	}
	void TB_CC_ApplyNativeSnapshot()
	{
		if (Replication.IsServer() || m_sTBCCNativeSnapshot.IsEmpty()) return;
		TB_CC_M4XP_NativeArlandComponent policy = TB_CC_M4XP_GetNativePolicy();
		if (policy) policy.ConsumeNativeSnapshot(m_sTBCCNativeSnapshot);
	}
	static TB_CC_M4XP_NativeArlandComponent TB_CC_M4XP_GetNativePolicy()
	{
		if (!GetGame() || !GetGame().GetWorld()) return null;
		IEntity entity = GetGame().GetWorld().FindEntityByName("TB_CC_NATIVE_ARLAND_CAMPAIGN");
		if (!entity) return null;
		return TB_CC_M4XP_NativeArlandComponent.Cast(entity.FindComponent(TB_CC_M4XP_NativeArlandComponent));
	}

	override protected void InitalBaseSetup()
	{
		if (!TB_CC_M4XP_GetNativePolicy()) { super.InitalBaseSetup(); return; }
		TB_CC_M4XP_InitializeBasePolicy();
	}

	override protected void CheckForWinner()
	{
		// Native control-point victory must not race BCM's HQ-unlock/capture policy.
		if (TB_CC_M4XP_GetNativePolicy()) return;
		super.CheckForWinner();
	}

	protected void TB_CC_M4XP_InitializeBasePolicy()
	{
		IEntity campaignEntity = GetGame().GetWorld().FindEntityByName("TB_CC_NATIVE_ARLAND_CAMPAIGN");
		TB_CC_M4XP_NativeArlandComponent campaign;
		if (campaignEntity) campaign = TB_CC_M4XP_NativeArlandComponent.Cast(campaignEntity.FindComponent(TB_CC_M4XP_NativeArlandComponent));
		if (!campaign) { Print("[TB_CC_M4XP] INIT_FAILED campaign-component-unavailable"); return; }
		string selectedMob = campaign.ResolveSelectedMobForStartup();
		if (selectedMob.IsEmpty()) { Print("[TB_CC_M4XP] INIT_FAILED mob-selection"); return; }
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		if (!us) { Print("[TB_CC_M4XP] INIT_FAILED US-faction-unavailable"); return; }
		TB_CC_M4XP_RetireAmbientSpawners(GetGame().GetWorld().FindEntityByName("ConflictMilitaryBase"));
		TB_CC_M4_1_MapDefinition definition = TB_CC_M4XP_ArlandNativeDefinition.Create();
		array<ref TB_CC_M4_1_StrategicPositionDefinition> positions = {};
		definition.GetPositions(positions);
		int callsignIndex = 0;
		foreach (TB_CC_M4_1_StrategicPositionDefinition position : positions)
		{
			IEntity entity = GetGame().GetWorld().FindEntityByName(position.GetEntityName());
			if (!entity) { PrintFormat("[TB_CC_M4XP] INIT_FAILED entity=%1", position.GetEntityName()); return; }
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base) { PrintFormat("[TB_CC_M4XP] INIT_FAILED base=%1", position.GetEntityName()); return; }
			TB_CC_M4XP_RetireAmbientSpawners(entity);
			base.Initialize();
			// Authored ownership need not emit OnFactionChanged during startup.
			// Preserve it, but perform the native radio setup that callback supplies.
			SCR_CampaignFaction radioFaction = base.GetCampaignFaction();
			if (!radioFaction) { PrintFormat("[TB_CC_M4XP] INIT_FAILED radio-faction id=%1", position.GetId()); return; }
			base.ChangeRadioSettings(radioFaction);
			if (TB_CC_M4XP_CampaignRoles.GetRole(position.GetId()) != TB_CC_StrategicRole.ORDINARY)
			{
				base.SetAsHQ(true);
			}
			// Vanilla CampaignMilitaryBaseManager.InitializeBases normally assigns
			// these indices after selecting HQs. Assign the same callsign contract
			// in stable registry order without replacing authored ownership.
			base.SetCallsignIndex(callsignIndex);
			if (base.GetCallsign() != callsignIndex)
			{
				PrintFormat("[TB_CC_M4XP] INIT_FAILED callsign id=%1 expected=%2 actual=%3", position.GetId(), callsignIndex, base.GetCallsign());
				return;
			}
			callsignIndex++;
		}
		// IsValidForSeizeTask also requires the native game-mode offset. The
		// stock initializer supplies it; our authored startup adapter must too.
		if (GetCallsignOffset() == SCR_MilitaryBaseComponent.INVALID_BASE_CALLSIGN)
			SetCallsignOffset(1);
		PrintFormat("[TB_CC_CAMPAIGN] service-start bases=12 callsigns=%1 hqUS=%2 hqUSSR=ARLAND::HQ_WEST ordinary=10 startupOwnership=authored gameplayOwnershipWrites=0", callsignIndex, selectedMob);
		LogFactionRelationship("US", "USSR");
		LogFactionRelationship("USSR", "US");
		LogFactionRelationship("US", "FIA");
		LogFactionRelationship("FIA", "US");
		LogFactionRelationship("USSR", "FIA");
		LogFactionRelationship("FIA", "USSR");
	}
	protected void TB_CC_M4XP_RetireAmbientSpawners(IEntity entity)
	{
		if (!entity) return;
		SCR_AmbientPatrolSpawnPointComponent patrol = SCR_AmbientPatrolSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientPatrolSpawnPointComponent));
		if (patrol)
		{
			// Native remnant spawners are owner-independent static FIA sources.
			// Territorial defence is owned by the single owner-aware M3 manager.
			patrol.SetIsEliminated(true);
			PrintFormat("[TB_CC_GARRISON] inheritedAmbientRetired entity=%1 source=ConflictMilitaryBase-child ownership=territorial-manager", entity.GetName());
		}
		IEntity child = entity.GetChildren();
		while (child) { TB_CC_M4XP_RetireAmbientSpawners(child); child = child.GetSibling(); }
	}

	protected void LogFactionRelationship(string sourceKey, string targetKey)
	{
		Faction source = GetGame().GetFactionManager().GetFactionByKey(sourceKey);
		Faction target = GetGame().GetFactionManager().GetFactionByKey(targetKey);
		if (!source || !target)
		{
			PrintFormat("[TB_CC_M4XP_FACTION] source=%1 target=%2 unresolved sourcePresent=%3 targetPresent=%4", sourceKey, targetKey, source != null, target != null);
			return;
		}
		PrintFormat("[TB_CC_M4XP_FACTION] source=%1 target=%2 enemy=%3 friendly=%4", sourceKey, targetKey, source.IsFactionEnemy(target), source.IsFactionFriendly(target));
	}
}

modded class SCR_CampaignFactionCommanderHandlerComponent
{
	override void CreateSeizeTask(notnull SCR_CampaignFaction faction, SCR_CampaignMilitaryBaseComponent targetBase, int playerid)
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && !policy.AllowsNativeSeizeTask(faction, targetBase)) return;
		super.CreateSeizeTask(faction, targetBase, playerid);
	}

	override protected void CheckSeizeTasks(notnull SCR_CampaignFaction faction, int playerid)
	{
		// Native blanket discovery presents every reachable base. BCM alone
		// chooses the single legal objective; its bridge uses native creation.
		if (SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy()) return;
		super.CheckSeizeTasks(faction, playerid);
	}
}

class TB_CC_M4XP_NativeArlandComponentClass : TB_CC_M4X_NativeCampaignComponentClass {}
class TB_CC_NativeStrategicPersistenceAdapter : TB_CC_StrategicPersistenceAdapter
{
	protected TB_CC_M4XP_NativeArlandComponent m_Campaign;
	void TB_CC_NativeStrategicPersistenceAdapter(TB_CC_M4XP_NativeArlandComponent campaign) { m_Campaign = campaign; }
	override bool Capture(TB_CC_CampaignContainer snapshot)
	{
		return m_Campaign && m_Campaign.CaptureStrategicSnapshot(snapshot);
	}
	override string GetCaptureError()
	{
		if (!m_Campaign) { return "campaign-component-unavailable"; }
		return m_Campaign.GetPersistenceCaptureError();
	}
}
class TB_CC_M4XP_NativeArlandComponent : TB_CC_M4X_NativeCampaignComponent
{
	// Persistence Phase 1: identity/container only; does not gate or restore gameplay yet.
	[Attribute(defvalue: "BCM_NATIVE_FULL_CAMPAIGN", category: "BCM Persistence")] protected string m_sPersistenceScenarioIdentity = "BCM_NATIVE_FULL_CAMPAIGN";
	[Attribute(defvalue: "THREE_FACTION_V1", category: "BCM Persistence")] protected string m_sPersistenceConfigIdentity = "THREE_FACTION_V1";
	[Attribute(defvalue: "60", category: "BCM Persistence")] protected int m_iPersistenceCheckpointSeconds = 60;
	protected ref TB_CC_PersistenceService m_Persistence;
	protected ref TB_CC_RestoreTransaction m_RestoreTransaction;
	protected ref TB_CC_CampaignContainer m_LastPersistenceObservation;
	protected ref map<string, string> m_StrategicFailureState = new map<string, string>();
	protected bool m_bFailureStateAwaitingImport;
	protected bool m_bRestoredTerminal;
	protected int m_iRestoredPressureDuration;
	protected int m_iFIAStrategicCooldown;
	protected int m_iFIAStrategicSerial;
	protected bool m_bFIAAdmissionAwaitingImport;
	protected string m_sPersistenceCaptureError;
	string GetPersistenceCaptureError() { return m_sPersistenceCaptureError; }
	protected bool RejectPersistenceCapture(string reason)
	{
		m_sPersistenceCaptureError = reason; return false;
	}
	static int PersistenceFlag(bool value)
	{
		if (value) return 1;
		return 0;
	}
	bool CaptureStrategicSnapshot(TB_CC_CampaignContainer snapshot)
	{
		m_sPersistenceCaptureError = "";
		if (!Replication.IsServer()) return RejectPersistenceCapture("authority-required");
		if (!snapshot) return RejectPersistenceCapture("candidate-missing");
		if (!m_bReady || !PersistenceAllowsCampaignConsumers()) return RejectPersistenceCapture("strategic-not-ready");
		if (!m_Registry || !m_Registry.IsReady()) return RejectPersistenceCapture("registry-unavailable");
		if (!m_USObjectiveCommitment || !m_FIARecaptureLedger) return RejectPersistenceCapture("objective-or-FIA-ledger-unavailable");
		snapshot.snapshotPresent = true; snapshot.campaignRevision = m_iRevision;
		snapshot.campaignState.Set("outcome", m_iOutcome.ToString());
		snapshot.campaignState.Set("foothold", PersistenceFlag(m_bBLUFORAdvanced).ToString());
		snapshot.campaignState.Set("USDepth", m_iDepthPermille.ToString()); snapshot.campaignState.Set("USSRDepth", m_iUSSRDepthPermille.ToString());
		snapshot.objectiveState.Set("committedUS", m_USObjectiveCommitment.GetCurrentId());
		snapshot.hqState.Set("US", m_iHQEastState.ToString()); snapshot.hqState.Set("USSR", m_iHQWestState.ToString());
		snapshot.pressureState.Set("active", PersistenceFlag(m_bPressureTimerActive).ToString());
		snapshot.pressureState.Set("remaining", GetPressureRemainingSeconds().ToString());
		snapshot.pressureState.Set("duration", GetConfiguredPressureDuration(m_iOutcome).ToString());
		snapshot.pressureState.Set("starts", m_iPressureStarts.ToString());
		snapshot.factionState.Set("FIA-strength", m_iFIAConventionalStrength.ToString());
		snapshot.factionState.Set("FIA-lossSerial", m_iFIALossSerial.ToString());
		if (m_FIARecapture) { m_iFIAStrategicCooldown = m_FIARecapture.GetStrategicCooldownSeconds(); m_iFIAStrategicSerial = m_FIARecapture.GetStrategicOperationSerial(); }
		snapshot.factionState.Set("FIA-cooldown", m_iFIAStrategicCooldown.ToString());
		snapshot.factionState.Set("FIA-operationSerial", m_iFIAStrategicSerial.ToString());
		if (m_M3Coordinator) m_M3Coordinator.ExportStrategicFailures(m_StrategicFailureState, m_aIds);
		string cooldown = "0";
		if (m_StrategicFailureState.Contains("USSR-cooldown")) cooldown = m_StrategicFailureState.Get("USSR-cooldown");
		snapshot.factionState.Set("USSR-cooldown", cooldown);
		foreach (string id : m_aIds)
		{
			FactionKey owner; if (!m_Registry.TryGetCurrentOwner(id, owner)) return RejectPersistenceCapture("ownership-unavailable:" + id);
			snapshot.strategicPositions.Insert(id); snapshot.positionOwners.Set(id, owner);
			snapshot.factionState.Set("FIA-former:" + id, PersistenceFlag(m_mFormerFIATerritory.Contains(id)).ToString());
			snapshot.factionState.Set("FIA-loss:" + id, m_mFIALossOrder.Get(id).ToString());
			string failures = "0";
			if (m_StrategicFailureState.Contains("USSR-failures:" + id)) failures = m_StrategicFailureState.Get("USSR-failures:" + id);
			snapshot.factionState.Set("USSR-failures:" + id, failures);
		}
		m_FIARecaptureLedger.ExportStrategicState(snapshot.factionState, m_aIds);
		// A live, unfinished capture has NOT changed authoritative ownership yet.
		// It is a restore precondition, not an incomplete committed save state.
		string problem = ValidateStrategicSnapshot(snapshot, false);
		if (!problem.IsEmpty()) return RejectPersistenceCapture(problem);
		return true;
	}
	// Resolve the COMPLETE document and every native owner before the first write.
	protected string ValidateStrategicSnapshot(TB_CC_CampaignContainer snapshot, bool forRestore = true)
	{
		if (!snapshot || !snapshot.snapshotPresent || snapshot.campaignRevision < 0 || snapshot.strategicPositions.Count() != m_aIds.Count() || snapshot.positionOwners.Count() != m_aIds.Count()) return "position-set";
		if (snapshot.campaignState.Count() != 4 || snapshot.objectiveState.Count() != 1 || !snapshot.objectiveState.Contains("committedUS") || snapshot.hqState.Count() != 2 || snapshot.pressureState.Count() != 4 || snapshot.factionState.Count() != 5 + 8 * m_aIds.Count() || !snapshot.operations.IsEmpty() || !TB_CC_PlayerProgression.Validate(snapshot.players)) return "payload-shape";
		int outcome; int foothold; int east; int west; int active; int remaining; int duration; int starts; int strength; int serial;
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.campaignState, "outcome", 1, 6, outcome) || !TB_CC_StrategicPersistence.ReadInt(snapshot.campaignState, "foothold", 0, 1, foothold)) return "campaign-state";
		int usDepth; int ussrDepth;
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.campaignState, "USDepth", 0, 1000, usDepth) || !TB_CC_StrategicPersistence.ReadInt(snapshot.campaignState, "USSRDepth", 0, 1000, ussrDepth)) return "depth-state";
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.hqState, "US", 0, 3, east) || !TB_CC_StrategicPersistence.ReadInt(snapshot.hqState, "USSR", 0, 3, west)) return "HQ-state";
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.pressureState, "active", 0, 1, active) || !TB_CC_StrategicPersistence.ReadInt(snapshot.pressureState, "remaining", -1, 86400, remaining) || !TB_CC_StrategicPersistence.ReadInt(snapshot.pressureState, "duration", 1, 86400, duration) || !TB_CC_StrategicPersistence.ReadInt(snapshot.pressureState, "starts", 0, 2147483647, starts)) return "pressure-state";
		// Duration is a bounded saved hold, not necessarily today's default.
		int fiaCooldown; int fiaOperationSerial; int ussrCooldown;
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-cooldown", 0, 86400, fiaCooldown) || !TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-operationSerial", 0, 2147483647, fiaOperationSerial) || !TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "USSR-cooldown", 0, 86400, ussrCooldown)) return "strategic-admission-state";
		if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-strength", 0, 1000, strength) || !TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-lossSerial", 0, 2147483647, serial)) return "faction-state";
		bool pressure = outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE || outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE;
		if ((active == 1) != pressure || (active == 0 && remaining != -1) || (active == 1 && (remaining < 0 || remaining > duration || starts < 1))) return "pressure-inconsistent";
		int us; int ussr; array<int> queueOrders = {};
		for (int i; i < m_aIds.Count(); i++)
		{
			string id = m_aIds[i];
			if (snapshot.strategicPositions[i] != id || !snapshot.positionOwners.Contains(id)) return "stable-ID-set";
			string owner = snapshot.positionOwners.Get(id);
			if (owner != "US" && owner != "USSR" && owner != "FIA") return "unsupported-owner";
			SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(owner));
			SCR_CampaignMilitaryBaseComponent base = GetBase(id);
			if (!faction) return "native-faction-unavailable:" + id;
			if (!base || !base.GetOwner() || !base.IsInitialized()) return "native-base-unavailable:" + id;
			if (forRestore && base.IsBeingCaptured()) return "restore-base-capture-in-progress:" + id;
			RplComponent replication = RplComponent.Cast(base.GetOwner().FindComponent(RplComponent));
			if (!replication || replication.IsProxy()) return "native-authority-unavailable:" + id;
			if (!base.GetOwner().FindComponent(SCR_FactionAffiliationComponent)) return "native-affiliation-unavailable:" + id;
			int role = TB_CC_M4XP_CampaignRoles.GetRole(id);
			if (base.IsHQ() != (role != TB_CC_StrategicRole.ORDINARY)) return "native-role";
			if (role == TB_CC_StrategicRole.HQ_US && ((east == TB_CC_HQLockState.CAPTURED && owner != "USSR") || (east != TB_CC_HQLockState.CAPTURED && owner != "US"))) return "US-HQ-owner";
			if (role == TB_CC_StrategicRole.HQ_USSR && ((west == TB_CC_HQLockState.CAPTURED && owner != "US") || (west != TB_CC_HQLockState.CAPTURED && owner != "USSR"))) return "USSR-HQ-owner";
			if (role == TB_CC_StrategicRole.ORDINARY) { if (owner == "US") us++; if (owner == "USSR") ussr++; }
			int former; int loss; int epoch; int pending; int attempts; int revision; int failures;
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-former:" + id, 0, 1, former)) return "FIA-former-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-loss:" + id, 0, serial, loss)) return "FIA-loss-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-epoch:" + id, 0, 2147483647, epoch)) return "FIA-epoch-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-pending:" + id, 0, 1, pending)) return "FIA-pending-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-attempts:" + id, 0, 2147483647, attempts)) return "FIA-attempt-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-revision:" + id, 0, snapshot.campaignRevision, revision)) return "FIA-revision-state";
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "USSR-failures:" + id, 0, 2147483647, failures)) return "USSR-failure-state";
			int queueOrder;
			if (!TB_CC_StrategicPersistence.ReadInt(snapshot.factionState, "FIA-queueOrder:" + id, -1, m_aIds.Count() - 1, queueOrder)) return "FIA-queue-order";
			if ((pending == 0 && queueOrder != -1) || (pending == 1 && (queueOrder < 0 || queueOrders.Contains(queueOrder)))) return "FIA-queue-order-inconsistent";
			if (pending == 1) queueOrders.Insert(queueOrder);
			if (pending == 1 && (role != TB_CC_StrategicRole.ORDINARY || owner == "FIA" || epoch < 1 || former != 1)) return "FIA-pending-state";
		}
		for (int queueIndex; queueIndex < queueOrders.Count(); queueIndex++) if (!queueOrders.Contains(queueIndex)) return "FIA-queue-order-gap";
		if (east == TB_CC_HQLockState.CAPTURED && outcome != TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY) return "US-HQ-terminal";
		if (west == TB_CC_HQLockState.CAPTURED && outcome != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY) return "USSR-HQ-terminal";
		if (outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY && west != TB_CC_HQLockState.CAPTURED) return "BLUFOR-terminal";
		if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY && east != TB_CC_HQLockState.CAPTURED) return "OPFOR-terminal";
		if (outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE && (us != 10 || west != TB_CC_HQLockState.PRESSURED)) return "BLUFOR-pressure-territory";
		if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE && (ussr != 10 || east != TB_CC_HQLockState.PRESSURED)) return "OPFOR-pressure-territory";
		if ((east == TB_CC_HQLockState.PRESSURED && outcome != TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) || (west == TB_CC_HQLockState.PRESSURED && outcome != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE)) return "stale-HQ-pressure";
		return "";
	}
	protected bool RestoreStrategicSnapshot(TB_CC_CampaignContainer snapshot)
	{
		string error = ValidateStrategicSnapshot(snapshot);
		if (!error.IsEmpty()) { m_RestoreTransaction.Fail(error); return false; }
		foreach (string id : m_aIds)
		{
			SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(snapshot.positionOwners.Get(id)));
			if (!TB_CC_QuietOwnershipRestore.Apply(GetBase(id), faction, m_RestoreTransaction)) { m_RestoreTransaction.Fail("quiet-owner-" + id); return false; }
		}
		m_iRevision = snapshot.campaignRevision;
		m_iOutcome = snapshot.campaignState.Get("outcome").ToInt(); m_bBLUFORAdvanced = snapshot.campaignState.Get("foothold") == "1";
		m_iDepthPermille = snapshot.campaignState.Get("USDepth").ToInt(); m_iUSSRDepthPermille = snapshot.campaignState.Get("USSRDepth").ToInt();
		m_iHQEastState = snapshot.hqState.Get("US").ToInt(); m_iHQWestState = snapshot.hqState.Get("USSR").ToInt();
		m_bPressureTimerActive = snapshot.pressureState.Get("active") == "1"; m_iPressureStarts = snapshot.pressureState.Get("starts").ToInt();
		m_iRestoredPressureDuration = snapshot.pressureState.Get("duration").ToInt();
		m_iFIAStrategicCooldown = snapshot.factionState.Get("FIA-cooldown").ToInt(); m_iFIAStrategicSerial = snapshot.factionState.Get("FIA-operationSerial").ToInt();
		m_bFIAAdmissionAwaitingImport = true;
		m_StrategicFailureState.Set("USSR-cooldown", snapshot.factionState.Get("USSR-cooldown"));
		m_iFIAConventionalStrength = snapshot.factionState.Get("FIA-strength").ToInt(); m_iFIALossSerial = snapshot.factionState.Get("FIA-lossSerial").ToInt();
		m_mFormerFIATerritory.Clear(); m_mFIALossOrder.Clear();
		foreach (string stateId : m_aIds)
		{
			if (snapshot.factionState.Get("FIA-former:" + stateId) == "1") m_mFormerFIATerritory.Set(stateId, true);
			m_mFIALossOrder.Set(stateId, snapshot.factionState.Get("FIA-loss:" + stateId).ToInt());
			m_StrategicFailureState.Set("USSR-failures:" + stateId, snapshot.factionState.Get("USSR-failures:" + stateId));
		}
		m_bFailureStateAwaitingImport = true;
		m_FIARecaptureLedger.RestoreStrategicState(snapshot.factionState, m_aIds, snapshot.positionOwners);
		m_bVanillaEndSignalled = IsTerminal(); // Do not replay a historical end-match bridge.
		m_bRestoredTerminal = IsTerminal();
		return true;
	}
	protected void ObservePersistenceState()
	{
		if (!PersistenceAllowsCampaignConsumers() || !m_Persistence) return;
		TB_CC_CampaignContainer observation = new TB_CC_CampaignContainer();
		if (!CaptureStrategicSnapshot(observation)) return;
		if (!m_LastPersistenceObservation || observation.campaignRevision != m_LastPersistenceObservation.campaignRevision || !TB_CC_StrategicPersistence.SamePayload(observation, m_LastPersistenceObservation)) m_Persistence.MarkDirty("strategic-state-or-clock");
		m_LastPersistenceObservation = observation;
	}
	protected void PublishRestoredStrategicState()
	{
		TB_CC_CampaignContainer snapshot = m_Persistence.GetLoadedContainer();
		string committed = m_USObjectiveCommitment.GetCurrentId();
		// Prime the existing semantic publication latch, preserving saved revision.
		m_sSemanticSignature = m_iOutcome.ToString() + "|" + GetBLUFORLegalObjective();
		m_sSemanticSignature += "|" + GetOPFORLegalObjective();
		m_sSemanticSignature += "|" + CandidateCsv();
		m_sSemanticSignature += "|" + StateCsv();
		m_sSemanticSignature += "|" + m_iDepthPermille.ToString();
		m_sSemanticSignature += "|" + m_iPressureStarts.ToString();
		m_sSemanticSignature += AdditionalSemanticSignature();
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.GetInstance();
		if (mode) mode.TB_CC_MarkRestoredPresentation(true, m_iHQEastState, m_iHQWestState);
		if (m_bPressureTimerActive)
		{
			int remaining = snapshot.pressureState.Get("remaining").ToInt();
			GetGame().GetCallqueue().CallLater(CheckVictoryPressure, remaining * 1000, false);
			PrintFormat("[TB_CC_PERSIST] action=PRESSURE_RESUMED remaining=%1 starts=%2 offlineTimeConsumed=0 announcement=0", remaining, m_iPressureStarts);
		}
		ConsumeProjection("strategic-restore"); PublishAdditionalProjection();
		PrintFormat("[TB_CC_PERSIST] action=STRATEGIC_RESTORED revision=%1 objective=%2 outcome=%3 historicalCaptureCallbacks=0", m_iRevision, committed, m_iOutcome);
	}
	override protected int GetValidity()
	{
		if (m_bRestoredTerminal && m_Registry && m_Registry.IsReady()) return TB_PVE_M2_2_CampaignValidity.VALID_GRAPH;
		return super.GetValidity();
	}
	override protected int GetProjectedState(string id)
	{
		if (m_bRestoredTerminal)
		{
			if (GetTerritorialOwner(id) == "US") return TB_PVE_M2_3_ObjectiveStrategicState.FRIENDLY_REAR;
			return TB_PVE_M2_3_ObjectiveStrategicState.ENEMY_REAR_LOCKED;
		}
		return super.GetProjectedState(id);
	}
	override protected void EnterVictoryPressure(int pressureOutcome, int durationSeconds, string reason)
	{
		if (m_iOutcome != pressureOutcome || !m_bPressureTimerActive)
		{
			m_iRestoredPressureDuration = 0;
			SCR_GameModeCampaign mode = SCR_GameModeCampaign.GetInstance();
			if (mode) mode.TB_CC_MarkRestoredPresentation(false);
		}
		super.EnterVictoryPressure(pressureOutcome, durationSeconds, reason);
	}
	TB_CC_PersistenceService GetPersistenceFoundation() { return m_Persistence; }
	bool PersistenceAllowsCampaignConsumers()
	{
		if (!Replication.IsServer())
		{
			return true; // Clients consume existing authoritative projections only.
		}
		if (!m_RestoreTransaction)
		{
			return false;
		}
		return m_RestoreTransaction.IsReadyForCampaignConsumers();
	}
	protected bool CompletePersistenceDerivedRebuild()
	{
		if (!m_RestoreTransaction || !m_RestoreTransaction.BeginDerivedRebuild()) return false;
		if (m_Persistence.HasStrategicSnapshot())
		{
			foreach (string id : m_aIds)
				if (!GetBase(id).TB_CC_ReconcileRestoredOwner(m_RestoreTransaction)) { m_RestoreTransaction.Fail("native-reconciliation-" + id); return false; }
			if (!SCR_RadioCoverageSystem.GetInstance()) { m_RestoreTransaction.Fail("radio-service-missing"); return false; }
			SCR_RadioCoverageSystem.UpdateAll(true);
			// A captured HQ invalidates the evaluator's live origin-owner premise.
			// Terminal/INVALID restores never invoke that normal campaign selector.
			if (!IsTerminal() && m_iOutcome != TB_PVE_M2_5_CampaignOutcome.INVALID)
			{
				TB_CC_M4_3_VanillaOwnershipView owners = new TB_CC_M4_3_VanillaOwnershipView(m_Registry);
				TB_CC_M4_2A_CampaignStartState opforStart = new TB_CC_M4_2A_CampaignStartState(m_iCampaignSeed, m_iStartMode, GetSelectedEndpointId(), GetSelectedMobId(), 10);
				if (!m_Evaluator.Evaluate(m_Registry, m_Start, owners, "", CreateRadioEligibilityView()) || !m_USSREvaluator.Evaluate(m_Registry, opforStart, owners)) { m_RestoreTransaction.Fail("derived-evaluation"); return false; }
				m_iDepthPermille = m_Evaluator.GetConnectedOrdinaryPositionCount() * 1000 / 10;
				m_iUSSRDepthPermille = m_USSREvaluator.GetConnectedOrdinaryPositionCount() * 1000 / 10;
			}
			TB_CC_CampaignContainer snapshot = m_Persistence.GetLoadedContainer();
			foreach (string ownerId : m_aIds)
				if (GetTerritorialOwner(ownerId) != snapshot.positionOwners.Get(ownerId)) { m_RestoreTransaction.Fail("reconciled-owner-readback"); return false; }
			string committed = snapshot.objectiveState.Get("committedUS");
			if (IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) committed = "";
			else if (committed.IsEmpty() || !TB_CC_ObjectiveCommitment.StructuralInvalidation(m_Registry, committed, GetSelectedMobId()).IsEmpty() || GetTerritorialOwner(committed) == "US")
			{
				committed = m_Evaluator.GetBLUFORLegalObjective();
				if (m_iHQWestState == TB_CC_HQLockState.VULNERABLE)
				{
					string radioReason; TB_CC_M4_3_RadioEligibilityView radio = CreateRadioEligibilityView();
					committed = "";
					if (radio && radio.IsReachable(GetSelectedEndpointId(), radioReason)) committed = GetSelectedEndpointId();
				}
			}
			m_USObjectiveCommitment.RestoreQuiet(committed, m_iRevision);
			if (m_bPressureTimerActive)
			{
				int remaining = snapshot.pressureState.Get("remaining").ToInt();
				m_PressureStarted = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetServerTimestamp();
				m_PressureDeadline = m_PressureStarted.PlusSeconds(remaining);
			}
		}
		return m_RestoreTransaction.CompleteDerivedRebuild(m_Registry && m_Registry.IsReady());
	}
	[Attribute(defvalue: "750", category: "BCM War Representation")] protected float m_fWarPhysicalRadius = 750;
	[Attribute(defvalue: "1000", category: "BCM War Representation")] protected float m_fWarPreRadius = 1000;
	[Attribute(defvalue: "1100", category: "BCM War Representation")] protected float m_fWarVirtualRadius = 1100;
	[Attribute(defvalue: "10", category: "BCM War Representation")] protected int m_iWarGraceSeconds = 10;
	[Attribute(defvalue: "10", category: "BCM War Representation")] protected int m_iWarDwellSeconds = 10;
	bool WarFootRouteAllowed(string source, string target)
	{
		if (!m_Registry) return false;
		TB_CC_M4_1_ResolvedPosition from = m_Registry.FindPositionById(source);
		TB_CC_M4_1_ResolvedPosition to = m_Registry.FindPositionById(target);
		if (!from || !to) return false;
		if (from.GetDefinition().HasTrait("TRANSPORT_REQUIRED") || to.GetDefinition().HasTrait("TRANSPORT_REQUIRED")) return false;
		FactionKey faction = GetTerritorialOwner(source);
		array<string> open = {source}; array<string> visited = {source};
		for (int cursor; cursor < open.Count(); cursor++)
		{
			array<string> neighbours = {}; GetTerritorialNeighbours(open[cursor], neighbours);
			foreach (string id : neighbours)
			{
				if (visited.Contains(id)) continue;
				visited.Insert(id);
				TB_CC_M4_1_ResolvedPosition next = m_Registry.FindPositionById(id);
				if (!next || next.GetDefinition().HasTrait("TRANSPORT_REQUIRED")) continue;
				if (id == target) return true;
				if (GetTerritorialOwner(id) == faction) open.Insert(id);
			}
		}
		return source == target;
	}
	bool WarSource(FactionKey faction, string target, out string source, out vector position)
	{
		vector targetPosition; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(target, targetPosition)) return false;
		array<string> neighbours = {}; GetTerritorialNeighbours(target, neighbours);
		float nearest = float.MAX;
		// Prefer a friendly graph neighbour; a retained withdrawal may use the
		// nearest reachable friendly registry anchor if no neighbour survives.
		for (int pass; pass < 2; pass++)
		{
			array<string> candidates = neighbours; if (pass == 1) candidates = m_aIds;
			foreach (string id : candidates)
			{
				if (id == target || GetTerritorialOwner(id) != faction || !WarFootRouteAllowed(id, target)) continue;
				vector candidate; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, candidate)) continue;
				float distance = vector.Distance(candidate, targetPosition);
				if (distance < nearest) { nearest = distance; source = id; position = candidate; }
			}
			if (!source.IsEmpty()) return true;
		}
		return false;
	}
	string WarNearestOwnedId(FactionKey faction, vector position)
	{
		string local = GetNearestBattleArea(position);
		TB_CC_M4_1_ResolvedPosition origin; if (m_Registry) origin = m_Registry.FindPositionById(local);
		if (origin && origin.GetDefinition().HasTrait("TRANSPORT_REQUIRED")) return local;
		string selected; float nearest = float.MAX;
		foreach (string id : m_aIds)
		{
			if (GetTerritorialOwner(id) != faction) continue;
			if (origin && !WarFootRouteAllowed(id, local)) continue;
			vector candidate; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, candidate)) continue;
			float distance = vector.Distance(position, candidate);
			if (distance < nearest) { nearest = distance; selected = id; }
		}
		if (selected.IsEmpty() && origin) return local;
		return selected;
	}
	protected ref TB_CC_M4_3_StrategicEvaluator m_USSREvaluator;
	protected int m_iHQEastState = TB_CC_HQLockState.PROTECTED;
	protected int m_iHQWestState = TB_CC_HQLockState.PROTECTED;
	protected int m_iUSSRDepthPermille;
	protected ref ScriptInvoker m_OnHQVulnerable = new ScriptInvoker();
	[Attribute(defvalue: "FIA_LIGHT", category: "BCM FIA")] protected string m_sFIALightPackage;
	[Attribute(defvalue: "FIA_STANDARD", category: "BCM FIA")] protected string m_sFIAStandardPackage;
	[Attribute(defvalue: "FIA_REINFORCED", category: "BCM FIA")] protected string m_sFIAReinforcedPackage;
	[Attribute(defvalue: "FIA_STRONGPOINT", category: "BCM FIA")] protected string m_sFIAStrongpointPackage;
	[Attribute(defvalue: "6", category: "BCM FIA")] protected int m_iMaximumGarrisonGroups;
	[Attribute(defvalue: "1000", category: "BCM FIA")] protected int m_iFIAConventionalStrength;
	[Attribute(defvalue: "2", category: "BCM FIA")] protected int m_iFIALightGroups;
	[Attribute(defvalue: "4", category: "BCM FIA")] protected int m_iFIARifleGroups;
	[Attribute(defvalue: "1", category: "BCM FIA")] protected int m_iFIAMGGroups;
	[Attribute(defvalue: "1", category: "BCM FIA")] protected int m_iFIAATGroups;
	// Three native USSR infantry slots per group: defaults 6/12/18/24 units.
	[Attribute(defvalue: "2", category: "BCM USSR Offensive Strength")] protected int m_iUSSRLightGroups;
	[Attribute(defvalue: "4", category: "BCM USSR Offensive Strength")] protected int m_iUSSRStandardGroups;
	[Attribute(defvalue: "6", category: "BCM USSR Offensive Strength")] protected int m_iUSSRReinforcedGroups;
	[Attribute(defvalue: "8", category: "BCM USSR Offensive Strength")] protected int m_iUSSRHeavyGroups;
	protected ref TB_PVE_M3_6_OPFORFrontlineGarrisonManager m_USSRFrontGarrison;
	[Attribute(defvalue: "40", category: "BCM Territorial Scoring")] protected int m_iTerritoryImportanceWeight;
	[Attribute(defvalue: "1000", category: "BCM Territorial Scoring")] protected float m_fUSObjectiveBandMeters = 1000;
	[Attribute(defvalue: "30", category: "BCM Territorial Scoring")] protected int m_iTerritoryProximityWeight;
	[Attribute(defvalue: "20", category: "BCM Territorial Scoring")] protected int m_iTerritoryConsolidationWeight;
	[Attribute(defvalue: "10", category: "BCM Territorial Scoring")] protected int m_iTerritoryConnectivityWeight;
	[Attribute(defvalue: "1500", category: "BCM Territorial Scoring")] protected float m_fTerritoryProximityScale;
	[Attribute(defvalue: "2", category: "BCM Territorial Scoring")] protected int m_iTerritoryVariation;
	[Attribute(category: "BCM Territorial Scoring", desc: "Optional stable-ID importance overrides; empty retains authored map metadata.")]
	protected ref array<ref TB_CC_TerritoryImportanceOverride> m_aPositionImportanceOverrides;
	[Attribute(defvalue: "60", category: "BCM Territorial Occupation")] protected int m_iConsolidationSeconds;
	[Attribute(defvalue: "60", category: "BCM FIA")] protected int m_iFIARecaptureCooldownSeconds;
	protected ref TB_CC_TerritorialOccupationService m_Occupation;
	protected ref TB_CC_FIARecaptureCoordinator m_FIARecapture;
	protected ref TB_CC_FIARecaptureLedger m_FIARecaptureLedger = new TB_CC_FIARecaptureLedger();
	protected ref TB_CC_StrategicResponse m_Responses;
	protected ref map<string, int> m_mResponseLossRevision = new map<string, int>();
	[Attribute(defvalue: "1", category: "BCM Strategic Response")] protected bool m_bContactResponseEnabled;
	[Attribute(defvalue: "1", category: "BCM Strategic Response")] protected bool m_bCollapseResponseEnabled;
	[Attribute(defvalue: "5", category: "BCM Strategic Response")] protected int m_iCollapseThreshold;
	[Attribute(defvalue: "2", category: "BCM Strategic Response")] protected int m_iPostCaptureWaves;
	[Attribute(defvalue: "120", category: "BCM Strategic Response")] protected int m_iPostDelayMin;
	[Attribute(defvalue: "240", category: "BCM Strategic Response")] protected int m_iPostDelayMax;
	[Attribute(defvalue: "1", category: "BCM Strategic Response")] protected bool m_bPressureRecoveryEnabled;
	[Attribute(defvalue: "HEAVY", category: "BCM Strategic Response")] protected string m_sPressureRecoveryPackage;
	[Attribute(defvalue: "15", category: "BCM Strategic Response")] protected int m_iPressureRecoveryRetrySeconds;
	protected TB_CC_WarForceLease m_FinalHQDefence;
	protected ref TB_CC_StrategicDefence m_StrategicDefence;
	[Attribute(defvalue: "1", category: "BCM Strategic Defence")] protected bool m_bStrategicDefenceEnabled;
	[Attribute(defvalue: "LIGHT_PREPARED", category: "BCM Strategic Defence")] protected string m_sDefenceProfile;
	[Attribute(defvalue: "3", category: "BCM Strategic Defence")] protected int m_iDefenceDensity;
	[Attribute(defvalue: "48", category: "BCM Strategic Defence")] protected int m_iMaxFortificationObjects;
	TB_CC_StrategicResponse GetResponses() { return m_Responses; }
	bool IsWarOperational() { return PersistenceAllowsCampaignConsumers() && m_bReady && !IsTerminal() && m_iOutcome != TB_PVE_M2_5_CampaignOutcome.INVALID; }
	bool IsSovietEmergency() { return IsPressure() || m_iHQWestState == TB_CC_HQLockState.VULNERABLE; }
	bool HasFIAResponseCapability() { return m_iFIAConventionalStrength > 0; }
	int GetResponseLossRevision(string id) { return m_mResponseLossRevision.Get(id); }
	void GetResponsePositionIds(out array<string> ids) { ids.Clear(); foreach (string id : m_aIds) ids.Insert(id); }
	TB_PVE_M3_1_Operation LaunchResponse(TB_CC_ResponseRequest request, string packageId)
	{
		if (!m_bEnableM3Coordinator || !m_Responses || !m_Responses.IsCurrent(request)) return null;
		if (request.m_sFaction == "USSR" && m_M3Coordinator) return m_M3Coordinator.TryLaunchResponse(request, packageId);
		if (request.m_sFaction == "FIA" && m_FIARecapture) return m_FIARecapture.TryLaunchResponse(request, packageId);
		return null;
	}
	void ObserveResponsePresence(SCR_CampaignMilitaryBaseComponent base, map<SCR_Faction, int> presence, bool humanUS)
	{
		if (!m_Responses || !base || !base.GetCampaignFaction()) return;
		m_Responses.ObserveAssault(GetId(base), humanUS, presence.Get(base.GetCampaignFaction()), System.GetTickCount());
	}
	TB_CC_FIARecaptureLedger GetFIARecaptureLedger() { return m_FIARecaptureLedger; }
	[Attribute(defvalue: "650", category: "BCM Physical War")] protected float m_fCombatSimulationRadius;
	[Attribute(defvalue: "5", category: "BCM Physical War")] protected int m_iActiveGarrisonRetrySeconds;
	[Attribute(defvalue: "3600", category: "BCM Physical War")] protected int m_iCorpsePersistenceSeconds;
	[Attribute(defvalue: "7200", category: "BCM Physical War")] protected int m_iWreckPersistenceSeconds;
	[Attribute(defvalue: "3600", category: "BCM Physical War")] protected int m_iDroppedEquipmentPersistenceSeconds;
	[Attribute(defvalue: "10", category: "BCM Physical War")] protected int m_iMaximumPersistentGarrisons;
	[Attribute(defvalue: "1", category: "BCM Physical War")] protected int m_iMaximumConcurrentUSSROperations;
	[Attribute(defvalue: "1", category: "BCM Physical War")] protected int m_iMaximumConcurrentFIAOperations;
	[Attribute(defvalue: "600", category: "BCM Physical War")] protected int m_iWithdrawalLifetimeSeconds;
	protected ref array<ref TB_PVE_M3_6_OPFORFrontlineGarrisonManager> m_aTerritorialManagers = {};
	float GetCombatSimulationRadius() { return Math.Max(m_fCombatSimulationRadius, 500); }
	int GetActiveGarrisonRetrySeconds() { return Math.Max(m_iActiveGarrisonRetrySeconds, 1); }
	int GetCorpsePersistenceSeconds() { return Math.Max(m_iCorpsePersistenceSeconds, 1800); }
	int GetWreckPersistenceSeconds() { return Math.Max(m_iWreckPersistenceSeconds, 3600); }
	int GetDroppedEquipmentPersistenceSeconds() { return Math.Max(m_iDroppedEquipmentPersistenceSeconds, 1800); }
	int GetMaximumConcurrentUSSROperations() { return Math.Max(m_iMaximumConcurrentUSSROperations, 1); }
	int GetMaximumConcurrentFIAOperations() { return Math.Max(m_iMaximumConcurrentFIAOperations, 1); }
	int GetWithdrawalLifetimeSeconds() { return Math.Max(m_iWithdrawalLifetimeSeconds, 60); }
	string GetNearestBattleArea(vector position)
	{
		string selected = "OPEN_FIELD"; float best = GetCombatSimulationRadius();
		foreach (string id : m_aIds)
		{
			if (TB_CC_M4XP_CampaignRoles.GetRole(id) != TB_CC_StrategicRole.ORDINARY) continue;
			vector centre; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, centre)) continue;
			float distance = vector.Distance(position, centre);
			if (distance < best) { best = distance; selected = id; }
		}
		return selected;
	}
	vector FindWithdrawalDestination(FactionKey faction, vector from, vector fallback)
	{
		// An island defence cannot acquire a mainland foot retreat simply by
		// choosing a nearby valid navmesh tile. Retain/regroup on its island.
		if (GetNearestBattleArea(from) == "ARLAND::GULL_ISLAND") return fallback;
		float best = 1000000; vector selected = fallback;
		AIWorld aiWorld = GetGame().GetAIWorld(); NavmeshWorldComponent navmesh;
		if (aiWorld) navmesh = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (!navmesh) return fallback;
		foreach (string id : m_aIds)
		{
			if (GetTerritorialOwner(id) != faction || id == "ARLAND::GULL_ISLAND") continue;
			vector position; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, position) || !navmesh.IsTileValid(position)) continue;
			float distance = vector.Distance(from, position);
			if (distance < best) { best = distance; selected = position; }
		}
		return selected;
	}
	protected ref map<string, bool> m_mFormerFIATerritory = new map<string, bool>();
	protected ref map<string, int> m_mFIALossOrder = new map<string, int>();
	protected int m_iFIALossSerial;
	TB_CC_TerritorialOccupationService GetOccupationService() { return m_Occupation; }
	override TB_PVE_M3_6_OPFORFrontlineGarrisonManager GetGarrisonManager()
	{
		foreach (TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager : m_aTerritorialManagers)
			if (manager.GetTerritorialTarget() == GetBLUFORLegalObjective()) return manager;
		return null;
	}
	int GetConsolidationSeconds() { return Math.Max(m_iConsolidationSeconds, 10); }
	void GetTerritorialNeighbours(string id, out array<string> neighbours) { if (m_Registry) m_Registry.GetConnections(id, neighbours); }
	bool IsFootTerritory(string id)
	{
		TB_CC_M4_1_ResolvedPosition position;
		if (m_Registry) position = m_Registry.FindPositionById(id);
		return position && TB_CC_M4XP_CampaignRoles.GetRole(id) == TB_CC_StrategicRole.ORDINARY && !position.GetDefinition().HasTrait("TRANSPORT_REQUIRED");
	}
	bool CanFIARecapture(string id)
	{
		if (!Replication.IsServer() || !m_bReady || m_iOutcome != TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_iFIAConventionalStrength <= 0 || !m_mFormerFIATerritory.Contains(id) || !IsFootTerritory(id)) return false;
		FactionKey owner = GetTerritorialOwner(id);
		if (owner != "US" && owner != "USSR") return false;
		array<string> neighbours = {}; GetTerritorialNeighbours(id, neighbours);
		foreach (string neighbour : neighbours) if (IsFootTerritory(neighbour) && GetTerritorialOwner(neighbour) == "FIA") return true;
		return false;
	}
	string SelectFIARecaptureTarget(array<string> excluded = null)
	{
		string selected; int bestLoss = -1; int bestImportance = -1;
		foreach (string id : m_aIds)
		{
			if (excluded && excluded.Contains(id)) continue;
			if (!CanFIARecapture(id)) continue;
			int loss = m_mFIALossOrder.Get(id); int importance = GetPositionImportance(id);
			if (loss > bestLoss || (loss == bestLoss && importance > bestImportance))
			{ selected = id; bestLoss = loss; bestImportance = importance; }
		}
		return selected;
	}
	string SelectFIAPackage(string id)
	{
		int humans = TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US");
		int depth = Math.Max(m_iDepthPermille, m_iUSSRDepthPermille);
		int importance = GetPositionImportance(id);
		string selected = m_sFIALightPackage;
		if (humans >= 2 || importance >= 70) selected = m_sFIAStandardPackage;
		if (humans >= 6 || importance >= 85 || (humans >= 4 && depth >= 334)) selected = m_sFIAReinforcedPackage;
		if (importance >= 95 || (humans >= 6 && depth >= 667 && importance >= 80)) selected = m_sFIAStrongpointPackage;
		if (m_iFIAConventionalStrength < 667 && (selected == m_sFIAReinforcedPackage || selected == m_sFIAStrongpointPackage)) selected = m_sFIAStandardPackage;
		if (m_iFIAConventionalStrength < 334) selected = m_sFIALightPackage;
		return selected;
	}
	protected void TickTerritorialOccupation()
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (m_Occupation) m_Occupation.Tick();
		TB_CC_PhysicalWarService.Tick(this);
		if (m_Responses) m_Responses.Tick(System.GetTickCount());
		if (m_StrategicDefence) m_StrategicDefence.Tick();
		if (m_bEnableM3Garrison && IsWarOperational() && m_iHQWestState == TB_CC_HQLockState.VULNERABLE && !m_FinalHQDefence && TB_CC_WarSimulation.Get())
		{
			m_FinalHQDefence = TB_CC_WarSimulation.Get().Defence(GetSelectedEndpointId(), "USSR", "HEAVY");
			if (m_FinalHQDefence) PrintFormat("[TB_CC_HQ_DEFENCE] hq=%1 state=PREPARED package=HEAVY strength=%2 representation=%3", GetSelectedEndpointId(), m_FinalHQDefence.m_State.m_iStrength, m_FinalHQDefence.m_State.m_iRepresentation);
		}
	}

	override protected void StartM3Consumers()
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		// Baseline reevaluation can call here indirectly before P5's durable receipt.
		if (m_Persistence && m_Persistence.IsResetActivationPending()) return;
		TB_PVE_M3_4_ForcePackageRegistry.ConfigureUSSRPackages(m_iUSSRLightGroups, m_iUSSRStandardGroups, m_iUSSRReinforcedGroups, m_iUSSRHeavyGroups);
		TB_PVE_M3_4_ForcePackageRegistry.ConfigureFIAPackages(m_iFIALightGroups, m_iFIARifleGroups, m_iFIAMGGroups, m_iFIAATGroups);
		TB_CC_WarConfig warConfig = new TB_CC_WarConfig();
		warConfig.m_fPhysicalRadius = m_fWarPhysicalRadius; warConfig.m_fPreRadius = m_fWarPreRadius; warConfig.m_fVirtualRadius = m_fWarVirtualRadius;
		warConfig.m_iGraceMs = m_iWarGraceSeconds * 1000; warConfig.m_iMinimumDwellMs = m_iWarDwellSeconds * 1000;
		TB_CC_WarSimulation.Initialize(new TB_CC_NativeWarAdapter(this), warConfig);
		if (!m_StrategicDefence)
		{
			TB_CC_DefenceConfig defenceConfig = new TB_CC_DefenceConfig();
			defenceConfig.m_bEnabled = m_bStrategicDefenceEnabled;
			defenceConfig.m_sProfile = m_sDefenceProfile; defenceConfig.m_iDensity = m_iDefenceDensity;
			defenceConfig.m_iMaximumObjects = Math.Clamp(m_iMaxFortificationObjects, 0, 64);
			m_StrategicDefence = new TB_CC_StrategicDefence();
			m_StrategicDefence.Initialize(new TB_CC_NativeDefenceAdapter(this), defenceConfig);
			foreach (string defenceId : m_aIds)
			{
				if (defenceId == GetSelectedMobId()) continue;
				TB_CC_DefencePlan plan = new TB_CC_DefencePlan(); plan.m_sId = defenceId;
				if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(defenceId, plan.m_vCentre)) continue;
				plan.m_bHQ = defenceId == GetSelectedEndpointId();
				float nearestApproach = float.MAX;
				foreach (string approachId : m_aIds)
				{
					if (GetTerritorialOwner(approachId) != "US") continue;
					vector approachPosition;
					if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(approachId, approachPosition)) continue;
					float approachDistance = vector.Distance(plan.m_vCentre, approachPosition);
					if (approachDistance < nearestApproach) { nearestApproach = approachDistance; plan.m_vApproach = approachPosition; }
				}
				m_StrategicDefence.Register(plan);
			}
		}
		if (!m_Responses)
		{
			TB_CC_ResponseConfig responseConfig = new TB_CC_ResponseConfig();
			responseConfig.m_bContactResponseEnabled = m_bContactResponseEnabled; responseConfig.m_bCollapseResponseEnabled = m_bCollapseResponseEnabled;
			responseConfig.m_iCollapseDefenderThreshold = Math.Max(m_iCollapseThreshold, 0); responseConfig.m_iPostCaptureWaveCount = Math.Clamp(m_iPostCaptureWaves, 0, 2);
			responseConfig.m_iPostCaptureDelayMinSeconds = m_iPostDelayMin; responseConfig.m_iPostCaptureDelayMaxSeconds = m_iPostDelayMax;
			responseConfig.m_bPressureRecoveryEnabled = m_bPressureRecoveryEnabled; responseConfig.m_sPressureRecoveryPackage = m_sPressureRecoveryPackage;
			responseConfig.m_iPressureRetrySeconds = m_iPressureRecoveryRetrySeconds;
			m_Responses = new TB_CC_StrategicResponse(); m_Responses.Initialize(new TB_CC_NativeResponseAdapter(this), responseConfig, m_iCampaignSeed);
			foreach (string responseId : m_aIds) if (TB_CC_M4XP_CampaignRoles.GetRole(responseId) == TB_CC_StrategicRole.ORDINARY) m_Responses.Seed(responseId, GetTerritorialOwner(responseId));
		}
		super.StartM3Consumers();
		if (m_M3Coordinator && m_bFailureStateAwaitingImport)
		{
			m_M3Coordinator.RestoreStrategicFailures(m_StrategicFailureState, m_aIds);
			m_bFailureStateAwaitingImport = false;
		}
		if (m_bEnableM3Coordinator && !m_FIARecapture && m_Occupation && !IsPressure() && !IsTerminal() && m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE)
		{
			m_FIARecapture = new TB_CC_FIARecaptureCoordinator(); m_FIARecapture.Start(this, m_iFIARecaptureCooldownSeconds);
			if (m_bFIAAdmissionAwaitingImport)
			{
				m_FIARecapture.RestoreStrategicAdmission(m_iFIAStrategicCooldown, m_iFIAStrategicSerial);
				m_bFIAAdmissionAwaitingImport = false;
			}
		}
		// Native territory owns bounded, fixed-position managers, rather than
		// recycling one front manager whenever the selected objective changes.
		if (m_M3Garrison) { m_M3Garrison.Stop("territorial-pool-handoff"); m_M3Garrison = null; }
		if (!m_bEnableM3Garrison || IsPressure() || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return;
		if (!m_aTerritorialManagers.IsEmpty())
		{
			foreach (TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager : m_aTerritorialManagers) manager.SuspendCreation(false);
			return;
		}
		foreach (string id : m_aIds)
		{
			if (TB_CC_M4XP_CampaignRoles.GetRole(id) != TB_CC_StrategicRole.ORDINARY) continue;
			if (m_aTerritorialManagers.Count() >= Math.Max(m_iMaximumPersistentGarrisons, 1)) break;
			TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager = new TB_PVE_M3_6_OPFORFrontlineGarrisonManager();
			manager.Configure("STANDARD", 250, 900, 250, 55, 32);
			manager.ConfigureOwnerAware(m_sFIALightPackage, m_sFIAStandardPackage, m_sFIAReinforcedPackage, m_sFIAStrongpointPackage, m_iMaximumGarrisonGroups, m_iFIAConventionalStrength);
			manager.SetTerritorialTarget(id); manager.Start(); m_aTerritorialManagers.Insert(manager);
		}
		PrintFormat("[TB_CC_PHYSICAL_WAR] configured radius=%1 retrySeconds=%2 corpseSeconds=%3 wreckSeconds=%4 equipmentSeconds=%5 persistentPositions=%6 USSRConcurrency=%7 FIAConcurrency=%8", GetCombatSimulationRadius(), GetActiveGarrisonRetrySeconds(), GetCorpsePersistenceSeconds(), GetWreckPersistenceSeconds(), GetDroppedEquipmentPersistenceSeconds(), m_aTerritorialManagers.Count(), GetMaximumConcurrentUSSROperations(), GetMaximumConcurrentFIAOperations());
	}
	override protected void StopM3Consumers(string reason)
	{
		if (m_M3Coordinator)
		{
			m_M3Coordinator.ExportStrategicFailures(m_StrategicFailureState, m_aIds);
			m_bFailureStateAwaitingImport = true;
		}
		// Pressure changes response policy, not the lifetime of active operations.
		if (reason == "M4.4H-victory-pressure" && IsWarOperational())
		{
			PrintFormat("[TB_CC_PRESSURE] state=PRESSURE_RECOVERY coordinatorRetained=%1 outcome=%2", m_M3Coordinator != null, m_iOutcome);
			return;
		}
		// Preserve FIA leases/queued epochs across temporary victory pressure.
		// Its tick pauses while inactive; terminal/invalid states end the ledger.
		if (m_FIARecapture && (IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID)) { m_FIARecapture.Stop(reason); m_FIARecapture = null; }
		if (m_USSRFrontGarrison) { m_USSRFrontGarrison.Stop(reason); m_USSRFrontGarrison = null; }
		foreach (TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager : m_aTerritorialManagers) manager.SuspendCreation(true);
		super.StopM3Consumers(reason);
	}
	protected ref map<string, FactionKey> m_mReplicatedOwners = new map<string, FactionKey>();

	protected string OwnerCsv()
	{
		string result;
		foreach (string id : m_aIds)
		{
			if (!result.IsEmpty()) result += ",";
			result += GetTerritorialOwner(id);
		}
		return result;
	}
	override protected string AdditionalSemanticSignature()
	{
		string signature = "|" + OwnerCsv();
		signature += "|" + m_iHQEastState.ToString();
		signature += "|" + m_iHQWestState.ToString();
		signature += "|" + m_iUSSRDepthPermille.ToString();
		return signature;
	}
	void ConsumeNativeSnapshot(string snapshot)
	{
		if (Replication.IsServer() || !m_Registry) return;
		array<string> fields = {}; snapshot.Split("|", fields, false);
		if (fields.Count() != 13) return;
		if (fields[0].ToInt() < TB_PVE_M2_3_FrontierQuery.GetRevision()) return;
		array<string> ids = {}; array<string> owners = {};
		fields[9].Split(",", ids, true); fields[12].Split(",", owners, true);
		if (ids.Count() != 12 || owners.Count() != ids.Count()) return;
		m_mReplicatedOwners.Clear();
		for (int i; i < ids.Count(); i++) m_mReplicatedOwners.Set(ids[i], owners[i]);
		m_iHQEastState = fields[7].ToInt(); m_iHQWestState = fields[8].ToInt();
		m_iDepthPermille = fields[5].ToInt(); m_iUSSRDepthPermille = fields[6].ToInt();
		string usObjective = fields[3]; string ussrObjective = fields[4]; string candidates = fields[11];
		if (usObjective == "-") usObjective = "";
		if (ussrObjective == "-") ussrObjective = "";
		if (candidates == "-") candidates = "";
		TB_CC_M4_4_ArlandGraphCampaignComponent.ConsumeReplicated(fields[0].ToInt(), fields[1].ToInt(), fields[2].ToInt(), "ARLAND::HQ_EAST", "ARLAND::HQ_WEST", usObjective, ussrObjective, m_iDepthPermille, SCR_GameModeCampaign.GetInstance().TB_CC_GetPressureRemaining(), fields[9], fields[10], candidates);
	}

	ScriptInvoker GetOnHQVulnerable() { return m_OnHQVulnerable; }
	int GetHQState(string id)
	{
		if (TB_CC_M4XP_CampaignRoles.GetRole(id) == TB_CC_StrategicRole.HQ_US) return m_iHQEastState;
		return m_iHQWestState;
	}
	FactionKey GetTerritorialOwner(string id)
	{
		if (!Replication.IsServer() && m_mReplicatedOwners.Contains(id)) return m_mReplicatedOwners.Get(id);
		FactionKey owner;
		if (m_Registry) m_Registry.TryGetCurrentOwner(id, owner);
		return owner;
	}
	int GetFactionDepth(FactionKey faction)
	{
		if (faction == "US") return m_iDepthPermille;
		if (faction == "USSR") return m_iUSSRDepthPermille;
		return -1;
	}
	int GetPositionImportance(string id)
	{
		TB_CC_M4_1_ResolvedPosition position;
		if (m_Registry) position = m_Registry.FindPositionById(id);
		if (position) return position.GetDefinition().GetStrategicImportance();
		return 0;
	}
	bool AuthorizeCapture(SCR_Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		if (!PersistenceAllowsCampaignConsumers()) return false;
		if (!Replication.IsServer() || !m_bReady || !faction || !base || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return false;
		string id = GetId(base);
		if (id.IsEmpty()) return false;
		FactionKey key = faction.GetFactionKey();
		int role = TB_CC_M4XP_CampaignRoles.GetRole(id);
		if (role == TB_CC_StrategicRole.HQ_US) return key == "USSR" && m_iHQEastState == TB_CC_HQLockState.VULNERABLE;
		if (role == TB_CC_StrategicRole.HQ_USSR) return key == "US" && m_iHQWestState == TB_CC_HQLockState.VULNERABLE;
		if (key == "US") return id == GetBLUFORLegalObjective();
		if (key == "USSR") return id == GetOPFORLegalObjective() || (m_Responses && m_Responses.Authorizes(key, id));
		if (key != "FIA" || GetTerritorialOwner(id) == "FIA") return false;
		array<string> neighbours = {}; m_Registry.GetConnections(id, neighbours);
		foreach (string neighbour : neighbours)
			if (TB_CC_M4XP_CampaignRoles.GetRole(neighbour) == TB_CC_StrategicRole.ORDINARY && GetTerritorialOwner(neighbour) == "FIA") return true;
		return false;
	}

	override string GetBLUFORLegalObjective()
	{
		if (!PersistenceAllowsCampaignConsumers()) return "";
		if (!m_bReady || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return "";
		return m_USObjectiveCommitment.GetCurrentId();
	}

	protected ref TB_CC_ObjectiveCommitment m_USObjectiveCommitment = new TB_CC_ObjectiveCommitment();
	protected ref TB_CC_NativeRadioObserver m_NativeRadioObserver;
	protected string m_sCampaignPathSignature;

	protected void TraceCampaignPath(int completedOrdinary, int required)
	{
		array<string> candidates = {}; m_Evaluator.GetBLUFORCandidateObjectives(candidates);
		TB_CC_M4_3_RadioEligibilityView radio = CreateRadioEligibilityView();
		int reachable;
		foreach (string candidate : candidates)
		{
			string radioReason;
			if (radio && radio.IsReachable(candidate, radioReason)) reachable++;
		}
		string current = GetBLUFORLegalObjective();
		bool waiting = current.IsEmpty() && m_Evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.RADIO_WAIT;
		string signature = string.Format("current=%1 completedOrdinary=%2 remainingOrdinary=%3 frontierCandidates=%4 radioReachable=%5 waitingForRadio=%6", current, completedOrdinary, required - completedOrdinary, candidates.Count(), reachable, waiting);
		if (signature == m_sCampaignPathSignature) return;
		m_sCampaignPathSignature = signature;
		PrintFormat("[TB_CC_CAMPAIGN_PATH] %1", signature);
	}

	protected void OnNativeRadioTopologyChanged()
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (!Replication.IsServer() || !m_bReady || IsTerminal()) return;
		Reevaluate("native-radio-topology-changed");
	}

	protected string GetUSObjectiveRecommendation()
	{
		if (!m_bReady || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return "";
		if (m_iHQWestState == TB_CC_HQLockState.VULNERABLE)
		{
			string radioReason;
			TB_CC_M4_3_RadioEligibilityView radio = CreateRadioEligibilityView();
			if (radio && !radio.IsReachable("ARLAND::HQ_WEST", radioReason)) return "";
			return "ARLAND::HQ_WEST";
		}
		if (m_Evaluator) return m_Evaluator.GetBLUFORLegalObjective();
		return "";
	}

	override protected void PublishIfChanged(string reason)
	{
		if (Replication.IsServer())
		{
			string current = m_USObjectiveCommitment.GetCurrentId();
			string invalidation = TB_CC_ObjectiveCommitment.StructuralInvalidation(m_Registry, current, GetSelectedMobId());
			FactionKey owner;
			if (!current.IsEmpty() && invalidation.IsEmpty()) owner = GetTerritorialOwner(current);
			if (m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) invalidation = "campaign-invalid";
			string recommendation;
			if (m_USObjectiveCommitment.NeedsSelection(owner, IsTerminal(), invalidation))
			{
				recommendation = GetUSObjectiveRecommendation();
				if (!TB_CC_ObjectiveCommitment.StructuralInvalidation(m_Registry, recommendation, GetSelectedMobId()).IsEmpty()) recommendation = "";
			}
			m_USObjectiveCommitment.Resolve(recommendation, owner, IsTerminal(), invalidation);
		}
		super.PublishIfChanged(reason);
		if (Replication.IsServer()) m_USObjectiveCommitment.Trace(m_iRevision);
		if (m_Persistence && PersistenceAllowsCampaignConsumers()) m_Persistence.MarkDirty("strategic-publication");
	}
	override string GetOPFORLegalObjective()
	{
		if (!PersistenceAllowsCampaignConsumers()) return "";
		if (!m_bReady || IsTerminal() || m_iOutcome == TB_PVE_M2_5_CampaignOutcome.INVALID) return "";
		if (m_iHQEastState == TB_CC_HQLockState.VULNERABLE) return "ARLAND::HQ_EAST";
		if (m_iHQWestState == TB_CC_HQLockState.VULNERABLE) return "";
		if (m_USSREvaluator) return m_USSREvaluator.GetBLUFORLegalObjective();
		return "";
	}

	override protected int GetConfiguredPressureDuration(int outcome)
	{
		if (m_bPressureTimerActive && m_iRestoredPressureDuration > 0) return m_iRestoredPressureDuration;
		return Math.Max(m_iPressureDurationSeconds, 1);
	}
	override protected void Reevaluate(string reason)
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (!m_bReady || IsTerminal()) return;
		TB_CC_M4_3_VanillaOwnershipView owners = new TB_CC_M4_3_VanillaOwnershipView(m_Registry);
		TB_CC_M4_2A_CampaignStartState opforStart = new TB_CC_M4_2A_CampaignStartState(m_iCampaignSeed, m_iStartMode, "ARLAND::HQ_WEST", "ARLAND::HQ_EAST", 10);
		bool usValid = m_Evaluator.Evaluate(m_Registry, m_Start, owners, "", CreateRadioEligibilityView());
		bool ussrValid = m_USSREvaluator.Evaluate(m_Registry, opforStart, owners);
		int usCount; int ussrCount; int required;
		if (!usValid || !ussrValid || !CountRequiredTerritory(usCount, ussrCount, required) || required != 10)
		{
			FailClosed("three-faction-registry-or-ownership"); PublishIfChanged(reason); return;
		}
		foreach (string id : m_aIds)
		{
			FactionKey owner = GetTerritorialOwner(id);
			if (owner != "US" && owner != "USSR" && owner != "FIA") { FailClosed("unsupported-owner-" + id); PublishIfChanged(reason); return; }
		}
		m_iDepthPermille = (m_Evaluator.GetConnectedOrdinaryPositionCount() * 1000) / required;
		m_iUSSRDepthPermille = (m_USSREvaluator.GetConnectedOrdinaryPositionCount() * 1000) / required;
		GetGame().GetCallqueue().Remove(RetryRadioObjective);
		if (m_Evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.RADIO_WAIT || (m_iHQWestState == TB_CC_HQLockState.VULNERABLE && GetBLUFORLegalObjective().IsEmpty()))
			GetGame().GetCallqueue().CallLater(RetryRadioObjective, 10000);
		if (usCount == 10 && m_iHQWestState != TB_CC_HQLockState.VULNERABLE)
		{
			if (m_iHQEastState == TB_CC_HQLockState.PRESSURED) m_iHQEastState = TB_CC_HQLockState.PROTECTED;
			m_iHQWestState = TB_CC_HQLockState.PRESSURED;
			EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE, GetConfiguredPressureDuration(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE), reason);
		}
		else if (ussrCount == 10 && m_iHQEastState != TB_CC_HQLockState.VULNERABLE)
		{
			if (m_iHQWestState == TB_CC_HQLockState.PRESSURED) m_iHQWestState = TB_CC_HQLockState.PROTECTED;
			m_iHQEastState = TB_CC_HQLockState.PRESSURED;
			EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE, GetConfiguredPressureDuration(TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE), reason);
		}
		else
		{
			CancelVictoryPressure("dominance-broken-or-unlock-earned");
			if (m_iHQEastState == TB_CC_HQLockState.PRESSURED) m_iHQEastState = TB_CC_HQLockState.PROTECTED;
			if (m_iHQWestState == TB_CC_HQLockState.PRESSURED) m_iHQWestState = TB_CC_HQLockState.PROTECTED;
			m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
			StartM3Consumers();
		}
		PublishIfChanged(reason);
		if (m_NativeRadioObserver) m_NativeRadioObserver.TraceWaiting(m_Evaluator, GetBLUFORLegalObjective());
		TraceCampaignPath(usCount, required);
		PrintFormat("[TB_CC_CAMPAIGN] revision=%1 ordinaryUS=%2 ordinaryUSSR=%3 ordinaryFIA=%4 USDepth=%5 USSRDepth=%6 HQEast=%7 HQWest=%8", m_iRevision, usCount, ussrCount, 10 - usCount - ussrCount, m_iDepthPermille, m_iUSSRDepthPermille, m_iHQEastState, m_iHQWestState);
	}

	override protected void CheckVictoryPressure()
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (!Replication.IsServer() || !m_bPressureTimerActive || !IsPressure() || IsTerminal()) return;
		int usCount; int ussrCount; int required;
		if (!CountRequiredTerritory(usCount, ussrCount, required)) { FailClosed("pressure-owner-unresolved"); PublishIfChanged("pressure-invalid"); return; }
		bool blufor = m_iOutcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE;
		if ((blufor && usCount != 10) || (!blufor && ussrCount != 10)) { Reevaluate("pressure-expiry-condition-broken"); return; }
		int completedDuration = GetConfiguredPressureDuration(m_iOutcome);
		CancelVictoryPressure("HQ-unlocked");
		string hq = "ARLAND::HQ_EAST";
		if (blufor) { m_iHQWestState = TB_CC_HQLockState.VULNERABLE; hq = "ARLAND::HQ_WEST"; }
		else m_iHQEastState = TB_CC_HQLockState.VULNERABLE;
		m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		PrintFormat("[TB_CC_HQ] hq=%1 state=VULNERABLE permanent=1 victory=0", hq);
		PrintFormat("[TB_CC_PRESSURE] state=COMPLETED duration=%1 hq=%2 newState=VULNERABLE", completedDuration, hq);
		m_OnHQVulnerable.Invoke(hq);
		Reevaluate("HQ-unlocked");
	}

	override protected void OnBaseFactionChanged(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (!m_bReady || IsTerminal()) return;
		string id = GetId(base);
		if (id.IsEmpty()) return;
		if (m_NativeRadioObserver) m_NativeRadioObserver.ObserveOwnershipChange(base, previousFaction, newFaction);
		if (m_M3Coordinator && previousFaction != newFaction) m_M3Coordinator.ResetTargetFailureHistory(id);
		int role = TB_CC_M4XP_CampaignRoles.GetRole(id);
		if (role != TB_CC_StrategicRole.ORDINARY)
		{
			FactionKey winner = "USSR"; int outcome = TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
			if (role == TB_CC_StrategicRole.HQ_USSR) { winner = "US"; outcome = TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY; }
			if (!newFaction || newFaction.GetFactionKey() != winner || GetHQState(id) != TB_CC_HQLockState.VULNERABLE) { FailClosed("unauthorized-HQ-ownership-" + id); PublishIfChanged("HQ-invalid"); return; }
			if (role == TB_CC_StrategicRole.HQ_US) m_iHQEastState = TB_CC_HQLockState.CAPTURED;
			else m_iHQWestState = TB_CC_HQLockState.CAPTURED;
			m_iOutcome = outcome;
			CancelVictoryPressure("terminal-HQ-capture"); StopM3Consumers("terminal-HQ-capture"); ApplyDeploymentPolicy();
			PublishIfChanged("native-HQ-capture"); return;
		}
		// Reserve the captured base in the same ownership callback, before a
		// frontline tick can create a duplicate replacement during consolidation.
		if (m_Occupation && newFaction)
		{
			TB_PVE_M3_1_Operation opforOperation;
			if (m_M3Coordinator) opforOperation = m_M3Coordinator.GetActiveOperation();
			if (newFaction.GetFactionKey() == "USSR" && opforOperation && !opforOperation.IsTerminal() && opforOperation.GetTargetObjectiveId() == id) m_Occupation.ReserveConsolidation(id, "USSR");
			if (newFaction.GetFactionKey() == "FIA" && m_FIARecapture && m_FIARecapture.IsAttacking(id)) m_Occupation.ReserveConsolidation(id, "FIA");
		}
		if (previousFaction && previousFaction.GetFactionKey() == "FIA" && newFaction && newFaction.GetFactionKey() != "FIA")
		{
			m_iFIALossSerial++; m_mFIALossOrder.Set(id, m_iFIALossSerial);
		}
		if (newFaction)
		{
			m_FIARecaptureLedger.Observe(id, newFaction.GetFactionKey(), m_iRevision + 1);
			if (m_Responses)
			{
				m_Responses.ObserveOwner(id, newFaction.GetFactionKey(), System.GetTickCount());
				if (newFaction.GetFactionKey() == "US")
				{
					m_mResponseLossRevision.Set(id, m_iRevision + 1);
					m_FIARecaptureLedger.CancelTarget(id, "phased-response-owns-US-capture");
				}
			}
		}
		super.OnBaseFactionChanged(base, previousFaction, newFaction);
	}
	protected SCR_GameModeCampaign m_NativeGameMode;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer()) return;
		// Readiness is native lifecycle-driven, not an arbitrary fifteen seconds.
		GetGame().GetCallqueue().Remove(InitializeAuthority);
		m_NativeGameMode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!m_NativeGameMode) { FailClosed("native-game-mode-unavailable"); return; }
		m_NativeGameMode.GetOnStarted().Insert(OnNativeCampaignStarted);
		if (m_NativeGameMode.HasStarted()) OnNativeCampaignStarted();
	}

	protected void OnNativeCampaignStarted()
	{
		// Defer one queue turn until all native OnStarted subscribers finish.
		GetGame().GetCallqueue().CallLater(InitializeAuthority, 0);
	}

	protected bool UsesNativeTaskService()
	{
		// Keep the proven former subclass bridge until this stock path is tested.
		return TB_CC_M4XP_GameMode.Cast(GetGame().GetGameMode()) == null;
	}

	bool AllowsNativeSeizeTask(Faction faction, SCR_CampaignMilitaryBaseComponent base)
	{
		if (!m_bReady || !UsesNativeTaskService() || !faction || !base) return false;
		string legal = GetBLUFORLegalObjective();
		return faction.GetFactionKey() == "US" && !legal.IsEmpty() && GetBase(legal) == base;
	}

	protected string GetPresentedTaskId(string legal, int revision)
	{
		if (!UsesNativeTaskService()) return "TB_CC_US_Seize_" + revision.ToString();
		SCR_CampaignMilitaryBaseComponent base = GetBase(legal);
		if (!base) return "";
		return string.Format("US_SeizeCampaignMilitaryBase_%1", base.GetCallsign());
	}
	protected ref TB_CC_M4_3_RadioEligibilityView m_CorrectiveProofRadioView;
	override protected TB_CC_M4_3_RadioEligibilityView CreateRadioEligibilityView()
	{
		if (m_CorrectiveProofRadioView) return m_CorrectiveProofRadioView;
		return new TB_CC_M4XP_VanillaRadioEligibilityView(m_Registry);
	}
	bool SetCorrectiveProofRadioView(TB_CC_M4_3_RadioEligibilityView view)
	{
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated || !GetGame().GetWorld().FindEntityByName("TB_CC_M4XP_CORRECTIVE_PROOF")) return false;
		m_CorrectiveProofRadioView = view;
		PrintFormat("[TB_CC_M4XP_CORRECTIVE] radio-proof-view active=%1 ownershipMutation=0", view != null);
		Reevaluate("corrective-proof-radio-view");
		return true;
	}
	bool CorrectiveProofTaskMatchesLegal()
	{
		string legal = GetBLUFORLegalObjective();
		if (legal.IsEmpty()) return m_CurrentObjectiveTask == null && m_sPresentedObjectiveId.IsEmpty();
		if (!m_CurrentObjectiveTask || m_sPresentedObjectiveId != legal) return false;
		return SCR_TaskSystem.GetTaskFromTaskID(GetPresentedTaskId(legal, m_iPresentedRevision), false) == m_CurrentObjectiveTask;
	}
	// The inherited Conflict task generator is deliberately not authoritative for
	// this graph: it would present every radio-reachable enemy base. One vanilla
	// seize task is instead owned by the M4 legal-objective projection.
	protected static const ResourceName SEIZE_TASK_PREFAB = "{1CE0D63C852E747B}Prefabs/Tasks/SeizeCampaignMilitaryBaseTaskEntity.et";
	protected SCR_CampaignMilitaryBaseTaskEntity m_CurrentObjectiveTask;
	protected string m_sPresentedObjectiveId;
	protected string m_sPresentedTaskId;
	protected int m_iPresentedRevision;
	protected string m_sLastBridgePending;
	protected string m_sLastClientTaskTrace;

	override protected void PublishAdditionalProjection()
	{
		if (Replication.IsServer() && m_Registry)
		{
			string usObjective = GetBLUFORLegalObjective(); string ussrObjective = GetOPFORLegalObjective(); string candidates = CandidateCsv();
			if (usObjective.IsEmpty()) usObjective = "-";
			if (ussrObjective.IsEmpty()) ussrObjective = "-";
			if (candidates.IsEmpty()) candidates = "-";
			// Same thirteen-field protocol; keep each expression below Enforce's
			// formula-complexity limit. Parser indices are in ConsumeNativeSnapshot.
			string packet = m_iRevision.ToString();
			packet += "|" + GetValidity().ToString();
			packet += "|" + m_iOutcome.ToString();
			packet += "|" + usObjective;
			packet += "|" + ussrObjective;
			packet += "|" + m_iDepthPermille.ToString();
			packet += "|" + m_iUSSRDepthPermille.ToString();
			packet += "|" + m_iHQEastState.ToString();
			packet += "|" + m_iHQWestState.ToString();
			packet += "|" + IdCsv();
			packet += "|" + StateCsv();
			packet += "|" + candidates;
			packet += "|" + OwnerCsv();
			SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
			if (mode)
			{
				mode.TB_CC_SetNativePressureDeadline(m_PressureDeadline);
				mode.TB_CC_PublishNativeSnapshot(packet);
			}
		}
		SyncObjectiveTask();
	}

	protected void SyncObjectiveTask()
	{
		if (!PersistenceAllowsCampaignConsumers()) return;
		if (!Replication.IsServer() || !m_bReady) return;
		// Existing bridge cadence also detects removal/completion if no unrelated
		// ownership event arrives. This checks lifecycle, never recommendation score.
		string committed = m_USObjectiveCommitment.GetCurrentId();
		if (!committed.IsEmpty() && !IsTerminal() && m_iOutcome != TB_PVE_M2_5_CampaignOutcome.INVALID)
		{
			string invalidation = TB_CC_ObjectiveCommitment.StructuralInvalidation(m_Registry, committed, GetSelectedMobId());
			bool completed;
			if (invalidation.IsEmpty()) completed = GetTerritorialOwner(committed) == "US";
			if (!invalidation.IsEmpty() || completed) Reevaluate("objective-lifecycle-ended");
		}
		string legalId = GetBLUFORLegalObjective();
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem) { LogBridgePending(legalId, "task-system-unavailable"); return; }
		if (legalId == m_sPresentedObjectiveId && m_CurrentObjectiveTask && SCR_TaskSystem.GetTaskFromTaskID(m_sPresentedTaskId, false) == m_CurrentObjectiveTask)
		{
			SCR_ETaskState currentTaskState = taskSystem.GetTaskState(m_CurrentObjectiveTask);
			if (currentTaskState == SCR_ETaskState.CREATED || currentTaskState == SCR_ETaskState.ASSIGNED) return;
			// A registered completed/cancelled task is not an active objective.
			// Retire it below before reusing the native stable callsign ID.
		}
		if (m_CurrentObjectiveTask)
		{
			// Native OnTargetBaseCaptured normally completes/deletes the task.
			// If it is still registered, finish/cancel through that same TaskSystem
			// API before deletion. Never repeat deletion of an already-ended task.
			SCR_Task oldTask = SCR_TaskSystem.GetTaskFromTaskID(m_sPresentedTaskId, false);
			if (oldTask == m_CurrentObjectiveTask)
			{
				SCR_ETaskState endState = SCR_ETaskState.CANCELLED;
				string structural = TB_CC_ObjectiveCommitment.StructuralInvalidation(m_Registry, m_sPresentedObjectiveId, GetSelectedMobId());
				if (structural.IsEmpty() && GetTerritorialOwner(m_sPresentedObjectiveId) == "US") endState = SCR_ETaskState.COMPLETED;
				taskSystem.SetTaskState(oldTask, endState);
				taskSystem.DeleteTask(oldTask);
			}
			PrintFormat("[TB_CC_M4XP_OBJECTIVE] cleared id=%1 revision=%2", m_sPresentedObjectiveId, m_iPresentedRevision);
			m_CurrentObjectiveTask = null;
			m_sPresentedObjectiveId = "";
			m_sPresentedTaskId = "";
		}
		if (legalId.IsEmpty()) return;
		SCR_CampaignMilitaryBaseComponent base = GetBase(legalId);
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		if (!base || !us || !base.IsInitialized()) { LogBridgePending(legalId, "base-or-faction-unresolved"); return; }
		if (base.GetCallsign() == SCR_MilitaryBaseComponent.INVALID_BASE_CALLSIGN) { LogBridgePending(legalId, "base-callsign-unavailable"); return; }
		SCR_GroupTaskManagerComponent groupTaskManager = SCR_GroupTaskManagerComponent.GetInstance();
		if (!groupTaskManager) { LogBridgePending(legalId, "group-task-manager-unavailable"); return; }
		if (!groupTaskManager.CanCreateNewTaskWithResourceName(SEIZE_TASK_PREFAB, us)) { LogBridgePending(legalId, "group-task-create-denied"); return; }
		string taskId = GetPresentedTaskId(legalId, m_iRevision);
		PrintFormat("[TB_CC_M4XP_OBJECTIVE] create-request desired=%1 resolvedEntity=%2 callsign=%3 existingTask=%4 createAttempt=1 taskId=%5 taskFaction=US", legalId, base.GetOwner(), base.GetCallsign(), m_CurrentObjectiveTask != null, taskId);
		SCR_CampaignMilitaryBaseTaskEntity task;
		if (UsesNativeTaskService())
		{
			SCR_CampaignFactionCommanderHandlerComponent handler = SCR_CampaignFactionCommanderHandlerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_CampaignFactionCommanderHandlerComponent));
			if (!handler) { LogBridgePending(legalId, "native-task-handler-unavailable"); return; }
			handler.CreateSeizeTask(us, base, -1);
			task = SCR_CampaignMilitaryBaseTaskEntity.Cast(SCR_TaskSystem.GetTaskFromTaskID(taskId, false));
		}
		else
			task = SCR_CampaignMilitaryBaseTaskEntity.Cast(taskSystem.CreateTask(SEIZE_TASK_PREFAB, taskId, "", "#AR-CampaignTasks_DescriptionSeize", base.GetOwner().GetOrigin()));
		if (!task) { LogBridgePending(legalId, "task-create-failed"); return; }
		if (!UsesNativeTaskService())
		{
			task.SetTaskName("#AR-CampaignTasks_TitleSeize-UC", {base.GetFormattedBaseNameWithCallsign(us)});
			task.SetMilitaryBaseCallSign(base.GetCallsign());
			taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.EXECUTOR);
			taskSystem.AddTaskFaction(task, "US");
			groupTaskManager.SetGroupTask(task, 0);
		}
		// BCM presentation policy widens native group visibility to all US players.
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		m_CurrentObjectiveTask = task;
		m_sPresentedObjectiveId = legalId;
		m_sPresentedTaskId = taskId;
		m_iPresentedRevision = m_iRevision;
		m_sLastBridgePending = "";
		SCR_Task registered = SCR_TaskSystem.GetTaskFromTaskID(taskId, false);
		PrintFormat("[TB_CC_M4XP_OBJECTIVE] published desired=%1 revision=%2 taskId=%3 taskCreated=1 registeredWithTaskSystem=%4 taskState=%5 taskFaction=US groupTaskRegistered=1 candidatesNotPresented=1 replicated=task-system-owned", legalId, m_iRevision, taskId, registered == task, taskSystem.GetTaskState(task));
	}

	protected void LogBridgePending(string desired, string reason)
	{
		string key = desired + ":" + reason;
		if (key == m_sLastBridgePending) return;
		m_sLastBridgePending = key;
		PrintFormat("[TB_CC_M4XP_OBJECTIVE] pending desired=%1 resolvedEntity=%2 existingTask=%3 taskCreated=0 reason=%4", desired, GetBase(desired) != null, m_CurrentObjectiveTask != null, reason);
	}

	override protected void InitializeClientDefinition()
	{
		super.InitializeClientDefinition();
		GetGame().GetCallqueue().CallLater(TraceClientObjectiveTask, 2000, true);
	}

	protected void TraceClientObjectiveTask()
	{
		if (Replication.IsServer()) return;
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (mode) mode.TB_CC_ApplyNativeSnapshot();
		if (TB_PVE_M2_3_FrontierQuery.GetReadiness() != TB_PVE_M2_3_QueryReadiness.READY) return;
		int revision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		string desired = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective();
		string taskId = GetPresentedTaskId(desired, revision);
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		SCR_Task task;
		if (taskSystem && !desired.IsEmpty()) task = SCR_TaskSystem.GetTaskFromTaskID(taskId, false);
		string trace = revision.ToString() + ":" + desired + ":" + (task != null).ToString();
		if (trace == m_sLastClientTaskTrace) return;
		m_sLastClientTaskTrace = trace;
		int taskState = -1;
		if (taskSystem && task) taskState = taskSystem.GetTaskState(task);
		PrintFormat("[TB_CC_M4XP_OBJECTIVE][CLIENT] desired=%1 revision=%2 taskId=%3 taskSystem=%4 taskQueryResult=%5 taskState=%6", desired, revision, taskId, taskSystem != null, task != null, taskState);
	}

	string ResolveSelectedMobForStartup()
	{
		if (m_sExplicitMobId != "ARLAND::HQ_EAST") return "";
		TB_CC_M4_1_StrategicPositionRegistry selectionRegistry = new TB_CC_M4_1_StrategicPositionRegistry();
		if (!selectionRegistry.Build(TB_CC_M4XP_ArlandNativeDefinition.Create())) return "";
		TB_CC_M4_2A_CampaignStartResolver resolver = new TB_CC_M4_2A_CampaignStartResolver();
		TB_CC_M4_2A_CampaignStartState selection = resolver.Resolve(selectionRegistry,
			TB_CC_M4_2A_ArlandStartDefinition.Create(), m_iCampaignSeed, m_iStartMode, m_sExplicitMobId);
		if (!selection) { PrintFormat("[TB_CC_M4XP] INIT_FAILED start=%1", resolver.GetFailure()); return ""; }
		if (selection.GetSelectedMobId() != "ARLAND::HQ_EAST") return "";
		return selection.GetSelectedMobId();
	}

	override protected bool BuildRegistryAndSpatialQuery()
	{
		m_Registry = new TB_CC_M4_1_StrategicPositionRegistry();
		if (!m_Registry.Build(TB_CC_M4XP_ArlandNativeDefinition.Create(m_aPositionImportanceOverrides))) return false;
		m_Registry.GetPositionIds(m_aIds);
		m_aPositions.Clear(); m_mIdsByBase.Clear();
		foreach (string id : m_aIds)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			m_aPositions.Insert(resolved.GetResolvedEntity().GetOrigin());
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(resolved.GetResolvedEntity().FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base) return false;
			m_mIdsByBase.Set(base, id);
		}
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureRoute(m_aIds);
		TB_PVE_M2_3_FrontierQuery.TB_M2_3_ConfigureObjectivePositions(m_aPositions);
		return true;
	}

	override protected void InitializeAuthority()
	{
		if (m_bReady) return;
		// Bootstrap BEFORE objective evaluation, consumers, observers or tickers.
		if (m_RestoreTransaction) return; // Failure is latched; no startup retry.
		m_RestoreTransaction = new TB_CC_RestoreTransaction();
		if (!m_RestoreTransaction.BeginContainerLoading()) return;
		if (!BuildRegistryAndSpatialQuery()) { m_RestoreTransaction.Fail("native-registry"); FailClosed("native-registry"); return; }
		string selectedMob = ResolveSelectedMobForStartup();
		if (selectedMob.IsEmpty()) { m_RestoreTransaction.Fail("native-mob-selection"); FailClosed("native-mob-selection"); return; }
		m_Persistence = new TB_CC_PersistenceService();
		m_Persistence.BindStrategicAdapter(new TB_CC_NativeStrategicPersistenceAdapter(this));
		string persistenceConfig = string.Format("%1:seed=%2:start=%3:mob=%4", m_sPersistenceConfigIdentity, m_iCampaignSeed, m_iStartMode, selectedMob);
		if (!m_Persistence.Start(m_Registry.GetMapDefinitionId(), m_sPersistenceScenarioIdentity, persistenceConfig, m_iPersistenceCheckpointSeconds))
		{ m_RestoreTransaction.Fail("container-load-failed"); return; }
		if (!m_RestoreTransaction.ContainerLoaded(m_Persistence.HasStrategicSnapshot())) return;
		bool restoring = m_Persistence.HasStrategicSnapshot();
		m_Start = new TB_CC_M4_2A_CampaignStartState(m_iCampaignSeed, m_iStartMode, selectedMob, "ARLAND::HQ_WEST", 10);
		foreach (string id : m_aIds)
		{
			SCR_CampaignMilitaryBaseComponent base = GetBase(id);
			if (!base || !base.IsInitialized() || !base.GetFaction()) { m_RestoreTransaction.Fail("native-base-uninitialized-" + id); FailClosed("native-base-uninitialized-" + id); return; }
			FactionKey expectedOwner = "FIA";
			int role = TB_CC_M4XP_CampaignRoles.GetRole(id);
			if (role == TB_CC_StrategicRole.HQ_US) expectedOwner = "US";
			if (role == TB_CC_StrategicRole.HQ_USSR) expectedOwner = "USSR";
			if ((!restoring && base.GetFaction().GetFactionKey() != expectedOwner) || base.IsHQ() != (role != TB_CC_StrategicRole.ORDINARY))
			{
				m_RestoreTransaction.Fail("native-start-role-" + id); FailClosed("native-start-role-" + id); return;
			}
			SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
			if (!seizing) { m_RestoreTransaction.Fail("native-seizing-missing-" + id); FailClosed("native-seizing-missing-" + id); return; }
			if (role == TB_CC_StrategicRole.ORDINARY)
			{
				m_mFormerFIATerritory.Set(id, true);
				m_FIARecaptureLedger.Seed(id, base.GetFaction().GetFactionKey());
			}
			seizing.TB_CC_M4_4_ConfigurePvECaptureRules();
			if (base.IsHQ()) seizing.TB_CC_M4_4_EnableTerminalHQSurface(base);
		}
		m_iAuthorityProofMode = 0;
		m_bBLUFORAdvanced = false;
		m_Evaluator = new TB_CC_M4_3_StrategicEvaluator();
		m_Evaluator.ConfigureExpansion("US", true);
		m_Evaluator.ConfigureTerritorialScoring(m_iTerritoryImportanceWeight, m_iTerritoryProximityWeight, m_iTerritoryConsolidationWeight, m_iTerritoryConnectivityWeight, m_fTerritoryProximityScale, m_iTerritoryVariation);
		m_Evaluator.ConfigureObjectiveBands(m_fUSObjectiveBandMeters);
		m_USSREvaluator = new TB_CC_M4_3_StrategicEvaluator();
		m_USSREvaluator.ConfigureExpansion("USSR", true);
		m_USSREvaluator.ConfigureTerritorialScoring(m_iTerritoryImportanceWeight, m_iTerritoryProximityWeight, m_iTerritoryConsolidationWeight, m_iTerritoryConnectivityWeight, m_fTerritoryProximityScale, m_iTerritoryVariation);
		m_iOutcome = TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		if (restoring && !RestoreStrategicSnapshot(m_Persistence.GetLoadedContainer())) return;
		if (!CompletePersistenceDerivedRebuild()) return;
		m_bReady = true;
		m_Occupation = new TB_CC_TerritorialOccupationService(this);
		GetGame().GetCallqueue().CallLater(TickTerritorialOccupation, 2000, true);
		ApplyDeploymentPolicy();
		m_NativeRadioObserver = new TB_CC_NativeRadioObserver();
		m_NativeRadioObserver.GetOnChanged().Insert(OnNativeRadioTopologyChanged);
		m_NativeRadioObserver.Start(m_Registry, SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US")));
		if (restoring) PublishRestoredStrategicState();
		else Reevaluate("native-full-initialization");
		int enabledSpawns;
		foreach (string id : m_aIds)
		{
			SCR_CampaignMilitaryBaseComponent base = GetBase(id);
			SCR_SpawnPoint spawn = base.GetSpawnPoint();
			bool enabled = spawn && spawn.IsSpawnPointEnabled();
			if (enabled) enabledSpawns++;
			PrintFormat("[TB_CC_M4XP] base id=%1 position=%2 owner=%3 HQ=%4 spawnPresent=%5 spawnEnabled=%6 selected=%7", id, base.GetOwner().GetOrigin(), base.GetFaction().GetFactionKey(), base.IsHQ(), spawn != null, enabled, id == selectedMob);
		}
		SCR_SpawnPoint selectedSpawn = GetBase(selectedMob).GetSpawnPoint();
		if (enabledSpawns != 1 || !selectedSpawn || !selectedSpawn.IsSpawnPointEnabled() ||
			(!restoring && (m_iOutcome != TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_iDepthPermille != 0)) ||
			GetRequiredOrdinaryPositionCount() != 10)
		{
			m_RestoreTransaction.Fail("native-start-contract"); FailClosed("native-start-contract"); return;
		}
		GetGame().GetCallqueue().CallLater(PublishPressureClock, 1000, true);
		// P3 starts only after the existing strategic READY/start contract above.
		if (!m_Persistence.StartPlayerProgression()) { FailClosed("player-persistence-start"); return; }
		// P5 only adopts a staged generation after a normal fresh-world baseline.
		if (m_Persistence.IsResetActivationPending() && !m_Persistence.CompleteResetActivation())
		{ m_RestoreTransaction.Fail("reset-activation-checkpoint"); FailClosed("reset-activation-checkpoint"); return; }
		if (m_iOutcome == TB_PVE_M2_5_CampaignOutcome.ACTIVE) StartM3Consumers();
		// Bounded service-readiness retry and JIP-safe single-task reconciliation.
		GetGame().GetCallqueue().CallLater(SyncObjectiveTask, 2000, true);
		GetGame().GetCallqueue().CallLater(ReportFullNativeState, 5000, true);
		ObservePersistenceState();
		m_Persistence.MarkDirty("strategic-startup"); m_Persistence.SaveNow("strategic-startup");
		GetGame().GetCallqueue().CallLater(ObservePersistenceState, 1000, true);
	}

	protected void ReportFullNativeState()
	{
		int usCount; int ussrCount; int requiredCount;
		CountRequiredTerritory(usCount, ussrCount, requiredCount);
		PrintFormat("[TB_CC_M4XP] state ready=%1 revision=%2 outcome=%3 depth=%4 ordinaryUS=%5 ordinaryUSSR=%6 required=%7 foothold=%8 pressure=%9", m_bReady, m_iRevision, m_iOutcome, m_iDepthPermille, usCount, ussrCount, requiredCount, m_bBLUFORAdvanced, m_bPressureTimerActive);
		PrintFormat("[TB_CC_M4XP] objectives mob=%1 USLegal=%2 USSRLegal=%3 humansUS=%4 USSRorigin=ARLAND::HQ_WEST", GetSelectedMobId(), GetBLUFORLegalObjective(), GetOPFORLegalObjective(), TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US"));
		TB_PVE_M3_7_HumanPresenceQuery.TraceAuthorityState("US");
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(ObservePersistenceState);
		ObservePersistenceState();
		if (m_Persistence) m_Persistence.Stop();
		if (m_NativeRadioObserver)
		{
			m_NativeRadioObserver.GetOnChanged().Remove(OnNativeRadioTopologyChanged);
			m_NativeRadioObserver.Stop();
		}
		StopM3Consumers("native-component-deleted");
		if (m_FIARecapture) m_FIARecapture.Stop("native-component-deleted");
		GetGame().GetCallqueue().Remove(TickTerritorialOccupation);
		if (m_Occupation) m_Occupation.Stop();
		if (m_USSRFrontGarrison) m_USSRFrontGarrison.Stop("native-component-deleted");
		foreach (TB_PVE_M3_6_OPFORFrontlineGarrisonManager manager : m_aTerritorialManagers) manager.Stop("native-component-deleted");
		m_aTerritorialManagers.Clear();
		TB_CC_WarSimulation.Reset();
		if (m_NativeGameMode) m_NativeGameMode.GetOnStarted().Remove(OnNativeCampaignStarted);
		GetGame().GetCallqueue().Remove(InitializeAuthority);
		GetGame().GetCallqueue().Remove(SyncObjectiveTask);
		GetGame().GetCallqueue().Remove(TraceClientObjectiveTask);
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (taskSystem && m_CurrentObjectiveTask) taskSystem.DeleteTask(m_CurrentObjectiveTask);
		GetGame().GetCallqueue().Remove(ReportFullNativeState);
		super.OnDelete(owner);
	}
}
