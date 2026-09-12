# Narrative scaffolding

Phase 12 adds temporary original subtitles around the Saint Denis encounter. `docs/DESIGN_LOCKS.md` remains authoritative; this does not add a new persistent combat HUD.

## Ownership

`SaintDenisDirector` starts stable narrative sequence IDs. `NarrativeController` owns one temporary sequence at a time. Text, speaker labels, text IDs, optional audio IDs and durations live in `Nightwalker.dialogue`, not in combat logic.

Live hooks:
- `saint_denis.pre_fight` during Confrontation.
- `saint_denis.post_defeat` after boss death.

Clue and alternate-outcome IDs are present as future hooks but Phase 12 does not add a choice mechanic.

## Format

```text
schema=1
line=<sequence-id>|<line-id>|<speaker>|<text-id>|<audio-id>|<duration-ms>|<subtitle text>
```

A missing, corrupt or unsupported external file falls back to the built-in original subtitle catalog. Duplicate line IDs are ignored with diagnostics and durations are bounded.

## Safety

Pre-fight text may delay combat only until `MaxConfrontationHoldMs`. Post-fight text may delay resolution only until `MaxSequenceMs`. Encounter abort, F10/F11, unsafe Story Mode transition and unload cancel narrative immediately.

Optional audio is isolated behind `IGameNarrativeAudioApi`. Phase 12 ships a subtitle-only backend, so unavailable audio can never block encounter flow or cleanup.

## Configuration

```ini
[Narrative]
Enabled=true
MaxConfrontationHoldMs=6000
MaxSequenceMs=9000
SkipKey=0x0D
OptionalAudio=true
```

Enter skips the current line on a new key press. Holding Enter does not continuously advance lines.

## In-game checks

1. Reach Confrontation and verify original pre-fight subtitles.
2. Test normal playback and Enter skip.
3. Confirm combat begins after playback or after the hard watchdog.
4. Kill the boss and verify bounded post-fight text.
5. Abort during either sequence and verify immediate subtitle cleanup.
6. Remove the external dialogue file and verify built-in fallback.
7. Confirm no new persistent combat HUD appears.
