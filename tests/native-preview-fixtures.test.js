'use strict';
const assert=require('node:assert/strict');
const {test}=require('node:test');
const fs=require('node:fs');
const os=require('node:os');
const path=require('node:path');
const {spawnSync}=require('node:child_process');
const {fixtures,activityPresets}=require('../scripts/native-preview-fixtures');
const core=require('pebble-watchface-core').wire;
const library=require('pebble-activities');
const activities=library.view;
const legacy=library.wire;

function activityRecords(packetBytes) {
  const packet=Buffer.from(packetBytes),records=[];
  assert.equal(packet.toString('ascii',0,4),'ACT2');
  assert.equal(packet[4],2);
  assert.ok(packet.length<=2672);
  assert.equal(packet.readUInt16LE(6),packet.length);
  assert.equal(legacy.crc16(packetBytes),packet.readUInt16LE(8));
  assert.equal(packet.readUInt16LE(10),0);
  let at=24;
  for(let index=0;index<packet[5];++index) {
    const header=at,length=packet.readUInt16LE(at),idLength=packet[at+16];
    const poolAt=header+88+36+idLength,poolLength=packet.readUInt16LE(header+22);
    assert.ok(length>=88&&header+length<=packet.length);
    assert.ok(idLength>0&&idLength<=64);
    assert.ok(poolLength>0&&poolLength<=320);
    const record={state:packet[header+2],flags:packet[header+3],
      revision:packet.readUInt32LE(header+4),observedAt:packet.readUInt32LE(header+8),
      validUntil:packet.readUInt32LE(header+12),meterKind:packet[header+19],
      meter:packet.readUInt16LE(header+28),meterSource:packet[header+30],
      width:packet[header+17],height:packet[header+18],pixelBytes:packet.readUInt16LE(header+20),
      provider:packet.toString('utf8',header+88,header+124),
      id:packet.toString('utf8',header+124,header+124+idLength)};
    assert.match(record.provider,/^[0-9a-f]{8}(?:-[0-9a-f]{4}){3}-[0-9a-f]{12}$/);
    assert.equal(packet[header+31],0);
    function text(offset,length) {
      assert.ok(offset+length<poolLength,'reference and NUL fit the bounded string pool');
      assert.equal(packet[poolAt+offset+length],0);
      const bytes=packet.subarray(poolAt+offset,poolAt+offset+length);
      const result=bytes.toString('utf8');
      assert.ok(!result.includes('\ufffd')&&!result.includes('\0'));
      assert.equal(Buffer.byteLength(result,'utf8'),length);
      return result;
    }
    function datum(offset) {
      if(!packet[offset]) {
        assert.ok(packet.subarray(offset,offset+28).every(value=>value===0));
        return null;
      }
      const item={type:packet[offset],flags:packet[offset+1],observedAt:packet.readUInt32LE(offset+4),
        validUntil:packet.readUInt32LE(offset+8),base:packet.readUInt32LE(offset+12),
        anchor:packet.readUInt32LE(offset+16),
        text:text(packet.readUInt16LE(offset+20),packet.readUInt16LE(offset+22)),
        label:text(packet.readUInt16LE(offset+24),packet.readUInt16LE(offset+26))};
      assert.ok(item.type>=1&&item.type<=3);
      assert.equal(packet.readUInt16LE(offset+2),0);
      assert.ok(item.label);
      assert.ok(item.observedAt<=record.observedAt&&item.validUntil<=record.validUntil);
      if(item.type!==3)assert.ok(item.text);
      return item;
    }
    record.caption=text(packet.readUInt16LE(header+24),packet.readUInt16LE(header+26));
    record.main=datum(header+32);record.support=datum(header+60);
    assert.ok(record.main);
    if(record.flags&8) {
      assert.ok(record.width>0&&record.width<=40&&record.height>0&&record.height<=40);
      assert.equal(record.pixelBytes,Math.ceil(record.width/2)*record.height);
      assert.equal(length,88+36+idLength+poolLength+16+record.pixelBytes);
    } else {
      assert.equal(record.width+record.height+record.pixelBytes,0);
      assert.equal(length,88+36+idLength+poolLength);
    }
    if(record.meterKind)assert.equal((record.meterSource?record.support:record.main).type,2);
    records.push(record);at+=length;
  }
  assert.equal(at,packet.length);
  return records;
}

test('native capture fixtures preserve all 20 state, format and theme presets using ACT2',()=>{
  const plan=fixtures({session:42,now:1793538299});
  assert.equal(plan.activityWireVersion,2);
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
    const cards=activityRecords(preset.bytes.ACTIVITY_DATA);
    assert.equal(cards.length,preset.state==='music'||preset.state==='notification-summary'?1:0);
    if(cards.length)assert.equal(cards[0].caption,'Music');
    assert.equal(packet.readUInt32LE(12),42);assert.equal(packet.readUInt32LE(16),index+1);
    assert.deepEqual(preset.preserve,preset.state==='timeline'?[{x:0,y:171,width:200,height:57}]:
      preset.state==='music'||preset.state==='notification-summary'?
        [{x:10,y:preset.state==='music'?92:116,width:40,height:40}]:[]);
  }
});

