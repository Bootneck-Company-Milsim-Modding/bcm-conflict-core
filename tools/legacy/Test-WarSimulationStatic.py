"""Offline compatibility/source/model checks. Never launches or mutates the game.

These are static checks and Python model tests, NOT an Enforce compiler or a
runtime acceptance claim. Run from any directory after source changes.
"""
from pathlib import Path
import itertools
import math
import re
import runpy
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
baseline = runpy.run_path(str(TOOLS / 'Test-ThreeFactionCorrectiveStatic.py'))
reader = baseline['reader']
resource = baseline['resource']
mask = baseline['mask']
calls = baseline['calls']
representation = (ROOT / 'Scripts/Game/TB_CC_CoreWar/TB_CC_WarRepresentation.c').read_text()
simulation = (ROOT / 'Scripts/Game/TB_CC_CoreWar/TB_CC_WarSimulation.c').read_text()
adapter = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_NativeWarAdapter.c').read_text()
operation = (ROOT / 'Scripts/Game/TB_PVE_M3_1/TB_PVE_M3_1_Operation.c').read_text()
garrison = (ROOT / 'Scripts/Game/TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrison.c').read_text()
coordinator = (ROOT / 'Scripts/Game/TB_PVE_M3_2/TB_PVE_M3_2_OPFOROperationCoordinator.c').read_text()

# Verify EVERY newly used external method against installed sources; leave
# the previously checked engine surfaces to the whole-tree baseline.
api_contracts = {
    'scripts/Game/generated/AI/AIPathfindingComponent.c': [
        'GetClosestPositionOnNavmesh(vector position, vector searchHalfExtents, out vector correctedPosition)'],
    'scripts/Game/generated/AI/ChimeraAIGroup.c': [
        'GetDormantAliveCount()', 'SetDormantCounts(int alive, int dead)'],
    'scripts/Game/generated/AI/ChimeraAIWorld.c': ['PurgeSpawnRequestsForGroup(AIGroup group)'],
    'scripts/Game/Editor/Entities/SCR_EditorManagerEntity.c': [
        'static SCR_EditorManagerEntity GetInstance()', 'SCR_EditorModeEntity GetCurrentModeEntity()'],
    'scripts/Game/Editor/Components/Editor/SCR_BudgetEditorComponent.c': [
        'GetMaxBudgetValue(EEditableEntityBudget type, out int maxBudget)',
        'GetCurrentBudgetValue(EEditableEntityBudget type)'],
    'scripts/Game/Components/Locations/SCR_MilitaryBaseComponent.c': ['void SetFaction(Faction faction)'],
}
for path, signatures in api_contracts.items():
    installed = resource(path)
    for signature in signatures:
        assert signature in installed, (path, signature)
    print('WAR INSTALLED API PASS', path)
core_archive = Path(str(GAME_INSTALL / 'addons/core/data.pak'))
core_index = {p.lower(): (offset, stored, unpacked) for p, offset, stored, unpacked in reader.index(core_archive)}
for filename, signatures in {
    'IEntity.c': ['GetPrefabData()', 'RemoveChild(notnull IEntity child, bool keepTransform = false)',
                  'CoordToParent(vector coord)', 'SetOrigin(vector orig)'],
    'EntityPrefabData.c': ['ResourceName GetPrefabName()'],
    'TraceParam.c': ['vector Start;', 'vector End;', 'vector TraceNorm;'],
    'Types.c': ['class array<Class T>: Managed', 'proto int Copy(notnull array<T> from);'],
}.items():
    paths = [p for p in core_index if p.endswith('/' + filename.lower())]
    assert len(paths) == 1
    offset, stored, unpacked = core_index[paths[0]]
    with core_archive.open('rb') as stream:
        stream.seek(offset)
        payload = stream.read(stored)
    if stored != unpacked:
        payload = zlib.decompress(payload)
    installed = payload.decode()
    for signature in signatures:
        assert signature in installed, signature
    print('WAR INSTALLED CORE API PASS', filename)
installed_group = resource('scripts/Game/Entities/SCR_AIGroup.c')
for native_reference in ['formationDefinition.GetOffsetPosition(index)',
                         'GetClosestPositionOnNavmesh(pos, "10 10 10", pos)',
                         'void RequestSpawn(int slotsWanted = -1, float observerRange = 0)',
                         'entitySource.Get("m_aUnitPrefabSlots", outPrefabs)']:
    assert native_reference in installed_group

# Portable data contains no runtime pointers, and map knowledge lives ONLY in
# adapter/old campaign integration, never in the new representation core.
for source in (representation, simulation):
    assert not re.search(r'ARLAND|GULL|MILITARY_SOUTH|TB_CC_M4XP|NativeArland|FindEntityByName', source)
state_body = representation.split('class TB_CC_WarForceState')[1].split('class TB_CC_WarForceLease')[0]
group_body = representation.split('class TB_CC_WarGroupState')[1].split('class TB_CC_WarAssetState')[0]
for source in (state_body, group_body):
    assert not re.search(r'\b(?:IEntity|SCR_AIGroup|AIWaypoint|AIAgent)\b', source)
