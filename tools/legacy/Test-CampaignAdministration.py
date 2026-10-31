"""P5 source/API contracts and independent crash/authority models. NOT engine execution."""
import copy
import json
import itertools
import re
from pathlib import Path
import runpy
import uuid

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-PersistenceSnapshotRepair.py'))
installed, base = prior['installed'], prior['base']
source = (ROOT / 'Scripts/Game/TB_CC_Persistence/TB_CC_CampaignAdministration.c').read_text()
command = (ROOT / 'Scripts/Game/TB_CC_Persistence/TB_CC_CampaignAdminCommand.c').read_text()
store = (ROOT / 'Scripts/Game/TB_CC_Persistence/TB_CC_PersistenceService.c').read_text()
source += store
players = (ROOT / 'Scripts/Game/TB_CC_Persistence/TB_CC_PlayerProgression.c').read_text()
native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
sdk = installed(base + 'data', '/ScrServerCommand.c')
roles = installed(base + 'data', '/Functions.c')
ban = installed(base + 'data', '/BanCommands.c')
hint = installed(base + 'data', '/SCR_HintManagerComponent.c')
# Check the WHOLE installed registration API, including ref return qualifiers.
# Compile success does not establish that every engine event is implemented.
api_pattern = re.compile(r'\bevent\s+(?:(protected)\s+)?((?:ref\s+)?\w+)\s+(\w+)\(([^)]*)\);')
required_api = {name: (returns, args) for _, returns, name, args in api_pattern.findall(sdk)}
assert required_api == {
    'GetKeyword': ('string', ''),
    'OnUpdate': ('ref ScrServerCmdResult', ''),
    'OnRCONExecution': ('ref ScrServerCmdResult', 'array<string> argv'),
    'RequiredRCONPermission': ('int', ''),
    'IsServerSide': ('bool', ''),
    'OnChatServerExecution': ('ref ScrServerCmdResult', 'array<string> argv, int playerId'),
    'OnChatClientExecution': ('ref ScrServerCmdResult', 'array<string> argv, int playerId'),
    'RequiredChatPermission': ('int', ''),
}, 'Installed command API changed; review required lifecycle before changing expectations'
assert '//! Update PENDING request.' in sdk

def command_classes(text):
    classes = {}
    for hit in re.finditer(r'\bclass\s+(\w+)\s*:\s*(\w+)\s*\{', text):
        start = hit.end(); end = start; depth = 1
        while depth:
            if text[end] == '{': depth += 1
            elif text[end] == '}': depth -= 1
            end += 1
        body = text[start:end - 1]
        methods = {}
        for method in re.finditer(r'\boverride\s+(?:(protected)\s+)?((?:ref\s+)?\w+)\s+(\w+)\(([^)]*)\)\s*\{', body):
            _, returns, name, args = method.groups()
            assert name not in methods, ('duplicate event', name)
            methods[name] = (returns, args)
        classes[hit[1]] = (hit[2], methods, body)
    return classes

def check_registration(text):
    classes = command_classes(text)
    for concrete in ('TB_CC_CampaignAdminCommand', 'TB_CC_LocalCampaignAdminCommand'):
        for name, expected in required_api.items():
            ancestor = concrete; seen = set()
            while ancestor in classes and name not in classes[ancestor][1]:
                assert ancestor not in seen, 'Command inheritance cycle'
                seen.add(ancestor); ancestor = classes[ancestor][0]
            assert ancestor in classes, (concrete, 'Missing api definition', name)
            assert classes[ancestor][1][name] == expected, (concrete, name, 'API signature mismatch')
        assert 'OnUpdate' in classes[concrete][1], (concrete, 'explicit update lifecycle binding')
    return classes

