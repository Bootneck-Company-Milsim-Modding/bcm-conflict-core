// Dedicated M4.4F acceptance sequencer. Modes:
// 4 East physical progression through BLUFOR terminal; 5 West two-step physical
// progression; 7 East advance-to-terminal-boundary then physical OPFOR retreat
// to the MOB; 8 diagnostic invalid fail-closed proof.
class TB_CC_M4_4_RemainingAuthorityProof
{
	protected static TB_CC_M4_4_ArlandGraphCampaignComponent s_Campaign;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_CampaignFaction s_USSR;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Actors = {};
	protected static int s_Mode;
	protected static int s_Ticks;
	protected static int s_Captures;
	protected static int s_PreviousRevision;
	protected static int s_MaxActors;
	protected static string s_Target;
	protected static FactionKey s_Attacker;

	static void Start(TB_CC_M4_4_ArlandGraphCampaignComponent campaign, int mode)
	{
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated || !campaign || !campaign.IsReady()) return;
		s_Campaign = campaign; s_Mode = mode; s_Ticks = 0; s_Captures = 0;
		s_US = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		s_USSR = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("USSR"));
		PrintFormat("[TB_CC_M4_4F] START mode=%1 mob=%2 endpoint=%3 revision=%4 directOwnership=0", mode, campaign.GetSelectedMobId(), campaign.GetSelectedEndpointId(), campaign.GetRevision());
		LogState("initial"); LogDeployment("initial");
		if (mode == 8) { campaign.TB_CC_M4_4_InjectInvalidForProof(); GetGame().GetCallqueue().CallLater(CheckInvalid, 2000); GetGame().GetCallqueue().CallLater(TB_CC_M4_4I_PlacementProbe.Run, 2000, true); return; }
		BeginNext();
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void BeginNext()
	{
		Cleanup();
		if (s_Mode == 5 && s_Captures >= 2) { Complete("west-two-step"); return; }
		if (s_Mode == 7 && s_Captures >= 1 && !TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() == s_Campaign.GetSelectedEndpointId())
		{
			s_Target = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(); s_Attacker = "USSR"; Spawn(s_Target, s_USSR); return;
		}
		if (s_Mode == 7 && s_Attacker == "USSR")
		{
			s_Target = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective();
			if (s_Target.IsEmpty()) { Fail("opfor-retreat-no-target"); return; }
			s_Attacker = "USSR"; Spawn(s_Target, s_USSR); return;
		}
		s_Target = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective();
		if (s_Target.IsEmpty()) { Fail("blufor-progression-no-target"); return; }
		s_Attacker = "US"; Spawn(s_Target, s_US);
	}

	protected static void Spawn(string id, SCR_CampaignFaction faction)
	{
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id);
		if (!base) { Fail("base-missing-" + id); return; }
		Resource resource = Resource.Load(faction.GetDefendersGroupPrefab()); EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; base.GetOwner().GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		if (!s_Group) { Fail("group-spawn-failed-" + id); return; }
		vector origin = base.GetOwner().GetOrigin(); int row; int column;
		for (int wave; wave < 10; wave++) foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
		{
			vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1; params.Transform[3] = position;
			IEntity actor = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params); if (actor) { s_Group.AddAgentFromControlledEntity(actor); s_Actors.Insert(actor); }
			column++; if (column == 5) { column = 0; row++; }
		}
		s_MaxActors = Math.Max(s_MaxActors, s_Actors.Count()); s_PreviousRevision = s_Campaign.GetRevision(); s_Ticks = 0;
		PrintFormat("[TB_CC_M4_4F] capture-begin sequence=%1 attacker=%2 id=%3 ownerBefore=%4 authorized=%5 actors=%6 LOD=%7", s_Captures + 1, faction.GetFactionKey(), id, Owner(id), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction.GetFactionKey(), id), s_Actors.Count(), s_Group.GetLOD());
	}

	protected static void Tick()
	{
		s_Ticks++;
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(s_Target); if (!base) { Fail("active-base-missing"); return; }
		if (s_Ticks % 5 == 0) PrintFormat("[TB_CC_M4_4F] capture-progress attacker=%1 id=%2 owner=%3 state=%4 revision=%5 ticks=%6", s_Attacker, s_Target, Owner(s_Target), base.GetCaptureState(), s_Campaign.GetRevision(), s_Ticks);
		if (Owner(s_Target) == s_Attacker)
		{
			s_Captures++; bool pass = s_Campaign.GetRevision() > s_PreviousRevision;
			PrintFormat("[TB_CC_M4_4F] %1 capture-finish sequence=%2 attacker=%3 id=%4 ownerAfter=%5 revisionBefore=%6 revisionAfter=%7", Result(pass), s_Captures, s_Attacker, s_Target, Owner(s_Target), s_PreviousRevision, s_Campaign.GetRevision());
			LogState("after-capture"); LogDeployment("after-capture");
			if (!pass) { Fail("revision-did-not-advance"); return; }
			int outcome = s_Campaign.GetOutcome();
			if (outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY || outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY)
			{
				bool expected = (s_Mode == 4 && outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY) || (s_Mode == 7 && outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY);
				PrintFormat("[TB_CC_M4_4F] %1 terminal outcome=%2 objectivesCleared=%3 capturesFrozenUS=%4 capturesFrozenUSSR=%5", Result(expected), outcome, TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty(), !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Target), !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", s_Target));
				Complete("terminal"); return;
			}
			BeginNext(); return;
		}
		if (s_Ticks > 180) { Fail("physical-capture-timeout-" + s_Target); return; }
	}

	protected static void CheckInvalid()
	{
		bool pass = s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.INVALID && TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty() && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", "ARLAND::MILITARY_SOUTH") && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", "ARLAND::HQ_EAST");
		PrintFormat("[TB_CC_M4_4F] %1 invalid-state validity=%2 outcome=%3 BLUFOR=%4 OPFOR=%5 noFalseVictory=%6", Result(pass), TB_PVE_M2_3_FrontierQuery.GetCampaignValidity(), s_Campaign.GetOutcome(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.INVALID);
		if (pass) Complete("invalid-fail-closed"); else Fail("invalid-fail-closed-mismatch");
	}

	protected static void LogState(string phase)
	{
		array<string> candidates = {}; array<string> friendly = {}; TB_PVE_M2_3_FrontierQuery.GetBLUFORCandidateObjectives(candidates); s_Campaign.GetEvaluator().GetFriendlyComponent(friendly);
		int depthBand = -1; TB_PVE_M3_5_ForceSelectionPolicy policy = s_Campaign.GetSelectionPolicy();
		if (policy) { TB_PVE_M3_5_SelectionDecision depth = policy.SelectFromContextForProof(1, s_Campaign.GetDepthPermille(), TB_PVE_M3_1_OperationState.SUCCEEDED); if (depth) depthBand = depth.m_iDepthBand; }
		PrintFormat("[TB_CC_M4_4F] state phase=%1 revision=%2 friendlyCount=%3 friendly=%4 candidates=%5 BLUFOR=%6 OPFOR=%7 depthPermille=%8 depthBand=%9", phase, s_Campaign.GetRevision(), friendly.Count(), friendly, candidates, TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_Campaign.GetDepthPermille(), depthBand);
		PrintFormat("[TB_CC_M4_4F] state-detail outcome=%1 evaluationMs=%2 projectionMs=%3", s_Campaign.GetOutcome(), s_Campaign.GetLastEvaluationDurationMs(), s_Campaign.GetLastProjectionDurationMs());
	}
	protected static void LogDeployment(string phase)
	{
		array<string> ids = {"ARLAND::HQ_EAST","ARLAND::HQ_WEST","ARLAND::AIRPORT_NORTH","ARLAND::FARM","ARLAND::BEAUREGARD","ARLAND::ARLEVILLE","ARLAND::MILITARY_SOUTH","ARLAND::MOSS_HILL"};
		foreach (string id : ids) { SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); SCR_SpawnPoint spawn = base.GetSpawnPoint(); bool enabled; if (spawn) enabled = spawn.IsSpawnPointEnabled(); PrintFormat("[TB_CC_M4_4F] deployment phase=%1 id=%2 owner=%3 valid=%4 enabled=%5 selected=%6", phase, id, Owner(id), spawn != null, enabled, id == s_Campaign.GetSelectedMobId()); }
	}
	protected static string Owner(string id) { string result = "<none>"; SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); if (base && base.GetFaction()) result = base.GetFaction().GetFactionKey(); return result; }
	protected static string Result(bool pass) { string result = "FAIL"; if (pass) result = "PASS"; return result; }
	protected static void Cleanup() { while (!s_Actors.IsEmpty()) { IEntity actor = s_Actors.Get(s_Actors.Count() - 1); s_Actors.Remove(s_Actors.Count() - 1); if (actor && !actor.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(actor); } if (s_Group && !s_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_Group); s_Group = null; }
	protected static void Complete(string scope) { Cleanup(); GetGame().GetCallqueue().Remove(Tick); PrintFormat("[TB_CC_M4_4F] COMPLETE PASS scope=%1 mode=%2 captures=%3 maxActors=%4 directOwnership=0", scope, s_Mode, s_Captures, s_MaxActors); }
	protected static void Fail(string reason) { Cleanup(); GetGame().GetCallqueue().Remove(Tick); PrintFormat("[TB_CC_M4_4F] COMPLETE FAIL reason=%1 mode=%2 captures=%3 target=%4 revision=%5", reason, s_Mode, s_Captures, s_Target, s_Campaign.GetRevision()); }
}
