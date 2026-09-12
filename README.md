# NIGHTWALKER

> **Release candidate 1.0.0-rc1 — Story Mode only. Nightwalker does not target RDR Online.**

Nightwalker is a native C++ Red Dead Redemption 2 Story Mode mod that expands the existing Saint Denis vampire into a cinematic supernatural encounter. It references Rockstar-authored content already present in the player's legitimate RDR2 installation at runtime rather than redistributing game assets.

The signature boss movement is Shadowstep: a very short disappearance, validated instant reposition, compact dark arrival smoke, a small forward carry, and then a readable attack startup. It is a teleport/reposition mechanic, not extreme running speed.

## Release status

`1.0.0-rc1` is a release candidate, not a final 1.0 declaration. The deterministic Windows/Linux regression suites and source/package policy checks are release-gated in CI, but the full Arthur/John, controller, wanted-level, terrain, mission-adjacent and low/high-frame-rate matrix still requires hands-on RDR2 Story Mode verification. See `docs/RELEASE_TEST_MATRIX.md`.

The temporary red Saint Denis boss-health bar remains the **only custom combat HUD**. Nightwalker does not add a player blood/hunger meter, Shadowstep cooldown bar, ability cards/icons, boss phase labels, power names, weakness panels, floating damage numbers, combo counters or a skill wheel.

## Dependencies

- Red Dead Redemption 2 for Windows with Story Mode.
- A compatible Script Hook RDR2 runtime/ASI loader installed by the user from a trusted source.
- For developers building from source: Visual Studio 2022, Desktop C++ workload, Windows SDK, and the Script Hook RDR2 developer SDK.

Nightwalker does not bundle Script Hook RDR2, Rockstar assets, LML, Dawnwalker assets, or third-party mod binaries.

## Installation

1. Install and verify your compatible Script Hook RDR2 runtime/ASI loader.
2. Extract `Nightwalker-1.0.0-rc1-win64.zip`.
3. Open the included `Nightwalker/` folder.
4. Copy `Nightwalker.asi`, `Nightwalker.ini`, `Nightwalker.dialogue`, `README.md`, `CHANGELOG.md`, and `THIRD_PARTY_NOTICES.md` into the RDR2 game directory used by your ASI loader.
5. Launch **Story Mode**.
6. Check `Nightwalker.log` after first launch if the mod does not initialize as expected.

No LML package is required for this release candidate. If optional LML content is introduced later, it will be distributed separately and documented separately.

## Uninstall

1. Exit RDR2 completely.
2. Remove `Nightwalker.asi`, `Nightwalker.ini`, `Nightwalker.dialogue`, `Nightwalker.log`, and `Nightwalker.state` from the location where you installed Nightwalker.
3. Remove any future optional Nightwalker LML pack separately if you installed one.

Nightwalker does not modify RDR2's own save files or overwrite vanilla game files.

## Default controls

Production gameplay requires no custom ability keybinds. The Saint Denis encounter and boss behavior run automatically when their Story Mode conditions are met.

- **Enter** — while a Nightwalker narrative subtitle sequence is active, advance/skip the current subtitle line.

Development/debug keys are disabled by default and only work when `[Debug] Enabled=true`:

- **F1** heavy strike test.
- **F2** short grab/control test.
- **F3** release/throw follow-up.
- **F4** combat-feed follow-up/direct stagger test.
- **F5 / F6** sip/drain feed tests.
- **F7** player-side Shadowstep safety harness.
- **F8 / F9** spawn/despawn the debug-owned vampire.
- **F10** safe config/dialogue reload.
- **F11** global cleanup.

These debug controls are not part of the normal player HUD or combat interface.

## Configuration

`Nightwalker.ini` contains backwards-compatible settings for the encounter, Shadowstep safety/presentation, vampire AI, movement, feeding, combat, narrative, boss HUD and debug diagnostics.

Important defaults:

```ini
[General]
Enabled=true
DebugMode=false

[Debug]
Enabled=false
ProfileRuntime=false

[BossHUD]
Enabled=true
DisplayName=THE VAMPIRE
IdleSeconds=6.0
FadeSeconds=0.35
DeathHoldSeconds=1.25
ShowNumericHealth=false
```

