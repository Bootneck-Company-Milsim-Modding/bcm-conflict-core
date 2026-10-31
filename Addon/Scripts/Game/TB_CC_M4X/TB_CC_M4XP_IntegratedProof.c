// Proof-only validation gate: production candidate validation still runs for
// every bearing accepted by this dedicated fixture.
class TB_CC_M4XP_CorrectiveStagingPolicy : TB_PVE_M3_3_TacticalStagingPolicy
{
	protected bool m_bDenyAll;
	protected bool m_bRestrictSector;
	protected float m_fSectorBearing;
	void DenyAllForProof(bool deny) { m_bDenyAll = deny; }
	void RestrictToSectorForProof(bool restrict, float bearing) { m_bRestrictSector = restrict; m_fSectorBearing = bearing; }
	float BearingForProof(vector position, vector target) { return Bearing(position, target); }
	override protected bool ValidateCandidate(vector targetPosition, vector candidate, array<vector> observers, out string rejection, bool enforceAnnulus = true)
	{
		if (m_bDenyAll) { rejection = "proof-deny-all"; return false; }
		if (m_bRestrictSector && BearingSeparation(Bearing(candidate, targetPosition), m_fSectorBearing) > 22.5) { rejection = "proof-single-sector"; return false; }
		return super.ValidateCandidate(targetPosition, candidate, observers, rejection, enforceAnnulus);
	}
}

class TB_CC_M4XP_IntegratedProofComponentClass : ScriptComponentClass {}

