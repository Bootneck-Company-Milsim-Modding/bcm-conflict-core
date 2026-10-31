// Reusable authority-only representation data. No world/entity pointers belong
// in the state DTOs. Runtime handles are deliberately kept in a separate lease.
enum TB_CC_WarRepresentation { VIRTUAL, MATERIALIZING, PHYSICAL }

class TB_CC_WarConfig
{
	float m_fPhysicalRadius = 750;
	float m_fPreRadius = 1000;
	float m_fVirtualRadius = 1100;
	int m_iGraceMs = 10000;
	int m_iMinimumDwellMs = 10000;
	int m_iRetryMs = 1000;
	int m_iReservedAISlots = 16;
	int m_iReservedGMUnits = 20;
	float m_fInfantrySpeed = 1.8;
	float m_fCombatLossPerSecond = 0.012;
	float m_fDefenceAdvantage = 1.35;
	float m_fCaptureSeconds = 60;
	float m_fMaximumProjectionDelta = 10;
	float m_fMinimumSupportNormalY = 0.55;
	float m_fPreferredSupportNormalY = 0.82;
	float m_fSupportProbeAbove = 4;
	float m_fSupportProbeBelow = 8;
	float m_fBodyClearance = 1.8;
	float m_fBodyHalfWidth = 0.35;
	float m_fMaximumPlacementSearch = 200;
	float m_fMaximumAnchorHeightDelta = 30;
	float m_fMinimumHumanSpawnDistance = 15;
	float m_fPreferHiddenRange = 900;
	void Normalize()
	{
		m_fPhysicalRadius = Math.Max(m_fPhysicalRadius, 1);
		m_fPreRadius = Math.Max(m_fPreRadius, m_fPhysicalRadius);
		m_fVirtualRadius = Math.Max(m_fVirtualRadius, m_fPreRadius + 1);
		m_iGraceMs = Math.Max(m_iGraceMs, 0); m_iMinimumDwellMs = Math.Max(m_iMinimumDwellMs, 0);
		m_iRetryMs = Math.Max(m_iRetryMs, 1000);
		m_iReservedAISlots = Math.Max(m_iReservedAISlots, 0); m_iReservedGMUnits = Math.Max(m_iReservedGMUnits, 0);
		m_fInfantrySpeed = Math.Max(m_fInfantrySpeed, 0.1); m_fCombatLossPerSecond = Math.Max(m_fCombatLossPerSecond, 0);
		m_fCaptureSeconds = Math.Max(m_fCaptureSeconds, 1); m_fDefenceAdvantage = Math.Max(m_fDefenceAdvantage, 1);
		m_fMaximumPlacementSearch = Math.Max(m_fMaximumPlacementSearch, 15);
		m_fMaximumProjectionDelta = Math.Max(m_fMaximumProjectionDelta, 0.1);
		m_fMaximumAnchorHeightDelta = Math.Max(m_fMaximumAnchorHeightDelta, 0.1);
		m_fMinimumSupportNormalY = Math.Clamp(m_fMinimumSupportNormalY, 0.1, 1);
		m_fPreferredSupportNormalY = Math.Clamp(m_fPreferredSupportNormalY, m_fMinimumSupportNormalY, 1);
		m_fSupportProbeAbove = Math.Max(m_fSupportProbeAbove, 0.1); m_fSupportProbeBelow = Math.Max(m_fSupportProbeBelow, 0.1);
		m_fBodyClearance = Math.Max(m_fBodyClearance, 0.5); m_fBodyHalfWidth = Math.Max(m_fBodyHalfWidth, 0.1);
		m_fMinimumHumanSpawnDistance = Math.Max(m_fMinimumHumanSpawnDistance, 0); m_fPreferHiddenRange = Math.Max(m_fPreferHiddenRange, 0);
	}
}

class TB_CC_WarGroupState
{
	ResourceName m_sGroupPrefab;
	string m_sRole;
	ref array<ResourceName> m_aSurvivorPrefabs = {};
	int m_iInitialStrength;
	vector m_vPosition;
}

// Vehicle extension seam; no vehicle is retired/materialized by this pass.
class TB_CC_WarAssetState
{
	string m_sAssetId;
	ResourceName m_sPrefab;
	string m_sClass;
	float m_fHealth;
	int m_iCrewStrength;
	vector m_vPosition;
}

