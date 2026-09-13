#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import pathlib
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
VOICEPACK = ROOT / "content/Nightwalker.voicepack"
MANIFEST = ROOT / "content/Nightwalker.audio"
BASE_DIALOGUE = ROOT / "content/Nightwalker.dialogue"
SUPPLEMENT_DIALOGUE = ROOT / "Nightwalker.voice.dialogue"

if not VOICEPACK.is_file():
    raise SystemExit("Missing content/Nightwalker.voicepack")


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

with zipfile.ZipFile(VOICEPACK) as archive:
    names = set(archive.namelist())
    audio_names = sorted(
        name for name in names
        if name.startswith("audio/") and name.lower().endswith((".mp3", ".wav"))
    )
    if len(audio_names) != 25:
        raise SystemExit(f"Expected 25 unique physical voice assets, found {len(audio_names)}")
    if "inventory.json" not in names:
        raise SystemExit("Nightwalker.voicepack is missing inventory.json")
    inventory = json.loads(archive.read("inventory.json").decode("utf-8"))
    if not isinstance(inventory, list) or len(inventory) != 25:
        raise SystemExit("Voice inventory must describe all 25 physical assets")

    inventory_by_path: dict[str, dict] = {}
    for item in inventory:
        relative = "audio/" + item["file"]
        if relative in inventory_by_path:
            raise SystemExit(f"Duplicate inventory file: {relative}")
        if relative not in names:
            raise SystemExit(f"Inventory references missing voice asset: {relative}")
        payload = archive.read(relative)
        if len(payload) != int(item["bytes"]):
            raise SystemExit(f"Voice size mismatch: {relative}")
        digest = hashlib.sha256(payload).hexdigest()
        if digest != item["sha256"]:
            raise SystemExit(f"Voice SHA-256 mismatch: {relative}")
        if int(item["duration_ms"]) <= 0:
            raise SystemExit(f"Invalid voice duration: {relative}")
        inventory_by_path[relative] = item

    if set(audio_names) != set(inventory_by_path):
        raise SystemExit("Voice archive and inventory file sets differ")

for audio_id, relative in mappings.items():
    if relative not in inventory_by_path:
        raise SystemExit(f"Manifest audio is absent from voice pack: {audio_id} -> {relative}")

physical_paths = set(mappings.values())
if len(physical_paths) != 25:
    raise SystemExit(
        f"Expected one intentional alias (26 IDs -> 25 files), found {len(physical_paths)} physical mappings"
    )
if mappings.get("nw.audio.sd.soul.01a") != mappings.get("nw.audio.sd.soul.recorded.01"):
    raise SystemExit("Soul 01a must reuse the reviewed Batch 2 'Do not ask' performance")

line_durations = parse_durations(BASE_DIALOGUE)
line_durations.update(parse_durations(SUPPLEMENT_DIALOGUE))
for audio_id, relative in mappings.items():
    if audio_id not in line_durations:
        raise SystemExit(f"Mapped voice id is not used by authored dialogue: {audio_id}")
    source_ms = int(inventory_by_path[relative]["duration_ms"])
    authored_ms = int(line_durations[audio_id])
    if authored_ms < source_ms:
        raise SystemExit(
            f"Dialogue timing clips {audio_id}: authored={authored_ms}ms source={source_ms}ms"
        )
    if authored_ms - source_ms > 850:
        raise SystemExit(
            f"Excessive timing tail for {audio_id}: authored={authored_ms}ms source={source_ms}ms"
        )

print(
    f"Validated Nightwalker voice archive: {len(mappings)} IDs, "
    f"{len(physical_paths)} physical assets, SHA-256 and timing contract OK"
)
