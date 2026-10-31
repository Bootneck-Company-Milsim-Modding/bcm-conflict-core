# Evidence-driven three-faction corrective pass — 2026-10-02

Status: IMPLEMENTED / PRIOR LIMITED STATIC GATE REJECTED BY WORKBENCH /
COMPILE COMPATIBILITY CORRECTED / WORKBENCH GATE REQUIRED.
See [repository-wide compatibility correction](TB-Conflict-Core-Three-Faction-Compile-Compatibility.md)
for the authoritative follow-up: the omitted proof override was incompatible;
the expanded validator now covers all 45 Game sources and 54 overrides.
The next real compile exposed a mixed float/remainder expression; the latest
[Enforce syntax follow-up](TB-Conflict-Core-Three-Faction-Enforce-Operators.md)
records its correction and operator-screening limitation. Compile remains pending.
New runtime acceptance and genuine-human FIA combat remain PENDING.
No server, client, publishing or commit action was performed for this pass.

## Authoritative baseline and evidence

Creative Director brief: attachment `5ffbe6d7-337e-4c16-baf6-b6903ee5995a`.
Matching existing interactive evidence:
`C:/Users/ruanb/Documents/My Games/ArmaReforgerWorkbench/logs/logs_2026-10-02_11-05-48/console.log`
and `script.log`. These remain in their original locations, unmodified.

The accepted campaign portion starts around 11:07:58. Earlier editor-load/
authoring notices in the same Workbench console are not a new campaign radio
failure. The manually authored RadioManager is authoritative and untouched.
The accepted interactive runtime has working HQ radio eligibility; this pass
does not reopen the resolved manager investigation.

Accepted baseline remains twelve nodes, twenty-one edges, two conventional
HQs and ten FIA ordinary positions; all six directed relationships hostile;
native US objective tasks, physical captures, USSR movement/capture, normal
human presence and MOB-only deployment. The capture-superiority implementation,
world transforms/hierarchy, native AI/perception, faction relationships, task
bridge and HQ pressure/unlock/victory rules were not edited.

Key observations:

| Time | Evidence |
|---|---|
| 11:08:07 | Both frontier FIA placements initially fail; thirty-second retry scheduled |
| 11:08:37 | Both materialize; unqualified FIA group references emit zero-GUID warnings |
| 11:11:02 | Military South changes to US; five graph/radio-eligible candidates; Airport North wins old additive score 123 |
| 11:13:31 | Airport North changes FIA -> USSR; US=1, USSR=1, FIA=8; next USSR objective North Runway |
| 11:13:32 | M3 operation succeeds, then terminal assembly rollback removes two victorious groups |

## FIA resource and faction audit

`Group_FIA_Remnants.et` was referenced by path alone in the preceding BCM
three-faction package implementation. It is a real installed vanilla resource,
not an invented prefab. An unqualified ResourceName supplies no registered GUID,
which produces the displayed `{0000000000000000}` warning. BCM has NO fallback
from a failed FIA lookup to a USSR group: load/spawn failure returns null and
rolls back the candidate. The observed successful materialization therefore
does not establish that a fallback occurred.

Registered identities were read from the installed data `resourceDatabase.rdb`
and cross-checked against archive resources. The RDB record interpretation was
checked against the known USSR_Defenders GUID. No database was modified.

| Group | Registered GUID | Native character slots |
|---|---|---|
| Group_FIA_Remnants.et | F22EDFBEFC193357 | Rifleman, Scout, MG |
| Group_FIA_Remnants_MG.et | 8D2035F179B9E08F | MG, AMG |
| Group_FIA_Remnants_AT.et | 079F822A6E7FEA30 | AT, Rifleman |

Exact characters under `Prefabs/Characters/Factions/INDFOR/FIA/`:

| Character | Registered GUID |
|---|---|
| Character_FIA_Rifleman.et | 84B40583F4D1B7A3 |
| Character_FIA_Scout.et | BF1E43FF39AA526B |
| Character_FIA_MG.et | 58E47E5A4D599432 |
| Character_FIA_AMG.et | 75FC25863194612A |
| Character_FIA_AT.et | D25BC9815A9F9E8D |

