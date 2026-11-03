#pragma once

#include <stdio.h>
#include "music_layout.h"
#include <pebble-activities/pebble-activity-view.h>

/* The host has two positions, not two different source contracts. */
enum { ACTIVITY_SECONDARY_TOP=96, ACTIVITY_SECONDARY_LEFT=32,
       ACTIVITY_SECONDARY_WIDTH=158, ACTIVITY_SECONDARY_ICON=16,
       ACTIVITY_CONTENT_TOP=92, ACTIVITY_CONTENT_BOTTOM=216 };

static inline const char *activity_identity(const PavCard *card) {
  return pav_caption(card);
}
static inline const char *activity_role(const PavCard *card,const PavDatum *datum) {
  const char *label=pav_label(card,datum);
  return strcmp(label,activity_identity(card))?label:"";
}
static inline bool activity_is_result(const PavCard *card) {
  return card->source_state==PAV_COMPLETED||card->source_state==PAV_CANCELLED;
}
/* A formatted score/pace is one value, not a decimal followed by a guessed
 * unit. Ambiguous or exotic suffixes use the complete body-font fallback. */
static inline bool activity_simple_unit(const char *unit) {
  const unsigned char first=(unsigned char)unit[0];
  if(first&&!((first>='a'&&first<='z')||(first>='A'&&first<='Z')||
      first=='%'||first=='/'||(first==0xc2&&(unsigned char)unit[1]==0xb0)))return false;
  for(const unsigned char *at=(const unsigned char *)unit;*at;++at)
    if((*at>='0'&&*at<='9')||*at==':'||*at=='+'||*at=='-')return false;
  return true;
}
static inline const PavDatum *activity_compact_datum(const PavCard *card) {
  return card->compact_item?&card->support:&card->main;
}
static inline bool activity_datum_changes(const PavCard *card,const PavDatum *datum,
                                           uint32_t now,bool connected) {
  if(!pav_datum_visible(card,datum,now,connected))return false;
  const PavClockValue clock=pav_clock_value(card,datum,now,connected);
  return clock.running&&((datum->flags&PAV_ELAPSED)||clock.seconds);
}
static inline bool activity_view_needs_tick(const PavCard *card,bool support_visible,
    const PavCard *alternate,bool summary_visible,uint32_t now,bool connected) {
  return activity_datum_changes(card,&card->main,now,connected)||
      (support_visible&&activity_datum_changes(card,&card->support,now,connected))||
      (summary_visible&&alternate&&activity_datum_changes(alternate,
          activity_compact_datum(alternate),now,connected));
}

typedef struct {
  MusicLines main,support,footer;
  int main_baseline[MUSIC_MAX_LINES],support_baseline[MUSIC_MAX_LINES];
  int footer_baseline[MUSIC_MAX_LINES];
  int point,left,width,role_baseline,footer_left,footer_width,footer_point,support_point;
  int icon_top,icon_size,meter_top,secondary_baseline;
  bool vector,scalar,secondary,meter,media;
} ActivityLayout;

/* All wrapping starts at byte zero. The last available line ellipsizes; no
 * continuation state or timer exists. A bounded plan is reused by draw/ticks. */
