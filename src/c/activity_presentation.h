#pragma once

#include "music_layout.h"
#include <pebble-activities/pebble-activity-view.h>

static inline bool activity_datum_changes(const PavCard *card,const PavDatum *datum,
                                           uint32_t now,bool connected) {
  const PavClockValue clock=pav_clock_value(card,datum,now,connected);
  return clock.running&&((datum->flags&PAV_ELAPSED)||clock.seconds);
}
static inline bool activity_view_needs_tick(const PavCard *card,bool support_visible,
    const PavCard *alternate,bool summary_visible,uint32_t now,bool connected) {
  return activity_datum_changes(card,&card->main,now,connected)||
      (support_visible&&activity_datum_changes(card,&card->support,now,connected))||
      (summary_visible&&alternate&&activity_datum_changes(alternate,&alternate->main,now,connected));
}

// Text activities retain the music composition. Clocks have one numeric home,
// with their label directly below and an optional secondary line at the bottom.
typedef struct {
  MusicLayout content;
  int footer_left, footer_width;
  int main_left, marks_left, marks_y, summary_baseline;
  bool summary, clock;
  int support_left, support_width, support_baseline, support_marks_left, support_marks_y;
  int summary_left, summary_width, summary_marks_left;
  int meter_left, meter_top, meter_width;
  bool support, meter;
} ActivityLayout;

static void __attribute__((noinline)) activity_clock_layout(const char *main,const char *footer,bool image,
    unsigned state_marks,const char *summary,int top,bool paired,MusicMeasure measure,
    MusicMeasure clock_measure,void *context,ActivityLayout *out) {
  MusicLayout *layout=&out->content;out->clock=true;layout->artist_point=20;
  const int steps[]={58,48,38,32,26};
  unsigned start=strlen(main)>8?3:strlen(main)>5?2:0;
  if(paired&&start<1)start=1;
  const int baseline=paired&&top<=104?148:160;
  const int caption_baseline=paired?baseline+28:190;
  MusicTextMetrics metrics={0};
  for(unsigned i=start;i<sizeof(steps)/sizeof(steps[0]);++i) {
    layout->title_point=steps[i];metrics=clock_measure(context,main,steps[i]);
    if(metrics.width<=180&&baseline-metrics.ascent>=top)break;
  }
  watch_layout_copy_utf8(main,layout->title.text[0],MUSIC_TEXT_BYTES);
  layout->title.count=1;layout->title.complete=true;layout->title.consumed=strlen(main);
  layout->title_baseline[0]=baseline;layout->title_top=baseline-metrics.ascent;
  layout->title_bottom=baseline+metrics.descent;out->main_left=100-metrics.width/2;
  const int marks=(int)state_marks*18;
  out->footer_width=(image?130:180)-marks;
  music_wrap(footer,20,out->footer_width,image&&!paired?2:1,false,measure,context,&layout->artist);
  if(image&&!paired) {
    const int height=music_line_offsets(&layout->artist,20,measure,context,layout->artist_baseline);
    for(int i=0;i<layout->artist.count;++i)layout->artist_baseline[i]+=MUSIC_BOTTOM-height;
    layout->cover_top=MUSIC_BOTTOM-40;layout->metadata_top=layout->cover_top;
    out->marks_left=60;out->marks_y=MUSIC_BOTTOM-9;out->footer_left=60+marks;
  } else {
    const MusicTextMetrics label=measure(context,layout->artist.text[0],20);
    out->marks_left=image?60:100-(marks+label.width)/2;
    out->footer_left=out->marks_left+marks;
    if(!image)out->footer_width=label.width;
    layout->artist_baseline[0]=caption_baseline;
    layout->metadata_top=caption_baseline-label.ascent;layout->cover_top=176;
    out->marks_y=paired?caption_baseline-7:
        caption_baseline-(label.ascent-label.descent)/2-1;
    if(!paired&&!image&&summary&&summary[0]) {
      const MusicTextMetrics secondary=measure(context,summary,14);
      out->summary_baseline=MUSIC_BOTTOM-secondary.descent;
      out->summary=secondary.width<=180&&
          out->summary_baseline-secondary.ascent>=caption_baseline+label.descent+6;
    }
  }
}

