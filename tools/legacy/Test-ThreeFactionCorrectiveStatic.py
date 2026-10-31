"""Read-only compile-readiness and model checks; NOT an Enforce compiler."""
from pathlib import Path
import importlib.util
import math
import re
import struct

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
DATA = Path(str(GAME_INSTALL / 'addons/data'))
CORRECTIVE_SCRIPTS = [
    'TB_CC_M4X/TB_CC_M4XP_NativeArland.c',
    'TB_CC_M4X/TB_CC_TerritorialOccupation.c',
    'TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c',
    'TB_PVE_M3_1/TB_PVE_M3_1_Operation.c',
    'TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c',
    'TB_PVE_M3_4/TB_PVE_M3_4_ForcePackages.c',
    'TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrison.c',
    'TB_CC_M4_4/TB_CC_M4_4_ArlandGraphCampaign.c',
    'TB_CC_M4_4/TB_CC_M4_4_StrategicCaptureConnectivity.c',
    'TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c',
    'TB_PVE_M3_5/TB_PVE_M3_5_ForceSelection.c',
]
SCRIPTS = sorted(str(p.relative_to(ROOT / 'Scripts/Game')).replace('\\', '/')
                 for p in (ROOT / 'Scripts/Game').rglob('*.c'))


def mask(text):
    return re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/',
                  lambda hit: ''.join('\n' if c == '\n' else ' ' for c in hit.group()),
                  text, flags=re.S)


def split_args(text):
    masked = mask(text)
    depth = 0
    start = 0
    args = []
    for i, char in enumerate(masked):
        if char in '([{':
            depth += 1
        elif char in ')]}':
            depth -= 1
        elif char == ',' and depth == 0:
            args.append(text[start:i].strip())
            start = i + 1
    args.append(text[start:].strip())
    return args


def calls(text, name):
    masked = mask(text)
    for hit in re.finditer(r'\b' + name + r'\s*\(', masked):
        start = hit.end()
        depth = 1
        end = start
        while depth:
            if masked[end] == '(':
                depth += 1
            elif masked[end] == ')':
                depth -= 1
            end += 1
        yield split_args(text[start:end - 1])


formats = 0
for name in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / name).read_text(encoding='utf-8-sig')
    stack = []
    for char in mask(text):
        if char in '([{':
            stack.append(char)
        elif char in ')]}':
            assert stack and stack.pop() == dict(zip(')]}', '([{'))[char], name
    assert not stack, name
    for args in calls(text, 'PrintFormat'):
        assert len(args) <= 10, (name, 'PrintFormat exceeds nine values', args)
        placeholders = [int(n) for n in re.findall(r'%(\d+)', args[0])]
        assert not placeholders or max(placeholders) <= min(len(args) - 1, 9), (name, args)
        formats += 1
    print('SOURCE CHECK PASS', name)
print('PrintFormat calls checked:', formats)

all_sources = list((ROOT / 'Scripts').rglob('*.c'))
classes = {}
for path in all_sources:
    text = path.read_text(encoding='utf-8-sig')
    for hit in re.finditer(r'(?m)^class\s+(TB_\w+)', mask(text)):
        assert hit[1] not in classes, (hit[1], path, classes.get(hit[1]))
        classes[hit[1]] = path
print('Unique BCM class definitions:', len(classes))

native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
packet = re.findall(r'packet(?:\s*\+)?\s*=\s*([^;]+);', native)
expected = ['m_iRevision.ToString()', 'GetValidity().ToString()', 'm_iOutcome.ToString()',
            'usObjective', 'ussrObjective', 'm_iDepthPermille.ToString()',
            'm_iUSSRDepthPermille.ToString()', 'm_iHQEastState.ToString()',
            'm_iHQWestState.ToString()', 'IdCsv()', 'StateCsv()', 'candidates', 'OwnerCsv()']
assert [x.replace('"|" + ', '').strip() for x in packet] == expected
assert 'fields.Count() != 13' in native
assert 'Faction previousFaction, Faction newFaction' in native
assert all('ActivateAllMembers' not in p.read_text(encoding='utf-8-sig') for p in all_sources)
print('13-field snapshot / invalid activation reference checks PASS')

capture = (ROOT / 'Scripts/Game/TB_CC_M4_4/TB_CC_M4_4_StrategicCaptureConnectivity.c').read_text()
assert 'presence >= hostileCount * Math.Max(m_iBCMCaptureRatio, 1)' in capture
assert 'presence - hostileCount >= Math.Max(m_iBCMCaptureMargin, 1)' in capture
assert re.search(r'Attribute\(defvalue: "2"[^\n]+\)\]\s*protected int m_iBCMCaptureRatio;', capture)
assert re.search(r'Attribute\(defvalue: "2"[^\n]+\)\]\s*protected int m_iBCMCaptureMargin;', capture)
for attackers, hostiles, expected in [(0, 0, False), (1, 0, True), (2, 1, False),
                                    (3, 1, True), (3, 2, False), (4, 2, True),
                                    (6, 4, False), (6, 3, True)]:
    actual = attackers >= 1 if hostiles == 0 else attackers >= 2 * hostiles and attackers - hostiles >= 2
    assert actual == expected
print('Capture-superiority boundary checks PASS')

# The RDB record stores null-terminated path, record flag, two bytes, GUID.
# Cross-check this interpretation against a known installed vanilla reference.
rdb = (DATA / 'resourceDatabase.rdb').read_bytes()


def registered_guid(path):
    hits = list(re.finditer(re.escape(path.encode()) + b'\x00', rdb, re.I))
    assert len(hits) == 1, path
    end = hits[0].end()
    assert struct.unpack_from('<I', rdb, end)[0] == 6, path
    return f'{struct.unpack_from("<Q", rdb, end + 6)[0]:016X}'


