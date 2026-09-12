# Building and downloading Nightwalker.asi

Nightwalker is a native Windows x64 RDR2 Story Mode ASI plugin. The repository does not commit the Script Hook RDR2 SDK or runtime binaries.

## Easiest path: GitHub Actions from a phone

1. Open the Nightwalker repository on GitHub.
2. Open **Actions**.
3. Select **Build Nightwalker ASI**.
4. Tap **Run workflow**, choose `main`, then run it.
5. Open the completed workflow run.
6. Under **Artifacts**, download `Nightwalker-Release-x64-<commit-sha>`.
7. On iPhone, open the downloaded ZIP in Files and extract it.
8. The package contains:
   - `Nightwalker.asi`
   - `Nightwalker.ini`
   - `Nightwalker.dialogue` when present
   - `README.md`
   - `SHA256.txt`

The workflow downloads the developer SDK directly from Alexander Blade's official Script Hook RDR2 site at build time, stages the headers/import library only inside the temporary GitHub runner, builds `Release | x64` with MSVC, verifies that `Nightwalker.asi` exists and is non-empty, records its SHA-256, then uploads only the Nightwalker package.

The SDK itself is not committed to this repository or included in the artifact.

## Local Windows one-command build

With PowerShell and Visual Studio 2022 Desktop C++ installed:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-asi.ps1 -Configuration Release
```

If `third_party\ScriptHookRDR2\inc\main.h`, `inc\natives.h`, and `lib\ScriptHookRDR2.lib` are missing, the script downloads the official developer SDK and stages it automatically.

Successful output:

```text
bin\Release\Nightwalker.asi
dist\Nightwalker\Nightwalker.asi
```

The script also prints the file size and SHA-256.

## Installing the built plugin into RDR2

The compiled Nightwalker artifact does **not** replace Script Hook RDR2 itself.

From the official Script Hook RDR2 distribution, the RDR2 game directory needs the normal Script Hook runtime/ASI loader setup (for example `ScriptHookRDR2.dll` and the supported ASI loader such as `dinput8.dll`). Follow the official Script Hook RDR2 installation instructions for those files.

Then place the Nightwalker package files beside `RDR2.exe` as appropriate for the plugin setup, including `Nightwalker.asi` and `Nightwalker.ini`. Keep `Nightwalker.dialogue` beside the plugin when using the Phase 12 external narrative file.

Story Mode only. Do not use Nightwalker in RDR Online.

## Failure behavior

The build stops instead of uploading a fake artifact when:

- the official SDK cannot be downloaded;
- `main.h`, `natives.h`, or `ScriptHookRDR2.lib` cannot be found in the SDK package;
- MSBuild returns a failure;
- `bin\Release\Nightwalker.asi` is missing or empty.

This means a green **Build Nightwalker ASI** run is evidence that the repository actually compiled and linked the ASI against the downloaded developer SDK on the GitHub Windows runner.
