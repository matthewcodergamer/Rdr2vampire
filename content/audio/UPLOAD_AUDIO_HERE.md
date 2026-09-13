# Upload Voice Batch 1 here

Upload the raw audio files from the `audio` folder of `voice_batch_1.zip` into this directory.

Do **not** upload the ZIP itself here.
Do **not** overwrite `content/Nightwalker.dialogue`, `content/Nightwalker.audio`, or the root `Nightwalker.voice.dialogue` with export-bundle copies.
Do **not** upload `INSTALL` or `voice_batch_1-inventory.json` unless explicitly requested for troubleshooting.

After upload, the cleanup pass will:

1. identify duplicate recordings/paths,
2. match each recording to the canonical `nw.audio.*` ID,
3. combine split recordings when one canonical line was exported as multiple clips,
4. repair subtitle text/timing from the authoritative dialogue catalog,
5. move the approved assets into `content/audio/`,
6. update `content/Nightwalker.audio`,
7. run dialogue/audio CI and release packaging checks.