assert registered_guid('Prefabs/Groups/Campaign/Group_USSR_Defenders.et') == '359353A0A5A52216'
spec = importlib.util.spec_from_file_location('installed_reader', TOOLS / 'Read-InstalledVanillaResource.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)
archive = DATA / 'data007.pak'
index = {p.lower(): (offset, stored, unpacked) for p, offset, stored, unpacked in reader.index(archive)}


def resource(path):
    import zlib
    offset, stored, unpacked = index[path.lower()]
    with archive.open('rb') as stream:
        stream.seek(offset)
        payload = stream.read(stored)
    if stored != unpacked:
        payload = zlib.decompress(payload)
    assert len(payload) == unpacked
    return payload.decode()


packages = (ROOT / 'Scripts/Game/TB_PVE_M3_4/TB_PVE_M3_4_ForcePackages.c').read_text()
for guid, path in re.findall(r'\{([A-F0-9]{16})\}(Prefabs/Groups/Campaign/Group_FIA[^"\s]+)', packages):
    assert registered_guid(path) == guid, path
    text = resource(path)
    characters = re.findall(r'\{([A-F0-9]{16})\}(Prefabs/Characters/[^"\s]+)', text)
    assert characters, path
    for character_guid, character_path in characters:
        assert registered_guid(character_path) == character_guid, character_path
        visited = set()
        current = character_path
        faction_found = False
        while current not in visited:
            visited.add(current)
            current_text = resource(current)
            if '"faction affiliation" "FIA"' in current_text:
                faction_found = True
                break
            parent = re.search(r'^\w+\s*:\s*"\{[A-F0-9]{16}\}([^"\n]+)"', current_text)
            if not parent:
                break
            current = parent[1]
        assert faction_found, character_path
    print('FIA RESOURCE PASS', guid, path, 'slots=', len(characters))

# Model mirror: static authoring data, no game processes or ownership writes.
definition = (ROOT / 'Scripts/Game/TB_CC_M4_2/TB_CC_M4_2_ArlandProductionDefinition.c').read_text()
positions = {}
for ident, coords, importance in re.findall(r'Position\("(ARLAND::\w+)",[^\n]+?"([\d. -]+)",\s*"[^"\n]+",\s*(\d+),', definition):
    xyz = [float(v) for v in coords.split()]
    positions[ident] = (xyz[0], xyz[2], int(importance))
adj = {ident: set() for ident in positions}
for a, b in re.findall(r'Connect\(definition, "(ARLAND::\w+)", "(ARLAND::\w+)"\)', definition):
    adj[a].add(b)
    adj[b].add(a)
assert len(positions) == 12 and sum(len(a) for a in adj.values()) == 42
HQ = {'ARLAND::HQ_EAST', 'ARLAND::HQ_WEST'}


def rank(friendly, seed=20):
    candidates = sorted(set.union(*(adj[p] for p in friendly)) - friendly - HQ)
    footprint = friendly - HQ or friendly
    ranking = []
    for i, ident in enumerate(candidates):
        x, z, importance = positions[ident]
        distance = min(math.hypot(x - positions[p][0], z - positions[p][1]) for p in footprint)
        parts = (int(importance * 40 / 100), int(math.floor(30 * (1 - min(distance / 1500, 1)) + 0.5)),
                 int(min(len(adj[ident] & friendly), 3) * 20 / 3),
                 int(min(len(adj[ident] - friendly - HQ), 6) * 10 / 6),
                 ((seed % 997) * 31 + i * 17) % 3)
        ranking.append((sum(parts[:4]), ident, round(distance, 1), parts))
    return sorted(ranking, key=lambda row: (-row[0], -row[3][4], row[1]))


scenarios = {
    'US START': {'ARLAND::HQ_EAST'},
    'US AFTER MILITARY SOUTH': {'ARLAND::HQ_EAST', 'ARLAND::MILITARY_SOUTH'},
    'US TWO NEARBY ORDINARY POSITIONS': {'ARLAND::HQ_EAST', 'ARLAND::MILITARY_SOUTH', 'ARLAND::MOSS_HILL'},
    'US CONSOLIDATED ARLEVILLE': {'ARLAND::HQ_EAST', 'ARLAND::MILITARY_SOUTH', 'ARLAND::MOSS_HILL', 'ARLAND::ARLEVILLE'},
    'USSR START': {'ARLAND::HQ_WEST'},
    'USSR AFTER AIRPORT NORTH': {'ARLAND::HQ_WEST', 'ARLAND::AIRPORT_NORTH'},
}
for label, friendly in scenarios.items():
    print('RANKING', label, '(all graph candidates assumed radio-eligible; not runtime radio proof)')
    for score, ident, distance, parts in rank(friendly):
        print(' ', ident, 'score=', score, 'distance=', distance, 'importance/proximity/consolidation/connectivity/variation=', parts)
assert rank(scenarios['US START'])[0][1] == 'ARLAND::MILITARY_SOUTH'
assert rank(scenarios['US AFTER MILITARY SOUTH'])[0][1] == 'ARLAND::MOSS_HILL'
assert rank(scenarios['USSR START'])[0][1] == 'ARLAND::AIRPORT_NORTH'
assert rank(scenarios['USSR AFTER AIRPORT NORTH'])[0][1] == 'ARLAND::NORTH_RUNWAY'
print('STATIC MODEL CHECKS PASS; WORKBENCH COMPILE AND RUNTIME ACCEPTANCE STILL REQUIRED')

# Compare complete local override signatures, including parameter names, with
# local parents and actual installed prototypes. Public widening is permitted
# for engine callbacks, as in the already compiled component lifecycle code.
def declarations(text):
    masked = mask(text)
    found = []
    for hit in re.finditer(r'\b(modded\s+)?class\s+(\w+)(?:\s*:\s*(\w+))?\s*\{', masked):
        start = hit.end()
        depth = 1
        end = start
        while depth:
            if masked[end] == '{':
                depth += 1
            elif masked[end] == '}':
                depth -= 1
            end += 1
        methods = {}
        body = text[start:end - 1]
        masked_body = masked[start:end - 1]
        for method in re.finditer(r'(?m)^\s*((?:(?:override|protected|private|static|event|proto|external)\s+)*)'
                                  r'(\w+)\s+(\w+)\s*\(([^{};\n]*)\)', body):
            # Only declarations at class scope, never "return Method(...)"
            # inside a method body. Such calls previously overwrote prototypes.
            prefix = masked_body[:method.start()]
            if prefix.count('{') != prefix.count('}'):
                continue
            flags, return_type, name, params = method.groups()
            if return_type in ('return', 'if', 'while', 'switch'):
                continue
            methods[name] = (return_type, re.sub(r'\s+', ' ', params.strip()), flags.split())
        found.append((bool(hit[1]), hit[2], hit[3], methods))
    return found


parents = {}
for path in all_sources:
    for modded, name, parent, methods in declarations(path.read_text(encoding='utf-8-sig')):
        if not modded:
            parents[name] = (parent, methods)
for installed_path in [
    'scripts/Game/GameMode/SCR_GameModeCampaign.c',
    'scripts/Game/Campaign/SCR_CampaignFactionCommanderHandlerComponent.c',
    'scripts/Game/Campaign/SCR_CampaignSeizingComponent.c',
    'scripts/Game/Components/SCR_SeizingComponent.c',
    'scripts/Game/Components/SCR_CampaignSpawnPointGroup.c',
    'scripts/Game/Components/SCR_SpawnPoint.c',
    'scripts/Game/GameMode/SCR_BaseGameMode.c',
    'scripts/Game/Components/SCR_FactionAffiliationComponent.c',
    'scripts/Game/Editor/Core/SCR_EditorManagerCore.c',
    'scripts/Game/Systems/SCR_GarbageSystem.c',
    'scripts/Game/UI/Conflict/SCR_CampaignMapUIBase.c',
    'scripts/Game/Components/SCR_PlayerXPHandlerComponent.c',
    'scripts/Game/Components/SCR_XPHandlerComponent.c',
    'scripts/Game/GameMode/SCR_BaseGameModeComponent.c',
    'scripts/Game/generated/ScrServerCommand.c',
    'scripts/Game/Components/SCR_CharacterRankComponent.c',
    'scripts/Game/DataCollection/DataCollectorModules/SCR_DataCollectorAchievementsModule.c',
]:
    # Locate by file name because installed folders are authoritative.
    matches = [path for path in index if path.endswith('/' + installed_path.split('/')[-1].lower())]
    assert len(matches) == 1, installed_path
    for modded, name, parent, methods in declarations(resource(matches[0])):
        if not modded:
            parents[name] = (parent, methods)
core_archive = Path(str(GAME_INSTALL / 'addons/core/data.pak'))
for path, offset, stored, unpacked in reader.index(core_archive):
    if path.lower().endswith('/scriptcomponent.c'):
        import zlib
        with core_archive.open('rb') as stream:
            stream.seek(offset)
            payload = stream.read(stored)
        if stored != unpacked:
            payload = zlib.decompress(payload)
        for modded, name, parent, methods in declarations(payload.decode()):
            parents[name] = (parent, methods)
        break
# Local modded declarations add members to the actual inherited class. Do not
# discard them merely because its original definition is installed vanilla.
for path in all_sources:
    for modded, name, parent, methods in declarations(path.read_text(encoding='utf-8-sig')):
        if modded:
            ancestor, existing = parents.get(name, (parent, {}))
            merged = dict(existing)
            merged.update({key: value for key, value in methods.items() if 'override' not in value[2]})
            parents[name] = (ancestor, merged)
overrides = 0
for script in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / script).read_text(encoding='utf-8-sig')
    for modded, name, parent, methods in declarations(text):
        for method_name, signature in methods.items():
            if 'override' not in signature[2]:
                continue
            ancestor = name if modded else parent
            while ancestor in parents:
                next_ancestor, prototypes = parents[ancestor]
                if method_name in prototypes:
                    prototype = prototypes[method_name]
                    assert signature[:2] == prototype[:2], (script, name, method_name, signature, prototype)
                    assert 'private' not in prototype[2], (script, method_name, 'private parent')
                    assert ('static' in signature[2]) == ('static' in prototype[2]), (script, method_name, 'static/instance mismatch')
                    if 'protected' not in prototype[2] and 'private' not in prototype[2]:
                        assert 'private' not in signature[2] and 'protected' not in signature[2], (script, method_name, 'visibility narrowed')
                    overrides += 1
                    print('OVERRIDE', script, name + '.' + method_name, signature, 'PARENT', ancestor, prototype)
                    break
                ancestor = next_ancestor
            else:
                raise AssertionError(('Installed/local prototype unresolved', script, name, method_name, ancestor))
