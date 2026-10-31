# M4.X — Conflict Core native scenario prototype

## Status and evidence boundary

2026-10-01. Experimental implementation; acceptance is **INCOMPLETE**.
Production migration is **NO-GO pending the required proofs**, not a finding
that the architecture is impossible. M4.4J and historical evidence remain intact.
No M4.5, production migration, persistence, siege, patrol or checkpoint work.

Normal experimental world: `Worlds/TB_CC_M4X_NativePrototype.ent`.
Separate dedicated actor/service proof: `Worlds/TB_CC_M4X_PhysicalProof.ent`.
Separate two-position pressure proof: `Worlds/TB_CC_M4X_PressureProof.ent`.
Separate read-only installed-resource audit: `Worlds/TB_CC_M4X_DependencyAudit.ent`.
Neither diagnostic world is inherited by the normal experimental world.
Normal mission: `{A4F2600000000000}Missions/TB_CC_M4X_NativePrototype.conf`,
using the generic SCR_MissionHeader, not the vanilla Conflict campaign header.

All normal prototype M2/M3/M4 proof switches remain off. The proof world alone
adds a dedicated-only driver. It does not write faction ownership, fake humans,
invoke CaptureFinish directly, teleport moving OPFOR, or rewrite M3.

## Installed-resource audit

Inspected actual loaded prefab entity containers and read installed vanilla
source through the engine virtual filesystem, not filename-based assumptions.
Read-only audit logs preserve exact source paths/line numbers and serialized
prefab content outside the addon tree. Vanilla implementations were not copied
into BCM prefabs or scripts. Empty container-string values are **not** evidence
of false/default values; raw source establishes those settings where needed.

| Installed resource | Observed contents | Decision |
| --- | --- | --- |
| ConflictBase_Base | 11 root components: campaign base, seizing, supplies, radio, faction, resource, map descriptor, editable system, area mesh, persistence and RPL; campaign spawn-point child | INHERIT through MOB; keep low-level services |
| ConflictBase_MOB | Same family; seizing `Enabled 0`, HQ eligibility, source radio, 3000 supplies, free-building generator; unused-HQ disable enabled | INHERIT for TB_CC_MOB; disable unused-HQ removal |
| ConflictMilitaryBase | 11 root components and a different component-ID family; seizing requires player and ignores non-playable attackers by default | INHERIT for TB_CC_StrategicBase; configure both flags off |
| ConflictControlPoint | Campaign-base family with control-point flag enabled | NOT REQUIRED; would import unwanted territorial victory counting |
| ConflictRelayRadio | Seven root components, base radius 10, radio, **no seizing** | NOT REQUIRED for this compact topology; reusable radio tool, not a capture parent |
| ConflictSourceBase | Source-base component rather than ordinary campaign-base class; radius 100 with seizing | NOT REQUIRED; do not import source-base logistics as ordinary objectives |
| SourceBase_Airfield / T1Harbor / T2Harbor / T3Harbor | Resolved installed resources; source-base component/radius 100/seizing present at root | NOT REQUIRED; specialized descendants not created |
| GameMode_Base | SCR_BaseGameMode, 32 root components including respawn and RPL | Insufficient as an unchanged campaign-base service parent |
| GameMode_Plain | SCR_BaseGameMode, 33 root components | Same service coupling limitation |
| GameMode_Campaign | SCR_GameModeCampaign, 52 root components | INHERIT service infrastructure, SUBCLASS campaign policy |

The source-base variants' root composition was inspected. Their complete
specialized logistical/child differences are not claimed interchangeable;
none is selected or needed for this prototype.

### Concrete game-mode dependency and decision

Chosen experimental class: `TB_CC_M4X_GameMode : SCR_GameModeCampaign`.
Prefab parent: `{3B230AAC5252108F}Prefabs/MP/Modes/Conflict/GameMode_Campaign.et`.
This is **not** inheritance of the vanilla Conflict world/scenario hierarchy.

Installed `Scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c`:

