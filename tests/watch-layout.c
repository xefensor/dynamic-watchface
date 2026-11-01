#include <assert.h>
#include "../src/c/watch_layout.h"
#include "../src/c/reading_presentation.h"

int main(void) {
  for (int large=0;large<=1;++large) {
    for (int contextual=0;contextual<=1;++contextual) {
      for (int height=171;height<=228;++height) {
        const WatchLayoutGeometry g=watch_layout_geometry(0,height,large,contextual);
        assert(g.slots[0].rect.x==8 && g.slots[1].rect.x==104);
        assert(g.slots[0].rect.width==88 && g.slots[1].rect.width==88);
        assert(g.slots[0].rect.y==g.slots[1].rect.y);
        assert(g.slots[0].rect.height==g.slots[1].rect.height);
        assert(g.slots[0].rect.y>=0);
        assert(g.slots[0].rect.y+g.slots[0].rect.height<=height);
        assert(g.band.x==0 && g.band.width==200);
        assert(g.band.y+g.band.height==height);
        assert(g.divider_top<g.divider_bottom);
        assert(g.divider_bottom<=height);
        assert(g.time.y+g.time.height<=g.slots[0].rect.y);
        if (!contextual || height<228) { assert(g.context.width==0); }
        else {
          assert(g.context.x==0 && g.context.width==200);
          assert(g.context.y==78 && g.context.height==150);
          assert(g.slots[0].rect.y+g.slots[0].rect.height==70);
        }
        if (height<228) {
          const WatchLayoutGeometry next=watch_layout_geometry(0,height+1,large,contextual);
          assert(next.time.y>=g.time.y && next.time.y<=g.time.y+1);
          assert(next.time.width==g.time.width && next.time.height==g.time.height);
        }
      }
    }
  }
  char abbr[16];
  watch_layout_abbreviate("ČTVRTEK",abbr,sizeof(abbr),3);
  assert(strcmp(abbr,"ČTV")==0);
  watch_layout_abbreviate("ÚTERÝ",abbr,4,3);
  assert(strcmp(abbr,"ÚT")==0);
  char bounded[5];
  watch_layout_copy_utf8("ČTVRTEK",bounded,sizeof(bounded));
  assert(strcmp(bounded,"ČTV")==0);
  char hours[3],minutes[3];
  assert(watch_layout_split_time("9:05",hours,minutes));
  assert(strcmp(hours,"9")==0 && strcmp(minutes,"05")==0);
  assert(!watch_layout_split_time("24:05",hours,minutes));
  ReadingParts negative;
  assert(reading_parts("−10°C",&negative));
  char native[READING_PART_BYTES];
  reading_native_number(negative.number,native,sizeof(native));
  assert(strcmp(native,"-10")==0 && strcmp(negative.number,"−10")==0);
  assert(strcmp(negative.unit,"°C")==0);
  char tiny[2] = {'x','y'};
  reading_native_number(negative.number,tiny,1);
  assert(tiny[0]=='\0' && tiny[1]=='y');
  return 0;
}
