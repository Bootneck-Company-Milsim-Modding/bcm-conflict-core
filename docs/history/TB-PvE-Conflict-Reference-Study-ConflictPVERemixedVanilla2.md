# TB PvE Conflict — Reference Study: ConflictPVERemixedVanilla2.0

**Reference addon:** `ConflictPVERemixedVanilla2.0` (`61B514B96692C049`, version 2.6.7)  
**Reference game version:** 1.8.0.10  
**TB project dependency:** vanilla data only (`58D0FB3206B6F859`)  
**Purpose:** understand a working PvE Conflict implementation and use the architectural lessons to design an original, smaller, testable TB implementation.  
**Constraint:** this is a clean-room design study. No reference script or resource is copied into the TB addon.

## Evidence labels

- **VERIFIED:** observed directly in the packaged project metadata, resource index, or decompressed packaged script.
- **INFERRED:** strongly indicated by resource relationships or code shape, but not executed in a controlled runtime test.
- **UNKNOWN:** requires runtime observation, Workbench resource inspection, or source access.

## Executive conclusion

The reference validates the broad feasibility of the intended game mode, but it is not a suitable implementation template to copy.

Its successful idea is to retain the vanilla Conflict framework and add PvE behavior at a few recognizable seams: campaign seizing, base management, AI group materialization/LOD, authored ambient spawn points, counterattack metadata, task presentation, and persistence serializers. It also builds complete scenarios from authored layers rather than attempting a runtime conversion of every vanilla base.

The TB project is currently much narrower and safer: it changes only the two capture exclusion settings and the forward deployment contract, then proves those changes under dedicated authority. That foundation should remain. The next implementation should adopt the reference's *separation of responsibilities* while avoiding its broad replacements of vanilla methods, global `modded class` behavior, static per-group state, hard-coded prefab identities, and duplicated vanilla algorithms.

Most importantly for the current M1.4 blocker, the reference independently confirms that AI materialization and LOD are central to reliable PvE capture. It explicitly queues group spawning, prevents maximum LOD after population, assigns a permanent LOD, and optionally prevents groups from going dormant. This supports the existing hypothesis that the TB proof actors can remain alive and geometrically inside the radius while becoming ineligible to vanilla seizing because their AI agents reached maximum LOD. It does not justify changing production capture rules.

## 1. What the reference contains

### Project shape

**VERIFIED:** The reference is a packed, vanilla-only addon. It declares three scenarios (Cain, Everon US, and an Everon US 4x variant) and replaces the default system configuration with its own `Configs/Systems/ConflictSystems.conf`.

**VERIFIED:** Its resource inventory includes:

- modified campaign base component and base manager scripts;
- a modified campaign seizing evaluator;
- modified campaign game-mode and AI-group scripts;
- custom ambient infantry and vehicle spawn-point components/systems;
- artillery activity and aiming helpers;
- custom faction, character, group, HQ, base, supply, radio, and loadout resources;
- scenario layers for bases, starting points, infantry patrols, vehicle patrols, civilians, supply caches, supply depots, bunkers, and machine-gun nests;
- a serializer for ambient patrol spawn-point state and a persistence configuration;
- `_NotSpawned` group prefabs used by its managed population system.

**VERIFIED:** The public metadata describes US players versus USSR AI, capturable locations, counterattacks, vehicle patrols, civilians, server-side loading, artillery in the 4x scenario, and server-config persistence.

**VERIFIED:** Strictly limited/linear seize-task selection is advertised through a separate recommended addon, not as an intrinsic guarantee of this reference addon. We must not attribute that feature to its core campaign code.

## 2. How its important systems work

### AI capture

**VERIFIED:** The reference overrides the entire `SCR_CampaignSeizingComponent.EvaluatePrevailingFaction()` method. It:

- counts tagged characters by faction;
- skips civilians;
- treats a non-playable faction as satisfying the player-presence requirement;
- comments out the vanilla non-playable-attacker and player-required rejection block;
- permits non-playable campaign factions without radio coverage;
- uses the normal superior-presence calculation and normal capture state/timer callbacks.

This proves the author understood the same two exclusion gates already handled by `TB_PVE_ConflictMilitaryBase.et`. The reference takes a much larger route by copying/replacing the whole evaluator. TB already has the better solution: configure the inherited vanilla component with `m_bIgnoreNonPlayableAttackers=0` and `m_bCapturingRequiresPlayer=0` and leave the algorithm intact.

### AI population, streaming, and LOD

**VERIFIED:** The reference extends ambient patrol spawn points with explicit roles and lifecycle data:

- normal patrol, vehicle crew, artillery group, or counterattack group;
- finite/infinite respawn waves;
- group-size multiplier;
- eliminated/triggered/processed state;
- association with the nearest campaign base;
- delayed group population through `RequestSpawn`;
- delayed maximum-LOD control after population.

