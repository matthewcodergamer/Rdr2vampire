# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external developer SDK is not stored in this repository. Put its headers under `third_party/ScriptHookRDR2/inc` and its import library under `third_party/ScriptHookRDR2/lib`, or override the project SDK-root property.

Expected output: `bin/Release/Nightwalker.asi` for Release builds.

For runtime setup, follow the official dependency documentation. This project is Story Mode only.
