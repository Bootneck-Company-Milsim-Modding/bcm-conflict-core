# M4.4I — Arland strategic expansion

Subsequent human play exposed the fresh-campaign pressure defect. Historical
capture/graph proof remains preserved, but production-start acceptance is
superseded by [M4.4J](TB-Conflict-Core-M4.4J-Foothold-Pressure.md). Do not interpret
the earlier pressure-mechanics PASS as proof of a correct new-campaign gate.

2026-10-01: **M4.4I DEDICATED AUTHORITY RUNTIME SUITE PASS.** The Creative Director approved all four distinct actual capture locations, manually inspected and processed the cleaned world. Final production smoke passed. See `TB-Conflict-Core-M4.4I-Runtime-Acceptance.md` for the authoritative runtime evidence and both A–AE matrices. Overall M4.4 criterion U remains PENDING HUMAN; N1–N5 remain separately PENDING.

**Resource hygiene:** archived duplicate addon copies remain outside the addon; final smoke has no duplicate GUID/metafile errors. Inherited vanilla warnings are acknowledged. The missing old markers are retired, not a blocker; no marker recreation is required.

The actual capture entities and registered M4 positions are now authoritative. The old placement markers served their authoring purpose and are no longer runtime or authoring dependencies.

## New source positions

All IDs below have prefix `ARLAND::`. All use VANILLA_CONFLICT_BASE backing.

| Marker | Stable ID / capture entity | Position X Y Z | Type / importance | Traits |
| --- | --- | --- | --- | --- |
| TB_MARKER_BEAUREGARD | BEAUREGARD_HARBOUR / TB_CC_BeauregardHarbour | 3112.44 44.02 2419.688 | PORT / 70 | POPULATION_CENTER, TRANSPORT, LOGISTICS, INFRASTRUCTURE |
| TB_MARKER_SIGNAL_HILL | SIGNAL_HILL / TB_CC_SignalHill | 2530.229 144.135 2286.244 | OUTPOST / 80 | MILITARY, COMMUNICATIONS, INFRASTRUCTURE |
| TB_MARKER_NORTH_AIRFIELD | NORTH_RUNWAY / TB_CC_NorthRunway | 1674.865 37.888 3003.17 | AIRFIELD / 85 | LOGISTICS, SUPPLY, TRANSPORT, INFRASTRUCTURE, MILITARY |
| TB_MARKER_GULL_ISLAND | GULL_ISLAND / TB_CC_GullIsland | 962.309 31.444 1395.995 | OUTPOST / 70 | COASTAL_CONTROL, TRANSPORT_REQUIRED |

Display names and metadata encode approved design concepts, not a verified vanilla location-name survey. Exact footprint, terrain/slope, shoreline, boat/landing access and navmesh suitability remain editor/runtime inspection debt. Harbour elevation must be checked physically. Importance never bypasses eligibility.

## Capture architecture

`Prefabs/Systems/MilitaryBase/TB_CC_M4_4I_StrategicCapture.et` derives from the existing no-player-deployment base and ultimately vanilla ConflictMilitaryBase. It retains campaign/seizing machinery and the M4 authorization/ownership callback; authors USSR affiliation, radius 100, non-control-point and non-HQ status.

Inherited deployment marker/disabled spawn group and existing M4 policy exclude forward deployment. Spawn resources are retained disabled, not claimed absent. No new adapter or gameplay SetFaction call was added; existing campaign-start initialization is unchanged. Vanilla prevailing faction, timer, interruption and CaptureFinish remain underneath, but require physical proof for the new resource.

The campaign layer's stale prefab path into TestProfiles was corrected to the live production system prefab with the same GUID. Historical test evidence was not edited.

## Graph and depth

Static source checks: **12 nodes, 10 ordinary, 21 undirected edges**; no self-edge, duplicate edge or missing endpoint. All ten ordinary positions are reachable from either HQ without traversing the opposite HQ.

Original thirteen edges retained; eight added:

- Harbour—Beauregard and Harbour—Arleville.
- Signal Hill—Farm, Signal Hill—Arleville and Signal Hill—Harbour.
- North Runway—Airport North and North Runway—Farm.
- Gull Island—Military South.

These are authored strategic relationships, not proven road/navmesh routes. The DOT artifact includes all nodes, marker identities and edges.

East opening remains Military South/Moss Hill. Military South opens the island branch. West opening remains Airport North/Farm; Airport North opens North Runway, while Farm opens North Runway/Signal Hill alongside existing mainland successors. Signal Hill opens Harbour, reconverging with Arleville/Beauregard.

Depth and pressure remain generic, excluding both HQs. Depth: 0,100,200,300,400,500,600,700,800,900,1000. EARLY 0–334, MID 335–667, LATE 668–1000 unchanged and naturally reachable. Victory requires all ten ordinary owners; no hard-coded ten added to campaign logic.

## M3 / island

M4 injects TRANSPORT_REQUIRED objectives into the generic staging policy's unsupported-foot-target list. M3 contains no Arland-ID hard-code. Staging returns unsupported-transport-requirement before assembly spawns groups/characters; source ordering checked. No teleport or transport implementation added.

Gull Island remains a human strategic objective. M3.6 local garrison placement is unaffected by the attack guard and remains unverified on island terrain. New mainland placement/staging also needs runtime acceptance.

## Validation / acceptance

All four positions physically captured and passed interruption/ownership/revision/deployment checks in clean East and West mode-9 runs. Exact depth 0–1000 in steps of 100 was observed. Both full BLUFOR paths and the clean OPFOR pressure path passed. Duplicate terminal notifications were idempotent and each clean run bridged end-match once.

Runtime foot staging denies Gull Island with unsupported-transport-requirement and no actors. Read-only production garrison placement predicates found valid local positions at all four new centres. Full human-triggered garrison materialization at each new site is not claimed.

A supplemental old proof logger dereferenced a null M3 selector during pressure; its narrow diagnostic-only null guard was verified in a corrected INVALID run. Failed evidence is preserved. No production gameplay/balance/rules were changed during acceptance.

Final source remains proof mode 0, holds 300/300, M3 coordinator/garrison enabled. Compiled dedicated production smoke loaded twelve positions/twenty-one edges/ten ordinary, one enabled MOB, and the production 300-second timer with no proof starts or BCM script/metafile/duplicate errors.

Full evidence, exact log paths and individually resolved M4.4I / overall M4.4 A–AE matrices: `TB-Conflict-Core-M4.4I-Runtime-Acceptance.md`. M4.4I dedicated suite PASS; overall M4.4 criterion U remains PENDING HUMAN, N1–N5 separately PENDING. No automatic commit or M4.5.
