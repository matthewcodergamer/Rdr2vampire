# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

The solution also builds `Nightwalker.Tests`, a console test target containing only pure C++ foundation logic. Run `bin/tests/Release/Nightwalker.Tests.exe` (or the Debug equivalent) after building. These tests do not require RDR2 or Script Hook.

For runtime setup, follow the official Script Hook RDR2 dependency documentation. This project is Story Mode only.

## Phase 1 in-game smoke test

1. Copy `Nightwalker.asi` into the RDR2 directory required by the Script Hook runtime.
2. Optionally copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini`.
3. Launch Story Mode only.
4. Confirm `Nightwalker.log` reports the startup sequence and Phase 1 initialization.
5. With `[Debug] Enabled=true`, press F10 and confirm `Configuration reloaded.` is logged.
6. Press F11 and confirm the debug cleanup action is logged without changing gameplay.
7. Start/enter a mission or cutscene and confirm the transition cleanup message appears once rather than spamming every frame.
8. Exit normally and confirm `Nightwalker shutdown complete.` is logged when the Script Hook lifecycle permits orderly shutdown.
