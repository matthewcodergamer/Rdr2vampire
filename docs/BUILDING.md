# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

SDK-independent test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep vector math and destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry setting logic.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning and Vampire AI tuning.
- `Nightwalker.Movement.Tests` — continuous-movement ramp math and config bounds.
- `Nightwalker.Feeding.Tests` — hidden-resource, feed geometry and backwards-compatible feeding config.
- `Nightwalker.Combat.Tests` — bounded physical-release vector math plus Phase 8 combat config/alias clamping.

Run test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all seven SDK-independent tests.
- Linux/g++ C++20 builds and runs the same deterministic logic.
- Linux syntax-compiles Shadowstep, Vampire AI, Movement, Feeding and Phase 8 combat controllers.
- Test-only native signature fixtures compile presentation/combat/movement/feeding/physical native boundaries without redistributing Script Hook files.

CI intentionally does **not** link `Nightwalker.asi`. The final plugin still requires the developer-local Script Hook RDR2 SDK and a Windows/RDR2 environment.

## Phase 8 in-game verification

1. Build `Release | x64` with the Script Hook RDR2 developer SDK available locally.
2. Install `Nightwalker.asi`, copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini`, and set `[Debug] Enabled=true`, `[Combat] Enabled=true`, `[VampireAI] Enabled=true`.
3. Launch **Story Mode only**. Confirm `Nightwalker.log` reports the Phase 8 runtime initialization line and no move/cooldown/resource HUD appears.
4. Free-aim a normal close ambient human and press **F1**. The player debug harness should enter the heavy-strike flow. A whiff must not receive the scripted strike bonus; the bonus is allowed only after RDR2 reports actual actor-to-target contact.
5. Press **F2** on another valid close human. Confirm the target aligns into a short grab/control approximation and is released after the short configured hold. No victim may remain task-locked.
6. Repeat F2, then press **F3 while the controller is in Hold**. Nightwalker must clear its participant tasks before ragdoll. The target should receive only a bounded physical release.
7. Test **F3 directly** from Idle. It should perform the same align -> short hold -> release sequence.
8. Put a solid wall/large prop directly behind the target and repeat the release. The projected segment should reject/suppress the launch impulse when blocked; ragdoll without a through-wall launch is acceptable.
9. Repeat the wall test where geometry tracing cannot return a conclusive answer. Nightwalker should conservatively suppress the impulse rather than guessing the path is clear.
10. Test release on stairs, slopes, alleys and tighter interiors. The impulse must remain modest and must not fling peds at absurd speed or distance.
11. Repeat F2, then press **F4 during Hold**. Confirm the short grab transitions into combat feed and then clears participant tasks.
12. For the direct player-debug combat-feed path, first ragdoll/stagger an ambient human through normal gameplay, free-aim the target, and press **F4**. A standing compatible target should not satisfy this direct stagger requirement.
13. Confirm player-debug combat feed transfers only the configured bounded health/resource amount and the internal resource remains invisible as HUD.
14. Spawn the owned `cs_vampire` with **F8** and enter combat. At Shadowstep range, confirm the existing vanish -> safe relocation -> reappear/carry -> telegraph sequence still occurs. Only after the readable tell may the Phase 8 follow-up strike begin; there must be no damage on the teleport frame.
15. Stay close to the boss long enough to exercise several close-range special opportunities. The sequence should rotate deterministically through heavy strike, grab/release and grab/feed with `BossSpecialCooldownMs` between opportunities instead of firing every frame.
16. Confirm the boss's scripted combat-feed health reduction cannot itself reduce the player below 1 health. Ordinary RDR2 combat may still produce normal lethal outcomes.
17. Mix normal RDR2 melee, Shadowsteps, special grabs, releases and feeds for at least **five uninterrupted minutes**. No task ownership, movement-rate override or player controls may remain corrupted between moves.
18. Press **F9** during Telegraph, Align, Hold, Strike, Release and Feed in separate tests. The boss/debug entity should despawn only after transient Phase 8 state is cancelled.
19. Press **F10** during each active Phase 8 state. Combat must cancel before configuration replacement; the next request should use reloaded/clamped values.
20. Press **F11** during every active state. All Nightwalker-owned tasks/motion overrides must restore, while vanilla world state not owned by Nightwalker remains untouched.
21. Test player death, boss death and mission/cutscene/player-control transitions during active Phase 8 moves. Runtime cancellation must return all mod-owned systems to safe idle state.
22. Set `[Combat] Enabled=false` and reload. F1-F4 combat requests and boss close specials should stop while older independent systems remain governed by their own feature flags.
23. Test extreme Combat values in the INI and reload. Verify clamping warnings rather than unsafe force/timing values.
24. Confirm throughout that there is no player blood meter, Shadowstep cooldown meter, move list, skill wheel, combo counter, boss power name or phase label.
25. Exit/unload normally and confirm orderly shutdown logs `Nightwalker shutdown complete.` when Script Hook supplies a normal unload path.

## Phase 8 boundaries

Phase 8 deliberately does not invent claw animation dictionaries, custom melee-style hashes, a perfect throat-lift animation or a custom neck-bite animation. The short grapple is a verified Rockstar-task approximation and is intentionally bounded because the generic grapple can become lethal if left running.

The physical release uses verified ragdoll and center-of-mass force natives behind `GamePhysicalApi`, plus the existing world raycast before any launch impulse. No attachment is created in Phase 8, so there is no attachment state to leak.

Fear behavior and passive regeneration are optional ideas from the phase brief and are deferred until their own civilian-query and resource/damage ownership policies can be implemented and tested without broadening this combat slice.
