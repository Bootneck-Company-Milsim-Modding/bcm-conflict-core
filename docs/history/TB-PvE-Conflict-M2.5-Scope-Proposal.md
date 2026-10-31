# TB PvE Conflict M2.5 — Endpoint Consequence Scope Proposal

## Status

**PROPOSAL ONLY — NOT IMPLEMENTED**

M2.4 implementation is complete and authority acceptance passed, but formal M2.4 remains incomplete while production-route network criteria M and N are unverified. M2.5 implementation must not begin until explicitly authorized. The prepared external retail-client M2.4A acceptance remains an outstanding prerequisite unless that dependency is consciously waived in a later decision.

## Proposed milestone

M2.5 should own the endpoint consequences deliberately excluded from M2.4.

The narrow question is:

> Can the established server-authoritative frontier produce a deterministic, one-shot campaign outcome at either authored route endpoint while preserving vanilla capture, ownership and the public strategic-state contract?

This milestone should convert the already-proven endpoint conditions into an explicit outcome contract. It should not expand into campaign population, attack generation or persistence.

## Foundation to preserve

M2.5 should consume rather than redesign:

- the authored route ID and ordered objective IDs;
- current vanilla ownership;
- authoritative frontier and validity state;
- monotonic strategic revision;
- M2.3 public semantic query contract;
- M2.4 `FRONTIER_AT_OPFOR_ENDPOINT` and MOB-boundary representation;
- vanilla physical capture and ownership callbacks;
- MOB-only player deployment;
- faction restrictions and FIA isolation;
- radio, supply, construction and services.

No consumer should depend directly on the private route array, raw lock bitmask or replicated-field layout.

## Proposed outcome contract

The server should be the sole authority that evaluates endpoint consequences.

Candidate outcome states:

```text
NONE
BLUFOR_VICTORY
OPFOR_VICTORY
INVALID
```

Proposed triggers:

- `BLUFOR_VICTORY`: the authoritative valid frontier reaches the authored OPFOR terminal endpoint after legitimate vanilla ownership change.
- `OPFOR_VICTORY`: OPFOR legitimately captures the authored BLUFOR MOB boundary node, or reaches the precise rear-boundary condition selected during design review.
- `INVALID`: the route/frontier becomes invalid; this must fail closed and must never award either faction victory.

The exact OPFOR defeat boundary requires an explicit design decision before implementation. Reaching frontier index zero is not automatically equivalent to losing the MOB if the MOB remains BLUFOR-owned. Outcome semantics must use authored endpoint identity and vanilla ownership, not index coincidence alone.

## Authority and lifecycle rules

The outcome service should:

1. run only on server authority;
2. evaluate only a `VALID` strategic snapshot;
3. react to accepted authoritative frontier/ownership revisions;
4. transition from `NONE` to one terminal outcome at most once;
5. record the route ID, triggering objective ID and triggering revision;
6. ignore duplicate callbacks and repeated reconstruction of the same state;
7. reject contradictory attempts to award both factions;
8. remain stable for JIP consumers;
9. cleanly reset only when a new scenario/session lifecycle explicitly starts;
10. invoke the narrowest supported vanilla game-over/end-screen mechanism rather than replacing vanilla game mode behavior.

The strategic revision should not advance merely because presentation consumes an outcome. Whether the outcome itself receives a separate revision or is incorporated into the existing projection should be decided from the smallest safe replication design.

## Replication and JIP proposal

Clients need a read-only outcome projection sufficient to answer:

- is the outcome ready;
- is it terminal;
- which faction won;
- which endpoint/objective triggered it;
- at which strategic revision it occurred.

A client connected before the final capture must receive the terminal transition live. A fresh client connected afterward must initialize directly with the same terminal outcome without replaying capture history.

This is why closing M2.4 criteria M and N is the preferred implementation gate: the endpoint consequence will rely on the same production-route projection and JIP path.

## Presentation boundary

M2.5 may connect the replicated outcome to a minimal vanilla-compatible end-state presentation if required for runtime proof. It should not design the final campaign UI.

Allowed presentation scope:

- one unambiguous BLUFOR victory signal;
- one unambiguous OPFOR victory/defeat signal;
- outcome and trigger diagnostics;
- supported vanilla game-over/end-screen invocation if integration is sufficiently understood.

If the correct vanilla end-screen API cannot be established safely, the milestone should first prove the replicated outcome contract and leave presentation integration explicitly pending rather than creating a parallel custom game-mode framework.

## Required investigation before implementation