// Dedicated-only child-world fixture. Every ownership transition below is a
// physical vanilla seizing completion; this component never assigns faction.
class TB_CC_M4XP_IntegratedProofComponent : ScriptComponent
{
	[Attribute(defvalue: "1", desc: "1–6 historical integrated proofs; 7 focused corrective Military South capture and radio projection.")]
	protected int m_iMode;
	protected TB_CC_M4XP_NativeArlandComponent m_Campaign;
	protected SCR_CampaignFaction m_US;
	protected SCR_CampaignFaction m_USSR;
	protected SCR_AIGroup m_Group;
	protected ref array<IEntity> m_Actors = {};
	protected SCR_CampaignSeizingComponent m_Seizing;
	protected string m_Target;
	protected string m_Attacker;
	protected string m_UnselectedHQ;
	protected int m_iStage;
	protected int m_iTicks;
	protected int m_iRevisionBefore;
	protected int m_iCaptures;
	protected int m_iMaximumActors;
	protected int m_iCaptureFinishes;
	protected int m_iPressureStarts;
	protected bool m_bInterrupted;
	protected bool m_bDone;
	protected ref TB_CC_M4XP_CorrectiveStagingPolicy m_CorrectiveStaging;
	protected ref TB_CC_M4_3_SyntheticRadioView m_CorrectiveRadio;
	protected string m_sCorrectiveRecoveryCandidate;
	protected int m_iCorrectiveWaitRevision;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer() && RplSession.Mode() == RplMode.Dedicated)
			GetGame().GetCallqueue().CallLater(Start, 22000);
	}

	protected void Start()
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName("TB_CC_NATIVE_ARLAND_CAMPAIGN");
		if (entity) m_Campaign = TB_CC_M4XP_NativeArlandComponent.Cast(entity.FindComponent(TB_CC_M4XP_NativeArlandComponent));
		m_US = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("US"));
		m_USSR = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey("USSR"));
		if (!m_Campaign || !m_Campaign.IsReady() || !m_US || !m_USSR) { Finish("FAIL", "campaign-or-faction-unavailable"); return; }
		if (m_Campaign.GetRequiredOrdinaryPositionCount() != 11 || m_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_Campaign.GetDepthPermille() != 0) { Finish("FAIL", "fresh-native-state"); return; }
		m_UnselectedHQ = "ARLAND::HQ_EAST";
		if (m_Campaign.GetSelectedMobId() == m_UnselectedHQ) m_UnselectedHQ = "ARLAND::HQ_WEST";
		if (!CheckDeployment() || !CheckCustomResources()) { Finish("FAIL", "resource-or-deployment-contract"); return; }
		PrintFormat("[TB_CC_M4XP_PROOF] START mode=%1 mob=%2 unselectedHQ=%3 revision=%4 depth=%5 foothold=%6 ordinary=11 directGameplayOwnershipMutations=0", m_iMode, m_Campaign.GetSelectedMobId(), m_UnselectedHQ, m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), m_Campaign.HasBLUFORFoothold());
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
		if (m_iMode == 1)
		{
			string legal = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective();
			array<string> candidates = {}; TB_PVE_M2_3_FrontierQuery.GetBLUFORCandidateObjectives(candidates);
			string alternate;
			foreach (string candidate : candidates) if (candidate != legal && alternate.IsEmpty()) alternate = candidate;
			if (legal.IsEmpty() || alternate.IsEmpty() || !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", legal) || TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", alternate)) { Finish("FAIL", "opening-authorization"); return; }
			m_iStage = 10; BeginCapture(alternate, m_US);
			return;
		}
		if (m_iMode == 6)
		{
			bool fresh = !m_Campaign.HasBLUFORFoothold() && !m_Campaign.IsVictoryPressureActive() && m_Campaign.GetPressureRemainingSeconds() == -1;
			PrintFormat("[TB_CC_M4XP_PROOF] fresh-zero pass=%1 outcome=%2 foothold=%3 deadline=%4", fresh, m_Campaign.GetOutcome(), m_Campaign.HasBLUFORFoothold(), m_Campaign.GetPressureRemainingSeconds());
			if (!fresh) { Finish("FAIL", "fresh-zero-pressure"); return; }
		}
		if (m_iMode == 7)
		{
			if (TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() != "ARLAND::MILITARY_SOUTH" || !CheckCorrectiveSelector()) { Finish("FAIL", "corrective-initial-or-selector"); return; }
			m_CorrectiveStaging = new TB_CC_M4XP_CorrectiveStagingPolicy();
			m_CorrectiveStaging.Configure(350, 500, 250, 900, 20, 24, true);
			vector stagingTarget; if (TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition("ARLAND::MILITARY_SOUTH", stagingTarget)) m_CorrectiveStaging.RequestNavigationForTarget(stagingTarget);
		}
		if (m_iMode < 1 || m_iMode > 7) { Finish("FAIL", "unknown-mode"); return; }
		m_iStage = 1; BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US);
	}

	protected bool CheckCorrectiveSelector()
	{
		TB_PVE_M3_2_OPFOROperationCoordinator coordinator = m_Campaign.GetCoordinator();
		if (!coordinator || !coordinator.GetSelectionPolicy()) return false;
		TB_PVE_M3_5_ForceSelectionPolicy selector = coordinator.GetSelectionPolicy();
		array<int> humans = {1, 2, 5, 8, 12, 16};
		array<int> expected = {6, 12, 18, 24, 24, 24}; // New offensive development defaults.
		for (int index; index < humans.Count(); index++)
		{
			TB_PVE_M3_5_SelectionDecision decision = selector.SelectFromContextForProof(humans[index], 90, -1);
			TB_PVE_M3_4_ForcePackage package = TB_PVE_M3_4_ForcePackageRegistry.GetPackage(decision.m_sPackageId);
			int groups; if (package) groups = package.GetExpectedGroupCount();
			bool pass = decision.m_bSelected && groups * 3 == expected[index];
			PrintFormat("[TB_CC_M4XP_CORRECTIVE] selector humans=%1 depth=90 package=%2 groups=%3 expectedUnits=%4 pass=%5", humans[index], decision.m_sPackageId, groups, groups * 3, pass);
			if (!pass) return false;
		}
		return true;
	}

	protected bool CheckDeployment()
	{
		array<string> ids = {}; for (int i; i < TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveCount(); i++) ids.Insert(TB_PVE_M2_3_FrontierQuery.GetRouteObjectiveId(i));
		if (ids.Count() != 12) return false;
		foreach (string id : ids)
		{
			SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(id);
			if (!base || !base.GetSpawnPoint()) return false;
			if (base.GetSpawnPoint().IsSpawnPointEnabled() != (id == m_Campaign.GetSelectedMobId())) return false;
		}
		return true;
	}

	protected bool CheckCustomResources()
	{
		array<string> ids = {"ARLAND::BEAUREGARD_HARBOUR", "ARLAND::SIGNAL_HILL", "ARLAND::NORTH_RUNWAY", "ARLAND::GULL_ISLAND"};
		foreach (string id : ids)
		{
			SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(id);
			if (!base || !base.GetFaction() || base.GetFaction().GetFactionKey() != "USSR" || !base.GetSpawnPoint() || base.GetSpawnPoint().IsSpawnPointEnabled()) return false;
			if (!base.GetOwner().FindComponent(SCR_CampaignSeizingComponent)) return false;
			PrintFormat("[TB_CC_M4XP_PROOF] custom-resource id=%1 entity=%2 owner=USSR seizing=1 deployable=0", id, base.GetOwner().GetName());
		}
		return true;
	}

	protected void BeginCapture(string id, SCR_CampaignFaction faction)
	{
		if (id.IsEmpty() || !faction || (m_iStage != 10 && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction.GetFactionKey(), id))) { Finish("FAIL", "target-not-authorized-" + id); return; }
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(id);
		if (!base) { Finish("FAIL", "target-not-registered-" + id); return; }
		m_Target = id; m_Attacker = faction.GetFactionKey(); m_iRevisionBefore = m_Campaign.GetRevision(); m_iTicks = 0;
		m_Seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (!m_Seizing) { Finish("FAIL", "seizing-missing-" + id); return; }
		m_Seizing.GetOnCaptureFinish().Insert(OnCaptureFinish);
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD; base.GetOwner().GetTransform(params.Transform);
		m_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(faction.GetDefendersGroupPrefab()), GetGame().GetWorld(), params));
		if (!m_Group) { Finish("FAIL", "group-spawn-" + id); return; }
		vector origin = base.GetOwner().GetOrigin(); int row; int column;
		for (int wave; wave < 10; wave++) foreach (ResourceName prefab : m_Group.m_aUnitPrefabSlots)
		{
			vector position = origin + Vector(column * 2, 0, row * 2); position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1; params.Transform[3] = position;
			IEntity actor = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), params);
			if (actor) { m_Group.AddAgentFromControlledEntity(actor); m_Actors.Insert(actor); }
			column++; if (column == 5) { column = 0; row++; }
		}
		if (m_Actors.IsEmpty()) { Finish("FAIL", "physical-actors-not-created-" + id); return; }
		m_iMaximumActors = Math.Max(m_iMaximumActors, m_Actors.Count());
		PrintFormat("[TB_CC_M4XP_PROOF] capture-begin mode=%1 stage=%2 id=%3 attacker=%4 actors=%5 owner=%6 revision=%7", m_iMode, m_iStage, id, m_Attacker, m_Actors.Count(), base.GetFaction().GetFactionKey(), m_iRevisionBefore);
	}

	protected void Tick()
	{
		if (m_bDone) return;
		m_iTicks++;
		if (m_iMode == 7 && m_iStage == 70)
		{
			if (TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() == m_sCorrectiveRecoveryCandidate) { m_iStage = 71; return; }
			bool withheld = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && m_Campaign.CorrectiveProofTaskMatchesLegal() && m_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE;
			if (!withheld) { Finish("FAIL", "radio-wait-published-impossible-objective"); return; }
			if (m_iTicks == 2) { m_CorrectiveRadio.Allow(m_sCorrectiveRecoveryCandidate); PrintFormat("[TB_CC_M4XP_CORRECTIVE] radio-proof-reachable id=%1 waitingForScheduledRetry=1", m_sCorrectiveRecoveryCandidate); }
			if (m_iTicks < 5) return;
			if (m_iTicks > 9) { Finish("FAIL", "radio-retry-timeout"); return; }
			return;
		}
		if (m_iMode == 7 && m_iStage == 71)
		{
			bool recovered = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective() == m_sCorrectiveRecoveryCandidate && m_Campaign.GetRevision() > m_iCorrectiveWaitRevision && m_Campaign.CorrectiveProofTaskMatchesLegal();
			PrintFormat("[TB_CC_M4XP_CORRECTIVE] radio-recovery pass=%1 selected=%2 revision=%3 taskMatches=%4", recovered, TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_Campaign.GetRevision(), m_Campaign.CorrectiveProofTaskMatchesLegal());
			if (!recovered) { Finish("FAIL", "radio-recovery"); return; }
			if (!m_Campaign.SetCorrectiveProofRadioView(null)) { Finish("FAIL", "radio-proof-restore"); return; }
			TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrison = m_Campaign.GetGarrisonManager();
			if (!garrison) { Finish("FAIL", "garrison-manager-unavailable"); return; }
			garrison.SetProofHumanCount(1);
			array<vector> noPhysicalObservers = {}; garrison.SetProofObservers(noPhysicalObservers);
			Print("[TB_CC_M4XP_CORRECTIVE] garrison-proof testOnlyHumanCount=1 physicalObservers=0 combatClaim=0");
			m_iStage = 80; m_iTicks = 0; return;
		}
		if (m_iMode == 7 && m_iStage == 80)
		{
			TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrison = m_Campaign.GetGarrisonManager();
			if (garrison && garrison.HasGarrison())
			{
				bool ready = garrison.GetGroupCount() == 2 && garrison.GetUnitCount() == 6 && garrison.AreGroupsCombatReadyForProof();
				PrintFormat("[TB_CC_M4XP_CORRECTIVE] garrison-invariants pass=%1 objective=%2 groups=%3 units=%4 combatEngagement=HUMAN-PENDING", ready, garrison.GetObjectiveId(), garrison.GetGroupCount(), garrison.GetUnitCount());
				garrison.SetProofHumanCount(-1);
				Finish(Result(ready), "corrective-focused-proof"); return;
			}
			if (m_iTicks > 30) Finish("FAIL", "garrison-invariant-timeout");
			return;
		}
		if (m_iStage == 90)
		{
			int expected = TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY;
			if (m_iMode == 6) expected = TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY;
			if (m_Campaign.GetOutcome() == expected)
			{
				bool frozen = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty() && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", m_Target) && Owner(m_Campaign.GetSelectedMobId()) == "US";
				PrintFormat("[TB_CC_M4XP_PROOF] terminal outcome=%1 frozen=%2 pressureStarts=%3 revision=%4", expected, frozen, m_Campaign.GetPressureStartCount(), m_Campaign.GetRevision());
				int revisionBefore = m_Campaign.GetRevision();
				SCR_CampaignMilitaryBaseComponent observed = m_Campaign.GetBase(m_Target);
				for (int duplicate; duplicate < 3; duplicate++) SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended().Invoke(observed, observed.GetFaction(), observed.GetFaction());
				bool idempotent = m_Campaign.GetRevision() == revisionBefore && m_Campaign.GetOutcome() == expected;
				PrintFormat("[TB_CC_M4XP_PROOF] terminal-idempotent pass=%1 duplicateCallbacks=3 revision=%2 endBridgeCount=CHECK_CAMPAIGN_LOG", idempotent, revisionBefore);
				Finish(Result(frozen && idempotent), "terminal"); return;
			}
			if (m_iTicks > 180) Finish("FAIL", "pressure-expiry-timeout");
			return;
		}
		if (m_iStage == 30)
		{
			TB_PVE_M3_6_OPFORFrontlineGarrisonManager garrison = m_Campaign.GetGarrisonManager();
			if (garrison && garrison.HasGarrison()) { PrintFormat("[TB_CC_M4XP_PROOF] M3.6 garrison objective=%1 groups=%2 units=%3", garrison.GetObjectiveId(), garrison.GetGroupCount(), garrison.GetUnitCount()); Finish("BLOCKED", "M3.2-human-presence-needed"); return; }
			if (m_iTicks > 30) { Print("[TB_CC_M4XP_PROOF] M3.6 observer-gated with humansUS=0; no fake participant injected"); Finish("BLOCKED", "M3-human-presence-needed"); }
			return;
		}
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(m_Target);
		if (!base) { Finish("FAIL", "active-base-missing"); return; }
		if (m_iStage == 10)
		{
			if (m_iTicks < 35) return;
			bool locked = Owner(m_Target) == "USSR" && base.GetCaptureState() != SCR_EBaseCaptureState.CAPTURING && m_Campaign.GetRevision() == m_iRevisionBefore;
			PrintFormat("[TB_CC_M4XP_PROOF] alternate-lock pass=%1 id=%2 state=%3 revision=%4", locked, m_Target, base.GetCaptureState(), m_Campaign.GetRevision());
			CleanupActors(); if (!locked) { Finish("FAIL", "alternate-progressed"); return; }
			m_iStage = 1; BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return;
		}
		if (m_iStage == 11)
		{
			if (base.GetCaptureState() == SCR_EBaseCaptureState.NONE)
			{
				bool stable = Owner(m_Target) == "USSR" && m_Campaign.GetRevision() == m_iRevisionBefore;
				PrintFormat("[TB_CC_M4XP_PROOF] interruption pass=%1 id=%2 revisionStable=%3", stable, m_Target, stable);
				if (!stable) { Finish("FAIL", "interruption-state"); return; }
				m_iStage = 12; BeginCapture(m_Target, m_US); return;
			}
			if (m_iTicks > 30) Finish("FAIL", "interruption-timeout");
			return;
		}
		if (m_iMode == 1 && m_iStage == 1 && !m_bInterrupted && base.GetCaptureState() == SCR_EBaseCaptureState.CAPTURING && m_Seizing && m_Seizing.GetSeizingStartTimestamp() != 0)
		{
			m_bInterrupted = true;
			SCR_Faction prevailing = m_Seizing.TB_PVE_M1_4A_GetPrevailingFaction();
			if (!prevailing || prevailing.GetFactionKey() != "US") { Finish("FAIL", "vanilla-prevailing-faction"); return; }
			PrintFormat("[TB_CC_M4XP_PROOF] vanilla-timer id=%1 start=%2 end=%3 prevailing=%4", m_Target, m_Seizing.GetSeizingStartTimestamp(), m_Seizing.GetSeizingEndTimestamp(), prevailing.GetFactionKey());
			CleanupActors(); m_iStage = 11; m_iTicks = 0; return;
		}
		if (m_iTicks % 5 == 0)
			PrintFormat("[TB_CC_M4XP_PROOF] physical mode=%1 stage=%2 id=%3 owner=%4 state=%5 finishCount=%6 revision=%7", m_iMode, m_iStage, m_Target, Owner(m_Target), base.GetCaptureState(), m_iCaptureFinishes, m_Campaign.GetRevision());
		if (Owner(m_Target) != m_Attacker) { if (m_iTicks > 180) Finish("FAIL", "physical-capture-timeout-" + m_Target); return; }
		if (m_Campaign.GetRevision() <= m_iRevisionBefore || m_iCaptureFinishes < m_iCaptures + 1) { Finish("FAIL", "ownership-without-callback-or-capturefinish"); return; }
		if (m_iMode == 1 && m_Campaign.GetRevision() != m_iRevisionBefore + 1) { Finish("FAIL", "ordinary-revision-not-plus-one"); return; }
		m_iCaptures++; CleanupActors();
		PrintFormat("[TB_CC_M4XP_PROOF] vanilla-capture id=%1 owner=%2 revision=%3 depth=%4 foothold=%5 physicalCaptures=%6", m_Target, m_Attacker, m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), m_Campaign.HasBLUFORFoothold(), m_iCaptures);
		if (!CheckDeployment()) { Finish("FAIL", "forward-deployment-enabled"); return; }
		AfterCapture();
	}

	protected bool RunCorrectiveStagingProof()
	{
		vector target; if (!TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition("ARLAND::MILITARY_SOUTH", target)) return false;
		array<vector> observers = {};
		vector anchor; string failure;
		if (!m_CorrectiveStaging.FindStagingPosition("USSR", "ARLAND::MILITARY_SOUTH", target, observers, anchor, failure)) { PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-anchor-failed reason=%1", failure); return false; }
		float anchorBearing = m_CorrectiveStaging.GetLastAcceptedBearing();
		array<vector> used = {anchor};
		vector second;
		if (!m_CorrectiveStaging.FindAdditionalGroupPosition("USSR", "ARLAND::MILITARY_SOUTH", target, used, 1, observers, second, failure)) { PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-second-failed reason=%1", failure); return false; }
		string validity; bool secondValid = m_CorrectiveStaging.ValidateCandidateForProof(target, second, observers, validity);
		float secondBearing = m_CorrectiveStaging.BearingForProof(second, target);
		float separation = m_CorrectiveStaging.BearingSeparationForProof(anchorBearing, secondBearing);
		PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-multi-axis requested=3 distinctAtLeast2=%1 anchor=%2 anchorBearing=%3 second=%4 secondBearing=%5 separation=%6 terrainNavPlayerVisibilityValid=%7", separation >= 45 && secondValid, anchor, anchorBearing, second, secondBearing, separation, secondValid);
		if (!secondValid || separation < 45) return false;
		used.Insert(second);
		vector third;
		bool thirdValid = m_CorrectiveStaging.FindAdditionalGroupPosition("USSR", "ARLAND::MILITARY_SOUTH", target, used, 2, observers, third, failure);
		if (thirdValid) { used.Insert(third); PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-third position=%1 bearing=%2 distinctAxes=%3", third, m_CorrectiveStaging.BearingForProof(third, target), used.Count()); }
		else PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-third-unavailable reason=%1 safeNoSpawn=1", failure);
		m_CorrectiveStaging.RestrictToSectorForProof(true, anchorBearing);
		array<vector> singleSectorUsed = {anchor};
		vector degraded; bool oneAxis = m_CorrectiveStaging.FindAdditionalGroupPosition("USSR", "ARLAND::MILITARY_SOUTH", target, singleSectorUsed, 1, observers, degraded, failure);
		m_CorrectiveStaging.RestrictToSectorForProof(false, 0);
		PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-single-sector safeFallback=%1 position=%2 reason=%3", oneAxis, degraded, failure);
		if (!oneAxis) return false;
		m_CorrectiveStaging.DenyAllForProof(true);
		vector impossible; bool unexpectedlyFound = m_CorrectiveStaging.FindStagingPosition("USSR", "ARLAND::MILITARY_SOUTH", target, observers, impossible, failure);
		m_CorrectiveStaging.DenyAllForProof(false);
		bool safeFailure = !unexpectedlyFound && failure == "NO_VALID_STAGING_POSITION";
		PrintFormat("[TB_CC_M4XP_CORRECTIVE] staging-no-valid pass=%1 reason=%2 noSpawn=1 boundedAttempts=24", safeFailure, failure);
		return safeFailure;
	}

	protected void AfterCapture()
	{
		if (m_iMode == 7)
		{
			array<string> candidates = {};
			TB_PVE_M2_3_FrontierQuery.GetBLUFORCandidateObjectives(candidates);
			string selected = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective();
			bool initialSelectionValid = (!selected.IsEmpty() && candidates.Contains(selected)) || (selected.IsEmpty() && m_Campaign.GetEvaluator().GetEvaluationState() == TB_CC_M4_3_EvaluationState.RADIO_WAIT);
			bool pass = m_Target == "ARLAND::MILITARY_SOUTH" && m_iCaptures == 1 && m_Campaign.GetDepthPermille() == 1000 / 11 && initialSelectionValid && !candidates.IsEmpty() && CheckDeployment() && m_Campaign.CorrectiveProofTaskMatchesLegal();
			PrintFormat("[TB_CC_M4XP_CORRECTIVE] post-capture candidates=%1 selected=%2 airportNorthGraphLegal=%3 airportNorthSelected=%4 revision=%5 depth=%6 deploymentValid=%7 taskMatches=%8", candidates, selected, candidates.Contains("ARLAND::AIRPORT_NORTH"), selected == "ARLAND::AIRPORT_NORTH", m_Campaign.GetRevision(), m_Campaign.GetDepthPermille(), CheckDeployment(), m_Campaign.CorrectiveProofTaskMatchesLegal());
			if (!pass || !RunCorrectiveStagingProof()) { Finish("FAIL", "corrective-capture-or-staging"); return; }
			m_sCorrectiveRecoveryCandidate = selected; if (m_sCorrectiveRecoveryCandidate.IsEmpty()) m_sCorrectiveRecoveryCandidate = candidates[0];
			m_CorrectiveRadio = new TB_CC_M4_3_SyntheticRadioView();
			if (!m_Campaign.SetCorrectiveProofRadioView(m_CorrectiveRadio)) { Finish("FAIL", "radio-proof-hook-unavailable"); return; }
			m_iCorrectiveWaitRevision = m_Campaign.GetRevision();
			bool withheld = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && m_Campaign.CorrectiveProofTaskMatchesLegal();
			PrintFormat("[TB_CC_M4XP_CORRECTIVE] radio-wait pass=%1 graphCandidates=%2 revision=%3 noOwnershipMutation=1", withheld, candidates, m_iCorrectiveWaitRevision);
			if (!withheld) { Finish("FAIL", "radio-wait"); return; }
			m_iStage = 70; m_iTicks = 0; return;
		}
		if (m_iMode == 1)
		{
			bool pass = m_bInterrupted && m_iCaptures == 1 && m_Campaign.HasBLUFORFoothold() && m_Campaign.GetDepthPermille() == 1000 / 11 && !TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty();
			Finish(Result(pass), "ordinary-capture"); return;
		}
		if (m_iMode == 2)
		{
			if (m_Target == m_UnselectedHQ)
			{
				bool pass = Owner(m_Campaign.GetSelectedMobId()) == "US" && !m_Campaign.GetBase(m_UnselectedHQ).IsHQ() && m_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY;
				Finish(Result(pass), "unselected-HQ-ordinary-capture"); return;
			}
			BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return;
		}
		if (m_iMode == 3)
		{
			if (m_Target == "ARLAND::GULL_ISLAND") { CheckIslandTransport(); return; }
			BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return;
		}
		if (m_iMode == 4)
		{
			PrintFormat("[TB_CC_M4XP_PROOF] M3 responseTarget=%1 humansUS=%2 coordinator=%3 garrison=%4", TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), TB_PVE_M3_7_HumanPresenceQuery.GetParticipatingHumanCount("US"), m_Campaign.GetCoordinator() != null, m_Campaign.GetGarrisonManager() != null);
			m_iStage = 30; m_iTicks = 0; return;
		}
		if (m_iMode == 5) { AdvanceBLUFOR(); return; }
		if (m_iMode == 6) { AdvanceOPFOR(); return; }
	}

	protected void CheckIslandTransport()
	{
		TB_CC_M4_1_MapDefinition definition = TB_CC_M4XP_ArlandNativeDefinition.Create();
		array<ref TB_CC_M4_1_StrategicPositionDefinition> positions = {}; definition.GetPositions(positions);
		bool trait;
		foreach (TB_CC_M4_1_StrategicPositionDefinition position : positions)
			if (position.GetId() == "ARLAND::GULL_ISLAND") trait = position.HasTrait("TRANSPORT_REQUIRED");
		vector objective; vector staging; string reason; array<vector> observers = {};
		TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition("ARLAND::GULL_ISLAND", objective);
		bool staged = m_Campaign.GetStagingPolicy().FindStagingPosition("USSR", "ARLAND::GULL_ISLAND", objective, observers, staging, reason);
		PrintFormat("[TB_CC_M4XP_PROOF] island transportTrait=%1 footStaged=%2 reason=%3 attackersTeleported=0", trait, staged, reason);
		Finish(Result(trait && !staged && reason == "unsupported-transport-requirement"), "Gull-Island-capture-and-foot-safety");
	}

	protected void AdvanceBLUFOR()
	{
		if (m_iStage == 1)
		{
			int expected = (m_iCaptures * 1000) / 11;
			if (m_Campaign.GetDepthPermille() != expected) { Finish("FAIL", "depth-normalization"); return; }
			if (m_iCaptures == 1 || m_iCaptures == 6 || m_iCaptures == 10) PrintFormat("[TB_CC_M4XP_PROOF] depth-band captures=%1 depth=%2 revision=%3", m_iCaptures, expected, m_Campaign.GetRevision());
			if (m_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) { BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return; }
			if (m_iCaptures != 11) { Finish("FAIL", "early-BLUFOR-pressure"); return; }
			m_iPressureStarts = m_Campaign.GetPressureStartCount();
			if (m_iPressureStarts != 1 || m_Campaign.GetPressureRemainingSeconds() < 0) { Finish("FAIL", "BLUFOR-first-pressure-timer"); return; }
			m_iStage = 2;
			BeginCapture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), m_USSR); return;
		}
		if (m_iStage == 2)
		{
			if (m_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_Campaign.IsVictoryPressureActive() || m_Campaign.GetPressureRemainingSeconds() != -1) { Finish("FAIL", "BLUFOR-pressure-not-cancelled"); return; }
			m_iStage = 3; BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return;
		}
		bool restarted = m_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE && m_Campaign.GetPressureStartCount() == m_iPressureStarts + 1 && m_Campaign.GetPressureRemainingSeconds() >= 290;
		if (!restarted) { Finish("FAIL", "BLUFOR-pressure-not-restarted"); return; }
		m_iStage = 90; m_iTicks = 0;
	}

	protected void AdvanceOPFOR()
	{
		if (m_iStage == 1)
		{
			if (!m_Campaign.HasBLUFORFoothold() || m_Campaign.GetDepthPermille() != 1000 / 11) { Finish("FAIL", "foothold-not-established"); return; }
			m_iStage = 2; BeginCapture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), m_USSR); return;
		}
		if (m_iStage == 2)
		{
			bool pressured = m_Campaign.GetDepthPermille() == 0 && m_Campaign.HasBLUFORFoothold() && m_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE;
			if (!pressured) { Finish("FAIL", "OPFOR-pressure-not-entered"); return; }
			m_iPressureStarts = m_Campaign.GetPressureStartCount();
			if (m_iPressureStarts != 1 || m_Campaign.GetPressureRemainingSeconds() < 0) { Finish("FAIL", "OPFOR-first-pressure-timer"); return; }
			m_iStage = 3;
			BeginCapture(TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), m_US); return;
		}
		if (m_iStage == 3)
		{
			if (m_Campaign.GetOutcome() != TB_PVE_M2_5_CampaignOutcome.ACTIVE || m_Campaign.IsVictoryPressureActive() || m_Campaign.GetPressureRemainingSeconds() != -1) { Finish("FAIL", "OPFOR-pressure-not-cancelled"); return; }
			m_iStage = 4; BeginCapture(TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective(), m_USSR); return;
		}
		bool restarted = m_Campaign.GetOutcome() == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE && m_Campaign.GetPressureStartCount() == m_iPressureStarts + 1 && m_Campaign.GetPressureRemainingSeconds() >= 290;
		if (!restarted) { Finish("FAIL", "OPFOR-pressure-not-restarted"); return; }
		m_iStage = 90; m_iTicks = 0;
	}

	protected string Owner(string id)
	{
		SCR_CampaignMilitaryBaseComponent base = m_Campaign.GetBase(id);
		if (!base || !base.GetFaction()) return "<none>";
		return base.GetFaction().GetFactionKey();
	}
	protected string Result(bool passed)
	{
		if (passed) return "PASS";
		return "FAIL";
	}
	protected void OnCaptureFinish(SCR_Faction faction, SCR_SeizingComponent component)
	{
		m_iCaptureFinishes++;
		PrintFormat("[TB_CC_M4XP_PROOF] CaptureFinish id=%1 faction=%2 count=%3", m_Target, faction.GetFactionKey(), m_iCaptureFinishes);
	}
	protected void CleanupActors()
	{
		if (m_Seizing) m_Seizing.GetOnCaptureFinish().Remove(OnCaptureFinish);
		m_Seizing = null;
		while (!m_Actors.IsEmpty())
		{
			IEntity actor = m_Actors.Get(m_Actors.Count() - 1); m_Actors.Remove(m_Actors.Count() - 1);
			if (actor && !actor.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(actor);
		}
		if (m_Group && !m_Group.IsDeleted()) SCR_EntityHelper.DeleteEntityAndChildren(m_Group);
		m_Group = null;
	}
	protected void Finish(string result, string reason)
	{
		if (m_bDone) return;
		m_bDone = true; GetGame().GetCallqueue().Remove(Tick); GetGame().GetCallqueue().Remove(Start); CleanupActors();
		int revision = -1; int depth = -1; if (m_Campaign) { revision = m_Campaign.GetRevision(); depth = m_Campaign.GetDepthPermille(); }
		PrintFormat("[TB_CC_M4XP_PROOF] COMPLETE result=%1 mode=%2 reason=%3 physicalCaptures=%4 directGameplayOwnershipMutations=0 maximumProofActors=%5 revision=%6 depth=%7", result, m_iMode, reason, m_iCaptures, m_iMaximumActors, revision, depth);
	}
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(Tick); GetGame().GetCallqueue().Remove(Start); CleanupActors();
		super.OnDelete(owner);
	}
}
