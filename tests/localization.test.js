'use strict';
const assert=require('node:assert/strict');
const {test}=require('node:test');
const localization=require('../src/pkjs/localization');
const config=require('../src/pkjs/config');

function items(value) {
  if(Array.isArray(value))return value.flatMap(items);
  if(!value||typeof value!=='object')return [];
  return [value].concat(Object.values(value).flatMap(items));
}
function visible(value) {
  return items(value).flatMap(item=>[
    item.label,item.description,
    ['text','heading','submit'].includes(item.type)?item.defaultValue:undefined
  ]).filter(value=>typeof value==='string');
}
const languages=['en','cs','sk','de','fr','es','it','pt','nl','pl','da','sv','no','fi','hu','ro','tr'];
const statusKeys=['complicationsWaiting','complicationsStatus','complicationsDisabled','complicationsError',
  'contextWaiting','contextReady','contextError','stateOn','stateOff'];

test('watch language wins, with phone/English fallback and Norwegian aliases',()=>{
  assert.deepEqual(localization.languages,languages);
  for(const [watch,phone,want] of [
    ['CS_cz.UTF-8','de-DE','cs'],[' FR-ca ','cs-CZ','fr'],['pt_BR','en-US','pt'],
    ['nb_NO','en','no'],['NN-no@variant','en','no'],['no-NO','en','no'],
    ['unsupported','SK_sk.UTF-8','sk'],[undefined,'tr-TR','tr'],[null,null,'en'],
    ['', 'fi-FI','fi'],[{},['cs'],'en']
  ])assert.equal(localization.resolveLanguage(watch,phone),want);
});
for(const language of languages)test(language+' translates every active control and status',()=>{
  const english=visible(config);
  const translated=visible(localization.translateConfig(config,language));
  assert.equal(translated.length,english.length);
  assert.ok(english.length>25);
  translated.forEach((value,index)=>{
    assert.ok(value.length>0,english[index]);
    const sharedWord=english[index]==='Complications'&&language==='fr';
    if(language!=='en'&&!sharedWord)assert.notEqual(value,english[index],english[index]);
    assert.doesNotMatch(value,/Halftone|halftone/);
  });
  for(const key of statusKeys){
    assert.notEqual(localization.t(key,language),key);
    if(language!=='en')assert.notEqual(localization.t(key,language),localization.t(key,'en'));
  }
  assert.match(localization.t('complicationsStatus',language,{ready:0,configured:2}),/0\/2/);
  assert.match(localization.t('contextError',language,{error:'PLUGIN_UNAVAILABLE'}),/PLUGIN_UNAVAILABLE/);
});
test('translation preserves preference identifiers, defaults and schema while cloning',()=>{
  const before=JSON.stringify(config);
  const translated=localization.translateConfig(config,'de');
  const raw=items(config),local=items(translated);
  assert.equal(raw.length,local.length);
  raw.forEach((item,index)=>{
    assert.notEqual(item,local[index]);
    for(const key of ['messageKey','id','type'])assert.equal(item[key],local[index][key]);
    if(item.messageKey)assert.equal(item.defaultValue,local[index].defaultValue);
  });
  assert.equal(JSON.stringify(config),before);
  assert.deepEqual(localization.translateConfig(config,'unknown'),config);
  assert.equal(localization.t('constructor','cs'),'constructor');
  assert.equal(localization.t('untranslated future string','cs'),'untranslated future string');
  assert.equal(localization.t('Background color','cs'),'Barva pozadí');
});
test('borderless settings expose only applicable controls and preserve privacy defaults',()=>{
  const all=items(config);
  const find=key=>all.find(item=>item.messageKey===key);
  assert.equal(find('BACKGROUND_COLOR').defaultValue,'0x000000');
  assert.equal(find('TEXT_COLOR').defaultValue,'0xFFFFFF');
  assert.equal(find('TEXT_SHADOW').defaultValue,false);
  assert.equal(find('SHOW_MUSIC').defaultValue,true);
  assert.equal(find('SHOW_NOTIFICATIONS').defaultValue,false);
  assert.equal(find('NOTIFICATION_PREVIEW').defaultValue,false);
  for(const key of ['DEPTH_EFFECT','PANEL_TRANSITIONS','LAYOUT_STYLE','WEATHER_LOCATION','FOLLOW_PEBBLE_UNITS','USE_FAHRENHEIT']){
    assert.equal(find(key),undefined,key);
  }
  assert.ok(visible(config).some(value=>value.includes('slots 1 and 2')));
  assert.ok(visible(config).some(value=>value.includes('Timeline Peek')));
  assert.doesNotMatch(visible(config).join('\n'),/Halftone|halftone|dot transition|dithered|Optical effects/);
});
