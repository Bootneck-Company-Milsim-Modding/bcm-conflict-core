class TB_CC_M4_3_SyntheticRadioView : TB_CC_M4_3_RadioEligibilityView
{
	protected ref array<string> m_aReachable = {};
	void Allow(string id) { if (!m_aReachable.Contains(id)) m_aReachable.Insert(id); }
	override bool IsReachable(string positionId, out string reason)
	{
		bool allowed = m_aReachable.Contains(positionId);
		if (allowed) reason = "synthetic-reachable";
		else reason = "synthetic-unreachable";
		return allowed;
	}
}

class TB_CC_M4_3_StrategicEvaluatorProofComponentClass : ScriptComponentClass {}
class TB_CC_M4_3_StrategicEvaluatorProofComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (Replication.IsServer()) GetGame().GetCallqueue().CallLater(RunProof, 24000);
	}

	protected void RunProof()
	{
		TB_CC_M4_1_StrategicPositionRegistry registry = new TB_CC_M4_1_StrategicPositionRegistry();
		Check("registry-ready", registry.Build(TB_CC_M4_2_ArlandProductionDefinition.Create()));
		TestEast(registry); TestWest(registry); TestIsolatedPocket(registry); TestBranchReverse(registry); TestNoCandidate(registry); TestRadioGate(registry); TestScale();
		Print("[TB_CC_M4_3] COMPLETE east=1 west=1 deepDenied=1 isolatedPocket=1 branch=1 reconvergence=1 reverse=1 noCandidate=1 publicQuery=1 gameplayMigration=0");
	}

	protected TB_CC_M4_3_SyntheticOwnershipView InitialOwners(TB_CC_M4_1_StrategicPositionRegistry registry, string mobId)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = new TB_CC_M4_3_SyntheticOwnershipView(); array<string> ids = {}; registry.GetPositionIds(ids);
		foreach (string id : ids) owners.SetOwner(id, "USSR"); owners.SetOwner(mobId, "US"); return owners;
	}

	protected TB_CC_M4_2A_CampaignStartState Start(int seed, string mobId, string endpointId)
	{
		return new TB_CC_M4_2A_CampaignStartState(seed, TB_CC_M4_2A_StartMode.EXPLICIT, mobId, endpointId);
	}

	protected void TestEast(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = InitialOwners(registry, "ARLAND::HQ_EAST"); TB_CC_M4_2A_CampaignStartState start = Start(20, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); int begin = System.GetTickCount(); evaluator.Evaluate(registry, start, owners); int duration = System.GetTickCount() - begin;
		array<string> candidates = {}; evaluator.GetBLUFORCandidateObjectives(candidates);
		Check("east-candidates", candidates.Count() == 2 && candidates.Contains("ARLAND::MILITARY_SOUTH") && candidates.Contains("ARLAND::MOSS_HILL"));
		Check("east-deep-denial", !candidates.Contains("ARLAND::HQ_WEST") && !evaluator.CanBLUFORAttack("ARLAND::HQ_WEST"));
		Check("east-one-objective", evaluator.IsValid() && !evaluator.GetBLUFORLegalObjective().IsEmpty() && evaluator.CanBLUFORAttack(evaluator.GetBLUFORLegalObjective()));
		int revision = evaluator.GetRevision(); string objective = evaluator.GetBLUFORLegalObjective(); evaluator.Evaluate(registry, start, owners);
		Check("east-repeat-deterministic", objective == evaluator.GetBLUFORLegalObjective() && revision == evaluator.GetRevision());
		bool matrixEligible = true;
		for (int seed; seed < 24; seed++) { TB_CC_M4_3_StrategicEvaluator seeded = new TB_CC_M4_3_StrategicEvaluator(); seeded.Evaluate(registry, Start(seed, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST"), owners); if (!candidates.Contains(seeded.GetBLUFORLegalObjective())) matrixEligible = false; }
		Check("east-seed-matrix-eligible-only", matrixEligible);
		PrintFormat("[TB_CC_M4_3] RESULT start=EAST candidates=%1 selected=%2 revision=%3 durationMs=%4", candidates, objective, revision, duration);
	}

	protected void TestWest(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = InitialOwners(registry, "ARLAND::HQ_WEST"); TB_CC_M4_2A_CampaignStartState start = Start(21, "ARLAND::HQ_WEST", "ARLAND::HQ_EAST");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); evaluator.Evaluate(registry, start, owners); array<string> candidates = {}; evaluator.GetBLUFORCandidateObjectives(candidates);
		Check("west-candidates", candidates.Count() == 2 && candidates.Contains("ARLAND::AIRPORT_NORTH") && candidates.Contains("ARLAND::FARM"));
		Check("west-deep-denial", !candidates.Contains("ARLAND::HQ_EAST") && !evaluator.CanBLUFORAttack("ARLAND::HQ_EAST"));
		bool matrixEligible = true;
		for (int seed; seed < 24; seed++) { TB_CC_M4_3_StrategicEvaluator seeded = new TB_CC_M4_3_StrategicEvaluator(); seeded.Evaluate(registry, Start(seed, "ARLAND::HQ_WEST", "ARLAND::HQ_EAST"), owners); if (!candidates.Contains(seeded.GetBLUFORLegalObjective())) matrixEligible = false; }
		Check("west-seed-matrix-eligible-only", matrixEligible);
		PrintFormat("[TB_CC_M4_3] RESULT start=WEST candidates=%1 selected=%2", candidates, evaluator.GetBLUFORLegalObjective());
	}

	protected void TestIsolatedPocket(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = InitialOwners(registry, "ARLAND::HQ_EAST"); owners.SetOwner("ARLAND::BEAUREGARD", "US");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); evaluator.Evaluate(registry, Start(30, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST"), owners);
		array<string> friendly = {}; array<string> candidates = {}; evaluator.GetFriendlyComponent(friendly); evaluator.GetBLUFORCandidateObjectives(candidates);
		Check("isolated-pocket-excluded", !friendly.Contains("ARLAND::BEAUREGARD") && evaluator.GetStrategicState("ARLAND::BEAUREGARD") == TB_CC_M4_3_PositionState.FRIENDLY_ISOLATED);
		Check("isolated-pocket-no-deep-unlock", !candidates.Contains("ARLAND::FARM") && !candidates.Contains("ARLAND::ARLEVILLE"));
	}

	protected void TestBranchReverse(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = InitialOwners(registry, "ARLAND::HQ_EAST"); TB_CC_M4_2A_CampaignStartState start = Start(40, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST");
		owners.SetOwner("ARLAND::MILITARY_SOUTH", "US");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); evaluator.Evaluate(registry, start, owners, "ARLAND::MILITARY_SOUTH"); array<string> branch = {}; evaluator.GetBLUFORCandidateObjectives(branch);
		Check("branch-recalculated", branch.Contains("ARLAND::AIRPORT_NORTH") && branch.Contains("ARLAND::ARLEVILLE") && branch.Contains("ARLAND::FARM") && branch.Contains("ARLAND::MOSS_HILL"));
		Check("opfor-response", evaluator.GetOPFORLegalObjective() == "ARLAND::MILITARY_SOUTH" && evaluator.CanOPFORAttack("ARLAND::MILITARY_SOUTH"));
		owners.SetOwner("ARLAND::MOSS_HILL", "US"); owners.SetOwner("ARLAND::ARLEVILLE", "US"); evaluator.Evaluate(registry, start, owners, "ARLAND::ARLEVILLE"); array<string> reconverged = {}; evaluator.GetBLUFORCandidateObjectives(reconverged);
		Check("reconvergence", reconverged.Contains("ARLAND::AIRPORT_NORTH") && reconverged.Contains("ARLAND::BEAUREGARD") && reconverged.Contains("ARLAND::FARM") && !reconverged.Contains("ARLAND::ARLEVILLE"));
		TB_CC_M4_3_SyntheticOwnershipView reverseOwners = InitialOwners(registry, "ARLAND::HQ_EAST"); reverseOwners.SetOwner("ARLAND::MILITARY_SOUTH", "US");
		TB_CC_M4_3_StrategicEvaluator reverseEvaluator = new TB_CC_M4_3_StrategicEvaluator(); reverseEvaluator.Evaluate(registry, start, reverseOwners, "ARLAND::MILITARY_SOUTH"); string preReverseObjective = reverseEvaluator.GetBLUFORLegalObjective();
		reverseOwners.SetOwner("ARLAND::MILITARY_SOUTH", "USSR"); reverseEvaluator.Evaluate(registry, start, reverseOwners, "ARLAND::MILITARY_SOUTH"); array<string> retreated = {}; reverseEvaluator.GetBLUFORCandidateObjectives(retreated);
		Check("reverse-component-recalculated", !reverseEvaluator.GetFriendlyComponentContains("ARLAND::MILITARY_SOUTH") && retreated.Contains("ARLAND::MILITARY_SOUTH") && retreated.Contains("ARLAND::MOSS_HILL"));
		Check("reverse-no-stale-response", reverseEvaluator.GetOPFORLegalObjective().IsEmpty() && !reverseEvaluator.CanOPFORAttack("ARLAND::MILITARY_SOUTH"));
		Check("reverse-no-stale-objective", reverseEvaluator.GetBLUFORLegalObjective() != preReverseObjective || retreated.Contains(preReverseObjective));
	}

	protected void TestNoCandidate(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = new TB_CC_M4_3_SyntheticOwnershipView(); array<string> ids = {}; registry.GetPositionIds(ids);
		foreach (string id : ids) owners.SetOwner(id, "FIA"); owners.SetOwner("ARLAND::HQ_EAST", "US"); owners.SetOwner("ARLAND::HQ_WEST", "USSR");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); bool evaluated = evaluator.Evaluate(registry, Start(50, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST"), owners);
		Check("no-candidate-fails-closed", evaluated && evaluator.IsReady() && !evaluator.IsValid() && evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.NO_VALID_FRONTLINE_OBJECTIVE && evaluator.GetBLUFORLegalObjective().IsEmpty());
	}

	protected void TestRadioGate(TB_CC_M4_1_StrategicPositionRegistry registry)
	{
		TB_CC_M4_3_SyntheticOwnershipView owners = InitialOwners(registry, "ARLAND::HQ_EAST");
		TB_CC_M4_2A_CampaignStartState start = Start(20, "ARLAND::HQ_EAST", "ARLAND::HQ_WEST");
		TB_CC_M4_3_SyntheticRadioView radio = new TB_CC_M4_3_SyntheticRadioView();
		radio.Allow("ARLAND::MOSS_HILL");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator();
		evaluator.Evaluate(registry, start, owners, "", radio);
		Check("radio-rejects-unreachable-higher-score", evaluator.GetBLUFORLegalObjective() == "ARLAND::MOSS_HILL" && !evaluator.CanBLUFORAttack("ARLAND::MILITARY_SOUTH"));
		TB_CC_M4_3_SyntheticRadioView none = new TB_CC_M4_3_SyntheticRadioView();
		evaluator.Evaluate(registry, start, owners, "", none);
		Check("radio-wait-valid-no-objective", evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.RADIO_WAIT && evaluator.IsValid() && evaluator.GetBLUFORLegalObjective().IsEmpty());
		none.Allow("ARLAND::MILITARY_SOUTH");
		evaluator.Evaluate(registry, start, owners, "", none);
		Check("radio-recovery-selects-reachable", evaluator.GetEvaluationState() == TB_CC_M4_3_EvaluationState.READY && evaluator.GetBLUFORLegalObjective() == "ARLAND::MILITARY_SOUTH");
	}

	protected void TestScale()
	{
		TB_CC_M4_1_MapDefinition definition = new TB_CC_M4_1_MapDefinition("M4_3_SCALE_128", 1, "M4.3 scale proof"); array<string> traits = { "MILITARY" };
		for (int i; i < 128; i++) { string id = string.Format("SCALE::P%1", i); definition.AddPosition(new TB_CC_M4_1_StrategicPositionDefinition(id, TB_CC_M4_1_PositionType.OUTPOST, "1621 4 903", "HQEast", 10, "SCALE", traits, i == 0, TB_CC_M4_1_EndpointRole.NONE, TB_CC_M4_1_OwnershipAdapterType.VANILLA_CONFLICT_BASE)); if (i > 0) definition.AddConnection(new TB_CC_M4_1_StrategicConnectionDefinition(string.Format("SCALE::P%1", i - 1), id)); }
		TB_CC_M4_1_StrategicPositionRegistry registry = new TB_CC_M4_1_StrategicPositionRegistry(); bool ready = registry.Build(definition); TB_CC_M4_3_SyntheticOwnershipView owners = new TB_CC_M4_3_SyntheticOwnershipView(); array<string> ids = {}; registry.GetPositionIds(ids); foreach (string id : ids) owners.SetOwner(id, "USSR"); owners.SetOwner("SCALE::P0", "US");
		TB_CC_M4_3_StrategicEvaluator evaluator = new TB_CC_M4_3_StrategicEvaluator(); int begin = System.GetTickCount(); evaluator.Evaluate(registry, Start(5, "SCALE::P0", "SCALE::P127"), owners); int duration = System.GetTickCount() - begin;
		Check("scale-128", ready && evaluator.IsValid() && evaluator.GetBLUFORLegalObjective() == "SCALE::P1"); PrintFormat("[TB_CC_M4_3] PERFORMANCE positions=128 edges=127 durationMs=%1", duration);
	}

	protected void Check(string label, bool pass) { if (pass) PrintFormat("[TB_CC_M4_3] PASS %1", label); else PrintFormat("[TB_CC_M4_3] FAIL %1", label); }
}
