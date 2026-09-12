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
8. The package contains `Nightwalker.asi`, `Nightwalker.ini`, `Nightwalker.dialogue` when present, `README.md`, and `SHA256.txt`.

The workflow downloads the developer SDK directly from Alexander Blade's official Script Hook RDR2 site at build time, stages the headers/import library only inside the temporary GitHub runner, builds `Release | x64` with Visual Studio 2022/MSVC, verifies that `Nightwalker.asi` exists and is a Windows PE image, records its SHA-256, then uploads only the Nightwalker package.

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

## Installing into RDR2

The Nightwalker artifact does not replace Script Hook RDR2 itself. Follow the official Script Hook RDR2 installation instructions for the runtime/ASI loader files in the RDR2 game directory, then place the Nightwalker package files in the normal plugin location beside the game executable as appropriate for the ASI setup.

Story Mode only. Do not use Nightwalker in RDR Online.

## Failure behavior

The build fails instead of uploading a fake artifact if the official SDK cannot be downloaded, expected SDK headers/import library are absent, MSBuild fails, or `Nightwalker.asi` is missing/empty/not a Windows PE image.