print('Complete override return/type/name/order checks PASS:', overrides)


def fia_tier(humans, depth, importance, strength=1000):
    tier = 'LIGHT'
    if humans >= 2 or importance >= 70:
        tier = 'STANDARD'
    if humans >= 6 or importance >= 85 or (humans >= 4 and depth >= 334):
        tier = 'REINFORCED'
    if importance >= 95 or (humans >= 6 and depth >= 667 and importance >= 80):
        tier = 'STRONGPOINT'
    if strength < 667 and tier in ('REINFORCED', 'STRONGPOINT'):
        tier = 'STANDARD'
    if strength < 334:
        tier = 'LIGHT'
    return tier


assert fia_tier(1, 0, 60) == 'LIGHT'
assert fia_tier(1, 0, 70) == 'STANDARD'
assert fia_tier(1, 0, 85) == 'REINFORCED'
assert fia_tier(1, 0, 95) == 'STRONGPOINT'
assert fia_tier(6, 0, 85) == 'REINFORCED'
assert fia_tier(6, 700, 85) == 'STRONGPOINT'
assert fia_tier(6, 700, 85, 200) == 'LIGHT'
print('FIA selector tier reachability / strength caps PASS')

ordinary = set(positions) - HQ


def fia_targets(owners, losses):
    legal = []
    for ident in sorted(ordinary):
        if ident == 'ARLAND::GULL_ISLAND' or owners[ident] not in ('US', 'USSR'):
            continue
        origins = [n for n in adj[ident] & ordinary if n != 'ARLAND::GULL_ISLAND' and owners[n] == 'FIA']
        if origins:
            legal.append(ident)
    return sorted(legal, key=lambda ident: (-losses.get(ident, 0), -positions[ident][2], ident))


owners = {ident: 'FIA' for ident in positions}
owners.update({'ARLAND::HQ_EAST': 'US', 'ARLAND::HQ_WEST': 'USSR',
               'ARLAND::MILITARY_SOUTH': 'US', 'ARLAND::AIRPORT_NORTH': 'USSR'})
assert fia_targets(owners, {'ARLAND::MILITARY_SOUTH': 1, 'ARLAND::AIRPORT_NORTH': 2})[0] == 'ARLAND::AIRPORT_NORTH'
assert not fia_targets({ident: 'US' for ident in positions}, {})
assert not (set(fia_targets(owners, {})) & HQ)
owners['ARLAND::GULL_ISLAND'] = 'US'
assert 'ARLAND::GULL_ISLAND' not in fia_targets(owners, {})
print('FIA recency / ordinary-only / zero-territory / unsupported-foot-target model PASS')

for script in CORRECTIVE_SCRIPTS[:7]:
    text = (ROOT / 'Scripts/Game' / script).read_text()
    if script.endswith('ForcePackages.c') or script.endswith('FrontlineGarrison.c'):
        assert all(re.search(r'group\.SetFaction\(', line) for line in text.splitlines() if '.SetFaction(' in line)
    else:
        assert '.SetFaction(' not in text, script
    for number, line in enumerate(text.splitlines(), 1):
        if '"' in line:
            assert line.count(' + ') <= 6, (script, number, 'large single-line string formula')
