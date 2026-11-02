'use strict';
const assert=require('node:assert/strict');
const {test}=require('node:test');
const {fixtures,activityPresets}=require('../scripts/native-preview-fixtures');
const core=require('pebble-watchface-core').wire;
const activities=require('pebble-activities').wire;

test('native capture fixtures preserve all 20 state, format and theme presets',()=>{
  const plan=fixtures({session:42,now:1793538299});
  const expected=[];
  for(const theme of ['light','dark'])for(const format of [24,12])
    for(const state of ['idle','music','notification-summary','notification','timeline'])
      expected.push(`${state}-${format}-${theme}`);
  assert.deepEqual(plan.presets.map(p=>p.name),expected);
  for(const [index,preset]of plan.presets.entries()) {
    assert.equal(preset.integers.CLOCK_FORMAT,preset.format);
    assert.equal(preset.integers.REQUEST_CONTEXT,42);
    assert.equal(preset.integers.BACKGROUND_COLOR,preset.theme==='dark'?0:0xffffff);
    for(const field of ['COMPLICATION_DATA','CONTEXT_DATA']) {
      const packet=preset.bytes[field].slice();
      assert.equal(packet[0],4);assert.equal(packet[2]|packet[3]<<8,packet.length);
      const crc=packet[4]|packet[5]<<8;packet[4]=packet[5]=0;
      assert.equal(core.crc16Xmodem(packet),crc);
    }
    const packet=Buffer.from(preset.bytes.ACTIVITY_DATA);
    assert.equal(packet.toString('ascii',0,4),'ACT1');assert.equal(packet[5],0);
    assert.equal(packet.readUInt32LE(12),42);assert.equal(packet.readUInt32LE(16),index+1);
    assert.equal(activities.crc16(preset.bytes.ACTIVITY_DATA),packet.readUInt16LE(8));
    assert.deepEqual(preset.preserve,preset.state==='timeline'?[{x:0,y:171,width:200,height:57}]:
      preset.state==='music'||preset.state==='notification-summary'?[{x:10,y:176,width:40,height:40}]:[]);
  }
});

test('capture plans require a current native session and UTC observation time',()=>{
  for(const options of [{session:1,now:100},{session:42,now:0},{session:'42',now:100}])
    assert.throws(()=>fixtures(options));
});

test('six affected generic fixtures keep both readings and use later ACT1 sequences',()=>{
  const plan=activityPresets({session:42,now:1793538300});
  assert.deepEqual(plan.presets.map(p=>p.name),['timer-running','timer-paused','glucose-numeric',
    'elapsed','timer-heading-banner','timer-timeline']);
  for(const [index,preset]of plan.presets.entries()) {
    const packet=Buffer.from(preset.bytes.ACTIVITY_DATA);
    assert.equal(packet[5],1);assert.equal(packet.readUInt32LE(12),42);
    assert.equal(packet.readUInt32LE(16),101+index);
    let at=6;const statuses=[];
    for(let slot=0;slot<5;slot++) {
      const bytes=preset.bytes.COMPLICATION_DATA;
      statuses.push(bytes[at+1]);
      at+=56+bytes[at+2]+bytes[at+22]+bytes[at+23]+
        (bytes[at]&2?16+(bytes[at+14]|bytes[at+15]<<8):0);
    }
    assert.deepEqual(statuses,[2,2,0,0,0]);
  }
  const glucose=Buffer.from(plan.presets[2].bytes.ACTIVITY_DATA);
  assert.ok(glucose[26]&4);assert.equal(glucose.readDoubleLE(52),6.2);
  assert.equal(plan.presets[1].activity.state,'paused');
  assert.equal(plan.presets[3].activity.clock,'elapsed');
  assert.equal(plan.presets[4].state,'notification-summary');
  assert.equal(plan.presets[5].state,'timeline');
  const later=activityPresets({session:42,now:1793538300,sequence:106});
  assert.equal(Buffer.from(later.presets[0].bytes.ACTIVITY_DATA).readUInt32LE(16),107);
});
