"""P3 source/API contracts and independent lifecycle/storage models; not Enforce execution."""
import copy
import json
from pathlib import Path
import re
import runpy
import uuid

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-PersistenceStrategicRestore.py'))
installed = prior['foundation']['installed']
base = prior['foundation']['base']
core = ROOT / 'Scripts/Game/TB_CC_Persistence'
source = (core / 'TB_CC_PlayerProgression.c').read_text()
service = (core / 'TB_CC_PersistenceService.c').read_text()
container = (core / 'TB_CC_CampaignContainer.c').read_text()
native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
xp = installed(base + 'data', '/SCR_PlayerXPHandlerComponent.c')
ranks = installed(base + 'data', '/SCR_CharacterRankComponent.c')
identity = installed(base + 'data', '/SCR_PlayerIdentityUtils.c')
callbacks = installed(base + 'data', '/SCR_BaseGameModeComponent.c')
achievements = installed(base + 'data', '/SCR_DataCollectorAchievementsModule.c')
rank_config = installed(base + 'data', '/BaseMilitaryRanks.conf')
rank_source = installed(base + 'data', '/SCR_RankContainer.c')
handler = installed(base + 'data', '/SCR_XPHandlerComponent.c')
for prototype in (
    'void AddPlayerXP(SCR_EXPRewards rewardID, float multiplier = 1.0, bool volunteer = false, int addDirectly = 0)',
    'void UpdatePlayerRank(bool notify = true)', 'int GetPlayerXP()',
    'OnPlayerXPChanged GetOnPlayerXPChanged()', 'protected int m_iPlayerXP = 0;',
    'SCR_ECharacterRank GetPlayerRankByXP()',
): assert prototype in xp, prototype
for prototype in (
    'void OnPlayerAuditSuccess(int playerId);', 'void OnPlayerAuditFail(int playerId);',
    'void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout);',
): assert prototype in callbacks, prototype
assert 'GetBackendApi().GetPlayerIdentityId(playerId)' in identity
assert 'playerName.Substring' in identity  # Native offline utility is intentionally NOT used.
assert 'SCR_PlayerIdentityUtils.GetPlayerIdentityId(' not in source
assert 'void RpcDoSetCharacterRank(SCR_ECharacterRank newRank, SCR_ECharacterRank prevRank, bool silent)' in ranks
assert 'protected void RankedUp(SCR_ECharacterRank prevRank, SCR_ECharacterRank newRank, IEntity playerEntity, bool silent)' in achievements
assert 'compXP.UpdatePlayerRank(false)' in handler
assert 'reqXP <= XP && reqXP > maxFoundXP' in rank_source
assert 'm_iPlayerXP = xp' in source and 'UpdatePlayerRank(false)' in source
restore = source.split('bool TB_CC_RestoreCampaignXP(int xp)', 1)[1].split('\n}', 1)[0]
assert 'AddPlayerXP(' not in restore and 'RpcDo_OnPlayerXPChanged' not in source
assert 'TB_CC_QuietRankRestore.s_Character == playerEntity' in source
assert 'RplRcver.Server' not in source  # No arbitrary client XP submission endpoint.
assert 'currentXP;' not in source
assert source.index('TB_CC_IsAccountAudited(playerId)') < source.index('GetBackendApi().GetPlayerIdentityId(playerId)')
for line in source.splitlines():
    if 'PrintFormat(' in line: assert 'identity' not in line.replace('backend-identity-unavailable', '')
assert '.Remove(OnXPChanged)' in source and '.Remove(Poll)' in source
assert service.index('m_StrategicAdapter.Capture(candidate)') < service.index('m_PlayerProgression.Capture(candidate)') < service.index('m_Container = candidate;')
assert 'TB_CC_PlayerProgression.Validate(players)' in container
assert native.index('CompletePersistenceDerivedRebuild())') < native.index('StartPlayerProgression()')
assert 'newCampaignId == m_Campaign' in source and 'm_Store.GetCampaignId() != newCampaignId' in source
assert 'SCHEMA_VERSION = 2' in container

