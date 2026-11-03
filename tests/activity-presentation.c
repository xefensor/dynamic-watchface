#include <assert.h>
#include "../src/c/activity_presentation.h"
#include "../src/c/generated_body_metrics.h"
#include "../src/c/generated_clock_glyphs.h"

static MusicTextMetrics measure(void *context,const char *text,int point) {
  (void)context;int width=0;
  if(point>32)point=32;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor){const uint32_t code=body_metric_next_codepoint(&cursor);width+=point/(code==' '?4:2);}
  return (MusicTextMetrics){width,body_text_ascent(text,point),body_text_descent(text,point)};
}
static MusicTextMetrics clock_measure(void *context,const char *text,int point) {
  (void)context;int width=0,ascent=0,descent=0;
  for(const unsigned char *at=(const unsigned char *)text;*at;++at) {
    const ClockGlyph *glyph=NULL;
    for(unsigned i=0;i<CLOCK_GLYPH_COUNT;++i)if(clock_glyphs[i].code==*at)glyph=&clock_glyphs[i];
    assert(glyph);width+=glyph->advance;
    if(glyph->y_max>ascent)ascent=glyph->y_max;
    if(-glyph->y_min>descent)descent=-glyph->y_min;
  }
  return (MusicTextMetrics){(width*point+500)/1000,(ascent*point+999)/1000,(descent*point+999)/1000};
}
static void check(ActivityLayout *layout,bool secondary,int top) {
  if(!layout->main.count)return;
  assert(layout->main.count<=3&&layout->footer.count<=2);
  int bottom=top+(secondary?26:4);
  for(int i=0;i<layout->main.count;++i) {
    const MusicTextMetrics m=(layout->vector?clock_measure:measure)(NULL,layout->main.text[i],layout->point);
    assert(m.width<=180);assert(layout->main_baseline[i]-m.ascent>=bottom);
    bottom=layout->main_baseline[i]+m.descent;
  }
  if(layout->role_baseline)assert(layout->role_baseline<layout->main_baseline[0]);
  for(int i=0;i<layout->support.count;++i) {
    assert(layout->support_baseline[i]>=bottom+6);
    bottom=layout->support_baseline[i]+measure(NULL,layout->support.text[i],layout->support_point).descent;
  }
  if(layout->meter) {
    assert(layout->meter_top==bottom+4);
    bottom=layout->meter_top+4;
  }
  int footer_top=216;
  for(int i=0;i<layout->footer.count;++i) {
    const MusicTextMetrics m=measure(NULL,layout->footer.text[i],layout->footer_point);
    assert(layout->footer_baseline[i]-m.ascent>=bottom);
    if(layout->footer_baseline[i]-m.ascent<footer_top)footer_top=layout->footer_baseline[i]-m.ascent;
    assert(layout->footer_baseline[i]+m.descent<=216);
    assert(m.width<=layout->footer_width);
  }
  if((layout->media&&layout->footer_left==60)||(!layout->media&&layout->footer.count))
    if(layout->icon_top<footer_top)footer_top=layout->icon_top;
  assert(bottom==footer_top-(footer_top<216?8:0));
  assert(layout->icon_top+layout->icon_size==216);
  if(secondary)assert(layout->secondary_baseline>=top+4&&layout->secondary_baseline<top+26);
}
static void build(const char *main,const char *role,const char *support,const char *footer,
    bool media,bool vector,bool scalar,bool secondary,bool image,bool meter,int top,ActivityLayout *layout) {
  activity_layout_build(main,role,support,footer,media,vector,scalar,secondary,image,meter,!media&&!vector&&!scalar?20:16,top,
      measure,vector?clock_measure:measure,NULL,layout);check(layout,secondary,top);
  if(role[0]&&!media&&layout->main.count) {
    const MusicTextMetrics ink=(vector?clock_measure:measure)(NULL,layout->main.text[0],layout->point);
    assert(layout->main_baseline[0]-ink.ascent==layout->role_baseline+measure(NULL,role,14).descent+4);
  }
}
int main(void) {
  ActivityLayout layout,alone;
  assert(activity_simple_unit("km")&&activity_simple_unit("mmol/L")&&activity_simple_unit("°C"));
  assert(!activity_simple_unit(":12 /km")&&!activity_simple_unit("–1")&&!activity_simple_unit("e+20"));
  build("00:36","","","Tea",false,true,false,true,false,false,92,&layout);
  assert(layout.main.count==1&&layout.vector&&layout.secondary&&layout.point>=40);
  assert(layout.footer.count==1&&layout.footer_baseline[0]>layout.main_baseline[0]);
  assert(!strcmp(layout.main.text[0],"00:36"));
  build("00:36","","","Tea",false,true,false,false,false,false,92,&alone);
  assert(alone.point>=layout.point);
  assert(alone.main_baseline[0]+clock_measure(NULL,"00:36",alone.point).descent==
      layout.main_baseline[0]+clock_measure(NULL,"00:36",layout.point).descent);
  const char *clocks[]={"00:00","59:59","1:14:56","24:00:00","1193046:28:15"};
  for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);++i){
    build(clocks[i],"","","Timer",false,true,false,true,false,false,92,&layout);
    assert(layout.main.count&&layout.main.complete&&!strcmp(layout.main.text[0],clocks[i]));
  }
  build("08:42","Rest","Elapsed 24:18","Evening run",false,true,false,true,false,false,92,&layout);
  assert(layout.role_baseline&&layout.support.count);
  build("Flower In A Hurricane [from the soundtrack]","","","Justice, Tame Impala",true,false,false,true,true,false,92,&layout);
  assert(layout.media&&layout.point==24&&layout.main.count<=2&&layout.icon_size==40&&layout.footer_left==60);
  assert(!layout.main.complete);
  build("Better Than The Cyberpunk Soundtrack","","","The Midnight",true,false,false,false,true,false,92,&layout);
  assert(layout.main.count==3&&layout.footer_point==20);
  build("Track","","","",true,false,false,false,false,false,92,&layout);
  assert(!layout.footer.count&&layout.footer_left==10);
  build("Petr","","","Messages",false,false,false,true,false,false,92,&alone);
  build("Petr","","I'm downstairs. Ready when you are?","Messages",false,false,false,true,false,false,92,&layout);
  assert(layout.main.count&&layout.support.count&&layout.icon_size==16);
  assert(layout.point==alone.point&&layout.secondary_baseline==alone.secondary_baseline);
  assert(layout.footer_baseline[0]==alone.footer_baseline[0]&&layout.icon_top==alone.icon_top);
  build("68%","","","Offline map",false,false,true,true,false,true,92,&layout);
  assert(layout.main.count&&layout.meter);
  build("Dlouhý název se třemi řádky a diakritikou","","","Acme",false,false,false,true,false,false,92,&layout);
  assert(layout.main.count<=2&&!layout.main.complete);
  build("00:36","","","Tea",false,true,false,false,false,false,130,&layout);
  assert(layout.main.count&&layout.point>=26);
  assert(ACTIVITY_SECONDARY_ICON==16&&ACTIVITY_SECONDARY_TOP==96);
  return 0;
}
