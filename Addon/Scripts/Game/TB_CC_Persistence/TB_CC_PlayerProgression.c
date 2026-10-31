// Campaign-scoped reconstruction. Native rewards, thresholds and permissions stay native.
class TB_CC_PlayerProgression
{
	static ref TB_CC_PlayerProgression s_Active;
	protected TB_CC_PersistenceService m_Store;
	protected SCR_BaseGameMode m_GameMode;
	protected string m_Campaign;
	protected bool m_ResetNewGeneration;
	protected ref map<string, int> m_XP = new map<string, int>();
	protected ref map<int, string> m_Identities = new map<int, string>();
	protected ref map<int, SCR_PlayerXPHandlerComponent> m_Components = new map<int, SCR_PlayerXPHandlerComponent>();
	protected ref map<string, SCR_PlayerXPHandlerComponent> m_IdentityOwners = new map<string, SCR_PlayerXPHandlerComponent>();
	protected ref array<int> m_Deferred = {};
	protected string m_CaptureError;
	string GetCaptureError() { return m_CaptureError; }

	// v2 reserved players array: version|backend UUID|signed native XP. No rank authority.
	static bool Decode(string row, out string identity, out int xp)
	{
		array<string> fields = {}; row.Split("|", fields, false);
		if (fields.Count() != 3 || fields[0] != "1" || !UUID.IsUUID(fields[1])) return false;
		UUID account = fields[1]; if (account.IsNull()) return false;
		identity = fields[1]; xp = fields[2].ToInt();
		string canonical = identity; canonical.ToLower();
		return xp.ToString() == fields[2] && identity == canonical;
	}
	static bool Validate(array<string> rows)
	{
		if (!rows) return false;
		array<string> seen = {};
		foreach (string row : rows)
		{
			string identity; int xp;
			if (!Decode(row, identity, xp) || seen.Contains(identity)) return false;
			seen.Insert(identity);
		}
		return true;
	}
	bool Start(TB_CC_PersistenceService store)
	{
		if (!Replication.IsServer() || !store || !store.IsReady() || s_Active) return false;
		m_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!m_GameMode) return false;
		m_Store = store; m_Campaign = store.GetCampaignId();
		m_ResetNewGeneration = store.IsResetActivationPending();
		foreach (string row : store.GetLoadedContainer().players)
		{
			string identity; int xp;
			if (!Decode(row, identity, xp)) return false;
			m_XP.Set(identity, xp);
		}
		s_Active = this;
		// Native world-end invoker runs BEFORE additional gamemode components.
		m_GameMode.GetOnGameEnd().Insert(CheckpointBeforeTeardown);
		GetGame().GetCallqueue().CallLater(Poll, 1000, true);
		return true;
	}
	protected bool Ready()
	{
		return Replication.IsServer() && m_Store && m_Store.IsReady() && m_Store.GetCampaignId() == m_Campaign;
	}
	protected void Observe(int playerId)
	{
		if (!Ready() || !m_Components.Contains(playerId) || !m_Identities.Contains(playerId)) return;
		SCR_PlayerXPHandlerComponent component = m_Components.Get(playerId);
		string identity = m_Identities.Get(playerId);
		if (!component || m_IdentityOwners.Get(identity) != component) return;
		int xp = component.GetPlayerXP();
		if (!m_XP.Contains(identity) || m_XP.Get(identity) != xp)
		{
			m_XP.Set(identity, xp); m_Store.MarkDirty("player-xp");
		}
	}
	void OnXPChanged(int playerId, int currentXP, int XPToAdd, SCR_EXPRewards rewardId)
	{
		// Ignore callback values as input: re-read registered authority component.
		Observe(playerId);
	}
	protected void Unbind(int playerId)
	{
		SCR_PlayerXPHandlerComponent component = m_Components.Get(playerId);
		if (component) component.GetOnPlayerXPChanged().Remove(OnXPChanged);
		string identity = m_Identities.Get(playerId);
		if (m_IdentityOwners.Get(identity) == component) m_IdentityOwners.Remove(identity);
		m_Components.Remove(playerId); m_Identities.Remove(playerId); m_Deferred.RemoveItem(playerId);
	}
	void Leave(int playerId)
	{
		Observe(playerId); Unbind(playerId);
		if (Ready() && m_Store.IsDirty()) m_Store.SaveNow("player-disconnect");
	}
	void Poll()
	{
		if (!Ready() || !GetGame().GetPlayerManager() || !GetGame().GetGameMode()) return;
		SCR_XPHandlerComponent nativeXP = SCR_XPHandlerComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_XPHandlerComponent));
		if (!nativeXP) return;
		array<int> players = {}; GetGame().GetPlayerManager().GetPlayers(players);
		array<int> departed = {};
		for (int index = 0; index < m_Components.Count(); index++)
		{
			int tracked = m_Components.GetKey(index); if (!players.Contains(tracked)) departed.Insert(tracked);
		}
		foreach (int gone : departed) Leave(gone);
		foreach (int playerId : players)
		{
			if (!nativeXP.TB_CC_IsAccountAudited(playerId))
			{
				if (!m_Deferred.Contains(playerId))
				{
					m_Deferred.Insert(playerId);
					PrintFormat("[TB_CC_PERSIST_PLAYER] session=%1 result=DEFERRED reason=account-audit-pending", playerId);
				}
				continue;
			}
			// Deliberately bypass the native utility's name-based offline polyfill.
			string identity = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
			identity.ToLower();
			UUID account = identity;
			if (!UUID.IsUUID(identity) || account.IsNull())
			{
				if (!m_Deferred.Contains(playerId))
				{
					m_Deferred.Insert(playerId);
					PrintFormat("[TB_CC_PERSIST_PLAYER] session=%1 result=DEFERRED reason=backend-identity-unavailable", playerId);
				}
				continue;
			}
			PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!controller) continue;
			SCR_PlayerXPHandlerComponent component = SCR_PlayerXPHandlerComponent.Cast(controller.FindComponent(SCR_PlayerXPHandlerComponent));
			if (!component || !component.TB_CC_CanRestoreCampaignXP()) continue;
			if (m_Components.Get(playerId) == component && m_Identities.Get(playerId) == identity)
			{
				Observe(playerId); continue;
			}
			// Controller replacement: flush old live XP before reconstructing its successor.
			Observe(playerId); Unbind(playerId);
			SCR_PlayerXPHandlerComponent previousOwner = m_IdentityOwners.Get(identity);
			if (previousOwner && previousOwner != component) continue; // duplicate active identity: defer
			// Explicit restart adoption only: fresh generation must not inherit a retained controller.
			if (!m_XP.Contains(identity) && m_ResetNewGeneration)
			{
				if (!component.TB_CC_RestoreCampaignXP(0)) continue;
				component.SetPlayerXPSinceLastSpawn(0); component.SetSurvivalRewardCycle(0);
			}
			if (m_XP.Contains(identity) && !component.TB_CC_RestoreCampaignXP(m_XP.Get(identity))) continue;
			m_Identities.Set(playerId, identity); m_Components.Set(playerId, component);
			m_IdentityOwners.Set(identity, component);
			component.GetOnPlayerXPChanged().Insert(OnXPChanged);
			Observe(playerId);
			PrintFormat("[TB_CC_PERSIST_PLAYER] session=%1 result=BOUND xp=%2 rank=%3", playerId, component.GetPlayerXP(), component.GetPlayerRankByXP());
		}
	}
	bool Capture(TB_CC_CampaignContainer snapshot)
	{
		m_CaptureError = "authority-readiness-or-campaign-generation";
		if (!Ready() || !snapshot || snapshot.campaignId != m_Campaign) return false;
		for (int index = 0; index < m_Components.Count(); index++) Observe(m_Components.GetKey(index));
		snapshot.players.Clear();
		array<string> identities = {};
		for (int i = 0; i < m_XP.Count(); i++) identities.Insert(m_XP.GetKey(i));
		identities.Sort();
		foreach (string identity : identities) snapshot.players.Insert("1|" + identity + "|" + m_XP.Get(identity).ToString());
		m_CaptureError = "player-record-version-UUID-integer-or-duplicate";
		if (!Validate(snapshot.players)) return false;
		m_CaptureError = ""; return true;
	}
	void CheckpointBeforeTeardown()
	{
		if (!Ready()) return;
		for (int index = 0; index < m_Components.Count(); index++) Observe(m_Components.GetKey(index));
		// Force fresh coherent export; never assert a previous cached root is current.
		m_Store.MarkDirty("world-end-checkpoint");
		m_Store.SaveNow("world-end-before-gamemode-component-cleanup");
	}
	// Future P5 calls ONLY after explicitly adopting a fresh root campaign ID.
	// This seam never changes campaign ID, strategic state, files or admin commands.
	bool ResetForAdoptedCampaign(string newCampaignId)
	{
		if (!Replication.IsServer() || !m_Store || !m_Store.IsReady()) return false;
		UUID generation = newCampaignId;
		if (!UUID.IsUUID(newCampaignId) || generation.IsNull() || newCampaignId == m_Campaign || m_Store.GetCampaignId() != newCampaignId) return false;
		foreach (int playerId, SCR_PlayerXPHandlerComponent component : m_Components)
		{
			if (component && !component.TB_CC_CanRestoreCampaignXP()) return false;
		}
		foreach (int resetPlayerId, SCR_PlayerXPHandlerComponent resetComponent : m_Components)
		{
			if (!resetComponent) continue;
			resetComponent.TB_CC_RestoreCampaignXP(0);
			resetComponent.SetPlayerXPSinceLastSpawn(0); resetComponent.SetSurvivalRewardCycle(0);
		}
		m_XP.Clear(); m_Campaign = newCampaignId;
		m_Store.GetLoadedContainer().players.Clear();
		m_Store.MarkDirty("new-campaign-player-reset");
		return true;
	}
	void Stop()
	{
		if (m_GameMode) m_GameMode.GetOnGameEnd().Remove(CheckpointBeforeTeardown);
		m_GameMode = null;
		GetGame().GetCallqueue().Remove(Poll);
		for (int index = 0; index < m_Components.Count(); index++) Observe(m_Components.GetKey(index));
		array<int> tracked = {};
		for (int i = 0; i < m_Components.Count(); i++) tracked.Insert(m_Components.GetKey(i));
		foreach (int playerId : tracked) Unbind(playerId);
		if (s_Active == this) s_Active = null;
	}
}

