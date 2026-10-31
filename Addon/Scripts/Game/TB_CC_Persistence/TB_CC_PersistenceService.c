enum TB_CC_PersistenceState { UNINITIALIZED, RESTORING, READY, FAILED }

// One authority-owned store per profile. Never silently replace foreign saves.
class TB_CC_PersistenceService
{
	static const string ROOT = "$profile:BCMConflictCore/Persistence";
	protected ref TB_CC_CampaignContainer m_Container;
	protected int m_State = TB_CC_PersistenceState.UNINITIALIZED;
	protected bool m_Dirty;
	protected bool m_Saving;
	protected string m_Map;
	protected string m_Scenario;
	protected string m_Config;
	protected int m_CheckpointSeconds = 60;
	protected bool m_IdentityRejected;
	protected bool m_CreatingFreshContainer;
	protected string m_SaveFailure;
	protected ref TB_CC_StrategicPersistenceAdapter m_StrategicAdapter;
	protected ref TB_CC_PlayerProgression m_PlayerProgression;
	bool StartPlayerProgression()
	{
		if (!m_StrategicAdapter || !IsReady()) return false;
		if (m_PlayerProgression) return true;
		m_PlayerProgression = new TB_CC_PlayerProgression();
		if (m_PlayerProgression.Start(this)) return true;
		m_PlayerProgression = null; return false;
	}
	bool ResetPlayersForAdoptedCampaign(string campaignId)
	{
		return m_PlayerProgression && m_PlayerProgression.ResetForAdoptedCampaign(campaignId);
	}
	void BindStrategicAdapter(TB_CC_StrategicPersistenceAdapter adapter) { m_StrategicAdapter = adapter; }
	TB_CC_CampaignContainer GetLoadedContainer() { return m_Container; }

	int GetState() { return m_State; }
	bool IsReady() { return m_State == TB_CC_PersistenceState.READY; }
	bool IsDirty() { return m_Dirty; }
	bool HasStrategicSnapshot()
	{
		if (!m_Container)
		{
			return false;
		}
		return m_Container.snapshotPresent;
	}
	string GetCampaignId()
	{
		if (!m_Container)
		{
			return "";
		}
		return m_Container.campaignId;
	}
	int GetCampaignRevision()
	{
		if (!m_Container)
		{
			return -1;
		}
		return m_Container.campaignRevision;
	}
	protected string Primary() { return ROOT + "/campaign.save.json"; }
	protected string Backup() { return ROOT + "/campaign.backup.save.json"; }
	protected string Temp() { return ROOT + "/campaign.temp.save.json"; }

	protected void State(int state, string reason)
	{
		m_State = state;
		string label = "UNINITIALIZED";
		if (state == TB_CC_PersistenceState.RESTORING) label = "RESTORING";
		if (state == TB_CC_PersistenceState.READY) label = "READY";
		if (state == TB_CC_PersistenceState.FAILED) label = "FAILED";
		PrintFormat("[TB_CC_PERSIST] action=STATE state=%1 reason=%2", label, reason);
	}

	protected TB_CC_CampaignContainer Load(string path, string source)
	{
		TB_CC_CampaignContainer value = new TB_CC_CampaignContainer();
		string problem = "read-or-json-integrity";
		if (value.Read(path)) problem = value.Validate(m_Map, m_Scenario, m_Config);
		if (!problem.IsEmpty())
		{
			if (problem.Contains("mismatch") || problem.Contains("unsupported-schema")) m_IdentityRejected = true;
			PrintFormat("[TB_CC_PERSIST] action=LOAD source=%1 result=INVALID reason=%2", source, problem);
			return null;
		}
		PrintFormat("[TB_CC_PERSIST] action=LOAD source=%1 campaignId=%2 schema=%3 map=%4 result=SUCCESS", source, value.campaignId, value.schemaVersion, value.mapIdentity);
		return value;
	}

