// Production exception to vanilla Conflict's blanket HQ actor rejection.
// It restores a vanilla-valid physical actor only for the exact authored
// BLUFOR terminal HQ while the authoritative frontier exposes that endpoint to
// conventional OPFOR. Vanilla continues to own prevailing presence, timers,
// interruption, capture completion, ownership callbacks and replication.
modded class SCR_CampaignSeizingComponent
{
	void TB_CC_M4_4_EnableTerminalHQSurface(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!Replication.IsServer() || !base || !base.IsInitialized() || !base.IsHQ()) return;
		m_Base = base;
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (campaign) m_iMinimumCharactersForContest = campaign.GetCharactersNeededContesting();
		PrintFormat("[TB_CC_M4_4G] terminal-HQ-surface enabled entity=%1", base.GetOwner().GetName());
	}
	override protected SCR_Faction EvaluateEntityFaction(IEntity ent)
	{
		SCR_Faction faction = super.EvaluateEntityFaction(ent);
		if (faction || !Replication.IsServer() || !m_Base || !m_Base.IsInitialized() || !m_Base.IsHQ())
			return faction;

		// Bypass only the campaign HQ gate, never the generic vanilla actor gates.
		faction = TB_PVE_M2_5_EvaluateVanillaActor(ent);
		if (!faction || faction == m_Base.GetFaction() || !faction.CanCaptureBases())
			return null;
		if (m_bIgnoreNonPlayableAttackers && !faction.IsPlayable())
			return null;
		// Production PvE config sets this false. Fail closed if another resource
		// enables it rather than weakening the vanilla player-presence contract.
		if (m_bCapturingRequiresPlayer)
			return null;

		SCR_CampaignFaction campaignFaction = SCR_CampaignFaction.Cast(faction);
		if (campaignFaction && campaignFaction.IsPlayable() && !m_Base.IsHQRadioTrafficPossible(campaignFaction) && !TB_CC_M4_4_ArlandGraphCampaignComponent.CanBypassPlayableRadioGate(faction, m_Base))
			return null;
		if (!TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(faction, m_Base))
			return null;

		return faction;
	}
}
