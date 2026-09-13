# Nightwalker voice and interaction system

Nightwalker uses authored dialogue families, stable audio IDs, and project-owned AI voice recordings. The game never generates dialogue at runtime.

## In-game interaction

The Saint Denis vampire uses RDR2 entity-linked prompts:

- hold **L2 / LT** to focus on the vampire;
- choose **TALK** to hear a randomized non-hostile response;
- choose **ANTAGONIZE** to hear a randomized hostile response and begin combat after it finishes;
- choose **LEAVE** to hear a randomized withdrawal line and disengage;
- **R2 / RT** remains the normal weapon/fire trigger. Aiming, sustained aim, lowering the weapon, firing, hitting, missing, drawing melee/lasso weapons, punching, and backing away feed the reactive dialogue state machine.

The prompt is Arthur/John's intent. Nightwalker does not clone or synthesize the Rockstar protagonists' actors.

## Randomization contract

Randomization happens at the complete-sequence level. Individual sentences from different speeches are never shuffled together.

Each family uses a non-repeating shuffle bag:

1. collect every eligible authored sequence in the context family;
2. randomly select one complete sequence;
3. remove it from the current bag;
4. exhaust the family before refilling;
5. avoid immediately repeating the final sequence when the bag refills.

Families include opening/soul speeches, TALK, ANTAGONIZE, LEAVE, aiming, sustained aim, lowered weapon, shot hit/miss, weapon posture, unarmed/melee contact, and movement reactions.

## Runtime layout

```text
Nightwalker.asi
Nightwalker.ini
Nightwalker.dialogue
Nightwalker.voice.dialogue
Nightwalker.audio
audio/
  nw.audio.sd.first_contact.nearer.01.mp3
  nw.audio.sd.soul.03a1.mp3
  nw.audio.sd.shot.hit.04.mp3
  ...
```

`Nightwalker.audio` maps stable IDs to relative MP3/WAV files. `Nightwalker.dialogue` is the authoritative base script. `Nightwalker.voice.dialogue` appends reviewed extra sequences without replacing base sequence IDs.

The Windows backend uses asynchronous MCI playback for MP3 and `PlaySoundW` for WAV. Missing or failed files always fall back to the exact subtitle and never block combat or cleanup.

## Complete uploaded library

The complete pass maps **67 dialogue IDs** to **64 physical MP3 files**. Three pairs intentionally share one recording:

- `nw.audio.sd.soul.01a` reuses the identical `nw.audio.sd.soul.recorded.01` performance;
- shot-hit `Good.` and unarmed-hit `Good.` share one neutral performance;
- lowered-weapon `Better.` and melee-hit `Better.` share one neutral performance.

Long sentences recorded as separate clips are authored as separate consecutive lines inside the same sequence. This applies to:

- `Death is not life's opposite.` → `It is its oldest shadow.`
- `You hear a heartbeat and call it life.` → `You hear silence and call it death.`
- `Their crowns are dust.` → `Their prayers are forgotten.`

This preserves natural timing and future per-line lip-sync data.

## Cleanup and interruption

Starting a new line stops the previous clip. Skip, combat interruption, encounter abort, F10/F11 cleanup, player death, script shutdown, and invalid actor ownership all stop voice playback and clear conversation prompts.

## Current limitation

The voice is audible and event-correct but is not yet positioned in true 3D at the vampire's mouth, and the mouth does not yet use phoneme/viseme lip-sync. Those remain separate presentation upgrades.
