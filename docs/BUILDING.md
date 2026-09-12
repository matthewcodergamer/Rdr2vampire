# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

SDK-independent test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry setting logic.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning and Vampire AI tuning.
- `Nightwalker.Movement.Tests` — continuous-movement ramp math and config bounds.
- `Nightwalker.Feeding.Tests` — hidden-resource clamping, feed geometry math and feeding config/backwards-compatibility rules.

Run test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all six SDK-independent tests.
- Linux/g++ C++20 builds and runs the same deterministic logic.
- Linux syntax-compiles Shadowstep, Vampire AI, Movement and Feeding controllers.
- Test-only native signature fixtures compile `GamePresentationApi.cpp`, `GameCombatApi.cpp`, `GameMovementApi.cpp`, and `GameFeedingApi.cpp` without redistributing Script Hook files.

CI intentionally does **not** link `Nightwalker.asi`. The final plugin still requires the developer-local Script Hook RDR2 SDK and a Windows/RDR2 environment.

## Phase 7 in-game verification

1. Build `Release | x64` with the Script Hook RDR2 developer SDK available locally.
2. Install `Nightwalker.asi`, copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini`, and set `[Debug] Enabled=true`, `[Feeding] Enabled=true`.
3. Launch **Story Mode only**. Confirm `Nightwalker.log` reports the Phase 7 runtime initialization line and no custom blood/hunger HUD appears.
4. Approach a normal ambient human NPC, free-aim at that ped from within about `MaxDistance`, and press **F5**. Sip should align/hold the pair, complete non-lethally, restore configured player health, then release/clear Nightwalker-owned tasks.
5. Confirm the Sip target remains alive and can resume normal AI after cleanup.
6. Repeat on another ambient human and press **F6**. Drain may enter the verified RDR2 generic grapple approximation; if the grapple cannot start, the stationary fallback should still complete safely. The completed Drain should kill the target and restore the configured player health/resource amount.
7. Press F5 or F6 again during **Align**, **Grab**, **FeedLoop**, and **ReleaseDrain** in separate tests. Every cancellation must return to Idle and clear only Nightwalker-owned participant tasks.
8. Press **F11** during each active feeding stage. Neither participant may remain frozen or task-locked afterward.
9. Press **F10** during an active feed. Feeding must cancel before configuration is replaced; the next attempt should use the reloaded safe/clamped values.
10. Move the target/player beyond the allowed range during a feed. The interaction should abort and clean up.
11. Break line of sight with a wall/large obstruction during a feed. The interaction should abort and clean up.
12. Test targets on stairs/uneven terrain where vertical separation exceeds the V1 limit. Unsafe alignment should reject rather than forcing bad positioning.
13. Aim at a mission-owned/script-owned ped where RDR2 reports mission ownership. Feeding must reject that target and leave its tasks untouched.
14. Aim at peds using ambient scenarios, mounted peds, vehicle occupants, swimmers, falling/ragdolled peds. V1 should reject them rather than interrupting incompatible game state.
15. With `AllowAnimalFeeding=false`, verify non-human peds reject. If temporarily enabled for testing, animal feeding should use the conservative stationary path rather than the human grapple approximation.
16. Test player death during a feed and a mission/player-control transition during a feed. Runtime cancellation must clear owned participant tasks.
17. Verify F7 Shadowstep does not start while a feed is active. After cleanup, F7 must work normally again.
18. Spawn the debug vampire with F8 and verify Phase 5/6 enemy Shadowstep/movement still function after multiple player feeding attempts; Phase 7 does not yet make the enemy vampire feed autonomously.
19. Disable `[Feeding] Enabled=false` and reload. F5/F6 should not start feeding.
20. Set extreme feeding values, reload, and verify clamping/warnings in `Nightwalker.log`.
21. Perform at least **20 varied ambient human feed attempts**, mixing Sip, Drain, successful completions, cancels, walls, stairs and different NPC archetypes. No victim may remain floating, attached, invisible, frozen, permanently task-locked, or stuck in a Nightwalker state.
22. Confirm no player blood meter, hunger meter, feed meter, ability card, cooldown bar, or other new custom HUD appears at any point.
23. Exit/unload normally and confirm orderly shutdown logs `Nightwalker shutdown complete.` when Script Hook supplies a normal unload path.

## Phase 7 boundaries

Phase 7 does not persist the internal resource yet; persistence belongs to the later separate Nightwalker save-data system. It does not add enemy-vampire autonomous feeding, combat-feed executions, regeneration, progression, boss-health UI, or encounter scripting. It also does not ship a guessed neck-blood particle or unverified vampire feeding animation. `TASK_GRAPPLE` is used only as a best-effort Drain approximation because its exact style parameters are under-documented; Sip deliberately uses the safer hold path.
