const assert = require('node:assert/strict');
const {test} = require('node:test');
const vm = require('node:vm');
const fs = require('node:fs');
const source = fs.readFileSync(require.resolve('../src/pkjs/index.js'), 'utf8');
const packageInfo = require('../package.json');
const shared = require('pebble-watchface-core');
const keys = Object.fromEntries(packageInfo.pebble.messageKeys.map((key,i) => [key,10000+i]));

function fixture(settings = {}, locales = {}) {
  let watchLanguage = locales.watch;
  let saved = JSON.stringify(settings);
  const storage = Object.assign({},locales.storage || {});
  let options, contextOptions, activityOptions, activityConfigured=0, clay, fail = false, hold = false, configured = 0, refreshed = 0, contextConfigured = 0, timerId = 0;
  const deferred=[];
  const contextRefreshes=[], connections=[],activityRefreshes=[],activityConnections=[];
  const handlers = {}, deliveries = [], clockQueries = [], urls = [], timers = [];
  const status = {available:true,configured:0,ready:0,error:'',slots:[]};
  const contextStatus = {available:true,ready:false,error:''};
  vm.runInNewContext(source, {
    require: name => {
      if (name === 'message_keys') return keys;
      if (name === './config') return require('../src/pkjs/config');
      if (name === './settings-page') return require('../src/pkjs/settings-page');
      if (name === './settings-preview') return require('../src/pkjs/settings-preview');
      if (name === './localization') return require('../src/pkjs/localization');
      if (name === '@rebble/clay') return function(config,_custom,opts) {
        assert.equal(opts.autoHandleEvents,false);
        this.config=JSON.parse(JSON.stringify(config));
        this.generateUrl=()=> 'data:text/html,settings';
        this.getSettings=response=> {
          const raw=JSON.parse(response);
          const flat=Object.fromEntries(Object.entries(raw).map(([key,value])=>
            [key,value && typeof value==='object'?value.value:value]));
          saved=JSON.stringify(flat);
          return Object.fromEntries(Object.entries(flat).map(([key,value])=>
            [keys[key]===undefined?key:keys[key],typeof value==='boolean'?Number(value):value]));
        };
        clay=this;
      };
      if (name === 'pebble-watchface-core/readings') return {
        createComplications: received=> {
          options=received;
          return {refresh:()=>refreshed++,configure:()=>configured++,status:()=>status};
        },
        createMessageQueue: shared.createMessageQueue
      };
      if (name === 'pebble-activities') return {
        createActivities: received=> {
          activityOptions=received;
          return {refresh:(...args)=>activityRefreshes.push(args),configure:()=>activityConfigured++,
            connectionChanged:(...args)=>activityConnections.push(args),status:()=>({available:true,ready:false,count:0,error:''})};
        },
        createContext: received=> {
          contextOptions=received;
          return {refresh:value=>contextRefreshes.push(value),configure:()=>contextConfigured++,
            connectionChanged:(value,token)=>connections.push({value,token}),status:()=>contextStatus};
        }
      };
      throw Error('Unexpected dependency '+name);
    },
    Pebble: {
      getActiveWatchInfo:()=> {if(locales.throwWatchInfo)throw Error('Disconnected');return {language:watchLanguage};},
      addEventListener:(event,fn)=> {handlers[event]=fn;},
      sendAppMessage:(payload,ok,error)=> {if(payload[keys.CLOCK_FORMAT]===255)clockQueries.push(payload);else deliveries.push(payload);if(hold){deferred.push({ok,error});}else if(fail){if(error)error();}else if(ok)ok();},
      openURL:url=>urls.push(url)
    },
    navigator:{language:locales.phone || 'en-US'},
    localStorage:{getItem:key=>key==='clay-settings'?saved:storage[key] || null,
      setItem:(key,value)=>{if(key==='clay-settings')saved=value;else storage[key]=value;}},
    setTimeout:(fn,delay)=>{const id=++timerId;timers.push({fn,delay,id});return id;},
    clearTimeout:id=>{const at=timers.findIndex(timer=>timer.id===id);if(at>=0)timers.splice(at,1);},
    console:{log:()=>{}}
  });
  return {handlers,deliveries,clockQueries,urls,status,contextStatus,clay,options,contextOptions,activityOptions,timers,contextRefreshes,connections,activityRefreshes,activityConnections,activityConfigured:()=>activityConfigured,
    refreshes:()=>refreshed,configured:()=>configured,settings:()=>JSON.parse(saved),
    contextConfigured:()=>contextConfigured,storage,
    fail:value=>{fail=value;},hold:value=>{hold=value;},deferred,watchLanguage:value=>{watchLanguage=value;}};
}
const allItems=f=>f.clay.config.flatMap(section=>[section].concat(section.items || []));
const item=(f,id)=>allItems(f).find(x=>x.id===id);
const named=(f,key)=>allItems(f).find(x=>x.messageKey===key);

test('ready and named/numeric complication requests refresh the generic coordinator',()=>{
  const f=fixture();f.handlers.ready();
  f.handlers.appmessage({payload:{REQUEST_COMPLICATIONS:1}});
  f.handlers.appmessage({payload:{10022:1}});
  f.handlers.appmessage({payload:{REQUEST_WEATHER:1,USE_FAHRENHEIT:1}});
  f.handlers.appmessage({payload:{TEXT_COLOR:123}});f.handlers.appmessage({});
  assert.equal(f.refreshes(),3);
});

test('the coordinator receives saved clock preference and active-watch language',()=>{
  for(const previous of [{USE_12_HOUR:true},{10007:true}]) {
    const f=fixture(previous,{watch:'cs-CZ',phone:'de-DE'});
    assert.equal(f.options.getSettings().use12Hour,true);
    assert.equal(f.options.getSettings().language,'cs');
    assert.equal(f.options.localizeBoolean(true),'Zapnuto');
    assert.equal(f.options.localizeBoolean(false),'Vypnuto');
    f.handlers.ready();assert.deepEqual(f.settings(),previous,'opening the face does not rewrite visual preferences');
  }
});