test('capture plans require a current native session and UTC observation time',()=>{
  for(const options of [{session:1,now:100},{session:42,now:0},{session:'42',now:100}])
    assert.throws(()=>fixtures(options));
});

test('legacy activity cases and new two-item cases use bounded immutable source metadata',()=>{
  const now=1793538300,plan=activityPresets({session:42,now});
  const names=['timer-running','timer-paused','glucose-numeric','elapsed',
    'timer-heading-banner','timer-timeline','timer-long','timer-24-hours',
    'timer-secondary','timer-paused-music','timer-large-text','timer-long-heading-banner',
    'clock-with-image','clock-long-footer','timer-dark','timer-due','timer-long-timeline',
    'value-eta-artwork-meter','two-independent-clocks','paused-with-running-wall-clock',
    'cached-public-value-age','support-expiry-before','support-expiry-after','completed-result',
    'clock-primary-clock-secondary','text-primary-running-secondary','unicode-value-body-fallback',
    'value-eta-artwork-meter-dark','timer-one-minute','matching-item-provenance',
    'different-item-provenance','completed-result-music','ongoing-result-music',
    'timer-acknowledged-idle','timer-acknowledged-music',
    'timer-primary-music-secondary','music-primary-timer-secondary','two-identified-timers',
    'delivery-primary-timer-secondary','support-yields-secondary','secondary-long-clock',
    'secondary-cached','small-scalar-icon-spacing','long-provider-identity'];
  assert.deepEqual(plan.presets.map(p=>p.name),names);
  for(const [index,preset]of plan.presets.entries()) {
    const packet=Buffer.from(preset.bytes.ACTIVITY_DATA),records=activityRecords(preset.bytes.ACTIVITY_DATA);
    assert.equal(packet[5],preset.activities.length);
    assert.equal(packet.readUInt32LE(12),42);assert.equal(packet.readUInt32LE(16),101+index);
    assert.equal(preset.integers.LARGE_TEXT,preset.name==='timer-large-text'?1:0);
    assert.equal(records[0].id,preset.activity.id);
    assert.equal(records[0].caption,preset.activity.caption);
    assert.equal(records[0].observedAt,preset.activity.observedAt);
    if(preset.activity.unconfirmed)assert.ok(records[0].flags&2);
    let at=6;const statuses=[];
    for(let slot=0;slot<5;slot++) {
      const bytes=preset.bytes.COMPLICATION_DATA;
      statuses.push(bytes[at+1]);
      at+=56+bytes[at+2]+bytes[at+22]+bytes[at+23]+
        (bytes[at]&2?16+(bytes[at+14]|bytes[at+15]<<8):0);
    }
    assert.deepEqual(statuses,[2,2,0,0,0]);
  }
  const find=name=>plan.presets.find(p=>p.name===name);
  const record=name=>activityRecords(find(name).bytes.ACTIVITY_DATA)[0];
  assert.equal(record('timer-running').main.base,522);
  assert.equal(record('timer-running').main.label,record('timer-running').caption);
  assert.equal(record('timer-running').support,null);
  assert.equal(record('timer-one-minute').main.base,36);
  assert.equal(record('timer-one-minute').caption,'Timer');
  const matching=record('matching-item-provenance'),different=record('different-item-provenance');
  assert.equal(matching.main.observedAt,matching.support.observedAt);
  assert.notEqual(different.main.observedAt,different.support.observedAt);
  assert.equal(record('timer-acknowledged-idle').state,3);
  assert.equal(record('timer-acknowledged-idle').flags&4,0);
  assert.equal(record('timer-acknowledged-music').flags&4,0);
  assert.equal(record('timer-long').main.base,4496);
  assert.equal(record('timer-24-hours').main.base,86400);
  assert.equal(record('timer-due').state,1);
  assert.equal(record('timer-due').main.base,0);
  const pair=activityRecords(find('timer-secondary').bytes.ACTIVITY_DATA);
  assert.equal(pair.length,2);assert.notEqual(pair[0].id,pair[1].id);
  assert.equal(pair[1].main.base,1040);assert.equal(pair[1].caption,'Timer');
  assert.ok(record('clock-with-image').flags&8);
  assert.equal(find('clock-long-footer').activity.caption,'Timer');
  assert.equal(find('timer-dark').theme,'dark');
  assert.equal(find('timer-long-heading-banner').state,'notification-summary');
  assert.equal(find('timer-long-timeline').state,'timeline');
  assert.equal(record('glucose-numeric').main.text,'6.2 mmol/L');
  const smallScalar=record('small-scalar-icon-spacing');
  assert.equal(smallScalar.main.type,2);assert.equal(smallScalar.main.text,'123456789012345678');
  assert.equal(smallScalar.main.label,'Items');assert.equal(smallScalar.width,40);
  assert.ok(record('elapsed').main.flags&1);
  const meter=record('value-eta-artwork-meter');
  assert.equal(meter.main.type,2);assert.equal(meter.main.text,'72%');
  assert.equal(meter.support.type,3);assert.equal(meter.support.label,'ETA');
  assert.equal(meter.width,40);assert.equal(meter.pixelBytes,800);
  assert.equal(meter.meterKind,1);assert.ok(Math.abs(meter.meter/65535-.72)<1/65535);
  const two=record('two-independent-clocks');
  assert.ok(two.main.flags&2);assert.ok(two.support.flags&2);assert.ok(two.support.flags&1);
  const paused=record('paused-with-running-wall-clock');
  assert.equal(paused.state,2);assert.equal(paused.main.flags&2,0);assert.ok(paused.support.flags&2);
  const cached=record('cached-public-value-age');
  assert.ok(cached.flags&2);assert.ok(cached.main.flags&4);assert.equal(now-cached.main.observedAt,300);
  const result=record('completed-result');
  assert.equal(result.state,3);assert.ok(result.flags&4);assert.equal(result.validUntil,now+60);
  assert.equal(result.main.text,'Finished');
  assert.equal(record('unicode-value-body-fallback').main.text,'6.2 mmol/L');
  assert.equal(record('unicode-value-body-fallback').caption,'Glucose');
  const textPrimary=activityRecords(find('text-primary-running-secondary').bytes.ACTIVITY_DATA);
  assert.equal(textPrimary[0].main.type,1);assert.equal(textPrimary[1].main.type,3);
  assert.ok(find('text-primary-running-secondary').verifyClockTick);
  assert.ok(find('paused-with-running-wall-clock').verifyClockTick);
  const before=find('support-expiry-before'),after=find('support-expiry-after');
  assert.deepEqual(before.bytes.ACTIVITY_DATA.slice(24),after.bytes.ACTIVITY_DATA.slice(24));
  assert.ok(after.captureDelaySeconds>record('support-expiry-after').support.validUntil-now);
  assert.deepEqual(before.expectInk,after.expectBlank);
  const later=activityPresets({session:42,now,sequence:128});
  assert.equal(Buffer.from(later.presets[0].bytes.ACTIVITY_DATA).readUInt32LE(16),129);
});

