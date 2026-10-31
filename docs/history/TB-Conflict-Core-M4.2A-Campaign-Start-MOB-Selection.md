# M4.2A — Seeded Campaign Start and MOB Candidate Model

> **M4.4H product amendment:** the selected MOB and its opposing HQ endpoint are permanent faction anchors and are never ordinary strategic capture objectives. Their relationship still selects campaign direction and deployment anchor. Victory is based on sustained control of all required ordinary strategic positions between them.

Status: **PASS — campaign-start infrastructure only**

M4.2A introduces deterministic selection of one campaign MOB and its opposing endpoint. It does not initialize factions, alter spawn points, select an objective, mutate ownership, or persist campaign data.

## Arland suitability audit

The audit ran against every real `ARLAND_PRODUCTION_01` vanilla base. All eight exposed a campaign spawn component, radio component, two construction providers, and three services. HQ capability, strategic depth, supply baseline, location, and graph role distinguish a production MOB from an ordinary capturable base.

| Position | Runtime capability | Graph/opening assessment | Classification |
|---|---|---|---|
| `ARLAND::HQ_EAST` | HQ; spawn enabled; radio 800; supplies 1000; income 10; 2 providers; 3 services | Southern end; opens to Military South and Moss Hill; opposite HQ reachable with depth | **VALID_MOB_CANDIDATE** |
| `ARLAND::HQ_WEST` | HQ; spawn enabled; radio 1000; supplies 1000; income 10; 2 providers; 3 services | Northwestern end; opens to Airport North and Farm; opposite HQ reachable with depth | **VALID_MOB_CANDIDATE** |
| `ARLAND::MOSS_HILL` | Not HQ; spawn enabled; radio 1000; supplies 179; income 1 | Boundary/high-ground position; selecting it skips the southern campaign end | NOT_MOB_SUITABLE |
| `ARLAND::MILITARY_SOUTH` | Not HQ; spawn enabled; radio 1000; supplies 215; income 10 | Interior military node with four graph neighbours; too central | NOT_MOB_SUITABLE |
| `ARLAND::ARLEVILLE` | Not HQ; spawn enabled; radio 1000; supplies 654; income 1 | Interior population/transport hub; starting here shortens both directions | NOT_MOB_SUITABLE |
| `ARLAND::BEAUREGARD` | Not HQ; spawn enabled; radio 1000; supplies 1540; income 10 | Peripheral, but a settlement branch rather than an HQ; only two neighbours | NOT_MOB_SUITABLE |
| `ARLAND::FARM` | Not HQ; spawn enabled; radio 1000; supplies 75; income 0 | Central logistics junction; five-neighbour start would erase campaign depth | NOT_MOB_SUITABLE |
| `ARLAND::AIRPORT_NORTH` | Not HQ; spawn enabled; radio 1000; supplies 175; income 0 | Important strategic airfield but not an HQ; near HQ West and too deep for a southern endpoint | NOT_MOB_SUITABLE |

The candidate decision is not based on spawn-component presence alone. The two HQs provide the coherent paired start/end geometry and strong baseline logistics. The other six stay capturable strategic positions.

## Generic authored contract

Both Arland HQ position definitions now carry `mobCandidate=true`. `TB_CC_M4_2A_CampaignStartDefinition` authors stable-ID MOB-to-endpoint relationships independently of terrain-specific core logic:

| Selected MOB | Opposing endpoint | Initial enemy boundary |
|---|---|---|
| `ARLAND::HQ_EAST` | `ARLAND::HQ_WEST` | Military South, Moss Hill |
| `ARLAND::HQ_WEST` | `ARLAND::HQ_EAST` | Airport North, Farm |

The resolver validates that each MOB exists, is authored as a candidate, has one unique relationship, differs from its endpoint, and can reach that endpoint through the graph.

## Seed and selection

The seed is a signed 32-bit integer suitable for later serialization. Candidate IDs are first sorted lexicographically, making selection independent of definition order and entity enumeration. Seeded mode uses the stable modulo index:

```text
sorted valid MOB IDs
index = absolute(seed modulo candidate count)
selected MOB = sorted[index]
endpoint = authored relationship for selected MOB
```

For seeds 0–15, even seeds selected HQ East and odd seeds selected HQ West. Each seed was resolved twice and returned the same MOB and endpoint.

`TB_CC_M4_2A_CampaignStartState` is the future persistence boundary:

