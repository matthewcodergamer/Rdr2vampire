# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

The solution also builds `Nightwalker.Tests`, a console test target for pure config/timing/watchdog/model-streaming logic. Run `bin/tests/Release/Nightwalker.Tests.exe` (or the Debug equivalent) after building. These tests do not require RDR2 or Script Hook.

For runtime setup, follow the official Script Hook RDR2 dependency documentation. This project is Story Mode only.

## Phase 2 in-game smoke test

1. Copy `Nightwalker.asi` into the RDR2 directory required by the Script Hook runtime.
2. Copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini` and set `[Debug] Enabled=true`.
3. Launch Story Mode only and stand in an open, dry outdoor area with normal player control.
4. Confirm `Nightwalker.log` reports Phase 2 initialization.
5. Press **F8** once. Confirm the log records the model request, then a single `cs_vampire` appears a few metres from the player and the log records its owned entity handle and model-load time.
6. Press **F8** again while that ped still exists. Confirm no second vampire is created and the duplicate request is ignored.
7. Press **F9**. Confirm only the Nightwalker-created test vampire is removed and cleanup is logged.
8. Press F8 again, then enter a mission/cutscene or otherwise cause the runtime safety transition. Confirm Nightwalker cancels the request or removes the owned test ped.
9. Spawn one more test vampire, then unload/exit normally. Confirm shutdown cleanup removes the owned ped when the Script Hook lifecycle permits orderly shutdown.
10. Test near a water edge or obstructed terrain. The system should refuse unsafe placement and log an error rather than creating a ped in water/geometry.
11. F10 still reloads config and F11 still performs global Nightwalker-owned cleanup.

No boss fight, Shadowstep, feeding, combat powers, or custom HUD should appear in this phase.
