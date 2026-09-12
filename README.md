# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. The repository references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 8: expanded vampire melee.** Existing safe Shadowstep, enemy vampire AI, controlled supernatural sprint, and Phase 7 feeding remain intact. Phase 8 adds a small physical combat layer shared by the Nightwalker-owned `cs_vampire` and an explicit player debug harness.

The combat layer stays grounded in RDR2 instead of inventing a second arcade combat system. Shadowstep keeps its existing readable arrival tell, then delegates the follow-up strike. Heavy/claw-like attacks use normal RDR2 unarmed combat and only receive a small configured bonus after RDR2 confirms the intended actor actually hit the intended target. Short throat-control uses the verified generic grapple for a tightly bounded window with a stationary fallback; Phase 8 does not pretend there is a verified custom vampire throat-lift animation.

Physical release clears Nightwalker-owned participant tasks before ragdoll. A short projected path is raycast first, and the bounded impulse is applied only when that trace is conclusive and clear; blocked or uncertain paths suppress the launch. Combat feed can follow the short grab, while the player debug path can also use it on an explicitly aimed ragdolled human target. Player combat feed replenishes the same hidden Phase 7 resource and **does not add a resource meter**.

The boss rotates close-range specials deterministically with an internal cooldown and keeps the existing Shadowstep telegraph. There is no direct teleport-frame damage and no custom UI exposing move names, cooldowns, phases or powers.

Fear behavior and passive regeneration are optional Phase 8 ideas and are intentionally deferred until civilian-query ownership and resource/damage policy are separately proven.

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
- `Nightwalker.Combat.Tests` — bounded release-vector math and Phase 8 combat-config/alias tests.

GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++. Linux CI also syntax-compiles the gameplay controllers and native boundaries against test-only signature fixtures. Public CI intentionally does not link `Nightwalker.asi` because Script Hook RDR2 is a developer-local dependency.

See `docs/BUILDING.md` for local dependency layout and the Story Mode verification checklist, `docs/FEEDING.md` for Phase 7 feeding, and `docs/COMBAT.md` for the Phase 8 ownership/safety contract.

## Debug controls

Copy `config/Nightwalker.example.ini` beside `Nightwalker.asi` as `Nightwalker.ini` and set `[Debug] Enabled=true`.

- **F1** — heavy/claw-like strike approximation on the explicitly aimed close ped.
- **F2** — short grab/throat-control approximation.
- **F3** — convert an active Phase 8 grab Hold into physical release, or request grab-then-release directly.
- **F4** — convert an active Phase 8 grab Hold into combat feed, or feed an explicitly aimed ragdolled human target.
- **F5** — Phase 7 non-lethal Sip.
- **F6** — Phase 7 lethal Drain.
- **F7** — player-side Shadowstep safety/targeting harness.
- **F8** — spawn one Nightwalker-owned `cs_vampire` debug ped for autonomous vampire combat testing.
- **F9** — cancel active transient combat/feed/movement/AI state and despawn only the Nightwalker-owned debug vampire.
- **F10** — cancel transient systems, reload configuration, and re-arm from safe state.
- **F11** — cancel systems and restore Nightwalker-owned temporary state.

The F1–F7 inputs are development/test harnesses, not a custom skill wheel or ability HUD.

## Phase 8 combat defaults

```ini
[Combat]
Enabled=true
MaxDistance=2.25
HeavyWindupMs=260
ShadowstepFollowupWindupMs=120
StrikeWindowMs=700
RecoveryMs=600
GrabAlignMs=300
GrabHoldMs=400
BiteHoldMs=500
ThrowRagdollMs=1200
StateTimeoutMs=4500
StrikeBonus=8
BiteDamage=18
BiteHeal=10
BiteBloodGain=12.0
StrikeMoveRate=1.08
ThrowHorizontalForce=1.35
ThrowUpForce=0.28
ThrowProjectionMeters=2.25
BossSpecialCooldownMs=2800
```

`BiteHoldMs`, `BiteDamage`, `BiteHeal`, and `BiteBloodGain` remain readable aliases for the internal CombatFeed settings. All values are clamped to conservative ranges.

## Native boundaries

- `GameApi` owns entity/geometry/model operations and the world raycast used before physical release.
- `GameCombatApi` owns combat state, aimed-ped lookup, velocity and ordinary combat tasks.
- `GamePresentationApi` owns visibility/alpha and compact smoke presentation.
- `GameMovementApi` owns movement-rate and locomotion-state queries.
- `GameFeedingApi` owns human/mission/restriction checks, LOS, health, face/hold/grapple tasks and participant-task cleanup.
- `GamePhysicalApi` owns Phase 8 hit-contact confirmation, ragdoll and center-of-mass impulse calls.

Controllers do not scatter raw native calls or embed guessed animation/style/effect hashes.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text when there is a conflict. No player blood/hunger meter, cooldown bar, move list, ability card, skill wheel, boss phase label, or power HUD is added. The only planned custom combat HUD remains the temporary red Saint Denis vampire boss-health bar described in `docs/BOSS_HEALTH_BAR.md`.