class TB_CC_WarForceState
{
	string m_sForceId;
	FactionKey m_sFaction;
	string m_sRole;
	string m_sPackageId;
	string m_sSourceId;
	string m_sTargetId;
	FactionKey m_sExpectedOwner;
	vector m_vPosition;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	float m_fProgress;
	float m_fSpeed;
	float m_fLossAccumulator;
	float m_fCaptureElapsed;
	float m_fCombatSeconds;
	int m_iInitialStrength;
	int m_iStrength;
	int m_iCasualties;
	int m_iOperationState;
	int m_iRepresentation;
	int m_iLastUpdateMs;
	int m_iTransitionMs;
	int m_iRemoteSinceMs;
	int m_iEscalationContext;
	bool m_bCompleted;
	bool m_bCaptureCommitted;
	ref array<ref TB_CC_WarGroupState> m_aGroups = {};
	ref array<ref TB_CC_WarAssetState> m_aAssets = {};
	void Recount()
	{
		m_iStrength = 0;
		foreach (TB_CC_WarGroupState group : m_aGroups) m_iStrength += group.m_aSurvivorPrefabs.Count();
		m_iCasualties = Math.Max(m_iInitialStrength - m_iStrength, 0);
	}
}

class TB_CC_WarForceLease
{
	ref TB_CC_WarForceState m_State;
	// One shared array, owned by this service; operations/managers observe it.
	ref array<ref TB_PVE_M3_4_AssembledGroup> m_aPhysical = {};
	int m_iRetryAfterMs;
	string m_sDeferredReason;
	bool m_bOwnedByOperation;
	int m_iRequestMs;
	int m_iFirstGroupMs;
	bool m_bFullMaterializationLogged;
	int m_iNextTransitionLogMs;
	string m_sPinReason;
	ref TB_CC_WarSearchStats m_Search = new TB_CC_WarSearchStats();
}

class TB_CC_WarSearchStats
{
	int m_iCandidates;
	int m_iNavRejected;
	int m_iWaterRejected;
	int m_iSlopeRejected;
	int m_iCollisionRejected;
	int m_iVerticalRejected;
	int m_iSupportRejected;
	int m_iFormationRejected;
	int m_iNavNotReady;
	float m_fBestDistance = -1;
	void Reject(string reason)
	{
		if (reason == "nav-data-not-ready") m_iNavNotReady++;
		else if (reason.Contains("nav")) m_iNavRejected++;
		else if (reason == "water") m_iWaterRejected++;
		else if (reason == "extreme-slope") m_iSlopeRejected++;
		else if (reason.Contains("collision") || reason == "human-overlap") m_iCollisionRejected++;
		else if (reason.Contains("delta") || reason == "inside-terrain") m_iVerticalRejected++;
		else if (reason.Contains("formation")) m_iFormationRejected++;
		else m_iSupportRejected++;
	}
}

// All campaign/map decisions cross this adapter, not the simulation core.
class TB_CC_WarMapAdapter
{
	bool IsActive() { return false; }
	string PlayerObjective() { return ""; }
	FactionKey Owner(string id) { return ""; }
	bool Position(string id, out vector position) { return false; }
	bool CanAttack(FactionKey faction, string id) { return false; }
	bool CanDefend(string id) { return false; }
	bool CanTravel(string source, string target) { return false; }
	int Importance(string id) { return 0; }
	bool Source(FactionKey faction, string target, out string source, out vector position) { return false; }
	bool CommitBackgroundCapture(TB_CC_WarForceState force) { return false; }
}

