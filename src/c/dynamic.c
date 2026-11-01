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
#define FULL_LAYOUT_HEIGHT 228
#define MOTION_FRAME_MS 50
#define LAYOUT_DURATION_MS 520

static Window *s_window;
static Layer *s_window_layer, *s_background_layer, *s_context_layer;
static GColor s_background_color, s_text_color, s_accent_color;
static bool s_text_shadow_enabled, s_large_text_enabled, s_use_12_hour;
static bool s_show_status, s_animations_enabled;
static BatteryChargeState s_battery;
static WatchfaceCore s_core;
#define s_context s_core.context
#define s_complications s_core.complications
#define s_has_complications s_core.has_complications
#define s_phone_connected s_core.phone_connected
static AppTimer *s_context_timer, *s_startup_timer, *s_motion_timer;
static char s_date_buffer[48], s_compact_date_buffer[48], s_time_buffer[6];
static char s_weekday_buffer[16], s_numeric_date_buffer[16];
static const uint8_t s_body_points[] = {11,14,16,20,24,26,28,32};
#define BODY_FONT_COUNT (sizeof(s_body_points)/sizeof(s_body_points[0]))
static GFont s_body_fonts[BODY_FONT_COUNT];
static bool s_body_loaded[BODY_FONT_COUNT];
static const uint32_t s_body_resources[BODY_FONT_COUNT] = {RESOURCE_ID_FONT_BODY_11,RESOURCE_ID_FONT_BODY_14,
  RESOURCE_ID_FONT_BODY_16,RESOURCE_ID_FONT_BODY_20,RESOURCE_ID_FONT_BODY_24,
  RESOURCE_ID_FONT_BODY_26,RESOURCE_ID_FONT_BODY_28,RESOURCE_ID_FONT_BODY_32};
static GPoint s_clock_path_points[CLOCK_CONTOUR_MAX_POINTS];
static GPath s_clock_path = {.points=s_clock_path_points};
static FontRasterPoint s_clock_raster_points[CLOCK_GLYPH_MAX_POINTS];
static FontRasterContour s_clock_raster_contours[CLOCK_GLYPH_MAX_CONTOURS];
static int32_t s_clock_intersections[CLOCK_GLYPH_MAX_POINTS];
static uint8_t s_clock_coverage[200];
static WatchPose s_pose, s_pose_from, s_pose_target;
static bool s_pose_ready, s_layout_active, s_slide_active, s_peek_transition;
static uint32_t s_layout_started, s_slide_started, s_slide_duration;
static float s_slide_amount = 1;
static int s_slide_direction = 1;
static bool s_private_rebaseline;