classes = check_registration(command)
kick = command_classes(ban)['KickCommand']
assert kick[1] == required_api, 'Native synchronous reference must implement the complete same API'
kick_update = kick[2].split('OnUpdate()', 1)[1]
assert 'return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);' in kick_update
assert 'EServerCmdResultType.PENDING' in ban.split('class KickCommand', 1)[0]
ban_update = command_classes(ban)['BanCommand'][2].split('override ref ScrServerCmdResult OnUpdate()', 1)[1]
assert 'switch (m_Callback.m_eState)' in ban_update and 'EServerCmdResultType.PENDING' in ban_update
main_body = classes['TB_CC_CampaignAdminCommand'][2]
local_body = classes['TB_CC_LocalCampaignAdminCommand'][2]
assert main_body.split('OnUpdate()', 1)[1].split('}', 1)[0].strip().endswith('return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);')
assert local_body.split('OnUpdate()', 1)[1].split('}', 1)[0].strip().endswith('return super.OnUpdate();')
assert 'EServerCmdResultType.PENDING' not in command

# Mutation fixtures must detect the original omissions AND the next possible
# registration failures; never run the real commands/reset or perform save IO.
registration_fixtures = 0
for name, (returns, args) in required_api.items():
    declaration = re.compile(r'override\s+' + re.escape(returns) + r'\s+' + name + r'\(' + re.escape(args) + r'\)')
    missing = declaration.sub('/* removed event */ void Missing_' + name + '()', command)
    wrong_return = 'bool' if returns == 'int' else 'int'
    mismatch = declaration.sub('override ' + wrong_return + ' ' + name + '(' + args + ')', command)
    for fixture in (missing, mismatch):
        try: check_registration(fixture)
        except AssertionError: pass
        else: raise AssertionError(('Registration mutation not detected', name))
        registration_fixtures += 1
for fixture in (command.replace('ref ScrServerCmdResult OnUpdate()', 'ScrServerCmdResult OnUpdate()'),
                command.replace('ref ScrServerCmdResult OnUpdate()', 'ref ScrServerCmdResult OnUpdate(int unexpected)')):
    try: check_registration(fixture)
    except AssertionError: pass
    else: raise AssertionError('OnUpdate qualifier/parameter mutation not detected')
    registration_fixtures += 1
assert 'return "bcmcampaign";' in main_body and 'return "bcmcampaignlocal";' in local_body
keywords = []
for path in (ROOT / 'Scripts/Game').rglob('*.c'):
    keywords.extend(re.findall(r'override\s+string\s+GetKeyword\(\)\s*\{\s*return\s+"([^"]+)";', path.read_text()))
assert keywords.count('bcmcampaign') == 1 and keywords.count('bcmcampaignlocal') == 1
assert all(not keyword.startswith('#') for keyword in keywords), 'Native GetKeyword excludes #'
assert 'return true;' in main_body.split('IsServerSide()', 1)[1].split('}', 1)[0]
assert 'return ExecuteAuthorized(argv, "CHAT_ADMIN", playerId);' in main_body
assert 'return ExecuteAuthorized(argv, "RCON_ADMIN", 0);' in main_body
assert 'return super.OnChatServerExecution(argv, playerId);' in local_body
assert 'return ScrServerCmdResult("Use bcmcampaign through authenticated admin RCON", EServerCmdResultType.ERR);' in local_body
assert 'store.AdministrationRequest(argv, source, session, success)' in main_body
assert 'string action = argv[1];' in store
for action, count in (('status', 2), ('save', 2), ('new', 4), ('confirm', 5)):
    assert f'action == "{action}"' in store or f'action != "{action}"' in store
    assert f'argv.Count() == {count}' in store or f'argv.Count() != {count}' in store
print(f'P5 COMMAND REGISTRATION API PASS: 2 classes x {len(required_api)} installed events; {registration_fixtures} rejection fixtures; synchronous native Kick lifecycle; dispatch source contracts')
assert 'return ERCONPermissions.PERMISSIONS_ADMIN;' in ban and 'return ERCONPermissions.PERMISSIONS_ADMIN;' in command
assert 'return EPlayerRole.ADMINISTRATOR;' in command
assert 'return EPlayerRole.SESSION_ADMINISTRATOR;' in command and 'RplMode.Dedicated' in command
assert 'static bool IsAdmin(int playerID)' in roles and 'HasPlayerRole(playerID, EPlayerRole.SESSION_ADMINISTRATOR)' in roles
assert 'SCR_Global.IsAdmin(playerId)' in command and 'SCR_Global.IsAdmin(session)' in source
assert 'static bool ShowCustomHint(string description, string name = string.Empty, float duration = 0' in hint
assert 'RplRcver.Server' not in source + command and 'HasMode(' not in command
assert 'source = argv' not in source + command and 'playerId = argv' not in command

