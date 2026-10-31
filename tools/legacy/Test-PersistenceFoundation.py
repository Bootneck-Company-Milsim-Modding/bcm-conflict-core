"""Phase 1 installed API/source checks and independent disk-failure model.

Does not execute Enforce, start a server, or read/write real campaign saves.
"""
from pathlib import Path
import copy
import importlib.util
import json
import re
import uuid
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
CORE = ROOT / 'Scripts/Game/TB_CC_Persistence'
model_source = (CORE/'TB_CC_CampaignContainer.c').read_text()
service_source = (CORE/'TB_CC_PersistenceService.c').read_text()
FIELDS = ('schemaVersion campaignId mapIdentity scenarioIdentity campaignConfigIdentity '
          'campaignRevision createdTimestamp lastSavedTimestamp campaignState strategicPositions '
          'objectiveState hqState pressureState factionState operations players').split()
for field in FIELDS:
    assert f'WriteValue("{field}"' in model_source
    assert f'ReadValue("{field}"' in model_source
for path in CORE.glob('*.c'):
    text = path.read_text()
    assert not re.search(r'ARLAND::|Arland|Worlds/Production|\?[^\n]*:', text)
    stripped = re.sub(r'//[^\n]*|"(?:\\.|[^"\\])*"', '', text)
    for opening, closing in (('{','}'),('(',')'),('[',']')):
        depth = 0
        for c in stripped:
            depth += (c == opening) - (c == closing)
            assert depth >= 0, (path, opening)
        assert depth == 0, (path, opening)
    assert 'SetLOD(' not in text
    if path.name in ('TB_CC_CampaignContainer.c', 'TB_CC_PersistenceService.c'):
        assert 'SetFaction(' not in text
assert 'm_CheckpointSeconds = 60' in service_source
assert 'if (m_Dirty) SaveNow("periodic-checkpoint")' in service_source
assert 'existing-files-unrecoverable-preserved' in service_source

# Conservative return-readiness lint for these small Phase 1 classes, NOT an
# Enforce parser/type checker. Require an unconditional terminal return and
# reject the inline conditional/double-return form rejected by Workbench.
def nonvoid_methods(source):
    source = re.sub(r'//[^\n]*', '', source)
    clean = re.sub(r'"(?:\\.|[^"\\])*"', '""', source)
    header = re.compile(r'\b(?:protected\s+)?(bool|int|string|TB_CC_CampaignContainer)\s+(\w+)\s*\([^{};]*\)\s*\{')
    for match in header.finditer(clean):
        begin = match.end(); depth = 1; end = begin
        while depth and end < len(clean):
            depth += (clean[end] == '{') - (clean[end] == '}'); end += 1
        assert depth == 0, match.group(2)
        yield match.group(2), clean[begin:end-1]

def return_ready(body):
    if re.search(r'\bif\s*\([^\n{}]*\)\s*return[^\n;]*;[^\n]*\breturn\b', body): return False
    # Drop nested blocks; only the outer method's terminal return counts.
    top = []; depth = 0
    for char in body:
        if char == '{': depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0: top.append('\n')
        elif depth == 0: top.append(char)
    return bool(re.search(r'\breturn\s+[^;{}]+;\s*$', ''.join(top)))

return_methods = 0
for source in (model_source, service_source):
    for name, body in nonvoid_methods(source):
        assert return_ready(body), f'Phase 1 terminal-return/inline-return hazard: {name}'
        return_methods += 1
assert return_methods == 37, return_methods  # P3's 21 plus 16 P5 service contracts, all linted.
for body, expected in [
    ('return 0;', True),
    ('if (!value) { return -1; } return value.revision;', True),
    ('if (value) { return value.revision; }', False),
    ('if (value) return value.revision; return -1;', False),
    ('value = 1;', False),
]: assert return_ready(body) == expected
for name, field, fallback in [('GetCampaignId','campaignId','""'),('GetCampaignRevision','campaignRevision','-1')]:
    getter = re.search(rf'\b{name}\(\)\s*\{{(.*?)\n\t\}}', service_source, re.S)[1]
    assert 'if (!m_Container)' in getter
    assert f'return {fallback};' in getter and f'return m_Container.{field};' in getter
    assert not re.search(r'GenV4|FileIO|SaveNow|MarkDirty|\+\+|m_Container\.\w+\s*=', getter)

