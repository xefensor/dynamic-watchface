#include <pebble.h>
#include "complication_data.h"
#include "context_data.h"
#include <pebble-watchface-core/pebble-watchface-core.h>
#include "watch_layout.h"
#include "reading_presentation.h"
#include "context_text_metrics.h"

#define PERSIST_KEY_BACKGROUND_COLOR 2
#define PERSIST_KEY_TEXT_COLOR 3
#define PERSIST_KEY_TEXT_SHADOW 5
#define PERSIST_KEY_LARGE_TEXT 6
#define PERSIST_KEY_USE_12_HOUR 7
#define PERSIST_KEY_LAYOUT_STYLE 16
#define PERSIST_KEY_SHOW_STATUS 19
#define PERSIST_KEY_COMPLICATION_FIRST 20
#define PERSIST_KEY_CONTEXT_SESSION 61

#define FULL_LAYOUT_HEIGHT 228
#define TIMELINE_LAYOUT_HEIGHT 171
#define COMPACT_FONT_ENTER_HEIGHT 205
#define COMPACT_FONT_EXIT_HEIGHT 215
#define TIMELINE_DATE_Y 25
#define TIMELINE_TIME_Y 45
#define LARGE_TIME_FONT_COUNT 8
#define COMPLICATION_FONT_COUNT 7

static Window *s_window;
static Layer *s_window_layer;
static Layer *s_background_layer;
static Layer *s_complication_layers[COMPLICATION_SLOT_COUNT];
static Layer *s_context_layer;
static Layer *s_status_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_shadow_layer;
static TextLayer *s_time_shadow_layer;
static GFont s_date_font;
static GFont s_time_font;
static GFont s_compact_date_font;
static GFont s_small_date_font;
static GFont s_compact_time_font;
static GFont s_large_date_font;
static GFont s_large_time_fonts[LARGE_TIME_FONT_COUNT];
static const uint8_t s_large_time_font_sizes[LARGE_TIME_FONT_COUNT] = {80,84,88,92,96,100,104,108};
static GFont s_large_compact_time_font;
static GFont s_complication_fonts[COMPLICATION_FONT_COUNT];
static const uint8_t s_complication_font_sizes[COMPLICATION_FONT_COUNT] = {44, 38, 32, 28, 24, 20, 18};
static GColor s_background_color;
static GColor s_text_color;
static bool s_compact_layout;
static bool s_text_shadow_enabled;
static bool s_large_text_enabled;
static bool s_use_12_hour;
static bool s_show_status;
static BatteryChargeState s_battery;
static WatchfaceCore s_core;
#define s_context s_core.context
#define s_complications s_core.complications
#define s_has_complications s_core.has_complications
#define s_phone_connected s_core.phone_connected
static AppTimer *s_context_timer;
static GPath *s_drop_path;
static GPath *s_small_drop_path;
static GPoint s_drop_points[] = {{0, -10}, {-7, 2}, {7, 2}};
static const GPathInfo s_drop_info = {.num_points = 3, .points = s_drop_points};
static GPoint s_small_drop_points[] = {{0, -8}, {-5, 1}, {5, 1}};
static const GPathInfo s_small_drop_info = {.num_points = 3, .points = s_small_drop_points};
static AppTimer *s_startup_timer;
static char s_date_buffer[40];
static char s_time_buffer[6];

static const char *const s_english_days[] = {
  "SUNDAY",
  "MONDAY",
  "TUESDAY",
  "WEDNESDAY",
  "THURSDAY",
  "FRIDAY",
  "SATURDAY"
};

static const char *const s_czech_days[] = {
  "NEDĚLE", "PONDĚLÍ", "ÚTERÝ", "STŘEDA", "ČTVRTEK", "PÁTEK", "SOBOTA"
};
static const char *const s_slovak_days[] = {
  "NEDEĽA", "PONDELOK", "UTOROK", "STREDA", "ŠTVRTOK", "PIATOK", "SOBOTA"
};
static const char *const s_german_days[] = {
  "SONNTAG", "MONTAG", "DIENSTAG", "MITTWOCH", "DONNERSTAG", "FREITAG", "SAMSTAG"
};
static const char *const s_french_days[] = {
  "DIMANCHE", "LUNDI", "MARDI", "MERCREDI", "JEUDI", "VENDREDI", "SAMEDI"
};
static const char *const s_spanish_days[] = {
  "DOMINGO", "LUNES", "MARTES", "MIÉRCOLES", "JUEVES", "VIERNES", "SÁBADO"
};
static const char *const s_italian_days[] = {
  "DOMENICA", "LUNEDÌ", "MARTEDÌ", "MERCOLEDÌ", "GIOVEDÌ", "VENERDÌ", "SABATO"
};
static const char *const s_portuguese_days[] = {
  "DOMINGO", "SEGUNDA", "TERÇA", "QUARTA", "QUINTA", "SEXTA", "SÁBADO"
};
static const char *const s_dutch_days[] = {
  "ZONDAG", "MAANDAG", "DINSDAG", "WOENSDAG", "DONDERDAG", "VRIJDAG", "ZATERDAG"
};
static const char *const s_polish_days[] = {
  "NIEDZIELA", "PONIEDZIAŁEK", "WTOREK", "ŚRODA", "CZWARTEK", "PIĄTEK", "SOBOTA"
};
static const char *const s_danish_days[] = {
  "SØNDAG", "MANDAG", "TIRSDAG", "ONSDAG", "TORSDAG", "FREDAG", "LØRDAG"
};
static const char *const s_swedish_days[] = {
  "SÖNDAG", "MÅNDAG", "TISDAG", "ONSDAG", "TORSDAG", "FREDAG", "LÖRDAG"
};
static const char *const s_norwegian_days[] = {
  "SØNDAG", "MANDAG", "TIRSDAG", "ONSDAG", "TORSDAG", "FREDAG", "LØRDAG"
};
static const char *const s_finnish_days[] = {
  "SUNNUNTAI", "MAANANTAI", "TIISTAI", "KESKIVIIKKO", "TORSTAI", "PERJANTAI", "LAUANTAI"
};
static const char *const s_hungarian_days[] = {
  "VASÁRNAP", "HÉTFŐ", "KEDD", "SZERDA", "CSÜTÖRTÖK", "PÉNTEK", "SZOMBAT"
};
static const char *const s_romanian_days[] = {
  "DUMINICĂ", "LUNI", "MARȚI", "MIERCURI", "JOI", "VINERI", "SÂMBĂTĂ"
};
static const char *const s_turkish_days[] = {
  "PAZAR", "PAZARTESİ", "SALI", "ÇARŞAMBA", "PERŞEMBE", "CUMA", "CUMARTESİ"
};

