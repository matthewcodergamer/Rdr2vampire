# Shadowstep Implementation Spec

## Reference behavior

The Blood of Dawnwalker's released gameplay describes Shadowstep as a short-range vampire teleport. A quick activation moves forward; holding the ability allows aiming before release. The mechanic is also used for combat repositioning and traversal.

For Nightwalker, the goal is **not** to duplicate proprietary code or animation data. We reproduce the visual/interaction language inside RDR2 using native scripting:

> vanish briefly → reposition instantly → reappear with momentum → attack can flow immediately.

---

## State machine

```text
Idle
  ↓ press
Targeting (hold only)
  ↓ release / tap
ValidateDestination
  ├─ invalid → Cancel
  └─ valid
      ↓
Departure
      ↓
HiddenTransit
      ↓
Arrival
      ↓
Recovery
      ↓
Cooldown
      ↓
Idle
```

Every state must have a timeout and cleanup path.

---

## Suggested first tuning values

These are starting points for testing, not final balance.

| Parameter | Initial value |
| --- | ---: |
| Quick-step distance | 6.5 m |
| Aimed max distance | 9.0 m |
| Combat flank radius | 1.6 m from target |
| Disappear window | 80–130 ms |
| Arrival carry | 1.25 m |
| Arrival carry time | 100–180 ms |
| Base cooldown | 550 ms |
| Combat chain cooldown | 700 ms |
| Stamina cost | 8% |
| Hunger cost | 1–2 points |
| Max vertical rise V1 | 1.5 m |

Later progression can extend range and reduce cost.

---

## Destination solving

### Quick step

1. Read player forward vector.
2. Set requested point at `position + forward * range`.
3. Ray/shape test from chest height toward requested point.
4. If blocked, shorten destination to just before the obstruction.
5. Cast downward near the endpoint to find ground.
6. Validate enough capsule/head clearance.
7. Reject if no safe point exists.

### Aimed step

1. Raycast from gameplay camera center.
2. Clamp hit/destination to max range from player.
3. Prefer ground directly below the aim point when landing on terrain.
4. If the surface is vertical, V1 rejects it; future vertical traversal may interpret it as a ledge target.
5. Show a marker only when the final landing point is valid.

### Combat target step

For a target ped with position `T`, forward vector `Tf` and right vector `Tr`:

- behind candidate: `T - Tf * 1.6`
- left flank: `T - Tr * 1.6`
- right flank: `T + Tr * 1.6`
- front pressure: `T + Tf * 1.8`

Score each point for:

- collision clearance;
- ground validity;
- distance from player;
- line of sight;
- distance from walls/props;
- whether the target is moving into that point.

Pick the best valid candidate. If none are safe, fall back to a shortened forward step.

---

## Visual sequence

### Departure

- Stop incompatible player tasks for the minimum necessary time.
- Spawn a short dark smoke/dust burst at feet/torso.
- Optional 1–3 bat entities or a bat-like particle burst for high-quality mode.
- Apply very short motion blur/camera impulse if the user allows it.
- Fade the ped's alpha or visibility rapidly.

### Transit

- Keep transit essentially instantaneous.
- Do **not** leave the player invisible for a long animation.
- Temporarily disable collision only if tests prove it is necessary, and restore it immediately after repositioning.

### Arrival

- Set the safe destination.
- Face movement direction or the combat target.
- Restore visibility.
- Spawn arrival smoke.
- Apply a tiny forward movement/velocity or short animation-driven carry.
- Let buffered melee input fire during the final recovery frames.

The small post-arrival carry is what creates the visual impression that the vampire appears and then slides/glides into striking distance.

---

## Enemy AI version

### Conditions to consider a blink

- target is alive;
- target is between roughly 4 m and 12 m away;
- vampire is not ragdolled, feeding, mounted or in a protected scene;
- ability cooldown is ready;
- at least one destination candidate is valid.

### Decision weights

If player is backing away:

- favor front/intercept candidate;
- estimate player velocity for a very short prediction window;
- arrive slightly off-center so the animation reads clearly.

If player is attacking:

- favor side evade or behind candidate.

If vampire is low health:

- favor escape step, then attempt to feed on a nearby victim if the boss design enables it.

### Fairness rules

- Never teleport directly inside the player's collision capsule.
- Minimum arrival tell: smoke/audio cue, even if very short.
- Do not deal damage on the exact teleport frame.
- Give a readable attack startup after arrival.
- Add anti-spam cooldown after chained steps.

---

## Supernatural speed versus Shadowstep

Keep these separate:

**Shadowstep** = discontinuous repositioning / teleport.

**Vampire sprint** = continuous running speed with movement-rate override.

Combining both all the time will make collision and combat unreadable. The sprint should make the vampire frighteningly fast; Shadowstep should be the special impossible movement.

---

## RDR2 systems we expect to use

Exact function calls should be confirmed against the Script Hook SDK/native database during implementation.

- Player/ped world position and heading.
- Gameplay camera position/direction.
- Shape tests/raycasting.
- Entity coordinate repositioning.
- Entity visibility/alpha.
- Collision toggle if absolutely necessary.
- `TASK_PLAY_ANIM` for departure/arrival/combat transitions where suitable.
- Particle FX on entity/coordinates.
- `SET_PED_MOVE_RATE_OVERRIDE` for continuous vampire speed, not the teleport itself.
- Ragdoll/force functions for throws and impacts.

---

## Bat effect strategy

RDR2 includes an `A_C_Bat_01` animal archetype. However, real bat peds are heavier and less deterministic than particles.

Build in this order:

1. smoke-only VFX;
2. fake/particle bat silhouettes if a usable effect exists;
3. optional short-lived real bats for cinematic encounters only.

Never spawn a fresh group of real bat peds every Shadowstep without strict cleanup.

---

## Failure recovery

At the start of every game tick, if the Shadowstep controller detects an impossible state for too long, force-reset:

- player visible = true;
- alpha = full;
- collision = enabled;
- invincibility/proofs = normal;
- movement rate = normal;
- input lock = released;
- ability state = Idle.

Also reset on death, character swap, save/load transition and script unload.

---

## Test cases

1. Flat Saint Denis street.
2. Narrow alley.
3. Stairs.
4. Inside a building.
5. Against a wall.
6. Fence/railing in the path.
7. Cliff edge.
8. Shallow and deep water.
9. Moving combat target.
10. Player backing away from vampire AI.
11. Target next to a wall.
12. Repeated rapid input.
13. Cutscene begins during targeting.
14. Player dies during arrival.
15. 100 consecutive random-direction blinks.

A Shadowstep build is not considered stable until it survives all of these without a soft lock.
