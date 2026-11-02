'use strict';

var library=require('pebble-activities');
var view=library.view,wire=library.wire,selection=library.selection;
var TIMER='50e9ef12-a5ab-4369-a3b5-3447bfba5151';
var names={timer:'Timer',music:'Music',workout:'Workout',navigation:'Navigation',notification:'Notification',notifications:'Notifications'};

// Phone-composed 40px standard artwork keeps the native packet and renderer
// independent of plugin kinds. Native tinting keeps single-ink icons legible.
function icon(kind){
  var size=40,pixels=[],i;
  for(i=0;i<size*size/2;i++){pixels.push(0);}
  function dot(x,y,value){x=Math.round(x);y=Math.round(y);if(x<0||x>=size||y<0||y>=size){return;}var at=y*20+(x>>1);pixels[at]=x&1?(pixels[at]&240)|value:(pixels[at]&15)|(value<<4);}
  function circle(cx,cy,r,value){for(var y=Math.floor(cy-r);y<=cy+r;y++){for(var x=Math.floor(cx-r);x<=cx+r;x++){if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=r*r){dot(x,y,value);}}}}
  function line(x1,y1,x2,y2,width){var n=Math.max(Math.abs(x2-x1),Math.abs(y2-y1));for(var p=0;p<=n;p++){circle(x1+(x2-x1)*p/n,y1+(y2-y1)*p/n,width/2,1);}}
  if(kind==='timer'){
    line(10,7,29,7,3);line(10,32,29,32,3);
    line(12,9,12,12,2);line(27,9,27,12,2);line(12,12,27,27,2);line(27,12,12,27,2);
    line(12,27,12,30,2);line(27,27,27,30,2);
    for(var y=25;y<=29;y++){line(20-(y-24),y,19+(y-24),y,1);}
    line(16,11,23,11,2);
  }else if(kind==='music'){
    line(16,11,29,8,4);line(16,11,16,28,3);line(29,8,29,25,3);
    circle(12,28,4,1);circle(25,25,4,1);
  }else{
    // Neutral activity mark; do not guess a timer from a countdown or music
    // from free text. Unknown kinds use this symbol and their explicit app name.
    circle(12,12,3,1);circle(27,12,3,1);circle(12,27,3,1);circle(27,27,3,1);
    line(12,20,27,20,2);line(20,12,20,27,2);
  }
  return {width:size,height:size,palette:[0,192],pixels:pixels};
}
function project(input){
  var card=view.normalize(input);if(!card){return null;}
  var identity=wire.text(input.providerName,32)||names[card.kind]||'Activity';
  // Compatibility for the native Timer 0.5 compact card, whose configured
  // duration was repeated in both fields. Arbitrary provider role labels stay.
  if(card.provider===TIMER&&card.main.label===card.caption){card.main.label=identity;}
  card.caption=identity;
  if(!card.image){card.image=icon(card.kind);}
  var result=view.normalize(card);
  if(!result&&card.support){
    // The main value and role/unit must remain complete. Supporting content is
    // optional when a full provider identity would exceed ACT2's shared pool.
    card.support=null;if(card.meter&&card.meter.source==='support'){card.meter=null;}
    result=view.normalize(card);
  }
  return result;
}
function select(cards,prefs,extraMusic){
  var all=(Array.isArray(cards)?cards:[]).slice();
  if(extraMusic){
    // A dual-convention provider keeps its ACT2 card and UUID preferences. The
    // builtin key orders only the compatibility card; it must never replace an
    // app's UUID controls. Hiding the app also hides its compatibility source.
    // UUID equality is authoritative; track text never establishes identity.
    var duplicate=all.some(function(card){return card.provider===extraMusic.provider&&card.kind==='music';});
    if(!duplicate&&selection.normalizePreferences(prefs).hidden.indexOf(extraMusic.provider)<0){all.push(extraMusic);}
  }
  // Preserve the previous default: ongoing activities, then Music, then opted-in
  // terminal results. Explicit provider preferences can move Music first.
  all=all.map(function(card,index){return {card:card,index:index};}).sort(function(a,b){
    var am=a.card.kind==='music',bm=b.card.kind==='music';return am!==bm?(am?1:-1):a.index-b.index;
  }).map(function(entry){return entry.card;});
  return selection.select(all,prefs).map(project).filter(Boolean);
}
module.exports={project:project,select:select};
