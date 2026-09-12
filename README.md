# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 6: controlled supernatural movement.** The Nightwalker-owned Saint Denis vampire (`cs_vampire`, hash `0xD95BCB7D`) now has a separate continuous-speed system in addition to the Phase 5 Shadowstep combat AI.

Shadowstep remains the impossible discontinuous move: vanish, validated relocation, reappear, short carry, readable attack. `MovementController` handles only ordinary chase locomotion while `VampireAIController` is in `Approach`. It ramps the owned vampire from normal movement toward a conservative configured multiplier, sustains a short burst, restores 1.0, then enforces an internal recovery window before another burst.

The controller automatically restores/withholds the modifier while the vampire is mounted, immediately after dismounting, swimming, falling, ragdolled, no longer in the AI approach state, invalid/despawned, or when Movement/Debug is disabled. Runtime mission/cutscene/player-death transitions, F11 cleanup, and script shutdown cancel the controller through the shared lifecycle path.

Phase 6 does **not** alter the player's movement multiplier. It also does not add a player speed meter, cooldown meter, FOV overlay, neon trail, boss phase label, or any new custom combat HUD.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables include:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests.
- `Nightwalker.Presentation.Tests` — disappearance/carry presentation-setting tests.
- `Nightwalker.Targeting.Tests` — target-relative planning and Vampire AI config tests.
- `Nightwalker.Movement.Tests` — Phase 6 acceleration-ramp math, velocity math and movement-config bounds.

GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++. Linux CI also syntax-compiles Phase 6 controller code and the game-native boundaries against test-only signature fixtures. Public CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local dependency.

See `docs/BUILDING.md` for local dependency layout and the exact Story Mode verification checklist.

## Debug encounter controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` and set `[Debug] Enabled=true`.

- **F7** — player-side Shadowstep safety/targeting harness.
- **F8** — spawn one Nightwalker-owned `cs_vampire` debug ped. With `[VampireAI] Enabled=true`, it becomes the autonomous vampire combat actor; with `[Movement] Enabled=true`, its ordinary approach/chase can receive Phase 6 speed bursts.
- **F9** — despawn only the Nightwalker-owned debug vampire.
- **F10** — reload configuration.
- **F11** — cancel systems and restore Nightwalker-owned temporary state.

## Phase 6 movement defaults

The initial tuning is deliberately restrained pending real RDR2 soak testing:

- `SprintMoveRate=1.15`
- `AccelerationMs=350`
- `BurstDurationMs=1800`
- `RecoveryMs=900`
- `DismountRecoveryMs=500`
- `ActivationDistance=4.5`
- `MinVelocity=0.30`
- `TrailFx=true`
- `TrailIntervalMs=180`

`SprintMoveRate` is clamped to `1.0–1.20`. The controller calls the verified RDR2 move-rate native every active frame and registers an idempotent restore to `1.0` before owning a non-default rate.

A small best-effort dark smoke/dust puff may be emitted at a low cadence while boosted. Missing VFX never blocks or extends movement ownership. Phase 6 deliberately avoids changing the player's gameplay camera for an enemy sprint, and no custom wind/footstep sound is shipped until a suitable verified RDR2 cue or licensed/original audio path is selected.

## Native boundaries

- `GameApi` owns entity/geometry/model operations.
- `GameCombatApi` owns combat state, velocity and combat-task calls.
- `GamePresentationApi` owns visibility/alpha and compact smoke presentation.
- `GameMovementApi` owns Phase 6 movement-rate and locomotion-restriction natives (`SET_PED_MOVE_RATE_OVERRIDE`, swimming, falling, ragdoll and mount state).

Controllers do not scatter raw native calls or embed guessed native hashes.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. The enemy/boss vampire owns the signature supernatural combat fantasy first. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
