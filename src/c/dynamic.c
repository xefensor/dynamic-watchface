#include <pebble.h>
#include "complication_data.h"
#include "context_data.h"
#include <pebble-watchface-core/pebble-watchface-core.h>
#include "watch_layout.h"
#include "reading_presentation.h"

#define PERSIST_KEY_BACKGROUND_COLOR 2
#define PERSIST_KEY_TEXT_COLOR 3
#define PERSIST_KEY_TEXT_SHADOW 5
#define PERSIST_KEY_LARGE_TEXT 6
#define PERSIST_KEY_USE_12_HOUR 7
#define PERSIST_KEY_LAYOUT_STYLE 16
#define PERSIST_KEY_SHOW_STATUS 19
#define PERSIST_KEY_COMPLICATION_FIRST 20
#define PERSIST_KEY_CONTEXT_SESSION 61
#define PERSIST_KEY_ACCENT_COLOR 70
#define FULL_LAYOUT_HEIGHT 228

static Window *s_window;
static Layer *s_window_layer;
static Layer *s_background_layer;
static Layer *s_complication_layers[COMPLICATION_SLOT_COUNT];
static Layer *s_context_layer;
static Layer *s_status_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;
static GColor s_background_color;
static GColor s_text_color;
static GColor s_accent_color;
static bool s_text_shadow_enabled; // Retained preference for upgrade compatibility.
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
static char s_compact_date_buffer[48];
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

static GRect prv_layout_rect(WatchLayoutRect rect) {
  return GRect(rect.x, rect.y, rect.width, rect.height);
}

static WatchLayoutGeometry prv_current_geometry(void) {
  return watch_layout_geometry(WATCH_LAYOUT_CLASSIC,
      layer_get_unobstructed_bounds(s_window_layer).size.h, s_large_text_enabled,
      context_mode(&s_context, time(NULL)) != CONTEXT_NONE);
}

static GFont prv_gothic(int point, bool bold) {
  if (point >= 28) { return fonts_get_system_font(bold ? FONT_KEY_GOTHIC_28_BOLD : FONT_KEY_GOTHIC_28); }
  if (point >= 24) { return fonts_get_system_font(bold ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_24); }
  if (point >= 18) { return fonts_get_system_font(bold ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_18); }
  return fonts_get_system_font(bold ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14);
}

static GFont prv_numeric(int point) {
  if (point >= 36) { return fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS); }
  if (point >= 32) { return fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS); }
  if (point >= 26) { return fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM); }
  return fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
}

static GSize prv_measure(const char *text, GFont font, int width) {
  return graphics_text_layout_get_content_size(text, font, GRect(0,0,width,512),
      GTextOverflowModeFill, GTextAlignmentLeft);
}

static bool prv_text_fits(const char *text, GFont font, int width, int height) {
  const GSize size = prv_measure(text, font, 2048);
  return size.w <= width && size.h <= height;
}

static GColor prv_contrast_ink(GColor color) {
  const unsigned r = (color.argb >> 4) & 3;
  const unsigned g = (color.argb >> 2) & 3;
  const unsigned b = color.argb & 3;
  return 299*r + 587*g + 114*b >= 1500 ? GColorBlack : GColorWhite;
}

static GColor prv_context_color(void) {
  return context_mode(&s_context, time(NULL)) == CONTEXT_NOTIFICATION
      ? GColorChromeYellow : s_accent_color;
}

static GColor prv_reading_ink(void) {
  const WatchLayoutGeometry geometry = prv_current_geometry();
  return geometry.context.width > 0 ? s_text_color : prv_contrast_ink(s_accent_color);
}

static GFont prv_fit_text(char *text, size_t capacity, int width, int height, int preferred) {
  static const uint8_t points[] = {28,24,18,14};
  for (unsigned i=0;i<sizeof(points);++i) {
    if (points[i] <= preferred && prv_text_fits(text,prv_gothic(points[i],true),width,height)) {
      return prv_gothic(points[i],true);
    }
  }
  GFont font = prv_gothic(14,true);
  char prefix[COMPLICATION_MAX_TEXT_BYTES + 1];
  watch_layout_copy_utf8(text,prefix,sizeof(prefix));
  while (!prv_text_fits(text,font,width,height) && watch_layout_pop_utf8(prefix)) {
    snprintf(text,capacity,"%s…",prefix);
  }
  return font;
}