# Installed role-grant API, actual native use, offline boundary and controller
# identity APIs. This is a trusted Workbench console action, NOT an RPC bypass.
player_api = installed(base + 'data', '/PlayerManager.c')
controller_api = installed(base + 'data', '/PlayerController.c')
game_api = installed(base + 'data', '/ChimeraGame.c')
core_game_api = installed(base + 'core', '/Game.c')
session_api = installed(base + 'core', '/RplSession.c')
mode_api = installed(base + 'core', '/RplMode.c')
voting = installed(base + 'data', '/SCR_VotingAdmin.c')
camera = installed(base + 'data', '/SCR_CameraLimitedEditorComponent.c')
for proto in ('proto external bool GivePlayerRole(int iPlayerId, EPlayerRole role);',
              'proto external bool HasPlayerRole(int iPlayerId, EPlayerRole role);',
              'proto external PlayerController GetPlayerController(int iPlayerId);',
              'proto external bool IsPlayerConnected(int iPlayerId);',
              'proto external int GetPlayers(out notnull array<int> outPlayers);'):
    assert proto in player_api
assert 'Can be called only by authority.' in player_api
assert 'GivePlayerRole(winner, EPlayerRole.SESSION_ADMINISTRATOR)' in voting
assert 'proto external int GetPlayerId();' in controller_api
assert 'proto external bool HasRole(EPlayerRole role);' in controller_api
assert 'proto external PlayerController GetPlayerController();' in game_api
assert 'proto external bool InPlayMode();' in core_game_api
assert 'static proto RplMode Mode();' in session_api
assert all(mode in mode_api for mode in ('None', 'Client', 'Listen', 'Dedicated'))
assert 'm_bIsMultiplayer = RplSession.Mode() != RplMode.None;' in camera
assert '#ifdef WORKBENCH' in camera

# Preprocessor exclusion proof for this non-nested Workbench-only block. Retail
# builds (including diagnostic retail) and dedicated builds have no grant helper.
wb_match = re.search(r'#ifdef WORKBENCH\n(.*?)\n#endif', command, re.S)
assert wb_match and '#if' not in wb_match[1]
workbench_helper = wb_match[1]
production_command = command[:wb_match.start()] + command[wb_match.end():]
check_registration(production_command)
assert 'class TB_CC_WorkbenchCampaignAdmin' in workbench_helper
assert 'static bool AuthorizeLocalHost()' in workbench_helper
assert 'static void DiagnoseLocalHost()' in workbench_helper
assert 'TB_CC_WorkbenchCampaignAdmin' not in production_command
assert 'GivePlayerRole(' not in production_command
assert '#define WORKBENCH' not in command, 'Workbench must be an engine build flag, never addon-enabled'
assert 'GivePlayerRole(localId, EPlayerRole.SESSION_ADMINISTRATOR)' in workbench_helper
assert 'GivePlayerRole(localId, EPlayerRole.ADMINISTRATOR)' not in workbench_helper
grant_body = workbench_helper.split('static bool AuthorizeLocalHost()', 1)[1]
for guard in ('!GetGame().InPlayMode()', '!GetGame().GetWorld()', '!Replication.IsServer()',
              'RplSession.Mode() != RplMode.None', '!localController || !playerManager',
              'localId <= 0', 'players.Count() != 1', 'players[0] != localId',
              'playerManager.GetPlayerController(localId) != localController',
              '!playerManager.IsPlayerConnected(localId)'):
    assert grant_body.index(guard) < grant_body.index('GivePlayerRole('), guard
