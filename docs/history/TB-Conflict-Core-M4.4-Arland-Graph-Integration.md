# M4.4 — Arland Graph Campaign Integration

## Status

Latest work: M4.4J corrects the confirmed fresh-campaign pressure defect and
adds generic player-facing pressure projection. The old Mode 10 result proved
mechanics, not the required foothold-history prerequisite. Follow
[M4.4J](TB-Conflict-Core-M4.4J-Foothold-Pressure.md) for the superseding status.
Human visual acceptance and network N1–N5 remain separate pending work. The
older status/evidence below is historical and is not the latest verdict.

**Latest result: M4.4J AUTHORITY ACCEPTANCE PASS.** Corrected physical Mode 10,
BLUFOR Mode 9, presentation-state probes and production smoke beyond six
minutes pass. Production restored to proof=0, holds=300/300, M3 enabled. Formal
M4.4 remains incomplete for genuine-human U/visual acceptance and N1–N5.

**M4.4H IMPLEMENTATION COMPLETE / SCRIPT COMPILE PASS / AUTHORITY ACCEPTANCE PENDING**

M4.4A–G remain historical evidence for ordinary graph capture, interruption, ownership publication, garrisoning, reverse capture, East/West initialization, depth bands and fail-closed behavior. M4.4H replaces only the rejected terminal-HQ design. Dedicated pressure/cancellation/expiry proof has not yet completed because the current diagnostic backend stops before scenario load with `ENV:[invalid]`. Network N1–N5 remain separately pending.

## Production scenario

- Mission: `{A442000000000000}Missions/TB_CC_M4_4_ArlandGraphCampaign.conf`
- World: `{A441000000000000}Worlds/TB_CC_M4_4_ArlandGraphCampaign.ent`
- System prefab: `{A440000000000000}Prefabs/Systems/TB_CC_M4_4_ArlandGraphCampaign.et`
- Map definition: `ARLAND_PRODUCTION_01`
- Dedicated configuration: `ServerConfigs/M4_4_ArlandGraphCampaign.json` (hidden, UDP 2205; A2S 17779)
- Production authored default: EXPLICIT East, seed 20, `ARLAND::HQ_EAST`.
- There is no enabled proof mode in the production resource.

The scenario inherits the real vanilla `worlds/MP/CTI_Campaign_Arland.ent`; it does not use the M4.1 fixture.

## Initialization and ownership

The authority waits for initialized vanilla campaign bases, builds the generic M4.1 registry, and resolves M4.2A exactly once. During the guarded initialization phase it uses the public vanilla `SCR_CampaignMilitaryBaseComponent.SetFaction(SCR_CampaignFaction)` API to set the selected MOB to US and all other pilot positions, including the selected endpoint, to USSR. Ownership callbacks generated during this initialization are ignored. No general gameplay ownership setter was introduced; after initialization, ownership may change only through vanilla Conflict seizing.

The selected MOB's existing spawn reference is preserved and enabled with `SCR_SpawnPoint.SetSpawnPointEnabled_S(true)`. Every other strategic spawn is disabled without deleting or nulling it. The M1.3 visibility seam also consults the selected-MOB policy. Both vanilla HQ components and their radio/resource/service semantics remain intact.

## Capture authorization and callbacks

The existing narrow `SCR_SeizingComponent` policy seam delegates to M4.4 only while an Arland graph campaign is ready. US may seize only the single deterministic M4 legal BLUFOR objective. USSR may seize only the current legal response target. FIA is not granted strategic authorization.

The terminal-HQ exception is restricted to:

- US against the selected enemy endpoint, only while it is the legal BLUFOR objective;
- USSR against the selected MOB, only at the legal terminal retreat boundary.

Vanilla still evaluates actors, life state, LOD and capture progress, changes ownership, and emits `GetOnFactionChangedExtended`. The M4 callback maps the changed base to a stable ID, reconstructs ownership, reevaluates M4.3, and publishes only when the public semantic signature changes. It never polls ownership every frame.

For repeated OPFOR retreat, the integration retains the ordered history of legitimate BLUFOR captures. M4.3 continues to evaluate one response target; after a legal recapture, the integration supplies the previous still-US captured boundary. Only after genuine BLUFOR advancement has occurred and the stack is empty may an adjacent-USSR selected MOB become the terminal OPFOR objective. This preserves initial MOB protection.

## Projection

The existing `TB_PVE_M2_3_FrontierQuery` facade remains the consumer boundary. It now accepts graph state using arrays of stable position IDs and semantic states rather than a 31-bit mask. The authority projection contains readiness/validity, semantic revision, outcome, selected MOB/endpoint, legal objectives, candidates, positions, and normalized depth.

The current implementation consumes this projection on authority. Actual transport to proxies/JIP has not been accepted and must not be inferred from the presence of the client parser scaffold. N1–N5 remain pending.

## M3 migration

- M3.1 obtains the OPFOR target through the unchanged public frontier-query contract.
- M3.2 and M3.6 now test the query's generic `IsCampaignValid()` rather than requiring `VALID_LINEAR_FRONTIER`; neither reads graph edges or scores.
- M3.2 remains lifecycle owner.
- M3.3 receives the selected target position and retains its normal navigation/staging search.
- M3.4 keeps the accepted LIGHT/STANDARD/REINFORCED package architecture.
- M3.5 receives a normalized 0–1000 value through the existing semantic depth query.
- M3.6 manages one garrison at the single authorized BLUFOR objective.
- M3.7A participation/observer logic was not changed.

Depth is deterministic connected-ownership progress: `(US connected positions - selected MOB) / (all positions - selected MOB)`, normalized to 0–1000. Endpoint-connected is exactly 1000. The graph remains hidden from M3.5.

## Endpoint and invalid behavior

A legally vanilla-captured selected endpoint produces BLUFOR victory; a legally vanilla-captured selected MOB produces OPFOR victory. Terminal publication clears both objectives, blocks further strategic capture, is latched/idempotent, and calls the existing vanilla end-match bridge once.

Registry/start/evaluator failure, malformed ownership, or an invalid/no-candidate state with enemy territory remaining produces INVALID and fails closed: no strategic capture, operation, or objective-dependent garrison authorization.

## Validation evidence

Final dedicated startup log:

`TestProfiles/M4_4_AuthorityGate_Final/logs/logs_2026-09-30_21-05-32/console.log`

Observed:

- Workbench script validation: exit 0.
- Workbench resource processing (`-createDB`): exit 0.
- Mission resolved and real Arland loaded.
- RPL listened on isolated UDP 2205 and server entered online game state.
- Initial campaign: East MOB, West endpoint, selected MOB US/all other strategic positions USSR.
- Exactly one enabled strategic deployment spawn.
- Initial candidates: `ARLAND::MILITARY_SOUTH`, `ARLAND::MOSS_HILL`.
- Deterministic legal BLUFOR objective: `ARLAND::MILITARY_SOUTH`.
- Initial OPFOR objective: empty (MOB protected).
- Revision 1, valid graph, ACTIVE outcome, depth 0.
- Initial evaluation/projection timing logged as 0 ms at engine millisecond resolution.
- Existing M3 coordinator, staging, package selection, and single-garrison manager started through semantic interfaces.

The first `-server ... -port 2205` attempt still bound the local-mode default UDP 2001 and collided with an already-running unrelated server. An unchanged M4.1 control did the same. The final gate therefore used the dedicated JSON and isolated ports; no existing process was stopped.

Physical AI count, staging/assembly duration, ownership-event duration, server FPS, and capture transition timings await the physical acceptance sessions.

## Authority acceptance A–AE

