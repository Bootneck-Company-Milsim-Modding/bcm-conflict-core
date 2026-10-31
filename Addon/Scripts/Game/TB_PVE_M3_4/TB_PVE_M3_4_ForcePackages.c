class TB_PVE_M3_4_ForcePackageEntry
{
	string m_sRole;
	ResourceName m_sGroupPrefab;
	int m_iQuantity;

	void TB_PVE_M3_4_ForcePackageEntry(string role, ResourceName groupPrefab, int quantity)
	{
		m_sRole = role; m_sGroupPrefab = groupPrefab; m_iQuantity = quantity;
	}
}

class TB_PVE_M3_4_ForcePackage
{
	string m_sPackageId;
	FactionKey m_sFactionKey;
	string m_sRole;
	ref array<ref TB_PVE_M3_4_ForcePackageEntry> m_aEntries = {};

	void TB_PVE_M3_4_ForcePackage(string packageId, FactionKey factionKey, string role)
	{
		m_sPackageId = packageId; m_sFactionKey = factionKey; m_sRole = role;
	}

	int GetExpectedGroupCount()
	{
		int result;
		foreach (TB_PVE_M3_4_ForcePackageEntry entry : m_aEntries) result += entry.m_iQuantity;
		return result;
	}
}

class TB_PVE_M3_4_ForcePackageRegistry
{
	protected static int s_iUSSRLightGroups = 2;
	protected static int s_iUSSRStandardGroups = 4;
	protected static int s_iUSSRReinforcedGroups = 6;
	protected static int s_iUSSRHeavyGroups = 8;
	static void ConfigureUSSRPackages(int light, int standard, int reinforced, int heavy)
	{
		s_iUSSRLightGroups = Math.Clamp(light, 1, 8);
		s_iUSSRStandardGroups = Math.Clamp(standard, s_iUSSRLightGroups, 8);
		s_iUSSRReinforcedGroups = Math.Clamp(reinforced, s_iUSSRStandardGroups, 8);
		s_iUSSRHeavyGroups = Math.Clamp(heavy, s_iUSSRReinforcedGroups, 8);
	}
	protected static int s_iFIALightGroups = 2;
	protected static int s_iFIARifleGroups = 4;
	protected static int s_iFIAMGGroups = 1;
	protected static int s_iFIAATGroups = 1;
	static void ConfigureFIAPackages(int light, int rifles, int mg, int at)
	{
		s_iFIALightGroups = Math.Clamp(light, 1, 6);
		s_iFIARifleGroups = Math.Clamp(rifles, 1, 6);
		s_iFIAMGGroups = Math.Clamp(mg, 0, 2);
		s_iFIAATGroups = Math.Clamp(at, 0, 2);
	}
	static const ResourceName USSR_DEFENDERS = "{359353A0A5A52216}Prefabs/Groups/Campaign/Group_USSR_Defenders.et";
	// Registered identities from the installed data/resourceDatabase.rdb.
	static const ResourceName FIA_DEFENDERS = "{F22EDFBEFC193357}Prefabs/Groups/Campaign/Group_FIA_Remnants.et";
	static const ResourceName FIA_MG = "{8D2035F179B9E08F}Prefabs/Groups/Campaign/Group_FIA_Remnants_MG.et";
	static const ResourceName FIA_AT = "{079F822A6E7FEA30}Prefabs/Groups/Campaign/Group_FIA_Remnants_AT.et";

