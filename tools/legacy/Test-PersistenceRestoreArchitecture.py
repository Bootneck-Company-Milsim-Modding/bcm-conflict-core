"""P2A source/API and independent architecture models. No engine execution."""
import copy
import json
import runpy
from pathlib import Path

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
foundation = runpy.run_path(str(TOOLS / 'Test-PersistenceFoundation.py'))
core = ROOT/'Scripts/Game/TB_CC_Persistence'
container = (core/'TB_CC_CampaignContainer.c').read_text()
service = (core/'TB_CC_PersistenceService.c').read_text()
transaction = (core/'TB_CC_RestoreTransaction.c').read_text()
native = (ROOT/'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
assert 'SCHEMA_VERSION = 2' in container
assert 'schemaVersion != 1 && schemaVersion != SCHEMA_VERSION' in container
for field in ('snapshotPresent','positionOwners'):
    assert f'WriteValue("{field}"' in container and f'ReadValue("{field}"' in container
assert 'schemaVersion == 1 || !snapshotPresent' in container
assert 'm_Container.schemaVersion = TB_CC_CampaignContainer.SCHEMA_VERSION;' in service
assert 'm_Container.campaignId = UUID.GenV4();' in service
assert 'RestoreStrategicSnapshot(m_Persistence.GetLoadedContainer())' in native
assert 'TB_CC_ReconcileRestoredOwner(m_RestoreTransaction)' in native
for name, body in foundation['nonvoid_methods'](transaction):
    assert foundation['return_ready'](body), name

start = native.split('override protected void InitializeAuthority()',1)[1].split('protected void ReportFullNativeState()',1)[0]
load = start.index('m_Persistence.Start(')
ready = start.index('CompletePersistenceDerivedRebuild()')
for action in ('Reevaluate(', 'StartM3Consumers(', 'CallLater(SyncObjectiveTask', 'new TB_CC_TerritorialOccupationService'):
    assert load < ready < start.index(action), action
assert start.count('m_Persistence.Start(')==1
for signature in ('protected void StartM3Consumers()', 'bool AuthorizeCapture(', 'string GetBLUFORLegalObjective()',
                  'string GetOPFORLegalObjective()', 'protected void OnNativeRadioTopologyChanged()',
                  'protected void Reevaluate(', 'protected void CheckVictoryPressure()',
                  'protected void OnBaseFactionChanged(', 'protected void SyncObjectiveTask()',
                  'protected void TickTerritorialOccupation()'):
    after = native.split(signature,1)[1].split('{',1)[1]
    assert 'PersistenceAllowsCampaignConsumers()' in after.split('\n',3)[1], signature
assert 'PersistenceAllowsCampaignConsumers() && m_bReady' in native
assert 'transaction.GetStage() != TB_CC_RestoreStage.STRATEGIC_RESTORE' in transaction
assert 'base.IsProxy()' not in transaction and 'replication.IsProxy()' in transaction
assert transaction.index('s_aScopes.Insert(owner)') < transaction.index('base.SetFaction(faction)') < transaction.index('s_aScopes.RemoveItem(owner)')
assert transaction.count('base.SetFaction(faction)')==1
assert 'TB_CC_QuietOwnershipRestore.Apply(' in native
assert 'if (TB_CC_QuietOwnershipRestore.IsScoped(GetOwner())) return;' in transaction
assert 'super.OnFactionChanged(previous, current);' in transaction

installed = foundation['installed']; base = foundation['base']
affiliation = installed(base+'data','/Components/SCR_FactionAffiliationComponent.c')
assert 'override protected void OnFactionChanged(Faction previous, Faction current)' in affiliation
assert 'm_OnFactionChangedInvoker.Invoke(this, previous, current)' in affiliation
assert 'm_OnFactionUpdate.Invoke(GetAffiliatedFaction())' in affiliation
engine = installed(base+'data','/generated/Components/FactionAffiliationComponent.c')
assert 'proto external void SetAffiliatedFaction(Faction faction)' in engine
rpl = installed(base+'core','/BaseRplComponent.c')
assert 'proto external bool IsProxy();' in rpl
campaign = installed(base+'data','/SCR_CampaignMilitaryBaseComponent.c')
assert 'void SetFaction(SCR_CampaignFaction faction)' in campaign
for side_effect in ('m_OnFactionChangedExtended.Invoke', 'xpHandlerComponent.OnBaseSeized', 'SendHQMessageBaseCaptured', 'remnants.SetIsEliminated', 'RequestSavePoint'):
    assert side_effect in campaign

cases=0
fresh=foundation['fresh']
legacy_validate=foundation['validate']
Invalid=foundation['Invalid']
def validate_v2(value):
    if value.get('schemaVersion')==1:
        legacy_validate(json.dumps(value)); return False
    if value.get('schemaVersion')!=2: raise Invalid('schema')
    if type(value.get('snapshotPresent')) is not bool or type(value.get('positionOwners')) is not dict: raise Invalid('new-fields')
    # Preserve legacy validation for root identity/critical scalar types.
    scalar=copy.deepcopy(value); scalar['schemaVersion']=1
    for key in foundation['FIELDS'][8:]: scalar[key]=[] if key in ('strategicPositions','operations','players') else {}
    legacy_validate(json.dumps(scalar))
    for key in foundation['FIELDS'][8:]:
        expected=list if key in ('strategicPositions','operations','players') else dict
        if type(value.get(key)) is not expected: raise Invalid(key)
    if not value['snapshotPresent']:
        if any(value[k] for k in foundation['FIELDS'][8:]) or value['positionOwners']: raise Invalid('no-snapshot')
        return False
    ids=value['strategicPositions']; owners=value['positionOwners']
    if not ids or len(ids)!=len(set(ids)) or any(not isinstance(i,str) or not i for i in ids) or set(ids)!=set(owners): raise Invalid('ids')
    if any(o not in ('US','USSR','FIA') for o in owners.values()): raise Invalid('faction')
    if value['players'] or value['operations']: raise Invalid('later-phases')
    return True
v1=fresh(); assert validate_v2(v1) is False; cases+=1
v2=copy.deepcopy(v1); v2.update(schemaVersion=2,snapshotPresent=False,positionOwners={})
assert validate_v2(v2) is False and v1['campaignId']==v2['campaignId']; cases+=1
snapshot=copy.deepcopy(v2); snapshot.update(snapshotPresent=True,strategicPositions=['MAP::A','MAP::B'],positionOwners={'MAP::A':'US','MAP::B':'FIA'})
assert validate_v2(snapshot) is True; cases+=1
for field,bad in [('schemaVersion',0),('schemaVersion',3),('snapshotPresent',None),('positionOwners',None),('strategicPositions',['MAP::A','MAP::A']),('positionOwners',{'MAP::A':'US'}),('positionOwners',{'MAP::A':'INVALID','MAP::B':'FIA'}),('operations',['fake-operation']),('players',['fake-player'])]:
    candidate=copy.deepcopy(snapshot); candidate[field]=bad
    try: validate_v2(candidate)
    except Invalid: pass
    else: raise AssertionError((field,bad))
    cases+=1

class Gate:
    def __init__(self): self.stage='UNINITIALIZED'; self.trace=[]; self.actions=[]
    def advance(self,expected,next_stage):
        if self.stage!=expected: return False
        self.stage=next_stage; self.trace.append(next_stage); return True
    def consume(self):
        if self.stage=='READY': self.actions.append('normal-action')
    def boot(self,valid=True,snapshot=False,derived=True):
        self.advance('UNINITIALIZED','CONTAINER_LOADING'); self.consume()
        if not valid: self.stage='FAILED'; return
        self.advance('CONTAINER_LOADING','STRATEGIC_RESTORE'); self.consume()
        if snapshot: self.stage='FAILED'; return # Explicit P2B capability boundary.
        self.advance('STRATEGIC_RESTORE','DERIVED_REBUILD'); self.consume()
        if not derived: self.stage='FAILED'; return
        self.advance('DERIVED_REBUILD','READY')
for valid,saved,derived,expected in [(True,False,True,'READY'),(False,False,True,'FAILED'),(True,True,True,'FAILED'),(True,False,False,'FAILED')]:
    gate=Gate(); gate.boot(valid,saved,derived); assert gate.stage==expected and not gate.actions
    gate.consume(); assert len(gate.actions)==(expected=='READY'); cases+=1
gate=Gate(); assert not gate.advance('DERIVED_REBUILD','READY'); cases+=1
assert Gate().stage!='READY'; cases+=1

events=('capture-callback','capture-message','CONTACT','COLLAPSE','POST_CAPTURE','FIA-recapture','USSR-response','pressure-enter','task-completion','XP','remnant-cleanup')
class Affiliation:
    def __init__(self, owner): self.owner=owner; self.scoped=False; self.notifications=[]
    def set(self, owner):
        if self.owner==owner: return
        self.owner=owner
        if not self.scoped: self.notifications.extend(events)
    def quiet(self, owner, stage):
        if stage!='STRATEGIC_RESTORE': return False
        self.scoped=True; self.set(owner); self.scoped=False
        return True
for previous in ('US','USSR','FIA'):
    for current in ('US','USSR','FIA'):
        for stage in ('UNINITIALIZED','CONTAINER_LOADING','STRATEGIC_RESTORE','DERIVED_REBUILD','READY','FAILED'):
            affiliation=Affiliation(previous)
            assert affiliation.quiet(current,stage)==(stage=='STRATEGIC_RESTORE')
            assert not affiliation.notifications and not affiliation.scoped
            assert affiliation.owner==(current if stage=='STRATEGIC_RESTORE' else previous)
            cases+=1
for previous in ('US','USSR','FIA'):
    for current in ('US','USSR','FIA'):
        affiliation=Affiliation(previous); affiliation.set(current)
        assert affiliation.notifications==(list(events) if previous!=current else [])
        cases+=1
for path in core.glob('*.c'):
    source=path.read_text()
    assert 'ARLAND::' not in source and 'Arland' not in source and 'TB_CC_NATIVE_' not in source
print(f'P2A RESTORE ARCHITECTURE STATIC/INSTALLED API/MODEL PASS: {cases} cases; startup/gates/v1-v2/scope suppression/portability')
print('P2A seam regression retained; P2B wiring present. Native execution/replication/Workbench remain PENDING. WORKBENCH GATE REQUIRED')
