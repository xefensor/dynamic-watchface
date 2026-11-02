#pragma once

#include "context_data.h"

// Compatibility session for the transient music/notification adapter. Reading
// caches belong to Core; this host-owned state never persists private content.
typedef struct {
  uint32_t token, generation, request_generation;
  bool rebaseline, awaiting_baseline, baseline_in_flight;
} DynamicContextSession;

static inline void dynamic_context_session_init(DynamicContextSession *session,
                                                uint32_t previous) {
  memset(session,0,sizeof(*session));
  session->token=context_next_session_token(previous);
  session->rebaseline=session->awaiting_baseline=true;
}
static inline void dynamic_context_session_reset(DynamicContextSession *session,
                                                 ComplicationSet *context,bool advance) {
  if(advance)session->token=context_next_session_token(session->token);
  ++session->generation;
  session->rebaseline=session->awaiting_baseline=true;
  session->baseline_in_flight=false;
  memset(context,0,sizeof(*context));
}
static inline uint32_t dynamic_context_request_marker(DynamicContextSession *session) {
  session->baseline_in_flight=session->rebaseline;
  session->request_generation=session->generation;
  return session->rebaseline||session->awaiting_baseline?session->token:1;
}
static inline void dynamic_context_request_failed(DynamicContextSession *session) {
  session->baseline_in_flight=false;
}
static inline void dynamic_context_request_acknowledged(DynamicContextSession *session) {
  if(session->baseline_in_flight&&session->request_generation==session->generation)
    session->rebaseline=false;
  session->baseline_in_flight=false;
}
static inline bool dynamic_context_receive(DynamicContextSession *session,
                                            ComplicationSet *context,bool connected,
                                            const uint8_t *bytes,size_t length,uint32_t marker) {
  const bool valid=connected&&length<=COMPLICATION_MAX_BYTES&&
      context_receive(bytes,length,context,&session->awaiting_baseline,
                      context_session_token_matches(marker,session->token));
  if(!valid)memset(context,0,sizeof(*context));
  return valid;
}
static inline uint32_t dynamic_context_prune(ComplicationSet *context,time_t now) {
  for(unsigned i=0;i<COMPLICATION_SLOT_COUNT;++i)
    if(!context_slot_fresh(&context->slots[i],now))
      memset(&context->slots[i],0,sizeof(context->slots[i]));
  return context_next_expiry(context,now);
}
