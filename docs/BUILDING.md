# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

SDK-independent test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry setting logic.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning and Vampire AI tuning.
- `Nightwalker.Movement.Tests` — Phase 6 movement-ramp math and movement-config bounds.

Run test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all five SDK-independent tests.
- Linux/g++ C++20 builds and runs the same deterministic logic.
- Linux syntax-compiles the Shadowstep, Vampire AI and Movement controllers.
- Test-only native signature fixtures compile `GamePresentationApi.cpp`, `GameCombatApi.cpp`, and `GameMovementApi.cpp` without redistributing Script Hook files.

CI intentionally does **not** link `Nightwalker.asi`. The final plugin still requires the developer-local Script Hook RDR2 SDK and a Windows/RDR2 environment.

## Phase 6 in-game verification

1. Build `Release | x64` with the Script Hook RDR2 developer SDK available locally.
2. Install `Nightwalker.asi`, copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini`, and set `[Debug] Enabled=true`, `[VampireAI] Enabled=true`, `[Movement] Enabled=true`.
3. Launch **Story Mode only**. Confirm the log reports the Phase 6 runtime initialization line.
4. Press **F8** in open ground. Let the owned `cs_vampire` enter ordinary combat and move into an approach/chase state.
5. Create several metres of distance. The vampire should accelerate over roughly `AccelerationMs`, not snap instantly to the configured rate.
6. Confirm the default boost remains modest (`SprintMoveRate=1.15`) and still respects normal RDR2 collision/steering/animation. It must not look like teleportation; Shadowstep remains visually distinct.
7. After roughly `BurstDurationMs`, confirm the movement modifier returns to normal and the internal `RecoveryMs` window prevents a permanent high-speed chase.
8. Confirm debug logs show Movement state transitions and restoration events when Debug logging is enabled.
9. Repeat in **Saint Denis streets**, **Valentine**, **forest terrain**, **open plains**, **stairs/slopes**, and **obstacle-heavy alleys**. Verify the vampire remains steerable and does not launch from small obstacles or desynchronize locomotion badly.
10. Trigger a Shadowstep while a speed burst is active. Continuous movement must restore to normal before/while the AI leaves `Approach`; the teleport presentation must remain the only instant relocation.
11. Force the vampire into ragdoll/falling/swimming states where practical. The modifier must stop immediately and remain off until the restriction clears.
12. If a test setup mounts the controlled ped, confirm the modifier is removed while mounted. After dismounting, confirm the configured `DismountRecoveryMs` delay occurs before movement can re-arm.
13. Press **F11** during a burst. The movement controller must return its owned move rate to `1.0` and become idle.
14. Trigger a mission/cutscene/player-control transition during a burst. Runtime cancellation must restore the move rate before controller shutdown/teardown proceeds.
15. Test player death and vampire death/despawn during a burst. No movement-rate ownership may remain active afterward.
16. Press **F9** to remove the debug vampire and spawn a new one. The new ped must start from normal movement rate.
17. Set `[Movement] Enabled=false` and press **F10** during a burst. The controller must restore normal movement on that update and stay inactive.
18. Change `SprintMoveRate`, `AccelerationMs`, `BurstDurationMs`, `RecoveryMs`, `ActivationDistance`, `MinVelocity`, or trail settings and press **F10**. Values outside configured bounds should clamp and log warnings.
19. With `TrailFx=false`, confirm movement remains functional with no dependency on particles.
20. Run at least a **five-minute chase/combat soak**. Confirm no permanent speed modifier, no continuous boost without recovery, no interference with Shadowstep telegraphs, and no custom player power HUD.
21. Exit/unload normally and confirm orderly shutdown restores owned movement state and logs `Nightwalker shutdown complete.` when Script Hook supplies a normal unload path.

## Phase 6 boundaries

Phase 6 does not add feeding, blood hunger, bespoke vampire attack animations, boss-health UI, encounter scripting, progression, or player supernatural sprint. Camera/FOV changes are intentionally omitted for enemy sprint because they would alter the player's view globally. Custom wind/footstep audio is deferred until a suitable verified RDR2 cue or licensed/original sound path is available.
