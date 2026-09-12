# Build Nightwalker ASI with GitHub Actions

Nightwalker now has a dedicated manual GitHub Actions workflow named **Build Nightwalker ASI**.

This workflow is separate from the normal CI/test workflow. Its purpose is to produce the real Windows x64 `Nightwalker.asi` file using MSBuild and the Script Hook RDR2 developer SDK.

## From iPhone or any browser

1. Open the Nightwalker GitHub repository.
2. Tap **Actions**.
3. In the workflow list, tap **Build Nightwalker ASI**.
4. Tap **Run workflow**.
5. Leave `source_ref` as `main` to build the current released repository state, or enter a feature branch such as `phase-12-narrative-scaffolding` when testing an unmerged phase.
6. Leave the developer SDK URL at its default unless the official SDK download URL changes.
7. Tap **Run workflow**.
8. Open the new workflow run and wait for **Build Nightwalker ASI** to finish successfully.
9. Scroll to **Artifacts** and download **Nightwalker-ASI**.
10. Unzip the artifact. It contains `Nightwalker.asi`, `Nightwalker.ini`, the SHA-256 file, and any runtime content file present in the source ref such as `Nightwalker.dialogue`.

## What the workflow does

The Windows runner:

- checks out the requested branch/tag/commit;
- downloads the Script Hook RDR2 developer SDK from the configured HTTPS URL;
- stages the SDK headers and `ScriptHookRDR2.lib` under the repository's expected local SDK layout;
- runs `MSBuild` for `Release | x64`;
- verifies that `bin/Release/Nightwalker.asi` exists;
- packages the ASI plus Nightwalker configuration/content files;
- uploads the result as the **Nightwalker-ASI** artifact.

## SDK source

The default workflow URL points to the developer SDK download published from Alexander Blade's official Script Hook RDR2 page on `dev-c.com`. The repository does not commit or redistribute the SDK itself.

If the official SDK URL changes, paste the new official HTTPS SDK ZIP URL into the workflow's `sdk_url` field before running it.

## Important distinction

The normal **Nightwalker CI** action validates deterministic tests and native/API compile boundaries. It intentionally does not produce the final ASI.

Use **Build Nightwalker ASI** when you specifically want a downloadable plugin artifact.
