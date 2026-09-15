#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"COMMERCIAL LICENSE AUDIT FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(path: str) -> str:
    full = ROOT / path
    if not full.is_file():
        fail(f"required file missing: {path}")
    return full.read_text(encoding="utf-8")


def tracked_files() -> list[str]:
    out = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT)
    return [item for item in out.decode("utf-8").split("\0") if item]


files = tracked_files()
for path in files:
    lower = path.lower()
    name = pathlib.PurePosixPath(path).name.lower()
    if lower.endswith((".pem", ".key", ".p12", ".pfx")) and path != "keys/nightwalker_license_public.pem":
        fail(f"private/unknown key material is tracked: {path}")
    if name in {"nightwalker.license", "nightwalker.device.txt"}:
        fail(f"customer/local activation file is tracked: {path}")

public_key = (ROOT / "keys/nightwalker_license_public.pem").read_bytes()
expected_public_sha = "31bb317f6391e86b46a9db1a4d8e44810cb077f74842a9267cdd0b0908247c28"
actual_public_sha = hashlib.sha256(public_key).hexdigest()
if actual_public_sha != expected_public_sha:
    fail(f"public key fingerprint changed: {actual_public_sha}")

project = read("Nightwalker.vcxproj")
for marker in (
    "src\\licensing\\License.cpp",
    "src\\licensing\\WindowsLicenseGate.cpp",
    "include\\nightwalker\\licensing\\License.h",
    "bcrypt.lib",
):
    if marker not in project:
        fail(f"native commercial build wiring missing: {marker}")

if "_DEBUG;NIGHTWALKER_DEV_LICENSE_BYPASS=1;" not in project:
    fail("Debug-only developer license bypass is missing")
release_section = project.split("<ItemDefinitionGroup Condition=\"'$(Configuration)'=='Release'\">", 1)
if len(release_section) != 2:
    fail("Release configuration block not found")
release_block = release_section[1].split("</ItemDefinitionGroup>", 1)[0]
if "NIGHTWALKER_DEV_LICENSE_BYPASS" in release_block:
    fail("developer license bypass leaked into Release configuration")

runtime = read("src/core/RuntimeLifecycle.cpp")
if "ValidateInstalledLicense" not in runtime:
    fail("runtime no longer verifies the installed commercial license")
if runtime.find("ValidateInstalledLicense") > runtime.find("config_=Config::Load"):
    fail("commercial license must be checked before gameplay/config systems initialize")

windows_gate = read("src/licensing/WindowsLicenseGate.cpp")
for marker in (
    "gamerstriperdev|nightwalker-rdr2|",
    "BCRYPT_ECDSA_PUBLIC_P256_MAGIC",
    "BCryptVerifySignature",
    "Nightwalker.device.txt",
    "Nightwalker.license",
):
    if marker not in windows_gate:
        fail(f"Windows commercial license boundary missing: {marker}")

signer = read("tools/sign-nightwalker-license.mjs")
for marker in ("issuer=gamerstriperdev", "product=nightwalker-rdr2", "ieee-p1363"):
    if marker not in signer:
        fail(f"seller-side signer contract changed: {marker}")

ignore = read(".gitignore")
for marker in ("Nightwalker.license", "Nightwalker.device.txt", "*.pem", "*.key"):
    if marker not in ignore:
        fail(f"secret/local-file ignore missing: {marker}")

print("gamerstriperdev Nightwalker commercial license audit passed")
