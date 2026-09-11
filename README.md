# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository uses RDR2 content by runtime reference.

## Status

**Phase 3: geometry-safe Shadowstep V1.** Nightwalker now has a debug-only forward blink with an explicit state machine and a reusable destination-safety resolver. The resolver shortens blocked paths, requires conclusive geometry traces, validates pedestrian navmesh/ground, limits vertical change, rejects deep water, and checks destination clearance/headroom before relocation.

There are still **no Shadowstep VFX, disappearance effects, arrival carry, combat targeting, boss AI, feeding, boss fight, or custom combat HUD** in this phase. V1 is intentionally about making the relocation safe and repeatable first.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

The solution contains two pure test executables:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests without launching RDR2.

See `docs/BUILDING.md` for the local dependency layout and exact in-game verification checklist.

## Configuration and debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini`. Debug commands remain disabled unless `[Debug] Enabled=true` (or legacy `DebugMode=true`).

Current debug keys:

- **F7** — request one forward Shadowstep V1.
- **F8** — request the existing `cs_vampire` model and create one Nightwalker-owned test ped near the player when a safe point is available.
- **F9** — remove only the Nightwalker-owned test vampire.
- **F10** — reload configuration.
- **F11** — cancel systems and restore/clean Nightwalker-owned temporary state.

Shadowstep defaults are conservative: 6.5 m forward range, 550 ms cooldown, 1.5 m maximum vertical delta, 250 ms validation timeout, and 0.65 m wall/prop clearance. Existing Phase 1/2 config clamp ranges remain backwards-compatible; legacy `MaxVerticalRise` is accepted as an alias for `MaxVerticalDelta`.

## Shadowstep V1 safety model

`ShadowstepController` owns the explicit runtime sequence:

`Idle -> ResolveIntent -> ValidateDestination -> Depart -> Relocate -> Arrive -> Recovery -> Cooldown -> Idle`

Error/cancel paths return safely to `Idle`. Phase 3 does **not** disable collision, hide the player, grant invincibility, modify camera/input, or apply supernatural movement. If the validated relocation cannot be confirmed, Nightwalker attempts to roll the player back to the known start point and logs the outcome.

`ShadowstepResolver` owns reusable destination checks so future aimed/combat Shadowsteps can reuse the same safety core instead of duplicating geometry logic.

## Native boundary

`GameApi` remains the game-native boundary. Phase 3 adds wrappers for player forward direction, ground probing, synchronous world/object/vehicle LOS shape tests, and no-offset relocation. Gameplay controllers do not embed raw native hashes or scatter direct native calls.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. Nightwalker must not add player power meters or boss ability reveals. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