spec = importlib.util.spec_from_file_location('reader', TOOLS / 'Read-InstalledVanillaResource.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
def installed(directory, suffix):
    for archive in Path(directory).glob('*.pak'):
        for path, offset, stored, unpacked in reader.index(archive):
            if not path.endswith(suffix): continue
            with archive.open('rb') as stream:
                stream.seek(offset); payload = stream.read(stored)
            return (zlib.decompress(payload) if stored != unpacked else payload).decode()
    raise AssertionError(suffix)
base = str(GAME_INSTALL / 'addons/') + '/'
api_checks = {
    ('core','/FileIO.c'): ['static proto bool MakeDirectory(string name)', 'static proto bool CopyFile(string sourceName, string destName)', 'static proto bool FileExists(string name)'],
    ('core','/UUID.c'): ['static proto UUID GenV4()', 'static proto bool IsUUID(string uuid)', 'proto external bool IsNull()'],
    ('core','/generated/System/System.c'): ['static proto int GetUnixTime()'],
    ('data','/JsonLoadContext.c'): ['proto external bool LoadFromFile(string fileName)'],
    ('data','/JsonSaveContext.c'): ['proto external bool SaveToFile(string fileName)'],
    ('data','/LoadContext.c'): ['proto bool ReadValue(string name, out void value)'],
    ('data','/SaveContext.c'): ['proto bool WriteValue(string name, void value)'],
}
for (directory, suffix), signatures in api_checks.items():
    source = installed(base+directory, suffix)
    for signature in signatures: assert signature in source, signature
assert 'Rename' not in installed(base+'core','/FileIO.c')
native = (ROOT/'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
assert 'm_Persistence.Start(m_Registry.GetMapDefinitionId()' in native
assert 'if (m_Persistence) m_Persistence.Stop();' in native

class Invalid(Exception): pass
class Foreign(Invalid): pass
def fresh():
    value = dict(schemaVersion=1, campaignId=str(uuid.uuid4()), mapIdentity='MAP_A',
                 scenarioIdentity='SCENARIO_A', campaignConfigIdentity='CONFIG_A',
                 campaignRevision=0, createdTimestamp=1, lastSavedTimestamp=1)
    for key in FIELDS[8:]: value[key] = [] if key in ('strategicPositions','operations','players') else {}
    return value
def validate(raw):
    try: value = json.loads(raw)
    except (ValueError, TypeError): raise Invalid('json')
    if not isinstance(value,dict) or any(k not in value for k in FIELDS): raise Invalid('root')
    if type(value['schemaVersion']) is not int: raise Invalid('schema-type')
    if value['schemaVersion'] != 1: raise Foreign('schema')
    try:
        if uuid.UUID(value['campaignId']).int == 0: raise ValueError()
    except (ValueError, TypeError, AttributeError): raise Invalid('id')
    for key, expected in [('mapIdentity','MAP_A'),('scenarioIdentity','SCENARIO_A'),('campaignConfigIdentity','CONFIG_A')]:
        if value[key] != expected: raise Foreign(key)
    for key in ('campaignRevision','createdTimestamp','lastSavedTimestamp'):
        if type(value[key]) is not int or value[key] < 0: raise Invalid(key)
    if value['lastSavedTimestamp'] < value['createdTimestamp']: raise Invalid('time')
    for key in FIELDS[8:]:
        expected = list if key in ('strategicPositions','operations','players') else dict
        if type(value[key]) is not expected or value[key]: raise Invalid(key)
    return value
class Store:
    def __init__(self, disk): self.disk=disk; self.value=None; self.dirty=False; self.state='RESTORING'; self.writes=0
    def start(self):
        for name in ('primary','backup'):
            if name not in self.disk: continue
            try: self.value=validate(self.disk[name]); break
            except Foreign: self.state='FAILED'; return
            except Invalid: pass
        if self.value is None:
            if any(name in self.disk for name in ('primary','backup','temp','rotation')): self.state='FAILED'; return
            self.value=fresh(); self.dirty=True
        self.state='READY'
    def checkpoint(self, fail=None):
        if self.state!='READY' or not self.dirty: return
        def write(name, content):
            self.writes+=1
            if fail == self.writes:
                self.disk[name]='{"partial":'; raise OSError('injected partial write')
            self.disk[name]=content
        try:
            raw=json.dumps(self.value); write('temp',raw); validate(self.disk['temp'])
            if 'primary' in self.disk:
                try: previous=validate(self.disk['primary'])
                except Foreign: return
                except Invalid:
                    write('rejected',self.disk['primary']); validate(self.disk['backup'])
                else:
                    if 'backup' in self.disk:
                        try: validate(self.disk['backup'])
                        except Foreign: return
                        except Invalid: pass
                    write('rotation',json.dumps(previous)); validate(self.disk['rotation'])
                    write('backup',self.disk['rotation']); validate(self.disk['backup'])
            elif 'backup' not in self.disk:
                write('backup',raw); validate(self.disk['backup'])
            else: validate(self.disk['backup'])
            write('primary',raw); assert validate(self.disk['primary'])==self.value
            self.dirty=False
        except (OSError,Invalid): pass

cases=0
s=Store({}); s.start(); assert s.state=='READY' and s.dirty
s.checkpoint(); identity=s.value['campaignId']; assert not s.dirty
r=Store(s.disk); r.start(); assert r.value['campaignId']==identity
r.checkpoint(); assert r.writes==0; cases+=4
for primary,backup,expected in [(json.dumps(s.value),'bad','READY'),('bad',json.dumps(s.value),'READY'),(None,json.dumps(s.value),'READY'),('bad','bad','FAILED'),('bad',None,'FAILED'),(None,'bad','FAILED')]:
    disk={k:v for k,v in [('primary',primary),('backup',backup)] if v is not None}; before=copy.deepcopy(disk)
    trial=Store(disk); trial.start(); assert trial.state==expected and disk==before; cases+=1
for key,bad_values in [('schemaVersion',[0,2,None,'1']),('campaignId',['',str(uuid.UUID(int=0)),'bad']),('mapIdentity',['MAP_B']),('scenarioIdentity',['OTHER']),('campaignConfigIdentity',['OTHER']),('campaignRevision',[-1,'0',None]),('createdTimestamp',[-1]),('lastSavedTimestamp',[0]),('campaignState',[None,{'future':'payload'}])]:
    for bad in bad_values:
        value=copy.deepcopy(s.value); value[key]=bad; disk={'primary':json.dumps(value)}; before=copy.deepcopy(disk)
        trial=Store(disk); trial.start(); assert trial.state=='FAILED' and disk==before; cases+=1
for key in FIELDS:
    value=copy.deepcopy(s.value); del value[key]
    trial=Store({'primary':json.dumps(value)}); trial.start(); assert trial.state=='FAILED'; cases+=1
for key,bad in [('schemaVersion',2),('mapIdentity','MAP_B'),('scenarioIdentity','OTHER'),('campaignConfigIdentity','OTHER')]:
    value=copy.deepcopy(s.value); value[key]=bad
    disk={'primary':json.dumps(value),'backup':json.dumps(s.value)}; before=copy.deepcopy(disk)
    trial=Store(disk); trial.start(); assert trial.state=='FAILED' and disk==before; cases+=1
for artifact in ('temp','rotation'):
    trial=Store({artifact:json.dumps(s.value)}); trial.start(); assert trial.state=='FAILED'; cases+=1
trial=Store({'primary':'bad','backup':json.dumps(s.value)}); trial.start()
trial.value['campaignRevision']=2; trial.dirty=True; trial.checkpoint()
assert not trial.dirty and trial.disk['rejected']=='bad' and validate(trial.disk['primary'])['campaignRevision']==2
assert validate(trial.disk['backup'])['campaignRevision']==0; cases+=1
trial=Store(copy.deepcopy(s.disk)); trial.start(); trial.value['campaignRevision']=3; trial.dirty=True; trial.checkpoint()
assert not trial.dirty and validate(trial.disk['primary'])['campaignRevision']==3
assert validate(trial.disk['backup'])['campaignRevision']==0
restart=Store(trial.disk); restart.start(); assert restart.value['campaignRevision']==3 and restart.value['campaignId']==identity; cases+=1
for fail in range(1,5):
    disk={'primary':json.dumps(s.value),'backup':json.dumps(s.value)}
    trial=Store(disk); trial.start(); trial.value['campaignRevision']=1; trial.dirty=True
    trial.checkpoint(fail); assert trial.dirty
    valid=[]
    for name in ('primary','backup','rotation'):
        try: valid.append(validate(disk.get(name)))
        except Invalid: pass
    assert any(v['campaignId']==identity and v['campaignRevision']==0 for v in valid); cases+=1
for fail in range(1,4):
    trial=Store({}); trial.start(); trial.checkpoint(fail); assert trial.dirty
    restart=Store(trial.disk); restart.start()
    assert restart.state=='FAILED' or restart.value['campaignId']==trial.value['campaignId']; cases+=1
print(f'PERSISTENCE FOUNDATION STATIC/INSTALLED API/MODEL PASS: {cases} cases; 16 root fields; map-agnostic core')
print(f'PHASE 1 RETURN-READINESS PASS: {return_methods} non-void methods; 5 lint fixtures; read-only getter invariants')
print('NOT Enforce compile, real serialization IO, filesystem atomicity or runtime PASS. WORKBENCH GATE REQUIRED')
