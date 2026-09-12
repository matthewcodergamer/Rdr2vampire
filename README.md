# NIGHTWALKER

> **Story Mode only.** Nightwalker does not target RDR Online.

Nightwalker is a native C++ Red Dead Redemption 2 vampire mod project. It references content already present in the player's legitimate RDR2 installation at runtime.

## Status

**Phase 12: original narrative scaffolding.** The existing Saint Denis encounter, temporary red boss-health bar, persistence, boss Shadowstep AI, movement, feeding and physical combat remain intact.

Phase 12 adds a mod-owned `Nightwalker.dialogue` format and a cancellable `NarrativeController`. Gameplay code calls stable sequence IDs; story text and optional audio IDs remain data-driven.

The live encounter now uses:

- `saint_denis.pre_fight` when Stalking becomes Confrontation;
- `saint_denis.post_defeat` when the encounter boss dies.

Pre-fight text can hold combat only for a bounded interval. Post-fight text can hold resolution only for a bounded interval. Abort, player/Story Mode unsafe transitions, F10/F11 and unload cancel narrative immediately, so missing narrative data can never block combat cleanup.

Optional audio is an isolated interface. Phase 12 intentionally ships a subtitle-only implementation, so subtitles remain the guaranteed fallback and no unverified sound identifiers are required.

`content/Nightwalker.dialogue` includes original confrontation, defeat, clue and future alternate-outcome hooks. If the external file is missing or rejected, the built-in original subtitle catalog is used.

There is still **no** player blood/hunger meter, player-health replacement, stamina replacement, Shadowstep cooldown bar, skill wheel, ability card, move list, boss phase text, power name, weakness panel, floating damage number, combo counter, or status-icon row. The temporary red Saint Denis boss-health bar remains the only approved custom combat HUD.

## Build

Use Visual Studio 2022 with the Desktop C++ workload, a Windows SDK, and the external Script Hook RDR2 developer SDK. Keep SDK files local under `third_party/ScriptHookRDR2` or override the SDK-root MSBuild property.

Open `Nightwalker.sln` and build `Debug | x64` or `Release | x64`. The target output is `Nightwalker.asi` under `bin/<Configuration>/`. Local builds copy `content/Nightwalker.dialogue` beside the ASI when the source file is present.

SDK-independent test executables now include eleven suites:

- `Nightwalker.Tests`
- `Nightwalker.Shadowstep.Tests`
- `Nightwalker.Presentation.Tests`
- `Nightwalker.Targeting.Tests`
- `Nightwalker.Movement.Tests`
- `Nightwalker.Feeding.Tests`
- `Nightwalker.Combat.Tests`
- `Nightwalker.Encounter.Tests`
- `Nightwalker.BossHud.Tests`
- `Nightwalker.SaveData.Tests`
- `Nightwalker.Narrative.Tests`

The narrative suite covers schema parsing, built-in sequence presence, subtitle wrapping, playback/skip/watchdog behavior, and settings clamps. GitHub Actions runs deterministic tests on Windows/MSBuild and Linux/g++, then syntax-compiles controllers and Runtime composition on Linux. Public CI intentionally does not link the final ASI because that requires the developer-local SDK and RDR2 Story Mode environment.

See `docs/NARRATIVE.md`, `docs/PERSISTENCE.md`, `docs/BUILDING.md`, `docs/ENCOUNTER.md`, and `docs/BOSS_HEALTH_BAR.md`.

## Narrative defaults

```ini
[Narrative]
Enabled=true
MaxConfrontationHoldMs=6000
MaxSequenceMs=9000
SkipKey=0x0D
OptionalAudio=true
```

Enter skips the current subtitle line only while a narrative sequence is active.

## Debug controls

Set `[Debug] Enabled=true` only for development harness actions. Production encounter, narrative, boss bar and persistence do not require Debug mode.

- **F1** — heavy strike debug harness.
- **F2** — short grab/control debug harness.
- **F3** — physical release/throw follow-up.
- **F4** — combat feed follow-up/direct stagger test.
- **F5/F6** — Sip/Drain feed tests.
- **F7** — player-side Shadowstep safety harness.
- **F8** — spawn a debug `cs_vampire` only when the authoritative boss registry is free.
- **F9** — despawn only a debug-owned vampire; ignored during the real encounter.
- **F10** — clean transient state, checkpoint progression, reload INI/dialogue data, and reapply saved tuning.
- **F11** — global cleanup.

## Ownership boundaries

- `SaintDenisDirector` owns encounter timing and narrative event hooks.
- `NarrativeController` owns one temporary narrative sequence.
- `NarrativeScript` owns schema parsing and stable sequence/line/text/audio IDs.
- `GameNarrativeAudioApi` is the optional-audio boundary; the Phase 12 implementation is subtitle-only.
- `GameBossBarApi` supplies the already verified literal text/rectangle drawing surface used for subtitles and the separate boss bar.
- `SaveData` / `ProgressionController` own mod state and never patch RDR2 save files.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text. Temporary narrative subtitles do not authorize a player power HUD. The temporary red Saint Denis boss-health bar remains Nightwalker's **only** custom combat HUD element.
