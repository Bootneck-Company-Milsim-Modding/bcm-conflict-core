# TB PvE Conflict M2.5 — Campaign Endpoints & Outcome State

## Verdict

**M2.5 IMPLEMENTATION COMPLETE — M2.5 AUTHORITY ACCEPTANCE PASS — M2.5 NETWORK VERIFICATION PENDING.**

The user confirmed a real Workbench addon compile PASS after the protected `RPC_DoEndMatch` call was moved into valid `modded SCR_GameModeCampaign` scope. ServerDiag then resolved `{A241000000000000}Worlds/TB_PVE_M2_4_ProductionRoute.ent`, entered the world, and initialized the authored ten-node route.

The clean BLUFOR dedicated proof passed. M2.5A then identified vanilla's explicit HQ actor exclusion, and M2.5B introduced the production-scoped terminal-HQ policy. A fresh dedicated OPFOR run proved premature denial, legal exposure, vanilla capture interruption, a second genuine capture, ownership `US -> USSR`, `OPFOR_VICTORY`, revision publication, objective shutdown, reconstruction stability, and one vanilla end-match transition to `POSTGAME`. No ownership setter or custom timer was introduced.

This is distinct from the closed M2.4A authentication issue. No client was launched and no authentication investigation was reopened.

## Endpoint identity model and validation

The route explicitly authors `m_sMobObjectiveId` and `m_sTerminalObjectiveId`. Initialization resolves both IDs with `Find`; it does not infer their identity from array position. Validation requires that the IDs are non-empty, distinct, unique route members, resolve to initialized Conflict military bases, and occupy the two route boundaries in the authored BLUFOR-to-OPFOR ordering. The BLUFOR endpoint must retain its HQ/deployment contract; the OPFOR endpoint must retain the ordinary-base contract. Invalid authoring prevents route readiness and therefore fails closed.

Current production endpoints:

- BLUFOR endpoint: `MOB_NORTH`, resolved route index 0.
- OPFOR endpoint: `OPFOR_ENDPOINT_SOUTH`, resolved route index 9.

The indices are stored from the resolved identities and subsequently used by endpoint policy and outcome derivation.

## Outcome model

`TB_PVE_M2_5_CampaignOutcome` defines:

| Value | Meaning |
|---|---|
| `NOT_READY` | No synchronized strategic snapshot is available. |
| `ACTIVE` | Ownership is valid, contiguous, and non-terminal. |
| `BLUFOR_VICTORY` | Vanilla ownership of the authored OPFOR endpoint is US. |
| `OPFOR_VICTORY` | Vanilla ownership of the authored BLUFOR endpoint is USSR after legal retreat/capture. |
| `INVALID` | Pre-terminal topology or diagnostic validity is fail-closed. |

The public query preserves `NOT_READY` until its first replicated snapshot.

## Derivation and capture policy

The authority observes `SCR_CampaignMilitaryBaseComponent.GetOnFactionChangedExtended`. It scans current vanilla ownership into a contiguous US-prefix/USSR-suffix frontier and then derives the outcome. Valid non-terminal initial, early, mid-route, advanced, and reverse states remain `ACTIVE`.

BLUFOR victory is derived only after the authored OPFOR endpoint's current vanilla owner is US. OPFOR victory is derived only after the authored BLUFOR endpoint's current vanilla owner is USSR. The implementation never assigns ownership.

The MOB is not an initial OPFOR objective. The authority records that BLUFOR has first advanced beyond its endpoint. Only after the frontier has legally retreated back to the MOB boundary is the MOB projected as the OPFOR legal objective. This keeps the existing anti-back-cap protection while permitting the final legal reverse capture.

## Terminal and invalid-state policy

The first authoritative `BLUFOR_VICTORY` or `OPFOR_VICTORY` is latched. Duplicate ownership callbacks, explicit reconstruction, or event ordering cannot change the winner or return the campaign to `ACTIVE`. Both legal objectives are cleared, so further strategic captures are denied. An identical reconstruction does not publish a new revision.

Before a terminal latch, malformed ownership derives `INVALID`, clears both legal objectives, locks the route, and authorizes no strategic capture. It does not repair ownership and cannot infer a winner. A diagnostic invalid override cannot replace an already latched winner.

## Replicated/public contract and revisions

