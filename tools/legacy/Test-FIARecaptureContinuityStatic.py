"""Source/API regression plus offline queue/graph models, NOT Enforce/runtime proof."""
from pathlib import Path
from dataclasses import dataclass
from collections import deque
import re
import runpy
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
runpy.run_path(str(TOOLS / 'Test-NativeRadioStatic.py'))
reader = runpy.run_path(str(TOOLS / 'Read-InstalledVanillaResource.py'))
core = Path(str(GAME_INSTALL / 'addons/core/data.pak'))
entry = next(e for e in reader['index'](core) if e[0].lower() == 'scripts/core/proto/types.c')
_, offset, stored, unpacked = entry
with core.open('rb') as stream:
    stream.seek(offset)
    payload = stream.read(stored)
if stored != unpacked:
    payload = zlib.decompress(payload)
assert b'proto bool RemoveItemOrdered(T value);' in payload
assert b'proto bool Remove(TKey key);' in payload
print('FIA QUEUE installed ordered-removal/map API PASS')
def read(path):
    return (ROOT / path).read_text()
ledger = read('Scripts/Game/TB_CC_Core/TB_CC_FIARecaptureOpportunity.c')
coordinator = read('Scripts/Game/TB_CC_M4X/TB_CC_TerritorialOccupation.c')
native = read('Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c')
operation = read('Scripts/Game/TB_PVE_M3_1/TB_PVE_M3_1_Operation.c')
war = read('Scripts/Game/TB_CC_CoreWar/TB_CC_WarSimulation.c')
evaluator = read('Scripts/Game/TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c')

assert 'm_Owners.Get(target) == owner' in ledger
assert 'm_Epochs.Get(target) + 1' in ledger
assert 'm_Current.Set(target, opportunity); m_Queue.Insert(opportunity)' in ledger
assert 'm_Queue.RemoveItemOrdered(opportunity); m_Queue.Insert(opportunity)' in ledger
assert 'm_Queue.RemoveItem(' not in ledger  # unordered removal would break FIFO
assert 'opportunity.m_bFinished = true' in ledger
assert 'm_FIARecaptureLedger.Seed(id, base.GetFaction().GetFactionKey())' in native
assert 'm_FIARecaptureLedger.Observe(id, newFaction.GetFactionKey(), m_iRevision + 1)' in native
assert 'm_Ledger.DeferAdmission(opportunity)' in coordinator
assert 'm_aOperations.Count() >= m_Policy.GetMaximumConcurrentFIAOperations()' in coordinator
assert 'm_sFactionKey == "FIA" || !war.WantsPhysical(m_WarForce)' in operation
assert 'm_sFactionKey == "FIA" && !war' in operation
assert 'stagingRadius = 300; stagingRadius <= 500; stagingRadius += 50' in war
assert 'bearing < 16' in war and 'TB_CC_WarPlacement.Surface(' in war and 'TB_CC_WarPlacement.Project(' in war
assert 'if (!initialFIA && radius > m_Config.m_fMaximumPlacementSearch)' in war
assert 'targetDistance < 300 || targetDistance > 500' in war
assert 'm_iRetryAfterMs = System.GetTickCount() + m_Config.m_iRetryMs' in war
assert 'Relevant(lease, m_Config.m_fPreRadius) || IsTargetPhysical(state.m_sTargetId)' in war
assert '[TB_CC_CAMPAIGN_PATH]' in native and '[TB_CC_FIA_RECAPTURE]' in ledger
for forbidden in ('ARLAND', 'MOSS_HILL', 'MILITARY_SOUTH', 'SetFaction(', 'SetLOD(', 'SpawnEntity'):
    assert forbidden not in ledger

@dataclass(eq=False)
class Opportunity:
    target: str
    owner: str
    epoch: int
    mode: str = 'QUEUED'
    finished: bool = False

class Model:
    def __init__(self):
        self.owners = dict.fromkeys('ABC', 'FIA')
        self.epochs = dict.fromkeys('ABC', 0)
        self.current = {}
        self.queue = deque()
        self.active = []
    def finish(self, item):
        item.finished = True
        if item in self.queue:
            self.queue.remove(item)
        if item in self.active:
            self.active.remove(item)
    def observe(self, target, owner):
        if self.owners[target] == owner:
            return
        self.owners[target] = owner
        self.epochs[target] += 1
        old = self.current.pop(target, None)
        if old in self.queue:
            self.finish(old)
        if owner in ('US', 'USSR'):
            item = Opportunity(target, owner, self.epochs[target])
            self.current[target] = item
            self.queue.append(item)
    def start(self, admission=True):
        if self.active or not self.queue:
            return None
        item = self.queue.popleft()
        if not admission:
            self.queue.append(item)
            return None
        item.mode = 'VIRTUAL'
        self.active.append(item)
        return item
    def placement(self, item, near, nav=False, budget=True):
        # Failed admission preserves virtual survivors; player departure is
        # representation/movement policy, not cancellation.
        if near and nav and budget:
            item.mode = 'PHYSICAL'
        elif not near:
            item.mode = 'VIRTUAL'

