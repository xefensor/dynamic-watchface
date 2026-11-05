"""Capture the same media track in primary/secondary playback states on Emery.

Run from Dynamic with recovery-validation/env.sh and the workspace FIFO adapter.
Arguments: candidate.pbw output-directory [--expect-bug]. All sources are
synthetic. The candidate native payload is installed without instrumentation.
"""
from pathlib import Path
import copy, hashlib, importlib.util, json, subprocess, sys, threading, time, uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT.parent / 'recovery-validation'))
from pipe_emulator import connect
from libpebble2.services.appmessage import AppMessageService, Uint32, ByteArray
from libpebble2.services.screenshot import Screenshot
from libpebble2.protocol.apps import AppRunState, AppRunStateStart
from libpebble2.communication.transports.qemu.protocol import QemuBluetoothConnection, QemuTimelinePeek
from pebble_tool.commands.emucontrol import send_data_to_qemu
from pebble_tool.commands.install import ToolAppInstaller
from PIL import Image

OUTPUT = Path(sys.argv[2]).resolve()
OUTPUT.mkdir(parents=True,exist_ok=True)
spec = importlib.util.spec_from_file_location('native_capture', ROOT/'scripts/capture-native-previews.py')
capture = importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
candidate = Path(sys.argv[1]).resolve()
info, native = capture.native_candidate(candidate, OUTPUT/'native-candidate.pbw')
target = uuid.UUID(info['uuid']);keys = info['messageKeys']
p = connect();service = AppMessageService(p);condition = threading.Condition()
receipts, sessions, statuses = {}, [], []

def received(_transaction, app, values):
    if app != target:return
    with condition:
        token = values.get(keys['REQUEST_ACTIVITIES'], values.get(keys['REQUEST_CONTEXT'], 0))
        if isinstance(token, int) and token >= 2:sessions.append(token)
        raw = values.get(keys['ACTIVITY_PRESENTED'])
        if raw is not None:
            raw = bytes(raw)
            statuses.append(dict(primary=raw[1], secondary=raw[2], session=int.from_bytes(raw[4:8], 'little'),
                view=int.from_bytes(raw[8:12], 'little'), sequence=int.from_bytes(raw[12:16], 'little'),
                reported=int.from_bytes(raw[16:20], 'little'), expiry=int.from_bytes(raw[20:24], 'little')))
        condition.notify_all()

def receipt(ok):
    def done(transaction, app):
        if app == target:
            with condition:receipts[transaction] = ok;condition.notify_all()
    return done

service.register_handler('appmessage', received)
service.register_handler('ack', receipt(True));service.register_handler('nack', receipt(False))

def send(values):
    with condition:
        transaction = service.send_message(target, values)
        assert condition.wait_for(lambda:transaction in receipts, 10), 'No native ACK'
        assert receipts.pop(transaction), 'Native rejected fixture'

def preset(name, sequence):
    plan = capture.fixture_plan(sessions[-1], now=int(time.time()), mode='summary', sequence=sequence)
    return next(item for item in plan['presets'] if item['name'] == name)

def send_preset(value):
    values = {keys[key]:Uint32(item) for key,item in value['integers'].items()}
    values.update({keys[key]:ByteArray(bytes(item)) for key,item in value['bytes'].items()})
    send(values)

captures = []
def shot(name, delay=.2):
    time.sleep(delay);rows = Screenshot(p).grab_image();raw = b''.join(rows)
    assert len(raw) == 200*228*3
    path = OUTPUT/(name+'.png');Image.frombytes('RGB', (200,228), raw).save(path)
    captures.append(dict(name=name, file=path.name, sha256=hashlib.sha256(path.read_bytes()).hexdigest(), nativePixelsModified=False))
    return raw

def encode(cards, sequence):
    script = "const a=require('pebble-activities'),p=require('./src/pkjs/activity-presentation'),d=JSON.parse(require('fs').readFileSync(0,'utf8'));const cards=d.cards.map(c=>c.type==='media'?p.project(c):c);process.stdout.write(JSON.stringify(a.summaryView.encode(cards,{session:d.session,sequence:d.sequence,sentAt:d.now})));"
    result = subprocess.run(['node', '-e', script], cwd=ROOT, input=json.dumps(dict(cards=cards,
        session=sessions[-1], sequence=sequence, now=int(time.time()))), text=True, capture_output=True, check=True)
    return ByteArray(bytes(json.loads(result.stdout)))

send_data_to_qemu(p.transport, QemuBluetoothConnection(connected=True))
send_data_to_qemu(p.transport, QemuTimelinePeek(enabled=False))
ToolAppInstaller(p, str(OUTPUT/'native-candidate.pbw'), quiet=True).install()
p.send_packet(AppRunState(data=AppRunStateStart(uuid=target)))
with condition:assert condition.wait_for(lambda:bool(sessions), 10), 'No native session'


