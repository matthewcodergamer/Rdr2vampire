# Building Nightwalker 1.0.0-rc1

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Nightwalker targets **Windows x64 / RDR2 Story Mode**.

## Native plugin dependency

The Script Hook RDR2 developer SDK is intentionally not stored in this repository. Keep its headers and import library in your local dependency location documented by `third_party/ScriptHookRDR2/README.md`, or override the `ScriptHookRdr2Root` MSBuild property.

A genuine `Nightwalker.asi` cannot be linked by public CI without that local developer SDK. Public CI therefore validates pure logic plus controller/runtime/native-boundary compilation, but it does not pretend a stub-linked binary is a releasable plugin.

## Build

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`.

Expected native output:

```text
bin/Release/Nightwalker.asi
```

`Release | x64` disables release PDB generation and, after the ASI links successfully, runs the deterministic release packager. The packager validates the binary, copies only the explicit release allowlist, scans the staged text files for local developer paths, and creates:

```text
artifacts/Nightwalker-1.0.0-rc1-win64.zip
```

Archive contents:

```text
Nightwalker/
  Nightwalker.asi
  Nightwalker.ini
  Nightwalker.dialogue
  README.md
  CHANGELOG.md
  THIRD_PARTY_NOTICES.md
```

The release ZIP intentionally excludes PDBs, logs, state files, build intermediates, repository metadata, SDK headers/libraries and optional LML content.

You can also invoke the packager manually after a successful local Release build:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package-release.ps1 `
  -PluginPath bin/Release/Nightwalker.asi `
  -Version 1.0.0-rc1
```

## Automated test targets

Nightwalker currently has eleven SDK-independent C++ suites:

- `Nightwalker.Tests`
- `Nightwalker.Shadowstep.Tests`
- `Nightwalker.Presentation.Tests`
- `Nightwalker.Targeting.Tests`
- `Nightwalker.Movement.Tests`
- `Nightwalker.Feeding.Tests`
- `Nightwalker.Combat.Tests`
- `Nightwalker.Encounter.Tests`
- `Nightwalker.BossHud.Tests`
- `Nightwalker.SaveData.Tests`
- `Nightwalker.Narrative.Tests`

The foundation suite includes long-session recovery/quarantine semantics and cached-model behavior. The save suite covers migration/corruption/backup recovery. The HUD suite covers fade/death/layout behavior.

## GitHub Actions release gates

`.github/workflows/ci.yml` runs on pushes and pull requests:

- Windows/MSBuild builds and runs all eleven Release x64 pure-test executables.
- Linux/g++ builds and runs the same deterministic logic.
- Linux syntax-compiles gameplay controllers, encounter, HUD, persistence, narrative and Runtime composition.
- Native-boundary sources compile against test-only signatures.
- Custom draw-native use is restricted to `GameBossBarApi.cpp`.
- `scripts/release_audit.py` verifies RC version consistency, Story-Mode-only source policy, required release documentation, local SDK exclusion, tracked build-junk exclusion, package allowlist and current no-attachment/no-collision-toggle release assumptions.

## Release-candidate target testing

Automation is not a substitute for RDR2 Story Mode. Before final `1.0.0`, run `docs/RELEASE_TEST_MATRIX.md` against the exact packaged RC binary. Required manual cases include Arthur, John, Saint Denis dense streets/cathedral/alleys, forest/plains, slopes/stairs, water edges, mounted transitions, wanted level, nearby missions, death interruption cases, boss death during special movement, low/high FPS, keyboard/mouse and controller.

A build should remain `1.0.0-rc*` until that matrix is complete enough to support a stable-release claim.
