# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. It references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 13: long-session hardening and compatibility.** The existing Saint Denis encounter, original narrative, persistence, temporary red boss-health bar, boss Shadowstep AI, movement, feeding and physical combat remain intact.

Phase 13 adds a pure `LongSessionGuard` around the established controller ownership graph. Player death, mission/control transitions, long script stalls, player-ped replacement and large world-position discontinuities converge on the same reverse-order cleanup path, followed by a short stable-state quarantine before gameplay can resume. This is intentionally conservative for save/load, fast travel and mod-conflict recovery.

The authoritative boss registry is now revalidated at a throttled 4 Hz without scanning the ped pool. A missing or handle-reused boss cancels Nightwalker ownership before the registry is cleared. Boss death itself still follows the normal encounter/death-hold path.

Model streaming reuses an already loaded model instead of issuing another request. Shadow smoke remains non-looped and best-effort; failed particle playback no longer starts an asset-request loop. Optional fixed-slot runtime profiling can be enabled only in debug mode with `ProfileRuntime=true` and writes 10-second update/average/max timing summaries to `Nightwalker.log`.

There is still **no** player blood/hunger meter, player-health replacement, stamina replacement, Shadowstep cooldown bar, skill wheel, ability card, move list, boss phase text, power name, weakness panel, floating damage number, combo counter, or status-icon row. The temporary red Saint Denis boss-health bar remains the only approved custom combat HUD. CI includes a source-boundary regression check so gameplay code cannot directly introduce a second custom draw surface.

See `docs/HARDENING.md` for the Phase 13 recovery matrix, performance audit, compatibility risks and target-environment verification checklist.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`. Local builds copy `content/Nightwalker.dialogue` beside the ASI when the source file is present.

SDK-independent test executables include eleven suites:

- `Nightwalker.Tests`
- `Nightwalker.Shadowstep.Tests`
- `Nightwalker.Presentation.Tests`
- `Nightwalker.Targeting.Tests`
- `Nightwalker.Movement.Tests`
- `Nightwalker.Feeding.Tests`
- `Nightwalker.Combat.Tests`
- `Nightwalker.Encounter.Tests`
- `Nightwalker.BossHud.Tests`
- `Nightwalker.SaveData.Tests`
- `Nightwalker.Narrative.Tests`

The foundation suite now also covers long-session recovery/quarantine semantics and cached model requests. GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++, syntax-compiles controllers and Runtime composition, compiles the native boundaries against stubs, and enforces the custom-HUD draw allowlist. Public CI intentionally does not link the final ASI because that requires the developer-local SDK and RDR2 Story Mode environment.

See `docs/NARRATIVE.md`, `docs/PERSISTENCE.md`, `docs/BUILDING.md`, `docs/ENCOUNTER.md`, `docs/BOSS_HEALTH_BAR.md`, and `docs/HARDENING.md`.

## Debug/runtime profiling

Set `[Debug] Enabled=true` only for development harness actions. Production encounter, narrative, boss bar, persistence and long-session recovery do not require Debug mode.

```ini
[Debug]
Enabled=false
ProfileRuntime=false
```

When both values are true, `ProfileRuntime` writes fixed-slot per-system update counts plus average/max update time every 10 seconds. It never draws a profiler HUD.

## Debug controls

- **F1** — heavy strike debug harness.
- **F2** — short grab/control debug harness.
- **F3** — physical release/throw follow-up.
- **F4** — combat feed follow-up/direct stagger test.
- **F5/F6** — Sip/Drain feed tests.
- **F7** — player-side Shadowstep safety harness.
- **F8** — spawn a debug `cs_vampire` only when the authoritative boss registry is free.
- **F9** — despawn only a debug-owned vampire; ignored during the real encounter.
- **F10** — clean transient state, checkpoint progression, reload INI/dialogue data, reapply saved tuning, and reset Phase 13 runtime guards.
- **F11** — global cleanup.

## Ownership boundaries

- `LongSessionGuard` owns pure transition/discontinuity policy and never calls a native.
- `SaintDenisDirector` owns encounter timing and narrative event hooks.
- `BossActorRegistry` remains the single cross-system boss reference; Runtime only validates that cached reference at a throttled cadence.
- `NarrativeController` owns one temporary narrative sequence.
- `GameBossBarApi` remains the only low-level custom rectangle/background-text drawing surface.
- `SaveData` / `ProgressionController` own mod state and never patch RDR2 save files.
- Gameplay controllers restore only the temporary visibility/movement/task/presentation state they own.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text. Temporary narrative subtitles do not authorize a player power HUD. The temporary red Saint Denis boss-health bar remains Nightwalker's **only** custom combat HUD element.