- `Initialize`, lines 514 onward, sets initialized state and calls OnInitialized.
- `OnInitialized`, lines 563–565, returns without a Campaign game mode, **before**
  spawn-point assignment, services, local faction hooks and capture callbacks.
- Lines 625–657 depend on the Campaign base manager and supply/spawn lifecycle.
- `SetAsHQ`, lines 1015 onward, registers the faction main base and normal HQ
  supply/spawn consequences.

Installed `SCR_GameModeCampaign.c`, `InitalBaseSetup`, lines 615–690, selects
two physical HQs and applies vanilla start policy. BCM overrides this method
only in its new class: initialize the authored three bases, register the
already US-owned MOB as HQ, and create no enemy HQ. It performs no initial
ownership writes. `CheckForWinner` is overridden because BCM pressure owns
outcomes. Vanilla Start/OnStarted, respawn, base manager, RPL and end-match
consequence remain inherited.

The addon already globally attaches the historical M1 capture harness to
Campaign OnGameStart. The experimental subclass overrides its Begin callback
to a no-op; it does not change the production harness or use a misleading
production entity name to suppress it. This inherited fixture coupling is
explicit migration debt, not a new campaign rule.

### Remaining toolbox classification

| System | Classification | Boundary |
| --- | --- | --- |
| CampaignSeizing / generic Seizing | KEEP VANILLA + existing WRAP | Physical actor eligibility, superiority, timer, interruption, CaptureFinish and ownership remain the existing vanilla-compatible lifecycle |
| FactionManager / CampaignFaction | INHERIT | Installed CampaignFactionManager prefab; existing M1.5 US/USSR policy retained; FIA not redesigned |
| Deployment / respawn | KEEP VANILLA + existing WRAP | Campaign spawn point and respawn system; disabled forward spawn flag plus existing UI exclusion marker |
| Radio | KEEP VANILLA | Source radio at MOB, ordinary radios and coverage system |
| Supplies | KEEP VANILLA | Inherited resource/supply/storage/service contracts; no BCM logistics replacement |
| Building / construction | KEEP VANILLA | Service manager/catalog retained; commander/establishing-base policy disabled in this experimental mode only |
| Map markers | KEEP VANILLA | Base descriptor and map manager infrastructure inherited |
| Capture presentation | KEEP VANILLA | Campaign base capture callbacks/descriptors retained; literal human UI not yet verified |
| Game-over / end-match | WRAP | Existing BCM outcome consequence bridge, not vanilla winner selection |
| Replication | KEEP VANILLA + existing WRAP | RPL components and existing stable-ID semantic query architecture; N1–N5 still pending |
| AI groups / navigation | KEEP VANILLA | Existing M3 operation/staging/assembly; vanilla AIWorld and baked Arland navmesh |
| Campaign tasks | NOT REQUIRED as strategic authority | Inherited task components have not yet been pruned; do not claim they are all independent of Campaign |

## Authored prototype and inheritance

`TB_CC_MOB.et` inherits `{3772B47BCF3C62FB}ConflictBase_MOB.et`.
It serializes US affiliation and prevents disabling an unused HQ. The parent
already disables seizing. Initialization registers it as the faction HQ.

`TB_CC_StrategicBase.et` inherits `{1391CE8C0E255636}ConflictMilitaryBase.et`.
It serializes USSR affiliation, HQ/control-point flags off, player-required
and ignore-non-playable capture flags off, the existing no-deployment marker,
and inherited spawn group disabled. It keeps the spawn reference rather than
removing the supply/respawn linkage or only hiding UI.

| Entity | Stable ID | Terrain-resolved technical position |
| --- | --- | --- |
| TB_CC_PROTO_MOB | PROTO::MOB | 1630, 42.2812, 2280 |
| TB_CC_PROTO_OBJ_01 | PROTO::OBJ_01 | 2030, 67.4375, 2280 |
| TB_CC_PROTO_OBJ_02 | PROTO::OBJ_02 | 2430, 126.438, 2280 |

