"""Installed-source chain validation and JSON comparison; never launches the game.

Run with --json-only to emit only the machine-readable comparison. These checks
do not substitute for config-container loading, Workbench or human interaction.
"""
from pathlib import Path
import contextlib
import io
import json
import re
import runpy
import sys
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
json_only = '--json-only' in sys.argv
saved_args = sys.argv[:]
sys.argv = [str(TOOLS / 'Test-ChatPackageRuntimeStatic.py')]
with contextlib.redirect_stdout(io.StringIO()) as output:
    checks = runpy.run_path(sys.argv[0])
sys.argv = saved_args
if not json_only:
    print(output.getvalue())
data = checks['data']
core = checks['core']
reader = checks['reader']
DATA = Path(str(GAME_INSTALL / 'addons/data'))

def installed(path):
    key = path.lower()
    if key in data:
        return data[key].decode()
    if key in core:
        return core[key].decode()
    for name, offset, stored, unpacked in reader['index'](DATA / 'data010.pak'):
        if name.lower() != key:
            continue
        with (DATA / 'data010.pak').open('rb') as stream:
            stream.seek(offset)
            payload = stream.read(stored)
        return (zlib.decompress(payload) if stored != unpacked else payload).decode()
    raise AssertionError(('missing installed resource', path))

def require(path, *tokens):
    source = installed(path)
    for token in tokens:
        assert token in source, (path, token)
    return source

def ref(guid, path):
    # Packaged vanilla assets omit authoring .meta files. Verify the exact
    # GUID/path pair in installed referring resources and the target exists.
    installed(path)
    pair = ('{' + guid + '}' + path).lower().encode()
    assert any(pair in payload.lower() for payload in data.values()), (guid, path)
    return '{' + guid + '}' + path

controller = ref('D39FCAB93FC4FB02', 'Prefabs/Characters/Core/DefaultPlayerControllerMP_Campaign.et')
mode = ref('3B230AAC5252108F', 'Prefabs/MP/Modes/Conflict/GameMode_Campaign.et')
require('Prefabs/MP/Modes/Conflict/GameMode_Campaign.et', controller)
for child, parent in (
    ('DefaultPlayerControllerMP_Campaign.et', '{A1AE127F16DD352E}Prefabs/Characters/Core/DefaultPlayerControllerMP_Factions.et'),
    ('DefaultPlayerControllerMP_Factions.et', '{225E51284CC95CFA}Prefabs/Characters/Core/DefaultPlayerControllerMP.et'),
    ('DefaultPlayerControllerMP.et', '{6E2BB64764E3BE9B}Prefabs/Characters/Core/DefaultPlayerController.et'),
):
    require('Prefabs/Characters/Core/' + child, parent)
root_layout = ref('3C9DCDD76AB83157', 'UI/layouts/HUD/HUDLayouts/HUDManager_Root.layout')
chat_layout = ref('D89BBA18F99053D6', 'UI/layouts/HUD/Chat/ChatHud.layout')
panel_layout = ref('28C54D576A8997CF', 'UI/layouts/HUD/Chat/ChatPanel.layout')
pc = require('Prefabs/Characters/Core/DefaultPlayerController.et', 'SCR_HUDManagerComponent',
             'SCR_ChatHud', chat_layout, 'm_sSlotName "Slot_Chat"', 'm_sInterfaceSettingName "m_bShowChat"')
assert pc.count('SCR_ChatHud ') == 1
require('Prefabs/Characters/Core/DefaultPlayerControllerMP.et', 'SCR_ChatComponent')
require('UI/layouts/HUD/HUDLayouts/HUDManager_Root.layout', 'Name "Slot_Chat"')
require('UI/layouts/HUD/Chat/ChatHud.layout', panel_layout, 'Name "ChatPanel"')
panel = require('UI/layouts/HUD/Chat/ChatPanel.layout', 'SCR_ChatPanel', 'Name "m_MessageEditBox"')
assert panel.count('SCR_ChatPanel ') == 1
assert root_layout in pc
require('scripts/Game/UI/HUD/SCR_HUDManagerComponent.c',
        'SCR_InfoDisplay FindInfoDisplay(typename type)', 'static SCR_HUDManagerComponent GetHUDManager()')
require('scripts/Game/UI/HUD/SCR_InfoDisplay.c', 'Widget GetRootWidget()', 'SCR_InfoDisplayHandler GetHandler(typename handlerType)')
require('scripts/Game/UI/HUD/Handlers/InfoDisplay/SCR_InfoDisplaySlotHandler.c', 'Widget GetSlotWidget()', 'CreateWidgets(display.m_LayoutPath, m_wSlot)')
require('scripts/Game/UI/Chat/SCR_ChatHud.c', 'CHAT_INPUT_ACTION = "ChatToggle"',
        'AddActionListener(CHAT_INPUT_ACTION, EActionTrigger.DOWN, Callback_OnToggleAction)',
        'm_ChatPanel.OnUpdateChat(timeSlice)', 'OpenChatPanel(m_ChatPanel)')
require('scripts/Game/UI/Chat/SCR_ChatPanel.c', 'chatPanelMgr.Register(this)',
        'void Internal_Open()', 'bool IsOpen()', '"ChatSendMessage"', 'chatComponent.SendMessage(message, channelId)')
require('scripts/Game/UI/Chat/SCR_ChatPanelManager.c', '[BaseContainerProps(configRoot: true)]',
        'protected ref array<SCR_ChatPanel> m_aChatPanels', 'override void OnGameStart()',
        'm_ChatEntity = ScriptedChatEntity.Cast(GetGame().GetChat())')
