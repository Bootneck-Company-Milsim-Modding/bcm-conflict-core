"""Installed native GM contracts and permissions model; no game process launched."""
from pathlib import Path
import hashlib
import re
import runpy
import zlib

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
# Preserve the earlier pending changes, including authored resources, through
# validation. This script never writes or launches the game.
locked = list((ROOT / 'Worlds').rglob('*')) + list((ROOT / 'Missions').rglob('*'))
locked += [p for p in (ROOT / 'Scripts/Game').rglob('*.c')
           if p.name != 'TB_CC_DedicatedGMDiagnostics.c']
locked = [p for p in locked if p.is_file()]
before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in locked}
continuity = runpy.run_path(str(TOOLS / 'Test-FIARecaptureContinuityStatic.py'))
archive = Path(str(GAME_INSTALL / 'addons/data/data007.pak'))
index = {p.lower(): (offset, stored, unpacked)
         for p, offset, stored, unpacked in continuity['reader']['index'](archive)}
def resource(path):
    offset, stored, unpacked = index[path.lower()]
    with archive.open('rb') as stream:
        stream.seek(offset)
        payload = stream.read(stored)
    if stored != unpacked:
        payload = zlib.decompress(payload)
    return payload.decode('utf-8')
source = (ROOT / 'Scripts/Game/TB_CC_Core/TB_CC_DedicatedGMDiagnostics.c').read_text()

contracts = {
    'scripts/Game/Editor/Core/SCR_EditorManagerCore.c': (
        'override void OnGameStart()', 'override void OnGameEnd()',
        'SCR_EditorManagerEntity GetEditorManager(int playerID)',
        'SCR_EditorSettingsEntity GetSettingsEntity()',
        'EEditorMode GetBaseModes(EEditorModeFlag flags = -1, bool coreOnly = false)',
        'flags |= EEditorModeFlag.WORKBENCH',
        'if (SCR_Global.IsAdminRole(roleFlags) && Replication.IsRunning())',
        'editorManager.SetEditorModes(EEditorModeAccess.ROLE, modes, false)',
        'rpl.Give(playerRplID)',
    ),
    'scripts/Game/Editor/Entities/SCR_EditorManagerEntity.c': (
        'bool CanOpen()', 'bool HasMode(EEditorMode mode)',
        'EEditorMode GetModes()', 'override bool IsOpened()',
        'SCR_EditorModeEntity FindModeEntity(EEditorMode mode)',
        'SCR_EGameModeState.POSTGAME',
    ),
    'scripts/Game/Editor/Entities/SCR_EditorSettingsEntity.c': (
        'bool IsEditorDisabled()', 'bool IsUnlimitedEditorLegal()',
        "but it's disabled in Conflict where only admin can access the editor",
    ),
    'scripts/Game/Global/Functions.c': (
        'static bool IsAdmin(int playerID)',
        'EPlayerRole.ADMINISTRATOR', 'EPlayerRole.SESSION_ADMINISTRATOR',
    ),
    'scripts/Game/generated/Network/PlayerManager.c': (
        'proto external int GetPlayers(out notnull array<int> outPlayers);',
        'proto external EPlayerRole GetPlayerRoles(int iPlayerId);',
    ),
    'scripts/Game/GameMode/Components/SCR_PlayerListedAdminManagerComponent.c': (
        'backendApi.IsListedServerAdmin(playerId)',
        'can login as admin even if the player is not yet logged in',
    ),
    'scripts/Game/Editor/Components/Editor/SCR_BudgetEditorComponent.c': (
        'GetMaxBudgetValue(EEditableEntityBudget type, out int maxBudget)',
        'GetCurrentBudgetValue(EEditableEntityBudget type)',
    ),
}
for path, signatures in contracts.items():
    installed = resource(path)
    for signature in signatures:
        assert signature in installed, (path, signature)
print('GM INSTALLED API / override signatures / role assignment / owner replication PASS')

