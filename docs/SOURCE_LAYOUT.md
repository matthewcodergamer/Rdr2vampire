# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, presentation, and gameplay AI.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — narrow RDR2-native boundaries, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers plus reusable Shadowstep and movement logic.
- `src/ui` — reserved for the later temporary red boss-health bar. Phase 6 adds no combat HUD.
- `src/util` — logging and shared utilities.
- `include/nightwalker` — public project headers matching those ownership areas.
- `tests` — SDK-independent deterministic tests and test-only native signature fixtures.

## Native boundaries

- `GameApi` owns entity validity, coordinates, forward vectors, ground/water/shape tests, relocation, model streaming and local ped creation/deletion.
- `GamePresentationApi` owns generic ped visibility/alpha restoration, melee-input observation and compact best-effort smoke PTFX.
- `GameCombatApi` owns aimed-ped lookup, entity velocity, combat-state queries, telegraph tasks, ordinary combat handoff and owned task cleanup.
- `GameMovementApi` owns Phase 6 movement-facing calls: move-rate override plus swimming, falling, ragdoll and mount-state checks.

Controllers do not scatter these native calls or embed unverified native hashes.

## Shadowstep ownership

`ShadowstepResolver` remains the single teleport landing-safety authority. `TargetedShadowstepPlanner` generates intercept, flank and behind candidates. `VampireAIController` is the primary autonomous Shadowstep combat owner for the Nightwalker-owned `cs_vampire`; the player F7 controller remains a secondary debug/safety harness.

## Phase 6 continuous movement

`MovementController` is deliberately separate from Shadowstep. It controls only the Nightwalker-owned `cs_vampire` during the AI `Approach` state and never teleports the actor.

Active flow:

`Idle -> RampUp -> Boost -> Recovery -> Idle`

Restrictions can divert to `Restricted`, which always restores the move-rate ownership first.

Ownership rules:

- before applying a multiplier above 1.0, the controller registers an idempotent `OwnedState::Motion` restore callback;
- the restore callback revalidates that the captured handle still belongs to `cs_vampire` before writing 1.0, protecting against stale/recycled handles;
- the move-rate native is applied every active frame because it is a per-update locomotion override;
- only `VampireAIState::Approach` is eligible, so Shadowstep departure/transit/arrival, telegraph and attack phases run at normal movement rate;
- a short acceleration ramp precedes the boost and a recovery window follows every completed burst;
- mounting, a detected dismount edge, swimming, falling or ragdoll immediately suppress movement ownership;
- player death, mission/cutscene transition, F11 cleanup and script shutdown are handled by Runtime's reverse-order lifecycle cancellation;
- no player movement modifier is applied in Phase 6.

`MovementMath` contains pure smooth-ramp and horizontal-speed helpers so timing/math can be tested without RDR2.

## Presentation boundary

Phase 6 can emit a small low-frequency dark smoke/dust puff through the already verified `GamePresentationApi` effect. Particle failure is non-fatal. Global camera/FOV manipulation is intentionally omitted because the supernatural sprint actor is the enemy vampire, not the player. Custom wind/footstep audio is deferred until an appropriate verified or licensed/original cue exists.

## Verification boundary

Public CI verifies pure movement math/configuration and compiles `MovementController` plus `GameMovementApi` against test-only native declarations. The final animation stability, steering feel, obstacle behavior and exact maximum comfortable multiplier require the documented Story Mode test matrix on a real RDR2/Script Hook setup.
