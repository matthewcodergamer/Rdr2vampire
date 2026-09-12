# Building

Use Visual Studio 2022 with the Desktop C++ workload and a Windows SDK. Build `Nightwalker.sln` as `Debug | x64` or `Release | x64`.

The external Script Hook RDR2 developer SDK is not stored in this repository. Put `main.h`, `natives.h`, and the SDK headers under `third_party/ScriptHookRDR2/inc`, and `ScriptHookRDR2.lib` under `third_party/ScriptHookRDR2/lib`, or override `ScriptHookRdr2Root`.

Expected Release plugin output: `bin/Release/Nightwalker.asi`.

SDK-independent test targets now include nine suites, with `Nightwalker.Encounter.Tests` covering encounter ownership and `Nightwalker.BossHud.Tests` covering Phase 10 fade/re-engage/death/smoothing/layout/config behavior.

## GitHub Actions

`.github/workflows/ci.yml` runs on pushes and pull requests targeting `main`:

- Windows/MSBuild Release x64 builds and runs all nine SDK-independent suites.
- Linux/g++ builds/runs the same deterministic logic.
- Linux syntax-compiles gameplay, encounter, HUD and Runtime composition.
- Test-only native fixtures compile `GameBossBarApi` with the existing native boundaries.

CI intentionally does **not** link `Nightwalker.asi`; a genuine plugin requires the developer-local Script Hook RDR2 SDK and Windows/RDR2 Story Mode.

## Phase 10 Story Mode verification

1. Build `Release | x64` with the official/local Script Hook RDR2 developer SDK and install `Nightwalker.asi` plus `Nightwalker.ini`.
2. Keep `[Debug] Enabled=false`, `[Encounter.SaintDenis] Enabled=true`, `[VampireAI] Enabled=true`, and `[BossHUD] Enabled=true`.
3. Approach the configured Saint Denis church district during the active night window. Omen/Stalking must show **no boss bar**.
4. Enter Confrontation and wait for the readable combat handoff. When Combat is armed, confirm the bar fades in near the lower safe area with title `THE VAMPIRE` and no ability/phase text.
5. Damage the vampire repeatedly. Confirm the actual health loss is reflected while the visible fill eases smoothly instead of snapping or lagging materially behind state.
6. Let the vampire damage the player. Confirm the visibility timer refreshes.
7. Stay in active close combat without exchanging damage for several seconds. Confirm confirmed engagement keeps the meter visible.
8. Break combat/contact for longer than `IdleSeconds` (default 6.0). Confirm a smooth fade-out rather than an instant hide.
9. Re-engage after the bar has faded or while it is fading. Confirm it returns smoothly using the **current** boss health, not a stale ratio.
10. Set `ShowNumericHealth=false` (default) and verify no numbers appear. Set it true explicitly, reload safely with F10, start a fresh encounter, and verify only current/max boss HP is added; no powers/phases/cooldowns appear.
11. Change `DisplayName` and verify the configured title is used without creating another HUD element.
12. Kill the boss while the meter is visible. Confirm fill reaches zero, remains briefly for `DeathHoldSeconds` (default 1.25), then fades away without victory statistics or loot cards.
13. Abort by leaving the encounter area beyond its grace period. The bar must hide immediately and no UI may remain after actor cleanup.
14. Test player death, boss invalidation/despawn, mission/cutscene/player-control transition, F10 reload, F11 cleanup and normal script unload. Every path must hide the HUD immediately.
15. Disable `[BossHUD] Enabled=false`. The encounter/combat should continue normally with **no custom HUD**.
16. Verify at 16:9 and an ultrawide resolution. The bar should remain centered with a restrained width and lower-screen placement rather than stretching across the display.
17. Confirm no permanent player health replacement, blood/hunger meter, stamina replacement, Shadowstep cooldown, icon row, move list, phase label, power name, weakness/resistance, floating damage number or combo counter appears anywhere.
18. Run several encounter start -> disengage -> re-engage -> boss death cycles and abort/restart cycles. No stale boss handle or stuck bar may survive into the next encounter.

## Current boundaries

Phase 10 uses normalized native rectangle/text drawing and aspect-aware width compensation. Exact visual safe-zone placement still requires target-environment verification across user HUD/safe-zone settings.

The HUD receives its boss handle only from `SaintDenisDirector`; it never scans the world. Damage/contact queries are read-only for the HUD and do not clear vanilla or Nightwalker combat state.

The encounter completion/cooldown remains session-owned. Nightwalker still does not write to RDR2 save structures.
