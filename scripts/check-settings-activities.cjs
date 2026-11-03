// Run a local Chromium with --remote-debugging-port=9227, then invoke this script.
// Exercises the actual bundled Clay page with synthetic provider metadata only.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const path=require('node:path'),os=require('node:os'),crypto=require('node:crypto');
const {execFileSync}=require('node:child_process');
const root=path.resolve(__dirname,'..');
const version=require(path.join(root,'package.json')).version;
const output=path.resolve(process.argv[2] || path.join(root,'docs/qa',version,'settings'));
const endpoint=process.env.DYNAMIC_CHROMIUM_ENDPOINT || 'http://127.0.0.1:9227';
const scratch=fs.mkdtempSync(path.join(os.tmpdir(),'dynamic-settings-'));
fs.mkdirSync(output,{recursive:true});
const config=require(root+'/src/pkjs/config'),localization=require(root+'/src/pkjs/localization');
const custom=require(root+'/src/pkjs/settings-page');
const candidate=path.resolve(process.env.DYNAMIC_QA_PBW || path.join(root,'dist','Dynamic-'+version+'.pbw'));
const candidateBytes=fs.existsSync(candidate)?fs.readFileSync(candidate):null;
if(process.env.DYNAMIC_QA_PBW)assert.ok(candidateBytes,'the requested packaged candidate exists');
const frozenCandidate=path.join(scratch,'candidate.pbw');
if(candidateBytes)fs.writeFileSync(frozenCandidate,candidateBytes);
const candidateScript=candidateBytes?execFileSync('unzip',['-p',frozenCandidate,'pebble-js-app.js'],{maxBuffer:8*1024*1024}):null;
const baselineRef=process.env.DYNAMIC_QA_BASELINE_REF || 'a9d2036f51ac3da2c84b69677c6a8d36aab32df6';
const baselineSource=candidateScript?(process.env.DYNAMIC_QA_BASELINE_SOURCE?
  fs.readFileSync(process.env.DYNAMIC_QA_BASELINE_SOURCE,'utf8'):
  execFileSync('git',['show',baselineRef+':src/pkjs/settings-page.js'],{cwd:root,encoding:'utf8'})):null;
let referenceRecolor;
if(baselineSource){
  const start=baselineSource.indexOf('var pixels = ctx.getImageData(0,0,200,228)'),end=baselineSource.indexOf('ctx.putImageData(pixels,0,0);',start);
  assert.ok(start>=0&&end>start,'the approved baseline supplies the independent old recoloring implementation');
  referenceRecolor=baselineSource.slice(start,end+'ctx.putImageData(pixels,0,0);'.length);
}
const keys=Object.fromEntries(require(root+'/package.json').pebble.messageKeys.map((key,index)=>[key,index]));
const timer='11111111-1111-4111-8111-111111111111',workout='22222222-2222-4222-8222-222222222222',absent='33333333-3333-4333-8333-333333333333',evil='44444444-4444-4444-8444-444444444444';
const pages={};
const packagedPages={},packagedMetadata={};
const installedClay=path.join(root,'node_modules/@rebble/clay');
const bundle=fs.existsSync(path.join(installedClay,'dist/js/index.js')) ? fs.readFileSync(path.join(installedClay,'dist/js/index.js'),'utf8') :
  execFileSync('unzip',['-p',path.join(installedClay,'dist.zip'),'js/index.js'],{encoding:'utf8'});