Group inheritance is FIA_Remnants_Base (`m_faction "FIA"`) ->
Group_Remnants_Base -> native Group_Base. Remnants_Base disables immediate
member spawning, matching the existing explicit slot materialization path.
Character inheritance reaches Character_FIA_base with native
SCR_CharacterFactionAffiliationComponent affiliation FIA. Native equipment,
weapons, brain, perception and natural LOD are retained.

The former `ussrUnits` counter actually incremented when character affiliation
matched `m_sDefenderFaction`. For a FIA group it counted FIA characters under
a USSR label. Thus `groupFaction=FIA ussrUnits=3` does NOT demonstrate three
USSR characters. Diagnostics now report FIAUnits, USSRUnits, USUnits,
unknownUnits and otherUnits, plus actual slot ResourceName/effective faction,
agent count, natural LOD, defensive order and hostility toward US/USSR.

All three package references now include their real GUID. No soldier faction
is forced. An explicit contradictory affiliation still fails materialization;
missing/null affiliation is reported UNKNOWN, not fabricated as FIA.

The exact reason for the human-observed lack of FIA firing remains UNCONFIRMED.
Correcting references and misleading counters is not proof of a combat fix.
Human acceptance must still observe native perception, tactical movement,
firing and defence against both US and USSR. No forced targets/firing/activation
or global faction changes were added.

## Territorial scoring

Native US and USSR evaluators share configurable 40/30/20/10 component weights.
Legacy non-three-faction fixtures retain their prior scoring arithmetic.

- Importance: authored per-position 0–100 metadata, weighted to forty points.
- Proximity: horizontal distance to the current connected ordinary footprint;
  own HQ is used only before an ordinary foothold exists. Linear falloff to
  zero at configurable 1500 metres, maximum thirty points. Enemy HQ is never
  a distance destination.
- Consolidation: friendly connected neighbours / three, capped at twenty points.
- Connectivity: other hostile ordinary neighbour links / six, capped at ten.
- Seed variation: configurable 0–2 by default, maximum three; breaks EXACT
  weighted-score ties only, followed by stable-ID order. Never reverses a
  weighted territorial advantage.

Ordinary graph candidates exclude both HQs, require hostile ownership and a
resolved ordinary capture adapter, and come from the connected faction front.
US vanilla radio eligibility is checked before scoring. All ordinary native
positions remain unlocked by the accepted startup/capture architecture. Enemy
HQ selection is handled separately only when VULNERABLE; its protected or
pressured location contributes no ordinary score.

Workbench component fields expose weights, distance scale, tie variation and
an optional `m_aPositionImportanceOverrides` stable-ID/value list. An empty list
retains map metadata. Overrides affect importance only, not physical transforms,
graph edges, roles or ownership; values clamp to 0–100. No display-name conditionals.

Static ranking on the actual authored Arland graph, seed 20, default importance:

| US state | Ranking: normalized weighted score |
|---|---|
| HQ East only | Military South 58; Moss Hill 54 |
| Military South captured | Moss Hill 52; Gull Island 51; Airport North 50; Arleville 49; Farm 42 |
| Two nearby ordinary positions: Military South + Moss Hill | Arleville 68; Gull Island 51; Airport North 50; Farm 42 |
| Additionally Arleville | Signal Hill 58; Gull Island 51; Airport North 50; Beauregard Harbour 50; Farm 47; Beauregard 39 |

| USSR state | Ranking |
|---|---|
| HQ West only | Airport North 66; Farm 48 |
| Airport North captured | North Runway 56; Military South 53; Farm 50 |

These are read-only mathematical checks, not runtime strategic acceptance.
Graph candidates are assumed radio eligible for ranking; the actual US gate
still filters using current vanilla radio state. The five post-Military-South
radio eligibilities are supported by the supplied interactive evidence, but
future-state radio reachability must be observed at runtime. Gull Island stays
human-capturable; these distances are not a promise of foot travel across water.

