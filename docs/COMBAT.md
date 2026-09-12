# Phase 8 — Expanded Vampire Combat

`docs/DESIGN_LOCKS.md` is authoritative. Phase 8 adds physical supernatural combat without adding an ability HUD, move list, cooldown display, combo counter, blood meter or boss-power labels.

## Scope

Phase 8 adds one shared `VampireCombatController` used by the Nightwalker-owned `cs_vampire` boss AI and by an explicit player debug harness. The controller owns short-lived combat tasks and motion overrides; `VampireAIController` decides when the boss should request a move.

Implemented moves:

1. Shadowstep follow-up strike.
2. Heavy/claw-like unarmed strike approximation.
3. Short grab/throat-control approximation.
4. Physical release/throw into ragdoll with bounded impulse.
5. Combat feed on a controlled or staggered target.

Fear behavior and passive regeneration are optional Phase 8 ideas and are deliberately deferred from this production slice. Fear needs a separately throttled civilian-query/AI ownership policy; regeneration needs a finalized damage/resource policy and should not be smuggled into the combat state machine before the required transitions are stable.

## Shadowstep follow-up

The enemy vampire keeps the existing Shadowstep sequence and its readable post-arrival telegraph. Only after that tell finishes does the AI request `ShadowstepStrike` from `VampireCombatController`.

There is no direct damage on the teleport frame. If the combat controller cannot accept the move, the AI falls back to RDR2's ordinary combat task.

The player Shadowstep debug harness retains its existing buffered normal-melee handoff. Phase 8 does not create a second teleport implementation or replace the shared landing-safety resolver.

## Heavy / claw-like strike

No unverified animation dictionary, melee-style hash or custom claw animation name is shipped.

The strongest verified V1 approximation is:

- use RDR2's ordinary unarmed combat task;
- allow the owned boss a small, bounded movement-rate increase during the strike window;
- clear the prior damage-source marker before the strike;
- only apply the configured small strike bonus after RDR2 reports that the intended target was actually damaged by the actor;
- apply the bonus once, then clear the marker.

A whiff therefore receives no scripted bonus damage. The boss still has a readable windup/cooldown around close-range specials.

## Grab / throat-control approximation

RDR2 exposes `TASK_GRAPPLE`, but the verified generic task is a combat grapple and can become lethal if allowed to run. Phase 8 therefore uses it only for a short controlled window.

Flow:

`Telegraph -> face/alignment -> short grapple -> follow-up or release -> cleanup`

If the grapple cannot start, Nightwalker uses a stationary hold fallback. No attachment, collision disable, invincibility, custom camera lock or guessed paired-animation hash is used in V1. This is intentionally a shorter grab/choke approximation rather than pretending RDR2 has a verified vampire throat-lift animation.

## Physical release / throw

The release path prioritizes stability:

1. clear Nightwalker-owned actor/target tasks first;
2. reject an incompatible target state;
3. build a bounded horizontal/upward impulse from actor to target;
4. project a short endpoint;
5. raycast that segment with the existing game geometry wrapper;
6. enter ragdoll;
7. apply the impulse only when the trace is conclusive and clear.

If a wall/prop blocks the path, or the trace is inconclusive, the target may still ragdoll but the launch impulse is suppressed. The configured force and projection distance are clamped to conservative ranges.

## Combat feed

Combat feed reuses the Phase 7 health/resource ownership rather than creating a second blood system.

- From a Nightwalker-owned short grab, the player or boss can transition to the brief feed state.
- Direct player-debug combat feed requires the explicitly aimed human target to already be ragdolled, which is the V1 staggered-target approximation.
- The feed applies a bounded configured health transfer only after the hold completes.
- Boss scripted feed damage never reduces the player below 1 health; ordinary RDR2 combat remains responsible for lethal combat outcomes.
- Player-debug combat feed can replenish the existing hidden internal resource through `FeedingController`; no resource meter is drawn.

## Boss decision policy

At close range and after an internal special cooldown, the debug boss rotates deterministically through:

`HeavyStrike -> GrabThrow -> CombatFeed`

This avoids random one-frame burst behavior and makes soak testing reproducible. Shadowstep retains its own cooldown and post-arrival tell.

## Cleanup

`VampireCombatController` owns only the transient state it starts. Cleanup runs on normal completion, cancellation, F9/F10/F11, feature disable, player/boss invalidation, mission/control transition and script shutdown.

Owned cleanup includes:

- clear Nightwalker-started actor/target tasks;
- restore any temporary strike movement rate to `1.0`;
- never leave an attachment because Phase 8 creates none;
- never keep collision, invincibility, camera or input state altered.

Runtime update order intentionally lets `MovementController` restore/suppress continuous sprint before `VampireCombatController` reapplies a short strike move-rate override. Reverse-order cancellation cleans combat first.

## Native boundary

`GamePhysicalApi` isolates the verified contact/ragdoll/force calls. Controllers contain no raw native hashes.

The current verified V1 surfaces are:

- entity damaged-by-entity query + last-damage-source clear;
- ped ragdoll request;
- center-of-mass force application;
- existing Phase 7 grapple/face/hold/health/task cleanup wrappers;
- existing geometry raycast for projected release safety.

## Debug controls

With `[Debug] Enabled=true`:

- `F1` — heavy strike on the explicitly aimed close ped.
- `F2` — short grab/control.
- `F3` — while a Phase 8 grab is in Hold, convert it to physical release; otherwise request a direct grab-then-release.
- `F4` — while a Phase 8 grab is in Hold, convert it to combat feed; otherwise request combat feed on an aimed ragdolled human target.
- `F5/F6` — Phase 7 Sip/Drain.
- `F7` — player Shadowstep safety harness.
- `F8/F9` — spawn/despawn the owned debug vampire.
- `F10/F11` — config reload / global cleanup.

These controls are test harnesses, not an in-game ability menu.

## Required Story Mode soak

Phase 8 is not target-environment verified until the following are tested in RDR2 Story Mode:

- repeated Shadowstep -> strike transitions;
- ordinary boss melee mixed with special attacks;
- grab ending normally;
- grab -> physical release;
- grab -> combat feed;
- direct combat feed on a ragdolled target;
- release beside walls/props, on stairs/slopes and in tighter interiors;
- F9/F10/F11, player death, vampire death and mission/control transitions during every active state;
- at least a five-minute boss fight with no stuck tasks, permanent motion override, corrupted player controls or unavoidable teleport-frame damage.
