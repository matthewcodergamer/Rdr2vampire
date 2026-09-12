# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

SDK-independent test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry settings.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning and Vampire AI tuning.
- `Nightwalker.Movement.Tests` — continuous-movement ramp math and config bounds.
- `Nightwalker.Feeding.Tests` — hidden-resource/feed geometry/config compatibility.
- `Nightwalker.Combat.Tests` — bounded physical-release math and combat config.
- `Nightwalker.Encounter.Tests` — night-window/radius/cooldown math, authoritative boss ownership, and Saint Denis setting clamps.

Run test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all eight SDK-independent tests.
- Linux/g++ C++20 builds and runs the same deterministic logic.
- Linux syntax-compiles the Shadowstep, vampire AI, movement, feeding, physical-combat, debug-spawn and Saint Denis encounter controllers.
- Linux syntax-compiles Runtime composition so constructor/system-order changes are checked.
- Test-only native signature fixtures compile `GameEncounterApi` alongside the existing presentation/combat/movement/feed/physics boundaries.

CI intentionally does **not** link `Nightwalker.asi`. A genuine plugin build still requires the developer-local Script Hook RDR2 SDK and Windows/RDR2.

## Phase 9 in-game verification

1. Build `Release | x64` with the local Script Hook RDR2 developer SDK, install `Nightwalker.asi`, and copy the example INI as `Nightwalker.ini`.
2. Leave `[Debug] Enabled=false`. Keep `[Encounter.SaintDenis] Enabled=true` and `[VampireAI] Enabled=true` to prove the production encounter does not depend on the debug harness.
3. Set RDR2 time inside the configured `StartHour`/`EndHour` window and approach the configured Saint Denis church district. Confirm the encounter remains dormant outside `TriggerRadius` and becomes eligible inside it.
4. Confirm the omen is restrained: compact smoke only, no control/camera lock, no custom power HUD, and no permanent effects.
5. During `SpawnPending`, move/look around. The vampire should appear only at a safe ground/water-valid point, prefer an off-camera candidate, and never create a duplicate actor.
6. During Stalking, confirm the vampire exists but does not run the Phase 5–8 combat AI yet. Aim at him, approach inside `ConfrontationDistance`, or allow the stalking timer to expire to enter Confrontation.
7. Confirm Confrontation provides its short readable delay before combat is armed. There must be no scripted damage during this handoff.
8. Fight normally. Existing Shadowstep, arrival carry/telegraph, supernatural chase movement, heavy melee, grab/throw, and combat feed should operate on the encounter-owned actor.
9. Move outside `AbortRadius`, return before `LeaveGraceMs`, and verify the encounter remains active. Then leave longer than the grace period and verify the boss is disarmed, tasks/presentation are restored, and the actor is removed.
10. Start a major mission/cutscene or otherwise lose player control during Omen, SpawnPending, Stalking, Confrontation, and Combat in separate tests. Runtime unsafe-state cleanup must remove the encounter actor before systems resume.
11. Test player death during active encounter states. No boss, movement override, invisible state, model request, or smoke ownership may be stranded.
12. Kill the boss. Confirm combat is disarmed immediately, Resolution holds briefly, Cleanup removes the owned actor, then the long `RespawnCooldownHours` session cooldown begins.
13. Temporarily shorten `RespawnCooldownHours` and `AbortCooldownMinutes` to practical test values. Verify the encounter cannot restart early, then can restart after the appropriate in-game-time cooldown.
14. Press F8 while the real encounter owns the registry. No debug duplicate should spawn. Press F9; the real encounter boss must not be deleted.
15. Press F11 during an active encounter. Global cleanup should abort the encounter and delete only Nightwalker-owned encounter state.
16. Press F10 during an active encounter. Combat/AI must clean first, the encounter actor must be removed, then the new config should load from a safe state.
17. Set `[Encounter.SaintDenis] Enabled=false` and reload. The real encounter should remain inactive while F8 debug testing still obeys `[Debug]`.
18. Move `CenterX/Y/Z` or radii to intentionally bad/extreme values and reload. Verify clamping warnings rather than unsafe ranges.
19. Repeat successful start -> fight -> death resolution and start -> abort cycles several times. There must never be more than one Nightwalker boss or a stranded `cs_vampire` from the previous cycle.
20. Confirm Phase 9 adds no boss-health bar yet and exposes no power names, phase labels, cooldowns, blood meter, skill wheel, or move list.
21. Exit/unload normally and confirm `Nightwalker shutdown complete.` when Script Hook supplies a normal unload path.

## Phase 9 boundaries

The encounter's completion and cooldown state are session-owned in Phase 9. Nightwalker does not write into RDR2 save structures; persistent mod-owned save data remains a later phase.

Omen V1 intentionally ships without guessed bell/scream audio, spawned corpse/blood clues, or permanent bat swarms. `docs/ENCOUNTER.md` documents the ownership and cleanup contract in detail.

The temporary red boss-health bar described in `docs/BOSS_HEALTH_BAR.md` is also intentionally deferred. Phase 9 provides the authoritative boss handle/state that the later HUD controller will consume.