static void prv_ellipsize_text(char *text, size_t capacity, GFont font,
                                int width, int height) {
  if (prv_text_fits(text,font,width,height)) { return; }
  char prefix[COMPLICATION_MAX_TEXT_BYTES+1];
  watch_layout_copy_utf8(text,prefix,sizeof(prefix));
  while (watch_layout_pop_utf8(prefix)) {
    snprintf(text,capacity,"%s…",prefix);
    if (prv_text_fits(text,font,width,height)) { return; }
  }
  snprintf(text,capacity,"…");
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
                            int x, int y, int size, GColor foreground, bool cover) {
  const BitmapLayout layout = prv_bitmap_layout(slot,size,cover);
  x += layout.x;
  y += layout.y;
  for (int row=0;row<layout.height;++row) {
    for (int column=0;column<layout.width;++column) {
      const uint8_t source = prv_bitmap_pixel(slot,&layout,column,row);
      if (!(source & 0xc0)) { continue; }
      GColor8 color = {.argb=source};
      if (!(slot->flags & COMPLICATION_FLAG_IMAGE)) {
        color.argb = (foreground.argb & 0x3f) | (source & 0xc0);
      }
      graphics_context_set_fill_color(ctx,color);
      graphics_fill_rect(ctx,GRect(x+column,y+row,1,1),0,GCornerNone);
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

static void prv_draw_text(GContext *ctx, const char *text, GFont font,
                           GRect frame, GColor ink, GTextAlignment alignment,
                           GTextOverflowMode overflow) {
  graphics_context_set_text_color(ctx,ink);
  graphics_draw_text(ctx,text,font,frame,overflow,alignment,NULL);
}

static bool prv_draw_number_reading(GContext *ctx, const ComplicationSlot *slot,
                                    const ReadingParts *parts, GRect bounds,
                                    int preferred, bool bitmap, GColor ink) {
  const bool droplet = slot->display_kind == COMPLICATION_DISPLAY_PRECIPITATION &&
      reading_is_probability(parts);
  // LECO is a numeric subset. Its ASCII minus gives a dependable display for
  // the typographic U+2212 minus supplied by some weather providers.
  char number_text[READING_PART_BYTES];
  reading_native_number(parts->number,number_text,sizeof(number_text));
  static const uint8_t points[] = {36,32,26,20};
  for (unsigned i=0;i<sizeof(points);++i) {
    if (points[i]>preferred) { continue; }
    const bool small = points[i]<32;
    const int icon = droplet ? (small ? 14 : 18) : bitmap ? 16 : 0;
    const int gap = icon ? 4 : 0;
    GFont font = prv_numeric(points[i]);
    GFont unit_font = prv_gothic(points[i]>=32 ? 18 : 14,true);
    const GSize number = prv_measure(number_text,font,2048);
    const GSize unit = parts->unit[0] ? prv_measure(parts->unit,unit_font,2048) : GSize(0,0);
    const int unit_gap = unit.w ? 2 : 0;
    const int width = icon+gap+number.w+unit_gap+unit.w;
    if (width>bounds.size.w || number.h>bounds.size.h || unit.h>bounds.size.h) { continue; }
    const int x = (bounds.size.w-width)/2;
    const int y = (bounds.size.h-number.h)/2;
    if (droplet) {
      prv_draw_droplet(ctx,x,(bounds.size.h-(small ? 18 : 22))/2,ink,small);
    } else if (bitmap) {
      prv_draw_bitmap(ctx,slot,x,(bounds.size.h-16)/2,16,ink,false);
    }
    const int text_x = x+icon+gap;
    prv_draw_text(ctx,number_text,font,GRect(text_x,y,number.w+2,bounds.size.h-y),
        ink,GTextAlignmentLeft,GTextOverflowModeFill);
    if (unit.w) {
      const int unit_y = y+number.h-unit.h-2;
      prv_draw_text(ctx,parts->unit,unit_font,GRect(text_x+number.w+unit_gap,unit_y,
          unit.w+2,bounds.size.h-unit_y),ink,GTextAlignmentLeft,GTextOverflowModeFill);
    }
    return true;
  }
  return false;
}

static void prv_draw_complication(Layer *layer, GContext *ctx) {
  const unsigned index = *(uint8_t *)layer_get_data(layer);
  if (!s_has_complications || index>=COMPLICATION_SLOT_COUNT) { return; }
  const ComplicationSlot *slot = &s_complications.slots[index];
  if (!complication_is_selected(slot)) { return; }
  const GRect bounds = layer_get_bounds(layer);
  const bool fresh = complication_is_fresh(slot,time(NULL));
  const bool bitmap = fresh && (slot->flags & COMPLICATION_FLAG_BITMAP);
  char text[COMPLICATION_MAX_TEXT_BYTES+5];
  bool has_text = prv_slot_text(slot,text,sizeof(text));
  if (!has_text && !bitmap) { snprintf(text,sizeof(text),"--"); has_text=true; }
  const int preferred = prv_current_geometry().slots[index].font_size;
  const GColor ink = prv_reading_ink();
  ReadingParts parts;
  if (fresh && has_text && !(slot->flags & COMPLICATION_FLAG_TIMESTAMP) &&
      reading_parts(text,&parts) &&
      prv_draw_number_reading(ctx,slot,&parts,bounds,preferred,bitmap,ink)) { return; }
  const int image = bitmap ? (has_text ? 16 : 28) : 0;
  const int gap = bitmap && has_text ? 4 : 0;
  GSize size = GSize(0,0);
  GFont font = prv_gothic(14,true);
  if (has_text) {
    font = prv_fit_text(text,sizeof(text),bounds.size.w-image-gap-2,bounds.size.h,preferred);
    size = prv_measure(text,font,2048);
  }
  const int x = (bounds.size.w-image-gap-size.w)/2;
  if (bitmap) { prv_draw_bitmap(ctx,slot,x,(bounds.size.h-image)/2,image,ink,false); }
  if (has_text) {
    prv_draw_text(ctx,text,font,GRect(x+image+gap,(bounds.size.h-size.h)/2,
        size.w+2,bounds.size.h),ink,GTextAlignmentLeft,GTextOverflowModeFill);
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

// The panel follows native Weather/Workout typography: a full-width headline
// and a separate source row, with no application-owned fonts or ink tables.
static void prv_draw_context(Layer *layer, GContext *ctx) {
  const ContextMode mode = context_mode(&s_context,time(NULL));
  if (mode==CONTEXT_NONE) { return; }
  const unsigned first = mode==CONTEXT_NOTIFICATION ? 2 : 0;
  const ComplicationSlot *title = &s_context.slots[first];
  const ComplicationSlot *subtitle = &s_context.slots[first+1];
  const GRect bounds = layer_get_bounds(layer);
  const GColor ink = prv_contrast_ink(prv_context_color());
  const bool has_subtitle = context_slot_fresh(subtitle,time(NULL)) &&
      (subtitle->flags & COMPLICATION_FLAG_TEXT) && subtitle->text_length;
  const bool bitmap = (title->flags & COMPLICATION_FLAG_BITMAP)!=0;
  const int inset = 8;
  const int width = bounds.size.w-inset*2;
  const int artwork = s_large_text_enabled ? 40 : 32;
  if (mode==CONTEXT_MUSIC) {
    const int footer_height = has_subtitle ? artwork+12 : 0;
    const int title_height = bounds.size.h-16-footer_height;
    prv_draw_text(ctx,title->text,prv_gothic(28,true),
        GRect(inset,3,width,title_height),ink,GTextAlignmentLeft,GTextOverflowModeTrailingEllipsis);
    if (has_subtitle) {
      const int row_y = bounds.size.h-8-artwork;
      const int text_x = inset+(bitmap ? artwork+8 : 0);
      char text[COMPLICATION_MAX_TEXT_BYTES+5];
      snprintf(text,sizeof(text),"%s",subtitle->text);
      GFont font = prv_gothic(s_large_text_enabled ? 28 : 24,true);
      prv_ellipsize_text(text,sizeof(text),font,bounds.size.w-text_x-inset-2,artwork);
      GSize measured = prv_measure(text,font,2048);
      // Native text frames include baseline leading. Anchor both the image
      // and the artist frame to the same lower row rather than centering art
      // against the title's two-line box.
      const int text_y = row_y+artwork-measured.h;
      if (bitmap) {
        const bool cover = (title->flags & COMPLICATION_FLAG_IMAGE)!=0;
        const int visible = prv_bitmap_visible_bottom(title,artwork,cover);
        if (visible) { prv_draw_bitmap(ctx,title,inset,row_y+artwork-visible,artwork,ink,cover); }
      }
      prv_draw_text(ctx,text,font,GRect(text_x,text_y,bounds.size.w-text_x-inset,
          measured.h+4),ink,GTextAlignmentLeft,GTextOverflowModeTrailingEllipsis);
    } else if (bitmap) {
      prv_draw_bitmap(ctx,title,inset,bounds.size.h-artwork-8,artwork,ink,
          (title->flags & COMPLICATION_FLAG_IMAGE)!=0);
    }
  } else {
    const int heading_height = artwork+8;
    const int text_x = inset+(bitmap ? artwork+8 : 0);
    char heading[COMPLICATION_MAX_TEXT_BYTES+5];
    snprintf(heading,sizeof(heading),"%s",title->text);
    GFont font = prv_gothic(s_large_text_enabled ? 28 : 24,true);
    prv_ellipsize_text(heading,sizeof(heading),font,bounds.size.w-text_x-inset-2,artwork);
    const GSize measured = prv_measure(heading,font,2048);
    const int bottom = 8+artwork;
    if (bitmap) {
      const int visible = prv_bitmap_visible_bottom(title,artwork,false);
      if (visible) { prv_draw_bitmap(ctx,title,inset,bottom-visible,artwork,ink,false); }
    }
    prv_draw_text(ctx,heading,font,GRect(text_x,bottom-measured.h,
        bounds.size.w-text_x-inset,measured.h+4),ink,GTextAlignmentLeft,GTextOverflowModeTrailingEllipsis);
    if (has_subtitle) {
      prv_draw_text(ctx,subtitle->text,prv_gothic(28,true),
          GRect(inset,heading_height+4,width,bounds.size.h-heading_height-10),
          ink,GTextAlignmentLeft,GTextOverflowModeTrailingEllipsis);
    }
  }
}

static void prv_apply_unobstructed_layout(void);

static void prv_draw_status(Layer *layer, GContext *ctx) {
  if (!s_show_status) { return; }
  const WatchLayoutGeometry geometry = prv_current_geometry();
  const int y = geometry.slots[0].rect.y + (geometry.slots[0].rect.height - 10) / 2;
  int x = geometry.slots[0].rect.x;
  graphics_context_set_stroke_color(ctx, prv_reading_ink());
  graphics_context_set_fill_color(ctx, prv_reading_ink());
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
  graphics_context_set_fill_color(ctx,s_background_color);
  graphics_fill_rect(ctx,layer_get_bounds(layer),0,GCornerNone);
  const WatchLayoutGeometry geometry = prv_current_geometry();
  graphics_context_set_fill_color(ctx,geometry.context.width>0 ? prv_context_color() : s_accent_color);
  graphics_fill_rect(ctx,prv_layout_rect(geometry.band),0,GCornerNone);
  if (!geometry.compact_header) {
    graphics_context_set_fill_color(ctx,s_accent_color);
    graphics_fill_rect(ctx,GRect(0,28,200,2),0,GCornerNone);
  }
  const GColor divider = geometry.context.width>0 ? s_text_color : prv_contrast_ink(s_accent_color);
  graphics_context_set_stroke_color(ctx,divider);
  graphics_context_set_stroke_width(ctx,1);
  graphics_draw_line(ctx,GPoint(geometry.divider_x,geometry.divider_top),
      GPoint(geometry.divider_x,geometry.divider_bottom));
}

static GFont prv_clock_font(const WatchLayoutGeometry *geometry) {
  if (geometry->compact_header) { return prv_numeric(26); }
  GFont font = fonts_get_system_font(s_large_text_enabled
      ? FONT_KEY_LECO_60_BOLD_NUMBERS_AM_PM : FONT_KEY_LECO_60_NUMBERS_AM_PM);
  if (prv_text_fits(s_time_buffer,font,geometry->time.width,geometry->time.height)) { return font; }
  font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  if (prv_text_fits(s_time_buffer,font,geometry->time.width,geometry->time.height)) { return font; }
  return fonts_get_system_font(FONT_KEY_LECO_38_BOLD_NUMBERS);
}

static void prv_apply_unobstructed_layout(void) {
  if (s_window_layer==NULL || s_date_layer==NULL || s_time_layer==NULL) { return; }
  const WatchLayoutGeometry geometry = prv_current_geometry();
  const char *date = geometry.compact_header ? s_compact_date_buffer : s_date_buffer;
  GFont date_font = prv_gothic(geometry.compact_header ? 14 : s_large_text_enabled ? 24 : 18,true);
  if (!prv_text_fits(date,date_font,geometry.date.width,geometry.date.height)) {
    date_font = prv_gothic(geometry.compact_header ? 14 : 18,true);
  }
  if (!prv_text_fits(date,date_font,geometry.date.width,geometry.date.height)) { date_font=prv_gothic(14,true); }
  text_layer_set_font(s_date_layer,date_font);
  text_layer_set_text(s_date_layer,date);
  text_layer_set_text_alignment(s_date_layer,geometry.compact_header ? GTextAlignmentRight : GTextAlignmentCenter);
  layer_set_frame(text_layer_get_layer(s_date_layer),prv_layout_rect(geometry.date));
  text_layer_set_font(s_time_layer,prv_clock_font(&geometry));
  text_layer_set_text(s_time_layer,s_time_buffer);
  text_layer_set_text_alignment(s_time_layer,geometry.compact_header ? GTextAlignmentLeft : GTextAlignmentCenter);
  layer_set_frame(text_layer_get_layer(s_time_layer),prv_layout_rect(geometry.time));
  for (unsigned i=0;i<COMPLICATION_SLOT_COUNT;++i) {
    const bool visible = s_has_complications && complication_is_selected(&s_complications.slots[i]) &&
        geometry.slots[i].rect.width>0;
    WatchLayoutRect rect = geometry.slots[i].rect;
    if (rect.width>0 && i==0 && s_show_status) {
      const int reserve = prv_status_left_reserve();
      rect.x += reserve;
      rect.width -= reserve;
    } else if (rect.width>0 && i==1 && s_show_status) {
      rect.width -= prv_status_right_reserve();
    }
    layer_set_frame(s_complication_layers[i],prv_layout_rect(rect));
    layer_set_hidden(s_complication_layers[i],!visible);
    layer_mark_dirty(s_complication_layers[i]);
  }
  layer_set_frame(s_context_layer,prv_layout_rect(geometry.context));
  layer_set_hidden(s_context_layer,geometry.context.width==0);
  layer_mark_dirty(s_context_layer);
  layer_mark_dirty(s_status_layer);
  layer_mark_dirty(s_background_layer);
}

static void prv_update_clock(void) {
  const time_t now = time(NULL);
  struct tm *current = localtime(&now);
  if (current==NULL) { return; }
  const char *day = prv_get_localized_days()[current->tm_wday];
  strftime(s_time_buffer,sizeof(s_time_buffer),s_use_12_hour ? "%I:%M" : "%H:%M",current);
  if (s_use_12_hour && s_time_buffer[0]=='0') { memmove(s_time_buffer,s_time_buffer+1,strlen(s_time_buffer)); }
  snprintf(s_date_buffer,sizeof(s_date_buffer),"%s · %d.%d.",day,current->tm_mday,current->tm_mon+1);
  char abbreviated[16];
  watch_layout_abbreviate(day,abbreviated,sizeof(abbreviated),3);
  snprintf(s_compact_date_buffer,sizeof(s_compact_date_buffer),"%s %d.%d.",abbreviated,
      current->tm_mday,current->tm_mon+1);
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
  if (s_window!=NULL) { window_set_background_color(s_window,s_background_color); }
  if (s_date_layer!=NULL) { text_layer_set_text_color(s_date_layer,s_text_color); }
  if (s_time_layer!=NULL) { text_layer_set_text_color(s_time_layer,s_text_color); }
  prv_apply_unobstructed_layout();
}

static void prv_apply_effects(void) {
  // Legacy TEXT_SHADOW is accepted, but native typography is always crisp.
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

static bool prv_color_value(const Tuple *tuple, uint32_t *result) {
  if (tuple==NULL || (tuple->type!=TUPLE_INT && tuple->type!=TUPLE_UINT)) { return false; }
  uint32_t value;
  if (tuple->length==1) {
    if (tuple->type==TUPLE_INT && tuple->value->int8<0) { return false; }
    value=tuple->value->uint8;
  } else if (tuple->length==2) {
    if (tuple->type==TUPLE_INT && tuple->value->int16<0) { return false; }
    value=tuple->value->uint16;
  } else if (tuple->length==4) {
    if (tuple->type==TUPLE_INT && tuple->value->int32<0) { return false; }
    value=tuple->value->uint32;
  } else { return false; }
  if (value>0xffffff) { return false; }
  *result=value;
  return true;
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
  Tuple *accent = dict_find(iterator, MESSAGE_KEY_ACCENT_COLOR);
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
  uint32_t accent_value;
  if (prv_color_value(accent,&accent_value)) {
    s_accent_color = GColorFromHEX(accent_value);
    persist_write_int(PERSIST_KEY_ACCENT_COLOR,(int32_t)accent_value);
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
  text_layer_set_background_color(layer,GColorClear);
  text_layer_set_text_color(layer,s_text_color);
  text_layer_set_text_alignment(layer,GTextAlignmentCenter);
  text_layer_set_overflow_mode(layer,GTextOverflowModeFill);
  text_layer_set_font(layer,font);
}

static void prv_window_load(Window *window) {
  s_window_layer = window_get_root_layer(window);
  const GRect bounds = layer_get_bounds(s_window_layer);
  s_background_layer = layer_create(bounds);
  layer_set_update_proc(s_background_layer,prv_draw_background);
  layer_add_child(s_window_layer,s_background_layer);
  s_drop_path = gpath_create(&s_drop_info);
  s_small_drop_path = gpath_create(&s_small_drop_info);
  s_date_layer = text_layer_create(GRect(8,0,184,28));
  prv_configure_text_layer(s_date_layer,prv_gothic(18,true));
  layer_add_child(s_window_layer,text_layer_get_layer(s_date_layer));
  s_time_layer = text_layer_create(GRect(4,48,192,78));
  prv_configure_text_layer(s_time_layer,fonts_get_system_font(FONT_KEY_LECO_60_NUMBERS_AM_PM));
  layer_add_child(s_window_layer,text_layer_get_layer(s_time_layer));
  for (unsigned i=0;i<COMPLICATION_SLOT_COUNT;++i) {
    s_complication_layers[i] = layer_create_with_data(GRect(0,0,0,0),sizeof(uint8_t));
    *(uint8_t *)layer_get_data(s_complication_layers[i]) = i;
    layer_set_update_proc(s_complication_layers[i],prv_draw_complication);
    layer_add_child(s_window_layer,s_complication_layers[i]);
  }
  s_context_layer = layer_create(GRect(0,78,200,150));
  layer_set_update_proc(s_context_layer,prv_draw_context);
  layer_add_child(s_window_layer,s_context_layer);
  s_status_layer = layer_create(bounds);
  layer_set_update_proc(s_status_layer,prv_draw_status);
  layer_add_child(s_window_layer,s_status_layer);
  prv_update_clock();
  UnobstructedAreaHandlers handlers = {
    .will_change=prv_unobstructed_area_will_change,
    .change=prv_unobstructed_area_change,
    .did_change=prv_unobstructed_area_did_change
  };
  unobstructed_area_service_subscribe(handlers,NULL);
}

static void prv_window_unload(Window *window) {
  unobstructed_area_service_unsubscribe();
  layer_destroy(s_status_layer);
  layer_destroy(s_context_layer);
  for (unsigned i=0;i<COMPLICATION_SLOT_COUNT;++i) { layer_destroy(s_complication_layers[i]); }
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
  if (s_drop_path!=NULL) { gpath_destroy(s_drop_path); s_drop_path=NULL; }
  if (s_small_drop_path!=NULL) { gpath_destroy(s_small_drop_path); s_small_drop_path=NULL; }
  layer_destroy(s_background_layer);
  s_window_layer=NULL;
}

static void prv_init(void) {
  s_background_color = GColorWhite;
  s_text_color = GColorBlack;
  s_accent_color = GColorFromHEX(0x55AAFF);
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
  if (persist_exists(PERSIST_KEY_ACCENT_COLOR)) {
    const int32_t accent = persist_read_int(PERSIST_KEY_ACCENT_COLOR);
    if (accent>=0 && accent<=0xffffff) { s_accent_color=GColorFromHEX(accent); }
  }
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