The existing `SCR_GameModeCampaign` replication adapter now includes outcome in the same authoritative snapshot as validity, frontier, legal objectives, lock mask, and revision. `TB_PVE_M2_3_FrontierQuery.GetCampaignOutcome()` is the stable read-only consumer API.

Outcome participates in duplicate comparison. A meaningful `ACTIVE -> victory` or `ACTIVE -> INVALID` transition increments the strategic revision once and invokes the existing strategic-state change notification. Recalculation of the same semantic state is revision-stable. JIP consumes only the current snapshot; historical capture events are not required by the representation.

## Vanilla game-end integration

The Arma Reforger public script API exposes `SCR_GameModeCampaign.RPC_DoEndMatch(int winningFactionId)`, `GetWinningFactionId()`, and `FactionManager.GetFactionIndex(Faction)`. M2.5 uses that minimum vanilla bridge after TB has independently derived and published a terminal outcome:

1. resolve `US` or `USSR` from the TB outcome;
2. obtain the vanilla faction index;
3. call a narrow public M2.5 method on the modded campaign game mode, which invokes the protected vanilla `RPC_DoEndMatch` from valid class scope exactly once.

`m_bInvokeVanillaGameEnd` defaults to enabled and is enabled in the production route. The bounded proof mode can disable it so the server remains available for terminal assertions. Vanilla game end is a consequence, never the source of campaign truth.

## FIA and system regression boundaries

Only faction keys `US` and `USSR` are accepted by the strategic attack query and outcome derivation. FIA cannot become a conventional winner. Physical capture, capture timers/interruption, ownership, radio, supplies, construction, services, and ownership replication remain vanilla-owned.

The proof harness checks the established deployment contract: the BLUFOR MOB is the only route spawn permitted to be enabled and every forward route spawn remains disabled. M2.5 introduces no spawn, radio, supply, construction, or service mutation.

## Reconstruction

Terminal outcome is calculated from explicit endpoint identities plus current vanilla ownership during the normal route scan. An all-US route reconstructs `BLUFOR_VICTORY`; an all-USSR route reconstructs `OPFOR_VICTORY`. No event history or persistence layer is consulted. The runtime proof includes an explicit second recalculation and requires the same outcome and unchanged revision.

## Proof harness and validation

`TB_PVE_M2_5_CampaignOutcomeProof` provides two fresh-session modes because terminal state is immutable:

- mode 1: physically capture every legal objective with US AI through the OPFOR endpoint;
- mode 2: physically capture objective 1 with US AI, reverse it with USSR AI, verify MOB legality, then physically capture the MOB with USSR AI.

Both modes also check initial `ACTIVE`, premature-MOB denial, FIA isolation, invalid fail-closed behavior, revision advancement, terminal idempotence, and deployment state. They use the same real AI-group/vanilla-seizing mechanism as the accepted M1/M2.4 authority proofs.

### Dedicated runtime evidence — 29 September 2026

Resource gate: **PASS**. ServerDiag loaded the production-route world, entered the online game state, resolved all ten bases, validated `MOB_NORTH` at index 0 and `OPFOR_ENDPOINT_SOUTH` at index 9, and published revision 1 as `ACTIVE`.

BLUFOR run: **PASS**. Every route ownership transition was a genuine vanilla AI capture. At revision 11 the OPFOR endpoint was the legal US objective. Vanilla changed its owner `USSR -> US`; TB then derived `BLUFOR_VICTORY`, published revision 12 with both legal objectives cleared, invoked the vanilla end-match bridge once, and vanilla entered `POSTGAME`. Explicit reconstruction retained outcome 2 and revision 12. The harness logged `PASS COMPLETE winner=BLUFOR ... oneShot=1 frozen=1 deployment=1 vanillaPhysicalCapture=1 FIAWinner=0`.

Original pre-M2.5B OPFOR run: **FAIL**, retained as root-cause evidence. It reached legal MOB authorization but vanilla's blanket HQ exclusion produced no eligible actors and timed out. This failure was resolved by the production-scoped policy below; it is not the final authority verdict.

Logs are preserved under:

- `TestProfiles/M2_5_BLufor`
- `TestProfiles/M2_5_BLufor_Run`
- `TestProfiles/M2_5_BLufor_Authority`
- `TestProfiles/M2_5_ResourceGate`
- `TestProfiles/M2_5_BLufor_Acceptance`
- `TestProfiles/M2_5_OPFOR_Acceptance`
- `TestProfiles/M2_5B_OPFOR_Authority_Acceptance`

