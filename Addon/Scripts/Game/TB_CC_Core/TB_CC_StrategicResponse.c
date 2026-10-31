enum TB_CC_ResponsePhase { CONTACT, COLLAPSE, POST_CAPTURE_1, POST_CAPTURE_2, PRESSURE_RECOVERY }

// Map-independent scheduling data. Operations, never this queue, own AI.
class TB_CC_ResponseConfig
{
	bool m_bContactResponseEnabled = true;
	bool m_bCollapseResponseEnabled = true;
	int m_iCollapseDefenderThreshold = 5;
	int m_iPostCaptureWaveCount = 2;
	int m_iPostCaptureDelayMinSeconds = 120;
	int m_iPostCaptureDelayMaxSeconds = 240;
	bool m_bPressureRecoveryEnabled = true;
	string m_sPressureRecoveryPackage = "HEAVY";
	int m_iPressureRetrySeconds = 15;
	int m_iPressureMinimumRemainingSeconds = 30;
}

class TB_CC_ResponseEpoch
{
	string m_sTarget;
	FactionKey m_sOwner;
	FactionKey m_sFormerOwner;
	int m_iEpoch;
	bool m_bContact;
	bool m_bCollapse;
	int m_iLastDefenders = -1;
}

class TB_CC_ResponseRequest
{
	string m_sTarget;
	FactionKey m_sFaction;
	FactionKey m_sExpectedOwner;
	int m_iEpoch;
	int m_iPhase;
	int m_iDueMs;
	int m_iSerial;
	bool m_bFinished;
	bool m_bAdmitting;
	ref TB_PVE_M3_1_Operation m_Operation;
}

class TB_CC_ResponseAdapter
{
	bool IsOperational() { return false; }
	bool IsOrdinary(string id) { return false; }
	FactionKey Owner(string id) { return ""; }
	bool CanRespond(FactionKey faction, string id, bool reinforcement) { return false; }
	int PressureRemaining() { return -1; }
	bool FinalDefence() { return false; }
	string RecoveryTarget() { return ""; }
	string Package(TB_CC_ResponseRequest request) { return ""; }
	TB_PVE_M3_1_Operation Launch(TB_CC_ResponseRequest request, string packageId) { return null; }
}

