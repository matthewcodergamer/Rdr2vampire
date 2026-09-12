# Phase 14 — Release Candidate Test Matrix

`docs/DESIGN_LOCKS.md` is authoritative. Phase 14 adds no gameplay features. This document separates what automation can actually prove from what still requires a real RDR2 Story Mode session.

## Version decision

Current candidate: **1.0.0-rc1**.

Nightwalker is not promoted to final `1.0.0` by Phase 14 automation alone. Final 1.0 requires the target-environment rows below to be exercised enough to support a stable-release claim, with no unresolved severity-1/2 cleanup, crash, save, ownership or HUD regressions.

## Automated release gates

| Area | Evidence | RC status |
| --- | --- | --- |
| Foundation/config/recovery | `Nightwalker.Tests` | PASS in CI when release commit is green |
| Shadowstep landing safety | `Nightwalker.Shadowstep.Tests` | PASS in CI when release commit is green |
| Shadowstep presentation | `Nightwalker.Presentation.Tests` | PASS in CI when release commit is green |
| Target-relative Shadowstep | `Nightwalker.Targeting.Tests` | PASS in CI when release commit is green |
| Movement reset logic | `Nightwalker.Movement.Tests` | PASS in CI when release commit is green |
| Feeding state/resource logic | `Nightwalker.Feeding.Tests` | PASS in CI when release commit is green |
| Combat/throw math | `Nightwalker.Combat.Tests` | PASS in CI when release commit is green |
| Encounter state/registry | `Nightwalker.Encounter.Tests` | PASS in CI when release commit is green |
| Boss HUD fade/death/layout | `Nightwalker.BossHud.Tests` | PASS in CI when release commit is green |
| Save migration/corruption recovery | `Nightwalker.SaveData.Tests` | PASS in CI when release commit is green |
| Narrative parse/playback/watchdogs | `Nightwalker.Narrative.Tests` | PASS in CI when release commit is green |
| Controller/runtime compilation | Linux syntax compilation against stable interfaces/stubs | PASS in CI when release commit is green |
| Custom HUD boundary | direct draw-native allowlist | PASS in CI when release commit is green |
| Story Mode source policy | `scripts/release_audit.py` rejects network-native use and requires Story-Mode compile/version flags | PASS in CI when release commit is green |
| Third-party/package hygiene | `scripts/release_audit.py` rejects tracked build outputs, SDK payloads and disallowed release content | PASS in CI when release commit is green |
| Release packaging | `scripts/package-release.ps1` uses an explicit allowlist and validates the local Release ASI before zipping | LOCAL SDK BUILD REQUIRED |

## Required Story Mode matrix

The rows below must be tested against the same RC binary intended for release. Record game build, Script Hook RDR2 version, graphics API/settings, average FPS band and input device in test notes.

| Case | Required checks | Status before target test |
| --- | --- | --- |
| Arthur | encounter start/combat/abort/death; narrative; HUD; cleanup | PENDING TARGET TEST |
| John | same core encounter/cleanup coverage as Arthur | PENDING TARGET TEST |
| Saint Denis dense streets | safe Shadowstep candidates, no crowd/prop embedding, no duplicated boss | PENDING TARGET TEST |
| Church/cathedral encounter zone | full omen -> confrontation -> combat -> resolution and abort paths | PENDING TARGET TEST |
| Alleys | flank/intercept rejection near walls; no unsafe landing | PENDING TARGET TEST |
| Forest | uneven ground/vegetation handling; safe abort on invalid candidate | PENDING TARGET TEST |
| Open plains | full-range Shadowstep/AI cadence; no unnecessary aborts | PENDING TARGET TEST |
| Steep slopes/stairs | ground/vertical validation; no under-world landing | PENDING TARGET TEST |
| Water edges | deep/unsafe water rejection; dry landing remains stable | PENDING TARGET TEST |
| Mounted -> unmounted | movement rate/AI state resets and encounter recovery | PENDING TARGET TEST |
| Unmounted -> mounted | same cleanup/reset checks | PENDING TARGET TEST |
| Wanted level active | law AI plus vampire encounter does not leave persistent ownership/task state | PENDING TARGET TEST |
| Nearby Story Mode mission | encounter refuses/aborts safely around unsafe mission state | PENDING TARGET TEST |
| Player death during player Shadowstep debug harness | visibility/presentation restored; no stuck state | PENDING TARGET TEST |
| Player death during feeding | target released; tasks restored; no persistent hold | PENDING TARGET TEST |
| Player death during grab/throw/combat feed | both actors recover; no owned task/motion state remains | PENDING TARGET TEST |
| Player death during boss encounter | boss/HUD/narrative/transients clean and later gameplay resumes | PENDING TARGET TEST |
| Boss death during Shadowstep/special movement | appearance/state cleanup; HUD zero/hold/fade; no stale registry | PENDING TARGET TEST |
| Low frame rate | state timeouts/telegraph/cleanup remain safe; no time-based soft lock | PENDING TARGET TEST |
| High frame rate | no update-frequency spam/duplicate transition; HUD timing remains correct | PENDING TARGET TEST |
| Keyboard/mouse | narrative skip + debug harness when explicitly enabled; normal gameplay unaffected | PENDING TARGET TEST |
| Controller | normal combat/encounter remains functional; no input ownership leak | PENDING TARGET TEST |
| Save/load during encounter | one cleanup, no stale boss/HUD, later encounter valid | PENDING TARGET TEST |
| Fast travel during encounter | discontinuity cleanup and stable-state quarantine work | PENDING TARGET TEST |
| Config feature disable while active | active mechanic cleans before disable takes effect | PENDING TARGET TEST |
| Script unload/reload | no invisible/speed-modified/held actor persists | PENDING TARGET TEST |