assert 'localController.GetPlayerId()' in grant_body
assert 'localController.HasRole(EPlayerRole.SESSION_ADMINISTRATOR)' in grant_body
assert command.count('AuthorizeLocalHost(') == 1, 'Explicit console call only; no automatic grant'
assert 'RplSession.Mode() != RplMode.None' in local_body
assert '!GetGame().InPlayMode()' in local_body and '!GetGame().GetWorld()' in local_body
assert 'localController.GetPlayerId() != playerId' in local_body
assert 'playerManager.GetPlayerController(playerId) != localController' in local_body
assert 'playerManager.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR)' in local_body
assert local_body.index('RplSession.Mode() != RplMode.None') < local_body.index('super.OnChatServerExecution')
assert 'EPlayerRole.NONE' not in command and 'return 0;' not in command
for forbidden in ('GAME_MASTER', 'HasMode(', 'Rpc(', 'RplRpc', 'argv[', 'FileIO.', 'SaveNow(', 'AdministrationRequest('):
    assert forbidden not in workbench_helper, forbidden
for stage in ('NATIVE_CHAT_REQUIREMENT', 'CHAT_CLIENT_ENTER', 'CHAT_SERVER_ENTER',
              'CHAT_SERVER_CHECK', 'RCON_SERVER_ENTER', 'LOCAL_SERVER_ENTER',
              'LOCAL_SERVER_CHECK', 'LOCAL_RCON_CHECK', 'WB_DIAG', 'WB_GRANT'):
    assert f'stage={stage}' in command
for line in command.splitlines():
    if 'Print' in line:
        # Native queries use the local ID internally; only their boolean result
        # is logged. Never log that ID itself or command arguments/replies.
        logged = re.sub(r'playerManager\.GetPlayerController\(localId\)|playerManager\.HasPlayerRole\(localId, EPlayerRole\.\w+\)', 'nativeQuery', line)
        assert all(secret not in logged for secret in ('argv', 'token', 'identity', 'localId', 'playerId', 'response'))

# Independent native-gate + handler model. Compiling as WORKBENCH never changes
# chat/RCON requirements; offline native session-role bootstrap is separate.
permission_cases = 0
for wb, mode, authority, local, admin, session_admin, gm, rcon_admin in itertools.product(
        (False, True), ('None', 'Client', 'Listen', 'Dedicated'), *( [(False, True)] * 6 )):
    main_chat = admin and authority  # native ADMINISTRATOR gate before SCR_Global
    local_chat = session_admin and authority and mode == 'None' and local
    main_rcon = rcon_admin and authority
    local_rcon = False
    assert main_chat == (admin and authority)
    assert local_chat == (session_admin and authority and local and mode == 'None')
    assert main_rcon == (rcon_admin and authority) and not local_rcon
    if gm and not admin and not session_admin: assert not main_chat and not local_chat
    if mode != 'None': assert not local_chat
    if not local: assert not local_chat
    # No Workbench/GM flag alone substitutes for a real native role.
    if not admin: assert not main_chat
    if not session_admin: assert not local_chat
    permission_cases += 4
bootstrap_cases = 0
for wb, mode, authority, play, world, registered, connected, sole_local, positive_id in itertools.product(
        (False, True), ('None', 'Client', 'Listen', 'Dedicated'), *( [(False, True)] * 7 )):
    allowed = wb and mode == 'None' and authority and play and world and registered and connected and sole_local and positive_id
    assert allowed == all((wb, mode == 'None', authority, play, world, registered, connected, sole_local, positive_id))
    bootstrap_cases += 1
assert bootstrap_cases == 1024
role_completion_cases = 0
for existing, native_success, controller_sees_role in itertools.product((False, True), repeat=3):
    calls = 0
    manager_has_role = existing
    if not existing:
        calls += 1
        manager_has_role = native_success
    verified = manager_has_role and controller_sees_role
    assert calls == int(not existing), 'No repeated native grant for an existing role'
    if not existing and not native_success: assert not verified
    if not controller_sees_role: assert not verified
    if existing and controller_sees_role: assert verified
    role_completion_cases += 1
