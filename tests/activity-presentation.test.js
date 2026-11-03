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
test('projection adds the provider identity/icon and preserves independent role, source and privacy metadata',()=>{
  const source=card(),before=json(source),projected=presentation.project(source);
  assert.equal(projected.caption,'Workout App');assert.equal(projected.main.label,'Rest');assert.equal(projected.support.label,'Elapsed');
  assert.equal(projected.support.observedAt,N-20);assert.equal(projected.support.anchorAt,N-20);assert.equal(projected.support.validUntil,N+500);
  assert.equal(projected.revision,9);assert.equal(projected.observedAt,N);assert.equal(projected.private,false);assert.equal(projected.image.width,40);
  assert.deepEqual(source,before);assert.ok(library.view.normalize(projected));
});
test('unknown kinds have an honest fallback and no clock or text guessing',()=>{
  const unknown=presentation.project(card({providerName:'',kind:'custom'})),timer=presentation.project(card({providerName:'',kind:'timer'}));
  assert.equal(unknown.caption,'Activity');assert.notDeepEqual(unknown.image.pixels,timer.image.pixels);
  assert.equal(unknown.main.label,'Rest');assert.equal(timer.main.label,'Rest');
});
test('only known native Timer compact duplication is removed; provider-defined role labels survive',()=>{
  const input=card({provider:T,providerName:'Timer',kind:'timer',caption:'1 min timer',main:{type:'clock',label:'1 min timer',mode:'deadline',running:true,baseSeconds:60,anchorAt:N},support:null});
  assert.equal(presentation.project(input).main.label,'Timer');
  assert.equal(presentation.project(Object.assign({},input,{provider:P})).main.label,'1 min timer');
});
test('album art and essential main units survive projection and optional supporting data yields to the pool bound',()=>{
  const image={width:2,height:1,palette:[192,255],pixels:[0x10]},source=card({image,providerName:'A'.repeat(32),caption:'',main:{type:'value',label:'L'.repeat(64),text:'M'.repeat(190)+' m'},support:{type:'text',label:'Detail',text:'S'.repeat(50)}});
  // A valid full source fits before adding its app identity.
  source.main.text='M'.repeat(180)+' mmol/L';source.support.text='S'.repeat(45);
  assert.ok(library.view.normalize(source));const projected=presentation.project(source);
  assert.ok(projected);assert.match(projected.main.text,/ mmol\/L$/);assert.equal(projected.main.label.length,64);
  assert.deepEqual(projected.image,library.wire.image(image));assert.equal(projected.support,null);assert.ok(library.view.poolFor(projected).bytes.length<=320);
});
test('Music can occupy either role, obey hide, and deduplicate only its actual provider',()=>{
  const timer=card({provider:T,providerName:'Timer',kind:'timer',support:null}),music=card({kind:'music',selectionKey:'builtin:music',providerName:'Music',private:true,main:{type:'text',label:'Track',text:'Genesis'},support:null});
  const selected=presentation.select([timer],{order:['builtin:music',T]},music);assert.equal(selected[0].kind,'music');assert.equal(selected[1].kind,'timer');
  assert.equal(presentation.select([timer],{hidden:['builtin:music']},music).length,1);
  assert.equal(presentation.select([music],{},music).length,1);
});
test('dual-convention Music keeps its app UUID controls and cannot bypass an app hide through the adapter',()=>{
  const timer=card({provider:T,providerName:'Timer',kind:'timer',support:null});
  const canonical=card({id:'canonical',kind:'music',providerName:'Player',private:true,main:{type:'text',label:'Track',text:'Song'},support:null});
  const legacy=Object.assign({},canonical,{id:'legacy',selectionKey:'builtin:music',providerName:'Music'}),before=JSON.stringify(canonical);
  assert.deepEqual(presentation.select([timer,canonical],{order:[P,T]},legacy).map(x=>x.id),['canonical','run']);
  assert.deepEqual(presentation.select([timer,canonical],{order:['builtin:music',T,P]},legacy).map(x=>x.id),['run','canonical']);
  assert.deepEqual(presentation.select([canonical],{hidden:[P]},legacy),[]);
  assert.deepEqual(presentation.select([],{hidden:[P]},legacy),[]);
  assert.deepEqual(presentation.select([canonical],{hidden:['builtin:music']},legacy).map(x=>x.id),['canonical']);
  assert.equal(JSON.stringify(canonical),before);
});
test('simple API projection follows installed app identity without inventing visible field labels',()=>{
  const raw={provider:P,id:'simple',appName:'Source name',revision:1,observedAt:N,validUntil:N+600,
    content:library.activity.countdown({deadline:N+60}),detail:'Collect at desk',private:false};
  const compiled=library.activity.toView(raw),before=JSON.stringify(compiled);
  const projected=presentation.project(Object.assign({},compiled,{providerName:'Installed name'}));
  assert.equal(projected.caption,'Installed name');assert.equal(projected.main.label,'Installed name');
  assert.equal(projected.support.label,'Installed name');assert.equal(projected.support.text,'Collect at desk');
  assert.equal(JSON.stringify(compiled),before);
  const labeled=library.activity.toView(Object.assign({},raw,{content:library.activity.countdown({deadline:N+60,label:'Rest'})}));
  assert.equal(presentation.project(labeled).main.label,'Rest');
});
test('compatibility Music projects useful metadata without Track/Artist placeholders',()=>{
  const raw=card({kind:'music',private:true,main:{type:'text',label:'Track',text:'Long title'},
    support:{type:'text',label:'Artist',text:'?'}}),before=JSON.stringify(raw);
  const result=presentation.project(raw);
  assert.equal(result.main.label,result.caption);assert.equal(result.support,null);
  assert.equal(JSON.stringify(raw),before);
  const known=presentation.project(Object.assign({},raw,{support:{type:'text',label:'Artist',text:'Justice'}}));
  assert.equal(known.support.text,'Justice');assert.equal(known.support.label,known.caption);
});
