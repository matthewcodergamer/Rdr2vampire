#!/usr/bin/env python3
from __future__ import annotations

import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]

inventory = json.loads(
    (ROOT / "tests/fixtures/VoiceBatch2Inventory.json").read_text(encoding="utf-8")
)
if inventory["format"] != {
    "sample_rate_hz": 44100,
    "channels": 1,
    "bits_per_sample": 16,
}:
    raise SystemExit("Unexpected Batch 2 PCM contract")

assets = {asset["audio_id"]: asset for asset in inventory["assets"]}
if len(assets) != 10:
    raise SystemExit("Batch 2 must contain ten unique assets")
if len({asset["sha256"] for asset in assets.values()}) != len(assets):
    raise SystemExit("Duplicate reviewed Batch 2 payload")

dialogue = (ROOT / "Nightwalker.voice.dialogue").read_text(encoding="utf-8")
authored: dict[str, int] = {}
for raw in dialogue.splitlines():
    if not raw.startswith("line="):
        continue
    fields = raw[5:].split("|", 6)
    if len(fields) != 7:
        raise SystemExit(f"Malformed supplemental line: {raw}")
    if fields[4]:
        authored[fields[4]] = int(fields[5])

if set(authored) != set(assets):
    raise SystemExit(
        f"Catalog/inventory mismatch: catalog={sorted(authored)} inventory={sorted(assets)}"
    )
for audio_id, duration_ms in authored.items():
    source_ms = int(assets[audio_id]["duration_ms"])
    tail_ms = duration_ms - source_ms
    if tail_ms < 0:
        raise SystemExit(f"Subtitle timing clips {audio_id}: {duration_ms}<{source_ms}")
    if tail_ms > 800:
        raise SystemExit(f"Excessive subtitle tail for {audio_id}: {tail_ms}ms")

excluded = inventory.get("excluded_duplicates", [])
if len(excluded) != 1 or excluded[0]["matches_audio_id"] != "nw.audio.sd.question.none.01":
    raise SystemExit("Expected reviewed Batch 1 duplicate record is missing")

print(f"Validated {len(assets)} Batch 2 inventory records")
