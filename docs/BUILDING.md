# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

The solution builds three SDK-independent C++ test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination-safety logic.
- `Nightwalker.Presentation.Tests` — Phase 4 presentation-setting parsing/default/clamping logic.

Run the test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs the SDK-independent tests on every push and on pull requests targeting `main`:

- Windows: Visual Studio/MSBuild Release x64 builds and execution of all three test programs.
- Linux: g++ C++20 compilation and execution of the same deterministic logic.

The workflow intentionally does **not** build/link `Nightwalker.asi`. The native plugin requires the developer-local Script Hook RDR2 SDK plus the Windows game environment, and those proprietary/local dependencies are not committed to the repository.

For runtime setup, follow the official Script Hook RDR2 dependency documentation. This project is Story Mode only.

## Phase 4 in-game verification

1. Build `Release | x64` with the official Script Hook RDR2 developer SDK present.
2. Copy `Nightwalker.asi` into the RDR2 directory used by the Script Hook runtime.
3. Copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini` and set `[Debug] Enabled=true`.
4. Launch **Story Mode only** and stand in a normal open, dry outdoor area with player control.
5. Confirm `Nightwalker.log` reports `Phase 4 runtime initialized; Shadowstep presentation remains debug-only and geometry-gated.`
6. Press **F7** once in open terrain. The step should still use the Phase 3 validated endpoint before any presentation starts.
7. Confirm a compact dark smoke puff appears at departure when the referenced RDR2 particle asset is available. If the asset fails to load, the teleport must still complete and cleanup must still succeed.
8. Confirm the player disappears only very briefly (default `DisappearMs=110`) and becomes visible again at the validated destination. The player must never remain invisible after the sequence.
9. Confirm arrival smoke is slightly stronger than departure and remains compact rather than filling the screen.
10. Confirm the player carries/glides forward up to the configured `ArrivalCarryMeters` (default 1.25 m) over `ArrivalCarryMs` (default 140 ms).
11. Repeat beside walls, props, railings, corners, stairs and narrow alleys. The carry must stop early when the dynamic segment becomes blocked or inconclusive; it must never push the player through geometry.
12. Repeat near slopes and water edges. Carry must stop when safe pedestrian placement, ground, vertical delta or water checks become unsafe.
13. Confirm the base blink still shortens/rejects blocked primary destinations exactly as Phase 3 did. Presentation must not weaken the original resolver.
14. Trigger F7 and then force **F11 cleanup** during the short sequence. Visibility must be restored, presentation state must return to idle, and no particle loop/state should remain owned.
15. Trigger a mission/cutscene/player-control transition during the sequence. The same cancellation/visibility restoration must occur.
16. Disable Shadowstep or Debug via the INI and press F10. Any active presentation must be cancelled and the player must be visible/normal.
17. Test player death during the sequence. Nightwalker must not leave visibility/alpha modified after normal game control returns.
18. Change `DisappearMs`, `ArrivalCarryMeters`, `ArrivalCarryMs`, `MeleeBufferMs`, `StateTimeoutMs`, or `SmokeFx`, press F10, and confirm Phase 4 settings reload without rebuilding. Unsafe values should clamp and generate warnings.
19. With `SmokeFx=false`, confirm F7 still performs the safe disappearance/reappearance/carry sequence with no smoke dependency.
20. Press melee during departure/hidden/arrival and confirm normal RDR2 melee controls remain live. The controller records a brief melee-intent window; synthetic replay of a released tap is currently disabled in this repository build and must **not** be claimed as verified until the target-environment dispatch path is implemented/tested.
21. Perform the existing **100 consecutive successful Shadowstep** stress run across open streets, alleys, stairs and varied safe terrain. No step may leave the player invisible, collisionless, invincible, control-locked, under the world, or embedded in geometry.
22. Confirm F8/F9 vampire spawn/despawn and F10/F11 infrastructure still work after the stress run.
23. Exit/unload normally and confirm `Nightwalker shutdown complete.` is logged when orderly Script Hook shutdown is available.

## Phase 4 boundaries

Phase 4 does not implement aimed/hold Shadowstep, combat target selection, enemy/boss AI, bats, custom audio, feeding, supernatural sprint, boss combat, or any custom player HUD. The destination-safety resolver remains reusable for those later phases.