Graph: MOB—OBJ_01—OBJ_02. These are technical proof locations, not final
Creative Director authoring. Registry uses live entity transforms.
`PROTO::OFF_MAP_OPFOR` is start-adapter metadata only: no entity, registry node,
capture objective, spawn point or physical USSR HQ.

### Generic reuse and explicit shared edits

The map adapter inherits the existing M4.4 controller to reuse its ownership
callback, pressure/history, deployment policy, public projection and M3
consumer lifecycle. Only registry/spatial construction and authority
initialization are overridden. It does not load ARLAND_PRODUCTION_01.
The historical Arland class name remains a **transitional architectural debt**;
extracting a map-neutral lifecycle before production migration is recommended.
No duplicate registry, evaluator, pressure engine or M3 implementation was added.

Two shared generic scripts were changed, explicitly:

1. M4.2A start state adds `RequiresPhysicalOPFOREndpoint()`, default **true**.
2. M4.3 evaluator conditions physical endpoint validation on that contract.

Existing start definitions retain true and their previous physical-endpoint
requirements. The experimental subclass returns false, leaving endpoint ID
empty. This does not put a fake enemy HQ in the graph or exclude OBJ_02 from
ordinary territory. The existing M4.2A physical-endpoint resolver itself was
not weakened; the off-map adapter constructs an explicit start state.

Production Arland **world, layers, mission, prefab settings and definition**
were not edited. Shared generic script/API edits above must not be hidden
behind the statement “no production resources changed.” Workbench generated
the updated resource database normally; it was not fabricated.

## Arland world and navigation

Normal world parent is `worlds/Arland/Arland.ent`: terrain, geometry, roads,
buildings, environment, map and preload resources remain vanilla. No
CTI/CTL_Campaign_Arland world or strategic layers are inherited.
The new Native layer owns only mode, faction manager, AIWorld, campaign adapter,
MOB and two strategic-base instances.

`SCR_AIWorld_Arland.et` does **not** assign Soldiers/BTRlike navmesh files by
itself; installed source leaves those configurations empty and only assigns
LowResArland. The first movement probe exposed empty high-resolution navmesh
initialization. Native.layer now supplies the existing vanilla baked files:

- `{E0E9D55644237D27}worlds/MP/Navmeshes/CTI_Campaign_Arland.nmn`
- `{3B27B3547843CC74}worlds/MP/Navmeshes/CTI_Campaign_Arland_vehicles.nmn`
- LowResArland remains inherited.

The baked data's filename contains CTI_Campaign; this **is a low-level asset
dependency**, not a dependency on the complete campaign hierarchy. BCM does
not copy or regenerate these files. Vehicle navigation availability does not
implement vehicle reinforcements. Final movement validation remains required.

## Runtime evidence and failed attempts preserved

All evidence is outside the addon resource tree at:
`C:/Users/ruanb/Documents/BCM-Conflict-Core-TestEvidence/`.

- `M4X_DependencyAudit_2026-10-01` and `_B/_C/_D/_E`: installed-container/source
  audit, readOnly=1, no actors/ownership writes.
- `M4X_NativeSmoke_2026-10-01_A`: incorrect plain serialized prefab references
  resulted in missing inherited services; not an architectural verdict.
- `_B`: new resource GUIDs not registered yet; invalid RPL prefab identities
  and incorrect MOB component IDs exposed. Both corrected without vanilla edits.
- Workbench `logs_2026-10-01_13-50-07`: normal `-createDB` saved the BCM mapping
  at 13:50:07.735. Workbench subsequently exited 0xC0000005; database registration
  succeeded, but this is **not a clean interactive Workbench compile/load PASS**.
- `M4X_NativeSmoke_2026-10-01_C`: online state; three initialized bases; READY
  three nodes/two edges, revision 1, OBJ_01 legal, OBJ_02 locked, depth 0,
  ACTIVE, humansUS=0; MOB enabled and both forward spawns disabled.
