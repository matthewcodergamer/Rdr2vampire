# Phase 9 — Saint Denis Nighttime Encounter

`docs/DESIGN_LOCKS.md` is authoritative. Phase 9 owns the cleanup-safe free-roam `cs_vampire` encounter; Phase 10 now consumes that authoritative encounter ownership for the one approved boss-health bar.

## Ownership

`BossActorRegistry` is the single authority for the current Nightwalker boss actor. It records the ped handle, whether Debug or Encounter owns it, and whether autonomous combat is armed.

`SaintDenisDirector` owns the real encounter actor. F8 may own a debug actor only when the registry is empty; F9 can never delete the live encounter boss.

Phase 10 adds one explicit presentation handoff: when Confrontation becomes Combat, `SaintDenisDirector` passes its owned actor directly to `BossHudController`. The HUD never scans the world or guesses a boss.

## State machine

```text
Dormant -> Eligible -> Omen -> SpawnPending -> Stalking -> Confrontation
        -> Combat -> Resolution -> Cleanup -> Cooldown -> Dormant

unsafe path:
active -> Abort -> Cleanup -> Cooldown
```

Eligibility requires a safe Story Mode runtime, the configured night window, living player, district radius, enabled encounter/AI features, free boss registry, and expired session cooldown.

Omen remains restrained compact smoke with no camera/control lock. Spawn candidates remain ground/water validated, distance bounded and camera-hidden-preferred. Stalking keeps combat disarmed until the player aims, closes distance or the stalking timer expires. Confrontation provides the readable delay before autonomous AI is armed.

## Combat and Phase 10 HUD handoff

When Confrontation completes:

1. the director arms registry combat;
2. it calls `BossHudController::BeginBoss(actor, DisplayName, now)`;
3. it records initial combat activity so the bar fades in;
4. the existing `VampireAIController` takes over that same ped on the normal runtime update.

The encounter continues to own only encounter-level rules. Shadowstep, continuous movement, melee, grab/throw and combat feed stay in their existing controllers.

Boss death first disarms autonomous combat and calls `BossHudController::EndBoss(true, now)`. The HUD can therefore perform its empty-bar death hold/fade while encounter Resolution/Cleanup removes the ped. Abort, invalid actor, F10/F11, unsafe Story Mode transition and shutdown call the immediate HUD cleanup path.

## Cleanup order

Forward update order ends with `BossHudController`, so it renders after gameplay/encounter state has updated. Reverse cancellation hides the HUD first, then restores physical combat/movement/AI, then EncounterDirector deletes its actor.

Dormant or already-Cooldown cancellation with no actor does not manufacture a new abort cooldown. An actually-started encounter still receives the configured abort cooldown after cleanup.

## Persistence boundary

Encounter completion/cooldown remains session-owned. Nightwalker does not modify RDR2 save files; persistent mod-owned save data is later work.

## Deliberate boundaries

The encounter still does not add boss phase labels, named powers, persistent player meters, custom cutscenes, guessed bell/scream audio, permanent bat swarms, or corpse/blood-clue entity ownership.

The only custom combat UI tied to this encounter is the Phase 10 temporary boss-health bar defined in `docs/BOSS_HEALTH_BAR.md`.

## Required Story Mode verification

1. Enter the configured district during the active night window with Debug off.
2. Verify omen/stalking show no bar and do not seize camera/controls.
3. Verify safe single-boss spawn and readable Confrontation delay.
4. When Combat is armed, verify the Phase 10 bar fades in for the exact encounter-owned actor.
5. Fight normally; existing Shadowstep/melee/feed/movement behavior must remain unchanged.
6. Disengage/re-engage and verify bar fade/return with current health.
7. Leave beyond abort grace, trigger mission/cutscene/player death, F10/F11, and unload; the actor and bar must clean safely.
8. Kill the boss; verify registry combat disarms, HUD reaches zero/death hold/fade, then encounter cleanup/cooldown proceeds.
9. Repeat start/abort/resolution cycles with shortened test cooldowns and verify no duplicate boss or stale bar.
10. F8/F9 must not interfere with the encounter-owned actor.
