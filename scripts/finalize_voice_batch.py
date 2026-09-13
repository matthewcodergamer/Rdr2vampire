#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import wave

ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / "content" / "audio"
CORE_DIALOGUE = ROOT / "content" / "Nightwalker.dialogue"
VOICE_DIALOGUE = ROOT / "Nightwalker.voice.dialogue"
MANIFEST = ROOT / "content" / "Nightwalker.audio"

EXPECTED_RATE = 44100
EXPECTED_CHANNELS = 1
EXPECTED_WIDTH = 2
SUBTITLE_TAIL_MS = 300


def read_pcm(path: Path):
    with wave.open(str(path), "rb") as wf:
        params = (wf.getnchannels(), wf.getsampwidth(), wf.getframerate(), wf.getcomptype())
        if params != (EXPECTED_CHANNELS, EXPECTED_WIDTH, EXPECTED_RATE, "NONE"):
            raise RuntimeError(f"Unexpected WAV format for {path}: {params}")
        return wf.readframes(wf.getnframes())


def write_pcm(path: Path, frames: bytes):
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(EXPECTED_CHANNELS)
        wf.setsampwidth(EXPECTED_WIDTH)
        wf.setframerate(EXPECTED_RATE)
        wf.writeframes(frames)


def silence_ms(ms: int) -> bytes:
    frames = round(EXPECTED_RATE * ms / 1000)
    return b"\x00" * frames * EXPECTED_CHANNELS * EXPECTED_WIDTH


def frame_at_ms(ms: int) -> int:
    return round(EXPECTED_RATE * ms / 1000) * EXPECTED_CHANNELS * EXPECTED_WIDTH


def require(path: Path):
    if not path.exists():
        raise FileNotFoundError(path)


def merge_pair(first_name: str, second_name: str, target_name: str, gap_ms: int = 160):
    first = AUDIO / first_name
    second = AUDIO / second_name
    require(first)
    require(second)
    write_pcm(AUDIO / target_name, read_pcm(first) + silence_ms(gap_ms) + read_pcm(second))
    first.unlink()
    second.unlink()


def split_challenge():
    src = AUDIO / "nw.audio.sd.choice.challenge.voice_batch_1.you.mistake.my.patience.for.21.wav"
    require(src)
    frames = read_pcm(src)
    # The uploaded AI take has a clean pause between the two authored clauses.
    # Speech ends around 2668 ms and resumes around 3489 ms.
    cut_a = frame_at_ms(2668)
    cut_b = frame_at_ms(3489)
    write_pcm(AUDIO / "nw.audio.sd.challenge.01a.wav", frames[:cut_a] + silence_ms(220))
    write_pcm(AUDIO / "nw.audio.sd.challenge.01b.wav", silence_ms(100) + frames[cut_b:])
    src.unlink()


def rename_fallbacks():
    renames = {
        "nw.audio.sd.react.aim.voice_batch_1.careful.steel.makes.men.confuse.05.wav": "nw.audio.sd.aim.01.wav",
        "nw.audio.sd.react.lasso.draw.voice_batch_1.you.mistake.a.body.for.10.wav": "nw.audio.sd.lasso.draw.02.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.i.watched.kings.pile.stone.30.wav": "nw.audio.sd.soul.08a.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.small.words.for.a.world.35.wav": "nw.audio.sd.soul.05b.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.then.i.learned.it.is.38.wav": "nw.audio.sd.soul.04b.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.i.have.walked.between.them.42.wav": "nw.audio.sd.soul.03b.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.men.pray.for.eternal.life.43.wav": "nw.audio.sd.soul.02a.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.i.have.stood.where.life.45.wav": "nw.audio.sd.soul.01b.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.do.not.ask.what.i.50.wav": "nw.audio.sd.soul.01a.wav",
        "nw.audio.sd.choice.question.voice_batch_1.a.thousand.souls.pressed.together.51.wav": "nw.audio.sd.question.city.01a.wav",
        "nw.audio.sd.choice.question.voice_batch_1.none.of.those.men.lived.56.wav": "nw.audio.sd.question.none.02.wav",
        "nw.audio.sd.choice.question.voice_batch_1.men.have.given.me.many.59.wav": "nw.audio.sd.question.names.02.wav",
        # Chronological take 20:45 is first-contact 01; 20:46 is first-contact 02.
        "nw.audio.sd.pre.fight.voice_batch_1.there.you.have.proven.yourself.62.wav": "nw.audio.sd.first_contact.wiser.01.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.there.you.have.proven.yourself.61.wav": "nw.audio.sd.first_contact.wiser.02.wav",
        # The separately named third export is retained as the second clean rare-wisdom variant.
        "nw.audio.sd.choice.leave.voice_batch_1.a.rare.wisdom.2.52.wav": "nw.audio.sd.leave.rare_wisdom.02.wav",
    }
    for src_name, dst_name in renames.items():
        src = AUDIO / src_name
        dst = AUDIO / dst_name
        require(src)
        if dst.exists():
            raise RuntimeError(f"Refusing to overwrite canonical asset: {dst}")
        src.rename(dst)