print(f'P5 AUTHORIZATION SOURCE/INSTALLED API/MODEL PASS: {permission_cases} route cases; {bootstrap_cases} offline bootstrap cases; {role_completion_cases} native role completion/idempotency cases; retail helper excluded; no native role or command executed')
assert 'm_ResetBlocksWrites' in store.split('bool SaveNow(', 1)[1].split('m_Saving = true', 1)[0]
assert store.index('LoadResetInstruction()') < store.index('Load(Primary(), "PRIMARY")')
assert 'm_ResetActivating && !m_ResetActivationWriting' in store
assert 'm_ResetActivationWriting) return CommitResetRoot(candidate)' in store
assert source.index('SaveNow("pre-reset-archive")') < source.index('record.newCampaignId = UUID.GenV4()') < source.index('FileIO.CopyFile(Primary(), record.ArchivePath())') < source.index('WriteResetJournal(ResetPrefix(), record)')
completion = source.split('bool CompleteResetActivation()', 1)[1].split('protected TB_CC_CampaignContainer CaptureAdminPreview()', 1)[0]
assert completion.index('SaveNow("reset-activation")') < completion.index('WriteResetJournal(ReceiptPrefix(), receipt)') < completion.index('m_ResetActivating = false')
startup = native.split('override protected void InitializeAuthority()', 1)[1].split('protected void ReportFullNativeState()', 1)[0]
assert startup.index('StartPlayerProgression()') < startup.index('CompleteResetActivation()') < startup.index('StartM3Consumers()')
consumer_start = native.split('override protected void StartM3Consumers()', 1)[1].split('super.StartM3Consumers()', 1)[0]
assert consumer_start.index('IsResetActivationPending()') < consumer_start.index('TB_CC_WarSimulation.Initialize(')
assert 'm_ResetNewGeneration = store.IsResetActivationPending()' in players
assert '!m_XP.Contains(identity) && m_ResetNewGeneration' in players
assert 'component.TB_CC_RestoreCampaignXP(0)' in players
assert 'FileIO.Delete' not in source and 'FileIO.Move' not in source
for path in (ROOT / 'Scripts/Game/TB_CC_Persistence').glob('*.c'):
    assert 'ARLAND::' not in path.read_text() and 'Arland' not in path.read_text()
for name, body in prior['prior']['prior']['foundation']['nonvoid_methods'](source):
    assert prior['prior']['prior']['foundation']['return_ready'](body), name

state = prior['strategic']['state']
validate_state = prior['strategic']['validate']
def uid(): return str(uuid.uuid4())
def root(campaign=None):
    return dict(campaign=campaign or uid(), map='MAP_A', scenario='SCENARIO', config='CONFIG',
                state=state(), players={uid(): 900, uid(): 150}, schema=2)
def valid_root(value, map_id='MAP_A'):
    return value and value['schema'] == 2 and value['map'] == map_id and validate_state(value['state'])
def same_scope(a, b):
    return (a['campaign'], a['map'], a['state']['revision'], a['state']['owners'], a['players']) == (b['campaign'], b['map'], b['state']['revision'], b['state']['owners'], b['players'])
def same_transaction(a, b):
    return {k: v for k, v in a.items() if k not in ('phase', 'committed')} == {k: v for k, v in b.items() if k not in ('phase', 'committed')}

class IOFault(Exception): pass
class Disk:
    def __init__(self, files=None, fail_at=None):
        self.files = copy.deepcopy(files or {}); self.steps = 0; self.fail_at = fail_at
    def gate(self):
        self.steps += 1
        if self.steps == self.fail_at: raise IOFault()
    def write(self, path, value):
        self.gate(); self.files[path] = json.dumps(value, sort_keys=True)
    def read(self, path):
        self.gate()
        try: return json.loads(self.files[path])
        except (KeyError, ValueError): return None
    def copy(self, src, dst):
        self.gate(); self.files[dst] = self.files[src]

