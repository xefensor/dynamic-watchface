"""Release transformations are deterministic, bounded and separately debuggable."""
from pathlib import Path
import io
import json
import subprocess
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from release_js import audit_js, prepare_js, zip_payloads


class ReleaseMinificationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code = (b'/* Copyright 2024 Google LLC; Licensed under the Apache License, Version 2.0. */\n'
                    b'(function(){function serialized(){var seconds=42;return seconds;}return serialized.toString();})();\n')
        cls.mapping = json.dumps({'version': 3, 'file': 'pebble-js-app.js', 'names': [],
                                 'sources': ['./src/pkjs/index.js'], 'sourcesContent': [cls.code.decode()],
                                 'mappings': 'AAAA;AACA'}).encode()
        cls.plugin = b'(function(){var settings={};return settings;})();\n'
        cls.runtime, cls.debug = prepare_js(ROOT, '0.16.0', cls.code, cls.mapping, cls.plugin)

    def test_exact_transform_and_chained_maps_are_preserved_outside_runtime(self):
        result = audit_js(ROOT, '0.16.0', self.runtime, self.debug,
                          sdk_code=self.code, sdk_map=self.mapping, plugin=self.plugin)
        self.assertTrue(result['recomputedCodeAndMapsMatch'])
        self.assertLess(len(self.runtime['pebble-js-app.js']), len(self.code))
        self.assertIn(b'Copyright 2024 Google LLC', self.runtime['pebble-js-app.js'])
        self.assertNotIn(b'sourceMappingURL=', self.runtime['pebble-js-app.js'])
        chained = json.loads(self.debug['runtime/pebble-js-app.js.map'])
        self.assertEqual(chained['sourcesContent'], [self.code.decode()])

    def test_serialized_settings_function_keeps_a_self_contained_body(self):
        result = subprocess.check_output(['node', '-e',
            "const vm=require('vm'),fs=require('fs');const body=vm.runInNewContext(fs.readFileSync(0,'utf8'));"
            "console.log(vm.runInNewContext('('+body+')()'));"], input=self.runtime['pebble-js-app.js'])
        self.assertEqual(result.strip(), b'42')

    def test_embedded_offline_data_url_map_is_preserved_without_a_runtime_comment(self):
        source = b'var offline="//# sourceMappingURL=data:application/json;base64,e30=";\n'
        mapping = json.dumps({'version': 3, 'file': 'pebble-js-app.js', 'names': [], 'mappings': 'AAAA',
                              'sources': ['offline.js'], 'sourcesContent': [source.decode()]}).encode()
        runtime, debug = prepare_js(ROOT, '0.16.0', source, mapping, self.plugin)
        result = audit_js(ROOT, '0.16.0', runtime, debug, sdk_code=source, sdk_map=mapping)
        self.assertTrue(result['runtimeMapUrlsAbsent'])
        self.assertIn(b'sourceMappingURL=data:', runtime['pebble-js-app.js'])

    def test_utf8_translations_survive_minification_and_function_serialization(self):
        text = 'Čeština · 日本語 · Türkçe'
        source = ('(function(){function serialized(){var text=' + json.dumps(text, ensure_ascii=False) +
                  ';return text;}return serialized.toString();})();\n').encode()
        mapping = json.dumps({'version': 3, 'file': 'pebble-js-app.js', 'names': [], 'mappings': 'AAAA',
                              'sources': ['localized.js'], 'sourcesContent': [source.decode()]}).encode()
        runtime, debug = prepare_js(ROOT, '0.16.0', source, mapping, self.plugin)
        self.assertIn(text.encode(), runtime['pebble-js-app.js'])
        audit_js(ROOT, '0.16.0', runtime, debug, sdk_code=source, sdk_map=mapping)
        result = subprocess.check_output(['node', '-e',
            "const vm=require('vm'),fs=require('fs');const body=vm.runInNewContext(fs.readFileSync(0,'utf8'));"
            "console.log(vm.runInNewContext('('+body+')()'));"], input=runtime['pebble-js-app.js'])
        self.assertEqual(result.decode().strip(), text)

    def test_real_clay_replacement_preserves_minified_bitwise_function(self):
        variables = ','.join('local%d=%d' % (index, index + 1) for index in range(180))
        expression = '^'.join('(local%d&255)' % index for index in range(180))
        source = ('(function(){return function customFn(){var ' + variables + ';return ' + expression + ';};})();\n').encode()
        mapping = json.dumps({'version': 3, 'file': 'pebble-js-app.js', 'names': [], 'mappings': 'AAAA',
                              'sources': ['clay-custom.js'], 'sourcesContent': [source.decode()]}).encode()
        runtime, _debug = prepare_js(ROOT, '0.16.0', source, mapping, self.plugin)
        script = r'''
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const {identifier}=require('./scripts/minify-js.cjs');
const names=Array.from({length:10000},(_,index)=>identifier.get(index));
assert.equal(new Set(names).size,names.length);
assert.ok(names.every(name=>/^[A-Za-z_][A-Za-z_0-9]*$/.test(name)));
const customFn=vm.runInNewContext(fs.readFileSync(0,'utf8'));
let expected=0;for(let value=1;value<=180;value++)expected^=value&255;
assert.equal(customFn(),expected);
const sandbox={module:{exports:{}},exports:{},localStorage:{getItem(){return null;}},
  Pebble:{platform:'native',addEventListener(){}},
  require(name){if(name==='message_keys')return {};throw Error(name);}};
const clayBundle=require('node:path').join(require('node:path').dirname(require.resolve('@rebble/clay/package.json')),'dist/js/index.js');
vm.runInNewContext(fs.readFileSync(clayBundle,'utf8'),sandbox);
const Clay=sandbox.module.exports;
const html=decodeURIComponent(new Clay([],customFn,{autoHandleEvents:false}).generateUrl().split(',').slice(1).join(','));
assert.ok(!html.includes('$$CUSTOM_FN$$'),'String.replace must not reinsert the custom function placeholder');
let scripts=0;for(const match of html.matchAll(/<script(?:\s[^>]*)?>([\s\S]*?)<\/script>/g)){
  new vm.Script(match[1]);scripts++;
}
assert.ok(scripts>0);
console.log('Actual Clay generateUrl and generated script parsing: PASS');
'''
        result = subprocess.check_output(['node', '-e', script], cwd=ROOT, input=runtime['pebble-js-app.js'])
        self.assertIn(b'Actual Clay generateUrl', result)

    def test_runtime_or_debug_map_tampering_is_rejected(self):
        runtime = dict(self.runtime)
        runtime['plugin.js'] += b'var unexpected=true;\n'
        with self.assertRaisesRegex(ValueError, 'exact minification'):
            audit_js(ROOT, '0.16.0', runtime, self.debug)
        debug = dict(self.debug)
        debug['runtime/pebble-js-app.js.map'] += b' '
        with self.assertRaisesRegex(ValueError, 'exact minification'):
            audit_js(ROOT, '0.16.0', self.runtime, debug)

    def test_sdk_identity_and_debug_inventory_are_not_trusted_from_hashes(self):
        with self.assertRaisesRegex(ValueError, 'Original SDK/source'):
            audit_js(ROOT, '0.16.0', self.runtime, self.debug, sdk_code=b'another SDK input')
        debug = dict(self.debug)
        debug['unexpected.txt'] = b'not part of the recipe'
        with self.assertRaisesRegex(ValueError, 'inventory'):
            audit_js(ROOT, '0.16.0', self.runtime, debug)
        debug = dict(self.debug)
        manifest = json.loads(debug['manifest.json']); manifest['toolVersion'] = '999'
        debug['manifest.json'] = json.dumps(manifest).encode()
        with self.assertRaisesRegex(ValueError, 'provenance'):
            audit_js(ROOT, '0.16.0', self.runtime, debug)

    def test_debug_zip_is_deterministic_and_uses_regular_safe_members(self):
        first, second = zip_payloads(self.debug), zip_payloads(dict(reversed(list(self.debug.items()))))
        self.assertEqual(first, second)
        with zipfile.ZipFile(io.BytesIO(first)) as archive:
            self.assertEqual(archive.namelist(), sorted(self.debug))
            self.assertTrue(all(info.date_time == (1980, 1, 1, 0, 0, 0) and
                                info.external_attr >> 16 == 0o100644 for info in archive.infolist()))


if __name__ == '__main__':
    unittest.main()