test('obsolete layout choices are ignored while unrelated appearance settings survive',()=>{
  const f=fixture();
  for(const value of ['1','2','0',2]) {
    f.handlers.webviewclosed({response:JSON.stringify({LAYOUT_STYLE:{value},TEXT_COLOR:{value:0},LARGE_TEXT:{value:true}})});
    const payload=f.deliveries.at(-1);
    assert.equal(payload[keys.LAYOUT_STYLE],undefined);
    assert.equal(payload[keys.TEXT_COLOR],0);
    assert.equal(payload[keys.LARGE_TEXT],undefined);
    assert.equal(f.settings().LAYOUT_STYLE,value);
  }
  assert.equal(f.configured(),4);
});

test('one text layout ignores saved and incoming legacy text-size choices',()=>{
  for(const field of ['LARGE_TEXT',String(keys.LARGE_TEXT)])for(const value of [false,0,true,1]) {
    const saved={[field]:value,TEXT_COLOR:123};
    const f=fixture(saved);
    f.handlers.showConfiguration();
    assert.equal(named(f,'LARGE_TEXT'),undefined);
    assert.deepEqual(f.settings(),saved,'opening settings need not rewrite old phone storage');
    f.handlers.webviewclosed({response:JSON.stringify({[field]:{value},TEXT_COLOR:456})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.TEXT_COLOR)]);
    assert.equal(f.deliveries[0][keys.TEXT_COLOR],456);
    f.handlers.webviewclosed({response:JSON.stringify({[field]:{value}})});
    assert.equal(f.deliveries.length,1,'a retired size-only save must not send a watch packet');
  }
});

test('queued legacy text-size settings are inert and cannot leave a false pending-save status',()=>{
  for(const mixed of [false,true]) {
    const f=fixture();
    const payload={[keys.LARGE_TEXT]:1,...(mixed?{[keys.TEXT_COLOR]:123}:{})};
    f.storage['dynamic-settings-delivery-v1']=JSON.stringify({id:'old-size',state:'pending',payload});
    f.handlers.ready();
    assert.ok(f.deliveries.every(value=>value[keys.LARGE_TEXT]===undefined));
    assert.equal(f.deliveries.some(value=>value[keys.TEXT_COLOR]===123),mixed);
    f.handlers.showConfiguration();
    const metadata=JSON.parse(item(f,'dynamic-settings-data').defaultValue.replace(/&quot;/g,'"'));
    assert.equal(metadata.save.state,mixed?'saved':'none');
  }
});

test('invalid layouts and unsupported fields cannot override unrelated visual settings',()=>{
  const f=fixture();
  for(const value of [-1,3,1.5,null,'','01','Classic',{},[]]) {
    f.handlers.webviewclosed({response:JSON.stringify({LAYOUT_STYLE:{value},TEXT_COLOR:{value:123},COMPLICATION_DATA:{value:'bad'}})});
    const payload=f.deliveries.at(-1);
    assert.equal(payload[keys.LAYOUT_STYLE],undefined);
    assert.equal(payload[keys.COMPLICATION_DATA],undefined);
    assert.equal(payload[keys.TEXT_COLOR],123);
  }
});

test('obsolete weather/unit controls never reach the new complication-only watch UI',()=>{
  const f=fixture({FOLLOW_PEBBLE_UNITS:false,USE_FAHRENHEIT:true,WEATHER_LOCATION:'home'});
  f.handlers.webviewclosed({response:JSON.stringify({FOLLOW_PEBBLE_UNITS:true,USE_FAHRENHEIT:true,
    WEATHER_LOCATION:'office',SHOW_DAILY_RANGE:true,SHOW_RAIN_CHANCE:true,SHOW_WEATHER_ICON:false,
    TEXT_COLOR:456,USE_12_HOUR:true})});
  assert.deepEqual(Object.keys(f.deliveries[0]).sort(),[String(keys.TEXT_COLOR),String(keys.USE_12_HOUR)].sort());
  assert.equal(f.deliveries[0][keys.USE_12_HOUR],1);
  assert.equal(f.configured(),1);
});

test('boolean settings preserve explicit off and reject malformed values',()=>{
  const f=fixture();
  f.handlers.webviewclosed({response:JSON.stringify({DEPTH_EFFECT:false,TEXT_SHADOW:true,LARGE_TEXT:'false',USE_12_HOUR:2,BACKGROUND_COLOR:-1,TEXT_COLOR:0x1000000,SHOW_STATUS:false})});
  assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.SHOW_STATUS)]);
  assert.equal(f.deliveries[0][keys.DEPTH_EFFECT],undefined);assert.equal(f.deliveries[0][keys.TEXT_SHADOW],undefined);
  assert.equal(f.deliveries[0][keys.SHOW_STATUS],0);
});

test('cancelled or invalid settings do nothing, empty valid settings reconfigure only',()=>{
  const f=fixture();f.handlers.webviewclosed({});f.handlers.webviewclosed({response:'bad json'});
  assert.equal(f.deliveries.length,0);assert.equal(f.configured(),0);
  f.handlers.webviewclosed({response:'{}'});assert.equal(f.deliveries.length,0);assert.equal(f.configured(),1);
});

