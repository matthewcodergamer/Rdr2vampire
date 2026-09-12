# Changelog

All notable Nightwalker release-candidate changes are documented here.

## 1.0.0-rc1 — 2026-09-12

### Release engineering

- Promoted the project version from the old developer placeholder to `1.0.0-rc1` while deliberately withholding final `1.0` status until the hands-on Story Mode regression matrix is complete enough to support it.
- Added a deterministic release audit covering Story-Mode-only source policy, custom-HUD draw isolation, tracked build/binary junk, local Script Hook SDK leakage, release documentation requirements, and proprietary-content filename policy.
- Added a Release x64 packaging step that creates `Nightwalker-1.0.0-rc1-win64.zip` only from an explicit allowlist and rejects a missing/non-PE `Nightwalker.asi`.
- Disabled Release PDB generation for the shipping ASI so developer debug-path metadata is not intentionally shipped with the RC package.
- Added `docs/RELEASE_TEST_MATRIX.md` with automated evidence, manual target-environment cases, blockers, and final-1.0 promotion criteria.
- Added third-party/asset redistribution notices and release installation/uninstall/troubleshooting documentation.

### Regression baseline carried into RC1

- Shared safe Shadowstep resolver for player debug harness and Saint Denis vampire AI.
- Compact non-looped smoke presentation, short disappearance, validated relocation, arrival carry and post-arrival attack telegraph.
- Supernatural movement cleanup and movement-rate restoration.
- Feeding, short grapple/control, physical release/throw and combat-feed cleanup paths.
- Saint Denis encounter ownership, persistence, original subtitle narrative and temporary red boss-health bar.
- Long-session recovery for player death, mission/control transitions, script stalls, world discontinuities, invalid/reused boss handles, model timeouts, config reload and unload.
- Boss-health HUD draw boundary: no player blood/hunger meter, cooldown bar, ability card, phase label, boss power name, weakness panel, floating damage number, combo counter or skill wheel.

### Known RC limitations

- Public CI cannot link the real `Nightwalker.asi` because the Script Hook RDR2 developer SDK is intentionally not committed.
- Arthur/John, controller, wanted-level, mission-adjacent, dense-city, forest/plains, steep terrain, water-edge, mounted transitions, and low/high-FPS cases still require hands-on RDR2 Story Mode verification before final `1.0` promotion.
- No optional LML content pack is included in RC1.

## Pre-RC development phases

Phases 0–13 established the native plugin architecture, Shadowstep safety/presentation, vampire AI, movement, feeding, combat, Saint Denis encounter, boss-health bar, persistence, original narrative scaffolding, and long-session compatibility/performance hardening. See repository history and `docs/` for phase-level details.
