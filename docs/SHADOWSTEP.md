# Shadowstep Implementation Spec

## Core fantasy

Nightwalker's signature Shadowstep is a combat grammar, not extreme running speed:

> vanish briefly -> skip space -> reappear close to the target -> carry forward slightly -> begin a readable attack.

The primary actor is the **enemy Saint Denis vampire** (`cs_vampire`, `0xD95BCB7D`). The player-side Shadowstep remains a debug/safety harness and optional future player mechanic. `docs/DESIGN_LOCKS.md` is authoritative if older roadmap text suggests otherwise.

The project may study the feel of modern vampire games but must not copy proprietary Dawnwalker code, animations, audio, dialogue or assets.

---

## Current implementation status — Phase 5

Phase 5 reuses the Phase 3 geometry safety and Phase 4 disappearance/carry presentation for enemy combat instead of building a second teleport implementation.

### Vampire AI flow

```text
Observe
-> Approach
-> Decide
-> ShadowstepDepart
-> HiddenTransit
-> ShadowstepArrive
-> Telegraph
-> Attack
-> Recover
-> Cooldown
-> Approach
```

Explicit future/exception states also exist:

```text
Evade
Reposition      // future seam
FeedAttempt     // future seam
Abort
```

Only the Nightwalker-owned debug `cs_vampire` is controlled in Phase 5. The AI does not scan for or take ownership of arbitrary vanilla vampires/peds.

### Combat read

1. Vampire approaches using ordinary RDR2 combat.
2. At a safe distance and after cooldown, it evaluates target-relative landing candidates.
3. If the player is retreating, a short velocity prediction biases toward intercept/flank candidates.
4. Every candidate goes through `ShadowstepResolver` before it is eligible.
5. Vampire plays compact departure smoke and becomes invisible only after cleanup ownership is registered.
6. Vampire relocates instantly to the chosen validated point.
7. Visibility is restored after the short disappearance window and arrival smoke plays.
8. A small carry toward striking range occurs only if the short segment remains safe.
9. Vampire pauses through a readable telegraph.
10. Only after the telegraph does Nightwalker hand the vampire back to ordinary RDR2 combat with `TASK_COMBAT_PED`.
11. Recovery and internal cooldown prevent continuous teleport spam.

Nightwalker applies **no direct damage on the teleport frame**.

---

## Shared destination safety

`ShadowstepResolver` is the single safety authority for both the player debug harness and vampire AI.

It supports:

- forward direction requests; and
- arbitrary target-relative point requests through `ResolveToPoint`.

A candidate is rejected or shortened as appropriate when:

- path tracing is inconclusive;
- an obstruction blocks the route;
- no safe pedestrian coordinate exists;
- navmesh snapping moves too far from the requested point;
- ground cannot be established;
- vertical change exceeds the configured limit;
- the point is in deep water;
- wall/prop clearance fails;
- headroom fails.

For target-relative AI points, Phase 5 requests exact candidates (`allowShorten=false`). If a flank/intercept/behind point is obstructed, that candidate loses rather than silently turning into a different tactical point.

---

## Target-relative candidate planner

`TargetedShadowstepPlanner` generates four candidates around a target:

- **Intercept** — pressure point biased into the target's short predicted movement path.
- **Left flank** — lateral point relative to target facing.
- **Right flank** — opposite lateral point.
- **Behind** — point behind target facing.

The target position is predicted conservatively from current velocity with a short configurable prediction window and a hard displacement cap.

Each safe candidate is scored for:

- requested tactical preference (intercept/evade/general pressure);
- target facing relationship;
- closeness to desired pre-carry striking distance;
- vertical difference;
- total actor travel distance.

Unsafe candidates retain their structured resolver rejection reason so debug telemetry can explain why they lost.

### Retreat behavior

If the player is moving away from the vampire above the configured threshold, the AI biases toward the intercept candidate, then flanks. The goal is the cinematic moment where the player backs away, the vampire disappears, and reappears where the player is heading.

Prediction remains intentionally short so the vampire does not unrealistically lead several seconds into the future.

### Evade behavior

At close range, a new player melee-button press can create an evade opportunity. Phase 5 deliberately rate-limits this in two ways:

- only alternating eligible close-range attack opportunities request an evade; and
- evade has its own longer cooldown.

Evade planning favors left/right flank, then behind. If no safe candidate exists, the vampire stays in ordinary combat instead of becoming untouchable through forced teleporting.

---

## Presentation

Current default presentation tuning:

