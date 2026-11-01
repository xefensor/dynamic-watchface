#include <pebble.h>
#include "complication_data.h"
#include "context_data.h"
#include <pebble-watchface-core/pebble-watchface-core.h>
#include "watch_layout.h"
#include "reading_presentation.h"
#include "generated_clock_glyphs.h"
#include "generated_body_metrics.h"
#include "font_raster.h"
#include "music_layout.h"
#include "native_reading.h"
#include "context_presentation.h"
#include "context_effects.h"
#include "native_settings.h"

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
#define PERSIST_KEY_ANIMATIONS 71
#define PERSIST_KEY_CLOCK_FORMAT 72
#define PERSIST_KEY_VIBRATION 73
#define PERSIST_KEY_NIGHT_ENABLED 74
#define PERSIST_KEY_NIGHT_BACKGROUND 75
#define PERSIST_KEY_NIGHT_TEXT 76
#define PERSIST_KEY_NIGHT_ACCENT 77
#define NATIVE_INBOX_BYTES 6144
#define FULL_LAYOUT_HEIGHT 228
#define MOTION_FRAME_MS 50
#define LAYOUT_DURATION_MS 520

static Window *s_window;
static Layer *s_window_layer, *s_background_layer, *s_context_layer;
static GColor s_background_color, s_text_color, s_accent_color;
static GColor s_day_colors[3],s_night_colors[3];
static bool s_night_enabled,s_music_paused;
static uint8_t s_vibration;
static uint32_t s_seen_burst;
static bool s_use_12_hour,s_app_focused=true;
static uint8_t s_clock_format;
static bool s_show_status, s_animations_enabled;
static BatteryChargeState s_battery;
// Allocate this bounded block once. Keeping the shared engine, transient
// snapshots and raster scratch off BSS leaves the app's load image below the
// SDK's16-bit virtual-size limit without adding any per-frame allocation.
typedef struct {
  WatchfaceCore core;
  RenderContext context_from,context_to;
  GPoint clock_path_points[CLOCK_CONTOUR_MAX_POINTS];
  GPath clock_path;
  FontRasterPoint clock_raster_points[CLOCK_GLYPH_MAX_POINTS];
  FontRasterContour clock_raster_contours[CLOCK_GLYPH_MAX_CONTOURS];
  int32_t clock_intersections[CLOCK_GLYPH_MAX_POINTS];
  uint8_t clock_coverage[200];
  char music_identity[COMPLICATION_MAX_TEXT_BYTES+1];
  uint16_t music_offset,music_next;
  bool music_visible;
} NativeState;
static NativeState *s_native;
#define s_core (s_native->core)
#define s_context_from (s_native->context_from)
#define s_context_to (s_native->context_to)
#define s_clock_path_points (s_native->clock_path_points)
#define s_clock_path (s_native->clock_path)
#define s_clock_raster_points (s_native->clock_raster_points)
#define s_clock_raster_contours (s_native->clock_raster_contours)
#define s_clock_intersections (s_native->clock_intersections)
#define s_clock_coverage (s_native->clock_coverage)
#define s_context s_core.context
#define s_complications s_core.complications
#define s_has_complications s_core.has_complications
#define s_phone_connected s_core.phone_connected
static AppTimer *s_context_timer, *s_startup_timer, *s_motion_timer,*s_title_timer;
// Three UTF-8 weekday scalars + separator + DD.MM. + NUL fit in24 bytes.
static char s_compact_date_buffer[24], s_time_buffer[6];
static char s_weekday_buffer[13], s_numeric_date_buffer[8];
static const uint8_t s_body_points[] = {11,14,16,20,24,26,28,32};
#define BODY_FONT_COUNT (sizeof(s_body_points)/sizeof(s_body_points[0]))
static GFont s_body_fonts[BODY_FONT_COUNT];
static bool s_body_loaded[BODY_FONT_COUNT];
static const uint32_t s_body_resources[BODY_FONT_COUNT] = {RESOURCE_ID_FONT_BODY_11,RESOURCE_ID_FONT_BODY_14,
  RESOURCE_ID_FONT_BODY_16,RESOURCE_ID_FONT_BODY_20,RESOURCE_ID_FONT_BODY_24,
  RESOURCE_ID_FONT_BODY_26,RESOURCE_ID_FONT_BODY_28,RESOURCE_ID_FONT_BODY_32};
static WatchPose s_pose, s_pose_from, s_pose_target;
static bool s_pose_ready, s_layout_active, s_slide_active, s_peek_transition;
static uint32_t s_layout_started, s_slide_started, s_slide_duration;
static float s_slide_amount = 1;
static int s_slide_direction = 1;
static bool s_private_rebaseline,s_banner_slide;
static bool s_format_reply_pending,s_outbox_format_only;

static void prv_apply_unobstructed_layout(void);
static void prv_sync_layers(void);
static void prv_send_clock_format(void);
static void prv_schedule_context_expiry(void);
static void prv_context_expired(void *context);
static MusicTextMetrics prv_music_measure(void *context,const char *text,int point);
static void prv_music_reset(void);
static void prv_music_cancel_timer(void);
static void prv_music_continue(void *context);
static void prv_apply_colors(void);


// The face displays three-character day names. Keep only that exact visible
// prefix, rather than storing unused full names in the constrained load image.
static const char *const s_english_days[] = {
  "SUN",
  "MON",
  "TUE",
  "WED",
  "THU",
  "FRI",
  "SAT"
};

