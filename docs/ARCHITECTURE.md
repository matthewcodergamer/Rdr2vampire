# Runtime Architecture

`docs/DESIGN_LOCKS.md` is the design authority. `docs/SOURCE_LAYOUT.md` is the current source-tree map. This document describes the runtime architecture that exists now; older planning layouts and superseded player-HUD/hunger concepts are intentionally not repeated here.

## Production target

**Windows x64, Red Dead Redemption 2 Story Mode, native `.asi` plugin.**

Primary runtime dependency: **Script Hook RDR2**. Optional LML content is only appropriate later if Nightwalker needs original content that cannot be delivered cleanly by the ASI. Nightwalker does not target RDR Online.

## Current repository layout

```text
/
├─ include/nightwalker/
│  ├─ core/
│  ├─ game/
│  ├─ narrative/
│  ├─ systems/
│  ├─ ui/
│  └─ util/
├─ src/
│  ├─ core/
│  ├─ game/
│  ├─ narrative/
│  ├─ systems/
│  ├─ ui/
│  └─ util/
├─ tests/
├─ config/
├─ content/
├─ docs/
├─ scripts/
└─ third_party/ScriptHookRDR2/   # local SDK only; binaries/headers are not committed
```

The concrete file-by-file responsibilities live in `docs/SOURCE_LAYOUT.md`.

## Runtime composition

The Script Hook callback remains thin. `Runtime` owns controller lifetime and exposes one controlled tick. Game-facing natives are isolated behind narrow `src/game` APIs; state machines and pure math live under `src/systems` where possible.

Current lifecycle composition includes:

```text
ProgressionController
DebugVampireSpawner
ShadowstepController
FeedingController
NarrativeController
SaintDenisDirector
VampireAIController
MovementController
VampireCombatController
BossHudController
```

Initialization is forward; cancellation/shutdown is reverse. This lets combat, AI, encounter, feeding and presentation release owned transient state before progression checkpoints Nightwalker-owned persistence.

## Story Mode safety and recovery

`RuntimeTick` observes player/control/world continuity and delegates discontinuity policy to the long-session guard. Recovery can be triggered by player death, unsafe mission/control transitions, long frame gaps, large world jumps, player-handle changes, clock rollback, config reload, global cleanup, or shutdown.

The governing rule is ownership: a controller restores only temporary state it explicitly owns. Cleanup methods are idempotent.

Nightwalker does not perform a full ped-pool ownership scan every frame. Boss identity is explicit through `BossActorRegistry`, with cached/throttled validation.

## Game/native boundaries

- `GameApi` — entity/model/geometry/ground/water/relocation.
- `GamePresentationApi` — visibility/alpha and compact Shadowstep smoke.
- `GameCombatApi` — combat queries/tasks and player target context.
- `GameMovementApi` — movement-rate override and locomotion restrictions.
- `GameFeedingApi` — human/health/feed checks and the verified generic Rockstar grapple task.
- `GamePhysicalApi` — damage-source/contact, ragdoll and bounded impulse.
- `GameEncounterApi` — clock/game-time/camera visibility.
- `GameBossBarApi` — normalized boss-bar rectangle/text drawing only.
- `GameNarrativeAudioApi` — optional narrative-audio seam; current release remains subtitle-safe.

Unverified native hashes, animation dictionaries, particle names or audio calls must not leak into controllers. Research gaps stay documented until verified.

## Shadowstep

Shadowstep is teleport/reposition, not extreme running speed. One resolver validates candidate destinations before relocation. Both the player debug harness and vampire AI use the same safety core.

High-level flow:

```text
resolve intent
-> validate full destination
-> compact departure effect
-> brief hide
-> instant relocation
-> restore visibility
-> compact arrival effect
-> short validated arrival carry
-> readable attack opportunity/telegraph
-> recovery/cooldown
```

No teleport-frame damage is applied. Every transient presentation state has a timeout and restoration path. See `docs/SHADOWSTEP.md`.

## Vampire AI and combat

The Saint Denis vampire is the primary production owner of the supernatural combat grammar. `VampireAIController` selects safe intercept/flank/evade Shadowstep candidates and hands physical attacks to `VampireCombatController`.

`VampireCombatController` owns short strikes, grapple/control, throw/release and combat-feed state. Movement-rate changes use watchdog restoration. Physical releases clear owned tasks before ragdoll/impulse and suppress impulse when the obstruction trace is blocked or inconclusive.

### Feeding presentation

`VampireFeedPresentation` is a policy layer, not another entity owner. Current production behavior prefers the already-established Rockstar `TASK_GRAPPLE` paired interaction, keeps it alive through the boss feed hold, and falls back to a bounded stationary hold when pairing cannot start.

The vanilla Saint Denis vampire corpse AnimScene is verified and documented in `docs/research/VAMPIRE_FEED_REUSE.md`, but it is not forced onto arbitrary standing live peds. Front/rear direct-grapple style names are documented research only until the remaining native contract is target-verified.

## Encounter ownership

`SaintDenisDirector` owns the real encounter lifecycle and its explicit `cs_vampire` actor. `BossActorRegistry` is the cross-system reference source. The F8 debug spawner may claim the registry only when it is free; it is not a second production boss implementation.

Conceptual encounter flow:

```text
Dormant/Eligibility
-> Omen/Stalking
-> Confrontation
-> Combat
-> Resolution
-> Cooldown
```

Abort and resolution paths cancel owned AI/combat/narrative/HUD state and release the encounter actor safely. Duplicate boss creation is not permitted.

## Boss HUD boundary

The temporary cinematic red boss-health bar is the **only custom combat HUD**. It is supplied the explicit encounter boss; it never scans for a boss. It fades after inactivity, returns on renewed combat activity, and has a bounded death hold.

No player blood/hunger meter, cooldown meter, ability icon/card, skill wheel, boss phase label, power name, weakness panel, floating damage number or combo counter is part of the architecture.

## Persistence

Nightwalker uses its own versioned `Nightwalker.state`; it never patches RDR2 save structures. `SaveData` is SDK-independent and owns parsing, migration, clamping, backup recovery and replacement. `ProgressionController` owns runtime checkpoint policy.

A supported schema can recover an invalid individual known field to that field's default while preserving other valid fields. Unsupported future schemas are rejected and writes are disabled to avoid destructive downgrade.

## Performance policy

- active per-frame state machines do only work required for the current transient action;
- broad encounter checks are throttled;
- boss-handle validation is cached/throttled;
- model/VFX requests are bounded and released;
- optional profiling uses fixed slots and debug logging rather than another HUD;
- no expensive full-world scan is added to solve local ownership or Shadowstep safety.

## Content boundaries

Allowed in the repository:

- original Nightwalker source/config/docs/dialogue;
- original or properly licensed assets;
- names/hashes used to reference assets already installed in the user's legitimate RDR2 copy.

Not shipped:

- Rockstar game files;
- Script Hook RDR2 redistributables not permitted by their terms;
- ripped Dawnwalker or other commercial assets/code/audio;
- third-party binaries without redistribution permission.

## Build and release boundary

Public CI builds/runs SDK-independent deterministic tests and syntax-compiles gameplay/native boundaries with test signatures. It intentionally does not fake a releasable `Nightwalker.asi`.

A genuine plugin is linked locally with the Script Hook RDR2 developer SDK using `Release | x64`. See `docs/BUILDING.md` and `docs/RELEASE_TEST_MATRIX.md`.
