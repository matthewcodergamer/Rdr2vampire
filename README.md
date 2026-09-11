# RDR2 Vampire — Nightwalker

A single-player **Red Dead Redemption 2** vampire gameplay mod inspired by predatory vampire movement systems such as *The Blood of Dawnwalker*, while using RDR2's own world, native scripting, ped/animation systems, and original mod-created logic.

## Vision

Turn RDR2's existing Saint Denis vampire concept into a full gameplay system:

- Short-range **Shadowstep** / blink movement with a vanish → reposition → arrival flow.
- Supernatural sprint, combat gap-closing, evasive movement, smoke and bat effects.
- Blood hunger, feeding, healing and optional non-lethal feeding.
- Night-focused vampire state and progression.
- A reusable vampire AI controller for encounters and boss fights.
- A scripted Saint Denis night encounter using RDR2's existing `cs_vampire` ped.
- Original quests, dialogue and audio hooks that do **not** redistribute assets from other games.

## Target mod stack

The planned production build is a Windows single-player `.asi` plugin using **Script Hook RDR2** and the RDR2 native API. Lenny's Mod Loader can be added later if the project needs streamed replacement assets, custom metadata, textures or map content.

This project is intentionally single-player only. Script Hook RDR2 does not support RDR Online.

## Development status

**Phase 0 — architecture / research.** The repository is being bootstrapped before in-game testing begins.

See [`docs/ROADMAP.md`](docs/ROADMAP.md) and [`docs/SHADOWSTEP.md`](docs/SHADOWSTEP.md) once the Phase 0 branch is merged.

## Asset policy

This repository should contain only original code/configuration and assets we have permission to distribute. RDR2 assets should be referenced at runtime rather than copied into the repository. Dialogue, music, voice acting, animations or other copyrighted files from *The Blood of Dawnwalker* should not be extracted or redistributed; the mod can recreate the *mechanical idea* with original implementation and original performances.

## Disclaimer

Fan-made, non-commercial mod project. Not affiliated with Rockstar Games, Take-Two Interactive, Rebel Wolves, or Bandai Namco Entertainment.
