// Native-scenario-only physical force registry. This owns simulation priority
// and terminal handoffs, NEVER base ownership, targeting, firing or perception.
class TB_CC_PhysicalWarService
{
	protected static ref array<ref TB_PVE_M3_4_AssembledGroup> s_aRecords = {};
	protected static ref array<IEntity> s_aNearby = {};
	protected static int s_iNextTelemetry;
	protected static int s_iBattles;
	protected static ref array<string> s_aBattleAreas = {};
	protected static ref array<IEntity> s_aCorpses = {};
	protected static ref array<IEntity> s_aWrecks = {};
	protected static ref array<ref TB_PVE_M3_1_Operation> s_aOperations = {};
	static void RegisterOperation(TB_PVE_M3_1_Operation operation) { if (SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy() && !s_aOperations.Contains(operation)) s_aOperations.Insert(operation); }
	static void ReleaseOperation(TB_PVE_M3_1_Operation operation) { s_aOperations.RemoveItem(operation); }
	static int RetainedCorpseCount() { return s_aCorpses.Count(); }
	static void TrackGarbage(IEntity entity)
	{
		if (SCR_ChimeraCharacter.Cast(entity) && !IsLiving(entity) && !s_aCorpses.Contains(entity)) s_aCorpses.Insert(entity);
		else if (Vehicle.Cast(entity) && !s_aWrecks.Contains(entity)) s_aWrecks.Insert(entity);
	}

