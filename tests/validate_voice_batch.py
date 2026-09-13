#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/Nightwalker.audio"
BASE_DIALOGUE = ROOT / "content/Nightwalker.dialogue"
SUPPLEMENT_DIALOGUE = ROOT / "Nightwalker.voice.dialogue"
INVENTORY = ROOT / "tests/fixtures/VoiceAssetInventory.complete.json"


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
        if not relative.startswith("audio/") or not relative.lower().endswith((".mp3", ".wav")):
            raise SystemExit(f"Unsafe/unsupported manifest path: {relative}")
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


mappings = parse_manifest()
if len(mappings) != 26:
    raise SystemExit(f"Expected 26 stable audio-id mappings, found {len(mappings)}")

inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
if inventory.get("schema") != 1:
    raise SystemExit("Unsupported complete voice inventory schema")
if inventory.get("physical_assets") != 25 or inventory.get("manifest_mappings") != 26:
    raise SystemExit("Complete voice inventory count contract changed")
fmt = inventory.get("format", {})
if fmt.get("codec") != "mp3" or fmt.get("sample_rate_hz") != 44100 or fmt.get("channels") != 1:
    raise SystemExit("Complete voice inventory format contract changed")

assets = inventory.get("assets", [])
if len(assets) != 25:
    raise SystemExit(f"Expected 25 physical voice inventory records, found {len(assets)}")
assets_by_id: dict[str, dict] = {}
for item in assets:
    audio_id = item["audio_id"]
    if audio_id in assets_by_id:
        raise SystemExit(f"Duplicate inventory audio id: {audio_id}")
    if item.get("duration_ms", 0) <= 0 or item.get("bytes", 0) <= 0:
        raise SystemExit(f"Invalid size/duration for {audio_id}")
    if item.get("sample_rate_hz") != 44100 or item.get("channels") != 1 or item.get("codec") != "mp3":
        raise SystemExit(f"Unexpected media contract for {audio_id}")
    if not re.fullmatch(r"[0-9a-f]{64}", item.get("sha256", "")):
        raise SystemExit(f"Invalid SHA-256 for {audio_id}")
    if item.get("batch") not in (1, 2, 3):
        raise SystemExit(f"Invalid batch number for {audio_id}")
    assets_by_id[audio_id] = item

aliases = inventory.get("aliases", [])
if aliases != [{
    "audio_id": "nw.audio.sd.soul.01a",
    "path": "audio/nw.audio.sd.soul.recorded.01.mp3",
    "reason": "Same owner-recorded performance is reused by the base Soul 01 sequence.",
}]:
    raise SystemExit("Expected Soul 01a alias contract changed")

physical_paths = set(mappings.values())
if len(physical_paths) != 25:
    raise SystemExit(f"Expected 26 IDs to resolve to 25 physical files, got {len(physical_paths)}")
if mappings.get("nw.audio.sd.soul.01a") != mappings.get("nw.audio.sd.soul.recorded.01"):
    raise SystemExit("Soul 01a must reuse the reviewed Batch 2 performance")

for audio_id, item in assets_by_id.items():
    path = mappings.get(audio_id)
    if path is None:
        raise SystemExit(f"Inventory asset missing manifest mapping: {audio_id}")
    if path != f"audio/{audio_id}.mp3":
        raise SystemExit(f"Unexpected physical path for {audio_id}: {path}")

line_durations = parse_durations(BASE_DIALOGUE)
line_durations.update(parse_durations(SUPPLEMENT_DIALOGUE))
for audio_id in mappings:
    if audio_id not in line_durations:
        raise SystemExit(f"Mapped voice id is not used by authored dialogue: {audio_id}")
    physical_id = "nw.audio.sd.soul.recorded.01" if audio_id == "nw.audio.sd.soul.01a" else audio_id
    source_ms = int(assets_by_id[physical_id]["duration_ms"])
    authored_ms = int(line_durations[audio_id])
    if authored_ms < source_ms:
        raise SystemExit(f"Dialogue timing clips {audio_id}: {authored_ms}ms < {source_ms}ms")
    if authored_ms - source_ms > 850:
        raise SystemExit(f"Excessive timing tail for {audio_id}: {authored_ms - source_ms}ms")

print("Validated Nightwalker voice catalog: 26 IDs, 25 reviewed MP3 assets, Batches 1-3 timing contract OK")
