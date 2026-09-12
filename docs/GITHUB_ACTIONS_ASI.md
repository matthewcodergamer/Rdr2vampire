# Build Nightwalker ASI with GitHub Actions

Nightwalker has one dedicated manual workflow named **Build Nightwalker ASI**. It is separate from **Nightwalker CI**.

The ASI workflow acquires the Script Hook RDR2 developer SDK on the temporary GitHub Windows runner, builds `Release | x64`, verifies that `Nightwalker.asi` is a Windows PE file, and uploads an install-ready artifact. You do not need Visual Studio on your phone and you do not need to configure a repository variable for the normal case.

## iPhone / mobile steps

1. Open the Nightwalker repository on GitHub.
2. Tap **Actions**.
3. If GitHub shows **All workflows**, open it and choose **Build Nightwalker ASI**.
4. Tap **Run workflow**.
5. Use the branch selector in GitHub's Run workflow panel. Choose **main** for the current repository build, or choose another branch when you intentionally want to test that branch.
6. Tap the green **Run workflow** button.
7. Open the new run named **Build Nightwalker ASI** and wait until the `Build Nightwalker ASI` job is green.
8. Scroll to **Artifacts**.
9. Download **Nightwalker-ASI**.
10. Unzip the artifact, then unzip `Nightwalker-Windows-x64.zip` inside it for the install-ready package.

The install-ready package contains `Nightwalker.asi`, `Nightwalker.ini`, `Nightwalker.dialogue` when that source branch provides it, the project documentation shipped by the repository, and a SHA-256 checksum for the ASI.

## No hidden SDK setup required

The workflow first tries the Script Hook RDR2 developer SDK URL published from `dev-c.com` (or the optional `SCRIPHOOK_SDK_URL` repository-variable override). It verifies the downloaded file is actually a ZIP before extracting it.

The current official download endpoint can return an HTML page to GitHub-hosted runners. When that happens, the workflow automatically falls back to a **pinned commit** of the public `VideoTechUK/DirectorsSuite` repository, whose `inc/` and `lib/` directories contain the Script Hook RDR2 SDK headers/import library and credit them to Alexander Blade.

The fallback is used only as a build-time dependency. Nightwalker does not commit those SDK files and does not include them in the downloadable Nightwalker artifact.

## What each Actions workflow is for

- **Build Nightwalker ASI** — manually creates the downloadable Windows x64 `Nightwalker.asi` package.
- **Nightwalker CI** — automatically runs deterministic tests, compiler regressions, policy checks, and native-boundary checks. It is not the download build.

If you are trying to get the `.asi` on your phone, use **Build Nightwalker ASI**, not **Nightwalker CI**.