The production prefab is left with proof mode `0`, the obsolete M2.4 auto-proof disabled, and vanilla game-end integration enabled.

## M2.5A — MOB/HQ Terminal Capture Investigation

### Verdict

**PASS (investigation and disposable experiment only).** The exact vanilla blocker is proven and a narrowly scoped experiment produced a genuine `US -> USSR` MOB capture while retaining HQ status and the established deployment contract. The experimental eligibility seam was removed after the run and the production route was restored to proof mode `0`. Therefore this result does **not** change overall M2.5 to PASS: the production correction still has to be deliberately applied and the clean OPFOR authority acceptance rerun.

### Ordinary base versus MOB

The ordinary route bases and `MOB_NORTH` share the same project-owned `TB_PVE_ConflictMilitaryBase.et` root and therefore the same inherited vanilla Conflict base, `SCR_CampaignSeizingComponent`, `m_bIgnoreNonPlayableAttackers=0`, and `m_bCapturingRequiresPlayer=0`. Both are initialized, use the same US/USSR faction objects, and remain inside the same Conflict game mode. The capture-relevant differences are:

| Property / role | Ordinary route base | `MOB_NORTH` |
|---|---|---|
| Runtime HQ state | `IsHQ()=0` | `IsHQ()=1` |
| Authored HQ eligibility | `m_bCanBeHQ=0` | `m_bCanBeHQ=1`, selected as the US main base |
| Normal seize-task/target role | Eligible when ownership/radio/policy allow | Excluded from normal seize targets |
| Player deployment | Forward spawn is deliberately disabled | Campaign HQ spawn remains the sole enabled US route spawn |
| Supplies/radio/services | Ordinary Conflict base behavior | HQ source/deployment, HQ radio, supply regeneration, construction and services remain attached |

There was no distinguishing non-playable-faction flag, player-required flag, missing initialization, radius, AI LOD, ownership, or TB strategic-policy error. The decisive difference was the runtime HQ designation.

### Exact vanilla path and blocker

The original failed run was repeated with state-change diagnostics under `[TB_PVE_M2_5A]`. At the legal terminal boundary the trace recorded:

`alive=30 inside=30 belowMaxLOD=30 vanillaEligible=0 prevailing=<none> contesting=0 captureState=0 owner=US isHQ=1 initialized=1 radius=50`

This locates rejection before prevailing-faction selection and before the vanilla timer/ownership path. Installed/current source tracing identifies the precise gate: `SCR_CampaignSeizingComponent.EvaluateEntityFaction(IEntity)` returns `null` when `m_Base.IsHQ()` (and also when the campaign base is uninitialized). The generic `SCR_SeizingComponent` actor checks had already succeeded geometrically and by life/LOD, but the campaign override rejects the actor solely because the base is an HQ. Consequently `EvaluatePrevailingFaction()` cannot select USSR, the base never enters `CONTESTED` or `CAPTURING`, and no timer starts. This is a campaign/HQ-layer rejection, not a TB frontier rejection and not a later ownership veto.

Vanilla also excludes HQs from ordinary seize-task/primary-target selection (`IsValidForSeizeTask` and campaign target selection reject HQ bases). No separate physical HQ area-capture path was found in the inspected current implementation.

### Vanilla Conflict endgame distinction

Normal Conflict does not expect victory to be earned by occupying the enemy HQ capture radius. The official Conflict description defines victory through capturing the configured number of strategic positions and holding them for the configured duration; the current scripts maintain control-point/victory state and exclude HQs from normal seize targeting. `RPC_DoEndMatch` is the downstream match-ending mechanism, not evidence of an HQ capture mechanism. HQ radio, spawn, supply and service behavior supports the faction campaign but is deliberately separate from ordinary area seizure. TB's desired “last legal objective is the enemy MOB” rule is therefore a project-specific terminal semantic layered onto Conflict, not restoration of a dormant vanilla HQ-capture setting.

### Options assessed

