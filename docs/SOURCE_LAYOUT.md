# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, persistence, presentation, gameplay AI, encounter direction, and the single approved combat HUD.

- `src/core` — runtime composition, INI config/reload, debug input, lifecycle infrastructure, and the pure `SaveData` codec/file replacement layer.
- `src/game` — narrow RDR2-native boundaries.
- `src/systems` — Shadowstep, movement, feeding, combat, boss registry, Saint Denis encounter direction, and `ProgressionController` runtime state ownership.
- `src/ui` — the Phase 10 boss-health model/controller only. No player power/progression HUD exists.
- `src/util` — logging/shared utilities.
- `tests` — deterministic SDK-independent suites plus test-only native signatures.

## Persistence split

`SaveData` has no RDR2-native dependency. It owns:

- schema-version parsing/serialization;
- migration hooks;
- clamps/defaults;
- corrupt-primary/backup recovery;
- temp -> backup -> primary replacement;
- bounded progression-to-config math.

`ProgressionController` owns runtime persistence policy. It samples the existing owners rather than duplicating their state:

- `FeedingController` remains authoritative for the hidden blood value;
- `SaintDenisDirector` remains authoritative for current completion/cooldown during a session;
- `ProgressionController` checkpoints those values into `Nightwalker.state`.

It is first in forward lifecycle order and therefore last in reverse cancellation. That lets combat/AI/EncounterDirector finalize cleanup before a player-death, unsafe-transition, F11, or shutdown checkpoint.

Saved encounter cooldown is enforced before ordinary encounter eligibility by temporarily gating `config.encounter.enabled` until the stored absolute RDR2 game-time timestamp expires. The user's original INI enabled/disabled preference is then restored.

F10 is explicit: gameplay owners clean, progression checkpoints, a fresh INI is parsed, then saved tuning is applied to that clean base. This prevents multiplier compounding.

## Native boundaries

- `GameApi` — entity/model/geometry/ground/water/relocation.
- `GamePresentationApi` — visibility/alpha and compact smoke.
- `GameCombatApi` — combat queries/tasks.
- `GameMovementApi` — move-rate and locomotion restrictions.
- `GameFeedingApi` — human/health/feed/grapple state.
- `GamePhysicalApi` — damage-source/contact, ragdoll and bounded impulse.
- `GameEncounterApi` — clock/game-time/camera visibility.
- `GameBossBarApi` — screen resolution plus normalized rectangle/text drawing only.

No controller embeds raw native hashes or guessed animation/audio/effect names.

## Encounter and boss ownership

`BossActorRegistry` remains the single cross-system boss source. `SaintDenisDirector` is the real encounter owner; the F8 debug spawner may claim the registry only when it is free. The mature AI/movement/combat systems continue consuming the registered actor rather than maintaining another boss implementation.

The boss HUD never scans the ped pool. `SaintDenisDirector` explicitly supplies its owned `cs_vampire` when Confrontation becomes Combat. Abort/cleanup hides immediately; boss death enters the HUD death-hold path before encounter cleanup removes the ped.

## Boss HUD split

`BossHudModel` is pure/testable logic for `Hidden -> FadeIn -> Visible -> FadeOut -> Hidden` plus the death-hold path. `BossHudController` owns the explicitly supplied boss association and `GameBossBarApi` isolates native drawing.

Numeric boss HP remains default-off. No powers, phases, cooldowns, weaknesses, player resource meters, floating damage, progression widgets, or icons are drawn.

## Progression consumption boundary

Phase 11 applies saved tuning only where ownership is unambiguous and does not silently strengthen the enemy:

- player Shadowstep debug-harness distance/cooldown;
- player feeding blood gain and health restoration.

The schema stores future sprint, flank, regeneration, and throw-strength fields but does not apply them to the currently boss-owned/shared systems. A later player gameplay phase must explicitly consume them if approved.

## Cleanup

Unsafe Story Mode transitions, player death, F10/F11, encounter abort, feature disable and plugin shutdown use the established cleanup paths. `ProgressionController` runs its checkpoint after those gameplay owners in reverse cancellation order.

Dormant/cooldown `SaintDenisDirector::Cancel()` already returns without manufacturing a new abort cooldown when no actor exists.

Public CI includes deterministic save/migration/replacement/recovery tests, syntax-compiles `ProgressionController` and Runtime composition, and retains all prior gameplay/native-boundary regressions. Actual restart persistence remains a Story Mode target-environment check.