static bool prv_locale_is(const char *locale, const char *language) {
  return locale != NULL && locale[0] == language[0] &&
         locale[1] == language[1] &&
         (locale[2] == '\0' || locale[2] == '_' || locale[2] == '-');
}

static const char *const *prv_get_localized_days(void) {
  const char *locale = i18n_get_system_locale();

  if (prv_locale_is(locale, "cs")) return s_czech_days;
  if (prv_locale_is(locale, "sk")) return s_slovak_days;
  if (prv_locale_is(locale, "de")) return s_german_days;
  if (prv_locale_is(locale, "fr")) return s_french_days;
  if (prv_locale_is(locale, "es")) return s_spanish_days;
  if (prv_locale_is(locale, "it")) return s_italian_days;
  if (prv_locale_is(locale, "pt")) return s_portuguese_days;
  if (prv_locale_is(locale, "nl")) return s_dutch_days;
  if (prv_locale_is(locale, "pl")) return s_polish_days;
  if (prv_locale_is(locale, "da")) return s_danish_days;
  if (prv_locale_is(locale, "sv")) return s_swedish_days;
  if (prv_locale_is(locale, "no") || prv_locale_is(locale, "nb") ||
      prv_locale_is(locale, "nn")) return s_norwegian_days;
  if (prv_locale_is(locale, "fi")) return s_finnish_days;
  if (prv_locale_is(locale, "hu")) return s_hungarian_days;
  if (prv_locale_is(locale, "ro")) return s_romanian_days;
  if (prv_locale_is(locale, "tr")) return s_turkish_days;

  return s_english_days;
}

static void prv_set_text_pair_layout(TextLayer *shadow_layer,
                                     TextLayer *text_layer, GRect frame,
                                     GFont font, int shadow_offset) {
  text_layer_set_font(shadow_layer, font);
  text_layer_set_font(text_layer, font);

  GRect shadow_frame = frame;
  shadow_frame.origin.x += shadow_offset;
  shadow_frame.origin.y += shadow_offset;
  layer_set_frame(text_layer_get_layer(shadow_layer), shadow_frame);
  layer_set_frame(text_layer_get_layer(text_layer), frame);
}

static GFont prv_get_large_time_font(int available_width, int maximum_size) {
  // Use the largest prepared bitmap font that fits the current digits. This
  // keeps both a wide time such as 00:00 and a narrow one such as 11:11 close
  // to the PT2 display edges without ever cropping a glyph.
  const GRect measurement_box = GRect(0, 0, 320, 130);

  for (int i = LARGE_TIME_FONT_COUNT - 1; i >= 0; i--) {
    if (s_large_time_font_sizes[i] > maximum_size) { continue; }
    const GSize size = graphics_text_layout_get_content_size(
        s_time_buffer, s_large_time_fonts[i], measurement_box,
        GTextOverflowModeFill, GTextAlignmentCenter);
    if (size.w <= available_width) {
      return s_large_time_fonts[i];
    }
  }

  // Wide digits can exceed the safe width even at 80px. The prepared 72px
  // time font keeps the complete clock visible rather than ellipsizing it.
  return s_large_compact_time_font;
}

static int prv_time_font_size(GFont font) {
  for (unsigned i = 0; i < LARGE_TIME_FONT_COUNT; ++i) {
    if (font == s_large_time_fonts[i]) { return s_large_time_font_sizes[i]; }
  }
  if (font == s_large_compact_time_font) { return 72; }
  return font == s_compact_time_font ? 64 : 76;
}

static bool prv_date_fits(GFont font, int available_width) {
  const GSize size = graphics_text_layout_get_content_size(
      s_date_buffer, font, GRect(0, 0, 320, 60),
      GTextOverflowModeFill, GTextAlignmentCenter);
  return size.w <= available_width;
}

static GFont prv_get_date_font(GFont preferred, int available_width) {
  if (prv_date_fits(preferred, available_width)) {
    return preferred;
  }
  if (preferred == s_large_date_font &&
      prv_date_fits(s_date_font, available_width)) {
    return s_date_font;
  }
  if (preferred != s_compact_date_font &&
      prv_date_fits(s_compact_date_font, available_width)) {
    return s_compact_date_font;
  }
  if (prv_date_fits(s_small_date_font, available_width)) { return s_small_date_font; }
  return s_complication_fonts[COMPLICATION_FONT_COUNT - 1];
}

static bool prv_text_fits(const char *text, GFont font, int width, int height) {
  const GSize size = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, 2048, 150), GTextOverflowModeFill, GTextAlignmentCenter);
  return size.w <= width && size.h <= height;
}

static GRect prv_layout_rect(WatchLayoutRect rect) {
  return GRect(rect.x, rect.y, rect.width, rect.height);
}

static WatchLayoutGeometry prv_current_geometry(void) {
  return watch_layout_geometry(WATCH_LAYOUT_CLASSIC,
      layer_get_unobstructed_bounds(s_window_layer).size.h, s_large_text_enabled,
      context_mode(&s_context, time(NULL)) != CONTEXT_NONE);
}

static int prv_storage_size(void *context, uint32_t key) {
  (void)context;
  return persist_get_size(key);
}
static int prv_storage_read(void *context, uint32_t key, void *buffer, size_t length) {
  (void)context;
  return persist_read_data(key, buffer, length);
}
static int prv_storage_write(void *context, uint32_t key, const void *buffer, size_t length) {
  (void)context;
  return persist_write_data(key, buffer, length);
}
static void prv_storage_remove(void *context, uint32_t key) {
  (void)context;
  persist_delete(key);
}

static void prv_remove_legacy_data(void) {
  // Keys2..7 remain theme/clock settings. Ignore the old layout choice and
  // never reinterpret obsolete provider-specific values as generic slot data.
  static const uint8_t keys[] = {1, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) { persist_delete(keys[i]); }
}

static bool prv_slot_text(const ComplicationSlot *slot, char *text, size_t capacity) {
  text[0] = '\0';
  if (!complication_is_fresh(slot, time(NULL))) {
    snprintf(text, capacity, "--");
    return true;
  }
  if (slot->flags & COMPLICATION_FLAG_TIMESTAMP) {
    const time_t value = (time_t)slot->timestamp;
    struct tm *local = value >= 0 && (uint64_t)value == slot->timestamp ? localtime(&value) : NULL;
    if (local != NULL && strftime(text, capacity, s_use_12_hour ? "%I:%M" : "%H:%M", local)) {
      if (s_use_12_hour && text[0] == '0') { memmove(text, text + 1, strlen(text)); }
      return true;
    }
    snprintf(text, capacity, "--");
    return true;
  }
  if ((slot->flags & COMPLICATION_FLAG_TEXT) && slot->text_length) {
    snprintf(text, capacity, "%s", slot->text);
    return true;
  }
  return false;
}

