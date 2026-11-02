#!/usr/bin/env python3
"""Capture all 20 Clay presets from an explicitly selected Emery emulator.

Example (run with the Python environment used by the Pebble CLI):
  python3 scripts/capture-native-previews.py --emulator emery --pbw build/dynamic-watchface.pbw

A native-only copy of the candidate is installed through the phone bridge so
its PKJS cannot replace injected fixture states. Native payload bytes remain
identical. The actual installed copy and source candidate both receive hashes.

For an already initialized persistent shell, import this file and call
capture(pebble, pbw, output, appmessage=existing_appmessage_service). No emulator
is selected or opened merely by importing this module or using --plan.
"""
from __future__ import annotations
import argparse
import base64
import datetime
import hashlib
import json
from pathlib import Path
import subprocess
import threading
import time
import uuid
import zipfile

ROOT=Path(__file__).resolve().parents[1]
NATIVE_FILES=('emery/pebble-app.bin','emery/app_resources.pbpack','emery/manifest.json')
SAMPLE_TIME=int(datetime.datetime(2026,11,1,13,5,tzinfo=datetime.timezone.utc).timestamp())

def sha256(value):
    return hashlib.sha256(value).hexdigest()

def fixture_plan(session,now=SAMPLE_TIME-1,mode=None,sequence=None):
    options={'session':session,'now':now,'mode':mode}
    if sequence is not None:options['sequence']=sequence
    result=subprocess.run(['node',str(ROOT/'scripts/native-preview-fixtures.js')],
        input=json.dumps(options),capture_output=True,text=True,check=True,cwd=ROOT)
    return json.loads(result.stdout)

def native_candidate(pbw,output):
    """Omit phone JS while retaining exact candidate native bytes and metadata."""
    with zipfile.ZipFile(pbw) as source:
        appinfo=json.loads(source.read('appinfo.json'))
        native={name:source.read(name) for name in NATIVE_FILES}
        with zipfile.ZipFile(output,'w') as target:
            for info in source.infolist():
                if info.filename=='appinfo.json' or info.filename.startswith('emery/'):
                    target.writestr(info,source.read(info.filename))
    with zipfile.ZipFile(output) as target:
        assert 'pebble-js-app.js' not in target.namelist()
        assert all(target.read(name)==data for name,data in native.items())
    return appinfo,{name:sha256(data) for name,data in native.items()}

