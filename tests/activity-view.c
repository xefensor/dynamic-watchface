#include <assert.h>
#include <string.h>
#include "../src/c/activity_presentation.h"

static PavCard card(void) {
  PavCard result={0};
  strcpy(result.provider,"01234567-89ab-cdef-0123-456789abcdef");strcpy(result.id,"run");
  result.source_state=PAV_ACTIVE;result.source_revision=1;
  result.source_observed_at=100;result.source_valid_until=1000;
  result.main.type=PAV_CLOCK;result.main.flags=PAV_RUNNING|PAV_RETAIN;
  result.main.observed_at=100;result.main.valid_until=800;
  result.main.base_seconds=200;result.main.anchor_at=100;
  result.support=result.main;result.support.flags|=PAV_ELAPSED;
  result.support.base_seconds=50;
  return result;
}
int main(void) {
  PavCard main=card(),other=card();strcpy(other.id,"other");
  assert(activity_view_needs_tick(&main,false,NULL,false,110,true));
  main.main.flags&=(uint8_t)~PAV_RUNNING;main.main.anchor_at=0;
  assert(!activity_view_needs_tick(&main,false,NULL,false,110,true));
  assert(activity_view_needs_tick(&main,true,NULL,false,110,true));
  assert(!activity_view_needs_tick(&main,true,NULL,false,800,true));
  assert(!activity_view_needs_tick(&main,false,&other,false,110,true));
  assert(activity_view_needs_tick(&main,false,&other,true,110,true));
  other.main.base_seconds=0;
  assert(!activity_view_needs_tick(&main,false,&other,true,110,true));
  other.main.flags|=PAV_ELAPSED;
  assert(activity_view_needs_tick(&main,false,&other,true,110,true));
  other.flags=PAV_PRIVATE;
  assert(!activity_view_needs_tick(&main,false,&other,true,110,false));
  main=card();main.source_state=PAV_PAUSED;
  assert(activity_view_needs_tick(&main,false,NULL,false,110,true));
  main.main.type=PAV_VALUE;main.main.flags=PAV_RETAIN;
  main.main.base_seconds=main.main.anchor_at=0;
  assert(!activity_view_needs_tick(&main,false,NULL,false,110,true));
  assert(activity_view_needs_tick(&main,true,NULL,false,110,true));
  return 0;
}
