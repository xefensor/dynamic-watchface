#include <assert.h>
#include "../src/c/watch_layout.h"

int main(void) {
  for (int large=0;large<=1;++large) {
    for (int contextual=0;contextual<=1;++contextual) {
      for (int height=171;height<=228;++height) {
        const WatchLayoutGeometry g=watch_layout_geometry(0,height,large,contextual);
        assert(g.slots[0].rect.x==6 && g.slots[1].rect.x==104);
        assert(g.slots[0].rect.width==g.slots[1].rect.width);
        assert(g.slots[0].rect.width==90);
        assert(g.slots[0].rect.x+g.slots[0].rect.width==96);
        assert(g.slots[1].rect.x+g.slots[1].rect.width==194);
        assert(g.slots[0].rect.y>=0);
        assert(g.slots[0].rect.y+g.slots[0].rect.height<=height-4);
        assert(g.slots[0].rect.y==g.slots[1].rect.y);
        if (height<228 || !contextual) { assert(g.context.width==0); }
        else {
          assert(g.context.x==6 && g.context.width==188);
          assert(g.context.y==106 && g.context.height==116);
          assert(g.context.y+g.context.height==222);
          assert(g.slots[0].rect.y+g.slots[0].rect.height<=g.context.y-6);
        }
      }
    }
  }
  const WatchLayoutGeometry peek=watch_layout_geometry(0,171,true,true);
  assert(peek.slots[0].rect.y==112 && peek.slots[0].rect.height==48);
  assert(peek.context.width==0);
  assert(watch_layout_clock_y(44,228,true)+13==40);
  assert(watch_layout_clock_y(44,228,true)+46==73);
  assert(watch_layout_clock_y(38,228,true)+11>=40);
  const int sizes[]={64,72,80,84,88,92,96,100,104,108};
  for (unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);++i) {
    for (int h=171;h<228;++h) {
      const int delta=watch_layout_clock_y(sizes[i],h+1,false)-watch_layout_clock_y(sizes[i],h,false);
      assert(delta>=0 && delta<=1);
    }
  }
  return 0;
}
