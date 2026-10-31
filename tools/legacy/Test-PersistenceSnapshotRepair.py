"""Live-save vs restore-precondition regression. Independent models, NOT Enforce/IO."""
import copy
from pathlib import Path
import runpy
import uuid

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-PersistencePlayerProgression.py'))
strategic = prior['prior']
installed, base = prior['installed'], prior['base']
native, service, player = prior['native'], prior['service'], prior['source']
mode = installed(base + 'data', '/SCR_BaseGameMode.c')
assert 'ScriptInvoker GetOnGameEnd()' in mode
end = mode.split('void OnGameEnd()', 1)[1].split('//------------------------------------------------------------------------------------------------', 1)[0]
assert end.index('m_OnGameEnd.Invoke();') < end.index('comp.OnGameEnd();')
assert 'm_GameMode.GetOnGameEnd().Insert(CheckpointBeforeTeardown)' in player
assert 'm_GameMode.GetOnGameEnd().Remove(CheckpointBeforeTeardown)' in player
assert 'm_Store.SaveNow("world-end-before-gamemode-component-cleanup")' in player
assert 'bool forRestore = true' in native
assert 'ValidateStrategicSnapshot(snapshot, false)' in native
assert 'if (forRestore && base.IsBeingCaptured())' in native
restore = native.split('protected bool RestoreStrategicSnapshot(', 1)[1].split('protected void ObservePersistenceState()', 1)[0]
assert 'ValidateStrategicSnapshot(snapshot)' in restore
assert 'ValidateStrategicSnapshot(snapshot, false)' not in restore
save = service.split('bool SaveNow(', 1)[1].split('void PeriodicCheckpoint()', 1)[0]
assert save.index('m_StrategicAdapter.Capture(candidate)') < save.index('m_PlayerProgression.Capture(candidate)') < save.index('m_Container = candidate;') < save.index('m_Container.Write(Temp())')
for section in ('strategic', 'players'):
    assert f'section={section} reason=%1 request=%2 dirty=1 filesUntouched=1' in save
assert 'm_SaveFailure = m_Container.Validate(' in save
assert 'if (success) m_Dirty = false;' in save
assert 'm_Container = previousContainer;' in save
assert 'staleSnapshotFallback=0' in service and 'UNSAVED_CHANGES' in service
for reason in ('registry-unavailable', 'ownership-unavailable:', 'native-base-unavailable:',
               'native-authority-unavailable:', 'native-affiliation-unavailable:',
               'pressure-inconsistent', 'FIA-queue-order-gap', 'US-HQ-owner'):
    assert reason in native, reason
for line in (player + service).splitlines():
    if 'PrintFormat(' in line:
        assert 'identity' not in line.replace('backend-identity-unavailable', '')

class Repair(prior['Model']):
    def __init__(self):
        super().__init__()
        self.strategic = strategic['state']()
        self.capturing = False
        self.dependencies = True
        self.failure = ''

    def checkpoint(self, fail=False, old_guard=False):
        # Committed owner/revision is the persistence boundary. No capture progress saved.
        if not self.dependencies:
            self.failure = 'native-base-unavailable:P0'; return False
        if old_guard and self.capturing:
            self.failure = 'snapshot-incomplete'; return False
        if not strategic['validate'](self.strategic):
            self.failure = 'strategic-invariant'; return False
        self.failure = ''
        return super().checkpoint(fail)

    def may_restore(self):
        return self.dependencies and not self.capturing and strategic['validate'](self.strategic)

def fixture():
    model = Repair()
    model.dirty = True
    assert model.checkpoint()
    return model

accounts = [str(uuid.uuid4()) for _ in range(2)]
covered = set()
model = fixture(); assert not model.primary['players']; covered.add('strategic-only')
before = copy.deepcopy(model.strategic)
assert model.join(1, accounts[0], 150); model.award(1, 25); assert model.checkpoint()
assert model.primary['strategic'] == before and prior['decode'](model.primary['players'])[accounts[0]] == 175
covered.add('player-only')
model.join(2, accounts[1], 900); model.award(2, 75); assert model.checkpoint()
assert prior['decode'](model.primary['players']) == {accounts[0]: 175, accounts[1]: 975}
covered.add('two-accounts')

