#include <assert.h>
#include "../src/c/activity_presentation.h"
#include "../src/c/generated_body_metrics.h"
#include "../src/c/generated_clock_glyphs.h"

static MusicTextMetrics measure(void *context,const char *text,int point) {
  (void)context;int count=0;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor){body_metric_next_codepoint(&cursor);++count;}
  return (MusicTextMetrics){count*point/2,body_text_ascent(text,point),body_text_descent(text,point)};
}
static MusicTextMetrics clock_measure(void *context,const char *text,int point) {
  (void)context;int width=0,ascent=0,descent=0;
  for(const unsigned char *at=(const unsigned char *)text;*at;++at) {
    const ClockGlyph *glyph=NULL;
    for(unsigned i=0;i<CLOCK_GLYPH_COUNT;++i)if(clock_glyphs[i].code==*at)glyph=&clock_glyphs[i];
    assert(glyph);
    width+=glyph->advance;
    if(glyph->y_max>ascent)ascent=glyph->y_max;
    if(-glyph->y_min>descent)descent=-glyph->y_min;
  }
  return (MusicTextMetrics){(width*point+500)/1000,(ascent*point+999)/1000,(descent*point+999)/1000};
}
static void clock_check(const ActivityLayout *layout,int top,bool image) {
  const MusicLayout *value=&layout->content;
  const MusicTextMetrics number=clock_measure(NULL,value->title.text[0],value->title_point);
  assert(layout->clock&&value->title.count==1&&value->title.complete);
  assert(value->title_top>=top&&number.width<=180&&layout->main_left>=10);
  assert(layout->main_left+number.width<=190);
  assert(value->title_baseline[0]==160);
  assert(value->title_bottom+6<=value->metadata_top);
  assert(value->artist_point==20&&value->artist.count<=(image?2:1));
  for(int i=0;i<value->artist.count;++i) {
    const MusicTextMetrics label=measure(NULL,value->artist.text[i],20);
    assert(label.width<=layout->footer_width);
    assert(layout->footer_left+label.width<=190);
  }
  if(layout->summary) {
    assert(!image);
    assert(layout->summary_baseline<=216);
    assert(layout->summary_baseline-measure(NULL,"Music",14).ascent>=
        value->artist_baseline[0]+body_text_descent(value->artist.text[0],20)+6);
  }
}
static void text_check(const ActivityLayout *layout) {
  const MusicLayout *value=&layout->content;
  assert(!layout->clock&&value->title_top>=92);
  assert(value->title.count<=3&&value->artist.count<=2&&value->title_point==24);
  assert(value->artist_point==20);
  if(value->artist.count) {
    const int i=value->artist.count-1;
    assert(value->artist_baseline[i]+body_text_descent(value->artist.text[i],20)==216);
  }
  for(int i=0;i<value->artist.count;++i)
    assert(measure(NULL,value->artist.text[i],20).width<=190-layout->footer_left);
}
int main(void) {
  ActivityLayout layout;
  activity_layout_build("08:42","10 min timer",false,true,0,"Night Trouble",measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,false);
  assert(layout.content.title_point==58&&layout.summary);
  activity_layout_build("00:36","1 min timer",false,true,2,NULL,measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,false);
  assert(layout.content.title_point==58&&!layout.support&&!layout.summary);
  assert(!strcmp(layout.content.artist.text[0],"1 min timer"));
  activity_layout_build("00:36","A meaningful custom timer name that does not fit",false,true,2,
      NULL,measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,false);
  assert(layout.content.title_point==58&&!layout.content.artist.complete);
  activity_layout_build("08:42","10 min timer",false,true,0,"Night Trouble",measure,clock_measure,NULL,&layout);
  const int numeric_home=layout.content.title_baseline[0];
  activity_layout_build("08:42","10 min timer",false,true,2,"Night Trouble",measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,false);
  assert(layout.content.title_baseline[0]==numeric_home&&layout.footer_left==58);
  activity_layout_build("08:42","",false,true,2,NULL,measure,clock_measure,NULL,&layout);
  assert(layout.marks_left==82&&!layout.summary&&layout.footer_width==0);
  activity_layout_build("08:42","ẤỨŘÍ",false,true,2,"Music",measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,false);
  const char *clocks[]={"00:00","59:59","1:14:56","24:00:00","1193046:28:15"};
  for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);++i) {
    activity_layout_build(clocks[i],"A long supporting title with accents Řeřicha",false,true,2,
        "A secondary",measure,clock_measure,NULL,&layout);
    clock_check(&layout,92,false);
    assert(!layout.content.artist.complete);
    assert(!strcmp(layout.content.title.text[0],clocks[i]));
  }
  activity_layout_build_at("08:42","Pasta",false,true,0,NULL,130,measure,clock_measure,NULL,&layout);
  clock_check(&layout,130,false);assert(layout.content.title_point==38&&!layout.summary);
  activity_layout_build("08:42","Two metadata lines remain legible",true,true,2,
      "Must not squeeze this in",measure,clock_measure,NULL,&layout);
  clock_check(&layout,92,true);assert(!layout.summary&&layout.footer_left==96);
  activity_layout_build("6.2 mmol/L","Glucose",false,false,0,"08:42 · Pasta",measure,clock_measure,NULL,&layout);
  text_check(&layout);
  activity_layout_build("Dlouhý název se třemi řádky a diakritikou","Dva dlouhé řádky metadat",true,false,0,
      "An alternate",measure,clock_measure,NULL,&layout);
  text_check(&layout);assert(!layout.summary&&layout.footer_left==60);
  activity_layout_build_at("A longer activity with a short notification above it",
      "Two metadata lines that remain legible",false,false,0,NULL,130,measure,clock_measure,NULL,&layout);
  text_check(&layout);assert(layout.content.title_top>=130&&!layout.summary);
  activity_layout_pair_at("08:42","Rest · Run","Elapsed 24:18",false,true,1,1,
      "Left 4:30 · Pasta",1,false,92,measure,clock_measure,NULL,&layout);
  assert(layout.clock&&layout.support&&layout.content.title_point==48);
  assert(layout.content.title_top>=92&&layout.content.title_bottom+6<=layout.content.metadata_top);
  assert(layout.support_baseline-measure(NULL,"Elapsed 24:18",14).ascent>=
      layout.content.artist_baseline[0]+measure(NULL,"Rest · Run",20).descent+4);
  assert(layout.summary&&layout.summary_width==162);
  activity_layout_pair_at("72%","Charge","ETA 37:00",true,true,1,0,
      "Left 4:30 · Pasta",0,true,92,measure,clock_measure,NULL,&layout);
  assert(layout.support&&layout.meter&&!layout.summary);
  assert(layout.support_left==60&&layout.meter_left==60&&layout.meter_width==130);
  assert(layout.content.title_top>=92&&layout.content.title_bottom<layout.content.metadata_top);
  activity_layout_pair_at("24:00:00","Pasta","Elapsed 0:00",false,true,2,1,
      NULL,0,false,130,measure,clock_measure,NULL,&layout);
  assert(layout.support&&!layout.summary&&layout.content.title_top>=130);
  activity_layout_pair_at("A long title that keeps its body type size","A caption",
      "Position 1:42",false,false,0,0,NULL,0,false,92,measure,clock_measure,NULL,&layout);
  assert(!layout.clock&&layout.content.title_point==24&&layout.content.title_top>=92);
  assert(layout.support);
  return 0;
}
