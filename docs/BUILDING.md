# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

SDK-independent test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination safety.
- `Nightwalker.Presentation.Tests` — Phase 4 disappearance/carry setting logic.
- `Nightwalker.Targeting.Tests` — Phase 5 intercept/flank/behind planning, unsafe fallback, and Vampire AI config/clamping.

Run test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all four SDK-independent tests.
- Linux/g++ C++20 builds and runs the same deterministic logic.
- Linux additionally syntax-compiles `ShadowstepController`, `ShadowstepPresentation`, and `VampireAIController`.
- Test-only native signature fixtures compile `GamePresentationApi.cpp` and `GameCombatApi.cpp` without redistributing Script Hook files.

CI intentionally does **not** link `Nightwalker.asi`. The final plugin still requires the developer-local Script Hook RDR2 SDK and a Windows/RDR2 environment.

## Phase 5 in-game verification

1. Build `Release | x64` with the official Script Hook RDR2 developer SDK available locally.
2. Install `Nightwalker.asi` in the RDR2 Script Hook loading location.
3. Copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini`.
4. Set `[Debug] Enabled=true`, `[VampireAI] Enabled=true`, and launch **Story Mode only**.
5. Confirm `Nightwalker.log` reports that the Phase 5 runtime initialized with the owned `cs_vampire` as the primary debug Shadowstep combat actor.
6. Press **F8** in an open dry area. Confirm exactly one Nightwalker-owned `cs_vampire` appears and begins ordinary RDR2 combat with the player.
7. Back away from the vampire until roughly within the configured `ShadowstepMinDistance`/`ShadowstepMaxDistance` band. The vampire should periodically choose a legal intercept/flank destination, emit compact departure smoke, disappear briefly, relocate, reappear with arrival smoke, perform the short carry when safe, pause for the configured telegraph, then resume normal RDR2 combat.
8. Confirm the vampire never damages the player on the exact teleport frame. Any damage must come from normal RDR2 combat after the readable arrival tell.
9. Repeat while moving laterally and while retreating quickly. Debug logs should record each candidate type, score or rejection reason, chosen candidate, and cooldown decisions.
10. Fight in a narrow Saint Denis-style alley, beside walls/props, on stairs and near water. If all target-relative candidates are unsafe, the vampire must remain in ordinary combat rather than teleport through geometry or force an unsafe landing.
11. Stand very close and commit to melee attacks. Occasional evade Shadowsteps may occur, but the configured evade cooldown must prevent repeated every-hit escapes.
12. Observe combat for **at least five continuous minutes**. Verify Shadowstep spacing feels deliberate rather than continuous spam; default base cooldown is 2400 ms and default evade cooldown is 5000 ms.
13. During that five-minute run, verify the vampire never remains invisible, becomes permanently frozen, remains task-locked, teleports inside the player, falls through the world or gets stuck under/inside geometry.
14. Press **F9** during ordinary combat and during/near a Shadowstep. Nightwalker must restore the owned vampire's appearance/tasks before the debug spawner removes only that owned ped.
15. Spawn again, trigger a Shadowstep, then press **F11**. The AI must abort, restore visibility/alpha, clear Nightwalker-owned combat/task state, and return safely to Observe/idle ownership.
16. Spawn again and trigger a mission/cutscene/player-control transition. Runtime cancellation must clean the AI presentation before the spawner is allowed to delete its ped.
17. Test player death and vampire death/despawn. No hidden/transient AI state may survive actor invalidation.
18. Change `ShadowstepCooldownMs`, `TelegraphMs`, `EvadeCooldownMs`, `PredictionMs`, `StrikingRange`, or distance-band values, then press **F10**. Safe values should reload; unsafe values should clamp and log warnings.
19. Set `SmokeFx=false` and reload. Vampire teleport behavior must remain functional even if smoke is disabled/unavailable.
20. Aim explicitly at a hostile ped and use **F7**. The player-side debug harness may choose a target-relative safe candidate, but confirm this remains secondary to the vampire's autonomous combat behavior.
21. Confirm no player power HUD, boss phase label, cooldown readout, ability name, floating damage number, or boss power reveal is displayed.
22. Exit/unload normally and confirm orderly shutdown restores owned presentation/tasks and logs `Nightwalker shutdown complete.` when Script Hook supplies a normal unload path.

## Phase 5 boundaries

Phase 5 does not implement the final Saint Denis encounter director, boss-health bar, feeding, supernatural sprint, bespoke vampire attack animations, scripted damage, boss phases, or progression. `Reposition` and `FeedAttempt` remain explicit AI seams for later phases, not hidden unfinished behavior.
