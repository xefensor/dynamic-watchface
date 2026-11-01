#!/usr/bin/env python3
"""Audit Dynamic's exact SDK bundle, wire compatibility and system-font use.

Run after building and permission-packaging the watchface. System fonts live
in PebbleOS, so this checks their references rather than claiming to measure
firmware glyph ink. Native screenshots remain the check for actual rendering.
Uses only Python's standard library and does not access the network.
"""

import argparse
import base64
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import struct
import tarfile
import uuid
import zipfile


ROOT = Path(__file__).resolve().parents[1]
APP_UUID = "cc1281f1-3b4d-45b5-b235-6881b1fd8fbd"
LEGACY_KEYS = (
    "TEMPERATURE", "REQUEST_WEATHER", "BACKGROUND_COLOR", "TEXT_COLOR",
    "DEPTH_EFFECT", "TEXT_SHADOW", "LARGE_TEXT", "USE_12_HOUR",
    "USE_FAHRENHEIT", "TEMPERATURE_FAHRENHEIT", "WEATHER_UPDATED_AT",
    "FORECAST_DATA", "FOLLOW_PEBBLE_UNITS", "WEATHER_LOCATION", "WEATHER_CLEAR",
    "SHOW_DAILY_RANGE", "SHOW_RAIN_CHANCE", "WEATHER_DETAILS", "LAYOUT_STYLE",
    "SHOW_WEATHER_ICON", "WEATHER_CONDITION", "COMPLICATION_DATA",
    "REQUEST_COMPLICATIONS", "CONTEXT_DATA", "REQUEST_CONTEXT", "SHOW_MUSIC",
    "SHOW_NOTIFICATIONS", "NOTIFICATION_PREVIEW", "NOTIFICATION_APPS",
    "SHOW_STATUS", "PANEL_TRANSITIONS",
)
APP_MODULES = {"index.js", "config.js", "localization.js", "settings-page.js"}
CORE_MODULES = {
    "index.js", "message-queue.js", "complications.js", "complication-data.js", "context.js",
}
CORE_SHA256 = "b53a21ccc039ce32843d5242f11b3c99d277c68420c40f59bab56040c45c8e33"
CORE_COMMIT = "d8ffdd11bd8e594378503e13d0a0d3cb46f5d897"
PBW_FILES = {
    "appinfo.json", "pebble-js-app.js", "pebble-js-app.js.map",
    "emery/pebble-app.bin", "emery/app_resources.pbpack", "emery/manifest.json",
}
LEGACY_ENDPOINTS = (
    "api.open-meteo.com", "api.openweathermap.org", "forecast.weather.gov",
)
NATIVE_FONT_SIZES = (
    ("LECO", 60), ("LECO", 42), ("LECO", 26), ("LECO", 20),
    ("GOTHIC", 14), ("GOTHIC", 18), ("GOTHIC", 24), ("GOTHIC", 28),
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def safe_name(name):
    path = PurePosixPath(name)
    return not path.is_absolute() and ".." not in path.parts and "\\" not in name


def source_matches(mapped, source, label):
    """Permit Webpack's trailing comments, never a transformed source body."""
    original = source.decode("utf-8")
    require(mapped.startswith(original), "Source map differs from current source: " + label)
    tail = mapped[len(original):]
    require(not tail or re.fullmatch(
        r"\n{2,}//////////////////\n// WEBPACK FOOTER(?:\n//[^\n]*)*\s*", tail),
        "Unexpected source-map transform: " + label)


def read_core(root, package):
    pin = json.loads((root / "vendor/CORE.json").read_text(encoding="utf-8"))
    require(pin["package"] == "pebble-watchface-core" and pin["version"] == "0.1.0",
            "Review this auditor before changing the shared core release")
    require(pin["source_commit"] == CORE_COMMIT and pin["sha256"] == CORE_SHA256,
            "Shared core provenance differs from the verified 0.1.0 release")
    name = pin["artifact"]
    require(name == "pebble-watchface-core-0.1.0.tgz", "Unexpected core archive name")
    require(package["dependencies"]["pebble-watchface-core"] == "file:vendor/" + name,
            "The app must use the committed, pinned core archive")
    raw = (root / "vendor" / name).read_bytes()
    require(sha256(raw) == CORE_SHA256, "Pinned core archive checksum differs")
    integrity = "sha512-" + base64.b64encode(hashlib.sha512(raw).digest()).decode()
    require(pin["integrity"] == integrity, "Pinned core npm integrity differs")
    lock = json.loads((root / "package-lock.json").read_text(encoding="utf-8"))
    entry = lock["packages"]["node_modules/pebble-watchface-core"]
    require(entry["version"] == "0.1.0" and entry["integrity"] == integrity,
            "Lockfile differs from the committed core archive")
    with tarfile.open(fileobj=io.BytesIO(raw), mode="r:gz") as archive:
        members = archive.getmembers()
        require(len(members) == len({item.name for item in members}) == 14,
                "Pinned core archive has unexpected entries")
        require(all(item.isfile() and safe_name(item.name) and
                    item.name.startswith("package/") for item in members),
                "Pinned core contains a nonregular or unsafe entry")
        files = {item.name: archive.extractfile(item).read() for item in members}
    core_info = json.loads(files["package/package.json"])
    require(core_info["name"] == pin["package"] and core_info["version"] == pin["version"],
            "Pinned core metadata differs")
    require(not core_info["pebble"].get("messageKeys"),
            "Unexpected package-owned keys could renumber existing app settings")
    installed = root / "node_modules/pebble-watchface-core"
    for name, content in files.items():
        relative = name.removeprefix("package/")
        require((installed / relative).read_bytes() == content,
                "Installed core differs from pinned archive: " + relative)
    with zipfile.ZipFile(io.BytesIO(files["package/dist.zip"])) as sdk:
        require(sdk.testzip() is None, "Pinned SDK library archive is corrupt")
        sdk_files = {name: sdk.read(name) for name in sdk.namelist()}
    for name, content in sdk_files.items():
        require(safe_name(name) and (installed / "dist" / name).read_bytes() == content,
                "SDK core staging differs from pinned archive: " + name)
    sources = {}
    for name in CORE_MODULES:
        content = files["package/src/js/" + name]
        require(sdk_files["js/" + name] == content,
                "Core source and packaged JavaScript differ: " + name)
        sources[name] = content
    return {"version": pin["version"], "sourceCommit": CORE_COMMIT,
            "archiveSha256": CORE_SHA256, "installedAndSdkStagingMatch": True}, sources


def audit(pbw, root=ROOT, sdk_font_header=None):
    package = json.loads((root / "package.json").read_text(encoding="utf-8"))
    settings = package["pebble"]
    require(settings["uuid"] == APP_UUID and settings["displayName"] == "Dynamic",
            "Dynamic's identity changed")
    require(settings["targetPlatforms"] == ["emery"] and
            settings["usesPermissions"] == ["ReadMusic"],
            "Unexpected platform or permission change")
    require(settings["resources"].get("media", []) == [],
            "The native style must not bundle custom font/image resources")
    require(tuple(settings["messageKeys"]) == LEGACY_KEYS + ("ACCENT_COLOR",),
            "Preserve the first 31 keys and append only ACCENT_COLOR")
    keys = {name: 10000 + index for index, name in enumerate(settings["messageKeys"])}
    generated = json.loads((root / "build/js/message_keys.json").read_text())
    require(generated == keys, "Generated JavaScript keys are stale")
    native_entries = re.findall(
        r"MESSAGE_KEY_(\w+)\s*=\s*(\d+)\s*;",
        (root / "build/src/message_keys.auto.c").read_text())
    native_keys = {name: int(value) for name, value in native_entries}
    require(len(native_entries) == len(native_keys) and native_keys == keys,
            "Generated C keys are stale or duplicated")
    core, core_sources = read_core(root, package)
    sdk_pbw = root / "build" / (package["name"] + ".pbw")
    with zipfile.ZipFile(pbw) as release, zipfile.ZipFile(sdk_pbw) as sdk:
        require(release.testzip() is None and sdk.testzip() is None, "Corrupt PBW archive")
        require(len(release.namelist()) == len(set(release.namelist())) == 6 and
                set(release.namelist()) == set(sdk.namelist()) == PBW_FILES,
                "Unexpected PBW entry inventory")
        info = json.loads(release.read("appinfo.json"))
        original = json.loads(sdk.read("appinfo.json"))
        expected = dict(original)
        expected["manifestVersion"] = 2
        expected["usesPermissions"] = settings["usesPermissions"]
        require(info == expected, "Permission packaging changed unexpected app metadata")
        require(info["uuid"] == APP_UUID and info["versionLabel"] == package["version"] and
                info["targetPlatforms"] == ["emery"] and
                info["watchapp"] == {"watchface": True} and
                info["usesPermissions"] == ["ReadMusic"], "Packaged metadata mismatch")
        require(info["appKeys"] == info["messageKeys"] == keys,
                "Packaged AppMessage key mapping differs")
        require(not info.get("resources", {}).get("media"),
                "The PBW still declares custom resources")
        inventory = []
        for name in sorted(PBW_FILES):
            content = release.read(name)
            require(name == "appinfo.json" or content == sdk.read(name),
                    "SDK payload changed while packaging: " + name)
            if name in ("pebble-js-app.js", "pebble-js-app.js.map"):
                require(content == (root / "build" / name).read_bytes(),
                        "Selected PBW differs from the current JavaScript build: " + name)
            inventory.append({"file": name, "bytes": len(content),
                              "sha256": sha256(content),
                              "sdkByteIdentical": content == sdk.read(name)})
        binary = release.read("emery/pebble-app.bin")
        resources = release.read("emery/app_resources.pbpack")
        for name, content in (("pebble-app.bin", binary), ("app_resources.pbpack", resources)):
            require(content == (root / "build/emery" / name).read_bytes(),
                    "Selected PBW differs from the current native build: " + name)
        require(binary.startswith(b"PBLAPP\x00\x00") and
                binary[104:120] == uuid.UUID(APP_UUID).bytes,
                "Native header UUID differs from appinfo")
        require(not resources or len(resources) >= 12, "Truncated resource pack")
        resource_count = struct.unpack_from("<I", resources)[0] if resources else 0
        require(resource_count == 0, "The app still contains custom resources")
        source_map = json.loads(release.read("pebble-js-app.js.map"))
        active_js = release.read("pebble-js-app.js").decode("utf-8")
    require(len(source_map["sources"]) == len(source_map["sourcesContent"]),
            "Invalid source map")
    require(len(source_map["sources"]) == len(set(source_map["sources"])),
            "Source map has duplicate modules")
    hosts, modules, xhr, bundled_keys = set(), set(), [], None
    for name, content in zip(source_map["sources"], source_map["sourcesContent"]):
        if name.startswith("./src/pkjs/"):
            relative = name.removeprefix("./")
            source_matches(content, (root / relative).read_bytes(), name)
            hosts.add(Path(relative).name)
        elif name.startswith("./~/pebble-watchface-core/dist/js/"):
            leaf = name.rsplit("/", 1)[1]
            require(leaf in core_sources, "Unexpected active core module: " + leaf)
            source_matches(content, core_sources[leaf], name)
            modules.add(leaf)
        elif name == "./build/js/message_keys.json":
            exported = re.match(r"^module\.exports\s*=\s*(\{[^\n]+\})", content)
            require(exported and json.loads(exported.group(1)) == keys,
                    "Bundled JavaScript key mapping is stale")
            bundled_keys = json.loads(exported.group(1))
        if "XMLHttpRequest" in content:
            xhr.append(name)
    require(hosts == APP_MODULES and modules == CORE_MODULES,
            "The active application/core dependency graph changed")
    require(bundled_keys == keys, "Bundled JavaScript key mapping is missing")
    require(xhr == ["./~/@rebble/clay/dist/js/index.js"],
            "Unexpected active HTTP client")
    require(not any(endpoint in active_js for endpoint in LEGACY_ENDPOINTS),
            "A legacy weather HTTP client is active")
    sources = "\n".join(path.read_text(encoding="utf-8") for path in
                        sorted((root / "src/c").glob("*")) if path.suffix in (".c", ".h"))
    require("fonts_get_system_font" in sources, "System-font loading is missing")
    require(not any(value in sources for value in
                    ("fonts_load_custom_font", "fonts_unload_custom_font", "FIRA_SANS")),
            "Legacy custom font code remains active")
    font_keys = sorted(set(re.findall(r"\bFONT_KEY_([A-Z0-9_]+)\b", sources)))
    linked_keys = [name for name in font_keys if ("RESOURCE_ID_" + name).encode() in binary]
    for family, size in NATIVE_FONT_SIZES:
        prefix = family + "_" + str(size)
        require(any(name == prefix or name.startswith(prefix + "_") for name in linked_keys),
                "The native binary is missing system-font family/size: " + prefix)
    if sdk_font_header is not None:
        catalog = set(re.findall(r"#define\s+FONT_KEY_([A-Z0-9_]+)\s+",
                                sdk_font_header.read_text(encoding="utf-8")))
        require(set(font_keys) <= catalog, "A font key is absent from the selected SDK catalog")
    return {
        "result": "PASS", "version": package["version"], "uuid": APP_UUID,
        "platform": "emery", "messageKeys": len(keys), "legacyKeysPreserved": True,
        "messageKeyCounts": {"package": len(keys), "generatedC": len(native_keys),
                             "generatedJavaScript": len(generated),
                             "bundledJavaScript": len(bundled_keys),
                             "pbwAppKeys": len(info["appKeys"]),
                             "pbwMessageKeys": len(info["messageKeys"])},
        "manifestVersion": info["manifestVersion"], "usesPermissions": info["usesPermissions"],
        "nativePayloadMatchesSdk": True, "customResourceCount": resource_count,
        "javaScriptPayloadMatchesBuild": True,
        "sourceSystemFontKeys": font_keys, "nativeSystemFontKeys": linked_keys,
        "sourceFontKeysNotLinked": sorted(set(font_keys) - set(linked_keys)),
        "hostModules": len(hosts),
        "coreModules": len(modules), "sourceMapMatchesCurrentSource": True,
        "sourceMapModuleCount": len(source_map["sources"]),
        "legacyWeatherClientActive": False, "core": core,
        "pbwSha256": sha256(pbw.read_bytes()), "inventory": inventory,
        "scope": "SDK/package/system-font-reference audit. Firmware glyph ink and screen layout require native rendering checks; no custom-PFO corpus is measured.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pbw", type=Path, required=True, help="Permission-packaged Dynamic PBW")
    parser.add_argument("--sdk-font-header", type=Path,
                        help="Optional selected SDK pebble_fonts.h catalog")
    args = parser.parse_args()
    try:
        result = audit(args.pbw, sdk_font_header=args.sdk_font_header)
    except (OSError, ValueError, KeyError, TypeError, UnicodeError, struct.error,
            tarfile.TarError, zipfile.BadZipFile) as error:
        parser.exit(1, "Dynamic native-style audit failed: " + str(error) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
