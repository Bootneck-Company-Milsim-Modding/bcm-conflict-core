// M1.3 experimental marker. The inherited prefab keeps its vanilla campaign
// spawn point (and therefore the base's supply/reference contract) but authors
// it disabled. This UI filter only hides marked forward-base spawn points; the
// vanilla enabled flag remains the authoritative server-side spawn gate.
[ComponentEditorProps(category: "GameScripted/TB PvE", description: "Marks an experimental MOB-only forward base")]
class TB_PVE_M1_3_NoPlayerDeploymentMarkerClass : ScriptComponentClass
{
}

class TB_PVE_M1_3_NoPlayerDeploymentMarker : ScriptComponent
{
}

modded class SCR_CampaignSpawnPointGroup
{
	override bool IsSpawnPointVisibleForPlayer(int pid)
	{
		if (!super.IsSpawnPointVisibleForPlayer(pid))
			return false;

		IEntity parent = GetParent();
		if (!parent)
			return true;
		if (!TB_CC_M4_4_ArlandGraphCampaignComponent.IsPlayerDeploymentAllowed(parent))
			return false;

		return !parent.FindComponent(TB_PVE_M1_3_NoPlayerDeploymentMarker);
	}
}