| Criterion | Status | Evidence / boundary |
|---|---|---|
| A | PASS | One guarded M4.2A resolution; revision 1 startup log. |
| B | PASS | East selected MOB initialized US. |
| C | PASS | West selected endpoint initialized USSR. |
| D | PASS | Exactly one enabled spawn, selected East MOB. |
| E | PASS | All forward strategic spawns disabled, references preserved. |
| F | PASS | Genuine Military South USSR→US and US→USSR callbacks observed. |
| G | PASS | Same physical US force denied at Moss Hill and admitted at Military South. |
| H | PASS | Only Military South entered vanilla CAPTURING. |
| I | PASS | Moss Hill stayed USSR/NONE for 70 seconds; revision stayed 1. |
| J | PASS | Genuine vanilla Military South capture completed after the radio-policy seam. |
| K | PASS | Ownership callbacks published revisions 1→2→3. |
| L | PASS (opening/reverse) | Candidate set advanced after capture and returned after reverse capture; deeper branch proof pending. |
| M | PASS (opening/reverse) | Legal objectives advanced to Airport North / Military South, then retreated. |
| N | PASS (first response) | Captured Military South became the legal OPFOR response target. |
| O | PASS (first response) | M3 used the public target path and physically recaptured Military South. |
| P | PASS (first response) | Coordinator created one successful operation after one staging failure/requery. |
| Q | PASS (first response) | A valid later staging result reached and entered the objective. |
| R | PASS (first response) | LIGHT package assembled for the successful operation. |
| S | PASS | Readiness threshold 5/6 ordinary positions produced physical EARLY 142/285, MID 428/571, and LATE 714. |
| T | PASS (opening) | STANDARD graph garrison created at Airport North: 2 groups / 6 units / LOD 0. |
| U | NOT REGRESSED IN CODE | M3.7A untouched; human runtime proof pending. |
| V | PASS (first response) | Genuine vanilla OPFOR recapture completed. |
| W | PASS (first response) | Reverse callback published revision 3 and restored opening frontier. |
| X | FAIL | East physical progression reaches the authorized endpoint, but Arland HQ has no functioning area-seizing lifecycle. |
| Y | PASS | Clean West initialization plus Airport North and Military South physical captures passed. |
| Z | FAIL | Both authorized terminal HQ paths remain state NONE; neither victory can complete physically. |
| AA | PASS | Diagnostic INVALID cleared objectives, denied both factions, created no M3 operation/garrison, and produced no false victory. |
| AB | PASS (inspection) | `SetFaction` appears only inside guarded initialization. |
| AC | PASS (architecture/inspection) | Vanilla seizing and ownership callback remain underneath; physical proof pending for full acceptance. |
| AD | PASS (compile boundary) | Everon resources/route logic not migrated; complete addon validates. |
| AE | PASS | Real validator, resource processing, mission resolution, RPL and online dedicated startup pass. |

Overall authority verdict: **FAIL**. Ordinary graph-base capture and reverse capture work, but terminal HQ physical capture and natural MID/LATE depth acceptance have concrete production blockers.

## M4.4A physical acceptance — 2026-09-30

The dedicated-only `TB_CC_M4_4_PhysicalAuthorityProof` harness materializes ordinary vanilla faction groups and observes Conflict capture state. It contains no ownership, revision, objective, graph, or timer mutation. Production `m_iAuthorityProofMode` has been restored to `0`.

Evidence:

- Locked-candidate run: `TestProfiles/M4_4A_East_Physical_6/logs/logs_2026-09-30_21-48-51/console.log`
- Legal-capture run: `TestProfiles/M4_4A_East_Physical_7/logs/logs_2026-09-30_21-53-57/console.log`

### Passed before the blocker

- Clean East state remained revision 1, depth 0, East US, all other strategic positions USSR.
- Military South and Moss Hill were candidates; Military South alone was authorized.
- All eight spawn references remained valid. Only HQ East was enabled.
- Thirty living US actors were placed at Moss Hill for 70 seconds.
- Moss Hill remained USSR, capture state remained NONE, and revision remained 1.
- The same actor construction at authorized Military South yielded 30 living/30 vanilla-eligible actors and immediately entered vanilla `CAPTURING`.
- This proves the strategic seam distinguishes a candidate from the one authorized objective without assigning ownership.

### First failure

Military South remained continuously in vanilla `CAPTURING` for 482 seconds. Throughout the observation window:

- owner remained USSR;
- actors remained 30 alive / 30 eligible;
- M4 authorization remained true;
- capture state remained `CAPTURING`;
- revision remained 1;
- no CaptureFinish or vanilla ownership callback occurred.

The harness ended with:

`[TB_CC_M4_4A] COMPLETE FAIL reason=opening-vanilla-capture-timeout phase=2 ticks=241 revision=1 maxProofActors=30 directOwnership=0`

This is a genuine production-scenario physical acceptance blocker, not a direct-ownership test artifact. The logs prove capture admission and sustained physical eligibility, but they do not identify why vanilla Arland capture does not finish. Radio/connectivity, base configuration, and capture-duration/progress prerequisites remain hypotheses and are not classified as causes without further diagnostics.

### Blocked dependent phases

Because no legitimate USSR-to-US ownership transition occurred, the run could not validly test M4 callback/revision progression, garrison retargeting, autonomous OPFOR response, retreat, depth bands, branch/reconvergence, West progression, terminal outcomes, or post-capture deployment. These were not bypassed.

## M4.4E — Strategic Capture Connectivity Policy

Evidence: `TestProfiles/M4_4E_StrategicConnectivity_2/logs/logs_2026-10-01_06-55-24/console.log`.

The confirmed vanilla playable-faction rule requires HQ radio traffic before the sole superior attacker can become prevailing. That rule conflicts with an M4 graph campaign because Military South can be the one authoritative legal objective while vanilla Arland radio topology reports no US traffic.

The engine exposes no callback around only that predicate. The narrow seam is therefore a modded `SCR_CampaignSeizingComponent.EvaluatePrevailingFaction` whose body matches the current vanilla 1.8 implementation except for its final connectivity expression. Vanilla radio still admits every case it admitted before. A second alternative admits a playable attacker only when the server-authoritative M4 component is ready, the base is initialized and registered, the evaluator is ready and valid, the campaign outcome is ACTIVE, and the public graph capture contract authorizes that exact faction/base pair. The public contract limits this to configured conventional campaign factions; FIA and unknown factions fail closed. Outside an active M4 Arland graph campaign the added alternative is always false, preserving vanilla behavior.

Registered M4 bases which expose the ordinary vanilla seizing component receive the established PvE configuration `m_bIgnoreNonPlayableAttackers=0` and `m_bCapturingRequiresPlayer=0`. These checks remain in the vanilla evaluator; the campaign config chooses not to require a human and not to exclude AI-only conventional forces. Vanilla still owns entity evaluation, faction buckets, ties, superiority, seizing count, state, clock, interruption, finish, ownership, callbacks, and replication. HQ entities without ordinary area seizing are not altered. Radio topology itself is untouched and remains available to all other systems.

The clean East causal run established:

- Illegal Moss Hill: 30 living US actors remained denied for 70 seconds; authorization false, owner USSR, capture state NONE, revision 1.
- Legal Military South: the same force produced 30 entity/faction-accepted actors; authorization true; prevailing US; 12 effective seizing characters; state CAPTURING; start/end timestamps populated.
- Interruption: removing the force before completion returned capture state to NONE, cleared prevailing faction, retained USSR ownership, and retained revision 1. Respawning the same force restarted normal capture.
- Completion: vanilla changed ownership USSR→US. The M4 callback published revision 2, depth 142, legal BLUFOR objective Airport North, and legal OPFOR objective Military South.
- M3.6 created the new Airport North STANDARD frontline garrison with 2 groups, 6 units, LOD 0.
- M3.2 retried after one staging failure, created a LIGHT operation, physically entered Military South, and completed genuine vanilla US→USSR recapture. M4 published revision 3, depth 0, and the opening legal objective set. Maximum proof actors were 30; no proof ownership or timer setter was used.

Human and AI forces share the same post-actor-evaluation path. AI compatibility is proven by this run. Human compatibility is preserved by inspection but still requires the dedicated human runtime criterion; the seam does not branch on player identity.

M4.4E verdict: **PASS**. The existing M4.4A East opening, garrison, autonomous counterattack, and reverse-capture portion also passes. Full M4.4 authority acceptance is not claimed because branch/reconvergence, clean West, terminal outcomes, invalid state, human capture, and complete regression/performance coverage remain outstanding.

## M4.4F — remaining Arland graph authority acceptance

Evidence:

- East progression: `TestProfiles/M4_4F_EastFull_2/logs/logs_2026-10-01_08-19-18/console.log`
- Clean West: `TestProfiles/M4_4F_West/logs/logs_2026-10-01_08-22-16/console.log`
- Invalid fail-closed: `TestProfiles/M4_4F_Invalid/logs/logs_2026-10-01_08-24-23/console.log`
- OPFOR terminal path: `TestProfiles/M4_4F_OPFORTerminal/logs/logs_2026-10-01_08-25-32/console.log`

### East physical progression and terminal blocker

A clean East campaign physically captured Military South and Airport North through ordinary vanilla seizing. Revisions advanced 1→2→3, connected friendly count advanced 1→2→3, and depth advanced 0→142→285. At each state the selected MOB remained the only enabled strategic spawn; captured positions remained disabled.

After Airport North, the evaluator selected `ARLAND::HQ_WEST` as the legal BLUFOR objective while four candidates existed: Arleville, Farm, HQ West, and Moss Hill. Thirty living authorized US actors were placed at HQ West. The base remained owner USSR, capture state NONE, and revision 3. The Arland HQ entity exposes no working ordinary seizing lifecycle, so the established terminal-HQ actor exception has no evaluator/timer on which to operate. No direct ownership mutation was used.

This is also a graph/depth defect for the requested acceptance: the endpoint becomes legal at depth 285, still EARLY under the unchanged M3.5 thresholds. A legitimate campaign cannot naturally reach MID or LATE because selecting/capturing the endpoint is terminal and the physical endpoint itself cannot currently be seized.

### Clean West

The separate explicit West campaign initialized HQ West US and HQ East USSR. Initial candidates were Airport North and Farm; Airport North alone was authorized. Only HQ West was enabled for deployment. Genuine vanilla captures completed at Airport North and then Military South, with revisions 1→2→3, connected counts 1→2→3, depth 0→142→285, and updated candidate/objective sets. All non-MOB spawns, including both captured positions, remained disabled. This proves the graph and physical capture path operate from the opposite direction.

### OPFOR retreat and terminal blocker

A clean bounded East run physically captured Military South and Airport North, then physically recaptured Airport North and Military South with USSR. Revisions advanced 1→2→3→4→5 and depth moved 0→142→285→142→0. Only after those legitimate retreats did HQ East become the authorized OPFOR terminal target, proving premature MOB protection and legal boundary release. Thirty authorized USSR actors at HQ East again produced capture state NONE and no ownership transition. Thus OPFOR victory is blocked by the same missing Arland HQ physical-seizing lifecycle, not by graph authorization.

### Invalid and no-candidate behavior

The dedicated diagnostic mode injected INVALID without changing ownership. Projection validity became INVALID, outcome remained INVALID rather than victory, both legal objectives cleared, both faction authorization queries denied, M3.2 stopped with zero operations created, and M3.6 stopped with zero garrisons created. PASS.

The current evaluator represents enemy-remains/no-frontline as `NO_VALID_FRONTLINE_OBJECTIVE`, which the campaign converts to INVALID. It is therefore the same fail-closed semantic path rather than a second production state. A separate fabricated ownership topology was not introduced.

### Verdict

M4.4 authority acceptance: **FAIL**.

Exact blockers:

1. Vanilla Arland HQ entities do not provide a functioning physical seizing lifecycle, so neither legitimate terminal HQ can reach prevailing faction, timer, CaptureFinish, ownership callback, or victory.
2. The production evaluator selects the enemy endpoint at connected count 3 / depth 285. Consequently the current route cannot naturally demonstrate MID or LATE M3.5 depth bands before terminal selection.

Passing additions: clean West initialization and two physical transitions; East and West deployment invariants; physical OPFOR retreat to the legal terminal boundary; invalid/no-objective fail-closed behavior; representative EARLY depth and graph-neutral semantic consumers. Network N1–N5 remain pending and are not implicated.

## M4.4G — Endpoint Readiness and Terminal HQ Capture

Evidence: `TestProfiles/M4_4G_East/logs/logs_2026-10-01_08-34-26/console.log`.

### Endpoint readiness model

Endpoint readiness is now authored on each generic M4.2A MOB/endpoint relationship as a minimum number of MOB-connected ordinary positions. M4.3 computes the connected ordinary count after building the friendly component. Enemy ownership and graph adjacency remain necessary, but a selected endpoint is removed before scoring while the configured count is unsatisfied. Consequently endpoint bonus and seed variation cannot promote a locked endpoint. The policy contains no Arland stable IDs.

Arland configures both East→West and West→East relationships with a threshold of **five connected ordinary positions out of six**. This is the smallest threshold compatible with the unchanged depth bands: connected friendly counts progress through depth 0, 142, 285, 428, 571, and 714 before terminal capture. Thus ordinary play reaches EARLY, MID, and LATE without requiring every ordinary position. Branch choice remains governed by normal eligibility and scoring.

The existing depth formula is retained: `(connected friendly count - selected MOB) / (all positions - selected MOB)`, normalized to 0–1000. It correctly measures connected territorial control rather than path distance.

### Runtime result

The clean East physical sequence was:

1. Military South — revision 2, depth 142, EARLY.
2. Airport North — revision 3, depth 285, EARLY. HQ West was adjacent but logged locked at connected ordinary 2/5; Arleville was selected instead.
3. Arleville — revision 4, depth 428, MID. HQ West remained locked at 3/5.
4. Moss Hill — revision 5, depth 571, MID. HQ West remained locked at 4/5.
5. Farm — revision 6, depth 714, LATE. Readiness became true; HQ West entered the eligible set and won ordinary scoring while Beauregard remained another candidate.

Every transition above was genuine vanilla capture. No ownership setter was used. This resolves endpoint readiness and criterion S's EARLY/MID/LATE defect.

### Terminal HQ resource blocker

The existing script-level HQ actor exception was narrowed to require M4 endpoint readiness for BLUFOR. A helper can initialize vanilla `SCR_CampaignSeizingComponent` for a registered authorized HQ without changing its timer or ownership implementation. Runtime established, however, that the inherited vanilla Arland HQ entity contains no `SCR_CampaignSeizingComponent` instance. Therefore there is no component to initialize and no vanilla physical lifecycle to receive the authorized actors.

At the legitimate unlock state, 30 US actors were placed at authorized HQ West. For at least 60 seconds it remained owner USSR, capture state NONE, and revision 6. No terminal-HQ-surface activation, prevailing faction, seizing count, timer, CaptureFinish, callback, or BLUFOR victory occurred.

Adding the component requires an authored world/prefab resource override for only the two Arland HQ instances. That resource edit cannot be safely inferred or fabricated from script because components cannot be attached dynamically and the inherited packed Arland subscene is not represented as editable source in this addon. No TB timer, direct ownership assignment, or HQ-status flattening was introduced as a workaround.

M4.4G verdict: **FAIL / PARTIAL**. Endpoint readiness and natural EARLY/MID/LATE progression pass. Terminal HQ capture, interruption, both outcomes, and final criteria X/Z remain blocked by the missing authored vanilla seizing component on the two Arland HQ entities.

## Network acceptance N1–N5

| Criterion | Status |
|---|---|
| N1 Initial client projection | PENDING |
| N2 Live objective transition | PENDING |
| N3 Progressed JIP | PENDING |
| N4 Reverse-state JIP | PENDING |
| N5 Terminal-state JIP | PENDING |

No admitted proxy evidence was generated, so no network PASS is claimed.

## Remaining limitations and next action

- Run a dedicated East acceptance scenario that spawns physical US actors at both the legal objective and alternate candidate, then continues through a branch/reconvergence capture and autonomous OPFOR recapture.
- Run a clean dedicated West session and verify Airport North/Farm selection and deployment inversion.
- Run legal BLUFOR/OPFOR terminal sessions plus an invalid/no-candidate session.
- Record M3 staging/assembly, AI counts, capture/event cost and FPS.
- After authority acceptance, perform N1–N5 only with genuinely admitted proxies; retain the known local-authentication evidence boundary.

Do not begin M4 population work until these M4.4 authority proofs are complete.

## M4.4B — vanilla Arland capture-completion investigation

Investigation date: 2026-10-01. Production gameplay was not changed. The disposable authority-proof component gained capture telemetry and one diagnostic mode; its authored production mode is restored to `0`.