- seed;
- start mode;
- selected stable MOB ID;
- selected stable endpoint ID.

A new/reset campaign generates or receives a seed and resolves once. Resume supplies the stored state; the resolver validates and reselects the stored MOB explicitly, so restart does not reroll. No storage implementation is included.

## Manual override

Modes are:

- `SEEDED`: deterministic selection from sorted valid candidate IDs;
- `EXPLICIT`: select a supplied authored candidate ID.

An explicit valid HQ selects exactly that HQ and its paired endpoint. `ARLAND::FARM` was tested as an explicit non-candidate and failed closed with `explicit-mob-invalid-ARLAND::FARM`.

## Initial faction and deployment contract

Vanilla Arland HQ faction assignment varied between clean sessions. Production must therefore not infer roles from whichever vanilla HQ happens to be US at startup.

Before graph campaign activation, the production scenario initializer must use supported scenario/faction configuration to establish:

- selected MOB owner: US;
- selected opposing endpoint: USSR;
- remaining authored initial ownership according to the future campaign-start ownership profile;
- exactly one enabled/visible/reservable US campaign spawn: the selected MOB.

This is initialization configuration, not a mid-campaign ownership shortcut. M4.2A does not perform it. The non-selected HQ candidate becomes an ordinary enemy endpoint for that campaign, and all non-selected bases/candidates must be unavailable for player deployment. Captures must never enable forward deployment.

The runtime audit found that the unmodified vanilla scenario currently exposes enabled spawn components on all eight bases. That is evidence for the required future initialization filter, not acceptance of vanilla deployment behavior for the new graph campaign. Existing Everon/M3 deployment remains untouched.

## Runtime proof

Final evidence: `TestProfiles/M4_2A_Final_2026-09-30_20-09-41/logs/logs_2026-09-30_20-09-41/console.log`

- real Arland world entered online state;
- production registry reached READY with two authored MOB candidates;
- seeds 0–15 exercised both candidates;
- repeated reconstruction was deterministic;
- explicit HQ East succeeded;
- explicit Farm failed closed;
- stored-state resume retained MOB and endpoint;
- both MOB opening candidate sets passed;
- all eight vanilla capabilities were logged;
- completion reported `ownershipMutation=0 deploymentMutation=0 objectiveSelection=0`.

The test also exposed and corrected an M4.1 lifetime defect: resolved registry positions now hold their definition through a strong `ref`, preventing definition loss after a builder object goes out of scope.

## Acceptance A–S

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Position data supports multiple candidates; generic start relationships added. |
| B | PASS | All eight bases audited at runtime. |
| C | PASS | Only the two real HQs are candidates. |
| D | PASS | Seeds 0–15 resolved identically on repetition. |
| E | PASS | Candidate list is stable-ID sorted. |
| F | PASS | Seed matrix reached both HQs. |
| G | PASS | Generic explicit mode implemented. |
| H | PASS | Explicit Farm failed closed. |
| I | PASS | Reciprocal authored endpoint relationship follows selected MOB. |
| J | PASS | Resolver rejects identical MOB/endpoint. |
| K | PASS | Each opposite HQ is reachable with meaningful depth. |
| L | PASS | Required deterministic US/USSR initialization contract documented. |
| M | PASS | Contract requires only selected MOB to remain deployable. |
| N | PASS | Non-selected HQ becomes ordinary strategic endpoint, not another spawn. |
| O | PASS | Both opening sets and endpoint reachability passed. |
| P | PASS | Proof reports `objectiveSelection=0`; no selector exists. |
| Q | PASS | No M3 consumer changed. |
| R | PASS | Everon resources/evaluator unchanged. |
| S | PASS | Workbench processing, compile, real Arland load, and runtime proof passed. |

## Remaining unknowns and M4.3 impact

- The exact supported production-world configuration used to seed faction ownership and enable only the selected spawn must be implemented and accepted during graph-campaign integration; direct gameplay ownership mutation is prohibited.
- Persistence must eventually store the complete `CampaignStartState` before a campaign can resume safely.
- Negative and externally generated seed policy can be broadened later without changing stable-ID selection semantics.

M4.3 must receive an already resolved `CampaignStartState`. Its friendly-component search begins at `selectedMobId`; its endpoint eligibility uses `selectedEndpointId`; and it must never choose a MOB or reroll a seed. M4.3 still selects one objective only after boundary eligibility and must not implement persistence or deployment initialization.
