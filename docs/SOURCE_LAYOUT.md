# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, presentation, gameplay AI, and encounter direction.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/reload, debug input, lifecycle and safety infrastructure.
- `src/game` — narrow RDR2-native boundaries, game context, model streaming, and Phase 9 clock/camera queries.
- `src/systems` — Shadowstep, movement, feeding, physical combat, authoritative boss ownership, and Saint Denis encounter direction.
- `src/ui` — reserved for the later temporary red boss-health bar. Phase 9 draws no combat HUD.
- `src/util` — logging and shared utilities.
- `tests` — SDK-independent deterministic tests and test-only native signature fixtures.

## Native boundaries

- `GameApi` — entity/model/coordinates, ground/water/shape tests, relocation, local ped creation/deletion, world traces.
- `GamePresentationApi` — visibility/alpha restoration, melee-input observation, compact smoke.
- `GameCombatApi` — aimed-ped lookup, velocity, combat queries/tasks, owned task cleanup.
- `GameMovementApi` — move-rate and locomotion restrictions.
- `GameFeedingApi` — feed/grapple-facing ped state, LOS, health and participant tasks.
- `GamePhysicalApi` — contact confirmation, ragdoll and bounded impulse boundary.
- `GameEncounterApi` — Phase 9 clock hour, in-game seconds, and camera sphere-visibility queries.

No controller embeds guessed animation/audio/effect hashes.

## Boss ownership registry

`BossActorRegistry` is the single cross-system source of the current Nightwalker boss ped. It records one actor, one owner, and whether autonomous combat is armed.

Owners are:

- `Debug` — F8/F9 development actor;
- `Encounter` — the real Saint Denis Phase 9 actor.

A second owner cannot claim the registry while it is occupied. This prevents duplicate Nightwalker bosses without broad ped-pool scanning.

`DebugVampireSpawner` remains the legacy boss-source dependency used by the mature AI/movement/combat controllers, but its `OwnedPed()` accessor now proxies the registry. Its private local handle is used only so F9 can delete a ped that the debug spawner itself created. F9 cannot delete an encounter-owned boss.

## Phase 9 encounter ownership

`SaintDenisDirector` fulfills the architecture's `EncounterDirector` responsibility. It owns the actual encounter actor handle and exposes `Name() == "EncounterDirector"` to the runtime lifecycle.

State flow:

`Dormant -> Eligible -> Omen -> SpawnPending -> Stalking -> Confrontation -> Combat -> Resolution -> Cleanup -> Cooldown`

Unsafe flow:

`active -> Abort -> Cleanup -> Cooldown`

`EncounterMath` contains SDK-independent time-window, horizontal-radius, and cooldown calculations. `SaintDenisSettingsLoader` reads/clamps the expanded `[Encounter.SaintDenis]` settings without destabilizing the older core parser.

### Spawn ownership

The director requests `cs_vampire` through the existing `ModelStreamRequest`, validates safe pedestrian/ground/water placement, prefers camera-hidden candidates, then claims `BossActorRegistry` as `Encounter`. A failed registry claim removes the just-created ped immediately.

### AI handoff

During Omen, SpawnPending, Stalking and Confrontation the registry actor exists but `CombatEnabled=false`. `VampireAIController` therefore remains idle.

Confrontation completion sets `CombatEnabled=true`. The existing AI then consumes the exact same registered ped; no second Shadowstep/melee implementation exists.

`VampireAIController`, `MovementController`, and `VampireCombatController` still use the existing `DebugVampireSpawner` constructor seam, whose registry-backed `OwnedPed()` accessor makes the handoff transparent.

### Runtime order

Forward update order is:

1. debug spawner;
2. player Shadowstep harness;
3. feeding;
4. EncounterDirector;
5. vampire AI;
6. continuous movement;
7. physical combat.

This lets the director arm combat before AI runs on that frame.

Reverse cancellation restores physical combat/movement/AI first, then EncounterDirector removes its ped. This is required for death, mission/cutscene/world transitions, F10/F11, feature disable, and shutdown.

## Existing combat ownership

`ShadowstepResolver` remains the single teleport landing-safety authority and `TargetedShadowstepPlanner` still generates intercept/flank/behind points. `VampireCombatController` still owns physical specials. `MovementController` remains continuous speed ownership. `FeedingController` retains ambient feeding and the hidden session resource.

Phase 9 only stages and owns the encounter actor around those existing systems.

## Cleanup and persistence boundary

Encounter cleanup disables autonomous combat before clearing tasks/restoring appearance/deleting the encounter-owned ped. Registry ownership is released only after successful deletion; deletion failure remains in Cleanup and retries.

Successful resolution starts the longer configured game-time cooldown. Abort starts a shorter cooldown. These values are session-owned in Phase 9 and do not patch RDR2's save structure. Persistent Nightwalker save data remains later work.

Public CI now tests encounter time/radius/cooldown math, registry exclusivity, and settings clamps; syntax-compiles the split encounter director, Runtime composition, and `GameEncounterApi` native fixture. Actual Saint Denis staging/camera placement and full abort/restart soak remain Story Mode tests documented in `docs/ENCOUNTER.md`.
