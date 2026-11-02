#include <assert.h>
#include <stdio.h>
#include <pebble-watchface-core/pebble-watchface-core.h>
#include <pebble-watchface-core/readings.h>
#include <pebble-activities/pebble-activity-view.h>
#include "../src/c/context_presentation.h"
#include "../src/c/context_session.h"

typedef struct {
  ContextMode mode;bool heading_only,has_music;
  ComplicationSlot title,subtitle,music_title,music_artist;
} PreviousRenderContext;
typedef struct {
  WatchfaceCore core;
  PreviousRenderContext from,to;
  char music_identity[COMPLICATION_MAX_TEXT_BYTES+1];
  uint16_t music_offset,music_next;
  bool music_visible;
} PreviousContentState;
typedef struct {
  WfcReadings core;
  ComplicationSet legacy_context;
  DynamicContextSession session;
  PavState activities;
  RenderContext from,to;
  char music_identity[PAV_TEXT_BYTES+1];
  char activity_identity[PAV_PROVIDER_BYTES+PAV_ID_BYTES+2];
  uint16_t music_offset,music_next;
  bool music_visible;
  void *activity_timer;
} ActivityContentState;

int main(void) {
  // Compare all changed content storage together. Moving the legacy set from
  // Core into the host is not itself counted as a saving.
  assert(sizeof(ActivityContentState)<sizeof(PreviousContentState));
  assert(sizeof(RenderContext)<sizeof(PreviousRenderContext));
  printf("content state: %lu -> %lu bytes (host ABI)\n",
      (unsigned long)sizeof(PreviousContentState),(unsigned long)sizeof(ActivityContentState));
  return 0;
}
