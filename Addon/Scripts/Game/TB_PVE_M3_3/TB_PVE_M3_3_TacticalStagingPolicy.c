class TB_PVE_M3_3_TacticalStagingPolicy
{
	protected float m_fMinimumTargetDistance;
	protected float m_fMaximumTargetDistance;
	protected float m_fMinimumPlayerDistance;
	protected float m_fVisibilityRelevanceDistance;
	protected float m_fNavigationTolerance;
	protected int m_iCandidateAttempts;
	protected int m_iProductionCandidateAttempts;
	protected bool m_bDetailedDiagnostics;
	protected NavmeshWorldComponent m_Navmesh;
	protected int m_iQuerySerial;
	protected vector m_vLastAcceptedPosition;
	protected float m_fLastAcceptedDistance;
	protected float m_fLastAcceptedBearing;
	protected int m_iLastAttemptsUsed;
	protected int m_iLastDurationMs;
	protected ref array<string> m_aUnsupportedFootTargets = {};

	void AddUnsupportedFootTarget(string objectiveId)
	{
		if (!objectiveId.IsEmpty() && !m_aUnsupportedFootTargets.Contains(objectiveId)) m_aUnsupportedFootTargets.Insert(objectiveId);
	}

	void Configure(float minimumTargetDistance, float maximumTargetDistance, float minimumPlayerDistance, float visibilityRelevanceDistance, float navigationTolerance, int candidateAttempts, bool detailedDiagnostics)
	{
		m_fMinimumTargetDistance = Math.Max(minimumTargetDistance, 100);
		m_fMaximumTargetDistance = Math.Max(maximumTargetDistance, m_fMinimumTargetDistance);
		m_fMinimumPlayerDistance = Math.Max(minimumPlayerDistance, 0);
		m_fVisibilityRelevanceDistance = Math.Max(visibilityRelevanceDistance, m_fMinimumPlayerDistance);
		m_fNavigationTolerance = Math.Max(navigationTolerance, 1);
		m_iCandidateAttempts = Math.Max(candidateAttempts, 0);
		m_iProductionCandidateAttempts = m_iCandidateAttempts;
		m_bDetailedDiagnostics = detailedDiagnostics;
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld) m_Navmesh = aiWorld.GetNavmeshWorldComponent("Soldiers");
		PrintFormat("[TB_PVE_M3_3] staging-policy-configured annulus=%1..%2 playerExclusion=%3 visibilityRange=%4 navTolerance=%5 attempts=%6 navmesh=%7", m_fMinimumTargetDistance, m_fMaximumTargetDistance, m_fMinimumPlayerDistance, m_fVisibilityRelevanceDistance, m_fNavigationTolerance, m_iCandidateAttempts, m_Navmesh != null);
	}

	bool FindStagingPosition(FactionKey factionKey, string targetObjectiveId, vector targetPosition, array<vector> observerPositions, out vector stagingPosition, out string failureReason)
	{
		int startMs = System.GetTickCount();
		m_iQuerySerial++;
		m_iLastAttemptsUsed = 0;
		if (!Replication.IsServer()) return FailQuery("authority-required", startMs, failureReason);
		if (factionKey != "USSR" && factionKey != "FIA") return FailQuery("unsupported-operation-faction", startMs, failureReason);
		// No amphibious/air-assault transport exists yet. Do not select a
		// mainland staging point for this approved offshore human objective.
		if (m_aUnsupportedFootTargets.Contains(targetObjectiveId)) return FailQuery("unsupported-transport-requirement", startMs, failureReason);
		if (!m_Navmesh) return FailQuery("infantry-navmesh-unavailable", startMs, failureReason);
		if (m_iCandidateAttempts <= 0) return FailQuery("no-candidate-attempts-configured", startMs, failureReason);
		RequestNavigationForTarget(targetPosition);
		vector targetNavAnchor;
		if (!FindTargetNavAnchor(targetPosition, targetNavAnchor))
		{
			// Failed before spawning: record tile state, not a guessed budget cause.
			vector navSample = targetPosition;
			navSample[1] = GetGame().GetWorld().GetSurfaceY(navSample[0], navSample[2]);
			PrintFormat("[TB_CC_SPAWN_DIAG] target=%1 faction=%2 anchor=%3 tileLoaded=%4 tileValid=%5 tileRequested=%6 searchRadius=%7 noSpawn=1 reason=target-navigation-anchor-unavailable", targetObjectiveId, factionKey, navSample, m_Navmesh.IsTileLoaded(navSample), m_Navmesh.IsTileValid(navSample), m_Navmesh.IsTileRequested(navSample), m_fNavigationTolerance);
			RequestNavigationForTarget(targetPosition);
			return FailQuery("target-navigation-anchor-unavailable", startMs, failureReason);
		}
		if (factionKey == "FIA")
		{
			array<vector> none = {};
			return FindFIATerritorialStaging(targetObjectiveId, targetPosition, observerPositions, none, 0, stagingPosition, failureReason);
		}

		for (int attempt; attempt < m_iCandidateAttempts; attempt++)
		{
			m_iLastAttemptsUsed = attempt + 1;
			// Interleave bearing and radius instead of spending the first 12
			// attempts in one radial band and adjacent sectors.
			int distanceBand = (attempt + attempt / 16) % 4;
			float distanceRequest = m_fMinimumTargetDistance + (distanceBand * ((m_fMaximumTargetDistance - m_fMinimumTargetDistance) / 3.0));
			int directionIndex = m_iQuerySerial * 3 + attempt * 5;
			directionIndex = directionIndex % 16;
			float angle = directionIndex * 0.392699;
			vector candidate = targetPosition + Vector(Math.Sin(angle) * distanceRequest, 0, Math.Cos(angle) * distanceRequest);
			candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
			string rejection;
			if (!ValidateCandidate(targetPosition, candidate, observerPositions, rejection))
			{
				LogRejected(targetObjectiveId, attempt, candidate, rejection);
				continue;
			}

			stagingPosition = candidate;
			m_vLastAcceptedPosition = candidate;
			m_fLastAcceptedDistance = HorizontalDistance(candidate, targetPosition);
			m_fLastAcceptedBearing = Bearing(candidate, targetPosition);
			m_iLastDurationMs = System.GetTickCount() - startMs;
			PrintFormat("[TB_PVE_M3_3] staging-accepted query=%1 target=%2 faction=%3 position=%4 distance=%5 bearing=%6 attemptsUsed=%7 durationMs=%8 observers=%9", m_iQuerySerial, targetObjectiveId, factionKey, stagingPosition, m_fLastAcceptedDistance, m_fLastAcceptedBearing, m_iLastAttemptsUsed, m_iLastDurationMs, observerPositions.Count());
			return true;
		}
		return FailQuery("NO_VALID_STAGING_POSITION", startMs, failureReason);
	}

	protected bool FindTargetNavAnchor(vector targetPosition, out vector anchor)
	{
		vector sample = targetPosition;
		sample[1] = GetGame().GetWorld().GetSurfaceY(sample[0], sample[2]);
		if (m_Navmesh.IsTileValid(sample)) { anchor = sample; return true; }
		for (float radius = 5; radius <= m_fNavigationTolerance; radius += 5)
		{
			for (int direction; direction < 16; direction++)
			{
				float angle = direction * 0.392699;
				sample = targetPosition + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
				sample[1] = GetGame().GetWorld().GetSurfaceY(sample[0], sample[2]);
				if (!m_Navmesh.IsTileValid(sample)) continue;
				anchor = sample;
				return true;
			}
		}
		return false;
	}

	// Streaming navmesh tiles are requested explicitly; callers still receive a
	// bounded failure until the engine reports the relevant tile as usable.
	void RequestNavigationForTarget(vector targetPosition)
	{
		if (!m_Navmesh) return;
		int requested;
		for (float radius; radius <= m_fNavigationTolerance; radius += 20)
		{
			for (int direction; direction < 8; direction++)
			{
				float angle = direction * 0.785398;
				vector sample = targetPosition + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
				sample[1] = GetGame().GetWorld().GetSurfaceY(sample[0], sample[2]);
				if (m_Navmesh.LoadTileIn(sample)) requested++;
			}
		}
		for (float stagingRadius = m_fMinimumTargetDistance; stagingRadius <= m_fMaximumTargetDistance; stagingRadius += 50)
		{
			for (int stagingDirection; stagingDirection < 16; stagingDirection++)
			{
				float stagingAngle = stagingDirection * 0.392699;
				vector stagingSample = targetPosition + Vector(Math.Sin(stagingAngle) * stagingRadius, 0, Math.Cos(stagingAngle) * stagingRadius);
				stagingSample[1] = GetGame().GetWorld().GetSurfaceY(stagingSample[0], stagingSample[2]);
				if (m_Navmesh.LoadTileIn(stagingSample)) requested++;
			}
		}
		if (m_bDetailedDiagnostics) PrintFormat("[TB_PVE_M3_3] navigation-stream-request target=%1 requested=%2 tolerance=%3", targetPosition, requested, m_fNavigationTolerance);
	}

	protected bool ValidateCandidate(vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true)
	{
		float targetDistance = HorizontalDistance(candidate, targetPosition);
		if (enforceAnnulus && (targetDistance < m_fMinimumTargetDistance - 1 || targetDistance > m_fMaximumTargetDistance + 1))
		{
			rejection = "outside-target-annulus"; return false;
		}
		if (ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), candidate))
		{
			rejection = "water"; return false;
		}
		if (!m_Navmesh.IsTileValid(candidate))
		{
			rejection = "navigation-tile-invalid"; return false;
		}
		if (!ValidateSupportingTerrain(candidate, rejection)) return false;

		foreach (vector observer : observers)
		{
			float observerDistance = vector.Distance(observer, candidate);
			if (observerDistance < m_fMinimumPlayerDistance)
			{
				rejection = "human-player-distance"; return false;
			}
			if (observerDistance <= m_fVisibilityRelevanceDistance && HasDirectVisibility(observer, candidate))
			{
				rejection = "human-player-direct-visibility"; return false;
			}
		}
		return true;
	}

	// Future authored approach bearing seam; no authored points are required now.
	protected bool TryGetAuthoredFIAApproachBearing(string targetId, int ordinal, out float bearingRadians)
	{
		return false;
	}

	protected bool FindFIATerritorialStaging(string targetId, vector targetPosition, array<vector> observers, array<vector> used, int ordinal, out vector result, out string failure)
	{
		int startMs = System.GetTickCount();
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		if (!policy || !policy.CanFIARecapture(targetId)) return FailQuery("FIA-no-legal-territorial-origin", startMs, failure);
		array<string> neighbours = {}; array<string> origins = {};
		policy.GetTerritorialNeighbours(targetId, neighbours);
		foreach (string neighbour : neighbours)
			if (policy.IsFootTerritory(neighbour) && policy.GetTerritorialOwner(neighbour) == "FIA") origins.Insert(neighbour);
		if (origins.IsEmpty()) return FailQuery("FIA-no-owned-adjacent-origin", startMs, failure);
		int overlapRejected; int navRejected; int waterRejected; int humanRejected; int surfaceRejected;
		string lastRejection = "source-position-unresolved";
		for (int attempt; attempt < m_iCandidateAttempts; attempt++)
		{
			m_iLastAttemptsUsed = attempt + 1;
			string source = origins[(attempt + ordinal) % origins.Count()];
			vector origin; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(source, origin)) continue;
			// Friendly territory authorizes and prefers an approach, not a spawn site.
			float preferredBearing = Math.Atan2(origin[0] - targetPosition[0], origin[2] - targetPosition[2]);
			float authoredBearing;
			if (TryGetAuthoredFIAApproachBearing(targetId, ordinal, authoredBearing)) preferredBearing = authoredBearing;
			int sector = attempt % 16;
			int offset = (sector + 1) / 2;
			if (sector % 2 == 0) offset = -offset;
			int radiusBand = (attempt + ordinal + m_iQuerySerial) % 5;
			float angle = preferredBearing + offset * 0.392699;
			float radius = 300 + radiusBand * 50;
			vector candidate = targetPosition + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
			candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
			if (m_Navmesh && !m_Navmesh.IsTileValid(candidate)) m_Navmesh.LoadTileIn(candidate);
			bool overlaps;
			foreach (vector previous : used) if (HorizontalDistance(previous, candidate) < 40) overlaps = true;
			if (overlaps) { overlapRejected++; lastRejection = "group-overlap"; continue; }
			string rejection;
			float targetDistance = HorizontalDistance(candidate, targetPosition);
			if (targetDistance < 299 || targetDistance > 501) { lastRejection = "outside-FIA-annulus"; continue; }
			// FIA has its own annulus; retain the shared terrain/navmesh/LOS checks.
			if (!ValidateCandidate(targetPosition, candidate, observers, rejection, false))
			{
				lastRejection = rejection;
				if (rejection == "navigation-tile-invalid") navRejected++;
				else if (rejection == "water") waterRejected++;
				else if (rejection == "human-player-distance" || rejection == "human-player-direct-visibility") humanRejected++;
				else surfaceRejected++;
				continue;
			}
			result = candidate;
			PrintFormat("[TB_CC_FIA] staging target=%1 source=%2 group=%3 position=%4 targetDistance=%5 attempts=%6 physical=1", targetId, source, ordinal, candidate, HorizontalDistance(candidate, targetPosition), attempt + 1);
			return true;
		}
		PrintFormat("[TB_CC_SPAWN_DIAG] target=%1 attempts=%2 navRejected=%3 waterRejected=%4 humanRejected=%5 surfaceRejected=%6 overlapRejected=%7 lastRejection=%8 noSpawn=1 budgetGateReached=0", targetId, m_iLastAttemptsUsed, navRejected, waterRejected, humanRejected, surfaceRejected, overlapRejected, lastRejection);
		return FailQuery("NO_VALID_FIA_TERRITORIAL_STAGING_POSITION", startMs, failure);
	}

	protected bool ValidateSupportingTerrain(vector candidate, out string rejection)
	{
		vector start = candidate + Vector(0, 4, 0);
		vector end = candidate - Vector(0, 8, 0);
		TraceParam trace = MakeTraceParam(start, end, TraceFlags.ENTS | TraceFlags.WORLD);
		float fraction = GetGame().GetWorld().TraceMove(trace, FilterTraceHit);
		if (fraction >= 1.0)
		{
			rejection = "no-supporting-surface"; return false;
		}
		if (trace.TraceNorm[1] < 0.82)
		{
			rejection = "excessive-slope"; return false;
		}
		return true;
	}

	protected bool HasDirectVisibility(vector observer, vector candidate)
	{
		vector destination = candidate + Vector(0, 1.0, 0);
		TraceParam trace = MakeTraceParam(observer, destination, TraceFlags.ENTS | TraceFlags.WORLD);
		float fraction = GetGame().GetWorld().TraceMove(trace, FilterTraceHit);
		return fraction >= 0.98;
	}

	protected bool FilterTraceHit(notnull IEntity target, vector traceStart = "0 0 0", vector traceDirection = "0 0 0")
	{
		return SCR_Global.FilterCallback_IgnoreCharactersWithChildren(target);
	}

	protected void LogRejected(string targetId, int attempt, vector candidate, string reason)
	{
		if (!m_bDetailedDiagnostics) return;
		PrintFormat("[TB_PVE_M3_3] staging-candidate-rejected target=%1 attempt=%2 position=%3 reason=%4", targetId, attempt + 1, candidate, reason);
	}

	protected bool FailQuery(string reason, int startMs, out string failureReason)
	{
		failureReason = reason;
		m_iLastDurationMs = System.GetTickCount() - startMs;
		PrintFormat("[TB_PVE_M3_3] staging-failed reason=%1 attemptsUsed=%2 durationMs=%3 boundedAttempts=%4", reason, m_iLastAttemptsUsed, m_iLastDurationMs, m_iCandidateAttempts);
		return false;
	}

	protected float HorizontalDistance(vector a, vector b)
	{
		float dx = a[0] - b[0]; float dz = a[2] - b[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	protected float Bearing(vector position, vector target)
	{
		float bearing = Math.Atan2(position[0] - target[0], position[2] - target[2]) * 57.2957795;
		if (bearing < 0) bearing += 360;
		return bearing;
	}
	protected float BearingSeparation(float first, float second)
	{
		float separation = Math.AbsFloat(first - second);
		if (separation > 180) separation = 360 - separation;
		return separation;
	}
	float BearingSeparationForProof(float first, float second) { return BearingSeparation(first, second); }

	// Bounded acceptance seams: no target selection or spawn mutation.
	bool ValidateCandidateForProof(vector targetPosition, vector candidate, array<vector> observers, out string rejection)
	{
		return ValidateCandidate(targetPosition, candidate, observers, rejection);
	}
	bool FindAdditionalGroupPosition(FactionKey factionKey, string targetObjectiveId, vector targetPosition, array<vector> usedPositions, int groupOrdinal, array<vector> observerPositions, out vector stagingPosition, out string failureReason)
	{
		int startMs = System.GetTickCount();
		if (!Replication.IsServer()) return FailQuery("authority-required", startMs, failureReason);
		if (factionKey != "USSR" && factionKey != "FIA") return FailQuery("unsupported-operation-faction", startMs, failureReason);
		if (!usedPositions || usedPositions.IsEmpty()) return FailQuery("missing-first-axis", startMs, failureReason);
		if (factionKey == "FIA") return FindFIATerritorialStaging(targetObjectiveId, targetPosition, observerPositions, usedPositions, groupOrdinal, stagingPosition, failureReason);
		// Seek a genuinely distinct bearing first. A later pass may relax the
		// angle, but every candidate still passes all terrain/player/nav checks.
		int passCount = 3;
		if (groupOrdinal >= 3) passCount = 1;
		for (int pass; pass < passCount; pass++)
		{
			float requiredSeparation = 80;
			if (pass == 1) requiredSeparation = 45;
			if (pass == 2 || groupOrdinal >= 3) requiredSeparation = 0;
			for (int attempt; attempt < m_iCandidateAttempts; attempt++)
			{
				int directionIndex = (m_iQuerySerial * 3 + groupOrdinal * 7 + attempt * 5) % 16;
				int band = (attempt + attempt / 16 + groupOrdinal) % 4;
				float radius = m_fMinimumTargetDistance + band * ((m_fMaximumTargetDistance - m_fMinimumTargetDistance) / 3.0);
				float angle = directionIndex * 0.392699;
				vector candidate = targetPosition + Vector(Math.Sin(angle) * radius, 0, Math.Cos(angle) * radius);
				candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
				float minimumAngle = 360; bool tooClose;
				foreach (vector previous : usedPositions)
				{
					if (HorizontalDistance(candidate, previous) < 40) { tooClose = true; break; }
					float separation = BearingSeparation(Bearing(candidate, targetPosition), Bearing(previous, targetPosition));
					if (separation < minimumAngle) minimumAngle = separation;
				}
				if (tooClose || minimumAngle < requiredSeparation) continue;
				string rejection;
				if (!ValidateCandidate(targetPosition, candidate, observerPositions, rejection))
				{
					LogRejected(targetObjectiveId, attempt, candidate, "additional-" + rejection);
					continue;
				}
				stagingPosition = candidate;
				PrintFormat("[TB_PVE_M3_3] axis-selected target=%1 group=%2 bearing=%3 minimumSeparation=%4 position=%5 pass=%6", targetObjectiveId, groupOrdinal, Bearing(candidate, targetPosition), minimumAngle, candidate, pass);
				if (pass > 0 && groupOrdinal < 3) PrintFormat("[TB_PVE_M3_3] axis-degraded target=%1 group=%2 requestedSeparation=80 achieved=%3 reason=no-valid-wider-sector", targetObjectiveId, groupOrdinal, minimumAngle);
				return true;
			}
		}
		return FailQuery("NO_VALID_ADDITIONAL_STAGING_POSITION", startMs, failureReason);
	}
	void SetCandidateAttemptsForProof(int attempts) { m_iCandidateAttempts = Math.Max(attempts, 0); }
	void RestoreProductionCandidateAttempts() { m_iCandidateAttempts = m_iProductionCandidateAttempts; }
	vector GetLastAcceptedPosition() { return m_vLastAcceptedPosition; }
	float GetLastAcceptedDistance() { return m_fLastAcceptedDistance; }
	float GetLastAcceptedBearing() { return m_fLastAcceptedBearing; }
	int GetLastAttemptsUsed() { return m_iLastAttemptsUsed; }
	int GetLastDurationMs() { return m_iLastDurationMs; }
}
