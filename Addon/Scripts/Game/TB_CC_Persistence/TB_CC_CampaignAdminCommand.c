// Native command transport supplies caller context; no BCM server-request RPC.
class TB_CC_CampaignAdminCommand : ScrServerCommand
{
	override string GetKeyword() { return "bcmcampaign"; }
	override bool IsServerSide() { return true; }
	override int RequiredRCONPermission() { return ERCONPermissions.PERMISSIONS_ADMIN; }
	override int RequiredChatPermission() { return EPlayerRole.ADMINISTRATOR; }
	override ref ScrServerCmdResult OnUpdate()
	{
		// Required native API, including synchronous commands (native KickCommand).
		// Execution callbacks return final OK/ERR; no PENDING work to poll or replay.
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}
	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		// If native denial appears without this marker, BCM execution was not entered.
		PrintFormat("[TB_CC_P5_AUTH] stage=CHAT_CLIENT_ENTER route=%1", GetKeyword());
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}
	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		PrintFormat("[TB_CC_P5_AUTH] stage=CHAT_SERVER_ENTER route=%1", GetKeyword());
		if (!Replication.IsServer() || !GetGame().GetPlayerManager() || !SCR_Global.IsAdmin(playerId))
		{
			PrintFormat("[TB_CC_P5_AUTH] stage=CHAT_SERVER_CHECK route=%1 result=DENIED reason=native-admin-or-authority", GetKeyword());
			return ScrServerCmdResult("BCM Campaign Administration: denied; native admin required", EServerCmdResultType.ERR);
		}
		PrintFormat("[TB_CC_P5_AUTH] stage=CHAT_SERVER_CHECK route=%1 result=ALLOWED", GetKeyword());
		return ExecuteAuthorized(argv, "CHAT_ADMIN", playerId);
	}
	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		Print("[TB_CC_P5_AUTH] stage=RCON_SERVER_ENTER route=bcmcampaign");
		// Engine invokes this protected callback only after RequiredRCONPermission.
		if (!Replication.IsServer())
		{
			Print("[TB_CC_P5_AUTH] stage=RCON_SERVER_CHECK result=DENIED reason=not-authority");
			return ScrServerCmdResult("Denied: server authority required", EServerCmdResultType.ERR);
		}
		return ExecuteAuthorized(argv, "RCON_ADMIN", 0);
	}
	protected ScrServerCmdResult ExecuteAuthorized(array<string> argv, string source, int session)
	{
		TB_CC_PersistenceService store = TB_CC_PersistenceService.GetAdministrationStore();
		if (!store)
		{
			PrintFormat("[TB_CC_P5_AUTH] stage=DISPATCH route=%1 result=UNAVAILABLE reason=campaign-store", GetKeyword());
			return ScrServerCmdResult("BCM Campaign Administration: campaign store unavailable", EServerCmdResultType.ERR);
		}
		bool success;
		string response = store.AdministrationRequest(argv, source, session, success);
		PrintFormat("[TB_CC_P5_AUTH] stage=DISPATCH route=%1 success=%2", GetKeyword(), success);
		EServerCmdResultType result = EServerCmdResultType.ERR; if (success) result = EServerCmdResultType.OK;
		return ScrServerCmdResult(response, result);
	}
}

// Separate native gate avoids assuming a combined role mask means OR.
// Offline local host must possess SESSION_ADMINISTRATOR; no editor/GM fallback.
class TB_CC_LocalCampaignAdminCommand : TB_CC_CampaignAdminCommand
{
	override string GetKeyword() { return "bcmcampaignlocal"; }
	override int RequiredChatPermission()
	{
		Print("[TB_CC_P5_AUTH] stage=NATIVE_CHAT_REQUIREMENT route=bcmcampaignlocal required=SESSION_ADMINISTRATOR");
		return EPlayerRole.SESSION_ADMINISTRATOR;
	}
	override ref ScrServerCmdResult OnUpdate()
	{
		// Explicit lifecycle binding for this separately registered native keyword.
		return super.OnUpdate();
	}
	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		Print("[TB_CC_P5_AUTH] stage=LOCAL_SERVER_ENTER route=bcmcampaignlocal");
		// No online/listen/client/dedicated execution, even by a native admin.
		if (!Replication.IsServer() || RplSession.Mode() != RplMode.None || !GetGame().InPlayMode() || !GetGame().GetWorld())
		{
			Print("[TB_CC_P5_AUTH] stage=LOCAL_SERVER_CHECK result=DENIED reason=offline-authority-only");
			return ScrServerCmdResult("Local command requires an offline local session; use bcmcampaign with native administrator authority online", EServerCmdResultType.ERR);
		}
		PlayerController localController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!localController || !playerManager || playerId <= 0 || localController.GetPlayerId() != playerId || playerManager.GetPlayerController(playerId) != localController || !playerManager.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR))
		{
			Print("[TB_CC_P5_AUTH] stage=LOCAL_SERVER_CHECK result=DENIED reason=local-controller-or-native-session-admin");
			return ScrServerCmdResult("Local command denied: local controller and native session-administrator role required", EServerCmdResultType.ERR);
		}
		return super.OnChatServerExecution(argv, playerId);
	}
	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		Print("[TB_CC_P5_AUTH] stage=LOCAL_RCON_CHECK result=DENIED");
		return ScrServerCmdResult("Use bcmcampaign through authenticated admin RCON", EServerCmdResultType.ERR);
	}
}

