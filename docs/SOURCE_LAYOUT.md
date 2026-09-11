# Source layout

Nightwalker is organized so game-native calls, lifecycle infrastructure, pure safety logic, and gameplay controllers stay separate.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — the `GameApi` RDR2-native boundary, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers and reusable gameplay logic.
- `src/ui` — reserved for the later temporary red boss-health bar; no Phase 3 HUD is implemented.
- `src/util` — logging and other shared utilities.
- `include/nightwalker` — project headers matching those ownership areas.
- `tests` — pure deterministic test programs that avoid requiring RDR2 where practical.

## Phase 3 Shadowstep ownership

- `ShadowstepController` owns the explicit V1 state machine, cooldown, cancellation and stress counter.
- `ShadowstepResolver` owns reusable destination validation: path obstruction, shortening, navmesh/ground, vertical delta, water and final clearance/headroom.
- `ShadowstepMath` contains game-independent vector/range helpers and is directly unit-testable.
- `GameApi` owns all Phase 3 RDR2 native calls. The controller/resolver do not embed native hashes or call the SDK directly.
- `Runtime` owns controller lifecycle and routes debug input/safety-transition cancellation.

Phase 3 deliberately does not implement Shadowstep VFX, invisibility, collision toggles, arrival carry, combat targeting, boss AI, feeding or custom HUD.