def capture(pebble,pbw,output,preview=None,appmessage=None):
    """Install a native-only candidate, inject fixtures, capture and embed PNGs.

    `pebble` must be a connected, asynchronously running libpebble2 connection
    to Emery/QEMU. An optional existing AppMessageService avoids registering a
    second service on a persistent shell's same endpoint. Other fixture senders
    should be stopped before calling this function.
    """
    from libpebble2.services.appmessage import AppMessageService,Uint32,ByteArray
    from libpebble2.services.screenshot import Screenshot
    from libpebble2.protocol.system import TimeMessage,SetUTC
    from libpebble2.protocol.apps import AppRunState,AppRunStateStart
    from libpebble2.communication.transports.qemu.protocol import QemuTimelinePeek,QemuTimeFormat,QemuBattery,QemuBluetoothConnection
    from pebble_tool.commands.emucontrol import send_data_to_qemu
    from pebble_tool.commands.install import ToolAppInstaller
    from PIL import Image

    pbw=Path(pbw).resolve();output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    preview=Path(preview or ROOT/'src/pkjs/settings-preview.js')
    native_pbw=output/'native-candidate.pbw'
    if native_pbw==pbw:raise ValueError('Source PBW must differ from the native-only output')
    appinfo,native_hashes=native_candidate(pbw,native_pbw)
    target=uuid.UUID(appinfo['uuid']);keys=appinfo['messageKeys']
    package=json.loads((ROOT/'package.json').read_text());version=package['version']
    if appinfo.get('versionLabel')!=version or appinfo['uuid']!=package['pebble']['uuid']:
        raise ValueError('Capture requires the current Dynamic version and UUID')
    service=appmessage or AppMessageService(pebble)
    owned=appmessage is None
    condition=threading.Condition();receipts={};sessions=[]
    def receive(_tid,app_uuid,data):
        if app_uuid!=target:return
        token=data.get(keys['REQUEST_ACTIVITIES'],data.get(keys['REQUEST_CONTEXT'],0))
        if isinstance(token,int) and token>=2:
            with condition:
                sessions.append(token);condition.notify_all()
    def receipt(success):
        def callback(tid,app_uuid):
            if app_uuid==target:
                with condition:receipts[tid]=success;condition.notify_all()
        return callback
    handles=[service.register_handler('appmessage',receive),
        service.register_handler('ack',receipt(True)),service.register_handler('nack',receipt(False))]
    def send(dictionary):
        with condition:
            tid=service.send_message(target,dictionary)
            if not condition.wait_for(lambda:tid in receipts,timeout=10):raise RuntimeError('AppMessage ACK timed out')
            if not receipts.pop(tid):raise RuntimeError('Watch rejected capture fixture')
    def set_time():
        pebble.send_packet(TimeMessage(message=SetUTC(unix_time=SAMPLE_TIME,utc_offset=0,tz_name='UTC')))
    def screenshot(path):
        rows=Screenshot(pebble).grab_image()
        if len(rows)!=228 or any(len(row)!=600 for row in rows):raise RuntimeError('Expected native 200×228 Screenshot-service RGB')
        pixels=b''.join(rows);Image.frombytes('RGB',(200,228),pixels).save(path)
        return pixels
    try:
        # The phone bridge replaces cached JS with a PBW whose JS source is
        # absent. Loading it stops the old JS runtime before native startup.
        send_data_to_qemu(pebble.transport,QemuBluetoothConnection(connected=True))
        ToolAppInstaller(pebble,str(native_pbw),quiet=True).install()
        # Some SDK bridge installs complete while the launcher/previous face
        # remains visible. Explicitly start this exact installed native UUID.
        pebble.send_packet(AppRunState(data=AppRunStateStart(uuid=target)))
        with condition:
            if not condition.wait_for(lambda:bool(sessions),timeout=8):
                raise RuntimeError('Native session request not received after install')
            session=sessions[-1]
        plan=fixture_plan(session)
        (output/'fixtures.json').write_text(json.dumps(plan,indent=2)+'\n')
        send_data_to_qemu(pebble.transport,QemuBattery(percent=100,charging=False))
        frames={};captures=[]
        for preset in plan['presets']:
            if any(token!=session for token in sessions):raise RuntimeError('Native session changed during capture')
            set_time()
            send_data_to_qemu(pebble.transport,QemuTimeFormat(is_24_hour=preset['format']==24))
            send_data_to_qemu(pebble.transport,QemuTimelinePeek(enabled=preset['state']=='timeline'))
            dictionary={keys[name]:Uint32(value) for name,value in preset['integers'].items()}
            dictionary.update({keys[name]:ByteArray(bytes(value)) for name,value in preset['bytes'].items()})
            send(dictionary)
            time.sleep(.8)
            path=output/(preset['name']+'.png');previous=None
            for attempt in range(3):
                pixels=screenshot(path)
                if previous==pixels:break
                previous=pixels;time.sleep(.15)
            else:raise RuntimeError('Native screenshot did not settle: '+preset['name'])
            png=path.read_bytes()
            frames[preset['name']]={'src':'data:image/png;base64,'+base64.b64encode(png).decode(),
                'sha256':sha256(png),'preserve':preset['preserve']}
            captures.append({'name':preset['name'],'file':path.name,'sha256':sha256(png),
                'rgb_sha256':sha256(pixels),'pixels':[200,228],'native_pixels_modified':False,
                'settled_native_rgb_equal':True})
            print('Captured '+preset['name'],flush=True)
        if len({row['rgb_sha256'] for row in captures})!=20:
            raise RuntimeError('Expected 20 distinct native state/clock/theme images; inspect captures before embedding')
        result={'version':version,'source':{'capturedFromCandidatePbwSha256':sha256(native_pbw.read_bytes()),
            'candidateSourcePbwSha256':sha256(pbw.read_bytes()),'nativePayloadSha256':native_hashes,
            'timelineSample':'Actual native unobstructed geometry; reserved OS panel, no event payload.'},'frames':frames}
        report={'version':version,'pbw_sha256':sha256(native_pbw.read_bytes()),
            'candidate_source_pbw_sha256':sha256(pbw.read_bytes()),'native_payload_sha256':native_hashes,
            'phone_javascript_executed':False,'fixture_encoder':{'core':plan['coreVersion'],
                'activities':plan['activitiesVersion'],'core_wire_version':4,'activity_wire_version':1},
            'fixture_time_utc':SAMPLE_TIME,'session':session,'captures':captures,
            'scope':'20 actual native Screenshot-service presets; sample data, not live phone/provider verification.',
            'visual_review':{'reviewed':False}}
        # Write assets only after all twenty native captures settle successfully.
        preview.write_text("'use strict';\n\n// Actual native "+version+" samples; generated by scripts/capture-native-previews.py.\nmodule.exports = "+json.dumps(result,separators=(',',':'))+";\n")
        (output/'previews.json').write_text(json.dumps(report,indent=2)+'\n')
        return report
    finally:
        send_data_to_qemu(pebble.transport,QemuTimelinePeek(enabled=False))
        for handle in handles:service.unregister_handler(handle)
        if owned:service.shutdown()


