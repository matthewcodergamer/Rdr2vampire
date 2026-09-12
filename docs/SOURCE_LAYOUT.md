# Source layout

Nightwalker separates native calls, lifecycle ownership, pure math, presentation, gameplay AI, encounter direction, and the single approved combat HUD.

- `src/core` — runtime composition, config/reload, debug input and lifecycle infrastructure.
- `src/game` — narrow RDR2-native boundaries.
- `src/systems` — Shadowstep, movement, feeding, combat, boss registry and Saint Denis encounter direction.
- `src/ui` — Phase 10 boss-health state/model/controller only. No player power HUD exists.
- `src/util` — logging/shared utilities.
- `tests` — deterministic SDK-independent suites plus test-only native signatures.

## Native boundaries

- `GameApi` — entity/model/geometry/ground/water/relocation.
- `GamePresentationApi` — visibility/alpha and compact smoke.
- `GameCombatApi` — combat queries/tasks.
- `GameMovementApi` — move-rate and locomotion restrictions.
- `GameFeedingApi` — human/health/feed/grapple state.
- `GamePhysicalApi` — damage-source/contact, ragdoll and bounded impulse.
- `GameEncounterApi` — clock/game-time/camera visibility.
- `GameBossBarApi` — Phase 10 screen resolution plus normalized rectangle/text drawing only.

No controller embeds raw native hashes or guessed animation/audio/effect names.

## Encounter and boss ownership

`BossActorRegistry` remains the single cross-system boss source. `SaintDenisDirector` is the real encounter owner; the F8 debug spawner may claim the registry only when it is free. The mature AI/movement/combat systems continue consuming the registered actor rather than maintaining another boss implementation.

Phase 10 does not scan the ped pool. `SaintDenisDirector` explicitly calls `BossHudController::BeginBoss` with its owned `cs_vampire` when Confrontation becomes Combat. Abort/cleanup calls the HUD cleanup path; boss death calls the death-hold path before encounter cleanup removes the ped.

## Boss HUD split

`BossHudModel` is pure/testable logic for:

`Hidden -> FadeIn -> Visible -> FadeOut -> Hidden`

and:

`Visible/FadeIn/FadeOut -> DeathHold -> FadeOut -> Hidden`.

It owns fade timing, idle timing, health clamping/smoothing and aspect-aware normalized layout math. It has no RDR2 dependency.

`BossHudController` owns one explicitly supplied boss handle. It refreshes activity from boss health loss, boss-caused player health loss, or confirmed close combat engagement. It never searches the world for a boss.

`GameBossBarApi` isolates the verified native drawing surface (`DRAW_RECT`, screen-resolution query, current background-text functions). Drawing stays centered in the lower normalized safe area with deep red fill, dark backing and subdued title.

Numeric boss HP is default-off. It appears only when the existing `[BossHUD] ShowNumericHealth=true` opt-in is set. No powers, phases, cooldowns, weaknesses, player resource meters, floating damage or icons are drawn.

## Runtime order and cleanup

Forward order places `EncounterDirector` before boss AI and places `BossHudController` last so the HUD draws after gameplay state is updated. Reverse lifecycle cancellation therefore hides the HUD first, then restores combat/movement/AI, then the director removes the encounter actor.

Unsafe Story Mode transitions, player death, F10/F11, encounter abort, feature disable and plugin shutdown converge on that cancellation path. Dormant/cooldown director cancellation no longer manufactures an abort cooldown when no encounter was active.

Public CI includes deterministic boss-HUD timing/layout/config tests, syntax-compiles the HUD controller/runtime composition, and compiles `GameBossBarApi` against test-only verified declarations. Actual visual placement remains a Story Mode target-environment check.