static const char *const s_czech_days[] = {
  "NED", "PON", "ÚTE", "STŘ", "ČTV", "PÁT", "SOB"
};
static const char *const s_slovak_days[] = {
  "NED", "PON", "UTO", "STR", "ŠTV", "PIA", "SOB"
};
static const char *const s_german_days[] = {
  "SON", "MON", "DIE", "MIT", "DON", "FRE", "SAM"
};
static const char *const s_french_days[] = {
  "DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"
};
static const char *const s_spanish_days[] = {
  "DOM", "LUN", "MAR", "MIÉ", "JUE", "VIE", "SÁB"
};
static const char *const s_italian_days[] = {
  "DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"
};
static const char *const s_portuguese_days[] = {
  "DOM", "SEG", "TER", "QUA", "QUI", "SEX", "SÁB"
};
static const char *const s_dutch_days[] = {
  "ZON", "MAA", "DIN", "WOE", "DON", "VRI", "ZAT"
};
static const char *const s_polish_days[] = {
  "NIE", "PON", "WTO", "ŚRO", "CZW", "PIĄ", "SOB"
};
static const char *const s_danish_days[] = {
  "SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"
};
static const char *const s_swedish_days[] = {
  "SÖN", "MÅN", "TIS", "ONS", "TOR", "FRE", "LÖR"
};
static const char *const s_norwegian_days[] = {
  "SØN", "MAN", "TIR", "ONS", "TOR", "FRE", "LØR"
};
static const char *const s_finnish_days[] = {
  "SUN", "MAA", "TII", "KES", "TOR", "PER", "LAU"
};
static const char *const s_hungarian_days[] = {
  "VAS", "HÉT", "KED", "SZE", "CSÜ", "PÉN", "SZO"
};
static const char *const s_romanian_days[] = {
  "DUM", "LUN", "MAR", "MIE", "JOI", "VIN", "SÂM"
};
static const char *const s_turkish_days[] = {
  "PAZ", "PAZ", "SAL", "ÇAR", "PER", "CUM", "CUM"
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


static int prv_round(float number) { return (int)(number < 0 ? number-0.5f : number+0.5f); }
static int prv_point(int preferred) {
  for(size_t i=BODY_FONT_COUNT;i>0;--i) if(s_body_points[i-1]<=preferred) return s_body_points[i-1];
  return s_body_points[0];
}
static GFont prv_body(int point) {
  point=prv_point(point);
  for(size_t i=0;i<BODY_FONT_COUNT;++i) if(s_body_points[i]==point) return s_body_fonts[i];
  return s_body_fonts[0];
}
static GSize prv_measure(const char *text,int point) {
  return graphics_text_layout_get_content_size(text,prv_body(point),GRect(0,0,2048,512),
      GTextOverflowModeFill,GTextAlignmentLeft);
}
static void prv_text(GContext *ctx,const char *text,int point,int x,int baseline,
                     int width,GColor color,GTextAlignment alignment) {
  point=prv_point(point);
  graphics_context_set_text_color(ctx,color);
  graphics_draw_text(ctx,text,prv_body(point),GRect(x,baseline-point,width,point+12),
      GTextOverflowModeFill,alignment,NULL);
}
static void prv_ellipsize(char *text,size_t capacity,int point,int width) {
  if(prv_measure(text,point).w<=width) return;
  if(capacity<4){if(capacity)text[0]='\0';return;}
  char prefix[COMPLICATION_MAX_TEXT_BYTES+5];
  const size_t prefix_capacity=capacity-3<sizeof(prefix)?capacity-3:sizeof(prefix);
  watch_layout_copy_utf8(text,prefix,prefix_capacity);
  while(watch_layout_pop_utf8(prefix)) {
    const size_t length=strlen(prefix);
    if(length+4>capacity)continue;
    memcpy(text,prefix,length);memcpy(text+length,"…",4);
    if(prv_measure(text,point).w<=width) return;
  }
  memcpy(text,"…",4);
}
static GColor prv_contrast_ink(GColor color) {
  return (GColor){.argb=reading_contrast_ink(color.argb)};
}
static GColor prv_accent_color(void) {
  return (GColor){.argb=reading_accent_ink(s_accent_color.argb,s_background_color.argb,s_text_color.argb)};
}
static GColor prv_warning_color(void) {
  return (GColor){.argb=reading_warning_ink(s_background_color.argb)};
}
static GColor prv_muted_text(float alpha) {
  const uint8_t ink=reading_secondary_ink(s_text_color.argb,s_background_color.argb);
  alpha=watch_clamp(alpha);
  const unsigned r=prv_round(watch_lerp((s_background_color.argb>>4)&3,(ink>>4)&3,alpha));
  const unsigned g=prv_round(watch_lerp((s_background_color.argb>>2)&3,(ink>>2)&3,alpha));
  const unsigned b=prv_round(watch_lerp(s_background_color.argb&3,ink&3,alpha));
  return (GColor){.argb=0xc0|(r<<4)|(g<<2)|b};
}
static GColor prv_faded_text(float alpha) {
  alpha=watch_clamp(alpha);
  const unsigned r=prv_round(watch_lerp((s_background_color.argb>>4)&3,(s_text_color.argb>>4)&3,alpha));
  const unsigned g=prv_round(watch_lerp((s_background_color.argb>>2)&3,(s_text_color.argb>>2)&3,alpha));
  const unsigned b=prv_round(watch_lerp(s_background_color.argb&3,s_text_color.argb&3,alpha));
  return (GColor){.argb=0xc0|(r<<4)|(g<<2)|b};
}
static const ClockGlyph *prv_clock_glyph(uint32_t code) {
  for(unsigned i=0;i<CLOCK_GLYPH_COUNT;++i) if(clock_glyphs[i].code==code) return &clock_glyphs[i];
  return NULL;
}
static float prv_vector_width(const char *text,float point) {
  unsigned advance=0;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor) {
    const ClockGlyph *glyph=prv_clock_glyph(body_metric_next_codepoint(&cursor));
    if(glyph==NULL) return -1;
    advance+=glyph->advance;
  }
  return advance*point/CLOCK_FONT_UNITS_PER_EM;
}
static bool prv_same_point(GPoint a,GPoint b) { return a.x==b.x&&a.y==b.y; }
static bool prv_collinear(GPoint a,GPoint b,GPoint c) {
  const int32_t ax=b.x-a.x,ay=b.y-a.y,bx=c.x-b.x,by=c.y-b.y;
  return ax*by-ay*bx==0 && ax*bx+ay*by>=0;
}
// A dense font outline can collapse several vertices onto one physical
// pixel. Feed GPath a clean polygon rather than zero-length/repeated edges.
static unsigned prv_pixel_polygon(unsigned count) {
  unsigned out=0;
  for(unsigned i=0;i<count;++i) {
    const GPoint point=s_clock_path_points[i];
    if(out&&prv_same_point(s_clock_path_points[out-1],point))continue;
    while(out>=2&&prv_collinear(s_clock_path_points[out-2],s_clock_path_points[out-1],point))--out;
    s_clock_path_points[out++]=point;
  }
  if(out>1&&prv_same_point(s_clock_path_points[0],s_clock_path_points[out-1]))--out;
  bool changed=true;
  while(changed&&out>3) {
    changed=false;
    if(prv_collinear(s_clock_path_points[out-1],s_clock_path_points[0],s_clock_path_points[1])) {
      memmove(s_clock_path_points,s_clock_path_points+1,(--out)*sizeof(GPoint));changed=true;
    }
    if(out>3&&prv_collinear(s_clock_path_points[out-2],s_clock_path_points[out-1],s_clock_path_points[0])) {
      --out;changed=true;
    }
  }
  return out;
}
static void prv_gpath_glyph(GContext *ctx,const ClockGlyph *glyph,float x,float baseline,
                            float scale,GColor color) {
    for(unsigned i=0;i<glyph->contour_count;++i) {
      const ClockContour *contour=&clock_contours[glyph->first_contour+i];
      for(unsigned j=0;j<contour->point_count;++j) {
        const ClockPoint p=clock_points[contour->first_point+j];
        s_clock_path_points[j]=GPoint(prv_round(x+p.x*scale),prv_round(baseline-p.y*scale));
      }
      s_clock_path.num_points=prv_pixel_polygon(contour->point_count);
      if(s_clock_path.num_points<3)continue;
      graphics_context_set_fill_color(ctx,contour->hole ? s_background_color : color);
      gpath_draw_filled(ctx,&s_clock_path);
    }
}
typedef struct { GContext *ctx; GColor colors[FONT_RASTER_COVERAGE+1]; } ClockPaint;
static void prv_clock_row(void *context,int y,int first,int end,const uint8_t *coverage) {
  ClockPaint *paint=context;
  for(int x=first;x<end;) {
    const GColor color=paint->colors[coverage[x]];
    const int begin=x++;
    while(x<end&&paint->colors[coverage[x]].argb==color.argb)++x;
    if(color.argb==s_background_color.argb)continue;
    graphics_context_set_fill_color(paint->ctx,color);
    graphics_fill_rect(paint->ctx,GRect(begin,y,x-begin,1),0,GCornerNone);
  }
}
static bool prv_raster_glyph(ClockPaint *paint,const ClockGlyph *glyph,
                             float x,float baseline,float scale) {
  if(glyph->contour_count>CLOCK_GLYPH_MAX_CONTOURS)return false;
  unsigned at=0;
  for(unsigned i=0;i<glyph->contour_count;++i) {
    const ClockContour *contour=&clock_contours[glyph->first_contour+i];
    if(at+contour->point_count>CLOCK_GLYPH_MAX_POINTS)return false;
    s_clock_raster_contours[i]=(FontRasterContour){at,contour->point_count};
    for(unsigned j=0;j<contour->point_count;++j) {
      const ClockPoint p=clock_points[contour->first_point+j];
      s_clock_raster_points[at++]=(FontRasterPoint){
        prv_round((x+p.x*scale)*FONT_RASTER_ONE),
        prv_round((baseline-p.y*scale)*FONT_RASTER_ONE)};
    }
  }
  const FontRasterScratch scratch={s_clock_coverage,sizeof(s_clock_coverage),
      s_clock_intersections,CLOCK_GLYPH_MAX_POINTS};
  return font_raster_rows(s_clock_raster_points,at,s_clock_raster_contours,
      glyph->contour_count,200,FULL_LAYOUT_HEIGHT,scratch,prv_clock_row,paint);
}
static void prv_vector(GContext *ctx,const char *text,float x,float baseline,float point,GColor color) {
  ClockPaint paint={.ctx=ctx};
  const unsigned fr=(color.argb>>4)&3,fg=(color.argb>>2)&3,fb=color.argb&3;
  const unsigned br=(s_background_color.argb>>4)&3,bg=(s_background_color.argb>>2)&3,bb=s_background_color.argb&3;
  for(unsigned i=0;i<=FONT_RASTER_COVERAGE;++i) {
    const unsigned back=FONT_RASTER_COVERAGE-i,half=FONT_RASTER_COVERAGE/2;
    paint.colors[i]=(GColor){.argb=0xc0|
      (((fr*i+br*back+half)/FONT_RASTER_COVERAGE)<<4)|
      (((fg*i+bg*back+half)/FONT_RASTER_COVERAGE)<<2)|
       ((fb*i+bb*back+half)/FONT_RASTER_COVERAGE)};
  }
  const float scale=point/CLOCK_FONT_UNITS_PER_EM;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor) {
    const ClockGlyph *glyph=prv_clock_glyph(body_metric_next_codepoint(&cursor));
    if(glyph==NULL)return;
    // A bounded static scanline preserves subpixel outlines; GPath remains a
    // safe fallback if a future asset exceeds the reviewed scratch capacity.
    if(!prv_raster_glyph(&paint,glyph,x,baseline,scale))
      prv_gpath_glyph(ctx,glyph,x,baseline,scale,color);
    x+=glyph->advance*scale;
  }
}
static uint32_t prv_millis(void) {
  time_t seconds; uint16_t millis;
  time_ms(&seconds,&millis);
  return (uint32_t)seconds*1000u+millis;
}
static WatchPose prv_full_pose(void) {
  if(s_context_to.mode==CONTEXT_NONE||s_private_rebaseline)return watch_pose_idle();
  if(s_context_to.heading_only&&!s_context_to.has_music)return watch_pose_summary();
  return watch_pose_context(s_context_to.mode==CONTEXT_NOTIFICATION);
}
static WatchPose prv_current_pose(void) {
  const int height=s_window_layer ? layer_get_unobstructed_bounds(s_window_layer).size.h : 228;
  return watch_pose_at_height(s_pose,height);
}
static void prv_redact_context(RenderContext *value) {
  render_context_redact(value,&s_context,s_phone_connected,s_private_rebaseline,time(NULL));
}
static void prv_private_clear(void) {
  memset(&s_context_from,0,sizeof(s_context_from));
  memset(&s_context_to,0,sizeof(s_context_to));
  s_slide_active=false;s_slide_amount=1;s_banner_slide=false;
  s_music_paused=false;prv_music_reset();
}
static void prv_motion_frame(void *context) {
  (void)context;s_motion_timer=NULL;
  const uint32_t now=prv_millis();
  if(s_layout_active) {
    const float t=watch_clamp((now-s_layout_started)/(float)LAYOUT_DURATION_MS);
    s_pose=watch_pose_between(s_pose_from,s_pose_target,watch_ease(t));
    if(t>=1) { s_pose=s_pose_target;s_layout_active=false; }
  }
  if(s_slide_active) {
    s_slide_amount=watch_smooth((now-s_slide_started)/(float)s_slide_duration);
    if(s_slide_amount>=1) { s_slide_active=false;memset(&s_context_from,0,sizeof(s_context_from)); }
  }
  prv_redact_context(&s_context_from);prv_redact_context(&s_context_to);
  prv_sync_layers();
  if(s_layout_active || s_slide_active) s_motion_timer=app_timer_register(MOTION_FRAME_MS,prv_motion_frame,NULL);
}
static void prv_cancel_motion(void) {
  if(s_motion_timer) { app_timer_cancel(s_motion_timer);s_motion_timer=NULL; }
  s_layout_active=false;s_slide_active=false;s_slide_amount=1;
  memset(&s_context_from,0,sizeof(s_context_from));
}
static void prv_apply_unobstructed_layout(void) {
  if(s_window_layer==NULL) return;
  const ContextView next=render_context_view(&s_context,s_phone_connected,s_private_rebaseline,time(NULL));
  const ContextMode previous=s_context_to.mode;
  const bool content_slide=render_context_should_slide(&s_context_to,&next);
  prv_redact_context(&s_context_to);prv_redact_context(&s_context_from);
  const bool motion=s_animations_enabled && !s_peek_transition &&
      layer_get_unobstructed_bounds(s_window_layer).size.h>=228;
  if(!render_context_equal(&s_context_to,&next)) {
    const ComplicationSlot *old_music=render_context_music(&s_context_to,false);
    const ComplicationSlot *old_artist=render_context_music(&s_context_to,true);
    const ComplicationSlot *new_music=next.mode==CONTEXT_MUSIC?next.title:next.music_title;
    const ComplicationSlot *new_artist=next.mode==CONTEXT_MUSIC?next.subtitle:next.music_artist;
    s_banner_slide=(s_context_to.heading_only||next.heading_only)&&old_music&&new_music&&
        render_slot_identity_equal(old_music,new_music)&&old_artist&&
        render_optional_slot_equal(old_artist,new_artist);
    s_context_from=s_context_to;render_context_capture(&s_context_to,&next);
    prv_redact_context(&s_context_from);
    s_slide_direction=next.mode==CONTEXT_MUSIC && previous==CONTEXT_NOTIFICATION ? -1 : 1;
    s_slide_duration=next.mode==previous ? 280 : 340;
    s_slide_active=motion && content_slide;
    s_slide_amount=s_slide_active ? 0 : 1;s_slide_started=prv_millis();
    if(!s_slide_active) memset(&s_context_from,0,sizeof(s_context_from));
  } else render_context_capture(&s_context_to,&next); // A lease confirmation does not replay motion.
  const ComplicationSlot *music=render_context_music(&s_context_to,false);
  if(!music||!context_slot_fresh(music,time(NULL)))prv_music_reset();
  else if(strcmp(s_native->music_identity,music->text)!=0) {
    prv_music_reset();watch_layout_copy_utf8(music->text,s_native->music_identity,
        sizeof(s_native->music_identity));
  }
  s_native->music_visible=next.mode==CONTEXT_MUSIC&&
      layer_get_unobstructed_bounds(s_window_layer).size.h>=228;
  if(!s_native->music_visible)prv_music_cancel_timer();
  const WatchPose target=prv_full_pose();
  if(!s_pose_ready) {s_pose=s_pose_from=s_pose_target=target;s_pose_ready=true;}
  else if(memcmp(&target,&s_pose_target,sizeof(target))!=0) {
    s_pose_from=s_pose;s_pose_target=target;s_layout_started=prv_millis();s_layout_active=motion;
    if(!motion)s_pose=target;
  }
  if(!s_animations_enabled) {prv_cancel_motion();s_pose=s_pose_target;}
  prv_sync_layers();
  if((s_layout_active||s_slide_active)&&s_motion_timer==NULL)
    s_motion_timer=app_timer_register(MOTION_FRAME_MS,prv_motion_frame,NULL);
}
static void prv_sync_layers(void) {
  if(!s_background_layer || !s_context_layer) return;
  const WatchPose p=prv_current_pose();
  const float reveal=watch_smooth((watch_context_visibility(p)-0.72f)/0.28f);
  const int height=prv_round(142*reveal);
  layer_set_frame(s_context_layer,GRect(0,86,200,height));
  layer_set_bounds(s_context_layer,GRect(0,0,200,height));
  layer_set_hidden(s_context_layer,height<=0);
  layer_mark_dirty(s_context_layer);layer_mark_dirty(s_background_layer);
}
static void prv_schedule_context_expiry(void) {
  if(s_context_timer) {app_timer_cancel(s_context_timer);s_context_timer=NULL;}
  const time_t now=time(NULL);
  const uint32_t next=wfc_prune_context(&s_core,now);
  if(next>(uint32_t)now) s_context_timer=app_timer_register((next-(uint32_t)now)*1000,prv_context_expired,NULL);
}
static void prv_context_expired(void *context) {
  (void)context;s_context_timer=NULL;
  prv_schedule_context_expiry();prv_apply_unobstructed_layout();
}