media = preset('media-alone-light', 1000)
active = copy.deepcopy(media['activities'][0]);active['state']='active';active['playback']='playing'
paused = copy.deepcopy(active);paused['state']='paused';paused['playback']='paused';paused['revision']+=1
send_preset(media)
first = shot('active-primary', .8)
send({keys['ACTIVITY_DATA']:encode([paused], 5000)})
second = shot('paused-primary', .3)
primary_same = first[200*92*3:] == second[200*92*3:]
if '--expect-bug' in sys.argv:assert primary_same, 'Old primary Paused bug did not reproduce'
else:assert not primary_same, 'Primary media still hides its paused state'
pair = preset('timer-media-light', 6000)
other = copy.deepcopy(pair['activities'][0])
active['revision']=paused['revision']+1
send({keys['ACTIVITY_DATA']:encode([other, active], 6000)})
active_secondary = shot('active-secondary', .7)
paused['revision']=active['revision']+1
send({keys['ACTIVITY_DATA']:encode([other, paused], 6001)})
paused_secondary = shot('paused-secondary', .3)
assert active_secondary[200*92*3:200*118*3] != paused_secondary[200*92*3:200*118*3], 'Secondary Paused missing'
no_artist = copy.deepcopy(paused);no_artist['revision']+=1;no_artist['support']=None;no_artist['caption']='Music';no_artist['qualifier']=''
send({keys['ACTIVITY_DATA']:encode([no_artist], 7000)})
shot('paused-primary-no-artist', .7)
typed_checks = []
if '--expect-bug' not in sys.argv:
    def typed(playback, revision):
        return dict(type='media', provider=active['provider'], private=True,
            revision=revision, observedAt=active['observedAt'], validUntil=active['validUntil'],
            record=dict(id=active['id'], type='media', title=active['main']['text'],
                artist=active['support']['text'], itemId=active['mediaItemId'],
                artwork=active['image'], playback=playback, openApp=active['openApp']))
    primary_baseline = secondary_baseline = None
    for index,playback in enumerate(['playing','paused','buffering','stopped','unknown']):
        # Settle the legitimate primary-role change before testing playback
        # alone; all four snapshots retain the same track visual identity.
        send({keys['ANIMATIONS']:Uint32(1), keys['ACTIVITY_DATA']:encode([typed('playing',20000+index*4)],8000+index*4)})
        time.sleep(.4)
        item = typed(playback, 20001+index*4)
        send({keys['ACTIVITY_DATA']:encode([item], 8001+index*4)})
        primary = shot('typed-'+playback+'-primary', .05)
        primary_settled = shot('typed-'+playback+'-primary-settled', .35)
        assert primary[200*92*3:] == primary_settled[200*92*3:], 'Playback-only change replayed track motion'
        if playback == 'playing':primary_baseline = primary
        else:assert (primary_baseline[200*92*3:] != primary[200*92*3:]) == (playback == 'paused')
        send({keys['ACTIVITY_DATA']:encode([other,typed('playing',20002+index*4)],8002+index*4)})
        time.sleep(.4)
        item['revision'] += 2
        send({keys['ACTIVITY_DATA']:encode([other,item],8003+index*4)})
        secondary = shot('typed-'+playback+'-secondary', .1)
        if playback == 'playing':secondary_baseline = secondary
        else:assert (secondary_baseline[200*92*3:200*118*3] != secondary[200*92*3:200*118*3]) == (playback == 'paused')
        typed_checks.append(dict(playback=playback,primaryAndSecondaryVerified=True,trackMotionNotReplayed=True))
    expiring = copy.deepcopy(paused);expiring['revision']=30000
    expiring['support']['validUntil']=int(time.time())+2
    send({keys['ACTIVITY_DATA']:encode([expiring], 10000)})
    artist = shot('paused-artist-before-expiry', .7)
    expired = shot('paused-artist-after-expiry', 2)
    assert artist[200*168*3:] != expired[200*168*3:], 'Expired artist remained visible'
report = dict(result='REPRODUCED' if '--expect-bug' in sys.argv else 'PASS', version=info['versionLabel'],
    candidatePbwSha256=hashlib.sha256(candidate.read_bytes()).hexdigest(),nativePayloadSha256=native,
    syntheticSourceData=True,phoneJavascriptExecuted=False,captures=captures,nativeStatuses=statuses,
    primaryActiveAndPausedPixelsEqual=primary_same,secondaryPausedPixelsDiffer=True,typedPlaybackChecks=typed_checks,
    scope='Actual Emery same-track primary/secondary state rendering. All source data synthetic; no real phone or watch claim.')
(OUTPUT/'media-state.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k] for k in ['result','version','primaryActiveAndPausedPixelsEqual','secondaryPausedPixelsDiffer']}),flush=True)