modded class SCR_PlayerXPHandlerComponent
{
	protected bool m_TB_CC_RestoredProgression;
	bool TB_CC_CanRestoreCampaignXP()
	{
		if (!Replication.IsServer() || IsProxy() || !GetGame().GetFactionManager()) return false;
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		SCR_FactionManager factions = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		return controller && factions && factions.GetFactionRanks(controller.GetPlayerId());
	}
	override void AddPlayerXP(SCR_EXPRewards rewardID, float multiplier = 1.0, bool volunteer = false, int addDirectly = 0)
	{
		// Resolve an audited controller before its first award, not one poll later.
		if (Replication.IsServer() && TB_CC_PlayerProgression.s_Active) TB_CC_PlayerProgression.s_Active.Poll();
		super.AddPlayerXP(rewardID, multiplier, volunteer, addDirectly);
	}
	override void UpdatePlayerRank(bool notify = true)
	{
		if (m_TB_CC_RestoredProgression && !notify && Replication.IsServer())
		{
			SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
			if (!controller || !controller.GetMainEntity()) return;
			SCR_CharacterRankComponent rank = SCR_CharacterRankComponent.Cast(controller.GetMainEntity().FindComponent(SCR_CharacterRankComponent));
			if (rank) rank.TB_CC_ReconstructCampaignRank(GetPlayerRankByXP());
			return;
		}
		super.UpdatePlayerRank(notify);
	}
	// Native authority field and native rank/replication; deliberately NO award invocation/RPC.
	bool TB_CC_RestoreCampaignXP(int xp)
	{
		if (!TB_CC_CanRestoreCampaignXP()) return false;
		m_TB_CC_RestoredProgression = true; m_iPlayerXP = xp;
		Replication.BumpMe(); UpdatePlayerRank(false);
		return GetPlayerXP() == xp;
	}
}

