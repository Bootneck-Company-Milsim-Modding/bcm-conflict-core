// Read-only native radio integration. No range, power, ownership or source writes.
// The small accessor exposes registration evidence unavailable through public API.
modded class SCR_RadioCoverageSystem
{
	bool TB_CC_IsRadioRegistered(SCR_CoverageRadioComponent radio)
	{
		return radio && m_aRadioComponents.Contains(radio);
	}
	bool TB_CC_IsCoverageSource(SCR_CoverageRadioComponent radio)
	{
		return radio && m_mEncryptionSignalSources.Get(radio.GetEncryptionKey()) == radio;
	}
}

class TB_CC_NativeRadioObserver
{
	protected TB_CC_M4_1_StrategicPositionRegistry m_Registry;
	protected SCR_CampaignFaction m_Faction;
	protected SCR_RadioCoverageSystem m_System;
	protected ref ScriptInvokerVoid m_OnChanged = new ScriptInvokerVoid();
	protected ref map<string, string> m_LastNodeState = new map<string, string>();
	protected bool m_bRunning;
	protected bool m_bPending;
	protected string m_sPendingNode;
	protected string m_sWaitingSignature;

	ScriptInvokerVoid GetOnChanged() { return m_OnChanged; }

	void Start(TB_CC_M4_1_StrategicPositionRegistry registry, SCR_CampaignFaction faction)
	{
		Stop();
		if (!Replication.IsServer() || !registry || !faction) return;
		m_Registry = registry;
		m_Faction = faction;
		m_System = SCR_RadioCoverageSystem.GetInstance();
		m_bRunning = true;
		if (m_System) m_System.GetOnCoverageChanged().Insert(OnNativeCoverageChanged);
		// Also observes delayed building/provider creation, which need not change coverage.
		GetGame().GetCallqueue().CallLater(TraceNodes, 2000, true);
		TraceNodes();
	}

	void Stop()
	{
		m_bRunning = false;
		m_bPending = false;
		if (m_System) m_System.GetOnCoverageChanged().Remove(OnNativeCoverageChanged);
		GetGame().GetCallqueue().Remove(FlushCoverageChange);
		GetGame().GetCallqueue().Remove(TraceNodes);
		GetGame().GetCallqueue().Remove(TraceOwnershipChange);
		m_System = null;
		m_LastNodeState.Clear();
		m_sWaitingSignature = "";
	}

