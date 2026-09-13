# Nightwalker voice assets

Nightwalker narrative lines already carry stable `audio-id` values such as `nw.audio.sd.aim.04`. The runtime now resolves those IDs to external WAV files and plays them when the matching authored line starts.

## Runtime layout

Place voice content next to `Nightwalker.asi`:

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.audio        # optional explicit filename mappings
audio/
  nw.audio.sd.soul.01a.wav
  nw.audio.sd.soul.01b.wav
  nw.audio.sd.aim.04.wav
  ...
```

If a line has `audio-id=nw.audio.sd.aim.04`, the default lookup is:

```text
audio/nw.audio.sd.aim.04.wav
```

No manifest entry is required when the WAV filename matches the audio ID.

## Friendly/uploaded filenames

If a recording arrives with another filename, add it to `Nightwalker.audio` instead of changing gameplay code:

```text
schema=1
asset=nw.audio.sd.soul.01a|audio/Opening Soul 01A.wav
asset=nw.audio.sd.aim.04|audio/Aim - Your hand speaks.wav
```

Only relative `.wav` paths are accepted. Absolute paths, parent traversal (`..`), unsafe audio IDs and non-WAV mappings are rejected.

## Playback behavior

- starting a narrative line attempts its `audio-id`;
- a new line stops the previous voice clip before starting its own clip;
- skipping a line stops its voice immediately;
- encounter/narrative cancellation stops voice immediately;
- script shutdown stops voice immediately;
- missing/bad clips never block subtitles, dialogue choices or combat;
- each missing/failed asset is warned once in `Nightwalker.log` rather than spamming every frame.

The current backend uses Windows asynchronous WAV playback. It is intentionally the first safe production step: recorded dialogue now works in-game, while true 3D positional voice and phoneme/viseme lip-sync remain separate follow-up work.

## Recording contract

Keep one WAV per stable `audio-id`. Multi-line conversations remain separate files even when they belong to one randomized sequence. This lets Nightwalker preserve subtitle timing, interruption, reaction priority and future facial/lip-sync data per line.

Prefer PCM WAV for the first production pass. When recordings are uploaded, organize them by audio ID or add explicit `Nightwalker.audio` mappings; do not rename dialogue sequence IDs just to match arbitrary filenames.

## Original/licensed audio only

Nightwalker may use an original or properly licensed vampire performance. Do not package copied Dawnwalker recordings, cloned proprietary actor performances, or extracted copyrighted dialogue audio.
