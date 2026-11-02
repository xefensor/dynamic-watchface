'use strict';

// Node-only capture fixture producer. Use the installed production encoders;
// this module is never imported by the phone or watch application.
const core=require('pebble-watchface-core').wire;
const activity=require('pebble-activities').wire;
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
    if(state==='music'||state==='notification-summary') {
      slots[0]=Object.assign(ready('Flower In A Hurricane [from the soundtrack]'),{bitmap:cover()});
      slots[1]=ready('Justice, Tame Impala');
    }
    if(state==='notification'||state==='notification-summary') {
      slots[2]=ready('Xef',8);
      if(state==='notification')slots[3]=ready('See you at the station!',8);
    }
    const dark=theme==='dark';
    presets.push({name:`${state}-${format}-${theme}`,state,format,theme,
      preserve:state==='music'||state==='notification-summary'?[{x:10,y:176,width:40,height:40}]:
        state==='timeline'?[{x:0,y:171,width:200,height:57}]:[],
      integers:{BACKGROUND_COLOR:dark?0:0xffffff,TEXT_COLOR:dark?0xffffff:0,
        ACCENT_COLOR:0x55aaff,ANIMATIONS:0,CLOCK_FORMAT:format,SHOW_STATUS:1,
        NOTIFICATION_VIBRATION:0,NIGHT_PALETTE:0,REQUEST_CONTEXT:session},
      bytes:{COMPLICATION_DATA:core.packComplications([temperature,rain],now),
        CONTEXT_DATA:core.packComplications(slots,now),
        ACTIVITY_DATA:activity.encode([],{session,revision:++sequence,sentAt:now})}});
  }
  return {coreVersion:require('pebble-watchface-core/package.json').version,
    activitiesVersion:require('pebble-activities/package.json').version,session,now,presets};
}
function activityPresets(options) {
  const plan=fixtures(options),now=options.now,session=options.session;
  const provider='46f0a5c9-48ce-4ecb-87b7-c1510088a3d2';
  const timer=(revision,state='active')=>({provider,id:'sample-pasta',kind:'timer',revision,
    state,clock:'deadline',observedAt:now,validUntil:now+3600,private:false,main:'',footer:'Pasta',
    ...(state==='paused'?{duration:522}:{deadline:now+522})});
  const specifications=[
    {name:'timer-running',base:'music',card:timer(1)},
    {name:'timer-paused',base:'idle',card:timer(2,'paused')},
    {name:'glucose-numeric',base:'idle',card:{provider,id:'sample-glucose',kind:'glucose',revision:1,
      state:'active',clock:'none',observedAt:now,validUntil:now+120,private:true,
      value:6.2,unit:'mmol/L',main:'6.2 mmol/L',footer:'Glucose'}},
    {name:'elapsed',base:'idle',card:{provider,id:'sample-walk',kind:'workout',revision:1,
      state:'active',clock:'elapsed',observedAt:now,validUntil:now+3600,private:false,
      duration:1830,clockAt:now,main:'',footer:'Outdoor walk'}},
    {name:'timer-heading-banner',base:'notification-summary',card:timer(3)},
    {name:'timer-timeline',base:'timeline',card:timer(4)}
  ];
  let sequence=options.sequence===undefined?100:options.sequence;
  return {coreVersion:plan.coreVersion,activitiesVersion:plan.activitiesVersion,session,now,
    presets:specifications.map(spec=>{
      const base=plan.presets.find(p=>p.name===`${spec.base}-24-light`);
      return {...base,name:spec.name,activity:spec.card,
        bytes:{...base.bytes,ACTIVITY_DATA:activity.encode([spec.card],{session,revision:++sequence,sentAt:now})}};
    })};
}
module.exports={fixtures,activityPresets};
if(require.main===module) {
  let input='';process.stdin.setEncoding('utf8');process.stdin.on('data',value=>input+=value);
  process.stdin.on('end',()=>{
    const options=JSON.parse(input);
    process.stdout.write(JSON.stringify(options.mode==='activities'?activityPresets(options):fixtures(options)));
  });
}