static void prv_draw_drop(GContext *ctx,int cx,int cy,int radius,GColor color) {
  const int r=radius<4 ? 4 : radius;
  GPoint points[3]={GPoint(cx,cy-r),GPoint(cx-r/2,cy),GPoint(cx+r/2,cy)};
  GPath path={.num_points=3,.points=points};
  graphics_context_set_fill_color(ctx,color);gpath_draw_filled(ctx,&path);
  graphics_fill_circle(ctx,GPoint(cx,cy+r/4),r/2);
}
static int prv_probability(const ReadingParts *parts) {
  if(!reading_is_probability(parts))return -1;
  const char *at=parts->number;if(*at=='+')++at;
  int value=0;while(*at>='0'&&*at<='9')value=value*10+*at++-'0';
  value*=100;
  if(*at=='.'||*at==',') {++at;if(*at)value+=(*at++-'0')*10;if(*at)value+=*at-'0';}
  return value;
}
static void prv_draw_symbol(GContext *ctx,const ComplicationSlot *slot,int cx,int cy,
                            int radius,int size,unsigned kind,int range) {
  const bool bitmap=(slot->flags&COMPLICATION_FLAG_BITMAP)!=0;
  graphics_context_set_antialiased(ctx,true);
  GColor ink=s_text_color;
  if(range>=0) {
    graphics_context_set_stroke_width(ctx,radius/4<2?2:radius/4);
    graphics_context_set_stroke_color(ctx,prv_muted_text(0.4f));
    graphics_draw_circle(ctx,GPoint(cx,cy),radius);
    graphics_context_set_stroke_color(ctx,prv_accent_color());
    if(range>0)graphics_draw_arc(ctx,GRect(cx-radius,cy-radius,2*radius,2*radius),
        GOvalScaleModeFitCircle,0,TRIG_MAX_ANGLE*range/10000);
  } else if(kind!=COMPLICATION_SEMANTIC_GENERIC || (bitmap&&!(slot->flags&COMPLICATION_FLAG_IMAGE))) {
    graphics_context_set_fill_color(ctx,prv_accent_color());graphics_fill_circle(ctx,GPoint(cx,cy),radius);
    ink=prv_contrast_ink(prv_accent_color());
  }
  if(bitmap)prv_draw_bitmap(ctx,slot,cx-size/2,cy-size/2,size,ink,false);
  else if(kind==COMPLICATION_SEMANTIC_PRECIPITATION||kind==COMPLICATION_SEMANTIC_HUMIDITY)
    prv_draw_drop(ctx,cx,cy,size/2,ink);
  else if(kind==COMPLICATION_SEMANTIC_TEMPERATURE) {
    const int h=size/2,w=size/5<2?2:size/5;
    graphics_context_set_stroke_width(ctx,2);graphics_context_set_stroke_color(ctx,ink);
    graphics_draw_round_rect(ctx,GRect(cx-w/2,cy-h/2-2,w,h),w/2);
    graphics_context_set_fill_color(ctx,ink);graphics_fill_circle(ctx,GPoint(cx,cy+h/2-1),w);
    graphics_draw_line(ctx,GPoint(cx,cy-h/3),GPoint(cx,cy+h/2));
  } else if(kind==COMPLICATION_SEMANTIC_BATTERY) {
    const int w=size>12?size-4:8,h=w/2;
    graphics_context_set_stroke_width(ctx,2);graphics_context_set_stroke_color(ctx,ink);
    graphics_draw_round_rect(ctx,GRect(cx-w/2,cy-h/2,w,h),2);
    graphics_draw_line(ctx,GPoint(cx+w/2,cy-1),GPoint(cx+w/2,cy+1));
  } else if(kind==COMPLICATION_SEMANTIC_WIND) {
    graphics_context_set_stroke_width(ctx,2);graphics_context_set_stroke_color(ctx,ink);
    graphics_draw_line(ctx,GPoint(cx-size/3,cy-2),GPoint(cx+size/3,cy-2));
    graphics_draw_line(ctx,GPoint(cx-size/3,cy+2),GPoint(cx+size/4,cy+2));
    graphics_draw_line(ctx,GPoint(cx+size/3,cy-2),GPoint(cx+size/3,cy-4));
  } else if(kind==COMPLICATION_SEMANTIC_UV) {
    graphics_context_set_stroke_width(ctx,2);graphics_context_set_stroke_color(ctx,ink);
    graphics_draw_circle(ctx,GPoint(cx,cy),2);
    graphics_draw_line(ctx,GPoint(cx-4,cy),GPoint(cx-3,cy));
    graphics_draw_line(ctx,GPoint(cx+3,cy),GPoint(cx+4,cy));
    graphics_draw_line(ctx,GPoint(cx,cy-4),GPoint(cx,cy-3));
    graphics_draw_line(ctx,GPoint(cx,cy+3),GPoint(cx,cy+4));
  }
}
static bool prv_draw_number(GContext *ctx,const ReadingParts *parts,float center,float baseline,
                            float point,float unit_point,float width) {
  char number[READING_PART_BYTES];reading_native_number(parts->number,number,sizeof(number));
  const float vector_unit_width=parts->unit[0]?prv_vector_width(parts->unit,unit_point):0;
  const bool vector_unit=vector_unit_width>=0;
  const int body_unit_point=prv_point(prv_round(unit_point)<14?14:prv_round(unit_point));
  const float unit_width=vector_unit?vector_unit_width:prv_measure(parts->unit,body_unit_point).w;
  float number_width=prv_vector_width(number,point);
  if(number_width<0)return false;
  if(number_width+unit_width>width) {
    point*=((width-unit_width)/number_width);
    if(point<16)return false;
    number_width=prv_vector_width(number,point);
  }
  const float x=center-(number_width+unit_width)/2;
  prv_vector(ctx,number,x,baseline,point,s_text_color);
  if(unit_width) {
    if(vector_unit)prv_vector(ctx,parts->unit,x+number_width,baseline,unit_point,s_text_color);
    else prv_text(ctx,parts->unit,body_unit_point,prv_round(x+number_width),
        prv_round(baseline),prv_round(unit_width)+3,s_text_color,GTextAlignmentLeft);
  }
  return true;
}
static int prv_label_width(const void *context,const char *text) {
  return prv_measure(text,*(const int *)context).w;
}
static void prv_reading_label(GContext *ctx,const ComplicationSlot *slot,unsigned kind,
                               unsigned index,WatchPose p) {
  const int center=index?150:50;
  const int point=14;
  char label[COMPLICATION_MAX_LABEL_BYTES+16];
  watch_layout_copy_utf8(reading_label(slot,kind,index,i18n_get_system_locale()),label,sizeof(label));
  reading_label_ellipsize(label,sizeof(label),reading_is_aged(slot,time(NULL))?76:88,prv_label_width,&point);
  const int baseline=prv_round(p.row_y+p.label_y);
  if(s_window_layer&&baseline+body_text_descent(label,point)>
      layer_get_unobstructed_bounds(s_window_layer).size.h-2)return;
  prv_text(ctx,label,point,center-44,baseline,reading_is_aged(slot,time(NULL))?76:88,
      prv_muted_text(p.label_alpha),GTextAlignmentCenter);
}
static void prv_age_mark(GContext *ctx,const ComplicationSlot *slot,unsigned index,WatchPose p,bool label) {
  if(reading_is_aged(slot,time(NULL))) {
    const int baseline=prv_round(p.row_y+(label?p.label_y:p.value_y));
    if(s_window_layer&&baseline>layer_get_unobstructed_bounds(s_window_layer).size.h-2)return;
    graphics_context_set_stroke_width(ctx,1);graphics_context_set_stroke_color(ctx,prv_muted_text(p.label_alpha));
    const GPoint at=GPoint((index?150:50)+45,baseline-3);
    graphics_draw_circle(ctx,at,3);
    graphics_draw_line(ctx,at,GPoint(at.x,at.y-2));
    graphics_draw_line(ctx,at,GPoint(at.x+1,at.y+1));
  }
}
static bool prv_reading_text(GContext *ctx,const char *text,int center,int baseline,int width,
                              WatchPose p,bool state) {
  const bool compact=p.icon_x<0;
  int point=state?(compact?14:16):(compact?16:24);
  if(p.context>0.5f&&body_text_ascent(text,point)>16)point=14;
  MusicLines lines;
  music_wrap(text,point,width,state&&compact?1:2,false,prv_music_measure,NULL,&lines);
  if(compact&&lines.count>1&&point>14) {
    point=14;music_wrap(text,point,width,2,false,prv_music_measure,NULL,&lines);
  }
  int offsets[MUSIC_MAX_LINES];
  int height=music_line_offsets(&lines,point,prv_music_measure,NULL,offsets);
  if(p.context>0.5f&&height>30) {
    point=14;music_wrap(text,point,width,1,false,prv_music_measure,NULL,&lines);
    music_line_offsets(&lines,point,prv_music_measure,NULL,offsets);
  }
  if(lines.count>1)baseline=compact?prv_round(p.row_y+18):baseline+3;
  if(s_window_layer&&baseline>layer_get_unobstructed_bounds(s_window_layer).size.h-4)
    baseline=layer_get_unobstructed_bounds(s_window_layer).size.h-4;
  const int bottom=lines.count?offsets[lines.count-1]+body_text_descent(lines.text[lines.count-1],point):0;
  for(int i=0;i<lines.count;++i)prv_text(ctx,lines.text[i],point,center-width/2,
      baseline-bottom+offsets[i],width,state?prv_muted_text(1):s_text_color,GTextAlignmentCenter);
  return compact&&lines.count>1;
}
static void prv_draw_reading(GContext *ctx,unsigned index,WatchPose p) {
  if(!s_has_complications || !complication_is_selected(&s_complications.slots[index]))return;
  const ComplicationSlot *slot=&s_complications.slots[index];
  const ReadingState state=reading_state(slot,time(NULL));
  const bool fresh=state==READING_VALUE;
  if(!fresh) {
    if(p.icon_x==0&&slot->semantic_kind>=COMPLICATION_SEMANTIC_TEMPERATURE&&
        slot->semantic_kind<=COMPLICATION_SEMANTIC_UV)
      prv_draw_symbol(ctx,slot,index?150:50,prv_round(p.row_y+p.icon_y),
          prv_round(p.icon_radius),prv_round(p.icon_size),slot->semantic_kind,-1);
    const bool two=prv_reading_text(ctx,reading_state_label(state,i18n_get_system_locale()),
        index?150:50,prv_round(p.row_y+p.value_y),88,p,true);
    if(!two)prv_reading_label(ctx,slot,slot->semantic_kind,index,p);
    return;
  }
  const bool bitmap=fresh&&(slot->flags&COMPLICATION_FLAG_BITMAP);
  char text[COMPLICATION_MAX_TEXT_BYTES+5];
  bool has_text=prv_slot_text(slot,text,sizeof(text));
  if(!has_text&&!bitmap){snprintf(text,sizeof(text),"--");has_text=true;}
  ReadingParts parts;const bool numeric=has_text&&reading_parts(text,&parts);
  const unsigned kind=reading_semantic(slot,&parts,numeric);
  double range_amount;
  int range=reading_range(slot,&range_amount)?prv_round(range_amount*10000):-1;
  if(range<0&&numeric&&kind==COMPLICATION_SEMANTIC_PRECIPITATION)range=prv_probability(&parts);
  bool symbol=bitmap||kind==COMPLICATION_SEMANTIC_TEMPERATURE||
      kind==COMPLICATION_SEMANTIC_PRECIPITATION||kind==COMPLICATION_SEMANTIC_BATTERY||
      kind==COMPLICATION_SEMANTIC_HUMIDITY||kind==COMPLICATION_SEMANTIC_WIND||
      kind==COMPLICATION_SEMANTIC_UV||range>=0;
  const float center=index?150:50;
  const float inline_amount=watch_clamp(-p.icon_x/25);
  if(inline_amount>0.5f&&!numeric&&has_text&&prv_measure(text,16).w>60)symbol=false;
  if(bitmap&&!has_text) {
    const int size=prv_round(watch_lerp(40,22,inline_amount));
    prv_draw_bitmap(ctx,slot,prv_round(center-size/2.0f),
        prv_round(p.row_y+p.value_y-size),size,s_text_color,false);
    prv_reading_label(ctx,slot,kind,index,p);
    prv_age_mark(ctx,slot,index,p,true);
    return;
  }
  if(symbol) {
    prv_draw_symbol(ctx,slot,prv_round(center+p.icon_x),prv_round(p.row_y+p.icon_y),
        prv_round(p.icon_radius),prv_round(p.icon_size),kind,range);
  }
  const float value_center=center+(symbol?p.value_x:0);
  const float value_width=(symbol?88-28*inline_amount:88)-(reading_is_aged(slot,time(NULL))?8:0);
  const float baseline=p.row_y+p.value_y;
  bool text_takes_label=false;
  if(has_text) {
    bool drawn=numeric&&prv_draw_number(ctx,&parts,value_center,baseline,p.number_size,
        p.unit_size,value_width);
    if(!drawn && prv_vector_width(text,p.number_size)>0) {
      float point=p.number_size,width=prv_vector_width(text,point);
      if(width>value_width){point*=value_width/width;width=value_width;}
      if(point>=16){prv_vector(ctx,text,value_center-width/2,baseline,point,s_text_color);drawn=true;}
    }
    if(!drawn) {
      text_takes_label=prv_reading_text(ctx,text,prv_round(value_center),prv_round(baseline),
          prv_round(value_width),p,false);
    }
  }
  if(p.label_alpha>0.01f&&!text_takes_label)prv_reading_label(ctx,slot,kind,index,p);
  prv_age_mark(ctx,slot,index,p,!text_takes_label&&p.label_alpha>0.01f);
}
static void prv_draw_status(GContext *ctx,WatchPose p) {
  if(!s_show_status)return;
  const int y=prv_round(watch_lerp(watch_lerp(115,45,watch_clamp(p.context)),104,p.peek));
  int x=5;graphics_context_set_stroke_color(ctx,prv_warning_color());
  graphics_context_set_fill_color(ctx,prv_warning_color());graphics_context_set_stroke_width(ctx,1);
  if(s_battery.charge_percent<=10&&!s_battery.is_charging) {
    graphics_draw_rect(ctx,GRect(x,y+2,9,6));graphics_fill_rect(ctx,GRect(x+9,y+4,1,2),0,GCornerNone);
    graphics_fill_rect(ctx,GRect(x+2,y+4,1,2),0,GCornerNone);x+=12;
  }
  if(context_phone_battery(&s_context.slots[4],time(NULL),NULL)) {
    graphics_draw_round_rect(ctx,GRect(x+1,y,6,10),1);graphics_fill_rect(ctx,GRect(x+3,y+7,2,1),0,GCornerNone);
  }
  if(!s_phone_connected) {
    const int bx=185;
    graphics_draw_line(ctx,GPoint(bx+3,y),GPoint(bx+3,y+10));
    graphics_draw_line(ctx,GPoint(bx+3,y),GPoint(bx+7,y+3));
    graphics_draw_line(ctx,GPoint(bx+7,y+3),GPoint(bx,y+8));
    graphics_draw_line(ctx,GPoint(bx+3,y+10),GPoint(bx+7,y+7));
    graphics_draw_line(ctx,GPoint(bx+7,y+7),GPoint(bx,y+2));
    graphics_draw_line(ctx,GPoint(bx-1,y+10),GPoint(bx+8,y));
  }
}
static void prv_draw_date(GContext *ctx,WatchPose p) {
  const float split=watch_date_split(p.context);
  const float vertical=watch_clamp(split*2);
  // Separate the baselines before converging the two text runs horizontally.
  const float lateral=watch_smooth((split-0.25f)/0.75f);
  const int inline_point=20;
  const float whole_width=prv_measure(s_compact_date_buffer,inline_point).w;
  const float week_width=prv_measure(s_weekday_buffer,inline_point).w;
  const float date_width=prv_measure(s_numeric_date_buffer,inline_point).w;
  const float week_center=p.date_x+(week_width-whole_width)/2*(1-lateral);
  const float date_center=p.date_x+(whole_width-date_width)/2*(1-lateral);
  const int week_point=prv_point(prv_round(watch_lerp(inline_point,14,vertical)));
  const int date_point=prv_point(prv_round(watch_lerp(inline_point,20,vertical)));
  prv_text(ctx,s_weekday_buffer,week_point,prv_round(week_center-38),
      prv_round(p.date_y-5*vertical),76,s_text_color,GTextAlignmentCenter);
  prv_text(ctx,s_numeric_date_buffer,date_point,prv_round(date_center-38),
      prv_round(p.date_y+16*vertical),76,s_text_color,GTextAlignmentCenter);
  if(vertical<0.99f) {
    const float center=p.date_x+(week_width-date_width)/2*(1-lateral);
    prv_text(ctx,"·",inline_point,prv_round(center-38),prv_round(p.date_y),76,
        prv_faded_text(1-vertical),GTextAlignmentCenter);
  }
}
static void prv_draw_background(Layer *layer,GContext *ctx) {
  graphics_context_set_fill_color(ctx,s_background_color);
  graphics_fill_rect(ctx,layer_get_bounds(layer),0,GCornerNone);
  graphics_context_set_antialiased(ctx,true);
  const WatchPose p=prv_current_pose();
  float point=p.clock_size,width=prv_vector_width(s_time_buffer,point);
  const float available=watch_lerp(184,100,watch_clamp(p.context));
  if(width>available){point*=available/width;width=available;}
  prv_vector(ctx,s_time_buffer,p.clock_x-width/2,p.clock_y,point,s_text_color);
  prv_draw_date(ctx,p);
  prv_draw_reading(ctx,0,p);prv_draw_reading(ctx,1,p);prv_draw_status(ctx,p);
}
static MusicTextMetrics prv_music_measure(void *context,const char *text,int point) {
  (void)context;
  return (MusicTextMetrics){prv_measure(text,point).w,body_text_ascent(text,point),
      body_text_descent(text,point)};
}
static int prv_music_top(void) { return 92; }
static void prv_music_cancel_timer(void) {
  if(s_title_timer){app_timer_cancel(s_title_timer);s_title_timer=NULL;}
}
static void prv_music_reset(void) {
  prv_music_cancel_timer();
  s_native->music_identity[0]='\0';s_native->music_offset=s_native->music_next=0;
  s_native->music_visible=false;
}
static void prv_music_continue(void *context) {
  (void)context;s_title_timer=NULL;
  if(!s_app_focused||!s_native->music_visible||!s_native->music_next)return;
  s_native->music_offset=s_native->music_next;s_native->music_next=0;
  prv_sync_layers();
}
static void prv_music_tile(GContext *ctx,int x,int y) {
  const GColor ink=prv_accent_color();
  graphics_context_set_stroke_color(ctx,ink);graphics_context_set_stroke_width(ctx,2);
  graphics_draw_round_rect(ctx,GRect(x,y,40,40),4);
  graphics_draw_line(ctx,GPoint(x+24,y+10),GPoint(x+24,y+27));
  graphics_draw_line(ctx,GPoint(x+24,y+10),GPoint(x+31,y+8));
  graphics_context_set_fill_color(ctx,ink);graphics_fill_circle(ctx,GPoint(x+20,y+28),4);
}
static void prv_draw_music(GContext *ctx,const ComplicationSlot *title,const ComplicationSlot *artist,
                            int x,int y,int top) {
  if(!title||!context_slot_fresh(title,time(NULL)))return;
  const bool has_artist=artist&&context_slot_fresh(artist,time(NULL))&&
      (artist->flags&COMPLICATION_FLAG_TEXT)&&artist->text_length;
  const bool bitmap=(title->flags&COMPLICATION_FLAG_BITMAP)!=0;
  const bool cover=(title->flags&COMPLICATION_FLAG_IMAGE)!=0;
  const int visible=bitmap?prv_bitmap_visible_bottom(title,40,cover):0;
  const bool active=s_native->music_visible&&strcmp(title->text,s_native->music_identity)==0;
  const size_t offset=active?s_native->music_offset:0;
  MusicLayout layout;
  music_layout_build_at(title->text+offset,has_artist?artist->text:"",true,
      top,prv_music_measure,NULL,&layout);
  if(s_app_focused&&active&&!layout.title.complete&&layout.title.consumed) {
    size_t next=offset+layout.title.consumed;
    while(music_space(title->text[next]))++next;
    if(title->text[next]) {
      s_native->music_next=(uint16_t)next;
      if(!s_title_timer)s_title_timer=app_timer_register(3000,prv_music_continue,NULL);
    }
  }
  for(int i=0;i<layout.title.count;++i)
    prv_text(ctx,layout.title.text[i],layout.title_point,x+10,y+layout.title_baseline[i],
        180,s_text_color,GTextAlignmentLeft);
  if(visible)prv_draw_bitmap(ctx,title,x+10,y+MUSIC_BOTTOM-visible,40,s_text_color,cover);
  else prv_music_tile(ctx,x+10,y+MUSIC_BOTTOM-40);
  if(s_music_paused) {
    graphics_context_set_fill_color(ctx,s_background_color);
    graphics_fill_rect(ctx,GRect(x+34,y+MUSIC_BOTTOM-15,16,15),2,GCornersAll);
    graphics_context_set_fill_color(ctx,s_text_color);
    graphics_fill_rect(ctx,GRect(x+37,y+MUSIC_BOTTOM-12,3,9),0,GCornerNone);
    graphics_fill_rect(ctx,GRect(x+43,y+MUSIC_BOTTOM-12,3,9),0,GCornerNone);
  }
  const int left=60;
  for(int i=0;i<layout.artist.count;++i)
    prv_text(ctx,layout.artist.text[i],layout.artist_point,x+left,y+layout.artist_baseline[i],
        190-left,prv_muted_text(1),GTextAlignmentLeft);
}
typedef struct {
  char text[COMPLICATION_MAX_TEXT_BYTES+5];
  int point,baseline,bottom,app_baseline;
  bool app_line;
} NoticeLine;
static int prv_notice_visible_bottom(const ComplicationSlot *title,int size) {
  if(!(title->flags&COMPLICATION_FLAG_IMAGE))size=size==24?18:12;
  return (title->flags&COMPLICATION_FLAG_BITMAP)?prv_bitmap_visible_bottom(title,size,false):0;
}
static bool prv_notice_app_line(const ComplicationSlot *title,bool banner) {
  return !prv_notice_visible_bottom(title,banner?16:24)&&title->label[0]&&strcmp(title->label,title->text)!=0;
}
static int prv_notice_music_top(const ComplicationSlot *title) {
  return prv_notice_app_line(title,true)?130:116;
}
static void prv_notice_line(const ComplicationSlot *title,bool banner,int bottom,NoticeLine *line) {
  memset(line,0,sizeof(*line));
  watch_layout_copy_utf8(title->text,line->text,sizeof(line->text));
  line->point=banner?16:20;
  const int width=banner?155:147;
  // App and sender are separate source fields. Long app names never force
  // the sender into a smaller face; only the sender's last word truncates.
  prv_ellipsize(line->text,sizeof(line->text),line->point,width);
  line->app_line=prv_notice_app_line(title,banner);
  if(banner) {
    const int target=bottom+(line->app_line&&bottom<MUSIC_BOTTOM?14:0);
    line->baseline=target-body_text_descent(line->text,line->point);
  } else line->baseline=line->app_line?124:120;
  line->bottom=line->baseline+body_text_descent(line->text,line->point);
  line->app_baseline=line->app_line?(banner?line->baseline-18:102):0;
}
static void prv_notice_app(GContext *ctx,const ComplicationSlot *title,const NoticeLine *line,
                             int x,int y,int left,int width) {
  if(!line->app_line)return;
  char app[COMPLICATION_MAX_LABEL_BYTES+5];
  watch_layout_copy_utf8(title->label,app,sizeof(app));
  prv_ellipsize(app,sizeof(app),14,width);
  prv_text(ctx,app,14,x+left,y+line->app_baseline,width,prv_muted_text(1),GTextAlignmentLeft);
}
static void prv_notice_icon(GContext *ctx,const ComplicationSlot *title,int x,int y,int bottom,int size) {
  const int visible=prv_notice_visible_bottom(title,size);
  const bool bitmap=visible>0;
  if(bitmap&&(title->flags&COMPLICATION_FLAG_IMAGE)) {
    prv_draw_bitmap(ctx,title,x+10,y+bottom-visible,size,s_text_color,false);
  } else {
    const int radius=size/2,cx=x+10+radius,cy=y+bottom-radius;
    const GColor ink=prv_contrast_ink(prv_accent_color());
    graphics_context_set_fill_color(ctx,prv_accent_color());graphics_fill_circle(ctx,GPoint(cx,cy),radius);
    const int symbol=size==24?18:12;
    if(bitmap)prv_draw_bitmap(ctx,title,cx-symbol/2,cy-symbol/2,symbol,ink,false);
    else {
      graphics_context_set_stroke_color(ctx,ink);graphics_context_set_stroke_width(ctx,1);
      graphics_draw_round_rect(ctx,GRect(cx-radius/2,cy-radius/2,radius,radius-1),2);
      graphics_draw_line(ctx,GPoint(cx-radius/4,cy+radius/2-1),GPoint(cx-radius/2,cy+radius/2+2));
    }
  }
}
static void prv_draw_notice_banner(GContext *ctx,const RenderContext *value,int x,int y,bool music) {
  if(!value->heading_only||!context_slot_fresh(&value->title,time(NULL)))return;
  NoticeLine line;prv_notice_line(&value->title,true,music?112:MUSIC_BOTTOM,&line);
  prv_notice_icon(ctx,&value->title,x,y,line.bottom,16);
  prv_notice_app(ctx,&value->title,&line,x,y,35,155);
  prv_text(ctx,line.text,line.point,x+35,y+line.baseline,155,s_text_color,GTextAlignmentLeft);
}
static void prv_draw_context_value(GContext *ctx,const RenderContext *value,int x,int y) {
  if(value->mode==CONTEXT_NONE || !context_slot_fresh(&value->title,time(NULL)))return;
  const ComplicationSlot *title=&value->title,*subtitle=&value->subtitle;
  const bool has_subtitle=context_slot_fresh(subtitle,time(NULL))&&
      (subtitle->flags&COMPLICATION_FLAG_TEXT)&&subtitle->text_length;
  if(value->mode==CONTEXT_MUSIC) {
    prv_draw_music(ctx,title,subtitle,x,y,prv_music_top());
  } else if(value->heading_only) {
    if(value->has_music)prv_draw_music(ctx,&value->music_title,&value->music_artist,x,y,prv_notice_music_top(title));
    prv_draw_notice_banner(ctx,value,x,y,value->has_music);
  } else {
    NoticeLine heading;prv_notice_line(title,false,0,&heading);
    prv_notice_icon(ctx,title,x,y,heading.bottom,24);
    prv_notice_app(ctx,title,&heading,x,y,43,147);
    prv_text(ctx,heading.text,heading.point,x+43,y+heading.baseline,147,s_text_color,GTextAlignmentLeft);
    if(has_subtitle) {
      const int top=heading.bottom+8>132?heading.bottom+8:132;
      const int point=26;int offsets[MUSIC_MAX_LINES];MusicLines lines;
      music_wrap(subtitle->text,point,180,3,false,prv_music_measure,NULL,&lines);
      for(int count=2;top+music_line_offsets(&lines,point,prv_music_measure,NULL,offsets)>MUSIC_BOTTOM&&count>=1;--count)
        music_wrap(subtitle->text,point,180,count,false,prv_music_measure,NULL,&lines);
      music_line_offsets(&lines,point,prv_music_measure,NULL,offsets);
      for(int i=0;i<lines.count;++i)prv_text(ctx,lines.text[i],point,x+10,y+top+offsets[i],
          180,s_text_color,GTextAlignmentLeft);
    }
  }
}
static void prv_draw_context(Layer *layer,GContext *ctx) {
  (void)layer;prv_redact_context(&s_context_from);prv_redact_context(&s_context_to);
  const WatchPose p=prv_current_pose();
  const float reveal=watch_smooth((watch_context_visibility(p)-0.72f)/0.28f);
  const int y=-86+prv_round(22*(1-reveal)+65*watch_clamp(p.peek));
  if(s_slide_active&&s_banner_slide) {
    const ComplicationSlot *music=render_context_music(&s_context_to,false);
    const ComplicationSlot *artist=render_context_music(&s_context_to,true);
    prv_draw_music(ctx,music,artist,0,y,s_context_to.heading_only?prv_notice_music_top(&s_context_to.title):prv_music_top());
    prv_draw_notice_banner(ctx,&s_context_from,prv_round(-s_slide_direction*200*s_slide_amount),y,true);
    prv_draw_notice_banner(ctx,&s_context_to,prv_round(s_slide_direction*200*(1-s_slide_amount)),y,true);
  } else if(s_slide_active) {
    prv_draw_context_value(ctx,&s_context_from,prv_round(-s_slide_direction*200*s_slide_amount),y);
    prv_draw_context_value(ctx,&s_context_to,prv_round(s_slide_direction*200*(1-s_slide_amount)),y);
  } else prv_draw_context_value(ctx,&s_context_to,0,y);
}
static void prv_update_clock(void) {
  const time_t now=time(NULL);struct tm *current=localtime(&now);if(!current)return;
  s_use_12_hour=watch_clock_uses_12(s_clock_format,clock_is_24h_style());
  const char *day=prv_get_localized_days()[current->tm_wday];
  strftime(s_time_buffer,sizeof(s_time_buffer),s_use_12_hour?"%I:%M":"%H:%M",current);
  if(s_use_12_hour&&s_time_buffer[0]=='0')memmove(s_time_buffer,s_time_buffer+1,strlen(s_time_buffer));
  watch_layout_abbreviate(day,s_weekday_buffer,sizeof(s_weekday_buffer),3);
  snprintf(s_numeric_date_buffer,sizeof(s_numeric_date_buffer),"%d.%d.",current->tm_mday,current->tm_mon+1);
  snprintf(s_compact_date_buffer,sizeof(s_compact_date_buffer),"%s · %s",s_weekday_buffer,s_numeric_date_buffer);
  prv_apply_unobstructed_layout();
}

