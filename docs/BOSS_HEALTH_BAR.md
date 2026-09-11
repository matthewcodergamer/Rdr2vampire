# Saint Denis Vampire — Boss Health Bar Specification

## Purpose

The boss bar exists for one reason: when the Saint Denis vampire becomes a true combat encounter, the player should immediately understand that this is a major enemy and how much health remains.

It must not become a general RPG HUD and must not explain the boss's mechanics.

## Visual target

The presentation should feel restrained, cinematic and compatible with Red Dead Redemption 2.

Recommended layout:

```text
                    THE VAMPIRE
        ─────────────────────────────────
        ███████████████████████░░░░░░░░░
```

- centered near the lower portion of the screen, above the normal bottom-edge HUD safe area;
- thin horizontal bar rather than a large card;
- dark translucent backing;
- deep blood-red health fill;
- muted off-white or warm-gray boss name;
- subtle dark border/edge;
- no neon glow;
- no icons for abilities;
- no numeric HP by default;
- no phase number;
- no weakness/resistance indicators;
- no status-effect row.

The exact title is configurable. Start with `THE VAMPIRE`; an original story-specific title can replace it later.

## Behavior state machine

Suggested states:

```text
Hidden
  -> FadeIn
  -> Visible
  -> FadeOut
  -> Hidden

Boss death:
Visible/FadeIn -> DeathHold -> FadeOut -> Hidden
```

### Show the bar when

- the encounter director marks the Saint Denis vampire as the active boss; AND
- the player and boss enter combat engagement; OR
- the boss damages the player; OR
- the player damages the boss; OR
- a scripted confrontation explicitly begins.

### Refresh its visibility timer when

- boss takes damage;
- player takes damage from boss;
- boss performs an aggressive action close to the player;
- player actively targets/aims at the boss within the encounter radius;
- boss is close enough and has active combat intent.

### Fade it away when

The boss is still alive, but no relevant combat interaction has happened for a configurable period.

Recommended starting value:

- `BossBarIdleSeconds = 6.0`

Use a smooth fade rather than an instant hide.

Recommended fade duration:

- `BossBarFadeSeconds = 0.35`

### Immediately hide/clean up when

- encounter is aborted;
- boss entity is invalid or despawned;
- player dies;
- player enters an incompatible mission/cutscene transition;
- the mod unloads;
- Story Mode state becomes invalid.

### Boss death behavior

When health reaches zero:

1. animate health fill down to zero;
2. optionally hold the empty red bar for roughly 1.0–1.5 seconds;
3. fade the entire widget away;
4. do not show victory statistics or loot cards unless a later design explicitly asks for them.

## Health smoothing

Do not snap the visible bar directly to every health change if it looks harsh.

Maintain:

- `actualHealthRatio` — authoritative boss health / max health;
- `displayHealthRatio` — eased visual value.

Interpolate `displayHealthRatio` toward `actualHealthRatio` over a short period. If desired, a second delayed damage layer can briefly show the previous health in a darker red, but keep it extremely subtle and remove it if it looks too arcade-like.

Clamp all values to `[0.0, 1.0]`.

## Encounter ownership

The HUD must never guess which random ped is a boss.

`EncounterDirector` owns the active boss handle/ID and explicitly tells `BossHudController` when the boss encounter starts and ends.

Suggested interface concept:

```cpp
class BossHudController {
public:
    void BeginBoss(Entity boss, const std::string& displayName);
    void NotifyCombatActivity();
    void Update(float deltaSeconds);
    void EndBoss(bool defeated);
    void ForceHide();
};
```

The actual implementation can vary with the chosen RDR2 drawing/native approach.

## Drawing strategy

Start with the simplest stable in-game drawing path available through RDR2/Script Hook RDR2 natives:

- draw rectangles/sprites/text every frame while visible;
- calculate normalized safe-zone coordinates;
- support 16:9 first, then verify ultrawide and common resolutions;
- cache fonts/text scale decisions;
- avoid loading a browser/CEF overlay just for one bar.

Only introduce custom texture assets if the native drawing path cannot achieve a clean result.

## Configuration

Suggested settings:

```ini
[BossHUD]
Enabled=true
DisplayName=THE VAMPIRE
IdleSeconds=6.0
FadeSeconds=0.35
DeathHoldSeconds=1.25
ShowNumericHealth=false
```

Do not expose boss abilities through this configuration.

## Acceptance tests

The feature is complete only when all of these pass:

1. Starting the encounter without combat does not permanently pin the bar onscreen.
2. First hostile engagement fades the bar in.
3. Damaging the boss reduces the bar correctly.
4. Boss damaging the player refreshes the visibility timer.
5. Breaking contact causes the bar to fade away after the idle period.
6. Re-engaging makes it return smoothly with the correct current health.
7. Going far away or aborting the encounter cleans it up.
8. Boss death reaches zero, holds briefly, then fades away.
9. Player death/mission transition/mod unload cannot leave the bar stuck.
10. No boss powers, cooldowns, phases or weaknesses are revealed anywhere in the widget.
