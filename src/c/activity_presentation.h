#pragma once

#include "music_layout.h"

// Text activities retain the music composition. Clocks have one numeric home,
// with their label directly below and an optional secondary line at the bottom.
typedef struct {
  MusicLayout content;
  int footer_left, footer_width;
  int main_left, marks_left, marks_y, summary_baseline;
  bool summary, clock;
} ActivityLayout;

static inline void activity_clock_layout(const char *main,const char *footer,bool image,
                                         unsigned state_marks,const char *summary,int top,
                                         MusicMeasure measure,MusicMeasure clock_measure,
                                         void *context,ActivityLayout *out) {
  MusicLayout *layout=&out->content;
  out->clock=true;
  layout->artist_point=MUSIC_ARTIST_POINT;
  const int steps[]={58,48,38,32,26};
  unsigned start=strlen(main)>8?3:strlen(main)>5?2:0;
  const int baseline=160;
  MusicTextMetrics metrics={0};
  for(unsigned i=start;i<sizeof(steps)/sizeof(steps[0]);++i) {
    layout->title_point=steps[i];
    metrics=clock_measure(context,main,steps[i]);
    if(metrics.width<=180&&baseline-metrics.ascent>=top)break;
  }
  watch_layout_copy_utf8(main,layout->title.text[0],MUSIC_TEXT_BYTES);
  layout->title.count=1;layout->title.complete=true;layout->title.consumed=strlen(main);
  layout->title_baseline[0]=baseline;
  layout->title_top=baseline-metrics.ascent;
  layout->title_bottom=baseline+metrics.descent;
  out->main_left=100-metrics.width/2;
  const int marks=(int)state_marks*18;
  const int available=image?130:180;
  out->footer_width=available-marks;
  music_wrap(footer,20,out->footer_width,image?2:1,false,measure,context,&layout->artist);
  if(image) {
    const int height=music_line_offsets(&layout->artist,20,measure,context,layout->artist_baseline);
    for(int i=0;i<layout->artist.count;++i)layout->artist_baseline[i]+=MUSIC_BOTTOM-height;
    layout->cover_top=MUSIC_BOTTOM-40;
    layout->metadata_top=layout->cover_top;
    out->marks_left=60;out->marks_y=MUSIC_BOTTOM-9;
    out->footer_left=60+marks;
  } else {
    const MusicTextMetrics label=measure(context,layout->artist.count?layout->artist.text[0]:"",20);
    const int group_width=marks+label.width;
    out->marks_left=100-group_width/2;
    out->footer_left=out->marks_left+marks;
    out->footer_width=label.width;
    layout->artist_baseline[0]=190;
    layout->metadata_top=190-label.ascent;
    out->marks_y=190-(label.ascent-label.descent)/2-1;
    if(summary&&summary[0]) {
      const MusicTextMetrics secondary=measure(context,summary,14);
      out->summary_baseline=MUSIC_BOTTOM-secondary.descent;
      out->summary=secondary.width<=180&&
          out->summary_baseline-secondary.ascent>=190+label.descent+6;
    }
  }
}

static inline void activity_layout_build_at(const char *main,const char *footer,
                                          bool image,bool clock,unsigned state_marks,
                                          const char *summary,int top,MusicMeasure measure,
                                          MusicMeasure clock_measure,void *context,ActivityLayout *out) {
  memset(out,0,sizeof(*out));
  if(clock) {
    activity_clock_layout(main,footer,image,state_marks,summary,top,measure,clock_measure,context,out);
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
