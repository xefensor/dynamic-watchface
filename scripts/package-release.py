#!/usr/bin/env python3
"""Permission-package exact SDK payloads with deterministic license documents."""
from pathlib import Path
import hashlib, json, zipfile, subprocess
from release_licenses import license_payloads
root = Path(__file__).resolve().parents[1]
pkg = json.loads((root / 'package.json').read_text())
source = root / 'build' / (pkg['name'] + '.pbw')
target = root / 'dist' / ('Dynamic-' + pkg['version'] + '.pbw')
target.parent.mkdir(exist_ok=True)
subprocess.run(['python3',str(root/'scripts/bundle-js.py'),str(root/'src/plugin/index.js'),str(root/'build/plugin.js')],check=True,cwd=root)
licenses = license_payloads(root)
with zipfile.ZipFile(source) as sdk:
    assert sdk.testzip() is None
    assert len(sdk.namelist()) == len(set(sdk.namelist())) == 6
    info = json.loads(sdk.read('appinfo.json'))
    assert info['uuid'] == pkg['pebble']['uuid']
    assert info['versionLabel'] == pkg['version']
    assert info['targetPlatforms'] == ['emery']
    assert info['watchapp']['watchface'] is True
    info['manifestVersion'] = 2
    info['usesPermissions'] = pkg['pebble']['usesPermissions']
    info['plugin'] = pkg['pebble']['plugin']
    payloads = {name: sdk.read(name) for name in sdk.namelist()}
    payloads['appinfo.json'] = (json.dumps(info, sort_keys=True, separators=(',', ':')) + '\n').encode()
    assert not set(payloads).intersection(licenses)
    payloads.update(licenses)
    payloads['plugin.js'] = (root/'build/plugin.js').read_bytes()
    with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as output:
        for name, data in sorted(payloads.items()):
            entry = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(target) as result:
        assert result.testzip() is None
        assert set(result.namelist()) == set(sdk.namelist()) | set(licenses) | {'plugin.js'}
        for name in sdk.namelist():
            if name != 'appinfo.json': assert sdk.read(name) == result.read(name)
        for name, data in licenses.items(): assert result.read(name) == data
print(target)
print(hashlib.sha256(target.read_bytes()).hexdigest())
