# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 9: Saint Denis nighttime boss encounter.** The existing safe Shadowstep, vampire AI, supernatural sprint, feeding, and expanded physical combat systems remain intact. Phase 9 adds the free-roam encounter lifecycle around the existing Saint Denis `cs_vampire` rather than creating another combat implementation.

The encounter director runs an explicit state flow:

`Dormant -> Eligible -> Omen -> SpawnPending -> Stalking -> Confrontation -> Combat -> Resolution -> Cleanup -> Cooldown`

with `Abort -> Cleanup` for unsafe exits.

Eligibility requires the configured midnight/pre-dawn time window, a living controllable Story Mode player, the player inside the configured Saint Denis church district, the encounter feature enabled, no other Nightwalker boss owner, and the session cooldown expired. Runtime mission/player-control safety remains authoritative; when Story Mode enters an incompatible mission/cutscene/world transition, the global lifecycle cancels the encounter and cleans its owned actor before gameplay systems resume.

Phase 9 introduces `BossActorRegistry` so there is exactly one authoritative Nightwalker boss actor. The encounter director owns its real encounter `cs_vampire`; F8 may claim the same registry only for a debug actor when no encounter boss exists. During Omen/Stalking/Confrontation the encounter owns the actor but keeps boss combat disarmed. Only after the confrontation delay does it arm the existing `VampireAIController`, which then reuses the already-tested Shadowstep, movement, and physical combat systems.

Spawn candidates are ground-validated, water-rejected, bounded away from the player, and ranked to prefer positions outside the current gameplay camera. If every safe candidate is visible, the safest visible candidate may be used with a warning rather than spawning in invalid geometry. Duplicate bosses are rejected by registry ownership.

Omen V1 intentionally stays restrained: compact existing dark smoke only. No guessed bell, scream, bat, corpse, or blood-clue asset names are shipped. The encounter does not force a cinematic cutscene and remains compatible with free-roam play.

On boss death the director disarms combat, briefly enters Resolution, cleans the owned ped/effects/tasks, and starts the configured in-game-time cooldown. Abort uses a shorter configurable cooldown so the encounter can become eligible again later. Completion/cooldown is **session-owned in Phase 9**; persistent mod save data remains a later project phase and RDR2's save structure is never modified.

The boss-health bar is **not** implemented in Phase 9. This phase establishes the authoritative encounter/boss handle that the later `BossHudController` will consume. No player blood/hunger meter, ability HUD, cooldown bar, power label, boss phase label, move list, or other RPG HUD is added.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables include:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests.
- `Nightwalker.Presentation.Tests` — disappearance/carry presentation-setting tests.
- `Nightwalker.Targeting.Tests` — target-relative planning and Vampire AI config tests.
- `Nightwalker.Movement.Tests` — acceleration-ramp math, velocity math and movement-config bounds.
- `Nightwalker.Feeding.Tests` — hidden-resource, feed-range/alignment and feeding-config tests.
- `Nightwalker.Combat.Tests` — bounded release-vector math and combat-config tests.
- `Nightwalker.Encounter.Tests` — night-window/radius/cooldown math, boss-registry exclusivity, and Saint Denis setting clamps.

GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++. Linux CI also syntax-compiles the gameplay controllers, Phase 9 runtime composition, and native boundaries against test-only signature fixtures. Public CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local dependency.

See `docs/BUILDING.md` for the Story Mode verification checklist, `docs/COMBAT.md` for physical combat, and `docs/ENCOUNTER.md` for the Phase 9 ownership/state contract.

## Debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` and set `[Debug] Enabled=true` for the development harness. The production Saint Denis encounter itself does **not** require Debug mode.

- **F1** — heavy/claw-like strike approximation on an explicitly aimed close ped.
- **F2** — short grab/throat-control approximation.
- **F3** — physical release/throw follow-up.
- **F4** — combat feed follow-up/direct stagger test.
- **F5/F6** — Phase 7 Sip/Drain.
- **F7** — player-side Shadowstep safety/targeting harness.
- **F8** — spawn the debug `cs_vampire` only if the authoritative boss registry is free.
- **F9** — despawn only a debug-owned vampire. It is ignored while the real Saint Denis encounter owns the boss.
- **F10** — clean transient/encounter state, reload config, and re-arm from a safe state.
- **F11** — global cleanup; an active encounter is aborted and its owned boss is removed.

## Encounter defaults

```ini
[Encounter.SaintDenis]
Enabled=true
StartHour=0
EndHour=4
RespawnCooldownHours=24
CenterX=2741.01245
CenterY=-1263.93384
CenterZ=50.61435
TriggerRadius=70.0
AbortRadius=115.0
SpawnMinDistance=18.0
SpawnMaxDistance=55.0
ConfrontationDistance=18.0
OmenDurationMs=2600
StalkingMs=5000
ConfrontationMs=900
LeaveGraceMs=7000
AbortCooldownMinutes=10
```

The district center and all gameplay radii/timings are configurable and clamped to safe ranges.

## Native boundaries

- `GameApi` owns entity, geometry, ground/water, relocation, and model operations.
- `GameCombatApi` owns combat state, aimed-ped lookup, velocity, and ordinary combat tasks.
- `GameEncounterApi` owns the verified RDR2 clock-hour, game-time-seconds, and camera sphere-visibility queries used by Phase 9.
- `GamePresentationApi` owns visibility/alpha and compact smoke presentation.
- `GameMovementApi`, `GameFeedingApi`, and `GamePhysicalApi` retain their existing movement/feed/physics boundaries.

Controllers do not scatter raw native calls or embed guessed animation, audio, or effect names.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. The only custom combat HUD allowed is the later temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
