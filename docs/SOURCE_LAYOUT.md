# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, presentation, and gameplay AI.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — narrow RDR2-native boundaries, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers plus reusable Shadowstep, movement and feeding logic.
- `src/ui` — reserved for the later temporary red boss-health bar. Phase 7 adds no combat HUD.
- `src/util` — logging and shared utilities.
- `include/nightwalker` — public project headers matching those ownership areas.
- `tests` — SDK-independent deterministic tests and test-only native signature fixtures.

## Native boundaries

- `GameApi` owns entity validity, coordinates, forward vectors, ground/water/shape tests, relocation, model streaming and local ped creation/deletion.
- `GamePresentationApi` owns generic ped visibility/alpha restoration, melee-input observation and compact best-effort smoke PTFX.
- `GameCombatApi` owns aimed-ped lookup, entity velocity, combat-state queries, telegraph tasks, ordinary combat handoff and owned task cleanup.
- `GameMovementApi` owns continuous movement-rate plus swimming, falling, ragdoll and mount-state checks.
- `GameFeedingApi` owns Phase 7 feed-facing native calls: human/mission checks, incompatible locomotion/scenario checks, LOS, health access, face/hold tasks, generic grapple attempt and participant task cleanup.

Controllers do not scatter these native calls or embed unverified native hashes, animation dictionary names or particle names.

## Existing vampire combat ownership

`ShadowstepResolver` remains the single teleport landing-safety authority. `TargetedShadowstepPlanner` generates intercept/flank/behind candidates. `VampireAIController` is the primary autonomous Shadowstep combat owner for the Nightwalker-owned `cs_vampire`; F7 remains a secondary player debug/safety harness.

`MovementController` is separate continuous locomotion ownership for that same Nightwalker-owned vampire during its AI `Approach` state. It never replaces Shadowstep with raw speed.

## Phase 7 feeding ownership

`FeedingController` owns one debug player feed interaction at a time:

`Candidate -> Align -> Grab -> FeedLoop -> ReleaseDrain -> Cleanup -> Idle`

Target acquisition uses the existing RDR2 free-aim context rather than a broad ped-pool scan. This keeps V1 deterministic and avoids expensive every-frame world searching.

Safety rules:

- player and target handles are revalidated while active;
- mission-owned peds reject before Nightwalker starts participant tasks;
- incompatible scenario/mount/vehicle/swim/fall/ragdoll states reject;
- LOS, maximum range and vertical alignment are rechecked;
- Sip deliberately avoids the generic grapple task so the non-lethal mode cannot accidentally become a combat kill;
- Drain may attempt RDR2's verified generic `TASK_GRAPPLE`; failure falls back to conservative stationary tasks;
- no attachment, collision disable, invincibility, camera lock, or guessed paired-animation dictionary is introduced;
- cancellation clears only participant task state that FeedingController marked as Nightwalker-owned;
- reverse lifecycle order places FeedingController last in `systems_`, so it cleans its participant state before movement/AI/spawner teardown;
- F10 cancels active feeding before replacing config state;
- player death, mission/player-control transition, F11 and script unload converge on the same cancellation path.

`FeedingMath` contains pure range/vertical helpers. `HiddenResource` stores an optional session-only clamped `0–100` internal value. It is never drawn as HUD and is not persisted until the later save-data system exists.

## Presentation boundary

Phase 7 does not ship a neck blood particle. The particle API is available but no suitable RDR2 asset/effect name was verified strongly enough to commit without guessing. Likewise, no custom feeding animation dictionary is invented. `docs/FEEDING.md` records the current Rockstar-task approximation and the known presentation gap.

## Verification boundary

Public CI verifies feeding config/resource/math and syntax-compiles `FeedingController` plus `GameFeedingApi` against test-only native declarations. Actual grapple appearance, victim animation recovery, mission-script interaction and the required 20-NPC soak must be tested in RDR2 Story Mode with the local Script Hook SDK build.