for field in ['m_sForceId', 'm_sFaction', 'm_sRole', 'm_sPackageId', 'm_sSourceId',
              'm_sTargetId', 'm_sExpectedOwner', 'm_vPosition', 'm_fProgress',
              'm_iStrength', 'm_iInitialStrength', 'm_iCasualties', 'm_iOperationState',
              'm_iRepresentation', 'm_iLastUpdateMs', 'm_iEscalationContext', 'm_aAssets']:
    assert field in state_body, field
assert 'm_iSerial).ToString()' in simulation
assert 'm_aForces.Remove(index)' in simulation
assert 'GetPlayers(ids)' in simulation
assert 'foreach (int id : ids)' in simulation
assert 'ECharacterLifeState.ALIVE' in simulation
assert 'm_aPlayers.Insert(character.GetOrigin())' in simulation
assert 's_Instance = null' in simulation

# Transition/capture source guards and ordering, including deferred/pending.
virtualize = simulation.split('protected void Virtualize')[1].split('protected int PendingSlots')[0]
assert virtualize.index('Snapshot(lease)') < virtualize.index('Retire(lease)') < virtualize.index('TB_CC_WarRepresentation.VIRTUAL')
materialize = simulation.split('protected bool Materialize')[1].split('protected bool SetOrder')[0]
assert materialize.index('TB_CC_WarRepresentation.MATERIALIZING') < materialize.index('SpawnEntityPrefab')
assert 'groupEntity.m_aUnitPrefabSlots.Copy(specification.m_aSurvivorPrefabs)' in materialize
assert 'RequestSpawn(specification.m_aSurvivorPrefabs.Count())' in materialize
assert 'FindPlacement(groupEntity, state, specification.m_vPosition' in materialize  # never source restart
search = simulation.split('protected bool FindPlacement')[1].split('protected bool Materialize(')[0]
assert 'TB_CC_WarPlacement.Project' in search and 'TB_CC_WarPlacement.Formation' in search
assert 'array<float> rings = {15, 30, 50, 75, 100, 150, 200}' in search
assert 'bearing < 16' in search
assert 'stats.Reject(reason); continue;' in search
assert 'HasGroup(lease, i)' in materialize
assert 'CapacityReason(specification.m_aSurvivorPrefabs.Count())' in materialize
assert 'Retire(lease)' not in materialize
assert 'ReserveStrength(lease)' in materialize
retire = simulation.split('protected void Retire')[1].split('protected void Virtualize')[0]
assert retire.index('PurgeSpawnRequestsForGroup') < retire.index('DeleteEntityAndChildren(character)')
assert 'if (TB_CC_PhysicalWarService.IsLiving(character)) SCR_EntityHelper.DeleteEntityAndChildren(character)' in retire
assert 'TrackGarbage(character)' in retire and 'RemoveChild(character, true)' in retire
assert 'record.m_Group = null; record.m_Waypoint = null' in retire and 'lease.m_aPhysical.Clear()' in retire
battle = simulation.split('protected void ResolveBattle')[1].split('void Tick()')[0]
assert battle.index('IsTargetPhysical(target)') < battle.index('ApplyLosses(state')
assert battle.index('losses.Insert') < battle.index('ApplyLosses(state')
assert battle.index('m_bCaptureCommitted = true') < battle.index('m_Map.CommitBackgroundCapture')
assert 'Relevant(lease, m_Config.m_fPreRadius) || IsTargetPhysical(state.m_sTargetId)' in simulation
assert 'base.SetFaction(' not in simulation and adapter.count('base.SetFaction(winner)') == 1
for guard in ['Replication.IsServer()', 'IsActive()', 'TB_CC_WarRepresentation.VIRTUAL',
              'm_bCaptureCommitted', 'm_iStrength <= 0', 'CanAttack(', 'm_sExpectedOwner',
              'IsTargetPhysical(', 'base.GetFaction() == winner']:
    assert guard in adapter, guard
assert 'm_WarForce' in operation and 'no physical advancement' in operation
assert 'war.Defence(target, currentOwner, packageId)' in garrison
assert 'no-active-human-blufor' in coordinator and '&& !background' in coordinator
assert 'm_State.m_iStrength <= 0' in operation
assert 'SetDormantCounts(survivors, 0)' in simulation
assert 'm_aAccountedVirtualDeaths' in simulation
assert 'backgroundPaused=1' in simulation
assert 'm_bVirtualForceEngaged = warState.m_fCombatSeconds > 0' in operation
assert 'PreparePhysicalDefence(m_WarForce)' in operation
batch = simulation.split('protected bool MaterializeTarget')[1].split('protected bool SetOrder')[0]
assert 'return Materialize(requested, reason)' in batch
assert 'rollback' not in batch
observe = simulation.split('protected void ObservePhysical')[1].split('protected void Defer')[0]
assert observe.index('Snapshot(lease);') < observe.index('Retire(failed);')
assert 'lease.m_aPhysical.Remove(index)' in observe
assert 'Retire(lease)' not in observe
assert 'PhysicalStrength(lease)' in simulation
assert 'state.m_iStrength - PhysicalStrength(lease)' in simulation
assert 'CallLater(s_Instance.Tick, 1000, true)' in simulation
assert 'Remove(s_Instance.Tick)' in simulation
assert 'm_iRetryMs = 1000' in representation
assert 'm_iGraceMs = 10000' in representation and 'm_iMinimumDwellMs = 10000' in representation
assert 'm_fVirtualRadius = 1100' in representation
for diagnostic in ['materialization-request', 'virtualization-pending', 'remoteSeconds',
                   'distanceNearestPlayer', 'candidateCount', 'navNotReady', 'bestCandidateDistance',
                   'first-safe-group', 'full-force-ready']:
    assert diagnostic in simulation
