// Read-only authority query separating durable vanilla faction participation
// from the transient existence of an alive, controlled world character.
class TB_PVE_M3_7_HumanPresenceQuery
{
	protected static string s_sLastTrace;

	static int GetParticipatingHumanCount(FactionKey factionKey)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return 0;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		int result;
		foreach (int playerId : playerIds)
		{
			Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
			if (faction && faction.GetFactionKey() == factionKey) result++;
		}
		return result;
	}

	static int GetActiveHumanObservers(FactionKey factionKey, out array<vector> observerPositions)
	{
		observerPositions.Clear();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return 0;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
			if (!faction || faction.GetFactionKey() != factionKey) continue;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playerManager.GetPlayerControlledEntity(playerId));
			if (!character) continue;
			CharacterControllerComponent characterController = character.GetCharacterController();
			if (!characterController || characterController.GetLifeState() != ECharacterLifeState.ALIVE) continue;
			observerPositions.Insert(character.GetOrigin() + Vector(0, 1.6, 0));
		}
		return observerPositions.Count();
	}

	// Emits only when the semantic player snapshot changes. This instrumentation
	// is diagnostic and does not retain or alter faction/lifecycle state.
	static void TraceAuthorityState(FactionKey relevantFactionKey)
	{
		if (!Replication.IsServer()) return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager) return;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		string snapshot = playerIds.Count().ToString();
		string details;
		foreach (int playerId : playerIds)
		{
			Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
			string playerFactionKey;
			if (playerFaction) playerFactionKey = playerFaction.GetFactionKey();
			IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(controlledEntity);
			CharacterControllerComponent characterController;
			int lifeState = -1;
			if (character)
			{
				characterController = character.GetCharacterController();
				if (characterController) lifeState = characterController.GetLifeState();
			}
			Faction entityFaction;
			if (controlledEntity) entityFaction = SCR_FactionManager.SGetFaction(controlledEntity);
			string entityFactionKey;
			if (entityFaction) entityFactionKey = entityFaction.GetFactionKey();
			bool hasPlayerController = playerManager.GetPlayerController(playerId) != null;
			string row = string.Format("id=%1 name=%2 faction=%3 controller=%4 entity=%5 character=%6 lifeState=%7 entityFaction=%8", playerId, playerManager.GetPlayerName(playerId), playerFactionKey, hasPlayerController, controlledEntity != null, character != null, lifeState, entityFactionKey);
			details += " [" + row + "]";
			snapshot += ":" + playerId.ToString() + ":" + playerFactionKey;
			snapshot += ":" + (controlledEntity != null).ToString() + ":" + lifeState.ToString();
			snapshot += ":" + entityFactionKey;
		}
		if (snapshot == s_sLastTrace) return;
		s_sLastTrace = snapshot;
		array<vector> observers = {};
		int observerCount = GetActiveHumanObservers(relevantFactionKey, observers);
		PrintFormat("[TB_PVE_M3_7A] human-presence players=%1 faction=%2 participants=%3 observers=%4 details=%5", playerIds.Count(), relevantFactionKey, GetParticipatingHumanCount(relevantFactionKey), observerCount, details);
	}
}
