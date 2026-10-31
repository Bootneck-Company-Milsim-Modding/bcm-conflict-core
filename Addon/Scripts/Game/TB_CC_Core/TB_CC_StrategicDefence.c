// Cover only: no agents, weapons, ownership writes or map-specific coordinates.
enum TB_CC_DefenceSlotKind { INNER_COVER, MID_BUNKER, OUTER_COVER, STATIC_WEAPON, LIGHT_VEHICLE, APC, ARMOUR }
class TB_CC_DefenceSlot
{
	vector m_vPosition;
	vector m_vFacing;
	int m_iKind;
}
class TB_CC_DefenceExclusion
{
	vector m_vPosition;
	float m_fRadius = 15;
}
class TB_CC_DefencePlan
{
	string m_sId;
	vector m_vCentre;
	vector m_vApproach;
	bool m_bHQ;
	ref array<ref TB_CC_DefenceSlot> m_aSlots = {};
	ref array<ref TB_CC_DefenceExclusion> m_aNoBuild = {};
	ref array<IEntity> m_aCover = {};
	int m_iAttempt;
	int m_iObjects;
	string m_sLastReason;
}
class TB_CC_DefenceConfig
{
	bool m_bEnabled = true;
	string m_sProfile = "LIGHT_PREPARED";
	int m_iDensity = 3;
	int m_iMaximumObjects = 48;
	int m_iMaximumAttempts = 32;
	float m_fSpacing = 14;
	float m_fPlayerExclusion = 300;
	// Reserved capability: unsupported categories are deliberately never spawned.
	bool m_bVehicleDefenceEnabled = false;
}
class TB_CC_DefenceAdapter
{
	bool IsOperational() { return false; }
	FactionKey Owner(string id) { return ""; }
	bool IsFinalHQ(string id) { return false; }
	bool Approach(string id, out vector position) { return false; }
}
class TB_CC_StrategicDefence
{
	protected ref TB_CC_DefenceAdapter m_Map;
	protected ref TB_CC_DefenceConfig m_Config;
	protected ref array<ref TB_CC_DefencePlan> m_Plans = {};
	protected ref array<vector> m_Placed = {};
	protected int m_iCursor;
	protected int m_iObjects;
	void Initialize(TB_CC_DefenceAdapter adapter, TB_CC_DefenceConfig config)
	{ m_Map = adapter; m_Config = config; }
	void Register(TB_CC_DefencePlan plan) { m_Plans.Insert(plan); }
	protected bool PlayerNear(vector position)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager) return true;
		array<int> ids = {}; manager.GetPlayers(ids);
		foreach (int id : ids)
		{
			IEntity entity = manager.GetPlayerControlledEntity(id);
			if (!entity) continue;
			float distance = vector.Distance(entity.GetOrigin(), position);
			if (distance < m_Config.m_fPlayerExclusion) return true;
			if (distance > 900) continue;
			TraceParam sight = new TraceParam();
			sight.Start = entity.GetOrigin() + Vector(0, 1.7, 0); sight.End = position + Vector(0, 2, 0);
			sight.Flags = TraceFlags.WORLD | TraceFlags.ENTS; sight.Exclude = entity;
			if (GetGame().GetWorld().TraceMove(sight, null) >= 1) return true;
		}
		return false;
	}
	protected bool Validate(TB_CC_DefencePlan plan, vector candidate, float radius, out vector supported, out string reason)
	{
		foreach (TB_CC_DefenceExclusion exclusion : plan.m_aNoBuild)
			if (vector.Distance(candidate, exclusion.m_vPosition) < radius + exclusion.m_fRadius) { reason = "authored-no-build"; return false; }
		foreach (vector placed : m_Placed)
			if (vector.Distance(candidate, placed) < Math.Max(m_Config.m_fSpacing, radius * 2 + 4)) { reason = "spacing"; return false; }
		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld || !aiWorld.GetRoadNetworkManager()) { reason = "road-service-unavailable"; return false; }
		array<BaseRoad> roads = {};
		vector extent = Vector(radius + 8, 20, radius + 8);
		aiWorld.GetRoadNetworkManager().GetRoadsInAABB(candidate - extent, candidate + extent, roads);
		if (!roads.IsEmpty()) { reason = "road-clearance"; return false; }
		TB_CC_WarConfig surface = new TB_CC_WarConfig();
		surface.m_fMinimumSupportNormalY = 0.97; surface.m_fBodyClearance = 3.5;
		if (!TB_CC_WarPlacement.Surface(candidate, surface, supported, reason)) return false;
		// Conservative square enclosing any yaw. Every corner must be supported;
		// flat placement is rejected rather than burying one side or tilting a bunker.
		for (int x = -1; x <= 1; x++)
			for (int z = -1; z <= 1; z++)
			{
				vector sample;
				if (!TB_CC_WarPlacement.Surface(supported + Vector(x * radius, 0, z * radius), surface, sample, reason)) return false;
				if (Math.AbsFloat(sample[1] - supported[1]) > 0.18) { reason = "footprint-support"; return false; }
			}
		TraceOBB box = new TraceOBB();
		// TraceOBB.Mat is vector[3] (orientation only); Start carries position.
		box.Mat[0] = Vector(1, 0, 0); box.Mat[1] = Vector(0, 1, 0); box.Mat[2] = Vector(0, 0, 1);
		box.Start = supported; box.Mins = Vector(-radius, 0.2, -radius); box.Maxs = Vector(radius, 3.5, radius);
		box.Flags = TraceFlags.ENTS; box.LayerMask = EPhysicsLayerPresets.Projectile;
		GetGame().GetWorld().TracePosition(box, null);
		if (box.TraceEnt) { reason = "footprint-collision"; return false; }
		NavmeshWorldComponent nav = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (!nav) { reason = "navmesh-unavailable"; return false; }
		vector accessPoint;
		if (!nav.GetReachablePoint(supported, radius + 4, accessPoint)) { reason = "nav-access-unavailable"; return false; }
		return true;
	}
	void Tick()
	{
		if (!Replication.IsServer() || !m_Config.m_bEnabled || !m_Map.IsOperational() || m_Plans.IsEmpty() || m_iObjects >= m_Config.m_iMaximumObjects) return;
		// One bounded candidate per two-second caller tick, round robin. No sweep.
		if (m_iCursor >= m_Plans.Count()) m_iCursor = 0;
		TB_CC_DefencePlan plan = m_Plans[m_iCursor]; m_iCursor++;
		FactionKey owner = m_Map.Owner(plan.m_sId);
		if (owner != "FIA" && owner != "USSR") return;
		int density = Math.Clamp(m_Config.m_iDensity, 0, 6);
		if (density == 0 || m_Config.m_sProfile == "NONE") return;
		if (m_Config.m_sProfile == "DENSE_PREPARED") density = Math.Min(density + 2, 6);
		else if (m_Config.m_sProfile != "LIGHT_PREPARED") return;
		if (owner == "USSR") density = Math.Min(density + 1, 6);
		if (plan.m_bHQ) { if (!m_Map.IsFinalHQ(plan.m_sId)) return; density = Math.Min(density * 2, 8); }
		if (plan.m_iObjects >= density || plan.m_iAttempt >= m_Config.m_iMaximumAttempts) return;
		vector currentApproach;
		if (m_Map.Approach(plan.m_sId, currentApproach)) plan.m_vApproach = currentApproach;
		vector facing = plan.m_vApproach - plan.m_vCentre; facing[1] = 0;
		if (facing.Length() < 1) facing = Vector(0, 0, 1);
		facing.Normalize();
		vector right = Vector(facing[2], 0, -facing[0]);
		int slot = plan.m_iObjects % 3;
		int sector = plan.m_iAttempt % 7;
		float angle = (sector - 3) * 0.22;
		vector direction = facing * Math.Cos(angle) + right * Math.Sin(angle);
		float ring = 22 + slot * 14 + (plan.m_iAttempt / 7) * 3;
		vector candidate = plan.m_vCentre + direction * ring;
		bool bunker = slot == 1;
		if (plan.m_iObjects < plan.m_aSlots.Count())
		{
			TB_CC_DefenceSlot authored = plan.m_aSlots[plan.m_iObjects];
			if (authored.m_iKind > TB_CC_DefenceSlotKind.OUTER_COVER) return;
			candidate = authored.m_vPosition; facing = authored.m_vFacing; facing[1] = 0;
			if (facing.Length() < 1) return;
			facing.Normalize(); bunker = authored.m_iKind == TB_CC_DefenceSlotKind.MID_BUNKER;
		}
		candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
		if (PlayerNear(candidate)) return;
		float radius = 3; if (bunker) radius = 6;
		vector supported; string reason;
		if (!Validate(plan, candidate, radius, supported, reason))
		{
			if (reason != "nav-data-not-ready" && reason != "navmesh-unavailable" && reason != "road-service-unavailable") plan.m_iAttempt++;
			if (reason != plan.m_sLastReason) PrintFormat("[TB_CC_DEFENCE] position=%1 status=DEFERRED reason=%2 attempt=%3", plan.m_sId, reason, plan.m_iAttempt);
			plan.m_sLastReason = reason; return;
		}
		ResourceName prefab = "{E027C0CF067256FA}Prefabs/Compositions/Misc/SubCompositions/Fortifications/Sandbag_MG_USSR_01.et";
		if (bunker && owner == "FIA") prefab = "{82AB62F3269E79C3}Prefabs/Compositions/Misc/SubCompositions/Fortifications/Sandbag_Bunker_Camouflaged_FIA_01.et";
		if (bunker && owner == "USSR") prefab = "{605A7848CB4D361A}Prefabs/Compositions/Misc/SubCompositions/Fortifications/Sandbag_Bunker_Camouflaged_USSR_01.et";
		Resource resource = Resource.Load(prefab); plan.m_iAttempt++;
		if (!resource || !resource.IsValid()) return;
		TraceParam arc = new TraceParam(); arc.Start = supported + Vector(0, 1.7, 0);
		arc.End = arc.Start + facing * 20; arc.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		if (GetGame().GetWorld().TraceMove(arc, null) < 0.5) return;
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		params.Transform[0] = Vector(facing[2], 0, -facing[0]); params.Transform[1] = Vector(0, 1, 0); params.Transform[2] = facing; params.Transform[3] = supported;
		IEntity cover = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!cover) return;
		plan.m_aCover.Insert(cover); m_Placed.Insert(supported); plan.m_iObjects++; m_iObjects++;
		PrintFormat("[TB_CC_DEFENCE] position=%1 owner=%2 profile=%3 fortifications=%4 global=%5 bunker=%6 retainedAfterCapture=1", plan.m_sId, owner, m_Config.m_sProfile, plan.m_iObjects, m_iObjects, bunker);
	}
}
