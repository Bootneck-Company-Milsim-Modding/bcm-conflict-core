// Read-only native editor permission evidence. Access continues to be assigned
// by SCR_EditorManagerCore and the server's native administrator roles.
[BaseContainerProps(configRoot: true)]
modded class SCR_EditorManagerCore
{
	protected ref map<int, string> m_TB_CC_GMStates = new map<int, string>();

	override void OnGameStart()
	{
		super.OnGameStart();
		TB_CC_ChatRuntimeDiagnostics.Reset();
		GetGame().GetCallqueue().Remove(TB_CC_ChatRuntimeDiagnostics.Snapshot);
		GetGame().GetCallqueue().CallLater(TB_CC_ChatRuntimeDiagnostics.Snapshot, 1000, false);
		GetGame().GetCallqueue().CallLater(TB_CC_ChatRuntimeDiagnostics.Snapshot, 5000, true);
		if (!Replication.IsServer()) return;
		GetGame().GetCallqueue().Remove(TB_CC_TraceGMPlayers);
		m_TB_CC_GMStates.Clear();
		GetGame().GetCallqueue().CallLater(TB_CC_TraceGMPlayers, 5000, true);
	}

	override void OnGameEnd()
	{
		GetGame().GetCallqueue().Remove(TB_CC_ChatRuntimeDiagnostics.Snapshot);
		GetGame().GetCallqueue().Remove(TB_CC_TraceGMPlayers);
		m_TB_CC_GMStates.Clear();
		super.OnGameEnd();
	}

	protected void TB_CC_TraceGMPlayers()
	{
		if (!Replication.IsServer()) return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return;
		array<int> ids = {}; players.GetPlayers(ids);
		for (int index = m_TB_CC_GMStates.Count() - 1; index >= 0; index--)
		{
			int previousId = m_TB_CC_GMStates.GetKey(index);
			if (!ids.Contains(previousId)) m_TB_CC_GMStates.Remove(previousId);
		}
		foreach (int playerId : ids) TB_CC_TraceGMPlayer(playerId, players);
	}

	protected void TB_CC_TraceGMPlayer(int playerId, PlayerManager players)
	{
		SCR_EditorManagerEntity editor = GetEditorManager(playerId);
		SCR_EditorSettingsEntity settings = GetSettingsEntity();
		BackendApi backend = GetGame().GetBackendApi();
		bool listed = backend && backend.IsListedServerAdmin(playerId);
		bool admin = SCR_Global.IsAdmin(playerId);
		int roles = players.GetPlayerRoles(playerId);
		bool disabled = settings && settings.IsEditorDisabled();
		bool unlimitedLegal = settings && settings.IsUnlimitedEditorLegal();
		bool adminEditConfigured = SCR_Enum.HasFlag(GetBaseModes(EEditorModeFlag.ADMIN, true), EEditorMode.EDIT);
		bool hasEdit; bool hasAdmin; bool canOpen; bool opened; int modes;
		if (editor)
		{
			hasEdit = editor.HasMode(EEditorMode.EDIT); hasAdmin = editor.HasMode(EEditorMode.ADMIN);
			canOpen = editor.CanOpen(); opened = editor.IsOpened(); modes = editor.GetModes();
		}
		string reason = "native-edit-mode-available";
		if (disabled) reason = "scenario-editor-disabled";
		else if (!editor) reason = "player-editor-manager-unavailable";
		else if (!hasEdit && !admin) reason = "native-admin-role-or-explicit-edit-permission-required";
		else if (!hasEdit && !adminEditConfigured) reason = "admin-edit-mode-missing-from-core-config";
		else if (!hasEdit) reason = "admin-role-present-edit-mode-not-created";
		else if (!canOpen) reason = "edit-mode-present-current-opening-gate-blocked";
		// Budget changes alone do not spam permission diagnostics. Inspect the
		// player's EDIT mode, not the dedicated host's nonexistent local editor.
		string signature = string.Format("admin=%1 listed=%2 editor=%3 edit=%4 adminMode=%5 modes=%6 disabled=%7 open=%8 reason=%9", admin, listed, editor != null, hasEdit, hasAdmin, modes, disabled, opened, reason);
		signature += string.Format(" roles=%1 unlimitedLegal=%2 adminEditConfigured=%3 canOpen=%4", roles, unlimitedLegal, adminEditConfigured, canOpen);
		if (m_TB_CC_GMStates.Get(playerId) == signature) return;
		m_TB_CC_GMStates.Set(playerId, signature);
		PrintFormat("[TB_CC_GM] playerId=%1 adminDetected=%2 listedAdmin=%3 editorPermission=%4 editorAvailable=%5 adminMode=%6 currentModeCanOpen=%7 reason=%8", playerId, admin, listed, hasEdit, editor != null, hasAdmin, canOpen, reason);
		PrintFormat("[TB_CC_GM] playerId=%1 roles=%2 modes=%3 editorDisabled=%4 scenarioUnlimitedEditorLegal=%5 adminEditConfigured=%6 opened=%7", playerId, roles, modes, disabled, unlimitedLegal, adminEditConfigured, opened);
		SCR_EditorModeEntity editMode;
		if (editor) editMode = editor.FindModeEntity(EEditorMode.EDIT);
		SCR_BudgetEditorComponent budget;
		if (editMode) budget = SCR_BudgetEditorComponent.Cast(editMode.FindComponent(SCR_BudgetEditorComponent));
		int maximum;
		if (budget && budget.GetMaxBudgetValue(EEditableEntityBudget.AI, maximum))
			PrintFormat("[TB_CC_GM] playerId=%1 budgetAvailable=1 budgetCurrent=%2 budgetMax=%3 budgetType=AI nativeLimitsUnchanged=1", playerId, budget.GetCurrentBudgetValue(EEditableEntityBudget.AI), maximum);
		else PrintFormat("[TB_CC_GM] playerId=%1 budgetAvailable=0 budgetCurrent=UNAVAILABLE budgetMax=UNAVAILABLE", playerId);
	}
}
