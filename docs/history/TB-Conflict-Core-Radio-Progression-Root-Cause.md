# Native radio progression correction — 2026-10-03

Status: source defect identified and corrected; WORKBENCH GATE REQUIRED. No dedicated session, human test, publishing or commit performed. Runtime correction acceptance remains pending. Chat/GM and previous warning cleanup remain accepted.

## First broken link and evidence boundary

BCM's authored startup bypasses `SCR_GameModeCampaign.InitalBaseSetup` and initializes already-owned bases directly. It called `base.Initialize()` and `SetAsHQ(true)` but never `ChangeRadioSettings` for those authored owners. Initialize powers the coverage radio; SetAsHQ assigns the faction main base. Neither applies the faction radio encryption/frequency.

The supplied runtime evidence independently shows US-owned HQ East with `encryptionMatches=0`, `source=1`, `selectedSource=0`, while HQ West is a selected source and all ordinary bases report receive/transmit coverage despite mismatched US keys. This is not evidence of a healthy US radio origin. Structural source eligibility is not faction-correct source operation.

Native capture changes the captured base radio to the new faction key. Native HQ reachability, however, asks for the HQ radio's **actual** key. Thus a captured US relay can cease forwarding the key still used by the incorrectly initialized HQ. A component can receive a signal under another key but cannot relay it under that key. This explains the observed loss rather than extension of reachability after capture.

Confirmed: missing startup radio setup in current source; reported HQ key mismatch; native key-specific propagation and capture update. Supported causal explanation: this split causes the reported post-Moss stall. Historical logs do not expose the literal keys or every intervening network event, so an exact historical source-key collision and the complete dedicated recovery are not claimed proven. Zero antennas may additionally limit range; they do not explain or repair a mismatched HQ key.

## Installed 1.8.0.13 authority

All paths below are inside the installed vanilla `addons/data/data007.pak`, read with `Tools/Read-InstalledVanillaResource.py`; no vanilla files edited. The inspected `Missions/23_Campaign_Arland.conf` points to `{DC924A8DDECC73AD}worlds/MP/CTI_Campaign_Arland.ent` and `Configs/Systems/ConflictSystems.conf`. BCM retains native base prefab inheritance rather than inheriting that campaign topology.

- `scripts/Game/Campaign/SCR_CampaignMilitaryBaseManager.c:453`: vanilla HQ faction assignment. `:583–618`: normal base initialization assigns faction when absent using SetFaction, and callsigns.
- `scripts/Game/Components/Locations/SCR_CampaignMilitaryBaseComponent.c:514–525`: Initialize only powers radio and initializes services. `:1015–1044`: SetAsHQ/OnHQSet sets faction main base, not encryption.
- Same file `:2212–2224`: SetFaction(SCR_CampaignFaction) returns early for an unchanged faction. Re-setting the same owner is not a radio repair.
- Same file `:1550–1595`: native OnFactionChanged invokes base logic, extended event, then authority-side ChangeRadioSettings and coverage recalculation. BCM's strategic callback observes this native event; it does not replace this method.
- Same file `:1716–1749`: public `void ChangeRadioSettings(notnull SCR_Faction faction)` sets faction encryption, checks frequency bounds, sets frequency, requests `SCR_RadioCoverageSystem.UpdateAll`, refreshes nearby seizing timers.
- `scripts/Game/Components/Locations/SCR_MilitaryBaseComponent.c:534`: native base logic receives owner changes. `scripts/Game/GameMode/Components/SCR_MilitaryBaseLogicComponent.c:80–87` updates attached provider/service affiliation. Provider affiliation and radio encryption are distinct systems.
- `scripts/Game/Systems/SCR_RadioCoverageSystem.c:93–144,175–218,263–287`: source selection by encryption key; power/range/key changes detected; native forward/reverse coverage traversal. No manual re-registration is needed for an owner/key change.
- `scripts/Game/Components/SCR_CoverageRadioComponent.c:275–305`: Ping records receive/send coverage before checking matching key and power for further propagation. `:338–341`: native radio registration.
- Campaign base `:1171–1220`: GetHQRadioCoverage uses faction.GetMainBase(), reads that HQ radio's actual key, then queries the destination coverage. IsHQRadioTrafficPossible defaults to RECEIVE; it does not require BOTH_WAYS or matching destination ownership.

## Vanilla / BCM comparison