static inline void activity_layout_build_at(const char *main,const char *footer,
                                          bool image,bool clock,unsigned state_marks,
                                          const char *summary,int top,MusicMeasure measure,
                                          MusicMeasure clock_measure,void *context,ActivityLayout *out) {
  memset(out,0,sizeof(*out));
  if(clock) {
    activity_clock_layout(main,footer,image,state_marks,summary,top,false,measure,clock_measure,context,out);
    return;
  }
  MusicLayout *layout=&out->content;
  layout->title_point=MUSIC_TITLE_POINT;
  layout->artist_point=MUSIC_ARTIST_POINT;
  out->footer_left=image?60:10+(int)state_marks*18;
  out->footer_width=190-out->footer_left;
  out->main_left=10;out->marks_left=10;out->marks_y=MUSIC_BOTTOM-9;
  out->summary_baseline=top+measure(context,summary?summary:"",14).ascent;
  music_wrap(footer,layout->artist_point,190-out->footer_left,2,false,measure,context,&layout->artist);
  for(int lines=MUSIC_MAX_LINES;lines>=1;--lines) {
    music_wrap(main,layout->title_point,180,lines,false,measure,context,&layout->title);
    if(!clock)music_title_boundary(main,&layout->title);
    music_position(layout,image||state_marks,measure,context);
    if(layout->title_top>=top)break;
  }
  // The optional alternate uses actual ink extents. Never reduce the primary
  // text or move its footer merely to force another activity onto the screen.
  if(summary&&summary[0]) {
    const MusicTextMetrics metrics=measure(context,summary,14);
    out->summary=metrics.width<=180&&top+metrics.ascent+metrics.descent+8<=layout->title_top;
  }
}

static inline void activity_layout_build(const char *main,const char *footer,
                                          bool image,bool clock,unsigned state_marks,
                                          const char *summary,MusicMeasure measure,
                                          MusicMeasure clock_measure,void *context,ActivityLayout *out) {
  activity_layout_build_at(main,footer,image,clock,state_marks,summary,92,measure,clock_measure,context,out);
}

// Two related items share the existing type assets. Extra detail uses a smaller
// numeric home; it never shrinks body titles or silently replaces the main item.
static inline void activity_layout_pair_at(const char *main,const char *footer,
    const char *support,bool image,bool vector,unsigned state_marks,unsigned support_marks,
    const char *summary,unsigned summary_marks,bool meter,int top,MusicMeasure measure,
    MusicMeasure clock_measure,void *context,ActivityLayout *out) {
  if((!support||!support[0])&&!meter) {
    activity_layout_build_at(main,footer,image,vector,state_marks,summary,top,
        measure,clock_measure,context,out);
    out->summary_left=10;out->summary_width=180-(int)summary_marks*18;
    out->summary_marks_left=190-(int)summary_marks*18;
    return;
  }
  memset(out,0,sizeof(*out));
  MusicLayout *layout=&out->content;
  const int baseline=top>104?160:148;
  const int caption_baseline=baseline+28;
  layout->title_point=vector?48:24;layout->artist_point=20;out->clock=vector;
  if(vector) {
    activity_clock_layout(main,footer,image,state_marks,NULL,top,true,measure,clock_measure,context,out);
  } else {
    for(int lines=2;lines>=1;--lines) {
      music_wrap(main,24,180,lines,false,measure,context,&layout->title);
      music_title_boundary(main,&layout->title);
      music_line_offsets(&layout->title,24,measure,context,layout->title_baseline);
      const int last=layout->title.count?layout->title.count-1:0;
      const int shift=baseline-layout->title_baseline[last];
      layout->title_top=shift;layout->title_bottom=baseline+
          measure(context,layout->title.text[last],24).descent;
      for(int i=0;i<layout->title.count;++i)layout->title_baseline[i]+=shift;
      if(layout->title_top>=top)break;
    }
    out->main_left=10;
  }
  const int left=image?60:10,width=image?130:180;
  if(!vector) {
    const int marks=(int)state_marks*18;
    out->footer_left=left+marks;out->footer_width=width-marks;
    music_wrap(footer,20,out->footer_width,1,false,measure,context,&layout->artist);
    layout->artist_baseline[0]=caption_baseline;
    layout->metadata_top=caption_baseline-measure(context,layout->artist.text[0],20).ascent;
    layout->cover_top=176;out->marks_left=left;out->marks_y=caption_baseline-7;
  }
  out->support_left=left;out->support_width=width-(int)support_marks*18;
  out->support_baseline=baseline+48;
  out->support_marks_left=left+out->support_width;out->support_marks_y=out->support_baseline-5;
  out->support=support&&support[0]&&measure(context,support,14).width<=out->support_width&&
      out->support_baseline-measure(context,support,14).ascent>=
          caption_baseline+measure(context,layout->artist.text[0],20).descent+4;
  // Under a heading banner the supporting line moves to 208. Optional progress
  // yields there rather than painting over the live supporting value.
  out->meter=meter&&(!out->support||out->support_baseline<=196);
  out->meter_left=left;out->meter_top=208;out->meter_width=width;
  out->summary_left=left;out->summary_width=width-(int)summary_marks*18;
  out->summary_marks_left=left+out->summary_width;
  out->summary_baseline=216-measure(context,summary?summary:"",14).descent;
  const int preceding=out->support?out->support_baseline+measure(context,support,14).descent:
      caption_baseline+measure(context,layout->artist.text[0],20).descent;
  out->summary=!meter&&summary&&summary[0]&&measure(context,summary,14).width<=out->summary_width&&
      out->summary_baseline-measure(context,summary,14).ascent>=preceding+4;
}