static GFont prv_fit_complication(char *text, size_t capacity, int width, int height, int preferred) {
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) {
    if (s_complication_font_sizes[i] <= preferred &&
        prv_text_fits(text, s_complication_fonts[i], width, height)) { return s_complication_fonts[i]; }
  }
  const GFont smallest = s_complication_fonts[COMPLICATION_FONT_COUNT - 1];
  if (!prv_text_fits(text, smallest, width, height)) {
    char prefix[COMPLICATION_MAX_TEXT_BYTES + 1];
    snprintf(prefix, sizeof(prefix), "%s", text);
    do {
      watch_layout_pop_utf8(prefix);
      snprintf(text, capacity, "%s…", prefix);
    } while (prefix[0] && !prv_text_fits(text, smallest, width, height));
  }
  return smallest;
}

typedef struct {
  int width, height, x, y, source_x, source_y, source_width, source_height;
} BitmapLayout;

static BitmapLayout prv_bitmap_layout(const ComplicationSlot *slot, int size, bool cover) {
  const int longest = slot->width > slot->height ? slot->width : slot->height;
  if (longest <= 0 || size <= 0) { return (BitmapLayout){0}; }
  const ReadingCoverCrop crop = reading_cover_crop(slot->width, slot->height);
  const int width = cover ? size : (slot->width * size + longest / 2) / longest;
  const int height = cover ? size : (slot->height * size + longest / 2) / longest;
  return (BitmapLayout){width, height, (size - width) / 2, (size - height) / 2,
      cover ? crop.x : 0, cover ? crop.y : 0,
      cover ? crop.size : slot->width, cover ? crop.size : slot->height};
}

static uint8_t prv_bitmap_pixel(const ComplicationSlot *slot, const BitmapLayout *layout,
                                int column, int row) {
  const int sx = layout->source_x + column * layout->source_width / layout->width;
  const int sy = layout->source_y + row * layout->source_height / layout->height;
  const uint8_t pair = slot->bitmap[sy * slot->stride + sx / 2];
  return slot->palette[sx % 2 ? pair & 15 : pair >> 4];
}

// Include contain-fit letterboxing and source transparency when aligning an
// icon. The same sampling path is used for drawing and visible-bound lookup.
static int prv_bitmap_visible_bottom(const ComplicationSlot *slot, int size, bool cover) {
  const BitmapLayout layout = prv_bitmap_layout(slot, size, cover);
  for (int row = layout.height - 1; row >= 0; --row) {
    for (int column = 0; column < layout.width; ++column) {
      if (prv_bitmap_pixel(slot, &layout, column, row) & 0xc0) {
        return layout.y + row + 1;
      }
    }
  }
  return 0;
}

static void prv_draw_bitmap(GContext *ctx, const ComplicationSlot *slot,
                            int x, int y, int size, bool shadow, bool cover) {
  const BitmapLayout layout = prv_bitmap_layout(slot, size, cover);
  x += layout.x;
  y += layout.y;
  for (int row = 0; row < layout.height; ++row) {
    for (int column = 0; column < layout.width; ++column) {
      const uint8_t source = prv_bitmap_pixel(slot, &layout, column, row);
      if (!(source & 0xc0)) { continue; }
      GColor8 color = {.argb = source};
      if (shadow) { color = GColorBlack; }
      else if (!(slot->flags & COMPLICATION_FLAG_IMAGE)) {
        color.argb = (s_text_color.argb & 0x3f) | (source & 0xc0);
      }
      graphics_context_set_fill_color(ctx, color);
      graphics_fill_rect(ctx, GRect(x + column, y + row, 1, 1), 0, GCornerNone);
    }
  }
}

static void prv_draw_droplet(GContext *ctx, int x, int y, GColor color, bool small) {
  graphics_context_set_fill_color(ctx, color);
  graphics_context_set_antialiased(ctx, true);
  graphics_fill_circle(ctx, GPoint(x + (small ? 7 : 8), y + (small ? 11 : 13)), small ? 6 : 8);
  GPath *path = small ? s_small_drop_path : s_drop_path;
  if (path != NULL) {
    gpath_move_to(path, GPoint(x + (small ? 7 : 8), y + (small ? 8 : 10)));
    gpath_draw_filled(ctx, path);
  }
}

static void prv_draw_number_text(GContext *ctx, const char *text, GFont font,
                                 GRect rect, GColor color) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, rect, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static GFont prv_complication_font_at_most(int size) {
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) {
    if (s_complication_font_sizes[i] <= size) { return s_complication_fonts[i]; }
  }
  return s_complication_fonts[COMPLICATION_FONT_COUNT - 1];
}

static bool prv_draw_number_reading(GContext *ctx, const ComplicationSlot *slot,
                                    const ReadingParts *parts, GRect bounds,
                                    int preferred, bool bitmap) {
  const bool droplet = slot->display_kind == COMPLICATION_DISPLAY_PRECIPITATION &&
      reading_is_probability(parts);
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) {
    if (s_complication_font_sizes[i] > preferred) { continue; }
    const bool small_droplet = droplet && s_complication_font_sizes[i] < 32;
    const int icon_width = droplet ? (small_droplet ? 14 : 18) : bitmap ? 16 : 0;
    const int icon_gap = icon_width ? 5 : 0;
    const int droplet_height = small_droplet ? 18 : 22;
    const GFont font = s_complication_fonts[i];
    const int unit_size = s_complication_font_sizes[i] >= 44 ? 24
        : s_complication_font_sizes[i] >= 32 ? 20 : 18;
    const GFont unit_font = prv_complication_font_at_most(unit_size);
    const GSize unit = parts->unit[0] ? graphics_text_layout_get_content_size(parts->unit,
        unit_font, GRect(0, 0, 2048, 100), GTextOverflowModeFill, GTextAlignmentLeft) : GSize(0, 0);
    const GSize number = graphics_text_layout_get_content_size(parts->number, font,
        GRect(0, 0, 2048, 100), GTextOverflowModeFill, GTextAlignmentLeft);
    const int unit_gap = unit.w ? 2 : 0;
    const int width = icon_width + icon_gap + number.w + unit_gap + unit.w + 2;
    if (width > bounds.size.w || number.h > bounds.size.h || unit.h > bounds.size.h) { continue; }
    const int x = (bounds.size.w - width) / 2;
    const int y = (bounds.size.h - number.h) / 2;
    for (unsigned shadow = s_text_shadow_enabled ? 1 : 0; ; --shadow) {
      const int offset = shadow ? 1 : 0;
      const GColor color = shadow ? GColorBlack : s_text_color;
      if (droplet) {
        prv_draw_droplet(ctx, x + offset, (bounds.size.h - droplet_height) / 2 + offset,
                         color, small_droplet);
      } else if (bitmap && (!shadow || !(slot->flags & COMPLICATION_FLAG_IMAGE))) {
        prv_draw_bitmap(ctx, slot, x + offset, (bounds.size.h - 16) / 2 + offset, 16, shadow, false);
      }
      const int text_x = x + icon_width + icon_gap + offset;
      prv_draw_number_text(ctx, parts->number, font, GRect(text_x, y + offset,
          number.w + 2, bounds.size.h), color);
      if (unit.w) {
        const int unit_y = y + number.h - unit.h - (number.h > unit.h ? 2 : 0) + offset;
        prv_draw_number_text(ctx, parts->unit, unit_font,
            GRect(text_x + number.w + unit_gap, unit_y, unit.w + 2, bounds.size.h), color);
      }
      if (!shadow) { break; }
    }
    return true;
  }
  return false;
}

