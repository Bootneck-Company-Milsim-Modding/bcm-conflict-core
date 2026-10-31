# M3.6 — OPFOR Frontline Garrison Foundation

## Verdict

**PASS.** A clean `ArmaReforgerServerDiag.exe` dedicated-authority run completed the bounded proof:

`TestProfiles/M3_6_Authority_6/logs/logs_2026-09-30_06-25-11/console.log`

The terminal diagnostic was `[TB_PVE_M3_6] COMPLETE PASS`. The proof switch was restored to `0` after the run. No commit was made.

## Architecture and ownership

`TB_PVE_M3_6_OPFORFrontlineGarrisonManager` exists only on server authority and owns at most one `TB_PVE_M3_6_FrontlineGarrison`. The garrison records its stable ID, `USSR` faction, objective ID, creation revision, fixed defensive package, state and owned groups. Its implemented states are `NONE`, `PENDING_ACTIVATION`, `ACTIVE`, `ENGAGED`, `DEFEATED`, `RETIRING` and `SUSPENDED`.

The manager selects `TB_PVE_M2_3_FrontierQuery.GetBLUFORLegalObjective()` and requires its public semantic state to be `ENEMY_FRONTLINE`. In the M2 US-oriented contract, that is precisely the first USSR-held objective. It does not read route arrays, raw frontier indices or lock masks. The proof resolved `OBJ_02_KERMOVAN`, not a USSR rear objective.

The manager owns defensive groups and their waypoints. M3.2/M3.1 continue to own attack-operation groups. Neither system transfers or deletes the other's groups. Vanilla `SCR_CampaignSeizingComponent` and base ownership events remain below both systems; M3.6 contains no ownership or frontier mutation.

## Force, placement and orders

The default garrison is the validated M3.4 `STANDARD` package: two `Group_USSR_Defenders` groups, three infantry each, six physical defenders total. M3.5 attack selection is not consulted.

Placement is a separate objective-local policy, not the M3.3 attack annulus. It evaluates bounded, distributed 18–55 m candidates around the objective and requires land, a valid Soldiers navmesh tile, a supporting surface and acceptable slope. Kermovan accepted two distinct positions at about 18 m; Andre's Beacon did likewise. Streaming initially made the first query unavailable, and the manager safely retried two seconds later rather than spawning at an invalid position.

Each group receives vanilla `{2FCBE5C76E285A7B}Prefabs/AI/Waypoints/AIWaypoint_DefendSmall.et`. No custom combat AI or activation lease was added.

## Visibility, players and cleanup

New physical garrisons require at least one active human BLUFOR player. A garrison already present is not deleted merely because that count later reaches zero. Creation is held pending when a human is within 250 m of the objective, or has direct visibility within 900 m. The proof held an observer 20 m from Kermovan for three manager ticks and created no force; creation proceeded only after the observer was cleared.

On frontier change, cleanup is immediate when no human is within 250 m. If a human is nearby, the garrison enters `RETIRING` and deletion is delayed until the area clears. This prevents both visible pop-in and close-range visible deletion without introducing population virtualization.

## Runtime lifecycle proof

The dedicated run established the first frontier using six physical US actors and genuine vanilla Conflict capture. With zero proof humans, no garrison was created. After visibility gating cleared, Kermovan received an ACTIVE STANDARD garrison: two groups, six units, natural LOD 0, created in 86 ms.

Six genuine US actors were then placed on the garrisoned objective. The vanilla defensive groups remained objective-local under `DefendSmall`; contemporaneous engine damage events were recorded. The harness subsequently destroyed the managed force deterministically to test the total-loss edge. The manager observed two viable groups fall to zero and entered `DEFEATED`. The same defeated record remained for 13 observed ticks with no regeneration while the US attack continued.

The US actors completed vanilla capture (`USSR -> US`), advancing the public frontier. M3.6 retired the defeated Kermovan responsibility and created a new two-group/six-unit garrison at `OBJ_03_ANDRES_BEACON`. Maximum concurrent garrisons remained one.