## Core regression checklist

1. **Shadowstep never leaves player invisible.** Automated presentation/recovery policy exists; final proof requires death/load/F10/F11 target tests during hidden transit.
2. **Collision always restores.** Current Shadowstep does not deliberately disable entity collision and the release audit rejects introducing a direct collision-toggle native without revisiting this matrix. Target tests still verify physical control after every abort path.
3. **Movement multiplier always resets.** Deterministic movement/controller cleanup is covered; target tests verify death/mount/cutscene/load cases.
4. **Feed target always releases/detaches.** Feeding cancellation clears Nightwalker-owned task state; no direct entity-attachment native is currently used. Target tests remain required.
5. **Throw/grab cannot leave target attached.** Current implementation uses grapple/tasks/ragdoll rather than persistent entity attachment; cancellation and target validity are still tested in-game.
6. **Boss cannot duplicate.** `BossActorRegistry` is the single authority and encounter/debug ownership is mutually exclusive; repeated start/abort and F8/F9 target cycles remain required.
7. **Encounter abort cleans VFX/entities.** Model requests, non-looped smoke, narrative, HUD and owned boss cleanup converge on encounter/runtime cancellation; target verification remains required.
8. **Boss health bar fades/reappears correctly and never becomes permanent.** `BossHudModel` deterministic tests cover state transitions; full combat/death/abort behavior is also an in-game gate.
9. **No boss abilities/phases are exposed by UI.** Design lock plus source draw-boundary audit are automated release gates.
10. **Save/config corruption fails gracefully.** Save migration/backup recovery and config parse/clamp tests are automated; a corrupt-file launch smoke test remains recommended.
11. **Mod does not target RDR Online.** Story-Mode compile/version flags and source audit reject network-native use; README/package also state Story Mode only.
12. **No proprietary Dawnwalker assets are included.** Release audit restricts tracked release content and rejects Dawnwalker-named tracked payloads; third-party notices document the inspiration boundary.
13. **Release ZIP contains no developer SDK paths, logs or build junk.** Packaging is allowlist-only, Release PDB generation is disabled, and the packager performs a final staged-file/path scan before compression.

## Release package acceptance

Expected artifact name:

`Nightwalker-1.0.0-rc1-win64.zip`

Expected archive structure:

```text
Nightwalker/
  Nightwalker.asi
  Nightwalker.ini
  Nightwalker.dialogue
  README.md
  CHANGELOG.md
  THIRD_PARTY_NOTICES.md
```

The archive must not contain `Nightwalker.log`, `Nightwalker.state`, PDB/OBJ/LIB/DLL files, the local Script Hook RDR2 SDK, repository metadata, CI files, source trees, build directories, or an optional LML pack.

## Final 1.0 promotion gate

Before changing `1.0.0-rc1` to `1.0.0`:

1. Run the required Story Mode matrix on the exact release candidate binary.
2. Repeat encounter start/abort loops and the core cleanup regressions after any code fix.
3. Confirm Arthur and John both complete the encounter without character-specific breakage.
4. Confirm keyboard/mouse and controller sessions.
5. Confirm at least one low-FPS and one high-FPS session where timing behavior remains safe.
6. Confirm wanted-level and nearby-mission behavior does not corrupt vanilla AI/mission state.
7. Inspect `Nightwalker.log` for cleanup/model/entity errors after an extended session.
8. Build `Release | x64` with the local developer SDK and inspect the generated ZIP contents.
9. Re-run CI on the exact commit being tagged/released.
10. Promote to final `1.0.0` only if no unresolved release-blocking issue remains.