	static void Register(TB_PVE_M3_4_AssembledGroup record)
	{
		if (Replication.IsServer() && SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy() && !s_aRecords.Contains(record)) s_aRecords.Insert(record);
	}
	static bool IsLiving(IEntity entity)
	{
		if (!entity || entity.IsDeleted()) return false;
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
		if (!controller) return SCR_ChimeraCharacter.Cast(entity) != null;
		return controller.GetLifeState() != ECharacterLifeState.DEAD;
	}
	static vector Position(TB_PVE_M3_4_AssembledGroup record)
	{
		array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
		foreach (AIAgent agent : agents) if (agent && IsLiving(agent.GetControlledEntity())) return agent.GetControlledEntity().GetOrigin();
		return record.m_Group.GetOrigin();
	}
	static int LivingCount(TB_PVE_M3_4_AssembledGroup record)
	{
		int count;
		foreach (IEntity character : record.m_aCharacters) if (IsLiving(character)) count++;
		return count;
	}
	static bool CollectCharacter(IEntity entity)
	{
		if (IsLiving(entity)) s_aNearby.Insert(entity);
		return true;
	}
	static bool HasPhysicalHostile(TB_PVE_M3_4_AssembledGroup record, float radius)
	{
		if (!record || !record.m_Group || record.m_Group.IsDeleted()) return false;
		Faction faction = record.m_Group.GetFaction(); if (!faction) return false;
		s_aNearby.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(Position(record), radius, CollectCharacter, null, EQueryEntitiesFlags.DYNAMIC);
		foreach (IEntity entity : s_aNearby)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			Faction other; if (character) other = character.GetFaction();
			if (!other)
			{
				AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
				AIAgent agent; if (control) agent = control.GetControlAIAgent();
				SCR_AIGroup group; if (agent) group = SCR_AIGroup.Cast(agent.GetParentGroup());
				if (group) other = group.GetFaction();
			}
			if (other && faction.IsFactionEnemy(other)) return true;
		}
		return false;
	}
	static void ApplySimulation(TB_PVE_M3_4_AssembledGroup record, bool relevant)
	{
		SCR_AIGroup group = record.m_Group; if (!group || group.IsDeleted()) return;
		if (relevant)
		{
			if (!record.m_bPriorityHeld)
			{
				record.m_iPreviousGroupPermanentLOD = group.GetPermanentLOD(); record.m_bPriorityHeld = true;
				PrintFormat("[TB_CC_PHYSICAL_WAR] simulation-acquire group=%1 role=%2 battle=%3 previousLOD=%4", group, record.m_sLifecycleRole, record.m_bBattleSimulation, group.GetLOD());
			}
			group.SetPermanentLOD(0);
			array<AIAgent> agents = {}; group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !IsLiving(agent.GetControlledEntity())) continue;
				if (!record.m_mPreviousAgentLOD.Contains(agent)) record.m_mPreviousAgentLOD.Set(agent, agent.GetPermanentLOD());
				agent.SetPermanentLOD(0);
				AIControlComponent control = AIControlComponent.Cast(agent.GetControlledEntity().FindComponent(AIControlComponent));
				SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(agent.GetControlledEntity().FindComponent(SCR_CharacterControllerComponent));
				if (control && controller && controller.GetLifeState() == ECharacterLifeState.ALIVE && !control.IsAIActivated()) control.ActivateAI();
			}
		}
		else if (record.m_bPriorityHeld)
		{
			if (group.GetPermanentLOD() == 0) group.SetPermanentLOD(record.m_iPreviousGroupPermanentLOD);
			foreach (AIAgent agent, int previous : record.m_mPreviousAgentLOD) if (agent && !agent.IsDeleted() && agent.GetPermanentLOD() == 0) agent.SetPermanentLOD(previous);
			record.m_mPreviousAgentLOD.Clear(); record.m_bPriorityHeld = false;
			PrintFormat("[TB_CC_PHYSICAL_WAR] simulation-release group=%1 role=%2 nativeLOD=1", group, record.m_sLifecycleRole);
		}
	}
	static void SetRole(TB_PVE_M3_4_AssembledGroup record, string role, bool moving)
	{
		Register(record); record.m_sLifecycleRole = role; record.m_bOperationSimulation = moving;
		ApplySimulation(record, moving || record.m_bBattleSimulation);
	}
	static void Withdraw(TB_PVE_M3_4_AssembledGroup record)
	{
		if (!record.m_Group || record.m_Group.IsDeleted()) return;
		Register(record);
		if (record.m_sLifecycleRole == "withdrawal" || record.m_sLifecycleRole == "regroup") return;
		if (record.m_Waypoint && !record.m_Waypoint.IsDeleted())
		{
			record.m_Group.RemoveWaypoint(record.m_Waypoint); SCR_EntityHelper.DeleteEntityAndChildren(record.m_Waypoint);
		}
		record.m_Waypoint = null; record.m_bDefensiveOrder = false;
		record.m_vWithdrawalDestination = record.m_vStartPosition;
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (policy && record.m_Group.GetFaction()) record.m_vWithdrawalDestination = policy.FindWithdrawalDestination(record.m_Group.GetFaction().GetFactionKey(), Position(record), record.m_vStartPosition);
		record.m_iWithdrawalStartMs = System.GetTickCount();
		Resource resource = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (resource && resource.IsValid())
		{
			EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = record.m_vWithdrawalDestination;
			record.m_Waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
			if (record.m_Waypoint) record.m_Group.AddWaypoint(record.m_Waypoint);
		}
		SetRole(record, "withdrawal", true);
		PrintFormat("[TB_CC_PHYSICAL_WAR] withdrawal group=%1 destination=%2 living=%3 order=%4 teleport=0", record.m_Group, record.m_vWithdrawalDestination, LivingCount(record), record.m_Waypoint != null);
	}
	static bool PreserveOnCleanup(TB_PVE_M3_4_AssembledGroup record)
	{
		if (!Replication.IsServer() || !SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy()) return false;
		// Native queue may still be adding members. Keep ownership of the shell.
		if (LivingCount(record) > 0 || record.m_bNativeMembersPending) { Withdraw(record); return true; }
		ApplySimulation(record, false);
		// Characters were separate world-spawned entities, not group children.
		// Their native OnDeath -> GarbageSystem insertion owns corpse lifetime.
		foreach (IEntity character : record.m_aCharacters) if (character && !character.IsDeleted()) TrackGarbage(character);
		record.m_aCharacters.Clear();
		if (record.m_Group && record.m_Group.GetChildren()) { SetRole(record, "exhausted", false); return true; }
		return false;
	}
	static void Tick(TB_CC_M4XP_NativeArlandComponent policy)
	{
		if (!Replication.IsServer()) return;
		s_iBattles = 0;
		s_aBattleAreas.Clear();
		for (int index = s_aRecords.Count() - 1; index >= 0; index--)
		{
			TB_PVE_M3_4_AssembledGroup record = s_aRecords[index];
			if (!record.m_Group || record.m_Group.IsDeleted()) { s_aRecords.Remove(index); continue; }
			array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
			foreach (AIAgent agent : agents) if (agent && agent.GetControlledEntity() && !record.m_aCharacters.Contains(agent.GetControlledEntity())) record.m_aCharacters.Insert(agent.GetControlledEntity());
			// Once per completed group, not per frame or per map-specific position.
			// Actual member positions may differ from the group shell's spawn point.
			if (!record.m_bNativeMembersPending && !record.m_bInitialPositionsDiagnosed && !record.m_aCharacters.IsEmpty())
			{
				foreach (IEntity member : record.m_aCharacters)
				{
					if (!member || member.IsDeleted()) continue;
					vector memberPosition = member.GetOrigin();
					float terrainY = GetGame().GetWorld().GetSurfaceY(memberPosition[0], memberPosition[2]);
					PrintFormat("[TB_CC_SPAWN_DIAG] group=%1 member=%2 role=%3 requestedGroupPosition=%4 actualMemberPosition=%5 terrainY=%6 deltaY=%7 observationOnly=1 collisionValidity=UNVERIFIED", record.m_Group, member, record.m_sLifecycleRole, record.m_vStartPosition, memberPosition, terrainY, memberPosition[1] - terrainY);
				}
				record.m_bInitialPositionsDiagnosed = true;
			}
			if (record.m_bNativeMembersPending && record.m_sLifecycleRole == "withdrawal")
			{
				string failure; TB_PVE_M3_4_ForceAssemblyService.ObserveNativeMembers(record, failure);
				if (!failure.IsEmpty()) record.m_bNativeMembersPending = false;
			}
			if (record.m_sLifecycleRole == "withdrawal" && (vector.Distance(Position(record), record.m_vWithdrawalDestination) <= 30 || System.GetTickCount() - record.m_iWithdrawalStartMs >= policy.GetWithdrawalLifetimeSeconds() * 1000))
			{
				TB_CC_TerritorialOccupationService.SetDefensiveOrder(record, Position(record)); SetRole(record, "regroup", false);
			}
			bool battle = LivingCount(record) > 0 && HasPhysicalHostile(record, policy.GetCombatSimulationRadius());
			if (battle)
			{
				s_iBattles++;
				string area = policy.GetNearestBattleArea(Position(record));
				if (!s_aBattleAreas.Contains(area)) s_aBattleAreas.Insert(area);
			}
			if (battle != record.m_bBattleSimulation) PrintFormat("[TB_CC_PHYSICAL_WAR] battle-change group=%1 active=%2 radius=%3 physicalHostile=1 combatUnverified=1", record.m_Group, battle, policy.GetCombatSimulationRadius());
			record.m_bBattleSimulation = battle;
			TB_CC_WarSimulation war = TB_CC_WarSimulation.Get();
			if (war && record.m_sStrategicForceId.IsEmpty() && !record.m_bNativeMembersPending && LivingCount(record) > 0 && record.m_Group.GetFaction())
				war.AdoptRetained(record, policy.WarNearestOwnedId(record.m_Group.GetFaction().GetFactionKey(), Position(record)));
			bool playerRelevant = war && war.Near(Position(record), war.Config().m_fPhysicalRadius);
			ApplySimulation(record, battle || record.m_bOperationSimulation || playerRelevant);
		}
		if (System.GetTickCount() < s_iNextTelemetry) return;
		s_iNextTelemetry = System.GetTickCount() + 30000;
		int living; int fia; int ussr; int garrisons; int offensive; int occupation;
		foreach (TB_PVE_M3_4_AssembledGroup record : s_aRecords)
		{
			int count = LivingCount(record); living += count;
			Faction faction = record.m_Group.GetFaction();
			if (faction && faction.GetFactionKey() == "FIA") fia += count;
			if (faction && faction.GetFactionKey() == "USSR") ussr += count;
			if (record.m_sLifecycleRole == "garrison") garrisons++;
			if (record.m_sLifecycleRole == "offensive") offensive++;
			if (record.m_sLifecycleRole == "occupation" || record.m_sLifecycleRole == "consolidation") occupation++;
			foreach (IEntity character : record.m_aCharacters) if (character && !character.IsDeleted() && !IsLiving(character)) TrackGarbage(character);
		}
		for (int index = s_aCorpses.Count() - 1; index >= 0; index--) if (!s_aCorpses[index] || s_aCorpses[index].IsDeleted()) s_aCorpses.Remove(index);
		for (int index = s_aWrecks.Count() - 1; index >= 0; index--) if (!s_aWrecks[index] || s_aWrecks[index].IsDeleted()) s_aWrecks.Remove(index);
		int fiaOperations; int ussrOperations; int wrecks;
		foreach (IEntity vehicle : s_aWrecks)
		{
			DamageManagerComponent damage = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
			if (damage && damage.IsDestroyed()) wrecks++;
		}
		foreach (TB_PVE_M3_1_Operation operation : s_aOperations)
		{
			if (operation.IsTerminal()) continue;
			if (operation.GetOperationFaction() == "FIA") fiaOperations++;
			if (operation.GetOperationFaction() == "USSR") ussrOperations++;
		}
		PrintFormat("[TB_CC_PHYSICAL_WAR] living=%1 FIA=%2 USSR=%3 garrisonGroups=%4 offensiveGroups=%5 occupationGroups=%6 battleGroups=%7 retainedCorpses=%8", living, fia, ussr, garrisons, offensive, occupation, s_iBattles, s_aCorpses.Count());
		// Read-only counters: engine AI admission and GM placement are distinct.
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld)
		{
			PrintFormat("[TB_CC_BUDGET] activeCurrent=%1 activeMaximum=%2 groupLessAdmission=%3 counters=engine activeLimitUnchanged=1", aiWorld.GetCurrentNumOfActiveAIs(), aiWorld.GetLimitOfActiveAIs(), aiWorld.CanActivateGroup(null));
			ChimeraAIWorld chimeraWorld = ChimeraAIWorld.Cast(aiWorld);
			if (chimeraWorld) PrintFormat("[TB_CC_BUDGET] FIALimitedCurrent=%1 FIALimitedMaximum=%2 USSRLimitedCurrent=%3 USSRLimitedMaximum=%4 counters=engine negativeMaximum=unlimited-or-undefined", chimeraWorld.GetCurrentAmountOfLimitedAIsForFaction("FIA"), chimeraWorld.GetAILimitForFaction("FIA"), chimeraWorld.GetCurrentAmountOfLimitedAIsForFaction("USSR"), chimeraWorld.GetAILimitForFaction("USSR"));
		}
		PrintFormat("[TB_CC_PHYSICAL_WAR] FIAOperations=%1 USSROperations=%2 GCTrackedVehicles=%3 trackedWrecks=%4 activeBattleAreas=%5 serverFPSsample=%6 metricsBoundary=BCM-and-GC-tracked simulationFPS=UNAVAILABLE engineBudget=UNAVAILABLE", fiaOperations, ussrOperations, s_aWrecks.Count(), wrecks, s_aBattleAreas.Count(), System.GetFPS());
	}
}