// The installed SCR_AIGroup uses AIPathfindingComponent's closest-navmesh API
// before SpawnGroupMember. Use the same API, with bounded vertical tolerances.
class TB_CC_WarPlacement
{
	static bool Formation(SCR_AIGroup group, TB_CC_WarConfig config, out string reason)
	{
		AIFormationComponent component = AIFormationComponent.Cast(group.FindComponent(AIFormationComponent));
		AIFormationDefinition definition; if (component) definition = component.GetFormation();
		for (int slot; slot < group.m_aUnitPrefabSlots.Count(); slot++)
		{
			vector offset = Vector(slot, 0, 0); if (definition) offset = definition.GetOffsetPosition(slot);
			vector candidate = group.CoordToParent(offset);
			vector result;
			// A safe anchor is mandatory; exact formation offsets are preferences.
			// Native SpawnGroupMember projects individual members within 10m.
			if (!Project(group, candidate, config, result, reason)) { reason = "formation-preference"; return true; }
		}
		return true;
	}
	static bool Surface(vector candidate, TB_CC_WarConfig config, out vector result, out string reason)
	{
		BaseWorld world = GetGame().GetWorld();
		AIWorld aiWorld = GetGame().GetAIWorld();
		NavmeshWorldComponent nav; if (aiWorld) nav = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (!nav) { reason = "navmesh-unavailable"; return false; }
		if (!nav.IsTileLoaded(candidate))
		{ nav.LoadTileIn(candidate); reason = "nav-data-not-ready"; return false; }
		if (!nav.IsTileValid(candidate)) { reason = "no-valid-navmesh"; return false; }
		float terrainY = world.GetSurfaceY(candidate[0], candidate[2]);
		TraceParam support = MakeTraceParam(candidate + Vector(0, config.m_fSupportProbeAbove, 0), candidate - Vector(0, config.m_fSupportProbeBelow, 0), TraceFlags.ENTS | TraceFlags.WORLD);
		float hit = world.TraceMove(support, SCR_Global.FilterCallback_IgnoreCharactersWithChildren);
		if (hit >= 1) { reason = "unsupported"; return false; }
		if (support.TraceNorm[1] < config.m_fMinimumSupportNormalY) { reason = "extreme-slope"; return false; }
		result = support.Start + (support.End - support.Start) * hit;
		if (result[1] < terrainY - 0.15) { reason = "inside-terrain"; return false; }
		// Trace-confirmed object support may legitimately stand above raw terrain.
		if (Math.AbsFloat(result[1] - candidate[1]) > config.m_fMaximumProjectionDelta)
		{ reason = "vertical-surface-delta"; return false; }
		if (ChimeraWorldUtils.TryGetWaterSurfaceSimple(world, result)) { reason = "water"; return false; }
		// Five body-height probes plus cross-section probes reject walls/ceilings.
		for (int sample; sample < 5; sample++)
		{
			vector offset;
			if (sample == 1) offset = Vector(config.m_fBodyHalfWidth, 0, 0);
			if (sample == 2) offset = Vector(-config.m_fBodyHalfWidth, 0, 0);
			if (sample == 3) offset = Vector(0, 0, config.m_fBodyHalfWidth);
			if (sample == 4) offset = Vector(0, 0, -config.m_fBodyHalfWidth);
			TraceParam body = MakeTraceParam(result + offset + Vector(0, 0.15, 0), result + offset + Vector(0, config.m_fBodyClearance, 0), TraceFlags.ENTS | TraceFlags.WORLD);
			if (world.TraceMove(body, SCR_Global.FilterCallback_IgnoreCharactersWithChildren) < 1) { reason = "body-collision"; return false; }
		}
		for (int axis; axis < 2; axis++)
		{
			vector span = Vector(config.m_fBodyHalfWidth, 0, 0);
			if (axis == 1) span = Vector(0, 0, config.m_fBodyHalfWidth);
			TraceParam width = MakeTraceParam(result - span + Vector(0, 1, 0), result + span + Vector(0, 1, 0), TraceFlags.ENTS | TraceFlags.WORLD);
			if (world.TraceMove(width, SCR_Global.FilterCallback_IgnoreCharactersWithChildren) < 1) { reason = "body-width-collision"; return false; }
		}
		reason = "";
		if (support.TraceNorm[1] < config.m_fPreferredSupportNormalY) reason = "slope-preference";
		return true;
	}
	static bool Project(SCR_AIGroup group, vector candidate, TB_CC_WarConfig config, out vector result, out string reason)
	{
		AIPathfindingComponent path = AIPathfindingComponent.Cast(group.FindComponent(AIPathfindingComponent));
		if (!path) { reason = "pathfinding-component-unavailable"; return false; }
		vector projected;
		float delta = config.m_fMaximumProjectionDelta;
		if (!path.GetClosestPositionOnNavmesh(candidate, Vector(delta, delta, delta), projected)) { reason = "navmesh-projection-failed"; return false; }
		if (Math.AbsFloat(projected[1] - candidate[1]) > delta) { reason = "navmesh-vertical-delta"; return false; }
		return Surface(projected, config, result, reason);
	}
}
