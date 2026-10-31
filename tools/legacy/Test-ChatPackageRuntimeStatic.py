"""Read-only package freshness comparison and installed diagnostic API checks."""
from pathlib import Path
import argparse
import hashlib
import runpy
import zlib
import re

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
parser = argparse.ArgumentParser()
parser.add_argument('--package', type=Path, default=LOCAL_PACKAGE)
args = parser.parse_args()
runpy.run_path(str(TOOLS / 'Test-NativeChatGMReference.py'))
reader = runpy.run_path(str(TOOLS / 'Read-InstalledVanillaResource.py'))
def contents(archive):
    result = {}
    with archive.open('rb') as stream:
        for path, offset, stored, unpacked in reader['index'](archive):
            stream.seek(offset)
            payload = stream.read(stored)
            result[path.lower()] = zlib.decompress(payload) if stored != unpacked else payload
    return result

core = contents(Path(str(GAME_INSTALL / 'addons/core/data.pak')))
data = contents(Path(str(GAME_INSTALL / 'addons/data/data007.pak')))
for path, token in (
    ('scripts/game/generated/ChimeraGame.c', 'proto external BaseChatEntity GetChat();'),
    ('scripts/game/generated/entities/BaseChatEntity.c', 'class BaseChatEntity: GenericEntity'),
    ('scripts/game/generated/entities/BaseChatEntity.c', 'proto external int GetChannelsCount();'),
    ('scripts/gamelib/generated/game.c', 'proto external string GetWorldFile();'),
    ('scripts/gamelib/generated/components/BaseRplComponent.c', 'proto external RplRole Role();'),
    ('scripts/core/generated/system/System.c', 'static proto bool IsConsoleApp();'),
    ('scripts/core/generated/entities/IEntity.c', 'proto external string GetName();'),
    ('scripts/core/generated/world/BaseWorld.c', 'proto external void GetBoundBox(out vector mins, out vector maxs);'),
    ('scripts/core/generated/world/BaseWorld.c', 'proto external bool QueryEntitiesByAABB(vector mins, vector maxs, QueryEntitiesCallback addEntity, QueryEntitiesCallback filterEntity = null, EQueryEntitiesFlags queryFlags = EQueryEntitiesFlags.ALL);'),
):
    assert token in (data.get(path.lower()) or core[path.lower()]).decode()
source = (ROOT / 'Scripts/Game/TB_CC_Core/TB_CC_ChatRuntimeDiagnostics.c').read_text()
assert re.findall(r'\bmodded\s+class\s+(\w+)', source) == ['SCR_ChatPanelManager']
assert 'BCM_CHAT_CHAIN_03' in source
assert '[TB_CC_CHAT_CLIENT]' in source and '[TB_CC_CHAT_SERVER]' in source
assert 'inputListener=NOT_EXPOSED' in source
assert 'clientState != s_ClientState' in source and 'serverState != s_ServerState' in source
for forbidden in ('SpawnEntity', 'DeleteEntity', 'SetChat', 'Rpc(', 'password', 'GetPlayerIdentityId'):
    assert forbidden not in source
print('CHAT DIAGNOSTIC installed API/read-only/static PASS; native registration remains engine-owned')
# Resolve every addon modded target against actual installed script source.
# Generated/native engine targets are rejected conservatively; this cannot
# establish every hidden compiler rule, so Workbench remains authoritative.
declarations = {}
for path, payload in list(core.items()) + list(data.items()):
    if not path.endswith('.c'):
        continue
    for name in re.findall(r'\bclass\s+(\w+)\s*(?::|\{)', payload.decode('utf8', errors='replace')):
        declarations.setdefault(name, []).append(path)
count = 0
for path in (ROOT / 'Scripts/Game').rglob('*.c'):
    text = path.read_text()
    for target in re.findall(r'\bmodded\s+class\s+(\w+)', text):
        candidates = declarations.get(target, [])
        assert candidates, ('Unresolved installed modded target', path, target)
        assert any('/generated/' not in p for p in candidates), ('Engine/generated class modding forbidden by readiness policy', path, target, candidates)
        for candidate in candidates:
            native = (data.get(candidate) or core.get(candidate)).decode('utf8', errors='replace')
            if re.search(r'\[BaseContainerProps\(configRoot:\s*true\)\]\s*class\s+' + target + r'\b', native):
                assert re.search(r'\[BaseContainerProps\(configRoot:\s*true\)\]\s*modded\s+class\s+' + target + r'\b', text), ('Missing native config-root metadata', path, target)
        count += 1
print('WHOLE-TREE MODDED TARGET AUDIT PASS:', count, 'extensions resolve to installed script-defined classes; no generated-only engine target')
if not args.package.is_file():
    print('PACKAGE UNAVAILABLE: actual packaged inclusion/freshness not verified')
else:
    package = contents(args.package)
    paths = ['Missions/Production/TB_CC_Arland.conf', 'Worlds/Production/Arland/TB_CC_Arland.ent']
    paths += [str(p.relative_to(ROOT)).replace('\\', '/') for p in (ROOT / 'Worlds/Production/Arland/TB_CC_Arland_Layers').glob('*.layer')]
    paths += ['Scripts/Game/TB_CC_Core/TB_CC_ChatRuntimeDiagnostics.c', 'Scripts/Game/TB_CC_Core/TB_CC_DedicatedGMDiagnostics.c']
    for path in paths:
        local = (ROOT / path).read_bytes()
        packed = package.get(path.lower())
        state = 'MISSING' if packed is None else ('EXACT_MATCH' if packed == local else 'DIFFERENT')
        print('PACKAGE', path, state, 'sourceSHA256=' + hashlib.sha256(local).hexdigest(),
              'packageSHA256=' + (hashlib.sha256(packed).hexdigest() if packed is not None else 'NONE'))
    layer = package.get('worlds/production/arland/tb_cc_arland_layers/a_managers.layer', b'')
    print('PACKAGE layer campaignChat=', b'536D80EB494DDC43' in layer,
          'genericChat=', b'F69BC912AC8236F9' in layer,
          'scriptedChatRecords=', layer.count(b'ScriptedChatEntity :'))
    print('Package mismatch is evidence about this local archive only, not the hosted revision. No runtime PASS claimed.')