	bool Start(string mapIdentity, string scenarioIdentity, string configIdentity, int checkpointSeconds = 60)
	{
		if (!Replication.IsServer() || m_State != TB_CC_PersistenceState.UNINITIALIZED) return false;
		State(TB_CC_PersistenceState.RESTORING, "init");
		if (mapIdentity.IsEmpty() || scenarioIdentity.IsEmpty() || configIdentity.IsEmpty() || checkpointSeconds < 1)
		{ State(TB_CC_PersistenceState.FAILED, "invalid-runtime-identity-or-interval"); return false; }
		m_Map = mapIdentity; m_Scenario = scenarioIdentity; m_Config = configIdentity; m_CheckpointSeconds = checkpointSeconds;
		if (!FileIO.MakeDirectory(ROOT)) { State(TB_CC_PersistenceState.FAILED, "storage-directory"); return false; }
		if (!LoadResetInstruction()) { State(TB_CC_PersistenceState.FAILED, "reset-instruction-recovery"); return false; }
		bool primaryExists = FileIO.FileExists(Primary());
		bool backupExists = FileIO.FileExists(Backup());
		if (m_ResetActivating) { m_Container = CreateResetContainer(); m_Dirty = true; }
		else if (primaryExists) m_Container = Load(Primary(), "PRIMARY");
		if (m_IdentityRejected) { State(TB_CC_PersistenceState.FAILED, "foreign-or-unsupported-primary-preserved"); return false; }
		if (!m_Container && backupExists)
		{
			m_Container = Load(Backup(), "BACKUP");
			if (m_Container) Print("[TB_CC_PERSIST] action=RECOVERY source=BACKUP warning=primary-missing-or-invalid container-only=1");
		}
		if (m_IdentityRejected) { State(TB_CC_PersistenceState.FAILED, "foreign-or-unsupported-backup-preserved"); return false; }
		if (!m_Container && (primaryExists || backupExists || FileIO.FileExists(Temp()) || FileIO.FileExists(ROOT + "/campaign.rotation.save.json")))
		{ State(TB_CC_PersistenceState.FAILED, "existing-files-unrecoverable-preserved"); return false; }
		if (!CheckResetGeneration()) { State(TB_CC_PersistenceState.FAILED, "reset-generation-recovery-mismatch"); return false; }
		if (!m_Container)
		{
			m_Container = new TB_CC_CampaignContainer();
			m_Container.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;
			m_Container.campaignId = UUID.GenV4();
			m_Container.mapIdentity = m_Map; m_Container.scenarioIdentity = m_Scenario; m_Container.campaignConfigIdentity = m_Config;
			m_Container.campaignRevision = 0;
			m_Container.createdTimestamp = System.GetUnixTime(); m_Container.lastSavedTimestamp = m_Container.createdTimestamp;
			m_Dirty = true;
		}
		State(TB_CC_PersistenceState.READY, "container-only-no-gameplay-restore");
		m_CreatingFreshContainer = m_Dirty;
		bool initialSaveFailed = m_Dirty && !m_ResetActivating && !SaveNow("fresh-container");
		m_CreatingFreshContainer = false;
		if (initialSaveFailed) { State(TB_CC_PersistenceState.FAILED, "initial-save"); return false; }
		s_AdministrationStore = this;
		GetGame().GetCallqueue().CallLater(PeriodicCheckpoint, m_CheckpointSeconds * 1000, true);
		return true;
	}

	void MarkDirty(string reason)
	{
		if (!Replication.IsServer() || !IsReady() || m_ResetBlocksWrites) return;
		if (!m_Dirty) PrintFormat("[TB_CC_PERSIST] action=DIRTY reason=%1", reason);
		m_Dirty = true;
	}

	// Foundation revision is independent of live strategy until Phase 2 wiring.
	bool SetCampaignRevision(int revision, string reason)
	{
		if (!Replication.IsServer() || !IsReady() || revision < m_Container.campaignRevision) return false;
		if (revision != m_Container.campaignRevision) { m_Container.campaignRevision = revision; MarkDirty(reason); }
		return true;
	}

	protected bool SameSnapshot(TB_CC_CampaignContainer a, TB_CC_CampaignContainer b)
	{
		return a && b && a.campaignId == b.campaignId && a.campaignRevision == b.campaignRevision && a.createdTimestamp == b.createdTimestamp && a.lastSavedTimestamp == b.lastSavedTimestamp && TB_CC_StrategicPersistence.SamePayload(a, b);
	}