1. **Configuration only — unsuitable while retaining a real HQ.** No seizing flag bypasses the explicit `IsHQ()` rejection. Clearing HQ selection/`m_bCanBeHQ` could make an ordinary base capturable, but would remove the very main-base role whose player spawn, HQ radio, supply and service semantics must be preserved.
2. **Scoped campaign-seizing extension — proven feasible and recommended.** Recover an otherwise valid physical actor only when authority is evaluating the exact authored BLUFOR endpoint, it is still an initialized HQ, the attacker is USSR, and TB currently exposes that endpoint as USSR's legal objective. Leave prevailing selection, capture state, timer, interruption, ownership mutation and all downstream callbacks in vanilla.
3. **Terminal capture proxy — viable fallback, not needed.** An adjacent ordinary capturable Conflict base could represent the final military objective while the MOB remains HQ, but it adds authored topology and weakens the requested literal-MOB terminal rule.
4. **Custom ownership — rejected.** Direct assignment/fake victory was neither implemented nor tested.

### Disposable experiment and exact temporary change

After the blocker was proven, a temporary `modded SCR_CampaignSeizingComponent.EvaluateEntityFaction` fallback was enabled by one default-off route-controller flag. It first called vanilla. It could recover only an alive, active-LOD, capture-capable USSR character at the exact initialized `IsHQ()` BLUFOR endpoint, and only when the existing TB eligibility query said that base was currently legal. It did not replace `SCR_SeizingComponent`, `EvaluatePrevailingFaction`, capture timing/state, interruption, faction ownership, radio, spawn, supply, construction, services, or end-match code.

For the experiment only, the production-route proof instance set campaign outcome proof mode `2` and the M2.5A flag `1`. A proof-harness pre-phase placed the same type of 30 valid USSR actors at the MOB while TB still denied it. After the run, the fallback, flag, and pre-phase were removed; proof mode was restored to `0`. The persistent source change from M2.5A is diagnostic logging only. No production MOB prefab or vanilla resource was changed.

### Dedicated experimental results — 29 September 2026

Logs: `TestProfiles/M2_5A_HQ_Eligibility_Experiment_Authority/logs/logs_2026-09-29_10-58-28/console.log`.

- **Premature physical presence: PASS.** Thirty USSR actors occupied the technically enabled MOB for about 20 seconds at the initial frontier. TB returned `DENY`; ownership stayed US, capture state stayed `NONE`, and revision 3 remained stable.
- **Legal retreat: PASS.** US physically captured `OBJ_01`, USSR physically recaptured it, revision advanced to 5, and `MOB_NORTH` became the legal OPFOR objective.
- **Vanilla actor eligibility/progression: PASS.** At the terminal MOB the trace changed to `vanillaEligible=30 prevailing=USSR contesting=1 captureState=2 owner=US isHQ=1`. This proves actor discovery, life, radius, active AI LOD, prevailing USSR, and entry into the vanilla capturing state.
- **Timer and ownership: PASS.** Vanilla completed its timer and emitted the normal ownership callback `MOB_NORTH previous=US new=USSR`; no ownership setter was used.
- **Outcome/idempotence: PASS.** TB derived outcome `OPFOR_VICTORY`, published revision 6 with both legal objectives cleared, invoked the USSR vanilla end-match bridge once, and entered `POSTGAME`. Reconstruction retained the winner/revision. The harness logged `PASS COMPLETE winner=OPFOR ... oneShot=1 frozen=1 deployment=1 vanillaPhysicalCapture=1 FIAWinner=0`.
- **Deployment/HQ regression: PASS within automated authority scope.** The experimental base remained `IsHQ=1`; the existing deployment contract check reported `deployment=1`, meaning the MOB remained the only potentially enabled US route spawn and all forward deployment stayed disabled. M1.3 remains the literal player UI/respawn evidence; M2.5A did not repeat a human UI session.
- **Supply/radio/services: no mutation and no observed initialization regression.** The experiment changed only per-actor terminal eligibility and retained the same HQ entity/component graph throughout. It did not mutate supplies, coverage radios, building services, spawn assignment, or construction. A destructive post-capture service-availability test is not meaningful after the match enters `POSTGAME`; pre-terminal interactive service use was not manually repeated and remains an explicit evidence boundary rather than a claimed new UI proof.

### Recommended production correction and remaining unknowns