	static TB_PVE_M3_4_ForcePackage GetPackage(string packageId, bool offensive = true)
	{
		TB_PVE_M3_4_ForcePackage package;
		int light = s_iUSSRLightGroups; int standard = s_iUSSRStandardGroups;
		int reinforced = s_iUSSRReinforcedGroups; int assault = s_iUSSRReinforcedGroups;
		int heavy = s_iUSSRHeavyGroups; int surge = s_iUSSRHeavyGroups;
		// Offensive tuning must not change accepted territorial defensive packages.
		if (!offensive) { light = 1; standard = 2; reinforced = 3; assault = 4; heavy = 5; surge = 6; }
		if (packageId == "FIA_LIGHT" || packageId == "FIA_STANDARD" || packageId == "FIA_REINFORCED" || packageId == "FIA_STRONGPOINT")
		{
			package = new TB_PVE_M3_4_ForcePackage(packageId, "FIA", "TERRITORIAL_MILITIA");
			int rifleGroups = s_iFIARifleGroups;
			if (packageId == "FIA_LIGHT") rifleGroups = s_iFIALightGroups;
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("MILITIA", FIA_DEFENDERS, rifleGroups));
			if (packageId == "FIA_REINFORCED" || packageId == "FIA_STRONGPOINT")
				package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("MG", FIA_MG, s_iFIAMGGroups));
			if (packageId == "FIA_STRONGPOINT")
				package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("AT", FIA_AT, s_iFIAATGroups));
		}
		else if (packageId == "LIGHT")
		{
			package = new TB_PVE_M3_4_ForcePackage("LIGHT", "USSR", "LIGHT_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, light));
		}
		else if (packageId == "STANDARD")
		{
			package = new TB_PVE_M3_4_ForcePackage("STANDARD", "USSR", "STANDARD_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, standard));
		}
		else if (packageId == "REINFORCED")
		{
			package = new TB_PVE_M3_4_ForcePackage("REINFORCED", "USSR", "REINFORCED_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, Math.Max(reinforced - 1, 0)));
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("SUPPORT_RIFLE", USSR_DEFENDERS, 1));
		}
		else if (packageId == "ASSAULT")
		{
			package = new TB_PVE_M3_4_ForcePackage("ASSAULT", "USSR", "ASSAULT_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, Math.Max(assault - 1, 0)));
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("SUPPORT_RIFLE", USSR_DEFENDERS, 1));
		}
		else if (packageId == "HEAVY")
		{
			package = new TB_PVE_M3_4_ForcePackage("HEAVY", "USSR", "HEAVY_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, Math.Max(heavy - 2, 0)));
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("SUPPORT_RIFLE", USSR_DEFENDERS, Math.Min(heavy, 2)));
		}
		else if (packageId == "SURGE")
		{
			package = new TB_PVE_M3_4_ForcePackage("SURGE", "USSR", "SURGE_INFANTRY");
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("RIFLE", USSR_DEFENDERS, Math.Max(surge - 2, 0)));
			package.m_aEntries.Insert(new TB_PVE_M3_4_ForcePackageEntry("SUPPORT_RIFLE", USSR_DEFENDERS, Math.Min(surge, 2)));
		}
		return package;
	}
}

class TB_PVE_M3_4_AssembledGroup
{
	string m_sStrategicForceId;
	int m_iStrategicGroupIndex;
	ref array<IEntity> m_aAccountedVirtualDeaths = {};
	SCR_AIGroup m_Group;
	AIWaypoint m_Waypoint;
	ref array<IEntity> m_aCharacters = {};
	vector m_vStartPosition;
	float m_fStartDistance;
	float m_fMinimumDistance;
	float m_fLastLoggedDistance = -1;
	bool m_bMeaningfulMovement;
	int m_iNaturalLOD;
	string m_sEntryRole;
	bool m_bDefensiveOrder;
	bool m_bNativeMembersPending;
	bool m_bInitialPositionsDiagnosed;
	int m_iNativeSpawnStartMs;
	bool m_bOperationSimulation;
	bool m_bBattleSimulation;
	bool m_bPriorityHeld;
	int m_iPreviousGroupPermanentLOD = -1;
	ref map<AIAgent, int> m_mPreviousAgentLOD = new map<AIAgent, int>();
	string m_sLifecycleRole = "assembly";
	vector m_vWithdrawalDestination;
	int m_iWithdrawalStartMs;
}

class TB_PVE_M3_4_ForceAssemblyService
{
	protected int m_iMaximumGroups = 8;
	protected int m_iProofMaximumGroups = -1;
	protected int m_iLastRollbackRemovedCount;

