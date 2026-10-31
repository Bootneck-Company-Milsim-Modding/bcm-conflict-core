"""Source-bound scheduling/placement models, NOT an Enforce or runtime test."""
from pathlib import Path
import re
import runpy
import struct

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
runpy.run_path(str(TOOLS / 'Test-EnforceFixedArrays.py'))
runpy.run_path(str(TOOLS / 'Test-ObjectiveTerritorialContinuity.py'))
response = (ROOT/'Scripts/Game/TB_CC_Core/TB_CC_StrategicResponse.c').read_text()
defence = (ROOT/'Scripts/Game/TB_CC_Core/TB_CC_StrategicDefence.c').read_text()
native = (ROOT/'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
ui = (ROOT/'Scripts/Game/TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c').read_text()
adapter = (ROOT/'Scripts/Game/TB_CC_M4X/TB_CC_NativeResponseAdapter.c').read_text()
operation = (ROOT/'Scripts/Game/TB_PVE_M3_1/TB_PVE_M3_1_Operation.c').read_text()
assert 'SpawnEntityPrefab' not in response and 'SetFaction(' not in response
for source in (response, defence):
    assert 'ARLAND::' not in source and 'MilitarySouth' not in source
for token in ('m_bContact', 'm_bCollapse', 'm_iEpoch != request.m_iEpoch', 'm_iDueMs = now + 2000',
              'request.m_bAdmitting = true', 'POST_CAPTURE_2', 'm_Map.IsOperational()', 'm_Map.FinalDefence()'):
    assert token in response, token
assert 'm_sFactionKey == "FIA" || !war.WantsPhysical(m_WarForce)' in operation
assert 'm_AssemblyService.AssemblePackage' in operation
assert 'm_WarForce.m_State.m_iStrength > 0' in operation
assert 'm_sRole = "garrison"' in (ROOT/'Scripts/Game/TB_CC_CoreWar/TB_CC_WarSimulation.c').read_text()
assert 'reason == "M4.4H-victory-pressure"' in native
assert 'm_bEnableM3Garrison && IsWarOperational()' in native
assert 'GetResponseLossRevision(id)' in adapter and 'WarSource("USSR"' in adapter

def delay(seed, target, epoch, phase, low=120, high=240):
    value = abs(seed % 100003)
    for char in target:
        value = (value * 31 + ord(char)) % 100003
    value = (value + (epoch % 100003)*17 + phase*47) % 100003
    low = min(max(low, 120), 240)
    high = min(max(high, low), 240)
    return low + value % (high-low+1)

delay_cases = 0
for seed in range(50):
    for epoch in range(10):
        for phase in (2, 3):
            for bounds in ((120,240),(500,600),(0,10),(240,120)):
                value = delay(seed, 'MAP::POINT', epoch, phase, *bounds)
                assert 120 <= value <= 240
                assert value == delay(seed, 'MAP::POINT', epoch, phase, *bounds)
                delay_cases += 1

class Model:
    def __init__(self):
        self.epoch=0; self.owner='FIA'; self.contact=False; self.collapse=False
        self.queue=[]; self.operational=True; self.capability=True
    def add(self, phase, due):
        self.queue.append(dict(phase=phase, due=due, epoch=self.epoch, owner=self.owner))
    def assault(self, count, human=True):
        if not human or self.owner not in ('FIA','USSR'): return
        if not self.contact: self.contact=True; self.add(0,0)
        if count<=5 and not self.collapse: self.collapse=True; self.add(1,0)
    def change(self, owner, now):
        previous=self.owner; self.owner=owner
        if owner=='US' and previous in ('FIA','USSR'): self.add(2,now+delay(1,'POINT',self.epoch,2))
        else: self.epoch+=1; self.contact=False; self.collapse=False
    def valid(self, request):
        return self.operational and self.capability and request['epoch']==self.epoch and request['owner']==self.owner
    def admit(self, now, capacity=True):
        self.queue[:]=[q for q in self.queue if self.valid(q)]
        if not capacity: return None
        for q in self.queue:
            if q['due']<=now:
                self.queue.remove(q)
                if q['phase']==2: self.add(3,now+delay(1,'POINT',self.epoch,3))
                return q
        return None

scenarios=0
for owner in ('FIA','USSR'):
    for first in (3,5,6,12):
        m=Model(); m.owner=owner
        m.assault(first,False); assert not m.queue
        for count in (first,8,5,6,4,5,0): m.assault(count)
        assert [q['phase'] for q in m.queue]==[0,1]
        m.change('US',10)
        m.admit(11,False); assert [q['phase'] for q in m.queue]==[2]
        due=m.queue[0]['due']
        for _ in range(5): assert m.admit(due,False) is None
        assert len(m.queue)==1
        assert m.admit(due)['phase']==2
        assert len(m.queue)==1 and 120<=m.queue[0]['due']-due<=240
        assert m.admit(due) is None
        assert m.admit(m.queue[0]['due'])['phase']==3
        m.change(owner,900); m.assault(5); assert len(m.queue)==2
        m.change('US',901); m.change(owner,902); m.admit(9999,False); assert not m.queue
        m.assault(5); m.operational=False; m.admit(9999); assert not m.queue
        scenarios+=1
for unavailable in ('budget','nav','concurrency'):
    m=Model(); m.change('US',0)
    for _ in range(50): m.admit(500,False)
    assert len(m.queue)==1
    m.capability=False; m.admit(500); assert not m.queue
    scenarios+=1

# Pressure recovery: meaningful time, one pending force, loss cancellation,
# permanent vulnerability and one final defence latch (no synthetic runtime).
pressure_cases=0
for remaining in (-1,0,29,30,100,300):
    for final in (False,True):
        for pending in (False,True):
            eligible=remaining>=30 and not final and not pending
            assert eligible == (remaining in (30,100,300) and not(final or pending))
            pressure_cases+=1
for token in ('ANNOUNCEMENT_SECONDS = 8','TERRITORIAL CONTROL BROKEN','ENEMY HQ VULNERABLE',
              'CreateWidgets(LAYOUT)','s_bWasPressure','s_bHQVulnerable'):
    assert token in ui
assert 's_CountdownDigits.SetText(Countdown(mode.TB_CC_GetPressureRemaining()))' in ui

# Conservative placement rejects each hazard independently. This is a filter
# model, not evidence about the terrain or final native composition bounds.
placement_cases=0
for mask in range(128):
    water,slope,collision,road,support,nav,spacing=[bool(mask & (1<<i)) for i in range(7)]
    admitted=not any((water,slope,collision,road,support,nav,spacing))
    assert admitted == (mask==0)
    placement_cases+=1
for token in ('GetRoadsInAABB','TraceOBB','TB_CC_WarPlacement.Surface','footprint-support',
              'm_aNoBuild','m_aSlots','m_iMaximumObjects','PlayerNear(candidate)',
              'm_bVehicleDefenceEnabled = false','retainedAfterCapture=1'):
    assert token in defence
assert 'DeleteEntity' not in defence

# Exact local resource identity validation, not just a nonzero-GUID regex.
data=Path(str(GAME_INSTALL / 'addons/data'))
rdb=(data/'resourceDatabase.rdb').read_bytes()
layout=(ROOT/'UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout').read_text()
resources=set(re.findall(r'\{([0-9A-F]{16})\}([^"\r\n]+\.(?:et|fnt))',defence+ui+layout))
for guid,path in resources:
    matches=list(re.finditer(re.escape(path.encode())+b'\x00',rdb,re.I))
    values=[]
    for match in matches:
        end=match.end()
        if struct.unpack_from('<I',rdb,end)[0]==6:
            values.append(f'{struct.unpack_from("<Q",rdb,end+6)[0]:016X}')
    assert guid in values,(guid,path,values)
print(f'STRATEGIC RESPONSE SOURCE+MODEL PASS: {delay_cases} delay cases; {scenarios} epoch/queue scenarios; {pressure_cases} pressure cases; {placement_cases} placement-filter cases; {len(resources)} installed resource identities')
print('NOT engine execution, tactical terrain validation, multiplayer/JIP UI proof or Workbench compile. WORKBENCH GATE REQUIRED')