Evidence:

- Stalled legal-capture baseline: `TestProfiles/M4_4B_Telemetry_3/logs/logs_2026-10-01_05-52-03/console.log`
- Single-variable causal run: `TestProfiles/M4_4B_Causal_PlayerRequirement_2/logs/logs_2026-10-01_05-55-45/console.log`
- Previously successful dedicated capture: `TestProfiles/M1_2_Server/logs/logs_2026-09-21_09-13-04/console.log`
- Installed public API reference: `ArmaReforgerScriptAPIPublic` for `SCR_SeizingComponent`, `SCR_CampaignSeizingComponent`, and `SCR_CampaignMilitaryBaseComponent`.

### Vanilla path and observed first divergence

The public API exposes the seizing radius, capture start/interrupt/finish invokers, start/end timestamps, current/minimum/maximum seizing time, seizing-character count, maximum counted characters, player/non-playable-attacker gates, and campaign radio/service time modifiers. It does not publish the implementation bodies. The path below therefore combines the exposed API contract with runtime observations rather than claiming unavailable vanilla source:

1. Actors in the seizing radius are evaluated for faction and eligibility.
2. Vanilla determines contesting factions and a prevailing faction.
3. A valid prevailing faction and the configured player/non-playable-attacker gates admit characters to seizing progress.
4. The seizing timer receives start/end timestamps and current progress.
5. Timer completion raises capture finish; the campaign base then performs the vanilla ownership transition.
6. The existing M4 ownership callback derives graph state, revision, and legal objectives from that vanilla change.

Military South diverges between steps 2 and 3. With 30 living US actors inside the radius and M4 authorization true, vanilla reports `CAPTURING`, but every telemetry sample reports:

- prevailing faction: none;
- contesting factions: 1;
- seizing characters: 0 of a maximum 12;
- start timestamp: 0;
- end timestamp: 0;
- current seizing time: 0;
- minimum/maximum time: 15/90 seconds.

The clock never starts. This is not a long-duration, paused-progress, callback, or post-capture ownership failure.

### Configuration and known-good comparison

| Field / observation | Arland Military South stall | Known-good M1.2 dedicated capture |
|---|---:|---:|
| Living/eligible physical actors | 30 / 30 | physical AI capture proved |
| Capture state | CAPTURING | CaptureStart then CaptureFinish |
| `m_bCapturingRequiresPlayer` | 1 | 0 |
| `m_bIgnoreNonPlayableAttackers` | 1 | 0 |
| Prevailing faction | none | timer admitted and completed |
| Seizing characters | 0 | non-zero progress implied by running clock |
| Start/end timestamp | 0 / 0 | populated (`start=3279`, advancing end timestamp) |
| HQ radio traffic | US coverage/traffic false | OPFOR HQ radio traffic false |
| Completion | none after sustained observation | ownership completed in about 50 seconds |

The known-good custom Conflict base explicitly authors both PvE gates false. The vanilla Arland base retains both true.

### Other capture prerequisites

- The base was initialized, owned by USSR, had a valid spawn reference, roughly 395–570 supplies across clean runs, two building providers, three services, radius 50, and radio range 1000.
- US radio coverage/send/receive were false. The configured modifiers were 60 seconds per radio connection and 45 seconds per service, but these affect duration only after timer admission. More importantly, the known-good dedicated AI capture completed while OPFOR HQ radio traffic was also false. Radio absence is therefore **UNSUPPORTED** as the primary blocker.
- Supply/storage/service state is **UNSUPPORTED** as the blocker: resources and services existed, while the failure occurred before timer creation.
- A stale-HQ or required refresh problem is **UNKNOWN**; no log evidence shows such a fault, and the selected East HQ and initial ownership were correct.

### One controlled causal experiment

The single authorized experiment changed only Military South's runtime `m_bCapturingRequiresPlayer` value from `1` to `0` in a disposable proof session. It did not change ownership, graph state, actors, mission content, radio, supplies, or capture timing.

Result: genuine capture did **not** complete. The base still reported CAPTURING with prevailing faction none, zero seizing characters, zero start/end timestamps, and zero current seizing time. `m_bIgnoreNonPlayableAttackers` remained `1`.

This falsifies `m_bCapturingRequiresPlayer` as the sole cause and leaves the non-playable-attacker gate as the material configuration difference blocking the all-AI proof force. No second mutation was performed.

### Root-cause classification and correction recommendation

- Immediate failure mechanism — no prevailing faction/seizing characters are admitted, so the vanilla timer never starts: **CONFIRMED**.
- Arland-versus-known-good gate mismatch (`1/1` versus `0/0`): **CONFIRMED**.
- `m_bCapturingRequiresPlayer` alone as root cause: **UNSUPPORTED** by the controlled experiment.
- `m_bIgnoreNonPlayableAttackers=1` as the remaining production AI-capture blocker: **PROBABLE**. A same-scenario completion with both PvE gates false was deliberately not run because only one controlled experiment was authorized.
- Radio/HQ connectivity as the primary cause: **UNSUPPORTED**.
- Supply/service state as the primary cause: **UNSUPPORTED**.
- Stale initialization requiring a refresh: **UNKNOWN**.

Recommended production correction, requiring approval before implementation: author an Arland-scenario base override for every graph strategic base so its `SCR_CampaignSeizingComponent` uses `m_bCapturingRequiresPlayer=0` and `m_bIgnoreNonPlayableAttackers=0`, matching the already proven PvE Conflict base contract. Keep vanilla seizing timers, capture callbacks, and ownership mutation underneath. Prefer the scenario/base override over a global class override so ordinary vanilla resources remain untouched and the policy is explicit for this PvE campaign.

M4.4B investigation verdict: **PASS — failure mechanism isolated and a narrow configuration correction identified; causal root is PROBABLE, not falsely promoted to CONFIRMED.**

M4.4 overall verdict remains **FAIL / INCOMPLETE**. Criterion J remains failed and all ownership-dependent authority phases remain blocked until an approved correction produces a genuine vanilla ownership transition. No M4 population work is authorized.

## M4.4C — PvE Strategic Base Capture Contract

Phase 1 was run on 2026-10-01 from a clean East-start dedicated session. Evidence is preserved at:

`TestProfiles/M4_4C_Causal_BothGates/logs/logs_2026-10-01_06-15-31/console.log`

The experiment used the same Military South proof force and changed only the two runtime seizing admission settings:

- `m_bCapturingRequiresPlayer: 1 -> 0`
- `m_bIgnoreNonPlayableAttackers: 1 -> 0`

No ownership, capture timer, graph state, radio, supply, force construction, position metadata, or mission prerequisite was changed.

### Final causal result

The required after-state did **not** occur. For more than one minute after the force was admitted to the radius, repeated samples reported:

- 30 living actors;
- 30 actors accepted by the exposed vanilla entity/faction eligibility evaluation;
- M4 strategic authorization true;
- vanilla capture state `CAPTURING`;
- one contesting faction;
- prevailing faction absent;
- `seizingCharacters=0`;
- capture start timestamp `0`;
- capture end timestamp `0`;
- current seizing time `0`;
- owner still USSR;
- revision still 1.

The telemetry simultaneously confirmed `capturingRequiresPlayer=0` and `ignoreNonPlayable=0`. No `CaptureFinish`, USSR-to-US ownership callback, or graph progression occurred.

### Revised classification

- Immediate mechanism — vanilla does not promote the detected US contesting faction to prevailing, admits zero seizing characters, and never starts its clock: **CONFIRMED**.
- `m_bCapturingRequiresPlayer=1` as the sole blocker: **DISPROVED**.
- `m_bIgnoreNonPlayableAttackers=1` as the remaining blocker: **DISPROVED by the required 0/0 causal experiment**.
- The deeper difference between the Arland vanilla-base-backed entity and the previously successful Conflict Core inherited base: **UNKNOWN**. The result establishes that another base/scenario initialization, faction-presence, character-registration, or vanilla capture prerequisite differs, but this run does not identify which one.

### Stop boundary

The task required an immediate stop if `0/0` did not produce a prevailing US faction, a running seizing clock, `CaptureFinish`, and USSR-to-US ownership. Consequently:

- no production PvE strategic-base prefab contract was applied;
- none of the eight Arland resources was changed;
- no vanilla resource was changed;
- human-capture regression was not rerun;
- M4.4A was not resumed;
- no garrison, counterattack, reverse capture, depth, branch, West-start, terminal, invalid-state, deployment, or performance claim was generated.

The disposable M4.4C setter and proof mode were removed after the run, and production `m_iAuthorityProofMode` was restored to `0`.

M4.4C verdict: **FAIL at Phase 1 causal confirmation**.

M4.4 remains **FAIL / INCOMPLETE**. The next permitted activity must investigate the still-unknown vanilla admission prerequisite; productionising the two-flag override is not supported by this experiment.

## M4.4D — Vanilla Capture Context Differential

Investigation date: 2026-10-01. Runtime build: Reforger 1.8.0.13, engine 192142. Production behavior was not changed and proof mode was restored to `0` afterward.

Evidence:

- Arland admission telemetry: `TestProfiles/M4_4D_AdmissionTelemetry/logs/logs_2026-10-01_06-26-16/console.log`
- Fresh current-build inherited-base control: `TestProfiles/M4_4D_CurrentBuild_GoodControl/logs/logs_2026-10-01_06-28-50/console.log`
- Public-setter experiment: `TestProfiles/M4_4D_Causal_RadioBranch/logs/logs_2026-10-01_06-33-09/console.log`
- HQ-range experiment: `TestProfiles/M4_4D_Causal_HQRadioRange/logs/logs_2026-10-01_06-36-01/console.log`
- Confirming branch experiment: `TestProfiles/M4_4D_Causal_PlayableRadioBranch/logs/logs_2026-10-01_06-38-28/console.log`

### Current vanilla decision tree

The installed game ships the scripts packed rather than as loose `.c` files. The exact exposed classes/members were checked against the installed 1.8 API database, and the implementation bodies were obtained from Bohemia Interactive's official `Arma-Reforger-Script-Diff` repository. The runtime observations below match that implementation, including enum values and the playable-faction radio branch.

`SCR_CampaignSeizingComponent.EvaluatePrevailingFaction()` performs this sequence:

1. Reject the whole evaluation when the tag system is absent.
2. Query `ETagCategory.Character` within the authored seizing radius.
3. If empty, set capture state to `NONE`, clear prevailing faction and seizing count, and return.
4. For every discovered entity:
   - optionally delete/skip disabled AI;
   - call campaign `EvaluateEntityFaction()`;
   - reject when the base reference is absent, is HQ, or is not initialized;
   - call base `SCR_SeizingComponent.EvaluateEntityFaction()`;
   - require `SCR_ChimeraCharacter`;
   - reject an in-vehicle character above maximum altitude;
   - require an alive controller when a controller exists;
   - for non-player AI with AI control and an agent, reject maximum-LOD agents;
   - reject players still in the post-respawn cooldown;
   - obtain `SCR_Faction` from `character.GetFaction()`;
   - if non-null, add the character to that faction's presence bucket;
   - separately record player presence when `m_bCapturingRequiresPlayer` is enabled.
5. Calculate total presence, maximum faction presence, tie state, and contesting-faction count.
6. Insert the defender with zero presence when absent.
7. For each non-defending faction:
   - require `CanCaptureBases()`;
   - optionally reject a non-playable attacker;
   - optionally require an actual player;
   - require sole maximum presence;
   - cast to `SCR_CampaignFaction`;
   - require **either** the faction to be non-playable **or** `base.IsHQRadioTrafficPossible(faction)` to be true;
   - only then assign prevailing faction, seizing count, and capture state `CAPTURING`.
8. Clamp effective seizing characters with `(2 * attackers) - total presence` and the maximum-character setting.
9. Publish capture-state changes. Enum values are `NONE=0`, `CONTESTED=1`, `CAPTURING=2`.
10. When prevailing faction changes to a non-owner, set the start timestamp, refresh the end timestamp, raise `CaptureStart`, and run the frame timer. Timer expiry raises `CaptureFinish`; the campaign base performs vanilla ownership mutation.

The exact condition explaining the observed state is:

`cFaction && (!cFaction.IsPlayable() || m_Base.IsHQRadioTrafficPossible(cFaction))`

US is playable and Military South reports no US HQ radio traffic, so it is counted as contesting but is not promoted to prevailing. Earlier reports incorrectly called numeric state `1` CAPTURING; it is vanilla `CONTESTED`.

### Audit of the old `vanillaEligible` label

The diagnostic called only `EvaluateEntityFaction(entity)`. That proves character type, altitude, life state, AI LOD, respawn cooldown, base existence/HQ/initialization, and faction lookup. It does **not** prove final seizing admission. It omits:

- `CanCaptureBases()`;
- non-playable-attacker policy;
- required-player presence;
- sole-maximum/tie evaluation;
- playable campaign faction HQ-radio traffic;
- effective seizing-character calculation.

The M4 diagnostic is therefore renamed `entityFactionAccepted`; historical `vanillaEligible` records must be interpreted only as the result of `EvaluateEntityFaction`, not final eligibility.

### Fresh current-build control

The current inherited PvE base was run on the same 1.8.0.13 server binary. It used direct group plus explicitly materialized members, alive AI controllers/agents, group membership, activation, and LOD 0.

- Three USSR actors attacked a US-owned base.
- USSR was non-playable, `CanCaptureBases()` true, and HQ radio traffic false.
- Vanilla transitioned to state `2` (`CAPTURING`), produced prevailing USSR, started its clock, raised `CaptureStart`, raised `CaptureFinish` about 50 seconds later, and changed ownership US to USSR.
- The same harness then placed three US actors against the new USSR owner. US was playable and had no HQ radio traffic. Vanilla transitioned only to state `1` (`CONTESTED`), produced no prevailing faction, and did not start its clock.

This same-base, same-materialization reverse case isolates faction playability/radio context without relying on an Arland-versus-Everon prefab difference.

### Differential tables

| Force context | Good current control | Failing Military South |
|---|---|---|
| Construction | Campaign defender group + explicit members | Campaign defender group + explicit members |
| Controller / AI / agent / member | present | present |
| Life / activation / LOD | alive, active, LOD 0 | alive, active, LOD 0 |
| Faction returned per actor | USSR | US |
| Count | 3 | 30 |
| Radius relationship | 0–4 m from centre | centred inside radius |
| Replication/authority | dedicated authority | dedicated authority |

| Faction context | Good USSR attacker | Failing US attacker |
|---|---|---|
| Key | USSR | US |
| Campaign faction | yes | yes |
| Playable | false | true |
| Player limit | 0 | -1 |
| `CanCaptureBases()` | true | true |
| HQ radio traffic at target | false | false |
| Vanilla radio condition | bypassed because non-playable | fails because playable |

| Base/campaign context | Inherited control base | Military South |
|---|---|---|
| Component | `SCR_CampaignMilitaryBaseComponent` | `SCR_CampaignMilitaryBaseComponent` |
| Seizing component | `SCR_CampaignSeizingComponent` | `SCR_CampaignSeizingComponent` |
| Initialized / HQ | true / false | true / false |
| Owner during attack | US | USSR |
| Radius | 50 | 50 |
| Minimum contest characters | current campaign value | 3 |
| Services/providers | present | 3 / 2 |
| Attacker radio | false | false |
| Game mode | `SCR_GameModeCampaign` dedicated authority | `SCR_GameModeCampaign` dedicated authority |
| Initial roles | ordinary scenario state | deterministic M4 East assignment before proof |

No evidence shows stale base registration or the M4 ownership initialization causing the failure. The same current-build base admits non-playable USSR without radio and rejects playable US without radio exactly as the vanilla branch specifies.

### Actual input buckets and first divergence

Military South has one faction bucket: US=30. USSR is inserted as defender=0. Total presence is 30, maximum presence is 30, there is no tie, and US passes `CanCaptureBases`, both optional seizing flags in the M4.4C baseline, and sole-maximum presence. The first rejection is the playable campaign-faction radio condition: `US.IsPlayable()==true` and `MilitarySouth.IsHQRadioTrafficPossible(US)==false`.

### Controlled experiments