// A settings preview asks for the OS clock style, independently of this
// face's explicit override. If the normal request owns the outbox, its packet
// already carries the style; otherwise finish the query after that callback.
static void prv_send_clock_format(void) {
  if(!s_format_reply_pending)return;
  DictionaryIterator *iterator;
  if(app_message_outbox_begin(&iterator)!=APP_MSG_OK)return;
  dict_write_uint8(iterator,MESSAGE_KEY_CLOCK_FORMAT,clock_is_24h_style()?24:12);
  s_outbox_format_only=true;s_format_reply_pending=false;
  if(app_message_outbox_send()!=APP_MSG_OK)s_outbox_format_only=false;
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
  dict_write_uint8(iterator, MESSAGE_KEY_CLOCK_FORMAT, clock_is_24h_style()?24:12);
  s_outbox_format_only=false;
  const AppMessageResult sent = app_message_outbox_send();
  if (sent != APP_MSG_OK) {
    wfc_request_failed(&s_core);
    APP_LOG(APP_LOG_LEVEL_WARNING, "Complication request failed: %d", sent);
  } else { s_format_reply_pending=false;wfc_request_accepted(&s_core, time(NULL)); }
}

static void prv_startup_request(void *context) {
  s_startup_timer = NULL;
  prv_request_complications();
}

