# Source layout

Nightwalker is organized so game-native calls, lifecycle infrastructure, pure safety logic, and gameplay controllers stay separate.

- `src/Plugin.cpp` — Script Hook registration boundary and shutdown signal only.
- `src/core` — runtime composition, config parsing/validation, debug input, lifecycle and safety infrastructure.
- `src/game` — RDR2-native boundaries, game context, and model streaming helpers.
- `src/systems` — owned gameplay/debug controllers and reusable gameplay logic.
- `src/ui` — reserved for the later temporary red boss-health bar; no Phase 4 HUD is implemented.
- `src/util` — logging and other shared utilities.
- `include/nightwalker` — project headers matching those ownership areas.
- `tests` — pure deterministic test programs that avoid requiring RDR2 where practical.

## Shadowstep ownership through Phase 4

- `ShadowstepController` owns the explicit Shadowstep state machine, cooldown, cancellation, presentation sequencing and stress counter.
- `ShadowstepResolver` remains the reusable authority for the **primary** destination: path obstruction, shortening, navmesh/ground, vertical delta, water and final clearance/headroom.
- `ShadowstepMath` contains game-independent vector/range helpers and remains directly unit-testable.
- `GameApi` owns geometry/entity/relocation RDR2 native calls. Resolver/controller code does not embed those natives.
- `GamePresentationApi` owns the small Phase 4 presentation-facing native surface: visibility/alpha restore, melee-input observation and best-effort smoke PTFX.
- `ShadowstepPresentationSettings` loads/clamps Phase 4-only feel values from the existing `Nightwalker.ini` without changing the older config contract.
- `ShadowstepPresentation.cpp` contains idempotent visibility cleanup, state watchdog timing and collision-aware post-arrival carry logic.
- `Runtime` owns controller lifecycle and routes debug input, config reload and game-safety-transition cancellation.

The post-arrival carry does not replace the primary resolver. It receives a separately prevalidated short endpoint and dynamically rechecks the current carry segment; uncertainty stops the carry instead of forcing movement.

Phase 4 deliberately does not implement aimed/hold Shadowstep, combat target selection, enemy/boss AI, bats, custom audio, feeding, supernatural sprint or custom player HUD. The melee-intent buffer has an explicit dispatch seam, but synthetic replay remains disabled until that native path is safely implemented and verified in the target environment.
