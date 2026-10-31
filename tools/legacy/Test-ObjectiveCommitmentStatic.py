"""Offline commitment source/model checks; never launches or mutates the game."""
from pathlib import Path
import itertools
import runpy

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
baseline = runpy.run_path(str(TOOLS / 'Test-WarSimulationStatic.py'))
core = (ROOT / 'Scripts/Game/TB_CC_Core/TB_CC_ObjectiveCommitment.c').read_text()
native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
assert all(word not in core for word in ('ARLAND', 'MILITARY_SOUTH', 'MOSS_HILL', 'NativeArland'))
assert 'else return;' in core
assert 'currentOwner == "US"' in core
assert '!terminal && !recommendation.IsEmpty()' in core
for event in ('objective-locked', 'objective-retained', 'objective-completed',
              'objective-invalidated', 'objective-reselected'):
    assert event in core
assert 'revision != m_iLastRetainedRevision' in core
structural = core.split('static string StructuralInvalidation')[1]
assert 'GetResolvedEntity()' in structural and 'entity.IsDeleted()' in structural
assert 'GetConnections(current, adjacent)' in structural
assert 'GetBLUFORCandidateObjectives' not in structural
assert 'GetFriendlyComponent' not in structural
assert 'owners.Get(' not in structural  # not an ownership-dependent path
getter = native.split('override string GetBLUFORLegalObjective()')[1].split('protected ref TB_CC_ObjectiveCommitment')[0]
assert 'm_USObjectiveCommitment.GetCurrentId()' in getter
assert 'm_Evaluator.GetBLUFORLegalObjective()' not in getter
publication = native.split('override protected void PublishIfChanged(string reason)')[1].split('override string GetOPFORLegalObjective()')[0]
assert publication.index('m_USObjectiveCommitment.Resolve(') < publication.index('super.PublishIfChanged(reason)')
assert publication.index('super.PublishIfChanged(reason)') < publication.index('m_USObjectiveCommitment.Trace(m_iRevision)')
assert 'm_USObjectiveCommitment.NeedsSelection(' in publication
bridge = native.split('protected void SyncObjectiveTask()')[1].split('protected void LogBridgePending')[0]
assert bridge.index('legalId == m_sPresentedObjectiveId') < bridge.index('taskSystem.DeleteTask(oldTask)')
assert 'm_iRevision == m_iPresentedRevision' not in bridge
assert 'if (oldTask == m_CurrentObjectiveTask)' in bridge
assert 'taskSystem.SetTaskState(oldTask, endState)' in bridge

# Exact supported vanilla task API/lifecycle, not externally inferred names.
task_system = baseline['resource']('scripts/Game/Tasks/SCR_TaskSystem.c')
task_entity = baseline['resource']('scripts/Game/Tasks/Campaign/SCR_CampaignMilitaryBaseTaskEntity.c')
assert 'void SetTaskState(notnull SCR_Task task, SCR_ETaskState state)' in task_system
assert 'void DeleteTask(notnull SCR_Task task)' in task_system
assert 'SetTaskState(SCR_ETaskState.COMPLETED);' in task_entity
assert 'm_TaskSystem.SetTaskState(this, SCR_ETaskState.CANCELLED);' in task_entity

# These are independent Python models, NOT executing the Enforce implementation.
def resolve(current, recommendation, owner, terminal=False, invalidation=''):
    if current:
        if owner == 'US' or terminal or invalidation:
            current = ''
        else:
            return current
    if not terminal and recommendation:
        current = recommendation
    return current

# 1: A retaken while attacking B; scoring/radio/frontier changes don't retask.
assert resolve('B', 'A', 'FIA') == 'B'
assert resolve('B', 'A', 'USSR') == 'B'
assert resolve('B', '', 'FIA') == 'B'  # transient radio/candidate exclusion
# 2: B captured; today's recommendation A is consumed exactly now.
assert resolve('B', 'A', 'US') == 'A'
# 3: another legitimate US ownership event has the same completion semantics.
assert resolve('B', 'C', 'US') == 'C'
# 4: unrelated USSR change/re-ranking cannot replace B.
for owner, recommendation in itertools.product(('FIA', 'USSR'), ('', 'A', 'B', 'C')):
    assert resolve('B', recommendation, owner) == 'B'
# 5: terminal state ends even when a stale recommendation exists.
for owner, recommendation in itertools.product(('US', 'FIA', 'USSR'), ('', 'A', 'B')):
    assert resolve('B', recommendation, owner, terminal=True) == ''
for invalid in ('position-removed', 'authoritative-entity-unavailable',
                'structural-graph-path-unavailable', 'campaign-invalid'):
    assert resolve('B', '', 'FIA', invalidation=invalid) == ''
assert resolve('', 'B', '') == 'B'

# Structural path remains valid when all intermediate ownership is hostile.
def connected(graph, start, target):
    pending, visited = [start], set()
    while pending:
        node = pending.pop(0)
        if node == target:
            return True
        if node in visited:
            continue
        visited.add(node)
        pending.extend(n for n in graph.get(node, ()) if n not in visited)
    return False
assert connected({'M': ['A'], 'A': ['M', 'B'], 'B': ['A']}, 'M', 'B')
assert not connected({'M': ['A'], 'A': ['M'], 'B': []}, 'M', 'B')

# Lifecycle model: revisions/candidate changes leave the same task identity;
# completed B creates one replacement, duplicate evaluation creates none.
objective, task, creates, clears = 'B', 'task-B', 1, 0
for recommendation, owner in (('A', 'FIA'), ('C', 'USSR'), ('', 'FIA'), ('A', 'US'), ('C', 'FIA')):
    next_objective = resolve(objective, recommendation, owner)
    if next_objective != objective:
        clears += 1
        creates += int(bool(next_objective))
        task = 'task-' + next_objective if next_objective else None
    objective = next_objective
assert (objective, task, creates, clears) == ('A', 'task-A', 2, 1)
print('OBJECTIVE COMMITMENT SOURCE/API/5-CASE/MODEL PASS; NOT Workbench or runtime PASS')
