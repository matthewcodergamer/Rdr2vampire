# Nightwalker voice assets

Nightwalker narrative lines carry stable `audio-id` values such as `nw.audio.sd.aim.04`. The runtime resolves those IDs through `Nightwalker.audio` and plays the matching reviewed WAV or MP3 asset when the authored line starts.

## Runtime layout

Installed voice content lives beside `Nightwalker.asi`:

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.voice.dialogue
Nightwalker.audio
audio/
  nw.audio.sd.first_contact.nearer.01.mp3
  nw.audio.sd.soul.recorded.01.mp3
  ...
```

`Nightwalker.dialogue` remains the authoritative base catalog. `Nightwalker.voice.dialogue` is the reviewed supplemental catalog added by the owner voice batches. Runtime appends only new sequence IDs; malformed or duplicate supplemental entries cannot replace base dialogue.

The repository stores the reviewed physical audio payload in `content/Nightwalker.voicepack`. It is a deterministic archive containing 25 unique MP3 files plus its checksum/duration inventory. The manifest exposes 26 stable audio IDs because `nw.audio.sd.soul.01a` intentionally reuses the same owner-recorded `Do not ask what I am...` performance as `nw.audio.sd.soul.recorded.01` rather than storing a duplicate.

## Voice Batch 1

Batch 1 supplies seven owner-created AI voice takes:

- `Come no nearer.`
- `There. You have proven yourself wiser than the last.` — two alternate performances
- `Men have given me many names.` — two alternate performances
- `None of those men lived long enough to make one matter.` — two alternate performances

The complete first-contact and QUESTION exchanges stay paired when randomized.

## Voice Batch 2

Batch 2 supplies ten unique performances:

- `Do not ask what I am, as though the world has only two answers.`
- `A thousand souls pressed together behind brick and iron.` → `Look around you.` → `Keep it.`
- three alternate `A rare wisdom.` LEAVE responses
- `Choose your next words with greater care.` for sustained aim
- `He has gone farther from you than I ever could.` → `From him.`

The additional uploaded `None of those men lived long enough to make one matter.` take matched Batch 1 exactly and is intentionally not stored twice.

## Voice Batch 3

Batch 3 completes the currently supplied soul performances:

- `I have stood where life ends and found no wall there.`
- `Names are for things that stay on one side.`
- `Men pray for eternal life until eternity answers them.`
- `Then they call the answer cursed.`
- `I have had centuries to enjoy the joke.`
- `Death is not life's opposite. It is its oldest shadow.` — assembled from the owner's two consecutive recorded clauses as one authored line
- `I have walked between them so long neither claims me.`
- `The grave was once a terror to me.`

The newly uploaded `Do not ask what I am...` performance matched the already-reviewed Batch 2 asset, so the base Soul 01 opening aliases that existing recording instead of adding another physical copy.

## Audio format and playback

The committed distribution assets are mono 44.1 kHz MP3 voice files. The backend still supports legacy WAV mappings. MP3 playback uses the Windows MCI API through `winmm`; WAV playback uses asynchronous `PlaySoundW`.

Playback remains interruption-safe:

- a new narrative line stops the previous clip;
- skipping a line stops its clip immediately;
- combat/encounter cancellation stops the active clip;
- script shutdown closes both WAV and MP3 playback;
- missing or rejected audio never blocks subtitles, interaction prompts, or combat;
- failures are warned once in `Nightwalker.log`.

The voice is still non-positional Windows playback. True 3D emission from the vampire and phoneme/viseme lip-sync are separate follow-up systems.

## Installing all batches

CI publishes `Nightwalker-Voice-Assets-All-Batches.zip`. Extract its contents beside `Nightwalker.asi`; the ZIP contains:

- `Nightwalker.audio`
- `Nightwalker.voice.dialogue`
- `audio/` with all 25 unique physical voice files

Keep `OptionalAudio=true` under `[Narrative]` in `Nightwalker.ini`, then restart RDR2/Nightwalker.

For a source build, run:

```powershell
./scripts/package-voice-assets.ps1
```

To build the normal Nightwalker release ZIP with the voice assets already embedded, call:

```powershell
./scripts/package-release.ps1 -PluginPath <path-to-Nightwalker.asi> -IncludeVoiceAssets
```

The ordinary release packager remains compatible with a voice-free package when `-IncludeVoiceAssets` is omitted.

## Validation contract

`Voice Batch CI` verifies:

- 26 stable manifest IDs resolve to 25 reviewed physical assets;
- every physical file in `Nightwalker.voicepack` matches its stored byte count and SHA-256;
- every mapped audio ID is referenced by authored base or supplemental dialogue;
- authored subtitle timing is never shorter than the corresponding recording;
- timing tails remain bounded;
- the supplemental catalog still uses coherent whole-sequence randomization;
- the installable all-batches ZIP can be produced from the committed repository.

## Original/licensed audio only

Only owner-created or properly licensed performances may ship. Nightwalker does not package extracted RDR2 dialogue, Dawnwalker production audio, or cloned proprietary actor recordings.
