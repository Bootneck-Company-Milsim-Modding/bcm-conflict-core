"""Native reference/resource checks, not Workbench or client acceptance."""
from pathlib import Path
import runpy
import zlib
import re

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
runpy.run_path(str(TOOLS / 'Test-DedicatedGMStatic.py'))
reader = runpy.run_path(str(TOOLS / 'Read-InstalledVanillaResource.py'))
DATA = Path(str(GAME_INSTALL / 'addons/data'))
def resource(name):
    for archive in DATA.glob('*.pak'):
        for path, offset, stored, unpacked in reader['index'](archive):
            if path.lower() != name.lower():
                continue
            with archive.open('rb') as stream:
                stream.seek(offset)
                payload = stream.read(stored)
            return zlib.decompress(payload) if stored != unpacked else payload
    raise AssertionError(name)

campaign = resource('worlds/MP/CTI_Campaign_Arland.ent')
assert b'ScriptedChatEntity' in campaign
assert bytes.fromhex('536D80EB494DDC43')[::-1] in campaign
chat = resource('Prefabs/MP/Campaign/CampaignMPChatEntity.et').decode()
assert chat.startswith('ScriptedChatEntity')
assert 'RplComponent' in chat and 'Streamable Disabled' in chat
for channel in ('Global', 'Faction', 'Group', 'Vehicle', 'Local', 'Whisper'):
    assert f'"{channel}"' in chat
layers = list((ROOT / 'Worlds/Production/Arland/TB_CC_Arland_Layers').glob('*.layer'))
texts = {p.name: p.read_text() for p in layers}
assert sum(len(re.findall(r'\bScriptedChatEntity\s*(?:\w+\s*)?:', t)) for t in texts.values()) == 1
managers = texts['A_Managers.layer']
assert 'ScriptedChatEntity : "{536D80EB494DDC43}Prefabs/MP/Campaign/CampaignMPChatEntity.et" {\n coords' in managers
assert managers.count('SCR_EditorSettingsEntity {') == 1
assert 'm_bIsUnlimitedEditorLegal 0' in managers
assert 'm_bOverrideBaseModes 1' not in managers
assert 'SCR_RestoreEditorModesComponent' not in managers
assert 'SCR_EditorManagerEntity' not in '\n'.join(texts.values())
assert 'SCR_EditorManagerCore' not in '\n'.join(texts.values())
mission = (ROOT / 'Missions/Production/TB_CC_Arland.conf').read_text()
assert 'SCR_MissionHeaderCampaign' in mission
assert '"{A4F3300000000000}Worlds/Production/Arland/TB_CC_Arland.ent"' in mission
assert 'A4F3500000000000' in (ROOT / 'Missions/Production/TB_CC_Arland.conf.meta').read_text()
assert 'A4F3300000000000' in (ROOT / 'Worlds/Production/Arland/TB_CC_Arland.ent.meta').read_text()
assert 'Parent "worlds/Arland/Arland.ent"' in (ROOT / 'Worlds/Production/Arland/TB_CC_Arland.ent').read_text()
native = resource('Missions/23_Campaign_Arland.conf').decode()
assert 'm_eEditableGameFlags 6' in native and 'm_eDefaultGameFlags 6' in native
gm = resource('Missions/22_GM_Arland.conf').decode()
assert 'm_eEditableGameFlags' not in gm and 'm_eDefaultGameFlags' not in gm
game = resource('scripts/Game/game.c').decode()
assert re.search(r'Metabolism\s*=\s*1', game)
assert re.search(r'SpawnVehicles\s*=\s*2', game)
assert re.search(r'SpawnAI\s*=\s*4', game)
header = resource('scripts/Game/Mission/SCR_MissionHeader.c').decode()
assert '[Attribute("0", uiwidget: UIWidgets.Flags, "Editable Game Flags"' in header
assert '[Attribute("0", uiwidget: UIWidgets.Flags, "Default Game Flags"' in header
assert 'm_eEditableGameFlags' not in mission and 'm_eDefaultGameFlags' not in mission
edit = resource('Prefabs/Editor/Modes/EditorModeEdit.et').decode()
assert 'SCR_PlaceableEntitiesRegistry' in edit and 'Configs/Editor/ActionLists/Command/Command.conf' in edit
print('NATIVE CHAT/GM REFERENCE PASS: native Conflict binary references campaign chat GUID; one production chat; native non-streamable replication/channels; production GUID chain; editor settings/core duplicate checks; header flags decoded; standard registries retained')
print('Packaged remote version, resource processing, initialization, client HUD/JIP and GM interaction NOT VERIFIED; WORKBENCH GATE REQUIRED')