- `M4X_PhysicalProof_2026-10-01_A`: first/root compile error in new proof
  (`%` applied directly to Count expression); fixed using typed integer locals.
  Downstream vanilla compile cascades were not patched.
- `_B`: US physical capture completed at 13:56:50.572; revision 2/depth 500,
  OBJ_02 legal, OBJ_01 response target and still non-deployable. M3.1 created
  a LIGHT force at 351.203 m. Movement failed at 13:56:55.486; empty navigation
  configuration was evidenced. Run stopped, logs preserved.
- `_C`: correctly assigned navigation files; operation failed closed at
  `target-navigation-anchor-unavailable` because streaming tiles were not ready.
- `_D`: requested tiles before physical capture and allowed a bounded five-second
  load interval before direct service creation. No M3 implementation change.
  US capture passed; operation created 14:06:52.041 with three USSR units at
  351.203 m; logged movement 296.741→245.851→188.359→137.891→87.7944→36.3106 m.
  Vanilla US→USSR ownership completed **14:09:14.607**, callback revision **3**,
  depth **0**, OBJ_01 BLUFOR-legal again, foothold=true, OPFOR pressure **300 s**.
  The driver prints COMPLETE pass=0 because it additionally required operation
  state SUCCEEDED. Existing M3 instead becomes CANCELLED at 14:09:15.206 with
  `campaign-left-active` after the legitimate pressure transition. **Preserve
  this failed strict assertion.** Physical movement/recapture/retreat are proved
  independently by the logs; an all-green operation/coordinator proof is not.
  No M3 workaround or relaxed assertion was applied.
- `M4X_PressureProof_2026-10-01_A`: clean separate two-position capture. OBJ_01
  US capture confirmed 14:11:07.671; OBJ_02 US ownership/BLUFOR pressure began
  14:11:13.872; revision 3/depth 1000/remaining 299/startCount 1 confirmed
  14:11:15.704; driver COMPLETE pass=1. Both ordinary spawns reject read-only
  `CanReserveFor_S(1)` with result **3002** before/after capture. Diagnostic
  player ID 1 is not an admitted human and is not a human deployment proof.
  Zero script errors, duplicate components, new-GUID failures or empty navmesh
  warnings in this run. Pressure expiry/end-match were not the test objective.
- Final normal Workbench indexing saved the BCM mapping at **14:13:21.799**;
  subsequent exit remained 0xC0000005. Do not
  interpret indexing success as successful interactive game creation.
- `M4X_NativeFinal_2026-10-01`: launched the new mission by its registered
  `{A4F2600000000000}` identity. Online **14:14:16.812**; registry READY
  **14:14:31.709**, revision 1/valid/ACTIVE/depth 0, OBJ_01 legal, OBJ_02 locked.
  USSR playable=0/playerLimit=0/CanCaptureBases=1. Zero script errors, proof
  starts, empty-navmesh warnings, new-GUID failures or duplicate components.
  Both forward read-only reservation checks reject 3002. This normal world
  contains no diagnostic driver; holds remain 300/300 and M3 enabled.
- `M4X_ProductionRegression_2026-10-01/logs/logs_2026-10-01_14-16-40`:
  unchanged production world entered online state **14:16:49.849**; registry
  READY **14:17:04.752**, 12 positions/21 edges. At **14:17:04.753–754**,
  selected HQ East was the sole enabled US MOB, all forward spawns disabled,
  evaluation VALID/ACTIVE/depth 0, foothold=false, pressure inactive and
  siegeHook=0. Zero script errors or prototype/Arland proof starts. This is a
  startup regression check, not a repeat of full production runtime acceptance.

No SteamDiag/client authentication attempts were performed. Task-owned test
servers are stopped by exact checked PID after evidence collection; forced
process termination is not described as graceful shutdown.

## World-service correction after manual Workbench validation (2026-10-01)

Creative Director confirmed normal interactive Workbench project load, game-script
compile, game creation, native world load and BCM MOB instantiation PASS. This
supersedes the earlier pending manual-validation gate; the historical automated
Workbench exit remains an environment observation, not a current project failure.
Human play has not begun.