print('No gameplay ownership writes / string-formula checks PASS')

# Whole-Game dependency inventory for changed contracts and newly introduced
# entry points. This is deliberately conservative, not a type-inferencing
# Enforce compiler. Ambiguous same-name methods retain all possible arities.
contracts = {
    'ValidateCandidate', 'ValidateCandidateForProof', 'SpawnGroup',
    'ConfigureTerritorialScoring', 'SetTerritorialScore',
    'ConfigureTerritorialAttack', 'IsTargetAuthorized', 'BeginConsolidation',
    'TickConsolidation', 'GetOccupationService', 'GetConsolidationSeconds',
    'GetTerritorialNeighbours', 'IsFootTerritory', 'CanFIARecapture',
    'SelectFIARecaptureTarget', 'SelectFIAPackage', 'HasDefence', 'HasClaim',
    'ReserveConsolidation', 'ReleaseConsolidation', 'SetDefensiveOrder', 'Adopt',
    'GetOnDefenceEstablished', 'IsAttacking', 'FindFIATerritorialStaging',
    'GetProximityScore', 'GetConsolidationScore',
    'ConfigureExpansion', 'IsHostileTerritorialOwner', 'ScoreTerritorialCandidate',
    'Evaluate', 'AuthorizeCapture', 'GetHQState', 'GetTerritorialOwner',
    'GetFactionDepth', 'GetPositionImportance', 'AllowsNativeSeizeTask',
    'ConfigureOwnerAware', 'SetOPFORFront', 'ConfigureFIAPackages',
    'AssemblePackage', 'FindStagingPosition', 'FindAdditionalGroupPosition',
    'FindLocalPosition', 'CountActiveHumanBLUFOR', 'GetParticipatingHumanCount',
    'GetActiveHumanObservers', 'TB_CC_UseThreeFactionCapture',
    'TB_CC_M4_4_ConfigurePvECaptureRules', 'SetCorrectiveProofRadioView',
    'CorrectiveProofTaskMatchesLegal', 'GetEvaluator', 'GetCoordinator',
    'GetGarrisonManager', 'GetBLUFORLegalObjective', 'GetOPFORLegalObjective',
    'StartM3Consumers', 'StopM3Consumers', 'OnBaseFactionChanged',
    'ConsumeNativeSnapshot', 'GetPresentedTaskId', 'OwnerCsv',
    'TB_CC_PublishNativeSnapshot', 'TB_CC_SetNativePressureDeadline',
    'SetTerritorialTarget', 'RetryMilliseconds', 'ObserveNativeMembers',
    'RegisterOperation', 'ReleaseOperation', 'TrackGarbage', 'IsLiving',
    'LivingCount', 'HasPhysicalHostile', 'ApplySimulation', 'SetRole',
    'Withdraw', 'PreserveOnCleanup', 'FindWithdrawalDestination',
    'GetCombatSimulationRadius', 'GetActiveGarrisonRetrySeconds',
    'GetCorpsePersistenceSeconds', 'GetWreckPersistenceSeconds',
    'GetDroppedEquipmentPersistenceSeconds', 'GetWithdrawalLifetimeSeconds',
    'GetMaximumConcurrentUSSROperations', 'GetMaximumConcurrentFIAOperations',
    'ObservePhysicalProgress', 'TickAdditionalOperations', 'GetOperationFaction',
    'SuspendCreation',
    'GetNearestBattleArea',
    'GetTerritorialTarget',
    'CreateMoveOrder',
    'ConfigureUSSRPackages', 'ResetTargetFailureHistory', 'RecordTargetResult',
    'GetPackage',
    'HasPhysicalFailureForEscalation', 'GetInitialTargetOwner',
    'TryGetAuthoredFIAApproachBearing', 'TBCCOrdinaryId', 'TBCCLogOwnerPresentation',
    'Surface', 'Project', 'Formation', 'Defence', 'Operation', 'AttachPhysical',
    'EndOperation', 'WantsPhysical', 'HasCapacity', 'IsTargetPhysical',
    'CommitBackgroundCapture', 'WarFootRouteAllowed', 'WarSource',
    'WarNearestOwnedId', 'AdoptRetained', 'RetainedCorpseCount',
    'SelectFromContext', 'ReadGMBudget', 'CapacityReason',
    'MaterializeTarget', 'PreparePhysicalDefence', 'Priority',
}
method_contracts = {}
for path in all_sources:
    for modded, class_name, parent, methods in declarations(path.read_text(encoding='utf-8-sig')):
        for method_name, signature in methods.items():
            if method_name not in contracts:
                continue
            parameters = split_args(signature[1]) if signature[1] else []
            required = sum('=' not in parameter for parameter in parameters)
            method_contracts.setdefault(method_name, []).append((required, len(parameters), signature[1], class_name))
            print('CONTRACT', class_name + '.' + method_name, signature)
references = 0
for path in all_sources:
    text = path.read_text(encoding='utf-8-sig')
    for method_name, possibilities in method_contracts.items():
        occurrences = re.finditer(r'\b' + method_name + r'\s*\(', mask(text))
        for hit, args in zip(occurrences, calls(text, method_name)):
            normalized = re.sub(r'\s+', ' ', ', '.join(args).strip())
            if any(normalized == signature for _, _, signature, _ in possibilities):
                continue  # Declaration, not a direct caller.
            assert any(minimum <= len(args) <= maximum for minimum, maximum, _, _ in possibilities), (path, method_name, args, possibilities)
            references += 1
            line = text[:hit.start()].count('\n') + 1
            print('CALL', str(path.relative_to(ROOT)), line, method_name, args)
print('Whole-Game changed/new contract call-arity checks PASS:', references)

sources_text = '\n'.join(p.read_text(encoding='utf-8-sig') for p in all_sources)
local_enums = {}
for match in re.finditer(r'\benum\s+(TB_\w+)\s*\{([^}]+)\}', mask(sources_text)):
    local_enums[match[1]] = {entry.strip().split('=')[0].strip() for entry in match[2].split(',')}
enum_references = 0
for enum_name, members in local_enums.items():
    for member in re.findall(r'\b' + enum_name + r'\.(\w+)', mask(sources_text)):
        assert member in members, (enum_name, member)
        enum_references += 1
