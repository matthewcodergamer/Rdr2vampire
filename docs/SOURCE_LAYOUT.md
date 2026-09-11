# Source layout

Phase 0 implements only the plugin boundary, runtime lifecycle, game context, and logging infrastructure.

- `src/Plugin.cpp`: registration boundary and stop signal.
- `src/core`: runtime lifecycle.
- `src/game`: game-facing context.
- `src/util`: shared infrastructure.
- `src/systems`: future gameplay systems.
- `src/ui`: future boss UI.
- `include/nightwalker`: public project headers.
- `tests`: deterministic tests as pure logic is introduced.

Future module names are forward-declared in `include/nightwalker/ArchitectureSeams.h` but are not implemented or instantiated in Phase 0.
