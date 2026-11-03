'use strict';

// Node-only capture fixture producer. Use the installed production encoders;
// this module is never imported by the phone or watch application.
const core=require('pebble-watchface-core').wire;
const library=require('pebble-activities');
const activity=library.view;
const simple=library.activity;
const presentation=require('../src/pkjs/activity-presentation');
if(!activity||!simple)throw Error('Native fixtures require the pinned Activities 0.3.0 package');
const states=['idle','music','notification-summary','notification','timeline'];
function cover() {
  const palette=[0xc0,0xff,0xc3,0xcf,0xfc,0xf3,0xf0,0xcc],pixels=[];
  function color(x,y) {
    const stripe=Math.floor(x/10);
    if(x>=13&&x<28&&y>=9&&y<32)return (x-20)*(x-20)+(y-20)*(y-20)<95?4:stripe+2;
    return stripe+2;
  }
  for(let y=0;y<40;y++)for(let x=0;x<40;x+=2)pixels.push((color(x,y)<<4)|color(x+1,y));
  return {width:40,height:40,palette,pixels,image:true};
}
// Exercise the actual production Music adapter, including its optional-field
// rules. Handwritten clean cards would miss real Track/Artist regressions.
function musicFromSource(now,session,title,artist,artwork) {
  let request;
  const provider='7f635d12-7288-4d10-885d-41ea49f4d5a4';
  const client=library.createContext({musicAsActivity:true,
    pebble:{subscribeToSource:value=>{if(value.category==='music')request=value;return {unsubscribe(){}};}},
    keys:{CONTEXT_DATA:'CONTEXT_DATA',REQUEST_CONTEXT:'REQUEST_CONTEXT'},now:()=>now*1000,
    later:()=>1,cancelLater(){},send:(_payload,ok)=>ok(),
    getSettings:()=>({showMusic:true,showStatus:false,showNotifications:false})});
  client.refresh(true,session);
  const properties={title:{shortText:{text:title}},artist:{shortText:{text:artist}},playing:{boolean:{value:true}}};
  if(artwork)properties.artwork={image:{width:artwork.width,height:artwork.height,
    palette:Buffer.from(artwork.palette).toString('base64'),pixels:Buffer.from(artwork.pixels).toString('base64')}};
  request.onData({pluginUuid:provider,validUntilMs:(now+120)*1000,instances:[{instanceId:'track',properties}]});
  const card=client.musicActivity();client.stop();
  if(!card)throw Error('Production Music adapter rejected native fixture');
  return card;
}
function fixtures(options) {
  const now=options.now,session=options.session;
  if(!Number.isInteger(now)||now<1||!Number.isInteger(session)||session<2)throw Error('Expected UTC seconds and native session >=2');
  const ready=(text,seconds=120)=>({status:2,text,confirmedAt:now,validUntil:now+seconds});
  const temperature=Object.assign(ready('5°C',7200),{semantic:{kind:core.SEMANTIC_KIND.TEMPERATURE,
    source:core.SOURCE_KIND.WEATHER,value:5,unit:'°C',label:'TEMP',detail:core.DETAIL_KIND.TEMPERATURE,observedAt:now}});
  const rain=Object.assign(ready('40%',7200),{displayKind:1,semantic:{kind:core.SEMANTIC_KIND.PRECIPITATION,
    source:core.SOURCE_KIND.WEATHER,value:40,min:0,max:100,unit:'%',label:'RAIN',detail:core.DETAIL_KIND.PRECIPITATION,observedAt:now}});
  const presets=[];
  let sequence=0;
  for(const theme of ['light','dark'])for(const format of [24,12])for(const state of states) {
    const slots=[];
    let music=null;
    if(state==='music'||state==='notification-summary') {
      if(options.legacyMusic) {
        slots[0]=Object.assign(ready('Flower In A Hurricane [from the soundtrack]'),{bitmap:cover()});
        slots[1]=ready('Justice, Tame Impala');
      } else {
        music=presentation.project(musicFromSource(now,session,
          'Flower In A Hurricane [from the soundtrack]','Justice, Tame Impala',cover()));
      }
    }
    if(state==='notification'||state==='notification-summary') {
      slots[2]=ready('Xef',8);
      if(state==='notification')slots[3]=ready('See you at the station!',8);
    }
    const dark=theme==='dark';
    presets.push({name:`${state}-${format}-${theme}`,state,format,theme,
      preserve:state==='music'||state==='notification-summary'?
        [{x:10,y:options.legacyMusic?176:state==='music'?92:116,
          width:options.legacyMusic?40:24,height:options.legacyMusic?40:24}]:
        state==='timeline'?[{x:0,y:171,width:200,height:57}]:[],
      integers:{BACKGROUND_COLOR:dark?0:0xffffff,TEXT_COLOR:dark?0xffffff:0,
        ACCENT_COLOR:0x55aaff,ANIMATIONS:0,CLOCK_FORMAT:format,SHOW_STATUS:1,
        NOTIFICATION_VIBRATION:0,NIGHT_PALETTE:0,REQUEST_CONTEXT:session},
      bytes:{COMPLICATION_DATA:core.packComplications([temperature,rain],now),
        CONTEXT_DATA:core.packComplications(slots,now),
        ACTIVITY_DATA:activity.encode(music?[music]:[],{session,revision:++sequence,sentAt:now})}});
  }
  return {coreVersion:require('pebble-watchface-core/package.json').version,
    activitiesVersion:require('pebble-activities/package.json').version,
    activityWireVersion:activity.VERSION,session,now,presets};
}
function activityPresets(options) {
  const plan=fixtures({...options,legacyMusic:true}),now=options.now,session=options.session;
  const provider='46f0a5c9-48ce-4ecb-87b7-c1510088a3d2';
  const timer=(revision,state='active',seconds=522,extra={})=>({provider:'50e9ef12-a5ab-4369-a3b5-3447bfba5151',providerName:'Timer',id:'sample-pasta',kind:'timer',revision,
    state,clock:'deadline',observedAt:now,validUntil:now+Math.max(3600,seconds+60),private:false,
    unconfirmed:true,main:'',footer:'10 min timer',totalDuration:600,
    ...(state==='paused'?{duration:seconds}:{deadline:now+seconds}),...extra});
  const clock=(label,mode,baseSeconds,extra={})=>({type:'clock',label,mode,baseSeconds,
    running:true,anchorAt:now,retain:true,...extra});
  const card=(id,caption,main,support=null,extra={})=>({provider,id,providerName:/printer/.test(id)?'Printer':/glucose|unicode/.test(id)?'Glucose':/charge/.test(id)?'Charger':/build/.test(id)?'Build':/timer|tea|rest/.test(id)?'Timer':'Workout',kind:id,revision:1,
    state:'active',observedAt:now,validUntil:now+3600,private:false,
    caption,main,support,...extra});
  const music=card('sample-music','Music',{type:'text',label:'Music',text:'One More Time'},null,
    {provider:'7f635d12-7288-4d10-885d-41ea49f4d5a4',providerName:'Music',selectionKey:'builtin:music',
      kind:'music',private:true,image:cover()});
  const live=(id,appName,content,detail=null,extra={})=>{
    const source={provider,id,revision:1,state:'active',observedAt:now,validUntil:now+3600,
      private:false,appName,content,detail,...extra};
    const instance=simple.toSourceInstance(source,{iconPixelSize:{w:40,h:40}});
    const decoded=instance&&simple.sourceCard(instance,source.provider);
    const compiled=decoded&&simple.toView(decoded);
    if(!compiled)throw Error('Invalid simple API native fixture: '+id);
    return compiled;
  };
  const expires=card('sample-support-expiry','Outdoor walk',clock('Elapsed','elapsed',1830),
    {type:'value',label:'Pulse',value:142,unit:'bpm',validUntil:now+3},
    {meter:{kind:'level',source:'support',min:60,max:180}});
  const specifications=[
    {name:'timer-running',base:'music',card:timer(1)},
    {name:'timer-paused',base:'idle',card:timer(2,'paused')},
    {name:'glucose-numeric',base:'idle',card:{provider,providerName:'Glucose',id:'sample-glucose',kind:'glucose',revision:1,
      state:'active',clock:'none',observedAt:now,validUntil:now+120,private:true,
      value:6.2,unit:'mmol/L',main:'6.2 mmol/L',footer:'Glucose'}},
    {name:'elapsed',base:'idle',card:{provider,providerName:'Workout',id:'sample-walk',kind:'workout',revision:1,
      state:'active',clock:'elapsed',observedAt:now,validUntil:now+3600,private:false,
      duration:1830,clockAt:now,main:'',footer:'Outdoor walk'}},
    {name:'timer-heading-banner',base:'notification-summary',card:timer(3)},
    {name:'timer-timeline',base:'timeline',card:timer(4)},
    {name:'timer-long',base:'music',card:timer(5,'active',4496,{footer:'2 h timer'})},
    {name:'timer-24-hours',base:'idle',card:timer(6,'active',86400,{footer:'24 h timer'})},
    {name:'timer-secondary',base:'idle',cards:[timer(7),
      timer(1,'active',1040,{id:'sample-second-timer',footer:'20 min timer'})]},
    {name:'timer-paused-music',base:'music',card:timer(8,'paused')},
    {name:'timer-large-text',base:'music',largeText:true,card:timer(9)},
    {name:'timer-long-heading-banner',base:'notification-summary',card:timer(10,'active',4496,{footer:'2 h timer'})},
    {name:'clock-with-image',base:'idle',card:timer(11,'active',522,{image:cover()})},
    {name:'clock-long-footer',base:'idle',card:timer(12,'active',522,
      {footer:'Dlouhý časovač s diakritikou a dalšími informacemi pro druhý řádek'})},
    {name:'timer-dark',base:'music',theme:'dark',card:timer(13)},
    {name:'timer-due',base:'idle',card:timer(14,'active',0,{footer:'10 min timer'})},
    {name:'timer-long-timeline',base:'timeline',card:timer(15,'active',4496,{footer:'2 h timer'})},
    {name:'value-eta-artwork-meter',base:'idle',card:card('sample-printer','Boat print',
      {type:'value',label:'Complete',value:72,unit:'%'},clock('ETA','deadline',2220,{estimated:true}),
      {image:cover(),meter:{kind:'progress',source:'main',min:0,max:100}})},
    {name:'two-independent-clocks',base:'idle',verifyClockTick:true,
      card:card('sample-interval','Interval run',clock('Rest','deadline',80),clock('Elapsed','elapsed',1458))},
    {name:'paused-with-running-wall-clock',base:'idle',verifyClockTick:true,
      card:card('sample-paused-session','Paused workout',
        clock('Active','elapsed',1458,{running:false,anchorAt:0}),clock('Wall','elapsed',1800),{state:'paused'})},
    {name:'cached-public-value-age',base:'idle',card:card('sample-cached-charge','Charger',
      {type:'value',label:'Charge',value:72,unit:'%',retain:true},null,
      {observedAt:now-300,unconfirmed:true})},
    {name:'support-expiry-before',base:'idle',card:expires,
      expectInk:[{x:10,y:195,width:180,height:4}]},
    {name:'support-expiry-after',base:'idle',captureDelaySeconds:4,card:expires,
      expectBlank:[{x:10,y:195,width:180,height:4}]},
    {name:'completed-result',base:'idle',card:card('sample-finished-timer','10 min timer',
      {type:'text',label:'State',text:'Finished',retain:true},
      clock('Elapsed','elapsed',600,{running:false,anchorAt:0}),
      {kind:'timer',state:'completed',unconfirmed:true,showTerminal:true,validUntil:now+60})},
    {name:'clock-primary-clock-secondary',base:'idle',verifyClockTick:true,cards:[
      card('sample-walk-primary','Outdoor walk',clock('Elapsed','elapsed',1830)),
      card('sample-tea-secondary','Tea',clock('Left','deadline',240))]},
    {name:'text-primary-running-secondary',base:'idle',verifyClockTick:true,cards:[
      card('sample-build-primary','Firmware build',{type:'text',label:'Stage',text:'Verifying'}),
      card('sample-rest-secondary','Pasta',clock('Left','deadline',80),null,{kind:'timer',unconfirmed:true})]},
    {name:'unicode-value-body-fallback',base:'idle',card:card('sample-value-unicode','Měření glukózy',
      {type:'value',label:'Glukóza',value:6.2,unit:'mmol/L'},null,{private:true})},
    {name:'value-eta-artwork-meter-dark',base:'idle',theme:'dark',card:card('sample-printer-dark','Boat print',
      {type:'value',label:'Complete',value:72,unit:'%'},clock('ETA','deadline',2220,{estimated:true}),
      {image:cover(),meter:{kind:'progress',source:'main',min:0,max:100}})},
    {name:'timer-one-minute',base:'idle',card:timer(16,'active',36,{footer:'1 min timer',totalDuration:60})},
    {name:'matching-item-provenance',base:'idle',card:card('sample-matching-provenance','Interval run',
      clock('Rest','deadline',600,{anchorAt:now-70}),clock('Elapsed','elapsed',1458,{anchorAt:now-70}),
      {kind:'workout',observedAt:now-70,unconfirmed:true})},
    {name:'different-item-provenance',base:'idle',card:card('sample-different-provenance','Interval run',
      clock('Rest','deadline',600,{anchorAt:now-70}),
      clock('Elapsed','elapsed',1458,{anchorAt:now-300,observedAt:now-300}),
      {kind:'workout',observedAt:now-70,unconfirmed:true})},
    {name:'completed-result-music',base:'music',card:card('sample-result-with-music','10 min timer',
      {type:'text',label:'State',text:'Finished',retain:true},null,
      {kind:'timer',state:'completed',unconfirmed:true,showTerminal:true,validUntil:now+60})},
    {name:'ongoing-result-music',base:'music',cards:[
      card('sample-ongoing-with-result','Outdoor walk',clock('Elapsed','elapsed',1830),null,{kind:'workout'}),
      card('sample-result-secondary','10 min timer',{type:'text',label:'State',text:'Finished',retain:true},null,
        {kind:'timer',state:'completed',unconfirmed:true,showTerminal:true,validUntil:now+60})]},
    {name:'timer-acknowledged-idle',base:'idle',card:timer(17,'completed',0,
      {clock:'none',showTerminal:false,deadline:0,validUntil:now+60})},
    {name:'timer-acknowledged-music',base:'music',card:timer(18,'completed',0,
      {clock:'none',showTerminal:false,deadline:0,validUntil:now+60})},
    {name:'timer-primary-music-secondary',base:'idle',verifyClockTick:true,cards:[
      timer(19,'active',25,{id:'sample-ordered-timer'}),music]},
    {name:'music-primary-timer-secondary',base:'idle',verifyClockTick:true,cards:[
      music,timer(20,'active',25,{id:'sample-ordered-timer'})]},
    {name:'two-identified-timers',base:'idle',cards:[
      timer(21,'active',25,{id:'sample-new-pasta'}),
      timer(22,'active',1040,{id:'sample-tea'})]},
    {name:'delivery-primary-timer-secondary',base:'idle',cards:[
      card('parcel','Delivery',{type:'text',label:'Delivery',text:'At your door'},null,
        {kind:'delivery',providerName:'Parcel'}),timer(23,'active',25,{id:'sample-delivery-timer'})]},
    {name:'support-yields-secondary',base:'idle',cards:[
      card('workout','Intervals',clock('Rest','deadline',80),clock('Elapsed','elapsed',1458),
        {kind:'workout',providerName:'Workout'}),music]},
    {name:'secondary-long-clock',base:'idle',cards:[music,
      card('long-clock','Long session',clock('Elapsed','elapsed',4294967295),null,
        {providerName:'Workout'})]},
    {name:'secondary-cached',base:'idle',cards:[music,
      timer(24,'active',25,{id:'sample-cached-secondary-timer',observedAt:now-70,unconfirmed:true})]},
    {name:'small-scalar-icon-spacing',base:'idle',card:card('small-scalar','Counter',
      {type:'value',label:'Items',text:'123456789012345678'},null,
      {providerName:'Counter',image:cover()}),
      expectBlank:[{x:10,y:116,width:180,height:4}],expectInk:[{x:10,y:145,width:180,height:22}]},
    {name:'long-provider-identity',base:'idle',card:card('provider-name','Instance',
      {type:'text',label:'Status',text:'Ready'},null,
      {providerName:'Dlouhý název aplikace s diakritikou',kind:'custom'})},
    {name:'simple-music-user-title',base:'idle',card:musicFromSource(now,session,
      'Better Than The Cyberpunk Soundtrack','?',null)},
    {name:'simple-music-user-title-dark',base:'idle',theme:'dark',card:musicFromSource(now,session,
      'Better Than The Cyberpunk Soundtrack','?',null)},
    {name:'simple-music-title-artist-artwork',base:'idle',card:musicFromSource(now,session,
      'Better Than The Cyberpunk Soundtrack','The Midnight',cover())},
    {name:'simple-timer-normal-delivery-compact',base:'idle',verifyClockTick:true,cards:[
      live('simple-countdown','Timer',simple.countdown({deadline:now+25}),null,
        {provider:'50e9ef12-a5ab-4369-a3b5-3447bfba5151',kind:'timer',unconfirmed:true}),
      live('simple-parcel-compact','Parcel','At your door')]},
    {name:'simple-delivery-normal-timer-compact',base:'idle',verifyClockTick:true,cards:[
      live('simple-parcel-normal','Parcel','At your door'),
      live('simple-countdown-secondary','Timer',simple.countdown({deadline:now+25}),null,
        {provider:'50e9ef12-a5ab-4369-a3b5-3447bfba5151',kind:'timer',unconfirmed:true})]},
    {name:'simple-workout-detail-clock',base:'idle',verifyClockTick:true,card:
      live('simple-workout','Workout',simple.countdown({deadline:now+80,label:'Rest'}),
        simple.elapsed({since:now-1458,label:'Elapsed'}),{kind:'workout'})},
    {name:'simple-value-detail',base:'idle',card:
      live('simple-charge','Charger',simple.value(72,'%'),simple.countdown({deadline:now+2220,label:'ETA'}))},
    {name:'simple-unknown-app',base:'idle',card:
      live('simple-custom','Acme','Ready for collection','Locker 12')}
  ];
  let sequence=options.sequence===undefined?100:options.sequence;
  return {coreVersion:plan.coreVersion,activitiesVersion:plan.activitiesVersion,
    activityWireVersion:activity.VERSION,session,now,
    presets:specifications.map(spec=>{
      const base=plan.presets.find(p=>p.name===`${spec.base}-24-${spec.theme||'light'}`);
      const cards=(spec.cards||[spec.card]).map(source=>{
        const result=source.main&&typeof source.main==='object'?activity.normalize(source):
          activity.fromLegacy(source,{terminalResults:true,timerPresentation:'compact'});
        if(!result)throw Error('Invalid native activity fixture: '+spec.name);
        return presentation.project({...result,providerName:source.providerName,
          selectionKey:source.selectionKey,sourceFormat:source.sourceFormat});
      });
      return {...base,name:spec.name,activity:cards[0],activities:cards,
        captureDelaySeconds:spec.captureDelaySeconds,verifyClockTick:spec.verifyClockTick===true,
        expectInk:spec.expectInk||[],expectBlank:spec.expectBlank||[],
        // Appearance settings are persistent on the watch. Reset the optional
        // large setting for every sample so captures are order-independent.
        integers:{...base.integers,LARGE_TEXT:spec.largeText?1:0},
        preserve:base.state==='timeline'?[{x:0,y:171,width:200,height:57}]:[],
        bytes:{...base.bytes,ACTIVITY_DATA:activity.encode(cards,{session,revision:++sequence,sentAt:now})}};
    })};
}
module.exports={fixtures,activityPresets,musicFromSource};
if(require.main===module) {
  let input='';process.stdin.setEncoding('utf8');process.stdin.on('data',value=>input+=value);
  process.stdin.on('end',()=>{
    const options=JSON.parse(input);
    process.stdout.write(JSON.stringify(options.mode==='activities'?activityPresets(options):fixtures(options)));
  });
}