static void prv_draw_complication(Layer *layer, GContext *ctx) {
  const unsigned index = *(uint8_t *)layer_get_data(layer);
  if (!s_has_complications || index >= COMPLICATION_SLOT_COUNT) { return; }
  const ComplicationSlot *slot = &s_complications.slots[index];
  if (!complication_is_selected(slot)) { return; }
  const GRect bounds = layer_get_bounds(layer);
  const bool fresh = complication_is_fresh(slot, time(NULL));
  bool bitmap = fresh && (slot->flags & COMPLICATION_FLAG_BITMAP);
  char text[COMPLICATION_MAX_TEXT_BYTES + 5];
  bool has_text = prv_slot_text(slot, text, sizeof(text));
  if (!has_text && !bitmap) { snprintf(text, sizeof(text), "--"); has_text = true; }
  const int bitmap_size = bitmap ? (has_text ? 16 : 24) : 0;
  const int gap = bitmap && has_text ? 4 : 0;
  const WatchLayoutGeometry geometry = prv_current_geometry();
  const int preferred = geometry.slots[index].font_size;
  ReadingParts parts;
  if (fresh && has_text && !(slot->flags & COMPLICATION_FLAG_TIMESTAMP) &&
      reading_parts(text, &parts) &&
      prv_draw_number_reading(ctx, slot, &parts, bounds, preferred, bitmap)) { return; }
  GFont font = s_complication_fonts[COMPLICATION_FONT_COUNT - 1];
  GSize text_size = GSize(0, 0);
  if (has_text) {
    font = prv_fit_complication(text, sizeof(text), bounds.size.w - bitmap_size - gap - 2,
                                bounds.size.h, preferred);
    text_size = graphics_text_layout_get_content_size(text, font, GRect(0, 0, 2048, 100),
        GTextOverflowModeFill, GTextAlignmentCenter);
  }
  const int text_width = has_text ? text_size.w + 2 : 0;
  const int group_width = bitmap_size + gap + text_width;
  const int x = (bounds.size.w - group_width) / 2;
  if (bitmap) {
    const int y = (bounds.size.h - bitmap_size) / 2;
    if (s_text_shadow_enabled && !(slot->flags & COMPLICATION_FLAG_IMAGE)) {
      prv_draw_bitmap(ctx, slot, x + 1, y + 1, bitmap_size, true, false);
    }
    prv_draw_bitmap(ctx, slot, x, y, bitmap_size, false, false);
  }
  if (has_text) {
    const GRect frame = GRect(x + bitmap_size + gap, (bounds.size.h - text_size.h) / 2,
                              text_width, bounds.size.h);
    if (s_text_shadow_enabled) {
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, text, font, GRect(frame.origin.x + 1, frame.origin.y + 1,
          frame.size.w, frame.size.h), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
    graphics_context_set_text_color(ctx, s_text_color);
    graphics_draw_text(ctx, text, font, frame, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

static int prv_status_left_reserve(void) {
  if (!s_show_status) { return 0; }
  unsigned left = s_battery.charge_percent <= 10 && !s_battery.is_charging ? 1 : 0;
  if (context_phone_battery(&s_context.slots[4], time(NULL), NULL)) { ++left; }
  return (int)left * 12;
}

static int prv_status_right_reserve(void) {
  return s_show_status && !s_phone_connected ? 12 : 0;
}

typedef struct {
  char text[COMPLICATION_MAX_TEXT_BYTES + 5];
  GFont font;
  GRect frame;
  int ink_bottom;
} PreparedContextText;

static PreparedContextText prv_prepare_context_text(const ComplicationSlot *slot,
                                                     GRect rect, int preferred) {
  PreparedContextText result = {.frame = rect};
  if (!context_slot_fresh(slot, time(NULL)) || !(slot->flags & COMPLICATION_FLAG_TEXT)) {
    return result;
  }
  snprintf(result.text, sizeof(result.text), "%s", slot->text);
  result.font = prv_fit_complication(result.text, sizeof(result.text),
      rect.size.w - 2, rect.size.h, preferred);
  const GSize size = graphics_text_layout_get_content_size(result.text, result.font,
      GRect(0, 0, 2048, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  result.frame.origin.y += (rect.size.h - size.h) / 2;
  int font_size = 18;
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) {
    if (result.font == s_complication_fonts[i]) { font_size = s_complication_font_sizes[i]; break; }
  }
  const int bottom = context_text_ink_bottom(result.text, font_size);
  result.ink_bottom = bottom ? result.frame.origin.y + bottom : 0;
  return result;
}

static void prv_draw_context_text(GContext *ctx, const PreparedContextText *text) {
  if (text->font == NULL || !text->text[0]) { return; }
  const GRect rect = text->frame;
  if (s_text_shadow_enabled) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, text->text, text->font, GRect(rect.origin.x + 1, rect.origin.y + 1,
        rect.size.w, rect.size.h), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }
  graphics_context_set_text_color(ctx, s_text_color);
  graphics_draw_text(ctx, text->text, text->font, rect,
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

typedef struct {
  GFont font;
  int nominal_height;
  int draw_height;
} PreparedContextTitle;

// A title owns the full width. Fit readable text into at most two lines;
// retain a descender reserve while using nominal height for the following row.
static PreparedContextTitle prv_prepare_context_title(const ComplicationSlot *slot,
                                                       int width, int height, int preferred) {
  PreparedContextTitle result = {0};
  if (!context_slot_fresh(slot, time(NULL)) || !(slot->flags & COMPLICATION_FLAG_TEXT)) {
    return result;
  }
  result.font = s_complication_fonts[4]; // 24px readability floor.
  int selected_size = 24;
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) {
    const int point = s_complication_font_sizes[i];
    if (point > preferred || point < 24) { continue; }
    const GSize size = graphics_text_layout_get_content_size(slot->text,
        s_complication_fonts[i], GRect(0, 0, width - 2, 512),
        GTextOverflowModeFill, GTextAlignmentLeft);
    if (size.h <= point * 2 && size.h + (point + 2) / 3 <= height) {
      result.font = s_complication_fonts[i]; selected_size = point; break;
    }
  }
  const GSize measured = graphics_text_layout_get_content_size(slot->text, result.font,
      GRect(0, 0, width - 2, 512), GTextOverflowModeFill, GTextAlignmentLeft);
  result.nominal_height = measured.h < selected_size * 2 ? measured.h : selected_size * 2;
  result.draw_height = result.nominal_height + (selected_size + 2) / 3;
  if (result.draw_height > height) { result.draw_height = height; }
  return result;
}

static void prv_draw_context_title(GContext *ctx, const ComplicationSlot *slot,
                                    GRect rect, const PreparedContextTitle *text) {
  if (text->font == NULL) { return; }
  rect.size.h = text->draw_height;
  if (s_text_shadow_enabled) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, slot->text, text->font, GRect(rect.origin.x + 1, rect.origin.y + 1,
        rect.size.w, rect.size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
  graphics_context_set_text_color(ctx, s_text_color);
  graphics_draw_text(ctx, slot->text, text->font, rect,
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void prv_draw_context(Layer *layer, GContext *ctx) {
  const ContextMode mode = context_mode(&s_context, time(NULL));
  if (mode == CONTEXT_NONE) { return; }
  const unsigned first = mode == CONTEXT_NOTIFICATION ? 2 : 0;
  const ComplicationSlot *title = &s_context.slots[first];
  const ComplicationSlot *subtitle = &s_context.slots[first + 1];
  const GRect bounds = layer_get_bounds(layer);
  const bool has_subtitle = context_slot_fresh(subtitle, time(NULL)) &&
      (subtitle->flags & COMPLICATION_FLAG_TEXT) && subtitle->text_length;
  const bool bitmap = (title->flags & COMPLICATION_FLAG_BITMAP) != 0;
  const int artwork = s_large_text_enabled ? 32 : 28;
  const int inset = bitmap ? artwork + 8 : 0;
  const int footer_height = 42;
  if (mode == CONTEXT_MUSIC) {
    // Let the song title span two full-width lines; album art shares the
    // separate artist row instead of stealing width from the title.
    const PreparedContextTitle headline = prv_prepare_context_title(title, bounds.size.w,
        has_subtitle ? bounds.size.h - footer_height - 2 : bounds.size.h,
        s_large_text_enabled ? 44 : 38);
    prv_draw_context_title(ctx, title, GRect(0, 0, bounds.size.w, headline.draw_height), &headline);
    if (has_subtitle) {
      const int footer_y = bounds.size.h - footer_height;
      PreparedContextText artist = prv_prepare_context_text(subtitle,
          GRect(inset, footer_y, bounds.size.w - inset, footer_height),
          s_large_text_enabled ? 28 : 24);
      if (artist.ink_bottom > bounds.size.h) {
        artist.frame.origin.y -= artist.ink_bottom - bounds.size.h;
        artist.ink_bottom = bounds.size.h;
      }
      const int bottom = artist.ink_bottom;
      if (bitmap) {
        const bool cover = (title->flags & COMPLICATION_FLAG_IMAGE) != 0;
        const int visible = prv_bitmap_visible_bottom(title, artwork, cover);
        if (visible) { prv_draw_bitmap(ctx, title, 0, bottom - visible, artwork, false, cover); }
      }
      prv_draw_context_text(ctx, &artist);
    }
  } else {
    // The application/title is a compact heading; the message gets the full
    // lower width, including room for two readable lines when preview is on.
    const int heading_height = has_subtitle ? 40 : bounds.size.h;
    PreparedContextText heading = prv_prepare_context_text(title,
        GRect(inset, 0, bounds.size.w - inset, heading_height),
        s_large_text_enabled ? 28 : 24);
    if (heading.ink_bottom > heading_height) {
      heading.frame.origin.y -= heading.ink_bottom - heading_height;
      heading.ink_bottom = heading_height;
    }
    if (bitmap) {
      const int bottom = heading.ink_bottom;
      const int visible = prv_bitmap_visible_bottom(title, artwork, false);
      if (visible) { prv_draw_bitmap(ctx, title, 0, bottom - visible, artwork, false, false); }
    }
    prv_draw_context_text(ctx, &heading);
    if (has_subtitle) {
      const PreparedContextTitle body = prv_prepare_context_title(subtitle,
          bounds.size.w, bounds.size.h - 44, s_large_text_enabled ? 32 : 28);
      prv_draw_context_title(ctx, subtitle, GRect(0, 44, bounds.size.w, body.draw_height), &body);
    }
  }
}

static void prv_apply_unobstructed_layout(void);

static void prv_draw_status(Layer *layer, GContext *ctx) {
  if (!s_show_status) { return; }
  const WatchLayoutGeometry geometry = prv_current_geometry();
  const int y = geometry.slots[0].rect.y + (geometry.slots[0].rect.height - 10) / 2;
  int x = geometry.slots[0].rect.x;
  graphics_context_set_stroke_color(ctx, s_text_color);
  graphics_context_set_fill_color(ctx, s_text_color);
  graphics_context_set_stroke_width(ctx, 1);
  if (s_battery.charge_percent <= 10 && !s_battery.is_charging) {
    graphics_draw_rect(ctx, GRect(x, y + 2, 9, 6));
    graphics_fill_rect(ctx, GRect(x + 9, y + 4, 1, 2), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(x + 2, y + 4, 1, 2), 0, GCornerNone);
    x += 12;
  }
  if (context_phone_battery(&s_context.slots[4], time(NULL), NULL)) {
    graphics_draw_round_rect(ctx, GRect(x + 1, y, 6, 10), 1);
    graphics_fill_rect(ctx, GRect(x + 3, y + 7, 2, 1), 0, GCornerNone);
  }
  if (!s_phone_connected) {
    const int bx = geometry.slots[1].rect.x + geometry.slots[1].rect.width - 9;
    graphics_draw_line(ctx, GPoint(bx + 3, y), GPoint(bx + 3, y + 10));
    graphics_draw_line(ctx, GPoint(bx + 3, y), GPoint(bx + 7, y + 3));
    graphics_draw_line(ctx, GPoint(bx + 7, y + 3), GPoint(bx, y + 8));
    graphics_draw_line(ctx, GPoint(bx + 3, y + 10), GPoint(bx + 7, y + 7));
    graphics_draw_line(ctx, GPoint(bx + 7, y + 7), GPoint(bx, y + 2));
    graphics_draw_line(ctx, GPoint(bx - 1, y + 10), GPoint(bx + 8, y));
  }
}

static void prv_apply_unobstructed_layout(void);
static void prv_context_expired(void *context);

static void prv_schedule_context_expiry(void) {
  if (s_context_timer != NULL) { app_timer_cancel(s_context_timer); s_context_timer = NULL; }
  const time_t now = time(NULL);
  const uint32_t next = wfc_prune_context(&s_core, now);
  if (next > (uint32_t)now) {
    s_context_timer = app_timer_register((next - (uint32_t)now) * 1000,
        prv_context_expired, NULL);
  }
}

static void prv_context_expired(void *context) {
  s_context_timer = NULL;
  prv_schedule_context_expiry();
  prv_apply_unobstructed_layout();
}

static void prv_draw_background(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, s_background_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void prv_apply_unobstructed_layout(void) {
  if (s_window_layer == NULL || s_date_layer == NULL || s_time_layer == NULL) { return; }
  const GRect root_bounds = layer_get_bounds(s_window_layer);
  const int visible_height = layer_get_unobstructed_bounds(s_window_layer).size.h;
  const bool has_context = context_mode(&s_context, time(NULL)) != CONTEXT_NONE;
  if (!s_compact_layout && visible_height <= COMPACT_FONT_ENTER_HEIGHT) { s_compact_layout = true; }
  else if (s_compact_layout && visible_height >= COMPACT_FONT_EXIT_HEIGHT) { s_compact_layout = false; }
  const bool compact_header = has_context || visible_height < FULL_LAYOUT_HEIGHT;
  const int date_x = 6;
  const int date_width = root_bounds.size.w - date_x * 2;
  const int time_x = 4;
  const int time_width = root_bounds.size.w - time_x * 2;
  const GFont preferred_date = compact_header
      ? (s_large_text_enabled ? s_compact_date_font : s_small_date_font)
      : (s_large_text_enabled ? s_large_date_font : s_date_font);
  const GFont date_font = prv_get_date_font(preferred_date,
      date_width - (s_text_shadow_enabled ? 2 : 0));
  const bool full_context = has_context && visible_height >= FULL_LAYOUT_HEIGHT;
  const GFont time_font = full_context
      ? (s_large_text_enabled ? s_complication_fonts[0] : s_complication_fonts[1])
      : s_compact_layout
          ? (s_large_text_enabled ? s_large_compact_time_font : s_compact_time_font)
          : prv_get_large_time_font(time_width - (s_text_shadow_enabled ? 6 : 4),
              s_large_text_enabled ? 108 : 92);
  const int time_size = full_context ? (s_large_text_enabled ? 44 : 38)
      : prv_time_font_size(time_font);
  const int date_y = compact_header ? 4
      : (s_large_text_enabled ? 2 : 4);
  prv_set_text_pair_layout(s_date_shadow_layer, s_date_layer,
      GRect(date_x, date_y, date_width, compact_header ? 32 : 46), date_font, 1);
  prv_set_text_pair_layout(s_time_shadow_layer, s_time_layer,
      GRect(time_x, watch_layout_clock_y(time_size, visible_height, full_context),
          time_width, full_context ? 60 : 122), time_font, 1);
  text_layer_set_text(s_date_layer, s_date_buffer);
  text_layer_set_text(s_date_shadow_layer, s_date_buffer);
  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_time_shadow_layer, s_time_buffer);
  const WatchLayoutGeometry geometry = prv_current_geometry();
  const bool contextual = geometry.context.width > 0 && context_mode(&s_context, time(NULL)) != CONTEXT_NONE;
  for (unsigned i = 0; i < COMPLICATION_SLOT_COUNT; ++i) {
    const bool visible = s_has_complications && complication_is_selected(&s_complications.slots[i]) &&
        geometry.slots[i].rect.width > 0;
    WatchLayoutRect rect = geometry.slots[i].rect;
    if (rect.width > 0 && i == 0 && s_show_status) {
      const int reserve = prv_status_left_reserve();
      rect.x += reserve;
      rect.width -= reserve;
      if (geometry.slots[1].rect.width == 0) { rect.width -= prv_status_right_reserve(); }
    } else if (rect.width > 0 && i == 1 && s_show_status) {
      rect.width -= prv_status_right_reserve();
    }
    layer_set_frame(s_complication_layers[i], prv_layout_rect(rect));
    layer_set_hidden(s_complication_layers[i], !visible);
    layer_mark_dirty(s_complication_layers[i]);
  }
  if (s_context_layer != NULL) {
    layer_set_frame(s_context_layer, prv_layout_rect(geometry.context));
    layer_set_hidden(s_context_layer, !contextual);
    layer_mark_dirty(s_context_layer);
  }
  if (s_status_layer != NULL) { layer_mark_dirty(s_status_layer); }
  layer_mark_dirty(s_background_layer);
}

static void prv_update_clock(void) {
  const time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  const char *const *localized_days = prv_get_localized_days();

  strftime(s_time_buffer, sizeof(s_time_buffer),
           s_use_12_hour ? "%I:%M" : "%H:%M", current_time);
  if (s_use_12_hour && s_time_buffer[0] == '0') {
    memmove(s_time_buffer, s_time_buffer + 1, strlen(s_time_buffer));
  }
  snprintf(s_date_buffer, sizeof(s_date_buffer), "%s · %d.%d.",
           localized_days[current_time->tm_wday], current_time->tm_mday,
           current_time->tm_mon + 1);

  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
  text_layer_set_text(s_time_shadow_layer, s_time_buffer);
  text_layer_set_text(s_date_shadow_layer, s_date_buffer);

  // Re-evaluate every minute: extra-large time follows the current digits and
  // long localized weekday names can select the compact date font.
  prv_apply_unobstructed_layout();
}


static void prv_request_complications(void) {
  DictionaryIterator *iterator;
  const AppMessageResult result = app_message_outbox_begin(&iterator);
  if (result != APP_MSG_OK || iterator == NULL) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Could not begin complication request: %d", result);
    return;
  }
  dict_write_uint8(iterator, MESSAGE_KEY_REQUEST_COMPLICATIONS, 1);
  dict_write_uint32(iterator, MESSAGE_KEY_REQUEST_CONTEXT, wfc_request_marker(&s_core));
  const AppMessageResult sent = app_message_outbox_send();
  if (sent != APP_MSG_OK) {
    wfc_request_failed(&s_core);
    APP_LOG(APP_LOG_LEVEL_WARNING, "Complication request failed: %d", sent);
  } else { wfc_request_accepted(&s_core, time(NULL)); }
}

static void prv_startup_request(void *context) {
  s_startup_timer = NULL;
  prv_request_complications();
}

static void prv_phone_connection_changed(bool connected) {
  if (!wfc_connection_changed(&s_core, connected)) { return; }
  prv_schedule_context_expiry();
  prv_apply_unobstructed_layout();
  if (connected) {
    persist_write_int(PERSIST_KEY_CONTEXT_SESSION, (int32_t)s_core.session_token);
    if (s_startup_timer != NULL) { app_timer_cancel(s_startup_timer); }
    s_startup_timer = app_timer_register(1000, prv_startup_request, NULL);
  }
}

static void prv_battery_changed(BatteryChargeState charge) {
  s_battery = charge;
  prv_apply_unobstructed_layout();
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  const bool request_due = wfc_tick(&s_core, time(NULL));
  prv_update_clock();
  // One request on the freshness transition; denied/unavailable slots do not
  // cause a polling loop. Provider refresh policies belong to the phone host.
  if (request_due) {
    prv_request_complications();
  }
  prv_schedule_context_expiry();
}

static void prv_unobstructed_area_will_change(GRect final_area, void *context) {
  prv_apply_unobstructed_layout();
}
static void prv_unobstructed_area_change(AnimationProgress progress, void *context) {
  prv_apply_unobstructed_layout();
}
static void prv_unobstructed_area_did_change(void *context) { prv_apply_unobstructed_layout(); }

static void prv_apply_colors(void) {
  if (s_window != NULL) { window_set_background_color(s_window, s_background_color); }
  if (s_date_layer != NULL) { text_layer_set_text_color(s_date_layer, s_text_color); }
  if (s_time_layer != NULL) { text_layer_set_text_color(s_time_layer, s_text_color); }
  prv_apply_unobstructed_layout();
}

static void prv_apply_effects(void) {
  if (s_date_shadow_layer != NULL) {
    layer_set_hidden(text_layer_get_layer(s_date_shadow_layer), !s_text_shadow_enabled);
  }
  if (s_time_shadow_layer != NULL) {
    layer_set_hidden(text_layer_get_layer(s_time_shadow_layer), !s_text_shadow_enabled);
  }
  prv_apply_unobstructed_layout();
}

static uint32_t prv_context_request_value(const Tuple *marker) {
  if (marker == NULL || (marker->type != TUPLE_INT && marker->type != TUPLE_UINT)) { return 0; }
  uint32_t value = 0;
  if (marker->length == 1) {
    if (marker->type == TUPLE_INT && marker->value->int8 < 0) { return 0; }
    value = marker->value->uint8;
  } else if (marker->length == 2) {
    if (marker->type == TUPLE_INT && marker->value->int16 < 0) { return 0; }
    value = marker->value->uint16;
  } else if (marker->length == 4) {
    if (marker->type == TUPLE_INT && marker->value->int32 < 0) { return 0; }
    value = marker->value->uint32;
  }
  return value <= CONTEXT_MAX_SESSION_TOKEN ? value : 0;
}

static void prv_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *context_data = dict_find(iterator, MESSAGE_KEY_CONTEXT_DATA);
  const uint32_t marker_value = prv_context_request_value(dict_find(iterator, MESSAGE_KEY_REQUEST_CONTEXT));
  if (context_data == NULL && marker_value == 1) {
    // PKJS may become ready after the first native request was acknowledged.
    // Answer its query with the current epoch; querying never advances it.
    wfc_context_query(&s_core);
    if (s_startup_timer != NULL) { app_timer_cancel(s_startup_timer); }
    s_startup_timer = app_timer_register(100, prv_startup_request, NULL);
  }
  if (context_data != NULL) {
    const bool bytes = context_data->type == TUPLE_BYTE_ARRAY;
    wfc_receive_context(&s_core, bytes ? context_data->value->data : NULL,
        bytes ? context_data->length : 0, marker_value);
    prv_schedule_context_expiry();
  }
  Tuple *data = dict_find(iterator, MESSAGE_KEY_COMPLICATION_DATA);
  if (data != NULL) {
    const bool bytes = data->type == TUPLE_BYTE_ARRAY;
    if (wfc_receive_complications(&s_core, bytes ? data->value->data : NULL,
        bytes ? data->length : 0, time(NULL))) {
      prv_remove_legacy_data();
    }
  }
  Tuple *background = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  Tuple *text = dict_find(iterator, MESSAGE_KEY_TEXT_COLOR);
  Tuple *shadow = dict_find(iterator, MESSAGE_KEY_TEXT_SHADOW);
  Tuple *large = dict_find(iterator, MESSAGE_KEY_LARGE_TEXT);
  Tuple *twelve = dict_find(iterator, MESSAGE_KEY_USE_12_HOUR);
  Tuple *show_status = dict_find(iterator, MESSAGE_KEY_SHOW_STATUS);
  if (background != NULL) {
    s_background_color = GColorFromHEX(background->value->int32);
    persist_write_int(PERSIST_KEY_BACKGROUND_COLOR, background->value->int32);
  }
  if (text != NULL) {
    s_text_color = GColorFromHEX(text->value->int32);
    persist_write_int(PERSIST_KEY_TEXT_COLOR, text->value->int32);
  }
  if (shadow != NULL) {
    s_text_shadow_enabled = shadow->value->int32 != 0;
    persist_write_bool(PERSIST_KEY_TEXT_SHADOW, s_text_shadow_enabled);
  }
  if (large != NULL) {
    s_large_text_enabled = large->value->int32 != 0;
    persist_write_bool(PERSIST_KEY_LARGE_TEXT, s_large_text_enabled);
  }
  if (twelve != NULL) {
    s_use_12_hour = twelve->value->int32 != 0;
    persist_write_bool(PERSIST_KEY_USE_12_HOUR, s_use_12_hour);
  }
  if (show_status != NULL && (show_status->type == TUPLE_INT || show_status->type == TUPLE_UINT)) {
    s_show_status = show_status->value->int32 != 0;
    persist_write_bool(PERSIST_KEY_SHOW_STATUS, s_show_status);
  }
  prv_update_clock();
  prv_apply_colors();
  prv_apply_effects();
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Complication message dropped: %d", reason);
}
static void prv_outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  wfc_request_failed(&s_core);
  APP_LOG(APP_LOG_LEVEL_WARNING, "Complication request failed: %d", reason);
}
static void prv_outbox_sent(DictionaryIterator *iterator, void *context) {
  wfc_request_acknowledged(&s_core);
}

static void prv_configure_text_layer(TextLayer *layer, GFont font) {
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, s_text_color);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(layer, GTextOverflowModeFill);
  text_layer_set_font(layer, font);
}

static void prv_configure_shadow_layer(TextLayer *layer, GFont font) {
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorBlack);
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(layer, GTextOverflowModeFill);
  text_layer_set_font(layer, font);
}


static void prv_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(s_window_layer);
  s_background_layer = layer_create(bounds);
  layer_set_update_proc(s_background_layer, prv_draw_background);
  layer_add_child(s_window_layer, s_background_layer);
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_24));
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_76));
  s_compact_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_22));
  s_small_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_20));
  s_compact_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_64));
  s_large_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_28));
  s_large_time_fonts[0] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_80));
  s_large_time_fonts[1] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_84));
  s_large_time_fonts[2] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_88));
  s_large_time_fonts[3] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_92));
  s_large_time_fonts[4] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_96));
  s_large_time_fonts[5] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_100));
  s_large_time_fonts[6] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_104));
  s_large_time_fonts[7] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_108));
  s_large_compact_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_72));
  s_complication_fonts[0] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_44));
  s_complication_fonts[1] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_38));
  s_complication_fonts[2] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_32));
  s_complication_fonts[3] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_28));
  s_complication_fonts[4] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_24));
  s_complication_fonts[5] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_20));
  s_complication_fonts[6] = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FIRA_SANS_CONDENSED_BOLD_COMPLICATION_18));
  s_drop_path = gpath_create(&s_drop_info);
  s_small_drop_path = gpath_create(&s_small_drop_info);
  s_date_shadow_layer = text_layer_create(GRect(9, 29, bounds.size.w - 16, 38));
  prv_configure_shadow_layer(s_date_shadow_layer, s_date_font);
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_shadow_layer));
  s_time_shadow_layer = text_layer_create(GRect(2, 54, bounds.size.w, 102));
  prv_configure_shadow_layer(s_time_shadow_layer, s_time_font);
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_shadow_layer));
  s_date_layer = text_layer_create(GRect(8, 28, bounds.size.w - 16, 38));
  prv_configure_text_layer(s_date_layer, s_date_font);
  layer_add_child(s_window_layer, text_layer_get_layer(s_date_layer));
  s_time_layer = text_layer_create(GRect(0, 52, bounds.size.w, 102));
  prv_configure_text_layer(s_time_layer, s_time_font);
  layer_add_child(s_window_layer, text_layer_get_layer(s_time_layer));
  for (unsigned i = 0; i < COMPLICATION_SLOT_COUNT; ++i) {
    s_complication_layers[i] = layer_create_with_data(GRect(0, 0, 0, 0), sizeof(uint8_t));
    *(uint8_t *)layer_get_data(s_complication_layers[i]) = i;
    layer_set_update_proc(s_complication_layers[i], prv_draw_complication);
    layer_add_child(s_window_layer, s_complication_layers[i]);
  }
  s_context_layer = layer_create(GRect(18, 164, 164, 43));
  layer_set_update_proc(s_context_layer, prv_draw_context);
  layer_add_child(s_window_layer, s_context_layer);
  s_status_layer = layer_create(bounds);
  layer_set_update_proc(s_status_layer, prv_draw_status);
  layer_add_child(s_window_layer, s_status_layer);
  prv_update_clock();
  prv_apply_effects();
  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_area_will_change,
    .change = prv_unobstructed_area_change,
    .did_change = prv_unobstructed_area_did_change,
  };
  unobstructed_area_service_subscribe(handlers, NULL);
}