def wav_duration_ms(path: Path) -> int:
    with wave.open(str(path), "rb") as wf:
        return round(wf.getnframes() * 1000 / wf.getframerate())


def audio_ids() -> dict[str, int]:
    result = {}
    for path in sorted(AUDIO.glob("*.wav")):
        audio_id = path.stem
        if "voice_batch_1" in audio_id:
            raise RuntimeError(f"Fallback id survived cleanup: {path.name}")
        result[audio_id] = wav_duration_ms(path)
    return result


def rewrite_dialogue_file(path: Path, durations: dict[str, int], skip_ids: set[str] | None = None):
    skip_ids = skip_ids or set()
    lines = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        if not raw.startswith("line="):
            lines.append(raw)
            continue
        fields = raw[5:].split("|", 6)
        if len(fields) != 7:
            raise RuntimeError(f"Malformed dialogue row in {path}: {raw}")
        sequence, line_id, speaker, text_id, audio_id, duration, subtitle = fields
        if audio_id in skip_ids:
            continue
        if audio_id in durations:
            duration = str(durations[audio_id] + SUBTITLE_TAIL_MS)
        # Keep authoritative repository subtitles; never derive subtitle text from filenames.
        lines.append("line=" + "|".join([sequence, line_id, speaker, text_id, audio_id, duration, subtitle]))
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def write_manifest(ids: dict[str, int]):
    lines = [
        "# Nightwalker narrative voice manifest",
        "# schema 1",
        "# asset=<audio-id>|<relative audio path>",
        "#",
        "# Canonical owner-provided voice library. WAV filenames are stable gameplay audio ids.",
        "# Missing ids remain subtitle-only and may use the runtime legacy fallback.",
        "schema=1",
        "",
    ]
    for audio_id in sorted(ids):
        lines.append(f"asset={audio_id}|audio/{audio_id}.wav")
    MANIFEST.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_readme(count: int):
    (AUDIO / "README.md").write_text(
        "# Nightwalker voice assets\n\n"
        f"This folder contains {count} canonical owner-provided vampire voice WAVs.\n\n"
        "- 44.1 kHz, mono, 16-bit PCM WAV\n"
        "- Filenames are stable `nw.audio.*` gameplay ids.\n"
        "- `content/Nightwalker.audio` maps those ids to these files.\n"
        "- Dialogue families randomize complete authored interactions; filenames must not be shuffled manually.\n"
        "- The original upload staging/export files were removed after canonical remapping.\n",
        encoding="utf-8",
    )


def main():
    require(AUDIO)

    # Three canonical dialogue lines arrived as two separate generated clips each.
    merge_pair(
        "nw.audio.sd.pre.fight.voice_batch_1.death.is.not.lifes.opposite.46.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.it.is.its.oldest.shadow.41.wav",
        "nw.audio.sd.soul.03a.wav",
    )
    merge_pair(
        "nw.audio.sd.pre.fight.voice_batch_1.you.hear.a.heartbeat.and.34.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.you.hear.silence.and.call.33.wav",
        "nw.audio.sd.soul.05a.wav",
    )
    merge_pair(
        "nw.audio.sd.pre.fight.voice_batch_1.their.crowns.are.dust.29.wav",
        "nw.audio.sd.pre.fight.voice_batch_1.their.prayers.are.forgotten.28.wav",
        "nw.audio.sd.soul.08b.wav",
    )

    # One take contains both authored challenge lines; split it at its natural pause.
    split_challenge()
    rename_fallbacks()

    marker = AUDIO / "UPLOAD_AUDIO_HERE.md"
    if marker.exists():
        marker.unlink()

    ids = audio_ids()
    if len(ids) != 59:
        raise RuntimeError(f"Expected 59 cleaned WAV assets, found {len(ids)}")

    rewrite_dialogue_file(CORE_DIALOGUE, ids)
    # soul.01a is now voiced in the base catalog, so its old supplemental duplicate is removed.
    # rare_wisdom.03 came only from a duplicate ZIP path and is removed with the duplicate collision.
    rewrite_dialogue_file(
        VOICE_DIALOGUE,
        ids,
        skip_ids={"nw.audio.sd.soul.recorded.01", "nw.audio.sd.leave.rare_wisdom.03"},
    )
    write_manifest(ids)
    write_readme(len(ids))

    leftovers = [p.name for p in AUDIO.iterdir() if p.is_file() and p.suffix.lower() not in {".wav", ".md"}]
    if leftovers:
        raise RuntimeError(f"Unexpected non-library files in content/audio: {leftovers}")
    print(f"Finalized {len(ids)} canonical Nightwalker voice WAVs")


if __name__ == "__main__":
    main()