The manual `World doesn't contain RadioManagerEntity to support any
BaseRadioComponent` warning identifies a genuinely missing engine radio service.
Installed vanilla `Prefabs/Systems/Radio/RadioManager.et` resolves to
`RadioManagerEntity`, one non-streamable RplComponent, zero children. Its generated
API supplies transceiver queries; it is not a strategic graph/campaign hierarchy.
The experimental Native.layer now authors one `TB_CC_NATIVE_RADIO_MANAGER` using
`{B8E09FAB91C4ECCD}`. No radio components were removed or copied.

Campaign base source initializes coverage and BaseRadioComponent (lines 670–680),
queries HQ coverage (1171–1205), and recalculates coverage on ownership change
(1590–1594). This makes a functioning radio service required for the retained
base-radio infrastructure, including MOB radios. It does not establish that every
spawn or AI capture primitive strictly depends on that manager: previous physical
capture passed without it. It is nevertheless required for a supported retained
vanilla radio/campaign environment before human acceptance.

The first corrected smoke exposed another missing service: `No SCR_LoadoutManager
found in the world, SCR_RespawnSystemComponent might not work as intended!`.
Actual installed respawn source lines 331–337 validates GetLoadoutManager().
The vanilla `{58FBD035E53D95C1}Prefabs/MP/Campaign/CampaignLoadoutManager.et`
provides SCR_LoadoutManager, inherited non-streamable replication and US campaign
player loadout entries. The experimental layer now authors one
`TB_CC_NATIVE_LOADOUT_MANAGER` from that prefab, unchanged. Its USSR/FIA loadout
entries do not alter faction playability; USSR restriction remains enforced.

| Service | Classification for retained prototype | Provision/action |
|---|---|---|
| RadioManagerEntity | REQUIRED for retained BaseRadioComponent functionality | Added vanilla RadioManager prefab only |
| Respawn/deployment and player spawn-point components | REQUIRED | Already inherited from GameMode_Campaign; do not duplicate |
| SCR_LoadoutManager | REQUIRED for normal retained human deployment path | Added vanilla CampaignLoadoutManager after evidenced warning |
| Campaign faction manager | REQUIRED | Existing authored CampaignFactionManager retained |
| AI world and soldier navigation | REQUIRED for autonomous physical AI | Existing authored Arland AIWorld and explicit baked navigation retained |
| Campaign/base manager, military-base and radio-coverage systems | REQUIRED by retained campaign bases | Existing mode/runtime systems; no extra world entity required by audit |
| Campaign building manager | REQUIRED support for retained construction functionality; not independently proved necessary for capture alone | Already inherited mode component; no separate manager added |
| Supply system/base supply components | REQUIRED for retained supply functionality | Existing runtime SCR_SupplySystem and inherited base components; no extra manager added |
| Game mode / replication | REQUIRED | Existing mode and inherited RplComponents; service prefab replication retained |
| Task UI/group/task solver/history components | OPTIONAL for BCM strategic capture; retained vanilla UI/task compatibility | Already inherited mode components; no additional task entity justified |
| Vanilla Conflict mission topology, two-HQ setup, random caches and campaign winner policy | NOT REQUIRED FOR CONFLICT CORE | No CTL/CTI hierarchy reintroduced; experimental policy remains BCM-owned |

This is a narrow audit, not proof of every optional vanilla construction/task
feature. No other absent manager was established as a blocker by these tests.

**Connectivity:** RadioManager supports engine transceivers, not BCM graph
eligibility or the metadata-only OPFOR origin. M4.4E remains retained. Adding this
service is not an A/B demonstration that the strategic-connectivity seam can be
removed. No connectivity policy changed.

