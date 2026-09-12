# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 5: targeted Shadowstep and enemy vampire teleport combat.** The existing Saint Denis vampire model (`cs_vampire`, hash `0xD95BCB7D`) is now the primary Shadowstep combat actor in the debug encounter.

The intended combat read is now implemented in code: the vampire observes the player, approaches in ordinary RDR2 combat, evaluates safe intercept/flank/behind destinations, briefly vanishes with the Phase 4 smoke/visibility presentation, teleports to a validated point, reappears, performs a short collision-safe arrival carry, gives a readable telegraph, and only then receives a normal RDR2 combat task. Nightwalker does **not** apply damage on the teleport frame.

When the player retreats, the planner uses conservative recent velocity to prefer an intercept/flank point near the player's short predicted position. If all candidates are unsafe, the vampire stays in ordinary combat instead of forcing a teleport. Close-range evade Shadowsteps are rate-limited separately so the vampire cannot become continuously untouchable.

The earlier player-side F7 Shadowstep remains available as a **debug/safety harness** and now supports target-relative candidate planning when the player explicitly aims at a hostile ped. It is not the primary fantasy. `docs/DESIGN_LOCKS.md` is authoritative: the enemy/boss vampire owns the signature mechanic first.

There is still **no boss health bar implementation, feeding system, supernatural sprint, bespoke vampire attack animation set, boss phases, power-reveal UI, or player power HUD** in this phase.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables include:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests.
- `Nightwalker.Presentation.Tests` — disappearance/carry presentation-setting tests.
- `Nightwalker.Targeting.Tests` — target-relative candidate planning, unsafe fallback and Vampire AI config/clamping tests.

GitHub Actions runs the deterministic tests on Windows/MSBuild and Linux/g++. Linux CI also syntax-compiles the Phase 5 controllers and the presentation/combat native boundaries against test-only signature fixtures. CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local dependency and is not committed to the repository.

See `docs/BUILDING.md` for local dependency layout and the Phase 5 Story Mode verification checklist.

## Debug encounter controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` and set `[Debug] Enabled=true`.

- **F7** — player-side Shadowstep safety/targeting harness.
- **F8** — spawn one Nightwalker-owned `cs_vampire` debug ped. With `[VampireAI] Enabled=true`, this ped becomes the Phase 5 vampire combat actor.
- **F9** — despawn only the Nightwalker-owned debug vampire.
- **F10** — reload configuration/presentation/AI tuning.
- **F11** — cancel systems and restore Nightwalker-owned temporary state.

Default vampire AI tuning is conservative: 4–10 m Shadowstep decision band, ~1.65 m desired striking range, 250 ms prediction, 2.4 s Shadowstep cooldown, 320 ms post-arrival telegraph, 850 ms recovery, and a 5 s evade cooldown. These values are internal and never appear as player-facing HUD.

## Vampire combat state ownership

`VampireAIController` owns the debug vampire combat sequence:

`Observe -> Approach -> Decide -> ShadowstepDepart -> HiddenTransit -> ShadowstepArrive -> Telegraph -> Attack -> Recover -> Cooldown`

`Evade`, `Reposition`, `FeedAttempt`, and `Abort` are explicit states; only Evade has Phase 5 behavior, while Reposition/FeedAttempt are reserved seams for later phases.

`TargetedShadowstepPlanner` generates intercept, left-flank, right-flank and behind candidates. `ShadowstepResolver` remains the single geometry-safety authority for player and vampire teleports, so AI does not duplicate collision/ground/water/headroom logic.

`GameCombatApi` centralizes Phase 5 combat-facing native calls: aimed-ped lookup, velocity, combat-state queries, stand-still telegraph tasks, normal combat tasks and owned-task cleanup. `GamePresentationApi` remains responsible for visibility/alpha restoration and best-effort compact smoke.

## Safety and fairness

- Vampire teleport destinations must pass the existing geometry/ground/water/clearance resolver.
- The vampire never teleports inside the player capsule.
- Teleport does not deal direct damage.
- Arrival includes a readable telegraph before RDR2 combat resumes.
- Shadowstep and evade have separate internal cooldowns.
- Unsafe alleys/rooms fall back to ordinary movement/combat.
- Player death, vampire death/despawn, cutscene/mission transition, F11 cleanup, config disable and script shutdown restore vampire appearance and clear Nightwalker-owned AI tasks.
- No custom power/cooldown UI is added.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. The only planned custom combat HUD is the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
