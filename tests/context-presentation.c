#include <assert.h>
#include "../src/c/context_presentation.h"

static ComplicationSet live;
static RenderContext snapshot;

static void text(unsigned index,const char *value,uint32_t until) {
  ComplicationSlot *slot=&live.slots[index];memset(slot,0,sizeof(*slot));
  slot->flags=COMPLICATION_FLAG_TEXT;slot->status=COMPLICATION_STATUS_READY;
  slot->confirmed_at=100;slot->valid_until=until;
  slot->text_length=(uint8_t)strlen(value);memcpy(slot->text,value,slot->text_length+1);
}
static void capture(time_t now) {
  const ContextView view=render_context_view(&live,true,false,now);
  render_context_capture(&snapshot,&view);
}
static bool contains(const void *memory,size_t length,const char *text_value) {
  const uint8_t *bytes=memory;const size_t n=strlen(text_value);
  for(size_t i=0;i+n<=length;++i)if(!memcmp(bytes+i,text_value,n))return true;
  return false;
}
static void zeroed(void) {
  const RenderContext empty={0};assert(!memcmp(&snapshot,&empty,sizeof(snapshot)));
}
int main(void) {
  text(0,"Track title",300);text(1,"Artist name",300);capture(200);
  assert(snapshot.mode==CONTEXT_MUSIC&&!snapshot.heading_only);
  assert(!strcmp(render_context_music(&snapshot,false)->text,"Track title"));
  live.slots[0].confirmed_at=200;live.slots[0].valid_until=320;
  ContextView view=render_context_view(&live,true,false,201);
  assert(render_context_equal(&snapshot,&view)); // A confirmation cannot replay motion.

  // Late artwork and lease refresh leave text identity and motion alone.
  live.slots[0].flags|=COMPLICATION_FLAG_BITMAP|COMPLICATION_FLAG_IMAGE;
  live.slots[0].width=4;live.slots[0].height=4;live.slots[0].stride=4;live.slots[0].pixel_bytes=16;
  live.slots[0].bitmap[0]=0x11;
  view=render_context_view(&live,true,false,201);
  assert(render_context_equal(&snapshot,&view));
  assert(!render_context_should_slide(&snapshot,&view));
  text(1,"Late artist correction",300);
  view=render_context_view(&live,true,false,201);
  assert(!render_context_equal(&snapshot,&view));
  assert(!render_context_should_slide(&snapshot,&view));
  text(0,"Different track",300);view=render_context_view(&live,true,false,201);
  assert(render_context_should_slide(&snapshot,&view));
  text(0,"Track title",300);text(1,"Artist name",300);capture(200);

  text(2,"Private messenger",212);capture(200);
  assert(snapshot.mode==CONTEXT_NOTIFICATION&&snapshot.heading_only&&snapshot.has_music);
  assert(!strcmp(render_context_music(&snapshot,true)->text,"Artist name"));
  render_context_redact(&snapshot,&live,true,false,212);
  assert(snapshot.mode==CONTEXT_MUSIC&&!snapshot.has_music&&!snapshot.heading_only);
  assert(!strcmp(snapshot.title.text,"Track title"));
  assert(!contains(&snapshot,sizeof(snapshot),"Private messenger"));

  text(2,"Same sender",212);strcpy(live.slots[2].label,"Signal");capture(200);
  live.slots[2].flags|=COMPLICATION_FLAG_BITMAP;live.slots[2].pixel_bytes=16;
  live.slots[2].bitmap[0]=0x22;
  view=render_context_view(&live,true,false,201);
  assert(render_context_equal(&snapshot,&view)); // Late app icon has no entrance.
  render_context_redact(&snapshot,&live,true,false,201);
  assert(snapshot.mode==CONTEXT_NOTIFICATION); // Asset refresh cannot redact the notice.
  text(2,"Updated sender",212);strcpy(live.slots[2].label,"Signal");
  view=render_context_view(&live,true,false,201);
  assert(!render_context_equal(&snapshot,&view));
  assert(!render_context_should_slide(&snapshot,&view)); // One entrance per burst.
  capture(201);strcpy(live.slots[2].label,"Other app");
  view=render_context_view(&live,true,false,201);
  assert(!render_context_equal(&snapshot,&view));
  assert(!render_context_should_slide(&snapshot,&view));

  text(2,"Private messenger",212);text(3,"Secret preview body",212);capture(200);
  assert(snapshot.mode==CONTEXT_NOTIFICATION&&!snapshot.heading_only&&!snapshot.has_music);
  memset(&live.slots[3],0,sizeof(live.slots[3]));
  render_context_redact(&snapshot,&live,true,false,201);
  assert(!snapshot.subtitle.flags&&!contains(&snapshot,sizeof(snapshot),"Secret preview body"));
  capture(201);
  assert(snapshot.heading_only&&snapshot.has_music);
  memset(&live.slots[0],0,sizeof(live.slots[0]));
  memset(&live.slots[1],0,sizeof(live.slots[1]));
  render_context_redact(&snapshot,&live,true,false,201);
  assert(snapshot.mode==CONTEXT_NOTIFICATION&&!snapshot.has_music);
  assert(!contains(&snapshot,sizeof(snapshot),"Artist name")&&!contains(&snapshot,sizeof(snapshot),"Track title"));
  render_context_redact(&snapshot,&live,false,false,201);zeroed();

  text(0,"Track title",300);text(1,"Artist name",300);capture(201);
  render_context_redact(&snapshot,&live,true,true,201);zeroed();
  capture(201);
  render_context_redact(&snapshot,&live,true,false,400);zeroed();

  text(2,"Old app name",212);capture(200);
  text(2,"New app name",213);
  render_context_redact(&snapshot,&live,true,false,201);
  assert(snapshot.mode==CONTEXT_MUSIC&&!contains(&snapshot,sizeof(snapshot),"Old app name"));
  view=render_context_view(&live,false,false,201);
  assert(view.mode==CONTEXT_NONE&&!view.title&&!view.music_title);
  view=render_context_view(&live,true,true,201);
  assert(view.mode==CONTEXT_NONE);

  // Notification snapshots borrow only the fixed live music slots. A provider
  // edit is visible immediately without another entrance or private image copy.
  memset(&live,0,sizeof(live));
  text(0,"First track",300);text(1,"First artist",300);text(2,"App banner",240);
  capture(201);RenderContext outgoing=snapshot;
  assert(snapshot.music_title==&live.slots[0]&&snapshot.music_artist==&live.slots[1]);
  assert(!contains(&snapshot,sizeof(snapshot),"First track"));
  text(0,"Latest track",300);text(1,"Latest artist",300);
  view=render_context_view(&live,true,false,202);
  assert(render_context_equal(&snapshot,&view));
  assert(!render_context_should_slide(&snapshot,&view));
  assert(!strcmp(render_context_music(&snapshot,false)->text,"Latest track"));
  render_context_redact(&snapshot,&live,true,false,240);
  assert(snapshot.mode==CONTEXT_MUSIC&&!snapshot.music_title&&!snapshot.music_artist);
  assert(!strcmp(snapshot.title.text,"Latest track"));

  // A clear revokes the borrowed slots for both transition directions, while
  // the normal-music fallback copy also obeys the same live permission gate.
  memset(&live.slots[0],0,sizeof(live.slots[0]));
  memset(&live.slots[1],0,sizeof(live.slots[1]));
  render_context_redact(&outgoing,&live,true,false,241);
  render_context_redact(&snapshot,&live,true,false,241);
  assert(!outgoing.music_title&&!outgoing.music_artist&&!outgoing.has_music);
  assert(!render_context_music(&outgoing,false));zeroed();
  return 0;
}