## FIA territorial recapture

New campaign-scoped authority-only coordinator permits at most ONE FIA
operation. Default cooldown is sixty seconds, configurable on the native
component. It follows the existing real-US-human participation gate.

- Target must be ordinary, currently US/USSR owned, originally FIA territory,
  and adjacent to a currently FIA-owned ordinary foot-accessible origin.
- Most recently lost eligible FIA position has priority; importance then
  stable-ID order break ties. History is recorded from genuine ownership
  callbacks, not direct ownership writes.
- Actual staging occurs 60–120 metres around an adjacent FIA-owned base;
  bounded 24-candidate terrain/nav/support/player-distance/visibility checks
  and group separation are reused. Source and target must support foot access.
- Native group slots, agents and move waypoints are used; capture remains
  native and uses the unchanged aggregate-hostile superiority rule.
- Movement/capture timeout remains 300 seconds. Failed assembly rolls back;
  another attempt is delayed by cooldown, never retried in a tight loop.
- HQs and transport-required Gull Island are excluded from autonomous foot
  operations. No teleportation or claimed amphibious capability.
- Zero FIA territory leaves the faction intact but supplies no legal foot
  origin. Future guerrilla behaviour remains deferred.

No FIA/USSR operation creates a US-visible task. Runtime FIA recapture,
staging/path success and combat remain PENDING.

## Survivor consolidation and persistent occupation

The observed USSR disappearance is CONFIRMED: success immediately invoked
coordinator terminal rollback. The native territorial path now uses:

`physical capture -> CONSOLIDATING -> defensive orders -> garrison handoff -> operation cleanup`

CONSOLIDATING is appended as state 8; existing state numbers remain unchanged.
Default consolidation is sixty seconds (configurable, minimum ten). Movement
waypoints are replaced with native DefendSmall orders; surviving groups remain
alive. Success is declared after persistent defensive handoff, not merely
after deleting waypoints.

The occupation service owns the SAME group/character records after handoff;
the operation clears its array without deleting those entities. No identical
replacement force is spawned. Base reservations are made in the ownership
callback, before a frontier manager can spawn a duplicate during consolidation.
M3.6 respects both consolidation and occupation claims.

Handoff retries are bounded to three at ten-second intervals. If defensive
initialization fails, survivors transfer to an explicitly pending occupation,
the operation reports failure, and the service performs at most three further
order attempts. It does not claim combat/defence PASS or erase victorious AI
to hide the error. Runtime proof must confirm the normal ready handoff path.

Owned defences persist as infantry with defensive orders. Genuine owner change
or exhaustion retires their records; nearby human observers defer visible
deletion. A second recapture can retain an old-owner retirement record while
the new owner's defence is adopted. Safe cleanup is re-evaluated every two
seconds; world deletion removes all records/callqueue consumers deterministically.
Pressure suspension stops new operations but does not delete existing infantry
occupation. An in-flight successful capture transfers its survivors during
coordinator shutdown rather than rolling them back.

`GetOnDefenceEstablished()` is the future extension hook. No sandbags, static
weapons, logistics, vehicles, siege forces or fortification construction were
implemented. Native installed reuse candidates are:

- SCR_CampaignBuildingManagerComponent: game-mode construction/composition
  lifecycle and native resource/refund accounting.
- SCR_CampaignBuildingProviderComponent: base-bound provider budgets,
  resource access, cooldown and tracked group/agent lifecycle.
- SCR_CampaignBuildingCompositionComponent / LayoutComponent and native
  SCR_ResourceComponent: authored compositions and supply-backed construction.

They require their actual native provider/layout/resource contracts; this is
an audit of reusable services, not permission to spawn fortifications or
change budgets. No service/entity was added to the world.

## Retirement, placement retries and package selection

