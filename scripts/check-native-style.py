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
import base64
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
RESOURCE_TABLE_BYTES = 256 * 16
BODY_SIZES = (11, 14, 16, 20, 24, 26, 28, 32)
CLOCK_CHARACTERS = "0123456789:.-+,/%°CF"
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


def audit_body_fonts(root, media, resources, sources):
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
    arrays = re.findall(
        r"static const char \*const s_(\w+)_days\[\]\s*=\s*\{(.*?)\};",
        sources, re.S)
    require(len(arrays) == 17, "Expected the reviewed 17 localized weekday arrays")
    words = [day for _language, body in arrays for day in re.findall(r'"([^"\\]+)"', body)]
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
            "languages": len(arrays), "localizedWeekdays": len(words),
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
    rows = [list(map(int, row.split(","))) for row in re.findall(
        r"\{([\d,-]+)\}", header.split("body_metric_descents[BODY_METRIC_COUNT][BODY_METRIC_POINT_COUNT] = {", 1)[1].split("};", 1)[0])]
    ascents = [list(map(int, row.split(","))) for row in re.findall(
        r"\{([\d,-]+)\}", header.split("body_metric_ascents[BODY_METRIC_COUNT][BODY_METRIC_POINT_COUNT] = {", 1)[1].split("};", 1)[0])]
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
    raw_rows = bytes(value & 0xff for row in rows for value in row)
    raw_ascents = bytes(value & 0xff for row in ascents for value in row)
    require(binary.count(raw_points) == 1 and binary.count(raw_rows) == 1 and
            binary.count(raw_ascents) == 1,
            "Native binary does not contain the exact body codepoint/descent/ascent tables")
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
    require(package["version"] == "0.3.2", "Review this auditor for a different release version")
    require(settings["uuid"] == APP_UUID and settings["displayName"] == "Dynamic",
            "Dynamic's identity changed")
    require(settings["targetPlatforms"] == ["emery"] and
            settings["usesPermissions"] == ["ReadMusic"],
            "Unexpected platform or permission change")
    media = settings["resources"].get("media", [])
    require(media and all(item.get("type") == "font" for item in media),
            "Only the declared custom body fonts are expected")
    require(tuple(settings["messageKeys"]) == LEGACY_KEYS + ("ANIMATIONS",),
            "Preserve the first 32 keys and append only ANIMATIONS")
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
        resource_entries = read_resource_pack(resources, len(media))
        resource_count = len(resource_entries)
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
    require("fonts_load_custom_font" in sources and "fonts_unload_custom_font" in sources,
            "Body font resource loading or lifetime cleanup is missing")
    require(not any(value in sources for value in ("FIRA_SANS", "FONT_KEY_LECO")),
            "The old Fira/LECO presentation is still referenced")
    body_fonts = audit_body_fonts(root, media, resource_entries, sources)
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
        "nativePayloadMatchesSdk": True, "customResourceCount": resource_count,
        "bodyFonts": body_fonts, "bodyMetrics": body_metrics, "clockOutlines": clock_outlines,
        "javaScriptPayloadMatchesBuild": True,
        "sourceSystemFontKeys": font_keys, "nativeSystemFontKeys": linked_keys,
        "sourceFontKeysNotLinked": sorted(set(font_keys) - set(linked_keys)),
        "hostModules": len(hosts),
        "coreModules": len(modules), "sourceMapMatchesCurrentSource": True,
        "sourceMapModuleCount": len(source_map["sources"]),
        "legacyWeatherClientActive": False, "core": core,
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
