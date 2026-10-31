// Dedicated authority proof. Ownership is changed only by physical vanilla AI
// capture. Each terminal outcome is tested in a fresh server session because a
// production terminal state is deliberately immutable.
class TB_PVE_M2_5_CampaignOutcomeProof
{
	protected static TB_PVE_M2_1_LinearFrontierComponent s_Route;
	protected static SCR_CampaignFaction s_US;
	protected static SCR_CampaignFaction s_USSR;
	protected static SCR_CampaignFaction s_FIA;
	protected static SCR_AIGroup s_Group;
	protected static ref array<IEntity> s_Characters = {};
	protected static int s_ProofMode;
	protected static int s_Phase;
	protected static int s_Target;
	protected static int s_Ticks;
	protected static int s_RevisionBefore;
	protected static string s_LastM2_5BDiagnostic;
	protected static SCR_CampaignSeizingComponent s_MobSeizing;
	protected static int s_CaptureStarts;
	protected static int s_CaptureInterrupts;
	protected static int s_CaptureFinishes;
	protected static bool s_InterruptRequested;

	static void Start(TB_PVE_M2_1_LinearFrontierComponent route, int proofMode)
	{
		if (!Replication.IsServer() || !route || !route.IsReady()) return;
		s_Route = route;
		s_ProofMode = proofMode;
		FactionManager fm = GetGame().GetFactionManager();
		s_US = SCR_CampaignFaction.Cast(fm.GetFactionByKey("US"));
		s_USSR = SCR_CampaignFaction.Cast(fm.GetFactionByKey("USSR"));
		s_FIA = SCR_CampaignFaction.Cast(fm.GetFactionByKey("FIA"));
		if (!s_US || !s_USSR || !s_FIA) { Fail("factions unavailable"); return; }

		bool endpoints = route.GetBluforEndpointIndex() == 0 && route.GetOpforEndpointIndex() == route.GetRouteCount() - 1;
		bool initial = route.GetCampaignOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE && TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE;
		bool prematureMobDenied = !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", route.GetRouteId(route.GetBluforEndpointIndex()));
		bool fiaDenied = !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("FIA", route.GetRouteId(1));
		PrintFormat("[TB_PVE_M2_5] %1 initial-contract endpoints=%2 initialActive=%3 prematureMobDenied=%4 fiaDenied=%5 revision=%6 BLUFOR=%7 OPFOR=%8", Result(endpoints && initial && prematureMobDenied && fiaDenied), endpoints, initial, prematureMobDenied, fiaDenied, TB_PVE_M2_3_FrontierQuery.GetRevision(), TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective(), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
		if (!endpoints || !initial || !prematureMobDenied || !fiaDenied) { Fail("initial contract"); return; }

		SCR_CampaignMilitaryBaseComponent mob = route.GetRouteBase(route.GetBluforEndpointIndex());
		bool initialPolicyNegative = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_USSR, mob);
		bool fiaPolicyNegative = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_FIA, mob);
		bool usPolicyNegative = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_US, mob);
		bool otherBaseNegative = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_USSR, route.GetRouteBase(1));
		bool mobFunction = LogMOBFunctionContract();
		PrintFormat("[TB_PVE_M2_5B] %1 initial-negative-policy mobNotLegal=%2 FIA=%3 US=%4 otherBase=%5 mobFunction=%6", Result(initialPolicyNegative && fiaPolicyNegative && usPolicyNegative && otherBaseNegative && mobFunction), initialPolicyNegative, fiaPolicyNegative, usPolicyNegative, otherBaseNegative, mobFunction);
		if (!initialPolicyNegative || !fiaPolicyNegative || !usPolicyNegative || !otherBaseNegative || !mobFunction) { Fail("initial production policy regression"); return; }

		int before = TB_PVE_M2_3_FrontierQuery.GetRevision();
		route.M2_2_SetDiagnosticInvalid(true);
		bool invalidPolicyNegative = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_USSR, mob);
		bool invalid = TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == TB_PVE_M2_5_CampaignOutcome.INVALID && !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", route.GetRouteId(1)) && invalidPolicyNegative;
		PrintFormat("[TB_PVE_M2_5] %1 invalid-fail-closed outcome=%2 attackAllowed=%3 revisionAdvanced=%4 terminalHQPolicyDenied=%5", Result(invalid), TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("US", route.GetRouteId(1)), TB_PVE_M2_3_FrontierQuery.GetRevision() > before, invalidPolicyNegative);
		route.M2_2_SetDiagnosticInvalid(false);
		if (!invalid) { Fail("invalid fail closed"); return; }

		PrintFormat("[TB_PVE_M2_5] START mode=%1 route=%2 dedicated=%3 server=%4", proofMode, route.GetRouteIdentity(), RplSession.Mode() == RplMode.Dedicated, Replication.IsServer());
		s_MobSeizing = SCR_CampaignSeizingComponent.Cast(mob.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (!s_MobSeizing) { Fail("MOB seizing component unavailable"); return; }
		s_MobSeizing.GetOnCaptureStart().Insert(OnCaptureStart);
		s_MobSeizing.GetOnCaptureInterrupt().Insert(OnCaptureInterrupt);
		s_MobSeizing.GetOnCaptureFinish().Insert(OnCaptureFinish);
		GetGame().GetCallqueue().CallLater(Tick, 2000, true);
		if (proofMode == 2)
		{
			s_Phase = -1;
			StartDeniedPresence(route.GetBluforEndpointIndex(), s_USSR, "premature-OPFOR-MOB-presence");
		}
		else
			StartCapture(1, s_US, "arm-forward");
	}

	protected static void Tick()
	{
		s_Ticks++;
		if (s_Phase == -1)
		{
			if (s_Ticks < 10) return;
			SCR_CampaignMilitaryBaseComponent mob = s_Route.GetRouteBase(s_Target);
			bool denied = Owner(s_Target) == "US" && mob.GetCaptureState() == 0 && TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore && s_CaptureStarts == 0;
			PrintFormat("[TB_PVE_M2_5B] %1 premature-physical-presence owner=%2 captureState=%3 revisionStable=%4 outcome=%5 exceptionStarts=%6", Result(denied), Owner(s_Target), mob.GetCaptureState(), TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore, s_Route.GetCampaignOutcome(), s_CaptureStarts);
			Cleanup();
			if (!denied) { Fail("premature physical MOB denial"); return; }
			s_Phase = 0;
			StartCapture(1, s_US, "arm-forward");
			return;
		}
		if (s_ProofMode == 2 && s_Phase >= 2)
			LogM2_5BCapturePath();
		if (s_Phase == 2)
		{
			if (!s_InterruptRequested && s_Route.GetRouteBase(s_Target).GetCaptureState() == SCR_EBaseCaptureState.CAPTURING && s_CaptureStarts > 0)
			{
				s_InterruptRequested = true;
				Cleanup();
				s_Ticks = 0;
				PrintFormat("[TB_PVE_M2_5B] interruption-requested owner=%1 starts=%2 revision=%3", Owner(s_Target), s_CaptureStarts, TB_PVE_M2_3_FrontierQuery.GetRevision());
				return;
			}
			if (s_InterruptRequested && s_Route.GetRouteBase(s_Target).GetCaptureState() == SCR_EBaseCaptureState.NONE && s_CaptureInterrupts > 0)
			{
				bool interrupted = Owner(s_Target) == "US" && TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore;
				PrintFormat("[TB_PVE_M2_5B] %1 vanilla-interruption owner=%2 interrupts=%3 revisionStable=%4", Result(interrupted), Owner(s_Target), s_CaptureInterrupts, TB_PVE_M2_3_FrontierQuery.GetRevision() == s_RevisionBefore);
				if (!interrupted) { Fail("terminal capture interruption regression"); return; }
				s_Phase = 3;
				StartCapture(s_Route.GetBluforEndpointIndex(), s_USSR, "OPFOR-MOB-terminal-success");
				return;
			}
			if (s_Ticks > 40) Fail("capture interruption timeout");
			return;
		}
		if (Owner(s_Target) == ExpectedOwner())
		{
			Cleanup();
			if (TB_PVE_M2_3_FrontierQuery.GetRevision() <= s_RevisionBefore) { Fail("revision did not advance"); return; }
			PrintFormat("[TB_PVE_M2_5] PASS physical-capture id=%1 owner=%2 frontier=%3 outcome=%4 revision=%5", s_Route.GetRouteId(s_Target), Owner(s_Target), s_Route.GetFrontierIndex(), s_Route.GetCampaignOutcome(), TB_PVE_M2_3_FrontierQuery.GetRevision());

			if (s_ProofMode == 1)
			{
				if (s_Target < s_Route.GetOpforEndpointIndex()) { StartCapture(s_Target + 1, s_US, "BLUFOR-forward"); return; }
				Finish(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY, "BLUFOR"); return;
			}

			if (s_Phase == 0)
			{
				bool mobStillDenied = !TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack("USSR", s_Route.GetRouteId(s_Route.GetBluforEndpointIndex()));
				PrintFormat("[TB_PVE_M2_5] %1 premature-MOB-after-advance denied=%2 legal=%3", Result(mobStillDenied), mobStillDenied, TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
				if (!mobStillDenied) { Fail("premature MOB unlocked"); return; }
				s_Phase = 1; StartCapture(1, s_USSR, "OPFOR-reverse-arm"); return;
			}
			if (s_Phase == 1)
			{
				bool mobLegal = TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective() == s_Route.GetRouteId(s_Route.GetBluforEndpointIndex());
				PrintFormat("[TB_PVE_M2_5] %1 MOB-unlocked-after-retreat legal=%2", Result(mobLegal), TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective());
				if (!mobLegal) { Fail("MOB did not unlock"); return; }
				bool productionException = TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_USSR, s_Route.GetRouteBase(s_Route.GetBluforEndpointIndex()));
				PrintFormat("[TB_PVE_M2_5B] %1 production-terminal-exception legal=%2", Result(productionException), productionException);
				if (!productionException) { Fail("production terminal exception not eligible"); return; }
				s_Phase = 2; StartCapture(s_Route.GetBluforEndpointIndex(), s_USSR, "OPFOR-MOB-terminal-interrupt"); return;
			}
			Finish(TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY, "OPFOR");
			return;
		}
		if (s_Ticks > 40) Fail("physical capture timeout");
	}

	protected static void Finish(int expected, string label)
	{
		int terminalRevision = TB_PVE_M2_3_FrontierQuery.GetRevision();
		bool outcome = s_Route.GetCampaignOutcome() == expected && TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == expected;
		bool frozen = TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective().IsEmpty() && TB_PVE_M2_3_FrontierQuery.GetOPFORLegalObjective().IsEmpty();
		s_Route.M2_2_ReconstructProjection();
		bool idempotent = TB_PVE_M2_3_FrontierQuery.GetRevision() == terminalRevision && TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome() == expected;
		bool deployment = CheckDeploymentContract();
		bool terminalPolicyDenied = !TB_PVE_M2_1_LinearFrontierComponent.CanFactionSeizeTerminalHQ(s_USSR, s_Route.GetRouteBase(s_Route.GetBluforEndpointIndex()));
		bool callbacks = s_ProofMode != 2 || (s_CaptureStarts >= 2 && s_CaptureInterrupts >= 1 && s_CaptureFinishes >= 1);
		PrintFormat("[TB_PVE_M2_5] %1 COMPLETE winner=%2 outcome=%3 oneShot=%4 frozen=%5 deployment=%6 vanillaPhysicalCapture=1 FIAWinner=0 terminalPolicyDenied=%7", Result(outcome && frozen && idempotent && deployment && terminalPolicyDenied && callbacks), label, expected, idempotent, frozen, deployment, terminalPolicyDenied);
		PrintFormat("[TB_PVE_M2_5] COMPLETE_CALLBACKS winner=%1 callbacks=%2/%3/%4", label, s_CaptureStarts, s_CaptureInterrupts, s_CaptureFinishes);
		UnsubscribeCaptureEvents();
		GetGame().GetCallqueue().Remove(Tick);
	}

	protected static bool LogMOBFunctionContract()
	{
		SCR_CampaignMilitaryBaseComponent mob = s_Route.GetRouteBase(s_Route.GetBluforEndpointIndex());
		SCR_SpawnPoint spawn = mob.GetSpawnPoint();
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(mob.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		array<SCR_CampaignBuildingProviderComponent> providers = {};
		array<SCR_ServicePointComponent> services = {};
		int providerCount = mob.GetBuildingProviders(providers);
		int serviceCount = mob.GetServices(services);
		bool pass = mob.IsInitialized() && mob.IsHQ() && spawn && spawn.IsSpawnPointEnabled() && mob.GetResourceComponent() && radio && providerCount > 0 && serviceCount > 0;
		int transceivers;
		if (radio) transceivers = radio.TransceiversCount();
		PrintFormat("[TB_PVE_M2_5B] MOB-function initialized=%1 isHQ=%2 spawnValid=%3 spawnEnabled=%4 resource=%5 radio=%6", mob.IsInitialized(), mob.IsHQ(), spawn != null, spawn && spawn.IsSpawnPointEnabled(), mob.GetResourceComponent() != null, radio != null);
		PrintFormat("[TB_PVE_M2_5B] MOB-support radioTransceivers=%1 providers=%2 services=%3 supplies=%4", transceivers, providerCount, serviceCount, mob.GetSupplies());
		return pass;
	}

	protected static bool CheckDeploymentContract()
	{
		int enabledUSMobs;
		for (int i; i < s_Route.GetRouteCount(); i++)
		{
			SCR_SpawnPoint spawn = s_Route.GetRouteBase(i).GetSpawnPoint();
			if (i == s_Route.GetBluforEndpointIndex())
			{
				if (spawn && spawn.IsSpawnPointEnabled()) enabledUSMobs++;
			}
			else if (spawn && spawn.IsSpawnPointEnabled())
				return false;
		}
		return enabledUSMobs <= 1;
	}

	protected static void StartCapture(int index, SCR_CampaignFaction faction, string label)
	{
		s_Target = index; s_Ticks = 0; s_RevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
		if (!TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction.GetFactionKey(), s_Route.GetRouteId(index))) { Fail("capture not authorized"); return; }
		SpawnActors(index, faction, label);
	}

	protected static void StartDeniedPresence(int index, SCR_CampaignFaction faction, string label)
	{
		s_Target = index; s_Ticks = 0; s_RevisionBefore = TB_PVE_M2_3_FrontierQuery.GetRevision();
		if (TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction.GetFactionKey(), s_Route.GetRouteId(index))) { Fail("premature capture unexpectedly authorized"); return; }
		SpawnActors(index, faction, label);
	}

	protected static void SpawnActors(int index, SCR_CampaignFaction faction, string label)
	{
		ResourceName groupPrefab = faction.GetDefendersGroupPrefab();
		EntitySpawnParams params = new EntitySpawnParams(); params.TransformMode = ETransformMode.WORLD;
		IEntity baseEntity = s_Route.GetRouteBase(index).GetOwner(); baseEntity.GetTransform(params.Transform);
		s_Group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(groupPrefab), GetGame().GetWorld(), params));
		if (!s_Group) { Fail("group spawn failed"); return; }
		vector origin = baseEntity.GetOrigin(); int column; int row;
		for (int wave; wave < 10; wave++)
		{
			foreach (ResourceName unitPrefab : s_Group.m_aUnitPrefabSlots)
			{
				vector position = origin + Vector(column * 2, 0, row * 2);
				position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]) + 1.0;
				params.Transform[3] = position;
				IEntity character = GetGame().SpawnEntityPrefab(Resource.Load(unitPrefab), GetGame().GetWorld(), params);
				if (character) { s_Group.AddAgentFromControlledEntity(character); s_Characters.Insert(character); }
				column++; if (column == 5) { column = 0; row++; }
			}
		}
		s_Group.SetLOD(0);
		PrintFormat("[TB_PVE_M2_5] capture-start label=%1 faction=%2 id=%3 revision=%4 characters=%5", label, faction.GetFactionKey(), s_Route.GetRouteId(index), s_RevisionBefore, s_Characters.Count());
	}

	protected static void LogM2_5BCapturePath()
	{
		SCR_CampaignMilitaryBaseComponent base = s_Route.GetRouteBase(s_Target);
		SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (!seizing) return;
		int alive;
		int inside;
		int belowMaxLOD;
		int vanillaEligible;
		vector origin = base.GetOwner().GetOrigin();
		float radius = seizing.GetRadius();
		foreach (IEntity entity : s_Characters)
		{
			if (!entity) continue;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
			CharacterControllerComponent controller;
			AIControlComponent aiControl;
			AIAgent agent;
			if (character) controller = character.GetCharacterController();
			if (controller && controller.GetLifeState() == ECharacterLifeState.ALIVE) alive++;
			if (controller) aiControl = controller.GetAIControlComponent();
			if (aiControl) agent = aiControl.GetAIAgent();
			if (vector.DistanceXZ(entity.GetOrigin(), origin) <= radius) inside++;
			if (agent && agent.GetLOD() < AIAgent.GetMaxLOD()) belowMaxLOD++;
			if (seizing.TB_PVE_M1_4A_EvaluateEntityFaction(entity)) vanillaEligible++;
		}
		Faction prevailing = seizing.TB_PVE_M1_4A_GetPrevailingFaction();
		FactionKey prevailingKey = "<none>";
		if (prevailing) prevailingKey = prevailing.GetFactionKey();
		string state = string.Format("alive=%1 inside=%2 belowMaxLOD=%3 vanillaEligible=%4 prevailing=%5 contesting=%6", alive, inside, belowMaxLOD, vanillaEligible, prevailingKey, seizing.GetContestingFactionsCount());
		state += string.Format(" captureState=%1 owner=%2 isHQ=%3 initialized=%4 radius=%5", base.GetCaptureState(), Owner(s_Target), base.IsHQ(), base.IsInitialized(), radius);
		if (state != s_LastM2_5BDiagnostic || s_Ticks == 1 || (s_Ticks % 10) == 0)
		{
			PrintFormat("[TB_PVE_M2_5B] capture-path tick=%1 %2", s_Ticks, state);
			s_LastM2_5BDiagnostic = state;
		}
	}

	protected static string ExpectedOwner()
	{
		if (s_ProofMode == 1 || s_Phase <= 0 || s_Phase == 2) return "US";
		return "USSR";
	}

	protected static void OnCaptureStart(SCR_Faction faction, SCR_SeizingComponent component)
	{
		s_CaptureStarts++;
		PrintFormat("[TB_PVE_M2_5B] vanilla-CaptureStart faction=%1 count=%2", faction.GetFactionKey(), s_CaptureStarts);
	}

	protected static void OnCaptureInterrupt(SCR_Faction faction, SCR_SeizingComponent component)
	{
		s_CaptureInterrupts++;
		PrintFormat("[TB_PVE_M2_5B] vanilla-CaptureInterrupt faction=%1 count=%2", faction.GetFactionKey(), s_CaptureInterrupts);
	}

	protected static void OnCaptureFinish(SCR_Faction faction, SCR_SeizingComponent component)
	{
		s_CaptureFinishes++;
		PrintFormat("[TB_PVE_M2_5B] vanilla-CaptureFinish faction=%1 count=%2", faction.GetFactionKey(), s_CaptureFinishes);
	}

	protected static void UnsubscribeCaptureEvents()
	{
		if (!s_MobSeizing) return;
		s_MobSeizing.GetOnCaptureStart().Remove(OnCaptureStart);
		s_MobSeizing.GetOnCaptureInterrupt().Remove(OnCaptureInterrupt);
		s_MobSeizing.GetOnCaptureFinish().Remove(OnCaptureFinish);
		s_MobSeizing = null;
	}

	protected static string Owner(int index)
	{
		Faction owner = s_Route.GetRouteBase(index).GetFaction();
		if (!owner) return "<none>";
		return owner.GetFactionKey();
	}

	protected static void Cleanup()
	{
		foreach (IEntity character : s_Characters) if (character) SCR_EntityHelper.DeleteEntityAndChildren(character);
		s_Characters.Clear();
		if (s_Group) SCR_EntityHelper.DeleteEntityAndChildren(s_Group);
		s_Group = null;
	}

	protected static string Result(bool pass)
	{
		if (pass)
			return "PASS";
		else
			return "FAIL";
	}
	protected static void Fail(string reason)
	{
		PrintFormat("[TB_PVE_M2_5] FAIL proof mode=%1 phase=%2 target=%3 reason=%4", s_ProofMode, s_Phase, s_Target, reason);
		Cleanup(); UnsubscribeCaptureEvents(); GetGame().GetCallqueue().Remove(Tick);
	}
}
