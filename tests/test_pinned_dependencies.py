"""Provenance regressions for both vendored libraries; no SDK required."""
from pathlib import Path
import base64
import hashlib
import importlib.util
import io
import json
import re
import struct
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import zipfile


SCRIPTS = Path(__file__).resolve().parents[1] / "scripts"
sys.path.insert(0, str(SCRIPTS))
from pinned_dependencies import DEPENDENCIES, verify_dependency

spec = importlib.util.spec_from_file_location("dynamic_native_audit", SCRIPTS / "check-native-style.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class PinnedDependenciesTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="dynamic-pins-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        (self.root / "vendor").mkdir()
        self.package = {"dependencies": {}}
        self.lock = {"packages": {"": {"dependencies": {}}}}

    def write_json(self, name, value):
        (self.root / name).write_text(json.dumps(value), encoding="utf-8")

    def fixture(self, specification, *, unsafe=False, package_keys=False, stale_sdk=False,
                stale_header=False):
        metadata_file, name, version = specification
        source = b"module.exports = {};\n"
        info = {"name": name, "version": version,
                "pebble": {"messageKeys": ["UNEXPECTED"] if package_keys else []}}
        header = b"int fixture(void);\n"
        sdk = {"js/index.js": b"stale" if stale_sdk else source,
               "include/" + name + "/fixture.h": b"stale header" if stale_header else header,
               "binaries/emery/lib" + name + ".a": b"fixture library"}
        buffer = io.BytesIO()
        with zipfile.ZipFile(buffer, "w") as archive:
            for path, data in sdk.items():
                archive.writestr(path, data)
        files = {"package.json": json.dumps(info).encode(), "LICENSE": b"fixture terms",
                 "src/js/index.js": source, "include/fixture.h": header,
                 "dist.zip": buffer.getvalue()}
        if unsafe:
            files["../escape"] = b"unexpected"
        buffer = io.BytesIO()
        with tarfile.open(fileobj=buffer, mode="w:gz") as archive:
            for path, data in files.items():
                member = tarfile.TarInfo("package/" + path)
                member.size = len(data)
                archive.addfile(member, io.BytesIO(data))
        raw = buffer.getvalue()
        artifact = name + "-" + version + ".tgz"
        (self.root / "vendor" / artifact).write_bytes(raw)
        integrity = "sha512-" + base64.b64encode(hashlib.sha512(raw).digest()).decode()
        pin = {"package": name, "version": version, "artifact": artifact,
               "repository": "https://github.com/xefensor/" + name,
               "source_commit": "a" * 40, "sha256": hashlib.sha256(raw).hexdigest(),
               "integrity": integrity, "sdk": "4.33.1", "platforms": ["emery"]}
        self.write_json("vendor/" + metadata_file, pin)
        reference = "file:vendor/" + artifact
        self.package["dependencies"][name] = reference
        self.lock["packages"][""]["dependencies"][name] = reference
        self.lock["packages"]["node_modules/" + name] = {
            "version": version, "resolved": reference, "integrity": integrity}
        self.write_json("package.json", self.package)
        self.write_json("package-lock.json", self.lock)
        installed = self.root / "node_modules" / name
        for path, data in files.items():
            if ".." not in Path(path).parts:
                target = installed / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        for path, data in sdk.items():
            target = installed / "dist" / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        return pin

    def test_both_libraries_match_archive_lock_install_and_sdk_staging(self):
        for specification in DEPENDENCIES:
            pin = self.fixture(specification)
            self.assertEqual(verify_dependency(self.root, specification)[0], pin)
            summary, sources = audit.read_dependency(self.root, specification, {"index.js"})
            self.assertEqual(summary["sourceCommit"], "a" * 40)
            self.assertEqual(summary["nativeSourceSha256"], {
                "include/fixture.h": hashlib.sha256(b"int fixture(void);\n").hexdigest()})
            self.assertRegex(summary["sdkArchiveSha256"], r"^[0-9a-f]{64}$")
            self.assertEqual(sources, {"index.js": b"module.exports = {};\n"})

    def test_changed_archive_is_rejected(self):
        pin = self.fixture(DEPENDENCIES[0])
        path = self.root / "vendor" / pin["artifact"]
        path.write_bytes(path.read_bytes() + b"changed")
        with self.assertRaisesRegex(ValueError, "checksum"):
            verify_dependency(self.root, DEPENDENCIES[0])

    def test_changed_lock_resolution_is_rejected_for_activities(self):
        self.fixture(DEPENDENCIES[1])
        self.lock["packages"]["node_modules/pebble-activities"]["resolved"] = "https://example.invalid/package.tgz"
        self.write_json("package-lock.json", self.lock)
        with self.assertRaisesRegex(ValueError, "Lockfile"):
            verify_dependency(self.root, DEPENDENCIES[1])

    def test_placeholder_source_commit_cannot_pass_release_validation(self):
        pin = self.fixture(DEPENDENCIES[1])
        pin["source_commit"] = "0" * 40
        self.write_json("vendor/ACTIVITIES.json", pin)
        with self.assertRaisesRegex(ValueError, "provenance"):
            verify_dependency(self.root, DEPENDENCIES[1])

    def test_archive_paths_and_package_owned_keys_are_rejected(self):
        for options in ({"unsafe": True}, {"package_keys": True}):
            with self.subTest(options=options):
                self.fixture(DEPENDENCIES[0], **options)
                with self.assertRaises(ValueError):
                    verify_dependency(self.root, DEPENDENCIES[0])

    def test_installed_or_staged_changes_cannot_pass_native_audit(self):
        for relative in ("src/js/index.js", "dist/js/index.js"):
            with self.subTest(relative=relative):
                self.fixture(DEPENDENCIES[1])
                (self.root / "node_modules/pebble-activities" / relative).write_bytes(b"changed")
                with self.assertRaises(ValueError):
                    audit.read_dependency(self.root, DEPENDENCIES[1], {"index.js"})

    def test_consistently_pinned_but_stale_sdk_source_is_rejected(self):
        self.fixture(DEPENDENCIES[0], stale_sdk=True)
        with self.assertRaisesRegex(ValueError, "Source and SDK JavaScript"):
            audit.read_dependency(self.root, DEPENDENCIES[0], {"index.js"})

    def test_consistently_pinned_but_stale_act2_sdk_header_is_rejected(self):
        self.fixture(DEPENDENCIES[1], stale_header=True)
        with self.assertRaisesRegex(ValueError, "Source and exported SDK header"):
            audit.read_dependency(self.root, DEPENDENCIES[1], {"index.js"})

    def metric_fixture(self):
        header = '''#define BODY_METRIC_COUNT 4
#define BODY_METRIC_DESCENT_ROW_COUNT 2
#define BODY_METRIC_ASCENT_ROW_COUNT 2
#define BODY_METRIC_WILDCARD_INDEX 3
#define BODY_FONT_SOURCE_SHA256 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
static const uint8_t body_metric_points[BODY_METRIC_POINT_COUNT] = {11,14,16,20,24,26,28,32};
static const uint16_t body_metric_codepoints[BODY_METRIC_COUNT] = {0x0020,0x0021,0x0022,0x25af};
static const uint8_t body_metric_descent_index[BODY_METRIC_COUNT] = {0,1,0,1};
static const uint8_t body_metric_ascent_index[BODY_METRIC_COUNT] = {1,0,1,0};
static const int8_t body_metric_descent_rows[BODY_METRIC_DESCENT_ROW_COUNT][BODY_METRIC_POINT_COUNT] = {{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1}};
static const int8_t body_metric_ascent_rows[BODY_METRIC_ASCENT_ROW_COUNT][BODY_METRIC_POINT_COUNT] = {{3,3,3,3,3,3,3,3},{4,4,4,4,4,4,4,4}};
static inline size_t body_metric_index(uint32_t codepoint) {
  return BODY_METRIC_WILDCARD_INDEX;
}
descent = body_metric_descent_rows[body_metric_descent_index[body_metric_index(codepoint)]][column];
ascent = body_metric_ascent_rows[body_metric_ascent_index[body_metric_index(codepoint)]][column];
'''
        (self.root / "src/c").mkdir(parents=True, exist_ok=True)
        (self.root / "resources/fonts").mkdir(parents=True, exist_ok=True)
        (self.root / "src/c/generated_body_metrics.h").write_text(header)
        self.write_json("resources/fonts/font-provenance.json", {
            "bodyFont": {"sha256": "a" * 64}, "generatedBodyMetrics": {
                "header": "src/c/generated_body_metrics.h", "sha256": hashlib.sha256(header.encode()).hexdigest(),
                "fontSha256": "a" * 64, "pointSizes": list(audit.BODY_SIZES),
                "metricCodepointCountIncludingWildcard": 4, "fontCodepointCount": 3}})
        binary = b"\x7f".join((b"\x20\x00\x21\x00\x22\x00\xaf\x25", b"\x00\x01\x00\x01", b"\x00" * 8 + b"\x01" * 8,
                                  b"\x01\x00\x01\x00", b"\x03" * 8 + b"\x04" * 8))
        resources = {index + 1: {"data": bytes([size])} for index, size in enumerate(audit.BODY_SIZES)}

        def font(data):
            size = data[0]
            return {"height": size, "metrics": {
                32: {"top": size - 4, "height": 4, "hasInk": True},
                33: {"top": size - 3, "height": 4, "hasInk": True},
                34: {"top": size - 4, "height": 4, "hasInk": True},
                0x25af: {"top": size - 3, "height": 4, "hasInk": True}}}
        return header, binary, resources, font

    def test_compressed_metric_audit_expands_maps_against_independent_glyphs(self):
        _header, binary, resources, font = self.metric_fixture()
        with patch.object(audit, "read_pfo", font):
            result = audit.audit_body_metrics(self.root, resources, binary,
                                             '#include "generated_body_metrics.h"')
            self.assertEqual(result["packagedGlyphAscentChecks"], 32)
            self.assertEqual(result["compressedMetricBytes"], 40)
            with self.assertRaisesRegex(ValueError, "Native binary"):
                audit.audit_body_metrics(self.root, resources, binary[:-1] + b"changed",
                                         '#include "generated_body_metrics.h"')

    def test_compressed_metric_audit_rejects_invalid_row_index(self):
        header, binary, resources, font = self.metric_fixture()
        header = header.replace("descent_index[BODY_METRIC_COUNT] = {0,1,0,1}",
                                "descent_index[BODY_METRIC_COUNT] = {0,2,0,1}")
        (self.root / "src/c/generated_body_metrics.h").write_text(header)
        with patch.object(audit, "read_pfo", font):
            with self.assertRaisesRegex(ValueError, "row/map shape"):
                audit.audit_body_metrics(self.root, resources, binary,
                                         '#include "generated_body_metrics.h"')

    def sdk_fixture(self):
        source = '/** Copyright 2024 Google LLC\nLicensed under the Apache License, Version 2.0 */\n(function(){})();\n'
        (self.root / "licenses").mkdir(exist_ok=True)
        self.write_json("licenses/sdk-bundled-provenance.json", {
            "sdkVersion": "4.33.1", "sourceMapSource": "_pkjs_shared_additions.js",
            "sourceBytes": len(source.encode()), "sourceSha256": hashlib.sha256(source.encode()).hexdigest(),
            "copyright": "Copyright 2024 Google LLC", "licenseFile": "licenses/Apache-2.0.txt",
            "licenseSha256": "a" * 64})
        return source, {"sources": ["_pkjs_shared_additions.js"],
                        "sourcesContent": [source + "\n\n//////////////////\n// WEBPACK FOOTER\n// sdk source\n"]}

    def test_sdk_notice_audit_checks_exact_reviewed_body_and_permits_only_webpack_footer(self):
        _source, mapped = self.sdk_fixture()
        self.assertTrue(audit.audit_sdk_additions(self.root, mapped)["copyrightNoticePreserved"])
        # A workspace-local SDK contributes a relative path instead of a bare
        # source name. Exact reviewed bytes and duplicate detection still apply.
        mapped["sources"] = ["../sdk/pebble/common/include/_pkjs_shared_additions.js"]
        self.assertTrue(audit.audit_sdk_additions(self.root, mapped)["reviewedSourceMatches"])
        mapped["sources"].append("_pkjs_shared_additions.js")
        mapped["sourcesContent"].append(mapped["sourcesContent"][0])
        with self.assertRaisesRegex(ValueError, "missing or duplicated"):
            audit.audit_sdk_additions(self.root, mapped)
        mapped["sources"] = ["_pkjs_shared_additions.js"]
        mapped["sourcesContent"] = mapped["sourcesContent"][:1]
        mapped["sourcesContent"][0] += "unexpected();"
        with self.assertRaisesRegex(ValueError, "source-map transform"):
            audit.audit_sdk_additions(self.root, mapped)

    def test_sdk_notice_audit_rejects_modified_or_duplicate_source(self):
        source, mapped = self.sdk_fixture()
        mapped["sourcesContent"][0] = source.replace("Google", "Other!")
        with self.assertRaisesRegex(ValueError, "reviewed source"):
            audit.audit_sdk_additions(self.root, mapped)
        mapped = {"sources": ["_pkjs_shared_additions.js"] * 2, "sourcesContent": [source] * 2}
        with self.assertRaisesRegex(ValueError, "duplicated"):
            audit.audit_sdk_additions(self.root, mapped)


class LocalizedWeekdayAuditTests(unittest.TestCase):
    def fixture(self):
        source = (SCRIPTS.parent / "src/c/dynamic.c").read_text(encoding="utf-8")
        table = re.search(r"s_weekdays\[\]\s*=\s*\{(.*?)\n\};", source, re.S).group(1)
        rows = re.findall(r'\{"([a-z]{2})",\{([^}]+)\}\}', table)
        encoded = b"".join(language.encode() + b"\0" + b"".join(
            day.encode().ljust(5, b"\0") for day in re.findall(r'"([^"]+)"', body))
            for language, body in rows)
        return source, b"native-prefix" + encoded + b"native-suffix"

    def test_compact_table_preserves_all_17_languages_and_native_utf8_bytes(self):
        source, binary = self.fixture()
        weekdays, result = audit.audit_localized_weekdays(source, binary)
        self.assertEqual(len(weekdays), 17)
        self.assertEqual(sum(map(len, weekdays.values())), 119)
        self.assertEqual(weekdays["cs"][2:5], ["ÚTE", "STŘ", "ČTV"])
        self.assertEqual(weekdays["no"], ["SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"])
        self.assertEqual(result["localeAliases"], {"no": "da", "nb": "da", "nn": "da"})
        self.assertEqual(result["storedWeekdayRows"], 16)
        self.assertEqual(result["nativeWeekdayTableBytes"], 608)

    def test_missing_duplicate_or_changed_language_cannot_reduce_glyph_requirements(self):
        source, binary = self.fixture()
        for before, after in (('{"cs",', '{"en",'), ('"ÚTE"', '"UTE"'),
                              ('"STŘ"', '"STŘX"'), ('"SØN"', '"SØN","EXTRA"')):
            with self.subTest(before=before):
                with self.assertRaises(ValueError):
                    audit.audit_localized_weekdays(source.replace(before, after), binary)

    def test_alias_region_and_fallback_behavior_changes_require_review(self):
        source, binary = self.fixture()
        for before, after in (("b=='n'", "b=='q'"), ("row=0", "row=1"),
                              ("day>6", "day>5"), ("locale[2]=='-'", "locale[2]=='/'"),
                              ("days[day]", "days[0]")):
            with self.subTest(before=before):
                with self.assertRaisesRegex(ValueError, "locale/alias/fallback"):
                    audit.audit_localized_weekdays(source.replace(before, after), binary)

    def test_stale_native_weekday_table_is_rejected(self):
        source, binary = self.fixture()
        with self.assertRaisesRegex(ValueError, "Native binary"):
            audit.audit_localized_weekdays(source, binary.replace("ÚTE".encode(), b"UTE\0"))


class NativeSizeTests(unittest.TestCase):
    def fixture(self, load_size=65532, virtual_size=65535, relocations=1):
        """Construct independent ELF section extents and uint16 metadata fields."""
        binary = bytearray(load_size)
        binary[:8] = b"PBLAPP\x00\x00"
        binary[104:120] = audit.uuid.UUID(audit.APP_UUID).bytes
        struct.pack_into("<H", binary, 14, load_size & 0xffff)
        struct.pack_into("<I", binary, 100, relocations)
        struct.pack_into("<H", binary, 128, virtual_size & 0xffff)
        names = b"\x00.header\x00.text\x00.bss\x00.shstrtab\x00"
        payload_offset = 52
        names_offset = payload_offset + load_size
        directory_offset = names_offset + len(names)
        elf = bytearray(directory_offset + 5 * 40)
        struct.pack_into("<16sHHIIIIIHHHHHH", elf, 0,
                         b"\x7fELF\x01\x01\x01" + b"\x00" * 9,
                         2, 40, 1, 0, 0, directory_offset, 0, 52, 0, 0, 40, 5, 4)
        elf[payload_offset:names_offset] = binary
        elf[names_offset:directory_offset] = names
        sections = [
            (0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
            (1, 1, 2, 0, payload_offset, 130, 0, 0, 1, 0),
            (9, 1, 6, 130, payload_offset + 130, load_size - 130, 0, 0, 1, 0),
            (15, 8, 3, load_size, names_offset, virtual_size - load_size, 0, 0, 1, 0),
            (20, 3, 0, 0, names_offset, len(names), 0, 0, 1, 0),
        ]
        for index, section in enumerate(sections):
            struct.pack_into("<10I", elf, directory_offset + index * 40, *section)
        return bytes(binary) + b"\x00" * (relocations * 4), bytes(elf)

    def test_complete_binary_over_65535_is_valid_with_bounded_loaded_and_virtual_sizes(self):
        binary, elf = self.fixture()
        result = audit.audit_native_sizes(binary, elf)
        self.assertEqual(result["fileBytes"], 65536)
        self.assertEqual(result["loadedBytes"], 65532)
        self.assertEqual(result["virtualBytes"], 65535)
        self.assertEqual(result["relocationBytes"], 4)
        self.assertEqual(result["virtualHeadroomBytes"], 0)

    def test_exact_uint16_and_emery_file_limits_are_inclusive(self):
        binary, elf = self.fixture(load_size=65535, virtual_size=65535)
        self.assertEqual(audit.audit_native_sizes(binary, elf)["loadedHeadroomBytes"], 0)
        binary, elf = self.fixture(relocations=16385)
        self.assertEqual(audit.audit_native_sizes(binary, elf)["fileBytes"], 131072)

    def test_elf_extents_detect_load_and_virtual_overflow_despite_wrapped_header(self):
        cases = ((65536, 65536, "loaded image"), (65532, 65536, "virtual size"))
        for load_size, virtual_size, message in cases:
            with self.subTest(load_size=load_size, virtual_size=virtual_size):
                binary, elf = self.fixture(load_size=load_size, virtual_size=virtual_size)
                with self.assertRaisesRegex(ValueError, message):
                    audit.audit_native_sizes(binary, elf)

    def test_emery_complete_binary_limit_and_relocation_length_remain_enforced(self):
        binary, elf = self.fixture(relocations=16386)
        with self.assertRaisesRegex(ValueError, "128 KiB"):
            audit.audit_native_sizes(binary, elf)
        binary, elf = self.fixture()
        with self.assertRaisesRegex(ValueError, "relocation table length"):
            audit.audit_native_sizes(binary + b"\x00", elf)

    def test_header_size_uuid_and_loaded_source_mismatch_are_rejected(self):
        binary, elf = self.fixture()
        for offset, message in ((14, "process-info sizes"), (104, "UUID"),
                                (200, "loaded section")):
            with self.subTest(offset=offset):
                changed = bytearray(binary)
                changed[offset] ^= 1
                with self.assertRaisesRegex(ValueError, message):
                    audit.audit_native_sizes(changed, elf)

    def test_truncated_elf_section_directory_is_rejected(self):
        binary, elf = self.fixture()
        with self.assertRaisesRegex(ValueError, "section directory"):
            audit.audit_native_sizes(binary, elf[:-1])


if __name__ == "__main__":
    unittest.main()
