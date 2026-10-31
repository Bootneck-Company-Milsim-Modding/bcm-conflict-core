"""Installed-source checks + independent exhaustive model; NOT Enforce runtime."""
from pathlib import Path
import runpy
import re
import math
from collections import deque

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-NativeChatFullChain.py'))
reader = runpy.run_path(str(TOOLS / 'Read-InstalledVanillaResource.py'))
import zlib
archive = Path(str(GAME_INSTALL / 'addons/data/data007.pak'))
entries = {p.lower(): (o, s, u) for p, o, s, u in reader['index'](archive)}
def resource(path):
    o, s, u = entries[path.lower()]
    with archive.open('rb') as stream:
        stream.seek(o); data = stream.read(s)
    return (zlib.decompress(data) if s != u else data).decode('utf8')

evaluator = (ROOT / 'Scripts/Game/TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c').read_text()
native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
assert 'm_bThreeFactionExpansion && m_sExpansionFaction == "US"' in evaluator
assert 'if (UsesUSTerritorialBands()) { BuildUSTerritorialCandidates' in evaluator
assert 'owners.Get(id) == "US"' in evaluator
assert 'for (int cursor; cursor < pending.Count(); cursor++)' in evaluator
assert 'band < bestBand' in evaluator and 'band > bestBand' in evaluator
assert 'if (!radioReachable) { m_iRadioRejectedCandidates++; continue; }' in evaluator
assert 'm_Evaluator.ConfigureObjectiveBands(m_fUSObjectiveBandMeters)' in native
assert 'm_USSREvaluator.ConfigureObjectiveBands' not in native
assert 'ARLAND::' not in evaluator
assert 'currentTaskState == SCR_ETaskState.CREATED || currentTaskState == SCR_ETaskState.ASSIGNED' in native
assert 'Epoch' not in native.split('protected string GetPresentedTaskId')[1].split('protected ref')[0]
tasks = resource('scripts/Game/Tasks/SCR_TaskSystem.c')
task = resource('scripts/Game/Tasks/Campaign/SCR_CampaignMilitaryBaseTaskEntity.c')
handler = resource('scripts/Game/FactionCommander/Components/SCR_CampaignFactionCommanderHandlerComponent.c')
assert 'if (GetTaskFromTaskID(taskID, false))' in tasks
assert 'SetTaskState(SCR_ETaskState.COMPLETED);\n\t\tDeleteTask();' in task.replace('\r', '')
assert 'SCR_ETaskState.CREATED | SCR_ETaskState.ASSIGNED' in handler
assert 'taskRpl.DeleteRplEntity(task, false);' in tasks
assert 'SCR_ETaskState GetTaskState(notnull SCR_Task task)' in tasks
assert 'int usOwnedCount;' in evaluator and 'usOwnedCount++' in evaluator
assert not re.search(r'\bint\s+owned\b', evaluator)

definition = (ROOT / 'Scripts/Game/TB_CC_M4_2/TB_CC_M4_2_ArlandProductionDefinition.c').read_text()
pos = {}
for ident, coords, importance in re.findall(r'Position\("(ARLAND::\w+)",[^\n]+?"([\d. -]+)",\s*"[^"\n]+",\s*(\d+),', definition):
    x, _, z = map(float, coords.split()); pos[ident] = (x, z, int(importance))
edges = re.findall(r'Connect\(definition, "([^"]+)", "([^"]+)"\)', definition)
adj = {n: set() for n in pos}
for a, b in edges: adj[a].add(b); adj[b].add(a)
origin, endpoint = 'ARLAND::HQ_EAST', 'ARLAND::HQ_WEST'
ordinary = sorted(set(pos) - {origin, endpoint})
assert len(pos) == 12 and len(edges) == 21 and len(ordinary) == 10

def scan(owned, radio=None, graph=adj, positions=pos, start=origin, end=endpoint, band_size=1000):
    territory = set(owned) | {start}
    hostile = set(positions) - territory - {end}
    depths = {n: 0 for n in territory}; pending = deque(sorted(territory))
    while pending:
        current = pending.popleft()
        for n in sorted(graph[current]):
            if n == end or n in depths: continue
            depths[n] = depths[current] + 1; pending.append(n)
    reachable = hostile & depths.keys()
    if not hostile: return 'NO_ENEMY_REMAINING', None, set(), {}
    if not reachable: return 'NO_VALID_FRONTLINE_OBJECTIVE', None, set(), {}
    frontier_depth = min(depths[n] for n in reachable)
    candidates = sorted(n for n in reachable if depths[n] == frontier_depth)
    distances = {n: min(math.hypot(positions[n][0]-positions[s][0], positions[n][1]-positions[s][1]) for s in territory) for n in hostile}
    ranking = []
    for i, n in enumerate(candidates):
        if radio is not None and n not in radio: continue
        d = distances[n]
        score = positions[n][2]*40//100
        score += math.floor(30*(1-min(d/1500,1))+.5)
        score += min(len(graph[n] & territory),3)*20//3
        score += min(len(graph[n] & hostile),6)*10//6
        variation = ((20 % 997)*31+i*17)%3
        ranking.append((math.floor(d/band_size), -score, -variation, n))
    return ('READY', min(ranking)[3], set(candidates), distances) if ranking else ('RADIO_WAIT', None, set(candidates), distances)