**VERIFIED:** Its `SCR_AIGroup` modification changes group population and dormancy behavior. It queues spawn requests through `ChimeraAIWorld`, implements member expansion, calls `PreventMaxLOD()`, assigns permanent LOD 9, and offers a per-group “can go dormant” switch.

**INFERRED:** These changes are intended to keep strategically important groups materialized and active even when ordinary ambient streaming would unload them. This is directly relevant to capture eligibility because vanilla seizing ignores a non-player AI agent at maximum LOD.

### Counterattacks

**VERIFIED:** Counterattack spawn points are authored as children of campaign bases. The base component tracks arrays of counterattack and artillery spawn points. The base manager can report when a base's tracked counter groups are eliminated. Counter groups support waves, randomized off-base placement, enlarged formations, and lifecycle removal from the base's tracked array.

**INFERRED:** Counterattack activation is event/state driven elsewhere in the ambient patrol system and scenario setup. The architecture couples a generic population mechanism to small per-base descriptors rather than embedding every group directly as always-live AI.

### Ambient infantry, vehicles, civilians, and artillery

**VERIFIED:** Infantry and vehicle population are separate systems with separate spawn-point components. Vehicle patrols can be kept from normal distance-based despawning. Artillery groups validate potential firing positions and associate an artillery target waypoint. The scenarios contain dedicated authored layers for these systems.

**INFERRED:** The reference scales primarily through authored spawn descriptors plus server-side materialization, not through a global strategic simulation that invents all activity dynamically.

### Base management

**VERIFIED:** The modified base component adds counterattack/artillery collections and changes two vanilla behaviors: defender evaluation no longer requires enemy HQ radio coverage, and HQ replacement does not apply the original enemy-HQ exclusion. The modified base manager adds counter-elimination state and changes HQ selection/closest-base lookup.

These changes are tailored to its one-sided scenario design. They should not be imported into TB without an independently stated requirement and a dedicated test.

### Tasks, supply allocation, and persistence

**VERIFIED:** On player audit success, the game mode forces seize tasks to faction ownership/visibility through several task, task-data, and executor fields. It also enables military supply allocation on the server and owning clients, adds that setting to arsenal serialization, enables autosaves, and deletes the latest save at match end to avoid a reported restart/JIP issue.

**VERIFIED:** The addon contains a serializer for ambient patrol spawn-point state, so its spawned/eliminated lifecycle can participate in persistence.

**INFERRED:** The repeated task-field writes are compatibility repair rather than a clean objective-selection policy. TB should define one authoritative objective state and derive tasks from it instead of periodically repairing every task representation.

## 3. Reference risks and defects we should not inherit

1. **Full vanilla-method replacement.** The copied seizing evaluator, game-mode constructor, group population logic, and vehicle processing logic are fragile across Reforger updates. The reference was built for 1.8.0.10 while the local TB investigation is on 1.8.0.13.
2. **Global behavior changes.** Broad `modded class` overrides affect every instance of the vanilla class loaded with the addon, not only PvE entities marked for the feature.
3. **Static state on AI groups.** The inspected `SCR_AIGroup` script stores ambient spawn point, vehicle-patrol status, and spawn-point entity in static fields. That risks state leaking between all group instances.
4. **Hard-coded resource identities.** Group expansion selects character prefabs by exact prefab GUID/name comparisons, making content extension and faction substitution brittle.
5. **Suspicious player-distance test.** In randomized counter/artillery spawn selection, squared distance greater than `45000` is treated as “players very near.” The condition appears reversed or misnamed and warrants runtime confirmation; it must not be reproduced.
6. **Duplicated state repair.** Task ownership and visibility are written through system, task, and task-data APIs. This is difficult to reason about and may hide authority mistakes.
7. **Destructive save handling.** Deleting `.LatestSave.json` at match end is a workaround with data-loss implications and is not a persistence design.
8. **Scenario-specific policy in base primitives.** HQ selection, defender rewards, counters, artillery, and PvE rules are mixed into vanilla base classes, increasing coupling.
9. **Permanent high-detail AI.** Preventing dormancy/permanent LOD can make capture reliable but is likely expensive at campaign scale. Strategic relevance should drive activation rather than applying this globally.

## 4. Comparison with TB PvE Conflict