	bool AssemblePackage(string packageId, FactionKey factionKey, string targetId, vector targetPosition, TB_PVE_M3_3_TacticalStagingPolicy stagingPolicy, array<vector> observers, out array<ref TB_PVE_M3_4_AssembledGroup> assembledGroups, out string failureReason)
	{
		int startMs = System.GetTickCount();
		if (!Replication.IsServer()) { failureReason = "authority-required"; return false; }
		TB_PVE_M3_4_ForcePackage package = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(packageId);
		PrintFormat("[TB_PVE_M3_4] package-requested id=%1 faction=%2 target=%3", packageId, factionKey, targetId);
		if (!package) { failureReason = "package-not-found"; return false; }
		if (package.m_sFactionKey != factionKey || (factionKey != "USSR" && factionKey != "FIA")) { failureReason = "package-faction-mismatch"; return false; }
		int expectedGroups = package.GetExpectedGroupCount();
		PrintFormat("[TB_PVE_M3_4] package-resolved id=%1 role=%2 entries=%3 expectedGroups=%4", package.m_sPackageId, package.m_sRole, package.m_aEntries.Count(), expectedGroups);
		int maximumGroups = m_iMaximumGroups;
		if (factionKey == "FIA") maximumGroups = 6;
		if (expectedGroups < 1 || expectedGroups > maximumGroups) { failureReason = "invalid-group-count"; return false; }

		vector anchor; string stagingFailure;
		if (!stagingPolicy.FindStagingPosition(factionKey, targetId, targetPosition, observers, anchor, stagingFailure))
		{
			failureReason = "anchor-" + stagingFailure; return false;
		}

		int groupOrdinal;
		array<vector> usedPositions = {anchor};
		foreach (TB_PVE_M3_4_ForcePackageEntry entry : package.m_aEntries)
		{
			PrintFormat("[TB_PVE_M3_4] package-entry role=%1 prefab=%2 quantity=%3", entry.m_sRole, entry.m_sGroupPrefab, entry.m_iQuantity);
			for (int quantityIndex; quantityIndex < entry.m_iQuantity; quantityIndex++)
			{
				if (m_iProofMaximumGroups >= 0 && groupOrdinal >= m_iProofMaximumGroups)
				{
					failureReason = "proof-capacity-rejection"; Rollback(assembledGroups, failureReason); return false;
				}
				vector groupPosition = anchor;
				if (groupOrdinal > 0 && !stagingPolicy.FindAdditionalGroupPosition(factionKey, targetId, targetPosition, usedPositions, groupOrdinal, observers, groupPosition, stagingFailure))
				{
					failureReason = "group-staging-" + stagingFailure; Rollback(assembledGroups, failureReason); return false;
				}
				TB_PVE_M3_4_AssembledGroup record = SpawnGroup(entry, groupPosition, targetPosition, groupOrdinal, factionKey);
				if (!record)
				{
					failureReason = "group-materialization-failed"; Rollback(assembledGroups, failureReason); return false;
				}
				assembledGroups.Insert(record);
				if (groupOrdinal > 0) usedPositions.Insert(groupPosition);
				PrintFormat("[TB_PVE_M3_4] group-ownership-registered ordinal=%1 role=%2 group=%3 units=%4 position=%5 LOD=%6", groupOrdinal, entry.m_sRole, record.m_Group, record.m_Group.GetAgentsCount(), groupPosition, record.m_iNaturalLOD);
				groupOrdinal++;
			}
		}
		if (assembledGroups.Count() != expectedGroups)
		{
			failureReason = "atomic-count-mismatch"; Rollback(assembledGroups, failureReason); return false;
		}
		PrintFormat("[TB_PVE_M3_4] assembly-success package=%1 groups=%2 units=%3 durationMs=%4", packageId, assembledGroups.Count(), CountUnits(assembledGroups), System.GetTickCount() - startMs);
		return true;
	}