**Map/minimap:** Installed vanilla data contains
`{B48C0B29FBEBA2A1}UI/Textures/Map/worlds/Arland/Arland.edds` and
`{F98F3D2CBA523091}UI/Textures/Map/worlds/Arland/ArlandRasterized.edds`.
The reported shorter `worlds/Arland/...` names do not by themselves prove these
packaged textures are absent: actual resolution context matters. The dedicated
smoke also reports `SCR_MapEntity: Cannot get the size from terrain. Using default.`
The final smoke positively instantiates inherited `MapEntity1` from
`{66FBC1063149819B}Prefabs/World/Game/Arland/MapEntity_Arland.et`; the map service
is therefore not simply absent. Terrain inheritance alone has not proved correct
map presentation/bounds. Eventual native presentation must resolve the vanilla
Arland map textures and correct Arland terrain bounds through this retained map
configuration. The exact cause of the interactive texture warnings remains
unverified without their full resource-resolution context. Do not copy a Conflict hierarchy
or invent replacement images. No map configuration was changed in this correction.
Dedicated startup cannot validate rendered map/deployment UI; verify those in the
next manual session and promote this to a blocker only if they impair gameplay.

Evidence is outside the addon tree in `BCM-Conflict-Core-TestEvidence`:
`M4X_ServiceAudit_2026-10-01`, `M4X_RadioServiceSmoke_2026-10-01`,
`M4X_LoadoutAudit_2026-10-01`, and `M4X_WorldServicesFinal_2026-10-01`.
Only experimental Native.layer and the isolated read-only dependency audit were
changed, plus this report. No production M4 resource, gameplay policy, proof
setting, player-count path or launch tool changed.

Final dedicated evidence: `M4X_WorldServicesFinal_2026-10-01/logs/
logs_2026-10-01_14-36-28/console.log`. Online **14:36:37.554**; READY/VALID,
three nodes/two edges, revision 1/depth 0/ACTIVE, OBJ_01 legal and MOB-only
deployment at **14:36:52.461**. Missing-radio-manager warnings **0**,
missing-loadout-manager warnings **0**, SCRIPT(E) **0**; new service prefab GUIDs
resolve without mismatch. Existing inherited sphere/SlidingTrackMaterial/Parent
diagnostics remain and are not represented as a completely warning-free world.
Proof mode 0, holds 300/300, M3 coordinator/garrison enabled, humansUS=0 remained
unchanged. Checked task-owned PID 18656 was stopped; all logs retained.

**Next gate:** required service gaps evidenced by this audit are resolved and
the prototype is ready to enter genuine-human interactive acceptance. Reopen
the updated native world normally in Workbench to confirm both services and
absence of their warnings before proceeding. No human test was run; actual
deployment, participant/observer counts and autonomous coordinator proof remain
pending. Map/deployment presentation must be checked there, not inferred from
this dedicated smoke. Migration remains unauthorized.

## Human-count and acceptance boundary

Creative Director manual dependency validation subsequently confirmed normal
Workbench project load, **5702 Game script files / 11207 classes compiled**,
successful game creation and native world load. Both previously missing manager
warnings are absent. The Arland minimap fallback will be assessed during human
deployment. This is an actual interactive Workbench result supplied by the
Creative Director, distinct from the earlier dedicated smoke.

`Tools/Run-M4X-NativeHumanAcceptance.ps1` prepares the next manual session. It
uses matched installed ServerDiag/SteamDiag binaries, the exact registered
`{A4F2600000000000}Missions/TB_CC_M4X_NativePrototype.conf`, addon
`6A6A5973FA53A5B1`, workspace-root working directories and fresh separate
Server/Client profiles under `Documents/BCM-Conflict-Core-TestEvidence/`.
It refuses to start with an existing diagnostic server/client, checks native
mission/world/proof-zero/M3-enabled serialization, waits for online RPL plus
three-node registry and MOB-only deployment, then launches exactly one visible
client after the established 257.705-second server settle interval. It observes
logs briefly without retrying rejected authentication and leaves the processes
for manual play. It does not create proof actors or set human counts or ownership.

Exact manual command from addon root:

```powershell
.\Tools\Run-M4X-NativeHumanAcceptance.ps1
```