require('Configs/Core/CoresManager.conf', 'SCR_EditorManagerCore', 'SCR_ChatPanelManager')
require('scripts/Game/Editor/Core/SCR_EditorManagerCore.c', '[BaseContainerProps(configRoot: true)]')
require('scripts/Game/UI/HUD/SCR_InfoDisplayExtended.c', 'Event_OnEditorManagerInitOwner.Insert')
require('Configs/System/chimeraInputCommon.conf', 'ChatToggle', 'Configs/Chat/ChatToggleInputSource.conf')
require('Configs/Chat/ChatToggleInputSource.conf', 'keyboard:KC_SLASH')

# Reject recurrence of the demonstrated reflection failure on either BCM core.
for path, name in (
    ('Scripts/Game/TB_CC_Core/TB_CC_DedicatedGMDiagnostics.c', 'SCR_EditorManagerCore'),
    ('Scripts/Game/TB_CC_Core/TB_CC_ChatRuntimeDiagnostics.c', 'SCR_ChatPanelManager'),
):
    text = (ROOT / path).read_text()
    assert re.search(r'\[BaseContainerProps\(configRoot:\s*true\)\]\s*modded class ' + name, text), path

layers = '\n'.join(p.read_text() for p in (ROOT / 'Worlds/Production/Arland/TB_CC_Arland_Layers').glob('*.layer'))
assert mode in layers
assert 'PlayerControllerPrefab' not in layers
# Native cache startup is enabled by BCM. Restore its map-specific resource
# while retaining the inherited zero cache count (no new gameplay spawns).
cache_ref = '{8E573788A979EBAB}Configs/Campaign/CampaignCachePool/CampaignCachePool_Arland.conf'
assert cache_ref in layers
require('Configs/Campaign/CampaignCachePool/CampaignCachePool_Arland.conf', 'SCR_CacheTerrainConfig', 'm_aCachePool')
cache_native = require('Prefabs/MP/Modes/Conflict/GameMode_Campaign.et', 'SCR_CacheManagerComponent "{691E6E472CBA6978}"')
cache_block = cache_native.split('SCR_CacheManagerComponent "{691E6E472CBA6978}" {', 1)[1].split('\n  }', 1)[0]
assert 'm_iAmount 0' in cache_block
assert re.search(r'SCR_CacheManagerComponent\s+"\{691E6E472CBA6978\}"\s*\{\s*m_sTerrainConfig "' + re.escape(cache_ref) + r'"\s*\}', layers)
require('scripts/Game/Caches/SCR_CacheManagerComponent.c', 'CreateInstanceFromPrefab(m_sTerrainConfig, true)', 'SpawnRandomCaches(m_iAmount)')
# Verify the GUID also occurs in the installed vanilla Arland world, whose
# compiled resource references are binary rather than text .meta records.
for name, offset, stored, unpacked in reader['index'](DATA / 'worlds001.pak'):
    if name.lower() == 'worlds/mp/cti_campaign_arland.ent':
        with (DATA / 'worlds001.pak').open('rb') as stream:
            stream.seek(offset)
            payload = stream.read(stored)
        native_world = zlib.decompress(payload) if stored != unpacked else payload
        assert bytes.fromhex('8E573788A979EBAB')[::-1] in native_world
        break
else:
    raise AssertionError('Installed vanilla Arland world not found')
for path in (controller, root_layout, chat_layout, panel_layout):
    assert not (ROOT / path.split('}', 1)[1]).exists(), ('Unexpected native resource override', path)
report = {
    'build': 'BCM_CHAT_CHAIN_03',
    'validation': 'STATIC_INSTALLED_SOURCE_PASS_NOT_RUNTIME',
    'first_observed_failure': "CoresManager.conf: Unknown class 'SCR_EditorManagerCore' at offset 349",
    'correction': 'Restore native BaseContainerProps(configRoot: true) on BCM editor-core extension',
    'comparison': [
        {'item': key, 'vanilla_conflict': value, 'BCM': value, 'status': 'MATCH_BY_REFERENCE_OR_INHERITANCE'}
        for key, value in (
            ('chat_world_entity', '{536D80EB494DDC43}Prefabs/MP/Campaign/CampaignMPChatEntity.et'),
            ('game_mode_resource', mode), ('player_controller', controller),
            ('HUD_manager', 'SCR_HUDManagerComponent in DefaultPlayerController.et'),
            ('HUD_root', root_layout), ('Slot_Chat', 'Dynamically populated by SCR_InfoDisplaySlotHandler'),
            ('SCR_ChatHud', chat_layout), ('SCR_ChatPanel', panel_layout),
            ('panel_manager_initialization', 'Native CoresManager.conf -> SCR_ChatPanelManager.OnGameStart'),
            ('input_listener', 'ChatToggle DOWN -> SCR_ChatHud.Callback_OnToggleAction -> OpenChatPanel'),
            ('update', 'SCR_ChatHud.DisplayUpdate -> SCR_ChatPanel.OnUpdateChat'),
            ('send', 'ChatSendMessage -> SCR_ChatPanel.SendMessage -> SCR_ChatComponent.SendMessage'),
        )
    ],
    'mission_flags': {'vanilla_conflict': 6, 'vanilla_GM': 0, 'BCM': 0,
                      'meaning': 'SpawnVehicles=2 and SpawnAI=4; not chat enable flags'},
    'editor_core_container_before': {'vanilla': 'configRoot:true', 'BCM': 'attribute absent'},
    'editor_core_container_after': {'vanilla': 'configRoot:true', 'BCM': 'configRoot:true'},
    'runtime_unknowns': ['core construction after correction', 'client panel registration',
                         'input listener registration', 'visible UI', 'message delivery', 'hosted package freshness'],
}
print(json.dumps(report, indent=2))
