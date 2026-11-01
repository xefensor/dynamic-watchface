#pragma once

#include "context_data.h"

typedef struct {
  ContextMode mode;
  bool heading_only,has_music;
  ComplicationSlot title,subtitle,music_title,music_artist;
} RenderContext;
typedef struct {
  ContextMode mode;
  bool heading_only,has_music;
  const ComplicationSlot *title,*subtitle,*music_title,*music_artist;
} ContextView;

// Confirmation timestamps and optional diagnostic metadata do not change the
// visible song/notice, so refreshing a lease never restarts its animation.
static inline bool render_slot_equal(const ComplicationSlot *a,const ComplicationSlot *b) {
  return a->flags==b->flags&&a->status==b->status&&a->width==b->width&&a->height==b->height&&
      a->stride==b->stride&&a->text_length==b->text_length&&a->pixel_bytes==b->pixel_bytes&&
      a->display_kind==b->display_kind&&memcmp(a->text,b->text,a->text_length)==0&&
      (!(a->flags&COMPLICATION_FLAG_BITMAP)||
       (memcmp(a->palette,b->palette,COMPLICATION_PALETTE_BYTES)==0&&memcmp(a->bitmap,b->bitmap,a->pixel_bytes)==0));
}
static inline bool render_optional_slot_equal(const ComplicationSlot *snapshot,const ComplicationSlot *live) {
  return live?render_slot_equal(snapshot,live):snapshot->flags==0;
}
static inline ContextView render_context_view(const ComplicationSet *live,bool connected,
                                               bool rebaseline,time_t now) {
  ContextView view={0};
  if(!connected||rebaseline)return view;
  view.mode=context_mode(live,now);
  if(view.mode!=CONTEXT_NONE) {
    const unsigned i=view.mode==CONTEXT_NOTIFICATION?2:0;
    view.title=&live->slots[i];
    const ComplicationSlot *subtitle=&live->slots[i+1];
    if(context_slot_fresh(subtitle,now)&&(subtitle->flags&COMPLICATION_FLAG_TEXT)&&subtitle->text_length)
      view.subtitle=subtitle;
    view.heading_only=view.mode==CONTEXT_NOTIFICATION&&!view.subtitle;
    if(view.heading_only&&context_slot_fresh(&live->slots[0],now)) {
      view.has_music=true;view.music_title=&live->slots[0];
      if(context_slot_fresh(&live->slots[1],now))view.music_artist=&live->slots[1];
    }
  }
  return view;
}
static inline bool render_context_equal(const RenderContext *snapshot,const ContextView *view) {
  return snapshot->mode==view->mode&&snapshot->heading_only==view->heading_only&&
      snapshot->has_music==view->has_music&&(view->mode==CONTEXT_NONE||
      (render_optional_slot_equal(&snapshot->title,view->title)&&
       render_optional_slot_equal(&snapshot->subtitle,view->subtitle)&&
       render_optional_slot_equal(&snapshot->music_title,view->music_title)&&
       render_optional_slot_equal(&snapshot->music_artist,view->music_artist)));
}
static inline void render_context_capture(RenderContext *snapshot,const ContextView *view) {
  memset(snapshot,0,sizeof(*snapshot));snapshot->mode=view->mode;
  snapshot->heading_only=view->heading_only;snapshot->has_music=view->has_music;
  if(view->title)snapshot->title=*view->title;
  if(view->subtitle)snapshot->subtitle=*view->subtitle;
  if(view->music_title)snapshot->music_title=*view->music_title;
  if(view->music_artist)snapshot->music_artist=*view->music_artist;
}
static inline const ComplicationSlot *render_context_music(const RenderContext *snapshot,bool artist) {
  if(snapshot->mode==CONTEXT_MUSIC)return artist?&snapshot->subtitle:&snapshot->title;
  if(snapshot->has_music)return artist?&snapshot->music_artist:&snapshot->music_title;
  return NULL;
}

// RAM copies retain their original leases. Live permission/clear/session state
// always wins over an outgoing transition; no expired private text is drawn.
static inline void render_context_redact(RenderContext *snapshot,const ComplicationSet *live,
                                         bool connected,bool rebaseline,time_t now) {
  if(snapshot->mode==CONTEXT_NONE)return;
  if(!connected||rebaseline) {memset(snapshot,0,sizeof(*snapshot));return;}
  unsigned i=snapshot->mode==CONTEXT_NOTIFICATION?2:0;
  if(!context_slot_fresh(&snapshot->title,now)||!context_slot_fresh(&live->slots[i],now)||
      (i==2&&!render_slot_equal(&snapshot->title,&live->slots[i]))) {
    if(snapshot->heading_only&&snapshot->has_music&&
        context_slot_fresh(&snapshot->music_title,now)&&context_slot_fresh(&live->slots[0],now)) {
      snapshot->title=snapshot->music_title;snapshot->subtitle=snapshot->music_artist;
      memset(&snapshot->music_title,0,sizeof(snapshot->music_title));
      memset(&snapshot->music_artist,0,sizeof(snapshot->music_artist));
      snapshot->mode=CONTEXT_MUSIC;snapshot->heading_only=false;snapshot->has_music=false;
      i=0;
    } else {memset(snapshot,0,sizeof(*snapshot));return;}
  }
  if(!context_slot_fresh(&snapshot->subtitle,now)||!context_slot_fresh(&live->slots[i+1],now)||
      !(live->slots[i+1].flags&COMPLICATION_FLAG_TEXT)||!live->slots[i+1].text_length)
    memset(&snapshot->subtitle,0,sizeof(snapshot->subtitle));
  if(snapshot->has_music) {
    if(!context_slot_fresh(&snapshot->music_title,now)||!context_slot_fresh(&live->slots[0],now)) {
      memset(&snapshot->music_title,0,sizeof(snapshot->music_title));
      memset(&snapshot->music_artist,0,sizeof(snapshot->music_artist));snapshot->has_music=false;
    } else if(!context_slot_fresh(&snapshot->music_artist,now)||!context_slot_fresh(&live->slots[1],now))
      memset(&snapshot->music_artist,0,sizeof(snapshot->music_artist));
  }
}
