#include <assert.h>
#include "../src/c/watch_layout.h"
#include "../src/c/reading_presentation.h"

int main(void) {
  assert(watch_clock_format(-1,-1)==WATCH_CLOCK_FOLLOW);
  assert(watch_clock_format(-1,0)==WATCH_CLOCK_24);
  assert(watch_clock_format(-1,1)==WATCH_CLOCK_12);
  assert(watch_clock_format(WATCH_CLOCK_FOLLOW,1)==WATCH_CLOCK_FOLLOW);
  assert(watch_clock_format(99,0)==WATCH_CLOCK_24);
  assert(watch_clock_format(1,-1)==WATCH_CLOCK_FOLLOW);
  assert(watch_clock_format(13,-1)==WATCH_CLOCK_FOLLOW);
  assert(watch_clock_uses_12(WATCH_CLOCK_FOLLOW,false));
  assert(!watch_clock_uses_12(WATCH_CLOCK_FOLLOW,true));
  assert(watch_clock_uses_12(WATCH_CLOCK_12,true));
  assert(!watch_clock_uses_12(WATCH_CLOCK_24,false));
  for (int large=0;large<=1;++large) {
    const WatchPose idle=watch_pose_idle(large), active=watch_pose_context(large,false);
    const WatchPose notice=watch_pose_context(large,true), peek=watch_pose_peek(large);
    const WatchPose summary=watch_pose_summary(large);
    assert(summary.clock_size==peek.clock_size&&summary.row_y==peek.row_y&&summary.peek==0);
    assert(watch_context_visibility(summary)==1&&watch_context_visibility(peek)==0);
    assert(watch_context_visibility(active)==1&&watch_context_visibility(notice)==1);
    assert(watch_context_visibility(watch_pose_at_height(summary,large,171))==0);
    assert(active.row_y+active.label_y<(large?96:92));
    if(large) {
      assert(active.icon_size==10&&active.icon_radius==6);
      assert(50+active.icon_x+active.icon_radius<50+active.value_x-72/2);
    }
    assert(watch_date_split(idle.context)==0 && watch_date_split(peek.context)==0);
    assert(watch_date_split(active.context)==1);
    assert(active.date_y-5==19 && active.date_y+16==40);
    assert(watch_date_split(0.55f)==0 && watch_date_split(0.8f)>0);
    assert(idle.clock_x==100 && idle.clock_y==108 && idle.context==0);
    assert(active.clock_x==58 && active.context==1 && active.music==1);
    assert(notice.notice==1 && notice.row_y==active.row_y);
    for (int height=160;height<=228;++height) {
      WatchPose p=watch_pose_at_height(active,large,height);
      assert(p.clock_size>=40 && p.clock_size<=76);
      assert(p.row_y+p.value_y<height);
      assert(p.row_y+p.icon_y+p.icon_radius<height);
      assert(p.date_y<p.clock_y && p.clock_y<p.row_y);
      if(height==171) {
        assert(p.clock_x==peek.clock_x && p.clock_y==peek.clock_y);
        assert(p.row_y==peek.row_y && p.context==0);
      }
    }
    const WatchPose overshoot=watch_pose_at_height(active,large,166);
    assert(overshoot.peek>1 && overshoot.row_y>peek.row_y);
    for (int step=0;step<=100;++step) {
      const float amount=watch_ease(step/100.0f);
      const WatchPose p=watch_pose_between(idle,active,amount);
      const WatchPose reverse=watch_pose_between(active,idle,1-amount);
      assert(p.clock_y<p.row_y && p.clock_size>=38 && p.clock_size<=85);
      assert(p.icon_x<=0 && p.icon_x>=(large?-36:-25) && p.value_x>=0 && p.value_x<=12);
      assert(reverse.icon_x-p.icon_x<0.0001f && reverse.icon_x-p.icon_x>-0.0001f);
      assert(reverse.number_size-p.number_size<0.0001f && reverse.number_size-p.number_size>-0.0001f);
      assert(watch_smooth(p.context)>=0 && watch_smooth(p.context)<=1);
      const float split=watch_date_split(p.context);
      assert(split>=0 && split<=1);
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
