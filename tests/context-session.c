#include <assert.h>
#include "../src/c/context_session.h"

int main(void) {
  DynamicContextSession state;ComplicationSet content;
  memset(&content,0xff,sizeof(content));
  dynamic_context_session_init(&state,40);
  assert(state.token==41&&state.rebaseline&&state.awaiting_baseline);
  assert(dynamic_context_request_marker(&state)==41&&state.baseline_in_flight);
  const uint32_t old_generation=state.generation;
  dynamic_context_session_reset(&state,&content,true);
  assert(state.token==42&&state.generation!=old_generation);
  dynamic_context_request_acknowledged(&state);
  assert(state.rebaseline); // ACK from before rotation cannot open the new gate.
  assert(complications_zero((const uint8_t *)&content,sizeof(content)));
  assert(dynamic_context_request_marker(&state)==42);
  dynamic_context_request_failed(&state);dynamic_context_request_acknowledged(&state);
  assert(state.rebaseline);
  assert(dynamic_context_request_marker(&state)==42);
  dynamic_context_request_acknowledged(&state);
  assert(!state.rebaseline&&state.awaiting_baseline);
  state.awaiting_baseline=false;
  assert(dynamic_context_request_marker(&state)==1);
  memset(&content,0xff,sizeof(content));
  assert(!dynamic_context_receive(&state,&content,false,NULL,0,42));
  assert(complications_zero((const uint8_t *)&content,sizeof(content)));
  dynamic_context_session_init(&state,CONTEXT_MAX_SESSION_TOKEN);
  assert(state.token==2);
  return 0;
}