class TB_CC_QuietRankRestore
{
	static IEntity s_Character;
}

modded class SCR_CharacterRankComponent
{
	void TB_CC_ReconstructCampaignRank(SCR_ECharacterRank rank)
	{
		if (!Replication.IsServer() || rank == m_iRank) return;
		Rpc(TB_CC_RpcDoReconstructRank, rank, m_iRank);
		TB_CC_RpcDoReconstructRank(rank, m_iRank);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void TB_CC_RpcDoReconstructRank(SCR_ECharacterRank rank, SCR_ECharacterRank previousRank)
	{
		IEntity previousScope = TB_CC_QuietRankRestore.s_Character;
		TB_CC_QuietRankRestore.s_Character = GetOwner();
		// Native rank change still updates native consumers/permissions and replication.
		RpcDoSetCharacterRank(rank, previousRank, true);
		TB_CC_QuietRankRestore.s_Character = previousScope;
	}
}

modded class SCR_DataCollectorAchievementsModule
{
	override protected void RankedUp(SCR_ECharacterRank prevRank, SCR_ECharacterRank newRank, IEntity playerEntity, bool silent)
	{
		if (silent && playerEntity && TB_CC_QuietRankRestore.s_Character == playerEntity) return;
		super.RankedUp(prevRank, newRank, playerEntity, silent);
	}
}

modded class SCR_XPHandlerComponent
{
	protected ref array<int> m_TB_CC_AuditedAccounts = {};
	bool TB_CC_IsAccountAudited(int playerId) { return m_TB_CC_AuditedAccounts.Contains(playerId); }
	override void OnPlayerAuditSuccess(int playerId)
	{
		super.OnPlayerAuditSuccess(playerId);
		if (Replication.IsServer() && !m_TB_CC_AuditedAccounts.Contains(playerId)) m_TB_CC_AuditedAccounts.Insert(playerId);
	}
	override void OnPlayerAuditFail(int playerId)
	{
		if (Replication.IsServer() && TB_CC_PlayerProgression.s_Active) TB_CC_PlayerProgression.s_Active.Leave(playerId);
		m_TB_CC_AuditedAccounts.RemoveItem(playerId); super.OnPlayerAuditFail(playerId);
	}
	override void OnPlayerAuditRevived(int playerId)
	{
		super.OnPlayerAuditRevived(playerId);
		if (Replication.IsServer() && !m_TB_CC_AuditedAccounts.Contains(playerId)) m_TB_CC_AuditedAccounts.Insert(playerId);
	}
	override void OnPlayerAuditTimeouted(int playerId)
	{
		if (Replication.IsServer() && TB_CC_PlayerProgression.s_Active) TB_CC_PlayerProgression.s_Active.Leave(playerId);
		m_TB_CC_AuditedAccounts.RemoveItem(playerId); super.OnPlayerAuditTimeouted(playerId);
	}
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (Replication.IsServer() && TB_CC_PlayerProgression.s_Active) TB_CC_PlayerProgression.s_Active.Leave(playerId);
		m_TB_CC_AuditedAccounts.RemoveItem(playerId);
		super.OnPlayerDisconnected(playerId, cause, timeout);
	}
}
