# Nightwalker voice assets

Nightwalker narrative lines carry stable `audio-id` values such as `nw.audio.sd.aim.04`. The runtime resolves those IDs to external WAV files and plays them when the matching authored line starts.

## Runtime layout

Voice content is installed beside `Nightwalker.asi`:

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.voice.dialogue
Nightwalker.audio
audio/
  nw.audio.sd.first_contact.nearer.01.wav
  nw.audio.sd.soul.recorded.01.wav
  ...
```

`Nightwalker.dialogue` is the authoritative base catalog. `Nightwalker.voice.dialogue` is an optional reviewed supplement. Runtime parses it separately and appends only sequence IDs that do not already exist. A missing, malformed or duplicate supplemental sequence cannot replace the base dialogue.

If a line has `audio-id=nw.audio.sd.aim.04`, the default lookup is `audio/nw.audio.sd.aim.04.wav`. `Nightwalker.audio` contains explicit reviewed mappings for the shipped voice batches.

## Voice Batch 1

Batch 1 contains seven owner-supplied Nightwalker AI voice takes:

- `Come no nearer.`
- `There. You have proven yourself wiser than the last.` — two alternate performances
- `Men have given me many names.` — two alternate performances
- `None of those men lived long enough to make one matter.` — two alternate performances

These are authored as complete first-contact and QUESTION exchanges. The conversation-level shuffle bag never mixes halves of different performances.

## Voice Batch 2

Batch 2 adds ten unique owner-supplied performances:

- a standalone soul response: `Do not ask what I am, as though the world has only two answers.`
- one complete Saint Denis QUESTION answer: `A thousand souls...` → `Look around you.` → `Keep it.`
- three distinct LEAVE deliveries of `A rare wisdom.`
- one sustained-aim warning: `Choose your next words with greater care.`
- one paired church/faith QUESTION answer: `He has gone farther from you than I ever could.` → `From him.`

The uploaded `None of those men lived long enough to make one matter.` file decodes to the same performance as Batch 1's `nw.audio.sd.question.none.01`, so it is intentionally not stored twice.

All shipped recordings are 44.1 kHz, mono, 16-bit PCM WAV with restrained level matching. Authored subtitle durations include a short tail buffer and are checked against the reviewed inventory in CI.

## Installing voice content

Copy `Nightwalker.audio`, `Nightwalker.voice.dialogue` and the complete `audio` folder beside `Nightwalker.asi`. Keep `OptionalAudio=true` under `[Narrative]` in `Nightwalker.ini`, then restart RDR2/Nightwalker. Missing clips remain subtitle-only.

The release packager includes the base dialogue, supplemental voice dialogue and manifest. It also copies reviewed WAV files from `content/audio/` when that directory is present in a release workspace. Standalone voice-pack ZIPs may supply the WAV directory separately.

## Playback behavior

- starting a narrative line attempts its `audio-id`;
- a new line stops the previous voice clip before starting its own clip;
- skipping a line stops its voice immediately;
- encounter/narrative cancellation stops voice immediately;
- script shutdown stops voice immediately;
- missing/bad clips never block subtitles, choices or combat;
- each missing/failed asset is warned once in `Nightwalker.log`.

The current backend uses Windows asynchronous WAV playback. True 3D positional voice and phoneme/viseme lip-sync remain separate follow-up work.

## Recording contract

Keep one WAV per stable `audio-id`. Multi-line conversations remain separate files even when they belong to one randomized sequence. This preserves subtitle timing, interruption priority and future facial/lip-sync data per line.

## Original/licensed audio only

Nightwalker may use an original or properly licensed vampire performance. Do not package copied game recordings, cloned proprietary actor performances or extracted copyrighted dialogue.
