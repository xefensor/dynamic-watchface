#!/usr/bin/env python3
"""Permission-package the exact SDK-built Dynamic PBW."""
from pathlib import Path
import copy, hashlib, json, zipfile
root = Path(__file__).resolve().parents[1]
pkg = json.loads((root / 'package.json').read_text())
source = root / 'build' / (pkg['name'] + '.pbw')
target = root / 'dist' / ('Dynamic-' + pkg['version'] + '.pbw')
target.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(source) as sdk:
    assert sdk.testzip() is None
    info = json.loads(sdk.read('appinfo.json'))
    assert info['uuid'] == pkg['pebble']['uuid']
    assert info['versionLabel'] == pkg['version']
    assert info['targetPlatforms'] == ['emery']
    assert info['watchapp']['watchface'] is True
    info['manifestVersion'] = 2
    info['usesPermissions'] = pkg['pebble']['usesPermissions']
    with zipfile.ZipFile(target, 'w', compression=zipfile.ZIP_DEFLATED) as output:
        for entry in sdk.infolist():
            data = (json.dumps(info, separators=(',', ':')) + '\n').encode() if entry.filename == 'appinfo.json' else sdk.read(entry.filename)
            output.writestr(copy.copy(entry), data)
    with zipfile.ZipFile(target) as result:
        assert result.testzip() is None
        assert sdk.namelist() == result.namelist()
        for name in sdk.namelist():
            if name != 'appinfo.json': assert sdk.read(name) == result.read(name)
print(target)
print(hashlib.sha256(target.read_bytes()).hexdigest())
