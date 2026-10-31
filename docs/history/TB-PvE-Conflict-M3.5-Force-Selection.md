# TB PvE Conflict M3.5 — Strategic Infantry Force Package Selection

## Verdict

**M3.5 PASS.** A clean dedicated ServerDiag authority run proved deterministic package selection, zero-player suppression, creation-time snapshots, immediate-result memory, real public campaign-depth bands, safe configuration rejection and end-to-end LIGHT/STANDARD/REINFORCED integration through M3.4.

Runtime evidence:

`TestProfiles/M3_5_Authority_2/logs/logs_2026-09-29_17-34-24/console.log`

The proof switch is returned to `0`. No later milestone was started.

## Selector contract and approved inputs

`TB_PVE_M3_5_ForceSelectionPolicy.SelectAttackPackage(previousResult)` is a separate server-authoritative service. It returns a `TB_PVE_M3_5_SelectionDecision`, containing selected/no-selection status, package ID, reason, human count, player/depth bands, modifiers, final strength and strategic revision. It cannot create operations, groups, staging or ownership.

Inputs are limited to:

- active PlayerManager entries that still control an entity affiliated with `US`;
- normalized conventional campaign depth from the public `TB_PVE_M2_3_FrontierQuery.GetCampaignDepthPermille()` method;
- the coordinator's immediately previous terminal result.

USSR/FIA AI are absent from PlayerManager player IDs. FIA human entities are explicitly excluded by the `US` faction check. Entries without a current controlled entity, including disconnected/deployment-state players, are excluded. No long-term result history exists.

## Configuration and deterministic rule

Production component configuration:

| Setting | Value |
|---|---:|
| MEDIUM player minimum | 2 |
| HIGH player minimum | 4 |
| EARLY maximum depth | 334/1000 |
| MID maximum depth | 667/1000 |
| MID modifier | +1 |
| LATE modifier | +2 |
| Previous FAILED modifier | +1 |
| LIGHT/STANDARD/REINFORCED mappings | matching M3.4 IDs |

Base strength is `LOW=0`, `MEDIUM=1`, `HIGH=2`. Depth adds `EARLY=0`, `MID=1`, `LATE=2`. Only `FAILED` adds the configured one step; `SUCCEEDED`, `CANCELLED` and no previous result add zero. The total is clamped to 0–2 and mapped to LIGHT/STANDARD/REINFORCED. There is no randomness.

These are conservative, explainable proof defaults, not a final balance claim.

## Public campaign depth

The M2 query normalizes its already-published frontier over the authored route to 0–1000. M3.5 never reads route component arrays or private frontier fields. The ten-node production route produced:

- frontier 3: `333`, EARLY;
- frontier 4: `444`, MID;
- frontier 7: `777`, LATE.

All three were established by genuine vanilla US capture actors and evaluated at runtime through the public query.

## Zero-player and failure policy

Zero active human US players prevents WAITING and new operation creation. Existing operations are allowed to finish or cancel under their existing lifecycle; no replacement is created while the count remains zero. The dedicated proof held the created-operation count stable and the frontier unchanged during the zero-player observation window.

Invalid thresholds, unavailable depth, non-authority calls and missing/invalid M3.4 package mappings return no package. There is no STANDARD fallback or silent substitution. The invalid-configuration runtime test returned `invalid-configuration`.

## Coordinator integration and snapshot

M3.2 continues to own whether/when an operation exists and its single active reference. After its normal delay it asks the selector once, validates the selected ID through the M3.4 registry, and creates M3.1 with that ID. The operation records selection count, bands, result, strength and revision. It never resizes or changes package after creation; later operations perform a fresh selection.

Runtime proved a LIGHT operation created with one human retained LIGHT after the proof count changed to four. After that operation failed, the next selection re-read one current human plus the immediate FAILED result and created STANDARD.

## Runtime selection and end-to-end results

