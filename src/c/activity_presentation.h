#pragma once

#include <stdio.h>
#include "music_layout.h"
#include <pebble-activities/pebble-activity-view.h>

enum { ACTIVITY_PAUSED=1u, ACTIVITY_UNCERTAIN=2u, ACTIVITY_AGE=4u };
static inline const char *activity_identity(const PavCard *card) {
  const char *identity=pav_caption(card);return identity[0]?identity:"Activity";
}
static inline const char *activity_role(const PavCard *card,const PavDatum *datum) {
  const char *label=pav_label(card,datum);
  return strcmp(label,activity_identity(card))?label:"";
}
static inline bool activity_is_result(const PavCard *card) {
  return card->source_state==PAV_COMPLETED||card->source_state==PAV_CANCELLED;
}
static inline uint8_t activity_status_marks(const PavCard *card,const PavDatum *datum,
    uint32_t now,bool connected,bool main) {
  const PavClockValue clock=pav_clock_value(card,datum,now,connected);
  const bool cached=!connected||(card->flags&PAV_SOURCE_UNCONFIRMED);
  const bool paused=!activity_is_result(card)&&((datum->type==PAV_CLOCK&&!(datum->flags&PAV_RUNNING))||
      (main&&card->source_state==PAV_PAUSED));
  const bool uncertain=cached||(datum->flags&PAV_ESTIMATED)||now<datum->observed_at||
      clock.estimated||clock.due;
  return (paused?ACTIVITY_PAUSED:0)|(uncertain?ACTIVITY_UNCERTAIN:0)|(cached?ACTIVITY_AGE:0);
}
static inline uint8_t activity_support_marks(const PavCard *card,uint8_t main,uint8_t support) {
  const uint8_t provenance=ACTIVITY_UNCERTAIN|ACTIVITY_AGE;
  // One source-age indication covers matching observations. Independent item
  // estimates and observations retain their own indication; pause is local.
  if(card->main.observed_at==card->support.observed_at&&
      !((card->main.flags^card->support.flags)&PAV_ESTIMATED)&&
      !((main^support)&provenance))support&=(uint8_t)~provenance;
  return support;
}

static inline bool activity_datum_changes(const PavCard *card,const PavDatum *datum,
                                           uint32_t now,bool connected) {
  const PavClockValue clock=pav_clock_value(card,datum,now,connected);
  const bool age=(!connected||(card->flags&PAV_SOURCE_UNCONFIRMED))&&
      pav_datum_visible(card,datum,now,connected)&&now>=datum->observed_at&&now-datum->observed_at<60;
  return age||(clock.running&&((datum->flags&PAV_ELAPSED)||clock.seconds));
}
static inline bool activity_view_needs_tick(const PavCard *card,bool support_visible,
    const PavCard *alternate,bool summary_visible,uint32_t now,bool connected) {
  return activity_datum_changes(card,&card->main,now,connected)||
      (support_visible&&activity_datum_changes(card,&card->support,now,connected))||
      (summary_visible&&alternate&&activity_datum_changes(alternate,&alternate->main,now,connected));
}

// Both roles carry the same identity and content. The optional second card
// always has its own band; primary detail never becomes another activity.
enum { ACTIVITY_SECONDARY_TOP=176, ACTIVITY_SECONDARY_LEFT=40,
       ACTIVITY_SECONDARY_WIDTH=150, ACTIVITY_PRIMARY_ICON=40,
       ACTIVITY_SECONDARY_ICON=20 };
typedef struct {
  MusicLines main;
  int main_baseline[MUSIC_MAX_LINES];
  int point,left,width,identity_baseline,role_baseline,status_baseline;
  int support_baseline,support_status_baseline,meter_top;
  bool vector,secondary,support,meter;
} ActivityLayout;

static inline void activity_status_text(const PavDatum *datum,uint8_t marks,
    uint32_t now,char *text,size_t capacity) {
  const char *paused=(marks&ACTIVITY_PAUSED)?"Paused":"";
  const char *separator=paused[0]?" · ":"";
  if(marks&ACTIVITY_AGE) {
    if(now<datum->observed_at) {
      snprintf(text,capacity,"%s%sLast reported time unknown",paused,separator);return;
    }
    const uint32_t age=now-datum->observed_at;
    const uint32_t divisor=age<60?1:age<3600?60:age<86400?3600:86400;
    const char unit=age<60?'s':age<3600?'m':age<86400?'h':'d';
    snprintf(text,capacity,"%s%sLast reported %lu%c ago",paused,separator,
        (unsigned long)(age/divisor),unit);
  } else snprintf(text,capacity,"%s%s%s",paused,
      (marks&ACTIVITY_UNCERTAIN)?separator:"",(marks&ACTIVITY_UNCERTAIN)?"Estimated":"");
}

