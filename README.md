# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository uses RDR2 content by runtime reference.

## Status

**Phase 1: runtime foundation.** No Shadowstep, feeding, vampire combat, boss AI, or custom combat HUD is implemented yet.

The runtime now owns deterministic initialization/update/shutdown, timestamped logging, typed INI configuration with safe defaults and clamping, feature flags, monotonic timing, debug-only hotkeys, conservative player/mission transition detection, and an idempotent cleanup ledger for temporary state future systems may own.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`. The solution also contains `Nightwalker.Tests`, which tests pure config/timing/cleanup logic without launching RDR2.

See `docs/BUILDING.md` for the local dependency layout and `docs/SOURCE_LAYOUT.md` for code ownership.

## Configuration

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` to override defaults. Missing or malformed values fall back safely; unsafe numeric ranges are clamped and logged. Debug hotkeys are disabled unless `[Debug] Enabled=true`.

Default debug keys are F10 to reload configuration and F11 to force Nightwalker-owned cleanup. They do not enable any gameplay mechanic.

## Runtime layout

For runtime setup, follow the official Script Hook RDR2 documentation. `Nightwalker.log` is written beside the plugin when the location is writable; logging failure does not terminate the plugin.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. Nightwalker must not add player power meters or boss ability reveals. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.

## Phase 1 foundation

- `Runtime` — controlled lifecycle and future controller update seam.
- `Config` — typed INI defaults, validation, clamping, feature flags, legacy aliases.
- `Logger` — debug/info/warn/error diagnostics.
- `SafetyWatchdog` — idempotent ownership/restoration bookkeeping.
- `DebugInput` — debug-only edge-triggered hotkeys.
- `GameContext` — plugin paths plus conservative player/mission safety state.
- `Timing` — monotonic clocks/deadlines independent of frame rate.
- `ILifecycleSystem` — explicit initialize/update/cancel/shutdown contract for later controllers.

