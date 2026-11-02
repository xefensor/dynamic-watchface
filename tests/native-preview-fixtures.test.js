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

function activityRecords(packetBytes) {
  const packet=Buffer.from(packetBytes),records=[];
  assert.equal(packet.readUInt16LE(6),packet.length);
  assert.equal(activities.crc16(packetBytes),packet.readUInt16LE(8));
  let at=24;
  for(let index=0;index<packet[5];++index) {
    const header=at,record={state:packet[at],clock:packet[at+1],flags:packet[at+2],
      deadline:packet.readUInt32LE(at+16),duration:packet.readUInt32LE(at+20)};
    at+=48;
    for(const [slot,name]of ['provider','id','kind','main','footer','unit'].entries()) {
      const count=packet[header+36+slot];
      record[name]=packet.toString('utf8',at,at+count);at+=count;
      assert.ok(!record[name].includes('\ufffd'),'fixture text is whole UTF-8');
    }
    if(record.flags&2)at+=16+packet.readUInt16LE(header+44);
    records.push(record);
  }
  assert.equal(at,packet.length);
  return records;
}

test('grouped activity samples cover long clocks, competing content and compact states',()=>{
  const now=1793538300,plan=activityPresets({session:42,now});
  const names=['timer-running','timer-paused','glucose-numeric','elapsed',
    'timer-heading-banner','timer-timeline','timer-long','timer-24-hours',
    'timer-secondary','timer-paused-music','timer-large-text','timer-long-heading-banner',
    'clock-with-image','clock-long-footer','timer-dark','timer-due','timer-long-timeline'];
  assert.deepEqual(plan.presets.map(p=>p.name),names);
  for(const [index,preset]of plan.presets.entries()) {
    const packet=Buffer.from(preset.bytes.ACTIVITY_DATA),records=activityRecords(preset.bytes.ACTIVITY_DATA);
    assert.equal(packet[5],preset.activities.length);
    assert.equal(packet.readUInt32LE(12),42);assert.equal(packet.readUInt32LE(16),101+index);
    assert.equal(preset.integers.LARGE_TEXT,preset.name==='timer-large-text'?1:0);
    assert.equal(records[0].id,preset.activity.id);
    assert.equal(records[0].footer,activities.normalize(preset.activity).footer);
    if(preset.activity.kind==='timer')assert.ok(records[0].flags&8,'native timer reports remain unconfirmed');
    let at=6;const statuses=[];
    for(let slot=0;slot<5;slot++) {
      const bytes=preset.bytes.COMPLICATION_DATA;
      statuses.push(bytes[at+1]);
      at+=56+bytes[at+2]+bytes[at+22]+bytes[at+23]+
        (bytes[at]&2?16+(bytes[at+14]|bytes[at+15]<<8):0);
    }
    assert.deepEqual(statuses,[2,2,0,0,0]);
    assert.deepEqual(preset.preserve,preset.state==='timeline'?[{x:0,y:171,width:200,height:57}]:[]);
  }
  const find=name=>plan.presets.find(p=>p.name===name);
  const clock=name=>activities.formatClock(activities.clockValue(find(name).activity,now));
  assert.equal(clock('timer-running'),'08:42');
  assert.equal(clock('timer-long'),'1:14:56');
  assert.equal(clock('timer-24-hours'),'24:00:00');
  assert.equal(clock('timer-due'),'00:00');
  const two=activityRecords(find('timer-secondary').bytes.ACTIVITY_DATA);
  assert.equal(two.length,2);assert.notEqual(two[0].id,two[1].id);
  assert.equal(two[1].deadline-now,1040);assert.equal(two[1].footer,'20 min timer');
  assert.equal(find('timer-paused-music').activity.state,'paused');
  assert.equal(find('timer-paused-music').state,'music');
  assert.ok(activityRecords(find('clock-with-image').bytes.ACTIVITY_DATA)[0].flags&2);
  assert.ok(find('clock-long-footer').activity.footer.length>50);
  assert.equal(find('timer-dark').theme,'dark');assert.equal(find('timer-dark').integers.BACKGROUND_COLOR,0);
  assert.equal(find('timer-long-heading-banner').state,'notification-summary');
  assert.equal(find('timer-long-timeline').state,'timeline');
  const glucose=Buffer.from(find('glucose-numeric').bytes.ACTIVITY_DATA);
  assert.ok(glucose[26]&4);assert.equal(glucose.readDoubleLE(52),6.2);
  assert.equal(find('elapsed').activity.clock,'elapsed');
  const later=activityPresets({session:42,now,sequence:117});
  assert.equal(Buffer.from(later.presets[0].bytes.ACTIVITY_DATA).readUInt32LE(16),118);
});
