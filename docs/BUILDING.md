# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the SDK headers under `third_party/ScriptHookRDR2/inc`, and `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override `ScriptHookRdr2Root`.

Expected Release plugin output: `bin/Release/Nightwalker.asi`. When `content/Nightwalker.dialogue` exists, the native project copies it beside the built ASI.

SDK-independent test targets now include **eleven** suites. `Nightwalker.Narrative.Tests` covers schema parsing, stable sequence presence, wrapping, playback/skip/watchdog behavior, and narrative-settings clamps.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all eleven SDK-independent suites.
- Linux/g++ builds/runs the same deterministic logic.
- Linux syntax-compiles gameplay, encounter, HUD, persistence, `NarrativeController`, and Runtime composition.
- Test-only native fixtures continue compiling the verified game boundaries.

CI intentionally does **not** link `Nightwalker.asi`; a genuine plugin requires the developer-local Script Hook RDR2 SDK and Windows/RDR2 Story Mode.

## Phase 12 Story Mode narrative verification

1. Build `Release | x64`, install `Nightwalker.asi`, `Nightwalker.ini`, and the copied `Nightwalker.dialogue` beside the plugin.
2. Keep `[Narrative] Enabled=true` and enter the Saint Denis encounter normally.
3. Reach Confrontation and verify the original pre-fight subtitle sequence appears while the boss is still non-combat.
4. Allow it to complete naturally and confirm Combat arms afterward with the existing boss AI/HUD behavior unchanged.
5. Start another encounter and press Enter once during the first line. Confirm only the current line advances. Hold Enter and confirm it does not repeatedly skip lines.
6. Temporarily shorten `MaxConfrontationHoldMs` and verify the watchdog cancels remaining text and still enters Combat.
7. Kill the boss and verify the bounded post-defeat subtitle appears before resolution cleanup.
8. Temporarily shorten `MaxSequenceMs`; confirm post-fight text cannot strand encounter cleanup.
9. Abort during pre-fight and post-fight playback by leaving the area, F11 cleanup, player death, and mission/cutscene transition. Subtitles must disappear immediately and actor/HUD cleanup must continue.
10. Remove `Nightwalker.dialogue`, relaunch, and verify the built-in original subtitle catalog is used.
11. Corrupt the external dialogue schema/records, relaunch, and verify warnings plus built-in fallback rather than a crash.
12. Leave `OptionalAudio=true`. The Phase 12 subtitle-only audio backend must quietly fall back to text and never delay encounter state.
13. Set `[Narrative] Enabled=false`. Encounter confrontation/combat/resolution must still work without narrative text.
14. F10 during an active sequence must clean the encounter/narrative first, reload INI/dialogue data, and leave no stale subtitle.
15. Confirm clue and alternate-outcome records remain data hooks only; Phase 12 does not spawn unsafe clue props or add a choice UI.
16. Confirm no player power HUD, dialogue wheel, blood meter, cooldown bar, boss phase label, or other persistent combat UI was added.

## Phase 11 persistence regression checks

Repeat the high-value persistence cases after Phase 12 integration: hidden blood survives restart; Saint Denis completion/cooldown survives restart; corrupt-primary backup recovery works; F10 does not compound tuning; Dormant cancellation does not create a false abort cooldown.

## Phase 10 boss-HUD regression checks

Omen/Stalking shows no bar; Combat fades in the red meter; inactivity fades it out; re-engagement restores current health; death reaches zero/holds/fades; abort/player death/F10/F11/unload hides immediately.

## Current boundaries

`Nightwalker.state` remains separate from RDR2 save data. Narrative text/audio IDs remain separate from gameplay code. Phase 12 ships no custom voice files and no guessed RDR2 audio identifiers; subtitles are the guaranteed production fallback.
