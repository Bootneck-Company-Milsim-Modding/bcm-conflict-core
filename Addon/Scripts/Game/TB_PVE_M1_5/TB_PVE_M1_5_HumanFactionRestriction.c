// Production PvE faction contract. The stock Conflict faction manager remains in
// place; authority only closes the USSR player slot and its human-playable flag.
// These are vanilla faction APIs and do not change AI, catalogs, ownership,
// radio, or CanCaptureBases configuration.
modded class SCR_GameModeCampaign
{
	override void OnGameStart()
	{
		super.OnGameStart();

		if (!Replication.IsServer() || IsProxy())
			return;

		GetGame().GetCallqueue().CallLater(TB_PVE_M1_5_EnforceHumanFactionContract, 0);
	}

	protected void TB_PVE_M1_5_EnforceHumanFactionContract()
	{
		SCR_CampaignFaction opfor = GetFactionByEnum(SCR_ECampaignFaction.OPFOR);
		if (!opfor)
		{
			Print("[TB_PVE_M1_5] FAIL missing OPFOR campaign faction");
			return;
		}

		opfor.SetPlayerLimit(0);
		opfor.SetIsPlayable(false);
		opfor.InitFactionIsPlayable(false);
		PrintFormat("[TB_PVE_M1_5] ENFORCED key=%1 playable=%2 playerLimit=%3 canCaptureBases=%4 server=%5 dedicated=%6 proxy=%7", opfor.GetFactionKey(), opfor.IsPlayable(), opfor.GetPlayerLimit(), opfor.CanCaptureBases(), Replication.IsServer(), RplSession.Mode() == RplMode.Dedicated, IsProxy());
	}
}