class Campaign:
    def __init__(self, disk, live, map_id='MAP_A'):
        self.disk = disk; self.live = copy.deepcopy(live); self.map = map_id
        self.blocked = False; self.dirty = False; self.challenge = None; self.receipt = None
        self.last = None; self.side_effects = []; self.phase = 'ACTIVE'
    def authorize(self, channel, native_role, server=True, forged=None, dedicated=False, mode='None', local=True):
        # Native RCON callback is already engine gated; forged values never participate.
        return server and ((channel == 'CHAT' and native_role == 'ADMIN') or
                           (channel == 'LOCAL_CHAT' and native_role == 'LOCAL_SESSION_ADMIN' and not dedicated and mode == 'None' and local) or
                           (channel == 'RCON' and native_role == 'RCON_ADMIN'))
    def save(self, actor='A', request=None):
        if self.blocked: return False
        if request and self.last and self.last[:2] == (actor, request): return self.last[2]
        self.dirty = True
        if not valid_root(self.live, self.map): return False
        try:
            self.disk.write('temp', self.live); assert self.disk.read('temp') == self.live
            old = self.disk.read('primary')
            if valid_root(old, self.map): self.disk.write('backup', old)
            self.disk.copy('temp', 'primary'); assert self.disk.read('primary') == self.live
        except IOFault: return False
        self.dirty = False; self.last = (actor, request, True); return True
    def preview(self, actor, now=1000):
        assert not self.blocked and valid_root(self.live, self.map)
        self.challenge = (actor, uid(), now+120, copy.deepcopy(self.live))
        return self.challenge[1]
    def journal(self, prefix, value):
        self.disk.write(prefix+'-prepared', value); assert self.disk.read(prefix+'-prepared') == value
        for dest in (prefix, prefix+'-backup'):
            self.disk.copy(prefix+'-prepared', dest); assert self.disk.read(dest) == value
    def confirm(self, actor, token, now=1001):
        if self.blocked or not self.challenge: return False
        expected = self.challenge; self.challenge = None
        if actor != expected[0] or token != expected[1] or now > expected[2] or not same_scope(expected[3], self.live): return False
        if not self.save(): return False
        record = dict(old=self.live['campaign'], new=uid(), map=self.map, phase='PENDING', at=now,
                      revision=self.live['state']['revision'], actor=actor, committed=0)
        archive = 'archive:'+record['new']; self.blocked = True
        try:
            self.disk.copy('primary', archive); assert self.disk.read(archive) == self.live
            self.disk.write(archive+':audit', record); assert self.disk.read(archive+':audit') == record
        except IOFault:
            self.blocked = False; return False
        try: self.journal('pending', record)
        except IOFault:
            self.phase = 'RECOVERY'; return False
        self.phase = 'RESET_PENDING_RESTART'; return True
    def select(self, prefix, phase):
        candidates = []
        for path in (prefix+'-prepared', prefix, prefix+'-backup'):
            if path not in self.disk.files: continue
            value = self.disk.read(path)
            if value:
                assert value['map'] == self.map and value['phase'] == phase
                candidates.append(value)
        if not candidates:
            assert not any(path in self.disk.files for path in (prefix+'-prepared', prefix, prefix+'-backup'))
            return None
        selected = candidates[0]
        for value in candidates[1:]:
            if value == selected: continue
            if value['old'] == selected['new']: selected = value
            else: assert selected['old'] == value['new']
        return selected
    def startup(self):
        try:
            pending = self.select('pending', 'PENDING'); receipt = self.select('receipt', 'COMMITTED')
            if not pending:
                assert not receipt
                self.live = self.disk.read('primary')
                if not valid_root(self.live, self.map): self.live = self.disk.read('backup')
                assert valid_root(self.live, self.map)
                return True
            archive = self.disk.read('archive:'+pending['new'])
            assert valid_root(archive, self.map) and archive['campaign'] == pending['old']
            assert archive['state']['revision'] == pending['revision']
            assert self.disk.read('archive:'+pending['new']+':audit') == pending
            if receipt and same_transaction(receipt, pending):
                self.live = self.disk.read('primary')
                if not valid_root(self.live, self.map): self.live = self.disk.read('backup')
                assert valid_root(self.live, self.map) and self.live['campaign'] == pending['new']
                return True
            assert not receipt or receipt['new'] == pending['old']
            # Mirrors ordinary authored startup, NOT restoration of old/native historical events.
            self.live = root(pending['new']); self.live['map'] = self.map
            self.live['players'] = {}; self.live['state']['revision'] = 1
            self.disk.write('new-recovery', self.live); assert self.disk.read('new-recovery') == self.live
            for dest in ('primary', 'backup', 'rotation'):
                self.disk.copy('new-recovery', dest); assert self.disk.read(dest) == self.live
            receipt = dict(pending, phase='COMMITTED', committed=pending['at']+1)
            self.journal('receipt', receipt)
            self.phase = 'ACTIVATED'; return True
        except (AssertionError, KeyError, TypeError, IOFault):
            self.phase = 'FAILED'; return False

