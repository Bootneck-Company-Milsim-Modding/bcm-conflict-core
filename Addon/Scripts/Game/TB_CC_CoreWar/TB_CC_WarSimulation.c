// One authority clock and one representation lease per force. Existing M3
// coordinators own strategic operation selection; this service owns simulation.
class TB_CC_WarSimulation
{
	protected static ref TB_CC_WarSimulation s_Instance;
	protected ref TB_CC_WarMapAdapter m_Map;
	protected ref TB_CC_WarConfig m_Config;
	protected ref array<ref TB_CC_WarForceLease> m_aForces = {};
	protected ref array<vector> m_aPlayers = {};
	protected int m_iSerial;
	protected int m_iLastTick;
	protected int m_iNextTelemetry;
	protected int m_iMaterializations;
	protected int m_iVirtualizations;
	protected int m_iDeferrals;
	protected int m_iPhysicalBattles;
	protected int m_iVirtualBattles;
	static TB_CC_WarSimulation Get() { return s_Instance; }
	static void Initialize(TB_CC_WarMapAdapter adapter, TB_CC_WarConfig config)
	{
		if (!Replication.IsServer() || s_Instance) return;
		s_Instance = new TB_CC_WarSimulation(); s_Instance.m_Map = adapter;
		s_Instance.m_Config = config; config.Normalize();
		s_Instance.m_iLastTick = System.GetTickCount();
		GetGame().GetCallqueue().CallLater(s_Instance.Tick, 1000, true);
		PrintFormat("[TB_CC_WAR] initialized physical=%1 pre=%2 virtual=%3 graceMs=%4 dwellMs=%5 activeReserve=%6 GMReserve=%7", config.m_fPhysicalRadius, config.m_fPreRadius, config.m_fVirtualRadius, config.m_iGraceMs, config.m_iMinimumDwellMs, config.m_iReservedAISlots, config.m_iReservedGMUnits);
	}
	static void Reset()
	{
		if (s_Instance) GetGame().GetCallqueue().Remove(s_Instance.Tick);
		s_Instance = null;
	}
	TB_CC_WarConfig Config() { return m_Config; }
	void Abandon(TB_CC_WarForceLease lease) { m_aForces.RemoveItem(lease); }
	void AdoptRetained(TB_PVE_M3_4_AssembledGroup record, string anchor)
	{
		if (!record.m_sStrategicForceId.IsEmpty() || record.m_bNativeMembersPending || !record.m_Group || record.m_Group.IsDeleted() || anchor.IsEmpty()) return;
		Faction faction = record.m_Group.GetFaction(); EntityPrefabData prefab = record.m_Group.GetPrefabData();
		vector destination; if (!faction || !prefab || !m_Map.Position(anchor, destination)) return;
		TB_CC_WarForceLease lease = new TB_CC_WarForceLease();
		TB_CC_WarForceState state = new TB_CC_WarForceState(); lease.m_State = state;
		state.m_sForceId = "RETAINED:" + (++m_iSerial).ToString(); state.m_sFaction = faction.GetFactionKey();
		state.m_sRole = "withdrawal"; state.m_sSourceId = anchor; state.m_sTargetId = anchor;
		state.m_vPosition = TB_CC_PhysicalWarService.Position(record); state.m_vSourcePosition = state.m_vPosition;
		state.m_vTargetPosition = destination; state.m_fSpeed = m_Config.m_fInfantrySpeed;
		state.m_iRepresentation = TB_CC_WarRepresentation.PHYSICAL; state.m_iTransitionMs = System.GetTickCount();
		TB_CC_WarGroupState group = new TB_CC_WarGroupState(); group.m_sGroupPrefab = prefab.GetPrefabName();
		group.m_sRole = record.m_sEntryRole; group.m_vPosition = state.m_vPosition;
		group.m_aSurvivorPrefabs.Copy(record.m_Group.m_aUnitPrefabSlots); group.m_iInitialStrength = group.m_aSurvivorPrefabs.Count();
		state.m_iInitialStrength = group.m_iInitialStrength; state.m_aGroups.Insert(group);
		record.m_sStrategicForceId = state.m_sForceId; record.m_iStrategicGroupIndex = 0;
		lease.m_aPhysical.Insert(record); m_aForces.Insert(lease); Snapshot(lease);
	}

