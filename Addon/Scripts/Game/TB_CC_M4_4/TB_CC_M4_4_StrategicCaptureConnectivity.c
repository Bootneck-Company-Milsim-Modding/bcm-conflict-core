// M4 graph campaigns use graph authorization as their strategic connectivity
// model. This seam changes only the final playable-faction HQ-radio predicate
// in vanilla SCR_CampaignSeizingComponent. All physical actor evaluation,
// presence buckets, player requirements, superiority, capture state, timers,
// interruption, CaptureFinish, ownership, callbacks and replication remain the
// vanilla implementation below.
modded class SCR_CampaignSeizingComponent
{
	[Attribute(defvalue: "2", category: "BCM Capture", desc: "Required attacker-to-hostile ratio when hostiles are present.")]
	protected int m_iBCMCaptureRatio;
	[Attribute(defvalue: "2", category: "BCM Capture", desc: "Required attacker-minus-hostile margin when hostiles are present.")]
	protected int m_iBCMCaptureMargin;
	protected SCR_Faction m_BCMInterruptedFaction;
	protected ref map<FactionKey, string> m_mBCMCaptureTrace = new map<FactionKey, string>();
	protected bool TB_CC_UseThreeFactionCapture()
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		return policy && m_Base && !policy.GetId(m_Base).IsEmpty();
	}

	override protected void OnPrevailingFactionChanged()
	{
		if (!TB_CC_UseThreeFactionCapture()) { super.OnPrevailingFactionChanged(); return; }
		bool resuming = m_PrevailingFaction && m_PrevailingFaction == m_BCMInterruptedFaction;
		float retainedMilliseconds;
		if (resuming) retainedMilliseconds = m_fInterruptedCaptureDuration;
		bool interrupting = !m_PrevailingFaction && m_fSeizingEndTimestamp != 0;
		if (interrupting) m_BCMInterruptedFaction = m_PrevailingFactionPrevious;
		// Vanilla gradual reset decays retained progress while contested. BCM
		// retains it without decay, using the same native timestamps/callbacks.
		m_bGradualTimerReset = false;
		super.OnPrevailingFactionChanged();
		if (resuming && retainedMilliseconds > 0)
		{
			m_fSeizingStartTimestamp = m_fSeizingStartTimestamp.PlusMilliseconds(-retainedMilliseconds);
			m_fSeizingEndTimestamp = m_fSeizingEndTimestamp.PlusMilliseconds(-retainedMilliseconds);
			OnSeizingTimestampChanged(); Replication.BumpMe();
		}
		if (m_PrevailingFaction)
		{
			m_fInterruptedCaptureDuration = 0;
			m_BCMInterruptedFaction = null;
		}
	}
	void TB_CC_M4_4_ConfigurePvECaptureRules()
	{
		// The production PvE contract permits AI-only conventional captures.
		// These are ordinary vanilla configuration fields; their checks remain
		// in EvaluatePrevailingFaction and are not bypassed by the radio seam.
		m_bIgnoreNonPlayableAttackers = false;
		m_bCapturingRequiresPlayer = false;
	}

	override protected void EvaluatePrevailingFaction()
	{
		if (!m_TagSystem)
			return;

		float delay;
		if (m_bCharacterPresent)
			delay = Math.RandomFloatInclusive(TRIGGER_CHECK_PERIOD_ACTIVE, TRIGGER_CHECK_PERIOD_ACTIVE + (TRIGGER_CHECK_PERIOD_ACTIVE * 0.2));
		else
			delay = Math.RandomFloatInclusive(TRIGGER_CHECK_PERIOD_IDLE, TRIGGER_CHECK_PERIOD_IDLE + (TRIGGER_CHECK_PERIOD_IDLE * 0.2));

		GetGame().GetCallqueue().CallLater(EvaluatePrevailingFaction, delay * 1000);

		array<IEntity> presentEntities = {};
		int presentEntitiesCount = m_TagSystem.GetTagsInRange(presentEntities, GetOwner().GetOrigin(), m_iRadius, ETagCategory.Character);
		m_bCharacterPresent = presentEntitiesCount != 0;

		if (!m_bCharacterPresent)
		{
			if (m_eCaptureState != SCR_EBaseCaptureState.NONE)
			{
				m_eCaptureState = SCR_EBaseCaptureState.NONE;
				Rpc(RpcDo_OnCaptureStateChanged);
				RpcDo_OnCaptureStateChanged();
			}

			if (m_PrevailingFaction)
			{
				m_PrevailingFactionPrevious = m_PrevailingFaction;
				m_iSeizingCharacters = 0;
				m_PrevailingFaction = null;
				OnPrevailingFactionChanged();
			}
			return;
		}

		map<SCR_Faction, int> factionsPresence = new map<SCR_Faction, int>();
		map<SCR_Faction, bool> factionsPlayerPresence = new map<SCR_Faction, bool>();
		SCR_Faction evaluatedEntityFaction;
		int factionCount;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		bool responseHumanUS;

		for (int i = 0; i < presentEntitiesCount; i++)
		{
			IEntity entity = presentEntities[i];
			if (m_bDeleteDisabledAIs && IsDisabledAI(entity))
			{
				RplComponent.DeleteRplEntity(entity, false);
				continue;
			}

			// Count combatants before strategic attacker authorization. Otherwise
			// an illegal attacking faction disappears from hostile contestation.
			if (TB_CC_UseThreeFactionCapture()) evaluatedEntityFaction = TB_PVE_M2_5_EvaluateVanillaActor(entity);
			else evaluatedEntityFaction = EvaluateEntityFaction(entity);
			if (!evaluatedEntityFaction)
				continue;
			if (evaluatedEntityFaction.GetFactionKey() == "US" && playerManager && playerManager.GetPlayerIdFromControlledEntity(entity) != 0) responseHumanUS = true;

			factionCount = factionsPresence.Get(evaluatedEntityFaction);
			if (factionCount == 0)
				factionsPresence.Insert(evaluatedEntityFaction, 1);
			else
				factionsPresence.Set(evaluatedEntityFaction, factionCount + 1);

			if (m_bCapturingRequiresPlayer && playerManager.GetPlayerIdFromControlledEntity(entity) != 0)
				factionsPlayerPresence.Set(evaluatedEntityFaction, true);
		}

		TB_CC_M4XP_NativeArlandComponent responsePolicy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (responsePolicy) responsePolicy.ObserveResponsePresence(SCR_CampaignMilitaryBaseComponent.Cast(m_Base), factionsPresence, responseHumanUS);
		int totalFactionPresenceCount, maxFactionPresence;
		m_iContestingFactionsCount = 0;
		bool multipleMaxPresence;
		foreach (SCR_Faction faction, int presence : factionsPresence)
		{
			totalFactionPresenceCount += presence;
			m_iContestingFactionsCount++;
			if (presence > maxFactionPresence)
			{
				maxFactionPresence = presence;
				multipleMaxPresence = false;
			}
			else if (presence == maxFactionPresence)
				multipleMaxPresence = true;
		}

		Replication.BumpMe();
		SCR_Faction defenderFaction = m_Base.GetCampaignFaction();
		if (defenderFaction && !factionsPresence.Contains(defenderFaction))
			factionsPresence.Insert(defenderFaction, 0);

		m_bDeleteDisabledAIs = false;
		SCR_Faction prevailingFaction;
		SCR_CampaignFaction campaignFaction;
		int curSeizingCharacters;
		SCR_EBaseCaptureState baseCaptureState = SCR_EBaseCaptureState.NONE;

		foreach (SCR_Faction faction, int presence : factionsPresence)
		{
			if (faction == defenderFaction)
			{
				if (presence * 2 < totalFactionPresenceCount && totalFactionPresenceCount - presence >= m_iMinimumCharactersForContest && m_iMinimumCharactersForContest != -1)
				{
					if (baseCaptureState == SCR_EBaseCaptureState.NONE)
						baseCaptureState = SCR_EBaseCaptureState.CONTESTED;
				}
			}
			else
			{
				if (TB_CC_UseThreeFactionCapture())
				{
					int hostileCount;
					int hostileUS; int hostileUSSR; int hostileFIA;
					foreach (SCR_Faction other, int otherPresence : factionsPresence)
					{
						if (!faction.IsFactionEnemy(other)) continue;
						hostileCount += otherPresence;
						if (other.GetFactionKey() == "US") hostileUS += otherPresence;
						if (other.GetFactionKey() == "USSR") hostileUSSR += otherPresence;
						if (other.GetFactionKey() == "FIA") hostileFIA += otherPresence;
					}
					bool superior = presence >= 1;
					if (hostileCount > 0) superior = presence >= hostileCount * Math.Max(m_iBCMCaptureRatio, 1) && presence - hostileCount >= Math.Max(m_iBCMCaptureMargin, 1);
					string trace = faction.GetFactionKey();
					trace += ":" + presence.ToString();
					trace += ":" + hostileCount.ToString();
					trace += ":" + superior.ToString();
					if (trace != m_mBCMCaptureTrace.Get(faction.GetFactionKey()))
					{
						m_mBCMCaptureTrace.Set(faction.GetFactionKey(), trace);
						PrintFormat("[TB_CC_CAPTURE] base=%1 faction=%2 attackers=%3 hostileUS=%4 hostileUSSR=%5 hostileFIA=%6 totalHostile=%7 superior=%8", GetOwner().GetName(), faction.GetFactionKey(), presence, hostileUS, hostileUSSR, hostileFIA, hostileCount, superior);
					}
					if (!superior) { if (hostileCount > 0) baseCaptureState = SCR_EBaseCaptureState.CONTESTED; continue; }
				}
				if (!faction.CanCaptureBases())
					continue;
				if (m_bIgnoreNonPlayableAttackers && !faction.IsPlayable())
					continue;
				if (m_bCapturingRequiresPlayer && !factionsPlayerPresence.Get(faction))
					continue;
				if (!TB_CC_UseThreeFactionCapture() && (presence != maxFactionPresence || multipleMaxPresence))
					continue;

			campaignFaction = SCR_CampaignFaction.Cast(faction);
			bool vanillaRadioAllows = campaignFaction && (!campaignFaction.IsPlayable() || m_Base.IsHQRadioTrafficPossible(campaignFaction));
			bool graphAllows = campaignFaction && campaignFaction.IsPlayable() && TB_CC_M4_4_ArlandGraphCampaignComponent.CanBypassPlayableRadioGate(faction, m_Base);
			// Native full Arland has two physical HQ candidates. The selected MOB
			// must remain protected even when vanilla radio would admit USSR AI;
			// all registered native positions use the same strategic authorization.
			bool nativeFullBase = m_Base && m_Base.GetOwner() && m_Base.GetOwner().GetName().StartsWith("TB_CC_NATIVE_");
			bool strategicAllows = nativeFullBase && TB_CC_M4_4_ArlandGraphCampaignComponent.CanFactionCaptureBase(faction, m_Base);
			if ((nativeFullBase && strategicAllows) || (!nativeFullBase && (vanillaRadioAllows || graphAllows)))
				{
					prevailingFaction = faction;
					curSeizingCharacters = presence;
					baseCaptureState = SCR_EBaseCaptureState.CAPTURING;
				}
			}
		}

		if (prevailingFaction)
		{
			baseCaptureState = SCR_EBaseCaptureState.CAPTURING;
			int relevantHostiles;
			if (TB_CC_UseThreeFactionCapture())
			{
				foreach (SCR_Faction otherFaction, int otherCount : factionsPresence)
					if (prevailingFaction.IsFactionEnemy(otherFaction)) relevantHostiles += otherCount;
				curSeizingCharacters = Math.Clamp(curSeizingCharacters - relevantHostiles, 1, m_iMaximumSeizingCharacters);
			}
			else curSeizingCharacters = Math.Min((2 * curSeizingCharacters) - totalFactionPresenceCount, m_iMaximumSeizingCharacters);
		}

		if (baseCaptureState != m_eCaptureState)
		{
			m_eCaptureState = baseCaptureState;
			Rpc(RpcDo_OnCaptureStateChanged);
			RpcDo_OnCaptureStateChanged();
		}

		if (prevailingFaction != m_PrevailingFaction)
		{
			m_iSeizingCharacters = curSeizingCharacters;
			m_PrevailingFactionPrevious = m_PrevailingFaction;
			m_PrevailingFaction = prevailingFaction;
			OnPrevailingFactionChanged();
		}
		else if (prevailingFaction && curSeizingCharacters != m_iSeizingCharacters)
		{
			m_iSeizingCharacters = curSeizingCharacters;
			RefreshSeizingTimer();
		}
	}
}
