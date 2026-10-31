// Generic Conflict Core presentation projection on the vanilla replicated
// game mode. Remaining time is sampled from the authority deadline, not from
// a client gameplay timer. RplProp current state also supplies late joiners.
modded class SCR_GameModeCampaign
{
	[RplProp()] protected int m_iTBCCPressureOutcome;
	[RplProp()] protected int m_iTBCCPressureDuration;
	[RplProp()] protected bool m_bTBCCFoothold;
	[RplProp()] protected bool m_bTBCCRestoredPresentation;
	[RplProp()] protected int m_iTBCCRestoredHQEast;
	[RplProp()] protected int m_iTBCCRestoredHQWest;
	void TB_CC_MarkRestoredPresentation(bool restored, int east = 0, int west = 0)
	{
		if (!Replication.IsServer()) return;
		m_bTBCCRestoredPresentation = restored; m_iTBCCRestoredHQEast = east; m_iTBCCRestoredHQWest = west;
		Replication.BumpMe();
	}
	bool TB_CC_GetRestoredPresentation() { return m_bTBCCRestoredPresentation; }
	int TB_CC_GetRestoredHQEast() { return m_iTBCCRestoredHQEast; }
	int TB_CC_GetRestoredHQWest() { return m_iTBCCRestoredHQWest; }
	[RplProp(onRplName: "TB_CC_OnPressureProjection")] protected int m_iTBCCPressureRemaining = -1;

	void TB_CC_PublishPressure(int outcome, int remaining, int duration, bool foothold)
	{
		if (!Replication.IsServer()) return;
		if (m_iTBCCPressureOutcome == outcome && m_iTBCCPressureRemaining == remaining && m_iTBCCPressureDuration == duration && m_bTBCCFoothold == foothold) return;
		m_iTBCCPressureOutcome = outcome;
		m_iTBCCPressureDuration = duration;
		m_bTBCCFoothold = foothold;
		m_iTBCCPressureRemaining = remaining;
		Replication.BumpMe();
		TB_CC_OnPressureProjection();
		if (remaining < 0 || remaining % 10 == 0) PrintFormat("[TB_CC_M4_4J] pressure-projection outcome=%1 remaining=%2 duration=%3 foothold=%4 authority=1", outcome, remaining, duration, foothold);
	}
	protected void TB_CC_OnPressureProjection() { TB_CC_VictoryPressurePresentation.Refresh(); }
	int TB_CC_GetPressureOutcome() { return m_iTBCCPressureOutcome; }
	int TB_CC_GetPressureRemaining() { return m_iTBCCPressureRemaining; }
	int TB_CC_GetPressureDuration() { return m_iTBCCPressureDuration; }
	bool TB_CC_GetFoothold() { return m_bTBCCFoothold; }
}

