// Read-only native panel inventory; preserve config-root metadata.
[BaseContainerProps(configRoot: true)]
modded class SCR_ChatPanelManager
{
	int TB_CC_GetRegisteredPanelCount()
	{
		return m_aChatPanels.Count();
	}
}

// Does not create UI, register input or send messages.
class TB_CC_ChatRuntimeDiagnostics
{
	static const string BUILD_TAG = "BCM_CHAT_CHAIN_03";
	protected static string s_ServerState;
	protected static string s_ClientState;

	static void Reset()
	{
		s_ServerState = "";
		s_ClientState = "";
	}

	static void Snapshot()
	{
		if (!GetGame().GetWorld()) return;
		BaseChatEntity chat = GetGame().GetChat();
		int role = -1;
		int channels;
		string chatClass = "NONE";
		if (chat)
		{
			chatClass = chat.Type().ToString();
			channels = chat.GetChannelsCount();
			RplComponent rpl = RplComponent.Cast(chat.FindComponent(RplComponent));
			if (rpl) role = rpl.Role();
		}
		if (Replication.IsServer())
		{
			string serverState = string.Format("[TB_CC_CHAT_SERVER] build=%1 getChat=%2 class=%3 rplRole=%4 channels=%5", BUILD_TAG, chat != null, chatClass, role, channels);
			if (serverState != s_ServerState)
			{
				Print(serverState);
				s_ServerState = serverState;
			}
		}
		if (System.IsConsoleApp()) return;
		SCR_ChatPanelManager manager = SCR_ChatPanelManager.GetInstance();
		SCR_HUDManagerComponent hud = SCR_HUDManagerComponent.GetHUDManager();
		SCR_ChatHud chatHud;
		Widget root;
		Widget slot;
		SCR_ChatPanel panel;
		int registeredPanels = -1;
		bool managerChat;
		if (manager)
		{
			registeredPanels = manager.TB_CC_GetRegisteredPanelCount();
			managerChat = manager.m_ChatEntity != null;
		}
		if (hud) chatHud = SCR_ChatHud.Cast(hud.FindInfoDisplay(SCR_ChatHud));
		if (chatHud)
		{
			root = chatHud.GetRootWidget();
			SCR_InfoDisplaySlotHandler handler = SCR_InfoDisplaySlotHandler.Cast(chatHud.GetHandler(SCR_InfoDisplaySlotHandler));
			if (handler) slot = handler.GetSlotWidget();
		}
		if (root)
		{
			Widget panelWidget = root.FindAnyWidget("ChatPanel");
			if (panelWidget) panel = SCR_ChatPanel.Cast(panelWidget.FindHandler(SCR_ChatPanel));
		}
		bool actionActive;
		bool panelOpen;
		InputManager input = GetGame().GetInputManager();
		if (input) actionActive = input.IsActionActive("ChatToggle");
		if (panel) panelOpen = panel.IsOpen();
		string reason = "NATIVE_CHAIN_PRESENT_INTERACTION_UNVERIFIED";
		if (!SCR_GameCoresManager.GetCore(SCR_EditorManagerCore)) reason = "EDITOR_CORE_MISSING";
		else if (!chat) reason = "CHAT_SERVICE_MISSING";
		else if (!manager) reason = "PANEL_MANAGER_MISSING";
		else if (!managerChat) reason = "PANEL_MANAGER_CHAT_MISSING";
		else if (!chatHud || !root || !slot || !panel) reason = "HUD_PANEL_NOT_INITIALIZED";
		else if (registeredPanels == 0) reason = "NO_REGISTERED_PANELS";
		string clientState = string.Format("[TB_CC_CHAT_CLIENT] build=%1 getChat=%2 panelManager=%3 registeredPanels=%4 chatHud=%5 slotChat=%6 panel=%7 panelOpen=%8", BUILD_TAG, chat != null, manager != null, registeredPanels, chatHud != null, slot != null, panel != null, panelOpen);
		// Action context activity is not proof of listener registration.
		clientState += string.Format(" inputListener=NOT_EXPOSED actionActive=%1 managerChat=%2 reason=%3", actionActive, managerChat, reason);
		if (clientState != s_ClientState)
		{
			Print(clientState);
			s_ClientState = clientState;
		}
	}
}