static void prv_window_unload(Window *window) {
  unobstructed_area_service_unsubscribe();
  layer_destroy(s_status_layer);
  layer_destroy(s_context_layer);
  for (unsigned i = 0; i < COMPLICATION_SLOT_COUNT; ++i) { layer_destroy(s_complication_layers[i]); }
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_shadow_layer);
  text_layer_destroy(s_time_shadow_layer);
  fonts_unload_custom_font(s_date_font);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_compact_date_font);
  fonts_unload_custom_font(s_small_date_font);
  fonts_unload_custom_font(s_compact_time_font);
  fonts_unload_custom_font(s_large_date_font);
  fonts_unload_custom_font(s_large_compact_time_font);
  if (s_drop_path != NULL) { gpath_destroy(s_drop_path); s_drop_path = NULL; }
  if (s_small_drop_path != NULL) { gpath_destroy(s_small_drop_path); s_small_drop_path = NULL; }
  for (unsigned i = 0; i < LARGE_TIME_FONT_COUNT; ++i) { fonts_unload_custom_font(s_large_time_fonts[i]); }
  for (unsigned i = 0; i < COMPLICATION_FONT_COUNT; ++i) { fonts_unload_custom_font(s_complication_fonts[i]); }
  layer_destroy(s_background_layer);
  s_window_layer = NULL;
}

