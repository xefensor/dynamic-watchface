"""Exact license documents carried by the standalone Dynamic PBW."""
from pathlib import Path
import hashlib
import io
import json
import tarfile


LICENSE_FILES = {
    "licenses/Dynamic-LICENSE.txt": "LICENSE",
    "licenses/THIRD-PARTY-NOTICES.txt": "THIRD-PARTY-NOTICES.md",
    "licenses/pebble-watchface-core-LICENSE.txt": "licenses/pebble-watchface-core-LICENSE.txt",
    "licenses/Clay-MIT.txt": "licenses/Clay-MIT.txt",
    "licenses/Clay-bundled-NOTICES.txt": "licenses/Clay-bundled-NOTICES.txt",
    "licenses/OFL-Fredoka.txt": "resources/fonts/OFL-Fredoka.txt",
    "licenses/OFL-RobotoCondensed.txt": "resources/fonts/OFL-RobotoCondensed.txt",
}


def license_payloads(root):
    root = Path(root)
    payloads = {name: (root / source).read_bytes()
                for name, source in LICENSE_FILES.items()}
    own = payloads["licenses/Dynamic-LICENSE.txt"]
    if (b"Required Notice: Dynamic watchface by Xef" not in own or
            b"Required Notice: Halftone watchface by Xef" not in own):
        raise ValueError("Required Dynamic or inherited Halftone attribution is missing")
    clay = payloads["licenses/Clay-MIT.txt"]
    if clay != (root / "node_modules/@rebble/clay/LICENSE").read_bytes():
        raise ValueError("Clay license differs from the installed pinned dependency")
    bundled = json.loads((root / "licenses/clay-bundled-provenance.json").read_text())
    if (hashlib.sha256((root / bundled["installed_bundle_file"]).read_bytes()).hexdigest() !=
            bundled["installed_bundle_sha256"] or
            hashlib.sha256(payloads["licenses/Clay-bundled-NOTICES.txt"]).hexdigest() !=
            bundled["notice_sha256"]):
        raise ValueError("Review bundled Clay dependency notices for this installed bundle")
    pin = json.loads((root / "vendor/CORE.json").read_text())
    with tarfile.open(fileobj=io.BytesIO((root / "vendor" / pin["artifact"]).read_bytes()),
                      mode="r:gz") as archive:
        core = archive.extractfile("package/LICENSE").read()
    if payloads["licenses/pebble-watchface-core-LICENSE.txt"] != core:
        raise ValueError("Core license differs from the pinned release archive")
    for name, marker in [("licenses/Clay-MIT.txt", b"Permission is hereby granted"),
                         ("licenses/OFL-Fredoka.txt", b"Fredoka Project Authors"),
                         ("licenses/OFL-RobotoCondensed.txt", b"Roboto Project Authors")]:
        if marker not in payloads[name]:
            raise ValueError("Required copyright/license text is missing: " + name)
    return payloads