1. Public `SetIsPlayable(false, false)` experiment: the call left US playable (`after=1`), so the intended variable did not change and the target remained contested. This is non-causal evidence, not a failed radio hypothesis.
2. Selected-HQ range experiment: the stored East HQ range changed from 800 to 5000, but the radio system still reported target traffic false. Since actual traffic did not change, the target remained contested. This proves that mutating the cached base range alone is not a valid way to establish a radio connection.
3. Exact branch-selector experiment: from the established `requiresPlayer=0`, `ignoreNonPlayable=0`, no-radio baseline, only the US playable field changed true to false. Military South immediately produced prevailing US, 12 effective seizing characters, state `2`, a running vanilla clock, vanilla USSR-to-US ownership, the M4 callback, and revision 1 to 2. No ownership or timer was assigned by the proof.

This third experiment confirms the vanilla playable-faction HQ-radio branch as the cause. Making US non-playable is diagnostic only and is not an acceptable production correction.

### Human control and correction recommendation

No new human Military South session was performed. Existing M3.7 human capture occurred in a different production route and is not presented as proof for Arland. The exact vanilla path applies the same radio requirement to playable US whether the physical actors are AI or human, so an Arland human without HQ traffic is expected to remain contested; that remains a runtime verification item.

Root cause: **CONFIRMED — playable US lacks vanilla HQ radio traffic at Military South, and vanilla therefore refuses to promote its presence bucket to prevailing.**

Recommended narrow production direction, not applied here: ensure every graph-authorized objective for a playable attacking faction is reachable through a valid vanilla HQ radio-traffic chain, using Conflict Core/Arland-owned radio topology or a narrowly designed PvE seizing policy that explicitly preserves human playability. Do not make US non-playable, do not bypass vanilla ownership, and do not treat a stored range change as equivalent to radio-system connectivity. The appropriate architecture must be selected and then causally verified before M4.4A resumes.

M4.4D verdict: **PASS — root cause confirmed.** M4.4 remains **FAIL / INCOMPLETE**; M4.4A was not resumed and population work remains blocked.

## M4.4H — Strategic Control Victory Pressure

### Product supersession

**TERMINAL HQ CAPTURE CANCELLED BY PRODUCT DESIGN.** The terminal model is now **SUSTAINED CONTROL OF REQUIRED ORDINARY STRATEGIC TERRITORY**. The M4.4F/G terminal-HQ failures above remain useful historical evidence explaining the design change, but they are no longer current acceptance requirements. No seizing component was added to either Arland HQ.

For `ARLAND_PRODUCTION_01`, required territory is derived generically as every registered strategic position except the M4.2A-selected MOB and selected opposing HQ endpoint. This produces six ordinary positions in either East or West direction. Both HQ IDs remain in the map definition for start anchoring, graph connectivity, radio/logistics references and future siege geometry.

### Outcome and timer contract

The public outcome set is now:

1. `NOT_READY`
2. `ACTIVE`
3. `BLUFOR_VICTORY_PRESSURE`
4. `OPFOR_VICTORY_PRESSURE`
5. `BLUFOR_VICTORY`
6. `OPFOR_VICTORY`
7. `INVALID`

The production system prefab authors separate BLUFOR and OPFOR hold durations at 300 seconds. These are data attributes, not strategic-logic constants. Dedicated proof modes 9 and 10 substitute 60 seconds in memory for bounded acceptance only; production mode remains 0 and the authored values remain 300 seconds.

Pressure starts once on state entry using an authority `WorldTimestamp` start and deadline plus a one-shot server callqueue callback. Re-evaluation of identical ownership does not restart it. A genuine vanilla ownership callback which breaks 6/6 cancels the callback immediately and returns the campaign to ACTIVE. A later restoration creates a fresh timestamp/deadline and increments the pressure-start counter. INVALID and component deletion also cancel the callback. Expiry recounts authoritative vanilla ownership before publishing a terminal result.

The public projection exposes the pressure outcome and semantic remaining seconds at publication. `IsVictoryPressureActive`, the outcome transition invoker, and `GetVictoryPressureRemainingSeconds` form the future pressure-consumer seam. In particular, entering/cancelling/expiring `OPFOR_VICTORY_PRESSURE` is the reserved **MOB SIEGE** hook. No siege actors, waves, checkpoints or patrols are implemented here.

### Objective, HQ and M3 policy

M4.3 excludes both selected HQ IDs before candidate scoring. Endpoint bonus can therefore never admit an HQ. M4 `CanFactionSeizeTerminalHQ` always returns false; Everon's separate M2.5 path is unchanged.

During BLUFOR pressure, only the current ordinary OPFOR response target remains physically authorized, allowing USSR to break 6/6. During OPFOR pressure, only the ordinary BLUFOR graph objective remains physically authorized, allowing US breakout. Terminal and INVALID outcomes deny both factions and clear objectives. M3.2 and M3.6 are stopped on pressure entry and recreated only if normal ACTIVE play resumes, preventing either system from treating an HQ as a target or approximating the future siege design.

MOB-only deployment is reapplied after every vanilla ownership callback and is independent of pressure. The selected MOB remains US and enabled; every other strategic spawn remains disabled.

### Depth

Depth is now `floor(US-owned required ordinary positions × 1000 / required ordinary position count)`. With six Arland ordinary positions this is exactly:

| US control | Permille | M3.5 band |
|---:|---:|---|
| 0/6 | 0 | EARLY |
| 1/6 | 166 | EARLY |
| 2/6 | 333 | EARLY |
| 3/6 | 500 | MID |
| 4/6 | 666 | MID |
| 5/6 | 833 | LATE |
| 6/6 | 1000 | LATE / BLUFOR pressure |

Integer division floors the fractional result. The accepted M3.5 thresholds remain unchanged.

### Validation and runtime boundary — 2026-10-01

The first ServerDiag local-addon compile exposed and was used to correct four root M4.4H errors: two over-wide `PrintFormat` calls, use of `World.GetServerTimestamp` instead of the current `ChimeraWorld` API, and an Enforce return-form issue in the proof formatter. The final clean compile loaded BCM Conflict Core and compiled **5,697 Game script files / 11,116 classes** with no script errors (CRC32 `03690a95`). Evidence: `TestProfiles/M4_4H_Compile_Final/logs/logs_2026-10-01_09-09-15/console.log`.

The installed Workbench SteamDiag process currently fails before initialization with Windows status `0xc0000142`, so `-createDB` could not be rerun. Direct ServerDiag loading proved source compilation, but both the JSON and direct local-addon launch then stopped before world initialization because the diagnostic backend returned `ENV:[invalid]` / `Could not connect to server`. Consequently no M4.4H ownership, timer, end-match or deployment runtime claim is made. Proof mode was restored to 0, M3 coordinator/garrison to 1, and both production durations to 300 seconds.

The dedicated-only `TB_CC_M4_4H_VictoryPressureProof` is prepared as follows:

- mode 9: physical US progression to 6/6, physical USSR cancellation, physical US restoration, BLUFOR expiry;
- mode 10: initial 0/6 OPFOR pressure, physical US breakout, physical USSR restoration, OPFOR expiry.

It uses spawned vanilla faction groups and observes ownership callbacks; it contains no ownership setter, revision mutation or timer mutation.

### Updated authority acceptance A–AE

Earlier evidence remains valid except where terminal-HQ criteria were explicitly superseded. `PENDING H RUNTIME` means implementation/compile exists but M4.4H physical authority proof has not run.

