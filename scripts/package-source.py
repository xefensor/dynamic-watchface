#!/usr/bin/env python3
"""Create buildable source with pinned libraries, excluding generated QA PNG copies."""
from pathlib import Path
import hashlib, io, json, re, subprocess, zipfile
root = Path(__file__).resolve().parents[1]
pkg = json.loads((root / 'package.json').read_text())
raw = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=root)
names = sorted({s.decode() for s in raw.split(b'\0') if s and not s.startswith(b'dist/')
    and not (s.startswith(b'docs/qa/') and s.lower().endswith(b'.png'))
    and (root / s.decode()).is_file()})
for name in names:
    assert not name.startswith(('build/', 'node_modules/', '.git/', '.pebble/', '.lock-waf'))
    assert not re.search(r'(^|/)(\.env|credentials|secrets)(\.|/|$)', name, re.I)
buffer = io.BytesIO()
with zipfile.ZipFile(buffer, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for name in names:
        info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
        info.create_system = 3
        info.external_attr = 0o100644 << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        archive.writestr(info, (root / name).read_bytes(), compresslevel=9)
target = root / 'dist' / ('Dynamic-' + pkg['version'] + '-source.zip')
target.parent.mkdir(exist_ok=True)
target.write_bytes(buffer.getvalue())
with zipfile.ZipFile(target) as archive:
    assert archive.testzip() is None and archive.namelist() == names
    for name in names: assert archive.read(name) == (root / name).read_bytes()
pbw = root / 'dist' / ('Dynamic-' + pkg['version'] + '.pbw')
checks = []
debug = root / 'dist' / ('Dynamic-' + pkg['version'] + '-debug.zip')
for path in (pbw, target, debug):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    checks.append(digest + '  ' + path.name)
    print(path.name, path.stat().st_size, digest)
(root / 'dist/SHA256SUMS.txt').write_text('\n'.join(checks) + '\n')
print('Source files:', len(names))