Adopt the scoped option-2 seam as a named production policy, but keep it narrower than the disposable prototype: authority-only; exact authored BLUFOR endpoint only; initialized real HQ only; USSR only; current strategic outcome `ACTIVE`; current OPFOR legal objective equal to the endpoint; normal actor/life/LOD/faction flags preserved. It should be separately documented and then subjected to a fresh clean OPFOR authority run. Do not globally make every HQ capturable and do not clear the MOB's HQ role.

Remaining evidence debt:

- real Workbench compilation of the eventual production correction;
- a clean production-config OPFOR authority rerun after that correction is intentionally applied;
- literal player deployment/respawn and interactive supply/radio/construction/service regression on that production correction (existing M1.3 evidence remains valid but predates it);
- M2.5 live-client outcome projection and post-victory JIP on an externally authenticated client.

## M2.5B — Production terminal-HQ policy and final authority acceptance

### Production implementation

`TB_PVE_M2_5_TerminalHQCapturePolicy.c` contains the permanent, named campaign policy. Its `modded SCR_CampaignSeizingComponent.EvaluateEntityFaction` calls vanilla first. A normal vanilla result is returned unchanged. Only when vanilla returns `null` at an initialized HQ does it evaluate the narrow exception.

The fallback obtains its actor faction by calling the original generic `SCR_SeizingComponent.EvaluateEntityFaction` through `TB_PVE_M2_5_EvaluateVanillaActor`. This preserves vanilla entity type, living state, vehicle/altitude, respawn-delay and AI maximum-LOD rejection without copying that evaluator. It then preserves campaign faction capture permission, current-owner rejection, `m_bIgnoreNonPlayableAttackers`, `m_bCapturingRequiresPlayer` (fail-closed if enabled), and playable-faction radio policy.

`CanFactionSeizeTerminalHQ` returns true only when all production conditions hold:

1. execution is on server authority;
2. the route is resolved;
3. the base is initialized and currently an HQ;
4. it is the exact resolved authored BLUFOR endpoint object;
5. campaign outcome is `ACTIVE` both authoritatively and through the public projection;
6. actor faction matches configured `m_sOpforFactionKey` (`USSR`);
7. the authoritative OPFOR legal index equals the BLUFOR endpoint index;
8. the public OPFOR legal objective equals that endpoint's authored ID;
9. the existing strategic eligibility calculation authorizes the faction/base pair.

Every other HQ retains vanilla's default rejection. The policy does not make the authored OPFOR endpoint special, does not authorize US or FIA, and becomes false immediately after terminal outcome latching.

### Clean dedicated authority run — 29 September 2026

Log: `TestProfiles/M2_5B_OPFOR_Authority_Acceptance/logs/logs_2026-09-29_11-29-36/console.log`.

- **MOB functionality:** PASS before exposure. `initialized=1`, `isHQ=1`, valid/enabled campaign spawn, resource component present, coverage radio present with one transceiver, two construction providers, three registered services, and 8,530 supplies. Forward deployment remained disabled by the existing route contract.
- **Negative policy matrix:** PASS. At the initial state the exception returned false for USSR at the not-yet-legal MOB, FIA at the MOB, US at the MOB, and USSR at another base. During diagnostic `INVALID`, it remained false. The policy is structurally restricted from the ordinary OPFOR endpoint and from any post-terminal outcome.
- **Premature physical capture:** PASS. Thirty otherwise-valid USSR actors occupied `MOB_NORTH` for approximately 20 seconds. Capture state remained `NONE`, ownership remained US, revision 3 stayed stable, outcome remained `ACTIVE`, and no `CaptureStart` callback fired.
- **Legal progression:** PASS. US physically captured `OBJ_01`; USSR physically recaptured it; the frontier returned to index 0 and published revision 5 with OPFOR legal objective `MOB_NORTH`. The production terminal exception then reported eligible.
- **Interruption:** PASS. Thirty eligible USSR actors produced `vanillaEligible=30`, prevailing USSR, capture state `CAPTURING`, and `CaptureStart` count 1. Removing those actors produced `CaptureInterrupt` count 1, returned capture state to `NONE`, retained US ownership and left revision 5 unchanged.
- **Successful capture:** PASS. A fresh 30-actor attempt produced a second `CaptureStart`, vanilla timer progression, `CaptureFinish` count 1, the normal ownership callback `MOB_NORTH previous=US new=USSR`, and no direct ownership mutation.
- **Outcome/order:** PASS. The ownership callback was observed first; TB derived `OPFOR_VICTORY`; revision 6 published with both legal objectives empty; the vanilla end-match bridge logged exactly once for USSR; the game entered `POSTGAME`. The terminal actor trace immediately returned `vanillaEligible=0`, confirming the exception shut after outcome latching.
- **Idempotence/reconstruction:** PASS. Explicit reconstruction from authored route plus current all-USSR vanilla ownership retained `OPFOR_VICTORY`, frontier `-1`, empty legal objectives, and revision 6. No historical event was required and no second publication/end-match signal occurred before vanilla's expected postgame world transition.
- **BLUFOR regression:** PASS by scope plus retained clean runtime proof. The new policy requires an HQ that is the BLUFOR endpoint and requires configured USSR, so it cannot affect the ordinary authored OPFOR endpoint or US capture. The prior clean BLUFOR run remains the positive runtime proof for `BLUFOR_VICTORY` and its one-shot end-match bridge.

