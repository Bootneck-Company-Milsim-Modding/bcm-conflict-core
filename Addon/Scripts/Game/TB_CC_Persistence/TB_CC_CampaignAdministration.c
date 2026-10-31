// P5 restart transaction. No live ownership, combat or construction reset.
class TB_CC_CampaignResetRecord
{
	int version = 1;
	string phase;
	string oldCampaignId;
	string newCampaignId;
	string mapIdentity;
	string scenarioIdentity;
	string configIdentity;
	int requestedAt;
	int committedAt;
	int archivedRevision;
	string authoritySource;
	int authoritySession;

	static bool ValidId(string value)
	{
		UUID id = value;
		return UUID.IsUUID(value) && !id.IsNull();
	}
	bool Valid(string expectedMap, string expectedScenario, string expectedConfig, string expectedPhase)
	{
		if (version != 1 || phase != expectedPhase || !ValidId(oldCampaignId) || !ValidId(newCampaignId) || oldCampaignId == newCampaignId) return false;
		if (mapIdentity != expectedMap || scenarioIdentity != expectedScenario || configIdentity != expectedConfig || requestedAt < 1 || archivedRevision < 0) return false;
		if (phase == "PENDING" && committedAt != 0) return false;
		if (phase == "COMMITTED" && committedAt < requestedAt) return false;
		if (authoritySource != "CHAT_ADMIN" && authoritySource != "RCON_ADMIN") return false;
		return (authoritySource == "CHAT_ADMIN" && authoritySession > 0) || (authoritySource == "RCON_ADMIN" && authoritySession == 0);
	}
	bool SameTransaction(TB_CC_CampaignResetRecord other)
	{
		return other && version == other.version && oldCampaignId == other.oldCampaignId && newCampaignId == other.newCampaignId && mapIdentity == other.mapIdentity && scenarioIdentity == other.scenarioIdentity && configIdentity == other.configIdentity && requestedAt == other.requestedAt && archivedRevision == other.archivedRevision && authoritySource == other.authoritySource && authoritySession == other.authoritySession;
	}
	bool Write(string path)
	{
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("version", version) || !context.WriteValue("phase", phase)) return false;
		if (!context.WriteValue("oldCampaignId", oldCampaignId) || !context.WriteValue("newCampaignId", newCampaignId)) return false;
		if (!context.WriteValue("mapIdentity", mapIdentity) || !context.WriteValue("scenarioIdentity", scenarioIdentity) || !context.WriteValue("configIdentity", configIdentity)) return false;
		if (!context.WriteValue("requestedAt", requestedAt) || !context.WriteValue("committedAt", committedAt) || !context.WriteValue("archivedRevision", archivedRevision)) return false;
		if (!context.WriteValue("authoritySource", authoritySource) || !context.WriteValue("authoritySession", authoritySession)) return false;
		return context.SaveToFile(path);
	}
	bool Read(string path)
	{
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path)) return false;
		if (!context.ReadValue("version", version) || !context.ReadValue("phase", phase)) return false;
		if (!context.ReadValue("oldCampaignId", oldCampaignId) || !context.ReadValue("newCampaignId", newCampaignId)) return false;
		if (!context.ReadValue("mapIdentity", mapIdentity) || !context.ReadValue("scenarioIdentity", scenarioIdentity) || !context.ReadValue("configIdentity", configIdentity)) return false;
		if (!context.ReadValue("requestedAt", requestedAt) || !context.ReadValue("committedAt", committedAt) || !context.ReadValue("archivedRevision", archivedRevision)) return false;
		if (!context.ReadValue("authoritySource", authoritySource) || !context.ReadValue("authoritySession", authoritySession)) return false;
		return true;
	}
	string ArchivePath()
	{
		// All filename inputs are validated UUIDs/integers, never user paths.
		string filename = string.Format("%1.%2.%3.save.json", oldCampaignId, requestedAt, newCampaignId);
		return TB_CC_PersistenceService.ROOT + "/archives/" + filename;
	}
}
