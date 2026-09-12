# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, presentation, and gameplay AI.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — narrow RDR2-native boundaries, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers plus reusable Shadowstep, movement, feeding and physical-combat logic.
- `src/ui` — reserved for the later temporary red boss-health bar. Phase 8 adds no ability HUD.
- `src/util` — logging and shared utilities.
- `include/nightwalker` — public project headers matching those ownership areas.
- `tests` — SDK-independent deterministic tests and test-only native signature fixtures.

## Native boundaries

- `GameApi` owns entity validity, coordinates, forward vectors, ground/water/shape tests, relocation, model streaming, local ped creation/deletion and the world trace used before a release impulse.
- `GamePresentationApi` owns generic ped visibility/alpha restoration, melee-input observation and compact best-effort smoke PTFX.
- `GameCombatApi` owns aimed-ped lookup, entity velocity, combat-state queries, telegraph tasks, ordinary combat handoff and owned task cleanup.
- `GameMovementApi` owns continuous movement-rate plus swimming, falling, ragdoll and mount-state checks.
- `GameFeedingApi` owns human/mission checks, incompatible locomotion/scenario checks, LOS, health access, face/hold tasks, generic grapple attempt and participant task cleanup.
- `GamePhysicalApi` owns Phase 8 actor-to-target contact confirmation, damage-source clearing, ragdoll entry and center-of-mass impulse application.

Controllers do not scatter these native calls or embed unverified native hashes, animation dictionary names, melee-style hashes or particle names.

## Shadowstep ownership

`ShadowstepResolver` remains the single teleport landing-safety authority. `TargetedShadowstepPlanner` generates intercept/flank/behind candidates. `VampireAIController` is the primary autonomous Shadowstep combat owner for the Nightwalker-owned `cs_vampire`; F7 remains a secondary player debug/safety harness.

Phase 8 does not create another teleport implementation. The boss completes its existing disappearance/relocation/carry/telegraph grammar and only then asks `VampireCombatController` for the follow-up strike.

## Continuous movement ownership

`MovementController` remains separate from Shadowstep and physical combat. It controls only the Nightwalker-owned `cs_vampire` during the AI `Approach` state.

Runtime update order intentionally places `MovementController` before `VampireCombatController`. If the combat controller temporarily owns the modest strike move-rate override, MovementController first restores/suppresses the continuous sprint and the combat controller reapplies only its short strike override. Reverse-order cancellation therefore cleans combat motion before the broader AI/spawner teardown.

## Feeding ownership

`FeedingController` retains the Phase 7 ambient feed state machine and the optional session-only `HiddenResource`. Phase 8 exposes one narrow resource-gain method so a completed player-debug combat feed can replenish the same internal value rather than creating a second resource system. That value remains invisible as HUD and is not persisted yet.

## Phase 8 combat ownership

`VampireCombatController` owns one physical special interaction at a time:

`Idle -> Telegraph -> Align/Hold or Strike -> Release/Feed -> Recover -> Idle`

Implemented move requests:

- `ShadowstepStrike`
- `HeavyStrike`
- `GrabControl`
- `GrabThrow`
- `CombatFeed`

The controller can be used by the Nightwalker-owned boss or the explicit player debug harness, but the actor/target handles are always explicit and revalidated.

### Strike path

The heavy/claw-like V1 path deliberately uses RDR2 ordinary combat. Before the strike, Nightwalker clears the target's previous damage-source marker. A configured small strike bonus is allowed only after `GamePhysicalApi` confirms the intended actor actually damaged the intended target, and the bonus is applied once.

The owned boss may receive a small bounded strike move-rate override; its restore callback returns the value to `1.0`.

### Grab path

Nightwalker aligns the two peds, attempts a short verified generic grapple, and uses a stationary fallback if that task does not start. The hold window is deliberately short because the generic grapple can become lethal if left running. Phase 8 creates no ped attachment and changes no collision, invincibility, camera or input state.

### Physical release path

Before release, all Nightwalker-owned participant tasks are cleared. `MotionImpulseMath` builds a bounded direction/impulse and projected endpoint. The existing world trace checks that short segment. The target is allowed to ragdoll, but the launch impulse is applied only when tracing is conclusive and clear; obstruction or uncertainty suppresses the impulse.

### Combat feed path

A combat feed can follow the owned short grab. The player debug path also allows a direct feed on an explicitly aimed ragdolled human target. Completion applies a bounded health transfer and player-debug completion can reward the existing hidden resource. Boss scripted feed damage is clamped so this scripted effect alone cannot reduce the player below 1 health.

## Vampire AI ownership

At close range, `VampireAIController` decides whether to request a special; it does not implement the special's native/task details itself. For repeatable testing the current close-range sequence rotates:

`HeavyStrike -> GrabThrow -> CombatFeed`

An internal special cooldown prevents frame-by-frame reuse. During a delegated move, ordinary-combat handoff is suppressed until `VampireCombatController` returns to Idle.

## Cleanup boundary

Global Runtime cancellation runs systems in reverse update order. Phase 8 cleanup therefore restores combat-owned tasks/motion before the owned vampire can be deleted. F9/F10/F11, death, mission/control transitions, feature disable and shutdown converge on these lifecycle paths.

Public CI tests the pure release math/configuration and syntax-compiles `VampireCombatController` plus `GamePhysicalApi` against test-only native declarations. Actual RDR2 task choreography, ragdoll appearance and five-minute mixed-combat soak remain target-environment checks documented in `docs/BUILDING.md`.
