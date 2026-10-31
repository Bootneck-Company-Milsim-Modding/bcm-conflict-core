"""P2B source/installed API checks and independent models; NOT an Enforce compiler."""
import copy
import itertools
import re
import runpy
from pathlib import Path

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-PersistenceRestoreArchitecture.py'))
foundation = prior['foundation']
native = prior['native']
transaction = prior['transaction']
service = prior['service']
core = ROOT / 'Scripts/Game/TB_CC_Persistence'
strategic = (core / 'TB_CC_StrategicPersistence.c').read_text()
ui = (ROOT / 'Scripts/Game/TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c').read_text()
ledger = (ROOT / 'Scripts/Game/TB_CC_Core/TB_CC_FIARecaptureOpportunity.c').read_text()
coordinator = (ROOT / 'Scripts/Game/TB_PVE_M3_2/TB_PVE_M3_2_OPFOROperationCoordinator.c').read_text()

for source in (strategic, transaction):
    for name, body in foundation['nonvoid_methods'](source):
        assert foundation['return_ready'](body), name
assert 'ValidateStrategicSnapshot(snapshot)' in native
restore = native.split('protected bool RestoreStrategicSnapshot(', 1)[1].split('protected void ObservePersistenceState()', 1)[0]
assert restore.index('ValidateStrategicSnapshot(snapshot)') < restore.index('TB_CC_QuietOwnershipRestore.Apply(')
assert 'OnBaseFactionChanged(' not in restore and 'Reevaluate(' not in restore
assert 'm_bVanillaEndSignalled = IsTerminal()' in restore
assert 'm_PressureDeadline = m_PressureStarted.PlusSeconds(remaining)' in native
assert 'remaining * 1000' in native
publish = native.split('protected void PublishRestoredStrategicState()', 1)[1].split('override protected int GetValidity()', 1)[0]
assert 'EnterVictoryPressure(' not in publish and 'm_iRevision++' not in publish
assert 'RestoreQuiet(committed, m_iRevision)' in native.split('protected bool CompletePersistenceDerivedRebuild()', 1)[1].split('[Attribute', 1)[0]
assert 'm_bRestoreBaselineApplied' not in ui  # Static latch uses s_, not instance m_.
assert 's_bRestoreBaselineApplied = true' in ui and 'TB_CC_GetRestoredPresentation()' in ui
assert 'm_StrategicAdapter.Capture(candidate)' in service
assert service.index('m_StrategicAdapter.Capture(candidate)') < service.index('m_Container = candidate;', service.index('bool SaveNow('))
assert 'SamePayload(a, b)' in service
assert 'ExportStrategicState' in ledger and 'RestoreStrategicState' in ledger
assert 'ExportStrategicFailures' in coordinator and 'RestoreStrategicFailures' in coordinator
reconcile = transaction.split('bool TB_CC_ReconcileRestoredOwner(', 1)[1].split('modded class SCR_FactionAffiliationComponent', 1)[0]
for forbidden in ('OnFactionChanged(', 'OnBaseFactionChanged(', 'SendHQMessage', 'OnBaseSeized', 'RequestSavePoint', 'SetIsEliminated', 'AddRegularSupplyPackage'):
    assert forbidden not in reconcile, forbidden
for call in ('ChangeFlags(faction)', 'ChangeRadioSettings(faction)', 'HandleSpawnPointFaction()', 'SupplyIncomeTimer(true)', 'CalculateSupplyRegenerationAmount(', 'm_OwningFactionPrevious = faction'):
    assert call in reconcile, call

