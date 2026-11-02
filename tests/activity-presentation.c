#include <assert.h>
#include "../src/c/activity_presentation.h"
#include "../src/c/generated_body_metrics.h"

static MusicTextMetrics measure(void *context,const char *text,int point) {
  (void)context;int count=0;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor){body_metric_next_codepoint(&cursor);++count;}
  return (MusicTextMetrics){count*point/2,body_text_ascent(text,point),body_text_descent(text,point)};
}
static void check(const ActivityLayout *layout) {
  const MusicLayout *value=&layout->content;
  assert(value->title_top>=92);
  assert(value->title.count<=3&&value->artist.count<=2);
  assert(value->artist_point==20);
  if(value->artist.count) {
    const int i=value->artist.count-1;
    assert(value->artist_baseline[i]+body_text_descent(value->artist.text[i],20)==216);
  }
  for(int i=0;i<value->artist.count;++i)
    assert(measure(NULL,value->artist.text[i],20).width<=190-layout->footer_left);
  if(layout->summary)assert(value->title_top>=92+measure(NULL,"08:42",14).ascent+8);
}
int main(void) {
  ActivityLayout layout;
  activity_layout_build("08:42","Pasta",false,true,0,"Night Trouble",measure,NULL,&layout);
  check(&layout);assert(layout.content.title_point==32&&layout.summary&&layout.footer_left==10);
  activity_layout_build("08:42","Pasta",false,true,2,"Night Trouble",measure,NULL,&layout);
  check(&layout);assert(layout.footer_left==46);
  activity_layout_build("08:42","",false,true,2,NULL,measure,NULL,&layout);
  check(&layout);assert(layout.content.title_bottom<=168&&layout.footer_left==46);
  activity_layout_build("6.2 mmol/L","Glucose",false,false,0,"08:42 · Pasta",measure,NULL,&layout);
  check(&layout);assert(layout.content.title_point==24);
  activity_layout_build("Dlouhý název se třemi řádky a diakritikou","Dva dlouhé řádky metadat",true,false,0,
      "An alternate",measure,NULL,&layout);
  check(&layout);assert(!layout.summary&&layout.footer_left==60);
  activity_layout_build("A long title that needs the whole main area above its anchored footer",
      "Supporting source information",false,false,0,"Second activity",measure,NULL,&layout);
  check(&layout);assert(!layout.summary);
  activity_layout_build_at("08:42","Pasta",false,true,0,NULL,130,measure,NULL,&layout);
  check(&layout);assert(layout.content.title_top>=130&&!layout.summary);
  activity_layout_build_at("A longer activity with a short notification above it",
      "Two metadata lines that remain legible",false,false,0,NULL,130,measure,NULL,&layout);
  check(&layout);assert(layout.content.title_top>=130&&!layout.summary);
  return 0;
}
