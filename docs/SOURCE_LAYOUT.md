# Source layout

Nightwalker is organized so native calls, lifecycle ownership, pure targeting math, presentation, and gameplay AI remain separate.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — narrow RDR2-native boundaries, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers plus reusable Shadowstep planning/safety logic.
- `src/ui` — reserved for the later temporary red boss-health bar. Phase 5 adds no combat HUD.
- `src/util` — logging and shared utilities.
- `include/nightwalker` — public project headers matching those ownership areas.
- `tests` — SDK-independent deterministic tests and test-only native signature fixtures.

## Native boundaries

- `GameApi` owns entity validity, coordinates, forward vectors, safe pedestrian coordinates, ground/water checks, shape tests, relocation, model streaming and local ped creation/deletion.
- `GamePresentationApi` owns generic ped visibility/alpha restoration, player melee-input observation, and compact best-effort smoke PTFX.
- `GameCombatApi` owns Phase 5 combat-facing calls: player aimed-ped lookup, entity velocity, ped combat state, stand-still telegraph tasks, ordinary `TASK_COMBAT_PED` handoff, and owned task cleanup.

Controllers do not scatter these native calls or embed unverified native hashes.

## Shared Shadowstep safety and targeting

- `ShadowstepMath` contains game-independent vector/range helpers.
- `ShadowstepResolver` remains the single landing-safety authority. Phase 5 adds `ResolveToPoint`, allowing target-relative AI candidates to reuse the same obstruction, navmesh, ground, vertical, water and clearance checks as the Phase 3 player harness.
- `TargetedShadowstepPlanner` generates and scores four target-relative candidates: intercept, left flank, right flank and behind. It owns only deterministic selection/prediction logic; it does not mutate game entities.
- `ShadowstepPresentationSettings` owns disappearance/carry timing loaded from `Nightwalker.ini`.

The planner never makes an unsafe point valid. If all candidate resolutions fail, callers must fall back to ordinary movement/combat.

## Actor ownership in Phase 5

`VampireAIController` is the primary Shadowstep combat owner. In Phase 5 it controls **only** the `cs_vampire` handle created and owned by `DebugVampireSpawner`; it does not scan for, take over, or delete arbitrary vanilla peds.

Its active flow is:

`Observe -> Approach -> Decide -> ShadowstepDepart -> HiddenTransit -> ShadowstepArrive -> Telegraph -> Attack -> Recover -> Cooldown`

`Evade`, `Reposition`, `FeedAttempt`, and `Abort` remain explicit states. Evade has conservative Phase 5 behavior; Reposition and FeedAttempt are future seams only.

Important ownership rules:

- vampire visibility restoration is registered before hiding;
- relocation must use a planner result already accepted by `ShadowstepResolver`;
- arrival carry is short and revalidated;
- the teleport itself does no damage;
- `TASK_COMBAT_PED` is issued only after the readable post-arrival telegraph;
- separate Shadowstep/evade cooldowns prevent spam;
- cancellation restores the owned vampire before the spawner may delete it;
- player death, ped invalidation, F9/F11, config reload, mission/cutscene transition and shutdown all converge on cleanup.

## Player-side harness

`ShadowstepController` remains a secondary debug/safety harness. F7 performs the forward blink by default. If the existing RDR2 free-aim context reports a living ped already in combat with the player, the same `TargetedShadowstepPlanner` can choose a target-relative debug landing.

This is not a new player-targeting system and it adds no custom landing UI.

## Known Phase 5 verification boundary

The geometry resolver checks world/object/vehicle obstruction and explicit separation from the combat target. Phase 5 deliberately avoids an expensive broad ambient-ped scan every frame. Dense crowd occupancy therefore remains an in-game verification item; any later nearby-ped occupancy helper must use a verified RDR2 native contract rather than guessed trace flags.