#ifdef WORKBENCH
// Explicit Script Editor console entry point, not a chat/RCON/RPC handler.
// Grants a REAL native session role only in offline play to the engine's sole
// registered local controller. No supplied player ID, automatic grant or GM test.
class TB_CC_WorkbenchCampaignAdmin
{
	static void DiagnoseLocalHost()
	{
		if (!GetGame())
		{
			Print("[TB_CC_P5_AUTH] stage=WB_DIAG result=UNAVAILABLE reason=no-game");
			return;
		}
		PrintFormat("[TB_CC_P5_AUTH] stage=WB_DIAG inPlay=%1 authority=%2 mode=%3", GetGame().InPlayMode(), Replication.IsServer(), RplSession.Mode());
		PlayerController localController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!localController || !playerManager)
		{
			Print("[TB_CC_P5_AUTH] stage=WB_DIAG result=UNAVAILABLE reason=local-player-not-ready");
			return;
		}
		int localId = localController.GetPlayerId();
		array<int> players = {};
		playerManager.GetPlayers(players);
		PrintFormat("[TB_CC_P5_AUTH] stage=WB_DIAG connectedCount=%1 localRegistered=%2 nativeAdmin=%3 nativeSessionAdmin=%4 controllerSessionAdmin=%5", players.Count(), playerManager.GetPlayerController(localId) == localController, playerManager.HasPlayerRole(localId, EPlayerRole.ADMINISTRATOR), playerManager.HasPlayerRole(localId, EPlayerRole.SESSION_ADMINISTRATOR), localController.HasRole(EPlayerRole.SESSION_ADMINISTRATOR));
	}
	static bool AuthorizeLocalHost()
	{
		// RplMode.None is the installed singleplayer boundary; listen is multiplayer.
		if (!GetGame() || !GetGame().InPlayMode() || !GetGame().GetWorld() || !Replication.IsServer() || RplSession.Mode() != RplMode.None)
		{
			Print("[TB_CC_P5_AUTH] stage=WB_GRANT result=DENIED reason=offline-play-authority-only");
			return false;
		}
		PlayerController localController = GetGame().GetPlayerController();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!localController || !playerManager)
		{
			Print("[TB_CC_P5_AUTH] stage=WB_GRANT result=DENIED reason=local-player-not-ready");
			return false;
		}
		int localId = localController.GetPlayerId();
		array<int> players = {};
		playerManager.GetPlayers(players);
		if (localId <= 0 || players.Count() != 1 || players[0] != localId || playerManager.GetPlayerController(localId) != localController || !playerManager.IsPlayerConnected(localId))
		{
			Print("[TB_CC_P5_AUTH] stage=WB_GRANT result=DENIED reason=sole-connected-local-controller-required");
			return false;
		}
		if (!playerManager.HasPlayerRole(localId, EPlayerRole.SESSION_ADMINISTRATOR))
		{
			if (!playerManager.GivePlayerRole(localId, EPlayerRole.SESSION_ADMINISTRATOR))
			{
				Print("[TB_CC_P5_AUTH] stage=WB_GRANT result=DENIED reason=native-role-grant-failed");
				return false;
			}
		}
		bool verified = playerManager.HasPlayerRole(localId, EPlayerRole.SESSION_ADMINISTRATOR) && localController.HasRole(EPlayerRole.SESSION_ADMINISTRATOR);
		PrintFormat("[TB_CC_P5_AUTH] stage=WB_GRANT nativeSessionAdminVerified=%1", verified);
		return verified;
	}
}
#endif

modded class SCR_GameModeCampaign
{
	[RplProp(onRplName: "TB_CC_OnResetPendingChanged")]
	protected bool m_TB_CC_ResetPendingNotice;
	protected bool m_TB_CC_ResetNoticeShown;
	protected int m_TB_CC_ResetNoticeAttempts;
	void TB_CC_AnnounceCampaignReset()
	{
		if (!Replication.IsServer()) return;
		m_TB_CC_ResetPendingNotice = true; Replication.BumpMe();
		TB_CC_OnResetPendingChanged();
	}
	protected void TB_CC_OnResetPendingChanged()
	{
		if (!m_TB_CC_ResetPendingNotice || m_TB_CC_ResetNoticeShown || RplSession.Mode() == RplMode.Dedicated) return;
		if (SCR_HintManagerComponent.ShowCustomHint("A new campaign is pending. A scenario/server restart is required; progress after staging will not be saved.", "BCM Campaign Administration", 15, true)) m_TB_CC_ResetNoticeShown = true;
		else if (m_TB_CC_ResetNoticeAttempts < 10)
		{
			m_TB_CC_ResetNoticeAttempts++;
			GetGame().GetCallqueue().Remove(TB_CC_OnResetPendingChanged);
			GetGame().GetCallqueue().CallLater(TB_CC_OnResetPendingChanged, 1000, false);
		}
	}
	override void OnGameEnd()
	{
		GetGame().GetCallqueue().Remove(TB_CC_OnResetPendingChanged);
		super.OnGameEnd();
	}
}