class TB_CC_StrategicResponse
{
	protected ref TB_CC_ResponseAdapter m_Map;
	protected ref TB_CC_ResponseConfig m_Config;
	protected ref map<string, ref TB_CC_ResponseEpoch> m_Epochs = new map<string, ref TB_CC_ResponseEpoch>();
	protected ref array<ref TB_CC_ResponseRequest> m_Requests = {};
	protected int m_iSeed;
	protected int m_iSerial;
	protected int m_iNextRecoveryMs;
	protected bool m_bWasPressure;
	static string PhaseName(int phase)
	{
		if (phase == TB_CC_ResponsePhase.CONTACT) return "CONTACT";
		if (phase == TB_CC_ResponsePhase.COLLAPSE) return "COLLAPSE";
		if (phase == TB_CC_ResponsePhase.POST_CAPTURE_1) return "POST_CAPTURE_1";
		if (phase == TB_CC_ResponsePhase.POST_CAPTURE_2) return "POST_CAPTURE_2";
		return "PRESSURE_RECOVERY";
	}
	bool HasDue(FactionKey faction, int now)
	{
		foreach (TB_CC_ResponseRequest request : m_Requests)
			if (!request.m_Operation && request.m_sFaction == faction && request.m_iDueMs <= now && IsCurrent(request)) return true;
		return false;
	}
	void Initialize(TB_CC_ResponseAdapter adapter, TB_CC_ResponseConfig config, int seed)
	{
		m_Map = adapter; m_Config = config; m_iSeed = seed;
	}
	void Seed(string id, FactionKey faction)
	{
		if (m_Epochs.Contains(id)) return;
		TB_CC_ResponseEpoch epoch = new TB_CC_ResponseEpoch();
		epoch.m_sTarget = id; epoch.m_sOwner = faction; m_Epochs.Set(id, epoch);
	}
	protected int Delay(string id, int epoch, int phase)
	{
		// Stable ordinal hash; bounded arithmetic, no client PRNG or wall-clock seed.
		int value = Math.AbsInt(m_iSeed % 100003);
		for (int index; index < id.Length(); index++) value = (value * 31 + id.ToAscii(index)) % 100003;
		value = (value + (epoch % 100003) * 17 + phase * 47) % 100003;
		int low = Math.Clamp(m_Config.m_iPostCaptureDelayMinSeconds, 120, 240);
		int high = Math.Clamp(m_Config.m_iPostCaptureDelayMaxSeconds, low, 240);
		return low + value % (high - low + 1);
	}
	protected void Queue(TB_CC_ResponseEpoch epoch, int phase, FactionKey faction, int now)
	{
		TB_CC_ResponseRequest request = new TB_CC_ResponseRequest();
		request.m_sTarget = epoch.m_sTarget; request.m_sFaction = faction;
		request.m_sExpectedOwner = epoch.m_sOwner; request.m_iEpoch = epoch.m_iEpoch;
		request.m_iPhase = phase; m_iSerial++; request.m_iSerial = m_iSerial;
		int seconds;
		if (phase == TB_CC_ResponsePhase.POST_CAPTURE_1 || phase == TB_CC_ResponsePhase.POST_CAPTURE_2) seconds = Delay(epoch.m_sTarget, epoch.m_iEpoch, phase);
		request.m_iDueMs = now + seconds * 1000; m_Requests.Insert(request);
		PrintFormat("[TB_CC_RESPONSE] target=%1 faction=%2 phase=%3 epoch=%4 delaySeconds=%5 scheduledAt=%6 status=QUEUED", request.m_sTarget, faction, PhaseName(phase), request.m_iEpoch, seconds, request.m_iDueMs);
	}
	void ObserveOwner(string id, FactionKey faction, int now)
	{
		if (!Replication.IsServer() || !m_Map.IsOrdinary(id)) return;
		if (!m_Epochs.Contains(id)) { Seed(id, faction); return; }
		TB_CC_ResponseEpoch epoch = m_Epochs.Get(id);
		if (epoch.m_sOwner == faction) return;
		FactionKey previous = epoch.m_sOwner;
		if (previous == "US" && faction != "US" && m_Map.PressureRemaining() >= 0)
			PrintFormat("[TB_CC_PRESSURE] state=BROKEN remaining=%1 lostPosition=%2 newOwner=%3", m_Map.PressureRemaining(), id, faction);
		epoch.m_sOwner = faction;
		// The enemy -> US transition continues the assault epoch for post waves.
		// Any subsequent ownership change invalidates all earlier queued requests.
		if (faction == "US" && (previous == "FIA" || previous == "USSR"))
		{
			epoch.m_sFormerOwner = previous;
			if (m_Config.m_iPostCaptureWaveCount > 0) Queue(epoch, TB_CC_ResponsePhase.POST_CAPTURE_1, previous, now);
		}
		else
		{
			epoch.m_iEpoch++; epoch.m_sFormerOwner = "";
			epoch.m_bContact = false; epoch.m_bCollapse = false; epoch.m_iLastDefenders = -1;
		}
	}
	void ObserveAssault(string id, bool humanUSInCaptureArea, int livingEligibleDefenders, int now)
	{
		if (!Replication.IsServer() || !m_Map.IsOperational() || !humanUSInCaptureArea || !m_Epochs.Contains(id)) return;
		TB_CC_ResponseEpoch epoch = m_Epochs.Get(id);
		if (epoch.m_sOwner != "FIA" && epoch.m_sOwner != "USSR") return;
		if (!epoch.m_bContact)
		{
			epoch.m_bContact = true;
			if (m_Config.m_bContactResponseEnabled) Queue(epoch, TB_CC_ResponsePhase.CONTACT, epoch.m_sOwner, now);
		}
		if (!epoch.m_bCollapse && livingEligibleDefenders >= 0 && livingEligibleDefenders <= m_Config.m_iCollapseDefenderThreshold)
		{
			epoch.m_bCollapse = true;
			if (m_Config.m_bCollapseResponseEnabled) Queue(epoch, TB_CC_ResponsePhase.COLLAPSE, epoch.m_sOwner, now);
			PrintFormat("[TB_CC_RESPONSE] target=%1 phase=COLLAPSE defendersRemaining=%2 previousDefenders=%3 epoch=%4", id, livingEligibleDefenders, epoch.m_iLastDefenders, epoch.m_iEpoch);
		}
		epoch.m_iLastDefenders = livingEligibleDefenders;
	}
	bool IsCurrent(TB_CC_ResponseRequest request)
	{
		if (!request || request.m_bFinished || !m_Map.IsOperational()) return false;
		TB_CC_ResponseEpoch epoch = m_Epochs.Get(request.m_sTarget);
		if (!epoch || epoch.m_iEpoch != request.m_iEpoch || m_Map.Owner(request.m_sTarget) != request.m_sExpectedOwner) return false;
		if (request.m_sFaction == "USSR" && m_Map.FinalDefence()) return false;
		if (request.m_iPhase == TB_CC_ResponsePhase.PRESSURE_RECOVERY && m_Map.PressureRemaining() < 0) return false;
		return m_Map.CanRespond(request.m_sFaction, request.m_sTarget, request.m_iPhase <= TB_CC_ResponsePhase.COLLAPSE);
	}
	bool Authorizes(FactionKey faction, string id)
	{
		foreach (TB_CC_ResponseRequest request : m_Requests)
			if ((request.m_bAdmitting || request.m_Operation) && request.m_sFaction == faction && request.m_sTarget == id && IsCurrent(request)) return true;
		return false;
	}
	void Tick(int now)
	{
		if (!Replication.IsServer()) return;
		bool pressure = m_Map.IsOperational() && m_Map.PressureRemaining() >= 0 && !m_Map.FinalDefence();
		if (pressure != m_bWasPressure)
		{
			m_bWasPressure = pressure; m_iNextRecoveryMs = now;
			PrintFormat("[TB_CC_PRESSURE] recoveryMode=%1 remaining=%2", pressure, m_Map.PressureRemaining());
		}
		bool recoveryPending;
		foreach (TB_CC_ResponseRequest request : m_Requests)
			if (!request.m_bFinished && request.m_iPhase == TB_CC_ResponsePhase.PRESSURE_RECOVERY) recoveryPending = true;
		if (pressure && m_Config.m_bPressureRecoveryEnabled && !recoveryPending && now >= m_iNextRecoveryMs && m_Map.PressureRemaining() >= m_Config.m_iPressureMinimumRemainingSeconds)
		{
			string target = m_Map.RecoveryTarget();
			TB_CC_ResponseEpoch epoch = m_Epochs.Get(target);
			if (epoch) Queue(epoch, TB_CC_ResponsePhase.PRESSURE_RECOVERY, "USSR", now);
			m_iNextRecoveryMs = now + Math.Max(m_Config.m_iPressureRetrySeconds, 2) * 1000;
		}
		// Snapshot size: a launched post wave may append its delayed successor.
		int count = m_Requests.Count();
		for (int index; index < count; index++)
		{
			TB_CC_ResponseRequest request = m_Requests[index];
			if (request.m_bFinished) continue;
			if (request.m_Operation && request.m_Operation.IsTerminal())
			{
				request.m_bFinished = true;
				PrintFormat("[TB_CC_RESPONSE] target=%1 phase=%2 epoch=%3 status=RESOLVED result=%4", request.m_sTarget, PhaseName(request.m_iPhase), request.m_iEpoch, request.m_Operation.GetState());
				continue;
			}
			if (!IsCurrent(request))
			{
				request.m_bFinished = true;
				PrintFormat("[TB_CC_RESPONSE] target=%1 phase=%2 epoch=%3 status=CANCELLED reason=ownership-capability-or-campaign", request.m_sTarget, PhaseName(request.m_iPhase), request.m_iEpoch);
				continue;
			}
			if (request.m_Operation || now < request.m_iDueMs) continue;
			string packageId = m_Map.Package(request);
			if (request.m_iPhase == TB_CC_ResponsePhase.PRESSURE_RECOVERY) packageId = m_Config.m_sPressureRecoveryPackage;
			request.m_bAdmitting = true;
			request.m_Operation = m_Map.Launch(request, packageId);
			request.m_bAdmitting = false;
			if (!request.m_Operation) { request.m_iDueMs = now + 2000; continue; }
			PrintFormat("[TB_CC_RESPONSE] target=%1 faction=%2 phase=%3 epoch=%4 package=%5 status=LAUNCHED", request.m_sTarget, request.m_sFaction, PhaseName(request.m_iPhase), request.m_iEpoch, request.m_Operation.GetPackageId());
			if (request.m_iPhase == TB_CC_ResponsePhase.PRESSURE_RECOVERY)
				PrintFormat("[TB_CC_PRESSURE] state=RECOVERY_ACTIVE remaining=%1 target=%2 operation=%3", m_Map.PressureRemaining(), request.m_sTarget, request.m_Operation.GetOperationId());
			if (request.m_iPhase == TB_CC_ResponsePhase.POST_CAPTURE_1 && m_Config.m_iPostCaptureWaveCount >= 2)
				Queue(m_Epochs.Get(request.m_sTarget), TB_CC_ResponsePhase.POST_CAPTURE_2, request.m_sFaction, now);
		}
		for (int removeIndex = m_Requests.Count() - 1; removeIndex >= 0; removeIndex--)
			if (m_Requests[removeIndex].m_bFinished && (!m_Requests[removeIndex].m_Operation || m_Requests[removeIndex].m_Operation.IsTerminal())) m_Requests.Remove(removeIndex);
	}
}