// Installed SCR_GarbageSystem is the supported script entry for GC modding.
// Preserve supply-container lifetime logic through super; native scenario only.
modded class SCR_GarbageSystem
{
	override protected float OnInsertRequested(IEntity entity, float lifetime)
	{
		float nativeLifetime = super.OnInsertRequested(entity, lifetime);
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (!policy || !Replication.IsServer()) return nativeLifetime;
		if (nativeLifetime <= 0) return nativeLifetime; // Preserve native insertion vetoes.
		TB_CC_PhysicalWarService.TrackGarbage(entity);
		if (SCR_ChimeraCharacter.Cast(entity)) return Math.Max(nativeLifetime, policy.GetCorpsePersistenceSeconds());
		if (Vehicle.Cast(entity)) return Math.Max(nativeLifetime, policy.GetWreckPersistenceSeconds());
		if (entity.FindComponent(InventoryItemComponent)) return Math.Max(nativeLifetime, policy.GetDroppedEquipmentPersistenceSeconds());
		return nativeLifetime;
	}
}

// Native attack flashing uses the attacker's colour without an ownership change.
// Keep BCM territory colour owned; native warning/task overlays remain intact.
modded class SCR_CampaignMapUIBase
{
	protected string m_sTBCCLastOwnershipFlash;
	protected string m_sTBCCLastPresentedOwner;
	protected Faction m_TBCCAttackPresentation;
	protected string TBCCOrdinaryId()
	{
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (!policy || !m_Base) return "";
		string id = policy.GetId(m_Base);
		if (id.IsEmpty() || TB_CC_M4XP_CampaignRoles.GetRole(id) != TB_CC_StrategicRole.ORDINARY) return "";
		return id;
	}
	protected void TBCCLogOwnerPresentation()
	{
		string id = TBCCOrdinaryId(); if (id.IsEmpty()) return;
		Faction owner = m_Base.GetFaction(); if (!owner) return;
		Faction attacker = m_Base.GetCapturingFaction();
		if (!attacker) attacker = m_TBCCAttackPresentation;
		string attackerKey = "NONE"; if (attacker) attackerKey = attacker.GetFactionKey();
		string ownerKey = owner.GetFactionKey();
		bool contested = attacker && attacker != owner;
		string key = ownerKey + ":" + attackerKey;
		if (key == m_sTBCCLastOwnershipFlash) return;
		bool ownershipChanged = !m_sTBCCLastPresentedOwner.IsEmpty() && ownerKey != m_sTBCCLastPresentedOwner;
		PrintFormat("[TB_CC_MAP_OWNER] stableId=%1 authoritativeOwner=%2 attackerFaction=%3 presentationFaction=%4 ownershipChanged=%5 contested=%6 revision=%7", id, ownerKey, attackerKey, m_sFactionKey, ownershipChanged, contested, TB_PVE_M2_3_FrontierQuery.GetRevision());
		m_sTBCCLastOwnershipFlash = key; m_sTBCCLastPresentedOwner = ownerKey;
	}
	override protected void SetBaseImage()
	{
		string id = TBCCOrdinaryId();
		if (!id.IsEmpty() && m_Base.GetFaction()) m_sFactionKey = m_Base.GetFaction().GetFactionKey();
		super.SetBaseImage();
		TBCCLogOwnerPresentation();
	}
	override void SetImage(string image, string imageset)
	{
		string id = TBCCOrdinaryId();
		bool ordinary = !id.IsEmpty();
		if (ordinary && m_Base.GetFaction()) m_sFactionKey = m_Base.GetFaction().GetFactionKey();
		super.SetImage(image, imageset);
		if (ordinary) SetBaseIconFactionColor(m_Base.GetFaction());
	}
	override void SetBaseIconFactionColor(Faction faction)
	{
		string id = TBCCOrdinaryId();
		if (!id.IsEmpty())
		{
			faction = m_Base.GetFaction();
			if (faction) m_sFactionKey = faction.GetFactionKey();
		}
		super.SetBaseIconFactionColor(faction);
	}
	override void FlashBaseIcon(Faction faction = null, bool changeToDefault = false)
	{
		string id = TBCCOrdinaryId();
		if (!id.IsEmpty())
		{
			// Do not pass through native radio/Unknown or attacker-colour flashing.
			// Native SetCaptureWarning remains a separate, unchanged overlay.
			m_TBCCAttackPresentation = faction;
			SetBaseIconFactionColor(m_Base.GetFaction());
			TBCCLogOwnerPresentation(); return;
		}
		super.FlashBaseIcon(faction, changeToDefault);
	}
}