static inline void activity_layout_build(const char *main,const char *role,
    const char *support,const char *footer,bool media,bool vector,bool scalar,
    bool secondary,bool image,bool meter,int support_point,int top,MusicMeasure measure,
    MusicMeasure scalar_measure,void *context,ActivityLayout *out) {
  memset(out,0,sizeof(*out));out->media=media;out->secondary=secondary;
  out->vector=vector;out->scalar=scalar;out->left=10;out->width=180;
  int content_top=top+(secondary?26:4);out->support_point=support_point;
  out->secondary_baseline=top+4+measure(context,"Ag",16).ascent;
  out->icon_size=media?40:scalar||vector?24:16;
  out->icon_top=MUSIC_BOTTOM-out->icon_size;
  out->footer_left=media?(image?60:10):footer[0]?10+out->icon_size+10:10;
  out->footer_width=190-out->footer_left;out->footer_point=media?20:scalar||vector?20:16;
  music_wrap(footer,out->footer_point,out->footer_width,media?2:1,media,measure,context,&out->footer);
  const int footer_height=music_line_offsets(&out->footer,out->footer_point,
      measure,context,out->footer_baseline);
  for(int i=0;i<out->footer.count;++i)out->footer_baseline[i]+=MUSIC_BOTTOM-footer_height;
  int footer_top=MUSIC_BOTTOM-footer_height;
  if((media&&image)||(!media&&footer[0])) {
    if(footer_top>out->icon_top)footer_top=out->icon_top;
  }
  const int limit=footer_top-(footer_top<MUSIC_BOTTOM?8:0);
  if(content_top>=limit){out->secondary=false;return;}
  if(media) {
    out->point=24;
    for(int count=secondary?2:3;count>=1;--count) {
      music_wrap(main,24,180,count,false,measure,context,&out->main);
      const int height=music_line_offsets(&out->main,24,measure,context,out->main_baseline);
      if(content_top+height<=limit) {
        for(int i=0;i<out->main.count;++i)out->main_baseline[i]+=content_top;
        return;
      }
    }
    out->main.count=0;out->secondary=false;return;
  }
  const int role_height=role[0]?measure(context,role,14).ascent+measure(context,role,14).descent+4:0;
  if(role[0]){out->role_baseline=content_top+measure(context,role,14).ascent;content_top+=role_height;}
  int support_height=0;
  if(support[0]) {
    music_wrap(support,support_point,180,vector||scalar?1:2,false,measure,context,&out->support);
    support_height=music_line_offsets(&out->support,support_point,measure,context,out->support_baseline)+6;
  }
  int main_limit=limit-support_height-(meter?8:0);
  if(main_limit-content_top<18){out->support.count=0;support_height=0;main_limit=limit-(meter?8:0);}
  int main_bottom=content_top;
  if(vector||scalar) {
    const int points[]={64,56,48,40,32,26,20,16,14};
    MusicTextMetrics value={0};
    unsigned first=secondary?1:0;
    for(unsigned pass=0;pass<2;++pass) {
      for(unsigned i=first;i<sizeof(points)/sizeof(points[0]);++i) {
        out->point=points[i];value=scalar_measure(context,main,out->point);
        if(value.width>0&&value.width<=180&&value.ascent+value.descent<=main_limit-content_top)break;
      }
      if(value.width>0&&value.width<=180&&value.ascent+value.descent<=main_limit-content_top)break;
      if(out->support.count){out->support.count=0;main_limit=limit-(meter?8:0);}
      else {out->main.count=0;out->secondary=false;return;}
    }
    if(!value.width||value.width>180||value.ascent+value.descent>main_limit-content_top) {
      out->main.count=0;out->secondary=false;return;
    }
    watch_layout_copy_utf8(main,out->main.text[0],MUSIC_TEXT_BYTES);
    out->main.count=1;out->main.complete=true;
    const int ink_top=content_top+(main_limit-content_top-value.ascent-value.descent)/2;
    out->main_baseline[0]=ink_top+value.ascent;
    if(vector)out->left=10+(180-value.width)/2;
    main_bottom=out->main_baseline[0]+value.descent;
  } else {
    out->point=24;
    for(int count=secondary?2:3;count>=1;--count) {
      music_wrap(main,24,180,count,false,measure,context,&out->main);
      const int height=music_line_offsets(&out->main,24,measure,context,out->main_baseline);
      if(content_top+height<=main_limit) {
        for(int i=0;i<out->main.count;++i)out->main_baseline[i]+=content_top;
        main_bottom=content_top+height;break;
      }
      out->main.count=0;
    }
    if(!out->main.count){out->secondary=false;return;}
  }
  if(out->support.count) {
    for(int i=0;i<out->support.count;++i)out->support_baseline[i]+=main_bottom+6;
    const int last=out->support.count-1;
    main_bottom=out->support_baseline[last]+measure(context,out->support.text[last],support_point).descent;
  }
  out->meter=meter&&main_bottom+8<=limit;
  out->meter_top=main_bottom+4;
}