| Component or path | Vanilla | BCM before correction | Impact |
|---|---|---|---|
| Campaign base | ConflictMilitaryBase.et | TB_CC_StrategicBase inherits it | Native registration/capture/services retained |
| Faction | Normal campaign assignment and owner callbacks | Ten authored FIA bases; US/USSR authored HQs | Initial assignment can predate radio setup; later capture callbacks still run |
| SCR_CoverageRadioComponent | Powered by Initialize; relay transceiver 1,000 m | Inherited unchanged | No missing radio component or custom range |
| HQ | ConflictBase_MOB.et, m_bIsSource=1; native main-base reference | TB_CC_ArlandHQCandidate inherits it, SetAsHQ called | Structurally valid, but reported US encryption binding was wrong |
| Radio settings | Native ChangeRadioSettings on faction transition | Missing explicit setup for already-authored owners | First demonstrated lifecycle omission; corrected |
| Building provider | Native service composition, registered master provider | Native composition/providers observed present | Presence is not radio traffic |
| Supplies | SCR_ResourceComponent consumer/generator and service storage | Inherited unchanged; runtime snapshot can be zero | Actual available supplies must be checked; no free antenna or supply injection |
| Construction | BuildingService_base master provider, radius 150, TRAIT_SERVICE, native build action | Inherited; no BCM catalog removal found | Source supports antenna build option; human visibility/access pending |
| Antenna | Native FreeRoamBuilding catalog; service range extension | Same inherited path | No custom antenna needed |
| Ownership hooks | Affiliation -> base logic/providers -> radio update -> coverage | Physical capture retains native lifecycle; virtual capture uses native base.SetFaction | No demonstrated missing capture-time setter; left unchanged |
| RadioManager | Native manager plus SCR_RadioCoverageSystem world system | Existing authored manager retained | No manager/layer/world edits |

## Diagnostic meanings

All current node diagnostics are evaluated relative to US, even for FIA/USSR nodes.

| Field | Actual meaning/API |
|---|---|
| providerPresent / providers | Nonempty/count of base.GetBuildingProviders; structural construction providers, not radio transmitters |
| masterProvider | base.GetMasterProvider non-null; construction provider, not selected radio source |
| providerFaction | Master provider owner's FactionAffiliationComponent; not radio encryption |
| compositionPresent | base.GetBaseBuildingComposition non-null |
| relayPresent | First transceiver casts to RelayTransceiver |
| relayOperational | BCM composite: relay present, powered, registered, key matches configured US key, and native HQ receive query true; **not a native API or proof of useful outgoing coverage** |
| registered | Membership in SCR_RadioCoverageSystem.m_aRadioComponents; not faction-specific registration |
| baseRegistered / initialized | Campaign base-manager membership / base.IsInitialized |
| source | radio.IsSource: eligible to originate coverage |
| selectedSource | Encryption-source map currently points to this radio, under its own key; does not imply the key belongs to US |
| powered / range | radio.IsPowered / first transceiver.GetRange |
| encryptionMatches | Radio key equals configured US faction key |
| receiveCoverage / transmitCoverage | IsHQRadioTrafficPossible(US, RECEIVE/SEND), using **HQ actual key**, not configured US key |
| upstream | Powered incoming neighbours with configured US key and receive coverage under that key; not a unique path or guaranteed HQ ancestry |
| antennas / onlineAntennas | Registered RADIO_ANTENNA services / those reporting ONLINE |
| suppliesSnapshot | base.GetSupplies at log time, not guaranteed build-budget affordability |
| hqEncryptionMatches (new) | Actual US HQ key equals configured US key; disambiguates apparent traffic under the wrong key |

Node logs now explicitly label `coverageReference=HQ-actual-key operationalReference=US-faction-key`. Keys themselves are not printed.

## Antenna / construction

Captured bases do **not** require a built antenna to relay within their inherited 1,000 m range once correctly keyed, powered and reached. Longer links require native range extension.

Native build item:
`{F7DC8BB193BCAF44}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatSmall/E_Antenna_S_US_01.et`

It is listed in `Configs/Editor/PlaceableEntities/Compositions/Compositions_FreeRoamBuilding.conf:26`, labelled `SLOT_FLAT_SMALL`, `TRAIT_SERVICE`, `SERVICE_ANTENNA`, `FACTION_US`, and costs 250 CAMPAIGN supplies. It inherits `{8B4B8BF44162B24E}Prefabs/Compositions/Slotted/SlotFlatSmall/Antenna_S_US_01.et`; the editable child references `E_Antenna_01_US.et`. Native Antenna_01_US inherits Antenna_01_base, whose relay range is 2,000 m. Antenna_Base supplies SCR_AntennaServicePointComponent, RADIO_ANTENNA and SERVICE_ANTENNA.

Use normal base construction at the captured base's native provider and a valid flat-small placement. Do not introduce a fabricated fixed world slot. Provider faction, rank, access/enemy obstruction, supplies and placement checks still apply. BuildingService_base has a master provider, registration-at-base and native SCR_CampaignBuildingStartUserAction; its resource consumer is Consumer_FreeBuilding.conf. Availability is established in source, not yet by a human construction UI test.

