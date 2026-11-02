#pragma once

#include "context_data.h"

typedef struct {
  ContextMode mode;
  bool heading_only,has_music;
  ComplicationSlot title,subtitle;
  // Background music follows validated live slots instead of duplicating two
  // image buffers in every outgoing/incoming notification snapshot.
  const ComplicationSlot *music_title,*music_artist;
} RenderContext;
typedef struct {
  ContextMode mode;
  bool heading_only,has_music;
  const ComplicationSlot *title,*subtitle,*music_title,*music_artist;
} ContextView;

// Text/app identity controls transitions. Covers and icons may arrive later;
// their pixels repaint without moving already-readable content again.
static inline bool render_slot_identity_equal(const ComplicationSlot *a,const ComplicationSlot *b) {
  const unsigned identity_flags=COMPLICATION_FLAG_TEXT|COMPLICATION_FLAG_TIMESTAMP;
  return (a->flags&identity_flags)==(b->flags&identity_flags)&&a->status==b->status&&
      a->text_length==b->text_length&&a->timestamp==b->timestamp&&
      memcmp(a->text,b->text,a->text_length)==0&&strcmp(a->label,b->label)==0;
}
static inline bool render_slot_equal(const ComplicationSlot *a,const ComplicationSlot *b) {
  return render_slot_identity_equal(a,b)&&a->flags==b->flags&&a->width==b->width&&a->height==b->height&&
      a->stride==b->stride&&a->pixel_bytes==b->pixel_bytes&&a->display_kind==b->display_kind&&
      (!(a->flags&COMPLICATION_FLAG_BITMAP)||
       (memcmp(a->palette,b->palette,COMPLICATION_PALETTE_BYTES)==0&&memcmp(a->bitmap,b->bitmap,a->pixel_bytes)==0));
}
static inline bool render_optional_slot_equal(const ComplicationSlot *snapshot,const ComplicationSlot *live) {
  return live?render_slot_identity_equal(snapshot,live):snapshot->flags==0;
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
       ((snapshot->music_title==NULL&&view->music_title==NULL)||
        (snapshot->music_title&&view->music_title&&render_slot_identity_equal(snapshot->music_title,view->music_title)))&&
       ((snapshot->music_artist==NULL&&view->music_artist==NULL)||
        (snapshot->music_artist&&view->music_artist&&render_slot_identity_equal(snapshot->music_artist,view->music_artist)))));
}
// A burst has one entrance. Edits to its heading/body remain in place, and
// an artist arriving after a title also must not replay the song entrance.
static inline bool render_context_should_slide(const RenderContext *snapshot,const ContextView *view) {
  if(snapshot->mode==CONTEXT_NONE||view->mode==CONTEXT_NONE)return false;
  if(snapshot->mode==CONTEXT_NOTIFICATION&&view->mode==CONTEXT_NOTIFICATION)return false;
  if(snapshot->mode==CONTEXT_MUSIC&&view->mode==CONTEXT_MUSIC)
    return !render_slot_identity_equal(&snapshot->title,view->title);
  return true;
}
static inline void render_context_capture(RenderContext *snapshot,const ContextView *view) {
  memset(snapshot,0,sizeof(*snapshot));snapshot->mode=view->mode;
  snapshot->heading_only=view->heading_only;snapshot->has_music=view->has_music;
  if(view->title)snapshot->title=*view->title;
  if(view->subtitle)snapshot->subtitle=*view->subtitle;
  snapshot->music_title=view->music_title;
  snapshot->music_artist=view->music_artist;
}
static inline const ComplicationSlot *render_context_music(const RenderContext *snapshot,bool artist) {
  if(snapshot->mode==CONTEXT_MUSIC)return artist?&snapshot->subtitle:&snapshot->title;
  if(snapshot->has_music)return artist?snapshot->music_artist:snapshot->music_title;
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
      (i==2&&!render_slot_identity_equal(&snapshot->title,&live->slots[i]))) {
    if(snapshot->heading_only&&snapshot->has_music&&
        snapshot->music_title&&context_slot_fresh(snapshot->music_title,now)&&context_slot_fresh(&live->slots[0],now)) {
      snapshot->title=*snapshot->music_title;
      if(snapshot->music_artist)snapshot->subtitle=*snapshot->music_artist;
      else memset(&snapshot->subtitle,0,sizeof(snapshot->subtitle));
      snapshot->music_title=NULL;snapshot->music_artist=NULL;
      snapshot->mode=CONTEXT_MUSIC;snapshot->heading_only=false;snapshot->has_music=false;
      i=0;
    } else {memset(snapshot,0,sizeof(*snapshot));return;}
  }
  if(!context_slot_fresh(&snapshot->subtitle,now)||!context_slot_fresh(&live->slots[i+1],now)||
      !(live->slots[i+1].flags&COMPLICATION_FLAG_TEXT)||!live->slots[i+1].text_length)
    memset(&snapshot->subtitle,0,sizeof(snapshot->subtitle));
  if(snapshot->has_music) {
    if(!snapshot->music_title||!context_slot_fresh(snapshot->music_title,now)||!context_slot_fresh(&live->slots[0],now)) {
      snapshot->music_title=NULL;snapshot->music_artist=NULL;snapshot->has_music=false;
    } else if((!snapshot->music_artist||!context_slot_fresh(snapshot->music_artist,now))||!context_slot_fresh(&live->slots[1],now))
      snapshot->music_artist=NULL;
  }
}