test('settings show complication counts/errors and no hard-coded weather picker',()=>{
  const f=fixture();Object.assign(f.status,{configured:2,ready:1,error:'PERMISSION_DENIED'});
  f.status.slots=[{slot:2,category:'weather',item:'location',error:'PERMISSION_DENIED',message:'Location permission required'}];
  f.handlers.showConfiguration();
  assert.ok(item(f,'complications-status').defaultValue.startsWith(
    'Dynamic ' + packageInfo.version + '. '));
  assert.match(item(f,'complications-status').defaultValue,/1\/2 ready/);
  assert.match(item(f,'complications-status').defaultValue,/Complication error: #2 · weather\/location · PERMISSION_DENIED · Location permission required/);
  assert.equal(named(f,'WEATHER_LOCATION'),undefined);
  assert.equal(named(f,'SHOW_DAILY_RANGE'),undefined);
  assert.deepEqual(f.urls,['data:text/html,settings']);
});

test('provider error messages are displayed as literal text in Clay settings',()=>{
  const f=fixture();Object.assign(f.status,{configured:1,ready:0,error:'PLUGIN_UNAVAILABLE'});
  f.status.slots=[{slot:1,category:'weather',item:'location',error:'PLUGIN_UNAVAILABLE',message:'No plugin <b>Weather</b> & "missing"'}];
  f.handlers.showConfiguration();
  const text=item(f,'complications-status').defaultValue;
  assert.match(text,/PLUGIN_UNAVAILABLE · No plugin &lt;b&gt;Weather&lt;\/b&gt; &amp; &quot;missing&quot;/);
  assert.doesNotMatch(text,/<b>|<\/b>/);
});

test('complication controls/status follow the active watch language and rebuild after a language change',()=>{
  const f=fixture({}, {watch:'cs_CZ',phone:'de-DE'});Object.assign(f.status,{configured:2,ready:1});
  f.handlers.showConfiguration();const czech=item(f,'complications-status').defaultValue;
  assert.match(czech,/1\/2/);assert.doesNotMatch(czech,/Complications:| ready/);
  assert.equal(named(f,'LAYOUT_STYLE'),undefined);
  assert.equal(named(f,'SHOW_MUSIC').messageKey,'SHOW_MUSIC');
  f.watchLanguage('de-DE');f.handlers.showConfiguration();assert.notEqual(item(f,'complications-status').defaultValue,czech);
  f.watchLanguage('en-US');f.handlers.showConfiguration();assert.match(item(f,'complications-status').defaultValue,/Complications: 1\/2 ready/);
});

test('disabled plugin status is explicit and locale lookup falls back to the phone',()=>{
  const f=fixture({}, {throwWatchInfo:true,phone:'cs-CZ'});f.status.available=false;f.handlers.showConfiguration();
  assert.doesNotMatch(item(f,'complications-status').defaultValue,/Enable plugins/);
  assert.equal(f.options.getSettings().language,'cs');
  const english=fixture({}, {watch:'ja-JP',phone:'ko-KR'});english.status.available=false;english.handlers.showConfiguration();
  assert.match(item(english,'complications-status').defaultValue,/Enable plugins in Pebble/);
});

test('settings delivery retries busy transport, then reconfigures once',()=>{
  const f=fixture();f.fail(true);f.handlers.webviewclosed({response:JSON.stringify({USE_12_HOUR:true})});
  assert.equal(f.deliveries.length,1);assert.equal(f.configured(),0);assert.equal(f.timers[0].delay,1000);
  f.timers.shift().fn();assert.equal(f.deliveries.length,2);
  f.fail(false);f.timers.shift().fn();assert.equal(f.deliveries.length,3);assert.equal(f.configured(),1);
  assert.ok(f.deliveries.every(p=>p[keys.USE_12_HOUR]===1));
});

test('settings delivery failure is bounded instead of retrying forever',()=>{
  const f=fixture();f.fail(true);f.handlers.webviewclosed({response:JSON.stringify({TEXT_COLOR:123})});
  while(f.timers.length)f.timers.shift().fn();
  assert.equal(f.deliveries.length,3);assert.equal(f.configured(),1);
});

test('context source defaults and saved named/numeric settings preserve explicit opt-outs',()=>{
  const defaults=fixture();
  assert.deepEqual(JSON.parse(JSON.stringify(defaults.contextOptions.getSettings())),
    {showMusic:true,showNotifications:false,notificationPreview:false,notificationAppOnly:false,notificationApps:'',showStatus:true});
  for(const settings of [{SHOW_MUSIC:false,SHOW_NOTIFICATIONS:true,NOTIFICATION_PREVIEW:true,NOTIFICATION_APPS:'Signal, WhatsApp',SHOW_STATUS:false},
    {[keys.SHOW_MUSIC]:0,[keys.SHOW_NOTIFICATIONS]:1,[keys.NOTIFICATION_PREVIEW]:1,[keys.NOTIFICATION_APPS]:'Signal, WhatsApp',[keys.SHOW_STATUS]:0}]) {
    const f=fixture(settings);
    assert.deepEqual(JSON.parse(JSON.stringify(f.contextOptions.getSettings())),
      {showMusic:false,showNotifications:true,notificationPreview:true,notificationAppOnly:false,notificationApps:'Signal, WhatsApp',showStatus:false});
  }
});

test('startup and reconnect context requests rebaseline while ordinary requests preserve notification history',()=>{
  const f=fixture();f.handlers.ready();
  f.handlers.appmessage({payload:{REQUEST_CONTEXT:1}});
  f.handlers.appmessage({payload:{[keys.REQUEST_CONTEXT]:2}});
  assert.deepEqual(f.contextRefreshes,[true,undefined]);
  assert.deepEqual(f.connections,[{value:true,token:2}]);
  assert.equal(f.deliveries[0][keys.REQUEST_CONTEXT],1,'Ready asks the native app for its current session');
});

test('phone-only notification and music choices never enter watch packets and apply before theme retries',()=>{
  const f=fixture();f.fail(true);
  f.handlers.webviewclosed({response:JSON.stringify({SHOW_NOTIFICATIONS:true,NOTIFICATION_PREVIEW:true,
    NOTIFICATION_APPS:'Signal',SHOW_MUSIC:false,SHOW_STATUS:false,TEXT_COLOR:123})});
  assert.equal(f.contextConfigured(),1);
  assert.equal(f.configured(),0);
  assert.deepEqual(Object.keys(f.deliveries[0]).sort(),[String(keys.SHOW_STATUS),String(keys.TEXT_COLOR)].sort());
  assert.equal(f.deliveries[0][keys.SHOW_STATUS],0);
  assert.equal(f.contextOptions.getSettings().notificationApps,'Signal');
});

test('dynamic source errors use translated framing and literal HTML-escaped diagnostics',()=>{
  const f=fixture();f.contextStatus.error='PERMISSION_DENIED <test>';
  f.handlers.showConfiguration();
  assert.match(item(f,'context-status').defaultValue,/PERMISSION_DENIED &lt;test&gt;/);
  f.contextStatus.error='';f.contextStatus.ready=true;f.handlers.showConfiguration();
  assert.match(item(f,'context-status').defaultValue,/Dynamic content: ready/);
});

test('Halftone border effects cannot enter Dynamic watch packets',()=>{
  const f=fixture();
  f.handlers.webviewclosed({response:JSON.stringify({PANEL_TRANSITIONS:true,DEPTH_EFFECT:true,TEXT_COLOR:123})});
  assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.TEXT_COLOR)]);
  assert.equal(named(f,'PANEL_TRANSITIONS'),undefined);
  assert.equal(named(f,'DEPTH_EFFECT'),undefined);
});