The proof mode is restored to `0`. Production policy remains enabled by its strict state conditions; no disposable enable flag exists.

## Network evidence boundary

M2.2/M2.3 already prove the replication adapter and current-snapshot JIP architecture, including two admitted proxies. That supports the design but is not new M2.5 outcome runtime evidence.

- Live M2.5 outcome projection: **PENDING external admitted-client acceptance**.
- Post-victory M2.5 JIP outcome: **PENDING external admitted-client acceptance**.
- M2.4 criteria M/N remain **UNVERIFIED** and unchanged.

## Acceptance A–O

| Criterion | Status | Evidence / boundary |
|---|---|---|
| A. Explicit endpoint identities and validation | PASS | Dedicated initialization resolved and validated both authored endpoints and all ten route bases. |
| B. Explicit outcome semantics | PASS | Five-state enum and `NOT_READY` query behavior compile. |
| C. Valid non-terminal states remain ACTIVE | PASS | Initial, progressive forward, advanced and reverse states remained outcome 1 in dedicated runs. |
| D. Genuine OPFOR-endpoint capture produces BLUFOR victory | PASS | Vanilla endpoint capture produced ownership US and outcome 2 at revision 12. |
| E. Genuine legal BLUFOR-endpoint capture produces OPFOR victory | PASS | Production exception admitted only vanilla-valid USSR actors after legal exposure; vanilla captured MOB `US -> USSR` and TB derived outcome 3 at revision 6. |
| F. Premature OPFOR MOB capture denied | PASS | Both runs denied the MOB initially; OPFOR run also denied it after advance until legal retreat. |
| G. Terminal outcome one-shot/idempotent | PASS | Both winners were latched; OPFOR explicit reconstruction remained outcome 3/revision 6 and emitted no second end-match signal. |
| H. Invalid topology cannot produce victory | PASS | Both dedicated runs projected INVALID, cleared objectives, and denied strategic attack before restoring ACTIVE. |
| I. Public query exposes outcome | PASS | `GetCampaignOutcome()` added. |
| J. Outcome uses revision/change semantics | PASS | BLUFOR transitioned ACTIVE revision 11 to victory revision 12; reconstruction generated no extra revision. |
| K. FIA cannot win | PASS | Dedicated initial/terminal diagnostics reported FIA attack denied and `FIAWinner=0`. |
| L. Vanilla capture/ownership remain underneath | PASS | All observed ownership changes followed physical AI capture; no ownership setter exists. |
| M. Deployment/base rules intact | PASS | MOB remained initialized HQ with enabled campaign spawn, resource/radio, providers/services and supplies; forward spawns remained disabled. |
| N. Terminal reconstruction from route + ownership | PASS | Both all-US and all-USSR terminal ownership states reconstructed their winner without event history or revision churn. |
| O. Minimum vanilla-supported end state | PASS | Both clean authority paths published terminal state before a single appropriate vanilla end-match bridge and `POSTGAME`. |

## Remaining unknowns and next action

Authority acceptance is complete. The remaining M2.5 debt is network-only: live terminal projection and post-victory JIP require the prepared external retail-client acceptance environment. M2.4 criteria M/N remain pending in that same external environment. Do not reopen local SteamDiag authentication investigation.

After authority acceptance, use the already-outstanding external retail-client session to verify live terminal projection and post-victory JIP. Do not reopen local authentication investigation. Do not begin M3 until the OPFOR authority path is genuinely proven.
