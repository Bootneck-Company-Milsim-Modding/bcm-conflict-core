enum TB_CC_RestoreStage { UNINITIALIZED, CONTAINER_LOADING, STRATEGIC_RESTORE, DERIVED_REBUILD, READY, FAILED }

// Separate from the Phase 1 IO gate: container-ready is not gameplay-ready.
class TB_CC_RestoreTransaction
{
	protected int m_Stage = TB_CC_RestoreStage.UNINITIALIZED;
	protected bool m_OwnershipApplied;
	int GetStage() { return m_Stage; }
	bool IsReadyForCampaignConsumers() { return m_Stage == TB_CC_RestoreStage.READY; }
	bool IsRestoring() { return m_Stage == TB_CC_RestoreStage.CONTAINER_LOADING || m_Stage == TB_CC_RestoreStage.STRATEGIC_RESTORE || m_Stage == TB_CC_RestoreStage.DERIVED_REBUILD; }
	bool HasAppliedOwnership() { return m_OwnershipApplied; }
	protected void Transition(int stage, string name)
	{
		m_Stage = stage;
		PrintFormat("[TB_CC_PERSIST] action=RESTORE_STATE state=%1", name);
	}
	void Fail(string reason)
	{
		if (m_Stage == TB_CC_RestoreStage.FAILED) return;
		Transition(TB_CC_RestoreStage.FAILED, "FAILED");
		PrintFormat("[TB_CC_PERSIST] action=RESTORE_FAILED reason=%1", reason);
	}
	bool BeginContainerLoading()
	{
		if (!Replication.IsServer() || m_Stage != TB_CC_RestoreStage.UNINITIALIZED) return false;
		Transition(TB_CC_RestoreStage.CONTAINER_LOADING, "CONTAINER_LOADING");
		return true;
	}
	bool ContainerLoaded(bool snapshotPresent)
	{
		if (!Replication.IsServer() || m_Stage != TB_CC_RestoreStage.CONTAINER_LOADING) return false;
		Transition(TB_CC_RestoreStage.STRATEGIC_RESTORE, "STRATEGIC_RESTORE");
		PrintFormat("[TB_CC_PERSIST] action=RESTORE_BOUNDARY snapshotPresent=%1 mechanism=scoped-affiliation-callback-suppression", snapshotPresent);
		return true;
	}
	bool NoteOwnershipApplied()
	{
		if (!Replication.IsServer() || m_Stage != TB_CC_RestoreStage.STRATEGIC_RESTORE) return false;
		m_OwnershipApplied = true;
		return true;
	}
	bool BeginDerivedRebuild()
	{
		if (!Replication.IsServer() || m_Stage != TB_CC_RestoreStage.STRATEGIC_RESTORE) return false;
		Transition(TB_CC_RestoreStage.DERIVED_REBUILD, "DERIVED_REBUILD");
		return true;
	}
	bool CompleteDerivedRebuild(bool coherent)
	{
		if (!Replication.IsServer() || m_Stage != TB_CC_RestoreStage.DERIVED_REBUILD) return false;
		if (!coherent) { Fail("derived-rebuild-not-coherent"); return false; }
		Transition(TB_CC_RestoreStage.READY, "READY");
		return true;
	}
}

// A scoped setter seam, NOT a snapshot loader. Never used by Phase 2A startup.
// P2B must prevalidate the entire snapshot and reconcile native derived services
// before permitting READY. No gameplay-capture event may substitute for rebuild.
class TB_CC_QuietOwnershipRestore
{
	protected static ref array<IEntity> s_aScopes = {};
	static bool IsScoped(IEntity owner)
	{
		return Replication.IsServer() && owner && s_aScopes.Contains(owner);
	}
	static bool Apply(SCR_CampaignMilitaryBaseComponent base, SCR_CampaignFaction faction, TB_CC_RestoreTransaction transaction)
	{
		if (!Replication.IsServer() || !base || !faction || !transaction) return false;
		string factionKey = faction.GetFactionKey();
		if (factionKey != "US" && factionKey != "USSR" && factionKey != "FIA") return false;
		if (transaction.GetStage() != TB_CC_RestoreStage.STRATEGIC_RESTORE || base.IsBeingCaptured()) return false;
		IEntity owner = base.GetOwner();
		if (!owner || s_aScopes.Contains(owner)) return false;
		RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!replication || replication.IsProxy()) return false;
		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		if (!affiliation || !base.IsInitialized()) return false;
		if (base.GetFaction() == faction) return true;
		s_aScopes.Insert(owner);
		base.SetFaction(faction);
		s_aScopes.RemoveItem(owner);
		if (base.GetFaction() != faction) { transaction.Fail("quiet-owner-readback"); return false; }
		return transaction.NoteOwnershipApplied();
	}
	static bool ReconcileAffiliation(IEntity owner, Faction faction)
	{
		if (!Replication.IsServer() || !owner || !faction || s_aScopes.Contains(owner)) return false;
		SCR_FactionAffiliationComponent affiliation = SCR_FactionAffiliationComponent.Cast(owner.FindComponent(SCR_FactionAffiliationComponent));
		if (!affiliation) return true; // Native logic components without affiliation have nothing to rebuild.
		s_aScopes.Insert(owner);
		affiliation.SetAffiliatedFaction(faction);
		s_aScopes.RemoveItem(owner);
		return affiliation.GetAffiliatedFaction() == faction;
	}
}

// Deliberate service rebuild, not OnFactionChanged or OnBaseFactionChanged.
// These native methods are inspected in the installed 1.8 Game source.
modded class SCR_CampaignMilitaryBaseComponent
{
	bool TB_CC_ReconcileRestoredOwner(TB_CC_RestoreTransaction transaction)
	{
		if (!transaction || transaction.GetStage() != TB_CC_RestoreStage.DERIVED_REBUILD || IsProxy() || !IsInitialized() || IsBeingCaptured()) return false;
		SCR_CampaignFaction faction = GetCampaignFaction();
		if (!faction) return false;
		foreach (SCR_MilitaryBaseLogicComponent logic : m_aSystems)
		{
			if (!logic || !TB_CC_QuietOwnershipRestore.ReconcileAffiliation(logic.GetOwner(), faction)) return false;
		}
		m_OwningFactionPrevious = faction;
		m_mDefendersData.Clear();
		m_iSupplyRegenAmount = 0;
		ChangeFlags(faction);
		ChangeRadioSettings(faction);
		HandleSpawnPointFaction();
		SupplyIncomeTimer(true); // Reset deadline only; do not grant a supply package.
		CalculateSupplyRegenerationAmount(GetSupplies(), GetSuppliesMax(), faction);
		if (m_MapDescriptor && RplSession.Mode() != RplMode.Dedicated)
		{
			m_MapDescriptor.MapSetup(faction);
			m_MapDescriptor.HandleMapInfo(faction);
		}
		Replication.BumpMe();
		return GetFaction() == faction;
	}
}

modded class SCR_FactionAffiliationComponent
{
	override protected void OnFactionChanged(Faction previous, Faction current)
	{
		if (TB_CC_QuietOwnershipRestore.IsScoped(GetOwner())) return;
		super.OnFactionChanged(previous, current);
	}
}
