# Phase 13 — Long-session hardening and compatibility

`docs/DESIGN_LOCKS.md` remains authoritative. Phase 13 changes safety, scheduling and diagnostics; it does not add another gameplay phase, a player-power HUD, Online support, or unverified native contracts.

## Runtime recovery policy

`LongSessionGuard` is pure/testable policy. `Runtime` supplies the current Story Mode observation and performs cleanup through the existing controller ownership graph. Recovery thresholds are deliberately internal rather than user-tunable: a conflicting INI cannot switch off critical cleanup.

After any unsafe/discontinuous transition, gameplay updates remain quarantined until Story Mode has been stable for 750 ms. This prevents a controller from immediately reacquiring a stale actor on the first frame after a load, cutscene, fast-travel or player-ped replacement.

| Recovery case | Phase 13 behavior |
| --- | --- |
| Player death / invalid player | Existing `GameContext` unsafe signal cancels every system and restores owned state. |
| Cutscene / mission / loss of player control | Existing Story Mode safety signal cancels and quarantines; no new native hash is introduced. |
| Save/load or fast travel | Unsafe/control transitions are primary; a >2.5 s script gap, player-handle replacement, or >120 m one-tick position discontinuity is an additional conservative fail-safe. |
| Boss death | Remains in the normal encounter + boss-HUD death path so the empty red bar can hold/fade correctly. A dead but still valid owned `cs_vampire` is not misclassified as a stale handle. |
| Boss despawn / invalid or reused handle | The single cached boss-registry reference is revalidated at 4 Hz. Invalid/reused references cancel systems before the registry is force-cleared. No ped-pool scan is performed. |
| Script reload/unload | `Runtime::Shutdown()` and the destructor retain reverse-order cancellation/restoration. |
| Model timeout | `ModelStreamRequest` releases the request and returns a timeout; a model already loaded is reused without issuing another request. |
| VFX unavailable | Shadow smoke remains non-looped and best-effort. The asset is requested during controller setup/reload, not once per failed particle play. Missing VFX never blocks relocation or cleanup. |
| Interrupted feed | Reverse cancellation runs `FeedingController::Cancel()` and releases only Nightwalker-owned task state. |
| Interrupted Shadowstep | Controller presentation watchdog restores owned visibility/appearance before reset. |
| Interrupted grab/throw/combat feed | `VampireCombatController` restores movement overrides and clears only its owned actor/target tasks. |
| Config feature disable / F10 reload | Active systems are cancelled before the new configuration is applied. Per-controller feature gates remain a second line of defense. |

The runtime deliberately does not guess a cutscene, loading-screen, or fast-travel native. The existing verified player/control/mission signals plus pure continuity checks provide a conservative fail-safe without expanding the native-risk surface.

## Performance audit

Phase 13 keeps the hot path bounded:

- there is no full-ped or full-world scan in Runtime;
- the encounter director keeps its configured eligibility poll (500 ms default) and spawn retry cadence;
- vampire AI keeps its configured decision interval (180 ms default); expensive target-relative Shadowstep planning only runs at a decision point;
- Shadowstep ground/raycast/clearance work only runs while a step is being resolved or while its short arrival carry is validated;
- the boss registry is a cached handle and is revalidated every 250 ms, not rediscovered;
- model streaming does not repeat `REQUEST_MODEL` when the model is already loaded;
- smoke is non-looped; failed `PlayShadowSmoke` calls no longer re-request the PTFX asset every play attempt;
- no bat-ped swarm exists in the current code path; the legacy VFX INI keys do not create entities;
- Runtime profiling uses a fixed 16-slot array and is completely opt-in (`[Debug] ProfileRuntime=true`); it logs update count, average microseconds and max microseconds every 10 seconds.

No Phase 13 path allocates a per-frame profiling container. Existing logging and controller strings can still allocate when a diagnostic is actually emitted; normal release gameplay does not manufacture profiling log strings with profiling disabled.

## Compatibility boundaries

Nightwalker remains a code-first ASI. It does not overwrite vanilla RDR2 files and does not require an LML replacement pack for Phase 13. Runtime data is clearly namespaced beside the plugin:

- `Nightwalker.ini` — owner-authored configuration;
- `Nightwalker.state` — mod-owned persistence only;
- `Nightwalker.log` — diagnostics;
- `Nightwalker.dialogue` — original/mod-owned narrative text.

Known compatibility risks:

1. Another script can delete or repurpose the Nightwalker-owned vampire handle. Phase 13 detects this from the cached registry reference and drops ownership safely rather than deleting an entity whose model no longer matches.
2. Another mod can change ped AI/tasks/movement while Nightwalker owns the same actor. Nightwalker restores only state it explicitly owns; it does not globally reset player or world attributes.
3. Extremely large third-party teleports (>120 m in one tick) are treated like a world transition. Nightwalker intentionally aborts active state rather than trying to continue a grab/Shadowstep across that discontinuity.
4. Script stalls over 2.5 seconds trigger conservative cleanup. The player may see an encounter abort, but should not be left invisible, speed-modified or attached.
5. Particle assets can be unavailable on a given build/mod stack. Smoke is presentation-only; the gameplay state machine continues and cleanup remains authoritative.

## HUD regression lock

The temporary red Saint Denis boss health bar remains the only custom combat HUD. Phase 13 adds no player blood/hunger meter, cooldown bar, ability icon/card, phase label, power name, weakness display, floating damage number, combo counter or skill wheel.

CI now enforces the low-level custom-drawing boundary: direct rectangle/background-text draw-native use under `src/` is allowed only inside `GameBossBarApi.cpp`. Gameplay controllers cannot quietly add a second custom combat renderer.

Narrative subtitles continue through the already isolated narrative presentation path and do not expose combat abilities, phases, weaknesses or player resources.

## Target-environment verification

Automated tests prove the pure recovery policy and cached-model behavior, but RDR2-specific transitions still require Story Mode verification:

1. Free-roam for at least 30 minutes with repeated Saint Denis encounter starts, exits and aborts; watch `Nightwalker.log` for cleanup failures.
2. Trigger player death during Shadowstep, feed and grab/throw states; respawn and verify visibility, movement, controls and target tasks are normal.
3. Enter/leave a Story mission and a cutscene while an encounter is active; verify boss HUD/narrative/transient state disappear and later gameplay resumes only after the safety delay.
4. Fast travel or load a save during an active encounter; verify cleanup happens once, no stale boss remains, and a later valid encounter can start.
5. With a development helper/mod, delete the Nightwalker-owned boss during combat; verify the cached-handle check aborts safely and the red boss bar disappears.
6. Repeat debug F8/F9 spawn/despawn cycles; verify no duplicate boss and no accumulating model requests.
7. Make the smoke asset unavailable/disabled; Shadowstep must still relocate/telegraph/clean up without a particle loop.
8. Toggle `[Debug] ProfileRuntime=true`, play for at least one report interval, and confirm per-system update/average/max timing is logged without any profiler HUD.
9. Inspect combat UI throughout: the temporary red boss bar is the only custom combat HUD.

A final x64 ASI link still requires the developer-local Script Hook RDR2 SDK and an installed Story Mode target for in-game validation.
