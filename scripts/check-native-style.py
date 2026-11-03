#!/usr/bin/env python3
"""Audit Dynamic's exact SDK bundle, compatible wire keys and native assets.

Run after building and permission-packaging the watchface. The body fonts are
read from this PBW, and the clock geometry is regenerated from its licensed
source font. This proves packaged/source identity and glyph availability;
native screenshots remain the check for actual appearance and motion.
The clock generator requires fontTools and skia-pathops. This audit never
accesses the network.
"""

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import sys
import tarfile
import unicodedata
import uuid
import zipfile
from release_licenses import LICENSE_FILES, license_payloads
from pinned_dependencies import DEPENDENCIES, verify_dependency


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
    "SHOW_STATUS", "PANEL_TRANSITIONS", "ACCENT_COLOR",
)
APP_MODULES = {"index.js", "config.js", "localization.js", "settings-page.js", "settings-preview.js", "activity-presentation.js", "activity-navigation.js"}
CORE_MODULES = {
    "readings.js", "message-queue.js", "complications.js", "complication-data.js",
}
ACTIVITY_MODULES = {
    "index.js", "wire.js", "view.js", "consumer.js", "publisher.js", "selection.js", "activity.js",
    "adapters/legacy/context.js", "adapters/legacy/complication-data.js", "summary.js", "summary-view.js",
}
ACTIVITY_RELEASE = {
    "version": "0.4.0",
    "sourceCommit": "9db426c36e591f3db98819b5b05ffd9da2af27b4",
    "archiveSha256": "9e640aea91d08633e91e59dab3d1e0ee6243efe0109d971e42d356a4d67affbe",
}
PROCESS_INFO_BYTES = 130
PROCESS_INFO_SIZE_LIMIT = 0xffff
EMERY_BINARY_SIZE_LIMIT = 0x20000
SDK_PBW_FILES = {
    "appinfo.json", "pebble-js-app.js", "pebble-js-app.js.map",
    "emery/pebble-app.bin", "emery/app_resources.pbpack", "emery/manifest.json",
}
PBW_FILES = SDK_PBW_FILES | set(LICENSE_FILES) | {"plugin.js"}
LEGACY_ENDPOINTS = (
    "api.open-meteo.com", "api.openweathermap.org", "forecast.weather.gov",
)
RESOURCE_TABLE_BYTES = 256 * 16
BODY_SIZES = (11, 14, 16, 20, 24, 26, 28, 32)
CLOCK_CHARACTERS = "0123456789:.-+,/%°CF"
# These are the reviewed three-codepoint prefixes from the original 17
# language arrays. Norwegian shares Danish storage without losing a locale.
REVIEWED_WEEKDAYS = {
    "en": "SUN MON TUE WED THU FRI SAT",
    "cs": "NED PON ÚTE STŘ ČTV PÁT SOB",
    "sk": "NED PON UTO STR ŠTV PIA SOB",
    "de": "SON MON DIE MIT DON FRE SAM",
    "fr": "DIM LUN MAR MER JEU VEN SAM",
    "es": "DOM LUN MAR MIÉ JUE VIE SÁB",
    "it": "DOM LUN MAR MER GIO VEN SAB",
    "pt": "DOM SEG TER QUA QUI SEX SÁB",
    "nl": "ZON MAA DIN WOE DON VRI ZAT",
    "pl": "NIE PON WTO ŚRO CZW PIĄ SOB",
    "da": "SØN MAN TIR ONS TOR FRE LØR",
    "sv": "SÖN MÅN TIS ONS TOR FRE LÖR",
    "fi": "SUN MAA TII KES TOR PER LAU",
    "hu": "VAS HÉT KED SZE CSÜ PÉN SZO",
    "ro": "DUM LUN MAR MIE JOI VIN SÂM",
    "tr": "PAZ PAZ SAL ÇAR PER CUM CUM",
}
REVIEWED_WEEKDAY_LOOKUP = """
static const char *prv_get_localized_day(const char *locale,unsigned day) {
  unsigned row=0;
  if(day>6)day=0;
  if(locale&&locale[0]&&locale[1]&&
      (!locale[2]||locale[2]=='_'||locale[2]=='-')) {
    char a=locale[0],b=locale[1];
    if(a=='n'&&(b=='o'||b=='b'||b=='n')){a='d';b='a';}
    for(unsigned i=1;i<sizeof(s_weekdays)/sizeof(s_weekdays[0]);++i)
      if(s_weekdays[i].language[0]==a&&s_weekdays[i].language[1]==b){row=i;break;}
  }
  return s_weekdays[row].days[day];
}
"""
BODY_TEXT_CORPORA = {
    "cyrillicMusicAndNames": (
        "Колесом / Kolesom", "Молчат Дома", "Україна", "БЕЛАРУСЬ", "Жыве", "Четверг",
    ),
    "greekMusicAndNames": ("Σήμερα", "Ελληνικά", "Άλμπουμ"),
    "latinExtendedAdditional": ("Tiếng Việt", "Ứng dụng"),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def safe_name(name):
    path = PurePosixPath(name)
    return not path.is_absolute() and ".." not in path.parts and "\\" not in name


def read_elf_sections(elf):
    """Read the linked ARM ELF32 section extents without requiring SDK tools."""
    require(len(elf) >= 52 and elf[:7] == b"\x7fELF\x01\x01\x01" and
            struct.unpack_from("<H", elf, 18)[0] == 40,
            "Expected a little-endian ARM ELF32 native build")
    offset = struct.unpack_from("<I", elf, 32)[0]
    entry_bytes, count, names_index = struct.unpack_from("<HHH", elf, 46)
    require(entry_bytes == 40 and 0 < count and names_index < count and
            offset + count * entry_bytes <= len(elf), "Invalid native ELF section directory")
    entries = [struct.unpack_from("<10I", elf, offset + index * entry_bytes)
               for index in range(count)]
    names_entry = entries[names_index]
    names = elf[names_entry[4]:names_entry[4] + names_entry[5]]
    require(names_entry[1] == 3 and len(names) == names_entry[5],
            "Invalid native ELF section names")
    sections = {}
    for entry in entries[1:]:
        name_at, kind, flags, address, data_at, size = entry[:6]
        require(name_at < len(names) and b"\x00" in names[name_at:],
                "Invalid native ELF section name")
        name = names[name_at:names.index(b"\x00", name_at)].decode("ascii")
        require(name and name not in sections and
                (kind == 8 or data_at + size <= len(elf)),
                "Invalid, duplicate or truncated native ELF section: " + name)
        sections[name] = {"type": kind, "flags": flags, "address": address,
                          "size": size, "offset": data_at}
    return sections


def audit_native_sizes(binary, elf):
    """Check uint16 loaded/static sizes separately from Emery's file limit.

    SDK 4.33.1 stores 32-bit relocation entries after load_size; these bytes
    count towards the 128 KiB file limit, not the loaded image or static extent.
    ELF extents independently detect overflow instead of trusting uint16 fields.
    """
    require(len(binary) >= PROCESS_INFO_BYTES and binary.startswith(b"PBLAPP\x00\x00") and
            binary[104:120] == uuid.UUID(APP_UUID).bytes,
            "Native header UUID differs from appinfo")
    sections = read_elf_sections(elf)
    header = sections.get(".header", {})
    require(header.get("address") == 0 and header.get("size") == PROCESS_INFO_BYTES,
            "Native ELF process-info header differs")
    allocated = [section for section in sections.values() if section["flags"] & 2]
    loaded = [section for section in allocated if section["type"] != 8]
    load_size = max(section["address"] + section["size"] for section in loaded)
    virtual_size = max(section["address"] + section["size"] for section in allocated)
    require(load_size <= PROCESS_INFO_SIZE_LIMIT,
            "Native loaded image exceeds the SDK's 65,535-byte process-info limit")
    require(virtual_size <= PROCESS_INFO_SIZE_LIMIT,
            "Native virtual size exceeds the SDK's 65,535-byte process-info limit")
    require(struct.unpack_from("<H", binary, 14)[0] == load_size and
            struct.unpack_from("<H", binary, 128)[0] == virtual_size,
            "Native process-info sizes differ from the linked ELF")
    require(len(binary) <= EMERY_BINARY_SIZE_LIMIT,
            "Complete native binary exceeds Emery's 128 KiB SDK limit")
    relocation_count = struct.unpack_from("<I", binary, 100)[0]
    require(len(binary) == load_size + relocation_count * 4,
            "Native relocation table length differs from process-info")
    for section in loaded:
        start, size = section["address"], section["size"]
        if start >= PROCESS_INFO_BYTES:
            require(binary[start:start + size] ==
                    elf[section["offset"]:section["offset"] + size],
                    "Native loaded section differs from the linked ELF")
    return {"fileBytes": len(binary), "loadedBytes": load_size,
            "virtualBytes": virtual_size, "relocationEntries": relocation_count,
            "relocationBytes": relocation_count * 4,
            "processInfoSizeLimit": PROCESS_INFO_SIZE_LIMIT,
            "completeBinaryLimit": EMERY_BINARY_SIZE_LIMIT,
            "loadedHeadroomBytes": PROCESS_INFO_SIZE_LIMIT - load_size,
            "virtualHeadroomBytes": PROCESS_INFO_SIZE_LIMIT - virtual_size,
            "linkedElfSectionsMatch": True,
            "scope": "Loaded image and static extent; this does not measure runtime heap availability."}


def read_resource_pack(packed, expected_count):
    """Read SDK resource entries without treating pack padding as resources."""
    require(len(packed) >= 12, "Truncated resource pack")
    count = struct.unpack_from("<I", packed)[0]
    require(count == expected_count, "Packaged resource count differs from the manifest")
    require(0 < count <= 256, "Expected the declared body font resources")
    data_start = 12 + RESOURCE_TABLE_BYTES
    require(len(packed) >= data_start, "Truncated resource table")
    entries = {}
    spans = []
    for index in range(count):
        resource_id, offset, size, crc = struct.unpack_from("<IIII", packed, 12 + index * 16)
        require(resource_id == index + 1, "Nonsequential or duplicate resource ID")
        start = data_start + offset
        require(size > 0 and start >= data_start and start + size <= len(packed),
                "Resource extends beyond its pack")
        spans.append((start, start + size))
        entries[resource_id] = {"data": packed[start:start + size], "crc": crc,
                                "offset": offset, "bytes": size}
    spans.sort()
    require(all(before[1] <= after[0] for before, after in zip(spans, spans[1:])),
            "Overlapping resource bodies")
    return entries


def font_codepoints(raw):
    """Read nonmissing Unicode cmap entries from a TrueType/OpenType font."""
    require(len(raw) >= 12 and raw[:4] in (b"\x00\x01\x00\x00", b"OTTO"),
            "Unsupported source-font container; expected an unpacked SFNT")
    tables = {}
    table_count = struct.unpack_from(">H", raw, 4)[0]
    require(12 + table_count * 16 <= len(raw), "Truncated source-font directory")
    for index in range(table_count):
        tag, _checksum, offset, size = struct.unpack_from(">4sIII", raw, 12 + index * 16)
        require(tag not in tables and offset + size <= len(raw),
                "Invalid or duplicate source-font table")
        tables[tag] = raw[offset:offset + size]
    require(b"cmap" in tables, "Source font lacks a Unicode cmap")
    cmap = tables[b"cmap"]
    require(len(cmap) >= 4, "Truncated source-font cmap")
    count = struct.unpack_from(">H", cmap, 2)[0]
    require(4 + count * 8 <= len(cmap), "Truncated source-font cmap directory")
    codepoints = set()
    for index in range(count):
        platform, encoding, offset = struct.unpack_from(">HHI", cmap, 4 + index * 8)
        if platform != 0 and not (platform == 3 and encoding in (1, 10)):
            continue
        require(offset + 2 <= len(cmap), "Invalid Unicode cmap offset")
        format_number = struct.unpack_from(">H", cmap, offset)[0]
        if format_number == 4:
            require(offset + 14 <= len(cmap), "Truncated format4 cmap header")
            size = struct.unpack_from(">H", cmap, offset + 2)[0]
            data = cmap[offset:offset + size]
            require(len(data) == size, "Truncated format4 cmap")
            segments = struct.unpack_from(">H", data, 6)[0] // 2
            end_at, start_at = 14, 16 + segments * 2
            delta_at, range_at = start_at + segments * 2, start_at + segments * 4
            require(range_at + segments * 2 <= len(data), "Truncated format4 cmap segments")
            for segment in range(segments):
                end = struct.unpack_from(">H", data, end_at + segment * 2)[0]
                start = struct.unpack_from(">H", data, start_at + segment * 2)[0]
                delta = struct.unpack_from(">h", data, delta_at + segment * 2)[0]
                distance = struct.unpack_from(">H", data, range_at + segment * 2)[0]
                require(start <= end, "Invalid format4 cmap range")
                for point in range(start, min(end, 0xfffe) + 1):
                    glyph = (point + delta) & 0xffff
                    if distance:
                        at = range_at + segment * 2 + distance + (point - start) * 2
                        require(at + 2 <= len(data), "Format4 glyph extends beyond cmap")
                        glyph = struct.unpack_from(">H", data, at)[0]
                        if glyph:
                            glyph = (glyph + delta) & 0xffff
                    if glyph:
                        codepoints.add(point)
        elif format_number == 12:
            require(offset + 16 <= len(cmap), "Truncated format12 cmap header")
            size, _language, groups = struct.unpack_from(">III", cmap, offset + 4)
            data = cmap[offset:offset + size]
            require(len(data) == size and 16 + groups * 12 <= size,
                    "Truncated format12 cmap groups")
            for group in range(groups):
                start, end, first_glyph = struct.unpack_from(">III", data, 16 + group * 12)
                require(start <= end <= 0x10ffff, "Invalid format12 cmap range")
                codepoints.update(range(start + (first_glyph == 0), end + 1))
    require(codepoints, "No supported Unicode cmap found in source font")
    return codepoints


def read_pfo(data):
    """Parse metrics/lookup structure of the SDK's raw-bitmap v3 font."""
    require(len(data) >= 10, "Truncated compiled font")
    version, height, count, wildcard, buckets, cp_bytes, header_size, features = struct.unpack_from(
        "<BBHHBBBB", data)
    require(version == 3 and cp_bytes in (2, 4) and header_size >= 10 and buckets > 0,
            "Unsupported compiled font header")
    require(features & ~1 == 0, "Unsupported compiled font encoding; review its reader")
    offset_bytes = 2 if features & 1 else 4
    table = header_size + buckets * 4
    require(table <= len(data), "Truncated compiled font bucket table")
    entries, total = {}, 0
    for bucket in range(buckets):
        _hash, number, position = struct.unpack_from("<BBH", data, header_size + bucket * 4)
        total += number
        row = []
        for index in range(number):
            at = table + position + index * (cp_bytes + offset_bytes)
            require(at + cp_bytes + offset_bytes <= len(data), "Truncated font lookup table")
            point = int.from_bytes(data[at:at + cp_bytes], "little")
            glyph_offset = int.from_bytes(data[at + cp_bytes:at + cp_bytes + offset_bytes], "little")
            require(point not in entries or point == wildcard, "Duplicate font codepoint")
            row.append((point, glyph_offset))
            entries.setdefault(point, glyph_offset)
        require(row == sorted(row, key=lambda entry: entry[0]), "Unsorted font lookup bucket")
        # Match the firmware's binary search, including the reserved fallback.
        for point in {entry[0] for entry in row}:
            left, right = 0, len(row) - 1
            while left <= right:
                middle = (left + right) // 2
                candidate, glyph_offset = row[middle]
                if candidate < point:
                    left = middle + 1
                elif candidate > point:
                    right = middle - 1
                else:
                    entries[point] = glyph_offset
                    break
    require(total == count, "Compiled glyph count differs from its lookup table")
    glyph_base = table + total * (cp_bytes + offset_bytes)
    metrics = {}
    for point, offset in entries.items():
        at = glyph_base + offset
        require(at + 5 <= len(data), "Truncated compiled glyph metrics")
        width, glyph_height, left, top, advance = struct.unpack_from("<BBbbb", data, at)
        size = (width * glyph_height + 7) // 8
        require(at + 5 + size <= len(data), "Truncated compiled glyph bitmap")
        bitmap = data[at + 5:at + 5 + size]
        metrics[point] = {"width": width, "height": glyph_height, "left": left,
                          "top": top, "advance": advance,
                          "hasInk": any(bitmap)}
    return {"height": height, "wildcard": wildcard, "lookupEntries": count,
            "metrics": metrics}


def source_matches(mapped, source, label):
    """Permit Webpack's trailing comments, never a transformed source body."""
    original = source.decode("utf-8")
    require(mapped.startswith(original), "Source map differs from current source: " + label)
    tail = mapped[len(original):]
    require(not tail or re.fullmatch(
        r"\n{2,}//////////////////\n// WEBPACK FOOTER(?:\n//[^\n]*)*\s*", tail),
        "Unexpected source-map transform: " + label)


def audit_sdk_additions(root, source_map):
    record = json.loads((root / "licenses/sdk-bundled-provenance.json").read_text())
    require(record["sdkVersion"] == "4.33.1" and
            record["sourceMapSource"] == "_pkjs_shared_additions.js",
            "Review SDK-injected JavaScript provenance for this SDK")
    matches = [content for name, content in zip(source_map["sources"], source_map["sourcesContent"])
               if PurePosixPath(name).name == record["sourceMapSource"]]
    require(len(matches) == 1, "SDK-injected JavaScript source is missing or duplicated")
    content = matches[0]
    encoded = content.encode("utf-8")
    original = encoded[:record["sourceBytes"]]
    require(len(original) == record["sourceBytes"] and sha256(original) == record["sourceSha256"],
            "SDK-injected JavaScript differs from the reviewed source")
    source_matches(content, original, record["sourceMapSource"])
    require(record["copyright"] == "Copyright 2024 Google LLC" and
            record["copyright"].encode() in original and
            b"Licensed under the Apache License, Version 2.0" in original,
            "SDK-injected JavaScript copyright/license notice is missing")
    return {"sdkVersion": record["sdkVersion"], "source": record["sourceMapSource"],
            "sourceSha256": record["sourceSha256"], "reviewedSourceMatches": True,
            "copyrightNoticePreserved": True, "licenseFile": record["licenseFile"],
            "licenseSha256": record["licenseSha256"]}


def audit_localized_weekdays(sources, binary):
    """Check the compact table, aliases and native bytes before glyph coverage.

    This is a deliberately finite source audit, not a general C interpreter.
    Lookup changes require review so a missing alias or fallback cannot pass
    merely because its letters happen to be present in a font.
    """
    declarations = re.findall(
        r"static\s+const\s+struct\s*\{\s*char\s+language\[3\];\s*"
        r"char\s+days\[7\]\[5\];\s*\}\s*s_weekdays\[\]\s*=\s*\{(.*?)\};",
        sources, re.S)
    require(len(declarations) == 1, "Expected the reviewed compact weekday table")
    row_pattern = r'\{\s*"([a-z]{2})"\s*,\s*\{([^{}]*)\}\s*\}'
    rows = re.findall(row_pattern, declarations[0])
    require(not re.sub(row_pattern, "", declarations[0]).strip(" \t\r\n,"),
            "Unexpected compact weekday table contents")
    languages = [language for language, _body in rows]
    require(languages == list(REVIEWED_WEEKDAYS),
            "Compact weekday languages differ from the reviewed 16 stored rows")
    weekdays = {}
    encoded = bytearray()
    for language, body in rows:
        words = re.findall(r'"([^"\\]*)"', body)
        require(not re.sub(r'"[^"\\]*"', "", body).strip(" \t\r\n,"),
                "Unsupported compact weekday string syntax")
        require(len(words) == 7 and all(len(day) == 3 and
                0 < len(day.encode("utf-8")) < 5 for day in words),
                "Compact weekday width/count differs from its UTF-8 storage")
        require(words == REVIEWED_WEEKDAYS[language].split(),
                "Localized weekday text differs from reviewed abbreviations: " + language)
        weekdays[language] = words
        encoded.extend(language.encode("ascii") + b"\0")
        for day in words:
            encoded.extend(day.encode("utf-8").ljust(5, b"\0"))
    lookup = re.findall(r"static const char \*prv_get_localized_day\(.*?^\}",
                        sources, re.S | re.M)
    compact = lambda value: re.sub(r"\s+", "", value)
    require(len(lookup) == 1 and compact(lookup[0]) == compact(REVIEWED_WEEKDAY_LOOKUP),
            "Review the changed weekday locale/alias/fallback lookup")
    require(bytes(encoded) in binary, "Native binary lacks the reviewed compact weekday table")
    weekdays["no"] = list(weekdays["da"])
    return weekdays, {"storedWeekdayRows": len(rows), "nativeWeekdayTableBytes": len(encoded),
                      "nativeWeekdayTableMatches": True, "localeLookupReviewed": True,
                      "localeAliases": {"no": "da", "nb": "da", "nn": "da"}}


def audit_body_fonts(root, media, resources, sources, binary):
    names = [item["name"] for item in media]
    sizes = BODY_SIZES
    require(names == ["FONT_BODY_" + str(size) for size in sizes],
            "Expected the reviewed eight body font sizes in order")
    definitions = re.findall(
        r"#define\s+RESOURCE_ID_(\w+)\s+(\d+)",
        (root / "build/emery/src/resource_ids.auto.h").read_text())
    ids = {name: int(number) for name, number in definitions}
    require(len(definitions) == len(ids) and ids == {
        name: index + 1 for index, name in enumerate(names)},
        "Generated resource IDs are stale or contain unexpected entries")
    weekdays, weekday_identity = audit_localized_weekdays(sources, binary)
    words = [day for days in weekdays.values() for day in days]
    require(len(words) == 17 * 7, "Unexpected localized weekday count")
    required = {ord(char) for char in "".join(words) +
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 ·.°%:/+-…−"}
    corpus_points = {ord(char) for texts in BODY_TEXT_CORPORA.values()
                     for text in texts for char in text}
    required |= corpus_points
    provenance = json.loads((root / "resources/fonts/font-provenance.json").read_text())
    require(provenance.get("schemaVersion") == 1, "Review the changed font provenance schema")
    expected_definitions = provenance["bodyResources"]["definitions"]
    canonical = lambda item: {**item, "targetPlatforms": item.get("targetPlatforms", ["emery"])}
    require(list(map(canonical, media)) == list(map(canonical, expected_definitions)),
            "Manifest fonts differ from reviewed source definitions")
    source_record = provenance["bodyFont"]
    license_record = source_record["license"]
    require(safe_name(license_record["file"]) and
            sha256((root / license_record["file"]).read_bytes()) == license_record["sha256"],
            "Body font license differs from reviewed provenance")
    samples = {sample["point"]: sample for sample in
               provenance["bodyResources"]["standaloneSdkPfoSamples"]}
    require(set(samples) == set(sizes), "SDK bitmap font samples have unexpected sizes")
    records = []
    source_files = set()
    for item, size in zip(media, sizes):
        require(item.get("targetPlatforms", ["emery"]) == ["emery"],
                "A body font unexpectedly targets another platform")
        relative = item["file"]
        require(safe_name(relative) and relative.startswith("fonts/") and
                relative.endswith(".ttf"), "Invalid source body font path")
        raw = (root / "resources" / relative).read_bytes()
        require(source_record["file"] == "resources/" + relative and
                source_record["sha256"] == sha256(raw) and source_record["bytes"] == len(raw),
                "Body font source differs from reviewed provenance")
        source_files.add(relative)
        available = font_codepoints(raw)
        coverage = provenance["bodyCoverage"]
        require(coverage["codepointCount"] == len(available) and
                coverage["codepoints"] == sorted(available),
                "Body source cmap differs from the reviewed coverage inventory")
        pattern = re.compile(item["characterRegex"])
        expected = {point for point in available if pattern.fullmatch(chr(point))}
        require(expected == available and all(32 <= point <= 0xffff for point in available),
                "Body regex truncates the reviewed complete BMP source repertoire")
        require(required <= expected, "Body regex/source omits required localized glyphs: " +
                item["name"] + " " + " ".join("U+%04X" % point for point in sorted(required - expected)))
        entry = resources[ids[item["name"]]]
        font = read_pfo(entry["data"])
        require(samples[size]["sha256"] == sha256(entry["data"]) and
                samples[size]["bytes"] == len(entry["data"]),
                "Body bitmap font differs from the reviewed exact SDK fontgen output")
        require(font["height"] == size, "Compiled body font has the wrong pixel size")
        actual = set(font["metrics"])
        require(actual == expected | {font["wildcard"]},
                "Compiled glyph repertoire differs from selected source/regex: " + item["name"])
        require(font["wildcard"] == 0x25af and font["lookupEntries"] == len(actual) and
                samples[size]["lookupCodepoints"] == len(actual),
                "Compiled body font fallback/lookup inventory differs")
        fallback = font["metrics"][font["wildcard"]]
        require(fallback["advance"] > 0 and fallback["hasInk"],
                "Missing body glyphs would silently disappear: " + item["name"])
        # Cover every source letter in these restored blocks, including letters
        # outside the concrete title/name regression corpus. Combining marks may
        # legitimately have zero advance and are not included in this check.
        restored_letters = {point for point in available
                            if unicodedata.category(chr(point)).startswith("L") and
                            (0x0370 <= point <= 0x052f or 0x1c80 <= point <= 0x1c8f or
                             0x1e00 <= point <= 0x1fff or 0xa640 <= point <= 0xa69f)}
        ink_checks = required | restored_letters
        for point in ink_checks:
            metric = font["metrics"][point]
            require(metric["advance"] > 0 and (chr(point).isspace() or metric["hasInk"]),
                    "Required localized body glyph is empty: " + item["name"] + " U+%04X" % point)
        require("RESOURCE_ID_" + item["name"] in sources,
                "A packaged body font is not referenced by the renderer: " + item["name"])
        records.append({"name": item["name"], "resourceId": ids[item["name"]],
                        "pixelSize": size, "bytes": entry["bytes"],
                        "sha256": sha256(entry["data"]),
                        "sourceFile": relative, "sourceSha256": sha256(raw),
                        "selectedSourceCodepoints": len(expected),
                        "packagedCodepointsIncludingFallback": len(actual),
                        "requiredLocalizedGlyphChecks": len(required),
                        "restoredBlockLetterChecks": len(restored_letters),
                        "directGlyphInkChecks": len(ink_checks),
                        "fallback": {"codepoint": font["wildcard"], **fallback}})
    require(len(source_files) == 1, "Expected one reviewed Roboto Condensed source font")
    return {"fontCount": len(records), "fonts": records,
            "languages": len(weekdays), "localizedWeekdays": len(words),
            **weekday_identity,
            "requiredGlyphChecks": len(required) * len(records),
            "directGlyphInkChecks": sum(record["directGlyphInkChecks"] for record in records),
            "regressionCorpora": BODY_TEXT_CORPORA,
            "allFallbacksVisible": True,
            "scope": "Exact packaged lookup/bitmap coverage and source repertoire; not a visual layout or arbitrary-Unicode guarantee."}


def audit_body_metrics(root, resources, binary, sources):
    header = (root / "src/c/generated_body_metrics.h").read_text(encoding="utf-8")
    require('#include "generated_body_metrics.h"' in sources,
            "The renderer does not use the generated body baseline metrics")
    sizes = [int(size) for size in re.search(
        r"body_metric_points\[BODY_METRIC_POINT_COUNT\] = \{([\d,]+)\}", header).group(1).split(",")]
    require(sizes == list(BODY_SIZES), "Body metric sizes changed")
    points = [int(point, 16) for point in re.findall(
        r"0x([0-9a-f]+)", header.split("body_metric_codepoints[BODY_METRIC_COUNT] = {", 1)[1].split("};", 1)[0])]
    pools, maps = {}, {}
    for metric in ("descent", "ascent"):
        count_name = "BODY_METRIC_" + metric.upper() + "_ROW_COUNT"
        count = int(re.search(r"#define " + count_name + r" (\d+)\b", header).group(1))
        start = "body_metric_" + metric + "_rows[" + count_name + "][BODY_METRIC_POINT_COUNT] = {"
        pool = [list(map(int, row.split(","))) for row in re.findall(
            r"\{([\d,-]+)\}", header.split(start, 1)[1].split("};", 1)[0])]
        start = "body_metric_" + metric + "_index[BODY_METRIC_COUNT] = {"
        indices = list(map(int, re.findall(
            r"\d+", header.split(start, 1)[1].split("};", 1)[0])))
        require(0 < count <= 256 and len(pool) == count and
                len({tuple(row) for row in pool}) == count and
                all(len(row) == len(sizes) and all(-128 <= value <= 127 for value in row)
                    for row in pool) and len(indices) == len(points) and
                all(0 <= index < count for index in indices),
                "Compressed body metric row/map shape changed: " + metric)
        lookup = "body_metric_" + metric + "_rows[body_metric_" + metric + "_index[body_metric_index(codepoint)]][column]"
        require(lookup in header, "Body metric helper bypasses its exact row map: " + metric)
        pools[metric], maps[metric] = pool, indices
    rows = [pools["descent"][index] for index in maps["descent"]]
    ascents = [pools["ascent"][index] for index in maps["ascent"]]
    provenance = json.loads((root / "resources/fonts/font-provenance.json").read_text())
    record = provenance["generatedBodyMetrics"]
    require(len(points) == len(rows) == len(ascents) == record["metricCodepointCountIncludingWildcard"] and
            record["fontCodepointCount"] == len(points) - 1 and points == sorted(set(points)) and
            all(len(row) == len(sizes) for row in rows + ascents), "Body metric lookup shape changed")
    fallback = re.search(r"#define BODY_METRIC_WILDCARD_INDEX (\d+)\b", header)
    require(fallback and int(fallback.group(1)) < len(points) and
            points[int(fallback.group(1))] == 0x25af,
            "Body metric fallback index does not select the SDK wildcard")
    index_function = header.split("static inline size_t body_metric_index(", 1)[1].split("\n}", 1)[0]
    require(index_function.rstrip().endswith("return BODY_METRIC_WILDCARD_INDEX;"),
            "Missing body glyphs do not use the explicit wildcard metrics")
    checks = 0
    for column, size in enumerate(sizes):
        font = read_pfo(resources[column + 1]["data"])
        require(set(points) == set(font["metrics"]), "Body metric/PFO repertoires differ")
        for point, row, ascent_row in zip(points, rows, ascents):
            glyph = font["metrics"][point]
            # The SDK stores a glyph's bitmap top relative to the font-size baseline.
            descent = glyph["top"] + glyph["height"] - font["height"]
            require(row[column] == descent,
                    "Body baseline differs from exact packaged glyph: %dpx U+%04X" % (size, point))
            # fontgen normalizes an empty glyph into a blank 1x1 bitmap at
            # top=size-1. It has no visible ascent despite those dummy bounds.
            ascent = font["height"] - glyph["top"] if glyph["hasInk"] else 0
            require(ascent_row[column] == ascent,
                    "Body ascent differs from exact packaged glyph: %dpx U+%04X" % (size, point))
            checks += 1
    raw_points = struct.pack("<" + "H" * len(points), *points)
    linked_tables = [raw_points]
    for metric in ("descent", "ascent"):
        linked_tables.extend((bytes(maps[metric]),
                              bytes(value & 0xff for row in pools[metric] for value in row)))
    require(all(binary.count(table) == 1 for table in linked_tables),
            "Native binary does not contain the exact body codepoint/metric pool/map tables")
    require(record["header"] == "src/c/generated_body_metrics.h" and
            record["sha256"] == sha256(header.encode()) and record["pointSizes"] == sizes,
            "Body metric header differs from reviewed provenance")
    require(record["fontSha256"] == provenance["bodyFont"]["sha256"],
            "Body metric source font provenance differs")
    embedded_sha = re.search(r'#define BODY_FONT_SOURCE_SHA256 "([0-9a-f]{64})"', header)
    require(embedded_sha and embedded_sha.group(1) == record["fontSha256"],
            "Body metric header has stale source-font provenance")
    return {"header": record["header"], "headerSha256": record["sha256"],
            "codepoints": len(points), "sizes": sizes, "packagedGlyphDescentChecks": checks,
            "packagedGlyphAscentChecks": checks,
            "uniqueDescentRows": len(pools["descent"]),
            "uniqueAscentRows": len(pools["ascent"]),
            "compressedMetricBytes": sum(len(table) for table in linked_tables[1:]),
            "wildcardIndex": int(fallback.group(1)), "wildcardCodepoint": 0x25af,
            "nativeMetricTablesMatch": True,
            "scope": "Every committed ascent/descent matches the exact packaged PFO bitmap metrics, including fallback and empty glyph normalization; visible text alignment remains a native rendering check."}


def audit_clock_outlines(root, binary, sources):
    header_path = root / "src/c/generated_clock_glyphs.h"
    header = header_path.read_text(encoding="utf-8")
    require('#include "generated_clock_glyphs.h"' in sources,
            "The renderer does not include the generated clock geometry")
    checked = subprocess.run(
        [sys.executable, str(root / "scripts/generate-clock-glyphs.py"), "--check", "--json"],
        cwd=root, capture_output=True, text=True, check=False, timeout=60)
    require(checked.returncode == 0, "Clock outline regeneration failed: " +
            (checked.stderr or checked.stdout).strip())
    info = json.loads(checked.stdout)
    require(info.get("result") == "PASS" and info["characters"] == CLOCK_CHARACTERS and
            info["glyphCount"] == len(CLOCK_CHARACTERS), "Unexpected generated clock repertoire")
    require(info["maximumContourPoints"] <= 256 and
            info["maximumCurveFlatnessPxAt80"] <= 0.2,
            "Clock outline polygon bounds or flatness changed")
    raw_font = (root / "resources/fonts/Fredoka-Bold-Clock.ttf").read_bytes()
    require(info["fontSha256"] == sha256(raw_font), "Clock TTF provenance differs")
    provenance = json.loads((root / "resources/fonts/font-provenance.json").read_text())
    require(provenance["clockFont"]["sha256"] == info["fontSha256"] and
            provenance["clockFont"]["bytes"] == len(raw_font),
            "Clock source differs from reviewed font provenance")
    require(provenance["generatedClock"]["sha256"] == sha256(header.encode()),
            "Generated clock header differs from reviewed provenance")
    license_record = provenance["clockFont"]["license"]
    require(safe_name(license_record["file"]) and
            sha256((root / license_record["file"]).read_bytes()) == license_record["sha256"],
            "Clock font license differs from reviewed provenance")
    embedded_sha = re.search(r'#define CLOCK_FONT_SOURCE_SHA256 "([0-9a-f]{64})"', header)
    require(embedded_sha and embedded_sha.group(1) == info["fontSha256"],
            "Generated header has stale clock source provenance")
    points_text = header.split("clock_points[CLOCK_POINT_COUNT] = {", 1)[1].split("};", 1)[0]
    points = [tuple(map(int, pair)) for pair in re.findall(r"\{(-?\d+),(-?\d+)\}", points_text)]
    require(len(points) == info["pointCount"], "Clock point count differs from generated metadata")
    packed_points = b"".join(struct.pack("<hh", *point) for point in points)
    require(binary.count(packed_points) == 1,
            "Native binary does not contain the exact generated clock point table")
    contour_text = header.split("clock_contours[CLOCK_CONTOUR_COUNT] = {", 1)[1].split("};", 1)[0]
    contours = [(int(first), int(count), hole == "true") for first, count, hole in
                re.findall(r"\{(\d+),(\d+),(true|false)\}", contour_text)]
    require(len(contours) == info["contourCount"] and
            sum(hole for _first, _count, hole in contours) == info["holeContours"],
            "Clock contour metadata differs from generated geometry")
    require(all(count >= 3 and first + count <= len(points) for first, count, _hole in contours),
            "Clock contours extend beyond the generated point table")
    # ClockContour's Arm ABI layout is two uint16_t fields, one bool and one pad byte.
    packed_contours = b"".join(struct.pack("<HH?x", *contour) for contour in contours)
    require(binary.count(packed_contours) == 1,
            "Native binary does not contain the exact generated contour/holes table")
    glyph_text = header.split("clock_glyphs[CLOCK_GLYPH_COUNT] = {", 1)[1].split("};", 1)[0]
    glyphs = [(literal if literal else chr(int(codepoint, 16)), *map(int, values.split(",")))
              for literal, codepoint, values in
              re.findall(r"\{(?:'(.)'|0x([0-9a-f]+)),([\d,-]+)\}", glyph_text)]
    require("".join(row[0] for row in glyphs) == info["characters"],
            "Generated glyph order differs")
    require(re.search(r"typedef struct\s*\{\s*uint32_t code;", header),
            "Clock glyph codepoint is not the reviewed 32-bit Unicode field")
    # Arm EABI: uint32 code, three uint16 metrics, four int16 bounds,
    # followed by two bytes of padding to the structure's four-byte alignment.
    packed_glyphs = b"".join(struct.pack("<IHHHhhhh2x", ord(row[0]), *row[1:]) for row in glyphs)
    require(binary.count(packed_glyphs) == 1,
            "Native binary does not contain the exact generated glyph metrics table")
    holes = {code: sum(hole for _at, _count, hole in contours[first:first + count])
             for code, _advance, first, count, *_bounds in glyphs}
    require({code: holes[code] for code in "0689"} == {"0": 1, "6": 1, "8": 2, "9": 1},
            "Clock counter/hole structure changed; review native appearance")
    require(holes["°"] == 1, "Clock temperature degree symbol lost its visible counter")
    license_text = (root / "resources/fonts/OFL-Fredoka.txt").read_text(encoding="utf-8")
    require("SIL OPEN FONT LICENSE Version 1.1" in license_text and
            "Fredoka Project Authors" in license_text,
            "The clock source/derived geometry license is missing")
    return {**info, "sourceFont": "resources/fonts/Fredoka-Bold-Clock.ttf",
            "header": "src/c/generated_clock_glyphs.h", "headerSha256": sha256(header.encode()),
            "regenerationExact": True, "nativePointContourGlyphTablesMatch": True,
            "selectedDigitHoleCounts": {code: holes[code] for code in "0689"},
            "degreeSymbolHoleCount": holes["°"],
            "scope": "Source geometry, polygon bounds and exact linked tables; native screenshots verify rasterized appearance."}


def read_dependency(root, specification, active_modules):
    pin, files = verify_dependency(root, specification)
    label = pin["package"]
    installed = root / "node_modules" / label
    for name, content in files.items():
        require((installed / name).read_bytes() == content,
                "Installed dependency differs from pinned archive: " + label + "/" + name)
    with zipfile.ZipFile(io.BytesIO(files["dist.zip"])) as sdk:
        require(sdk.testzip() is None and
                len(sdk.namelist()) == len(set(sdk.namelist())) and
                all(safe_name(name) for name in sdk.namelist()),
                "Pinned SDK library archive is corrupt, duplicated or unsafe: " + label)
        sdk_files = {name: sdk.read(name) for name in sdk.namelist()}
    for name, content in sdk_files.items():
        require((installed / "dist" / name).read_bytes() == content,
                "SDK staging differs from pinned archive: " + label + "/" + name)
        if name.startswith("js/"):
            require(files["src/" + name] == content,
                    "Source and SDK JavaScript differ: " + label + "/" + name)
    for name, content in files.items():
        if name.startswith("include/") and name.endswith(".h"):
            exported = "include/" + label + "/" + name.removeprefix("include/")
            require(sdk_files[exported] == content,
                    "Source and exported SDK header differ: " + label + "/" + name)
    require({name for name in sdk_files if name.startswith("binaries/")} ==
            {"binaries/emery/lib" + label + ".a"},
            "Unexpected SDK binary inventory: " + label)
    sources = {}
    for name in active_modules:
        content = files["src/js/" + name]
        require(sdk_files["js/" + name] == content,
                "Active source and packaged JavaScript differ: " + label + "/" + name)
        sources[name] = content
    native_sources = {name: sha256(content) for name, content in sorted(files.items())
                      if name.startswith(("include/", "src/c/")) and
                      name.endswith((".c", ".h"))}
    return {"version": pin["version"], "sourceCommit": pin["source_commit"],
            "archiveSha256": pin["sha256"], "sdkArchiveSha256": sha256(files["dist.zip"]),
            "nativeSourceSha256": native_sources,
            "installedAndSdkStagingMatch": True}, sources


def audit_preview_identity(root, version, binary, resources):
    source = (root / "src/pkjs/settings-preview.js").read_text(encoding="utf-8")
    exported = re.search(r"module\.exports\s*=\s*(\{.*\})\s*;\s*$", source, re.S)
    require(exported, "Native preview asset export is missing")
    preview = json.loads(exported.group(1))
    require(preview["version"] == version and len(preview["frames"]) == 20,
            "Native preview assets belong to a different release")
    for name, content in (("emery/pebble-app.bin", binary),
                          ("emery/app_resources.pbpack", resources)):
        require(preview["source"]["nativePayloadSha256"][name] == sha256(content),
                "Native previews belong to a different renderer: " + name)
    return {"samples": 20, "nativeBinaryAndResourcesMatch": True}


def audit(pbw, root=ROOT, sdk_font_header=None):
    package = json.loads((root / "package.json").read_text(encoding="utf-8"))
    settings = package["pebble"]
    require(package["version"] == "0.12.1", "Review this auditor for a different release version")
    require(settings["uuid"] == APP_UUID and settings["displayName"] == "Dynamic",
            "Dynamic's identity changed")
    require(settings["targetPlatforms"] == ["emery"] and
            settings["usesPermissions"] == ["ReadMusic"],
            "Unexpected platform or permission change")
    media = settings["resources"].get("media", [])
    require(media and all(item.get("type") == "font" for item in media),
            "Only the declared custom body fonts are expected")
    require(tuple(settings["messageKeys"]) == LEGACY_KEYS + ("ANIMATIONS", "CLOCK_FORMAT", "NOTIFICATION_VIBRATION", "NIGHT_PALETTE", "NIGHT_BACKGROUND_COLOR", "NIGHT_TEXT_COLOR", "NIGHT_ACCENT_COLOR", "NOTIFICATION_APP_ONLY", "CONTEXT_STATE", "ACTIVITY_DATA", "REQUEST_ACTIVITIES", "SHOW_ACTIVITIES", "ACTIVITY_PRESENTED", "REQUEST_PRESENTATION"),
            "Preserve existing keys and append the two navigation status keys")
    keys = {name: 10000 + index for index, name in enumerate(settings["messageKeys"])}
    generated = json.loads((root / "build/js/message_keys.json").read_text())
    require(generated == keys, "Generated JavaScript keys are stale")
    native_entries = re.findall(
        r"MESSAGE_KEY_(\w+)\s*=\s*(\d+)\s*;",
        (root / "build/src/message_keys.auto.c").read_text())
    native_keys = {name: int(value) for name, value in native_entries}
    require(len(native_entries) == len(native_keys) and native_keys == keys,
            "Generated C keys are stale or duplicated")
    core, core_sources = read_dependency(root, DEPENDENCIES[0], CORE_MODULES)
    activities, activity_sources = read_dependency(root, DEPENDENCIES[1], ACTIVITY_MODULES)
    require(all(activities[key] == value for key, value in ACTIVITY_RELEASE.items()),
            "Reviewed ACT3 dependency provenance differs")
    licenses = license_payloads(root)
    sdk_pbw = root / "build" / (package["name"] + ".pbw")
    with zipfile.ZipFile(pbw) as release, zipfile.ZipFile(sdk_pbw) as sdk:
        require(release.testzip() is None and sdk.testzip() is None, "Corrupt PBW archive")
        require(len(release.namelist()) == len(set(release.namelist())) == len(PBW_FILES) and
                set(release.namelist()) == PBW_FILES and
                len(sdk.namelist()) == len(set(sdk.namelist())) == len(SDK_PBW_FILES) and
                set(sdk.namelist()) == SDK_PBW_FILES,
                "Unexpected PBW entry inventory")
        info = json.loads(release.read("appinfo.json"))
        original = json.loads(sdk.read("appinfo.json"))
        expected = dict(original)
        expected["manifestVersion"] = 2
        expected["usesPermissions"] = settings["usesPermissions"]
        expected["plugin"] = settings["plugin"]
        require(info == expected, "Permission packaging changed unexpected app metadata")
        require(info["uuid"] == APP_UUID and info["versionLabel"] == package["version"] and
                info["targetPlatforms"] == ["emery"] and
                info["watchapp"] == {"watchface": True} and
                info["usesPermissions"] == ["ReadMusic"], "Packaged metadata mismatch")
        require(info["appKeys"] == info["messageKeys"] == keys,
                "Packaged AppMessage key mapping differs")
        packed_media = info.get("resources", {}).get("media", [])
        require(len(packed_media) == len(media), "Packaged resource declarations differ")
        for source, packed in zip(media, packed_media):
            expected_resource = dict(source)
            expected_resource.setdefault("targetPlatforms", ["emery"])
            actual_resource = dict(packed)
            actual_resource.setdefault("targetPlatforms", ["emery"])
            require(actual_resource == expected_resource, "Packaged body-font declaration differs")
        inventory = []
        for name in sorted(PBW_FILES):
            content = release.read(name)
            if name in licenses:
                require(content == licenses[name], "Packaged license differs from source: " + name)
                inventory.append({"file": name, "bytes": len(content),
                                  "sha256": sha256(content),
                                  "sourceFile": LICENSE_FILES[name], "sourceByteIdentical": True})
                continue
            if name == "plugin.js":
                require(content == (root/"build/plugin.js").read_bytes(), "Packaged navigation plugin differs")
                result = subprocess.run([sys.executable, str(root/"scripts/bundle-js.py"), str(root/"src/plugin/index.js"), "/dev/stdout"], capture_output=True, check=True, cwd=root)
                require(content == result.stdout, "Navigation plugin differs from current source")
                inventory.append({"file":name,"bytes":len(content),"sha256":sha256(content),"sourceByteIdentical":True})
                continue
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
        native_sizes = audit_native_sizes(binary, (root / "build/emery/pebble-app.elf").read_bytes())
        resource_entries = read_resource_pack(resources, len(media))
        resource_count = len(resource_entries)
        source_map = json.loads(release.read("pebble-js-app.js.map"))
        active_js = release.read("pebble-js-app.js").decode("utf-8")
    require(len(source_map["sources"]) == len(source_map["sourcesContent"]),
            "Invalid source map")
    sdk_additions = audit_sdk_additions(root, source_map)
    preview_identity = audit_preview_identity(root, package["version"], binary, resources)
    require(len(source_map["sources"]) == len(set(source_map["sources"])),
            "Source map has duplicate modules")
    hosts, modules, activity_modules, xhr, bundled_keys = set(), set(), set(), [], None
    for name, content in zip(source_map["sources"], source_map["sourcesContent"]):
        if name.startswith("./src/pkjs/") or name.startswith("./src/common/"):
            relative = name.removeprefix("./")
            source_matches(content, (root / relative).read_bytes(), name)
            hosts.add(Path(relative).name)
        elif name.startswith("./~/pebble-watchface-core/dist/js/"):
            leaf = name.removeprefix("./~/pebble-watchface-core/dist/js/")
            require(leaf in core_sources, "Unexpected active core module: " + leaf)
            source_matches(content, core_sources[leaf], name)
            modules.add(leaf)
        elif name.startswith("./~/pebble-activities/dist/js/"):
            relative = name.removeprefix("./~/pebble-activities/dist/js/")
            require(relative in activity_sources, "Unexpected active activity module: " + relative)
            source_matches(content, activity_sources[relative], name)
            activity_modules.add(relative)
        elif "pebble-watchface-core/" in name or "pebble-activities/" in name:
            require(False, "Unexpected dependency module path: " + name)
        elif name == "./build/js/message_keys.json":
            exported = re.match(r"^module\.exports\s*=\s*(\{[^\n]+\})", content)
            require(exported and json.loads(exported.group(1)) == keys,
                    "Bundled JavaScript key mapping is stale")
            bundled_keys = json.loads(exported.group(1))
        if "XMLHttpRequest" in content:
            xhr.append(name)
    require(hosts == APP_MODULES and modules == CORE_MODULES and
            activity_modules == ACTIVITY_MODULES,
            "The active application/core/activity dependency graph changed")
    require(bundled_keys == keys, "Bundled JavaScript key mapping is missing")
    require(xhr == ["./~/@rebble/clay/dist/js/index.js"],
            "Unexpected active HTTP client")
    require(not any(endpoint in active_js for endpoint in LEGACY_ENDPOINTS),
            "A legacy weather HTTP client is active")
    sources = "\n".join(path.read_text(encoding="utf-8") for path in
                        sorted((root / "src/c").glob("*")) if path.suffix in (".c", ".h"))
    require("fonts_load_custom_font" in sources and "fonts_unload_custom_font" in sources,
            "Body font resource loading or lifetime cleanup is missing")
    require(not any(value in sources for value in ("FIRA_SANS", "FONT_KEY_LECO")),
            "The old Fira/LECO presentation is still referenced")
    body_fonts = audit_body_fonts(root, media, resource_entries, sources, binary)
    body_metrics = audit_body_metrics(root, resource_entries, binary, sources)
    clock_outlines = audit_clock_outlines(root, binary, sources)
    body_license = (root / "resources/fonts/OFL-RobotoCondensed.txt").read_text(encoding="utf-8")
    require("SIL OPEN FONT LICENSE Version 1.1" in body_license and
            "Roboto Project Authors" in body_license,
            "The body font license is missing")
    font_keys = sorted(set(re.findall(r"\bFONT_KEY_([A-Z0-9_]+)\b", sources)))
    linked_keys = [name for name in font_keys if ("RESOURCE_ID_" + name).encode() in binary]
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
        "nativePayloadMatchesSdk": True, "nativeSizes": native_sizes,
        "activityWireVersion": 3, "customResourceCount": resource_count,
        "licenseFileCount": len(licenses), "licenseSourceIdentityVerified": True,
        "nativePreviews": preview_identity,
        "bodyFonts": body_fonts, "bodyMetrics": body_metrics, "clockOutlines": clock_outlines,
        "javaScriptPayloadMatchesBuild": True,
        "sourceSystemFontKeys": font_keys, "nativeSystemFontKeys": linked_keys,
        "sourceFontKeysNotLinked": sorted(set(font_keys) - set(linked_keys)),
        "hostModules": len(hosts),
        "coreModules": len(modules), "activityModules": len(activity_modules),
        "sourceMapMatchesCurrentSource": True,
        "sourceMapModuleCount": len(source_map["sources"]),
        "legacyWeatherClientActive": False, "core": core, "activities": activities,
        "sdkInjectedJavaScript": sdk_additions,
        "pbwSha256": sha256(pbw.read_bytes()), "inventory": inventory,
        "scope": "SDK/package/source identity, generated/linked clock outlines and packaged body glyph coverage. Native screenshots and captured transitions verify appearance, fitting and motion; no physical-device behavior is claimed.",
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
            IndexError, re.error, subprocess.SubprocessError, tarfile.TarError, zipfile.BadZipFile) as error:
        parser.exit(1, "Dynamic native-style audit failed: " + str(error) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
