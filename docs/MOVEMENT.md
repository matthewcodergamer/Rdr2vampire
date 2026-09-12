# Supernatural Movement — Phase 6

## Purpose

Continuous supernatural speed is a separate mechanic from Shadowstep.

- **Shadowstep** is discontinuous: disappear, instant validated relocation, reappear, short arrival carry, readable attack.
- **Supernatural movement** is continuous: the vampire remains in normal world collision/locomotion and receives a modest per-frame move-rate boost while chasing.

The enemy/boss vampire is the primary actor. Phase 6 does not grant the player a supernatural sprint.

## State machine

```text
Idle
  -> RampUp
  -> Boost
  -> Recovery
  -> Idle

Any restriction -> Restricted -> Idle
```

The controller is active only for the Nightwalker-owned `cs_vampire` while `VampireAIController` is in `Approach`.

## Initial tuning

```ini
[Movement]
Enabled=true
SprintMoveRate=1.15
AccelerationMs=350
BurstDurationMs=1800
RecoveryMs=900
DismountRecoveryMs=500
ActivationDistance=4.5
MinVelocity=0.30
TrailFx=true
TrailIntervalMs=180
```

`SprintMoveRate` is clamped to `1.0–1.20`. This is deliberately conservative until the real game test matrix establishes a comfortable animation/steering ceiling.

## Ownership and restoration

Before the controller applies a non-default move rate it owns `OwnedState::Motion` through a local `SafetyWatchdog`.

Restoration sets the owned vampire back to `1.0`. The captured handle is revalidated against `cs_vampire` before restoration, so a stale/recycled handle is never used to modify an unrelated vanilla entity.

The controller releases movement ownership when:

- the AI leaves ordinary Approach/chase;
- the feature or Debug encounter is disabled;
- the owned vampire becomes invalid/despawned;
- the player becomes invalid/dead;
- the vampire is mounted;
- the vampire has just dismounted and is inside the recovery delay;
- the vampire is swimming;
- the vampire is falling;
- the vampire is ragdolled;
- a burst completes and enters recovery;
- Runtime cancels systems for mission/cutscene/player-control transitions;
- F11 cleanup or script shutdown occurs.

The controller never changes player move rate.

## Native boundary

`GameMovementApi` is the only Phase 6 class that calls the movement-state natives used by this system:

- `SET_PED_MOVE_RATE_OVERRIDE`
- `IS_PED_SWIMMING`
- `IS_PED_RAGDOLL`
- `IS_PED_FALLING`
- `IS_PED_ON_MOUNT`

No raw hashes are embedded in the controller.

## Presentation

The movement boost may emit a restrained, low-frequency dark smoke/dust puff using the already verified runtime smoke effect. This is optional and never affects state cleanup.

Phase 6 intentionally does not change gameplay camera/FOV because the speed actor is the enemy vampire. A global camera effect tied to an NPC sprint would make the player's camera react to someone else's locomotion. Wind/footstep emphasis is deferred until a suitable verified RDR2 audio cue or licensed/original sound is selected.

## Real-game test matrix

Verify with the owned debug vampire in:

- Saint Denis streets;
- Valentine;
- forest terrain;
- open plains;
- stairs and slopes;
- obstacle-heavy alleys.

Acceptance requires:

- acceleration reads as a ramp rather than an instant speed jump;
- steering remains controllable;
- normal collision is respected;
- animations remain stable enough for combat approach;
- completed bursts always enter recovery;
- Shadowstep still reads as the impossible instant move;
- restrictions restore normal movement immediately;
- a five-minute chase/combat soak leaves no persistent multiplier after cleanup/despawn/shutdown.