test('installed production C decoder accepts every generated ACT2 capture frame',()=>{
  const root=path.resolve(__dirname,'..'),scratch=fs.mkdtempSync(path.join(os.tmpdir(),'dynamic-act2-fixtures-'));
  try {
    const source=path.join(scratch,'decode.c'),binary=path.join(scratch,'decode');
    fs.writeFileSync(source,`#include "pebble-activity-view.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
  assert(argc>2);const uint32_t now=(uint32_t)strtoul(argv[1],NULL,10);
  PavState state;pav_state_init(&state,42,true);
  for(int i=2;i<argc;++i) {
    FILE *f=fopen(argv[i],"rb");assert(f);
    uint8_t bytes[PAV_MAX_PACKET_BYTES+1];size_t length=fread(bytes,1,sizeof(bytes),f);fclose(f);
    PavSnapshot snapshot;assert(pav_decode(bytes,length,NULL));assert(pav_decode(bytes,length,&snapshot));
    if(!pav_state_receive(&state,bytes,length,now)){fprintf(stderr,"State rejected %s\\n",argv[i]);return 1;}
    if(!snapshot.count)continue;
    const PavCard *card=&snapshot.items[0];
    if(strstr(argv[i],"paused-with-running-wall-clock")) {
      assert(card->source_state==PAV_PAUSED);
      assert(pav_clock_value(card,&card->main,now+2,true).seconds==1458);
      assert(pav_clock_value(card,&card->support,now+2,true).seconds==1802);
    }
    if(strstr(argv[i],"support-expiry-before")) {
      assert(pav_datum_visible(card,&card->support,now,true));assert(pav_meter_visible(card,now,true));
    }
    if(strstr(argv[i],"support-expiry-after")) {
      assert(pav_card_visible(card,now+4,true));assert(!pav_datum_visible(card,&card->support,now+4,true));
      assert(!pav_meter_visible(card,now+4,true));
    }
    if(strstr(argv[i],"cached-public-value-age"))assert(pav_card_visible(card,now,false));
    if(strstr(argv[i],"completed-result")) {
      assert(pav_card_visible(card,now,true));assert(!pav_card_visible(card,now+61,true));
    }
  }
  puts("Production C decoded ACT2 native preview frames and validated clock/expiry/result cases");return 0;
}\n`);
    const compile=spawnSync(process.env.CC||'cc',['-std=c99','-Wall','-Wextra','-Werror','-pedantic',
      '-I',path.join(root,'node_modules/pebble-activities/include'),source,
      path.join(root,'node_modules/pebble-activities/src/c/activity-view.c'),'-o',binary],{stdio:'inherit'});
    assert.equal(compile.status,0);
    const now=1793538300,plans=[fixtures({session:42,now}),activityPresets({session:42,now})],files=[];
    for(const plan of plans)for(const preset of plan.presets) {
      const filename=path.join(scratch,preset.name+'.bin');fs.writeFileSync(filename,Buffer.from(preset.bytes.ACTIVITY_DATA));files.push(filename);
    }
    assert.equal(spawnSync(binary,[String(now),...files],{stdio:'inherit'}).status,0);
  } finally {fs.rmSync(scratch,{recursive:true,force:true});}
});
