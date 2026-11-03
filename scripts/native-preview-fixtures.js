'use strict';

// Node-only capture fixture producer. Use the installed production encoders;
// this module is never imported by the phone or watch application.
const core=require('pebble-watchface-core').wire;
const library=require('pebble-activities');
const activity=library.summaryView;
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
  let sequence=options.sequence||0;
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
    if(music)music.revision=sequence+1;
    const dark=theme==='dark';
    presets.push({name:`${state}-${format}-${theme}`,state,format,theme,fixtureNow:now,
      preserve:state==='music'||state==='notification-summary'?
        [{x:10,y:176,width:40,height:40}]:
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
function summaryPresets(options){
  const now=options.now,session=options.session;
  const base=fixtures(options),summary=library.summary;
  const T='50e9ef12-a5ab-4369-a3b5-3447bfba5151',P='46f0a5c9-48ce-4ecb-87b7-c1510088a3d2';
  const timer=(seconds=151,name='Tea',id='tea')=>({id,type:'standard',name,icon:'timer',headline:{type:'clock',mode:'countdown',deadline:now+seconds},openApp:{watchAppUuid:T}});
  const music={id:'music-session',type:'media',title:'Better Than The Cyberpunk Soundtrack',artist:'The Midnight',artwork:cover(),itemId:'track-1',playback:'playing',openApp:{watchAppUuid:'1f03293d-47af-4f28-b960-f2b02a6dd757'}};
  const recording={id:'recording',type:'standard',name:'Voice recording',icon:'recording',headline:{type:'clock',mode:'elapsed',since:now-728}};
  const delivery={id:'delivery',type:'standard',name:'Food delivery',icon:'delivery',headline:{type:'value',value:8,unit:'min',label:'Arriving in'}};
  const specs=[
    {name:'timer-media',records:[timer(),music],verifyClockTick:true},
    {name:'media-timer',records:[music,timer()],verifyClockTick:true},
    {name:'workout-timer',records:[{id:'run',type:'standard',name:'Outdoor run',icon:'workout',headline:{type:'value',value:5.24,unit:'km'},detail:{type:'value',value:'5:12',unit:'/km'}},timer(45,'Rest','rest')],verifyClockTick:true},
    {name:'navigation-media',records:[{id:'route',type:'standard',name:'Maps · Home',icon:'navigation',headline:{type:'value',value:200,unit:'m',symbol:'turn-right'},detail:{type:'text',text:'Oak Street'}},music]},
    {name:'recording-delivery',records:[recording,delivery],verifyClockTick:true},
    {name:'download-recording',records:[{id:'download',type:'standard',name:'Offline map',icon:'download',headline:{type:'value',value:68,unit:'%',range:{max:100}}},recording],verifyClockTick:true},
    {name:'message-timer',records:[{id:'message',type:'standard',name:'Messages',icon:'message',headline:{type:'text',text:'Petr'},detail:{type:'text',text:'I’m downstairs. Ready when you are?'}},timer()],verifyClockTick:true},
    {name:'media-alone',records:[music]},
    {name:'timer-alone',records:[timer(204)],verifyClockTick:true},
    {name:'long-timer',records:[timer(45296,'Slow cook'),music],verifyClockTick:true},
    {name:'long-artist',records:[{...music,title:'Flower In A Hurricane [from the soundtrack]',artist:'Justice, Tame Impala'},timer(85,'Pasta')],verifyClockTick:true},
    {name:'delivery-text',records:[delivery,{id:'garage',type:'standard',name:'Garage',headline:{type:'text',text:'Door open'}}]},
    {name:'local-promotion',records:[timer(2),music],captureDelaySeconds:3,expireFirst:true},
    {name:'compact-detail-clock',records:[music,{id:'match',type:'standard',name:'Match',icon:'workout',headline:{type:'value',value:'2–1'},detail:{type:'clock',mode:'elapsed',since:now-2800},summary:'detail'}],verifyClockTick:true},
    {name:'secondary-long-clock',records:[music,{id:'long',type:'standard',name:'Session',headline:{type:'clock',mode:'elapsed',seconds:4294967295}}]},
    {name:'secondary-unfit-value',records:[music,{id:'unfit',type:'standard',name:'Counter',headline:{type:'value',value:'9'.repeat(64),unit:'km'}}]},
    {name:'direction-long-value',records:[{id:'long-route',type:'standard',name:'Maps',icon:'navigation',headline:{type:'value',value:'1234567890123',unit:'mi',symbol:'turn-left'}}]},
    {name:'direction-formatted-value',records:[{id:'fraction-route',type:'standard',name:'Maps',icon:'navigation',headline:{type:'value',value:'¼',unit:'mi',symbol:'turn-right'}}]}
  ];
  let sequence=options.sequence||100;
  const presets=[];
  for(const theme of ['light','dark'])for(const spec of specs){
    const template=base.presets.find(p=>p.name==='idle-24-'+theme);
    const cards=spec.records.map((record,index)=>{
      const source={provider:record.icon==='timer'?T:P,record,revision:sequence+1,observedAt:now,
        validUntil:spec.expireFirst&&index===0?now+2:now+Math.max(3600,record.headline&&record.headline.deadline-now+60||0),
        private:record.type==='media',displayName:record.type==='media'?'Music':'Activity'};
      const result=summary.toView(source);
      if(!result)throw Error('Invalid summary fixture '+spec.name+' '+index);
      return presentation.project(result);
    });
    const seq=++sequence;
    presets.push({...template,name:spec.name+'-'+theme,state:spec.name,theme,
      activity:cards[0],activities:cards,verifyClockTick:spec.verifyClockTick===true,
      captureDelaySeconds:spec.captureDelaySeconds||0,viewSequence:seq,preserve:[],
      bytes:{...template.bytes,ACTIVITY_DATA:activity.encode(cards,{session,revision:seq,sentAt:now})}});
  }
  return {coreVersion:base.coreVersion,activitiesVersion:base.activitiesVersion,activityWireVersion:3,session,now,presets};
}
function activityPresets(options){return summaryPresets(options);}
module.exports={fixtures,activityPresets,summaryPresets,musicFromSource};
if(require.main===module) {
  let input='';process.stdin.setEncoding('utf8');process.stdin.on('data',value=>input+=value);
  process.stdin.on('end',()=>{
    const options=JSON.parse(input);
    process.stdout.write(JSON.stringify(options.mode==='activities'||options.mode==='summary'?summaryPresets(options):fixtures(options)));
  });
}