test('Dynamic uses the installed core API and its own complication cache namespace',()=>{
  const f=fixture();
  assert.equal(f.options.cacheKey,'dynamic-complications-v1');
  assert.doesNotMatch(source,/require\(['"]\.\/(?:complications|context|message-queue|complication-data)['"]\)/);
  assert.match(source,/require\('pebble-watchface-core\/readings'\)/);
  assert.match(source,/require\('pebble-activities'\)/);
});


test('accent colors preserve zero and valid named/numeric values',()=>{
  for(const value of [0,0x55AAFF,0xFFFFFF])for(const field of ['ACCENT_COLOR',String(keys.ACCENT_COLOR)]) {
    const f=fixture();
    f.handlers.webviewclosed({response:JSON.stringify({[field]:{value}})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.ACCENT_COLOR)]);
    assert.equal(f.deliveries[0][keys.ACCENT_COLOR],value);
    assert.equal(f.contextConfigured(),1);
  }
});

test('invalid accent colors are omitted while valid page settings still deliver',()=>{
  for(const value of [-1,0x1000000,1.5,null,'0x55AAFF',{},[]]) {
    const f=fixture();
    f.handlers.webviewclosed({response:JSON.stringify({ACCENT_COLOR:{value},TEXT_COLOR:123})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.TEXT_COLOR)]);
    assert.equal(f.deliveries[0][keys.ACCENT_COLOR],undefined);
  }
});

test('existing colors and privacy choices remain untouched when opening the rebuilt face',()=>{
  const saved={BACKGROUND_COLOR:0,TEXT_COLOR:0xFFFFFF,ACCENT_COLOR:0xFF5500,
    SHOW_NOTIFICATIONS:false,NOTIFICATION_PREVIEW:false,TEXT_SHADOW:true};
  const f=fixture(saved);f.handlers.ready();f.handlers.showConfiguration();
  assert.deepEqual(f.settings(),saved);
  assert.equal(named(f,'TEXT_SHADOW'),undefined);
});

test('new appearance keys extend the original wire contract without renumbering existing keys',()=>{
  const legacy=['TEMPERATURE','REQUEST_WEATHER','BACKGROUND_COLOR','TEXT_COLOR','DEPTH_EFFECT',
    'TEXT_SHADOW','LARGE_TEXT','USE_12_HOUR','USE_FAHRENHEIT','TEMPERATURE_FAHRENHEIT',
    'WEATHER_UPDATED_AT','FORECAST_DATA','FOLLOW_PEBBLE_UNITS','WEATHER_LOCATION','WEATHER_CLEAR',
    'SHOW_DAILY_RANGE','SHOW_RAIN_CHANCE','WEATHER_DETAILS','LAYOUT_STYLE','SHOW_WEATHER_ICON',
    'WEATHER_CONDITION','COMPLICATION_DATA','REQUEST_COMPLICATIONS','CONTEXT_DATA','REQUEST_CONTEXT',
    'SHOW_MUSIC','SHOW_NOTIFICATIONS','NOTIFICATION_PREVIEW','NOTIFICATION_APPS','SHOW_STATUS','PANEL_TRANSITIONS'];
  assert.deepEqual(packageInfo.pebble.messageKeys.slice(0,31),legacy);
  assert.equal(packageInfo.pebble.messageKeys.length,44);
  assert.equal(keys.ACCENT_COLOR,10031);
  assert.equal(keys.ANIMATIONS,10032);
  assert.equal(keys.CLOCK_FORMAT,10033);
});

test('animations deliver explicit on and off through named and numeric settings',()=>{
  for(const field of ['ANIMATIONS',String(keys.ANIMATIONS)])for(const value of [false,0,true,1]) {
    const f=fixture();
    f.handlers.webviewclosed({response:JSON.stringify({[field]:{value}})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.ANIMATIONS)]);
    assert.equal(f.deliveries[0][keys.ANIMATIONS],Number(value));
    assert.equal(f.contextConfigured(),1);
    assert.equal(f.contextOptions.getSettings().showNotifications,false);
    assert.equal(f.contextOptions.getSettings().notificationPreview,false);
  }
});

test('malformed animation settings cannot override other valid settings',()=>{
  for(const field of ['ANIMATIONS',String(keys.ANIMATIONS)])for(const value of [-1,2,0.5,null,'true','false','1','0',{},[]]) {
    const f=fixture();
    f.handlers.webviewclosed({response:JSON.stringify({[field]:{value},TEXT_COLOR:123})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.TEXT_COLOR)]);
    assert.equal(f.deliveries[0][keys.ANIMATIONS],undefined);
  }
});