// One non-interactive overlay for both directions. No input interception,
// world/ownership mutation, independent deadline, or Arland location logic.
class TB_CC_VictoryPressurePresentation
{
	protected static Widget s_Root;
	protected static Widget s_Announcement;
	protected static RichTextWidget s_Title;
	protected static RichTextWidget s_Body;
	protected static Widget s_Countdown;
	protected static RichTextWidget s_CountdownLabel;
	protected static RichTextWidget s_CountdownDigits;
	protected static int s_iAnnouncementStarted;
	protected static bool s_bAnnouncementActive;
	protected static bool s_bLayoutWarning;
	protected static bool s_bWasPressure;
	protected static bool s_bHQVulnerable;
	protected static bool s_bOurHQVulnerable;
	protected static bool s_bRestoreBaselineApplied;
	protected static string s_sPendingTitle;
	protected static string s_sPendingBody;
	static const int ANNOUNCEMENT_SECONDS = 8;
	static const ResourceName LAYOUT = "{A4F3600000000001}UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout";
	static void Start()
	{
		Stop();
		if (RplSession.Mode() == RplMode.Dedicated) return;
		GetGame().GetCallqueue().CallLater(Refresh, 250, true);
	}
	static void Stop()
	{
		GetGame().GetCallqueue().Remove(Refresh);
		ClearWidgets();
		s_bAnnouncementActive = false; s_iAnnouncementStarted = 0; s_bLayoutWarning = false;
		s_bWasPressure = false; s_bHQVulnerable = false; s_bOurHQVulnerable = false; s_sPendingTitle = ""; s_sPendingBody = "";
		s_bRestoreBaselineApplied = false;
	}
	static string Title(int outcome)
	{
		if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) return "MOB UNDER SIEGE";
		if (outcome == TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE) return "TOTAL TERRITORIAL CONTROL";
		return "";
	}
	static string Body(int outcome)
	{
		if (SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy())
		{
			if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) return "Enemy forces control all ordinary positions.\nBreak their control before your HQ becomes vulnerable.";
			return "All strategic positions secured.\nHold the line until the enemy headquarters becomes vulnerable.";
		}
		if (outcome == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) return "Enemy forces control all strategic positions.\nRecapture a strategic position before time expires.";
		return "Hold all strategic positions until the timer expires to secure victory.";
	}
	static string Countdown(int remaining)
	{
		remaining = Math.Max(remaining, 0);
		int minutes = remaining / 60;
		int seconds = remaining % 60;
		string minuteText = minutes.ToString(); string secondText = seconds.ToString();
		if (minutes < 10) minuteText = "0" + minuteText;
		if (seconds < 10) secondText = "0" + secondText;
		return minuteText + ":" + secondText;
	}
	protected static void ClearWidgets()
	{
		if (s_Root) s_Root.RemoveFromHierarchy();
		s_Root = null; s_Announcement = null; s_Title = null; s_Body = null;
		s_Countdown = null; s_CountdownLabel = null; s_CountdownDigits = null;
	}
	protected static bool EnsureWidgets()
	{
		if (s_Root) return true;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace) return false;
		s_Root = workspace.CreateWidgets(LAYOUT);
		if (s_Root)
		{
			s_Root.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
			s_Announcement = s_Root.FindAnyWidget("Announcement");
			s_Title = RichTextWidget.Cast(s_Root.FindAnyWidget("Title"));
			s_Body = RichTextWidget.Cast(s_Root.FindAnyWidget("Body"));
			s_Countdown = s_Root.FindAnyWidget("Countdown");
			s_CountdownLabel = RichTextWidget.Cast(s_Root.FindAnyWidget("CountdownLabel"));
			s_CountdownDigits = RichTextWidget.Cast(s_Root.FindAnyWidget("CountdownDigits"));
		}
		if (!s_Announcement || !s_Title || !s_Body || !s_Countdown || !s_CountdownLabel || !s_CountdownDigits)
		{
			ClearWidgets();
			if (!s_bLayoutWarning) Print("[TB_CC_PRESSURE_UI] layout-or-widget-unavailable resource=TB_CC_VictoryPressure.layout");
			s_bLayoutWarning = true; return false;
		}
		s_Title.SetTextWrapping(true);
		s_Body.SetTextWrapping(true);
		s_CountdownLabel.SetTextWrapping(true);
		s_CountdownDigits.SetTextWrapping(false);
		s_Announcement.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_Countdown.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_Title.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_Body.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_CountdownLabel.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_CountdownDigits.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		s_Root.SetZOrder(100);
		// Scope explicit white to BCM widgets; vanilla notifications are untouched.
		s_Title.SetColor(Color.FromRGBA(255, 255, 255, 255));
		s_Body.SetColor(Color.FromRGBA(255, 255, 255, 255));
		s_CountdownLabel.SetColor(Color.FromRGBA(255, 255, 255, 255));
		s_CountdownDigits.SetColor(Color.FromRGBA(255, 255, 255, 255));
		s_Announcement.SetVisible(false); s_Countdown.SetVisible(false);
		return true;
	}
	protected static void ShowAnnouncement(string title, string body)
	{
		s_Title.SetText(title); s_Body.SetText(body);
		s_iAnnouncementStarted = System.GetTickCount(); s_bAnnouncementActive = true;
		s_Announcement.SetVisible(true); s_Announcement.SetOpacity(0);
	}
	static void Refresh()
	{
		if (RplSession.Mode() == RplMode.Dedicated) return;
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.Cast(GetGame().GetGameMode());
		if (!mode) return;
		bool pressure = !Title(mode.TB_CC_GetPressureOutcome()).IsEmpty() && mode.TB_CC_GetPressureRemaining() >= 0;
		TB_CC_M4XP_NativeArlandComponent policy = SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy();
		bool vulnerable = policy && policy.GetHQState(policy.GetSelectedEndpointId()) == TB_CC_HQLockState.VULNERABLE;
		bool ourVulnerable = policy && policy.GetHQState(policy.GetSelectedMobId()) == TB_CC_HQLockState.VULNERABLE;
		if (mode.TB_CC_GetRestoredPresentation() && !s_bRestoreBaselineApplied)
		{
			// Wait for matching initial native projection; do not announce history
			// merely because independent replicated fields arrive in different order.
			if (!policy || mode.TB_CC_GetPressureOutcome() == TB_PVE_M2_5_CampaignOutcome.NOT_READY || policy.GetHQState(policy.GetSelectedMobId()) != mode.TB_CC_GetRestoredHQEast() || policy.GetHQState(policy.GetSelectedEndpointId()) != mode.TB_CC_GetRestoredHQWest()) return;
			s_bWasPressure = pressure; s_bHQVulnerable = vulnerable; s_bOurHQVulnerable = ourVulnerable;
			s_bRestoreBaselineApplied = true;
		}
		if (pressure && !s_bWasPressure)
		{ s_sPendingTitle = Title(mode.TB_CC_GetPressureOutcome()); s_sPendingBody = Body(mode.TB_CC_GetPressureOutcome()); }
		if (s_bWasPressure && !pressure && !vulnerable && !ourVulnerable && mode.TB_CC_GetPressureOutcome() == TB_PVE_M2_5_CampaignOutcome.ACTIVE)
		{ s_sPendingTitle = "TERRITORIAL CONTROL BROKEN"; s_sPendingBody = "Enemy forces have re-established a foothold."; }
		if (vulnerable && !s_bHQVulnerable)
		{ s_sPendingTitle = "ENEMY HQ VULNERABLE"; s_sPendingBody = "Enemy headquarters is now open to assault."; }
		if (ourVulnerable && !s_bOurHQVulnerable)
		{ s_sPendingTitle = "OUR HQ VULNERABLE"; s_sPendingBody = "Defend our headquarters. Its capture will end the campaign."; }
		s_bWasPressure = pressure; s_bHQVulnerable = vulnerable; s_bOurHQVulnerable = ourVulnerable;
		if (!pressure && s_sPendingTitle.IsEmpty() && !s_bAnnouncementActive && !s_Root) return;
		if (!EnsureWidgets()) return;
		if (!s_sPendingTitle.IsEmpty())
		{ ShowAnnouncement(s_sPendingTitle, s_sPendingBody); s_sPendingTitle = ""; s_sPendingBody = ""; }
		if (s_bAnnouncementActive)
		{
			int elapsed = System.GetTickCount() - s_iAnnouncementStarted;
			int remaining = ANNOUNCEMENT_SECONDS * 1000 - elapsed;
			if (remaining <= 0) { s_bAnnouncementActive = false; s_Announcement.SetVisible(false); }
			else s_Announcement.SetOpacity(Math.Clamp(Math.Min(elapsed / 1000.0, remaining / 1000.0), 0, 1));
		}
		// Notification timing is local presentation only. Pressure time is replicated.
		s_Countdown.SetVisible(pressure && !s_bAnnouncementActive);
		if (!pressure) return;
		string label = "ENEMY HQ VULNERABLE IN";
		if (mode.TB_CC_GetPressureOutcome() == TB_PVE_M2_5_CampaignOutcome.OPFOR_VICTORY_PRESSURE) label = "OUR HQ VULNERABLE IN";
		s_CountdownLabel.SetText(label);
		s_CountdownDigits.SetText(Countdown(mode.TB_CC_GetPressureRemaining()));
	}
}
