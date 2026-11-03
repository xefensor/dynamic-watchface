#include <assert.h>
#include "../src/c/activity_presentation.h"
#include "../src/c/generated_body_metrics.h"
#include "../src/c/generated_clock_glyphs.h"

static MusicTextMetrics measure(void *context,const char *text,int point) {
  (void)context;int width=0;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor){const uint32_t code=body_metric_next_codepoint(&cursor);width+=point/(code==' '?4:2);}
  return (MusicTextMetrics){width,body_text_ascent(text,point),body_text_descent(text,point)};
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
static void check(const ActivityLayout *layout,const char *identity,const char *role,
    const char *support,const char *status,const char *support_status,int top) {
  const MusicTextMetrics heading=measure(NULL,identity,16);
  assert(layout->identity_baseline-heading.ascent>=top);
  assert(layout->identity_baseline+heading.descent<=top+ACTIVITY_PRIMARY_ICON);
  if(!layout->main.count) {
    assert(!layout->secondary&&!layout->support&&!layout->meter);return;
  }
  assert(layout->main.count<=3);
  assert(layout->width==180);
  int bottom=top+ACTIVITY_PRIMARY_ICON;
  for(int i=0;i<layout->main.count;++i) {
    const MusicTextMetrics content=(layout->vector?clock_measure:measure)(NULL,
        layout->main.text[i],layout->point);
    assert(content.width<=layout->width);
    assert(layout->left>=10&&layout->left+content.width<=190);
    assert(layout->main_baseline[i]-content.ascent>=bottom+2);
    bottom=layout->main_baseline[i]+content.descent;
  }
  const char *detail[]={role,status,support,support_status};
  const int points[]={14,11,16,11};
  const int baseline[]={layout->role_baseline,layout->status_baseline,
      layout->support_baseline,layout->support_status_baseline};
  for(unsigned i=0;i<4;++i)if(baseline[i]) {
    if(i==1&&layout->header_status) {
      assert(baseline[i]==layout->identity_baseline);continue;
    }
    const MusicTextMetrics content=measure(NULL,detail[i],points[i]);
    assert(baseline[i]-content.ascent>=bottom+3);
    bottom=baseline[i]+content.descent;
  } else if(i<2)assert(!detail[i][0]);
  assert(bottom<=MUSIC_BOTTOM);
  if(layout->meter){assert(layout->meter_top>=bottom+4);bottom=layout->meter_top+4;}
  if(layout->secondary)assert(bottom+4<=ACTIVITY_SECONDARY_TOP);
}
static void build(const char *identity,const char *main,const char *role,const char *support,
    const char *status,const char *support_status,bool vector,bool scalar,bool secondary,
    bool meter,int top,ActivityLayout *layout) {
  activity_layout_build(identity,main,role,support,status,support_status,vector,scalar,
      secondary,meter,top,measure,clock_measure,NULL,layout);
  check(layout,identity,role,support,status,support_status,top);
}
int main(void) {
  ActivityLayout layout,without;
  build("Timer","00:36","Left","","","",true,false,true,false,92,&layout);
  assert(layout.vector&&layout.point==48&&layout.left>10&&layout.secondary);
  assert(!strcmp(layout.main.text[0],"00:36"));
  build("Timer","00:36","","","","",true,false,true,false,92,&layout);
  assert(layout.secondary&&!layout.role_baseline);
  assert(activity_secondary_fits("Timer","00:36","Last reported 35s ago",measure,NULL));
  assert(activity_secondary_identity_width("Last reported 35s ago",measure,NULL)>=24);
  assert(!activity_secondary_fits("Timer","Elapsed 1193046:28:15","",measure,NULL));
  assert(!activity_secondary_fits("Timer","00:36","Last reported time unknown",measure,NULL));
  assert(ACTIVITY_PRIMARY_ICON==24&&ACTIVITY_SECONDARY_ICON==20);
  assert(ACTIVITY_PRIMARY_IDENTITY_LEFT==42&&ACTIVITY_PRIMARY_IDENTITY_WIDTH==148);
  build("Timer","00:36","Left","Elapsed 0:24","","",true,false,true,false,92,&layout);
  assert(layout.support);
  build("Timer","00:36","Left","Elapsed 0:24","","",true,false,false,false,92,&without);
  assert(layout.point==without.point&&layout.main_baseline[0]==without.main_baseline[0]);
  build("Timer","00:36","Left","","Last reported 35s ago","",true,false,true,false,92,&layout);
  assert(layout.header_status&&layout.secondary);
  build("Timer","00:36","","","Paused","",true,false,true,false,92,&layout);
  assert(layout.header_status&&layout.secondary);
  const char *clocks[]={"00:00","59:59","1:14:56","24:00:00","1193046:28:15"};
  for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);++i) {
    build("Intervals",clocks[i],"Rest","Elapsed 24:18","","",true,false,true,false,92,&layout);
    assert(!strcmp(layout.main.text[0],clocks[i])&&layout.main.complete);
  }
  build("Timer","24:00:00","Left","Elapsed 0:00","","",true,false,false,false,130,&layout);
  assert(!layout.secondary&&layout.point>=26);
  build("Training","08:42","Rest","Elapsed 24:18","Paused · Last reported 2m ago",
      "Last reported 45s ago",true,false,true,true,92,&layout);
  assert(layout.support&&!layout.secondary);
  build("Training","08:42","Rest","Elapsed 24:18","Paused · Last reported 2m ago",
      "Last reported 45s ago",true,false,true,true,130,&layout);
  assert(layout.main.count&&!layout.support&&!layout.meter&&!layout.secondary);
  build("Long provider identity","Ẵ(","Ẵ(","","Paused · Last reported 2m ago","",
      false,false,true,false,130,&layout);
  assert(!layout.main.count);
  build("Glucose","6.2 mmol/L","Glucose","","","",false,true,true,false,92,&layout);
  assert(layout.main.complete&&!strcmp(layout.main.text[0],"6.2 mmol/L"));
  build("Battery","72%","Charge","ETA 37:00","","",true,true,true,true,92,&layout);
  assert(layout.support&&layout.meter&&!layout.secondary);
  build("Music","Night Trouble","Track","Indie band","","",false,false,true,false,92,&layout);
  assert(layout.point==24&&layout.support);
  build("Music","Better Than The Cyberpunk...","","","","",false,false,false,false,92,&layout);
  assert(layout.left==10&&layout.width==180&&layout.point==24);
  assert(layout.main.count==2&&!strcmp(layout.main.text[0],"Better Than The"));
  assert(!strcmp(layout.main.text[1],"Cyberpunk..."));
  assert(!layout.role_baseline&&!layout.support&&!layout.status_baseline);
  build("Travel","A long title that needs its own readable lines before optional activities",
      "Route","","","",false,false,true,false,92,&layout);
  assert(layout.point==24&&!layout.secondary&&!layout.main.complete&&layout.main.consumed);
  build("Řeřicha","Dlouhý název se třemi řádky a diakritikou","Cíl","","","",
      false,false,true,false,130,&layout);
  assert(layout.point==24&&!layout.secondary);
  build("Counter","123456789012345678","Items","","","",
      false,true,true,false,92,&layout);
  assert(layout.point==20&&!strcmp(layout.main.text[0],"123456789012345678"));
  assert(layout.role_baseline-measure(NULL,"Items",14).ascent>=92+ACTIVITY_PRIMARY_ICON+3);
  build("Measurements","1234567890123456789012345678901234567890 mmol/L","Value",
      "","","",false,true,true,false,92,&layout);
  assert(!strcmp(layout.main.text[0],"Open app for"));
  assert(!layout.vector);
  // Layout storage has one wrapped item, rather than a copied title/artist
  // composition. Both draw and tick scheduling use this same bounded plan.
  assert(sizeof(ActivityLayout)<sizeof(MusicLayout));
  return 0;
}