test('opening the face and settings retains saved animation opt-outs',()=>{
  for(const saved of [{ANIMATIONS:false},{ANIMATIONS:0},{[keys.ANIMATIONS]:false},{[keys.ANIMATIONS]:0}]) {
    const f=fixture(saved);f.handlers.ready();f.handlers.showConfiguration();
    assert.deepEqual(f.settings(),saved);
    assert.equal(named(f,'ANIMATIONS').defaultValue,false,'the form retains the opt-out for either key format');
    assert.equal(f.contextOptions.getSettings().showMusic,true);
    assert.equal(f.contextOptions.getSettings().showNotifications,false);
  }
});

test('missing or malformed saved animation choices retain the enabled new-install default',()=>{
  for(const saved of [{},{ANIMATIONS:null},{ANIMATIONS:'false'},{ANIMATIONS:2},{[keys.ANIMATIONS]:-1}]) {
    const f=fixture(saved);f.handlers.showConfiguration();
    assert.equal(named(f,'ANIMATIONS').defaultValue,true);
    assert.deepEqual(f.settings(),saved);
  }
});

test('clock format follows Pebble for new installs and preserves legacy explicit choices',()=>{
  for(const [saved,wanted] of [[{},0],[{USE_12_HOUR:true},12],[{USE_12_HOUR:false},24],
    [{[keys.USE_12_HOUR]:0},24],[{CLOCK_FORMAT:0,USE_12_HOUR:true},0],
    [{CLOCK_FORMAT:12},12],[{[keys.CLOCK_FORMAT]:24},24]]) {
    const f=fixture(saved);f.handlers.showConfiguration();
    assert.equal(named(f,'CLOCK_FORMAT').defaultValue,wanted);
    assert.equal(f.options.getSettings().clockFormat,wanted);
    assert.deepEqual(f.settings(),saved);
  }
});

test('clock format accepts only the three supported numeric choices',()=>{
  for(const value of [0,12,24]) {
    const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({CLOCK_FORMAT:{value}})});
    assert.equal(f.deliveries[0][keys.CLOCK_FORMAT],value);
  }
  for(const value of [-1,1,13,12.5,null,'12',true,{},[]]) {
    const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({CLOCK_FORMAT:{value},TEXT_COLOR:123})});
    assert.deepEqual(Object.keys(f.deliveries[0]),[String(keys.TEXT_COLOR)]);
  }
});

test('failed settings remain pending for reconnect and clear only after watch acknowledgement',()=>{
  const f=fixture();f.fail(true);
  f.handlers.webviewclosed({response:JSON.stringify({TEXT_COLOR:123})});
  while(f.timers.length)f.timers.shift().fn();
  let record=JSON.parse(f.storage['dynamic-settings-delivery-v1']);
  assert.equal(record.state,'failed');assert.equal(record.payload[keys.TEXT_COLOR],123);
  f.fail(false);f.handlers.appmessage({payload:{REQUEST_COMPLICATIONS:1}});
  record=JSON.parse(f.storage['dynamic-settings-delivery-v1']);
  assert.equal(record.state,'saved');assert.equal(record.payload,undefined);
  assert.equal(f.deliveries.length,4);
});

test('an older settings acknowledgement cannot mark a newer save as delivered',()=>{
  const f=fixture();f.hold(true);
  f.handlers.webviewclosed({response:JSON.stringify({TEXT_COLOR:123})});
  f.handlers.webviewclosed({response:JSON.stringify({TEXT_COLOR:456})});
  const latest=JSON.parse(f.storage['dynamic-settings-delivery-v1']).id;
  f.deferred.shift().ok();
  const pending=JSON.parse(f.storage['dynamic-settings-delivery-v1']);
  assert.equal(pending.id,latest);assert.equal(pending.state,'pending');
  assert.equal(pending.payload[keys.TEXT_COLOR],456);
  f.deferred.shift().ok();
  assert.equal(JSON.parse(f.storage['dynamic-settings-delivery-v1']).state,'saved');
});

test('startup reapplies validated saved presentation without sending private source preferences',()=>{
  for(const saved of [
    {BACKGROUND_COLOR:0,TEXT_COLOR:0xFFFFFF,ACCENT_COLOR:0x55AAFF,CLOCK_FORMAT:12,
      NIGHT_PALETTE:true,NIGHT_BACKGROUND_COLOR:0x005555,NOTIFICATION_VIBRATION:2,
      SHOW_MUSIC:false,SHOW_NOTIFICATIONS:true,NOTIFICATION_PREVIEW:true,NOTIFICATION_APPS:'Private app choice'},
    {[keys.BACKGROUND_COLOR]:0,[keys.TEXT_COLOR]:0xFFFFFF,[keys.NIGHT_PALETTE]:1,
      [keys.NOTIFICATION_VIBRATION]:1,[keys.NOTIFICATION_APPS]:'Private app choice'}]) {
    const f=fixture(saved);f.handlers.ready();
    const presentation=f.deliveries[0];
    assert.equal(presentation[keys.BACKGROUND_COLOR],0);assert.equal(presentation[keys.TEXT_COLOR],0xFFFFFF);
    assert.equal(presentation[keys.NIGHT_PALETTE],1);
    assert.ok([1,2].includes(presentation[keys.NOTIFICATION_VIBRATION]));
    for(const name of ['SHOW_MUSIC','SHOW_NOTIFICATIONS','NOTIFICATION_PREVIEW','NOTIFICATION_APPS'])assert.equal(presentation[keys[name]],undefined);
    assert.ok(!JSON.stringify(presentation).includes('Private'));
    assert.equal(f.deliveries[1][keys.REQUEST_CONTEXT],1);
    assert.deepEqual(f.settings(),saved,'presentation recovery does not rewrite phone preferences');
    assert.equal(f.contextConfigured(),0,'recovery does not change private source controls');
  }
  const f=fixture({TEXT_COLOR:-1,ACCENT_COLOR:'bad',CLOCK_FORMAT:255,NOTIFICATION_VIBRATION:3,SHOW_MUSIC:true});
  f.handlers.ready();assert.equal(f.deliveries.length,1);assert.equal(f.deliveries[0][keys.REQUEST_CONTEXT],1);
});

