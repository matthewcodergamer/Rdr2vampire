# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the rest of the SDK headers under `third_party/ScriptHookRDR2/inc`, and put `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override the `ScriptHookRdr2Root` MSBuild property.

Expected plugin output: `bin/Release/Nightwalker.asi` for Release builds.

The solution also builds two pure C++ test targets:

- `Nightwalker.Tests` — config/timing/watchdog/model-streaming regression logic.
- `Nightwalker.Shadowstep.Tests` — Shadowstep V1 vector math and destination-safety logic.

Run both test executables from `bin/tests/<Configuration>/`. They do not require RDR2 or Script Hook.

For runtime setup, follow the official Script Hook RDR2 dependency documentation. This project is Story Mode only.

## Phase 3 in-game verification

1. Build `Release | x64` with the official Script Hook RDR2 developer SDK present.
2. Copy `Nightwalker.asi` into the RDR2 directory used by the Script Hook runtime.
3. Copy `config/Nightwalker.example.ini` beside it as `Nightwalker.ini` and set `[Debug] Enabled=true`.
4. Launch **Story Mode only** and stand in a normal open, dry outdoor area with player control.
5. Confirm `Nightwalker.log` reports `Phase 3 runtime initialized; Shadowstep V1 is debug-only and geometry-gated.`
6. Press **F7** once in open terrain. The player should relocate forward roughly `QuickDistance` metres with no VFX. Confirm debug logs show requested coordinates, resolved coordinates, final distance, shortening state, and state-transition elapsed times.
7. Wait for the configured cooldown and repeat several times in open terrain. The player must remain visible, collidable, controllable, and normally affected by the game after every step.
8. Face a wall from several metres away and press F7. The step should shorten and stop with clearance before the wall, or reject if the remaining safe distance is too small. It must never place the player through the wall.
9. Stand extremely close to a wall/large prop and press F7. Confirm rejection rather than clipping through geometry.
10. Test at steep drops, steep rises, roof/ledge edges, irregular ground, and stair-like terrain. If navmesh/ground/vertical validation is inconclusive or exceeds `MaxVerticalDelta`, the step must reject.
11. Test near shallow and deep water. Deep-water destinations must reject; the player must not be teleported into unsafe water.
12. Test under low ceilings and beside large props. Destination headroom/clearance must reject unsafe endpoints.
13. Trigger F7, then enter a mission/cutscene or other player-control transition before the sequence completes. Nightwalker must cancel the state machine and return safely to `Idle`.
14. During testing, press **F11**. Confirm Shadowstep cancels and the existing global cleanup path remains healthy.
15. Perform **100 consecutive successful** Shadowsteps in varied safe outdoor areas. Each success logs `consecutiveStressCounter=N/100`; any rejected/cancelled/failed attempt resets the counter. At 100, confirm the stress milestone is logged.
16. During that 100-step run, verify the player never falls through the world, remains permanently moved into geometry, becomes collisionless/invisible/invincible, loses controls, or accumulates a persistent camera/movement modification. Phase 3 does not deliberately mutate any of those states.
17. Confirm F8/F9 debug vampire spawn/despawn, F10 config reload, and F11 cleanup still work after repeated Shadowsteps.
18. Exit/unload normally and confirm `Nightwalker shutdown complete.` is logged when orderly Script Hook shutdown is available.

If post-relocation verification differs materially from the validated endpoint, Nightwalker attempts to roll the player back to the known start point and logs whether rollback could be confirmed.

No smoke, bats, disappearance effect, arrival slide/carry, combat targeting, attack buffering, boss AI, feeding, boss health bar, or other custom combat HUD should appear in Phase 3.
