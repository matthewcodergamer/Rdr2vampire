# Saint Denis Vampire — Boss Health Bar Specification

## Purpose

The boss bar exists for one reason: when the Saint Denis vampire becomes a true combat encounter, the player should immediately understand that this is a major enemy and how much health remains.

It must not become a general RPG HUD and must not explain the boss's mechanics. `docs/DESIGN_LOCKS.md` remains authoritative.

## Phase 10 implementation

Phase 10 implements this specification with three ownership layers:

- `SaintDenisDirector` explicitly supplies its encounter-owned `cs_vampire`; the HUD never scans the world.
- `BossHudController` owns the active HUD/boss association and combat-activity detection.
- `BossHudModel` owns pure/testable fade, death-hold, health smoothing and aspect-aware layout logic.
- `GameBossBarApi` isolates the verified native rectangle/text drawing calls.

The implemented state flow is:

```text
Hidden -> FadeIn -> Visible -> FadeOut -> Hidden

Boss death:
Visible/FadeIn/FadeOut -> DeathHold -> FadeOut -> Hidden
```

Beginning an encounter boss association does not itself pin the meter. `SaintDenisDirector` starts the HUD when Confrontation actually becomes Combat and immediately records combat activity. Thereafter boss health loss, boss-caused player health loss, or confirmed close combat engagement refreshes visibility. Quiet combat fades after the configured idle period; re-engagement reverses the fade with the current health.

Abort, invalid/despawned boss, player/Story Mode unsafe state, F10/F11 cleanup, configuration disable and script shutdown all converge on immediate `ForceHide()` cleanup.

## Visual target

The presentation should feel restrained and compatible with RDR2:

```text
                    THE VAMPIRE
        ─────────────────────────────────
        ███████████████████████░░░░░░░░░
```

- centered near the lower portion of the screen;
- thin horizontal bar rather than a card;
- dark translucent backing and subtle border;
- deep blood-red fill;
- muted warm-gray title;
- no neon glow;
- no ability/status icons;
- no phase number or power names;
- no weaknesses/resistances;
- no floating damage.

Phase 10 uses normalized native drawing coordinates and compensates bar width for wide aspect ratios. Exact placement against every user safe-zone/HUD configuration remains an in-game verification case.

## Behavior

### Show/refresh

The bar is eligible only for the explicit active encounter boss. It fades in/refreshes when:

- scripted confrontation enters Combat;
- boss health decreases;
- the player's health decreases and RDR2 reports the boss as the damage/contact source;
- boss/player have confirmed combat engagement within the controller's close-combat refresh distance.

### Idle hide

Default inactivity timeout: `6.0` seconds. Fade duration: `0.35` seconds. New activity during FadeOut transitions directly back to FadeIn.

### Health

Maintain separate values:

- `actualHealthRatio` — current/max boss health, authoritative and clamped `[0,1]`;
- `displayHealthRatio` — eased toward actual for presentation.

The smoothing layer does not change gameplay health and never becomes the authoritative state.

### Death

Boss death sets authoritative health to zero, enters `DeathHold`, holds the empty meter for the configured duration (default `1.25` seconds), then fades away. No victory statistics or loot card is displayed.

## Configuration

```ini
[BossHUD]
Enabled=true
DisplayName=THE VAMPIRE
IdleSeconds=6.0
FadeSeconds=0.35
DeathHoldSeconds=1.25
ShowNumericHealth=false
```

`ShowNumericHealth` is deliberately default-off. Phase 10 permits numeric current/max HP only when the user explicitly opts in. It never enables ability names, phases, weaknesses, cooldowns or any player-resource HUD.

## Drawing boundary

`GameBossBarApi` uses the current RDR2 native drawing surface for:

- screen resolution;
- normalized rectangles;
- literal centered text.

No browser/CEF overlay or redistributed Rockstar UI asset is required.

## Acceptance tests

The feature is complete only when all of these pass:

1. Starting/stalking without Combat shows no bar.
2. Combat handoff fades the bar in.
3. Boss damage updates the authoritative ratio and smoothed fill.
4. Boss-caused player damage refreshes visibility.
5. Confirmed close combat refreshes visibility.
6. Inactivity fades out after the configured timeout.
7. Re-engagement returns smoothly with current health.
8. Boss death reaches zero, holds, then fades.
9. Boss invalidation/despawn and encounter abort hide immediately.
10. Player death/mission transition/F10/F11/unload cannot leave the bar stuck.
11. 16:9 and ultrawide layouts remain centered/restrained.
12. No additional custom combat HUD is introduced.