| Concern | Reference approach | Current TB approach | TB decision |
| --- | --- | --- | --- |
| AI may capture | Replaces the full prevailing-faction evaluator | Two inherited component values disable only the confirmed gates | Keep TB approach |
| Ownership/replication | Uses vanilla faction change after modified evaluation | Proven vanilla US -> USSR -> US on dedicated server and proxy | Preserve |
| Forward deployment | No distinct reference proof found in inspected code | Retained campaign spawn point, disabled server gate, marker-scoped UI hiding | Preserve and continue regression tests |
| Supply/radio/construction | Large scenario resources and some game-mode changes | Vanilla systems retained and measured independently | Preserve; do not replace |
| AI materialization | Custom spawn descriptors, queued population, LOD/dormancy control | Test harness directly creates characters and adds agents | Replace only the disposable proof setup first; design production activation later |
| Counterattacks | Child spawn points tracked by each base, with waves | Not implemented | Adopt the descriptor/state-machine idea, not the code |
| Ambient activity | Authored layers and specialized infantry/vehicle systems | Not implemented | Use authored zones/descriptors with one TB population service |
| Objectives | Task visibility repair; strict limiting delegated to a separate addon | Not implemented | Build an explicit server-authoritative frontier/objective policy |
| Persistence | Custom component serializer plus vanilla save manager | Not implemented | Persist compact strategic state, not live entity graphs |
| Scenario authoring | Complete scenario variants and many layers | One controlled regression world | Continue milestone-first controlled worlds, then author production layers |

## 5. Original TB architecture

### Principle 1 — vanilla owns physical truth

Vanilla continues to own base faction, capture timing, capture interruption, radio graph, supplies, construction, services, spawn registration, flags, and replication. TB systems consume vanilla events and apply narrowly scoped policy.

### Principle 2 — policy is explicit and server authoritative

Create a `TB_PVE_CampaignDirector` (name provisional) on the authority. It owns only strategic policy:

- current campaign phase and active objective set;
- eligible attack/defence bases;
- garrison and reinforcement intent;
- counterattack state;
- victory/defeat state;
- compact persistent campaign snapshot.

It never assigns base ownership to simulate capture. It directs real AI into the vanilla seizing area and observes vanilla faction-change events.

### Principle 3 — descriptors are data, population is a service

Use small, TB-owned descriptor components/resources for garrison, patrol, reinforcement, vehicle, and artillery roles. A single population service evaluates relevance and budgets, materializes groups through supported vanilla spawning, and records abstract state when they unload.

Do not add global behavior to every `SCR_AIGroup`. Keep group lifecycle state per instance and scoped to groups registered with the TB service.

### Principle 4 — activation is a lease

A group remains active while it has a reason:

- near a human observer;
- inside or approaching an active capture area;
- defending the current objective;
- participating in a counterattack;
- in combat or recently observed.

The service renews an activation lease while any reason applies. Only the small set of capture-critical groups receives LOD/dormancy protection, and only for the duration required. This should give the reliability learned from the reference without permanent high-detail AI across the map.

### Principle 5 — persist intent and outcome

Persist base ownership through supported vanilla state and persist TB strategic records such as objective index, descriptor state, wave counts, cooldowns, casualties, and abstract group strength. Recreate transient entities after load. Do not serialize pointers to live AI or repair persistence by deleting saves.

## 6. Build plan

### Gate 0 — resolve the present M1.4 blocker

Before feature work:

1. Add read-only logging for each proof AI's current LOD, maximum LOD, activation state, faction, life state, and distance at every capture-state transition.
2. Repeat the dedicated test using the current direct-materialization harness.
3. Repeat with a normally placed or vanilla-queued group so streaming/navmesh setup matches production.
4. If maximum LOD is confirmed, keep the fix inside the test actor lifecycle: maintain an observer/activation lease or use the supported queued spawn path. Do not replace `EvaluatePrevailingFaction()`.
5. Complete the original M1.4 supply/radio/ownership regression before accepting later milestones.

**Exit:** deterministic dedicated US -> USSR -> US capture plus the already required supply/radio/deployment evidence.

### Phase 1 — campaign state and objective eligibility

1. Add a server-only campaign state component subscribing to vanilla base-initialized and faction-changed events.
2. Define objectives by stable authored IDs and explicit predecessor/successor relationships.
3. Compute a small active-objective set from ownership plus radio reachability.
4. Gate *orders and task exposure* first. Add a capture eligibility gate only if AI or players can physically bypass the intended frontier, and keep that gate marker-scoped.
5. Replicate a compact read-only objective snapshot for client UI.

**Tests:** exactly the intended objective(s) visible; no skip; rollback after recapture; reconnect/JIP consistency; no change to ordinary unmarked Conflict bases.

### Phase 2 — deterministic garrison lifecycle

1. Author a TB garrison descriptor under each test base with faction, composition category, strength, activation radius, and persistence ID.
2. Implement one population service using vanilla entity catalogs/group prefabs and queued group spawning.
3. Add capture-critical activation leases and log every activation/deactivation reason.
4. Maintain per-instance group records; prohibit static shared group ownership state.
5. Treat disabled/dead/dormant counts explicitly and reconcile them after streaming.

