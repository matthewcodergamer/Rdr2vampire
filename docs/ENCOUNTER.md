# Phase 9 — Saint Denis Nighttime Encounter

`docs/DESIGN_LOCKS.md` is authoritative. This phase turns the existing RDR2 Saint Denis vampire into a cleanup-safe free-roam encounter without adding power UI, boss phase labels, or the later boss-health bar.

## Ownership

`BossActorRegistry` is the single authority for a Nightwalker boss actor. It records:

- the current ped handle;
- whether the owner is the debug harness or the Saint Denis encounter;
- whether autonomous vampire combat is armed.

The encounter director owns the real encounter actor. F8 can own a debug actor only when the registry is empty. F9 only deletes a debug-owned actor and cannot delete the live encounter boss.

The implementation class is `SaintDenisDirector`; its lifecycle name is `EncounterDirector` because that is the project-level responsibility it fulfills.

## State machine

```text
Dormant
-> Eligible
-> Omen
-> SpawnPending
-> Stalking
-> Confrontation
-> Combat
-> Resolution
-> Cleanup
-> Cooldown
-> Dormant

unsafe path:
active state -> Abort -> Cleanup -> Cooldown
```

### Dormant / Eligible

Eligibility is polled at a bounded interval rather than scanning every frame. The runtime must already report a safe Story Mode state before any gameplay controller is updated.

The director additionally requires:

- `[Encounter.SaintDenis] Enabled=true`;
- `[VampireAI] Enabled=true`;
- the configured night window;
- a living player;
- the player inside `TriggerRadius` of the configured district center;
- no existing boss owner;
- the current session cooldown expired.

The default time window is `00:00` through `04:00` with an exclusive end hour. Cross-midnight windows are supported by pure encounter math.

### Omen

Omen V1 uses the already-verified compact Nightwalker smoke presentation at a low frequency. It does not start a cutscene, lock the camera, disable controls, spawn permanent bats, or invent an unverified bell/scream effect name.

This phase intentionally leaves corpse/blood-clue hooks unused until they can be introduced without taking ownership of vanilla mission/world entities.

### SpawnPending

`cs_vampire` (`0xD95BCB7D`) is requested through the existing `ModelStreamRequest` lifecycle.

Candidate positions are sampled around the configured district center. Every candidate must:

- resolve through RDR2's safe-ped coordinate helper;
- have a valid ground height;
- remain within a conservative vertical band;
- reject unsafe water;
- remain between `SpawnMinDistance` and `SpawnMaxDistance` from the player.

Candidates outside the gameplay camera are strongly preferred using the verified camera sphere-visibility query. If all otherwise-safe candidates are visible, the safest visible fallback may be used and a warning is logged. Geometry safety wins over presentation preference.

Only one actor can claim `BossActorRegistry`. If another owner wins the race, the just-created ped is deleted immediately.

### Stalking

The director owns the actor but keeps registry combat disabled. The existing `VampireAIController` therefore remains idle.

The encounter progresses to Confrontation when any of these happens:

- the player explicitly aims at the vampire;
- the player closes to `ConfrontationDistance`;
- the restrained stalking timer expires.

The actor is held briefly and receives only a compact smoke cue. This is free-roam staging, not a giant scripted cutscene.

### Confrontation

After the configured readable delay, the director sets the registry's combat-enabled flag. No damage is issued by this transition.

Because the existing vampire systems consume the registry-backed boss source, the next normal runtime update lets `VampireAIController` take over the same ped. Shadowstep, supernatural movement, heavy melee, grab/throw, and combat-feed behavior are therefore reused rather than duplicated.

### Combat

The director watches only encounter-level conditions while the existing AI owns combat decisions.

Combat aborts when:

- the encounter or vampire-AI feature is disabled;
- the configured night window closes;
- the authoritative actor becomes invalid;
- the player remains outside `AbortRadius` longer than `LeaveGraceMs`.

Player death and mission/cutscene/player-control transitions are handled one level higher by Runtime's unsafe-state gate, which reverse-cancels combat/AI before the director removes its actor.

### Resolution

Boss death immediately disarms registry combat. The director enters a short Resolution hold, then Cleanup removes the owned ped and starts `RespawnCooldownHours` using RDR2's in-game time counter.

Phase 9 completion/cooldown is session-owned only. It is intentionally not written into RDR2's save files. A later Nightwalker save-data phase may persist this mod-owned state separately.

### Abort / Cleanup

Abort disarms the boss before deletion. Cleanup:

1. releases any outstanding model request;
2. clears Nightwalker-owned boss task state;
3. restores appearance;
4. revalidates the model before deleting the handle;
5. deletes only the encounter-owned `cs_vampire`;
6. releases registry ownership only after successful deletion.

If deletion fails, the director stays in Cleanup and retries rather than silently dropping ownership and leaving a stranded actor.

Abort uses `AbortCooldownMinutes`, which is intentionally shorter than the successful-resolution cooldown so leaving the district or an interrupted world transition does not permanently consume the encounter.

## Runtime cleanup order

The systems are registered so reverse cancellation runs approximately:

```text
VampireCombatController
MovementController
VampireAIController
EncounterDirector
FeedingController
ShadowstepController
DebugVampireSpawner
```

This means actor-specific combat/presentation/movement ownership is restored before EncounterDirector attempts to delete its ped.

F10 follows the same principle explicitly: combat/movement/AI are cancelled, then the encounter is cleaned, then config is replaced.

## Native boundary

`GameEncounterApi` contains the only Phase 9-specific native calls:

- current RDR2 clock hour;
- RDR2 game-time seconds used for session cooldown comparison;
- gameplay-camera sphere visibility used only as a spawn presentation preference.

Spawn ground/water/model operations continue through `GameApi`; aim/combat tasks continue through `GameCombatApi`; smoke remains in `GamePresentationApi`.

## Deliberate boundaries

Phase 9 does **not** implement:

- the red boss-health bar — that is the next UI phase;
- boss phase labels or named powers;
- persistent Nightwalker save data;
- custom dialogue/cutscene direction;
- guessed bell/scream audio;
- permanent bat swarms;
- corpse or blood-clue entity ownership.

## Required Story Mode verification

A real Script Hook RDR2 build must still verify:

1. Enter the configured church district shortly after midnight with Debug off; the encounter should become eligible.
2. Confirm the omen remains restrained and controls/camera remain free.
3. Confirm the vampire spawns on valid ground, not in water/geometry, and preferably off-camera.
4. Repeat approaches to ensure one encounter never creates duplicate bosses.
5. Aim at or approach the stalking vampire and verify the Confrontation delay occurs before AI combat starts.
6. Fight normally and confirm existing Shadowstep/melee/feed/movement behavior still works.
7. Leave beyond `AbortRadius`, return before `LeaveGraceMs`, and confirm the fight remains active.
8. Leave longer than the grace period and confirm actor/effects/tasks clean up.
9. Trigger a mission/cutscene/player-control transition and confirm global cleanup removes the encounter actor.
10. Test player death and script unload during Omen, SpawnPending, Stalking, Confrontation, and Combat.
11. Kill the boss; verify Resolution cleanup and the long completion cooldown.
12. Temporarily shorten cooldowns in config and verify abort/restart and resolved/restart behavior without duplicates.
13. F8 during a live encounter must not create a debug boss; F9 must not delete the encounter boss.
14. F11 must abort and clean the active encounter.
15. Verify no player power HUD, cooldown display, boss phase label, or boss-health bar is introduced by Phase 9.
