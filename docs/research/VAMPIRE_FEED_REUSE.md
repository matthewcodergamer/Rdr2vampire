# RDR2 vampire feeding reuse research

## Purpose

Nightwalker should reuse Rockstar-authored interaction systems and installed game content wherever that gives a more convincing result than inventing a parallel animation stack. This note records what is verified today and what is still target-environment research.

`docs/DESIGN_LOCKS.md` remains authoritative. This work is Story Mode only, adds no custom combat HUD, and does not redistribute Rockstar assets.

## Verified Saint Denis vampire content

The decompiled Story Mode `spd_vampire` script references the installed AnimScene:

```text
script@specialped@pdsdv_vampire@ig@ig_1_confront@ig_1_confront
```

The script configures feeding-related playback lists including:

```text
pl_sucking_loop_base
pl_sucking_blood_loop
pl_sucking_blood_loop_02
```

The same encounter binds scene roles for the vampire, Arthur/John, the corpse, and the vampire knife. This is strong evidence that the vanilla Saint Denis feeding presentation is authored as a multi-entity AnimScene rather than as one isolated ped animation.

### Important limitation

The verified vanilla feeding scene is authored around the encounter corpse. That does **not** prove that replacing `CORPSE` with an arbitrary living standing ambient ped is safe or visually aligned. Nightwalker therefore does not force this AnimScene onto live civilians in the production path.

The corpse feeding scene remains a future candidate for an actual-corpse ambient presentation after separate in-game verification.

## Verified standing interaction path

Nightwalker already uses Rockstar's native `TASK_GRAPPLE` through `GameFeedingApi`. That path is preferable to entity attachments because the game owns the paired melee interaction and Nightwalker only owns the tasks it requested.

Current implementation priority is therefore:

```text
paired vanilla TASK_GRAPPLE
    -> keep paired task alive through feed hold
    -> apply Nightwalker feed result
    -> clear only Nightwalker-owned tasks
    -> normal recovery
```

If the paired grapple cannot start, Nightwalker falls back to the existing bounded stationary hold rather than attaching, teleporting, or freezing entities indefinitely.

## Front/rear grapple research seam

Established RDR3 native references document style names such as:

```text
AR_GRAPPLE_BACK_FROM_BACK
AR_GRAPPLE_FRONT_FROM_FRONT
```

for `TASK_PUT_PED_DIRECTLY_INTO_GRAPPLE`.

Nightwalker has pure logic that can identify whether the actor is behind the target and an interface seam for a future styled-grapple implementation. The production `GameFeedingApi` deliberately does **not** call that direct native yet because the remaining parameter semantics and target-environment behavior have not been verified enough for the project's native-safety rule.

Until that research is complete, the styled seam returns unavailable and `VampireFeedPresentation` falls back to the already-established `TASK_GRAPPLE` call.

## Boss combat-feed behavior

The Saint Denis vampire's combat-feed path now prefers the paired Rockstar grapple and keeps that paired task owned during the feed hold. Previously the controller cleared the grapple as it entered `Feed` and replaced both peds with stand-still tasks, which made the interaction mechanically safe but visually flat.

The new ownership flow is:

```text
Shadowstep/approach
-> combat-feed telegraph
-> align actor
-> paired vanilla grapple
-> feed hold while paired task remains active
-> apply bounded feed result
-> clear owned tasks
-> recover
```

The boss feed continues to respect the existing health floor and cleanup rules. It does not deal teleport-frame damage and does not expose an ability name or phase through UI.

## Player/debug feed behavior

The player-side feeding harness uses the same presentation helper for a human Drain target. Sip behavior remains non-lethal under ordinary completion rules. If a Sip target dies from another cause while a feed is already active, the interaction now takes the same safe completion/release path instead of aborting and silently losing the earned result.

## Cleanup ownership

The presentation layer does not:

- attach entities;
- disable collision;
- change invincibility;
- change camera state;
- add a custom HUD;
- keep a permanent reference to arbitrary ambient peds.

The existing controller cleanup still clears only tasks marked as Nightwalker-owned, and it runs on cancellation, timeout, feature disable, unsafe Story Mode transitions, death, F10/F11 cleanup, encounter abort, and plugin shutdown through the existing runtime lifecycle.

## Required in-game verification

Before promoting this presentation as target-environment verified, test at minimum:

1. Boss combat feed from the front.
2. Boss combat feed after a behind/flank Shadowstep.
3. Male and female ambient human archetypes where the player/debug feed is enabled.
4. Short/tall and heavy/light-looking ambient ped variants.
5. Flat street, alley, stairs, slope, and wall-adjacent positions.
6. Target starts moving just before grapple ownership.
7. Target or actor dies during grapple and during feed hold.
8. F10 config reload during paired hold.
9. F11 global cleanup during paired hold.
10. Encounter abort/despawn while paired.
11. Nearby mission state rejecting unsafe targets.
12. Repeated feeds to confirm no accumulated stuck tasks.
13. Generic grapple failure path reaches stationary fallback and still cleans up.

## Next research slice

Do not guess animation dictionaries or native parameters. A separate research pass should verify one of these before adding more production presentation:

- exact safe parameter contract for `TASK_PUT_PED_DIRECTLY_INTO_GRAPPLE` with front/rear human styles; or
- a Rockstar-authored standing two-person neck/bite AnimScene suitable for arbitrary live human peds.

Only after that evidence exists should `GameFeedingApi::StartStyledGrapple` gain a game-facing implementation.