static inline int activity_secondary_identity_width(const char *status,MusicMeasure measure,void *context) {
  return ACTIVITY_SECONDARY_WIDTH-(status[0]?measure(context,status,11).width+6:0);
}
static inline bool activity_secondary_fits(const char *identity,const char *content,
    const char *status,MusicMeasure measure,void *context) {
  const MusicTextMetrics heading=measure(context,identity,14),value=measure(context,content,16);
  return activity_secondary_identity_width(status,measure,context)>=24&&
      value.width<=ACTIVITY_SECONDARY_WIDTH&&
      180+heading.ascent+heading.descent+2<=MUSIC_BOTTOM-value.ascent-value.descent;
}

static inline int activity_line_baseline(const char *text,int point,int *bottom,
    MusicMeasure measure,void *context) {
  if(!text||!text[0])return 0;
  const MusicTextMetrics metrics=measure(context,text,point);
  const int baseline=*bottom+3+metrics.ascent;
  *bottom=baseline+metrics.descent;return baseline;
}

static inline void activity_layout_build(const char *identity,const char *main,const char *role,
    const char *support,const char *status,const char *support_status,bool vector,bool scalar,
    bool secondary,bool meter,int top,MusicMeasure measure,MusicMeasure clock_measure,
    void *context,ActivityLayout *out) {
  // An oversized formatted scalar must not lose its trailing unit to clipping.
  // Its explicit fallback is preferable to displaying a partial measurement.
  if(scalar&&!vector&&measure(context,main,14).width>180) {
    main="Open app for details";scalar=false;
  }
  memset(out,0,sizeof(*out));out->vector=vector;
  const MusicTextMetrics heading=measure(context,identity,20);
  out->identity_baseline=top+heading.ascent;
  const int content_top=top+heading.ascent+heading.descent+5;
  int bottom=0;
  // These passes fit the primary's own essentials. A secondary never changes
  // its type size, title wrapping or independent supporting value.
  for(int pass=0;pass<3;++pass) {
    out->left=60;out->width=130;
    if(vector||scalar) {
      const int clocks[]={48,40,32,26},values[]={32,28,24,20,16,14};
      const int *points=vector?clocks:values;
      const unsigned count=vector?sizeof(clocks)/sizeof(clocks[0]):sizeof(values)/sizeof(values[0]);
      MusicTextMetrics value={0};
      for(unsigned i=(unsigned)pass;i<count;++i) {
        out->point=points[i];value=(vector?clock_measure:measure)(context,main,out->point);
        if(value.width<=130)break;
      }
      if(value.width>130){out->left=10;out->width=180;}
      int start=content_top;
      if(out->left==10&&start<top+44)start=top+44;
      watch_layout_copy_utf8(main,out->main.text[0],MUSIC_TEXT_BYTES);
      out->main.count=1;out->main.complete=true;out->main.consumed=strlen(main);
      out->main_baseline[0]=start+value.ascent;
      bottom=out->main_baseline[0]+value.descent;
    } else {
      out->point=24;
      music_wrap(main,24,out->width,3-pass,false,measure,context,&out->main);
      music_title_boundary(main,&out->main);
      bottom=content_top+music_line_offsets(&out->main,24,measure,context,out->main_baseline);
      for(int i=0;i<out->main.count;++i)out->main_baseline[i]+=content_top;
    }
    if(bottom<top+ACTIVITY_PRIMARY_ICON)bottom=top+ACTIVITY_PRIMARY_ICON;
    out->role_baseline=activity_line_baseline(role,14,&bottom,measure,context);
    out->status_baseline=activity_line_baseline(status,11,&bottom,measure,context);
    out->support_baseline=activity_line_baseline(support,16,&bottom,measure,context);
    out->support_status_baseline=activity_line_baseline(support_status,11,&bottom,measure,context);
    if(bottom+(meter?8:0)<=MUSIC_BOTTOM)break;
  }
  out->support=support&&support[0];out->meter=meter&&bottom+8<=MUSIC_BOTTOM;
  out->meter_top=bottom+4;
  out->secondary=secondary&&bottom+(out->meter?8:0)+4<=ACTIVITY_SECONDARY_TOP;
}