static void prv_phone_connection_changed(bool connected) {
  if (!wfc_connection_changed(&s_core, connected)) { return; }
  prv_private_clear();
  s_seen_burst=0;
  s_private_rebaseline=false;
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
  prv_apply_colors();
  // One request on the freshness transition; denied/unavailable slots do not
  // cause a polling loop. Provider refresh policies belong to the phone host.
  if (request_due) {
    prv_request_complications();
  }
  prv_schedule_context_expiry();
}


static void prv_unobstructed_area_will_change(GRect final_area,void *context) {
  (void)context;s_peek_transition=true;
  const int current_height=layer_get_unobstructed_bounds(s_window_layer).size.h;
  prv_cancel_motion();
  prv_music_reset();
  if(final_area.size.h>=current_height) s_pose=s_pose_target=prv_full_pose();
  prv_sync_layers();
}
static void prv_unobstructed_area_change(AnimationProgress progress,void *context) {
  (void)progress;(void)context;prv_sync_layers();
}
static void prv_unobstructed_area_did_change(void *context) {
  (void)context;s_peek_transition=false;s_pose=s_pose_target=prv_full_pose();prv_apply_unobstructed_layout();
}
static void prv_apply_colors(void) {
  const GColor *palette=s_night_enabled&&quiet_time_is_active()?s_night_colors:s_day_colors;
  s_background_color=palette[0];s_text_color=palette[1];s_accent_color=palette[2];
  if(s_window)window_set_background_color(s_window,s_background_color);
  prv_sync_layers();
}
static void prv_focus_will_change(bool focused) {
  if(!focused){s_app_focused=false;prv_music_cancel_timer();}
}
static void prv_focus_did_change(bool focused) {
  if(focused){s_app_focused=true;prv_sync_layers();}
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
    // A new phone process starts a fresh event sequence and native session.
    wfc_context_query(&s_core);
    persist_write_int(PERSIST_KEY_CONTEXT_SESSION,(int32_t)s_core.session_token);
    s_seen_burst=0;
    prv_private_clear();s_private_rebaseline=true;
    if (s_startup_timer != NULL) { app_timer_cancel(s_startup_timer); }
    s_startup_timer = app_timer_register(100, prv_startup_request, NULL);
  }
  if (context_data != NULL) {
    const bool bytes = context_data->type == TUPLE_BYTE_ARRAY;
    const bool accepted=wfc_receive_context(&s_core, bytes ? context_data->value->data : NULL,
        bytes ? context_data->length : 0, marker_value);
    if(accepted && marker_value==s_core.session_token)s_private_rebaseline=false;
    if(!accepted)prv_private_clear();
    ContextEffects effects;
    const Tuple *state=dict_find(iterator,MESSAGE_KEY_CONTEXT_STATE);
    const bool valid=accepted&&state&&state->type==TUPLE_BYTE_ARRAY&&
        context_effects_read(state->value->data,state->length,&effects)&&
        effects.session==s_core.session_token;
    s_music_paused=valid&&effects.paused&&context_slot_fresh(&s_context.slots[0],time(NULL));
    if(valid&&!s_core.context_awaiting_baseline&&!s_private_rebaseline&&
        context_slot_fresh(&s_context.slots[2],time(NULL))&&
        context_effects_alert(effects.burst,&s_seen_burst,s_vibration,quiet_time_is_active())) {
      if(s_vibration==1)vibes_short_pulse();else vibes_double_pulse();
    }
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
  Tuple *twelve = dict_find(iterator, MESSAGE_KEY_USE_12_HOUR);
  Tuple *clock_format = dict_find(iterator, MESSAGE_KEY_CLOCK_FORMAT);
  Tuple *show_status = dict_find(iterator, MESSAGE_KEY_SHOW_STATUS);
  Tuple *animations = dict_find(iterator, MESSAGE_KEY_ANIMATIONS);
  uint32_t rgb,flag;
  if(native_setting_value(background,&rgb)){s_day_colors[0]=GColorFromHEX(rgb);persist_write_int(PERSIST_KEY_BACKGROUND_COLOR,(int32_t)rgb);}
  if(native_setting_value(text,&rgb)){s_day_colors[1]=GColorFromHEX(rgb);persist_write_int(PERSIST_KEY_TEXT_COLOR,(int32_t)rgb);}
  uint32_t accent_value;
  if (native_setting_value(accent,&accent_value)) {
    s_day_colors[2] = GColorFromHEX(accent_value);
    persist_write_int(PERSIST_KEY_ACCENT_COLOR,(int32_t)accent_value);
  }
  uint32_t format;
  if(native_setting_value(clock_format,&format)&&format==WATCH_CLOCK_QUERY) {
    s_format_reply_pending=true;prv_send_clock_format();
  } else if(native_setting_value(clock_format,&format)&&
      (format==WATCH_CLOCK_FOLLOW||format==WATCH_CLOCK_12||format==WATCH_CLOCK_24)) {
    s_clock_format=(uint8_t)format;persist_write_int(PERSIST_KEY_CLOCK_FORMAT,s_clock_format);
  } else if (native_setting_value(twelve,&flag)&&flag<=1) {
    s_use_12_hour = flag != 0;
    persist_write_bool(PERSIST_KEY_USE_12_HOUR, s_use_12_hour);
    s_clock_format=s_use_12_hour?WATCH_CLOCK_12:WATCH_CLOCK_24;
    persist_write_int(PERSIST_KEY_CLOCK_FORMAT,s_clock_format);
  }
  if (native_setting_value(show_status,&flag)&&flag<=1) {
    s_show_status = flag != 0;
    persist_write_bool(PERSIST_KEY_SHOW_STATUS, s_show_status);
  }
  if(native_setting_value(animations,&flag)&&flag<=1){s_animations_enabled=flag!=0;persist_write_bool(PERSIST_KEY_ANIMATIONS,s_animations_enabled);}
  if(native_setting_value(dict_find(iterator,MESSAGE_KEY_NOTIFICATION_VIBRATION),&flag)&&flag<=2) {
    s_vibration=(uint8_t)flag;persist_write_int(PERSIST_KEY_VIBRATION,(int32_t)flag);
  }
  if(native_setting_value(dict_find(iterator,MESSAGE_KEY_NIGHT_PALETTE),&flag)&&flag<=1) {
    s_night_enabled=flag!=0;persist_write_bool(PERSIST_KEY_NIGHT_ENABLED,s_night_enabled);
  }
  const uint32_t night_keys[]={MESSAGE_KEY_NIGHT_BACKGROUND_COLOR,MESSAGE_KEY_NIGHT_TEXT_COLOR,MESSAGE_KEY_NIGHT_ACCENT_COLOR};
  for(unsigned i=0;i<3;++i)if(native_setting_value(dict_find(iterator,night_keys[i]),&rgb)) {
    s_night_colors[i]=GColorFromHEX(rgb);persist_write_int(PERSIST_KEY_NIGHT_BACKGROUND+i,(int32_t)rgb);
  }
  prv_update_clock();
  prv_apply_colors();
}

