# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository uses RDR2 content by runtime reference.

## Status

**Phase 2: safe game-native integration.** Nightwalker can now request and validate the existing `cs_vampire` model, find a nearby navmesh-safe dry spawn point, create one locally owned debug ped, and clean up only that owned entity. No Shadowstep, feeding, boss AI, boss HUD, or combat phases are implemented yet.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`. The solution also contains `Nightwalker.Tests` for pure runtime/model-streaming logic.

See `docs/BUILDING.md` for the local dependency layout and verification checklist.

## Configuration and debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini`. Debug commands remain disabled unless `[Debug] Enabled=true` (or legacy `DebugMode=true`).

Phase 2 debug keys:

- **F8** — request the existing `cs_vampire` model and create one Nightwalker-owned test ped near the player when a safe point is available.
- **F9** — remove only the Nightwalker-owned test vampire.
- **F10** — reload configuration.
- **F11** — cancel systems and restore/clean Nightwalker-owned temporary state.

Repeated F8 presses do not create unlimited duplicates. Model requests time out instead of blocking forever, and mission/player-state transitions or plugin shutdown cancel the request and remove the owned debug ped.

## Native boundary

`GameApi` is the only Phase 2 wrapper for the new model/entity/spawn natives. Controllers do not scatter raw native calls through gameplay code. The debug spawner owns the handle it creates and never attempts to delete arbitrary vanilla entities.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. Nightwalker must not add player power meters or boss ability reveals. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.