	protected TB_PVE_M3_4_AssembledGroup SpawnGroup(TB_PVE_M3_4_ForcePackageEntry entry, vector position, vector targetPosition, int ordinal, FactionKey factionKey)
	{
		TB_CC_WarSimulation war = TB_CC_WarSimulation.Get();
		if (war)
		{
			vector supported; string placementReason;
			if (!TB_CC_WarPlacement.Surface(position, war.Config(), supported, placementReason))
			{ PrintFormat("[TB_CC_SPAWN_DIAG] rejected phase=surface position=%1 reason=%2", position, placementReason); return null; }
			position = supported;
		}
		Resource groupResource = Resource.Load(entry.m_sGroupPrefab);
		if (!groupResource || !groupResource.IsValid()) return null;
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = position;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(groupResource, GetGame().GetWorld(), params));
		if (!group) return null;
		TB_PVE_M3_4_AssembledGroup record = new TB_PVE_M3_4_AssembledGroup();
		record.m_Group = group; record.m_vStartPosition = position; record.m_fStartDistance = vector.Distance(position, targetPosition); record.m_fMinimumDistance = record.m_fStartDistance; record.m_sEntryRole = entry.m_sRole;
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!faction) { DeleteRecord(record); return null; }
		if (group.GetFaction() != faction) group.SetFaction(faction);
		if (group.GetFaction() != faction) { DeleteRecord(record); return null; }
		if (war)
		{
			vector projected; string projectionReason;
			if (!TB_CC_WarPlacement.Project(group, position, war.Config(), projected, projectionReason))
			{ DeleteRecord(record); return null; }
			group.SetOrigin(projected); record.m_vStartPosition = projected;
			if (factionKey == "FIA")
			{
				vector delta = projected - targetPosition; delta[1] = 0; float distance = delta.Length();
				if (distance < 300 || distance > 500) { DeleteRecord(record); return null; }
			}
			if (!TB_CC_WarPlacement.Formation(group, war.Config(), projectionReason)) { DeleteRecord(record); return null; }
		}
		TB_CC_PhysicalWarService.Register(record);
		if (factionKey == "FIA" || SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy() || war)
		{
			// Same queue used by the human-proven territorial FIA defenders.
			// Native SpawnGroupMember activates control and initializes membership.
			if (group.GetSpawnImmediately() || group.m_aUnitPrefabSlots.IsEmpty()) { DeleteRecord(record); return null; }
			record.m_bNativeMembersPending = true;
			record.m_iNativeSpawnStartMs = System.GetTickCount();
			group.RequestSpawn(group.m_aUnitPrefabSlots.Count());
			return record;
		}
		int slot;
		foreach (ResourceName unitPrefab : group.m_aUnitPrefabSlots)
		{
			int row = slot / 4; int column = slot - row * 4;
			vector unitPosition = position + Vector(column * 1.5, 0, row * 1.5);
			unitPosition[1] = GetGame().GetWorld().GetSurfaceY(unitPosition[0], unitPosition[2]) + 0.25;
			params.Transform[3] = unitPosition;
			IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
			if (!character) { DeleteRecord(record); return null; }
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
			if (affiliation && affiliation.GetAffiliatedFaction() && affiliation.GetAffiliatedFaction() != faction)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(character); DeleteRecord(record); return null;
			}
			group.AddAgentFromControlledEntity(character); record.m_aCharacters.Insert(character);
			slot++;
		}
		if (group.GetAgentsCount() == 0 || group.GetAgentsCount() != group.m_aUnitPrefabSlots.Count()) { DeleteRecord(record); return null; }
		record.m_iNaturalLOD = group.GetLOD();
		if (record.m_iNaturalLOD == AIAgent.GetMaxLOD()) { DeleteRecord(record); return null; }
		return record;
	}

	void Rollback(array<ref TB_PVE_M3_4_AssembledGroup> groups, string reason)
	{
		int startMs = System.GetTickCount(); int count = groups.Count();
		if (count > 0) m_iLastRollbackRemovedCount = count;
		foreach (TB_PVE_M3_4_AssembledGroup record : groups) DeleteRecord(record);
		groups.Clear();
		PrintFormat("[TB_PVE_M3_4] assembly-rollback reason=%1 groupsRemoved=%2 durationMs=%3", reason, count, System.GetTickCount() - startMs);
	}

	static void DeleteRecord(TB_PVE_M3_4_AssembledGroup record)
	{
		if (!record) return;
		if (TB_CC_PhysicalWarService.PreserveOnCleanup(record)) return;
		if (record.m_Group && record.m_Waypoint && !record.m_Waypoint.IsDeleted()) record.m_Group.RemoveWaypoint(record.m_Waypoint);
		if (record.m_Waypoint && !record.m_Waypoint.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(record.m_Waypoint);
		foreach (IEntity character : record.m_aCharacters) if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		record.m_aCharacters.Clear();
		if (record.m_Group && !record.m_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(record.m_Group);
		record.m_Group = null; record.m_Waypoint = null;
	}

	// A queued shell is not yet an offensive force. Wait before issuing movement.
	static bool ObserveNativeMembers(TB_PVE_M3_4_AssembledGroup record, out string failure)
	{
		if (!record.m_Group || record.m_Group.IsDeleted()) { failure = "native-group-deleted"; return false; }
		if (!record.m_bNativeMembersPending) return true;
		array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent || !agent.GetControlledEntity()) continue;
			IEntity character = agent.GetControlledEntity();
			if (!record.m_aCharacters.Contains(character)) record.m_aCharacters.Insert(character);
			FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
			Faction faction; if (affiliation) faction = affiliation.GetAffiliatedFaction();
			if (faction && faction != record.m_Group.GetFaction()) { failure = "native-character-faction-mismatch"; return false; }
		}
		if (agents.Count() == record.m_Group.m_aUnitPrefabSlots.Count() && record.m_aCharacters.Count() == agents.Count())
		{
			record.m_bNativeMembersPending = false; record.m_iNaturalLOD = record.m_Group.GetLOD(); return true;
		}
		if (System.GetTickCount() - record.m_iNativeSpawnStartMs >= 30000) failure = "native-member-spawn-timeout";
		return false;
	}

	static int CountUnits(array<ref TB_PVE_M3_4_AssembledGroup> groups)
	{
		int result;
		foreach (TB_PVE_M3_4_AssembledGroup record : groups)
		{
			if (record.m_Group && !record.m_Group.IsDeleted())
				result += record.m_Group.GetAgentsCount();
		}
		return result;
	}

	void SetProofMaximumGroups(int maximumGroups) { m_iProofMaximumGroups = maximumGroups; }
	void ClearProofMaximumGroups() { m_iProofMaximumGroups = -1; }
	int GetLastRollbackRemovedCount() { return m_iLastRollbackRemovedCount; }
}
