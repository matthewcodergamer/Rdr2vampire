# Systems source directory

Phase 0 intentionally contains no gameplay controller implementations.

Future modules belong here when their owning phase begins: `SafetyWatchdog`, `ShadowstepController`, `MovementController`, `FeedingController`, `VampireAIController`, and `EncounterDirector`.

The ownership names are forward-declared in `include/nightwalker/ArchitectureSeams.h`. Do not add empty implementation files merely to make the tree look complete.
