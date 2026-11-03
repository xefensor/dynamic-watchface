'use strict';
const assert=require('node:assert/strict');
const {test}=require('node:test');
const library=require('pebble-activities');
const fixtures=require('../scripts/native-preview-fixtures');
const N=1793700000,S=7;
function records(value){
  const bytes=Buffer.from(value);assert.equal(bytes.toString('ascii',0,4),'ACT3');assert.equal(bytes[4],3);
  assert.equal(bytes.length,bytes.readUInt16LE(6));assert.ok(bytes.length<=2672);
  assert.equal(library.wire.crc16(Array.from(bytes)),bytes.readUInt16LE(8));
  let at=24,result=[];
  for(let i=0;i<bytes[5];i++){
    const length=bytes.readUInt16LE(at),idLength=bytes[at+16],pool=at+104+36+idLength;
    const text=(offset,len)=>bytes.toString('utf8',pool+offset,pool+offset+len);
    assert.ok(length>=104&&at+length<=bytes.length);assert.ok(bytes.readUInt16LE(at+22)<=304);
    result.push({presentation:bytes[at+88],identity:bytes[at+89],compact:bytes[at+90],directions:bytes[at+91],
      mainType:bytes[at+32],supportType:bytes[at+60],state:bytes[at+2],validUntil:bytes.readUInt32LE(at+12),
      main:text(bytes.readUInt16LE(at+52),bytes.readUInt16LE(at+54)),
      support:text(bytes.readUInt16LE(at+80),bytes.readUInt16LE(at+82)),
      caption:text(bytes.readUInt16LE(at+24),bytes.readUInt16LE(at+26))});
    at+=length;
  }
  assert.equal(at,bytes.length);return result;
}
test('twenty native settings states use production ACT3 encoders and retain header controls',()=>{
  const plan=fixtures.fixtures({now:N,session:S});assert.equal(plan.presets.length,20);assert.equal(plan.activityWireVersion,3);
  for(const preset of plan.presets){assert.equal(preset.integers.ANIMATIONS,0);assert.equal(preset.integers.REQUEST_CONTEXT,S);const cards=records(preset.bytes.ACTIVITY_DATA);
    if(preset.state==='music'||preset.state==='notification-summary'){assert.equal(cards[0].presentation,2);assert.equal(cards[0].support,'Justice, Tame Impala');}
  }
});
test('approved normal and compact activity combinations have light and dark native fixtures',()=>{
  const plan=fixtures.summaryPresets({now:N,session:S});assert.equal(plan.presets.length,36);
  const names=['timer-media','media-timer','workout-timer','navigation-media','recording-delivery','download-recording','message-timer','media-alone','timer-alone','long-timer','long-artist','delivery-text'];
  for(const name of names)for(const theme of ['light','dark'])assert.ok(plan.presets.some(x=>x.name===name+'-'+theme));
  for(const preset of plan.presets){assert.equal(records(preset.bytes.ACTIVITY_DATA).length,preset.activities.length);assert.ok(preset.activities.length<=2);}
  const find=name=>records(plan.presets.find(x=>x.name===name+'-light').bytes.ACTIVITY_DATA);
  assert.deepEqual(find('timer-media').map(x=>x.presentation),[3,2]);assert.deepEqual(find('media-timer').map(x=>x.presentation),[2,3]);
  assert.equal(find('navigation-media')[0].directions&7,2);assert.equal(find('download-recording')[0].presentation,5);
  assert.equal(find('compact-detail-clock')[1].compact,1);assert.equal(find('compact-detail-clock')[1].supportType,3);
  assert.equal(find('local-promotion')[0].validUntil,N+2);assert.equal(find('secondary-unfit-value')[1].mainType,2);assert.equal(find('media-alone')[0].caption,'The Midnight');
});