static void prv_inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Complication message dropped: %d", reason);
}
static void prv_outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  if(s_outbox_format_only){s_outbox_format_only=false;return;}
  wfc_request_failed(&s_core);
  APP_LOG(APP_LOG_LEVEL_WARNING, "Complication request failed: %d", reason);
  prv_send_clock_format();
}
static void prv_outbox_sent(DictionaryIterator *iterator, void *context) {
  if(s_outbox_format_only)s_outbox_format_only=false;
  else wfc_request_acknowledged(&s_core);
  prv_send_clock_format();
}


static void prv_window_load(Window *window) {
  s_window_layer=window_get_root_layer(window);
  const GRect bounds=layer_get_bounds(s_window_layer);
  for(size_t i=0;i<BODY_FONT_COUNT;++i) {
    s_body_fonts[i]=fonts_load_custom_font(resource_get_handle(s_body_resources[i]));
    s_body_loaded[i]=s_body_fonts[i]!=NULL;
    if(!s_body_fonts[i])s_body_fonts[i]=fonts_get_system_font(s_body_points[i]>=20?
        FONT_KEY_GOTHIC_24_BOLD:FONT_KEY_GOTHIC_14_BOLD);
  }
  s_background_layer=layer_create(bounds);
  layer_set_update_proc(s_background_layer,prv_draw_background);
  layer_add_child(s_window_layer,s_background_layer);
  s_context_layer=layer_create(GRect(0,86,200,0));
  layer_set_clips(s_context_layer,true);layer_set_update_proc(s_context_layer,prv_draw_context);
  layer_add_child(s_window_layer,s_context_layer);
  prv_update_clock();
  const UnobstructedAreaHandlers handlers={.will_change=prv_unobstructed_area_will_change,
    .change=prv_unobstructed_area_change,.did_change=prv_unobstructed_area_did_change};
  unobstructed_area_service_subscribe(handlers,NULL);
}
static void prv_window_unload(Window *window) {
  (void)window;unobstructed_area_service_unsubscribe();prv_cancel_motion();prv_private_clear();
  layer_destroy(s_context_layer);layer_destroy(s_background_layer);
  s_context_layer=NULL;s_background_layer=NULL;s_window_layer=NULL;
  for(size_t i=0;i<BODY_FONT_COUNT;++i) {
    if(s_body_loaded[i])fonts_unload_custom_font(s_body_fonts[i]);
    s_body_fonts[i]=NULL;s_body_loaded[i]=false;
  }
}
static bool prv_init(void) {
  s_native=calloc(1,sizeof(*s_native));
  if(!s_native)return false;
  s_clock_path.points=s_clock_path_points;
  s_background_color=GColorWhite;s_text_color=GColorBlack;s_accent_color=GColorFromHEX(0x55AAFF);
  s_animations_enabled=true;s_show_status=true;
  const WfcConfig core_config={.storage={.size=prv_storage_size,.read=prv_storage_read,
    .write=prv_storage_write,.remove=prv_storage_remove},.cache_first_key=PERSIST_KEY_COMPLICATION_FIRST};
  wfc_init(&s_core,&core_config,connection_service_peek_pebble_app_connection(),
      persist_exists(PERSIST_KEY_CONTEXT_SESSION)?(uint32_t)persist_read_int(PERSIST_KEY_CONTEXT_SESSION):0);
  persist_write_int(PERSIST_KEY_CONTEXT_SESSION,(int32_t)s_core.session_token);
  s_battery=battery_state_service_peek();
  if(persist_exists(PERSIST_KEY_BACKGROUND_COLOR))s_background_color=GColorFromHEX(persist_read_int(PERSIST_KEY_BACKGROUND_COLOR));
  if(persist_exists(PERSIST_KEY_TEXT_COLOR))s_text_color=GColorFromHEX(persist_read_int(PERSIST_KEY_TEXT_COLOR));
  if(persist_exists(PERSIST_KEY_ACCENT_COLOR)) {
    const int32_t color=persist_read_int(PERSIST_KEY_ACCENT_COLOR);
    if(color>=0&&color<=0xffffff)s_accent_color=GColorFromHEX(color);
  }
  s_day_colors[0]=s_background_color;s_day_colors[1]=s_text_color;s_day_colors[2]=s_accent_color;
  s_night_colors[0]=GColorBlack;s_night_colors[1]=GColorWhite;s_night_colors[2]=GColorFromHEX(0x55AAFF);
  for(unsigned i=0;i<3;++i)if(persist_exists(PERSIST_KEY_NIGHT_BACKGROUND+i)) {
    const int32_t color=persist_read_int(PERSIST_KEY_NIGHT_BACKGROUND+i);
    if(color>=0&&color<=0xffffff)s_night_colors[i]=GColorFromHEX(color);
  }
  if(persist_exists(PERSIST_KEY_NIGHT_ENABLED))s_night_enabled=persist_read_bool(PERSIST_KEY_NIGHT_ENABLED);
  if(persist_exists(PERSIST_KEY_VIBRATION)) {
    const int32_t preference=persist_read_int(PERSIST_KEY_VIBRATION);
    if(preference>=0&&preference<=2)s_vibration=(uint8_t)preference;
  }
  // The retired shadow effect had no rendering path. Keep its wire key
  // reserved and remove stale state rather than accepting unused settings.
  persist_delete(PERSIST_KEY_TEXT_SHADOW);
  // Retire the old size preference; legacy live packets are also ignored.
  persist_delete(PERSIST_KEY_LARGE_TEXT);
  s_clock_format=watch_clock_format(persist_exists(PERSIST_KEY_CLOCK_FORMAT)?persist_read_int(PERSIST_KEY_CLOCK_FORMAT):-1,
      persist_exists(PERSIST_KEY_USE_12_HOUR)?persist_read_bool(PERSIST_KEY_USE_12_HOUR):-1);
  if(persist_exists(PERSIST_KEY_SHOW_STATUS))s_show_status=persist_read_bool(PERSIST_KEY_SHOW_STATUS);
  if(persist_exists(PERSIST_KEY_ANIMATIONS))s_animations_enabled=persist_read_bool(PERSIST_KEY_ANIMATIONS);
  persist_delete(PERSIST_KEY_LAYOUT_STYLE);wfc_load_cache(&s_core,time(NULL));
  prv_apply_colors();
  s_window=window_create();
  if(!s_window){memset(s_native,0,sizeof(*s_native));free(s_native);s_native=NULL;return false;}
  window_set_background_color(s_window,s_background_color);
  window_set_window_handlers(s_window,(WindowHandlers){.load=prv_window_load,.unload=prv_window_unload});
  app_message_register_inbox_received(prv_inbox_received);app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);app_message_register_outbox_sent(prv_outbox_sent);
  app_focus_service_subscribe_handlers((AppFocusHandlers){.will_focus=prv_focus_will_change,
      .did_focus=prv_focus_did_change});
  // One v4 snapshot is bounded at5206bytes, plus dictionary/setting overhead.
  app_message_open(NATIVE_INBOX_BYTES,32);tick_timer_service_subscribe(MINUTE_UNIT,prv_tick_handler);
  window_stack_push(s_window,false);
  connection_service_subscribe((ConnectionHandlers){.pebble_app_connection_handler=prv_phone_connection_changed});
  battery_state_service_subscribe(prv_battery_changed);
  s_startup_timer=app_timer_register(1000,prv_startup_request,NULL);
  return true;
}
static void prv_deinit(void) {
  if(!s_native)return;
  if(s_startup_timer)app_timer_cancel(s_startup_timer);
  if(s_context_timer)app_timer_cancel(s_context_timer);
  prv_cancel_motion();prv_private_clear();memset(&s_context,0,sizeof(s_context));
  battery_state_service_unsubscribe();connection_service_unsubscribe();tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
  app_message_deregister_callbacks();window_destroy(s_window);s_window=NULL;
  memset(s_native,0,sizeof(*s_native));free(s_native);s_native=NULL;
}
int main(void) {if(!prv_init())return 0;app_event_loop();prv_deinit();return 0;}