	// Player-manager IDs, not an assumed leader or a fabricated participant count.
	protected void CollectPlayers()
	{
		m_aPlayers.Clear();
		PlayerManager manager = GetGame().GetPlayerManager(); if (!manager) return;
		array<int> ids = {}; manager.GetPlayers(ids);
		foreach (int id : ids)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(manager.GetPlayerControlledEntity(id));
			if (!character) continue;
			CharacterControllerComponent control = character.GetCharacterController();
			if (!control || control.GetLifeState() != ECharacterLifeState.ALIVE) continue;
			m_aPlayers.Insert(character.GetOrigin());
		}
	}
	bool Near(vector position, float radius)
	{
		foreach (vector player : m_aPlayers) if (vector.Distance(player, position) <= radius) return true;
		return false;
	}
	protected bool Relevant(TB_CC_WarForceLease lease, float radius)
	{
		TB_CC_WarForceState state = lease.m_State;
		if (state.m_sTargetId == m_Map.PlayerObjective()) return true;
		if (Near(state.m_vPosition, radius) || Near(state.m_vTargetPosition, radius)) return true;
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
			foreach (IEntity member : record.m_aCharacters) if (TB_CC_PhysicalWarService.IsLiving(member) && Near(member.GetOrigin(), radius)) return true;
		return false;
	}
	bool IsTargetPhysical(string id)
	{
		vector position; if (!m_Map.Position(id, position)) return true;
		if (id == m_Map.PlayerObjective() || Near(position, m_Config.m_fPreRadius)) return true;
		foreach (TB_CC_WarForceLease lease : m_aForces)
			if (lease.m_State.m_sTargetId == id && lease.m_State.m_iRepresentation != TB_CC_WarRepresentation.VIRTUAL && lease.m_State.m_iStrength > 0) return true;
		return false;
	}
	protected TB_CC_WarForceLease Build(string key, FactionKey faction, string role, string packageId, string target, bool offensive)
	{
		TB_PVE_M3_4_ForcePackage package = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(packageId, offensive);
		if (!package || package.m_sFactionKey != faction) return null;
		TB_CC_WarForceLease lease = new TB_CC_WarForceLease();
		TB_CC_WarForceState state = new TB_CC_WarForceState(); lease.m_State = state;
		state.m_sForceId = key + ":" + (++m_iSerial).ToString();
		state.m_sFaction = faction; state.m_sRole = role; state.m_sPackageId = packageId;
		state.m_sTargetId = target; state.m_sExpectedOwner = m_Map.Owner(target);
		if (!m_Map.Position(target, state.m_vTargetPosition)) return null;
		state.m_vPosition = state.m_vTargetPosition; state.m_fSpeed = m_Config.m_fInfantrySpeed;
		state.m_iLastUpdateMs = System.GetTickCount(); state.m_iTransitionMs = state.m_iLastUpdateMs;
		foreach (TB_PVE_M3_4_ForcePackageEntry entry : package.m_aEntries)
		{
			Resource resource = Resource.Load(entry.m_sGroupPrefab);
			if (!resource || !resource.IsValid()) return null;
			IEntitySource source = resource.GetResource().ToEntitySource();
			array<ResourceName> slots = {}; if (!source || !source.Get("m_aUnitPrefabSlots", slots) || slots.IsEmpty()) return null;
			foreach (ResourceName prefab : slots) { Resource unit = Resource.Load(prefab); if (!unit || !unit.IsValid()) return null; }
			for (int quantity; quantity < entry.m_iQuantity; quantity++)
			{
				TB_CC_WarGroupState group = new TB_CC_WarGroupState();
				group.m_sGroupPrefab = entry.m_sGroupPrefab; group.m_sRole = entry.m_sRole;
				group.m_aSurvivorPrefabs.Copy(slots); group.m_iInitialStrength = slots.Count(); group.m_vPosition = state.m_vPosition;
				state.m_aGroups.Insert(group); state.m_iInitialStrength += slots.Count();
			}
		}
		state.Recount(); m_aForces.Insert(lease);
		PrintFormat("[TB_CC_WAR] force-created id=%1 faction=%2 role=%3 package=%4 strength=%5 target=%6 representation=VIRTUAL", state.m_sForceId, faction, role, packageId, state.m_iStrength, target);
		return lease;
	}
	TB_CC_WarForceLease Defence(string target, FactionKey faction, string packageId)
	{
		if (!Replication.IsServer() || !m_Map.CanDefend(target)) return null;
		foreach (TB_CC_WarForceLease existing : m_aForces)
		{
			if (existing.m_State.m_sTargetId != target || existing.m_State.m_sFaction != faction) continue;
			if (existing.m_State.m_sRole == "garrison") return existing;
			if (existing.m_bOwnedByOperation && m_Map.Owner(target) == faction)
			{ EndOperation(existing, true); return existing; } // Adopt winners, not fresh defenders.
		}
		return Build("DEFENCE", faction, "garrison", packageId, target, false);
	}
	TB_CC_WarForceLease Operation(string id, FactionKey faction, string packageId, string target)
	{
		if (!Replication.IsServer() || !m_Map.IsActive() || !m_Map.CanAttack(faction, target)) return null;
		foreach (TB_CC_WarForceLease active : m_aForces)
			if (active.m_bOwnedByOperation && !active.m_State.m_bCompleted && active.m_State.m_sTargetId == target && active.m_State.m_sFaction == faction) return null;
		string source; vector position;
		if (!m_Map.Source(faction, target, source, position) || !m_Map.CanTravel(source, target)) return null;
		TB_CC_WarForceLease lease = Build(id, faction, "offensive", packageId, target, true);
		if (!lease) return null;
		lease.m_bOwnedByOperation = true; lease.m_State.m_sSourceId = source;
		lease.m_State.m_vPosition = position; lease.m_State.m_vSourcePosition = position;
		foreach (TB_CC_WarGroupState group : lease.m_State.m_aGroups) group.m_vPosition = position;
		lease.m_State.m_iOperationState = TB_PVE_M3_1_OperationState.MOVING;
		return lease;
	}
	bool WantsPhysical(TB_CC_WarForceLease lease) { CollectPlayers(); return Relevant(lease, m_Config.m_fPreRadius); }
	FactionKey Owner(string id) { return m_Map.Owner(id); }
	bool PreparePhysicalDefence(TB_CC_WarForceLease attacker)
	{
		// Admission is incremental. Local reserve is frozen by IsTargetPhysical,
		// not converted into invisible damage or rolled back with another group.
		foreach (TB_CC_WarForceLease defence : m_aForces)
		{
			if (defence == attacker || defence.m_State.m_sTargetId != attacker.m_State.m_sTargetId || defence.m_State.m_sRole != "garrison") continue;
			string reason; if (!Materialize(defence, reason)) Defer(defence, reason);
		}
		return HasCapacity(attacker.m_State.m_iStrength);
	}
	void AttachPhysical(TB_CC_WarForceLease lease, array<ref TB_PVE_M3_4_AssembledGroup> groups)
	{
		if (lease.m_iRequestMs == 0)
		{
			lease.m_iRequestMs = System.GetTickCount();
			PrintFormat("[TB_CC_WAR] materialization-request id=%1 nearestPlayerDistance=%2 reason=counterattack", lease.m_State.m_sForceId, NearestDistance(lease));
		}
		lease.m_aPhysical = groups;
		for (int i; i < groups.Count(); i++)
		{
			TB_PVE_M3_4_AssembledGroup record = groups[i];
			record.m_sStrategicForceId = lease.m_State.m_sForceId; record.m_iStrategicGroupIndex = i;
			lease.m_State.m_aGroups[i].m_vPosition = record.m_vStartPosition;
		}
		lease.m_State.m_iRepresentation = TB_CC_WarRepresentation.MATERIALIZING;
		if (!groups.IsEmpty())
		{
			lease.m_State.m_vPosition = groups[0].m_vStartPosition;
			lease.m_State.m_vSourcePosition = groups[0].m_vStartPosition;
		}
		lease.m_State.m_iTransitionMs = System.GetTickCount();
	}
	void EndOperation(TB_CC_WarForceLease lease, bool won)
	{
		if (!lease) return;
		lease.m_bOwnedByOperation = false; lease.m_State.m_bCompleted = true;
		if (won && m_Map.Owner(lease.m_State.m_sTargetId) == lease.m_State.m_sFaction)
		{
			lease.m_State.m_sRole = "garrison";
			foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical) TB_CC_TerritorialOccupationService.SetDefensiveOrder(record, lease.m_State.m_vTargetPosition);
			return;
		}
		lease.m_State.m_sRole = "withdrawal";
		string source; vector destination;
		if (m_Map.Source(lease.m_State.m_sFaction, lease.m_State.m_sTargetId, source, destination))
		{
			lease.m_State.m_sTargetId = source; lease.m_State.m_vTargetPosition = destination;
		}
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical) TB_CC_PhysicalWarService.Withdraw(record);
	}
	protected void Snapshot(TB_CC_WarForceLease lease)
	{
		TB_CC_WarForceState state = lease.m_State;
		if (state.m_iRepresentation == TB_CC_WarRepresentation.VIRTUAL) return;
		vector position; int count;
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
		{
			TB_CC_WarGroupState group = state.m_aGroups[record.m_iStrategicGroupIndex];
			if (record.m_bNativeMembersPending)
			{
				foreach (IEntity pendingMember : record.m_aCharacters)
				{
					if (!pendingMember || pendingMember.IsDeleted() || TB_CC_PhysicalWarService.IsLiving(pendingMember) || record.m_aAccountedVirtualDeaths.Contains(pendingMember)) continue;
					EntityPrefabData pendingPrefab = pendingMember.GetPrefabData();
					if (pendingPrefab) group.m_aSurvivorPrefabs.RemoveItem(pendingPrefab.GetPrefabName());
					record.m_aAccountedVirtualDeaths.Insert(pendingMember);
				}
				continue; // Never classify not-yet-spawned native slots as casualties.
			}
			// Engine eviction can leave a dormant shell; never mistake it for deaths.
			if (record.m_Group && !record.m_Group.IsDeleted() && record.m_Group.GetAgentsCount() == 0 && record.m_Group.GetDormantAliveCount() > 0)
			{
				foreach (IEntity dormantMember : record.m_aCharacters)
				{
					if (!dormantMember || dormantMember.IsDeleted() || TB_CC_PhysicalWarService.IsLiving(dormantMember) || record.m_aAccountedVirtualDeaths.Contains(dormantMember)) continue;
					EntityPrefabData dormantPrefab = dormantMember.GetPrefabData();
					if (dormantPrefab) group.m_aSurvivorPrefabs.RemoveItem(dormantPrefab.GetPrefabName());
					record.m_aAccountedVirtualDeaths.Insert(dormantMember);
				}
				while (group.m_aSurvivorPrefabs.Count() > record.m_Group.GetDormantAliveCount()) group.m_aSurvivorPrefabs.Remove(group.m_aSurvivorPrefabs.Count() - 1);
				continue;
			}
			group.m_aSurvivorPrefabs.Clear();
			if (record.m_Group && !record.m_Group.IsDeleted())
			{
				array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
				foreach (AIAgent agent : agents)
				{
					if (!agent || !TB_CC_PhysicalWarService.IsLiving(agent.GetControlledEntity())) continue;
					IEntity character = agent.GetControlledEntity();
					EntityPrefabData prefabData = character.GetPrefabData();
					if (prefabData) group.m_aSurvivorPrefabs.Insert(prefabData.GetPrefabName());
					position += character.GetOrigin(); count++;
					if (!record.m_aCharacters.Contains(character)) record.m_aCharacters.Insert(character);
				}
				group.m_vPosition = TB_CC_PhysicalWarService.Position(record);
			}
		}
		if (count > 0)
		{
			state.m_vPosition = position / count;
			// Unleased reserve belongs to the moving force, not a new operation at
			// the old source. Physical members are never teleported by this update.
			if (state.m_sRole != "garrison")
				for (int reserveIndex; reserveIndex < state.m_aGroups.Count(); reserveIndex++)
					if (!HasGroup(lease, reserveIndex)) state.m_aGroups[reserveIndex].m_vPosition = state.m_vPosition;
		}
		state.Recount();
		float initialDistance = vector.Distance(state.m_vSourcePosition, state.m_vTargetPosition);
		if (initialDistance > 1) state.m_fProgress = Math.Clamp(1 - vector.Distance(state.m_vPosition, state.m_vTargetPosition) / initialDistance, 0, 1);
	}
	// Explicit representation retirement bypasses normal survivor withdrawal.
	// Only live entities are removed. Corpse lifetime stays with GarbageSystem.
	protected void Retire(TB_CC_WarForceLease lease)
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
		{
			if (record.m_Group && !record.m_Group.IsDeleted())
			{
				array<AIAgent> attached = {}; record.m_Group.GetAgents(attached);
				foreach (AIAgent member : attached) if (member && member.GetControlledEntity() && !record.m_aCharacters.Contains(member.GetControlledEntity())) record.m_aCharacters.Insert(member.GetControlledEntity());
			}
			ChimeraAIWorld world = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
			if (world && record.m_Group && !record.m_Group.IsDeleted()) world.PurgeSpawnRequestsForGroup(record.m_Group);
			TB_CC_PhysicalWarService.ApplySimulation(record, false);
			foreach (IEntity character : record.m_aCharacters)
			{
				if (TB_CC_PhysicalWarService.IsLiving(character)) SCR_EntityHelper.DeleteEntityAndChildren(character);
				else if (character && !character.IsDeleted())
				{
					TB_CC_PhysicalWarService.TrackGarbage(character);
					if (record.m_Group && !record.m_Group.IsDeleted() && character.GetParent() == record.m_Group) record.m_Group.RemoveChild(character, true);
				}
			}
			record.m_aCharacters.Clear();
			if (record.m_Waypoint && !record.m_Waypoint.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(record.m_Waypoint);
			if (record.m_Group && !record.m_Group.IsDeleted())
			{
				// Native members normally have no parent. Preserve any exceptional
				// surviving corpse child before deleting the group hierarchy.
				foreach (IEntity corpse : record.m_aAccountedVirtualDeaths)
					if (corpse && !corpse.IsDeleted() && corpse.GetParent() == record.m_Group) record.m_Group.RemoveChild(corpse, true);
				SCR_EntityHelper.DeleteEntityAndChildren(record.m_Group);
			}
			record.m_Group = null; record.m_Waypoint = null; record.m_bNativeMembersPending = false;
		}
		lease.m_aPhysical.Clear();
	}
	protected void Virtualize(TB_CC_WarForceLease lease)
	{
		int now = System.GetTickCount();
		PrintFormat("[TB_CC_WAR] virtualization-complete id=%1 remoteSeconds=%2 distanceNearestPlayer=%3", lease.m_State.m_sForceId, (now - lease.m_State.m_iRemoteSinceMs) / 1000.0, NearestDistance(lease));
		Snapshot(lease); Retire(lease);
		lease.m_iRequestMs = 0; lease.m_iFirstGroupMs = 0; lease.m_bFullMaterializationLogged = false;
		lease.m_State.m_iRepresentation = TB_CC_WarRepresentation.VIRTUAL;
		lease.m_State.m_iTransitionMs = System.GetTickCount(); lease.m_State.m_iLastUpdateMs = lease.m_State.m_iTransitionMs;
		m_iVirtualizations++;
		PrintFormat("[TB_CC_WAR] virtualized id=%1 strength=%2 casualties=%3 position=%4 progress=%5 state=%6", lease.m_State.m_sForceId, lease.m_State.m_iStrength, lease.m_State.m_iCasualties, lease.m_State.m_vPosition, lease.m_State.m_fProgress, lease.m_State.m_iOperationState);
	}
	protected int PendingSlots()
	{
		int pending;
		foreach (TB_CC_WarForceLease lease : m_aForces)
			foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
				if (record.m_bNativeMembersPending && record.m_Group && !record.m_Group.IsDeleted()) pending += Math.Max(record.m_Group.m_aUnitPrefabSlots.Count() - record.m_Group.GetAgentsCount(), 0);
		return pending;
	}
	bool HasCapacity(int strength)
	{
		return CapacityReason(strength).IsEmpty();
	}
	protected string CapacityReason(int strength)
	{
		AIWorld world = GetGame().GetAIWorld(); if (!world) return "ai-world-unavailable";
		if (!world.CanActivateGroup(null) || world.GetCurrentNumOfActiveAIs() + PendingSlots() + strength > world.GetLimitOfActiveAIs() - m_Config.m_iReservedAISlots) return "active-ai-reserve";
		int usage; int maximum;
		if (ReadGMBudget(usage, maximum) && maximum > 0 && usage + PendingSlots() + strength > maximum - m_Config.m_iReservedGMUnits) return "GM-placement-reserve-estimate";
		return "";
	}
	protected bool ReadGMBudget(out int usage, out int maximum)
	{
		// Only claim counters when an actual local editor mode exposes them.
		// Dedicated servers commonly have no local editor owner: report unavailable.
		SCR_EditorManagerEntity manager = SCR_EditorManagerEntity.GetInstance(); if (!manager) return false;
		SCR_EditorModeEntity mode = manager.GetCurrentModeEntity(); if (!mode) return false;
		SCR_BudgetEditorComponent budget = SCR_BudgetEditorComponent.Cast(mode.FindComponent(SCR_BudgetEditorComponent));
		if (!budget || !budget.GetMaxBudgetValue(EEditableEntityBudget.AI, maximum)) return false;
		usage = budget.GetCurrentBudgetValue(EEditableEntityBudget.AI); return usage >= 0;
	}
	protected bool HasGroup(TB_CC_WarForceLease lease, int index)
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
			if (record.m_iStrategicGroupIndex == index && record.m_Group && !record.m_Group.IsDeleted()) return true;
		return false;
	}
	protected int ReserveStrength(TB_CC_WarForceLease lease)
	{
		int reserve;
		for (int i; i < lease.m_State.m_aGroups.Count(); i++)
			if (!HasGroup(lease, i)) reserve += lease.m_State.m_aGroups[i].m_aSurvivorPrefabs.Count();
		return reserve;
	}
	protected int PhysicalStrength(TB_CC_WarForceLease lease)
	{
		array<IEntity> counted = {}; int living;
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
			foreach (IEntity character : record.m_aCharacters)
				if (TB_CC_PhysicalWarService.IsLiving(character) && !counted.Contains(character)) { counted.Insert(character); living++; }
		return living;
	}
	protected bool FindPlacement(SCR_AIGroup shell, TB_CC_WarForceState state, vector anchor, int ordinal, TB_CC_WarSearchStats stats, out vector position, out string reason)
	{
		array<float> rings = {15, 30, 50, 75, 100, 150, 200};
		bool initialFIA = state.m_sFaction == "FIA" && state.m_sRole == "offensive" && state.m_fProgress <= 0;
		vector searchCentre = anchor;
		if (initialFIA)
		{
			// Same placement validator, but cover the full accepted target-relative
			// staging annulus, rather than only the sector near the virtual origin.
			rings.Clear();
			for (int stagingRadius = 300; stagingRadius <= 500; stagingRadius += 50) rings.Insert(stagingRadius);
			searchCentre = state.m_vTargetPosition;
		}
		bool found; float bestScore = float.MAX;
		// Search all candidates once; preferences rank valid anchors, never reject
		// the whole force. Projection failures continue to the next bearing.
		foreach (float radius : rings)
		{
			if (!initialFIA && radius > m_Config.m_fMaximumPlacementSearch) continue;
			for (int bearing; bearing < 16; bearing++)
			{
				stats.m_iCandidates++;
				float angle = (bearing + ordinal * 7) * 0.392699;
				vector candidate = searchCentre + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
				candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
				// Keep the probe near the objective's supported elevation if a floor,
				// platform or bridge could be above terrain.
				if (Math.AbsFloat(candidate[1] - anchor[1]) <= m_Config.m_fSupportProbeAbove) candidate[1] = anchor[1];
				vector surface;
				if (Near(candidate, m_Config.m_fMinimumHumanSpawnDistance)) { reason = "human-overlap"; stats.Reject(reason); continue; }
				if (!TB_CC_WarPlacement.Surface(candidate, m_Config, surface, reason)) { stats.Reject(reason); continue; }
				bool slopePreference = reason == "slope-preference";
				vector projected;
				if (!TB_CC_WarPlacement.Project(shell, surface, m_Config, projected, reason)) { stats.Reject(reason); continue; }
				if (Near(projected, m_Config.m_fMinimumHumanSpawnDistance)) { reason = "human-overlap"; stats.Reject(reason); continue; }
				slopePreference = slopePreference || reason == "slope-preference";
				// Preserve the accepted initial FIA recapture annulus. A progressed
				// force resumes its current position instead of returning to staging.
				vector targetOffset = projected - state.m_vTargetPosition; targetOffset[1] = 0;
				float targetDistance = targetOffset.Length();
				if (state.m_sFaction == "FIA" && state.m_sRole == "offensive" && state.m_fProgress <= 0 && (targetDistance < 300 || targetDistance > 500))
				{ reason = "initial-staging-annulus"; stats.Reject(reason); continue; }
				float heightDelta = Math.AbsFloat(projected[1] - anchor[1]);
				if (heightDelta > m_Config.m_fMaximumAnchorHeightDelta) { reason = "extreme-anchor-height-delta"; stats.Reject(reason); continue; }
				bool overlap;
				foreach (TB_CC_WarForceLease existing : m_aForces)
					foreach (TB_PVE_M3_4_AssembledGroup record : existing.m_aPhysical)
						if (record.m_Group && !record.m_Group.IsDeleted() && vector.Distance(record.m_Group.GetOrigin(), projected) < 5) overlap = true;
				shell.SetOrigin(projected);
				TB_CC_WarPlacement.Formation(shell, m_Config, reason);
				bool formationPreference = reason == "formation-preference";
				if (formationPreference) stats.m_iFormationRejected++; // offset preference, not anchor rejection
				float score = radius + heightDelta * 8;
				if (projected[1] < anchor[1] - 2) score += heightDelta * 12; // avoid lower cliff shelves
				if (slopePreference || reason == "slope-preference") score += 100;
				if (formationPreference) score += 25;
				if (overlap) score += 50;
				foreach (vector player : m_aPlayers)
				{
					if (vector.Distance(player, projected) > m_Config.m_fPreferHiddenRange) continue;
					TraceParam sight = MakeTraceParam(player + Vector(0, 1.6, 0), projected + Vector(0, 1, 0), TraceFlags.ENTS | TraceFlags.WORLD);
					if (GetGame().GetWorld().TraceMove(sight, SCR_Global.FilterCallback_IgnoreCharactersWithChildren) >= 1) score += 50;
				}
				if (score >= bestScore) continue;
				found = true; bestScore = score; position = projected; stats.m_fBestDistance = radius;
			}
		}
		if (found) { reason = ""; return true; }
		if (stats.m_iNavNotReady > 0) reason = "nav-data-not-ready";
		return false;
	}
	protected bool Materialize(TB_CC_WarForceLease lease, out string reason)
	{
		TB_CC_WarForceState state = lease.m_State;
		if (lease.m_iRequestMs == 0)
		{
			lease.m_iRequestMs = System.GetTickCount();
			PrintFormat("[TB_CC_WAR] materialization-request id=%1 nearestPlayerDistance=%2 reason=%3", state.m_sForceId, NearestDistance(lease), RequestReason(lease));
		}
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(state.m_sFaction);
		if (!faction) { reason = "faction-unavailable"; return false; }
		lease.m_Search = new TB_CC_WarSearchStats();
		bool added; bool hadPhysical = !lease.m_aPhysical.IsEmpty();
		for (int i; i < state.m_aGroups.Count(); i++)
		{
			TB_CC_WarGroupState specification = state.m_aGroups[i];
			if (specification.m_aSurvivorPrefabs.IsEmpty() || HasGroup(lease, i)) continue;
			string groupReason = CapacityReason(specification.m_aSurvivorPrefabs.Count());
			if (!groupReason.IsEmpty()) { reason = groupReason; continue; }
			Resource resource = Resource.Load(specification.m_sGroupPrefab);
			if (!resource || !resource.IsValid()) { reason = "group-resource"; continue; }
			// Mark local representation before spawning; no reserve contributes to
			// abstract combat while any local physical/pending group exists.
			if (state.m_iRepresentation == TB_CC_WarRepresentation.VIRTUAL) state.m_iRepresentation = TB_CC_WarRepresentation.MATERIALIZING;
			EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = specification.m_vPosition;
			SCR_AIGroup groupEntity = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
			if (!groupEntity) { reason = "group-spawn"; continue; }
			if (groupEntity.GetSpawnImmediately()) { SCR_EntityHelper.DeleteEntityAndChildren(groupEntity); reason = "native-spawn-contract"; continue; }
			groupEntity.m_aUnitPrefabSlots.Copy(specification.m_aSurvivorPrefabs);
			vector position;
			if (!FindPlacement(groupEntity, state, specification.m_vPosition, i, lease.m_Search, position, groupReason))
			{ SCR_EntityHelper.DeleteEntityAndChildren(groupEntity); reason = groupReason; continue; }
			groupEntity.SetOrigin(position); groupEntity.SetFaction(faction);
			if (groupEntity.GetFaction() != faction) { SCR_EntityHelper.DeleteEntityAndChildren(groupEntity); reason = "group-faction"; continue; }
			TB_PVE_M3_4_AssembledGroup record = new TB_PVE_M3_4_AssembledGroup(); record.m_Group = groupEntity;
			record.m_sStrategicForceId = state.m_sForceId; record.m_iStrategicGroupIndex = i; record.m_sEntryRole = specification.m_sRole;
			record.m_vStartPosition = position; record.m_fStartDistance = vector.Distance(position, state.m_vTargetPosition); record.m_fMinimumDistance = record.m_fStartDistance;
			lease.m_aPhysical.Insert(record);
			groupEntity.RequestSpawn(specification.m_aSurvivorPrefabs.Count());
			record.m_bNativeMembersPending = true; record.m_iNativeSpawnStartMs = System.GetTickCount();
			TB_CC_PhysicalWarService.Register(record); added = true;
		}
		if (lease.m_aPhysical.IsEmpty()) state.m_iRepresentation = TB_CC_WarRepresentation.VIRTUAL;
		if (added)
		{
			// Reserve retries must not restart dwell/remote timers for existing groups.
			if (!hadPhysical) state.m_iTransitionMs = System.GetTickCount();
			PrintFormat("[TB_CC_WAR] materializing id=%1 totalStrength=%2 unleasedReserveStrength=%3 groups=%4 sourceRestart=0", state.m_sForceId, state.m_iStrength, ReserveStrength(lease), lease.m_aPhysical.Count());
		}
		if (ReserveStrength(lease) == 0) { reason = ""; return true; }
		if (reason.IsEmpty()) reason = "reserve-pending";
		return false;
	}
	protected bool MaterializeTarget(TB_CC_WarForceLease requested, out string reason)
	{
		// Retain safe admitted groups even if another target-side force fails.
		// IsTargetPhysical freezes abstract combat for ALL sides of this target.
		return Materialize(requested, reason);
	}
	protected bool SetOrder(TB_CC_WarForceLease lease, TB_PVE_M3_4_AssembledGroup record)
	{
		if (record.m_Waypoint && !record.m_Waypoint.IsDeleted()) return true;
		if (lease.m_State.m_sRole == "garrison") return TB_CC_TerritorialOccupationService.SetDefensiveOrder(record, lease.m_State.m_vTargetPosition);
		Resource resource = Resource.Load("{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!resource || !resource.IsValid()) return false;
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; params.Transform[3] = lease.m_State.m_vTargetPosition;
		record.m_Waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!record.m_Waypoint) return false;
		record.m_Group.AddWaypoint(record.m_Waypoint); record.m_bOperationSimulation = true; return true;
	}
	protected void ObservePhysical(TB_CC_WarForceLease lease)
	{
		bool pending; bool ready;
		for (int index = lease.m_aPhysical.Count() - 1; index >= 0; index--)
		{
			TB_PVE_M3_4_AssembledGroup record = lease.m_aPhysical[index];
			if (!record.m_Group || record.m_Group.IsDeleted())
			{
				// A removed shell is not proof that every requested slot died. Carry
				// forward only demonstrable deaths before releasing its group index.
				TB_CC_WarGroupState removed = lease.m_State.m_aGroups[record.m_iStrategicGroupIndex];
				foreach (IEntity corpse : record.m_aCharacters)
				{
					if (!corpse || corpse.IsDeleted() || TB_CC_PhysicalWarService.IsLiving(corpse) || record.m_aAccountedVirtualDeaths.Contains(corpse)) continue;
					EntityPrefabData data = corpse.GetPrefabData();
					if (data) removed.m_aSurvivorPrefabs.RemoveItem(data.GetPrefabName());
					record.m_aAccountedVirtualDeaths.Insert(corpse);
				}
				// Retire orphan live references/orders as well; they cannot coexist
				// with a subsequent representation of this same logical group.
				TB_CC_WarForceLease orphan = new TB_CC_WarForceLease(); orphan.m_State = lease.m_State;
				orphan.m_aPhysical.Insert(record); Retire(orphan); lease.m_aPhysical.Remove(index);
				lease.m_State.Recount(); continue;
			}
			if (m_Map.IsActive() && !record.m_bNativeMembersPending && record.m_Group.GetAgentsCount() == 0 && record.m_Group.GetDormantAliveCount() > 0 && Relevant(lease, m_Config.m_fPreRadius))
			{
				Snapshot(lease);
				TB_CC_WarGroupState dormant = lease.m_State.m_aGroups[record.m_iStrategicGroupIndex];
				int survivors = dormant.m_aSurvivorPrefabs.Count();
				if (survivors > 0 && HasCapacity(survivors))
				{
					record.m_Group.m_aUnitPrefabSlots.Copy(dormant.m_aSurvivorPrefabs);
					record.m_Group.SetDormantCounts(survivors, 0);
					record.m_aCharacters.Clear();
					record.m_Group.RequestSpawn(survivors);
					record.m_bNativeMembersPending = true; record.m_iNativeSpawnStartMs = System.GetTickCount();
				}
				else { Defer(lease, "engine-eviction-readmission-reserve"); pending = true; continue; }
			}
			string failure;
			if (!TB_PVE_M3_4_ForceAssemblyService.ObserveNativeMembers(record, failure)) pending = true;
			if (failure.IsEmpty() && !record.m_bNativeMembersPending && !SetOrder(lease, record)) failure = "order-creation";
			if (!failure.IsEmpty())
			{
				// Snapshot once BEFORE retirement; retain all other admitted groups.
				// The failed group's unspawned slots remain reserve, known deaths do not.
				Snapshot(lease);
				TB_CC_WarForceLease failed = new TB_CC_WarForceLease(); failed.m_State = lease.m_State;
				failed.m_aPhysical.Insert(record); Retire(failed); lease.m_aPhysical.Remove(index);
				Defer(lease, failure); continue;
			}
			if (!record.m_bNativeMembersPending && TB_CC_PhysicalWarService.LivingCount(record) > 0) ready = true;
			TB_CC_PhysicalWarService.ApplySimulation(record, Relevant(lease, m_Config.m_fPhysicalRadius) || record.m_bOperationSimulation || record.m_bBattleSimulation);
		}
		if (ready && lease.m_iFirstGroupMs == 0)
		{
			lease.m_iFirstGroupMs = System.GetTickCount();
			PrintFormat("[TB_CC_WAR] first-safe-group id=%1 latencyMs=%2", lease.m_State.m_sForceId, lease.m_iFirstGroupMs - lease.m_iRequestMs);
		}
		if (lease.m_aPhysical.IsEmpty()) lease.m_State.m_iRepresentation = TB_CC_WarRepresentation.VIRTUAL;
		else if (ready || !pending)
		{
			if (lease.m_State.m_iRepresentation == TB_CC_WarRepresentation.MATERIALIZING) m_iMaterializations++;
			// PHYSICAL can include reserve. Presence of ready actors does not wait
			// for other groups; reserve is counted separately and never simulates here.
			lease.m_State.m_iRepresentation = TB_CC_WarRepresentation.PHYSICAL;
		}
		else lease.m_State.m_iRepresentation = TB_CC_WarRepresentation.MATERIALIZING;
		Snapshot(lease);
		if (!pending && ReserveStrength(lease) == 0 && !lease.m_bFullMaterializationLogged && lease.m_State.m_iRepresentation == TB_CC_WarRepresentation.PHYSICAL)
		{
			lease.m_bFullMaterializationLogged = true;
			PrintFormat("[TB_CC_WAR] full-force-ready id=%1 latencyMs=%2 physicalStrength=%3", lease.m_State.m_sForceId, System.GetTickCount() - lease.m_iRequestMs, PhysicalStrength(lease));
		}
	}
	protected void Defer(TB_CC_WarForceLease lease, string reason)
	{
		lease.m_iRetryAfterMs = System.GetTickCount() + m_Config.m_iRetryMs; m_iDeferrals++;
		if (lease.m_sDeferredReason == reason && System.GetTickCount() < lease.m_iNextTransitionLogMs) return;
		lease.m_sDeferredReason = reason; lease.m_iNextTransitionLogMs = System.GetTickCount() + 10000;
		if (lease.m_State.m_sFaction == "FIA" && lease.m_State.m_sRole == "offensive")
			PrintFormat("[TB_CC_FIA_RECAPTURE] physical-staging-deferred target=%1 reason=%2 retryInMs=%3", lease.m_State.m_sTargetId, reason, m_Config.m_iRetryMs);
		PrintFormat("[TB_CC_WAR] materialization-deferred id=%1 target=%2 reason=%3 strength=%4 backgroundPaused=1", lease.m_State.m_sForceId, lease.m_State.m_sTargetId, reason, lease.m_State.m_iStrength);
		TB_CC_WarSearchStats stats = lease.m_Search;
		PrintFormat("[TB_CC_WAR] search id=%1 candidateCount=%2 navRejected=%3 waterRejected=%4 slopeRejected=%5 collisionRejected=%6 verticalRejected=%7 supportRejected=%8 formationRejected=%9", lease.m_State.m_sForceId, stats.m_iCandidates, stats.m_iNavRejected, stats.m_iWaterRejected, stats.m_iSlopeRejected, stats.m_iCollisionRejected, stats.m_iVerticalRejected, stats.m_iSupportRejected, stats.m_iFormationRejected);
		PrintFormat("[TB_CC_WAR] search id=%1 navNotReady=%2 bestCandidateDistance=%3 virtualReserveStrength=%4", lease.m_State.m_sForceId, stats.m_iNavNotReady, stats.m_fBestDistance, ReserveStrength(lease));
	}
	protected void Move(TB_CC_WarForceState state, float seconds)
	{
		vector delta = state.m_vTargetPosition - state.m_vPosition; float distance = delta.Length();
		if (distance <= 30) return;
		float step = Math.Min(state.m_fSpeed * seconds, distance - 30);
		vector movement = delta * (step / distance); state.m_vPosition += movement;
		foreach (TB_CC_WarGroupState group : state.m_aGroups) group.m_vPosition += movement;
		float initialDistance = vector.Distance(state.m_vSourcePosition, state.m_vTargetPosition);
		if (initialDistance > 1) state.m_fProgress = Math.Clamp(1 - vector.Distance(state.m_vPosition, state.m_vTargetPosition) / initialDistance, 0, 1);
	}
	protected float PackagePower(string packageId)
	{
		if (packageId == "HEAVY" || packageId == "SURGE" || packageId == "FIA_STRONGPOINT") return 1.2;
		if (packageId == "REINFORCED" || packageId == "ASSAULT" || packageId == "FIA_REINFORCED") return 1.1;
		return 1;
	}
	protected void ApplyLosses(TB_CC_WarForceState state, float rate)
	{
		state.m_fLossAccumulator += rate;
		while (state.m_fLossAccumulator >= 1 && state.m_iStrength > 0)
		{
			state.m_fLossAccumulator -= 1;
			// Deterministic rotating group distribution, not a new fresh package.
			int offset = state.m_iCasualties - (state.m_iCasualties / state.m_aGroups.Count()) * state.m_aGroups.Count();
			for (int n; n < state.m_aGroups.Count(); n++)
			{
				int index = offset + n; if (index >= state.m_aGroups.Count()) index -= state.m_aGroups.Count();
				array<ResourceName> slots = state.m_aGroups[index].m_aSurvivorPrefabs;
				if (slots.IsEmpty()) continue;
				slots.Remove(slots.Count() - 1); break;
			}
			state.Recount();
		}
	}
	protected void ResolveBattle(string target, float seconds)
	{
		if (IsTargetPhysical(target)) { m_iPhysicalBattles++; return; }
		array<ref TB_CC_WarForceLease> participants = {}; array<float> losses = {};
		bool attackers;
		foreach (TB_CC_WarForceLease lease : m_aForces)
		{
			TB_CC_WarForceState state = lease.m_State;
			if (state.m_sTargetId != target || state.m_iStrength <= 0 || vector.Distance(state.m_vPosition, state.m_vTargetPosition) > 80) continue;
			if (state.m_sRole != "offensive" && state.m_sRole != "garrison") continue;
			if (state.m_sRole == "offensive" && !state.m_bCompleted) attackers = true;
			participants.Insert(lease);
		}
		if (!attackers) return;
		m_iVirtualBattles++;
		// Compute every side's loss from the SAME pre-tick strengths, then apply.
		foreach (TB_CC_WarForceLease victim : participants)
		{
			float hostilePower;
			foreach (TB_CC_WarForceLease enemy : participants)
			{
				if (enemy.m_State.m_sFaction == victim.m_State.m_sFaction) continue;
				float power = enemy.m_State.m_iStrength * PackagePower(enemy.m_State.m_sPackageId);
				if (enemy.m_State.m_sRole == "garrison") power *= m_Config.m_fDefenceAdvantage;
				hostilePower += power;
			}
			float protection = 1;
			if (victim.m_State.m_sRole == "garrison") protection = m_Config.m_fDefenceAdvantage + m_Map.Importance(target) * 0.002;
			losses.Insert(hostilePower * m_Config.m_fCombatLossPerSecond * seconds / protection);
		}
		for (int i; i < participants.Count(); i++)
		{
			TB_CC_WarForceState state = participants[i].m_State;
			ApplyLosses(state, losses[i]); state.m_fCombatSeconds += seconds;
			if (state.m_sRole != "offensive" || state.m_bCompleted) continue;
			state.m_iOperationState = TB_PVE_M3_1_OperationState.ASSAULTING;
			if (state.m_iStrength == 0) { state.m_iOperationState = TB_PVE_M3_1_OperationState.FAILED; state.m_bCompleted = true; }
		}
		foreach (TB_CC_WarForceLease attacker : participants)
		{
			TB_CC_WarForceState state = attacker.m_State;
			if (state.m_sRole != "offensive" || state.m_bCompleted || state.m_iStrength == 0) continue;
			bool opposed;
			foreach (TB_CC_WarForceLease other : participants) if (other.m_State.m_sFaction != state.m_sFaction && other.m_State.m_iStrength > 0) opposed = true;
			if (opposed) { state.m_fCaptureElapsed = 0; continue; }
			state.m_iOperationState = TB_PVE_M3_1_OperationState.CAPTURING;
			state.m_fCaptureElapsed += seconds;
			if (state.m_fCaptureElapsed < m_Config.m_fCaptureSeconds || state.m_bCaptureCommitted) continue;
			if (!m_Map.IsActive() || !m_Map.CanAttack(state.m_sFaction, target) || m_Map.Owner(target) != state.m_sExpectedOwner || IsTargetPhysical(target)) continue;
			// Latch BEFORE callback reentrancy. The adapter has the only ownership write.
			state.m_bCaptureCommitted = true;
			if (!m_Map.CommitBackgroundCapture(state)) { state.m_bCaptureCommitted = false; continue; }
			state.m_iOperationState = TB_PVE_M3_1_OperationState.SUCCEEDED; state.m_bCompleted = true;
			PrintFormat("[TB_CC_WAR] virtual-victory id=%1 target=%2 strength=%3 casualties=%4 seconds=%5", state.m_sForceId, target, state.m_iStrength, state.m_iCasualties, state.m_fCombatSeconds);
			break;
		}
	}
	protected float NearestDistance(TB_CC_WarForceLease lease)
	{
		float nearest = float.MAX;
		foreach (vector player : m_aPlayers)
		{
			nearest = Math.Min(nearest, vector.Distance(player, lease.m_State.m_vPosition));
			nearest = Math.Min(nearest, vector.Distance(player, lease.m_State.m_vTargetPosition));
			foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
				foreach (IEntity member : record.m_aCharacters)
					if (TB_CC_PhysicalWarService.IsLiving(member)) nearest = Math.Min(nearest, vector.Distance(player, member.GetOrigin()));
		}
		if (nearest == float.MAX) return -1;
		return nearest;
	}
	protected bool CounterattackRelevant(TB_CC_WarForceLease lease)
	{
		return lease.m_State.m_sRole == "offensive" && Near(lease.m_State.m_vTargetPosition, m_Config.m_fVirtualRadius);
	}
	protected bool HumanBattle(TB_CC_WarForceLease lease)
	{
		foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
			if (record.m_bBattleSimulation && Near(TB_CC_PhysicalWarService.Position(record), m_Config.m_fVirtualRadius)) return true;
		return false;
	}
	protected int Priority(TB_CC_WarForceLease lease)
	{
		if (lease.m_State.m_sTargetId == m_Map.PlayerObjective()) return 0;
		float distance = NearestDistance(lease);
		if (distance >= 0 && distance <= m_Config.m_fPhysicalRadius) return 1;
		if (distance >= 0 && distance <= m_Config.m_fPreRadius) return 2;
		if (CounterattackRelevant(lease)) return 3;
		return 4;
	}
	protected string RequestReason(TB_CC_WarForceLease lease)
	{
		int priority = Priority(lease);
		if (priority == 0) return "current-objective";
		if (priority == 1) return "inside-physical-radius";
		if (priority == 2) return "inside-prematerialization-radius";
		if (priority == 3) return "counterattack-player-relevant";
		if (HumanBattle(lease)) return "active-human-battle";
		return "";
	}
	protected string PinReason(TB_CC_WarForceLease lease, int now)
	{
		TB_CC_WarForceState state = lease.m_State;
		if (state.m_sTargetId == m_Map.PlayerObjective()) return "current-objective";
		if (HumanBattle(lease)) return "active-human-battle";
		if (CounterattackRelevant(lease)) return "counterattack-player-relevant";
		float distance = NearestDistance(lease);
		if (distance >= 0 && distance <= m_Config.m_fVirtualRadius) return "within-radius";
		if (state.m_iRemoteSinceMs == 0) state.m_iRemoteSinceMs = now;
		if (now - state.m_iTransitionMs < m_Config.m_iMinimumDwellMs) return "physical-dwell";
		if (now - state.m_iRemoteSinceMs < m_Config.m_iGraceMs) return "remote-grace";
		if (!state.m_aAssets.IsEmpty()) return "unsupported-asset";
		return "";
	}
	void Tick()
	{
		if (!Replication.IsServer()) return;
		int now = System.GetTickCount(); float seconds = Math.Clamp((now - m_iLastTick) / 1000.0, 0, 5); m_iLastTick = now;
		CollectPlayers(); m_iPhysicalBattles = 0; m_iVirtualBattles = 0;
		bool active = m_Map.IsActive();
		// Reclaim distant completed physical leases before admitting nearer ones.
		foreach (TB_CC_WarForceLease lease : m_aForces)
		{
			TB_CC_WarForceState state = lease.m_State; state.m_iLastUpdateMs = now;
			if (state.m_iRepresentation != TB_CC_WarRepresentation.VIRTUAL) ObservePhysical(lease);
			if (state.m_sRole == "garrison" && m_Map.Owner(state.m_sTargetId) != state.m_sFaction)
			{
				state.m_sRole = "withdrawal"; string source; vector destination;
				if (m_Map.Source(state.m_sFaction, state.m_sTargetId, source, destination))
				{ state.m_sTargetId = source; state.m_vTargetPosition = destination; }
				foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical) TB_CC_PhysicalWarService.Withdraw(record);
			}
			if (state.m_iRepresentation == TB_CC_WarRepresentation.VIRTUAL) continue;
			string pin = PinReason(lease, now);
			if (pin == "current-objective" || pin == "active-human-battle" || pin == "counterattack-player-relevant" || pin == "within-radius") state.m_iRemoteSinceMs = 0;
			if (!pin.IsEmpty())
			{
				if (pin != lease.m_sPinReason || now >= lease.m_iNextTransitionLogMs)
				{
					PrintFormat("[TB_CC_WAR] virtualization-pending id=%1 distanceNearestPlayer=%2 threshold=%3 reason=%4", state.m_sForceId, NearestDistance(lease), m_Config.m_fVirtualRadius, pin);
					lease.m_sPinReason = pin; lease.m_iNextTransitionLogMs = now + 10000;
				}
				continue;
			}
			Virtualize(lease);
		}
		// Explicit priority admission: full player bubbles, current objective,
		// then early-materialization forces. Never raise native/GM limits.
		for (int priority; priority < 5; priority++)
		{
			foreach (TB_CC_WarForceLease lease : m_aForces)
			{
				TB_CC_WarForceState state = lease.m_State;
				if (!active || state.m_iStrength <= 0 || ReserveStrength(lease) == 0 || now < lease.m_iRetryAfterMs) continue;
				int rank = Priority(lease);
				string request = RequestReason(lease);
				if (rank != priority || request.IsEmpty()) continue;
				if (lease.m_iRequestMs == 0)
				{
					lease.m_iRequestMs = now;
					PrintFormat("[TB_CC_WAR] materialization-request id=%1 nearestPlayerDistance=%2 reason=%3", state.m_sForceId, NearestDistance(lease), request);
				}
				string failure; if (!MaterializeTarget(lease, failure)) Defer(lease, failure); else lease.m_sDeferredReason = "";
			}
		}
		array<string> battles = {};
		if (active) foreach (TB_CC_WarForceLease lease : m_aForces)
		{
			TB_CC_WarForceState state = lease.m_State;
			if (state.m_iStrength <= 0 || state.m_iRepresentation != TB_CC_WarRepresentation.VIRTUAL || Relevant(lease, m_Config.m_fPreRadius) || IsTargetPhysical(state.m_sTargetId)) continue;
			if (state.m_sRole == "withdrawal") { Move(state, seconds); continue; }
			if (state.m_sRole != "offensive" || state.m_bCompleted) continue;
			if (!m_Map.CanAttack(state.m_sFaction, state.m_sTargetId) || m_Map.Owner(state.m_sTargetId) != state.m_sExpectedOwner)
			{ state.m_iOperationState = TB_PVE_M3_1_OperationState.CANCELLED; state.m_bCompleted = true; continue; }
			Move(state, seconds);
			if (vector.Distance(state.m_vPosition, state.m_vTargetPosition) <= 80 && !battles.Contains(state.m_sTargetId)) battles.Insert(state.m_sTargetId);
		}
		foreach (string target : battles) ResolveBattle(target, seconds);
		array<string> physicalBattles = {};
		foreach (TB_CC_WarForceLease observed : m_aForces)
			foreach (TB_PVE_M3_4_AssembledGroup record : observed.m_aPhysical)
				if (record.m_bBattleSimulation && !physicalBattles.Contains(observed.m_State.m_sTargetId)) physicalBattles.Insert(observed.m_State.m_sTargetId);
		m_iPhysicalBattles = physicalBattles.Count();
		for (int index = m_aForces.Count() - 1; index >= 0; index--)
		{
			TB_CC_WarForceLease expired = m_aForces[index];
			if (expired.m_State.m_iStrength <= 0 && !expired.m_bOwnedByOperation && expired.m_State.m_sRole != "garrison")
			{ Retire(expired); m_aForces.Remove(index); }
		}
		if (now >= m_iNextTelemetry) { m_iNextTelemetry = now + 30000; Telemetry(); }
	}
	protected void Telemetry()
	{
		int physicalLiving; int virtualLiving; int physicalGroups; int virtualForces; int trackedActivatedAgents;
		int physicalGarrisons; int virtualGarrisons; int physicalOperations; int virtualOperations;
		foreach (TB_CC_WarForceLease lease : m_aForces)
		{
			TB_CC_WarForceState state = lease.m_State;
			virtualLiving += Math.Max(state.m_iStrength - PhysicalStrength(lease), 0);
			if (ReserveStrength(lease) > 0) virtualForces++;
			if (state.m_iRepresentation == TB_CC_WarRepresentation.VIRTUAL)
			{
				if (state.m_sRole == "garrison") virtualGarrisons++;
				if (lease.m_bOwnedByOperation) virtualOperations++;
			}
			else
			{
				physicalLiving += PhysicalStrength(lease);
				foreach (TB_PVE_M3_4_AssembledGroup record : lease.m_aPhysical)
				{
					if (!record.m_Group || record.m_Group.IsDeleted()) continue;
					physicalGroups++;
					array<AIAgent> agents = {}; record.m_Group.GetAgents(agents);
					foreach (AIAgent agent : agents) if (agent && agent.IsAIActivated() && TB_CC_PhysicalWarService.IsLiving(agent.GetControlledEntity())) trackedActivatedAgents++;
				}
				if (state.m_sRole == "garrison") physicalGarrisons++;
				if (lease.m_bOwnedByOperation) physicalOperations++;
			}
			PrintFormat("[TB_CC_WAR] strength id=%1 totalStrength=%2 physicalStrength=%3 virtualReserveStrength=%4 unleasedReserveStrength=%5 representation=%6", state.m_sForceId, state.m_iStrength, PhysicalStrength(lease), Math.Max(state.m_iStrength - PhysicalStrength(lease), 0), ReserveStrength(lease), state.m_iRepresentation);
		}
		PrintFormat("[TB_CC_WAR] physicalLivingAI=%1 virtualLivingStrength=%2 physicalGroups=%3 virtualForces=%4 physicalGarrisons=%5 virtualGarrisons=%6 physicalOperations=%7 virtualOperations=%8 activePlayerBubbles=%9", physicalLiving, virtualLiving, physicalGroups, virtualForces, physicalGarrisons, virtualGarrisons, physicalOperations, virtualOperations, m_aPlayers.Count());
		PrintFormat("[TB_CC_WAR] materializations=%1 virtualizations=%2 deferredMaterializations=%3 activeBattlesPhysical=%4 activeBattlesVirtual=%5 serverFPS=%6 retainedCorpses=%7", m_iMaterializations, m_iVirtualizations, m_iDeferrals, m_iPhysicalBattles, m_iVirtualBattles, System.GetFPS(), TB_CC_PhysicalWarService.RetainedCorpseCount());
		int usage; int maximum;
		AIWorld world = GetGame().GetAIWorld();
		if (ReadGMBudget(usage, maximum)) PrintFormat("[TB_CC_WAR] GMPlacementUsage=%1 GMPlacementMax=%2 reservedGMUnits=%3 counters=local-editor-mode", usage, maximum, m_Config.m_iReservedGMUnits);
		else Print("[TB_CC_WAR] GMPlacementUsage=UNAVAILABLE GMPlacementMax=UNAVAILABLE counters=no-local-editor-mode");
		if (world) PrintFormat("[TB_CC_WAR] nativeActiveAICount=%1 nativeAIMax=%2 pendingSlots=%3 reservedSlots=%4", world.GetCurrentNumOfActiveAIs(), world.GetLimitOfActiveAIs(), PendingSlots(), m_Config.m_iReservedAISlots);
		PrintFormat("[TB_CC_WAR] trackedActivatedLivingAgents=%1 physicalLivingAI=%2 counters=BCM-members-not-global-native-budget", trackedActivatedAgents, physicalLiving);
	}
}
