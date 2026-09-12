# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 7: feeding and optional hidden blood resource.** Phase 5/6 vampire Shadowstep AI and controlled supernatural movement remain intact. Phase 7 adds a cleanup-first player feeding interaction for close ambient NPC validation; the enemy vampire AI `FeedAttempt` state remains a future hook.

Feeding is deliberately conservative. With Debug enabled, the player explicitly free-aims a close target and uses **F5** for non-lethal Sip or **F6** for lethal Drain. Sip uses verified face/hold tasks so it does not accidentally turn into a lethal grapple. Drain attempts RDR2's verified generic grapple task as a Rockstar-authored approximation and falls back to stationary participant tasks if the grapple does not start. No invented animation dictionary or vampire-bite animation name is shipped.

Every feed validates both peds continuously and rejects mission-owned targets, blocked line of sight, excessive distance/height mismatch, mounted/vehicle/scenario/swim/fall/ragdoll states, and non-human targets unless animal feeding is explicitly enabled. Cancellation clears only task state that Nightwalker explicitly started.

An optional session-only internal resource is clamped to `0–100` and replenished by successful feeding. **It is never drawn as a player HUD meter.** Persistence, regeneration, enemy-AI feeding, progression, and combat-feed executions are not part of this phase.

A neck blood particle is also intentionally deferred: the particle API is available, but a suitable RDR2 asset/effect name was not verified strongly enough to commit without guessing.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables include:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regression tests.
- `Nightwalker.Shadowstep.Tests` — Shadowstep math and destination-safety tests.
- `Nightwalker.Presentation.Tests` — disappearance/carry presentation-setting tests.
- `Nightwalker.Targeting.Tests` — target-relative planning and Vampire AI config tests.
- `Nightwalker.Movement.Tests` — acceleration-ramp math, velocity math and movement-config bounds.
- `Nightwalker.Feeding.Tests` — hidden-resource clamping, feed-range/alignment math, legacy config aliases and feeding safety clamps.

GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++. Linux CI also syntax-compiles the gameplay controllers and native boundaries against test-only signature fixtures. Public CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local dependency.

See `docs/BUILDING.md` for local dependency layout and the exact Story Mode verification checklist. See `docs/FEEDING.md` for the Phase 7 safety contract.

## Debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` and set `[Debug] Enabled=true`.

- **F5** — request non-lethal Sip on the explicitly free-aimed close feed target; press again while feeding to cancel.
- **F6** — request lethal Drain on the explicitly free-aimed close feed target; press again while feeding to cancel.
- **F7** — player-side Shadowstep safety/targeting harness; ignored while a feed is active.
- **F8** — spawn one Nightwalker-owned `cs_vampire` debug ped for autonomous vampire combat testing.
- **F9** — cancel active participant state and despawn only the Nightwalker-owned debug vampire.
- **F10** — cancel active transient systems, reload configuration, then re-arm from safe state.
- **F11** — cancel systems and restore Nightwalker-owned temporary state.

## Feeding defaults

```ini
[Feeding]
Enabled=true
AllowNonLethal=true
AllowAnimalFeeding=false
HiddenBloodEnabled=true
InitialBlood=50.0
SipBloodGain=20.0
DrainBloodGain=55.0
HealthRestoreSip=15
HealthRestoreDrain=45
MaxDistance=1.70
AlignMs=350
GrabMs=450
SipDurationMs=1200
DrainDurationMs=1800
ReleaseMs=250
StateTimeoutMs=3500
```

Legacy `HungerRestoreSip` and `HungerRestoreDrain` keys remain accepted as aliases for backwards compatibility. The internal value is session-only until Nightwalker gets its separate save-data system.

## Native boundaries

- `GameApi` owns entity/geometry/model operations.
- `GameCombatApi` owns combat state, aimed-ped lookup, velocity and combat tasks.
- `GamePresentationApi` owns visibility/alpha and compact smoke presentation.
- `GameMovementApi` owns continuous movement-rate and locomotion-restriction natives.
- `GameFeedingApi` owns Phase 7 human/mission/restriction checks, line of sight, health access, face/hold/grapple tasks and participant task cleanup.

Controllers do not scatter raw native calls or embed guessed animation/effect names.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. No player blood/hunger meter, feed meter, cooldown bar, power card, or ability HUD is added. The only planned custom combat HUD remains the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
