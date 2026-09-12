# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the SDK headers under `third_party/ScriptHookRDR2/inc`, and `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override `ScriptHookRdr2Root`.

Expected Release plugin output: `bin/Release/Nightwalker.asi`.

SDK-independent test targets now include **ten** suites. `Nightwalker.SaveData.Tests` covers schema round-trip/migration, value clamps, tuning application, temp/replace writes, replacement, and corrupt-primary backup recovery.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all ten SDK-independent suites.
- Linux/g++ builds/runs the same deterministic logic.
- Linux syntax-compiles gameplay, encounter, HUD, `ProgressionController`, and Runtime composition.
- Test-only native fixtures continue compiling the verified game boundaries.

CI intentionally does **not** link `Nightwalker.asi`; a genuine plugin requires the developer-local Script Hook RDR2 SDK and Windows/RDR2 Story Mode.

## Phase 11 Story Mode persistence verification

1. Build `Release | x64` with the official/local Script Hook RDR2 developer SDK and install `Nightwalker.asi` plus `Nightwalker.ini`.
2. Start with no `Nightwalker.state`, `.tmp`, or `.bak`. Launch Story Mode and confirm the log reports safe default state rather than an error/crash.
3. Complete a normal feed so hidden blood changes. Allow several seconds for the throttled checkpoint or exit normally. Confirm `Nightwalker.state` is created beside the plugin and contains `schemaVersion=1`.
4. Exit RDR2 completely, relaunch, and confirm the stored blood value is loaded as the feeding resource seed. No blood/hunger meter should appear.
5. Resolve the Saint Denis vampire encounter. Confirm the state file records `encounter.saintDenis.completed=true` and a non-zero absolute `cooldownUntilGameSeconds`.
6. Exit and restart before that cooldown expires. Enter the Saint Denis district during the normal night window and verify the encounter remains unavailable until the stored timestamp expires.
7. Temporarily use practical encounter cooldown values for testing, let the stored timestamp expire, and verify the user's `[Encounter.SaintDenis] Enabled` preference becomes effective again.
8. Abort an active encounter through the existing leave-grace or unsafe-transition path. Confirm reverse lifecycle cleanup occurs before the persistence checkpoint, so the resulting abort cooldown is stored.
9. Verify the already-fixed Dormant cancellation edge: load during an unsafe mission/cutscene with no encounter actor. A cancellation while `Dormant` must not create a fresh abort cooldown.
10. Trigger F10 during/after an encounter. Confirm active gameplay cleans first, progression state checkpoints, the INI reloads, and saved tuning is reapplied once without multiplying itself again.
11. Change safe progression multipliers in `Nightwalker.state` **while RDR2 is closed**, restart, and verify player Shadowstep debug-harness range/cooldown and feed efficiency change within documented clamps.
12. Set extreme values in the state file and restart. Confirm clamp warnings/default behavior and no unsafe multiplier is accepted.
13. Keep a valid `Nightwalker.state.bak`, corrupt the primary state file, then launch. Confirm a warning and backup recovery rather than a crash.
14. Corrupt the primary with no valid backup. Confirm Nightwalker recovers to safe defaults and remains playable.
15. Set a future unsupported `schemaVersion`. Confirm runtime avoids destructive downgrade writes for that session.
16. Interrupt a write in a controlled development copy so a valid `.tmp`/`.bak` remains, then verify the recovery path on next launch.
17. Run feed -> checkpoint -> restart and encounter abort/resolution -> checkpoint -> restart cycles repeatedly. No resource reset, duplicate boss, stranded actor, or stale cooldown should appear.
18. Confirm **no** skill tree, radial wheel, progression overlay, blood meter, Shadowstep cooldown display, ability card, or new custom combat HUD appears. The Phase 10 red boss-health bar remains the only custom combat HUD.

## Phase 10 boss-HUD regression checks

After Phase 11 integration, also repeat the high-value HUD cases: Omen/Stalking shows no bar; Combat fades in the red meter; inactivity fades it out; re-engagement restores current health; death reaches zero/holds/fades; abort/player death/F10/F11/unload hides immediately.

## Current persistence boundaries

`Nightwalker.state` is a Nightwalker-owned file. It never replaces or patches RDR2 save data. Phase 11 immediately applies saved tuning only to the player Shadowstep debug harness and feeding efficiency. Stored sprint/flank/regeneration/throw-strength progression is reserved for later explicitly approved player gameplay because current sprint/physical systems also affect the enemy vampire.

Editing `Nightwalker.state` while the game is running is unsupported; runtime checkpoints can overwrite external edits. F10 re-reads the INI and reapplies the state already loaded into memory. Re-reading the state file itself requires a restart in Phase 11.
