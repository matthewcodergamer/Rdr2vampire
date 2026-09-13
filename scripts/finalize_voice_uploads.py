#!/usr/bin/env python3
from __future__ import annotations

# One-shot migration: validates the uploaded owner MP3 library, writes the final
# runtime catalogs/build wiring, then removes itself from the completed branch.
import base64
import json
import pathlib
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
PARTS = sorted((ROOT / "scripts").glob("finalize_voice_payload.part*.txt"))
if not PARTS:
    raise SystemExit("Final voice payload chunks are missing")
encoded = "".join(part.read_text(encoding="utf-8").strip() for part in PARTS)
payload = json.loads(zlib.decompress(base64.b64decode(encoded)).decode("utf-8"))

voice_files = sorted((ROOT / "content").glob("Vam-*.mp3"))
if len(voice_files) != 64:
    raise SystemExit(f"Expected 64 uploaded owner MP3 files, found {len(voice_files)}")
for voice in voice_files:
    header = voice.read_bytes()[:3]
    if not (header == b"ID3" or (len(header) >= 2 and header[0] == 0xFF and header[1] & 0xE0 == 0xE0)):
        raise SystemExit(f"Not a valid MP3 source: {voice.name}")

for relative, content in payload.items():
    target = ROOT / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8", newline="\n")

for obsolete in (
    ROOT / "content" / "Nightwalker.voicepack",
    ROOT / "scripts" / "materialize-voicepack.ps1",
):
    if obsolete.exists():
        obsolete.unlink()

readme = ROOT / "README.md"
text = readme.read_text(encoding="utf-8")
text = text.replace(
    "- `Nightwalker.dialogue` — original Nightwalker subtitle/narrative data;",
    "- `Nightwalker.dialogue` — original Nightwalker subtitle/narrative data;\n"
    "- `Nightwalker.voice.dialogue` — supplemental recorded conversation variants;\n"
    "- `Nightwalker.audio` and `audio/` — the owner-created vampire voice library;",
)
text = text.replace(
    "4. Copy `Nightwalker.asi`, `Nightwalker.ini`, `Nightwalker.dialogue`, `README.md`, `CHANGELOG.md`, `THIRD_PARTY_NOTICES.md`, and optionally the checksum text file into the RDR2 game directory scanned by your ASI loader — normally the same main folder that contains the game executable and Script Hook files.",
    "4. Copy the complete extracted Nightwalker package into the RDR2 game directory scanned by your ASI loader. Keep `Nightwalker.asi`, `Nightwalker.ini`, both dialogue catalogs, `Nightwalker.audio`, and the complete `audio` folder together.",
)
control_note = """
### Vampire conversation controls

During the Saint Denis confrontation, hold **L2 / LT** to focus the vampire. The entity-linked RDR2 prompts offer **TALK**, **ANTAGONIZE**, and **LEAVE**. **R2 / RT remains the normal fire/hostile trigger**: aiming, holding aim, firing, drawing weapons, punching, melee attacks, lasso posture, approaching and backing away all feed the reactive dialogue system.

Dialogue is randomized by complete authored sequence through a non-repeating shuffle bag. Individual sentences from different performances are never mixed.
"""
marker = "Development/debug keys are disabled by default"
if "### Vampire conversation controls" not in text and marker in text:
    text = text.replace(marker, control_note + "\n" + marker)
text = text.replace(
    "- Optional audio remains subtitle-first/fallback-safe; this RC does not bundle proprietary game audio.",
    "- Owner-created Nightwalker MP3 voice assets are bundled with subtitle-first fallback. Spatial 3D mouth-positioned audio and phoneme lip-sync remain future upgrades.",
)
readme.write_text(text, encoding="utf-8", newline="\n")

for part in PARTS:
    part.unlink()
for one_shot in (
    ROOT / "scripts" / "finalize_voice_uploads.py",
    ROOT / ".github" / "workflows" / "finalize-voice-uploads.yml",
):
    if one_shot.exists():
        one_shot.unlink()

print(f"Finalized {len(voice_files)} physical MP3s and {len(payload)} runtime/source files")
