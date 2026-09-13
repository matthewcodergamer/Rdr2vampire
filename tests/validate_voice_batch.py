#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import wave

ROOT = pathlib.Path(__file__).resolve().parents[1]
AUDIO_ROOT = ROOT / "content" / "audio"
MANIFEST = ROOT / "content" / "Nightwalker.audio"
BASE_DIALOGUE = ROOT / "content" / "Nightwalker.dialogue"
SUPPLEMENT_DIALOGUE = ROOT / "Nightwalker.voice.dialogue"
EXPECTED_ASSETS = 59
EXPECTED_RATE = 44100
EXPECTED_CHANNELS = 1
EXPECTED_WIDTH = 2


def parse_manifest() -> dict[str, str]:
    mappings: dict[str, str] = {}
    for raw in MANIFEST.read_text(encoding="utf-8").splitlines():
        raw = raw.strip()
        if not raw or raw.startswith("#") or raw.startswith("schema="):
            continue
        if not raw.startswith("asset=") or "|" not in raw:
            raise SystemExit(f"Malformed manifest line: {raw}")
        audio_id, relative = raw[6:].split("|", 1)
        if audio_id in mappings:
            raise SystemExit(f"Duplicate manifest id: {audio_id}")
        if "voice_batch_1" in audio_id:
            raise SystemExit(f"Fallback audio id survived cleanup: {audio_id}")
        if not relative.startswith("audio/") or not relative.lower().endswith(".wav"):
            raise SystemExit(f"Non-canonical manifest path: {relative}")
        if pathlib.PurePosixPath(relative).name != f"{audio_id}.wav":
            raise SystemExit(f"Manifest filename does not match stable id: {audio_id} -> {relative}")
        mappings[audio_id] = relative
    return mappings


def parse_durations(path: pathlib.Path) -> dict[str, int]:
    durations: dict[str, int] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        if not raw.startswith("line="):
            continue
        fields = raw[5:].split("|", 6)
        if len(fields) != 7:
            raise SystemExit(f"Malformed dialogue line in {path.name}: {raw}")
        audio_id = fields[4]
        if audio_id:
            durations[audio_id] = int(fields[5])
    return durations


def wav_duration_ms(path: pathlib.Path) -> int:
    with wave.open(str(path), "rb") as wav:
        if wav.getnchannels() != EXPECTED_CHANNELS:
            raise SystemExit(f"Voice WAV is not mono: {path}")
        if wav.getsampwidth() != EXPECTED_WIDTH:
            raise SystemExit(f"Voice WAV is not 16-bit PCM: {path}")
        if wav.getframerate() != EXPECTED_RATE:
            raise SystemExit(f"Voice WAV is not 44.1 kHz: {path}")
        if wav.getcomptype() != "NONE":
            raise SystemExit(f"Voice WAV is compressed: {path}")
        if wav.getnframes() <= 0:
            raise SystemExit(f"Voice WAV is empty: {path}")
        return round(wav.getnframes() * 1000 / wav.getframerate())


mappings = parse_manifest()
if len(mappings) != EXPECTED_ASSETS:
    raise SystemExit(f"Expected {EXPECTED_ASSETS} stable audio-id mappings, found {len(mappings)}")

physical = sorted(AUDIO_ROOT.glob("*.wav"))
if len(physical) != EXPECTED_ASSETS:
    raise SystemExit(f"Expected {EXPECTED_ASSETS} canonical WAV files, found {len(physical)}")

physical_relatives = {"audio/" + path.name for path in physical}
manifest_relatives = set(mappings.values())
if len(manifest_relatives) != EXPECTED_ASSETS:
    raise SystemExit("Every stable audio id must map to a unique canonical WAV")
if physical_relatives != manifest_relatives:
    missing = sorted(manifest_relatives - physical_relatives)
    orphaned = sorted(physical_relatives - manifest_relatives)
    raise SystemExit(f"Manifest/audio mismatch; missing={missing}, orphaned={orphaned}")

source_durations: dict[str, int] = {}
for path in physical:
    if "voice_batch_1" in path.name:
        raise SystemExit(f"Fallback filename survived cleanup: {path.name}")
    source_durations[path.stem] = wav_duration_ms(path)

line_durations = parse_durations(BASE_DIALOGUE)
line_durations.update(parse_durations(SUPPLEMENT_DIALOGUE))
for audio_id, source_ms in source_durations.items():
    if audio_id not in line_durations:
        raise SystemExit(f"Mapped voice id is not used by authored dialogue: {audio_id}")
    authored_ms = line_durations[audio_id]
    if authored_ms < source_ms:
        raise SystemExit(
            f"Dialogue timing clips {audio_id}: authored={authored_ms}ms source={source_ms}ms"
        )
    if authored_ms - source_ms > 850:
        raise SystemExit(
            f"Excessive timing tail for {audio_id}: authored={authored_ms}ms source={source_ms}ms"
        )

combined_text = BASE_DIALOGUE.read_text(encoding="utf-8") + "\n" + SUPPLEMENT_DIALOGUE.read_text(encoding="utf-8")
for truncated in ("Let us c\n", "behind brick a\n", "make one\n"):
    if truncated in combined_text:
        raise SystemExit(f"Truncated subtitle survived cleanup: {truncated.strip()}")

print(
    f"Validated canonical Nightwalker voice library: {len(mappings)} stable ids, "
    f"{len(physical)} unique 44.1 kHz mono PCM WAVs, dialogue timing contract OK"
)
