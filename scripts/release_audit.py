#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXPECTED_VERSION = "1.0.0-rc1"
EXPECTED_VOICE_WAVS = 59


def fail(message: str) -> None:
    print(f"RELEASE AUDIT FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(path: str) -> str:
    full = ROOT / path
    if not full.is_file():
        fail(f"required file missing: {path}")
    return full.read_text(encoding="utf-8")


def tracked_files() -> list[str]:
    try:
        output = subprocess.check_output(
            ["git", "ls-files", "-z"], cwd=ROOT, stderr=subprocess.STDOUT
        )
        return [item for item in output.decode("utf-8").split("\0") if item]
    except Exception as exc:
        fail(f"could not enumerate tracked files: {exc}")


version_h = read("include/nightwalker/Version.h")
if 'kString = "1.0.0-rc1"' not in version_h:
    fail("Version.h must identify 1.0.0-rc1")
if "kMajor = 1" not in version_h or "kMinor = 0" not in version_h or "kPatch = 0" not in version_h:
    fail("Version.h numeric version must be 1.0.0")
if "kStoryModeOnly = true" not in version_h:
    fail("Story Mode compile-time version guard is missing")

project = read("Nightwalker.vcxproj")
if f"<NightwalkerVersion>{EXPECTED_VERSION}</NightwalkerVersion>" not in project:
    fail("Nightwalker.vcxproj release version does not match Version.h")
if "NIGHTWALKER_STORY_MODE_ONLY=1" not in project:
    fail("Story Mode project definition is missing")
if "<GenerateDebugInformation>false</GenerateDebugInformation>" not in project:
    fail("Release build must disable shipping PDB/debug-link generation")
if "package-release.ps1" not in project:
    fail("Release x64 build is not wired to the deterministic packager")

readme = read("README.md")
required_readme = [
    "Story Mode only",
    "## Dependencies",
    "## Installation",
    "## Uninstall",
    "## Default controls",
    "## Configuration",
    "## Troubleshooting",
    "## Compatibility notes",
    "## Copyright and original-asset statement",
    "## Known limitations",
    "Nightwalker-1.0.0-rc1-win64.zip",
]
for marker in required_readme:
    if marker not in readme:
        fail(f"README release requirement missing: {marker}")

changelog = read("CHANGELOG.md")
if "## 1.0.0-rc1" not in changelog:
    fail("CHANGELOG does not contain the RC1 release section")
notices = read("THIRD_PARTY_NOTICES.md")
for marker in ("Red Dead Redemption 2", "Script Hook RDR2", "Dawnwalker"):
    if marker not in notices:
        fail(f"third-party notice missing required boundary: {marker}")

matrix = read("docs/RELEASE_TEST_MATRIX.md")
for marker in (
    "Arthur", "John", "Saint Denis dense streets", "Church/cathedral encounter zone",
    "Alleys", "Forest", "Open plains", "Steep slopes/stairs", "Water edges",
    "Mounted -> unmounted", "Wanted level active", "Nearby Story Mode mission",
    "Low frame rate", "High frame rate", "Keyboard/mouse", "Controller",
    "Shadowstep never leaves player invisible", "Boss cannot duplicate",
    "No proprietary Dawnwalker assets are included",
):
    if marker not in matrix:
        fail(f"release test matrix missing required case: {marker}")

files = tracked_files()
forbidden_suffixes = {
    ".asi", ".dll", ".exe", ".lib", ".pdb", ".obj", ".zip", ".log",
    ".ilk", ".exp", ".iobj", ".ipdb", ".tlog",
}
for path in files:
    p = pathlib.PurePosixPath(path)
    lower = path.lower()
    if p.suffix.lower() in forbidden_suffixes:
        fail(f"tracked build/runtime artifact: {path}")
    if lower.startswith(("bin/", "build/", "artifacts/", ".vs/")):
        fail(f"tracked build directory content: {path}")
    if lower.startswith("third_party/scripthookrdr2/") and path != "third_party/ScriptHookRDR2/README.md":
        fail(f"local Script Hook SDK payload is tracked: {path}")
    if "dawnwalker" in p.name.lower():
        fail(f"Dawnwalker-named payload is tracked: {path}")

voice_wavs = sorted(
    path for path in files
    if path.startswith("content/audio/") and pathlib.PurePosixPath(path).suffix.lower() == ".wav"
)
if len(voice_wavs) != EXPECTED_VOICE_WAVS:
    fail(f"canonical voice WAV set changed: expected {EXPECTED_VOICE_WAVS}, found {len(voice_wavs)}")
if any("voice_batch_1" in pathlib.PurePosixPath(path).name for path in voice_wavs):
    fail("fallback voice_batch_1 filename remains in canonical voice library")

for path in files:
    if not path.startswith("content/"):
        continue
    if path in {
        "content/Nightwalker.dialogue",
        "content/Nightwalker.audio",
        "content/audio/README.md",
    }:
        continue
    suffix = pathlib.PurePosixPath(path).suffix.lower()
    if path.startswith("content/audio/") and suffix == ".wav":
        continue
    fail(f"unreviewed release content payload under content/: {path}")

source_text = "\n".join(
    (ROOT / path).read_text(encoding="utf-8", errors="ignore")
    for path in files
    if path.startswith(("src/", "include/"))
)
for pattern in (r"\bNETWORK::", r"\bNETWORK_[A-Z0-9_]+"):
    if re.search(pattern, source_text):
        fail(f"network-native token found in Story Mode source: {pattern}")

collision_tokens = ("SET_ENTITY_COLLISION", "SET_ENTITY_COMPLETELY_DISABLE_COLLISION")
for token in collision_tokens:
    if token in source_text:
        fail(f"collision-changing native introduced without release-matrix ownership update: {token}")

attachment_tokens = ("ATTACH_ENTITY_TO_ENTITY", "DETACH_ENTITY")
for token in attachment_tokens:
    if token in source_text:
        fail(f"persistent entity-attachment native introduced without release-matrix ownership update: {token}")

hud_hits: list[str] = []
for path in files:
    if not path.startswith("src/"):
        continue
    text = (ROOT / path).read_text(encoding="utf-8", errors="ignore")
    if "DRAW_RECT" in text or "_BG_DISPLAY_TEXT" in text:
        hud_hits.append(path)
if sorted(set(hud_hits)) != ["src/game/GameBossBarApi.cpp"]:
    fail(f"custom draw-native boundary changed: {sorted(set(hud_hits))}")

packager = read("scripts/package-release.ps1")
for marker in (
    "Nightwalker.asi", "Nightwalker.ini", "Nightwalker.dialogue",
    "Nightwalker.voice.dialogue", "Nightwalker.audio", "content/audio",
    "Expected 59 canonical voice WAVs", "README.md", "CHANGELOG.md",
    "THIRD_PARTY_NOTICES.md", "Compress-Archive",
):
    if marker not in packager:
        fail(f"release packager allowlist/behavior missing: {marker}")
if "materialize-voicepack.ps1" in packager:
    fail("release packager still depends on obsolete materialized voicepack")

voice_packager = read("scripts/package-voice-assets.ps1")
for marker in ("content/audio", "Expected 59 canonical voice WAVs", "Nightwalker.audio"):
    if marker not in voice_packager:
        fail(f"voice packager missing canonical-library behavior: {marker}")
if "materialize-voicepack.ps1" in voice_packager:
    fail("voice-only packager still depends on obsolete materialized voicepack")

if "scripts/materialize-voicepack.ps1" in files:
    fail("obsolete voicepack materializer is still tracked")
if "content/Nightwalker.voicepack" in files:
    fail("obsolete Nightwalker.voicepack archive is still tracked")
if any(path.startswith("content/voicepack/") for path in files):
    fail("obsolete base64 voicepack chunks are still tracked")

print(f"Nightwalker release audit passed for {EXPECTED_VERSION}")
