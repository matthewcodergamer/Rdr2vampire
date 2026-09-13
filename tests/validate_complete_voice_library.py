#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/Nightwalker.audio"
BASE = ROOT / "content/Nightwalker.dialogue"
SUPPLEMENT = ROOT / "Nightwalker.voice.dialogue"


def parse_manifest() -> dict[str, pathlib.Path]:
    mappings: dict[str, pathlib.Path] = {}
    for raw in MANIFEST.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line.startswith("asset="):
            continue
        audio_id, relative = line[6:].split("|", 1)
        if audio_id in mappings:
            raise SystemExit(f"duplicate audio id: {audio_id}")
        if not re.fullmatch(r"[A-Za-z0-9._-]+", audio_id):
            raise SystemExit(f"unsafe audio id: {audio_id}")
        path = pathlib.PurePosixPath(relative)
        if path.is_absolute() or ".." in path.parts:
            raise SystemExit(f"unsafe audio path: {relative}")
        mappings[audio_id] = ROOT / "content" / path
    return mappings


def dialogue_audio_ids(path: pathlib.Path) -> set[str]:
    ids: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        if not raw.startswith("line="):
            continue
        fields = raw[5:].split("|", 6)
        if len(fields) != 7:
            raise SystemExit(f"malformed dialogue line in {path}: {raw}")
        if fields[4]:
            ids.add(fields[4])
    return ids


mappings = parse_manifest()
if len(mappings) != 67:
    raise SystemExit(f"expected 67 audio-id mappings, found {len(mappings)}")

missing_files = sorted(audio_id for audio_id, path in mappings.items() if not path.is_file())
if missing_files:
    raise SystemExit(f"manifest references missing files: {missing_files}")

physical = {path.resolve() for path in mappings.values()}
if len(physical) != 64:
    raise SystemExit(f"expected 64 unique voice files, found {len(physical)}")

for path in physical:
    if path.suffix.lower() not in {".mp3", ".wav"}:
        raise SystemExit(f"unsupported voice payload: {path}")
    prefix = path.read_bytes()[:3]
    if path.suffix.lower() == ".mp3" and prefix != b"ID3" and prefix[:2] != b"\xff\xfb":
        raise SystemExit(f"MP3 header not recognized: {path}")

used = dialogue_audio_ids(BASE) | dialogue_audio_ids(SUPPLEMENT)
unused = sorted(set(mappings) - used)
if unused:
    raise SystemExit(f"manifest mappings not used by dialogue: {unused}")

raw_uploads = sorted((ROOT / "content").glob("Vam-*.mp3"))
if raw_uploads:
    raise SystemExit(f"loose upload filenames remain in content/: {[p.name for p in raw_uploads]}")
if (ROOT / "content/Nightwalker.voicepack").exists():
    raise SystemExit("obsolete Nightwalker.voicepack remains tracked")

print(f"Validated {len(mappings)} dialogue IDs across {len(physical)} physical voice files")