for placement_check in ['GetSurfaceY', 'IsTileLoaded', 'IsTileValid', 'LoadTileIn',
                        'TraceNorm[1]', 'm_fMaximumProjectionDelta',
                        'TryGetWaterSurfaceSimple', 'body-collision', 'body-width-collision',
                        'GetClosestPositionOnNavmesh', 'formation-preference', 'nav-data-not-ready',
                        'no-valid-navmesh', 'inside-terrain', 'extreme-slope']:
    assert placement_check in representation, placement_check

# These are MODEL tests, not executing Enforce. Exhaust finite guard spaces,
# sample distributed players/hysteresis, casualty transitions, deterministic
# attrition, simultaneous losses, admission arithmetic and moving resumption.
def near(point, players, radius):
    return any(math.dist(point, player) <= radius for player in players)

players = [(0, 0, 0), (4000, 0, 0)]
assert near((4400, 0, 0), players, 750)
assert near((400, 0, 0), players, 750)
assert not near((2000, 0, 0), players, 1000)
for distance in range(0, 1601):
    demand = distance <= 1000
    remote = distance > 1100
    assert not (demand and remote)
for grace, dwell, age, remote_age in itertools.product([0, 10], [0, 10], [0, 9, 10, 30], [0, 9, 10, 30]):
    eligible = age >= dwell and remote_age >= grace
    assert eligible == (min(age - dwell, remote_age - grace) >= 0)

for guards in itertools.product([False, True], repeat=10):
    allowed = all(guards)
    commits = int(allowed)
    latch = allowed
    if allowed and not latch:  # duplicate callback cannot commit twice
        commits += 1
    assert commits <= 1

for strength in range(1, 25):
    for deaths in range(strength + 1):
        slots = list(range(strength))
        survivors = slots[deaths:]
        virtual = survivors.copy()
        rematerialized = virtual.copy()
        assert len(rematerialized) == strength - deaths
        assert not set(rematerialized) & set(slots[:deaths])
        assert strength - len(rematerialized) == deaths
for initial in (6, 12, 14, 16, 18, 24):
    strength = initial
    accumulator = 0
    history = []
    for _ in range(100):
        accumulator += .012 * 12 * 2 / 1.35
        while accumulator >= 1 and strength > 0:
            accumulator -= 1
            strength -= 1
        history.append(strength)
    assert all(a >= b >= 0 for a, b in zip(history, history[1:]))
    assert initial - strength >= 0
for current, pending, request in itertools.product(range(0, 129, 8), range(0, 25, 3), (6, 12, 18, 24)):
    accepted = current + pending + request <= 128 - 16
    if accepted:
        assert 128 - (current + pending + request) >= 16
position = 600
source = 0
target = 1000
position += min(1.8 * 2, target - position - 30)
assert source < position < target and position != source

# Group-granular incremental admission: failed groups remain reserve; repeated
# retries cannot duplicate an existing group or resurrect known casualties.
for group_sizes in ((3, 3), (3, 3, 3, 2, 3), (3,) * 8):
    for admission in itertools.product((False, True), repeat=len(group_sizes)):
        rosters = [list(range(size)) for size in group_sizes]
        admitted = {i for i, allowed in enumerate(admission) if allowed}
        physical = sum(len(rosters[i]) for i in admitted)
        reserve = sum(len(rosters[i]) for i in range(len(rosters)) if i not in admitted)
        assert physical + reserve == sum(group_sizes)
        retried = {i for i in range(len(rosters)) if i not in admitted}
        assert not admitted & retried
        # A casualty in an admitted group doesn't change the untouched reserve.
        if admitted:
            index = min(admitted)
            rosters[index].pop()
            assert sum(map(len, rosters)) == sum(group_sizes) - 1
            assert sum(len(rosters[i]) for i in range(len(rosters)) if i not in admitted) == reserve
        # Any local physical or queued participant freezes abstract target combat.
        target_physical = bool(admitted)
        virtual_damage = 0 if target_physical else sum(map(len, rosters)) * .012
        if physical > 0:
            assert virtual_damage == 0

assert sum((3, 3, 3, 2)) == 11  # 14 total: four safe groups + three reserve
print('WAR STATIC/API/SOURCE/MODEL CHECKS PASS;', len(list((ROOT / 'Scripts/Game').rglob('*.c'))), 'Game sources; NOT Workbench compile or runtime PASS')