The launcher was **created and statically parsed successfully**, but has **not
been executed**, including its `-DryRun` path. Therefore no new admission,
participant, observer, player deployment, human capture, autonomous coordinator
or minimap acceptance claim follows from launcher preparation. Server/client
logs from the eventual manual run will be in distinct profile folders beneath
the printed timestamped run root.

M3.7A query remains unchanged. Server has zero human players: humansUS=0 is
correct, not a failure. Production M3.2/M3.5 consequently will not autonomously
create an attack. No participant/observer/player-count override was introduced.
The existing SetDefaultPackageForProof helper was deliberately not used because
it internally calls SetProofHumanCount(1).

The separate actor proof invokes the **unchanged M3.1 operation service** after
physical US AI capture, using the genuine semantic target and existing staging
and assembly. It is **not** evidence of human deployment, production human
package selection or automatic M3.2 triggering. Those require an admitted real
US participant in a later human session; proof-service success cannot satisfy
the complete migration GO gate by itself.

## Acceptance A–Z (current)

| Criterion | Status | Evidence/boundary |
| --- | --- | --- |
| A no complete vanilla campaign hierarchy | PASS | Parent/resource inspection |
| B Arland world loads | PASS with inherited warnings | Dedicated native smoke and Creative Director manual Workbench world load/compile PASS |
| C BCM MOB | PASS | New inherited resource, initialized US HQ |
| D BCM objectives | PASS | Two instances of new BCM inherited resource |
| E inheritance, no full copies | PASS | Narrow prefab deltas |
| F no vanilla edits | PASS | No installed vanilla files edited |
| G registry READY | PASS | 3 positions/2 edges |
| H graph valid | PASS | Generic evaluator valid initial and advanced state |
| I US MOB | PASS | Runtime faction US, HQ=1 |
| J objectives initially USSR | PASS | Runtime ownership |
| K only MOB deployable | AUTHORITY CONTRACT PASS / HUMAN PENDING | MOB enabled, objectives disabled and read-only reservation denied 3002; real UI/deploy not tested here |
| L OBJ_01 initially legal | PASS | Public semantic query |
| M OBJ_02 initially locked | PASS | Query authorization false |
| N physical US capture | AI ACTOR PASS / HUMAN PENDING | Vanilla ownership callback; no real US human yet |
| O callback | PASS | Extended vanilla ownership callback |
| P advancement | PASS | revision 1→2, depth 0→500 |
| Q OBJ_02 legal | PASS | Query after capture |
| R OBJ_01 non-deployable | AUTHORITY CONTRACT PASS | Disabled after callback; reservation denied 3002 |
| S M3 semantic interface | SERVICE PASS / AUTONOMY PENDING | Existing service targets OBJ_01; coordinator retains real-human gate |
| T physical counterattack | SERVICE CREATED / COORDINATOR PENDING | Dedicated direct-operation proof is not autonomous coordinator activation |
| U physical movement | SERVICE PROOF PASS | 351.203 m to 36.3106 m, real waypoint movement; no teleport |
| V physical USSR recapture | SERVICE PROOF PASS | Vanilla US→USSR at 14:09:14.607; operation terminal-state assertion separately failed |
| W graph retreat | PASS | Revision 3/depth 0/OBJ_01 legal; OPFOR pressure with actual foothold |
| X no direct gameplay ownership | PASS for inspected/executed paths | No SetFaction in new scripts; transitions through vanilla |
| Y abstract origin, no enemy MOB | PASS | Metadata-only explicit start state |
| Z production preserved | AUTHORED RESOURCES UNCHANGED / STARTUP PASS | Two shared generic opt-in API changes disclosed; unchanged production 12-node/21-edge startup regression passed |

## Radio comparison

Physical proof B reported `vanillaRadioAllows=1` before US capture. The authored
MOB source radio can satisfy the playable-US predicate at OBJ_01 naturally.
However the existing M4.4E seam also remains available; no isolated A/B proof
has disabled that path. Do **not** claim it removable based on one true predicate.
Retain the existing strategic authorization seam for now. Before migration,
repeat the cycle with an explicitly isolated native-radio-only comparison and
verify authorization remains separate from radio coverage. Do not remove the
production override for architectural purity.