| Parameter | Default |
| --- | ---: |
| Disappear window | 110 ms |
| Arrival carry | 1.25 m max |
| Arrival carry time | 140 ms |
| State watchdog | 1000 ms |
| Smoke | enabled |

Current best-effort RDR2 particle reference:

- asset: `scr_fme_spawn_effects`
- effect: `scr_fme_smoke_puff_tint`

Departure is compact and arrival is slightly stronger. Missing/unloaded PTFX must never block relocation or cleanup.

Bats and custom audio remain deferred until a suitable verified lightweight path is selected.

Collision is not deliberately disabled for Shadowstep in the current implementation.

---

## Enemy fairness rules

- Never teleport directly inside the player.
- Do not deal damage on the teleport frame.
- Keep a readable post-arrival attack startup.
- Use smoke/reappearance as the visual tell.
- Maintain internal Shadowstep cooldown.
- Maintain a separate longer evade cooldown.
- If all candidate points are unsafe, keep ordinary RDR2 movement/combat.
- Do not repeatedly teleport behind the player every second.
- Do not expose ability names, cooldowns, phases or next moves in UI.

Default Phase 5 AI tuning:

| Parameter | Default |
| --- | ---: |
| Shadowstep minimum distance | 4.0 m |
| Shadowstep maximum distance | 10.0 m |
| Final striking range | 1.65 m |
| Prediction | 250 ms |
| Decision interval | 180 ms |
| Shadowstep cooldown | 2400 ms |
| Arrival telegraph | 320 ms |
| Attack/recovery observation | 850 ms |
| Evade cooldown | 5000 ms |
| Retreat speed threshold | 0.55 |

These values are internal and never displayed to the player.

---

## Player-side targeted debug harness

F7 remains the existing player test path.

If RDR2 reports an explicitly free-aimed ped that is alive and already in combat with the player, the player harness uses the same candidate planner for a target-relative test step. Otherwise F7 remains the forward safety blink.

This is intentionally narrow:

- it uses existing RDR2 aim context;
- it does not create a custom lock-on system;
- it does not draw a landing marker;
- it does not add a cooldown/power HUD.

---

## Native ownership boundaries

Exact RDR2 calls remain isolated behind wrappers:

- `GameApi` — entity/geometry/ground/water/relocation.
- `GamePresentationApi` — ped visibility/alpha restoration, player melee-input observation and compact smoke.
- `GameCombatApi` — aimed-ped lookup, velocity, combat-state queries, telegraph stand-still task, normal combat task, and owned-task cleanup.

Controllers must not invent raw hashes or guessed animation names.

---

## Cleanup rules

The vampire AI must restore owned transient state when:

- player dies/becomes invalid;
- owned vampire dies/becomes invalid;
- F9 despawns the owned vampire;
- F11 cleanup is requested;
- config is reloaded or disables the feature;
- a mission/cutscene/player-control transition occurs;
- an internal state timeout/error occurs;
- the plugin unloads.

Cleanup restores vampire appearance and clears only Nightwalker-owned task state after revalidating that the handle still belongs to the owned `cs_vampire`.

The runtime system order is arranged so `VampireAIController` is cancelled before `DebugVampireSpawner` deletes its owned ped.

---

## Known Phase 5 boundary

The existing resolver traces world/object/vehicle geometry and enforces explicit separation from the combat target. Phase 5 does **not** perform an expensive broad nearby-ped scan every decision tick, and no unverified ped trace flag is guessed.

Dense crowds therefore remain a required in-game verification case. If additional ambient-ped occupancy logic is needed, it must be implemented through a verified RDR2 native contract in a later hardening slice.

---

## Required in-game tests

1. Open street combat.
2. Player continuously retreats from vampire.
3. Lateral player movement.
4. Narrow Saint Denis alley.
5. Stairs and uneven terrain.
6. Wall/prop beside candidate positions.
7. Water edge.
8. Dense crowd/manual occupancy check.
9. Player commits repeated close melee attacks; evade must remain occasional.
10. Five-minute uninterrupted fight; no teleport spam or stuck invisible state.
11. F9 during/near Shadowstep.
12. F11 during hidden/arrival state.
13. Player death.
14. Vampire death/despawn.
15. Mission/cutscene transition.
16. F10 config reload during combat.
17. Smoke disabled/unavailable.
18. Explicit hostile aim + F7 player harness.

A Phase 5 build is not considered target-environment verified until the five-minute fight and cleanup cases pass in RDR2 Story Mode.
