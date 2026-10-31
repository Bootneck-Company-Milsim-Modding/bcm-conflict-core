// Disposable M1.1 runtime proof harness.
// It observes vanilla Conflict capture and spawns the vanilla OPFOR defender group
// at a BLUFOR-owned non-HQ base. Ownership is never assigned by this script.

// Read-only diagnostic exposure of vanilla configuration. These accessors do
// not alter the seizing calculation or any ownership state.
modded class SCR_SeizingComponent
{
	bool TB_PVE_M1_1_GetIgnoreNonPlayableAttackers()
	{
		return m_bIgnoreNonPlayableAttackers;
	}

	bool TB_PVE_M1_1_GetCapturingRequiresPlayer()
	{
		return m_bCapturingRequiresPlayer;
	}

	float TB_PVE_M1_1_GetMaximumSeizingTime()
	{
		return m_fMaximumSeizingTime;
	}

	float TB_PVE_M1_1_GetMinimumSeizingTime()
	{
		return m_fMinimumSeizingTime;
	}

	int TB_PVE_M1_1_GetMaximumSeizingCharacters()
	{
		return m_iMaximumSeizingCharacters;
	}

	float TB_CC_M4_4B_GetCurrentSeizingTime() { return m_fCurrentSeizingTime; }
	float TB_CC_M4_4B_GetInterruptedCaptureDuration() { return m_fInterruptedCaptureDuration; }
	int TB_CC_M4_4B_GetSeizingCharacters() { return m_iSeizingCharacters; }

	// M1.4A diagnostic only. This calls the vanilla eligibility function and
	// exposes its result without changing presence, timers, or ownership.
	SCR_Faction TB_PVE_M1_4A_EvaluateEntityFaction(IEntity entity)
	{
		return EvaluateEntityFaction(entity);
	}

	SCR_Faction TB_PVE_M1_4A_GetPrevailingFaction()
	{
		return m_PrevailingFaction;
	}
}

modded class SCR_CampaignSeizingComponent
{
	float TB_CC_M4_4B_GetExtraTimePerRadioConnection() { return m_fExtraTimePerRadioConnection; }
	float TB_CC_M4_4B_GetExtraTimePerService() { return m_fExtraTimePerService; }
	int TB_CC_M4_4B_GetMinimumCharactersForContest() { return m_iMinimumCharactersForContest; }
}