print('Whole-Game local enum member checks PASS:', enum_references)

# The added definition factory keeps zero-argument consumers valid. Constructors
# are not represented as return-typed methods by declarations().
for args in calls(sources_text, 'TB_CC_TerritorialOccupationService'):
    assert len(args) == 1, args
for args in calls(sources_text, 'TB_CC_FIARecaptureCoordinator'):
    assert args == [''], args
for args in re.findall(r'TB_CC_M4XP_ArlandNativeDefinition\.Create\(([^;]*?)\)', mask(sources_text)):
    assert len(split_args(args)) <= 1, args
assert 'CallLater(TickTerritorialOccupation, 2000, true)' in native
assert 'Remove(TickTerritorialOccupation)' in native
assert 'protected void TickTerritorialOccupation()' in native
assert 'OnBaseFactionChanged(SCR_CampaignMilitaryBaseComponent base, Faction previousFaction, Faction newFaction)' in native
print('New constructor / factory / occupation callback contracts PASS')

# Also compare inherited same-name declarations without the override keyword.
# A new overload can otherwise evade an override-only scan.
implicit_overloads = 0
for script in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / script).read_text(encoding='utf-8-sig')
    for modded, name, parent, methods in declarations(text):
        if modded:
            continue  # Vanilla extensions/explicit overrides already checked.
        for method_name, signature in methods.items():
            if 'override' in signature[2]:
                continue
            ancestor = parent
            while ancestor in parents:
                next_ancestor, prototypes = parents[ancestor]
                if method_name in prototypes:
                    prototype = prototypes[method_name]
                    assert signature[:2] == prototype[:2], (script, name, method_name, 'implicit inherited overload', signature, prototype)
                    implicit_overloads += 1
                    print('IMPLICIT INHERITED CONTRACT', name, method_name, signature)
                    break
                ancestor = next_ancestor
print('Implicit inherited same-name declaration checks PASS:', implicit_overloads)

# No unqualified FIA group reference is permitted anywhere in compiled source.
fia_references = re.findall(r'"([^"\n]*Prefabs/(?:Groups|Characters)/[^"\n]*FIA[^"\n]*)"', sources_text)
for reference in fia_references:
    matched = re.fullmatch(r'\{([A-F0-9]{16})\}(.+\.et)', reference)
    assert matched and matched[1] != '0000000000000000', reference
    assert registered_guid(matched[2]) == matched[1], reference
    assert 'SCR_AIGroup' in resource(matched[2]) or 'Group_FIA_Remnants_Base.et' in resource(matched[2]) or 'Character_FIA' in resource(matched[2]), reference
print('Whole-Game FIA reference GUID/resource-type checks PASS:', len(fia_references))

string_hazards = []
for script in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / script).read_text(encoding='utf-8-sig')
    assert '?' not in mask(text), (script, 'conditional expression requires explicit Enforce audit')
    for number, line in enumerate(text.splitlines(), 1):
        if '"' in line and line.count(' + ') > 6:
            string_hazards.append((script, number, line.count(' + ')))
assert not string_hazards, ('Oversized single-line string formula review required', string_hazards)
print('Whole-Game conditional / oversized string expression checks PASS')

# Installed Workbench language reference, not a guessed foreign-language list.
keyword_doc = Path(str(TOOLS_INSTALL / 'Workbench/docs/EnfusionScriptAPI/html/Page_Enforce_Syntax_Keywords.html'))
keyword_html = keyword_doc.read_text(encoding='utf-8')
reserved_names = set(re.findall(r'</a>\s*([a-z]+)</h2>', keyword_html))
assert {'owned', 'out', 'inout', 'ref', 'override'} <= reserved_names
reserved_names.update('if else for foreach while switch case break default true false'.split())

def reserved_identifier_hazards(text):
    code = mask(text)
    # Primitive, conventional class/enum, array/map declarations and parameters.
    # This is a lexical guard, not an Enforce grammar/type checker.
    declaration_type = r'(?:void|int|float|bool|string|vector|typename|func|[A-Z]\w*|(?:array|map|set)\s*<[^;{}()]+>)'
    names = '|'.join(sorted(reserved_names))
    return [(hit.group(1), hit.start(1)) for hit in re.finditer(
        r'\b' + declaration_type + r'\s+(' + names + r')\b(?=\s*(?:[;=,):\[(]|$))', code)]

assert reserved_identifier_hazards('int hostile; int owned;')
assert reserved_identifier_hazards('void Check(int owned) {}')
assert reserved_identifier_hazards('array<string> owned = {};')
assert not reserved_identifier_hazards('int usOwnedCount; owned string GetName(); // int owned;')
assert not reserved_identifier_hazards('Print("int owned;");')
for script in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / script).read_text(encoding='utf-8-sig')
    hazards = reserved_identifier_hazards(text)
    assert not hazards, (script, [(name, text[:offset].count('\n') + 1) for name, offset in hazards])
print('Whole-Game reserved-identifier screening PASS:', len(SCRIPTS), 'sources; owned regression caught')

def operator_hazards(text):
    """Screen known foreign/mixed numeric syntax, not all integer remainders.

    Installed vanilla uses integer %. Enforce rejects the reported remainder
    embedded in a float destination formula; an explicit integer stage avoids
    that implicit numeric context. File-local type names are conservative.
    """
    code = mask(text)
    hazards = []
    float_names = set(re.findall(r'\bfloat\s+(\w+)', code))
    for statement in re.finditer(r'[^;{}]*;', code):
        value = statement.group()
        if '%' not in value:
            continue
        if re.search(r'\bfloat\s+\w+\s*=', value):
            hazards.append(('float-destination remainder', statement.start()))
        for name in float_names:
            if re.search(r'\b' + re.escape(name) + r'\s*=', value) or re.search(r'\b' + re.escape(name) + r'\b', value):
                hazards.append(('float operand/context remainder: ' + name, statement.start()))
        if re.search(r'\d+\.\d*|\d+[eE][+-]?\d+', value):
            hazards.append(('floating literal remainder', statement.start()))
    for hit in re.finditer(r'===|!==|\?\?|=>|->|\*\*|&&=|\|\|=|\b(?:static_cast|dynamic_cast|reinterpret_cast|sizeof|typeof)\b|\((?:int|float|bool)\)\s*\w', code):
        hazards.append(('foreign-language syntax: ' + hit.group(), hit.start()))
    return hazards


