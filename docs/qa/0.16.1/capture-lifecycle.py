"""Focused actual Emery cover/expiry/clock proof; synthetic sources, no phone.

Run from Dynamic with recovery-validation/env.sh and the workspace FIFO adapter.
Uses the candidate native payload unchanged; it does not instrument shipping C.
"""
from pathlib import Path
import copy, hashlib, importlib.util, json, subprocess, sys, threading, time, uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT.parent / 'recovery-validation'))
from pipe_emulator import connect
from libpebble2.services.appmessage import AppMessageService, Uint32, ByteArray
from libpebble2.services.screenshot import Screenshot
from libpebble2.protocol.apps import AppRunState, AppRunStateStart
from libpebble2.protocol.phone import PhoneNotification, IncomingCall, CallEnd
from libpebble2.communication.transports.qemu.protocol import QemuBluetoothConnection, QemuTimelinePeek, QemuButton
from pebble_tool.commands.emucontrol import send_data_to_qemu
from pebble_tool.commands.install import ToolAppInstaller
from PIL import Image

OUTPUT = Path(__file__).resolve().parent / 'native-lifecycle'
OUTPUT.mkdir(exist_ok=True)
spec = importlib.util.spec_from_file_location('native_capture', ROOT/'scripts/capture-native-previews.py')
capture = importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
candidate = ROOT/'build/dynamic-watchface.pbw'
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
    script = "const a=require('pebble-activities'),d=JSON.parse(require('fs').readFileSync(0,'utf8'));process.stdout.write(JSON.stringify(a.summaryView.encode(d.cards,{session:d.session,sequence:d.sequence,sentAt:d.now})));"
    result = subprocess.run(['node', '-e', script], cwd=ROOT, input=json.dumps(dict(cards=cards,
        session=sessions[-1], sequence=sequence, now=int(time.time()))), text=True, capture_output=True, check=True)
    return ByteArray(bytes(json.loads(result.stdout)))

send_data_to_qemu(p.transport, QemuBluetoothConnection(connected=True))
send_data_to_qemu(p.transport, QemuTimelinePeek(enabled=False))
ToolAppInstaller(p, str(OUTPUT/'native-candidate.pbw'), quiet=True).install()
p.send_packet(AppRunState(data=AppRunStateStart(uuid=target)))
with condition:assert condition.wait_for(lambda:bool(sessions), 10), 'No native session'

timer = preset('timer-media-light', 1000);send_preset(timer)
first = shot('independent-clock-before', .8);second = shot('independent-clock-after', 1.2)
assert first[200*116*3:] != second[200*116*3:], 'Visible countdown stopped ticking'
media = preset('media-alone-light', 2000);send_preset(media)
before = shot('static-before-heartbeat', .8)
baseline = statuses[-1]
with condition:
    assert condition.wait_for(lambda:any(item['view'] == baseline['view'] and
        item['reported'] >= baseline['reported']+30 for item in statuses), 35), 'No30-second native heartbeat'
after = shot('static-after-heartbeat', 0)
assert before[200*92*3:] == after[200*92*3:], 'Static activity moved during heartbeat'

moving = copy.deepcopy(media['activities'][0]);moving['main']['text'] = 'A different song';moving['visualToken'] = [1,2,3,4,5,6,7,8]
send({keys['ANIMATIONS']:Uint32(1), keys['ACTIVITY_DATA']:encode([moving], 5000)})
cookie = 0x44594e31
p.send_packet(PhoneNotification(cookie=cookie, message=IncomingCall(number='000', name='Dynamic cover QA')))
covered = shot('actual-os-call-cover', .3)

under = preset('notification-media-light', 6000)
cards = copy.deepcopy(under['activities'])
expires = int(time.time())+4
cards[0]['validUntil'] = expires
for field in ('main', 'support'):
    if cards[0].get(field):cards[0][field]['validUntil'] = expires
send({keys['ACTIVITY_DATA']:encode(cards, 6000)})
time.sleep(5.2)
assert not any(item['view'] == 6000 for item in statuses), 'Covered view manufactured native presentation status'
shot('covered-after-source-expiry', 0)
p.send_packet(PhoneNotification(cookie=cookie, message=CallEnd()))
ended = shot('actual-os-call-summary', .6)
if ended[:3] == bytes((255,0,0)):
    send_data_to_qemu(p.transport, QemuButton(state=int(QemuButton.Button.Back)));time.sleep(.08)
    send_data_to_qemu(p.transport, QemuButton(state=0))
with condition:
    assert condition.wait_for(lambda:any(item['view'] == 6000 and item['primary'] == 1 for item in statuses), 5), 'Fresh promoted view was not confirmed after cover'
returned = shot('returned-promoted-media', .7)
settled = shot('returned-still-media', .3)
assert returned[200*92*3:] == settled[200*92*3:], 'Cover return replayed activity motion'
assert returned != covered, 'System cover did not dismiss'
assert all(item['primary'] == 1 and item['secondary'] == 255 for item in statuses if item['view'] == 6000)
report = dict(result='PASS', version=info['versionLabel'], candidatePbwSha256=hashlib.sha256(candidate.read_bytes()).hexdigest(),
    nativePayloadSha256=native, syntheticSourceData=True, phoneJavascriptExecuted=False,
    captures=captures, nativeStatuses=statuses, proofs=dict(localClockAdvancedWithoutResend=True,
        staticHeartbeatWithoutPaintChange=True, coveredPresentationAbsent=True,
        coveredExpiryPromotedOriginalIndex=1, returnStillWithoutMotionReplay=True),
    scope='Actual isolated Emery source/cover/expiry/clock pixels. Callback work counts are separately proven by the fake-SDK test; no physical battery measurement.')
(OUTPUT/'lifecycle.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(dict(result='PASS', captures=len(captures), proofs=report['proofs'])), flush=True)
