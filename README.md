# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. It references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 11: Nightwalker-owned progression and persistence.** The Phase 9 Saint Denis encounter, Phase 10 temporary red boss-health bar, boss-first Shadowstep AI, movement, feeding and physical combat remain intact. Phase 11 adds a separate versioned `Nightwalker.state` file for mod-owned state without touching RDR2's proprietary saves and without adding any progression HUD.

Persisted state includes:

- Saint Denis encounter completion and absolute game-time cooldown;
- the hidden blood/hunger value;
- progression points;
- optional unlock flags;
- bounded progression tuning multipliers.

The file uses schema version `1`. Missing files use safe defaults. Schema-0 development aliases migrate forward. A corrupt primary attempts `.bak` recovery before defaults. A newer unsupported schema disables writes for that session rather than downgrading the file.

Writes use a conservative temp/replace path: serialize to `Nightwalker.state.tmp`, rotate the previous primary to `.bak`, promote the temp file, and restore the backup if promotion fails. Runtime checkpoints are throttled and skipped when serialized state has not changed.

`ProgressionController` runs first and therefore cancels last. On player death, mission/cutscene transition, F11 cleanup, or shutdown, encounter/combat systems clean first; progression then captures the resulting cooldown/resource state. F10 checkpoints after safe encounter cleanup, reloads the clean INI, and reapplies saved tuning so multipliers do not compound.

The previously identified Dormant-cancel edge case is already fixed in the Phase 9/10 baseline: cancelling `EncounterDirector` with no actor in `Dormant` or an existing `Cooldown` does not create a new abort cooldown.

### Progression boundary

Phase 11 immediately consumes only progression that belongs to currently player/debug-owned gameplay:

- player Shadowstep debug-harness range;
- player Shadowstep debug-harness cooldown;
- feeding blood gain;
- feeding health restoration.

The schema also stores sprint, enhanced-flank, regeneration and throw-strength progression, but those are not globally applied yet. Current continuous sprint is boss-owned and the physical throw controller is shared with the boss; silently applying player progression there would strengthen the enemy. Those fields are reserved for later explicitly approved player gameplay.

Development tuning is file/config driven. Edit `Nightwalker.state` only while RDR2 is closed, then restart. F10 reloads `Nightwalker.ini` and reapplies the already loaded state; it does not re-read an externally edited state file. No F12 hotkey, skill tree, radial menu, or progression overlay is shipped.

There is still **no** player blood/hunger meter, player-health replacement, stamina replacement, Shadowstep cooldown bar, skill wheel, ability card, move list, boss phase text, power name, weakness panel, floating damage number, combo counter, or status-icon row. The temporary red Saint Denis boss-health bar remains the only approved custom combat HUD.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`.

SDK-independent test executables now include ten suites:

- `Nightwalker.Tests` — runtime/config/timing/watchdog/model-streaming regressions.
- `Nightwalker.Shadowstep.Tests` — Shadowstep destination safety.
- `Nightwalker.Presentation.Tests` — disappearance/carry settings.
- `Nightwalker.Targeting.Tests` — intercept/flank/behind planning.
- `Nightwalker.Movement.Tests` — controlled supernatural movement.
- `Nightwalker.Feeding.Tests` — feeding/resource rules.
- `Nightwalker.Combat.Tests` — physical-combat math/config.
- `Nightwalker.Encounter.Tests` — encounter math/registry/settings.
- `Nightwalker.BossHud.Tests` — boss-bar fade/re-engage/death/layout behavior.
- `Nightwalker.SaveData.Tests` — schema round trip/migration, clamps, tuning, replacement and corrupt-primary recovery.

GitHub Actions builds/runs deterministic tests on Windows/MSBuild and Linux/g++. Linux additionally syntax-compiles gameplay controllers, `ProgressionController`, Runtime composition and test-only native signature fixtures. Public CI intentionally does not link `Nightwalker.asi`; the final plugin requires the developer-local Script Hook RDR2 SDK and Windows/RDR2 Story Mode.

See `docs/PERSISTENCE.md`, `docs/BUILDING.md`, `docs/ENCOUNTER.md`, and `docs/BOSS_HEALTH_BAR.md`.

## Debug controls

Set `[Debug] Enabled=true` only for development harness actions. The production encounter, boss bar and persistence layer do not require Debug mode.

- **F1** — heavy strike debug harness.
- **F2** — short grab/control debug harness.
- **F3** — physical release/throw follow-up.
- **F4** — combat feed follow-up/direct stagger test.
- **F5/F6** — Sip/Drain feed tests.
- **F7** — player-side Shadowstep safety harness.
- **F8** — spawn a debug `cs_vampire` only when the authoritative boss registry is free.
- **F9** — despawn only a debug-owned vampire; ignored during the real encounter.
- **F10** — clean encounter/HUD/transient state, checkpoint progression, reload INI and reapply saved tuning.
- **F11** — global cleanup; progression checkpoints after gameplay cleanup through reverse lifecycle ordering.

## Native and ownership boundaries

- `GameApi` — entity/geometry/model operations.
- `GameCombatApi` — combat state and ordinary combat tasks.
- `GameEncounterApi` — clock/game-time/camera-visibility queries.
- `GamePresentationApi` — compact smoke and visibility.
- `GameMovementApi`, `GameFeedingApi`, `GamePhysicalApi` — movement/feed/physics boundaries.
- `GameBossBarApi` — normalized boss-bar rectangle/text drawing.
- `SaveData` — pure Nightwalker state codec, migration/clamps and file replacement.
- `ProgressionController` — runtime ownership of persisted resource/encounter/progression state.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text. Internal progression/resource state is allowed; it does not authorize a player power HUD. The temporary red Saint Denis boss-health bar remains Nightwalker's **only** custom combat HUD element.