covered = set(); cases = 0
old = root(); old['state']['owners']['P0'] = 'US'; old['state']['revision'] = 31
files = {'primary': json.dumps(old), 'backup': json.dumps(old)}
session = Campaign(Disk(files), old)
assert session.authorize('CHAT', 'ADMIN') and session.authorize('LOCAL_CHAT', 'LOCAL_SESSION_ADMIN')
assert not session.authorize('LOCAL_CHAT', 'LOCAL_SESSION_ADMIN', dedicated=True)
assert session.authorize('RCON', 'RCON_ADMIN')
for role in ('PLAYER', 'GM', 'EDITOR', 'LISTED_NOT_LOGGED_IN', 'RCON_MONITOR'):
    assert not session.authorize('CHAT', role, forged={'admin': True, 'id': 1})
    assert not session.authorize('RCON', role, forged={'admin': True})
    cases += 2
assert not session.authorize('CHAT', 'ADMIN', server=False)
covered.update(('authorized-checkpoint', 'non-admin-denial', 'forged-denial'))
request = uid()
assert session.save(request=request); steps = session.disk.steps
assert session.save(request=request) and session.disk.steps == steps
for change in ('revision', 'players', 'owners', 'actor', 'expired', 'token'):
    trial = Campaign(Disk(files), old); token = trial.preview('A')
    actor, now = 'A', 1001
    if change == 'revision': trial.live['state']['revision'] += 1
    if change == 'players': trial.live['players'][next(iter(trial.live['players']))] += 1
    if change == 'owners': trial.live['state']['owners']['P1'] = 'US'
    if change == 'actor': actor = 'B'
    if change == 'expired': now = 1200
    if change == 'token': token = uid()
    assert not trial.confirm(actor, token, now) and trial.disk.files == files
    cases += 1
covered.add('stale-confirmation')
token = session.preview('A'); assert session.confirm('A', token)
archive = session.disk.read('archive:'+session.disk.read('pending')['new'])
assert archive == old and not session.confirm('A', token) and not session.save()
covered.update(('two-step', 'duplicate-confirmation', 'archive', 'pending-durability', 'old-generation-checkpoint'))
staged = copy.deepcopy(session.disk.files)
next_session = Campaign(Disk(staged), old); assert next_session.startup()
fresh = next_session.live; assert fresh['campaign'] != old['campaign']
assert fresh['state']['owners'] == state()['owners'] and fresh['state']['commitment'] == 'P0'
assert not fresh['players'] and fresh['state']['revision'] == 1
assert fresh['state']['failures'] == state()['failures'] and not fresh['state']['pending']
assert not fresh['state']['active'] and fresh['state']['east'] == fresh['state']['west'] == 0
assert not next_session.side_effects
covered.update(('new-UUID', 'authored-ownership', 'initial-objective', 'XP-rank-reset', 'FIA-USSR-reset', 'pressure-reset'))
after = copy.deepcopy(next_session.disk.files)
restart = Campaign(Disk(after), old); assert restart.startup() and restart.live == fresh
covered.update(('idempotent-activation', 'normal-restart', 'no-radio-spawn-regression'))
# Second explicit reset forms a strict lineage; old journal copies remain during a crash.
token = restart.preview('A'); assert restart.confirm('A', token)
assert restart.disk.read('pending')['old'] == fresh['campaign']
assert Campaign(Disk(restart.disk.files), fresh).startup(); cases += 1