modded class SCR_GameModeCampaign
{
	protected const string TB_PVE_M1_1_LOG = "[TB_PVE_M1_1]";
	protected const int TB_PVE_M1_1_START_DELAY_MS = 10000;
	protected const int TB_PVE_M1_1_DIAGNOSTIC_PERIOD_MS = 3000;
	protected const int TB_PVE_M1_1_TIMEOUT_TICKS = 120;
	protected const int TB_PVE_M1_4_SUPPLY_OBSERVATION_PERIOD_MS = 5000;
	protected const int TB_PVE_M1_4_SUPPLY_OBSERVATION_TICKS = 16;
	protected const bool TB_PVE_M1_1_RUN_INTERRUPT_REGRESSION = false;
	protected const bool TB_PVE_M1_4A_CAPTURE_INVESTIGATION = false;
	protected const bool TB_PVE_M1_4A_USE_QUEUED_SPAWN = false;
	protected const bool TB_PVE_M1_4A_FORCE_MAX_LOD_EXPERIMENT = false;
	protected const bool TB_PVE_M1_4B_LIFECYCLE_REGRESSION = true;
	protected const int TB_PVE_M1_4B_POST_RECAPTURE_OBSERVATION_TICKS = 24;

	protected SCR_CampaignMilitaryBaseComponent m_TB_PVE_M1_1_TargetBase;
	protected SCR_CampaignSeizingComponent m_TB_PVE_M1_1_Seizing;
	protected SCR_AIGroup m_TB_PVE_M1_1_AttackingGroup;
	protected ref array<IEntity> m_TB_PVE_M1_1_SpawnedCharacters = {};
	protected SCR_CampaignFaction m_TB_PVE_M1_1_Opfor;
	protected SCR_CampaignFaction m_TB_PVE_M1_1_Blufor;
	protected int m_TB_PVE_M1_1_RegressionPhase;
	protected bool m_TB_PVE_M1_1_Started;
	protected int m_TB_PVE_M1_1_DiagnosticTicks;
	protected int m_TB_PVE_M1_1_LastLivingInside = -1;
	protected SCR_EBaseCaptureState m_TB_PVE_M1_1_LastCaptureState = -1;
	protected FactionKey m_TB_PVE_M1_1_LastOwnerKey = "<unset>";
	protected bool m_TB_PVE_M1_1_InterruptRequested;
	protected int m_TB_PVE_M1_1_BaseLookupAttempts;
	protected float m_TB_PVE_M1_4_StartingSupplies;
	protected int m_TB_PVE_M1_4_SupplyObservationTicks;
	protected WorldTimestamp m_TB_PVE_M1_4_SupplyObservationStarted;
	protected ref array<string> m_TB_PVE_M1_4A_LastCharacterStates = {};
	protected string m_TB_PVE_M1_4A_LastAggregateState;
	protected int m_TB_PVE_M1_4A_QueuedCollectionAttempts;
	protected bool m_TB_PVE_M1_4A_MaxLODForced;
	protected bool m_TB_PVE_M1_4A_MaxLODExperimentCompleted;
	protected float m_TB_PVE_M1_4B_PostRecaptureStartingSupplies;
	protected int m_TB_PVE_M1_4B_PostRecaptureObservationTicks;

	override void OnGameStart()
	{
		super.OnGameStart();

		PrintFormat("%1 OnGameStart server=%2 dedicated=%3 proxy=%4", TB_PVE_M1_1_LOG, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());

		SCR_MilitaryBaseSystem militaryBaseSystem = SCR_MilitaryBaseSystem.GetInstance();
		if (militaryBaseSystem)
			militaryBaseSystem.GetOnBaseFactionChanged().Insert(TB_PVE_M1_1_OnSystemBaseFactionChanged);

		SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Insert(TB_PVE_M1_1_OnExtendedBaseFactionChanged);
		GetBaseManager().GetOnAllBasesInitialized().Insert(TB_PVE_M1_1_OnAllBasesInitialized);

		if (GetBaseManager().IsBasesInitDone())
			GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_Begin, TB_PVE_M1_1_START_DELAY_MS);
	}

	protected void TB_PVE_M1_1_OnAllBasesInitialized()
	{
		PrintFormat("%1 All bases initialized server=%2 dedicated=%3 proxy=%4", TB_PVE_M1_1_LOG, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_Begin, TB_PVE_M1_1_START_DELAY_MS);
	}

	protected void TB_PVE_M1_1_Begin()
	{
		// This assertion belongs only to the historical M1 fixture topology.
		if (GetGame().GetWorld().FindEntityByName("TB_M4_4_ARLAND_GRAPH_CAMPAIGN")) return;
		if (GetGame().GetWorld().FindEntityByName("TB_CC_NATIVE_ARLAND_CAMPAIGN")) return;
		if (TB_PVE_M2_1_LinearFrontierComponent.IsFrontierActive())
		{
			Print("[TB_PVE_M1_1] M2.1 route controller active; legacy lifecycle harness suppressed");
			return;
		}

		if (m_TB_PVE_M1_1_Started)
			return;

		m_TB_PVE_M1_1_Started = true;

		m_TB_PVE_M1_1_Blufor = GetFactionByEnum(SCR_ECampaignFaction.BLUFOR);
		m_TB_PVE_M1_1_Opfor = GetFactionByEnum(SCR_ECampaignFaction.OPFOR);
		if (!m_TB_PVE_M1_1_Blufor || !m_TB_PVE_M1_1_Opfor)
		{
			PrintFormat("%1 FAIL missing campaign faction BLUFOR=%2 OPFOR=%3", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_Blufor, m_TB_PVE_M1_1_Opfor);
			return;
		}

		PrintFormat("[TB_PVE_M1_5] BLUFOR key=%1 playable=%2 playerLimit=%3 canCaptureBases=%4 server=%5 dedicated=%6 proxy=%7", m_TB_PVE_M1_1_Blufor.GetFactionKey(), m_TB_PVE_M1_1_Blufor.IsPlayable(), m_TB_PVE_M1_1_Blufor.GetPlayerLimit(), m_TB_PVE_M1_1_Blufor.CanCaptureBases(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		PrintFormat("[TB_PVE_M1_5] OPFOR key=%1 playable=%2 playerLimit=%3 canCaptureBases=%4 server=%5 dedicated=%6 proxy=%7", m_TB_PVE_M1_1_Opfor.GetFactionKey(), m_TB_PVE_M1_1_Opfor.IsPlayable(), m_TB_PVE_M1_1_Opfor.GetPlayerLimit(), m_TB_PVE_M1_1_Opfor.CanCaptureBases(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		FactionManager factionManager = GetGame().GetFactionManager();
		SCR_CampaignFaction fia = SCR_CampaignFaction.Cast(factionManager.GetFactionByKey("FIA"));
		if (fia)
			PrintFormat("[TB_PVE_M1_5] FIA key=%1 playable=%2 playerLimit=%3 canCaptureBases=%4 unchanged=1 server=%5 dedicated=%6 proxy=%7", fia.GetFactionKey(), fia.IsPlayable(), fia.GetPlayerLimit(), fia.CanCaptureBases(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());

		array<SCR_CampaignMilitaryBaseComponent> bases = {};
		GetBaseManager().GetBases(bases);
		SCR_CampaignMilitaryBaseComponent fallbackTarget;
		foreach (SCR_CampaignMilitaryBaseComponent base : bases)
		{
			if (base && Replication.IsServer() && !IsProxy())
				PrintFormat("[TB_PVE_M1_4B] topology-base entity=%1 display=%2 position=%3 initialized=%4 isHQ=%5", base.GetOwner().GetName(), base.GetBaseName(), base.GetOwner().GetOrigin(), base.IsInitialized(), base.IsHQ());
			if (!base || !base.IsInitialized() || base.IsHQ())
				continue;
			string entityName = base.GetOwner().GetName();
			if (entityName != "TB_M1_3_FORWARD_Base" && entityName != "TB_M1_4B_FORWARD_SOUTH")
				continue;
			if (!fallbackTarget)
				fallbackTarget = base;
			if (base.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_Blufor, SCR_ERadioCoverageStatus.BOTH_WAYS))
			{
				m_TB_PVE_M1_1_TargetBase = base;
				break;
			}
		}
		if (!m_TB_PVE_M1_1_TargetBase)
			m_TB_PVE_M1_1_TargetBase = fallbackTarget;

		if (!m_TB_PVE_M1_1_TargetBase)
		{
			m_TB_PVE_M1_1_Started = false;
			m_TB_PVE_M1_1_BaseLookupAttempts++;
			if (m_TB_PVE_M1_1_BaseLookupAttempts <= 30)
			{
				GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_Begin, 1000);
				return;
			}
			PrintFormat("%1 FAIL M1.3 requires replicated TB_M1_3_FORWARD_Base server=%2 proxy=%3", TB_PVE_M1_1_LOG, Replication.IsServer(), IsProxy());
			return;
		}

		m_TB_PVE_M1_1_Seizing = SCR_CampaignSeizingComponent.Cast(m_TB_PVE_M1_1_TargetBase.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (!m_TB_PVE_M1_1_Seizing)
		{
			PrintFormat("%1 FAIL target has no SCR_CampaignSeizingComponent base=%2", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_TargetBase.GetBaseName());
			return;
		}

		m_TB_PVE_M1_1_Seizing.GetOnCaptureStart().Insert(TB_PVE_M1_1_OnCaptureStart);
		m_TB_PVE_M1_1_Seizing.GetOnCaptureInterrupt().Insert(TB_PVE_M1_1_OnCaptureInterrupt);
		m_TB_PVE_M1_1_Seizing.GetOnCaptureFinish().Insert(TB_PVE_M1_1_OnCaptureFinish);
		m_TB_PVE_M1_1_Seizing.GetOnCaptureStateChanged().Insert(TB_PVE_M1_1_OnCaptureStateChanged);

		PrintFormat("%1 M1.3 initial proof base=%2 entity=%3 owner=%4 initialized=%5 isHQ=%6 type=%7 baseRadius=%8 seizingRadius=%9", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_TargetBase.GetBaseName(), m_TB_PVE_M1_1_TargetBase.GetOwner().GetName(), m_TB_PVE_M1_1_TargetBase.GetFaction().GetFactionKey(), m_TB_PVE_M1_1_TargetBase.IsInitialized(), m_TB_PVE_M1_1_TargetBase.IsHQ(), m_TB_PVE_M1_1_TargetBase.GetType(), m_TB_PVE_M1_1_TargetBase.GetRadius(), m_TB_PVE_M1_1_Seizing.GetRadius());
		TB_PVE_M1_1_LogSpawnAndSupplyContract("initial US");
		TB_PVE_M1_1_LogUSMobContract();
		TB_PVE_M1_4_LogFunctionality("initial US");
		PrintFormat("%1 M1.3 role server=%2 dedicated=%3 proxy=%4", TB_PVE_M1_1_LOG, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		PrintFormat("%1 M1.3 production gates ignoreNonPlayableAttackers=%2 capturingRequiresPlayer=%3 opforHQRadioTraffic=%4", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_Seizing.TB_PVE_M1_1_GetIgnoreNonPlayableAttackers(), m_TB_PVE_M1_1_Seizing.TB_PVE_M1_1_GetCapturingRequiresPlayer(), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_Opfor));
		PrintFormat("[TB_PVE_M1_4B] initial owner=%1 initialized=%2 isHQ=%3 directMaterialization=1 forceLOD=0", m_TB_PVE_M1_1_TargetBase.GetFaction().GetFactionKey(), m_TB_PVE_M1_1_TargetBase.IsInitialized(), m_TB_PVE_M1_1_TargetBase.IsHQ());

		if (IsProxy())
		{
			GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_ClientDiagnosticTick, 1000, true);
			return;
		}

		PrintFormat("%1 M1.3 vanilla timing maximumSeconds=%2 minimumSeconds=%3 maximumCharacters=%4", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_Seizing.TB_PVE_M1_1_GetMaximumSeizingTime(), m_TB_PVE_M1_1_Seizing.TB_PVE_M1_1_GetMinimumSeizingTime(), m_TB_PVE_M1_1_Seizing.TB_PVE_M1_1_GetMaximumSeizingCharacters());
		if (TB_PVE_M1_4A_CAPTURE_INVESTIGATION)
		{
			Print("[TB_PVE_M1_4A] supply observation bypassed for capture-lifecycle investigation; no supply state was mutated");
			TB_PVE_M1_1_SpawnCaptureGroup(m_TB_PVE_M1_1_Opfor, "M1.4A direct-spawn control");
			return;
		}

		m_TB_PVE_M1_4_StartingSupplies = m_TB_PVE_M1_1_TargetBase.GetSupplies();
		m_TB_PVE_M1_4_SupplyObservationStarted = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetServerTimestamp();
		PrintFormat("%1 M1.4 supply regeneration observation started supplies=%2 arrivalTime=%3", TB_PVE_M1_1_LOG, m_TB_PVE_M1_4_StartingSupplies, m_TB_PVE_M1_1_TargetBase.GetSuppliesArrivalTime());
		GetGame().GetCallqueue().CallLater(TB_PVE_M1_4_SupplyObservationTick, TB_PVE_M1_4_SUPPLY_OBSERVATION_PERIOD_MS, true);
	}

	protected void TB_PVE_M1_4_SupplyObservationTick()
	{
		m_TB_PVE_M1_4_SupplyObservationTicks++;
		float supplies = m_TB_PVE_M1_1_TargetBase.GetSupplies();
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		int elapsedMs = world.GetServerTimestamp().DiffMilliseconds(m_TB_PVE_M1_4_SupplyObservationStarted);
		PrintFormat("%1 M1.4 supply observation tick=%2 elapsedMs=%3 start=%4 current=%5 income=%6 arrivalTime=%7", TB_PVE_M1_1_LOG, m_TB_PVE_M1_4_SupplyObservationTicks, elapsedMs, m_TB_PVE_M1_4_StartingSupplies, supplies, m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome(), m_TB_PVE_M1_1_TargetBase.GetSuppliesArrivalTime());

		if (supplies > m_TB_PVE_M1_4_StartingSupplies)
		{
			PrintFormat("%1 PASS M1.4 vanilla supply regeneration start=%2 end=%3 delta=%4 elapsedMs=%5 spawnEnabled=0", TB_PVE_M1_1_LOG, m_TB_PVE_M1_4_StartingSupplies, supplies, supplies - m_TB_PVE_M1_4_StartingSupplies, elapsedMs);
			GetGame().GetCallqueue().Remove(TB_PVE_M1_4_SupplyObservationTick);
			TB_PVE_M1_1_SpawnCaptureGroup(m_TB_PVE_M1_1_Opfor, "M1.4 US-to-USSR AI capture");
		}
		else if (m_TB_PVE_M1_4_SupplyObservationTicks >= TB_PVE_M1_4_SUPPLY_OBSERVATION_TICKS)
		{
			PrintFormat("%1 M1.4 TOPOLOGY BLOCKED supply regeneration observation elapsedMs=%2 supplies=%3 income=%4 hqTrafficBothWays=%5; no production mutation performed", TB_PVE_M1_1_LOG, elapsedMs, supplies, m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome(), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_Blufor, SCR_ERadioCoverageStatus.BOTH_WAYS));
			GetGame().GetCallqueue().Remove(TB_PVE_M1_4_SupplyObservationTick);
			TB_PVE_M1_1_SpawnCaptureGroup(m_TB_PVE_M1_1_Opfor, "M1.4 US-to-USSR AI capture after topology observation");
		}
	}

	protected void TB_PVE_M1_1_LogSpawnAndSupplyContract(string phase)
	{
		SCR_SpawnPoint spawnPoint = m_TB_PVE_M1_1_TargetBase.GetSpawnPoint();
		FactionKey spawnFaction = "<none>";
		bool enabled;
		bool visible;
		bool canReserve;
		SCR_ESpawnResult reserveResult = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
		if (spawnPoint)
		{
			spawnFaction = spawnPoint.GetFactionKey();
			enabled = spawnPoint.IsSpawnPointEnabled();
			visible = spawnPoint.IsSpawnPointVisibleForPlayer(SCR_PlayerController.GetLocalPlayerId());
			if (Replication.IsServer() && !IsProxy())
				canReserve = spawnPoint.CanReserveFor_S(1, reserveResult);
		}
		PrintFormat("%1 M1.3 deployment contract phase=%2 spawnPointValid=%3 spawnEnabled=%4 visibleInDeployUI=%5 canReserve=%6 reserveResult=%7 spawnFaction=%8", TB_PVE_M1_1_LOG, phase, spawnPoint != null, enabled, visible, canReserve, reserveResult, spawnFaction);
		PrintFormat("%1 M1.3 supply contract phase=%2 supplies=%3 suppliesMax=%4 supplyIncome=%5", TB_PVE_M1_1_LOG, phase, m_TB_PVE_M1_1_TargetBase.GetSupplies(), m_TB_PVE_M1_1_TargetBase.GetSuppliesMax(), m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome());
	}

	protected void TB_PVE_M1_1_LogUSMobContract()
	{
		int enabledUSMobs;
		foreach (SCR_SpawnPoint spawnPoint : SCR_SpawnPoint.GetSpawnPointsForFaction("US"))
		{
			SCR_CampaignSpawnPointGroup campaignSpawn = SCR_CampaignSpawnPointGroup.Cast(spawnPoint);
			if (!campaignSpawn || !campaignSpawn.GetParent())
				continue;
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(campaignSpawn.GetParent().FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (base && base.IsHQ() && spawnPoint.IsSpawnPointEnabled())
				enabledUSMobs++;
		}
		PrintFormat("%1 M1.3 MOB contract enabledUSMobSpawnPoints=%2", TB_PVE_M1_1_LOG, enabledUSMobs);
	}

	protected void TB_PVE_M1_4_LogFunctionality(string phase)
	{
		SCR_ResourceComponent resourceComponent = m_TB_PVE_M1_1_TargetBase.GetResourceComponent();
		SCR_ResourceConsumer resourceConsumer = m_TB_PVE_M1_1_TargetBase.GetResourceConsumer();
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(m_TB_PVE_M1_1_TargetBase.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		array<SCR_CampaignBuildingProviderComponent> providers = {};
		array<SCR_ServicePointComponent> services = {};
		array<SCR_ServicePointDelegateComponent> delegates = {};
		int providerCount = m_TB_PVE_M1_1_TargetBase.GetBuildingProviders(providers);
		int serviceCount = m_TB_PVE_M1_1_TargetBase.GetServices(services);
		int delegateCount = m_TB_PVE_M1_1_TargetBase.GetServiceDelegates(delegates);
		SCR_CampaignBuildingProviderComponent masterProvider = m_TB_PVE_M1_1_TargetBase.GetMasterProvider();
		Faction owner = m_TB_PVE_M1_1_TargetBase.GetFaction();
		FactionKey ownerKey = "<none>";
		int transceivers;
		bool isMasterProvider;
		float buildingRadius;
		if (owner)
			ownerKey = owner.GetFactionKey();
		if (radio)
			transceivers = radio.TransceiversCount();
		if (masterProvider)
		{
			isMasterProvider = masterProvider.IsMasterProvider();
			buildingRadius = masterProvider.GetBuildingRadius();
		}

		PrintFormat("%1 M1.4 storage phase=%2 owner=%3 resourceComponentValid=%4 supplyConsumerValid=%5 supplies=%6 max=%7 income=%8", TB_PVE_M1_1_LOG, phase, ownerKey, resourceComponent != null, resourceConsumer != null, m_TB_PVE_M1_1_TargetBase.GetSupplies(), m_TB_PVE_M1_1_TargetBase.GetSuppliesMax(), m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome());
		PrintFormat("%1 M1.4 radio phase=%2 radioValid=%3 transceivers=%4 range=%5 hqCoverage=%6 hqTrafficReceive=%7 hqTrafficBothWays=%8", TB_PVE_M1_1_LOG, phase, radio != null, transceivers, m_TB_PVE_M1_1_TargetBase.GetRadioRange(), m_TB_PVE_M1_1_TargetBase.GetHQRadioCoverage(m_TB_PVE_M1_1_TargetBase.GetCampaignFaction()), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_TargetBase.GetCampaignFaction()), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_TargetBase.GetCampaignFaction(), SCR_ERadioCoverageStatus.BOTH_WAYS));
		PrintFormat("%1 M1.4 construction phase=%2 providers=%3 masterProviderValid=%4 master=%5 radius=%6", TB_PVE_M1_1_LOG, phase, providerCount, masterProvider != null, isMasterProvider, buildingRadius);
		PrintFormat("%1 M1.4 services phase=%2 services=%3 delegates=%4 armory=%5 lightVehicles=%6 heavyVehicles=%7 fieldHospital=%8 barracks=%9", TB_PVE_M1_1_LOG, phase, serviceCount, delegateCount, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.ARMORY) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.LIGHT_VEHICLE_DEPOT) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.HEAVY_VEHICLE_DEPOT) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.FIELD_HOSPITAL) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.BARRACKS) != null);
		PrintFormat("%1 M1.4 services-extra phase=%2 antenna=%3 helipad=%4 fuel=%5 repair=%6 supply=%7", TB_PVE_M1_1_LOG, phase, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.RADIO_ANTENNA) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.HELIPAD) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.FUEL_DEPOT) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.REPAIR_DEPOT) != null, m_TB_PVE_M1_1_TargetBase.GetServiceDelegateByType(SCR_EServicePointType.SUPPLY_DEPOT) != null);
	}

	protected void TB_PVE_M1_1_ClientDiagnosticTick()
	{
		if (!m_TB_PVE_M1_1_TargetBase)
			return;

		Faction owner = m_TB_PVE_M1_1_TargetBase.GetFaction();
		FactionKey ownerKey = "<none>";
		if (owner)
			ownerKey = owner.GetFactionKey();
		SCR_EBaseCaptureState captureState = m_TB_PVE_M1_1_TargetBase.GetCaptureState();
		m_TB_PVE_M1_1_DiagnosticTicks++;
		if (captureState != m_TB_PVE_M1_1_LastCaptureState || ownerKey != m_TB_PVE_M1_1_LastOwnerKey || (m_TB_PVE_M1_1_DiagnosticTicks % 10) == 0)
		{
			PrintFormat("%1 M1.3 remote observation tick=%2 captureState=%3 owner=%4 server=%5 dedicated=%6 proxy=%7", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_DiagnosticTicks, captureState, ownerKey, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
			PrintFormat("%1 M1.4 remote state tick=%2 owner=%3 supplies=%4 max=%5 income=%6 spawnEnabled=%7", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_DiagnosticTicks, ownerKey, m_TB_PVE_M1_1_TargetBase.GetSupplies(), m_TB_PVE_M1_1_TargetBase.GetSuppliesMax(), m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome(), m_TB_PVE_M1_1_TargetBase.GetSpawnPoint().IsSpawnPointEnabled());
			m_TB_PVE_M1_1_LastCaptureState = captureState;
			m_TB_PVE_M1_1_LastOwnerKey = ownerKey;
		}

		if (owner == GetFactionByEnum(SCR_ECampaignFaction.OPFOR) && m_TB_PVE_M1_1_RegressionPhase == 0)
		{
			m_TB_PVE_M1_1_RegressionPhase = 1;
			PrintFormat("%1 M1.3 remote client observed intermediate owner=USSR captureState=%2 proxy=%3", TB_PVE_M1_1_LOG, captureState, IsProxy());
		}

		if (owner == GetFactionByEnum(SCR_ECampaignFaction.BLUFOR) && m_TB_PVE_M1_1_RegressionPhase == 1)
		{
			m_TB_PVE_M1_1_RegressionPhase = 2;
			PrintFormat("%1 PASS M1.3 remote client observed authoritative regression finalOwner=US captureState=%2 proxy=%3", TB_PVE_M1_1_LOG, captureState, IsProxy());
			GetGame().GetCallqueue().Remove(TB_PVE_M1_1_ClientDiagnosticTick);
		}
	}

	protected void TB_PVE_M1_1_SpawnCaptureGroup(SCR_CampaignFaction faction, string phase)
	{
		m_TB_PVE_M1_1_DiagnosticTicks = 0;
		m_TB_PVE_M1_1_SpawnedCharacters.Clear();
		m_TB_PVE_M1_4A_QueuedCollectionAttempts = 0;
		ResourceName groupPrefab = faction.GetDefendersGroupPrefab();
		Resource groupResource = Resource.Load(groupPrefab);
		if (!groupResource)
		{
			PrintFormat("%1 FAIL %2 group resource could not be loaded prefab=%3", TB_PVE_M1_1_LOG, phase, groupPrefab);
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		m_TB_PVE_M1_1_TargetBase.GetOwner().GetTransform(params.Transform);
		m_TB_PVE_M1_1_AttackingGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(groupResource, GetGame().GetWorld(), params));

		if (!m_TB_PVE_M1_1_AttackingGroup)
		{
			PrintFormat("%1 FAIL %2 AI group did not spawn prefab=%3", TB_PVE_M1_1_LOG, phase, groupPrefab);
			return;
		}

		m_TB_PVE_M1_4A_LastCharacterStates.Clear();
		m_TB_PVE_M1_4A_LastAggregateState = string.Empty;

		if (TB_PVE_M1_4A_CAPTURE_INVESTIGATION && TB_PVE_M1_4A_USE_QUEUED_SPAWN)
		{
			int requestedSlots = m_TB_PVE_M1_1_AttackingGroup.m_aUnitPrefabSlots.Count();
			m_TB_PVE_M1_1_AttackingGroup.SetMaxUnitsToSpawn(requestedSlots);
			m_TB_PVE_M1_1_AttackingGroup.RequestSpawn(requestedSlots, 10000);
			PrintFormat("[TB_PVE_M1_4A] attempt-start spawnMethod=vanilla-request-spawn group=%1 faction=%2 requestedCharacters=%3 groupLOD=%4 maxLOD=%5 server=%6 dedicated=%7 proxy=%8", m_TB_PVE_M1_1_AttackingGroup, m_TB_PVE_M1_1_AttackingGroup.GetFaction().GetFactionKey(), requestedSlots, m_TB_PVE_M1_1_AttackingGroup.GetLOD(), AIAgent.GetMaxLOD(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
			GetGame().GetCallqueue().CallLater(TB_PVE_M1_4A_CollectQueuedCharacters, 1000);
			return;
		}

		// A runtime-spawned group outside an active streaming grid does not create its
		// scene members. Materialize the vanilla unit slots explicitly and register
		// their native AI agents with the vanilla group.
		vector baseOrigin = m_TB_PVE_M1_1_TargetBase.GetOwner().GetOrigin();
		int unitIndex;
		foreach (ResourceName unitPrefab : m_TB_PVE_M1_1_AttackingGroup.m_aUnitPrefabSlots)
		{
			Resource unitResource = Resource.Load(unitPrefab);
			if (!unitResource)
				continue;

			params.Transform[3] = baseOrigin + Vector(unitIndex * 2, 0, 0);
			IEntity character = GetGame().SpawnEntityPrefab(unitResource, GetGame().GetWorld(), params);
			if (character)
			{
				m_TB_PVE_M1_1_AttackingGroup.AddAgentFromControlledEntity(character);
				m_TB_PVE_M1_1_SpawnedCharacters.Insert(character);
			}
			unitIndex++;
		}

		PrintFormat("%1 AI group spawned phase=%2 group=%3 faction=%4 unitSlots=%5 characters=%6 position=%7 targetPosition=%8 prefab=%9", TB_PVE_M1_1_LOG, phase, m_TB_PVE_M1_1_AttackingGroup, m_TB_PVE_M1_1_AttackingGroup.GetFaction().GetFactionKey(), m_TB_PVE_M1_1_AttackingGroup.m_aUnitPrefabSlots.Count(), m_TB_PVE_M1_1_SpawnedCharacters.Count(), m_TB_PVE_M1_1_AttackingGroup.GetOrigin(), baseOrigin, groupPrefab);
		PrintFormat("[TB_PVE_M1_4A] attempt-start spawnMethod=direct-group-plus-explicit-members group=%1 faction=%2 requestedCharacters=%3 groupLOD=%4 maxLOD=%5 server=%6 dedicated=%7 proxy=%8", m_TB_PVE_M1_1_AttackingGroup, m_TB_PVE_M1_1_AttackingGroup.GetFaction().GetFactionKey(), m_TB_PVE_M1_1_SpawnedCharacters.Count(), m_TB_PVE_M1_1_AttackingGroup.GetLOD(), AIAgent.GetMaxLOD(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		TB_PVE_M1_4A_LogEligibilitySnapshot("spawn-complete", true);
		GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_DiagnosticTick, TB_PVE_M1_1_DIAGNOSTIC_PERIOD_MS, true);
	}

	protected void TB_PVE_M1_4B_RemoveCaptureGroup()
	{
		GetGame().GetCallqueue().Remove(TB_PVE_M1_1_DiagnosticTick);
		foreach (IEntity character : m_TB_PVE_M1_1_SpawnedCharacters)
		{
			if (character)
				SCR_EntityHelper.DeleteEntityAndChildren(character);
		}
		m_TB_PVE_M1_1_SpawnedCharacters.Clear();
		if (m_TB_PVE_M1_1_AttackingGroup)
			SCR_EntityHelper.DeleteEntityAndChildren(m_TB_PVE_M1_1_AttackingGroup);
		m_TB_PVE_M1_1_AttackingGroup = null;
		Print("[TB_PVE_M1_4B] removed completed capture force before opposing vanilla capture");
	}

	protected void TB_PVE_M1_4B_StartBluforRecapture()
	{
		TB_PVE_M1_1_SpawnCaptureGroup(m_TB_PVE_M1_1_Blufor, "M1.4B USSR-to-US vanilla AI recapture");
	}

	protected void TB_PVE_M1_4B_PostRecaptureObservationTick()
	{
		m_TB_PVE_M1_4B_PostRecaptureObservationTicks++;
		float supplies = m_TB_PVE_M1_1_TargetBase.GetSupplies();
		PrintFormat("[TB_PVE_M1_4B] post-recapture tick=%1 startSupplies=%2 currentSupplies=%3 income=%4 owner=%5 hqTrafficBothWays=%6 spawnValid=%7 spawnEnabled=%8", m_TB_PVE_M1_4B_PostRecaptureObservationTicks, m_TB_PVE_M1_4B_PostRecaptureStartingSupplies, supplies, m_TB_PVE_M1_1_TargetBase.GetSuppliesIncome(), m_TB_PVE_M1_1_TargetBase.GetFaction().GetFactionKey(), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_Blufor, SCR_ERadioCoverageStatus.BOTH_WAYS), m_TB_PVE_M1_1_TargetBase.GetSpawnPoint() != null, m_TB_PVE_M1_1_TargetBase.GetSpawnPoint().IsSpawnPointEnabled());
		if (supplies > m_TB_PVE_M1_4B_PostRecaptureStartingSupplies || m_TB_PVE_M1_4B_PostRecaptureObservationTicks >= TB_PVE_M1_4B_POST_RECAPTURE_OBSERVATION_TICKS)
		{
			PrintFormat("[TB_PVE_M1_4B] COMPLETE sequence=US->USSR->US postRecaptureSupplyDelta=%1 observationTicks=%2", supplies - m_TB_PVE_M1_4B_PostRecaptureStartingSupplies, m_TB_PVE_M1_4B_PostRecaptureObservationTicks);
			GetGame().GetCallqueue().Remove(TB_PVE_M1_4B_PostRecaptureObservationTick);
		}
	}

	protected void TB_PVE_M1_4A_CollectQueuedCharacters()
	{
		m_TB_PVE_M1_4A_QueuedCollectionAttempts++;
		array<AIAgent> agents = {};
		m_TB_PVE_M1_1_AttackingGroup.GetAgents(agents);
		m_TB_PVE_M1_1_SpawnedCharacters.Clear();
		foreach (AIAgent agent : agents)
		{
			if (agent && agent.GetControlledEntity())
				m_TB_PVE_M1_1_SpawnedCharacters.Insert(agent.GetControlledEntity());
		}

		if (m_TB_PVE_M1_1_SpawnedCharacters.Count() == 0 && m_TB_PVE_M1_4A_QueuedCollectionAttempts < 15)
		{
			GetGame().GetCallqueue().CallLater(TB_PVE_M1_4A_CollectQueuedCharacters, 1000);
			return;
		}

		PrintFormat("[TB_PVE_M1_4A] queued-spawn-ready collectionAttempts=%1 groupAgents=%2 controlledCharacters=%3", m_TB_PVE_M1_4A_QueuedCollectionAttempts, agents.Count(), m_TB_PVE_M1_1_SpawnedCharacters.Count());
		TB_PVE_M1_4A_LogEligibilitySnapshot("queued-spawn-ready", true);
		GetGame().GetCallqueue().CallLater(TB_PVE_M1_1_DiagnosticTick, TB_PVE_M1_1_DIAGNOSTIC_PERIOD_MS, true);
	}

	protected void TB_PVE_M1_4A_LogEligibilitySnapshot(string reason, bool force)
	{
		if (!m_TB_PVE_M1_1_TargetBase || !m_TB_PVE_M1_1_Seizing)
			return;

		array<AIAgent> groupAgents = {};
		if (m_TB_PVE_M1_1_AttackingGroup)
			m_TB_PVE_M1_1_AttackingGroup.GetAgents(groupAgents);

		int existing;
		int alive;
		int inside;
		int eligible;
		int atMaxLOD;
		int aiActivated;
		vector baseOrigin = m_TB_PVE_M1_1_TargetBase.GetOwner().GetOrigin();
		float radius = m_TB_PVE_M1_1_Seizing.GetRadius();

		for (int index = 0; index < m_TB_PVE_M1_1_SpawnedCharacters.Count(); index++)
		{
			IEntity entity = m_TB_PVE_M1_1_SpawnedCharacters[index];
			if (!entity)
			{
				string missingState = string.Format("index=%1 exists=0", index);
				if (force || !m_TB_PVE_M1_4A_LastCharacterStates.IsIndexValid(index) || m_TB_PVE_M1_4A_LastCharacterStates[index] != missingState)
					PrintFormat("[TB_PVE_M1_4A] character-state reason=%1 %2", reason, missingState);
				if (m_TB_PVE_M1_4A_LastCharacterStates.IsIndexValid(index))
					m_TB_PVE_M1_4A_LastCharacterStates[index] = missingState;
				else
					m_TB_PVE_M1_4A_LastCharacterStates.Insert(missingState);
				continue;
			}

			existing++;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			CharacterControllerComponent controller;
			AIControlComponent aiControl;
			AIAgent agent;
			bool isAlive;
			bool isActivated;
			int lod = -1;
			if (character)
				controller = character.GetCharacterController();
			if (controller)
			{
				isAlive = controller.GetLifeState() == ECharacterLifeState.ALIVE;
				aiControl = controller.GetAIControlComponent();
			}
			if (aiControl)
			{
				agent = aiControl.GetAIAgent();
				isActivated = aiControl.IsAIActivated();
			}
			if (agent)
				lod = agent.GetLOD();

			float distance = vector.DistanceXZ(entity.GetOrigin(), baseOrigin);
			bool isInside = distance <= radius;
			bool isMember = agent && groupAgents.Contains(agent);
			SCR_Faction countedFaction = m_TB_PVE_M1_1_Seizing.TB_PVE_M1_4A_EvaluateEntityFaction(entity);
			FactionKey countedFactionKey = "<none>";
			if (countedFaction)
				countedFactionKey = countedFaction.GetFactionKey();

			if (isAlive)
				alive++;
			if (isInside)
				inside++;
			if (countedFaction)
				eligible++;
			if (agent && lod == AIAgent.GetMaxLOD())
				atMaxLOD++;
			if (isActivated)
				aiActivated++;

			string stateKey = string.Format("index=%1 character=%2 alive=%3 inside=%4 controller=%5 aiControl=%6 agent=%7", index, character != null, isAlive, isInside, controller != null, aiControl != null, agent != null);
			stateKey += string.Format(" member=%1 lod=%2 activated=%3 entityFactionAccepted=%4", isMember, lod, isActivated, countedFactionKey);
			string state = string.Format("%1 position=%2 distance=%3 maxLOD=%4", stateKey, entity.GetOrigin(), distance, AIAgent.GetMaxLOD());
			if (force || !m_TB_PVE_M1_4A_LastCharacterStates.IsIndexValid(index) || m_TB_PVE_M1_4A_LastCharacterStates[index] != stateKey)
				PrintFormat("[TB_PVE_M1_4A] character-state reason=%1 %2", reason, state);
			if (m_TB_PVE_M1_4A_LastCharacterStates.IsIndexValid(index))
				m_TB_PVE_M1_4A_LastCharacterStates[index] = stateKey;
			else
				m_TB_PVE_M1_4A_LastCharacterStates.Insert(stateKey);
		}

		Faction baseOwner = m_TB_PVE_M1_1_TargetBase.GetFaction();
		FactionKey ownerKey = "<none>";
		if (baseOwner)
			ownerKey = baseOwner.GetFactionKey();
		Faction prevailing = m_TB_PVE_M1_1_Seizing.TB_PVE_M1_4A_GetPrevailingFaction();
		FactionKey prevailingKey = "<none>";
		if (prevailing)
			prevailingKey = prevailing.GetFactionKey();
		string aggregate = string.Format("group=%1 groupFaction=%2 groupAgents=%3 groupLOD=%4 existing=%5 alive=%6 inside=%7", m_TB_PVE_M1_1_AttackingGroup != null, m_TB_PVE_M1_1_AttackingGroup.GetFaction().GetFactionKey(), groupAgents.Count(), m_TB_PVE_M1_1_AttackingGroup.GetLOD(), existing, alive, inside);
		aggregate += string.Format(" entityFactionAccepted=%1 maxLODCharacters=%2 activated=%3 prevailing=%4 contestingFactions=%5 owner=%6 captureState=%7", eligible, atMaxLOD, aiActivated, prevailingKey, m_TB_PVE_M1_1_Seizing.GetContestingFactionsCount(), ownerKey, m_TB_PVE_M1_1_TargetBase.GetCaptureState());
		aggregate += string.Format(" opforCanCapture=%1 opforPlayable=%2 opforHQRadio=%3", m_TB_PVE_M1_1_Opfor.CanCaptureBases(), m_TB_PVE_M1_1_Opfor.IsPlayable(), m_TB_PVE_M1_1_TargetBase.IsHQRadioTrafficPossible(m_TB_PVE_M1_1_Opfor));
		if (force || aggregate != m_TB_PVE_M1_4A_LastAggregateState)
		{
			PrintFormat("[TB_PVE_M1_4A] aggregate-state reason=%1 %2", reason, aggregate);
			m_TB_PVE_M1_4A_LastAggregateState = aggregate;
		}
	}

	protected void TB_PVE_M1_1_DiagnosticTick()
	{
		if (!m_TB_PVE_M1_1_TargetBase || !m_TB_PVE_M1_1_AttackingGroup)
			return;

		TB_PVE_M1_4A_LogEligibilitySnapshot("diagnostic-tick", false);

		array<AIAgent> agents = {};
		m_TB_PVE_M1_1_AttackingGroup.GetAgents(agents);
		int livingInside;
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (!character || !character.GetCharacterController() || character.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE)
				continue;

			float distance = vector.DistanceXZ(character.GetOrigin(), m_TB_PVE_M1_1_TargetBase.GetOwner().GetOrigin());
			if (distance <= m_TB_PVE_M1_1_Seizing.GetRadius())
				livingInside++;
		}

		Faction owner = m_TB_PVE_M1_1_TargetBase.GetFaction();
		FactionKey ownerKey = "<none>";
		if (owner)
			ownerKey = owner.GetFactionKey();
		SCR_EBaseCaptureState captureState = m_TB_PVE_M1_1_TargetBase.GetCaptureState();
		m_TB_PVE_M1_1_DiagnosticTicks++;
		if (TB_PVE_M1_1_RUN_INTERRUPT_REGRESSION && !m_TB_PVE_M1_1_InterruptRequested && captureState == SCR_EBaseCaptureState.CAPTURING && m_TB_PVE_M1_1_DiagnosticTicks >= 3)
		{
			vector outsidePosition = m_TB_PVE_M1_1_TargetBase.GetOwner().GetOrigin() + Vector(m_TB_PVE_M1_1_Seizing.GetRadius() + 100, 0, 0);
			foreach (IEntity spawnedCharacter : m_TB_PVE_M1_1_SpawnedCharacters)
			{
				if (spawnedCharacter)
					spawnedCharacter.SetOrigin(outsidePosition);
			}

			m_TB_PVE_M1_1_InterruptRequested = true;
			PrintFormat("%1 M1.1B interruption regression movedCharacters=%2 outsidePosition=%3 seizingRadius=%4", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_SpawnedCharacters.Count(), outsidePosition, m_TB_PVE_M1_1_Seizing.GetRadius());
		}
		if (livingInside != m_TB_PVE_M1_1_LastLivingInside || captureState != m_TB_PVE_M1_1_LastCaptureState || ownerKey != m_TB_PVE_M1_1_LastOwnerKey || (m_TB_PVE_M1_1_DiagnosticTicks % 10) == 0)
		{
			PrintFormat("%1 Observation tick=%2 agents=%3 livingInsideRadius=%4 aiFaction=%5 captureState=%6 owner=%7 contestingFactions=%8 server=%9", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_DiagnosticTicks, agents.Count(), livingInside, m_TB_PVE_M1_1_AttackingGroup.GetFaction().GetFactionKey(), captureState, ownerKey, m_TB_PVE_M1_1_Seizing.GetContestingFactionsCount(), Replication.IsServer());
			ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
			PrintFormat("%1 CaptureClock affiliatedFaction=%2 now=%3 start=%4 end=%5", TB_PVE_M1_1_LOG, m_TB_PVE_M1_1_Seizing.GetFaction(), world.GetServerTimestamp(), m_TB_PVE_M1_1_Seizing.GetSeizingStartTimestamp(), m_TB_PVE_M1_1_Seizing.GetSeizingEndTimestamp());
			m_TB_PVE_M1_1_LastLivingInside = livingInside;
			m_TB_PVE_M1_1_LastCaptureState = captureState;
			m_TB_PVE_M1_1_LastOwnerKey = ownerKey;
		}

		if (owner == m_TB_PVE_M1_1_Opfor && m_TB_PVE_M1_1_RegressionPhase == 0)
		{
			PrintFormat("%1 PASS M1.3 AI-only OPFOR capture initialOwner=US intermediateOwner=%2 captureState=%3", TB_PVE_M1_1_LOG, ownerKey, captureState);
			TB_PVE_M1_1_LogSpawnAndSupplyContract("after USSR capture");
			TB_PVE_M1_4_LogFunctionality("after USSR capture");
			GetGame().GetCallqueue().Remove(TB_PVE_M1_1_DiagnosticTick);
			if (TB_PVE_M1_4A_CAPTURE_INVESTIGATION)
			{
				Print("[TB_PVE_M1_4A] PASS direct-spawn control completed US-to-USSR; investigation run intentionally stops before legacy recapture phase");
				return;
			}
			m_TB_PVE_M1_1_RegressionPhase = 1;
			if (TB_PVE_M1_4B_LIFECYCLE_REGRESSION)
			{
				TB_PVE_M1_4B_RemoveCaptureGroup();
				GetGame().GetCallqueue().CallLater(TB_PVE_M1_4B_StartBluforRecapture, 3000);
			}
			else
				TB_PVE_M1_1_SpawnCaptureGroup(m_TB_PVE_M1_1_Blufor, "M1.3 USSR-to-US AI recapture");
		}
		else if (owner == m_TB_PVE_M1_1_Blufor && m_TB_PVE_M1_1_RegressionPhase == 1)
		{
			m_TB_PVE_M1_1_RegressionPhase = 2;
			PrintFormat("%1 PASS M1.3 ownership regression sequence=US->USSR->US finalOwner=%2 captureState=%3", TB_PVE_M1_1_LOG, ownerKey, captureState);
			TB_PVE_M1_1_LogSpawnAndSupplyContract("after return to US");
			TB_PVE_M1_4_LogFunctionality("after return to US");
			GetGame().GetCallqueue().Remove(TB_PVE_M1_1_DiagnosticTick);
			if (TB_PVE_M1_4B_LIFECYCLE_REGRESSION)
			{
				TB_PVE_M1_4B_RemoveCaptureGroup();
				m_TB_PVE_M1_4B_PostRecaptureStartingSupplies = m_TB_PVE_M1_1_TargetBase.GetSupplies();
				m_TB_PVE_M1_4B_PostRecaptureObservationTicks = 0;
				GetGame().GetCallqueue().CallLater(TB_PVE_M1_4B_PostRecaptureObservationTick, TB_PVE_M1_4_SUPPLY_OBSERVATION_PERIOD_MS, true);
			}
		}
		else if (m_TB_PVE_M1_1_DiagnosticTicks >= TB_PVE_M1_1_TIMEOUT_TICKS)
		{
			PrintFormat("%1 FAIL M1.2 timeout initialOwner=US finalOwner=%2 captureState=%3 agents=%4 livingInsideRadius=%5 captureStart=%6 captureEnd=%7; vanilla ownership did not transition", TB_PVE_M1_1_LOG, ownerKey, captureState, agents.Count(), livingInside, m_TB_PVE_M1_1_Seizing.GetSeizingStartTimestamp(), m_TB_PVE_M1_1_Seizing.GetSeizingEndTimestamp());
			GetGame().GetCallqueue().Remove(TB_PVE_M1_1_DiagnosticTick);
		}
	}

	protected void TB_PVE_M1_1_OnCaptureStart(SCR_Faction faction, SCR_SeizingComponent component)
	{
		PrintFormat("%1 CaptureStart faction=%2 server=%3 dedicated=%4 proxy=%5", TB_PVE_M1_1_LOG, faction.GetFactionKey(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		TB_PVE_M1_4A_LogEligibilitySnapshot("CaptureStart", true);
		if (TB_PVE_M1_4A_CAPTURE_INVESTIGATION && TB_PVE_M1_4A_FORCE_MAX_LOD_EXPERIMENT && !m_TB_PVE_M1_4A_MaxLODForced && !m_TB_PVE_M1_4A_MaxLODExperimentCompleted)
			GetGame().GetCallqueue().CallLater(TB_PVE_M1_4A_ForceMaximumLOD, 15000);
	}

	protected void TB_PVE_M1_4A_ForceMaximumLOD()
	{
		m_TB_PVE_M1_4A_MaxLODForced = true;
		array<AIAgent> agents = {};
		m_TB_PVE_M1_1_AttackingGroup.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (agent)
				agent.SetLOD(AIAgent.GetMaxLOD());
		}
		PrintFormat("[TB_PVE_M1_4A] EXPERIMENT forced maximum LOD agents=%1 targetLOD=%2", agents.Count(), AIAgent.GetMaxLOD());
		TB_PVE_M1_4A_LogEligibilitySnapshot("forced-maximum-LOD", true);
	}

	protected void TB_PVE_M1_4A_ReleaseMaximumLOD(string reason)
	{
		if (!m_TB_PVE_M1_4A_MaxLODForced)
			return;

		array<AIAgent> agents = {};
		m_TB_PVE_M1_1_AttackingGroup.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (agent)
				agent.SetLOD(0);
		}
		m_TB_PVE_M1_4A_MaxLODForced = false;
		m_TB_PVE_M1_4A_MaxLODExperimentCompleted = true;
		PrintFormat("[TB_PVE_M1_4A] RELEASE restored engine-active LOD reason=%1 agents=%2 requestedLOD=0", reason, agents.Count());
		GetGame().GetCallqueue().CallLater(TB_PVE_M1_4A_LogEligibilitySnapshot, 1000, false, "post-release", true);
	}

	protected void TB_PVE_M1_1_OnCaptureInterrupt(SCR_Faction faction, SCR_SeizingComponent component)
	{
		PrintFormat("%1 CaptureInterrupt faction=%2 server=%3 dedicated=%4 proxy=%5", TB_PVE_M1_1_LOG, faction.GetFactionKey(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		TB_PVE_M1_4A_LogEligibilitySnapshot("CaptureInterrupt", true);
		TB_PVE_M1_4A_ReleaseMaximumLOD("CaptureInterrupt");
		if (TB_PVE_M1_1_RUN_INTERRUPT_REGRESSION && m_TB_PVE_M1_1_InterruptRequested)
		{
			PrintFormat("%1 PASS M1.1B interruption regression vanilla capture interrupted after AI left the seizing radius", TB_PVE_M1_1_LOG);
			GetGame().GetCallqueue().Remove(TB_PVE_M1_1_DiagnosticTick);
		}
	}

	protected void TB_PVE_M1_1_OnCaptureFinish(SCR_Faction faction, SCR_SeizingComponent component)
	{
		PrintFormat("%1 CaptureFinish faction=%2 server=%3 dedicated=%4 proxy=%5", TB_PVE_M1_1_LOG, faction.GetFactionKey(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		TB_PVE_M1_4A_LogEligibilitySnapshot("CaptureFinish", true);
		TB_PVE_M1_4A_ReleaseMaximumLOD("CaptureFinish");
	}

	protected void TB_PVE_M1_1_OnCaptureStateChanged(SCR_EBaseCaptureState state)
	{
		PrintFormat("%1 CaptureStateChanged state=%2 server=%3 dedicated=%4 proxy=%5", TB_PVE_M1_1_LOG, state, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
		TB_PVE_M1_4A_LogEligibilitySnapshot("CaptureStateChanged", true);
	}

	protected void TB_PVE_M1_1_OnSystemBaseFactionChanged(SCR_MilitaryBaseComponent base, Faction newFaction)
	{
		if (base != m_TB_PVE_M1_1_TargetBase)
			return;

		FactionKey newKey = "<none>";
		if (newFaction)
			newKey = newFaction.GetFactionKey();
		PrintFormat("%1 MilitaryBaseSystem.OnBaseFactionChanged newOwner=%2 server=%3 dedicated=%4 proxy=%5", TB_PVE_M1_1_LOG, newKey, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
	}

	protected void TB_PVE_M1_1_OnExtendedBaseFactionChanged(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)
	{
		if (base != m_TB_PVE_M1_1_TargetBase)
			return;

		FactionKey previousKey = "<none>";
		FactionKey newKey = "<none>";
		if (previousFaction)
			previousKey = previousFaction.GetFactionKey();
		if (newFaction)
			newKey = newFaction.GetFactionKey();
		PrintFormat("%1 CampaignMilitaryBase.OnFactionChangedExtended previous=%2 new=%3 server=%4 dedicated=%5 proxy=%6", TB_PVE_M1_1_LOG, previousKey, newKey, Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());

	}
}