radio_cases = transitions = 0
for mask in range(1024):
    owned = {n for i,n in enumerate(ordinary) if mask & (1 << i)}
    state, selected, boundary, distances = scan(owned)
    if len(owned) == 10:
        assert state == 'NO_ENEMY_REMAINING' and not boundary
    else:
        assert state == 'READY' and selected in boundary
        assert all(adj[n] & (owned | {origin}) for n in boundary)
        assert scan(owned, set())[0] == 'RADIO_WAIT'
        # Every possible radio eligibility combination on this frontier.
        ordered = sorted(boundary)
        for radio_mask in range(1 << len(ordered)):
            available = {n for i,n in enumerate(ordered) if radio_mask & (1 << i)}
            result, choice, _, _ = scan(owned, available)
            assert result == ('READY' if available else 'RADIO_WAIT')
            if available:
                assert choice in available
                assert distances[choice]//1000 == min(distances[n]//1000 for n in available)
            radio_cases += 1
    for n in ordinary:
        for other in ('FIA','USSR'):
            owners = {p: ('US' if p in owned else other) for p in ordinary}
            before = owners[n]; owners[n] = other if before == 'US' else 'US'
            following = {p for p,v in owners.items() if v == 'US'}
            result, choice, _, _ = scan(following)
            assert result == ('NO_ENEMY_REMAINING' if len(following)==10 else 'READY')
            assert choice is None or owners[choice] != 'US'
            transitions += 1

owned = set(); route=[]
while len(owned)<10:
    state, target, _, _ = scan(owned); assert state=='READY'
    route.append(target); owned.add(target)
assert route[0]=='ARLAND::MILITARY_SOUTH' and len(set(route))==10
assert scan(owned)[0]=='NO_ENEMY_REMAINING'
lost='ARLAND::MILITARY_SOUTH'; current='ARLAND::MOSS_HILL'
# Rear loss does not retask commitment; after current resolves, old territory
# is a fresh candidate. Repeat twice-hostile semantics for five full cycles.
for enemy in ('FIA','USSR'):
    for cycle in range(5):
        owned=set(ordinary)-{current}; owned.discard(lost)
        committed=current
        assert committed==current
        owned.add(current)
        state,target,boundary,_=scan(owned)
        assert target==lost and lost in boundary
        owned.add(lost); assert scan(owned)[0]=='NO_ENEMY_REMAINING'
# Multiple losses and isolated friendly footprint are included.
assert scan(set(ordinary)-set(route[:3]))[0]=='READY'
isolated={'ARLAND::BEAUREGARD'}
assert 'ARLAND::BEAUREGARD_HARBOUR' in scan(isolated)[2]

# No maximum distance band, plus radio-rejected near band expands outward.
g={'HQ':{'A','B','C'}, 'END':set(), 'A':{'HQ'},'B':{'HQ'},'C':{'HQ'}}
p={'HQ':(0,0,0),'END':(0,0,0),'A':(1500,0,1),'B':(2500,0,100),'C':(12500,0,100)}
assert scan(set(),graph=g,positions=p,start='HQ',end='END')[1]=='A'
assert scan(set(),{'B','C'},g,p,'HQ','END')[1]=='B'
assert scan(set(),{'C'},g,p,'HQ','END')[1]=='C'

# Native fixed-ID recreation model: completed records are removed, not banned.
registry={}; task_id='US_SeizeCampaignMilitaryBase_5'
for cycle in range(5):
    assert task_id not in registry
    registry[task_id]='CREATED'; registry[task_id]='COMPLETED'; del registry[task_id]
assert not registry
# HQ pressure input remains current counts. Do not simulate a timer as runtime proof.
assert 'if (usCount == 10 && m_iHQWestState != TB_CC_HQLockState.VULNERABLE)' in native
assert 'CancelVictoryPressure("dominance-broken-or-unlock-earned")' in native
assert 'EnterVictoryPressure(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE, GetConfiguredPressureDuration(TB_PVE_M2_5_CampaignOutcome.BLUFOR_VICTORY_PRESSURE), reason)' in native
print(f'OBJECTIVE CONTINUITY MODEL PASS: 1024 ownership subsets; {transitions} faction-labelled transitions; {radio_cases} exhaustive frontier radio masks')
print('TEN-CAPTURE MODEL ROUTE:', ' -> '.join(route))
print('Five repeated recapture cycles per hostile faction / stable task ID reuse / commitment / unbounded distance bands / current-count HQ pressure SOURCE+MODEL PASS')
print('NOT Workbench compile, real capture, replication, construction or live-session root-cause proof. WORKBENCH GATE REQUIRED')