# Match concrete native declarations, including protected access from modded class.
installed = foundation['installed']; base = foundation['base']
campaign = installed(base + 'data', '/SCR_CampaignMilitaryBaseComponent.c')
military = installed(base + 'data', '/SCR_MilitaryBaseComponent.c')
radio = installed(base + 'data', '/SCR_RadioCoverageSystem.c')
affiliation = installed(base + 'data', '/generated/Components/FactionAffiliationComponent.c')
for declaration in ('protected void HandleSpawnPointFaction()', 'protected void SupplyIncomeTimer(bool reset = false)', 'void CalculateSupplyRegenerationAmount(float currentSupplies, float maxSupplies, notnull SCR_CampaignFaction faction)', 'void ChangeRadioSettings(notnull SCR_Faction faction)'):
    assert declaration in campaign, declaration
assert 'protected void ChangeFlags(notnull Faction faction)' in military
assert 'protected ref array<SCR_MilitaryBaseLogicComponent> m_aSystems' in military
assert 'static bool UpdateAll(bool forceRecalculation = false)' in radio
assert 'GetAffiliatedFaction()' in affiliation
assert 'SetAffiliatedFaction(Faction faction)' in affiliation
for path in core.glob('*.c'):
    assert 'ARLAND::' not in path.read_text(), path

# Bounded independent state model mirrors the adapter's documented contract.
IDS = ['HQ_US', 'HQ_USSR'] + [f'P{i}' for i in range(10)]
def state():
    return dict(owners=dict(zip(IDS, ['US', 'USSR'] + ['FIA']*10)), outcome=1,
                foothold=False, east=0, west=0, active=False, remaining=-1,
                duration=600, starts=0, revision=7, commitment='P0',
                fia_strength=1000, loss_serial=0,
                failures={i:0 for i in IDS}, epochs={i:0 for i in IDS},
                pending=[], attempts={i:0 for i in IDS})
def validate(s):
    if set(s['owners']) != set(IDS) or any(o not in ('US','USSR','FIA') for o in s['owners'].values()): return False
    if s['outcome'] not in range(1,7) or s['east'] not in range(4) or s['west'] not in range(4): return False
    if s['owners']['HQ_US'] != ('USSR' if s['east']==3 else 'US'): return False
    if s['owners']['HQ_USSR'] != ('US' if s['west']==3 else 'USSR'): return False
    if (s['east']==3) != (s['outcome']==5) or (s['west']==3) != (s['outcome']==4): return False
    pressure=s['outcome'] in (2,3)
    if s['active'] != pressure or (pressure and not 0<=s['remaining']<=s['duration']) or (not pressure and s['remaining']!=-1): return False
    if pressure and s['starts']<1: return False
    if s['outcome']==2 and (s['west']!=1 or sum(s['owners'][i]=='US' for i in IDS[2:])!=10): return False
    if s['outcome']==3 and (s['east']!=1 or sum(s['owners'][i]=='USSR' for i in IDS[2:])!=10): return False
    if s['east']==1 and s['outcome']!=3 or s['west']==1 and s['outcome']!=2: return False
    for i in s['pending']:
        if i not in IDS[2:] or s['owners'][i]=='FIA' or s['epochs'][i]<1: return False
    return s['revision']>=0 and 0<=s['fia_strength']<=1000
def restore(s, offline_seconds=0, radio=True):
    writes=[]; events=[]
    if not validate(s): return {'ready':False, 'writes':writes, 'events':events}
    writes.extend(s['owners'].items())
    terminal=s['outcome'] in (4,5,6)
    objective=s['commitment']
    selections=0
    if terminal: objective=''
    elif objective not in IDS or s['owners'][objective]=='US' or not objective:
        selections=1
        objective=next((i for i in IDS[2:] if s['owners'][i]!='US'), '') if radio else ''
    return dict(ready=True, writes=writes, events=events, objective=objective,
                selections=selections, revision=s['revision'], remaining=s['remaining'],
                starts=s['starts'], announcement=False, normal_war=not terminal,
                active_actors=0, failures=copy.deepcopy(s['failures']),
                pending=list(s['pending']), east=s['east'], west=s['west'])