static void prv_init(void) {
  s_background_color = GColorBlack;
  s_text_color = GColorWhite;
  s_text_shadow_enabled = false;
  s_large_text_enabled = false;
  s_use_12_hour = false;
  s_show_status = true;
  const WfcConfig core_config = {
    .storage = {.size = prv_storage_size, .read = prv_storage_read,
      .write = prv_storage_write, .remove = prv_storage_remove},
    .cache_first_key = PERSIST_KEY_COMPLICATION_FIRST
  };
  wfc_init(&s_core, &core_config, connection_service_peek_pebble_app_connection(),
      persist_exists(PERSIST_KEY_CONTEXT_SESSION)
          ? (uint32_t)persist_read_int(PERSIST_KEY_CONTEXT_SESSION) : 0);
  persist_write_int(PERSIST_KEY_CONTEXT_SESSION, (int32_t)s_core.session_token);
  s_battery = battery_state_service_peek();
  if (persist_exists(PERSIST_KEY_BACKGROUND_COLOR)) {
    s_background_color = GColorFromHEX(persist_read_int(PERSIST_KEY_BACKGROUND_COLOR));
  }
  if (persist_exists(PERSIST_KEY_TEXT_COLOR)) { s_text_color = GColorFromHEX(persist_read_int(PERSIST_KEY_TEXT_COLOR)); }
  if (persist_exists(PERSIST_KEY_TEXT_SHADOW)) { s_text_shadow_enabled = persist_read_bool(PERSIST_KEY_TEXT_SHADOW); }
  if (persist_exists(PERSIST_KEY_LARGE_TEXT)) { s_large_text_enabled = persist_read_bool(PERSIST_KEY_LARGE_TEXT); }
  if (persist_exists(PERSIST_KEY_USE_12_HOUR)) { s_use_12_hour = persist_read_bool(PERSIST_KEY_USE_12_HOUR); }
  if (persist_exists(PERSIST_KEY_SHOW_STATUS)) { s_show_status = persist_read_bool(PERSIST_KEY_SHOW_STATUS); }
  persist_delete(PERSIST_KEY_LAYOUT_STYLE);
  wfc_load_cache(&s_core, time(NULL));
  s_window = window_create();
  window_set_background_color(s_window, s_background_color);
  window_set_window_handlers(s_window, (WindowHandlers){.load = prv_window_load, .unload = prv_window_unload});
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(2048, 32);
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  window_stack_push(s_window, false);
  connection_service_subscribe((ConnectionHandlers){.pebble_app_connection_handler = prv_phone_connection_changed});
  battery_state_service_subscribe(prv_battery_changed);
  s_startup_timer = app_timer_register(1000, prv_startup_request, NULL);
}

static void prv_deinit(void) {
  if (s_startup_timer != NULL) { app_timer_cancel(s_startup_timer); }
  if (s_context_timer != NULL) { app_timer_cancel(s_context_timer); }
  memset(&s_context, 0, sizeof(s_context));
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
