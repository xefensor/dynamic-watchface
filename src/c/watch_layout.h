#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define WATCH_LAYOUT_SLOT_COUNT 5

typedef enum { WATCH_LAYOUT_CLASSIC = 0 } WatchLayoutStyle;
typedef struct { int16_t x, y, width, height; } WatchLayoutRect;
typedef struct { WatchLayoutRect rect; uint8_t font_size; } WatchSlotGeometry;
typedef struct {
  WatchLayoutRect date, time, minutes;
  WatchSlotGeometry slots[WATCH_LAYOUT_SLOT_COUNT];
  WatchLayoutRect context, band;
  int16_t divider_x, divider_top, divider_bottom;
  bool compact_header;
} WatchLayoutGeometry;

static inline int watch_layout_interpolate(int full, int peek, int height) {
  if (height >= 228) { return full; }
  if (height <= 171) { return peek; }
  const int distance = full - peek;
  return peek + (distance * (height - 171) + (distance >= 0 ? 28 : -28)) / 57;
}

// All positions are native 200px-wide PT2 coordinates. A dynamic panel yields
// to the system Timeline Peek, while its compact clock remains stable.
static inline WatchLayoutGeometry watch_layout_geometry(int style, int height,
                                                         bool large, bool contextual) {
  (void)style;
  if (height < 171) { height = 171; }
  if (height > 228) { height = 228; }
  WatchLayoutGeometry result = {0};
  result.compact_header = contextual;
  result.date = (WatchLayoutRect){8,0,184,28};
  result.time = (WatchLayoutRect){4,watch_layout_interpolate(48,30,height),192,78};
  int row_y = watch_layout_interpolate(148,112,height);
  int row_height = height - row_y;
  int font = large ? 36 : 32;
  if (contextual) {
    result.time = (WatchLayoutRect){8,-3,84,36};
    result.date = (WatchLayoutRect){98,5,94,22};
    if (height >= 228) {
      row_y = 34;
      row_height = 36;
      font = large ? 26 : 20;
      result.context = (WatchLayoutRect){0,78,200,150};
      result.band = result.context;
    } else {
      // Keep both readings side by side as the system takes the context area.
      row_y = 34;
      row_height = height - row_y;
      result.band = (WatchLayoutRect){0,row_y,200,row_height};
    }
  } else {
    result.band = (WatchLayoutRect){0,row_y,200,row_height};
  }
  result.slots[0] = (WatchSlotGeometry){{8,row_y,88,row_height},font};
  result.slots[1] = (WatchSlotGeometry){{104,row_y,88,row_height},font};
  result.divider_x = 100;
  result.divider_top = row_y + (row_height > 36 ? 12 : 4);
  result.divider_bottom = row_y + row_height - (row_height > 36 ? 12 : 4);
  return result;
}

// Used while fitting text to a pixel width, never as a fixed character limit.
static inline bool watch_layout_pop_utf8(char *text) {
  if (text == NULL) { return false; }
  size_t length = strlen(text);
  if (length == 0) { return false; }
  size_t at = length - 1;
  while (at > 0 && ((unsigned char)text[at] & 0xc0) == 0x80) { --at; }
  text[at] = '\0';
  return true;
}

// Copy whole UTF-8 characters for the compact localized weekday header.
static inline void watch_layout_abbreviate(const char *source, char *result,
                                            size_t capacity, size_t characters) {
  if (capacity == 0) { return; }
  size_t at = 0;
  while (source != NULL && *source && characters) {
    size_t bytes = 1;
    while (source[bytes] && ((unsigned char)source[bytes] & 0xc0) == 0x80) { ++bytes; }
    if (at + bytes >= capacity) { break; }
    memcpy(result + at, source, bytes);
    at += bytes;
    source += bytes;
    --characters;
  }
  result[at] = '\0';
}

static inline void watch_layout_copy_utf8(const char *source, char *result, size_t capacity) {
  // The byte capacity also bounds the maximum number of complete characters.
  watch_layout_abbreviate(source,result,capacity,capacity);
}

static inline bool watch_layout_split_time(const char *text,
                                           char hours[3], char minutes[3]) {
  if (hours == NULL || minutes == NULL) { return false; }
  hours[0] = minutes[0] = '\0';
  if (text == NULL) { return false; }
  size_t length = 0;
  while (length < 6 && text[length]) { ++length; }
  if (length != 4 && length != 5) { return false; }
  const size_t colon = length - 3;
  if (text[colon] != ':' || text[colon + 1] < '0' || text[colon + 1] > '5' ||
      text[colon + 2] < '0' || text[colon + 2] > '9') { return false; }
  int hour = 0;
  for (size_t i = 0; i < colon; ++i) {
    if (text[i] < '0' || text[i] > '9') { return false; }
    hour = hour * 10 + text[i] - '0';
  }
  if (hour > 23) { return false; }
  memcpy(hours, text, colon);
  hours[colon] = '\0';
  memcpy(minutes, text + colon + 1, 2);
  minutes[2] = '\0';
  return true;
}