Campaign base OnServiceBuilt / OnServiceRegistered calls RecalculateRadioRange. That method takes registered antenna relay range, turns the antenna radio off to avoid redundant coverage work, and applies the range to the **base** relay. OnRadioRangeChanged schedules native coverage refresh. Therefore antenna radio powered=false is not itself failure. No invented requirement that every radio be powered, or that the onlineAntennas diagnostic alone determines range, is imposed.

## Native map lines

`SCR_RadioCoverageMapDescriptorComponent.c:58–132` creates links on map opening. Source radio encryption must match the local player's nonempty faction key; coverage must be RECEIVE or BOTH_WAYS. It links radios in outgoing range with visible target MapItems, avoiding duplicate links. Native ColorMapLink handles styling. These are not BCM strategic graph edges and their absence alone does not prove no radio traffic.

The old mismatched HQ key fails the native source-side presentation condition as well. Retained native base descriptors support the path. Reopen the map after network changes during acceptance; do not infer that a previously open map must redraw immediately. No custom lines added.

## Exact correction and preservation

In TB_CC_M4XP_InitializeBasePolicy, immediately after each base.Initialize(), obtain its existing GetCampaignFaction(), fail initialization if absent, and call native base.ChangeRadioSettings(radioFaction). This is one-time native configuration of authored ownership, not an ownership mutation, fabricated coverage, range increase, antenna spawn, or polling repair. It uses the same public native method as real capture. No Arland IDs/coordinates are used in the added logic.

Added read-only ownership-transition observation: the extended owner event runs before native radio changes, so snapshot registration there and log after 250 ms. The log records requested old/new owners, observed native/provider faction, registration before/after, antenna/service state and native US traffic. `refreshTriggered=native-OnFactionChanged-path` identifies the installed callback path, not an assertion that async propagation has finished. Existing change-only node logs supply later settled state. Pending callbacks are removed on Stop.

Coverage subscription, 250 ms deferred reselection and 10-second RADIO_WAIT retry are unchanged. No world, prefab, mission, RadioManager, range, supplies, graph, scores, commitment, capture, M3, FIA, war, chat or GM configuration changed. Layer hierarchy and strategic positions remain intact.

## Next dedicated acceptance (after normal Workbench gate)

Use the existing production native mission `{A4F3500000000000}Missions/Production/TB_CC_Arland.conf`, proof=0, holds=300/300, M3 enabled. The shortened GUID in the task brief is not used.

1. Start clean. Confirm 12 nodes / 21 edges; HQ East US and correctly keyed/selected source, HQ West USSR; correct source references; no startup pressure. Confirm Military South and Moss Hill RECEIVE coverage. Stop on any mismatch.
2. Physically capture Military South through vanilla capture. Preserve old/new owner and transition log; wait for settled native coverage. Moss Hill must remain committed. Confirm US base/provider/key, registration and no forward deployment.
3. Inspect normal construction UI. If extending range, deliver required supplies and build the native US antenna (250 supplies) with normal access/placement. Record service registration, resulting base range and coverage event. Do not use GM construction as human UI evidence.
4. Physically capture Moss Hill. Repeat ownership/key/provider/registration checks. Inspect/build native antenna if required by the actual link distance. Do not force a candidate or ownership.
5. Observe native coverage notification and deferred reselection (10-second fallback remains). Require a graph-legal next target with native radioReachable=1; record actual selected target, not a forced Arleville result. Reopen map and inspect native link presentation.
6. Preserve server and retail-client logs. Separate authority traffic, objective selection, build UI and map-line evidence. A-M dedicated acceptance remains UNVERIFIED until this sequence runs.

## Validation and changed files

Changed: TB_CC_M4XP_NativeArland.c; TB_CC_NativeRadioObserver.c; Tools/Test-NativeRadioStatic.py; this document.

Static regression adds installed API checks for GetCampaignFaction/ChangeRadioSettings and exact callback ordering; startup call count/order; deferred diagnostic teardown; independent key-split model. Existing directed range-extension and commitment models remain.

Whole-tree suite: Tools/Test-NativeChatFullChain.py completed with exit code 0 after the final edits. Its chained installed API, source/override checks, native resources, 12-node/21-edge model, objective commitment, FIA continuity, physical/virtual war, chat and GM checks PASS. It screened 54 Game sources and 18 script-defined modded targets; campaign model covered 390 ownership subsets / 1516 candidate transitions. Static checks are not an Enforce compiler or resource-processing/runtime proof. The local packaged archive comparison reports an existing A_Managers difference from the accepted cache-config cleanup; this is not a claim about the hosted Workshop version and was not republished. Some inherited validator status text still refers to historical chat runtime unknowns; this does not revoke the Creative Director's accepted chat/GM result.

WORKBENCH GATE REQUIRED: compile the Game module normally and load the production world. Then perform the dedicated acceptance above. No claim of compile, resource-processing or radio-runtime PASS is made by this correction.
