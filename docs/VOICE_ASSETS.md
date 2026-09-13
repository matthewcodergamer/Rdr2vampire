# Nightwalker voice assets

Nightwalker narrative lines carry stable `audio-id` values such as `nw.audio.sd.aim.04`. The runtime resolves those IDs to external WAV files and plays them when the matching authored line starts.

## Runtime layout

Voice content is installed beside `Nightwalker.asi`:

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.audio
audio/
  nw.audio.sd.first_contact.nearer.01.wav
  nw.audio.sd.question.names.01.wav
  ...
```

If a line has `audio-id=nw.audio.sd.aim.04`, the default lookup is:

```text
audio/nw.audio.sd.aim.04.wav
```

No manifest entry is required when the WAV filename matches the audio ID. `Nightwalker.audio` may still provide explicit mappings for friendly uploaded filenames.

## First integrated voice batch

The first user-supplied Nightwalker batch is normalized to 22.05 kHz, mono, 16-bit PCM WAV and wired as coherent randomized exchanges:

- `Come no nearer.`
- `There. You have proven yourself wiser than the last.` — two alternate performances
- `Men have given me many names.` — two alternate performances
- `None of those men lived long enough to make one matter.` — two alternate performances

The first two lines form two complete `saint_denis.pre_fight` variants. The latter two form two complete `saint_denis.choice.question` variants. The game randomizes the complete exchange; it never mixes the first half of one take with the second half of another.

The uploaded `clip_*` source excerpts are reference-only and are deliberately excluded from release voice payloads.

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

## Original/licensed audio only

Nightwalker may use an original or properly licensed vampire performance. Do not package copied Dawnwalker recordings, cloned proprietary actor performances, or extracted copyrighted dialogue audio.
