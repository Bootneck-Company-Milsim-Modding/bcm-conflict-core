class TB_CC_M4_2_ArlandSurveyComponentClass : ScriptComponentClass {}

// Read-only inventory of the vanilla Arland Conflict bases. It exists only on
// the M4.1/M4.2 Arland survey world and never changes faction or capture state.
class TB_CC_M4_2_ArlandSurveyComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(RunSurvey, 15000);
	}

	protected void RunSurvey()
	{
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!campaign)
		{
			Print("[TB_CC_M4_2_SURVEY] FAIL gameMode=not-campaign");
			return;
		}
		array<SCR_CampaignMilitaryBaseComponent> bases = {};
		campaign.GetBaseManager().GetBases(bases);
		PrintFormat("[TB_CC_M4_2_SURVEY] BEGIN vanillaBaseCount=%1", bases.Count());
		for (int i; i < bases.Count(); i++)
		{
			SCR_CampaignMilitaryBaseComponent base = bases.Get(i);
			if (!base)
			{
				PrintFormat("[TB_CC_M4_2_SURVEY] BASE_NULL index=%1", i);
				continue;
			}
			IEntity baseEntity = base.GetOwner();
			if (!baseEntity)
			{
				PrintFormat("[TB_CC_M4_2_SURVEY] BASE_NO_ENTITY component=%1 initialized=%2", base, base.IsInitialized());
				continue;
			}
			FactionKey owner = "<none>";
			if (base.GetFaction()) owner = base.GetFaction().GetFactionKey();
			PrintFormat("[TB_CC_M4_2_SURVEY] BASE entity=%1 display=%2 position=%3 type=%4 owner=%5 initialized=%6 isHQ=%7 radius=%8 radioRange=%9",
				baseEntity.GetName(), base.GetBaseName(), baseEntity.GetOrigin(), base.GetType(), owner,
				base.IsInitialized(), base.IsHQ(), base.GetRadius(), base.GetRadioRange());
			PrintFormat("[TB_CC_M4_2_SURVEY] BASE_CONTROL_POINT entity=%1 controlPoint=%2", baseEntity.GetName(), base.IsControlPoint());
		}
		PrintFormat("[TB_CC_M4_2_SURVEY] COMPLETE vanillaBaseCount=%1 mutation=0", bases.Count());
	}
}
