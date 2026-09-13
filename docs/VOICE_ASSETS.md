# Nightwalker voice assets

Nightwalker narrative lines carry stable `audio-id` values such as `nw.audio.sd.aim.04`. The runtime resolves those IDs to external WAV files and plays them when the matching authored line starts.

## Runtime layout

Voice content is installed beside `Nightwalker.asi`:

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.voice.dialogue   # optional supplemental recorded sequences
Nightwalker.audio
audio/
  nw.audio.sd.opening.warning.wav
  nw.audio.sd.opening.wiser.a.wav
  ...
```

`Nightwalker.dialogue` remains the authoritative base catalog. If present, `Nightwalker.voice.dialogue` is parsed separately and appends only new sequence IDs. A missing, corrupt, or duplicate supplemental sequence cannot replace the base dialogue.

If a line has `audio-id=nw.audio.sd.aim.04`, the default lookup is:

```text
audio/nw.audio.sd.aim.04.wav
```

No manifest entry is required when the WAV filename matches the audio ID. `Nightwalker.audio` may provide explicit mappings for friendly uploaded filenames.

## First owner-supplied voice batch

Batch 1 contains seven owner-supplied AI voice performances normalized to 44.1 kHz, mono, 16-bit PCM WAV:

- `Come no nearer.`
- `There. You have proven yourself wiser than the last.` — two alternate performances
- `Men have given me many names.` — two alternate performances
- `None of those men lived long enough to make one matter.` — two alternate performances

On first approach, the game prefers the two-item `saint_denis.recorded_opening` family when the supplemental catalog is installed. Selecting **QUESTION** prefers the two-item `saint_denis.recorded_question` family. Each A/B exchange stays intact; individual recordings are never shuffled across performances. The existing base families remain the automatic fallback when the voice pack is absent or rejected.

Subtitle durations include a small tail buffer so playback is not clipped when the next authored line starts.

## Installing Batch 1

Copy these from `Nightwalker-Voice-Batch-1.zip` beside `Nightwalker.asi`:

- `Nightwalker.audio`
- `Nightwalker.voice.dialogue`
- the complete `audio` folder

Keep `OptionalAudio=true` under `[Narrative]` in `Nightwalker.ini`, then restart RDR2/Nightwalker. Missing clips remain subtitle-only.

The four owner-supplied `clip_*` uploads remain unassigned because their exact transcript and gameplay-trigger mapping have not yet been confirmed. They are not part of Batch 1.

## Friendly/uploaded filenames

To retain another filename, map it in `Nightwalker.audio`:

```text
schema=1
asset=nw.audio.sd.soul.01a|audio/Opening Soul 01A.wav
asset=nw.audio.sd.aim.04|audio/Aim - Your hand speaks.wav
```

Only relative `.wav` paths are accepted. Absolute paths, parent traversal (`..`), unsafe audio IDs, and non-WAV mappings are rejected.

## Playback behavior

- starting a narrative line attempts its `audio-id`;
- a new line stops the previous voice clip before starting its own clip;
- skipping a line stops its voice immediately;
- encounter/narrative cancellation stops voice immediately;
- script shutdown stops voice immediately;
- missing/bad clips never block subtitles, dialogue choices, or combat;
- each missing/failed asset is warned once in `Nightwalker.log`.

The current backend uses Windows asynchronous WAV playback. Recorded dialogue works in-game, while true 3D positional voice and phoneme/viseme lip-sync remain separate follow-up work.

## Recording contract

Keep one WAV per stable `audio-id`. Multi-line conversations remain separate files even when they belong to one randomized sequence. This preserves subtitle timing, interruption, reaction priority, and future facial/lip-sync data per line.

## Audio boundary

Only owner-created or properly licensed voice performances may ship. Do not package extracted game dialogue or recordings from another production.