	protected bool Commit()
	{
		m_SaveFailure = "temporary-readback";
		TB_CC_CampaignContainer candidate = Load(Temp(), "TEMP_VALIDATION");
		if (!SameSnapshot(candidate, m_Container)) return false;
		if (m_ResetActivationWriting) return CommitResetRoot(candidate);
		if (FileIO.FileExists(Primary()))
		{
			m_SaveFailure = "primary-or-backup-validation";
			TB_CC_CampaignContainer previous = Load(Primary(), "PREVIOUS_VALIDATION");
			if (m_IdentityRejected) return false;
			if (previous)
			{
				if (previous.campaignId != m_Container.campaignId) return false;
				if (FileIO.FileExists(Backup()))
				{
					Load(Backup(), "EXISTING_BACKUP_CHECK");
					if (m_IdentityRejected) return false;
				}
				// Extra verified safety copy protects existing backup during rotation.
				string recovery = ROOT + "/campaign.rotation.save.json";
				m_SaveFailure = "verified-backup-rotation";
				if (!FileIO.CopyFile(Primary(), recovery)) return false;
				if (!SameSnapshot(previous, Load(recovery, "ROTATION_VALIDATION"))) return false;
				if (!FileIO.CopyFile(recovery, Backup())) return false;
				if (!SameSnapshot(previous, Load(Backup(), "BACKUP_VALIDATION"))) return false;
			}
			else
			{
				// Archive invalid primary intact before repairing it from a loaded backup.
				string rejected = ROOT + "/campaign.rejected." + UUID.GenV4() + ".json";
				m_SaveFailure = "invalid-primary-recovery";
				if (!FileIO.CopyFile(Primary(), rejected)) return false;
				if (!Load(Backup(), "RECOVERY_VALIDATION")) return false;
			}
		}
		else if (!FileIO.FileExists(Backup()))
		{
			m_SaveFailure = "first-backup-establishment";
			// First save also establishes a validated recovery copy before primary.
			if (!FileIO.CopyFile(Temp(), Backup())) return false;
			if (!SameSnapshot(candidate, Load(Backup(), "FIRST_BACKUP_VALIDATION"))) return false;
		}
		else
		{
			m_SaveFailure = "missing-primary-recovery";
			TB_CC_CampaignContainer recovered = Load(Backup(), "MISSING_PRIMARY_RECOVERY");
			if (!recovered || recovered.campaignId != m_Container.campaignId) return false;
		}
		m_SaveFailure = "primary-copy-or-readback";
		if (!FileIO.CopyFile(Temp(), Primary())) return false;
		return SameSnapshot(candidate, Load(Primary(), "COMMIT_VALIDATION"));
	}

	bool SaveNow(string reason)
	{
		if (!Replication.IsServer() || !IsReady() || m_Saving || !m_Dirty || m_ResetBlocksWrites || (m_ResetActivating && !m_ResetActivationWriting)) return false;
		m_Saving = true;
		m_SaveFailure = "";
		TB_CC_CampaignContainer previousContainer = m_Container;
		if (m_StrategicAdapter && !m_CreatingFreshContainer)
		{
			TB_CC_CampaignContainer candidate = new TB_CC_CampaignContainer();
			candidate.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;
			candidate.campaignId = m_Container.campaignId;
			candidate.mapIdentity = m_Map; candidate.scenarioIdentity = m_Scenario; candidate.campaignConfigIdentity = m_Config;
			candidate.createdTimestamp = m_Container.createdTimestamp; candidate.lastSavedTimestamp = m_Container.lastSavedTimestamp;
			if (!m_StrategicAdapter.Capture(candidate))
			{
				m_Saving = false;
				PrintFormat("[TB_CC_PERSIST] action=SAVE result=ERROR section=strategic reason=%1 request=%2 dirty=1 filesUntouched=1", m_StrategicAdapter.GetCaptureError(), reason);
				return false;
			}
			// Capture both live sections in one authority turn; never reuse a stale root.
			if (m_PlayerProgression)
			{
				if (!m_PlayerProgression.Capture(candidate))
				{
					m_Saving = false;
					PrintFormat("[TB_CC_PERSIST] action=SAVE result=ERROR section=players reason=%1 request=%2 dirty=1 filesUntouched=1", m_PlayerProgression.GetCaptureError(), reason);
					return false;
				}
			}
			else foreach (string playerRecord : m_Container.players) candidate.players.Insert(playerRecord);
			m_Container = candidate;
		}
		int previousTimestamp = m_Container.lastSavedTimestamp;
		// Explicit empty-v1 compatibility upgrade; campaign identity is unchanged.
		m_Container.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;
		m_Container.lastSavedTimestamp = Math.Max(previousTimestamp, System.GetUnixTime());
		bool success = false;
		m_SaveFailure = m_Container.Validate(m_Map, m_Scenario, m_Config);
		if (m_SaveFailure.IsEmpty())
		{
			m_SaveFailure = "temporary-write";
			if (m_Container.Write(Temp())) success = Commit();
		}
		if (success) m_Dirty = false;
		else { m_Container = previousContainer; m_Container.lastSavedTimestamp = previousTimestamp; }
		m_Saving = false;
		string result = "ERROR"; if (success) result = "SUCCESS";
		if (!success) PrintFormat("[TB_CC_PERSIST] action=SAVE_FAILURE section=root-or-storage reason=%1 request=%2 dirty=1", m_SaveFailure, reason);
		PrintFormat("[TB_CC_PERSIST] action=SAVE campaignId=%1 revision=%2 reason=%3 result=%4 dirty=%5", m_Container.campaignId, m_Container.campaignRevision, reason, result, m_Dirty);
		return success;
	}