def decode(rows):
    records = {}
    for row in rows:
        version, account, text = row.split('|')
        assert version == '1' and account == str(uuid.UUID(account)) and uuid.UUID(account).int
        xp_value = int(text); assert str(xp_value) == text and -(2**31) <= xp_value < 2**31
        assert account not in records
        records[account] = xp_value
    return records

class Model:
    def __init__(self, generation=None, records=None):
        self.generation = generation or str(uuid.uuid4())
        self.records = dict(records or {})
        self.sessions = {}; self.owners = {}; self.dirty = False; self.awards = 0
        self.strategic = {'revision': 47, 'owners': ['FIA', 'US'], 'pressure': 411, 'objective': 'P2'}
        self.primary = None; self.backup = None
    def join(self, session, account, native_xp=0, ready=True, audited=True):
        if not ready or not audited or not account: return False
        if session in self.sessions: return self.sessions[session]['identity'] == account
        if account in self.owners: return False
        self.sessions[session] = {'identity': account, 'xp': self.records.get(account, native_xp)}
        self.owners[account] = session
        if account not in self.records: self.records[account] = native_xp; self.dirty = True
        return True
    def award(self, session, delta):
        player = self.sessions[session]; player['xp'] += delta
        self.records[player['identity']] = player['xp']; self.dirty = True; self.awards += 1
    def leave(self, session):
        if session not in self.sessions: return
        player = self.sessions.pop(session); self.records[player['identity']] = player['xp']
        self.owners.pop(player['identity']); self.dirty = True
    def checkpoint(self, fail=False):
        candidate = {'campaignId': self.generation, 'strategic': copy.deepcopy(self.strategic),
                     'players': [f'1|{key}|{self.records[key]}' for key in sorted(self.records)]}
        assert decode(candidate['players']) == self.records
        if fail: return False
        self.backup = self.primary or copy.deepcopy(candidate)
        self.primary = json.loads(json.dumps(candidate)); self.dirty = False
        return True
    def reset(self, adopted_generation, root_generation=None):
        if adopted_generation == self.generation or root_generation != adopted_generation: return False
        self.generation = adopted_generation; self.records.clear()
        for player in self.sessions.values(): player['xp'] = 0
        self.dirty = True; return True

cases = 0
accounts = [str(uuid.uuid4()) for _ in range(4)]
for xp_value in (0, 1, 499, 500, 9999, -1, -(2**31), 2**31-1):
    model = Model(records={accounts[0]: xp_value})
    before = copy.deepcopy(model.strategic)
    assert model.join(1, accounts[0]) and model.sessions[1]['xp'] == xp_value
    assert model.join(1, accounts[0]) and model.awards == 0
    model.leave(1); assert model.join(27, accounts[0]) and model.sessions[27]['xp'] == xp_value
    assert model.strategic == before; model.checkpoint()
    restarted = Model(model.generation, decode(model.primary['players']))
    assert restarted.join(99, accounts[0]) and restarted.sessions[99]['xp'] == xp_value
    cases += 5
for count in (1, 2, 4):
    model = Model()
    for i in range(count): assert model.join(i+1, accounts[i])
    for i in range(count): model.award(i+1, (i+1)*100)
    for i in reversed(range(count)): model.leave(i+1)
    for i in range(count): assert model.join(i+41, accounts[i]) and model.sessions[i+41]['xp'] == (i+1)*100
    model.checkpoint(); assert len(decode(model.primary['players'])) == count
    cases += 4
for ready, audited, account in ((False,True,accounts[0]), (True,False,accounts[0]), (True,True,'')):
    model = Model(); assert not model.join(1,account,ready=ready,audited=audited) and not model.records
    cases += 1