def capture_activities(pebble,session,output,appmessage=None):
    """Capture affected generic layouts on the existing native candidate.

    Call after capture() in the same emulator session. This function does not
    reinstall, launch, reconnect or modify the twenty Clay preview frames.
    """
    from libpebble2.services.appmessage import AppMessageService,Uint32,ByteArray
    from libpebble2.services.screenshot import Screenshot
    from libpebble2.protocol.system import TimeMessage,SetUTC
    from libpebble2.communication.transports.qemu.protocol import QemuTimelinePeek,QemuTimeFormat
    from pebble_tool.commands.emucontrol import send_data_to_qemu
    from PIL import Image

    output=Path(output).resolve();output.mkdir(parents=True,exist_ok=True)
    exported=(ROOT/'src/pkjs/settings-preview.js').read_text().split('module.exports = ',1)[1].strip().removesuffix(';')
    preview=json.loads(exported)
    with zipfile.ZipFile(output.parent/'native-previews/native-candidate.pbw') if (output.parent/'native-previews/native-candidate.pbw').is_file() else zipfile.ZipFile(ROOT/'build/dynamic-watchface.pbw') as archive:
        appinfo=json.loads(archive.read('appinfo.json'))
    target=uuid.UUID(appinfo['uuid']);keys=appinfo['messageKeys']
    if preview['version']!=json.loads((ROOT/'package.json').read_text())['version']:
        raise ValueError('Capture the current twenty native presets first')
    service=appmessage or AppMessageService(pebble);owned=appmessage is None
    condition=threading.Condition();receipts={};changed=[]
    def receipt(success):
        def callback(tid,app_uuid):
            if app_uuid==target:
                with condition:receipts[tid]=success;condition.notify_all()
        return callback
    def receive(_tid,app_uuid,data):
        if app_uuid!=target:return
        token=data.get(keys['REQUEST_ACTIVITIES'],data.get(keys['REQUEST_CONTEXT'],0))
        if isinstance(token,int) and token>=2 and token!=session:changed.append(token)
    handles=[service.register_handler('ack',receipt(True)),service.register_handler('nack',receipt(False)),
        service.register_handler('appmessage',receive)]
    sequence=100
    previous_plan=output/'fixtures.json'
    if previous_plan.is_file():
        previous=json.loads(previous_plan.read_text())
        if previous.get('session')==session:
            sequence=max([sequence]+[int.from_bytes(bytes(p['bytes']['ACTIVITY_DATA'])[16:20],'little') for p in previous['presets']])
    plan=fixture_plan(session,now=SAMPLE_TIME,mode='activities',sequence=sequence)
    (output/'fixtures.json').write_text(json.dumps(plan,indent=2)+'\n')
    captures=[]
    try:
        for preset in plan['presets']:
            if changed:raise RuntimeError('Native session changed; repeat the twenty-preview capture first')
            pebble.send_packet(TimeMessage(message=SetUTC(unix_time=SAMPLE_TIME,utc_offset=0,tz_name='UTC')))
            send_data_to_qemu(pebble.transport,QemuTimeFormat(is_24_hour=True))
            send_data_to_qemu(pebble.transport,QemuTimelinePeek(enabled=preset['state']=='timeline'))
            dictionary={keys[name]:Uint32(value) for name,value in preset['integers'].items()}
            dictionary.update({keys[name]:ByteArray(bytes(value)) for name,value in preset['bytes'].items()})
            with condition:
                tid=service.send_message(target,dictionary)
                if not condition.wait_for(lambda:tid in receipts,timeout=10):raise RuntimeError('Activity fixture ACK timed out')
                if not receipts.pop(tid):raise RuntimeError('Watch rejected activity capture fixture')
            time.sleep(.8)
            previous=None;path=output/(preset['name']+'.png')
            for attempt in range(3):
                rows=Screenshot(pebble).grab_image()
                if len(rows)!=228 or any(len(row)!=600 for row in rows):raise RuntimeError('Expected native 200×228 RGB')
                pixels=b''.join(rows)
                # Clocks advance locally once a second. Compare settled clock,
                # date, both readings and footer without freezing the activity.
                footer=pixels[200*176*3:]
                if len(preset.get('activities',[]))>1:
                    # A second timer is a locally advancing secondary line.
                    # Compare its static label region without freezing it.
                    footer=pixels[200*176*3:200*199*3]+pixels[200*216*3:]
                stable=pixels[:200*92*3]+footer
                if previous==stable:break
                previous=stable;time.sleep(.15)
            else:raise RuntimeError('Activity header/readings/footer did not settle: '+preset['name'])
            Image.frombytes('RGB',(200,228),pixels).save(path)
            captures.append({'name':preset['name'],'file':path.name,'sha256':sha256(path.read_bytes()),
                'rgb_sha256':sha256(pixels),'pixels':[200,228],'native_pixels_modified':False,
                'settled_header_readings_footer_rgb_equal':True,'running_clock_changes_allowed':True,
                'two_configured_readings':True})
            print('Captured '+preset['name'],flush=True)
        # Native Peek hides the activity area, so different clock values must
        # intentionally produce the same unobstructed face above its OS panel.
        visible=[row for row,preset in zip(captures,plan['presets']) if preset['state']!='timeline']
        if len({row['rgb_sha256'] for row in visible})!=len(visible):raise RuntimeError('Expected distinct visible generic layouts')
        report={'version':preview['version'],'session':session,'source':preview['source'],
            'phone_javascript_executed':False,'candidate_install_unchanged':True,
            'fixture_encoder':{'core':plan['coreVersion'],'activities':plan['activitiesVersion'],
                'core_wire_version':4,'activity_wire_version':1},'captures':captures,
            'scope':str(len(captures))+' actual generic activity layouts with two configured sample readings. Elapsed workout is a plugin fixture, not native Workout integration.',
            'visual_review':{'reviewed':False}}
        (output/'activities.json').write_text(json.dumps(report,indent=2)+'\n')
        return report
    finally:
        send_data_to_qemu(pebble.transport,QemuTimelinePeek(enabled=False))
        for handle in handles:service.unregister_handler(handle)
        if owned:service.shutdown()


def main():
    parser=argparse.ArgumentParser(description=__doc__,formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--emulator',choices=['emery'],help='Explicit emulator selection; never inferred')
    parser.add_argument('--pbw',type=Path,default=ROOT/'build/dynamic-watchface.pbw')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--preview',type=Path,default=ROOT/'src/pkjs/settings-preview.js')
    parser.add_argument('--plan',action='store_true',help='Print fixture plan only; no connection, installation or images')
    args=parser.parse_args()
    if args.plan:
        print(json.dumps(fixture_plan(42),indent=2));return
    if not args.emulator:parser.error('Capture requires explicit --emulator emery')
    from pebble_tool.commands.base import PebbleCommand
    class PreviewConnection(PebbleCommand):
        valid_connections={'emulator'}
    command=PreviewConnection();command._set_debugging(0)
    connection=command._connect(argparse.Namespace(v=0,emulator=args.emulator,sdk=None,vnc=False,sunlight=False))
    version=json.loads((ROOT/'package.json').read_text())['version']
    capture(connection,args.pbw,args.output or ROOT/'docs/qa'/version/'native-previews',args.preview)

if __name__=='__main__':main()
