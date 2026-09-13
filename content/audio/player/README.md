# Nightwalker player dialogue audio drop

Upload Arthur / player-character dialogue recordings into this directory.

## Drop location

`content/audio/player/`

Do not put Saint Denis vampire recordings here. Existing vampire voice assets remain in `content/audio/`.

## Preferred stable audio ids

When a recording has been assigned its final Nightwalker id, name the WAV after that id:

- `nw.audio.player.arthur.talk.01.wav`
- `nw.audio.player.arthur.talk.02.wav`
- `nw.audio.player.arthur.antagonize.01.wav`
- `nw.audio.player.arthur.leave.01.wav`
- `nw.audio.player.arthur.react.aim.01.wav`

Raw/generated filenames are also okay for the initial upload. They can be normalized to stable ids when the batch is mapped into `Nightwalker.dialogue`.

## Dialogue integration rule

Player recordings are authored into whole conversation variants, not shuffled sentence-by-sentence. A selected sequence may contain:

1. Arthur/player line
2. vampire response
3. optional Arthur follow-up
4. optional vampire closer

The whole sequence remains intact when randomized.

Existing vampire-only sequences stay available. That means repeated Saint Denis encounters can naturally vary between player-led exchanges, vampire-led exchanges, and moments where Arthur says nothing before the vampire speaks.

Every family uses Nightwalker's existing non-repeating shuffle bag: variants are exhausted before refill, and a refill cannot immediately replay the previous variant when multiple choices exist.

## Audio format

WAV is preferred for the canonical release library. MP3 remains runtime-supported but should not become the canonical source format unless intentionally approved.

The runtime/player-audio path is reserved for ids beginning with `nw.audio.player.` and release packaging preserves WAV files placed in this folder.
