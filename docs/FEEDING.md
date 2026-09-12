# Feeding V1

`docs/DESIGN_LOCKS.md` remains authoritative. Phase 7 adds no player blood/hunger meter, no feed HUD, and no power-reveal UI.

## Scope

Phase 7 implements a cleanup-first player feeding interaction for debug/validation use. It does not yet connect feeding to the enemy vampire AI `FeedAttempt` state, persistence, regeneration, progression, or encounter scripting.

Debug controls while `[Debug] Enabled=true`:

- `F5` — request non-lethal Sip on the currently free-aimed close ped; press again while active to cancel.
- `F6` — request lethal Drain on the currently free-aimed close ped; press again while active to cancel.

The explicit aimed-ped requirement avoids broad ambient-ped scans and makes target ownership deterministic during this phase. Phase 7 also rejects active melee/combat participants; a true combat-feed execution is reserved for the later combat-feed hook.

## State machine

`Candidate -> Align -> Grab -> FeedLoop -> ReleaseDrain -> Cleanup -> Idle`

Every active state has a timeout. F5/F6 cancellation, F11, config reload, player death, mission/player-control transition, and script unload converge on the same cleanup path.

## Target validation

A target is accepted only when:

- player and target handles are valid and alive;
- target is not the player;
- target is human unless `AllowAnimalFeeding=true`;
- target is not reported by RDR2 as a mission entity;
- player/target are not already in melee or direct combat with each other;
- player and target are not ragdolled, falling, swimming, mounted, in a vehicle, or using a scenario;
- target is within `MaxDistance`;
- player has clear line of sight to target;
- vertical separation is small enough for the V1 alignment.

If any validation fails during the interaction, feeding aborts and Nightwalker clears only the participant task state that it explicitly started.

## Animation/task strategy

RDR2 exposes a verified `TASK_GRAPPLE` native. Its documented behavior is a combat grab/beat sequence and its exact style/hash parameters are under-documented, so Phase 7 does not invent a vampire animation dictionary or pretend the vanilla vampire feeding scene has been reconstructed.

- **Sip:** uses verified face/hold task behavior only. This avoids allowing a combat grapple task to accidentally kill a victim that must survive.
- **Drain:** attempts the verified generic grapple task for a stronger Rockstar-authored approximation. If the grapple does not start, Nightwalker falls back to stationary participant tasks and still preserves cleanup.
- No ped attachment, collision disable, invincibility toggle, camera lock, or custom paired-animation hash is introduced in V1.

The production target remains a better verified paired/grab animation if research later identifies one with a documented contract.

## Internal resource

`HiddenResource` stores an optional normalized internal value in `[0, 100]`. It starts from `InitialBlood`, gains `SipBloodGain` or `DrainBloodGain`, and never draws a HUD element.

Persistence is deliberately deferred until the separate Nightwalker save system is implemented. The resource currently lives only for the plugin session and is not written into RDR2 save data.

Existing `HungerRestoreSip` / `HungerRestoreDrain` INI keys remain accepted as aliases for backwards compatibility.

## Health effects

On successful completion:

- Sip restores `HealthRestoreSip` to the player, clamped to the current ped maximum health, and does not intentionally damage the target.
- Drain restores `HealthRestoreDrain` and sets the surviving feed target to zero health at completion.
- If a Drain grapple causes the target to die before the configured loop ends, the drain is treated as completed once and cleanup follows.

No direct health/resource reward is granted on an ordinary cancellation or validation failure.

## Blood presentation gap

Phase 7 does not ship a neck blood particle. The native particle-on-ped-bone surface is verified, but a suitable RDR2 asset/effect name was not verified strongly enough to commit without guessing. This is an intentional presentation gap rather than a fake asset reference.

## Configuration defaults

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

All numeric values are validated/clamped by `ConfigValidation.cpp`.

## Required Story Mode soak

Before calling Phase 7 target-environment verified, perform at least 20 feed attempts across varied ambient human NPC archetypes. Include both Sip and Drain, cancellations during every state, targets near walls/steps, wanted/combat-adjacent situations, and mission-area rejection checks. No test may leave a victim floating, permanently frozen, attached, invisible, or task-locked after Nightwalker cleanup.