# No permission or lifecycle setters in the diagnostic. Every native override
# calls super; all observations execute only on authority and stop at game end.
masked = re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/', '', source, flags=re.S)
for setter in ('GivePlayerRole', 'ClearPlayerRole', 'SetEditorModes', 'AddEditorModes',
               'RemoveEditorModes', 'SetCanOpen', 'EnableBaseOverride', 'SetBaseModes',
               'SpawnEntity', 'SetFaction', 'SetLOD', 'SetBudgetValue', 'Rpc'):
    assert not re.search(r'\b' + setter + r'\s*\(', masked), setter
for token in ('super.OnGameStart();', 'super.OnGameEnd();',
              'if (!Replication.IsServer()) return;',
              'Remove(TB_CC_TraceGMPlayers)',
              'm_TB_CC_GMStates.Get(playerId) == signature',
              'editor.FindModeEntity(EEditorMode.EDIT)',
              'CallLater(TB_CC_TraceGMPlayers, 5000, true)'):
    assert token in source, token
for private_or_map_specific in ('GetPlayerIdentityId', 'GetPlayerPlatformId',
                                'GetTicket', 'password', 'SteamId', 'ARLAND',
                                'MILITARY_SOUTH', 'MOSS_HILL'):
    assert private_or_map_specific not in source

config = resource('Configs/Core/EditorManagerCore.conf')
edit_flags = int(re.search(r'SCR_EditorModePrefab EDIT\s*\{.*?m_Flags (\d+)', config, re.S)[1])
photo_flags = int(re.search(r'SCR_EditorModePrefab PHOTO\s*\{.*?m_Flags (\d+)', config, re.S)[1])
assert edit_flags & 2 and edit_flags & 16 and not edit_flags & 1
assert photo_flags & 1

def modes(admin=False, session_admin=False, listed=False, workbench=False, explicit=False):
    # Native listing changes login eligibility, not active role assignment.
    flags = (2 if admin or session_admin else 0) | (16 if workbench else 0)
    return {'EDIT'} if edit_flags & flags or explicit else set()

assert modes(admin=True) == {'EDIT'}             # A, permission architecture
assert modes(session_admin=True) == {'EDIT'}
assert not modes()                             # B, no automatic GM
assert not modes(listed=True)                  # listed but not logged in
assert modes(listed=True, admin=True) == {'EDIT'}
assert modes(workbench=True) == {'EDIT'}        # F, native WB path retained
assert modes(explicit=True) == {'EDIT'}         # explicit delegated access allowed

# C/D: native EDIT prefab resolves camera, placement, inspection, deletion
# tooling through its parent chain. This establishes wiring, not actual use.
edit = resource('Prefabs/Editor/Modes/EditorModeEdit.et')
assert 'SCR_CameraEditorComponent' in edit
assert 'SCR_PlacingEditorComponent' in edit
assert 'SCR_ContentBrowserEditorComponent' in edit
assert 'Configs/Editor/ActionLists/Context/TempEdit.conf' in edit
actions = resource('Configs/Editor/ActionLists/Context/TempEdit.conf')
assert 'SCR_DeleteSelectedContextAction' in actions
mission = (ROOT / 'Missions/Production/TB_CC_Arland.conf').read_text()
assert 'SCR_MissionHeaderCampaign' in mission
assert 'Worlds/Production/Arland/TB_CC_Arland.ent' in mission
world_managers = (ROOT / 'Worlds/Production/Arland/TB_CC_Arland_Layers/A_Managers.layer').read_text()
assert 'SCR_GameModeCampaign GameMode_Campaign' in world_managers
assert 'Prefabs/MP/Modes/Conflict/GameMode_Campaign.et' in world_managers
assert before == {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in locked}
print('GM A/B/F permission MODEL PASS; C/D native tooling SOURCE PASS; E/G read-only and locked-file integrity PASS')
print('GM interactive/dedicated A-F remain PENDING; FIA/campaign model regression G PASS')
print('DEDICATED GM STATIC/API/MODEL PASS; WORKBENCH GATE REQUIRED; no runtime GM access claimed')
