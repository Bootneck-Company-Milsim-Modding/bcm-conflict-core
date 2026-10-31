// Isolated, read-only installed-resource inspection. Never attached to production.
class TB_CC_M4X_DependencyAuditComponentClass : ScriptComponentClass {}
class TB_CC_M4X_DependencyAuditComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer()) GetGame().GetCallqueue().CallLater(RunAudit, 2000);
	}
	protected void RunAudit()
	{
		array<string> names = { "ConflictBase_Base", "ConflictBase_MOB", "ConflictControlPoint", "ConflictMilitaryBase", "ConflictRelayRadio", "ConflictSourceBase", "ConflictSourceBase_Airfield", "ConflictSourceBase_T1Harbor", "ConflictSourceBase_T2Harbor", "ConflictSourceBase_T3Harbor" };
		foreach (string name : names) Inspect("Prefabs/Systems/MilitaryBase/" + name + ".et");
		Inspect("Prefabs/MP/Modes/GameMode_Base.et");
		Inspect("Prefabs/MP/Modes/Plain/GameMode_Plain.et");
		Inspect("Prefabs/MP/Modes/Conflict/GameMode_Campaign.et");
		InspectScript("Scripts/Game/GameMode/Components/SCR_CampaignSeizingComponent.c");
		InspectScript("Scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c");
		InspectScript("Scripts/Game/GameMode/SCR_GameModeCampaign.c");
		array<string> scripts = { "Faction/SCR_CampaignFaction", "GameMode/FactionManager/SCR_CampaignFactionManager", "GameMode/FactionManager/SCR_FactionManager", "GameMode/Respawn/SCR_RespawnSystemComponent", "Components/SCR_CoverageRadioComponent", "Campaign/SCR_CampaignSuppliesComponent", "Campaign/SCR_CampaignBuildingManagerComponent", "Components/Locations/SCR_CampaignMilitaryBaseMapDescriptorComponent", "GameMode/SCR_BaseGameMode", "Systems/SCR_MilitaryBaseSystem", "Systems/SCR_RadioCoverageSystem", "GameMode/Components/SCR_SeizingComponent" };
		foreach (string script : scripts) InspectScript("Scripts/Game/" + script + ".c");
		InspectScript("Prefabs/Systems/MilitaryBase/ConflictBase_MOB.et");
		InspectScript("Prefabs/Systems/MilitaryBase/ConflictBase_Base.et");
		InspectScript("Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et");
		InspectScript("Scripts/Game/Campaign/SCR_CampaignMilitaryBaseManager.c");
		InspectScript("Prefabs/AI/SCR_AIWorld_Arland.et");
		InspectScript("Prefabs/AI/SCR_AIWorld.et");
		Inspect("Prefabs/AI/SCR_AIWorld_Arland.et");
		Inspect("Prefabs/Systems/Radio/RadioManager.et");
		InspectScript("Prefabs/Systems/Radio/RadioManager.et");
		InspectScript("scripts/Game/generated/Entities/RadioManagerEntity.c");
		InspectScript("Prefabs/MP/Modes/Conflict/GameMode_Campaign.et");
		InspectScript("worlds/Arland/Arland.ent");
		Inspect("Prefabs/MP/Campaign/CampaignLoadoutManager.et");
		InspectScript("Prefabs/MP/Campaign/CampaignLoadoutManager.et");
		InspectScript("Prefabs/MP/Managers/Loadouts/LoadoutManager_USxUSSR.et");
		InspectScript("Prefabs/MP/Managers/Loadouts/LoadoutManager_Base.et");
		PrintFormat("[TB_CC_M4X_AUDIT] TERRAIN mobY=%1 obj01Y=%2 obj02Y=%3", GetGame().GetWorld().GetSurfaceY(1630, 2280), GetGame().GetWorld().GetSurfaceY(2030, 2280), GetGame().GetWorld().GetSurfaceY(2430, 2280));
		Print("[TB_CC_M4X_AUDIT] COMPLETE readOnly=1 actors=0 ownershipWrites=0");
	}
	protected void Inspect(ResourceName path)
	{
		Resource resource = Resource.Load(path);
		if (!resource || !resource.IsValid()) { PrintFormat("[TB_CC_M4X_AUDIT] UNRESOLVED %1", path); return; }
		IEntitySource source = resource.GetResource().ToEntitySource();
		if (!source) { PrintFormat("[TB_CC_M4X_AUDIT] NOT_ENTITY %1", path); return; }
		PrintFormat("[TB_CC_M4X_AUDIT] RESOURCE path=%1 resolved=%2 class=%3 components=%4", path, resource.GetResource().GetResourceName(), source.GetClassName(), source.GetComponentCount());
		for (int i; i < source.GetComponentCount(); i++)
		{
			BaseContainer component = source.GetComponent(i);
			PrintFormat("[TB_CC_M4X_AUDIT] COMPONENT path=%1 class=%2 name=%3", path, component.GetClassName(), component.GetName());
			array<string> fields = { "m_bCanBeHQ", "m_bDisableWhenUnusedAsHQ", "m_bIsControlPoint", "m_iRadius", "m_bIgnoreNonPlayableAttackers", "m_bCapturingRequiresPlayer", "m_fMinimumSeizingTime", "m_fMaximumSeizingTime", "m_fRange", "m_bSpawnPointEnabled" };
			foreach (string field : fields)
			{
				if (component.GetVarIndex(field) < 0) continue;
				string value; component.Get(field, value);
				PrintFormat("[TB_CC_M4X_AUDIT] FIELD class=%1 field=%2 value=%3", component.GetClassName(), field, value);
			}
		}
		PrintFormat("[TB_CC_M4X_AUDIT] CHILDREN path=%1 count=%2", path, source.GetNumChildren());
	}
	protected void InspectScript(string path)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file) { PrintFormat("[TB_CC_M4X_AUDIT] SCRIPT_UNAVAILABLE path=%1", path); return; }
		string line; int number;
		while (file.ReadLine(line) >= 0)
		{
			number++;
			PrintFormat("[TB_CC_M4X_AUDIT] SCRIPT path=%1 line=%2 text=%3", path, number, line);
		}
		file.Close();
		PrintFormat("[TB_CC_M4X_AUDIT] SCRIPT_READ path=%1 lines=%2", path, number);
	}
}
