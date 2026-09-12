# Phase 11 — Nightwalker-owned progression and persistence

`docs/DESIGN_LOCKS.md` is authoritative. Phase 11 adds persistent mod-owned state and bounded internal progression without adding a player power HUD, skill wheel, radial menu, cooldown meter, blood meter, ability cards, or phase labels.

## File ownership

Nightwalker stores its state beside the plugin in:

```text
Nightwalker.state
```

This file belongs only to Nightwalker. The mod never reads, patches, replaces, or rewrites Red Dead Redemption 2's proprietary save files.

The format is intentionally small, human-readable, and dependency-free. It is a schema-versioned key/value document. Example:

```text
schemaVersion=1
encounter.saintDenis.completed=true
encounter.saintDenis.cooldownUntilGameSeconds=123456
resource.blood=72.5000
progression.points=3
unlock.targetedShadowstep=true
unlock.enhancedFlankLogic=true
unlock.regeneration=false
tuning.shadowstepRangeMultiplier=1.0750
tuning.shadowstepCooldownMultiplier=0.8950
tuning.feedingEfficiencyMultiplier=1.1500
tuning.sprintMultiplier=1.0240
tuning.throwStrengthMultiplier=1.0450
```

Edit this file only while RDR2/Nightwalker is not running. Runtime checkpoints may otherwise overwrite an external edit with the current in-memory state.

## Schema and migration

Current schema: `1`.

The loader has an explicit migration seam. A schema-0 compatibility parser accepts the development aliases `version`, `bossCompleted`, `cooldownUntil`, `hunger`, and `points` and normalizes them into schema 1.

Unknown keys are ignored for forward-compatible additive development. A file declaring a schema newer than the running build is **not downgraded**: Nightwalker uses safe runtime defaults and disables writes for that session to avoid destroying newer data.

## Corruption recovery

Writes use a conservative temp/replace sequence:

1. serialize bounded state to `Nightwalker.state.tmp`;
2. flush and close the temp file;
3. rotate an existing primary file to `Nightwalker.state.bak`;
4. promote the temp file to the primary path;
5. restore the backup if promotion fails;
6. remove the backup after a successful promotion.

On load, a corrupt primary attempts the previous backup before falling back to safe defaults. A missing primary can also recover a valid backup/temp file left by an interrupted replacement. Errors are logged; corrupt state must never crash gameplay.

## Persisted fields

Phase 11 persists only Nightwalker-owned state:

- Saint Denis encounter completion flag;
- absolute RDR2 game-time cooldown timestamp;
- hidden blood/hunger value (`0..100`);
- progression points;
- unlock flags;
- bounded progression tuning multipliers.

Encounter completion is informational in Phase 11; the persisted cooldown is what gates repeat eligibility. When the timestamp expires, the encounter returns to the user's normal `[Encounter.SaintDenis] Enabled` setting.

## Runtime ownership and checkpoint order

`ProgressionController` is the first lifecycle system in forward update order and therefore the last system cancelled in reverse order.

That ordering is deliberate: when player death, a mission/cutscene transition, F11 cleanup, or script shutdown occurs, combat/AI/EncounterDirector clean first. The progression controller checkpoints afterward, so it sees the encounter's final abort/resolution cooldown rather than stale pre-cleanup state.

Normal runtime checks are throttled to roughly once every two seconds and only write when serialized state changed. F10 explicitly checkpoints after active encounter cleanup, reloads the base INI, then reapplies saved progression tuning from the clean base values so multipliers never compound across reloads.

The previously identified Dormant-cancel issue is already fixed in the Phase 9/10 baseline: cancelling `EncounterDirector` while no actor exists in `Dormant` or an existing `Cooldown` does not manufacture a new abort cooldown.

## Progression tuning boundary

Stored values are aggressively clamped:

| Field | Allowed range |
| --- | ---: |
| Shadowstep range multiplier | `1.00 .. 1.25` |
| Shadowstep cooldown multiplier | `0.65 .. 1.00` |
| Feeding efficiency multiplier | `1.00 .. 1.50` |
| Sprint multiplier | `1.00 .. 1.08` |
| Throw strength multiplier | `1.00 .. 1.15` |
| Hidden blood | `0 .. 100` |

Phase 11 immediately consumes only modifiers that currently belong to player/debug-owned gameplay without changing the boss fantasy:

- player Shadowstep debug-harness range;
- player Shadowstep debug-harness cooldown;
- feeding blood gain;
- feeding health restoration.

The save schema also stores sprint, enhanced-flank, regeneration, and throw-strength progression for future use. They are **not** currently applied globally because Nightwalker's continuous sprint is boss-owned and the physical throw controller is shared with the boss. Applying those multipliers now would silently strengthen the enemy, which is not the purpose of player progression.

`targetedShadowstep`, `enhancedFlankLogic`, and `regeneration` are versioned unlock flags, but Phase 11 does not invent a new player skill system to consume them automatically. They establish the persistence contract for later explicitly approved player gameplay.

## Development surface

There is no progression UI. Development balancing is file/config driven:

- set bounded fields in `Nightwalker.state` while the game is closed;
- launch Story Mode and inspect `Nightwalker.log`;
- F10 reloads the INI and reapplies the already-loaded save tuning without compounding it;
- changing `Nightwalker.state` itself requires a restart in Phase 11.

`AdvanceDebugProgression()` exists only as a deterministic code/test helper for verifying tuning caps. No F12 or radial-menu binding is shipped.

## Acceptance verification

Automated tests cover schema-1 round trip, schema-0 migration, clamps, future-schema refusal, tuning application, temp/replace behavior, primary replacement, and corrupt-primary backup recovery.

A real Story Mode restart test must still verify:

1. feed so the hidden blood value changes, exit cleanly, restart, and confirm the persisted value is loaded;
2. resolve/abort the Saint Denis encounter, restart before cooldown expiry, and confirm it cannot restart early;
3. let the stored cooldown expire and confirm the user's encounter-enabled preference becomes effective again;
4. corrupt the primary file while preserving a valid `.bak`, then confirm warning + backup recovery without a crash;
5. remove all state files and confirm safe defaults are used;
6. verify no new custom player/progression HUD appears at any time.
