#include <assert.h>
#include "../src/c/context_effects.h"

int main(void) {
  ContextEffects value;
  uint8_t packet[]={0x78,0x56,0x34,0x12,0xef,0xcd,0xab,0x89,1};
  assert(context_effects_read(packet,sizeof(packet),&value));
  assert(value.session==0x12345678&&value.burst==0x89abcdef&&value.paused);
  assert(!context_effects_read(packet,8,&value));
  assert(!context_effects_read(NULL,9,&value));
  assert(!context_effects_read(packet,9,NULL));
  packet[8]=2;assert(!context_effects_read(packet,sizeof(packet),&value));
  packet[8]=0;assert(context_effects_read(packet,sizeof(packet),&value)&&!value.paused);
  uint32_t seen=0;
  assert(!context_effects_alert(0,&seen,1,false)&&seen==0);
  assert(context_effects_alert(1,&seen,1,false)&&seen==1);
  assert(!context_effects_alert(1,&seen,2,false)); // retry / art change
  assert(!context_effects_alert(2,&seen,0,false)&&seen==2);
  assert(!context_effects_alert(2,&seen,1,false)); // enable is not an event
  assert(!context_effects_alert(3,&seen,2,true)&&seen==3);
  assert(!context_effects_alert(3,&seen,2,false)); // leaving Quiet Time
  assert(!context_effects_alert(0,&seen,2,false)&&seen==3); // boundary/expiry
  assert(!context_effects_alert(2,&seen,2,false)); // old queued snapshot
  assert(context_effects_alert(4,&seen,2,false));
  return 0;
}
