"""600-second pressure and native-style presentation source/model acceptance."""
from pathlib import Path
import importlib.util
import re
import runpy
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
runpy.run_path(str(TOOLS / 'Test-StrategicResponse.py'))
base = (ROOT/'Scripts/Game/TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c').read_text()
native = (ROOT/'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
response = (ROOT/'Scripts/Game/TB_CC_Core/TB_CC_StrategicResponse.c').read_text()
coordinator = (ROOT/'Scripts/Game/TB_PVE_M3_2/TB_PVE_M3_2_OPFOROperationCoordinator.c').read_text()
ui = (ROOT/'Scripts/Game/TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c').read_text()
layout_path = ROOT/'UI/layouts/BCM/Campaign/TB_CC_VictoryPressure.layout'
layout = layout_path.read_text()
meta = layout_path.with_suffix('.layout.meta').read_text()
duration = int(re.search(r'\[Attribute\(defvalue: "(\d+)"[^\n]+\] protected int m_iPressureDurationSeconds;', base)[1])
assert duration == 600
assert base.count('protected int m_iPressureDurationSeconds;') == 1
assert 'return Math.Max(m_iPressureDurationSeconds, 1);' in native
assert 'int duration = GetConfiguredPressureDuration(m_iOutcome);' in base
assert 'm_PressureDeadline = m_PressureStarted.PlusSeconds(Math.Max(durationSeconds, 1));' in base
assert 'Math.Max(durationSeconds, 1) * 1000' in base
assert re.search(r'protected int GetConfiguredPressureDuration\(int outcome\)', base)
assert re.search(r'override protected int GetConfiguredPressureDuration\(int outcome\)', native)
assert 'm_iBLUFORVictoryHoldSeconds' not in native and 'm_iOPFORVictoryHoldSeconds' not in native
for faction in ('BLUFOR', 'OPFOR'):
    assert f'EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.{faction}_VICTORY_PRESSURE, GetConfiguredPressureDuration(TB_PVE_M2_5_CampaignOutcome.{faction}_VICTORY_PRESSURE), reason)' in native
for name in ('Run-M4XP-HumanAcceptance.ps1','Run-M4XP-IntegratedProof.ps1',
             'Run-M4XP-Phase1-Unrestricted.ps1','Run-M4XP-CorrectiveAcceptance.ps1'):
    launcher = (TOOLS /name).read_text()
    assert 'Assert-NativePressureConfiguration' in launcher
    assert 'VictoryHoldSeconds 300' not in launcher

class Pressure:
    def __init__(self):
        self.deadline = None; self.vulnerable = False; self.starts = 0; self.unlocks = 0
        self.terminal = False; self.ends = 0
    def observe(self, controlled, now_ms):
        if self.terminal: return
        if self.vulnerable: self.deadline = None; return
        if controlled != 10: self.deadline = None; return
        if self.deadline is None:
            self.deadline = now_ms + duration*1000; self.starts += 1
        if now_ms >= self.deadline:
            self.vulnerable = True; self.unlocks += 1; self.deadline = None
    def remaining(self, now_ms):
        if self.deadline is None: return -1
        return max((self.deadline-now_ms+999)//1000,0)
    def capture_hq(self):
        if self.vulnerable and not self.terminal:
            self.terminal = True; self.ends += 1

timer_cases = 0
for side in ('US','USSR'):
    p = Pressure(); p.observe(9,0); assert p.deadline is None; timer_cases += 1
    p.observe(10,0); assert p.remaining(0)==600 and p.starts==1; timer_cases += 1
    for time,expected in ((1,599),(299,301),(300,300),(539,61),(540,60),(599,1)):
        p.observe(10,time*1000); assert p.remaining(time*1000)==expected and not p.vulnerable
        assert p.starts == 1; timer_cases += 1
    assert p.remaining(1)==600 and p.remaining(1001)==599; timer_cases += 2
    assert p.remaining(600000)==0
    p.observe(10,600000); assert p.vulnerable and p.unlocks==1 and not p.terminal; timer_cases += 1
    for time in (601,800,1000): p.observe(10,time*1000)
    assert p.unlocks==1; timer_cases += 1
    p.observe(9,1001000); p.observe(10,1002000)
    assert p.vulnerable and p.starts==1; timer_cases += 1
    p.capture_hq(); p.capture_hq(); assert p.ends==1; timer_cases += 1
    p=Pressure(); p.observe(10,0); assert p.remaining(180000)==420
    old_deadline=p.deadline; p.observe(9,180000)
    assert p.deadline is None and p.remaining(180000)==-1 and not p.vulnerable; timer_cases += 1
    p.observe(10,200000); assert p.remaining(200000)==600 and p.starts==2; timer_cases += 1
    p.observe(10,old_deadline); assert not p.vulnerable and p.remaining(old_deadline)==200; timer_cases += 1
    p.observe(10,800000); assert p.vulnerable and p.unlocks==1; timer_cases += 1

# Existing recovery lifecycle: one request persists through blocked admission;
# a completed operation frees that request on one tick and requeues next tick.
class Recovery:
    def __init__(self): self.request=False; self.active=False; self.terminal=False; self.next=0; self.launches=0; self.max_active=0
    def tick(self, now, remaining, capacity=True, final=False):
        if remaining<0 or final: self.request=False; self.active=False; return
        pending=self.request
        if not pending and now>=self.next and remaining>=30:
            self.request=True; self.next=now+15
        if self.terminal:
            self.request=False; self.active=False; self.terminal=False; return
        if self.request and not self.active and capacity:
            self.active=True; self.launches+=1; self.max_active=max(self.max_active,1)
recovery_cases=0
for blocked_ticks in (0,1,5,20):
    r=Recovery()
    for time in range(0,blocked_ticks*2,2):
        r.tick(time,600-time,False); assert r.request and not r.active; recovery_cases += 1
    admitted=blocked_ticks*2; r.tick(admitted,600-admitted)
    assert r.active and r.launches==1; recovery_cases += 1
    for time in range(admitted+2,100,2): r.tick(time,600-time)
    assert r.launches==1; recovery_cases += 1
    r.terminal=True; r.tick(100,500); assert not r.request and not r.active
    r.tick(102,498); assert r.active and r.launches==2 and r.max_active==1; recovery_cases += 1
    r.tick(104,496,final=True); assert not r.active and not r.request; recovery_cases += 1
for remaining in (29,0,-1):
    r=Recovery(); r.tick(500,remaining); assert not r.request; recovery_cases += 1
assert 'request.m_Operation.IsTerminal()' in response
assert 'request.m_bFinished = true;' in response and 'm_Requests.Remove(removeIndex)' in response
assert 'm_iPressureMinimumRemainingSeconds = 30' in response and 'm_iPressureRetrySeconds = 15' in response
assert 'm_iConcurrentOperations >= Capacity()' in coordinator
assert 'if (campaignPolicy && campaignPolicy.IsSovietEmergency()) return;' in coordinator

formats={600:'10:00',599:'09:59',301:'05:01',300:'05:00',61:'01:01',60:'01:00',1:'00:01',0:'00:00',-1:'00:00'}
for seconds,expected in formats.items():
    clamped=max(seconds,0); assert f'{clamped//60:02d}:{clamped%60:02d}'==expected
assert 'remaining = Math.Max(remaining, 0)' in ui
assert 'int minutes = remaining / 60;' in ui and 'int seconds = remaining % 60;' in ui

# Presentation edge models: repeated projection ticks cannot extend/requeue an
# announcement; countdown disappears immediately when authoritative pressure ends.
class Presentation:
    def __init__(self): self.pressure=False; self.hq=False; self.announce_until=-1; self.events=[]
    def tick(self, now, pressure, hq=False):
        event=None
        if pressure and not self.pressure: event='STARTED'
        if self.pressure and not pressure and not hq: event='BROKEN'
        if hq and not self.hq: event='HQ_VULNERABLE'
        self.pressure=pressure; self.hq=hq
        if event: self.events.append(event); self.announce_until=now+8
        return pressure and now>=self.announce_until
view=Presentation()
for tick in range(32): assert not view.tick(tick/4,True)
assert view.events==['STARTED'] and view.tick(8,True)
assert not view.tick(180,False) and view.events==['STARTED','BROKEN']
for tick in range(181,190): assert not view.tick(tick,False)
assert view.tick(210,True)==False
assert not view.tick(810,False,True)
for tick in range(811,820): assert not view.tick(tick,False,True)
assert view.events==['STARTED','BROKEN','STARTED','HQ_VULNERABLE']
assert 'pressure && !s_bAnnouncementActive' in ui
assert 'ANNOUNCEMENT_SECONDS = 8;' in ui
assert 's_CountdownDigits.SetText(Countdown(mode.TB_CC_GetPressureRemaining()))' in ui
assert 'PopupMsg(' not in ui and 'CreateWidgetInWorkspace' not in ui
assert 'default28' not in ui and '255, 230, 160' not in ui
for name in ('s_Title','s_Body','s_CountdownLabel','s_CountdownDigits'):
    assert f'{name}.SetColor(Color.FromRGBA(255, 255, 255, 255));' in ui
assert layout.count('Color 1 1 1 1')==4
assert 'ShadowSize 5' in layout and layout.count('Anchor 1 0 1 0') == 2
# Validate serialized frame geometry, not only editor Position/Size hints.
slots = re.findall(r'Slot FrameWidgetSlot\s*\{([^}]+)\}', layout)
assert len(slots) == 6
rects = []
for slot in slots:
    values = {key: float(value) for key, value in re.findall(
        r'(PositionX|PositionY|SizeX|SizeY|OffsetLeft|OffsetTop|OffsetRight|OffsetBottom)\s+(-?[\d.]+)', slot)}
    assert values['PositionX'] == values['OffsetLeft']
    assert values['PositionY'] == values['OffsetTop']
    assert values['SizeX'] == -values['OffsetRight'] - values['OffsetLeft']
    assert values['SizeY'] == -values['OffsetBottom'] - values['OffsetTop']
    rects.append(values)
for parent, first, second in ((rects[0], rects[1], rects[2]), (rects[3], rects[4], rects[5])):
    assert first['PositionY'] + first['SizeY'] < second['PositionY']
    for child in (first, second):
        assert child['PositionX'] >= 16 and child['PositionY'] >= 12
        assert child['PositionX'] + child['SizeX'] <= parent['SizeX'] - 16
        assert child['PositionY'] + child['SizeY'] <= parent['SizeY'] - 12
    for width, height in ((1280,720),(1600,900),(1920,1080),(2560,1440)):
        assert width + parent['PositionX'] >= 0
        assert width + parent['PositionX'] + parent['SizeX'] <= width - 40
        assert parent['PositionY'] + parent['SizeY'] < height
for name in ('s_Title', 's_Body', 's_CountdownLabel'):
    assert f'{name}.SetTextWrapping(true);' in ui
assert 's_CountdownDigits.SetTextWrapping(false);' in ui
assert '0000000000000000' not in ui+layout+meta
guid,path=re.search(r'\{([0-9A-F]{16})\}(UI/[^"\n]+\.layout)',ui).groups()
assert f'Name "{{{guid}}}{path}"' in meta and (ROOT/path).is_file()
owners=[]
for item in ROOT.rglob('*.meta'):
    if 'TestProfiles' in item.parts: continue
    if f'{{{guid}}}' in item.read_text(errors='replace'): owners.append(item)
assert owners==[layout_path.with_suffix('.layout.meta')],owners

# Confirm provenance against the installed vanilla layout, not remembered fonts.
spec=importlib.util.spec_from_file_location('vanilla_reader',TOOLS / 'Read-InstalledVanillaResource.py')
reader=importlib.util.module_from_spec(spec); spec.loader.exec_module(reader)
data=Path(str(GAME_INSTALL / 'addons/data'))
def vanilla(resource):
    for archive in data.glob('*.pak'):
        for p,offset,stored,unpacked in reader.index(archive):
            if p.lower()!=resource.lower(): continue
            with archive.open('rb') as stream: stream.seek(offset); payload=stream.read(stored)
            if stored!=unpacked: payload=zlib.decompress(payload)
            return payload.decode()
    raise AssertionError(resource)
popup=vanilla('UI/layouts/Common/PopupUI.layout')
fonts=set(re.findall(r'Font "([^"\n]+)"',layout))
assert len(fonts)==2
for font in fonts: assert font in popup,font
for resource in (layout,meta):
    stripped=re.sub(r'"(?:\\.|[^"\\])*"','""',resource)
    balance=0
    for char in stripped:
        if char=='{': balance+=1
        if char=='}': balance-=1
        assert balance>=0
    assert balance==0
for widget in ('Title','Body','CountdownLabel','CountdownDigits'):
    assert f'Name "{widget}"' in layout and f'FindAnyWidget("{widget}")' in ui

print(f'ENDGAME PRESSURE SOURCE+MODEL PASS: {timer_cases} timer/reset/HQ cases; {recovery_cases} bounded recovery cases; {len(formats)} countdown formats; 4 notification edges; 2 installed native fonts')
print('Default/effective native hold 600; world serialization unchanged; new layout metadata unique. Workbench resource processing and human visual acceptance PENDING. WORKBENCH GATE REQUIRED')