## Migration survival matrix — evidence, not historical rewrite

These classifications describe the observed contract/use; they do not assign
PASS to milestones whose native runtime requirements have not been repeated.

| System | Classification | Proven / remaining |
| --- | --- | --- |
| M1 | MINOR ADAPTATION | Vanilla capture/deployment/faction service reused; old automatic fixture must stay excluded; literal human deployment pending |
| M2 | SIGNIFICANT ADAPTATION | Semantic public query reused; linear-route/physical enemy endpoint scenario model not suitable for this native map |
| M3 | REUSABLE UNCHANGED, service scope only | Existing staging/assembly/operation physically recaptured; real-human coordinator and success-vs-pressure-cancellation semantics remain pending |
| M4.1 | REUSABLE UNCHANGED | Three-node registry READY with vanilla ownership adapters |
| M4.2 | MINOR ADAPTATION | New map-specific graph data using unchanged definition types |
| M4.2A | MINOR ADAPTATION | Explicit metadata-origin start state; physical-endpoint resolver retained for historical maps |
| M4.3 | MINOR ADAPTATION | Opt-in endpoint validation extension; existing selection/candidates/progression reused |
| M4.4 | SIGNIFICANT ADAPTATION | Transitional subclass ports registry/start only; needs map-neutral controller extraction before production migration |
| M4.4E | REUSABLE UNCHANGED provisionally | Used current seam; natural-radio predicate true but isolated removal comparison pending |
| M4.4H/J | REUSABLE UNCHANGED, partial runtime | Fresh ACTIVE, foothold acquisition, retreat-triggered OPFOR pressure and US 2/2 BLUFOR pressure proved; full timer cancellation/expiry/end-match suite not repeated |
| M4.4I | SIGNIFICANT ADAPTATION for authoring | Current 10-position production resources untouched; not imported into the native prototype |

## Risks and next gate

1. Correct AIWorld prefab alone is not sufficient: assign baked high-resolution
   navigation data, then prove movement; tile existence is not a completed path.
2. Real human deploy/respawn, observer/participant semantics and autonomous
   coordinator triggering remain separate required tests; never fake counts.
3. Transitional inheritance from an Arland-named integration class is deliberate
   code reuse, not the final map-neutral dependency direction.
4. Campaign task/service overhead and remaining low-level Campaign couplings
   require a measured pruning audit rather than wholesale removal.
5. Full interactive Workbench compile/load has now been confirmed manually;
   human deployment, map presentation and admission remain separate proofs.
6. Inherited Arland resource warnings are not attributed to BCM: Gizmo sphere,
   SlidingTrackMaterial and Parent fields were also logged in earlier baseline.
7. Baked Conflict navigation data may encode scenario obstacles; alternative
   vanilla terrain/navigation data should be compared if routes fail, without
   rebuilding vanilla AI or teleporting actors.
8. Last-territory recapture enters pressure before the next operation tick;
   existing M3 cancels on leaving ACTIVE instead of recording SUCCEEDED.
   Physical campaign progression works, but coordinator accounting needs
   explicit acceptance/review in the real-human native session. No production
   behavior was modified to make the diagnostic green.

**No production migration is authorized or recommended yet.** Physical reverse
capture is evidenced, but the strict service assertion is not green. The
next gate is a real-human MOB deploy→OBJ_01 capture→production M3.2
counterattack session.
Only after those proofs should a GO decision and a production migration
milestone be proposed. Network N1–N5 remain separate pending debt.

### Next manual session

Run the prepared launcher manually from the addon root. Its server must reach
online, READY/VALID revision 1 and MOB-only deployment before it starts the
single visible client. With an admitted US player, verify MOB deployment,
M3.7A participants=1/observers=1 while alive, then physically capture OBJ_01
and observe the production M3.2 coordinator. Never substitute a diagnostic
human count. The launcher has not been executed in this preparation task.
