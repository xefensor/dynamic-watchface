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
  assert(activity_view_needs_tick(&main,false,&other,true,110,false));
  assert(!activity_view_needs_tick(&main,false,&other,true,160,false));
  main=card();main.source_state=PAV_PAUSED;
  assert(!activity_is_result(&main));
  assert(activity_view_needs_tick(&main,false,NULL,false,110,true));
  main.main.type=PAV_VALUE;main.main.flags=PAV_RETAIN;
  main.main.base_seconds=main.main.anchor_at=0;
  assert(!activity_view_needs_tick(&main,false,NULL,false,110,true));
  assert(activity_view_needs_tick(&main,true,NULL,false,110,true));

  /* A saved native snapshot is visibly uncertain once per shared observation,
   * without treating a separately stopped or estimated item as equivalent. */
  main=card();main.flags=PAV_SOURCE_UNCONFIRMED;
  const uint8_t cached=ACTIVITY_UNCERTAIN|ACTIVITY_AGE;
  uint8_t marks=activity_status_marks(&main,&main.main,110,true,true);
  uint8_t support=activity_status_marks(&main,&main.support,110,true,false);
  assert(marks==cached&&support==cached);
  assert(activity_support_marks(&main,marks,support)==0);
  main.support.observed_at=99;
  assert(activity_support_marks(&main,marks,support)==cached);
  main.support.observed_at=main.main.observed_at;
  main.support.flags|=PAV_ESTIMATED;
  assert(activity_support_marks(&main,marks,support)==cached);
  main.support.flags&=(uint8_t)~(PAV_ESTIMATED|PAV_RUNNING);
  main.support.anchor_at=0;
  support=activity_status_marks(&main,&main.support,110,true,false);
  assert(activity_support_marks(&main,marks,support)==ACTIVITY_PAUSED);
  assert(activity_support_marks(&main,0,cached)==cached);
  assert(activity_status_marks(&main,&main.main,110,false,true)==cached);

  /* Finished elapsed values are final results, not paused activity clocks. */
  main.source_state=PAV_COMPLETED;main.flags|=PAV_SHOW_TERMINAL;
  assert(activity_is_result(&main));
  assert(activity_status_marks(&main,&main.support,110,true,false)==cached);
  main.source_state=PAV_CANCELLED;
  assert(activity_is_result(&main));
  assert(!(activity_status_marks(&main,&main.support,110,true,false)&ACTIVITY_PAUSED));
  main.source_state=PAV_PAUSED;
  assert(activity_status_marks(&main,&main.main,110,true,true)==(cached|ACTIVITY_PAUSED));
  assert(activity_status_marks(&main,&main.main,110,true,false)==cached);
  /* Identity repeats are omitted, while metric roles remain local. */
  PavCard named={0};strcpy(named.strings,"Timer");strcpy(named.strings+6,"Rest");
  named.caption_length=5;named.main.label_length=5;
  assert(!strcmp(activity_identity(&named),"Timer"));
  assert(!activity_role(&named,&named.main)[0]);
  named.main.label_offset=6;named.main.label_length=4;
  assert(!strcmp(activity_role(&named,&named.main),"Rest"));

  main=card();main.flags=PAV_SOURCE_UNCONFIRMED;
  main.main.type=PAV_VALUE;main.support.type=PAV_NONE;
  assert(activity_view_needs_tick(&main,false,NULL,false,135,true));
  assert(!activity_view_needs_tick(&main,false,NULL,false,160,true));
  assert(activity_view_needs_tick(&named,false,&main,true,135,true));
  assert(!activity_view_needs_tick(&named,false,&main,false,135,true));
  char status[64];
  activity_status_text(&main.main,cached,135,status,sizeof(status));
  assert(!strcmp(status,"Last reported 35s ago"));
  assert(!strstr(status,"<1m"));
  activity_status_text(&main.main,cached|ACTIVITY_PAUSED,160,status,sizeof(status));
  assert(!strcmp(status,"Paused · Last reported 1m ago"));
  activity_status_text(&main.main,cached,99,status,sizeof(status));
  assert(!strcmp(status,"Last reported time unknown"));
  activity_status_text(&main.main,ACTIVITY_UNCERTAIN,110,status,sizeof(status));
  assert(!strcmp(status,"Estimated"));
  activity_status_text(&main.main,0,110,status,sizeof(status));assert(!status[0]);
  return 0;
}
