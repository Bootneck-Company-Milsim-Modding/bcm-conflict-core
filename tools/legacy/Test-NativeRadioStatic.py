"""Installed native radio/API checks and abstract coverage cases, not runtime proof."""
from pathlib import Path
import runpy

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-ObjectiveCommitmentStatic.py'))
resource = prior['baseline']['resource']
observer = (ROOT / 'Scripts/Game/TB_CC_Core/TB_CC_NativeRadioObserver.c').read_text()
native = (ROOT / 'Scripts/Game/TB_CC_M4X/TB_CC_M4XP_NativeArland.c').read_text()
coverage = resource('scripts/Game/Systems/SCR_RadioCoverageSystem.c')
radio = resource('scripts/Game/Components/SCR_CoverageRadioComponent.c')
base = resource('scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c')
manager = resource('scripts/Game/Campaign/SCR_CampaignMilitaryBaseManager.c')

for token in ('OnCoverageChangedInvoker GetOnCoverageChanged()',
              'protected ref array<SCR_CoverageRadioComponent> m_aRadioComponents',
              'protected ref map<string, SCR_CoverageRadioComponent> m_mEncryptionSignalSources'):
    assert token in coverage, token
for token in ('void GetRadiosInRangeOf(notnull array<SCR_CoverageRadioComponent> radios)',
              'SCR_ERadioCoverageStatus GetCoverageByEncryption(string encryptionKey)',
              'bool IsSource()', 'manager.RegisterRadioComponent(this, m_fSavedRange)'):
    assert token in radio, token
for token in ('SCR_CampaignBuildingProviderComponent GetMasterProvider()',
              'IEntity GetBaseBuildingComposition()', 'void OnAllBasesInitialized()',
              'radio.SetPower(true)', 'ChangeRadioSettings(newCampaignFaction)',
              'transceiver.SetRange(range)', 'SCR_EServicePointType.RADIO_ANTENNA'):
    assert token in base, token
assert 'int GetBases(notnull out array<SCR_CampaignMilitaryBaseComponent> bases, Faction faction = null)' in manager

# Resources support a native provider and antenna catalogue; presence in a
# running captured base is deliberately NOT inferred from these source tests.
building = resource('Prefabs/Props/Military/Compositions/BuildingService_base.et')
assert 'SCR_CampaignBuildingProviderComponent' in building
assert 'm_bIsMasterProvider 1' in building and 'm_bRegisterAtBase 1' in building
assert 'SCR_CampaignBuildingStartUserAction' in building
catalogue = resource('Configs/Editor/PlaceableEntities/Compositions/Compositions_FreeRoamBuilding.conf')
assert 'E_Antenna_S_US_01.et' in catalogue
assert 'm_bIsSource 1' in resource('Prefabs/Systems/MilitaryBase/ConflictBase_MOB.et')
assert '"Transmitting Range" 1000' in resource('Prefabs/Systems/MilitaryBase/ConflictMilitaryBase.et')

# Observer is read-only and portable; no silent registration/range repair.
for forbidden in ('SetPower(', 'SetRange(', 'SetFaction(', 'SetIsSource(',
                  'RegisterRadioComponent(', 'SpawnEntity', 'ARLAND', 'MOSS_HILL', 'MILITARY_SOUTH'):
    assert forbidden not in observer, forbidden
assert 'm_System.GetOnCoverageChanged().Insert(OnNativeCoverageChanged)' in observer
assert 'm_System.GetOnCoverageChanged().Remove(OnNativeCoverageChanged)' in observer
assert 'CallLater(FlushCoverageChange, 250, false)' in observer
assert 'Remove(FlushCoverageChange)' in observer
assert 'm_bPending' in observer and '!Replication.IsServer()' in observer
assert 'm_LastNodeState.Get(id) == state' in observer
assert 'WAITING_FOR_RADIO_EXTENSION' in observer
assert 'if (!committedObjective.IsEmpty()' in observer
assert 'CallLater(RetryRadioObjective, 10000)' in native
assert 'Reevaluate("native-radio-topology-changed")' in native
assert 'base.IsHQRadioTrafficPossible(m_US)' in native

