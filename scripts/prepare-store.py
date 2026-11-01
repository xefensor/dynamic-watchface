#!/usr/bin/env python3
"""Prepare/check genuine native store captures and the held publication record."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import zipfile

from release_licenses import license_payloads


ROOT = Path(__file__).resolve().parents[1]
SCREENSHOTS = {
    "emery_idle.png": "normal.png",
    "emery_music.png": "music.png",
    "emery_notification_summary.png": "notification-summary.png",
    "emery_notification.png": "notification.png",
    "emery_dark.png": "dark.png",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def expected_manifest(root):
    package = json.loads((root / "package.json").read_text())
    listing = json.loads((root / "store/listing.json").read_text())
    provenance = json.loads((root / "docs/asset-provenance.json").read_text())
    require(listing["version"] == package["version"] == provenance["version"],
            "Refresh the listing and final native provenance for this release")
    require(listing["name"] == package["pebble"]["displayName"] == "Dynamic" and
            listing["author"] == package["author"] == "Xef" and
            listing["type"] == "watchface" and listing["target_platforms"] == ["emery"],
            "Publication identity or supported platform differs")
    require(listing["publication_status"] == "held-plugin-preview" and
            listing["store_submission_allowed"] is False,
            "Review Pebble's current plugin-preview publication rule explicitly")
    require(listing["screenshots"] == list(SCREENSHOTS), "Unexpected screenshot list")
    for name in [listing["description_file"], listing["release_notes_file"]]:
        require(Path(name).name == name and (root / "store" / name).read_text().strip(),
                "Missing publication copy: " + name)
    for name in ["PRIVACY.md", "SUPPORT.md", "THIRD-PARTY-NOTICES.md"]:
        require((root / name).read_text().strip(), "Missing publication document: " + name)
    pbw = root / "dist" / ("Dynamic-" + package["version"] + ".pbw")
    pbw_hash = digest(pbw.read_bytes())
    require(provenance["pbw"]["file"] == pbw.name and
            provenance["pbw"]["sha256"] == pbw_hash, "Final PBW provenance differs")
    native = {}
    with zipfile.ZipFile(pbw) as archive:
        require(archive.testzip() is None, "Corrupt release PBW")
        info = json.loads(archive.read("appinfo.json"))
        require(info["versionLabel"] == package["version"] and
                info["uuid"] == package["pebble"]["uuid"] and
                info["displayName"] == listing["name"] and
                info["companyName"] == listing["author"] and
                info["targetPlatforms"] == ["emery"] and
                info["watchapp"] == {"watchface": True} and
                info["usesPermissions"] == package["pebble"]["usesPermissions"],
                "PBW metadata differs from the reviewed listing")
        for name in ["emery/pebble-app.bin", "emery/app_resources.pbpack"]:
            native[name] = digest(archive.read(name))
            require(native[name] == provenance["native_payload_sha256"][name],
                    "Final native payload provenance differs: " + name)
        for name, content in license_payloads(root).items():
            require(archive.read(name) == content, "PBW license differs: " + name)
    declared = {image["file"]: image["sha256"] for image in provenance["images"]}
    images = []
    for target, source in SCREENSHOTS.items():
        relative = "docs/images/" + source
        data = (root / relative).read_bytes()
        require(data[:8] == b"\x89PNG\r\n\x1a\n" and len(data) >= 24 and
                struct.unpack_from(">II", data, 16) == (200, 228),
                "Native screenshot must be a raw 200x228 PNG: " + relative)
        require(digest(data) == declared[relative], "Screenshot provenance differs: " + relative)
        images.append({"file": target, "source": relative, "sha256": digest(data),
                       "width": 200, "height": 228, "native_pixels_modified": False})
    return {"version": package["version"], "pbw": {"file": pbw.name, "sha256": pbw_hash},
            "native_payload_sha256": native, "screenshots": images,
            "fixture_data": True, "public_store_submission_allowed": False,
            "status": "held-plugin-preview", "provenance": "docs/asset-provenance.json"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check without changing the kit")
    args = parser.parse_args()
    manifest = expected_manifest(ROOT)
    path = ROOT / "store/asset-manifest.json"
    if args.check:
        require(json.loads(path.read_text()) == manifest, "Store asset manifest is stale")
        for image in manifest["screenshots"]:
            require(digest((ROOT / "store" / image["file"]).read_bytes()) == image["sha256"],
                    "Store screenshot is stale: " + image["file"])
    else:
        for image in manifest["screenshots"]:
            shutil.copyfile(ROOT / image["source"], ROOT / "store" / image["file"])
        path.write_text(json.dumps(manifest, indent=2) + "\n")
    print("Store kit " + ("check" if args.check else "preparation") +
          ": PASS; 5 native screenshots; public submission HELD for plugin preview")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError, struct.error, zipfile.BadZipFile) as error:
        raise SystemExit("Store kit: FAIL: " + str(error))