1. Identify the production Conflict game-mode method/event intended to end a match and assign the winning faction.
2. Verify whether that method is server-only and how it replicates to connected/JIP clients.
3. Determine whether ending the match prevents meaningful post-outcome JIP testing; structure the proof accordingly.
4. Decide the exact OPFOR victory boundary and whether the MOB itself is capturable in the intended production scenario.
5. Determine reset behavior during scenario reload and whether existing session-save behavior is involved.
6. Confirm that outcome presentation does not re-enable forward deployment or bypass faction policy.

## Proposed proof scenario

Use a bounded derivative of the production route with the same public strategic contract.

### Forward endpoint proof

1. Start from a valid state one legal capture before the OPFOR endpoint.
2. Confirm outcome `NONE`.
3. Complete the final legitimate vanilla BLUFOR capture.
4. Confirm the authoritative frontier reaches the OPFOR endpoint.
5. Confirm exactly one `BLUFOR_VICTORY` transition with the correct route, objective and revision.
6. Confirm duplicate callbacks/recalculation do not retrigger the outcome.

### Rear endpoint proof

1. Start from a valid state one legal OPFOR capture before the agreed BLUFOR defeat boundary.
2. Complete the legitimate vanilla OPFOR capture.
3. Confirm exactly one `OPFOR_VICTORY` transition.
4. Confirm BLUFOR cannot receive a contradictory outcome afterward.

### Invalid-state proof

Create a non-contiguous ownership state and recalculate. The campaign must become invalid, expose no legal objectives and award no victory.

### Network proof

1. An already-connected proxy observes the terminal outcome live.
2. A fresh post-outcome client initializes directly with the same terminal outcome if the game lifecycle permits JIP after completion.
3. If the vanilla game-over lifecycle intentionally rejects post-completion JIP, prove the outcome projection immediately before invoking final termination and document that engine constraint explicitly.

## Proposed acceptance criteria

M2.5 should pass only if:

A. Endpoint consequences are configured from authored endpoint identity, not hard-coded route indices.

B. Only server authority can create an outcome.

C. BLUFOR victory follows a legitimate final vanilla capture.

D. OPFOR victory follows the explicitly agreed legitimate rear-boundary capture.

E. Invalid/non-contiguous state cannot award victory.

F. Outcome transition is one-shot and idempotent under duplicate callbacks/reconstruction.

G. Conflicting outcomes fail closed.

H. Outcome records stable route, objective and revision identifiers.

I. A connected proxy receives the terminal transition live.

J. JIP/post-outcome behavior is runtime-proven within the supported vanilla game lifecycle.

K. Vanilla capture, ownership, radio, supply, construction, deployment and faction policy remain intact.

L. No persistence, AI Director, population service or campaign UI is introduced.

## Explicitly out of scope

Do not include:

- automatic attack selection;
- AI Director behavior;
- garrisons, patrols or counterattack waves;
- activation leases or population streaming;
- reinforcements, vehicles or artillery;
- persistence across server shutdown;
- campaign restart/voting flow beyond the minimum required vanilla end-state call;
- strategic economy;
- branching routes or multiple fronts;
- final campaign map/UI;
- human FIA access or safehouses;
- route/radio rebalance;
- changes intended to work around the closed local authentication failure.

## Risks and decisions

| Item | Risk | Required decision |
|---|---|---|
| OPFOR rear boundary | Frontier index zero may precede actual MOB loss | Define victory from endpoint ownership, not index alone |
| Vanilla game termination | Ending immediately may prevent post-outcome JIP proof | Order projection proof before final termination or document engine behavior |
| Duplicate callbacks | Capture/reconstruction can present the same terminal state repeatedly | Persist one authority-owned terminal latch per scenario lifecycle |
| Invalid route | Broken topology could accidentally resemble an endpoint | Require `VALID` before evaluating outcomes |
| Presentation coupling | Custom UI/game-mode replacement would broaden scope | Prefer the supported vanilla outcome path |
| Pending M2.4 network acceptance | Endpoint replication would inherit an unproven production-route client path | Close M/N before implementation where practical |

## Recommended implementation sequence after authorization

1. Close or explicitly waive the M2.4 M/N implementation gate.
2. Perform read-only reconnaissance of the vanilla Conflict end-game API.
3. Decide the precise OPFOR victory boundary.
4. Define the smallest authority-owned outcome state and read-only public query.
5. Add an isolated proof harness without changing frontier derivation.
6. Prove forward, rear, invalid and idempotence cases on dedicated authority.
7. Prove live-client and JIP outcome projection.
8. Integrate the supported vanilla end-state presentation last.

No M2.5 implementation is authorized by this proposal.

