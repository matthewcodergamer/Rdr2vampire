#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXPECTED_VERSION = "1.0.0-rc1"


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
        output = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT)
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
for marker in (
    "Story Mode only", "## Dependencies", "## Installation", "## Uninstall",
    "## Default controls", "## Configuration", "## Troubleshooting",
    "## Compatibility notes", "## Copyright and original-asset statement",
    "## Known limitations", "Nightwalker-1.0.0-rc1-win64.zip",
):
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

allowed_content = {"content/Nightwalker.dialogue", "content/Nightwalker.audio"}
for path in files:
    if not path.startswith("content/"):
        continue
    if path in allowed_content:
        continue
    suffix = pathlib.PurePosixPath(path).suffix.lower()
    if path.startswith("content/Vam-") and suffix == ".mp3":
        continue
    fail(f"unreviewed release content payload under content/: {path}")

manifest = read("content/Nightwalker.audio")
mapped_paths: set[str] = set()
for raw in manifest.splitlines():
    line = raw.strip()
    if not line.startswith("asset="):
        continue
    fields = line[6:].split("|", 1)
    if len(fields) != 2 or not fields[0] or not fields[1]:
        fail(f"malformed Nightwalker.audio mapping: {line}")
    asset_id, relative = fields
    if not re.fullmatch(r"[A-Za-z0-9._-]+", asset_id):
        fail(f"unsafe audio id: {asset_id}")
    if ".." in relative or relative.startswith(("/", "\\")):
        fail(f"unsafe audio path: {relative}")
    if pathlib.PurePosixPath(relative).suffix.lower() not in {".wav", ".mp3"}:
        fail(f"unsupported audio path: {relative}")
    relative_path = pathlib.PurePosixPath(relative)
    if len(relative_path.parts) != 2 or relative_path.parts[0] != "audio":
        fail(f"audio mapping must target installed audio/ layout: {relative}")
    source = ROOT / "content" / relative_path.name
    if not source.is_file():
        fail(f"manifest audio source missing from content/: {relative_path.name}")
    mapped_paths.add(f"content/{relative_path.name}")

physical_audio = {p for p in files if p.startswith("content/Vam-") and p.endswith(".mp3")}
unmapped = sorted(physical_audio - mapped_paths)
if unmapped:
    fail(f"unmapped voice payload(s): {unmapped}")
if len(physical_audio) != 64:
    fail(f"expected 64 physical owner voice MP3s, found {len(physical_audio)}")
if len(mapped_paths) != 64:
    fail(f"expected 64 unique mapped voice files, found {len(mapped_paths)}")
if manifest.count("asset=") != 67:
    fail(f"expected 67 dialogue audio mappings, found {manifest.count('asset=')}")

source_text = "\n".join(
    (ROOT / path).read_text(encoding="utf-8", errors="ignore")
    for path in files if path.startswith(("src/", "include/"))
)
for pattern in (r"\bNETWORK::", r"\bNETWORK_[A-Z0-9_]+"):
    if re.search(pattern, source_text):
        fail(f"network-native token found in Story Mode source: {pattern}")
for token in ("SET_ENTITY_COLLISION", "SET_ENTITY_COMPLETELY_DISABLE_COLLISION"):
    if token in source_text:
        fail(f"collision-changing native introduced without release-matrix ownership update: {token}")
for token in ("ATTACH_ENTITY_TO_ENTITY", "DETACH_ENTITY"):
    if token in source_text:
        fail(f"persistent entity-attachment native introduced without release-matrix ownership update: {token}")

hud_hits: list[str] = []
for path in files:
    if path.startswith("src/"):
        text = (ROOT / path).read_text(encoding="utf-8", errors="ignore")
        if "DRAW_RECT" in text or "_BG_DISPLAY_TEXT" in text:
            hud_hits.append(path)
if sorted(set(hud_hits)) != ["src/game/GameBossBarApi.cpp"]:
    fail(f"custom draw-native boundary changed: {sorted(set(hud_hits))}")

packager = read("scripts/package-release.ps1")
for marker in (
    "Nightwalker.asi", "Nightwalker.ini", "Nightwalker.dialogue",
    "Nightwalker.voice.dialogue", "Nightwalker.audio", "Vam-*.mp3",
    "README.md", "CHANGELOG.md", "THIRD_PARTY_NOTICES.md", "Compress-Archive",
):
    if marker not in packager:
        fail(f"release packager allowlist/behavior missing: {marker}")

print(f"Nightwalker release audit passed for {EXPECTED_VERSION} with {len(mapped_paths)} voice files")
