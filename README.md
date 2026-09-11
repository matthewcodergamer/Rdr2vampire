# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository uses RDR2 content by runtime reference.

## Status

**Phase 4: Shadowstep presentation.** The geometry-safe Phase 3 relocation is now wrapped in the signature Nightwalker presentation sequence: validate first, compact departure smoke, very short disappearance, instant relocation, guaranteed visibility restoration, slightly stronger arrival smoke, a short collision-aware forward carry, a brief melee-input buffer window, recovery, and internal cooldown.

The base destination resolver remains the authority. Presentation is not allowed to make an unsafe destination valid. The arrival carry prevalidates a short endpoint and rechecks its moving segment every frame; blocked or inconclusive carry movement simply stops early rather than clipping.

There is still **no combat targeting, enemy/boss Shadowstep AI, feeding, boss fight, aimed landing HUD, cooldown meter, power HUD, or boss HUD implementation** in this phase. Bats and custom audio are also deferred until a suitable lightweight, verified asset/cue is selected.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

The solution contains three SDK-independent test executables:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests.
- `Nightwalker.Presentation.Tests` — Phase 4 presentation-setting parsing/default/clamping tests.

GitHub Actions runs these pure tests on both Windows/MSBuild and Linux/g++. CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local external dependency and is not committed to the repository.

See `docs/BUILDING.md` for the local dependency layout and exact in-game verification checklist.

## Configuration and debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini`. Debug commands remain disabled unless `[Debug] Enabled=true` (or legacy `DebugMode=true`).

Current debug keys:

- **F7** — request one forward Shadowstep.
- **F8** — request the existing `cs_vampire` model and create one Nightwalker-owned test ped near the player when a safe point is available.
- **F9** — remove only the Nightwalker-owned test vampire.
- **F10** — reload configuration, including Phase 4 presentation tuning.
- **F11** — cancel systems and restore/clean Nightwalker-owned temporary state.

Phase 4 presentation defaults are `DisappearMs=110`, `ArrivalCarryMeters=1.25`, `ArrivalCarryMs=140`, `MeleeBufferMs=220`, `StateTimeoutMs=1000`, and `SmokeFx=true`. These are internal timing/feel values and are never exposed as player HUD.

## Shadowstep safety and presentation

`ShadowstepController` now owns:

`Idle -> ResolveIntent -> ValidateDestination -> Depart -> Relocate -> HiddenTransit -> Arrive -> ArrivalCarry -> MeleeWindow -> Recovery -> Cooldown -> Idle`

The player is hidden only after the main destination is fully validated. Visibility restoration is registered with an idempotent watchdog before the hide occurs. Phase 4 does **not** disable player collision, grant invincibility, lock input, or modify the camera.

Smoke is best-effort presentation. Nightwalker references the existing RDR2 `scr_fme_spawn_effects` / `scr_fme_smoke_puff_tint` particle at runtime; a missing/unloaded particle never blocks relocation or cleanup.

The melee buffer currently captures melee intent during the transition and owns the short post-arrival window. The production native boundary contains a dispatch seam, but synthetic control re-injection is intentionally disabled in this repository build until it can be written and verified safely in the target environment. Normal RDR2 melee input is never locked.

## Native boundaries

`GameApi` remains the geometry/entity/relocation boundary. `GamePresentationApi` contains only Phase 4 presentation-facing natives: visibility/alpha restoration, melee-input observation, and best-effort particle streaming/playback. Controllers do not embed raw native calls.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. Nightwalker must not add player power meters or boss ability reveals. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