test('native baseline requests recover presentation even when a reset reuses a token',()=>{
  const f=fixture({TEXT_COLOR:0x0055AA,NOTIFICATION_APP_ONLY:true});f.handlers.ready();
  const initial=f.deliveries.length;
  f.handlers.appmessage({payload:{REQUEST_CONTEXT:71}});
  assert.equal(f.deliveries.length,initial+1);assert.equal(f.deliveries.at(-1)[keys.TEXT_COLOR],0x0055AA);
  f.handlers.appmessage({payload:{[keys.REQUEST_CONTEXT]:71}});assert.equal(f.deliveries.length,initial+2);
  f.handlers.appmessage({payload:{REQUEST_CONTEXT:1}});assert.equal(f.deliveries.length,initial+2);
  f.handlers.appmessage({payload:{REQUEST_CONTEXT:72}});assert.equal(f.deliveries.length,initial+3);
  assert.equal(f.contextOptions.getSettings().notificationAppOnly,true);assert.equal(f.contextConfigured(),0);
});

test('presentation recovery carries pending values forward and ignores late ACKs across a newer save',()=>{
  const f=fixture();f.hold(true);
  f.handlers.webviewclosed({response:JSON.stringify({BACKGROUND_COLOR:0xFFFFFF,TEXT_COLOR:123})});
  const oldId=JSON.parse(f.storage['dynamic-settings-delivery-v1']).id;
  f.handlers.appmessage({payload:{REQUEST_CONTEXT:81}});
  const sync=JSON.parse(f.storage['dynamic-settings-delivery-v1']);assert.notEqual(sync.id,oldId);assert.equal(sync.payload[keys.TEXT_COLOR],123);
  f.handlers.webviewclosed({response:JSON.stringify({BACKGROUND_COLOR:0xFFFFFF,TEXT_COLOR:456})});
  const latest=JSON.parse(f.storage['dynamic-settings-delivery-v1']).id;
  f.deferred.shift().ok();assert.equal(JSON.parse(f.storage['dynamic-settings-delivery-v1']).id,latest);
  f.deferred.shift().ok();assert.equal(JSON.parse(f.storage['dynamic-settings-delivery-v1']).state,'pending');
  f.deferred.shift().ok();assert.equal(JSON.parse(f.storage['dynamic-settings-delivery-v1']).state,'saved');
  assert.deepEqual(f.deliveries.map(p=>p[keys.TEXT_COLOR]),[123,123,456]);
  const pending={id:'prior-phone-runtime',state:'failed',payload:{[keys.TEXT_COLOR]:0x005500,[keys.ACCENT_COLOR]:-1}};
  const restored=fixture({BACKGROUND_COLOR:0xFFFFFF,TEXT_COLOR:0},{storage:{'dynamic-settings-delivery-v1':JSON.stringify(pending)}});
  restored.handlers.ready();assert.equal(restored.deliveries[0][keys.TEXT_COLOR],0x005500);
  assert.equal(restored.deliveries[0][keys.BACKGROUND_COLOR],0xFFFFFF);assert.equal(restored.deliveries[0][keys.ACCENT_COLOR],undefined);
});