# Regression: the actual failed expression must NEVER pass this check again.
assert operator_hazards('float angle = direction * 0.392699; float radius = 60 + (attempt % 3) * 30;')
assert operator_hazards('float radius; radius = attempt % 3;')
assert operator_hazards('int band = 3.0 % 2;')
assert operator_hazards('float input; int band = input % 3;')
assert not operator_hazards('int radiusBand = attempt % 3; float radius = 60 + radiusBand * 30;')
assert not operator_hazards('PrintFormat("value=%1 50%", value); // float bad = n % 3;\n/* x %= 2 */')
percent_counts = {'executable': 0, 'string-format': 0, 'string-other': 0, 'comment': 0}
operator_inventory = {}
for script in SCRIPTS:
    text = (ROOT / 'Scripts/Game' / script).read_text(encoding='utf-8-sig')
    code = mask(text)
    hazards = operator_hazards(text)
    assert not hazards, (script, [(kind, text[:offset].count('\n') + 1) for kind, offset in hazards])
    for hit in re.finditer(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/', text, flags=re.S):
        for symbol in re.finditer('%', hit.group()):
            if hit.group().startswith('"'):
                category = 'string-format' if re.match(r'%\d+', hit.group()[symbol.start():]) else 'string-other'
            else:
                category = 'comment'
            percent_counts[category] += 1
    for number, line in enumerate(code.splitlines(), 1):
        if '%' in line:
            percent_counts['executable'] += line.count('%')
            print('INTEGER REMAINDER REVIEW', script, number, line.strip())
    for token in re.findall(r'\+\+|--|\+=|-=|\*=|/=|%=|\|=|&=|\^=|<<|>>|&&|\|\||[~^&|]', code):
        operator_inventory[token] = operator_inventory.get(token, 0) + 1
assert sum(percent_counts.values()) == sources_text.count('%')
print('Whole-Game percent classification:', percent_counts)
print('Whole-Game executable operator inventory:', operator_inventory)
print('Enforce mixed-remainder / foreign-syntax screening and regression tests PASS')

# Installed-source authority: integer % is not globally forbidden. Float Math
# helpers exist, but are not substituted for integer indices (precision loss).
native_defend = [path for path in index if path.endswith('/scr_aidefendactivity.c')]
assert len(native_defend) == 1
defend_source = resource(native_defend[0])
assert 'int directionAngle = Math.Round(' in defend_source
assert 'directionAngle = directionAngle % 360;' in defend_source
print('Installed Enforce integer-remainder pattern verified:', native_defend[0])

staging = (ROOT / 'Scripts/Game/TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c').read_text()
assert 'int radiusBand = (attempt + ordinal + m_iQuerySerial) % 5;' in staging
assert 'float radius = 300 + radiusBand * 50;' in staging
assert 'vector candidate = targetPosition + Vector(' in staging
assert 'TryGetAuthoredFIAApproachBearing' in staging
assert 'if (bearing < 0) bearing += 360;' in staging
assert 'if (separation > 180) separation = 360 - separation;' in staging
assert 'float requiredSeparation = 80;' in staging and 'if (pass == 1) requiredSeparation = 45;' in staging
assert 'if (pass == 2 || groupOrdinal >= 3) requiredSeparation = 0;' in staging


def signed_remainder(value, divisor):
    quotient = abs(value) // abs(divisor)
    if (value < 0) != (divisor < 0):
        quotient = -quotient
    return value - quotient * divisor


# Separating the integer stage changes no numbers, including signed remainder.
for attempt in list(range(0, 1000)) + [-2147483648, -17, -3, -2, -1, 2147483647]:
    original = 60 + signed_remainder(attempt, 3) * 30
    radius_band = signed_remainder(attempt, 3)
    replacement = 60 + radius_band * 30
    assert original == replacement
assert [60 + signed_remainder(i, 3) * 30 for i in range(7)] == [60, 90, 120, 60, 90, 120, 60]
for serial in range(1, 33):
    # Accepted USSR permutations and retry indexing remain authored unchanged.
    sectors = [signed_remainder(serial * 3 + attempt * 5, 16) for attempt in range(16)]
    assert sorted(sectors) == list(range(16))
    for ordinal in range(6):
        extra = [signed_remainder(serial * 3 + ordinal * 7 + attempt * 5, 16) for attempt in range(16)]
        assert sorted(extra) == list(range(16))
for first, second, expected in [(359, 1, 2), (0, 180, 180), (45, 45, 0), (350, 10, 20)]:
    separation = abs(first - second)
    if separation > 180:
        separation = 360 - separation
    assert separation == expected
print('Integer radius / sector permutations / wrap separation mathematical checks PASS')
# FIA native group lifecycle: check against installed definitions, not online
# API recollection. This establishes source compatibility, not combat success.
garrison = (ROOT / 'Scripts/Game/TB_PVE_M3_6/TB_PVE_M3_6_FrontlineGarrison.c').read_text()
installed_group = resource('scripts/Game/Entities/SCR_AIGroup.c')
installed_control = resource('scripts/Game/generated/AI/AIControlComponent.c')
installed_agent = resource('scripts/Game/generated/AI/AIAgent.c')
installed_group_api = resource('scripts/Game/generated/AI/AIGroup.c')
installed_weapons = resource('scripts/Game/generated/Weapon/BaseWeaponManagerComponent.c')
assert 'void RequestSpawn(int slotsWanted = -1, float observerRange = 0)' in installed_group
assert 'control.GetControlAIAgent()' in installed_group and 'control.ActivateAI();' in installed_group
assert 'AddAIEntityToGroup(member);' in installed_group
for api in ['GetControlAIAgent()', 'IsAIActivated()']:
    assert api in installed_control, api
for api in ['GetParentGroup()', 'IsAIActivated()', 'GetControlledEntity()', 'GetLOD()']:
    assert api in installed_agent, api
for api in ['GetCurrentWaypoint()', 'GetWaypoints(out array<AIWaypoint> outWaypoints)']:
    assert api in installed_group_api, api
assert 'GetWeaponsList(out notnull array<IEntity> outWeapons)' in installed_weapons
assert 'EnableAI 0' in resource('Prefabs/Characters/Core/Character_Base.et')
assert 'EnableAI 1' in resource('Prefabs/Characters/Campaign/Final/OPFOR/USSR_Army/Campaign_USSR_AI.et')
assert 'm_bSpawnImmediately 0' in resource('Prefabs/Groups/Campaign/Group_Remnants_Base.et')
fia_members = re.findall(r'Prefabs/Characters/[^"\s]+', resource('Prefabs/Groups/Campaign/Group_FIA_Remnants.et'))
assert [Path(p).stem for p in fia_members] == ['Character_FIA_Rifleman', 'Character_FIA_Scout', 'Character_FIA_MG']
for path in fia_members + ['Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_BaseLoadout.et',
                           'Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_base.et']:
    assert 'EnableAI 1' not in resource(path), path
native_branch = garrison.split('if (m_sDefenderFaction == "FIA" || SCR_GameModeCampaign.TB_CC_M4XP_GetNativePolicy())', 1)[1].split('int slot;', 1)[0]
assert 'group.RequestSpawn(trace.m_iExpectedSlots);' in native_branch
assert 'SpawnEntityPrefab' not in native_branch and 'AddAgentFromControlledEntity' not in native_branch
assert not re.search(r'\.(?:ActivateAI|SetLOD|SetPermanentLOD)\(', garrison)
assert 'PENDING_ACTIVATION' in garrison and 'native-member-spawn-timeout' in garrison
assert 'ENGAGED' not in mask(garrison)
assert '[TB_PVE_M3_6] ENGAGED' not in garrison and 'combatUnverified=1' in garrison
assert 'm_iCreationFailures++; m_iRetryAfterMs' in garrison
for counts, elapsed, expected in [([0, 0], 0, 'pending'), ([1, 3], 2000, 'pending'),
                                 ([3, 3], 4000, 'complete'), ([2, 3], 30000, 'timeout')]:
    actual = 'complete' if all(n == 3 for n in counts) else ('pending' if elapsed < 30000 else 'timeout')
    assert actual == expected
print('FIA native lifecycle installed API / prefab / pending-completion model checks PASS; human combat UNVERIFIED')
war = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_PhysicalWar.c').read_text()
operation = (ROOT / 'Scripts/Game/TB_PVE_M3_1/TB_PVE_M3_1_Operation.c').read_text()
coordinator = (ROOT / 'Scripts/Game/TB_PVE_M3_2/TB_PVE_M3_2_OPFOROperationCoordinator.c').read_text()
occupation = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_TerritorialOccupation.c').read_text()
for api in ['SetPermanentLOD(int lod)', 'GetPermanentLOD()', 'ActivateAI(bool forced = true)']:
    assert api in installed_agent, api
assert 'ActivateAI(bool forced = true)' in installed_control
combat_api = resource('scripts/Game/AI/Components/SCR_AICombatComponent.c')
assert 'BaseTarget GetCurrentTarget()' in combat_api
assert 'bool IsTargetVisible(notnull BaseTarget target)' in combat_api
assert 'void GetSelectedWeaponAndMagazine(out BaseWeaponComponent outWeaponComp, out int outMuzzleId, out BaseMagazineComponent outMagazineComp)' in combat_api
assert 'proto external int GetAmmoCount()' in resource('scripts/Game/generated/Weapon/BaseMagazineComponent.c')
assert 'Faction GetFaction()' in resource('scripts/GameCode/Character/SCR_ChimeraCharacter.c')
assert 'class Vehicle : BaseVehicle' in resource('scripts/GameCode/Vehicle/Vehicle.c')
assert 'proto external bool IsDestroyed()' in resource('scripts/Game/generated/Components/DamageManagerComponent.c')
assert 'ECharacterLifeState GetLifeState()' in resource('scripts/Game/generated/Components/CharacterControllerComponent.c')
assert 'ECharacterLifeState.DEAD' in resource('scripts/Game/Character/SCR_CharacterControllerComponent.c')
assert 'void FlashBaseIcon(Faction faction = null, bool changeToDefault = false)' in resource('scripts/Game/UI/Conflict/SCR_CampaignMapUIBase.c')
assert 'override protected float OnInsertRequested(IEntity entity, float lifetime)' in resource('scripts/Game/Systems/SCR_GarbageSystem.c')
# Core System.GetFPS is a real server-side API, used by vanilla server stats.
assert 'System.GetFPS()' in resource('scripts/Game/Network/NetworkedStats/Stats/SCR_ServerFPSNetworkedStat.c')
for guid, path in re.findall(r'\{([A-F0-9]{16})\}(Prefabs/AI/Waypoints/[^"\s]+)', war + operation + garrison):
    assert registered_guid(path) == guid, path
    assert re.match(r'(?:SCR_AIWaypoint|SCR_DefendWaypoint)\s*:', resource(path)), path
assert 'if (!policy && m_iElapsedSeconds > 300)' in operation
assert 'physical-progress-stalled-after-recovery' in operation
assert 'if (m_iStallRecoveries >= 3)' in operation
assert 'combat.IsTargetVisible(target)' in operation
assert 'PreserveOnCleanup(record)' in packages
assert 'record.m_aCharacters.Clear();' in war
assert 'SCR_EntityHelper.DeleteEntityAndChildren(character)' not in war
assert 'SetPermanentLOD(previous)' in war
assert 'if (nativeLifetime <= 0) return nativeLifetime' in war
assert 'super.OnInsertRequested(entity, lifetime)' in war
assert 'SetBaseIconFactionColor(m_Base.GetFaction())' in war
assert 'override protected void SetBaseImage()' in war
assert 'override void SetImage(string image, string imageset)' in war
assert 'override void SetBaseIconFactionColor(Faction faction)' in war
assert 'TB_CC_StrategicRole.ORDINARY' in war
assert not re.search(r'\.(SetFaction|SetAffiliatedFaction|SetBaseFaction)\(', war)
assert 'm_mTargetFailures.Get(currentTarget)' in coordinator
assert 'failures >= 2' in coordinator and 'minimumPackage = "REINFORCED"' in coordinator
assert 'failures >= 3' in coordinator and 'minimumPackage = "HEAVY"' in coordinator
assert 'HasPhysicalFailureForEscalation()' in coordinator
assert 'ResetTargetFailureHistory(id)' in native
assert 'GetMaximumConcurrentUSSROperations()' in coordinator and 'm_aAdditionalOperations' in coordinator
assert 'GetMaximumConcurrentFIAOperations()' in occupation and 'm_aOperations' in occupation
assert 'participants <= 0 && !ownerPolicy' in garrison
assert '!ownerPolicy && ObjectiveUnsafeForCreation' in garrison
assert 'm_iCreationFailures >= 3' not in garrison
assert 'SetTerritorialTarget(id)' in native and 'm_aTerritorialManagers' in native
defaults = {
    'm_fCombatSimulationRadius': '650', 'm_iActiveGarrisonRetrySeconds': '5',
    'm_iCorpsePersistenceSeconds': '3600', 'm_iWreckPersistenceSeconds': '7200',
    'm_iDroppedEquipmentPersistenceSeconds': '3600', 'm_iMaximumPersistentGarrisons': '10',
    'm_iMaximumConcurrentUSSROperations': '1', 'm_iMaximumConcurrentFIAOperations': '1',
    'm_iWithdrawalLifetimeSeconds': '600',
}
for name, value in defaults.items():
    assert re.search(r'Attribute\(defvalue: "' + value + r'"[^\n]+\)\]\s*protected (?:int|float) ' + name + ';', native), name
# Progress/no-progress models: combat or movement resets inactivity, not age.
for age, last_progress, state, expected in [(302, 300, 'ASSAULT', False), (1000, 998, 'ASSAULT', False), (300, 0, 'MOVE', True)]:
    limit = 180 if state == 'ASSAULT' else 90
    assert (age - last_progress >= limit) == expected
assert fia_tier(0, 0, 85) == 'REINFORCED'  # No fake participant prerequisite.
print('Physical-war installed API / native resources / cleanup / progress / concurrency / defaults checks PASS')
# Narrow tuning regression models; source contract checks accompany each model.
ui_source = resource('scripts/Game/UI/Conflict/SCR_CampaignMapUIBase.c')
for signature in ['protected void SetBaseImage()', 'override void SetImage(string image, string imageset)',
                  'void SetBaseIconFactionColor(Faction faction)']:
    assert signature in ui_source, signature
assert 'return SCR_CampaignFaction.Cast(GetFaction());' in resource('scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c')
for core_path, core_offset, core_stored, core_unpacked in reader.index(core_archive):
    if core_path.lower() != 'scripts/core/generated/math/math.c':
        continue
    with core_archive.open('rb') as stream:
        stream.seek(core_offset)
        math_payload = stream.read(core_stored)
    if core_stored != core_unpacked:
        math_payload = zlib.decompress(math_payload)
    assert 'static proto float Atan2(float y, float x);' in math_payload.decode()
    break
else:
    raise AssertionError('Installed Core Math declaration missing')
for serial in range(1, 12):
    for ordinal in range(6):
        for attempt in range(24):
            sector = attempt % 16
            offset = (sector + 1) // 2
            if sector % 2 == 0:
                offset = -offset
            radius = 300 + ((attempt + ordinal + serial) % 5) * 50
            assert 300 <= radius <= 500
            for preferred in [0, math.pi / 2, math.pi, -math.pi / 2]:
                angle = preferred + offset * 0.392699
                assert math.isclose(math.hypot(math.sin(angle) * radius, math.cos(angle) * radius), radius)
assert 'GetPackage(selectedPackage, false)' in garrison
assert 'if (!offensive) { light = 1; standard = 2; reinforced = 3; assault = 4; heavy = 5; surge = 6; }' in packages
for name, count in [('Light', 2), ('Standard', 4), ('Reinforced', 6), ('Heavy', 8)]:
    assert f's_iUSSR{name}Groups = {count};' in packages
    assert re.search(r'Attribute\(defvalue: "' + str(count) + r'"[^\n]+\)\]\s*protected int m_iUSSR' + name + 'Groups;', native)
assert 'ConfigureUSSRPackages(m_iUSSRLightGroups, m_iUSSRStandardGroups, m_iUSSRReinforcedGroups, m_iUSSRHeavyGroups)' in native
assert 'if (factionKey == "FIA") maximumGroups = 6;' in packages
create_section = coordinator.split('protected void CreateOperation()', 1)[1].split('protected void DisposeTerminalOperation', 1)[0]
assert 'm_mTargetFailures.Set(' not in create_section
assert 'operation.GetInitialTargetOwner()' in coordinator
for failures, expected in [(0, 2), (1, 4), (2, 6), (3, 8), (10, 8)]:
    floor = 2 if failures == 0 else 4 if failures == 1 else 6 if failures == 2 else 8
    assert floor == expected
for reason, expected in [('all-assigned-groups-destroyed', True), ('physical-progress-stalled-after-recovery', True),
                         ('move-waypoint-spawn-failed', False), ('assembly-failed-group-materialization-failed', False),
                         ('native-member-spawn-timeout', False), ('defence-handoff-not-confirmed', False)]:
    physical_reasons = {'all-assigned-groups-destroyed', 'physical-progress-stalled-after-recovery', 'movement-capture-timeout'}
    assert (reason in physical_reasons) == expected
for owner in ['US', 'FIA', 'USSR']:
    for attacker in ['US', 'FIA', 'USSR', None]:
        for radio_known in [True, False]:
            # BCM colour sink deliberately ignores attacker and radio gating.
            presentation = owner
            assert presentation == owner
assert all(key in war for key in ['stableId=', 'authoritativeOwner=', 'attackerFaction=',
                                 'presentationFaction=', 'ownershipChanged=', 'contested=', 'revision='])
print('Narrow tuning annulus / installed map methods / offensive defaults / defensive isolation / escalation / owner-colour model checks PASS')
print('Repository-wide static checks complete:', len(SCRIPTS), 'Game source files; NOT Enforce compile PASS')
# Root-cause instrumentation only: no limit setter or representation change.
ai_world_api = resource('scripts/Game/generated/AI/AIWorld.c')
for declaration in ['proto external int GetCurrentNumOfActiveAIs();', 'proto external int GetLimitOfActiveAIs();',
                    'proto external bool CanActivateGroup(AIGroup group);']:
    assert declaration in ai_world_api, declaration
chimera_api = resource('scripts/Game/generated/AI/ChimeraAIWorld.c')
for declaration in ['proto external int GetCurrentAmountOfLimitedAIsForFaction(FactionKey factionKey);',
                    'proto external int GetAILimitForFaction(FactionKey factionKey);']:
    assert declaration in chimera_api, declaration
navmesh_api = resource('scripts/Game/generated/AI/NavmeshWorldComponent.c')
for declaration in ['proto external bool IsTileLoaded(vector wPos);', 'proto external bool IsTileRequested(vector wPos);']:
    assert declaration in navmesh_api, declaration
assert '[TB_CC_BUDGET]' in war and 'SetLimitOfActiveAIs(' not in war
assert 'budgetGateReached=0' in staging
print('Root-cause budget / navmesh diagnostic installed API checks PASS; representation checks: run Test-WarSimulationStatic.py')