| Criterion | Current status | Updated evidence / boundary |
|---|---|---|
| A | PASS | Guarded seeded initialization previously proven. |
| B | PASS | East selected MOB initializes US. |
| C | PASS | Opposing HQ initializes USSR. |
| D | PASS | Exactly one selected-MOB spawn enabled. |
| E | PASS | All ordinary/endpoint forward spawns disabled. |
| F | PASS | Genuine ordinary ownership callbacks proven. |
| G | PASS | Legal-vs-locked graph capture authorization proven. |
| H | PASS | Only authorized ordinary position enters vanilla capture. |
| I | PASS | Non-selected ordinary position remains locked. |
| J | PASS | Genuine ordinary vanilla capture completes. |
| K | PASS | Ownership callback advances semantic revision. |
| L | PASS | Branch/reconvergence ordinary progression proven in M4.4F/G evidence. |
| M | PASS | Ordinary legal objective advances/retreats. |
| N | PASS | Ordinary OPFOR response is derived from legal BLUFOR advance. |
| O | PASS | M3 physical ordinary recapture proven. |
| P | PASS | Single-operation coordination previously proven. |
| Q | PASS | Valid physical staging previously proven. |
| R | PASS | Data-driven force package assembly previously proven. |
| S | PASS (UPDATED) | Six-position formula gives 0/166/333/500/666/833/1000; physical old depth-band progression evidence exists, exact H values await runtime. |
| T | PASS | Ordinary frontline garrison previously proven. |
| U | PENDING HUMAN | Human Arland capture remains separate human evidence debt. |
| V | PASS | Genuine ordinary OPFOR recapture previously proven. |
| W | PASS | Reverse callback and retreat publication previously proven. |
| X | PENDING H RUNTIME (SUPERSEDED) | Old HQ capture criterion replaced by 6/6 US → BLUFOR pressure → hold expiry. |
| Y | PASS | Clean West initialization and physical ordinary progression previously proven. |
| Z | PENDING H RUNTIME (SUPERSEDED) | Old two-HQ terminal capture criterion replaced by both pressure/cancel/restart/expiry paths. |
| AA | PASS (prior) / H RECHECK PENDING | Prior INVALID fail-closed passed; timer cancellation during pressure awaits H runtime. |
| AB | PASS | Gameplay ownership setter remains absent; guarded initialization only. |
| AC | PASS (architecture) / H RUNTIME PENDING | Vanilla ordinary seizing/ownership remain underneath. |
| AD | PASS (compile) | Everon M2.5 policy retained; M4 dispatch is isolated. |
| AE | PENDING H RUNTIME | M4.4H source compiles; resource processing and clean production startup await a functioning Workbench/backend session. |

Additional H terminal requirements are all **PENDING RUNTIME**: US 6/6 entry; USSR cancellation; US restart; BLUFOR expiry; USSR 6/6 entry; US breakout cancellation; USSR restart; OPFOR expiry; selected-MOB deployment throughout; stable HQ ownership; one-shot end-match; idempotent terminal reconstruction; and observed siege-hook transition ordering.

### Network N1–N5

| Network criterion | Status |
|---|---|
| N1 Initial admitted-proxy projection | PENDING |
| N2 Live pressure entry/cancellation | PENDING |
| N3 Live expiry/terminal projection | PENDING |
| N4 Mid-pressure JIP with semantic timer state | PENDING |
| N5 Terminal-state JIP | PENDING |

M4.4H authority verdict: **IMPLEMENTATION COMPLETE / COMPILE PASS / AUTHORITY ACCEPTANCE PENDING**. The next action is to restore normal Workbench/backend availability, run resource processing, then execute exactly one clean mode-9 and one clean mode-10 dedicated proof before claiming M4.4 PASS. Do not begin checkpoints, patrols or physical MOB siege forces first.

## M4.4H acceptance attempt — 2026-10-01

Normal interactive Arma Reforger Tools startup and BCM Conflict Core project loading were manually verified successfully. The earlier Windows process status `0xc0000142` is therefore classified as specific to the automated Workbench invocation environment and is not reproduced by ordinary interactive Workbench use. No Workbench repair was attempted.

The safest supported command-line resource-processing checks (`-validate` and `-createDB`) were attempted once more. Both automated Workbench processes again terminated during process initialization with `0xc0000142`; neither produced a project/resource diagnostic. This does not overturn the manual project-load result or the existing clean direct ServerDiag script compile. The last completed compile remains the 5,697-file / 11,116-class, zero-script-error compile recorded above.

A fresh production-mode direct local-addon startup check was then performed before enabling either authority proof. Evidence is preserved at:

`TestProfiles/M4_4H_BackendCheck_2026-10-01_09-16-51/logs/logs_2026-10-01_09-16-51/console.log`

The check successfully:

- discovered and mounted BCM Conflict Core GUID `6A6A5973FA53A5B1`;
- loaded the project and its resource database;
- resolved `{A441000000000000}Worlds/TB_CC_M4_4_ArlandGraphCampaign.ent` sufficiently to construct the authority world;
- compiled Game scripts without a script error.

It then reproduced the prerequisite backend failure. Beginning at `09:16:53.915`, four consecutive `GameConfig/List` attempts failed with:

- `Curl error=Could not connect to server`;
- `Http Error (BAD_REQUEST)`;
- `ENV:[invalid] Error - Cannot send request`.

The diagnostic server was stopped after confirming the repeated failure. Because the requested acceptance procedure permits MODE 9 and MODE 10 only when the backend is available, neither proof mode was enabled or executed. There was no ownership mutation, proof actor creation, timer start, outcome transition, end-match invocation, or production gameplay change in this attempt.

### Authority matrix resolution for this attempt

Criteria A–W and AB–AD retain the previously documented evidence and statuses above. The current M4.4H-dependent criteria resolve as follows:

| Criterion | Final status for this attempt | Reason |
|---|---|---|
| X | FAIL / BLOCKED | Superseding BLUFOR 6/6 pressure/cancel/restart/expiry sequence could not start because the backend prerequisite failed. |
| Z | FAIL / BLOCKED | The two clean pressure sessions could not execute; neither BLUFOR nor OPFOR terminal path received runtime proof. |
| AA | PASS (prior) / H RECHECK BLOCKED | Prior INVALID fail-closed evidence remains valid; the requested M4.4H pressure cancellation regression check could not run. |
| AE | FAIL / BLOCKED | Source compilation and world/resource resolution succeeded, but clean production startup could not pass backend initialization. |

The additional M4.4H runtime requirements remain unverified: stable HQ ownership during both runs; pressure timer one-shot behavior; physical cancellation and fresh restart; expiry to each winner; legal-objective clearing; M3.2/M3.6 suspension; absence of siege AI; siege-hook publication; deployment invariants during ownership changes; exactly-once end-match bridging; and idempotent terminal reconstruction. They are not reported as gameplay failures because execution never reached the proof world.

Final production configuration remains restored: proof mode `0`, M3 coordinator enabled, M3 garrison enabled, BLUFOR hold `300` seconds, OPFOR hold `300` seconds, and no diagnostic actors/state.

**M4.4 AUTHORITY ACCEPTANCE FAIL — the exact remaining blocker is diagnostic backend initialization: `GameConfig/List`, `ENV:[invalid]`, `Curl error=Could not connect to server`.**

Network N1–N5 remain separate pending debt. No local authentication investigation was reopened, and M4.5 was not begun.

## M4.4H backend recovery and authority execution — 2026-10-01

Retail Arma Reforger was manually confirmed healthy on the same machine: startup, multiplayer backend and Community browser all passed. All installed executables report version `1.8.0.13` / engine build `192142`:

- ServerDiag: `D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServerDiag.exe`;
- retail dedicated server: `D:\SteamLibrary\steamapps\common\Arma Reforger Server\ArmaReforgerServer.exe`;
- retail client: `D:\SteamLibrary\steamapps\common\Arma Reforger\ArmaReforgerSteam.exe`;
- Workbench: `D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe`.

No version mismatch was found. WinHTTP and user Internet proxy configuration were both direct/off, no relevant proxy environment variables or hosts-file overrides were present, and system date/time/timezone were sane. Firewall and time-service administrative queries were unavailable in the restricted diagnostic shell, but the successful unrestricted control below made them non-causal for this failure.

### Backend root cause

The failing command was:

`ArmaReforgerServerDiag.exe -server {A441000000000000}Worlds/TB_CC_M4_4_ArlandGraphCampaign.ent -addons 6A6A5973FA53A5B1 -addonsDir <server addons>,<Workbench addons> -profile <fresh M4.4H profile> -maxFPS 60 -logLevel debug`

It was initially launched from the BCM workspace through the restricted automation environment. It discovered vanilla data, BCM `addon.gproj`, the mission/world resources and compiled scripts, then failed its first `GameConfig/List` request with `ENV:[invalid]` / `Curl error=Could not connect to server`.

