// Dedicated-only M4.4H acceptance. Mode 9 proves BLUFOR pressure;
// mode 10 proves OPFOR pressure. Ownership changes remain vanilla-owned.
class TB_CC_M4_4H_VictoryPressureProof
{
	protected static TB_CC_M4_4_ArlandGraphCampaignComponent s_Campaign;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_CampaignFaction s_USSR;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Actors = {};
	protected static int s_Mode;
	protected static int s_Phase;
	protected static int s_Ticks;
	protected static int s_Captures;
	protected static int s_RevisionBefore;
	protected static int s_PressureStartBefore;
	protected static string s_Target;
	protected static FactionKey s_Attacker;
	protected static SCR_CampaignSeizingComponent s_ObservedSeizing;
	protected static ref array<string> s_InterruptedTargets = {};
	protected static bool s_WaitingInterruption;

	static void Start(TB_CC_M4_4_ArlandGraphCampaignComponent campaign, int mode)
	{
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.Dedicated || !campaign || !campaign.IsReady()) return;
		s_Campaign = campaign; s_Mode = mode; s_Phase = 0; s_Ticks = 0; s_Captures = 0;
		s_US = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		s_USSR = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("USSR"));
		PrintFormat("[TB_CC_M4_4H_PROOF] START mode=%1 mob=%2 endpoint=%3 outcome=%4 requiredOrdinary=%5 holdSeconds=60 directOwnership=0", mode, campaign.GetSelectedMobId(), campaign.GetSelectedEndpointId(), campaign.GetOutcome(), campaign.GetRequiredOrdinaryPositionCount());
		LogAnchors("initial");
		LogDeployment("initial");
		bool fresh = campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && !campaign.HasBLUFORFoothold() && !campaign.IsVictoryPressureActive() && campaign.GetPressureRemainingSeconds() == -1 && campaign.GetPressureStartCount() == 0 && campaign.GetCoordinator() && campaign.GetGarrisonManager();
		PrintFormat("[TB_CC_M4_4J_PROOF] %1 fresh-start foothold=%2 outcome=%3 remaining=%4 starts=%5 M3Operational=%6", Result(fresh), campaign.HasBLUFORFoothold(), campaign.GetOutcome(), campaign.GetPressureRemainingSeconds(), campaign.GetPressureStartCount(), campaign.GetCoordinator() && campaign.GetGarrisonManager());
		if (!fresh) { Fail("fresh-start-contract"); return; }
		if (TB_CC_VictoryPressurePresentation.Countdown(277) != "04:37") { Fail("countdown-format"); return; }
		TB_PVE_M3_3_TacticalStagingPolicy transportPolicy = new TB_PVE_M3_3_TacticalStagingPolicy();
		transportPolicy.Configure(350, 500, 250, 900, 20, 12, false);
		transportPolicy.AddUnsupportedFootTarget("ARLAND::GULL_ISLAND");
		vector islandPosition; vector staging; string reason; array<vector> observers = {};
		TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition("ARLAND::GULL_ISLAND", islandPosition);
		bool staged = transportPolicy.FindStagingPosition("USSR", "ARLAND::GULL_ISLAND", islandPosition, observers, staging, reason);
		PrintFormat("[TB_CC_M4_4I_PROOF] %1 island-foot-staging allowed=%2 reason=%3 actorsCreated=0", Result(!staged && reason == "unsupported-transport-requirement"), staged, reason);
		if (staged || reason != "unsupported-transport-requirement") { Fail("island-transport-guard"); return; }
		if (mode == 10)
		{
			s_Phase = 18; Capture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_US); // first genuine foothold
		}
		else
		{
			s_Phase = 1; Capture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_US);
		}
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
	}

	protected static void Tick()
	{
		s_Ticks++;
		if (s_Phase == 90) { CheckTerminal(); return; }
		if (s_Target.IsEmpty()) { Fail("empty-target"); return; }
		SCR_CampaignMilitaryBaseComponent targetBase = s_Campaign.GetBase(s_Target);
		if (s_WaitingInterruption)
		{
			if (targetBase.GetCaptureState() != SCR_EBaseCaptureState.NONE) { if (s_Ticks > 30) Fail("interruption-timeout"); return; }
			bool stable = Owner(s_Target) == "USSR" && s_Campaign.GetRevision() == s_RevisionBefore;
			PrintFormat("[TB_CC_M4_4I_PROOF] %1 interruption id=%2 owner=%3 revisionStable=%4", Result(stable), s_Target, Owner(s_Target), stable);
			if (!stable) { Fail("interruption-mutated-ownership-or-revision"); return; }
			s_WaitingInterruption = false; Capture(s_Target, s_US); return;
		}
		if (s_Mode == 9 && s_Phase == 1 && IsNewPosition(s_Target) && !s_InterruptedTargets.Contains(s_Target) && targetBase.GetCaptureState() == SCR_EBaseCaptureState.CAPTURING && Owner(s_Target) == "USSR")
		{
			s_InterruptedTargets.Insert(s_Target);
			PrintFormat("[TB_CC_M4_4I_PROOF] interruption-start id=%1 state=%2 actors=%3", s_Target, targetBase.GetCaptureState(), s_Actors.Count());
			Cleanup(); s_WaitingInterruption = true; s_Ticks = 0; return;
		}
		if (s_Ticks % 5 == 0 && s_ObservedSeizing)
		{
			SCR_Faction prevailing = s_ObservedSeizing.TB_PVE_M1_4A_GetPrevailingFaction(); string prevailingKey = "<none>"; if (prevailing) prevailingKey = prevailing.GetFactionKey();
			PrintFormat("[TB_CC_M4_4I_PROOF] physical id=%1 state=%2 prevailing=%3 start=%4 end=%5 actors=%6", s_Target, targetBase.GetCaptureState(), prevailingKey, s_ObservedSeizing.GetSeizingStartTimestamp(), s_ObservedSeizing.GetSeizingEndTimestamp(), s_Actors.Count());
		}
		if (s_Ticks % 5 == 0) PrintFormat("[TB_CC_M4_4H_PROOF] progress phase=%1 attacker=%2 target=%3 owner=%4 outcome=%5 remaining=%6 revision=%7", s_Phase, s_Attacker, s_Target, Owner(s_Target), s_Campaign.GetOutcome(), s_Campaign.GetPressureRemainingSeconds(), s_Campaign.GetRevision());
		if (Owner(s_Target) != s_Attacker) { if (s_Ticks > 180) Fail("physical-capture-timeout-" + s_Target); return; }
		s_Captures++;
		bool revisionAdvanced = s_Campaign.GetRevision() > s_RevisionBefore;
		PrintFormat("[TB_CC_M4_4H_PROOF] %1 vanilla-capture phase=%2 attacker=%3 id=%4 revisionBefore=%5 revisionAfter=%6 outcome=%7", Result(revisionAdvanced), s_Phase, s_Attacker, s_Target, s_RevisionBefore, s_Campaign.GetRevision(), s_Campaign.GetOutcome());
		Cleanup(); s_Ticks = 0;
		if (!revisionAdvanced) { Fail("revision-did-not-advance"); return; }
		LogDeployment("after-" + s_Target);
		if (!CheckPresentationProjection()) { Fail("pressure-presentation-projection"); return; }
		if (s_Mode == 9) AdvanceBLUFORProof(); else AdvanceOPFORProof();
	}

	protected static void AdvanceBLUFORProof()
	{
		if (s_Phase == 1 && s_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE)
		{
			s_Phase = 1; Capture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_US); return;
		}
		if (s_Phase == 1)
		{
			PrintFormat("[TB_CC_M4_4H_PROOF] PASS BLUFOR-pressure-enter depth=%1 starts=%2 remaining=%3 endpointOwner=%4", s_Campaign.GetDepthPermille(), s_Campaign.GetPressureStartCount(), s_Campaign.GetPressureRemainingSeconds(), Owner(s_Campaign.GetSelectedEndpointId()));
			s_PressureStartBefore = s_Campaign.GetPressureStartCount(); s_Phase = 2; Capture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_USSR); return;
		}
		if (s_Phase == 2)
		{
			bool cancelled = s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && !s_Campaign.IsVictoryPressureActive();
			PrintFormat("[TB_CC_M4_4H_PROOF] %1 BLUFOR-pressure-cancel outcome=%2 legal=%3 timerActive=%4", Result(cancelled), s_Campaign.GetOutcome(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_Campaign.IsVictoryPressureActive());
			if (!cancelled) { Fail("blufor-pressure-not-cancelled"); return; }
			s_Phase = 3; Capture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_US); return;
		}
		bool restarted = s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE && s_Campaign.GetPressureStartCount() == s_PressureStartBefore + 1;
		PrintFormat("[TB_CC_M4_4H_PROOF] %1 BLUFOR-pressure-restart starts=%2 remaining=%3", Result(restarted), s_Campaign.GetPressureStartCount(), s_Campaign.GetPressureRemainingSeconds());
		if (!restarted) { Fail("blufor-pressure-not-restarted"); return; }
		s_Phase = 90;
	}

	protected static void AdvanceOPFORProof()
	{
		if (s_Phase == 18)
		{
			bool established = s_Campaign.HasBLUFORFoothold() && s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && s_Campaign.GetDepthPermille() == 1000 / s_Campaign.GetRequiredOrdinaryPositionCount();
			PrintFormat("[TB_CC_M4_4J_PROOF] %1 first-capture foothold=%2 depth=%3 outcome=%4", Result(established), s_Campaign.HasBLUFORFoothold(), s_Campaign.GetDepthPermille(), s_Campaign.GetOutcome());
			if (!established) { Fail("foothold-not-established"); return; }
			s_Phase = 19; Capture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_USSR); return;
		}
		if (s_Phase == 19)
		{
			bool entered = s_Campaign.HasBLUFORFoothold() && s_Campaign.IsVictoryPressureActive() && s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE && s_Campaign.GetPressureStartCount() == 1;
			PrintFormat("[TB_CC_M4_4J_PROOF] %1 first-retreat foothold=%2 starts=%3 remaining=%4 title=%5", Result(entered), s_Campaign.HasBLUFORFoothold(), s_Campaign.GetPressureStartCount(), s_Campaign.GetPressureRemainingSeconds(), TB_CC_VictoryPressurePresentation.Title(s_Campaign.GetOutcome()));
			if (!entered) { Fail("first-retreat-pressure"); return; }
			s_PressureStartBefore = s_Campaign.GetPressureStartCount(); s_Phase = 20; Capture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), s_US); return;
		}
		if (s_Phase == 20)
		{
			bool cancelled = s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && !s_Campaign.IsVictoryPressureActive() && s_Campaign.GetPressureRemainingSeconds() == -1 && s_Campaign.HasBLUFORFoothold();
			PrintFormat("[TB_CC_M4_4H_PROOF] %1 OPFOR-pressure-breakout outcome=%2 legalOPFOR=%3 mobOwner=%4", Result(cancelled), s_Campaign.GetOutcome(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), Owner(s_Campaign.GetSelectedMobId()));
			if (!cancelled) { Fail("opfor-pressure-not-cancelled"); return; }
			s_Phase = 21; Capture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), s_USSR); return;
		}
		bool restarted = s_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE && s_Campaign.GetPressureStartCount() == s_PressureStartBefore + 1;
		PrintFormat("[TB_CC_M4_4H_PROOF] %1 OPFOR-pressure-restart starts=%2 remaining=%3 mobOwner=%4", Result(restarted), s_Campaign.GetPressureStartCount(), s_Campaign.GetPressureRemainingSeconds(), Owner(s_Campaign.GetSelectedMobId()));
		if (!restarted) { Fail("opfor-pressure-not-restarted"); return; }
		s_Phase = 90;
	}

	protected static void CheckTerminal()
	{
		int expected = TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY; if (s_Mode == 10) expected = TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
		if (s_Campaign.GetOutcome() != expected) return;
		bool protectedHQs = Owner(s_Campaign.GetSelectedMobId()) == "US" && Owner(s_Campaign.GetSelectedEndpointId()) == "USSR";
		bool objectivesCleared = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty();
		bool frozen = !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Target) && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", s_Target);
		if (!CheckPresentationProjection()) { Fail("terminal-presentation-not-cleared"); return; }
		int revision = s_Campaign.GetRevision();
		SCR_CampaignMilitaryBaseComponent observedBase = s_Campaign.GetBase(s_Target);
		for (int duplicate; duplicate < 3; duplicate++) SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Invoke(observedBase, observedBase.GetFaction(), observedBase.GetFaction());
		bool idempotent = s_Campaign.GetRevision() == revision && s_Campaign.GetOutcome() == expected;
		PrintFormat("[TB_CC_M4_4I_PROOF] %1 terminal-reconstruction duplicateCallbacks=3 revisionBefore=%2 revisionAfter=%3 outcome=%4 ownershipMutation=0", Result(idempotent), revision, s_Campaign.GetRevision(), s_Campaign.GetOutcome());
		if (!idempotent) { Fail("terminal-reconstruction-not-idempotent"); return; }
		PrintFormat("[TB_CC_M4_4H_PROOF] %1 terminal outcome=%2 objectivesCleared=%3 capturesFrozen=%4 hqOwnersStable=%5 pressureStarts=%6", Result(protectedHQs && objectivesCleared && frozen), expected, objectivesCleared, frozen, protectedHQs, s_Campaign.GetPressureStartCount());
		if (protectedHQs && objectivesCleared && frozen) Complete(); else Fail("terminal-contract-mismatch");
	}

	protected static void Capture(string id, SCR_CampaignFaction faction)
	{
		if (id.IsEmpty()) { Fail("no-legal-objective"); return; }
		SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); if (!base) { Fail("base-missing-" + id); return; }
		s_ObservedSeizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (s_ObservedSeizing) s_ObservedSeizing.GetOnCaptureFinish().Insert(OnCaptureFinish);
		Resource resource = Resource.Load(faction.GetDefendersGroupPrefab()); EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; base.GetOwner().GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params)); if (!s_Group) { Fail("group-spawn-failed"); return; }
		vector origin = base.GetOwner().GetOrigin(); int row; int column;
		for (int wave; wave < 10; wave++) foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
		{
			vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1; params.Transform[3] = position;
			IEntity actor = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params); if (actor) { s_Group.AddAgentFromControlledEntity(actor); s_Actors.Insert(actor); }
			column++; if (column == 5) { column = 0; row++; }
		}
		s_Target = id; s_Attacker = faction.GetFactionKey(); s_RevisionBefore = s_Campaign.GetRevision(); s_Ticks = 0;
		PrintFormat("[TB_CC_M4_4H_PROOF] capture-begin phase=%1 attacker=%2 id=%3 owner=%4 authorized=%5 actors=%6", s_Phase, s_Attacker, id, Owner(id), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(s_Attacker, id), s_Actors.Count());
	}

	protected static void LogAnchors(string phase) { PrintFormat("[TB_CC_M4_4H_PROOF] anchors phase=%1 mob=%2 mobOwner=%3 endpoint=%4 endpointOwner=%5 mobSeizeAllowedUSSR=%6 endpointSeizeAllowedUS=%7", phase, s_Campaign.GetSelectedMobId(), Owner(s_Campaign.GetSelectedMobId()), s_Campaign.GetSelectedEndpointId(), Owner(s_Campaign.GetSelectedEndpointId()), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", s_Campaign.GetSelectedMobId()), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", s_Campaign.GetSelectedEndpointId())); }
	protected static bool CheckPresentationProjection()
	{
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!mode) return false;
		bool pressure = s_Campaign.IsVictoryPressureActive();
		bool visible = !TB_CC_VictoryPressurePresentation.Title(mode.TB_CC_GetPressureOutcome()).IsEmpty() && mode.TB_CC_GetPressureRemaining() >= 0;
		bool matched = mode.TB_CC_GetPressureOutcome() == s_Campaign.GetOutcome() && mode.TB_CC_GetFoothold() == s_Campaign.HasBLUFORFoothold() && visible == pressure;
		if (pressure) matched = matched && Math.AbsInt(mode.TB_CC_GetPressureRemaining() - s_Campaign.GetPressureRemainingSeconds()) <= 1;
		else matched = matched && mode.TB_CC_GetPressureRemaining() == -1;
		PrintFormat("[TB_CC_M4_4J_PROOF] %1 presentation-projection outcome=%2 visible=%3 title=%4 remaining=%5 countdown=%6 foothold=%7 visualHumanAcceptance=PENDING", Result(matched), mode.TB_CC_GetPressureOutcome(), visible, TB_CC_VictoryPressurePresentation.Title(mode.TB_CC_GetPressureOutcome()), mode.TB_CC_GetPressureRemaining(), TB_CC_VictoryPressurePresentation.Countdown(mode.TB_CC_GetPressureRemaining()), mode.TB_CC_GetFoothold());
		return matched;
	}
	protected static string Owner(string id) { string result = "<none>"; SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); if (base && base.GetFaction()) result = base.GetFaction().GetFactionKey(); return result; }
	protected static string Result(bool passed) { string result = "FAIL"; if (passed) result = "PASS"; return result; }
	protected static void Cleanup() { if (s_ObservedSeizing) s_ObservedSeizing.GetOnCaptureFinish().Remove(OnCaptureFinish); s_ObservedSeizing = null; while (!s_Actors.IsEmpty()) { IEntity actor = s_Actors.Get(s_Actors.Count() - 1); s_Actors.Remove(s_Actors.Count() - 1); if (actor && !actor.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(actor); } if (s_Group && !s_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(s_Group); s_Group = null; }
	protected static void OnCaptureFinish(SCR_Faction faction, SCR_SeizingComponent component) { PrintFormat("[TB_CC_M4_4I_PROOF] CaptureFinish id=%1 faction=%2 dedicated=1", s_Target, faction.GetFactionKey()); }
	protected static bool IsNewPosition(string id) { return id == "ARLAND::BEAUREGARD_HARBOUR" || id == "ARLAND::SIGNAL_HILL" || id == "ARLAND::NORTH_RUNWAY" || id == "ARLAND::GULL_ISLAND"; }
	protected static void LogDeployment(string phase)
	{
		array<string> ids = {}; for (int index; index < TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveCount(); index++) ids.Insert(TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveId(index));
		foreach (string id : ids)
		{
			SCR_CampaignMilitaryBaseComponent base = s_Campaign.GetBase(id); if (!base) continue;
			SCR_SpawnPoint spawn = base.GetSpawnPoint(); bool enabled; if (spawn) enabled = spawn.IsSpawnPointEnabled();
			PrintFormat("[TB_CC_M4_4I_PROOF] deployment phase=%1 id=%2 owner=%3 enabled=%4 selected=%5", phase, id, Owner(id), enabled, id == s_Campaign.GetSelectedMobId());
			if (phase == "initial" && IsNewPosition(id)) PrintFormat("[TB_CC_M4_4I_PROOF] locked-new-objective id=%1 allowedUS=%2 allowedUSSR=%3", id, TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", id), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", id));
			if (enabled != (id == s_Campaign.GetSelectedMobId())) Fail("deployment-policy-" + id);
		}
	}
	protected static void Complete() { Cleanup(); GetGame().GetCallqueue().Remove(Tick); PrintFormat("[TB_CC_M4_4H_PROOF] COMPLETE PASS mode=%1 captures=%2 directOwnership=0", s_Mode, s_Captures); }
	protected static void Fail(string reason) { Cleanup(); GetGame().GetCallqueue().Remove(Tick); PrintFormat("[TB_CC_M4_4H_PROOF] COMPLETE FAIL mode=%1 phase=%2 reason=%3 target=%4 outcome=%5", s_Mode, s_Phase, reason, s_Target, s_Campaign.GetOutcome()); }
}