# Fault at every staged copy/write/readback, then inspect recoverability after process restart.
for fail in range(1, 30):
    trial = Campaign(Disk(files), old); token = trial.preview('A')
    trial.disk.fail_at = fail; trial.confirm('A', token)
    trial.disk.fail_at = None
    assert any(valid_root(json.loads(raw)) and json.loads(raw)['campaign'] == old['campaign']
               for name, raw in trial.disk.files.items() if name in ('primary', 'backup') or name.startswith('archive:') and not name.endswith(':audit'))
    resumed = Campaign(Disk(trial.disk.files), old)
    if resumed.startup():
        assert resumed.live['campaign'] == old['campaign'] or not resumed.live['players']
    cases += 1
covered.update(('failed-archive', 'crash-before-activation', 'recovery-compatibility'))
for fail in range(1, 40):
    resumed = Campaign(Disk(staged, fail), old); resumed.startup()
    retried = Campaign(Disk(resumed.disk.files), old)
    assert retried.startup() and retried.live['campaign'] == fresh['campaign'] and not retried.live['players']
    cases += 1
covered.add('crash-during-after-activation')
for corrupt in ('primary', 'backup', 'pending', 'pending-prepared', 'pending-backup'):
    trial_files = dict(staged); trial_files[corrupt] = 'partial-json'
    assert Campaign(Disk(trial_files), old).startup(); cases += 1
trial_files = dict(staged)
for key in ('pending', 'pending-prepared', 'pending-backup'): trial_files[key] = 'partial-json'
assert not Campaign(Disk(trial_files), old).startup(); cases += 1
assert not Campaign(Disk(staged), old, 'OTHER_MAP').startup(); covered.add('wrong-map')
trial_files = dict(after); trial_files['primary'] = json.dumps(old); trial_files['backup'] = json.dumps(old)
assert not Campaign(Disk(trial_files), old).startup(); cases += 1
for outcome in (2, 4):
    terminal = root(); terminal['state']['outcome'] = outcome
    if outcome == 2:
        terminal['state'].update(active=True, remaining=450, starts=1, west=1)
        for key in terminal['state']['owners']:
            if key.startswith('P'): terminal['state']['owners'][key] = 'US'
    else: terminal['state']['west'] = 3; terminal['state']['owners']['HQ_USSR'] = 'US'
    assert valid_root(terminal)
    trial = Campaign(Disk({'primary': json.dumps(terminal), 'backup': json.dumps(terminal)}), terminal)
    assert trial.confirm('A', trial.preview('A'))
    resumed = Campaign(Disk(trial.disk.files), terminal); assert resumed.startup()
    assert resumed.live['state']['outcome'] == 1 and not resumed.live['state']['active']; cases += 1
covered.add('terminal-reset')
future = root(); future['map'] = 'FUTURE_MAP'
trial = Campaign(Disk({'primary': json.dumps(future), 'backup': json.dumps(future)}), future, 'FUTURE_MAP')
assert trial.confirm('A', trial.preview('A'))
resumed = Campaign(Disk(trial.disk.files), future, 'FUTURE_MAP')
assert resumed.startup() and resumed.live['map'] == 'FUTURE_MAP'
cases += 1
assert len(covered) == 24, covered
print(f'P5 SOURCE/INSTALLED API/INDEPENDENT MODEL PASS: {len(covered)} categories; {cases} authority/stale/crash/recovery cases')
print('No real save files, roles, RCON/UI, native baseline/XP/RPC, filesystem durability or Linux/AMP runtime exercised.')
print('P3.5/P4 unchanged and unimplemented. WORKBENCH GATE REQUIRED; DEDICATED LINUX/AMP GATE REQUIRED')