The last known-good M4.4G command was recovered verbatim from `TestProfiles/M4_4G_ProductionSmoke/logs/logs_2026-10-01_08-38-53/console.log`. It ran ServerDiag from the server installation directory using `-config ServerConfigs/M4_4_ArlandGraphCampaign.json`, the same addon directories, a fresh profile, `-maxFPS 60` and debug logging. Its first meaningful divergence was immediately after `Attempting online Game Config instead`: the good run downloaded/resolved the required addon and registered online, while the restricted run emitted its first curl failure.

Re-running the known-good config/working-directory topology inside the restricted environment reproduced the same curl failure. Running the same topology outside that network-restricted environment immediately succeeded: required addons resolved, the server registered, and `Entered online game state` was reached. A subsequent unrestricted direct-local world launch also reached online state. Therefore:

**Root cause: CONFIRMED — the `ENV:[invalid]` failure was caused by the restricted automated process environment/network containment, not BCM, M4.4H, ServerDiag version, profile state, working directory, or the host Internet/backend path.**

No retail dedicated-server control was rerun because retail client backend health was already manually established and unrestricted ServerDiag itself recovered. No firewall, router, proxy, credentials or gameplay setting was changed.

### Resource differential discovered during recovery

The recovered backend exposed a separate resource-processing problem. The workspace `resourceDatabase.rdb` contained duplicate GUID entries from:

`TestProfiles/M4_4H_BLUFOR/addons/BCMConflictCore_6A6A5973FA53A5B1/...`

for the live M4.4 mission, world and campaign prefab. Workbench explicitly reported those duplicate pairs. This caused the runtime to select the stale processed MODE 9 prefab even when the live source prefab contained mode 0 or mode 10.

All test evidence was preserved. `TestProfiles` was temporarily moved intact outside the addon root, Workbench was launched with the installed base-game addon directory and `-wbsilent -createDB`, and a clean 8,847-byte resource database was generated without duplicate-GUID warnings. `TestProfiles` was then restored unchanged. The authoritative proof source itself was not changed except for the intended proof-mode switch.

### MODE 9 — BLUFOR victory pressure

Evidence:

`TestProfiles/M4_4H_ProductionSmoke_Refreshed2_2026-10-01_09-32-11/logs/logs_2026-10-01_09-32-11/console.log`

Result: **PASS**.

- all six ordinary strategic positions changed USSR → US through genuine vanilla capture;
- revisions progressed from 1 through 7 during the six advances;
- HQ East remained US, HQ West remained USSR and both terminal-seize queries remained false;
- deployment policy repeatedly reported exactly one enabled US MOB spawn and all forward spawns disabled;
- BLUFOR pressure began at 6/6 with depth 1000;
- USSR genuinely recaptured Beauregard, cancelling pressure and returning ACTIVE;
- US genuinely recaptured Beauregard, starting a fresh pressure timer;
- expiry produced `BLUFOR_VICTORY`, revision 10, cleared objectives and froze capture authorization;
- terminal check reported `hqOwnersStable=1`;
- vanilla end-state bridge reported winner US and `count=1`;
- proof completed `captures=8 directOwnership=0`.

The initial East-start state naturally entered OPFOR pressure because USSR owned all six ordinary positions. The first genuine US capture cancelled it before normal progression. M3 was suspended on pressure entry and resumed on return to ACTIVE. No siege actors were created; pressure logs exposed `siegeHook=1` only for OPFOR pressure.

### MODE 10 — OPFOR victory pressure

Evidence:

`TestProfiles/M4_4H_MODE10_Clean_2026-10-01_09-42-51/logs/logs_2026-10-01_09-42-51/console.log`

Result: **PASS**.

- the clean initial state had USSR control of all six ordinary positions;
- the selected US MOB remained US-owned and deployment policy retained one enabled MOB/all forward disabled;
- OPFOR pressure began once with `siegeHook=1`;
- US genuinely captured Military South, cancelling pressure and returning ACTIVE;
- USSR genuinely recaptured Military South, starting a fresh defeat timer;
- expiry produced `OPFOR_VICTORY`, revision 4, cleared objectives and froze capture authorization;
- terminal check reported `hqOwnersStable=1` and the US MOB remained protected;
- vanilla end-state bridge reported winner USSR and `count=1`;
- proof completed `captures=2 directOwnership=0`.

Both runs retained vanilla capture/ownership underneath, never assigned gameplay ownership directly, left FIA outside conventional victory, suspended M3.2/M3.6 during pressure, created no siege force, and preserved the prior INVALID fail-closed evidence. Repeated evaluation during each pressure did not restart its timer, terminal state remained latched, and end-match bridging remained exactly once.

### Final production restoration boundary

**Final M4.4I runtime update (2026-10-01):** Dedicated suite PASS on the final ten-ordinary/twelve-node/twenty-one-edge dataset. Both East and West full BLUFOR pressure proofs and the clean OPFOR proof passed, with physical capture/interruption at all four new positions, MOB-only deployment, exact 100-permille depth progression, stable HQs, terminal idempotence and one bridge per clean run. Supplemental INVALID and read-only placement probes passed after a proof-logger null guard. Final mode-0 production startup passed with 300/300 source holds, enabled M3 systems and no BCM script/metafile/duplicate-GUID errors. Old marker retirement and inherited vanilla sign warnings are explicitly accepted by the Creative Director. `TB-Conflict-Core-M4.4I-Runtime-Acceptance.md` is authoritative for the current evidence and both A–AE matrices; historical FAIL/PENDING narratives below are retained as history. Overall M4.4 U remains PENDING HUMAN, not replaced by AI proof. N1–N5 remain separate PENDING debt. No formal all-criteria milestone closure or M4.5 is claimed.

**Latest M4.4I hygiene evidence:** See `TB-Conflict-Core-M4.4I-Resource-Hygiene.md`. Forty-eight generated BCM addon copies are preserved outside the addon tree; the supported Workbench scan saved a refreshed database without duplicate-GUID warnings. Metafile GUIDs are preserved, and proof-off dedicated startup compiled and initialized 12 nodes/21 edges with 10 required ordinary positions. Four inherited vanilla supply-sign hierarchy errors remain, so full clean-resource validation and physical acceptance are not claimed.

**Subsequent M4.4I decision (2026-10-01):** The Creative Director approved all four saved marker locations as distinct. Source now authors ten ordinary positions, two permanent HQs and twenty-one edges, using separate non-deployable vanilla-compatible capture entities. The latest saved Beauregard transform is authoritative. See `TB-Conflict-Core-M4.4I-Arland-Strategic-Expansion.md` for the current dataset and validation boundary. Automated Workbench validation again failed at launch with 0xc0000142; new resource processing/compile requires the documented normal interactive save. The old final-smoke instruction below is historical and must not be executed against stale resources. Six-position MODE 9/10 PASS remains historical evidence only. Expanded physical and pressure acceptance remains pending; formal closure is incomplete and network N1–N5 remain PENDING.

The source prefab is restored to:

- proof mode `0`;
- M3 coordinator `1`;
- M3 garrison `1`;
- BLUFOR hold `300` seconds;
- OPFOR hold `300` seconds.

However, command-line Workbench `WorldEditor -forceSaveAll` still crashes in the automated invocation before the editor can open (`Cannot open Enfusion shader version file` followed by an application dump). A final ServerDiag smoke therefore continued to instantiate the previously serialized MODE 9/60-second payload despite the correctly restored source and clean database. That smoke was stopped and is not acceptance evidence.

Normal interactive Workbench remains manually proven healthy. The smallest remaining action is to open `Prefabs/Systems/TB_CC_M4_4_ArlandGraphCampaign.et` in normal interactive Workbench, save it, close Workbench normally, and then run one production startup smoke confirming 300 seconds and absence of `[TB_CC_M4_4H_PROOF] START`.

**M4.4 AUTHORITY ACCEPTANCE FAIL — both MODE 9 and MODE 10 authority behaviors passed, but final production processed-resource restoration is not yet runtime-verified. The exact remaining blocker is the automated Workbench prefab-resave crash; interactive save is required.**

Network N1–N5 remain PENDING. No local SteamDiag authentication work was reopened, M4.5 was not begun, and no gameplay implementation was changed.