function replayHost(initialStorage={}) {
  const storage={...initialStorage},handlers={},deliveries=[];let options;
  const localStorage={getItem:key=>storage[key] || null,setItem:(key,value)=>{storage[key]=value;}};
  const Pebble={platform:'test',getActiveWatchInfo:()=>({language:'en'}),getAccountToken:()=>'',getWatchToken:()=>'',
    addEventListener:(event,handler)=>{handlers[event]=handler;},sendAppMessage:(payload,ok)=>{deliveries.push(payload);if(ok)ok();},openURL:()=>{}};
  const claySandbox={module:{exports:{}},exports:{},require:name=>{assert.equal(name,'message_keys');return keys;},Pebble,localStorage,console};
  vm.runInNewContext(bundle,claySandbox);
  const library=require(root+'/node_modules/pebble-activities'),now=1700000000;
  const base={observedAt:now,validUntil:now+3600,revision:1,state:'active',caption:'App',main:{type:'text',label:'Current',text:'Example'}};
  const timerCard={...base,provider:timer,providerName:'Timer',id:'tea',kind:'timer'};
  const musicCard={...base,provider:'66666666-6666-4666-8666-666666666666',providerName:'Music',id:'track',kind:'music',selectionKey:'builtin:music',private:true};
  const notificationCard={...base,provider:'cdb6d497-547a-4940-8026-f7c97ea73a9d',providerName:'Notifications',id:'notice',kind:'message',
    selectionKey:'builtin:notifications',private:true,caption:'Signal',main:{type:'text',label:'',text:'Sam'}};
  vm.runInNewContext(fs.readFileSync(root+'/src/pkjs/index.js','utf8'),{
    require:name=>{
      if(name==='message_keys')return keys;
      if(name==='@rebble/clay')return claySandbox.module.exports;
      if(name==='../common/activity-navigation')return require(root+'/src/common/activity-navigation');
      if(name==='pebble-activities/host')return {selection:library.selection,
        createContext:()=>({musicActivity:()=>musicCard,notificationActivity:()=>notificationCard,configure:()=>{},status:()=>({}),refresh:()=>{},connectionChanged:()=>{}}),
        createActivities:received=>{options=received;return {configure:()=>{},reproject:()=>{},providers:()=>[],status:()=>({}),refresh:()=>{},connectionChanged:()=>{}};}};
      if(name==='pebble-watchface-core/readings')return {
        createMessageQueue:()=>((payload,ok)=>{deliveries.push(payload);if(ok)ok();}),
        createComplications:()=>({configure:()=>{},refresh:()=>{},status:()=>({slots:[]})})};
      if(name.startsWith('./'))return require(root+'/src/pkjs/'+name.slice(2));
      throw Error(name);
    },Pebble,localStorage,navigator:{language:'en'},console,setTimeout:()=>1,clearTimeout:()=>{}
  });
  return {storage,save:response=>handlers.webviewclosed({response:JSON.stringify(response)}),
    selected:()=>Array.from(options.select([timerCard]),card=>card.kind),deliveries};
}
function packagedPage(language){
  const storage={'clay-settings':JSON.stringify({SHOW_MUSIC:true,SHOW_ACTIVITIES:true,SHOW_NOTIFICATIONS:false}),
    'dynamic-activity-preferences-v1':JSON.stringify({order:[timer,'builtin:music','builtin:notifications',workout,absent],hidden:[workout]})};
  const handlers={},urls=[],messages=[],subscriptions=[],logs=[];
  const source=require(root+'/node_modules/pebble-activities/summary');
  const Pebble={platform:'test',getActiveWatchInfo:()=>({language}),getAccountToken:()=>'',getWatchToken:()=>'',
    addEventListener:(name,handler)=>{(handlers[name]||(handlers[name]=[])).push(handler);},
    sendAppMessage:(payload,ok)=>{messages.push(payload);if(ok)ok();},openURL:url=>urls.push(url),
    enumeratePlugins:()=>[{uuid:timer,name:'Timer'},{uuid:workout,name:'Workout'},{uuid:evil,name:'<img src=x onerror="window.__xss=true">'}].map(provider=>({...provider,
      sources:[{category:'activity',items:['summary'],supportsMultiple:true,properties:source.SOURCE_PROPERTIES}]})),
    subscribeToSource:request=>{subscriptions.push({category:request.category,item:request.item});return {unsubscribe:()=>{}};}};
  const localStorage={getItem:key=>Object.hasOwn(storage,key)?storage[key]:null,setItem:(key,value)=>{storage[key]=String(value);},removeItem:key=>{delete storage[key];}};
  vm.runInNewContext(candidateScript.toString('utf8'),{Pebble,localStorage,navigator:{language},console:{log:message=>logs.push(String(message)),warn:()=>{},error:()=>{}},
    setTimeout:()=>1,clearTimeout:()=>{}},{filename:'actual-minified-pebble-js-app.js',timeout:10000});
  assert.ok(handlers.ready&&handlers.showConfiguration,'actual packaged runtime registers lifecycle/configuration events');
  handlers.ready.forEach(handler=>handler());
  handlers.showConfiguration.forEach(handler=>handler());
  assert.equal(urls.length,1,'the actual packaged handler opens one settings page');
  assert.ok(urls[0].startsWith('data:text/html;charset=utf-8,'));
  const html=decodeURIComponent(urls[0].slice('data:text/html;charset=utf-8,'.length));
  assert.match(html,/window\.customFn=/);
  assert.doesNotMatch(html,/\$\$CUSTOM_FN\$\$/,'Clay replacement-string expansion must not corrupt the serialized custom function');
  // Expose the real browser Clay instance without replacing or rewriting the
  // serialized minified custom function. The original page bytes are hashed.
  const hook='customFn.call(clayConfig, minified);';
  assert.equal(html.split(hook).length,2,'expected one stock Clay invocation');
  const exposed=html.replace(hook,'window.__testClay=clayConfig;'+hook);
  const file=path.join(scratch,'packaged-settings-'+language+'.html');fs.writeFileSync(file,exposed);
  return {file,metadata:{language,pbwSha256:crypto.createHash('sha256').update(candidateBytes).digest('hex'),
    installedPkjsSha256:crypto.createHash('sha256').update(candidateScript).digest('hex'),installedPkjsBytes:candidateScript.length,
    actualGeneratedHtmlSha256:crypto.createHash('sha256').update(html).digest('hex'),actualGeneratedHtmlBytes:Buffer.byteLength(html),
    realPackagedRuntime:true,realPackagedShowConfiguration:true,serializedFunctionUnchanged:true,
    browserTestHook:'Expose stock Clay instance immediately before its unchanged customFn.call',syntheticProviders:true,
    sourceSubscriptions:subscriptions,watchMessages:messages.length,phoneVmLogs:logs}};
}
if(candidateScript)for(const language of ['en','cs']){const result=packagedPage(language);packagedPages[language]=result.file;packagedMetadata[language]=result.metadata;}
for(const scenario of ['en','cs','bounded','off-app']){
  const language=['bounded','off-app'].includes(scenario)?'en':scenario;
  const saved=scenario==='off-app'?{SHOW_MUSIC:false,SHOW_ACTIVITIES:false,SHOW_NOTIFICATIONS:false,NOTIFICATION_APP_ONLY:true,NOTIFICATION_PREVIEW:true}:
    {SHOW_MUSIC:true,SHOW_ACTIVITIES:true,SHOW_NOTIFICATIONS:false};
  const sandbox={module:{exports:{}},exports:{},require:name=>{if(name==='message_keys')return keys;throw Error(name);},
    Pebble:{platform:'test',getActiveWatchInfo:()=>({language}),getAccountToken:()=>'',getWatchToken:()=>'',addEventListener:()=>{}},
    localStorage:{getItem:()=>JSON.stringify(saved)},console};
  vm.runInNewContext(bundle,sandbox);
  const Clay=sandbox.module.exports;
  const wrapped=vm.runInNewContext('(function(){window.__testClay=this;('+custom.toString()+').call(this);})');
  const clay=new Clay(localization.translateConfig(config,language),wrapped,{autoHandleEvents:false});
  const meta={language,slots:[],sources:{},save:{state:'none'},activityProviders:[{uuid:timer,name:'Timer',available:true},{uuid:workout,name:'Workout',available:true},{uuid:evil,name:'<img src=x onerror="window.__xss=true">',available:true}],
    activityPreferences:{order:[timer,'builtin:music','builtin:notifications',workout,absent],hidden:[workout]}};
  if(scenario==='bounded')meta.activityPreferences.order=Array.from({length:64},(_,i)=>`${(i+1).toString(16).padStart(8,'0')}-aaaa-4aaa-8aaa-aaaaaaaaaaaa`);
  const escape=value=>String(value).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  for(const section of clay.config)for(const item of [section,...(section.items||[])]){
    if(item.id==='dynamic-settings-data')item.defaultValue=escape(JSON.stringify(meta));
    if(item.id==='dynamic-settings-strings')item.defaultValue=escape(JSON.stringify(localization.settingsStrings(language)));
    if(item.id==='dynamic-preview-assets')item.defaultValue=escape(JSON.stringify(require(root+'/src/pkjs/settings-preview')));
  }
  const htmlPath=scratch+'/settings-'+scenario+'.html';
  fs.writeFileSync(htmlPath,decodeURIComponent(clay.generateUrl().replace(/^data:text\/html;charset=utf-8,/,'')));
  pages[scenario]=htmlPath;
}
async function run(){
  const tabs=await(await fetch(endpoint+'/json')).json();
  const socket=new WebSocket(tabs.find(t=>t.type==='page').webSocketDebuggerUrl),pending=new Map(),events=[];let seq=0;
  await new Promise((resolve,reject)=>{socket.onopen=resolve;socket.onerror=reject;});
  socket.onmessage=message=>{const data=JSON.parse(message.data);if(data.id){const waiter=pending.get(data.id);pending.delete(data.id);data.error?waiter.reject(data.error):waiter.resolve(data.result);}else events.push(data);};
  const send=(method,params={})=>new Promise((resolve,reject)=>{const id=++seq;pending.set(id,{resolve,reject});socket.send(JSON.stringify({id,method,params}));});
  const evaluate=async expression=>{const result=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(result.exceptionDetails)throw new Error(result.result.description||result.exceptionDetails.text);return result.result.value;};
  const settle=()=>evaluate('new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))');
  const capture=async name=>{await evaluate('new Promise(resolve=>setTimeout(resolve,250))');const image=await send('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});fs.writeFileSync(path.join(output,name+'.png'),Buffer.from(image.data,'base64'));};
  await send('Page.enable');await send('Runtime.enable');
  // Runtime.enable can replay exceptions from the tab's previous document.
  // The audit records only errors from pages it subsequently navigates to.
  events.length=0;
  const results=[];
  for(const language of ['en','cs']){
    await send('Emulation.setDeviceMetricsOverride',{width:360,height:800,deviceScaleFactor:1,mobile:true});
    await send('Page.navigate',{url:'file://'+pages[language]});
    for(let i=0;i<100;i++){if(await evaluate('!!document.querySelector(".dynamic-activity-row")'))break;await new Promise(r=>setTimeout(r,50));}
    assert.deepEqual(await evaluate('Array.from(document.querySelectorAll(".dynamic-section-nav button"),button=>button.textContent)'),
      ['appearance','content','helpConnection'].map(key=>localization.t(key,language)));
    const initialNavigation=await evaluate('({hash:location.hash,length:history.length})');
    for(const section of [1,2,0]) {
      await evaluate('document.querySelectorAll(".dynamic-section-nav button")['+section+'].focus()');
      await send('Input.dispatchKeyEvent',{type:'keyDown',key:'Enter',code:'Enter',windowsVirtualKeyCode:13,text:'\r',unmodifiedText:'\r'});
      await send('Input.dispatchKeyEvent',{type:'keyUp',key:'Enter',code:'Enter',windowsVirtualKeyCode:13});
      assert.equal(await evaluate('document.activeElement.id'),'dynamic-section-heading-'+section);
      assert.deepEqual(await evaluate('({hash:location.hash,length:history.length})'),initialNavigation);
      assert.equal(await evaluate('(()=>{const box=document.querySelector(".component-submit button").getBoundingClientRect();return box.top>=0&&box.bottom<=innerHeight;})()'),true);
    }
    await evaluate('window.scrollTo(0,0)');
    const paletteBefore=await evaluate('window.__testClay.serialize()');
    assert.equal(await evaluate('document.querySelector(".dynamic-palette-details").open'),false);
    assert.equal(await evaluate('document.querySelectorAll(".dynamic-preview-tabs button").length'),6);
    await evaluate('document.querySelector(".dynamic-preview-tabs button").focus()');
    for(const [key,want] of [['End',5],['Home',0],['ArrowLeft',5],['ArrowRight',0]]) {
      await send('Input.dispatchKeyEvent',{type:'keyDown',key,code:key,windowsVirtualKeyCode:{End:35,Home:36,ArrowLeft:37,ArrowRight:39}[key]});
      await send('Input.dispatchKeyEvent',{type:'keyUp',key,code:key,windowsVirtualKeyCode:{End:35,Home:36,ArrowLeft:37,ArrowRight:39}[key]});
      assert.equal(await evaluate('Array.from(document.querySelectorAll(".dynamic-preview-tabs button")).indexOf(document.activeElement)'),want);
    }
    await evaluate('document.querySelectorAll(".dynamic-preview-tabs button")[2].click()');
    await settle();
    assert.match(await evaluate('document.getElementById("dynamic-preview-canvas").getAttribute("aria-label")'),new RegExp(localization.t('previewTimer',language)));
    assert.deepEqual(await evaluate('window.__testClay.serialize()'),paletteBefore,'sample selection never changes appearance or privacy');
    await evaluate('window.scrollTo(0,0)');await capture('timer-preview-'+language);
    await evaluate('document.querySelector(".dynamic-palette-details summary").focus()');
    await send('Input.dispatchKeyEvent',{type:'keyDown',key:'Enter',code:'Enter',windowsVirtualKeyCode:13,text:'\r'});
    await send('Input.dispatchKeyEvent',{type:'keyUp',key:'Enter',code:'Enter',windowsVirtualKeyCode:13});
    assert.equal(await evaluate('document.querySelector(".dynamic-palette-details").open'),true);
    await evaluate('document.getElementById("dynamic-palette-input").focus()');
    await evaluate('new Promise(resolve=>setTimeout(resolve,50))');
    assert.equal(await evaluate('(()=>{const r=document.getElementById("dynamic-palette-input").getBoundingClientRect();return r.top>=0&&r.bottom<=document.querySelector(".component-submit").getBoundingClientRect().top-12;})()'),true);
    await capture('share-colours-'+language);
    await evaluate('document.querySelector(".dynamic-palette-details").open=false;document.querySelectorAll(".dynamic-preview-tabs button")[0].click();window.scrollTo(0,0)');
    if(language==='en') {
      for(const scheme of ['light','dark']) {
        await send('Emulation.setEmulatedMedia',{features:[{name:'prefers-color-scheme',value:scheme}]});
        await evaluate('document.fonts.ready');
        const shot=await send('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
        fs.writeFileSync(path.join(output,'appearance-'+scheme+'.png'),Buffer.from(shot.data,'base64'));
      }
      await send('Emulation.setEmulatedMedia',{features:[{name:'prefers-color-scheme',value:'light'}]});
    }
    const rows=()=>evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row"),r=>r.dataset.provider)');
    assert.deepEqual(await rows(),[timer,'builtin:music','builtin:notifications',workout,absent,evil]);
    const notificationRow='Array.from(document.querySelectorAll(".dynamic-activity-row")).find(r=>r.dataset.provider==="builtin:notifications")';
    const notificationToggle=()=>evaluate(notificationRow+'.querySelector("input").checked');
    assert.equal(await notificationToggle(),false,'Notifications preserve the Off default');
    assert.deepEqual(await evaluate('Array.from(document.getElementById("dynamic-notification-mode-select").options,o=>o.value)'),['app','summary','preview']);
    assert.equal(await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].hidden'),true,'privacy details are hidden while Notifications are off');
    assert.equal(await evaluate('window.__testClay.getItemById("dynamic-notification-disabled").$element[0].hidden'),false);
    assert.equal(await evaluate('window.__testClay.getItemById("context-status").$element[0].hidden'),true,'duplicate context summary is hidden');
    assert.equal(await evaluate('document.body.innerText.includes("Clock mark")'),false);
    const pluginMaster=async enabled=>evaluate('window.__testClay.getItemByMessageKey("SHOW_ACTIVITIES").set('+enabled+')');
    const beforeMaster=await evaluate('window.__testClay.serialize().__dynamicActivityPreferences.value');
    await pluginMaster(false);
    assert.equal(await evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row")).filter(r=>!r.dataset.provider.startsWith("builtin:")).every(r=>r.querySelector("input").disabled&&r.dataset.disabled==="true")'),true);
    assert.equal(await evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row")).filter(r=>r.dataset.provider.startsWith("builtin:")).some(r=>r.querySelector("input").disabled)'),false);
    assert.equal(await evaluate('window.__testClay.serialize().__dynamicActivityPreferences.value'),beforeMaster,'master switch preserves saved per-app order/visibility');
    await evaluate('window.__testClay.getItemByMessageKey("SHOW_ACTIVITIES").$element[0].scrollIntoView({block:"start"})');
    await capture('plugins-off-'+language);
    await pluginMaster(true);
    assert.equal(await evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row input")).some(input=>input.disabled)'),false);
    await evaluate('document.querySelectorAll(".dynamic-activity-row")[0].scrollIntoView({block:"center"})');
    const touch=await evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row")).slice(0,2).map(r=>{const box=r.querySelector(".dynamic-activity-handle").getBoundingClientRect();return {x:box.x+box.width/2,y:box.y+box.height/2};})');
    await send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[touch[0]]});
    await send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[touch[1]]});
    await send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
    assert.deepEqual(await rows(),['builtin:music',timer,'builtin:notifications',workout,absent,evil],'native touch drag reorders');
    const musicFirstResponse=await evaluate('window.__testClay.serialize()');
    assert.equal(await evaluate('document.querySelector(".dynamic-save-state").textContent'),localization.t('saveUnsaved',language));
    await evaluate('(()=>{const rows=document.querySelectorAll(".dynamic-activity-row"),handle=rows[0].querySelector(".dynamic-activity-handle"),transfer=new DataTransfer();handle.dispatchEvent(new DragEvent("dragstart",{bubbles:true,dataTransfer:transfer}));rows[1].dispatchEvent(new DragEvent("dragover",{bubbles:true,cancelable:true,dataTransfer:transfer}));rows[1].dispatchEvent(new DragEvent("drop",{bubbles:true,cancelable:true,dataTransfer:transfer}));})()');
    assert.deepEqual(await rows(),[timer,'builtin:music','builtin:notifications',workout,absent,evil],'desktop drag handlers reorder');
    const timerFirstResponse=await evaluate('window.__testClay.serialize()');
    assert.equal(await evaluate('Boolean(window.__xss)'),false);
    assert.equal(await evaluate('document.querySelectorAll(".dynamic-activity-name img").length'),0);
    await evaluate('document.querySelectorAll(".dynamic-activity-row")[1].querySelector("[data-action=up]").click()');
    assert.deepEqual(await rows(),['builtin:music',timer,'builtin:notifications',workout,absent,evil]);
    await evaluate('document.querySelectorAll(".dynamic-activity-row")[0].querySelector(".dynamic-activity-handle").dispatchEvent(new KeyboardEvent("keydown",{key:"ArrowDown",bubbles:true}))');
    assert.deepEqual(await rows(),[timer,'builtin:music','builtin:notifications',workout,absent,evil]);
    await evaluate('document.querySelectorAll(".dynamic-activity-row")[1].querySelector("input").click()');
    const serialized=await evaluate('window.__testClay.serialize()');
    const preferences=JSON.parse(serialized.__dynamicActivityPreferences.value);
    assert.deepEqual(preferences,{order:[timer,'builtin:music','builtin:notifications',workout,absent,evil],hidden:[workout,'builtin:notifications','builtin:music']});
    assert.equal(serialized.SHOW_MUSIC.value,false);
    assert.equal(serialized.SHOW_NOTIFICATIONS.value,false);
    const host=replayHost();host.save(musicFirstResponse);assert.deepEqual(host.selected(),['music','timer']);
    assert.deepEqual(replayHost(host.storage).selected(),['music','timer'],'fresh host restores the actual Clay response');
    host.save(timerFirstResponse);assert.deepEqual(host.selected(),['timer','music']);
    host.save(serialized);assert.deepEqual(host.selected(),['timer']);
    assert.equal(JSON.parse(host.storage['clay-settings']).__dynamicActivityPreferences,undefined);
    assert.ok(host.deliveries.every(payload=>payload.NaN===undefined&&payload.__dynamicActivityPreferences===undefined));
    assert.doesNotMatch(host.storage['dynamic-activity-preferences-v1'],/Example|track|tea|onerror/);
    await evaluate('(()=>{const input='+notificationRow+'.querySelector("input");input.focus();input.click();})()');
    assert.equal(await notificationToggle(),true);
    assert.equal(await evaluate('document.activeElement.closest(".dynamic-activity-row").dataset.provider'),'builtin:notifications','checkbox focus survives the privacy block update');
    assert.equal(await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].hidden'),false);
    assert.equal(await evaluate('document.getElementById("dynamic-notification-mode-select").value'),'summary');
    const notificationsSecondResponse=await evaluate('window.__testClay.serialize()');
    assert.equal(notificationsSecondResponse.SHOW_NOTIFICATIONS.value,true);
    host.save(notificationsSecondResponse);assert.deepEqual(host.selected(),['timer','message']);
    for(let move=0;move<2;move++)await evaluate(notificationRow+'.querySelector("[data-action=up]").click()');
    const notificationsFirstResponse=await evaluate('window.__testClay.serialize()');
    host.save(notificationsFirstResponse);assert.deepEqual(host.selected(),['message','timer']);
    assert.deepEqual(replayHost(host.storage).selected(),['message','timer'],'Notifications priority survives a fresh host');
    const setNoticeMode=async value=>evaluate('(()=>{const mode=document.getElementById("dynamic-notification-mode-select");mode.value='+JSON.stringify(value)+';mode.dispatchEvent(new Event("change",{bubbles:true}));})()');
    await setNoticeMode('preview');
    await evaluate('(()=>{const field=document.getElementById("dynamic-notification-mode-select");const r=field.getBoundingClientRect();scrollBy(0,r.top-(innerHeight-30));field.focus();})()');
    await evaluate('new Promise(resolve=>setTimeout(resolve,50))');
    assert.equal(await evaluate('(()=>{const r=document.getElementById("dynamic-notification-mode-select").getBoundingClientRect();return r.top>=0&&r.bottom<=document.querySelector(".component-submit").getBoundingClientRect().top-12;})()'),true,'select focus stays above fixed Save');
    assert.equal(await notificationToggle(),true);
    assert.equal((await evaluate('window.__testClay.serialize()')).NOTIFICATION_PREVIEW.value,true);
    await setNoticeMode('app');
    assert.equal((await evaluate('window.__testClay.serialize()')).NOTIFICATION_APP_ONLY.value,true);
    await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].scrollIntoView({block:"start"})');
    await capture('notification-details-'+language);
    await evaluate(notificationRow+'.querySelector("input").click()');
    assert.equal(await notificationToggle(),false,'activity row is the single Notifications visibility control');
    assert.equal(await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].hidden'),true);
    await evaluate(notificationRow+'.querySelector("input").click()');
    assert.equal(await evaluate('document.getElementById("dynamic-notification-mode-select").value'),'app','re-enabling preserves app-only privacy');
    const appOnlyResponse=await evaluate('window.__testClay.serialize()');
    host.save(appOnlyResponse);assert.deepEqual(host.selected(),['message','timer'],'privacy does not select a separate layout or role');
    await evaluate(notificationRow+'.querySelector("input").click()');
    assert.equal(await evaluate('document.getElementById("dynamic-notification-mode-select").value'),'app','hidden notifications retain app-only privacy');
    const hiddenResponse=await evaluate('window.__testClay.serialize()');host.save(hiddenResponse);assert.deepEqual(host.selected(),['timer']);
    const overflow=await evaluate('({width:innerWidth,scrollWidth:document.documentElement.scrollWidth,buttons:Array.from(document.querySelectorAll(".dynamic-activity-row button"),b=>b.getBoundingClientRect().height)})');
    assert.ok(overflow.scrollWidth<=overflow.width);assert.ok(overflow.buttons.every(h=>h>=44));
    await evaluate('window.__testClay.getItemById("dynamic-activity-order").$element[0].scrollIntoView({block:"center"})');
    const screenshot=await send('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
    fs.writeFileSync(path.join(output,language+'.png'),Buffer.from(screenshot.data,'base64'));
    await evaluate('window.__testClay.getItemById("dynamic-source-status").$element[0].scrollIntoView({block:"start"})');
    await capture('connection-'+language);
    results.push({language,rows:await rows(),serialized:preferences,realClayResponse:serialized,notificationResponses:{secondary:notificationsSecondResponse,primary:notificationsFirstResponse,appOnly:appOnlyResponse,hidden:hiddenResponse},overflow,checks:['buttons','keyboard','native touch drag','desktop drag/drop','order-only dirty state','Music show/hide','Notifications default Off','three privacy modes with no duplicate Off','master switch preserves order and does not disable built-ins','checkbox focus retained','Notifications primary/secondary priority saved and restored','Notifications visibility and privacy mode synchronized','re-enable retains app-only privacy','literal provider names','retained absent provider','appended new provider','actual Clay response persisted by production PKJS','fresh host restores Music-first order','actual Clay response restores Timer-first order','actual Clay response hides Music']});
  }
  await send('Emulation.setDeviceMetricsOverride',{width:320,height:800,deviceScaleFactor:1,mobile:true});
  await send('Page.navigate',{url:'file://'+pages['off-app']});
  for(let i=0;i<100;i++){if(await evaluate('!!document.querySelector(".dynamic-activity-row")'))break;await new Promise(r=>setTimeout(r,50));}
  assert.equal(await evaluate('document.getElementById("dynamic-notification-mode-select").value'),'app');
  assert.equal(await evaluate('Array.from(document.querySelectorAll(".dynamic-section-nav button"),button=>button.getBoundingClientRect()).every(box=>box.width>=44&&box.height>=44)'),true);
  await evaluate('document.querySelectorAll(".dynamic-section-nav button")[1].click()');
  assert.equal(await evaluate('document.activeElement.id'),'dynamic-section-heading-1');
  assert.equal(await evaluate('document.documentElement.scrollWidth<=innerWidth'),true);
  assert.equal(await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].hidden'),true);
  assert.equal(await evaluate('document.querySelector(".dynamic-save-state").textContent'),localization.t('saveNone','en'),'opening saved hidden/app-only settings is not a change');
  const notificationToggleExpression='Array.from(document.querySelectorAll(".dynamic-activity-row")).find(r=>r.dataset.provider==="builtin:notifications").querySelector("input")';
  await evaluate(notificationToggleExpression+'.click()');
  let reopened=await evaluate('window.__testClay.serialize()');
  assert.equal(reopened.SHOW_ACTIVITIES.value,false);assert.equal(reopened.SHOW_NOTIFICATIONS.value,true);
  assert.equal(reopened.NOTIFICATION_APP_ONLY.value,true);assert.equal(reopened.NOTIFICATION_PREVIEW.value,true,'app-only retains saved body preference without exposing it');
  await evaluate('window.__testClay.getItemById("dynamic-notification-mode").$element[0].scrollIntoView({block:"start"})');
  assert.equal(await evaluate('document.documentElement.scrollWidth<=innerWidth'),true);await capture('reopened-app-only-320');
  await evaluate(notificationToggleExpression+'.click()');reopened=await evaluate('window.__testClay.serialize()');
  assert.equal(reopened.SHOW_NOTIFICATIONS.value,false);assert.equal(reopened.NOTIFICATION_APP_ONLY.value,true);
  await send('Emulation.setDeviceMetricsOverride',{width:360,height:800,deviceScaleFactor:1,mobile:true});
  await send('Page.navigate',{url:'file://'+pages.bounded});
  for(let i=0;i<100;i++){if(await evaluate('document.querySelectorAll(".dynamic-activity-row").length===64'))break;await new Promise(r=>setTimeout(r,50));}
  const boundedRows=await evaluate('Array.from(document.querySelectorAll(".dynamic-activity-row"),r=>r.dataset.provider)');
  assert.equal(boundedRows.length,64);
  for(const id of [timer,workout,evil,'builtin:music','builtin:notifications'])assert.ok(boundedRows.includes(id),'current controls survive 64 absent entries');
  await evaluate('(()=>{for(let i=0;i<64;i++){const row=Array.from(document.querySelectorAll(".dynamic-activity-row")).find(r=>r.dataset.provider==="builtin:music");const up=row.querySelector("[data-action=up]");if(up.disabled)break;up.click();}})()');
  const boundedResponse=await evaluate('window.__testClay.serialize()');
  assert.equal(JSON.parse(boundedResponse.__dynamicActivityPreferences.value).order[0],'builtin:music');
  const boundedHost=replayHost();boundedHost.save(boundedResponse);assert.deepEqual(boundedHost.selected(),['music','timer']);
  const boundary={savedAbsentProviderCount:64,renderedProviderCount:boundedRows.length,allInstalledControlsVisible:true,musicFirstSavedAndRestored:true,
    sectionNavigation:{labelsFromSections:true,keyboardEnter:true,focusTarget:true,noHistoryOrHashChanges:true,saveAlwaysVisible:true,touchTargets44px:true},
    reopenedSavedPrivacy:{viewportWidth:320,notificationsRemainHidden:true,appOnlyRetained:true,savedBodyPreferenceRetained:true,genericMasterRemainsOff:true,noInitialDirtyState:true,noHorizontalOverflow:true}};
  const responsive=[];
  for(const [height,textScale] of [[360,1],[800,2],[360,2]]) {
    await send('Emulation.setDeviceMetricsOverride',{width:320,height,deviceScaleFactor:1,mobile:true});
    await send('Page.navigate',{url:'file://'+pages.en});
    for(let i=0;i<100;i++){if(await evaluate('!!document.querySelector(".dynamic-activity-row")'))break;await new Promise(r=>setTimeout(r,30));}
    if(textScale===2)await evaluate('(()=>{const sizes=Array.from(document.querySelectorAll("body,body *")).filter(e=>e.getClientRects().length).map(e=>[e,parseFloat(getComputedStyle(e).fontSize)]);sizes.forEach(([e,size])=>e.style.setProperty("font-size",(size*2)+"px","important"));})()');
    await evaluate('new Promise(resolve=>setTimeout(resolve,150))');
    await evaluate('document.getElementById("dynamic-theme-select").focus()');
    await evaluate('new Promise(resolve=>setTimeout(resolve,50))');
    const layout=await evaluate('(()=>{const input=document.getElementById("dynamic-theme-select"),bar=document.querySelector(".component-submit"),r=input.getBoundingClientRect(),b=bar.getBoundingClientRect(),v=visualViewport;return {height:innerHeight,visualHeight:v.height,scrollWidth:document.documentElement.scrollWidth,width:innerWidth,position:getComputedStyle(bar).position,focusTop:r.top,focusBottom:r.bottom,visibleBottom:getComputedStyle(bar).position==="fixed"?Math.min(v.offsetTop+v.height,b.top)-12:v.offsetTop+v.height-12};})()');
    assert.ok(layout.scrollWidth<=layout.width,'enlarged text remains horizontally bounded');
    assert.ok(layout.focusTop>=0&&layout.focusBottom<=layout.visibleBottom,'focused select remains fully visible');
    if(height<=420)assert.equal(layout.position,'static','short viewports use document-flow actions');
    responsive.push({textScale,...layout});await capture('responsive-'+height+'-'+textScale+'x');
  }
  await send('Emulation.setDeviceMetricsOverride',{width:320,height:800,deviceScaleFactor:1,mobile:true});
  await send('Page.navigate',{url:'file://'+pages.en});
  for(let i=0;i<100;i++){if(await evaluate('!!document.querySelector(".dynamic-activity-row")'))break;await new Promise(r=>setTimeout(r,30));}
  await send('Emulation.setPageScaleFactor',{pageScaleFactor:2});
  await evaluate('new Promise(resolve=>setTimeout(resolve,150))');
  const visualOnly=await evaluate('({layoutHeight:innerHeight,visualHeight:visualViewport.height,position:getComputedStyle(document.querySelector(".component-submit")).position})');
  assert.ok(visualOnly.visualHeight<visualOnly.layoutHeight);assert.equal(visualOnly.position,'static');
  await send('Emulation.setPageScaleFactor',{pageScaleFactor:1});
  boundary.responsive={layouts:responsive,visualViewportOnly:visualOnly,scope:'Actual Chromium layout at short heights, doubled computed CSS font sizes and visual-viewport-only browser zoom; not a physical keyboard or OS accessibility-font test.'};
  const packaged=[];
  if(candidateScript)for(const language of ['en','cs']){
    await send('Emulation.setDeviceMetricsOverride',{width:360,height:800,deviceScaleFactor:1,mobile:true});
    await send('Page.navigate',{url:'file://'+packagedPages[language]});
    for(let i=0;i<100;i++){if(await evaluate('!!window.__testClay&&!!document.querySelector(".dynamic-activity-row")'))break;await new Promise(r=>setTimeout(r,30));}
    await evaluate('document.fonts.ready');await settle();await evaluate('new Promise(resolve=>setTimeout(resolve,150))');
    assert.equal(await evaluate('document.querySelectorAll(".dynamic-preview-tabs button").length'),6,'actual minified serialized settings function builds all six preview tabs');
    assert.equal(await evaluate('document.documentElement.scrollWidth<=innerWidth'),true);
    assert.equal(await evaluate('Boolean(window.__xss)'),false);
    assert.equal(await evaluate('document.querySelectorAll(".dynamic-activity-name img").length'),0);
    assert.deepEqual(await evaluate('Array.from(document.querySelectorAll(".dynamic-section-nav button"),button=>button.textContent)'),
      ['appearance','content','helpConnection'].map(key=>localization.t(key,language)));
    await evaluate('window.scrollTo(0,0)');await capture('packaged-default-'+language);
    const initial=await evaluate('window.__testClay.serialize()');
    await evaluate('document.querySelectorAll(".dynamic-preview-tabs button")[2].click()');await settle();
    assert.deepEqual(await evaluate('window.__testClay.serialize()'),initial,'packaged sample controls preserve saved settings');
    const notification='Array.from(document.querySelectorAll(".dynamic-activity-row")).find(row=>row.dataset.provider==="builtin:notifications").querySelector("input")';
    await evaluate(notification+'.click()');
    assert.equal((await evaluate('window.__testClay.serialize()')).SHOW_NOTIFICATIONS.value,true);
    await evaluate(notification+'.click()');
    const resize=[];
    for(const target of ['window','visualViewport']){
      if(target==='visualViewport'&&!await evaluate('!!window.visualViewport'))continue;
      await evaluate('document.activeElement.blur()');await settle();
      const counts=await evaluate(`(async()=>{
        const bar=document.querySelector('.component-submit'),style=document.documentElement.style;
        const originalRect=bar.getBoundingClientRect,originalSet=style.setProperty,originalToggle=document.body.classList.toggle;
        const result={events:40,measurements:0,heightWrites:0,flowWrites:0};
        bar.getBoundingClientRect=function(){++result.measurements;return originalRect.apply(this,arguments);};
        style.setProperty=function(name){if(name==='--dynamic-save-height')++result.heightWrites;return originalSet.apply(this,arguments);};
        document.body.classList.toggle=function(name){if(name==='dynamic-actions-flow')++result.flowWrites;return originalToggle.apply(this,arguments);};
        for(let i=0;i<40;++i)${target}.dispatchEvent(new Event('resize'));
        await new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
        bar.getBoundingClientRect=originalRect;style.setProperty=originalSet;document.body.classList.toggle=originalToggle;
        return result;
      })()`);
      assert.equal(counts.measurements,1,target+' resize burst performs one actual fixed-footer measurement');
      assert.equal(counts.heightWrites,0,'unchanged measured height does not write CSS');
      assert.equal(counts.flowWrites,0,'unchanged flow mode does not write its class');
      resize.push({target,...counts});
    }
    await evaluate('window.__testClay.getItemByMessageKey("CLOCK_FORMAT").set(24);document.querySelectorAll(".dynamic-preview-tabs button")[2].click()');
    await settle();await evaluate('new Promise(resolve=>setTimeout(resolve,100))');
    const preview=await evaluate(`(async()=>{
      const prototype=CanvasRenderingContext2D.prototype,draw=prototype.drawImage,put=prototype.putImageData;
      const counts={changes:40,imageDraws:0,recolors:0};
      prototype.drawImage=function(){if(this.canvas.id==='dynamic-preview-canvas')++counts.imageDraws;return draw.apply(this,arguments);};
      prototype.putImageData=function(){if(this.canvas.id==='dynamic-preview-canvas')++counts.recolors;return put.apply(this,arguments);};
      for(let i=0;i<40;++i)window.__testClay.getItemByMessageKey('ACCENT_COLOR').set(i%2?0xAA55FF:0xFFAA55);
      await new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
      prototype.drawImage=draw;prototype.putImageData=put;
      return counts;
    })()`);
    assert.equal(preview.imageDraws,1,'rapid palette input draws the final native sample once');
    assert.equal(preview.recolors,1,'rapid palette input recolors once');
    await capture('packaged-custom-preview-'+language);
    const comparisons=[];
    if(language==='en'){
      const assets=require(root+'/src/pkjs/settings-preview');
      await evaluate('window.__testClay.getItemByMessageKey("BACKGROUND_COLOR").set(0x0055AA);window.__testClay.getItemByMessageKey("TEXT_COLOR").set(0xFFFFAA);window.__testClay.getItemByMessageKey("ACCENT_COLOR").set(0xAA55FF)');
      for(const format of [12,24])for(const [index,state]of ['idle','music','timer','notification-summary','notification','timeline'].entries()){
        const frame=assets.frames[state+'-'+format+'-light'];assert.ok(frame);
        await evaluate('window.__testClay.getItemByMessageKey("CLOCK_FORMAT").set('+format+');document.querySelectorAll(".dynamic-preview-tabs button")['+index+'].click()');
        await settle();await evaluate('new Promise(resolve=>setTimeout(resolve,80))');
        const same=await evaluate(`(async()=>{
          const frame=${JSON.stringify(frame)},source=typeof frame==='string'?{src:frame}:frame;
          const image=new Image();image.src=source.src;await image.decode();
          const reference=document.createElement('canvas');reference.width=200;reference.height=228;
          const ctx=reference.getContext('2d');ctx.drawImage(image,0,0);
          (new Function('ctx','frame','foreground','background','highlight',${JSON.stringify(referenceRecolor)}))(ctx,source,0xFFFFAA,0x0055AA,0xAA55FF);
          const expected=ctx.getImageData(0,0,200,228).data;
          const actual=document.getElementById('dynamic-preview-canvas').getContext('2d').getImageData(0,0,200,228).data;
          let different=0;for(let i=0;i<expected.length;++i)if(actual[i]!==expected[i])++different;
          return {differentChannels:different,pixels:45600};
        })()`);
        assert.equal(same.differentChannels,0,'optimized recoloring matches the actual approved old implementation: '+state+'/'+format);
        comparisons.push({state,format,...same});
      }
    }
    await send('Emulation.setEmulatedMedia',{features:[{name:'prefers-color-scheme',value:'dark'}]});
    await evaluate('window.__testClay.getItemByMessageKey("ACCENT_COLOR").set(0x55AAFF);(()=>{const theme=document.getElementById("dynamic-theme-select");theme.value="dark";theme.dispatchEvent(new Event("change",{bubbles:true}));})();document.querySelectorAll(".dynamic-preview-tabs button")[2].click();window.scrollTo(0,0)');
    await settle();await capture('packaged-dark-'+language);
    await send('Emulation.setEmulatedMedia',{features:[{name:'prefers-color-scheme',value:'light'}]});
    packaged.push({...packagedMetadata[language],resize,preview,referenceRecolor:{baseline:baselineRef,baselineSourceSha256:crypto.createHash('sha256').update(baselineSource).digest('hex'),comparisons},
      checks:['actual minified PKJS executed','actual showConfiguration generated HTML','unchanged minified customFn serialized by actual Clay','six previews and translated sections built','preview selection leaves settings unchanged','Notifications toggle serializes','resize events batched','unchanged footer writes suppressed','rapid palette changes draw and recolor once'].concat(comparisons.length?['all twelve custom-color state/format pixel arrays match the actual approved old algorithm']:[])});
  }
  const sourceSha256=Object.fromEntries(['src/pkjs/config.js','src/pkjs/index.js','src/pkjs/settings-page.js','src/pkjs/localization.js','src/pkjs/activity-presentation.js','vendor/ACTIVITIES.json','scripts/check-settings-activities.cjs'].map(file=>[file,crypto.createHash('sha256').update(fs.readFileSync(path.join(root,file))).digest('hex')]));
  fs.writeFileSync(path.join(output,'validation.json'),JSON.stringify({version,actualClay:true,syntheticFixtures:true,viewport:{width:360,height:800},sourceSha256,results,boundary,packaged,errors:events.filter(e=>e.method==='Runtime.exceptionThrown')},null,2)+'\n');
  assert.equal(events.filter(e=>e.method==='Runtime.exceptionThrown').length,0);
  socket.close();fs.rmSync(scratch,{recursive:true,force:true});console.log(JSON.stringify({passed:true,output,languages:results.map(result=>result.language)},null,2));
}
run().catch(error=>{console.error(error);process.exit(1);});
