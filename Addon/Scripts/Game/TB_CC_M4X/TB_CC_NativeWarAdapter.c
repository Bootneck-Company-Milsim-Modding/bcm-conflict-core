// Native campaign adapter. Registry traits/positions contain map-specific data;
// the reusable simulation has no Arland IDs, coordinates or terrain exceptions.
class TB_CC_NativeWarAdapter : TB_CC_WarMapAdapter
{
	protected TB_CC_M4XP_NativeArlandComponent m_Policy;
	void TB_CC_NativeWarAdapter(TB_CC_M4XP_NativeArlandComponent policy) { m_Policy = policy; }
	override bool IsActive()
	{
		return m_Policy && TB_PVE_M2_3_FrontierQuery.IsCampaignValid() && m_Policy.IsWarOperational();
	}
	override string PlayerObjective() { return m_Policy.GetBLUFORLegalObjective(); }
	override FactionKey Owner(string id) { return m_Policy.GetTerritorialOwner(id); }
	override bool Position(string id, out vector position) { return TB_PVE_M2_3_FrontierQuery.TryGetObjectiveWorldPosition(id, position); }
	override bool CanAttack(FactionKey faction, string id)
	{
		if (!IsActive() || !CanDefend(id) || !m_Policy.IsFootTerritory(id)) return false;
		if (m_Policy.GetResponses() && m_Policy.GetResponses().Authorizes(faction, id)) return true;
		if (faction == "FIA") return m_Policy.CanFIARecapture(id);
		if (faction == "USSR") return id == m_Policy.GetOPFORLegalObjective() && TB_PVE_M2_3_FrontierQuery.CanFactionStrategicallyAttack(faction, id);
		return false;
	}
	override bool CanDefend(string id)
	{
		return m_Policy.GetBase(id) && (TB_CC_M4XP_CampaignRoles.GetRole(id) == TB_CC_StrategicRole.ORDINARY || (id == m_Policy.GetSelectedEndpointId() && m_Policy.GetHQState(id) == TB_CC_HQLockState.VULNERABLE));
	}
	override bool CanTravel(string source, string target) { return m_Policy.WarFootRouteAllowed(source, target); }
	override int Importance(string id) { return m_Policy.GetPositionImportance(id); }
	override bool Source(FactionKey faction, string target, out string source, out vector position)
	{
		if (!m_Policy.WarSource(faction, target, source, position)) return false;
		// Virtual FIA starts in the SAME target-relative annulus as physical FIA.
		// No physical entity or navmesh path is fabricated at this abstract point.
		if (faction == "FIA")
		{
			vector targetPosition; if (!Position(target, targetPosition)) return false;
			vector delta = position - targetPosition; delta[1] = 0; float distance = delta.Length();
			if (distance < 1) return false;
			position = targetPosition + delta * (400 / distance);
			position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		}
		return true;
	}
	override bool CommitBackgroundCapture(TB_CC_WarForceState force)
	{
		if (!Replication.IsServer() || !force || !IsActive() || force.m_iRepresentation != TB_CC_WarRepresentation.VIRTUAL || !force.m_bCaptureCommitted || force.m_iStrength <= 0) return false;
		if (!CanAttack(force.m_sFaction, force.m_sTargetId) || Owner(force.m_sTargetId) != force.m_sExpectedOwner) return false;
		TB_CC_WarSimulation war = TB_CC_WarSimulation.Get();
		if (!war || war.IsTargetPhysical(force.m_sTargetId)) return false;
		SCR_CampaignMilitaryBaseComponent base = m_Policy.GetBase(force.m_sTargetId);
		Faction winner = GetGame().GetFactionManager().GetFactionByKey(force.m_sFaction);
		if (!base || !winner || base.GetFaction() == winner) return false;
		// THE ONLY background ownership write. Installed SCR_MilitaryBaseComponent
		// SetFaction(Faction) invokes affiliation -> native base manager callbacks.
		base.SetFaction(winner);
		bool changed = base.GetFaction() == winner;
		PrintFormat("[TB_CC_WAR] background-capture id=%1 target=%2 previous=%3 winner=%4 committed=%5 nativeCallbacks=1", force.m_sForceId, force.m_sTargetId, force.m_sExpectedOwner, force.m_sFaction, changed);
		return changed;
	}
}
