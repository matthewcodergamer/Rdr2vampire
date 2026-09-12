# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. It references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 10: temporary Saint Denis boss-health bar.** Phase 9's nighttime encounter now explicitly owns the one approved custom combat HUD element: a restrained red health meter for its `cs_vampire` boss. No world scan guesses which ped is a boss; `SaintDenisDirector` passes its owned handle directly to `BossHudController` only when Confrontation becomes Combat.

The encounter flow remains:

`Dormant -> Eligible -> Omen -> SpawnPending -> Stalking -> Confrontation -> Combat -> Resolution -> Cleanup -> Cooldown`

with `Abort -> Cleanup` for unsafe exits. Existing Shadowstep, vampire AI, supernatural sprint, feeding, grab/throw and physical melee systems remain the boss combat implementation.

The HUD flow is:

`Hidden -> FadeIn -> Visible -> FadeOut -> Hidden`

and boss death uses:

`Visible/FadeIn/FadeOut -> DeathHold -> FadeOut -> Hidden`.

Starting/stalking the encounter does not pin a bar onscreen. Entering combat creates the explicit boss HUD ownership and registers activity. Boss damage, boss-caused player damage, or confirmed close combat refreshes the idle timer. After the configured quiet period the bar fades out; re-engagement fades it back in with the current authoritative health ratio.

Health is read from RDR2 through the existing health native boundary and clamped to `0..1`. The displayed fill eases toward the authoritative value for presentation while the internal actual ratio remains exact. Boss death drives the authoritative ratio to zero, holds the empty bar briefly, then fades away. Encounter abort, invalid/despawned boss, player death/unsafe Story Mode transition, F10/F11 cleanup, configuration disable, and plugin shutdown hide it immediately.

Presentation deliberately stays narrow: thin lower-screen meter, deep blood-red fill, dark translucent backing, warm-gray title, normalized coordinates and aspect-aware width compensation. The default title is `THE VAMPIRE`. Numeric health is **off by default** and appears only if `ShowNumericHealth=true` is explicitly configured.

There is still **no** player blood/hunger meter, custom player-health replacement, stamina replacement, Shadowstep cooldown bar, skill wheel, ability card, move list, boss phase text, power name, weakness panel, floating damage number, combo counter, or status-icon row.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables now include nine suites:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regressions.
- `Nightwalker.Shadowstep.Tests` — Shadowstep destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry settings.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning.
- `Nightwalker.Movement.Tests` — controlled supernatural movement.
- `Nightwalker.Feeding.Tests` — feeding/resource rules.
- `Nightwalker.Combat.Tests` — physical-combat math/config.
- `Nightwalker.Encounter.Tests` — Phase 9 encounter math/registry/settings.
- `Nightwalker.BossHud.Tests` — Phase 10 fade/re-engage/death/smoothing/layout/config behavior.

GitHub Actions builds/runs the deterministic tests on Windows/MSBuild and Linux/g++. Linux also syntax-compiles the gameplay/runtime controllers and test-only native signature fixtures, including the Phase 10 drawing boundary. Public CI intentionally does not link `Nightwalker.asi`; the final plugin requires the developer-local Script Hook RDR2 SDK and a Windows/RDR2 Story Mode environment.

See `docs/BUILDING.md`, `docs/ENCOUNTER.md`, and `docs/BOSS_HEALTH_BAR.md` for target-environment checks and ownership rules.

## Boss HUD defaults

```ini
[BossHUD]
Enabled=true
DisplayName=THE VAMPIRE
IdleSeconds=6.0
FadeSeconds=0.35
DeathHoldSeconds=1.25
ShowNumericHealth=false
```

## Debug controls

Set `[Debug] Enabled=true` only for development harness actions. The production encounter and boss bar do not require Debug mode.

- **F1** — heavy strike debug harness.
- **F2** — short grab/control debug harness.
- **F3** — physical release/throw follow-up.
- **F4** — combat feed follow-up/direct stagger test.
- **F5/F6** — Sip/Drain feed tests.
- **F7** — player-side Shadowstep safety harness.
- **F8** — spawn a debug `cs_vampire` only when the authoritative boss registry is free.
- **F9** — despawn only a debug-owned vampire; ignored during the real encounter.
- **F10** — clean encounter/HUD/transient state then reload config.
- **F11** — global cleanup; active encounter and boss HUD are removed safely.

## Native boundaries

- `GameApi` — entity/geometry/model operations.
- `GameCombatApi` — combat state and ordinary combat tasks.
- `GameEncounterApi` — clock/game-time/camera-visibility queries.
- `GamePresentationApi` — compact smoke and visibility.
- `GameMovementApi`, `GameFeedingApi`, `GamePhysicalApi` — existing movement/feed/physics boundaries.
- `GameBossBarApi` — Phase 10 normalized rectangle/text drawing and screen-resolution query only.

`BossHudModel` contains the testable state/timing/layout logic. `BossHudController` owns only the explicitly supplied encounter boss handle; it never scans the ped pool.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text. The temporary red Saint Denis boss-health bar implemented in Phase 10 is the **only custom combat HUD element** approved for Nightwalker.
