#pragma once

#include "music_layout.h"

// Activities share the established main/footer composition. Clock formatting
// changes the main type size, not its home or the footer's bottom anchor.
typedef struct {
  MusicLayout content;
  int footer_left;
  bool summary;
} ActivityLayout;

static inline void activity_layout_build_at(const char *main,const char *footer,
                                          bool image,bool clock,unsigned state_marks,
                                          const char *summary,int top,MusicMeasure measure,
                                          void *context,ActivityLayout *out) {
  memset(out,0,sizeof(*out));
  MusicLayout *layout=&out->content;
  layout->title_point=clock?32:MUSIC_TITLE_POINT;
  layout->artist_point=MUSIC_ARTIST_POINT;
  out->footer_left=image?60:10+(int)state_marks*18;
  music_wrap(footer,layout->artist_point,190-out->footer_left,2,false,measure,context,&layout->artist);
  for(int lines=clock?1:MUSIC_MAX_LINES;lines>=1;--lines) {
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
                                          void *context,ActivityLayout *out) {
  activity_layout_build_at(main,footer,image,clock,state_marks,summary,92,measure,context,out);
}