test('settings metadata keeps per-slot property and per-feature permission states distinct',()=>{
  const f=fixture({SHOW_NOTIFICATIONS:true});
  f.status.slots=[{slot:1,category:'weather',item:'location',property:'temperature',text:'5°C',status:'ready'},
    {slot:2,category:'weather',item:'location',property:'precipitation',status:'denied',message:'Allow location <access>'}];
  f.contextStatus.sourceStates={music:{state:'idle',message:''},notifications:{state:'denied',message:'PERMISSION_DENIED'},phone:{state:'ready',message:''}};
  f.handlers.showConfiguration();
  const encoded=item(f,'dynamic-settings-data').defaultValue;
  const data=JSON.parse(encoded.replace(/&quot;/g,'"').replace(/&#39;/g,"'").replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&amp;/g,'&'));
  assert.equal(data.slots[0].property,'temperature');assert.equal(data.slots[0].value,'5°C');
  assert.equal(data.slots[1].status,'denied');assert.equal(data.sources.music.state,'idle');
  assert.equal(data.sources.notifications.state,'denied');assert.equal(data.sources.phone.state,'ready');
  assert.doesNotMatch(encoded,/<access>/);
});

test('private selected readings never enter settings examples or provider error diagnostics',()=>{
  const f=fixture();Object.assign(f.status,{configured:2,ready:0,error:'PLUGIN_UNAVAILABLE'});
  f.status.slots=[{slot:1,category:'music',item:'track',property:'title',label:'Private artist',text:'Private song',status:'unavailable',error:'PLUGIN_UNAVAILABLE',message:'Private provider detail'},
    {slot:2,category:'notifications',item:'notification',property:'body',label:'Private sender',text:'Private message',status:'ready'}];
  f.handlers.showConfiguration();
  const visible=JSON.stringify(f.clay.config);
  assert.doesNotMatch(visible,/Private artist|Private song|Private provider detail|Private sender|Private message/);
  assert.match(visible,/PLUGIN_UNAVAILABLE/);
});

function metadataItem(f,id) {
  return JSON.parse(item(f,id).defaultValue.replace(/&quot;/g,'"').replace(/&amp;/g,'&').replace(/&lt;/g,'<').replace(/&gt;/g,'>').replace(/&#39;/g,"'"));
}
test('native system-format replies are cached independently of an explicit clock override',()=>{
  const f=fixture({CLOCK_FORMAT:24});
  f.handlers.appmessage({payload:{CLOCK_FORMAT:12}});
  assert.equal(f.storage['dynamic-system-clock-v1'],'12');
  assert.deepEqual(f.settings(),{CLOCK_FORMAT:24});
  assert.equal(f.options.getSettings().clockFormat,24);
  f.handlers.showConfiguration();
  assert.equal(metadataItem(f,'dynamic-preview-assets').systemFormat,12);
  assert.equal(f.clockQueries.length,1);assert.equal(f.clockQueries[0][keys.CLOCK_FORMAT],255);
  for(const invalid of [0,255,'12',true,{},null])f.handlers.appmessage({payload:{CLOCK_FORMAT:invalid}});
  assert.equal(f.storage['dynamic-system-clock-v1'],'12');
  f.handlers.appmessage({payload:{[keys.CLOCK_FORMAT]:24}});f.handlers.showConfiguration();
  assert.equal(metadataItem(f,'dynamic-preview-assets').systemFormat,24);
  assert.deepEqual(f.settings(),{CLOCK_FORMAT:24});
});
test('unknown system-format remains explicit metadata instead of inventing a watch preference',()=>{
  const f=fixture({CLOCK_FORMAT:0});f.handlers.showConfiguration();
  assert.equal(metadataItem(f,'dynamic-preview-assets').systemFormat,null);
  assert.equal(require('../src/pkjs/settings-preview').systemFormat,undefined,'shared sample assets are not mutated');
  assert.deepEqual(f.settings(),{CLOCK_FORMAT:0});
});
test('settings group appearance, content and recovery in one three-section page',()=>{
  const f=fixture();f.handlers.showConfiguration();
  const sections=f.clay.config.filter(x=>x.type==='section');
  assert.equal(sections.length,3);
  assert.deepEqual(sections.map(x=>x.items[0].defaultValue),['Appearance','Content','Connection & help']);
});


test('new night palette and haptic controls deliver validated native preferences only',()=>{
  for(const name of ['NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR'])for(const field of [name,String(keys[name])]) {
    for(const value of [0,0x55AAFF,0xFFFFFF]) {
      const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({[field]:{value}})});
      assert.equal(f.deliveries[0][keys[name]],value);
    }
    for(const value of [-1,0x1000000,1.5,null,'0xFFFFFF',{},[]]) {
      const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({[field]:{value},TEXT_COLOR:123})});
      assert.equal(f.deliveries[0][keys[name]],undefined);
    }
  }
  for(const value of [false,0,true,1]) {
    const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({NIGHT_PALETTE:{value}})});
    assert.equal(f.deliveries[0][keys.NIGHT_PALETTE],Number(value));
  }
  for(const value of [0,1,2]) {
    const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({NOTIFICATION_VIBRATION:{value}})});
    assert.equal(f.deliveries[0][keys.NOTIFICATION_VIBRATION],value);
  }
  for(const value of [-1,3,0.5,null,'1',{},[]]) {
    const f=fixture();f.handlers.webviewclosed({response:JSON.stringify({NOTIFICATION_VIBRATION:{value},TEXT_COLOR:123})});
    assert.equal(f.deliveries[0][keys.NOTIFICATION_VIBRATION],undefined);
  }
});

test('app-only is a phone privacy preference and saved native choices survive opening settings',()=>{
  for(const name of ['NOTIFICATION_APP_ONLY',String(keys.NOTIFICATION_APP_ONLY)]) {
    const saved={[name]:true,NOTIFICATION_PREVIEW:true,NOTIFICATION_VIBRATION:2,NIGHT_PALETTE:true,NIGHT_BACKGROUND_COLOR:0x005555};
    const f=fixture(saved);f.handlers.showConfiguration();
    assert.equal(f.contextOptions.getSettings().notificationAppOnly,true);
    assert.equal(f.contextOptions.getSettings().notificationPreview,true);
    assert.equal(named(f,'NOTIFICATION_VIBRATION').defaultValue,2);
    assert.equal(named(f,'NIGHT_PALETTE').defaultValue,true);
    assert.equal(named(f,'NIGHT_BACKGROUND_COLOR').defaultValue,0x005555);
    assert.deepEqual(f.settings(),saved);
    f.handlers.webviewclosed({response:JSON.stringify({[name]:true,TEXT_COLOR:123})});
    assert.equal(f.deliveries[0][keys.NOTIFICATION_APP_ONLY],undefined);
    assert.equal(f.contextOptions.getSettings().notificationAppOnly,true);
  }
  assert.deepEqual(packageInfo.pebble.messageKeys.slice(34,41),['NOTIFICATION_VIBRATION','NIGHT_PALETTE','NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR','NOTIFICATION_APP_ONLY','CONTEXT_STATE']);
});

test('generic activity discovery starts with the host and follows native session requests',()=>{
  const f=fixture();
  assert.equal(f.activityOptions.getSettings().enabled,true);
  assert.equal(f.activityOptions.nativeFormat,2);
  f.handlers.ready();assert.equal(f.activityRefreshes.length,1);
  f.handlers.appmessage({payload:{REQUEST_ACTIVITIES:81}});
  f.handlers.appmessage({payload:{[keys.REQUEST_ACTIVITIES]:82}});
  assert.deepEqual(f.activityConnections,[[true,81],[true,82]]);
  f.handlers.appmessage({payload:{REQUEST_ACTIVITIES:1}});
  assert.equal(f.activityRefreshes.length,2);
  for(const value of [0,-1,1.2,2147483648,'garbage'])
    f.handlers.appmessage({payload:{REQUEST_ACTIVITIES:value}});
  assert.equal(f.activityConnections.length,2);
});