model = Model(); assert model.join(1,accounts[0]); model.award(1,750); model.checkpoint()
assert not model.join(2,accounts[0]); model.leave(1); assert model.join(2,accounts[0])
model.leave(1); assert model.sessions[2]['xp'] == 750; cases += 3
prior_primary = copy.deepcopy(model.primary); prior_backup = copy.deepcopy(model.backup)
model.award(2,30); assert not model.checkpoint(True) and model.dirty
assert model.primary == prior_primary and model.backup == prior_backup; cases += 2
model.strategic['revision'] += 1; model.strategic['pressure'] -= 1
assert model.checkpoint() and decode(model.primary['players'])[accounts[0]] == 780
assert model.primary['strategic'] == model.strategic; cases += 2
assert not model.reset(model.generation)
new_generation = str(uuid.uuid4()); assert not model.reset(new_generation)
assert model.reset(new_generation, root_generation=new_generation)
assert model.sessions[2]['xp'] == 0 and not model.records
model.award(2,5); assert model.records[accounts[0]] == 5
assert model.join(4,accounts[1]) and model.sessions[4]['xp'] == 0; cases += 5
for bad in ('1|bad|5', f'2|{accounts[0]}|5', f'1|{accounts[0]}|+5', f'1|{accounts[0]}|01', f'1|{accounts[0]}|2147483648', f'1|{uuid.UUID(int=0)}|0'):
    try: decode([bad])
    except (AssertionError, ValueError): cases += 1
    else: raise AssertionError(bad)
assert decode([]) == {}; cases += 1
try: decode([f'1|{accounts[0]}|5']*2)
except AssertionError: cases += 1
else: raise AssertionError('duplicate accepted')
# Controller replacement / respawn / renamed sessions / checkpoint-interleaving.
model = Model(); model.join(1, accounts[0]); model.award(1,900)
saved_awards = model.awards
for cycle in range(8):
    # Native character respawn retains controller; repeated binding must not write XP.
    snapshot = copy.deepcopy(model.sessions[1]); assert model.join(1,accounts[0])
    assert snapshot == model.sessions[1] and model.awards == saved_awards
    # Replacement controller flushes the old one before binding its successor.
    model.leave(1); assert model.join(1,accounts[0]) and model.sessions[1]['xp'] == 900
    assert model.awards == saved_awards
    cases += 2
model.leave(1); assert model.join(1,accounts[1]) and model.sessions[1]['xp'] == 0
assert model.records[accounts[0]] == 900; cases += 1
for checkpoint_order in ('player-first', 'strategic-first'):
    model = Model(); model.join(1,accounts[0]); model.checkpoint()
    if checkpoint_order == 'player-first': model.award(1,25); model.checkpoint()
    model.strategic['revision'] = 999
    if checkpoint_order == 'strategic-first': model.checkpoint(); model.award(1,25)
    model.checkpoint()
    assert model.primary['strategic']['revision'] == 999 and decode(model.primary['players'])[accounts[0]] == 25
    cases += 1
thresholds = []
for block in re.findall(r'SCR_RankInfoCampaign\s+"[^"]+"\s*\{([^}]+)\}', rank_config):
    threshold = int(re.search(r'm_iRequiredXP\s+(-?\d+)', block)[1])
    rank_match = re.search(r'm_iRank\s+(\w+)', block)
    thresholds.append((threshold, rank_match[1] if rank_match else 'PRIVATE'))
assert len(thresholds) == 7
for threshold, expected_rank in thresholds:
    for xp_value in (threshold-1, threshold, threshold+1):
        def derived(value):
            eligible = [(minimum, rank) for minimum, rank in thresholds if minimum <= value]
            return max(eligible)[1] if eligible else 'RENEGADE'
        model = Model(records={accounts[0]: xp_value}); model.join(1, accounts[0])
        assert derived(model.sessions[1]['xp']) == derived(xp_value)
        if xp_value == threshold: assert derived(xp_value) == expected_rank
        before = copy.deepcopy(model.sessions[1]); model.join(1, accounts[0])
        assert model.sessions[1] == before and model.awards == 0
        cases += 1
print('Native default rank thresholds checked from installed resource:', thresholds)
print(f'P3 PLAYER PROGRESSION SOURCE/INSTALLED API/INDEPENDENT MODEL PASS: {cases} cases')
print('Identity unavailable defers; no name fallback. Native XP/rank/RPC/lifecycle runtime gates remain pending.')
print('NOT Workbench compile or dedicated/player/HUD/real IO acceptance. WORKBENCH GATE REQUIRED')
