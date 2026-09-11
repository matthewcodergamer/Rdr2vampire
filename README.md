# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository uses RDR2 content by runtime reference 

## Status

**Phase 0: native project skeleton.** No supernatural gameplay or custom HUD is implemented yet.

The project now has a Visual Studio x64 plugin target, a minimal initialize/tick/shutdown lifecycle, game-context and logging infrastructure, and reserved module seams for later phases.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

See `docs/BUILDING.md` for the local dependency layout and `docs/SOURCE_LAYOUT.md` for code ownership.

## Runtime layout

For runtime setup, follow the official Script Hook RDR2 documentation. Nightwalker itself is delivered as `Nightwalker.asi`; future optional configuration will remain separate from the binary.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. In particular, Nightwalker must not add player power meters or boss ability reveals. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.

## Phase 0 contents

- `src/Plugin.cpp` — external registration boundary only.
- `src/core/Runtime.cpp` — thin lifecycle.
- `src/game/GameContext.cpp` — plugin path/context seam.
- `src/util/Logger.cpp` — diagnostics.
- `include/nightwalker/ArchitectureSeams.h` — future module declarations without fake implementations.

