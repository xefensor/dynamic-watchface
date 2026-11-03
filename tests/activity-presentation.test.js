'use strict';
const assert=require('node:assert/strict');
const {test}=require('node:test');
const library=require('pebble-activities');
const presentation=require('../src/pkjs/activity-presentation');
const T='50e9ef12-a5ab-4369-a3b5-3447bfba5151',P='c9e7cb18-eaf7-4b30-9b09-9a8fae187801',N=1700000000;
const json=value=>JSON.parse(JSON.stringify(value));
const card=(extra={})=>Object.assign({provider:P,providerName:'Workout App',id:'run',kind:'workout',state:'active',revision:9,
  observedAt:N,validUntil:N+600,private:false,caption:'Intervals',main:{type:'clock',label:'Rest',mode:'deadline',running:true,baseSeconds:80,anchorAt:N,retain:true},
  support:{type:'clock',label:'Elapsed',mode:'elapsed',running:true,baseSeconds:100,anchorAt:N-20,observedAt:N-20,validUntil:N+500,retain:true}},extra);
const source=(record,extra={})=>({provider:P,record,revision:3,observedAt:N,validUntil:N+600,private:false,displayName:'Example',...extra});
test('projection preserves instance context, independent clocks and authoritative metadata',()=>{
  const raw=card(),before=json(raw),projected=presentation.project(raw);
  assert.equal(projected.caption,'Intervals');assert.equal(projected.main.label,'Rest');assert.equal(projected.support.label,'Elapsed');
  assert.equal(projected.support.observedAt,N-20);assert.equal(projected.support.anchorAt,N-20);assert.equal(projected.support.validUntil,N+500);
  assert.equal(projected.revision,9);assert.equal(projected.observedAt,N);assert.equal(projected.private,false);
  assert.equal(projected.identitySymbol,7);assert.equal(projected.image.width,40);
  assert.deepEqual(raw,before);assert.ok(library.summaryView.normalize(projected));
});
test('unknown activity has no guessed timer identity or app destination',()=>{
  const unknown=presentation.project(card({kind:'custom'})),timer=presentation.project(card({provider:T,kind:'timer'}));
  assert.equal(unknown.identitySymbol,0);assert.equal(unknown.openApp,undefined);
  assert.equal(timer.identitySymbol,1);assert.deepEqual(timer.openApp,{watchAppUuid:T});
});
test('declared identity symbols retain distinct generated glyphs including calls and messages',()=>{
  const images=['stopwatch','message','call','workout','recording','download','delivery'].map(icon=>{
    const projected=presentation.project({...source({id:'identity',type:'standard',icon,
      headline:{type:'text',text:'Current'}}),type:'standard'});
    assert.ok(projected.image.pixels.some(byte=>byte!==0));
    return JSON.stringify(projected.image.pixels);
  });
  assert.equal(new Set(images).size,images.length);
});
test('media is explicit and preserves artwork and artist without placeholder labels',()=>{
  const image={width:2,height:1,palette:[192,255],pixels:[0x10]},raw=card({kind:'music',private:true,image,
    main:{type:'text',label:'Track',text:'Long title'},support:{type:'text',label:'Artist',text:'Justice'}}),before=json(raw);
  const projected=presentation.project(raw);assert.equal(projected.presentation,2);assert.equal(projected.main.label,'');
  assert.equal(projected.support.label,'');assert.equal(projected.support.text,'Justice');assert.equal(projected.caption,'Justice');
  assert.deepEqual(projected.image,library.wire.image(image));assert.deepEqual(raw,before);
  const missing=presentation.project({...raw,support:{type:'text',label:'Artist',text:'?'}});
  assert.equal(missing.support,null);assert.equal(missing.caption,'');assert.notEqual(missing.main.label,'__dynamic_media__');
});
test('Music can occupy either role and follows Dynamic preferences',()=>{
  const timer=card({provider:T,kind:'timer',support:null}),music=card({kind:'music',selectionKey:'builtin:music',private:true,main:{type:'text',label:'Track',text:'Genesis'},support:null});
  const selected=presentation.select([timer],{order:['builtin:music',T]},music);assert.equal(selected[0].presentation,2);assert.equal(selected[1].kind,'timer');
  assert.equal(presentation.select([timer],{hidden:['builtin:music']},music).length,1);assert.equal(presentation.select([music],{},music).length,1);
});
test('dual-convention media retains exact provider controls and cannot bypass a provider hide',()=>{
  const timer=card({provider:T,kind:'timer',support:null});
  const canonical=card({id:'canonical',kind:'music',private:true,main:{type:'text',label:'Track',text:'Song'},support:null});
  const legacy={...canonical,id:'legacy',selectionKey:'builtin:music'};
  assert.deepEqual(presentation.select([timer,canonical],{order:[P,T]},legacy).map(x=>x.id),['canonical','run']);
  assert.deepEqual(presentation.select([canonical],{hidden:[P]},legacy),[]);
  assert.deepEqual(presentation.select([],{hidden:[P]},legacy),[]);
  assert.deepEqual(presentation.select([canonical],{hidden:['builtin:music']},legacy).map(x=>x.id),['canonical']);
});
test('schema4 selects an independent compact detail and preserves both typed datums',()=>{
  const raw=source({id:'run',type:'standard',name:'Evening run',icon:'workout',headline:{type:'value',value:5.24,unit:'km'},
    detail:{type:'clock',mode:'countdown',deadline:N+45,label:'Rest'},summary:'detail'});
  const projected=presentation.project({...raw,type:'standard'});assert.ok(projected);
  assert.equal(projected.compactItem,1);assert.equal(projected.main.type,'value');assert.equal(projected.support.type,'clock');
  assert.equal(projected.support.baseSeconds,45);assert.equal(projected.caption,'Evening run');
});
test('media visual token changes on an item replacement, not artwork, artist or lease',()=>{
  const raw=source({id:'player',type:'media',title:'Song',artist:'Artist',itemId:'one'}, {private:true});
  const a=presentation.project({...raw,type:'media'}),artist=presentation.project({...raw,type:'media',record:{...raw.record,artist:'New artist'},validUntil:N+900});
  assert.deepEqual(a.visualToken,artist.visualToken);
  const next=presentation.project({...raw,type:'media',record:{...raw.record,itemId:'two'}});assert.notDeepEqual(a.visualToken,next.visualToken);
});
test('only verified built-in media and Timer receive host-known app destinations',()=>{
  const music=card({provider:'00000000-0000-0000-0001-000000000003',kind:'music',private:true,main:{type:'text',label:'Track',text:'Song'},support:null});
  assert.deepEqual(presentation.project(music).openApp,{watchAppUuid:'1f03293d-47af-4f28-b960-f2b02a6dd757'});
  assert.equal(presentation.project({...music,provider:P}).openApp,undefined);
});

test('unconfirmed native Timer predictions keep original leases while unrelated uncertain sources yield',()=>{
  const timer=card({provider:T,kind:'timer',unconfirmed:true,support:null});
  assert.equal(presentation.project(timer).validUntil,N+600);assert.equal(presentation.project(timer).observedAt,N);
  assert.equal(presentation.project({...timer,provider:P}),null);
});

test('unprojectable earlier sources cannot starve a valid secondary',()=>{
  const stale=card({id:'stale',unconfirmed:true}),timer=card({provider:T,id:'timer',kind:'timer',support:null});
  const music=card({provider:'00000000-0000-0000-0001-000000000003',id:'music',kind:'music',selectionKey:'builtin:music',private:true,main:{type:'text',label:'',text:'Song'},support:null});
  assert.deepEqual(presentation.select([stale,timer,music],{}).map(x=>x.id),['timer','music']);
});