**Tests:** dedicated headless operation with no nearby player; capture eligibility remains stable; groups unload/reload without resurrection; budget limits hold.

### Phase 3 — reinforcements and counterattacks

1. Model each operation as a server-side state machine: `PLANNED -> SPAWNING -> ASSEMBLING -> MOVING -> ENGAGED -> SUCCEEDED/FAILED -> COOLDOWN`.
2. Select authored staging descriptors outside immediate player view and validate terrain, water, road access, navmesh, base exclusion, and player distance with independently tested predicates.
3. Issue normal AI waypoints to the target and let vanilla seizing decide the result.
4. Drive wave counts, cooldowns, and strength from data; never hard-code prefab GUID branches.
5. Separate infantry, motorized, and artillery capabilities behind common operation interfaces.

**Tests:** no spawn in view/water/blocked terrain; finite waves terminate; ownership is never assigned directly; failed operations clean up; simultaneous operations respect budgets.

### Phase 4 — ambient world activity

1. Reuse the descriptor/population service for patrols, civilians, traffic, and static positions.
2. Put authored content in optional scenario layers so variants can alter density without code forks.
3. Use distance, relevance, combat, and objective state to control activation.
4. Keep ambient activity outside the strategic ownership path unless explicitly promoted into an operation.

**Tests:** server FPS/entity budgets at low/normal/high density; deterministic cleanup; vehicle crew recovery; no interference with capture counts from civilians.

### Phase 5 — supply, construction, and services policy

1. Keep vanilla storage, income, resource consumers, construction providers, and service delegates.
2. Add policy only where a written game rule differs from vanilla—for example AI operation cost or an objective unlock prerequisite.
3. Charge costs through legitimate resource transactions and log authority-side before/after values.
4. Continue enforcing MOB-only player deployment independently of the retained forward campaign spawn-point reference.

**Tests:** ownership lifecycle, supply regeneration, construction interaction, service availability, and deployment after reconnect/JIP.

### Phase 6 — persistence

1. Version a small TB campaign save schema.
2. Serialize objective state, operation state, descriptor casualties/strength, cooldowns, and stable IDs.
3. On load, wait for vanilla bases to initialize, reconcile ownership/state, then rematerialize only currently relevant groups.
4. Support schema migration and fail safely without deleting user save data.

**Tests:** save/load during idle, active capture, counterattack, and cooldown; server restart plus remote JIP; missing/renamed descriptor recovery.

### Phase 7 — production scenario authoring and balance

1. Build one short vertical slice: MOB plus three objectives, one rollback path, garrisons, one reinforcement route, and one counterattack.
2. Prove it on dedicated authority before expanding the map.
3. Convert the full terrain through reviewed data layers, not runtime mass conversion.
4. Add density presets through configuration rather than separate code paths such as a hard-coded “4x” implementation.

## 7. Immediate recommendation

The next action should remain the narrow M1.4 blocker investigation, with the reference's LOD/materialization design used only as a diagnostic clue. Add LOD/activation telemetry, compare direct materialization with normal queued group spawning, and prove the production vanilla capture contract again under dedicated authority.

M1.4A has now confirmed the precise vanilla failure mechanism in a controlled dedicated experiment: moving three alive, in-radius, still-activated AI agents to maximum LOD made vanilla reject all three and interrupt capture; restoring LOD 0 restored eligibility and restarted capture. Three natural-LOD direct controls completed their initial capture. A queued-spawn comparison materialized no actors in the isolated headless test, demonstrating that vanilla queued spawning requires population/observer context that the disposable harness did not provide. This strengthens the case for a narrowly scoped capture-critical activation lease, but does not justify copying the reference's global/permanent LOD policy. The historical intermittent M1.4 run remains strongly consistent with maximum LOD rather than retrospectively proven, because it predates LOD telemetry.

After M1.4 passes, the first new feature should be a three-objective vertical slice of the explicit campaign-state/frontier layer. Do not begin with counterattacks or ambient population: those depend on a stable definition of which base matters and why.

## 8. Unknowns requiring later verification

1. Exact runtime trigger path for the reference's counterattacks and how it handles multiple simultaneous captures.
2. Runtime cost of its permanent LOD/dormancy changes at normal and 4x density.
3. Whether its suspicious player-distance predicate is a real bug, decompilation/context artifact, or compensated elsewhere.
4. Which fields its patrol serializer persists and how it reconciles live groups after restart.
5. How its separate recommended linear-objective addon interacts with task generation and capture eligibility.
6. Whether the reference's current packaged build behaves correctly on local game version 1.8.0.13.

These unknowns do not block the original TB plan because none of those mechanisms should be copied. They are candidates for controlled behavioral experiments only when the corresponding TB phase begins.
