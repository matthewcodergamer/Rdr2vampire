# Build Nightwalker ASI with GitHub Actions

Nightwalker has one dedicated manual workflow named **Build Nightwalker ASI**. It is separate from **Nightwalker CI**.

The ASI workflow downloads the official Script Hook RDR2 developer SDK for the temporary GitHub Windows runner, builds `Release | x64`, verifies that `Nightwalker.asi` is a Windows PE file, and uploads an install-ready artifact. You do not need Visual Studio on your phone and you do not need to configure a repository variable for the normal case.

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

The workflow uses the official Script Hook RDR2 developer SDK download from `dev-c.com` by default.

An optional repository variable named `SCRIPHOOK_SDK_URL` can override the SDK download URL later if Alexander Blade changes the official SDK package location. Leaving that variable unset is now supported and is the normal setup.

The SDK is downloaded only to the temporary GitHub runner. It is not committed to the Nightwalker repository or included in the Nightwalker artifact.

## What each Actions workflow is for

- **Build Nightwalker ASI** — manually creates the downloadable Windows x64 `Nightwalker.asi` package.
- **Nightwalker CI** — automatically runs deterministic tests, compiler regressions, policy checks, and native-boundary checks. It is not the download build.

If you are trying to get the `.asi` on your phone, use **Build Nightwalker ASI**, not **Nightwalker CI**.
