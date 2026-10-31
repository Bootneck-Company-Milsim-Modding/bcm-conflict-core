// Authority-local ownership epochs: bounded to one current opportunity per
// registered ordinary position. Repeated callbacks cannot generate new attacks.
class TB_CC_FIARecaptureOpportunity
{
	string m_sTarget;
	string m_sOwner;
	int m_iEpoch;
	int m_iRevision;
	int m_iAttempts;
	bool m_bFinished;
	string m_sDeferredReason;
	int m_iNextLogMs;
}

class TB_CC_FIARecaptureLedger
{
	protected ref map<string, string> m_Owners = new map<string, string>();
	protected ref map<string, int> m_Epochs = new map<string, int>();
	protected ref map<string, ref TB_CC_FIARecaptureOpportunity> m_Current = new map<string, ref TB_CC_FIARecaptureOpportunity>();
	protected ref array<ref TB_CC_FIARecaptureOpportunity> m_Queue = {};
	void ExportStrategicState(map<string, string> values, array<string> ids)
	{
		foreach (string id : ids)
		{
			values.Set("FIA-epoch:" + id, m_Epochs.Get(id).ToString());
			TB_CC_FIARecaptureOpportunity opportunity = m_Current.Get(id);
			int pending; int attempts; int revision; int queueOrder = -1;
			if (opportunity && !opportunity.m_bFinished)
			{
				// Only unlaunched intent survives P2B. In-flight reconstruction is Phase 4.
				if (m_Queue.Contains(opportunity)) { pending = 1; queueOrder = m_Queue.Find(opportunity); }
				attempts = opportunity.m_iAttempts; revision = opportunity.m_iRevision;
			}
			values.Set("FIA-pending:" + id, pending.ToString());
			values.Set("FIA-attempts:" + id, attempts.ToString());
			values.Set("FIA-revision:" + id, revision.ToString());
			values.Set("FIA-queueOrder:" + id, queueOrder.ToString());
		}
	}
	void RestoreStrategicState(map<string, string> values, array<string> ids, map<string, string> owners)
	{
		m_Owners.Clear(); m_Epochs.Clear(); m_Current.Clear(); m_Queue.Clear();
		foreach (string id : ids)
		{
			m_Owners.Set(id, owners.Get(id));
			int epoch = values.Get("FIA-epoch:" + id).ToInt();
			m_Epochs.Set(id, epoch);
		}
		for (int queueOrder; queueOrder < ids.Count(); queueOrder++)
		{
			foreach (string queuedId : ids)
			{
				if (values.Get("FIA-pending:" + queuedId) != "1" || values.Get("FIA-queueOrder:" + queuedId).ToInt() != queueOrder) continue;
				TB_CC_FIARecaptureOpportunity opportunity = new TB_CC_FIARecaptureOpportunity();
				opportunity.m_sTarget = queuedId; opportunity.m_sOwner = owners.Get(queuedId); opportunity.m_iEpoch = m_Epochs.Get(queuedId);
				opportunity.m_iAttempts = values.Get("FIA-attempts:" + queuedId).ToInt(); opportunity.m_iRevision = values.Get("FIA-revision:" + queuedId).ToInt();
				m_Current.Set(queuedId, opportunity); m_Queue.Insert(opportunity);
			}
		}
	}

	void Seed(string target, string owner)
	{
		if (!m_Owners.Contains(target)) m_Owners.Set(target, owner);
	}
	void Complete(TB_CC_FIARecaptureOpportunity opportunity, string result, string reason)
	{
		if (!opportunity || opportunity.m_bFinished) return;
		opportunity.m_bFinished = true;
		m_Queue.RemoveItemOrdered(opportunity);
		PrintFormat("[TB_CC_FIA_RECAPTURE] completed target=%1 epoch=%2 result=%3 reason=%4", opportunity.m_sTarget, opportunity.m_iEpoch, result, reason);
	}
	void Observe(string target, string owner, int revision)
	{
		if (!m_Owners.Contains(target) || m_Owners.Get(target) == owner) return;
		m_Owners.Set(target, owner);
		int epoch = m_Epochs.Get(target) + 1;
		m_Epochs.Set(target, epoch);
		TB_CC_FIARecaptureOpportunity old = m_Current.Get(target);
		// A running operation may finish its genuine FIA capture/consolidation;
		// its coordinator resolves that outcome, rather than labelling it stale.
		if (old && m_Queue.Contains(old)) Complete(old, "CANCELLED", "ownership-changed-before-launch");
		m_Current.Remove(target);
		if (owner != "US" && owner != "USSR") return;
		TB_CC_FIARecaptureOpportunity opportunity = new TB_CC_FIARecaptureOpportunity();
		opportunity.m_sTarget = target; opportunity.m_sOwner = owner;
		opportunity.m_iEpoch = epoch; opportunity.m_iRevision = revision;
		m_Current.Set(target, opportunity); m_Queue.Insert(opportunity);
		PrintFormat("[TB_CC_FIA_RECAPTURE] queued target=%1 revision=%2 epoch=%3 owner=%4 queueDepth=%5", target, revision, epoch, owner, m_Queue.Count());
	}
	bool IsCurrent(TB_CC_FIARecaptureOpportunity opportunity) { return opportunity && m_Current.Get(opportunity.m_sTarget) == opportunity; }
	TB_CC_FIARecaptureOpportunity Peek()
	{
		if (m_Queue.IsEmpty()) return null;
		return m_Queue[0];
	}
	void Started(TB_CC_FIARecaptureOpportunity opportunity)
	{
		m_Queue.RemoveItemOrdered(opportunity);
		PrintFormat("[TB_CC_FIA_RECAPTURE] started target=%1 mode=VIRTUAL attempt=%2 epoch=%3", opportunity.m_sTarget, opportunity.m_iAttempts, opportunity.m_iEpoch);
	}
	void DeferAdmission(TB_CC_FIARecaptureOpportunity opportunity)
	{
		// One admission attempt per FIFO round: a temporarily unavailable
		// resource must not permanently block every later eligible capture.
		m_Queue.RemoveItemOrdered(opportunity); m_Queue.Insert(opportunity);
	}
	void CancelPending(string reason)
	{
		while (!m_Queue.IsEmpty()) Complete(m_Queue[0], "CANCELLED", reason);
	}
	void CancelTarget(string target, string reason)
	{
		TB_CC_FIARecaptureOpportunity opportunity = m_Current.Get(target);
		if (opportunity && m_Queue.Contains(opportunity)) Complete(opportunity, "CANCELLED", reason);
	}
}
