# Narrative scaffolding

Phase 12 adds temporary original subtitles around the Saint Denis encounter. `docs/DESIGN_LOCKS.md` remains authoritative; this does not add a new persistent combat HUD.

## Ownership

`SaintDenisDirector` starts stable narrative sequence families. `NarrativeController` owns one temporary sequence at a time. Text, speaker labels, text IDs, optional audio IDs and durations live in `Nightwalker.dialogue`, not in combat logic.

Live hooks:
- `saint_denis.pre_fight` during Confrontation. This is now a **sequence family**.
- `saint_denis.post_defeat` after boss death.

Clue and alternate-outcome IDs are present as future hooks but Phase 12 does not add a choice mechanic.

## Coherent variation

A sequence family includes the exact family ID and child IDs separated by a dot. Example:

```text
saint_denis.pre_fight
saint_denis.pre_fight.soul_02
saint_denis.pre_fight.soul_03
```

The director asks `NarrativeController` for the family, not a specific variant. `NarrativeVariantSelector` then uses a non-repeating shuffle bag:

- a **whole authored sequence** is selected at once;
- individual lines are never shuffled;
- every available variant is used before the bag refills;
- the first selection after a refill cannot immediately repeat the previously played variant when multiple choices exist.

This keeps dialogue surprising across repeated encounters without destroying sentence-to-sentence flow.

## Voice direction

The Saint Denis vampire is written as calm, ancient, self-possessed and predatory. His speech should feel older through worldview and construction rather than fake Shakespearean vocabulary.

The desired dramatic traits are:
- metaphysical contradiction rather than ordinary exposition;
- moral inversion rather than simple villain boasting;
- compact observations about mortality, hunger, faith and time;
- quiet amusement instead of constant growling;
- anger becomes colder and quieter, not louder;
- silence remains part of the performance.

Nightwalker may study the *dramatic grammar* of other vampire fiction, but it must not copy another game's dialogue, recordings, character identity or actor performance. All shipped Nightwalker dialogue and voice assets must remain original/licensed.

## Format

```text
schema=1
line=<sequence-id>|<line-id>|<speaker>|<text-id>|<audio-id>|<duration-ms>|<subtitle text>
```

A missing, corrupt or unsupported external file falls back to the built-in original subtitle catalog. Duplicate line IDs are ignored with diagnostics and durations are bounded.

## Safety

Pre-fight text may delay combat only until `MaxConfrontationHoldMs`. Post-fight text may delay resolution only until `MaxSequenceMs`. Encounter abort, F10/F11, unsafe Story Mode transition and unload cancel narrative immediately.

Optional audio is isolated behind `IGameNarrativeAudioApi`. The current backend remains subtitle-first, so unavailable audio can never block encounter flow or cleanup.

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

1. Reach Confrontation repeatedly and confirm different complete pre-fight variants are selected.
2. Confirm every line within a selected variant remains in authored order.
3. Confirm no variant repeats until the current family bag has been exhausted.
4. Confirm the first variant after refill does not immediately repeat the previous one.
5. Test normal playback and Enter skip.
6. Confirm combat begins after playback or after the hard watchdog.
7. Kill the boss and verify bounded post-fight text.
8. Abort during a sequence and verify immediate subtitle cleanup.
9. Remove the external dialogue file and verify the built-in variation catalog.
10. Confirm no new persistent combat HUD appears.