	void PeriodicCheckpoint() { if (m_Dirty) SaveNow("periodic-checkpoint"); }
	void Stop()
	{
		if (s_AdministrationStore == this) s_AdministrationStore = null;
		if (!GetGame()) return;
		GetGame().GetCallqueue().Remove(PeriodicCheckpoint);
		if (m_PlayerProgression) m_PlayerProgression.Stop();
		if (m_ResetBlocksWrites) Print("[TB_CC_ADMIN] action=STOP state=RESET_PENDING_RESTART oldGenerationCheckpointBlocked=1");
		else if (m_Dirty)
		{
			SaveNow("component-shutdown-best-effort");
			if (m_Dirty) Print("[TB_CC_PERSIST] action=SHUTDOWN result=UNSAVED_CHANGES previousRecoverableSaveRetained=1 staleSnapshotFallback=0");
		}
	}

	// P5 administration and restart transaction; uses the same store and writer.
	protected static TB_CC_PersistenceService s_AdministrationStore;
	protected ref TB_CC_CampaignResetRecord m_ResetRecord;
	protected bool m_ResetBlocksWrites;
	protected bool m_ResetActivating;
	protected bool m_ResetActivationWriting;
	protected bool m_ResetCommitted;
	protected string m_ResetError;
	protected ref TB_CC_CampaignContainer m_ConfirmationSnapshot;
	protected string m_ConfirmationActor;
	protected string m_ConfirmationToken;
	protected int m_ConfirmationExpires;
	protected ref map<string, string> m_AdminSaveReplies = new map<string, string>();
	protected ref map<string, bool> m_AdminSaveResults = new map<string, bool>();
	protected ref array<string> m_AdminSaveOrder = {};