- Matrix: one player selected LIGHT at EARLY, STANDARD at MID and REINFORCED at LATE.
- Previous FAILED: one EARLY player changed LIGHT base to STANDARD; a maximum-strength case remained clamped to REINFORCED.
- Previous CANCELLED: neutral; the one-player EARLY matrix remained LIGHT. End-to-end, a cancelled STANDARD operation followed by four EARLY players selected REINFORCED solely from the current HIGH band.
- Previous SUCCEEDED: neutral; one EARLY player remained LIGHT.
- LIGHT: coordinator selected LIGHT, M3.4 assembled one group/three units, M3.3 staged it and physical movement occurred. Changing the live input did not change the active package. The proof then destroyed the force to establish the FAILED modifier.
- STANDARD: the following selection used immediate FAILED, assembled two groups/six units, staged both and proved both moving before bounded strategic cancellation.
- REINFORCED: the following selection used neutral CANCELLED plus four current players, assembled three groups/nine units and proved all three moving before bounded cancellation.
- Maximum concurrent operations remained one.

These end-to-end cases intentionally stopped after physical movement because package assembly, vanilla capture and cleanup were already accepted in M3.4. M3.5 tested the new selection/snapshot boundary without repeating lengthy capture proofs.

## LOD and performance

All end-to-end groups continued to report natural `LOD=0`; no activation lease was introduced. M3.4 assembly remained in its previously observed bounded tens-to-low-hundreds-of-milliseconds range. Selector work is a fixed set of integer comparisons, one M3.4 registry lookup and one diagnostic event per selection.

## Acceptance A–X

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Selector rejects non-server authority and ran on dedicated authority. |
| B | PASS | Dedicated M3.5 policy class is separate from coordinator, operation and assembly. |
| C | PASS | Final IDs must resolve as valid USSR M3.4 packages. |
| D | PASS | Active human US count is a selection input and operation snapshot field. |
| E | PASS | Only PlayerManager-controlled US-affiliated entities count; FIA/AI are excluded. |
| F | PASS | Depth comes from the new public normalized M2 semantic query. |
| G | PASS | Only M3.2's immediately previous terminal state is supplied. |
| H | PASS | Runtime matrix returned identical transparent rule outcomes without randomness. |
| I | PASS | Logs expose every input, band, modifier, result and reason. |
| J | PASS | Zero-human runtime window created no operation or frontier progress. |
| K | PASS | LIGHT snapshot remained LIGHT when current count changed 1 → 4. |
| L | PASS | Next operation re-read inputs and selected STANDARD after failure. |
| M | PASS | Actual production-route depths 333/444/777 proved EARLY/MID/LATE. |
| N | PASS | FAILED changed early LOW from LIGHT to STANDARD and clamped at REINFORCED. |
| O | PASS | CANCELLED was neutral in matrix and end-to-end selection. |
| P | PASS | SUCCEEDED was neutral in the runtime matrix. |
| Q | PASS | LIGHT selected and traversed coordinator → M3.4 → M3.3 → movement. |
| R | PASS | STANDARD selected, assembled two groups and moved both. |
| S | PASS | REINFORCED selected, assembled three groups and moved all. |
| T | PASS | Invalid configuration returned no package; no fallback attack. |
| U | PASS | Dedicated maximum concurrent operation count remained one. |
| V | PASS | No direct ownership/frontier mutation was introduced. |
| W | PASS | FIA is excluded from count and package validation remains USSR-only. |
| X | PASS | LOD 0 and bounded selection/assembly observations were retained. |

## Remaining unknowns

- Thresholds and depth modifiers require multiplayer balance testing; they are not claimed final.
- The dedicated proof used a controlled human-count seam because local SteamDiag authentication remains unavailable. Actual retail-client join/leave counting remains a client-facing acceptance observation.
- Players without a controlled world entity are deliberately excluded, including deployment UI and post-death intervals; whether persistent servers should count those connected players is a future policy choice.
- Immediate operation cleanup and full-route path viability remain the documented M3.4 concerns.

## Recommended next milestone

The next milestone should validate the smallest production operation-pressure/cadence policy using this selector, while retaining one operation, no reinforcement waves, no vehicles and no economy. Scope should be agreed before implementation.
