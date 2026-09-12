# Nightwalker — Current Roadmap

`docs/DESIGN_LOCKS.md` is authoritative. This roadmap reflects the implemented repository as of `1.0.0-rc1`; the original zero-to-release planning document is preserved as `docs/ROADMAP_LEGACY.md` for historical context.

If the legacy roadmap conflicts with `DESIGN_LOCKS.md`, do not implement the legacy item. In particular, the old player blood/hunger HUD, cooldown meter, ability cards/icons, skill wheel, boss phase labels/power names, and similar custom RPG UI are superseded. The temporary red Saint Denis boss-health bar remains the only custom combat HUD.

## Completed implementation phases

- **Foundation/runtime:** native Windows x64 Script Hook RDR2 `.asi` project, config, logger, lifecycle, debug input and fail-safe cleanup.
- **Game/native boundaries:** explicit entity/model ownership, local `cs_vampire` debug spawning, model timeouts and safe cleanup.
- **Shadowstep safety:** reusable destination resolver with ground, obstruction, vertical and water validation.
- **Shadowstep presentation:** short disappearance, instant validated reposition, compact smoke, arrival carry and attack opportunity.
- **Vampire Shadowstep AI:** intercept/flank/behind candidate planning, readable telegraph, recovery and anti-spam cooldowns.
- **Movement:** bounded supernatural move-rate control with watchdog restoration.
- **Feeding:** candidate validation, Sip/Drain state machine, hidden resource, health result and owned-task cleanup.
- **Physical combat:** strikes, short grapple/control, throw/release and combat-feed state machines.
- **Saint Denis encounter:** explicit boss ownership, eligibility/stalking/confrontation/combat/resolution/abort flow and duplicate prevention.
- **Boss health bar:** temporary cinematic red boss bar with inactivity fade/re-engagement and bounded death hold.
- **Persistence/progression:** separate versioned `Nightwalker.state`, migration/clamping/recovery and bounded tuning consumption.
- **Original narrative:** subtitle-first original dialogue scaffolding with watchdogs and optional audio seam.
- **Hardening:** long-session discontinuity recovery, throttled boss validation, model/VFX hardening and debug profiler logging.
- **Release candidate engineering:** `1.0.0-rc1`, release audit, deterministic packaging allowlist, build docs and target-test matrix.

## Current post-RC work

### Rockstar-first feeding presentation

- Preserve the game's paired `TASK_GRAPPLE` interaction through combat-feed holds rather than replacing it immediately with stand-still tasks.
- Use stationary hold only as a bounded fallback.
- Keep front/rear styled-grapple support behind a verified native boundary; do not guess parameters.
- The vanilla Saint Denis corpse feeding AnimScene is verified/documented but is not forced onto arbitrary living ambient peds.
- Required target tests are documented in `docs/research/VAMPIRE_FEED_REUSE.md`.

### Release-candidate verification

The largest remaining gate is real RDR2 Story Mode testing of the exact locally linked RC binary. `docs/RELEASE_TEST_MATRIX.md` is the authoritative checklist.

Priority cases include:

- Arthur and John;
- dense Saint Denis streets, cathedral/church area and alleys;
- forest/open plains/slopes/stairs/water edges;
- mounted/unmounted transitions;
- wanted level and nearby missions;
- player death during every major transient mechanic;
- boss death during special movement;
- keyboard/mouse and controller;
- low/high frame rate;
- repeated/long-session use with no accumulating broken state;
- paired feeding on varied ambient peds with interruption cleanup.

## Research still required before production expansion

These are research gaps, not permission to guess native contracts or asset names:

- a verified standing two-person neck/bite animation or AnimScene suitable for arbitrary live humans;
- exact target-tested contract for direct front/rear grapple placement if the generic grapple is not visually sufficient;
- suitable original/licensed or verified runtime sound cues for Shadowstep/feeding;
- optional lightweight bat accent path;
- any custom claw/throat-lift/bite animation work only after verified reuse options are exhausted.

## Final 1.0 gate

Do not promote `1.0.0-rc1` to final `1.0.0` merely because CI is green. Final 1.0 requires:

1. a genuine locally linked `Nightwalker.asi` built with the developer's legitimate Script Hook RDR2 SDK;
2. the hands-on Story Mode matrix completed enough to support the compatibility claim;
3. no cleanup regression in Shadowstep, movement, feeding, combat, encounter, narrative or boss HUD;
4. no duplicate boss or stale entity ownership;
5. the boss health bar still being the only custom combat HUD;
6. clean release ZIP contents and installation documentation;
7. no Online support and no redistributed proprietary Rockstar/Dawnwalker content.

## Working rule for the next agent

Read, in order:

1. `docs/DESIGN_LOCKS.md`
2. this file
3. `docs/ARCHITECTURE.md`
4. `docs/SOURCE_LAYOUT.md`
5. the mechanic-specific document for the task
6. `docs/HARDENING.md`
7. `docs/RELEASE_TEST_MATRIX.md`

Implement the smallest coherent verified change, run the relevant automated gates, report what remains in-game-only, commit it, and stop rather than automatically beginning another feature.