	static TB_CC_PersistenceService GetAdministrationStore() { return s_AdministrationStore; }
	bool IsResetActivationPending() { return m_ResetActivating; }
	protected string ResetPrefix() { return ROOT + "/campaign.reset.pending"; }
	protected string ReceiptPrefix() { return ROOT + "/campaign.reset.committed"; }
	protected bool ResetReject(string reason)
	{
		m_ResetError = reason;
		PrintFormat("[TB_CC_ADMIN] action=RESET result=FAILED reason=%1 recoverableFilesPreserved=1", reason);
		return false;
	}
	protected bool ReadResetJournal(string prefix, string phase, out TB_CC_CampaignResetRecord selected)
	{
		selected = null;
		array<string> suffixes = {".prepared.json", ".json", ".backup.json"};
		bool present;
		foreach (string suffix : suffixes)
		{
			string path = prefix + suffix;
			if (!FileIO.FileExists(path)) continue;
			present = true;
			TB_CC_CampaignResetRecord candidate = new TB_CC_CampaignResetRecord();
			if (!candidate.Read(path)) continue; // Another verified copy can recover a torn write.
			if (!candidate.Valid(m_Map, m_Scenario, m_Config, phase)) return ResetReject("reset-journal-version-identity-or-fields");
			if (!selected) { selected = candidate; continue; }
			if (selected.SameTransaction(candidate))
			{
				if (selected.committedAt != candidate.committedAt) return ResetReject("reset-receipt-time-ambiguity");
				continue;
			}
			// A durable newer intent can coexist with previous committed journal copies.
			if (candidate.oldCampaignId == selected.newCampaignId) selected = candidate;
			else if (selected.oldCampaignId != candidate.newCampaignId) return ResetReject("reset-journal-lineage-ambiguity");
		}
		if (present && !selected) return ResetReject("reset-journal-unrecoverable");
		return true;
	}
	protected bool VerifyResetRecord(string path, TB_CC_CampaignResetRecord expected)
	{
		TB_CC_CampaignResetRecord readback = new TB_CC_CampaignResetRecord();
		return readback.Read(path) && readback.Valid(m_Map, m_Scenario, m_Config, expected.phase) && expected.SameTransaction(readback) && expected.committedAt == readback.committedAt;
	}
	protected bool WriteResetJournal(string prefix, TB_CC_CampaignResetRecord record)
	{
		string prepared = prefix + ".prepared.json";
		if (!record.Write(prepared) || !VerifyResetRecord(prepared, record)) return false;
		if (!FileIO.CopyFile(prepared, prefix + ".json") || !VerifyResetRecord(prefix + ".json", record)) return false;
		if (!FileIO.CopyFile(prepared, prefix + ".backup.json")) return false;
		return VerifyResetRecord(prefix + ".backup.json", record);
	}
	protected bool LoadResetInstruction()
	{
		TB_CC_CampaignResetRecord receipt;
		if (!ReadResetJournal(ResetPrefix(), "PENDING", m_ResetRecord) || !ReadResetJournal(ReceiptPrefix(), "COMMITTED", receipt)) return false;
		if (!m_ResetRecord)
		{
			if (receipt) return ResetReject("committed-receipt-without-reset-instruction");
			return true;
		}
		TB_CC_CampaignContainer archive = Load(m_ResetRecord.ArchivePath(), "RESET_ARCHIVE_VALIDATION");
		if (!archive || archive.campaignId != m_ResetRecord.oldCampaignId || archive.campaignRevision != m_ResetRecord.archivedRevision || !archive.snapshotPresent) return ResetReject("reset-archive-missing-invalid-or-generation");
		if (!VerifyResetRecord(m_ResetRecord.ArchivePath() + ".reset.json", m_ResetRecord)) return ResetReject("reset-archive-audit-record");
		if (receipt && receipt.SameTransaction(m_ResetRecord))
		{
			m_ResetCommitted = true;
			return true;
		}
		if (receipt && receipt.newCampaignId != m_ResetRecord.oldCampaignId) return ResetReject("reset-receipt-lineage");
		m_ResetActivating = true;
		Print("[TB_CC_ADMIN] action=RESET state=ACTIVATING_AUTHORED_BASELINE historicalStateLoaded=0");
		return true;
	}
	protected TB_CC_CampaignContainer CreateResetContainer()
	{
		TB_CC_CampaignContainer fresh = new TB_CC_CampaignContainer();
		fresh.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;
		fresh.campaignId = m_ResetRecord.newCampaignId;
		fresh.mapIdentity = m_Map; fresh.scenarioIdentity = m_Scenario; fresh.campaignConfigIdentity = m_Config;
		fresh.createdTimestamp = m_ResetRecord.requestedAt;
		fresh.lastSavedTimestamp = fresh.createdTimestamp; fresh.campaignRevision = 0;
		return fresh; // Empty validated container: normal map adapter builds its baseline.
	}
	protected bool CheckResetGeneration()
	{
		if (!m_ResetCommitted) return true;
		return m_Container && m_Container.campaignId == m_ResetRecord.newCampaignId;
	}
	protected bool CommitResetRoot(TB_CC_CampaignContainer candidate)
	{
		m_SaveFailure = "reset-activation-archive-or-generation";
		TB_CC_CampaignContainer archive = Load(m_ResetRecord.ArchivePath(), "RESET_ARCHIVE_RECHECK");
		if (!archive || archive.campaignId != m_ResetRecord.oldCampaignId || !candidate || candidate.campaignId != m_ResetRecord.newCampaignId || !candidate.snapshotPresent) return false;
		// No rename assumed. Immutable old archive protects every subsequent copy.
		string recovery = ROOT + "/campaign.reset.activation." + m_ResetRecord.newCampaignId + ".json";
		m_SaveFailure = "reset-activation-recovery-copy";
		if (!FileIO.CopyFile(Temp(), recovery) || !SameSnapshot(candidate, Load(recovery, "RESET_NEW_RECOVERY"))) return false;
		m_SaveFailure = "reset-activation-primary-copy";
		if (!FileIO.CopyFile(recovery, Primary()) || !SameSnapshot(candidate, Load(Primary(), "RESET_NEW_PRIMARY"))) return false;
		m_SaveFailure = "reset-activation-backup-copy";
		if (!FileIO.CopyFile(recovery, Backup()) || !SameSnapshot(candidate, Load(Backup(), "RESET_NEW_BACKUP"))) return false;
		m_SaveFailure = "reset-activation-rotation-copy";
		if (!FileIO.CopyFile(recovery, ROOT + "/campaign.rotation.save.json")) return false;
		return SameSnapshot(candidate, Load(ROOT + "/campaign.rotation.save.json", "RESET_NEW_ROTATION"));
	}
	bool CompleteResetActivation()
	{
		if (!Replication.IsServer() || !IsReady() || !m_ResetActivating || m_Saving) return false;
		MarkDirty("new-generation-baseline"); m_ResetActivationWriting = true;
		bool saved = SaveNow("reset-activation"); m_ResetActivationWriting = false;
		if (!saved) { State(TB_CC_PersistenceState.FAILED, "reset-activation-save"); return ResetReject("new-generation-save"); }
		TB_CC_CampaignResetRecord receipt = new TB_CC_CampaignResetRecord();
		if (!receipt.Read(ResetPrefix() + ".prepared.json") || !receipt.SameTransaction(m_ResetRecord))
		{
			// Recovery may have selected a backup after a torn prepared file.
			if (!WriteResetJournal(ResetPrefix(), m_ResetRecord) || !receipt.Read(ResetPrefix() + ".prepared.json"))
			{ State(TB_CC_PersistenceState.FAILED, "reset-instruction-repair"); return ResetReject("pending-journal-repair"); }
		}
		receipt.phase = "COMMITTED"; receipt.committedAt = Math.Max(System.GetUnixTime(), receipt.requestedAt);
		if (!WriteResetJournal(ReceiptPrefix(), receipt))
		{ State(TB_CC_PersistenceState.FAILED, "reset-receipt-write"); return ResetReject("commit-receipt-write"); }
		m_ResetActivating = false; m_ResetCommitted = true;
		PrintFormat("[TB_CC_ADMIN] action=RESET state=ACTIVATED campaignId=%1 revision=%2 archiveRetained=1", GetCampaignId(), GetCampaignRevision());
		return true;
	}
	protected TB_CC_CampaignContainer CaptureAdminPreview()
	{
		if (!Replication.IsServer() || !IsReady() || m_Saving || m_ResetBlocksWrites || m_ResetActivating || !m_StrategicAdapter || !m_PlayerProgression) return null;
		TB_CC_CampaignContainer value = new TB_CC_CampaignContainer();
		value.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;
		value.campaignId = GetCampaignId(); value.mapIdentity = m_Map; value.scenarioIdentity = m_Scenario; value.campaignConfigIdentity = m_Config;
		value.createdTimestamp = m_Container.createdTimestamp; value.lastSavedTimestamp = m_Container.lastSavedTimestamp;
		if (!m_StrategicAdapter.Capture(value) || !m_PlayerProgression.Capture(value)) return null;
		string problem = value.Validate(m_Map, m_Scenario, m_Config);
		if (!problem.IsEmpty()) return null;
		return value;
	}
	protected bool SameResetScope(TB_CC_CampaignContainer a, TB_CC_CampaignContainer b)
	{
		if (!a || !b || a.campaignId != b.campaignId || a.campaignRevision != b.campaignRevision || a.mapIdentity != b.mapIdentity || a.players.Count() != b.players.Count()) return false;
		if (!TB_CC_StrategicPersistence.SameMap(a.positionOwners, b.positionOwners)) return false;
		for (int i = 0; i < a.players.Count(); i++) if (a.players[i] != b.players[i]) return false;
		return true; // Clock/cooldown passage alone does not change destructive scope.
	}
	protected string AdministrationStatus()
	{
		string state = "STORE_READY";
		if (m_ResetActivating) state = "RESET_ACTIVATING";
		if (m_ResetBlocksWrites) state = "RESET_PENDING_RESTART";
		if (!IsReady()) state = "FAILED_ARCHIVE_PRESERVED";
		TB_CC_CampaignContainer current = CaptureAdminPreview();
		bool live = current != null;
		if (!current) current = m_Container;
		int ownedUS; int ownedUSSR; int ownedFIA;
		string ownedIds;
		if (current)
		{
			foreach (string id : current.strategicPositions)
			{
				string owner = current.positionOwners.Get(id);
				if (owner == "US") { ownedUS++; ownedIds += id + ";"; }
				if (owner == "USSR") ownedUSSR++; if (owner == "FIA") ownedFIA++;
			}
		}
		string status = string.Format("BCM Campaign Administration: campaign=%1 map=%2 savedRevision=%3 dirty=%4 state=%5 ownerCountsIncludingHQ(US/USSR/FIA)=%6/%7/%8 liveAvailable=%9", GetCampaignId(), m_Map, GetCampaignRevision(), m_Dirty, state, ownedUS, ownedUSSR, ownedFIA, live);
		if (current) status += string.Format(" observedRevision=%1 US-ownedPositionIDsIncludingHQ=%2", current.campaignRevision, ownedIds);
		return status;
	}
	// Called ONLY from native server-side permission-gated command callbacks.
	string AdministrationRequest(array<string> argv, string source, int session, out bool success)
	{
		success = false;
		if (!Replication.IsServer() || !argv || argv.Count() < 2) return "BCM Campaign Administration: status | save [requestUUID] | new <campaignUUID> <map> | confirm <campaignUUID> <map> <token>";
		if (source != "RCON_ADMIN" && source != "CHAT_ADMIN") return "Denied: unsupported authority source";
		if (source == "CHAT_ADMIN" && (!GetGame().GetPlayerManager() || !SCR_Global.IsAdmin(session))) return "Denied: native administrator role required";
		if (source == "RCON_ADMIN" && session != 0) return "Denied: invalid RCON context";
		string action = argv[1];
		if (action == "status" && argv.Count() == 2) { success = true; return AdministrationStatus(); }
		if (!IsReady() || m_ResetBlocksWrites || m_ResetActivating || m_Saving) return "Reset pending/recovery or store not ready: writes rejected; restart required if reset staged";
		string actor = source + ":" + session.ToString();
		if (action == "save" && (argv.Count() == 2 || argv.Count() == 3))
		{
			string request = UUID.GenV4();
			if (argv.Count() == 3) request = argv[2];
			if (!TB_CC_CampaignResetRecord.ValidId(request)) return "Invalid save request UUID";
			string key = string.Format("%1|%2|%3", actor, GetCampaignId(), request);
			if (m_AdminSaveReplies.Contains(key))
			{ success = m_AdminSaveResults.Get(key); return m_AdminSaveReplies.Get(key); }
			MarkDirty("administrator-checkpoint"); success = SaveNow("administrator-checkpoint");
			string reply = "Save failed; dirty state retained; consult server section/reason diagnostic";
			if (success) reply = "Save succeeded: validated coherent campaign checkpoint";
			m_AdminSaveReplies.Set(key, reply); m_AdminSaveResults.Set(key, success); m_AdminSaveOrder.Insert(key);
			if (m_AdminSaveOrder.Count() > 32)
			{
				string oldest = m_AdminSaveOrder[0]; m_AdminSaveOrder.Remove(0);
				m_AdminSaveReplies.Remove(oldest); m_AdminSaveResults.Remove(oldest);
			}
			return reply;
		}
		if (action == "new" && argv.Count() == 4)
		{
			if (argv[2] != GetCampaignId() || argv[3] != m_Map) return "Stale campaign or wrong map: obtain status first";
			TB_CC_CampaignContainer preview = CaptureAdminPreview();
			if (!preview) return "Reset preview failed: authoritative snapshot unavailable; nothing staged";
			if (m_ConfirmationActor != actor || m_ConfirmationExpires < System.GetUnixTime() || !SameResetScope(m_ConfirmationSnapshot, preview)) m_ConfirmationToken = UUID.GenV4();
			m_ConfirmationActor = actor; m_ConfirmationSnapshot = preview; m_ConfirmationExpires = System.GetUnixTime() + 120;
			success = true;
			return string.Format("WARNING: Starting a new campaign resets all territorial progress and every player's campaign XP/rank. Archive retained; scenario/server restart required. No construction persistence exists. Confirm within 120s: confirm %1 %2 %3", GetCampaignId(), m_Map, m_ConfirmationToken);
		}
		if (action != "confirm" || argv.Count() != 5) return "Invalid BCM campaign command/argument count";
		TB_CC_CampaignContainer confirmation = CaptureAdminPreview();
		if (m_ConfirmationToken.IsEmpty() || actor != m_ConfirmationActor || argv[2] != GetCampaignId() || argv[3] != m_Map || argv[4] != m_ConfirmationToken || System.GetUnixTime() > m_ConfirmationExpires || !SameResetScope(m_ConfirmationSnapshot, confirmation))
		{ m_ConfirmationToken = ""; return "Confirmation expired/stale/invalid: request a new preview; nothing staged"; }
		m_ConfirmationToken = ""; // Consume once, including failure; never replay a destructive request.
		MarkDirty("pre-reset-archive");
		if (!SaveNow("pre-reset-archive")) return "Reset failed: pre-reset coherent checkpoint failed; old campaign preserved";
		TB_CC_CampaignResetRecord record = new TB_CC_CampaignResetRecord();
		record.phase = "PENDING"; record.oldCampaignId = GetCampaignId(); record.newCampaignId = UUID.GenV4();
		record.mapIdentity = m_Map; record.scenarioIdentity = m_Scenario; record.configIdentity = m_Config;
		record.requestedAt = System.GetUnixTime(); record.archivedRevision = GetCampaignRevision();
		record.authoritySource = source; record.authoritySession = session;
		if (!record.Valid(m_Map, m_Scenario, m_Config, "PENDING")) return "Reset failed: invalid transaction metadata";
		m_ResetBlocksWrites = true; // No concurrent/teardown old-generation writer after this point.
		if (!FileIO.MakeDirectory(ROOT + "/archives") || FileIO.FileExists(record.ArchivePath()) || FileIO.FileExists(record.ArchivePath() + ".reset.json") || !FileIO.CopyFile(Primary(), record.ArchivePath()) || !SameSnapshot(m_Container, Load(record.ArchivePath(), "ADMIN_ARCHIVE_READBACK")))
		{ m_ResetBlocksWrites = false; return "Reset failed: archive copy/validation; old campaign preserved; retry with new preview"; }
		if (!record.Write(record.ArchivePath() + ".reset.json") || !VerifyResetRecord(record.ArchivePath() + ".reset.json", record))
		{ m_ResetBlocksWrites = false; return "Reset failed: archive audit metadata; old campaign and archive preserved"; }
		m_ResetRecord = record;
		if (!WriteResetJournal(ResetPrefix(), record))
		{
			ResetReject("pending-journal-write-or-readback");
			return "Reset staging incomplete; old archive preserved. Writes blocked: restart for journal recovery; inspect server diagnostics";
		}
		PrintFormat("[TB_CC_ADMIN] action=RESET state=RESET_PENDING_RESTART source=%1 session=%2 campaignId=%3 archiveValidated=1", source, session, GetCampaignId());
		SCR_GameModeCampaign mode = SCR_GameModeCampaign.GetInstance();
		if (mode) mode.TB_CC_AnnounceCampaignReset();
		success = true;
		return "Reset staged and verified: RESET_PENDING_RESTART. Restart this scenario/server; old-generation saves are blocked";
	}
}