M3.2 then created one independent LIGHT attack operation against Kermovan while the Andre's Beacon garrison remained present. Runtime cost at coexistence was nine physical AI: six garrison defenders plus three attackers. Coordinator maximum operations and garrison-manager maximum garrisons were both one. The autonomous operation physically moved into the objective and vanilla ownership changed `US -> USSR`. M3.6 converged back to Kermovan, removed the stale Andre's Beacon garrison and created exactly one replacement. Created-garrison serial count reached three across advance and retreat, never concurrently.

Finally, the M2 diagnostic invalid state cleared the live garrison and prevented creation. The same production branch covers terminal outcomes because only `ACTIVE` plus `VALID_LINEAR_FRONTIER` permits creation. FIA is never selected or materialized by this manager.

## LOD and performance

- Initial garrison: 2 groups / 6 units, natural LOD 0.
- Garrison creation: 86 ms at Kermovan; 67 ms at Andre's Beacon; terminal summary reported the last creation at 65 ms.
- Coexistence: 9 physical AI (6 defensive + 3 offensive).
- Cleanup: 0 ms at the observed millisecond resolution.
- No activation lease was needed. Natural LOD is logged on each group; maximum LOD in an empty-player production interval remains an observation item, not an automatic failure.

## Acceptance A–X

| Criterion | Result | Evidence |
|---|---|---|
| A | PASS | Dedicated server started one authority-only manager. |
| B | PASS | Target came exclusively from M2.3 public query methods. |
| C | PASS | First USSR node/Kermovan selected; rear nodes ignored. |
| D | PASS | Runtime maximum concurrent garrisons = 1. |
| E | PASS | Validated M3.4 STANDARD package, 2 groups/6 units. |
| F | PASS | Each accepted position logged land/nav/support validation and distinct coordinates. |
| G | PASS | 20 m observer held creation pending for three ticks. |
| H | PASS | Both groups received vanilla `AIWaypoint_DefendSmall`. |
| I | PASS | Runtime coexistence logged 2 garrison groups and 1 separately owned attack group. |
| J | PASS | Physical BLUFOR contact, defensive waypoint execution and contemporaneous damage processing were observed. |
| K | PASS | Total managed-force loss was injected after contact and observed as zero viable groups/units. |
| L | PASS | DEFEATED record persisted for 13 ticks without regeneration. |
| M | PASS | Genuine vanilla BLUFOR capture changed Kermovan USSR -> US. |
| N | PASS | Advance cleaned the old responsibility. |
| O | PASS | Andre's Beacon received a new safe STANDARD garrison. |
| P | PASS | Autonomous OPFOR recapture returned the target to Kermovan. |
| Q | PASS | 9-AI coexistence; each owner retained its own groups; both maxima = 1. |
| R | PASS | Zero humans prevented new physical creation. |
| S | PASS | Runtime INVALID cleared/suppressed; terminal uses the same non-ACTIVE gate. |
| T | PASS | Advance/retreat churn produced serial replacements but no duplicate or stale live garrison. |
| U | PASS | Only USSR is accepted; FIA remained outside the manager. |
| V | PASS | Ownership changes in the log came from vanilla capture; M3.6 performs none. |
| W | PASS | Natural group LOD 0 was recorded; no lease added. |
| X | PASS | Counts, creation time, combined AI cost and cleanup time recorded above. |

## Remaining unknowns

- Natural garrison LOD behavior over a long zero-player interval was not forced; the manager now records it for future observation.
- The deterministic all-force destruction seam proves defeat lifecycle and non-regeneration. A longer unscripted player firefight remains useful playtest evidence for combat quality, but is not required for the bounded foundation contract.
- Delayed retirement with a real nearby remote human was validated by the same distance policy used for creation but was not exercised with an authenticated remote client because the documented local SteamDiag authentication limitation remains.

## Recommended next milestone

Do not extend M3.6 into rear garrisons, reinforcement, patrols or virtualization. The next milestone should be scoped separately around sustained player-facing combat validation and only add population/activation machinery if runtime evidence demonstrates a concrete need.