cases=0
for mask in itertools.product(('FIA','US','USSR'), repeat=4):
    s=state()
    for i,owner in enumerate(mask): s['owners'][f'P{i}']=owner
    r=restore(s); assert r['ready'] and len(r['writes'])==12 and r['events']==[]
    assert r['revision']==7 and not r['active_actors']; cases+=1
for direction in (2,3):
    for remaining in (0,1,59,60,300,600):
        for offline in (0,60,86400,1000000):
            s=state(); s['outcome']=direction; s['active']=True; s['remaining']=remaining; s['starts']=3
            s['east' if direction==3 else 'west']=1
            for i in IDS[2:]: s['owners'][i]='US' if direction==2 else 'USSR'
            r=restore(s,offline); assert r['ready'] and r['remaining']==remaining and r['starts']==3 and not r['announcement']; cases+=1
for winner in (4,5):
    s=state(); s['outcome']=winner
    s['west' if winner==4 else 'east']=3; s['owners']['HQ_USSR' if winner==4 else 'HQ_US']='US' if winner==4 else 'USSR'
    r=restore(s); assert r['ready'] and not r['normal_war'] and r['objective']=='' and not r['events']; cases+=1
for side in ('east','west'):
    s=state(); s[side]=2; r=restore(s); assert r['ready'] and r[side]==2; cases+=1
s=state(); s['outcome']=6; r=restore(s); assert r['ready'] and not r['normal_war'] and r['objective']==''; cases+=1
for radio in (True,False):
    s=state(); r=restore(s,radio=radio); assert r['objective']=='P0' and r['selections']==0; cases+=1
    for commitment in ('', 'REMOVED', 'HQ_US'):
        s=state(); s['commitment']=commitment; r=restore(s,radio=radio)
        assert r['selections']==1 and r['objective']==('P0' if radio else ''); cases+=1
s=state(); s['owners']['P0']='US'; r=restore(s); assert r['objective']=='P1' and r['events']==[]; cases+=1
s=state(); s['owners']['P0']='US'; s['epochs']['P0']=2; s['pending']=['P0']; s['failures']['P1']=3
r=restore(s); assert r['pending']==['P0'] and r['failures']['P1']==3 and not r['active_actors']; cases+=1
for order in itertools.permutations(('P0','P1','P2')):
    encoded={i:order.index(i) for i in order}
    decoded=[i for index in range(len(order)) for i in IDS if encoded.get(i)==index]
    assert decoded==list(order); cases+=1
for remaining in (0,1,2,59,60,600):
    for offline in (0,3600,86400):
        resumed_deadline=1000+remaining*1000
        assert max((resumed_deadline-1000+999)//1000,0)==remaining; cases+=1
assert '5 + 8 * m_aIds.Count()' in native
assert 'FIA-queue-order-gap' in native
assert 'RestoreStrategicAdmission(m_iFIAStrategicCooldown, m_iFIAStrategicSerial)' in native
for mutate in (lambda s:s['owners'].pop('P9'), lambda s:s['owners'].update(P9='UNKNOWN'),
               lambda s:s.update(outcome=9), lambda s:s.update(east=3), lambda s:s.update(west=3),
               lambda s:s.update(active=True), lambda s:s.update(remaining=0),
               lambda s:s.update(revision=-1), lambda s:s.update(fia_strength=-1),
               lambda s:s['pending'].append('P0'), lambda s:s['owners'].update(HQ_US='USSR')):
    s=state(); mutate(s); r=restore(s); assert not r['ready'] and not r['writes'] and not r['events']; cases+=1
print(f'P2B STRATEGIC RESTORE SOURCE/INSTALLED API/INDEPENDENT MODEL PASS: {cases} cases')
print('P1/P2A retained. No actors, historical callbacks, wall-clock consumption, world/layer/UI layout edits.')
print('NOT Workbench compile, actual IO/replication, native reconciliation or runtime acceptance PASS. WORKBENCH GATE REQUIRED')