`ProfileRuntime=true` only has an effect when Debug mode is also enabled. It logs fixed-slot per-system timing summaries and never draws a profiler HUD.

`Nightwalker.state` is mod-owned persistence. It is separate from RDR2 save data. Corrupt state/config input is handled conservatively with defaults, validation, logging and backup recovery where applicable.

## Troubleshooting

`Nightwalker.log` is written beside the plugin when file logging is available. It is the first place to check for initialization, configuration, model-streaming, cleanup or encounter diagnostics.

If the mod does not load:

1. confirm you launched Story Mode, not RDR Online;
2. confirm your Script Hook RDR2 runtime matches your current RDR2 build;
3. confirm `Nightwalker.asi` is in the directory scanned by your ASI loader;
4. temporarily restore the shipped `Nightwalker.ini` defaults;
5. inspect `Nightwalker.log` for a specific failure reason;
6. remove other gameplay mods temporarily if you suspect an ownership/task conflict.

If an encounter aborts after a load, fast travel, long script stall or large third-party teleport, that is intentional fail-safe behavior: Nightwalker prefers cleanup over carrying an active grab/Shadowstep across a discontinuous world state.

## Compatibility notes

- Story Mode only; no multiplayer/network hooks are implemented.
- Code-first ASI release; no vanilla file replacement is required.
- The boss registry owns one explicit Nightwalker vampire reference; no full ped-pool scan is used for encounter ownership.
- Another script can still delete, retask or replace an entity Nightwalker owns. The runtime validates ownership and fails safe rather than deleting a mismatched handle.
- Nightwalker restores only temporary state it explicitly owns. It does not globally reset unrelated player/world attributes from other mods.
- Dense crowds, custom teleports, AI-overhaul mods and mission scripting can change the environment around validated Shadowstep candidates; these remain important target-environment compatibility cases.

See `docs/HARDENING.md` and `docs/RELEASE_TEST_MATRIX.md` for the detailed recovery and RC verification matrix.

## Copyright and original-asset statement

Nightwalker source code and the shipped Nightwalker dialogue are project-owned/original mod content unless a file states otherwise. Red Dead Redemption 2, Rockstar Games, and related game content are property of their respective rights holders.

Nightwalker references the existing RDR2 `cs_vampire` model and selected Rockstar-authored runtime effects/behaviors from the user's installed game. Those assets are **not** redistributed in this repository or release package.

Nightwalker may take inspiration from the broad combat grammar of modern vampire games, including rapid vanish/reposition movement, but it does **not** ship copied Dawnwalker code, character models, animations, music, voice recordings, dialogue, textures or other proprietary assets.

See `THIRD_PARTY_NOTICES.md` for dependency and redistribution boundaries.

## Known limitations

- This RC is not promoted to final 1.0 until the hands-on Story Mode release matrix is complete enough to support that claim.
- The public repository/CI cannot link a genuine `Nightwalker.asi` because the Script Hook RDR2 developer SDK is intentionally not committed. A developer with the local SDK must perform the final Release x64 link/package step.
- Exact game-build/Script Hook compatibility must be rechecked when RDR2 or Script Hook RDR2 updates.
- Dense ambient crowds remain a manual Shadowstep occupancy test because the current resolver intentionally avoids an expensive broad ped scan and does not guess unverified trace flags.
- Optional audio remains subtitle-first/fallback-safe; this RC does not bundle proprietary game audio.
- No optional LML content pack is included in `1.0.0-rc1`.

## Building from source

See `docs/BUILDING.md`. A local `Release | x64` build with the required developer SDK produces `bin/Release/Nightwalker.asi` and then packages the allowlisted release files as:

`artifacts/Nightwalker-1.0.0-rc1-win64.zip`

The release packager rejects a missing/non-PE plugin and copies only the explicit release allowlist, preventing logs, PDBs, SDK files and general build junk from entering the ZIP.

## Design authority

`docs/DESIGN_LOCKS.md` overrides older planning text. In particular, older roadmap sections that discuss player power HUDs, phase labels or broader RPG UI do not authorize those features. The temporary red Saint Denis boss-health bar remains Nightwalker's only custom combat HUD.