m = Model()
m.observe('A', 'US')
for _ in range(100):
    m.observe('A', 'US')
assert len(m.queue) == 1 and m.epochs['A'] == 1  # A
a = m.start()
for _ in range(1000):
    m.placement(a, near=True, nav=False)
assert a in m.active and not a.finished and a.mode == 'VIRTUAL'  # B
m.placement(a, near=True, nav=True, budget=False)
assert a in m.active and a.mode == 'VIRTUAL'
m.placement(a, near=True, nav=True)
assert a.mode == 'PHYSICAL'  # C
m.placement(a, near=False)
assert a.mode == 'VIRTUAL' and not a.finished  # D
m.observe('B', 'USSR')
assert m.start() is None and [x.target for x in m.queue] == ['B']  # E
m.finish(a)
b = m.start()
assert b.target == 'B'  # F
m.observe('C', 'US'); old = m.current['C']
m.observe('C', 'FIA')
assert old.finished and not m.queue  # G
m.observe('C', 'US'); new = m.current['C']
assert new.epoch == 3 and new is not old
m.observe('C', 'USSR')
assert new.finished and len(m.queue) == 1
for i in range(1000):
    m.observe('C', ('US', 'USSR')[i % 2])
assert len(m.queue) == 1  # bounded by registered target count, not history
m.finish(b); m.observe('A', 'USSR')
head = m.queue[0]
m.start(admission=False)
assert m.queue[-1] is head and len(m.queue) == 2  # no head-of-line starvation
assert m.start().target == 'A'
print('FIA QUEUE A-G + budget/repeated callbacks/epochs/bounds/admission fairness MODEL PASS')

# Exhaust every reachable ownership subset and every candidate choice, not a
# single hand-picked route. This mirrors the evaluator's connected-component /
# hostile-boundary rules and uses the actual authored definition as input.
definition = read('Scripts/Game/TB_CC_M4_2/TB_CC_M4_2_ArlandProductionDefinition.c')
nodes = set(re.findall(r'definition.AddPosition\(Position\("([^"]+)"', definition))
edges = re.findall(r'Connect\(definition, "([^"]+)", "([^"]+)"\)', definition)
assert len(nodes) == 12 and len(edges) == 21
origin, endpoint = 'ARLAND::HQ_EAST', 'ARLAND::HQ_WEST'
ordinary = nodes - {origin, endpoint}
assert len(ordinary) == 10
adj = {n: set() for n in nodes}
for left, right in edges:
    adj[left].add(right); adj[right].add(left)

def candidates(owned):
    reached, pending = set(), [origin]
    while pending:
        n = pending.pop()
        if n in reached or (n != origin and n not in owned):
            continue
        reached.add(n)
        pending.extend(adj[n] - reached)
    return set().union(*(adj[n] for n in reached)) & (ordinary - owned)

seen = {frozenset()}; todo = deque(seen); terminal = set(); transitions = 0
while todo:
    owned = todo.popleft()
    boundary = candidates(owned)
    assert not (boundary & {origin, endpoint})
    if len(owned) < 10:
        assert boundary, owned  # H: synthetic radio all connected
    else:
        assert not boundary
        terminal.add(owned)
    for target in boundary:
        transitions += 1
        following = owned | {target}
        if following not in seen:
            seen.add(following); todo.append(following)
assert terminal == {frozenset(ordinary)}
assert {len(state) for state in seen} == set(range(11))
def selection(owned, native_radio):
    boundary = candidates(owned)
    eligible = boundary & native_radio
    if eligible:
        return 'READY', sorted(eligible)[0]
    if boundary:
        return 'RADIO_WAIT', None
    return 'NO_ENEMY_REMAINING', None

for owned in seen:
    if len(owned) == 10:
        continue
    assert selection(owned, set()) == ('RADIO_WAIT', None)  # I, no terminal outcome
    state, objective = selection(owned, ordinary)  # J, native event reevaluation
    assert state == 'READY' and objective in candidates(owned)
assert 'm_aFriendly.Clear(); m_aCandidates.Clear()' in evaluator
assert 'm_iState = TB_CC_M4_3_EvaluationState.RADIO_WAIT' in evaluator
assert 'adjacentId == m_sSelectedMobId || adjacentId == m_sSelectedEndpointId' in evaluator
assert 'if (!radioReachable) { m_iRadioRejectedCandidates++; continue; }' in evaluator
assert 'Reevaluate("native-radio-topology-changed")' in native
assert 'CallLater(RetryRadioObjective, 10000)' in native
print(f'CAMPAIGN H-J MODEL PASS: 12 nodes / 21 edges / 10 ordinary; {len(seen)} ownership subsets / {transitions} candidate transitions; all paths reach 10/10')
print('FIA RECAPTURE / CAMPAIGN CONTINUITY STATIC/API/MODEL PASS; WORKBENCH GATE REQUIRED; no runtime proof claimed')