M3.6 retirement logging is transition-based by garrison/reason. Safety is
unchanged: no near-player owner-change deletion. Every normal tick continues
the cleanup check; once the observer leaves the exclusion radius, cleanup can
complete on the next two-second tick. Pending occupation retirement is also
transition-based.

Placement remains 32 candidates, three creation attempts per revision, thirty
seconds between failures. The evidence shows first failure at 11:08:07 and
success at 11:08:37. No retry/balance adjustment is justified by that successful
bounded recovery. If all three attempts fail, the existing per-revision stop
remains a limitation requiring fresh revision/restart and runtime observation;
it is not reported as guaranteed eventual recovery. Accepted USSR multi-axis
search/degraded-axis fallback remains unchanged.

LIGHT was selected because the observed run had one participating US human
and early depth. Corrected native FIA selection reaches every approved tier:

| Tier | Default selection context | Composition / units |
|---|---|---|
| LIGHT | Initial one-human/early context | 2 militia groups / 6 |
| STANDARD | >=2 humans, or importance >=80 with depth >=334 | 4 militia groups / 12 |
| REINFORCED | >=6 humans, or >=4 with importance >=80 and depth >=334 | 4 militia + 1 MG / 14 |
| STRONGPOINT | >=6 humans, depth >=667, importance >=80 | 4 militia + 1 MG + 1 AT / 16 |

Depth is the higher conventional faction depth. Strength below 667 caps higher
tiers at STANDARD; below 334 caps at LIGHT; zero suppresses conventional FIA
creation/recapture without destroying the faction. Strength remains configured,
not a new depletion simulation. Package IDs, quantities, maximum groups and
strength remain configurable; no final manpower/combat balancing claimed.

## Compile-readiness and next gate

Read-only validator: `Tools/Test-ThreeFactionCorrectiveStatic.py`.

The initial limited pass checked eleven affected/dependent scripts; delimiters; 104 PrintFormat calls
against the installed nine-value limit; 94 unique BCM class declarations;
27 complete override return/type/name/order comparisons against actual local/
installed parents; unchanged 13-field snapshot ordering/parser count; real
registered FIA group AND character GUIDs; FIA affiliation inheritance; capture
boundaries; graph/ranking matrix; FIA tier reachability, recency, HQ/transport
exclusion and zero-territory behavior; absence of invented activation and
gameplay ownership writes; bounded string formulas. That scope missed an
incompatible IntegratedProof override and was insufficient as a repository-wide
compatibility gate. The linked follow-up supersedes this limited result with
45-file/479-format/54-override/137-caller checks and records tooling limits.

Installed API references reviewed include SCR_AIGroup AddAgentFromControlledEntity
(void; registration verified by count), SetFaction/GetFaction, native AIGroup
GetAgents/GetAgentsCount/AddWaypoint/RemoveWaypoint, entity spawn/resource API,
existing navmesh/tile/terrain queries, FactionAffiliationComponent, ScriptInvoker,
callqueue lifecycle, Math.Round/Clamp, BaseContainerProps and container operations.
No new engine RPC, custom activation, or forced LOD/target API was introduced.

Source/config status preserved: proof=0, holds=300/300, M3 coordinator/garrison
enabled; authored managers, HQ affiliations, capture resources and transforms
unchanged. The old production implementation and all historical evidence remain.

Changed game sources: NativeArland, StrategicEvaluator, Operation,
TacticalStagingPolicy, ForcePackages, FrontlineGarrison, and new
TB_CC_TerritorialOccupation.c. Added static validator and this report; updated
the foundation integration document to point to this corrective status.

WORKBENCH GATE REQUIRED: compile normally, process/load the existing production
world. Static checks are not a compiler or new runtime PASS. After that gate,
focused runtime evidence must establish FIA GUID warning removal/materialization,
genuine FIA recapture, territorial ranking/radio behavior, consolidation with
same surviving groups and no duplicate defence, safe retirement and deployment/
HQ regressions. Real-human FIA combat remains a separate required observation.
Do not publish on the strength of this static pass.