# Actual old source rejection reproduced: valid committed snapshot + live capture + XP.
model.capturing = True; model.award(1, 5)
files = copy.deepcopy((model.primary, model.backup))
assert not model.checkpoint(old_guard=True) and model.failure == 'snapshot-incomplete'
assert model.dirty and (model.primary, model.backup) == files
assert not model.may_restore()  # Historical ownership restore remains unsafe mid-capture.
assert model.checkpoint() and not model.dirty
assert model.primary['strategic'] == before and prior['decode'](model.primary['players'])[accounts[0]] == 180
covered.update(('combat-XP', 'XP-before-capture', 'failed-dirty-retained', 'success-dirty-cleared'))

# Genuine ownership callback supplies new committed owner/revision/commitment together.
model.capturing = False
model.strategic['owners']['P0'] = 'US'; model.strategic['revision'] += 1
model.strategic['commitment'] = 'P1'; model.award(2, 1); assert model.checkpoint()
assert model.primary['strategic'] == model.strategic
assert prior['decode'](model.primary['players'])[accounts[1]] == 976
covered.update(('concurrent-strategy-XP', 'revision-checkpoint'))
model.award(1, 10); model.leave(1); assert model.checkpoint()
assert prior['decode'](model.primary['players'])[accounts[0]] == 190
covered.add('disconnect-after-XP')
model.award(2, 10); assert model.checkpoint(); model.dependencies = False
model.dirty = True; files = copy.deepcopy((model.primary, model.backup))
assert not model.checkpoint() and model.dirty and files == (model.primary, model.backup)
covered.add('shutdown-after-XP')
model.dependencies = True; model.strategic['owners']['P1'] = 'US'
model.strategic['commitment'] = 'P2'; model.strategic['revision'] += 1
assert model.checkpoint(); model.dependencies = False
model.dirty = True; files = copy.deepcopy((model.primary, model.backup))
assert not model.checkpoint() and files == (model.primary, model.backup)
covered.add('shutdown-after-capture')

model.dependencies = True
for mutate in (lambda s: s['owners'].pop('P9'), lambda s: s.update(active=True),
               lambda s: s['owners'].update(HQ_US='USSR'), lambda s: s.update(revision=-1)):
    good = copy.deepcopy(model.strategic); mutate(model.strategic)
    files = copy.deepcopy((model.primary, model.backup)); model.dirty = True
    assert not model.checkpoint() and model.dirty and files == (model.primary, model.backup)
    model.strategic = good
covered.add('incomplete-corrupt-rejected')
files = copy.deepcopy((model.primary, model.backup))
assert not model.checkpoint(fail=True) and model.dirty and files == (model.primary, model.backup)
assert model.checkpoint()
# Recover ONE prior whole root; no mixing newer XP with older territory.
recovered = copy.deepcopy(model.backup)
assert strategic['validate'](recovered['strategic'])
assert prior['decode'](recovered['players'])
assert recovered['campaignId'] == model.generation
covered.add('primary-backup-recovery')
restarted = prior['Model'](model.generation, prior['decode'](model.primary['players']))
assert restarted.join(41, accounts[0]) and restarted.sessions[41]['xp'] == 190
assert restarted.join(41, accounts[0]) and restarted.awards == 0
covered.add('quiet-native-rank')  # Native threshold/quiet source coverage is in prior P3 suite.
old_generation = model.generation; old_files = copy.deepcopy((model.primary, model.backup))
assert not model.reset(old_generation, old_generation)
new_generation = str(uuid.uuid4()); assert model.reset(new_generation, new_generation)
assert not model.records and model.sessions[2]['xp'] == 0
assert old_files == (model.primary, model.backup)
assert model.checkpoint() and model.primary['campaignId'] == new_generation
assert not model.primary['players']
covered.add('explicit-generation-boundary')
assert len(covered) == 16, covered
print('P3 SNAPSHOT REPAIR SOURCE/INSTALLED API/INDEPENDENT MODEL PASS: 16 required categories')
print('Old live-capture rejection reproduced; same coherent payload accepted for save, not restore.')
print('Prior P1/P2A/P2B/P3 cases retained. No real IO, teardown, rewards or Workbench runtime PASS.')