test('activity preferences remain phone-only and reconfigure before native settings delivery',()=>{
  for(const key of ['SHOW_ACTIVITIES',String(keys.SHOW_ACTIVITIES)]) {
    const f=fixture({[key]:false});
    assert.equal(f.activityOptions.getSettings().enabled,false);
    f.handlers.webviewclosed({response:JSON.stringify({[key]:true,TEXT_COLOR:123})});
    assert.equal(f.activityConfigured(),1);
    assert.equal(f.activityOptions.getSettings().enabled,true);
    assert.equal(f.deliveries[0][keys.SHOW_ACTIVITIES],undefined);
    assert.equal(f.deliveries[0][keys.TEXT_COLOR],123);
  }
  assert.deepEqual(packageInfo.pebble.messageKeys.slice(41),
    ['ACTIVITY_DATA','REQUEST_ACTIVITIES','SHOW_ACTIVITIES']);
});

test('generic sources respect music opt-outs and reserve notifications for the baseline adapter',()=>{
  for(const saved of [{SHOW_MUSIC:false},{[keys.SHOW_MUSIC]:0}]) {
    const f=fixture(saved),accept=f.activityOptions.acceptActivity;
    for(const kind of ['timer','glucose','workout'])assert.equal(accept({kind}),true);
    assert.equal(accept({kind:'music'}),false);
    for(const kind of ['notification','notifications','Notification'])assert.equal(accept({kind}),false);
    f.handlers.webviewclosed({response:JSON.stringify({SHOW_MUSIC:true})});
    assert.equal(accept({kind:'music'}),true);
  }
  assert.equal(fixture().activityOptions.acceptActivity({kind:'music'}),true);
});

test('ongoing generic activities take priority over music without reordering their sources',()=>{
  const select=fixture().activityOptions.select;
  const music={kind:'music',id:'song'},timer={kind:'timer',id:'tea'},glucose={kind:'glucose',id:'level'};
  assert.deepEqual(Array.from(select([music,timer])),[timer,music]);
  assert.deepEqual(Array.from(select([music,timer,glucose])),[timer,glucose]);
  assert.deepEqual(Array.from(select([glucose,timer,music])),[glucose,timer]);
});

test('soonest running deadline is projected even from the eighth timer slot',()=>{
  const select=fixture().activityOptions.select;
  const cards=Array.from({length:8},(_,index)=>({provider:'timer-provider',kind:'timer',
    id:String(index),state:'active',main:{type:'clock',mode:'deadline',running:true,anchorAt:1000,baseSeconds:1000-index*100}}));
  const original=cards.slice();
  assert.deepEqual(Array.from(select(cards),card=>card.id),['7','6']);
  cards[7]={...cards[7],state:'paused',main:{...cards[7].main,running:false,anchorAt:0}};
  assert.deepEqual(Array.from(select(cards),card=>card.id),['6','5']);
  assert.equal(original[0].id,'0');
});

test('clock ordering preserves other provider positions and stable equal deadlines',()=>{
  const select=fixture().activityOptions.select;
  const reading={provider:'health',kind:'glucose',id:'reading'};
  const later={provider:'timer',kind:'timer',id:'later',state:'active',main:{type:'clock',mode:'deadline',running:true,anchorAt:100,baseSeconds:400}};
  const sooner={...later,id:'sooner',main:{...later.main,baseSeconds:200}};
  assert.deepEqual(Array.from(select([reading,later,sooner]),card=>card.id),['reading','sooner']);
  assert.deepEqual(Array.from(select([later,{...later,id:'equal'}]),card=>card.id),['later','equal']);
});

test('native view selection preserves canonical items, units, artwork and source data across roles',()=>{
  const select=fixture().activityOptions.select;
  const reading={provider:'sensor',id:'reading',kind:'glucose',revision:7,state:'active',caption:'Sensor',
    main:{type:'value',label:'Glucose',text:'6.2 mmol/L'},support:null,image:{width:2,height:1,palette:[192],pixels:[0]}};
  const timer={provider:'timer',id:'timer',kind:'timer',state:'active',main:{type:'clock',mode:'deadline',running:true,anchorAt:100,baseSeconds:400},support:{type:'clock',mode:'elapsed',running:true,anchorAt:100,baseSeconds:20}};
  const before=JSON.stringify([reading,timer]);
  assert.equal(select([reading,timer])[0],reading);assert.equal(select([timer,reading])[1],reading);
  assert.equal(reading.main.text,'6.2 mmol/L');assert.deepEqual(reading.image,{width:2,height:1,palette:[192],pixels:[0]});
  assert.equal(JSON.stringify([reading,timer]),before,'host role selection must not rewrite source or view content');
});

test('retained source results follow ongoing activities and precede fallback music',()=>{
  const select=fixture().activityOptions.select;
  const result={kind:'timer',state:'completed',showTerminal:true,id:'finished'},active={kind:'workout',state:'active',id:'run'},music={kind:'music',state:'active',id:'song'};
  assert.deepEqual(Array.from(select([music,result,active]),card=>card.id),['run','finished']);
  assert.deepEqual(Array.from(select([result,music]),card=>card.id),['finished','song']);
});

test('deadline ordering uses each datum state and its anchor plus base',()=>{
  const select=fixture().activityOptions.select;
  const first={provider:'provider',kind:'timer',id:'first',state:'paused',main:{type:'clock',mode:'deadline',running:true,anchorAt:100,baseSeconds:200}};
  const equal={...first,id:'equal',state:'active',main:{...first.main,anchorAt:200,baseSeconds:100}};
  const paused={...first,id:'paused-clock',state:'active',main:{...first.main,running:false,anchorAt:0,baseSeconds:1}};
  assert.deepEqual(Array.from(select([paused,first,equal]),card=>card.id),['first','equal']);
});