# Authored owners do not guarantee an ownership-change event at startup.
# Use the installed native setup, not SetFaction(sameOwner) (which returns early).
startup = native.split('protected void TB_CC_M4XP_InitializeBasePolicy()')[1].split('protected void TB_CC_M4XP_RetireAmbientSpawners')[0]
assert startup.count('base.ChangeRadioSettings(radioFaction);') == 1
assert startup.index('base.Initialize();') < startup.index('base.ChangeRadioSettings(radioFaction);')
assert 'void ChangeRadioSettings(notnull SCR_Faction faction)' in base
assert 'SCR_CampaignFaction GetCampaignFaction()' in base
assert 'm_RadioComponent.SetEncryptionKey(faction.GetFactionRadioEncryptionKey());' in base
assert 'tsv.SetFrequency(factionFrequency);' in base
assert 'SCR_RadioCoverageSystem.UpdateAll();' in base
assert 'string encryption = radioHQ.GetEncryptionKey();' in base
assert 'return radio.GetCoverageByEncryption(encryption);' in base
assert 'm_sSavedEncryptionKey != encryptionKey || !IsPowered()' in radio
assert base.index('m_OnFactionChangedExtended.Invoke(this, previousFaction, faction);') < base.index('ChangeRadioSettings(newCampaignFaction);')
assert 'CallLater(TraceOwnershipChange, 250, false' in observer
assert 'Remove(TraceOwnershipChange)' in observer
assert 'hqEncryptionMatches=' in observer
assert '[TB_CC_RADIO_TRANSITION]' in observer

# Reproduce key split independently of range: the receiving enemy node is
# reached, but only a same-key powered node may continue the native ping.
def keyed_receive(keys, source_key):
    edges = {'HQ': ['A'], 'A': ['B'], 'B': []}
    reached, pending = set(), ['HQ']
    while pending:
        node = pending.pop(0)
        if node in reached:
            continue
        reached.add(node)
        if keys[node] == source_key:
            pending.extend(edges[node])
    return reached
assert 'B' in keyed_receive({'HQ': '', 'A': '', 'B': ''}, '')
assert 'B' not in keyed_receive({'HQ': '', 'A': 'US', 'B': ''}, '')
assert 'B' in keyed_receive({'HQ': 'US', 'A': 'US', 'B': 'FIA'}, 'US')
assert 'B' not in keyed_receive({'HQ': 'US', 'A': 'FIA', 'B': 'FIA'}, 'US')

# Independent directed-coverage model: native Ping marks a receiver, but only
# a powered same-key node propagates to its neighbours. Coordinates are generic
# model units, never map configuration or proposed radio ranges.
def receive(nodes):
    reached, pending = set(), ['HQ']
    while pending:
        name = pending.pop(0)
        if name in reached:
            continue
        reached.add(name)
        x, radius, faction, powered = nodes[name]
        if faction != 'US' or not powered:
            continue
        pending.extend(other for other, (ox, _, _, opowered) in nodes.items()
                       if other != name and opowered and abs(x - ox) <= radius)
    return reached

nodes = {'HQ': (0, 1, 'US', True), 'A': (.8, 1, 'FIA', True),
         'B': (2.1, 1, 'FIA', True)}
assert 'A' in receive(nodes)  # A: opening receive coverage.
nodes['A'] = (.8, 1, 'US', True)
assert 'B' not in receive(nodes)  # B: owning A doesn't create unlimited reach.
old_owners = {key: value[2] for key, value in nodes.items()}
nodes['A'] = (.8, 2, 'US', True)  # C/F: native antenna increases relay range.
assert 'B' in receive(nodes)
assert old_owners == {key: value[2] for key, value in nodes.items()}
nodes['A'] = (.8, 2, 'FIA', True)
assert 'B' not in receive(nodes)
assert prior['resolve']('B', '', 'FIA') == 'B'  # D: committed assault survives.
assert 'A' in receive(nodes)
assert prior['resolve']('B', 'A', 'US') == 'A'  # E: restoration objective.

flush = observer.split('protected void FlushCoverageChange()')[1].split('protected void TraceNodes()')[0]
assert flush.index('!Replication.IsServer()') < flush.index('m_OnChanged.Invoke()')  # G
callback = native.split('protected void OnNativeRadioTopologyChanged()')[1].split('protected string GetUSObjectiveRecommendation()')[0]
assert callback.index('!Replication.IsServer()') < callback.index('Reevaluate(')
print('NATIVE RADIO SOURCE / INSTALLED API / A-G COVERAGE MODEL PASS; runtime chain and construction remain unverified')