	protected string ResolveNode(IEntity entity)
	{
		if (!entity || !m_Registry) return "unregistered-radio";
		array<string> ids = {};
		m_Registry.GetPositionIds(ids);
		foreach (string id : ids)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			if (resolved && resolved.GetResolvedEntity() == entity) return id;
		}
		return entity.GetName();
	}

	protected void OnNativeCoverageChanged(SCR_CoverageRadioComponent radio)
	{
		if (!m_bRunning || !Replication.IsServer() || !radio || m_bPending) return;
		m_bPending = true;
		m_sPendingNode = ResolveNode(radio.GetOwner());
		// Native FinishCoverageUpdate notifies one radio at a time. Do not select
		// against a partially updated network or re-enter the coverage traversal.
		GetGame().GetCallqueue().CallLater(FlushCoverageChange, 250, false);
	}

	protected void FlushCoverageChange()
	{
		m_bPending = false;
		if (!m_bRunning || !Replication.IsServer()) return;
		TraceNodes();
		PrintFormat("[TB_CC_RADIO] topology-changed node=%1 reason=native-coverage-changed reselect=1", m_sPendingNode);
		m_OnChanged.Invoke();
	}

	protected void TraceNodes()
	{
		if (!m_bRunning || !Replication.IsServer() || !m_Registry || !m_Faction) return;
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		array<SCR_CampaignMilitaryBaseComponent> registeredBases = {};
		if (campaign && campaign.GetBaseManager()) campaign.GetBaseManager().GetBases(registeredBases);
		array<string> ids = {};
		m_Registry.GetPositionIds(ids);
		foreach (string id : ids)
		{
			TB_CC_M4_1_ResolvedPosition resolved = m_Registry.FindPositionById(id);
			IEntity entity;
			if (resolved) entity = resolved.GetResolvedEntity();
			if (!entity || entity.IsDeleted()) continue;
			SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
			if (!base) continue;
			SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(entity.FindComponent(SCR_CoverageRadioComponent));
			array<SCR_CampaignBuildingProviderComponent> providers = {};
			base.GetBuildingProviders(providers);
			array<SCR_ServicePointComponent> antennas = {};
			base.GetServicesByType(antennas, SCR_EServicePointType.RADIO_ANTENNA);
			int onlineAntennas;
			foreach (SCR_ServicePointComponent antenna : antennas)
				if (antenna && antenna.GetServiceState() == SCR_EServicePointStatus.ONLINE) onlineAntennas++;
			FactionKey owner;
			if (base.GetFaction()) owner = base.GetFaction().GetFactionKey();
			bool receive = base.IsHQRadioTrafficPossible(m_Faction);
			bool transmit = base.IsHQRadioTrafficPossible(m_Faction, SCR_ERadioCoverageStatus.SEND);
			bool relayPresent;
			bool powered;
			bool encryptionMatches;
			bool source;
			bool selectedSource;
			bool hqEncryptionMatches;
			bool registered;
			float range;
			string upstream;
			if (radio)
			{
				SCR_CampaignMilitaryBaseComponent hq = m_Faction.GetMainBase();
				SCR_CoverageRadioComponent hqRadio;
				if (hq) hqRadio = SCR_CoverageRadioComponent.Cast(hq.GetOwner().FindComponent(SCR_CoverageRadioComponent));
				if (hqRadio) hqEncryptionMatches = hqRadio.GetEncryptionKey() == m_Faction.GetFactionRadioEncryptionKey();
				powered = radio.IsPowered();
				source = radio.IsSource();
				encryptionMatches = radio.GetEncryptionKey() == m_Faction.GetFactionRadioEncryptionKey();
				registered = m_System && m_System.TB_CC_IsRadioRegistered(radio);
				selectedSource = m_System && m_System.TB_CC_IsCoverageSource(radio);
				if (radio.TransceiversCount() > 0)
				{
					BaseTransceiver transceiver = radio.GetTransceiver(0);
					relayPresent = RelayTransceiver.Cast(transceiver) != null;
					if (transceiver) range = transceiver.GetRange();
				}
				array<SCR_CoverageRadioComponent> incoming = {};
				radio.GetRadiosInRangeOf(incoming);
				array<string> links = {};
				foreach (SCR_CoverageRadioComponent neighbour : incoming)
				{
					if (!neighbour || !neighbour.IsPowered() || neighbour.GetEncryptionKey() != m_Faction.GetFactionRadioEncryptionKey()) continue;
					int coverage = neighbour.GetCoverageByEncryption(m_Faction.GetFactionRadioEncryptionKey());
					if (coverage != SCR_ERadioCoverageStatus.RECEIVE && coverage != SCR_ERadioCoverageStatus.BOTH_WAYS) continue;
					links.Insert(ResolveNode(neighbour.GetOwner()));
				}
				links.Sort();
				foreach (string link : links) upstream += link + ";";
			}
			// Operational means this faction's powered, registered relay has receive
			// coverage. Antenna service availability is reported separately.
			bool operational = relayPresent && powered && registered && encryptionMatches && receive;
			SCR_CampaignBuildingProviderComponent master = base.GetMasterProvider();
			string providerFaction = "UNKNOWN";
			if (master && master.GetOwner())
			{
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(master.GetOwner().FindComponent(FactionAffiliationComponent));
				if (affiliation && affiliation.GetAffiliatedFaction()) providerFaction = affiliation.GetAffiliatedFaction().GetFactionKey();
			}
			string state = string.Format("owner=%1 providerPresent=%2 relayPresent=%3 relayOperational=%4 registered=%5 receiveCoverage=%6 transmitCoverage=%7 upstream=%8", owner, !providers.IsEmpty(), relayPresent, operational, registered, receive, transmit, upstream);
			state += string.Format(" baseRegistered=%1 initialized=%2 source=%3 powered=%4 encryptionMatches=%5 range=%6 selectedSource=%7", registeredBases.Contains(base), base.IsInitialized(), source, powered, encryptionMatches, range, selectedSource);
			state += string.Format(" providers=%1 masterProvider=%2 compositionPresent=%3 antennas=%4 onlineAntennas=%5 providerFaction=%6", providers.Count(), master != null, base.GetBaseBuildingComposition() != null, antennas.Count(), onlineAntennas, providerFaction);
			state += string.Format(" hqEncryptionMatches=%1 coverageReference=HQ-actual-key operationalReference=US-faction-key", hqEncryptionMatches);
			if (m_LastNodeState.Get(id) == state) continue;
			m_LastNodeState.Set(id, state);
			PrintFormat("[TB_CC_RADIO_NODE] id=%1 %2 suppliesSnapshot=%3 upstreamKind=connected-incoming-neighbours", id, state, base.GetSupplies());
		}
	}

	void ObserveOwnershipChange(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)
	{
		if (!m_bRunning || !Replication.IsServer() || !base || previousFaction == newFaction) return;
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		bool registeredBefore = m_System && m_System.TB_CC_IsRadioRegistered(radio);
		string oldOwner = "NONE";
		string newOwner = "NONE";
		if (previousFaction) oldOwner = previousFaction.GetFactionKey();
		if (newFaction) newOwner = newFaction.GetFactionKey();
		// Extended faction event precedes native ChangeRadioSettings. Observe
		// after that synchronous callback, never repair or re-enter it here.
		GetGame().GetCallqueue().CallLater(TraceOwnershipChange, 250, false, base, oldOwner, newOwner, registeredBefore);
	}

	protected void TraceOwnershipChange(SCR_CampaignMilitaryBaseComponent base, string oldOwner, string newOwner, bool registeredBefore)
	{
		if (!m_bRunning || !Replication.IsServer() || !base || !base.GetOwner() || base.GetOwner().IsDeleted()) return;
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		bool registeredAfter = m_System && m_System.TB_CC_IsRadioRegistered(radio);
		string nativeFaction = "NONE";
		if (base.GetFaction()) nativeFaction = base.GetFaction().GetFactionKey();
		string providerFaction = "UNKNOWN";
		SCR_CampaignBuildingProviderComponent master = base.GetMasterProvider();
		if (master && master.GetOwner())
		{
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(master.GetOwner().FindComponent(FactionAffiliationComponent));
			if (affiliation && affiliation.GetAffiliatedFaction()) providerFaction = affiliation.GetAffiliatedFaction().GetFactionKey();
		}
		array<SCR_ServicePointComponent> antennas = {};
		base.GetServicesByType(antennas, SCR_EServicePointType.RADIO_ANTENNA);
		int onlineAntennas;
		foreach (SCR_ServicePointComponent antenna : antennas)
			if (antenna && antenna.GetServiceState() == SCR_EServicePointStatus.ONLINE) onlineAntennas++;
		bool traffic = m_Faction && base.IsHQRadioTrafficPossible(m_Faction);
		bool operational;
		if (radio && radio.TransceiversCount() > 0 && m_Faction)
			operational = RelayTransceiver.Cast(radio.GetTransceiver(0)) != null && radio.IsPowered() && registeredAfter && radio.GetEncryptionKey() == m_Faction.GetFactionRadioEncryptionKey() && traffic;
		string state = string.Format("id=%1 oldOwner=%2 newOwner=%3 nativeBaseFaction=%4 providerFaction=%5 registeredBefore=%6 registeredAfter=%7", ResolveNode(base.GetOwner()), oldOwner, newOwner, nativeFaction, providerFaction, registeredBefore, registeredAfter);
		state += string.Format(" refreshTriggered=native-OnFactionChanged-path relayOperational=%1 antennas=%2 onlineAntennas=%3 trafficFromHQ=%4 reason=post-native-callback-snapshot", operational, antennas.Count(), onlineAntennas, traffic);
		PrintFormat("[TB_CC_RADIO_TRANSITION] %1", state);
	}

	void TraceWaiting(TB_CC_M4_3_StrategicEvaluator evaluator, string committedObjective)
	{
		if (!m_bRunning || !Replication.IsServer() || !evaluator) return;
		if (!committedObjective.IsEmpty() || evaluator.GetEvaluationState() != TB_CC_M4_3_EvaluationState.RADIO_WAIT)
		{
			m_sWaitingSignature = "";
			return;
		}
		array<string> candidates = {};
		evaluator.GetBLUFORCandidateObjectives(candidates);
		string signature;
		foreach (string id : candidates) signature += id + ";";
		if (signature == m_sWaitingSignature) return;
		m_sWaitingSignature = signature;
		// RADIO_WAIT explicitly means every graph candidate was radio-rejected.
		PrintFormat("[TB_CC_OBJECTIVE] waiting-for-radio state=WAITING_FOR_RADIO_EXTENSION frontierCandidates=%1 radioRejected=%2", candidates.Count(), candidates.Count());
	}
}
