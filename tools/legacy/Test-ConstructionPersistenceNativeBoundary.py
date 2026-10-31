"""Read-only installed API boundary checks; NOT P3.5 implementation/functionality tests."""
from pathlib import Path
import runpy

from bcm_test_paths import ROOT, TOOLS, GAME_INSTALL, TOOLS_INSTALL, LOCAL_PACKAGE
prior = runpy.run_path(str(TOOLS / 'Test-PersistenceFoundation.py'))
installed, base = prior['installed'], prior['base']
def read(name):
    return installed(base + 'data', '/' + name)

system = read('PersistenceSystem.c')
request = read('PersistenceSpawnRequest.c')
serializer = read('PersistenceSerializerBase.c')
link = read('SCR_EditorLinkComponentSerializer.c')
composition = read('SCR_CampaignBuildingCompositionComponentSerializer.c')
building = read('SCR_CampaignBuildingManagerComponent.c')
service = read('SCR_ServicePointComponent.c')
resource = read('SCR_ResourceComponentSerializer.c')
configuration = read('BuildableComposition.conf')
checks = 0
def check(condition):
    global checks
    assert condition
    checks += 1

check('proto external ref Managed DeserializeSpawn(notnull LoadContext context);' in system)
check(system.count('DeserializeSpawn(') == 1)
check('If the original request does not provide any callback to invoke these tasks will be ignored.' in system)
check('proto external void RequestSpawn(notnull PersistenceSpawnRequest spawnRequest, PersistenceResultCallback callback = null);' in system)
check('PersistenceCollection Collection;' in request and 'ref array<UUID> Include;' in request)
check('LoadContext' not in request and 'SaveContext' not in request)
check('Serialize a tracked entity or state instance' in system)
check('private void PersistenceSerializerBase();' in serializer)
check('return EDeserializeFailHandling.IGNORE;' in serializer)
check('!GetSystem().IsTracked(child)' in link)
check('GetSystem().Serialize(child, item.Context, defaults)' in link)
check('DeserializeLoad' in link and 'removed' in link.lower())
check('linkComponent.SpawnComposition();' in composition)
check('layout.SetBuildingValue(' in composition)
check('GetOnEntitySpawnedByProvider()' in building)
check('GetOnCompositionUnregistered()' in building)
check('GetBuildingCompositions(SCR_CampaignMilitaryBaseComponent base, out array<SCR_CampaignBuildingCompositionComponent> compositions)' in building)
check('baseManager.RegisterLogicComponent(this);' in service)
check('GetOnLinkedEntitiesSpawned().Insert(RegisterService)' in service)
check('if (container.IsEncapsulated())' in resource)
check('container.SetResourceValue(loadResource.m_fValue)' in resource)
check('BuildableEntitySerializer' in configuration)
check('SCR_EditorLinkComponentSerializer' in configuration and 'SCR_CampaignBuildingCompositionComponentSerializer' in configuration)
container = (ROOT / 'Scripts/Game/TB_CC_Persistence/TB_CC_CampaignContainer.c').read_text()
check('SCHEMA_VERSION = 2' in container and 'WriteValue("structures"' not in container)
print(f'NATIVE CONSTRUCTION API BOUNDARY VERIFIED: {checks} installed-source assertions')
print('Native construction serialization EXISTS. No complete inline-root completion/error contract verified.')
print('P3.5 NOT IMPLEMENTED; barracks/maintenance/damage/supplies/JIP persistence tests NOT RUN.')