typedef struct {
  ContextMode mode;
  ComplicationSlot title, subtitle;
} RenderContext;
static RenderContext s_context_from, s_context_to;
static void prv_apply_unobstructed_layout(void);
static void prv_sync_layers(void);
static void prv_schedule_context_expiry(void);
static void prv_context_expired(void *context);


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
  const unsigned r=(color.argb>>4)&3,g=(color.argb>>2)&3,b=color.argb&3;
  return 299*r+587*g+114*b>=1500 ? GColorBlack : GColorWhite;
}
static GColor prv_muted_text(float alpha) {
  const unsigned fr=(s_text_color.argb>>4)&3,fg=(s_text_color.argb>>2)&3,fb=s_text_color.argb&3;
  const unsigned br=(s_background_color.argb>>4)&3,bg=(s_background_color.argb>>2)&3,bb=s_background_color.argb&3;
  alpha=watch_clamp(alpha);
  const unsigned r=prv_round(watch_lerp(br,(2*fr+br+1)/3,alpha));
  const unsigned g=prv_round(watch_lerp(bg,(2*fg+bg+1)/3,alpha));
  const unsigned b=prv_round(watch_lerp(bb,(2*fb+bb+1)/3,alpha));
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
  return context_mode(&s_context,time(NULL))==CONTEXT_NONE || s_private_rebaseline ?
      watch_pose_idle(s_large_text_enabled) : watch_pose_context(s_large_text_enabled,false);
}
static WatchPose prv_current_pose(void) {
  const int height=s_window_layer ? layer_get_unobstructed_bounds(s_window_layer).size.h : 228;
  return watch_pose_at_height(s_pose,s_large_text_enabled,height);
}
static bool prv_slot_visual_equal(const ComplicationSlot *a,const ComplicationSlot *b) {
  return a->flags==b->flags && a->status==b->status && a->width==b->width &&
      a->height==b->height && a->stride==b->stride && a->text_length==b->text_length &&
      a->pixel_bytes==b->pixel_bytes && a->display_kind==b->display_kind &&
      memcmp(a->text,b->text,a->text_length)==0 &&
      (!(a->flags&COMPLICATION_FLAG_BITMAP) ||
       (memcmp(a->palette,b->palette,COMPLICATION_PALETTE_BYTES)==0 &&
        memcmp(a->bitmap,b->bitmap,a->pixel_bytes)==0));
}
static bool prv_context_equal(const RenderContext *a,const RenderContext *b) {
  return a->mode==b->mode && (a->mode==CONTEXT_NONE ||
      (prv_slot_visual_equal(&a->title,&b->title) && prv_slot_visual_equal(&a->subtitle,&b->subtitle)));
}
static RenderContext prv_capture_context(void) {
  RenderContext result={0};
  if(!s_phone_connected || s_private_rebaseline) return result;
  result.mode=context_mode(&s_context,time(NULL));
  if(result.mode!=CONTEXT_NONE) {
    unsigned i=result.mode==CONTEXT_NOTIFICATION ? 2 : 0;
    result.title=s_context.slots[i];
    if(context_slot_fresh(&s_context.slots[i+1],time(NULL))) result.subtitle=s_context.slots[i+1];
  }
  return result;
}
// Copies used during a short slide retain their original lease. Permission,
// reconnect and preview clearing always outrank the visual transition.
static void prv_redact_context(RenderContext *value) {
  if(value->mode==CONTEXT_NONE) return;
  const unsigned i=value->mode==CONTEXT_NOTIFICATION ? 2 : 0;
  const ComplicationSlot *live=&s_context.slots[i];
  if(!s_phone_connected || s_private_rebaseline ||
      !context_slot_fresh(&value->title,time(NULL)) || !context_slot_fresh(live,time(NULL)) ||
      (i==2 && !prv_slot_visual_equal(&value->title,live))) {
    memset(value,0,sizeof(*value)); return;
  }
  if(!context_slot_fresh(&value->subtitle,time(NULL)) ||
      !context_slot_fresh(&s_context.slots[i+1],time(NULL)) ||
      !(s_context.slots[i+1].flags&COMPLICATION_FLAG_TEXT) ||
      s_context.slots[i+1].text_length==0) memset(&value->subtitle,0,sizeof(value->subtitle));
}
static void prv_private_clear(void) {
  memset(&s_context_from,0,sizeof(s_context_from));
  memset(&s_context_to,0,sizeof(s_context_to));
  s_slide_active=false;s_slide_amount=1;
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
  RenderContext next=prv_capture_context();
  const ContextMode previous=s_context_to.mode;
  prv_redact_context(&s_context_to);prv_redact_context(&s_context_from);
  const bool motion=s_animations_enabled && !s_peek_transition &&
      layer_get_unobstructed_bounds(s_window_layer).size.h>=228;
  if(!prv_context_equal(&next,&s_context_to)) {
    s_context_from=s_context_to;s_context_to=next;
    prv_redact_context(&s_context_from);
    s_slide_direction=next.mode==CONTEXT_MUSIC && previous==CONTEXT_NOTIFICATION ? -1 : 1;
    s_slide_duration=next.mode==previous ? 280 : 340;
    s_slide_active=motion && previous!=CONTEXT_NONE && next.mode!=CONTEXT_NONE;
    s_slide_amount=s_slide_active ? 0 : 1;s_slide_started=prv_millis();
    if(!s_slide_active) memset(&s_context_from,0,sizeof(s_context_from));
  } else s_context_to=next; // Refreshing a lease does not replay a song animation.
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
  const float reveal=watch_smooth((p.context-0.72f)/0.28f);
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
static bool prv_temperature(const ReadingParts *parts) {
  return strcmp(parts->unit,"°C")==0 || strcmp(parts->unit,"°F")==0;
}
static const char *prv_reading_label(bool rain) {
  const char *locale=i18n_get_system_locale();
  static const struct {const char *lang,*now,*rain;} labels[]={
    {"cs","NYNÍ","DÉŠŤ"},{"sk","TERAZ","DÁŽĎ"},{"de","JETZT","REGEN"},
    {"fr","ACTUEL","PLUIE"},{"es","AHORA","LLUVIA"},{"it","ORA","PIOGGIA"},
    {"pt","AGORA","CHUVA"},{"nl","NU","REGEN"},{"pl","TERAZ","DESZCZ"},
    {"da","NU","REGN"},{"sv","NU","REGN"},{"no","NÅ","REGN"},
    {"fi","NYT","SADE"},{"hu","MOST","ESŐ"},{"ro","ACUM","PLOAIE"},
    {"tr","ŞİMDİ","YAĞMUR"}};
  for(unsigned i=0;i<sizeof(labels)/sizeof(labels[0]);++i)
    if(prv_locale_is(locale,labels[i].lang) || (strcmp(labels[i].lang,"no")==0 &&
       (prv_locale_is(locale,"nb")||prv_locale_is(locale,"nn")))) return rain?labels[i].rain:labels[i].now;
  return rain?"RAIN":"NOW";
}
static void prv_draw_symbol(GContext *ctx,const ComplicationSlot *slot,int cx,int cy,
                            int radius,int size,bool temp,int probability) {
  const bool bitmap=(slot->flags&COMPLICATION_FLAG_BITMAP)!=0;
  graphics_context_set_antialiased(ctx,true);
  GColor ink=s_text_color;
  if(probability>=0) {
    graphics_context_set_stroke_width(ctx,radius/4<2?2:radius/4);
    graphics_context_set_stroke_color(ctx,s_background_color.argb==GColorBlack.argb?GColorDarkGray:GColorLightGray);
    graphics_draw_circle(ctx,GPoint(cx,cy),radius);
    graphics_context_set_stroke_color(ctx,GColorChromeYellow);
    if(probability>0)graphics_draw_arc(ctx,GRect(cx-radius,cy-radius,2*radius,2*radius),
        GOvalScaleModeFitCircle,0,TRIG_MAX_ANGLE*probability/10000);
  } else if(temp || (bitmap&&!(slot->flags&COMPLICATION_FLAG_IMAGE))) {
    graphics_context_set_fill_color(ctx,s_accent_color);graphics_fill_circle(ctx,GPoint(cx,cy),radius);
    ink=prv_contrast_ink(s_accent_color);
  }
  if(bitmap)prv_draw_bitmap(ctx,slot,cx-size/2,cy-size/2,size,ink,false);
  else if(probability>=0)prv_draw_drop(ctx,cx,cy,size/2,s_text_color);
  else if(temp) {
    const int h=size/2,w=size/5<2?2:size/5;
    graphics_context_set_stroke_width(ctx,2);graphics_context_set_stroke_color(ctx,ink);
    graphics_draw_round_rect(ctx,GRect(cx-w/2,cy-h/2-2,w,h),w/2);
    graphics_context_set_fill_color(ctx,ink);graphics_fill_circle(ctx,GPoint(cx,cy+h/2-1),w);
    graphics_draw_line(ctx,GPoint(cx,cy-h/3),GPoint(cx,cy+h/2));
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
static void prv_draw_reading(GContext *ctx,unsigned index,WatchPose p) {
  if(!s_has_complications || !complication_is_selected(&s_complications.slots[index]))return;
  const ComplicationSlot *slot=&s_complications.slots[index];
  const bool fresh=complication_is_fresh(slot,time(NULL));
  const bool bitmap=fresh&&(slot->flags&COMPLICATION_FLAG_BITMAP);
  char text[COMPLICATION_MAX_TEXT_BYTES+5];
  bool has_text=prv_slot_text(slot,text,sizeof(text));
  if(!has_text&&!bitmap){snprintf(text,sizeof(text),"--");has_text=true;}
  ReadingParts parts;const bool numeric=fresh&&has_text&&reading_parts(text,&parts);
  const bool temp=numeric&&prv_temperature(&parts);
  const int probability=numeric&&slot->display_kind==COMPLICATION_DISPLAY_PRECIPITATION?
      prv_probability(&parts):-1;
  const bool symbol=bitmap||temp||probability>=0;
  const float center=index?150:50;
  const float inline_amount=watch_clamp(-p.icon_x/25);
  if(bitmap&&!has_text) {
    const int size=prv_round(watch_lerp(40,22,inline_amount));
    prv_draw_bitmap(ctx,slot,prv_round(center-size/2.0f),
        prv_round(p.row_y+16*(1-inline_amount)-size/2.0f),size,s_text_color,false);
    return;
  }
  if(symbol) {
    prv_draw_symbol(ctx,slot,prv_round(center+p.icon_x),prv_round(p.row_y+p.icon_y),
        prv_round(p.icon_radius),prv_round(p.icon_size),temp,probability);
  }
  const float value_center=center+(symbol?p.value_x:0);
  const float value_width=symbol?88-42*inline_amount:88;
  const float baseline=p.row_y+p.value_y;
  if(has_text) {
    bool drawn=numeric&&prv_draw_number(ctx,&parts,value_center,baseline,p.number_size,
        p.unit_size,value_width);
    if(!drawn && prv_vector_width(text,p.number_size)>0) {
      float point=p.number_size,width=prv_vector_width(text,point);
      if(width>value_width){point*=value_width/width;width=value_width;}
      if(point>=16){prv_vector(ctx,text,value_center-width/2,baseline,point,s_text_color);drawn=true;}
    }
    if(!drawn) {
      const int point=prv_point(p.context>0.5f?20:s_large_text_enabled?28:24);
      prv_ellipsize(text,sizeof(text),point,prv_round(value_width));
      prv_text(ctx,text,point,prv_round(value_center-value_width/2),prv_round(baseline),
          prv_round(value_width),s_text_color,GTextAlignmentCenter);
    }
  }
  if(p.label_alpha>0.01f && (temp||probability>=0))
    prv_text(ctx,prv_reading_label(probability>=0),11,prv_round(center-44),
        prv_round(p.row_y+p.label_y),88,prv_muted_text(p.label_alpha),GTextAlignmentCenter);
}
static void prv_draw_status(GContext *ctx,WatchPose p) {
  if(!s_show_status)return;
  const int y=prv_round(watch_lerp(watch_lerp(115,45,watch_clamp(p.context)),104,p.peek));
  int x=5;graphics_context_set_stroke_color(ctx,s_text_color);
  graphics_context_set_fill_color(ctx,s_text_color);graphics_context_set_stroke_width(ctx,1);
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
  const int inline_point=s_large_text_enabled?20:16;
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
static void prv_draw_context_value(GContext *ctx,const RenderContext *value,int x,int y) {
  if(value->mode==CONTEXT_NONE || !context_slot_fresh(&value->title,time(NULL)))return;
  const ComplicationSlot *title=&value->title,*subtitle=&value->subtitle;
  const bool has_subtitle=context_slot_fresh(subtitle,time(NULL))&&
      (subtitle->flags&COMPLICATION_FLAG_TEXT)&&subtitle->text_length;
  const bool bitmap=(title->flags&COMPLICATION_FLAG_BITMAP)!=0;
  if(value->mode==CONTEXT_MUSIC) {
    const int artwork=40;
    const bool cover=(title->flags&COMPLICATION_FLAG_IMAGE)!=0;
    const int visible=bitmap?prv_bitmap_visible_bottom(title,artwork,cover):0;
    MusicLayout layout;
    music_layout_build(title->text,has_subtitle?subtitle->text:"",visible>0,s_large_text_enabled,
        prv_music_measure,NULL,&layout);
    for(int i=0;i<layout.title.count;++i) {
      prv_text(ctx,layout.title.text[i],layout.title_point,x+10,y+layout.title_baseline[i],
          180,s_text_color,GTextAlignmentLeft);
    }
    if(visible)prv_draw_bitmap(ctx,title,x+10,y+MUSIC_BOTTOM-visible,artwork,s_text_color,cover);
    const int left=visible?60:10;
    for(int i=0;i<layout.artist.count;++i) {
      prv_text(ctx,layout.artist.text[i],layout.artist_point,x+left,y+layout.artist_baseline[i],
          190-left,s_text_color,GTextAlignmentLeft);
    }
  } else {
    const int point=s_large_text_enabled?24:20;
    char heading[COMPLICATION_MAX_TEXT_BYTES+5];watch_layout_copy_utf8(title->text,heading,sizeof(heading));
    prv_ellipsize(heading,sizeof(heading),point,147);
    const int bottom=111+body_text_descent(heading,point);
    if(bitmap&&(title->flags&COMPLICATION_FLAG_IMAGE)) {
      const int visible=prv_bitmap_visible_bottom(title,24,false);
      if(visible)prv_draw_bitmap(ctx,title,x+10,y+bottom-visible,24,s_text_color,false);
    } else {
      const GColor ink=prv_contrast_ink(GColorChromeYellow);
      const int cy=bottom-12;
      graphics_context_set_fill_color(ctx,GColorChromeYellow);
      graphics_fill_circle(ctx,GPoint(x+22,y+cy),12);
      if(bitmap)prv_draw_bitmap(ctx,title,x+13,y+cy-9,18,ink,false);
      else {
        graphics_context_set_stroke_color(ctx,ink);graphics_context_set_stroke_width(ctx,2);
        graphics_draw_round_rect(ctx,GRect(x+15,y+cy-6,14,10),4);
        graphics_draw_line(ctx,GPoint(x+18,y+cy+4),GPoint(x+16,y+cy+7));
      }
    }
    prv_text(ctx,heading,point,x+43,y+111,147,s_text_color,GTextAlignmentLeft);
    if(has_subtitle) {
      const int body=s_large_text_enabled?32:28;
      graphics_context_set_text_color(ctx,s_text_color);
      graphics_draw_text(ctx,subtitle->text,prv_body(body),GRect(x+10,y+150-body,180,218-(150-body)),
          GTextOverflowModeTrailingEllipsis,GTextAlignmentLeft,NULL);
    }
  }
}
static void prv_draw_context(Layer *layer,GContext *ctx) {
  (void)layer;prv_redact_context(&s_context_from);prv_redact_context(&s_context_to);
  const WatchPose p=prv_current_pose();
  const float reveal=watch_smooth((p.context-0.72f)/0.28f);
  const int y=-86+prv_round(22*(1-reveal)+65*watch_clamp(p.peek));
  if(s_slide_active) {
    prv_draw_context_value(ctx,&s_context_from,prv_round(-s_slide_direction*200*s_slide_amount),y);
    prv_draw_context_value(ctx,&s_context_to,prv_round(s_slide_direction*200*(1-s_slide_amount)),y);
  } else prv_draw_context_value(ctx,&s_context_to,0,y);
}
static void prv_update_clock(void) {
  const time_t now=time(NULL);struct tm *current=localtime(&now);if(!current)return;
  const char *day=prv_get_localized_days()[current->tm_wday];
  strftime(s_time_buffer,sizeof(s_time_buffer),s_use_12_hour?"%I:%M":"%H:%M",current);
  if(s_use_12_hour&&s_time_buffer[0]=='0')memmove(s_time_buffer,s_time_buffer+1,strlen(s_time_buffer));
  snprintf(s_date_buffer,sizeof(s_date_buffer),"%s · %d.%d.",day,current->tm_mday,current->tm_mon+1);
  watch_layout_abbreviate(day,s_weekday_buffer,sizeof(s_weekday_buffer),3);
  snprintf(s_numeric_date_buffer,sizeof(s_numeric_date_buffer),"%d.%d.",current->tm_mday,current->tm_mon+1);
  snprintf(s_compact_date_buffer,sizeof(s_compact_date_buffer),"%s · %s",s_weekday_buffer,s_numeric_date_buffer);
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
  prv_private_clear();
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
  if(final_area.size.h>=current_height) s_pose=s_pose_target=prv_full_pose();
  prv_sync_layers();
}
static void prv_unobstructed_area_change(AnimationProgress progress,void *context) {
  (void)progress;(void)context;prv_sync_layers();
}
static void prv_unobstructed_area_did_change(void *context) {
  (void)context;s_peek_transition=false;s_pose=s_pose_target=prv_full_pose();prv_sync_layers();
}
static void prv_apply_colors(void) {
  if(s_window)window_set_background_color(s_window,s_background_color);
  prv_sync_layers();
}
static void prv_apply_effects(void) { prv_sync_layers(); }

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
  Tuple *animations = dict_find(iterator, MESSAGE_KEY_ANIMATIONS);
  uint32_t rgb;
  if(prv_color_value(background,&rgb)){s_background_color=GColorFromHEX(rgb);persist_write_int(PERSIST_KEY_BACKGROUND_COLOR,(int32_t)rgb);}
  if(prv_color_value(text,&rgb)){s_text_color=GColorFromHEX(rgb);persist_write_int(PERSIST_KEY_TEXT_COLOR,(int32_t)rgb);}
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
  uint32_t flag;
  if(prv_color_value(animations,&flag)&&flag<=1){s_animations_enabled=flag!=0;persist_write_bool(PERSIST_KEY_ANIMATIONS,s_animations_enabled);}
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
static void prv_init(void) {
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
  if(persist_exists(PERSIST_KEY_TEXT_SHADOW))s_text_shadow_enabled=persist_read_bool(PERSIST_KEY_TEXT_SHADOW);
  if(persist_exists(PERSIST_KEY_LARGE_TEXT))s_large_text_enabled=persist_read_bool(PERSIST_KEY_LARGE_TEXT);
  if(persist_exists(PERSIST_KEY_USE_12_HOUR))s_use_12_hour=persist_read_bool(PERSIST_KEY_USE_12_HOUR);
  if(persist_exists(PERSIST_KEY_SHOW_STATUS))s_show_status=persist_read_bool(PERSIST_KEY_SHOW_STATUS);
  if(persist_exists(PERSIST_KEY_ANIMATIONS))s_animations_enabled=persist_read_bool(PERSIST_KEY_ANIMATIONS);
  persist_delete(PERSIST_KEY_LAYOUT_STYLE);wfc_load_cache(&s_core,time(NULL));
  s_window=window_create();window_set_background_color(s_window,s_background_color);
  window_set_window_handlers(s_window,(WindowHandlers){.load=prv_window_load,.unload=prv_window_unload});
  app_message_register_inbox_received(prv_inbox_received);app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_failed(prv_outbox_failed);app_message_register_outbox_sent(prv_outbox_sent);
  app_message_open(2048,32);tick_timer_service_subscribe(MINUTE_UNIT,prv_tick_handler);
  window_stack_push(s_window,false);
  connection_service_subscribe((ConnectionHandlers){.pebble_app_connection_handler=prv_phone_connection_changed});
  battery_state_service_subscribe(prv_battery_changed);
  s_startup_timer=app_timer_register(1000,prv_startup_request,NULL);
}
static void prv_deinit(void) {
  if(s_startup_timer)app_timer_cancel(s_startup_timer);
  if(s_context_timer)app_timer_cancel(s_context_timer);
  prv_cancel_motion();prv_private_clear();memset(&s_context,0,sizeof(s_context));
  battery_state_service_unsubscribe();connection_service_unsubscribe();tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();window_destroy(s_window);
}
int main(void) {prv_init();app_event_loop();prv_deinit();}
