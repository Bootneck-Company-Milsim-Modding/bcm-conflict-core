class TB_CC_M4X_PhysicalProofComponentClass : ScriptComponentClass {}

// Dedicated-only service proof. No human-count override and no ownership
// assignment. Direct operation invocation is NOT coordinator acceptance.
class TB_CC_M4X_PhysicalProofComponent : ScriptComponent
{
	[Attribute(defvalue: "0", desc: "Separate dedicated two-position pressure observation only.")] protected bool m_bPressureOnly;
	protected TB_CC_M4X_NativeCampaignComponent m_Campaign;
	protected SCR_AIGroup m_USGroup;
	protected ref array<IEntity> m_USCharacters = {};
	protected ref TB_PVE_M3_1_ActivationLease m_USLease;
	protected ref TB_PVE_M3_1_Operation m_Operation;
	protected ref TB_PVE_M3_4_ForceAssemblyService m_Assembly;
	protected int m_iPhase;
	protected int m_iElapsed;
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer() && RplSession.Mode() == RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(StartProof, 22000);
	}
	protected void StartProof()
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName("TB_CC_NATIVE_CAMPAIGN");
		if (entity) m_Campaign = TB_CC_M4X_NativeCampaignComponent.Cast(entity.FindComponent(TB_CC_M4X_NativeCampaignComponent));
		if (!m_Campaign || !m_Campaign.IsReady()) { Finish(false, "campaign-not-ready"); return; }
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase("PROTO::OBJ_01");
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		if (!base || !us) { Finish(false, "required-service-missing"); return; }
		PrintFormat("[TB_CC_M4X_PROOF] START revision=%1 humanOverride=0 ownershipWrites=0 vanillaRadioAllows=%2 initialUSLegal=%3 OBJ02Authorized=%4", m_Campaign.GetRevision(), base.IsHQRadioTrafficPossible(us), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", "PROTO::OBJ_02"));
		m_Campaign.GetStagingPolicy().RequestNavigationForTarget(base.GetOwner().GetOrigin());
		SpawnUS("PROTO::OBJ_01");
		if (!m_USGroup) return;
		m_iPhase = 1; m_iElapsed = 0;
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}
	protected void SpawnUS(string id)
	{
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(id);
		SCR_CampaignFaction us = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; base.GetOwner().GetTransform(params.Transform);
		m_USGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(us.GetDefendersGroupPrefab()), GetGame().GetWorld(), params));
		if (!m_USGroup) { Finish(false, "US-group-spawn-failed"); return; }
		for (int wave; wave < 4; wave++) foreach (ResourceName unit : m_USGroup.m_aUnitPrefabSlots)
		{
			int index = m_USCharacters.Count();
			int column = index % 4;
			int row = index / 4;
			vector position = base.GetOwner().GetOrigin() + Vector(column * 2, 0, row * 2);
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1;
			params.Transform[3] = position;
			IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unit), GetGame().GetWorld(), params);
			if (character) { m_USGroup.AddAgentFromControlledEntity(character); m_USCharacters.Insert(character); }
		}
		m_USLease = new TB_PVE_M3_1_ActivationLease();
		if (!m_USLease.Acquire(m_USGroup, "M4X-physical-proof")) { Finish(false, "US-activation-failed"); return; }
	}
	protected void Tick()
	{
		m_iElapsed += 2;
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase("PROTO::OBJ_01");
		if (m_iElapsed % 10 == 0)
			PrintFormat("[TB_CC_M4X_PROOF] physical phase=%1 seconds=%2 owner=%3 captureState=%4 revision=%5 humansUS=%6", m_iPhase, m_iElapsed, base.GetFaction().GetFactionKey(), base.GetCaptureState(), m_Campaign.GetRevision(), TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US"));
		if (m_iPhase == 1 && base.GetFaction().GetFactionKey() == "US")
		{
			bool progressed = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() == "PROTO::OBJ_02" && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() == "PROTO::OBJ_01" && !base.GetSpawnPoint().IsSpawnPointEnabled();
			PrintFormat("[TB_CC_M4X_PROOF] US_CAPTURE pass=%1 revision=%2 depth=%3 OBJ01Deployable=%4 literalHuman=0", progressed, m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), base.GetSpawnPoint().IsSpawnPointEnabled());
			CleanupUS();
			if (!progressed) { Finish(false, "graph-or-deployment-transition"); return; }
			if (m_bPressureOnly)
			{
				SpawnUS("PROTO::OBJ_02"); m_iPhase = 3; m_iElapsed = 0; return;
			}
			Print("[TB_CC_M4X_PROOF] coordinator-human-gate retained=1 humansUS=0 automaticOperationAcceptance=PENDING; invoking M3.1 service proof only");
			m_Campaign.GetStagingPolicy().RequestNavigationForTarget(base.GetOwner().GetOrigin());
			m_iPhase = 11; m_iElapsed = 0;
			GetGame().GetCallqueue().CallLater(CreateServiceOperation, 5000);
		}
		else if (m_iPhase == 3 && m_Campaign.GetBase("PROTO::OBJ_02").GetFaction().GetFactionKey() == "US")
		{
			bool pressure = m_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE && m_Campaign.GetDepthPermille() == 1000 && m_Campaign.HasBLUFORFoothold() && m_Campaign.GetPressureStartCount() == 1;
			PrintFormat("[TB_CC_M4X_PROOF] BLUFOR_PRESSURE pass=%1 revision=%2 depth=%3 remaining=%4 startCount=%5 OBJ02Deployable=%6", pressure, m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), m_Campaign.GetPressureRemainingSeconds(), m_Campaign.GetPressureStartCount(), m_Campaign.GetBase("PROTO::OBJ_02").GetSpawnPoint().IsSpawnPointEnabled());
			Finish(pressure, "two-of-two-pressure-observed"); return;
		}
		else if (m_iPhase == 2)
		{
			m_Operation.Tick(m_iElapsed);
			if (m_Operation.IsTerminal())
			{
				bool retreated = m_Operation.GetState() == TB_PVE_M3_1_OperationState.SUCCEEDED && base.GetFaction().GetFactionKey() == "USSR" && TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() == "PROTO::OBJ_01" && m_Campaign.GetDepthPermille() == 0;
				PrintFormat("[TB_CC_M4X_PROOF] RECAPTURE pass=%1 operationState=%2 revision=%3 depth=%4 outcome=%5 foothold=%6", retreated, m_Operation.GetState(), m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), m_Campaign.GetOutcome(), m_Campaign.HasBLUFORFoothold());
				Finish(retreated, "physical-service-cycle"); return;
			}
		}
		if (m_iElapsed > 330) Finish(false, "physical-timeout");
	}
	protected void CreateServiceOperation()
	{
		m_Assembly = new TB_PVE_M3_4_ForceAssemblyService(); m_Operation = new TB_PVE_M3_1_Operation();
		if (!m_Operation.Create("M4X_SERVICE_PROOF", m_Campaign.GetStagingPolicy(), m_Assembly, "LIGHT")) { Finish(false, "operation-service-create-failed"); return; }
		m_iPhase = 2; m_iElapsed = 0;
	}
	protected void CleanupUS()
	{
		if (m_USLease) m_USLease.Release("M4X-proof-cleanup");
		foreach (IEntity character : m_USCharacters) if (character && !character.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(character);
		m_USCharacters.Clear();
		if (m_USGroup && !m_USGroup.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(m_USGroup);
		m_USGroup = null;
	}
	protected void Finish(bool pass, string reason)
	{
		GetGame().GetCallqueue().Remove(Tick); GetGame().GetCallqueue().Remove(CreateServiceOperation); CleanupUS();
		if (m_Operation) m_Operation.CleanupAssignedForce("M4X-proof-finished");
		PrintFormat("[TB_CC_M4X_PROOF] COMPLETE pass=%1 reason=%2 humanOverride=0 ownershipWrites=0 coordinatorHumanAcceptance=PENDING", pass, reason);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(StartProof); GetGame().GetCallqueue().Remove(Tick); GetGame().GetCallqueue().Remove(CreateServiceOperation); CleanupUS();
		if (m_Operation) m_Operation.CleanupAssignedForce("M4X-proof-deleted");
		super.OnDelete(owner);
	}
}
