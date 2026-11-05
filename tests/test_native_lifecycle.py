"""Exercise actual native callbacks with counted SDK boundaries, without QEMU."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def callback(source, name):
    match = re.search(r'^static [^\n]+\b' + name + r'\([^)]*\)\s*\{', source, re.M)
    if not match:
        raise AssertionError('Missing production callback ' + name)
    begin = source.index('{', match.start())
    depth = 1
    end = begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


class NativeLifecycleTests(unittest.TestCase):
    def test_actual_callbacks_cancel_covered_work_and_reconcile_without_replaying_motion(self):
        source = (ROOT / 'src/c/dynamic.c').read_text()
        names = ['prv_dirty', 'prv_cancel_motion', 'prv_motion_frame', 'prv_schedule_motion',
                 'prv_apply_unobstructed_layout', 'prv_sync_layers',
                 'prv_activity_clock_tick', 'prv_context_expired',
                 'prv_focus_will_change', 'prv_focus_did_change',
                 'prv_draw_context', 'prv_update_clock']
        bodies = [callback(source, name) for name in names]
        declarations = '\n'.join(body[:body.index('{')].strip() + ';' for body in bodies)
        harness = r'''
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "watch_layout.h"
#include "activity_plan_cache.h"
#define PAV_PROVIDER_BYTES 37
#define PAV_ID_BYTES 65
#define PAV_PRESENTATION_MEDIA 2
#define MOTION_FRAME_MS 50
#define LAYOUT_DURATION_MS 520
typedef struct { int value; } Layer, AppTimer, GContext;
typedef struct { struct { int h; } size; } GRect;
#define GRect(x,y,w,h) ((GRect){{h}})
typedef struct { char provider[37],id[65];int presentation;uint8_t visual_token[8]; } PavCard;
typedef struct { ActivityPlanCache cache;struct { struct { int count; } main; } layout; } Plan;
typedef struct { Plan activity_plan;char activity_identity[105];uint8_t activity_visual[8]; } Native;
static Native storage,*s_native=&storage;
static Layer background,context,window;
static Layer *s_background_layer=&background,*s_context_layer=&context,*s_window_layer=&window;
static AppTimer timers[64],*s_motion_timer,*s_activity_timer,*s_presentation_timer,*s_context_timer;
static bool s_app_focused=true,s_animations_enabled=true,s_peek_transition,s_layout_active,s_slide_active,s_pose_ready;
static WatchPose s_pose,s_pose_target,s_pose_from;
static float s_slide_amount=1;
static uint32_t s_layout_started,s_slide_started,millis;
static PavCard card,*selected=&card;
static bool s_phone_connected=true,s_use_12_hour;
static uint8_t s_clock_format=24;
static char s_time_buffer[6],s_weekday_buffer[13],s_numeric_date_buffer[8],s_compact_date_buffer[24];
static const char english[]="en",czech[]="cs";
static const char *locale=english;
static time_t now=1700000000;
static unsigned dirty_background,dirty_context,registrations,cancellations,geometry,scheduled,prunes,draws,reports;
static bool fail_timer;
static time_t fake_time(time_t *out){if(out)*out=now;return now;}
#define time fake_time
static uint32_t prv_millis(void){return millis;}
static const PavCard *prv_primary_activity(void){return selected;}
static WatchPose prv_full_pose(void){return selected?watch_pose_context(false):watch_pose_idle();}
static WatchPose prv_current_pose(void){return s_pose;}
static int prv_round(float value){return (int)(value+0.5f);}
static GRect layer_get_unobstructed_bounds(Layer *layer){(void)layer;return GRect(0,0,200,228);}
static void layer_mark_dirty(Layer *layer){if(layer==&background)++dirty_background;else if(layer==&context)++dirty_context;}
static void layer_set_frame(Layer *layer,GRect rect){(void)layer;(void)rect;++geometry;}
static void layer_set_bounds(Layer *layer,GRect rect){(void)layer;(void)rect;++geometry;}
static void layer_set_hidden(Layer *layer,bool hidden){(void)layer;(void)hidden;++geometry;}
static AppTimer *app_timer_register(uint32_t ms,void(*fn)(void *),void *ctx){(void)ms;(void)fn;(void)ctx;++registrations;return fail_timer?NULL:&timers[registrations%64];}
static void app_timer_cancel(AppTimer *timer){assert(timer);++cancellations;}
static void prv_activity_clock_schedule(void){++scheduled;}
static void prv_schedule_context_expiry(void){++prunes;}
static void prv_apply_colors(void){}
static int s_activities;
static void pav_state_prune(int *state,uint32_t at){(void)state;(void)at;++prunes;}
static const char *i18n_get_system_locale(void){return locale;}
static const char *reading_language(const char *language){return language;}
static bool clock_is_24h_style(void){return true;}
static const char *prv_get_localized_day(const char *language,unsigned day){(void)language;(void)day;return "MON";}
static void prv_draw_activity(GContext *ctx,const PavCard *item,int y){(void)ctx;(void)item;(void)y;++draws;}
static void prv_note_presentation(const PavCard *item,bool stable){(void)item;(void)stable;++reports;}
'''
        harness += declarations + '\n' + '\n'.join(bodies)
        harness += r'''
static void reset_counts(void){dirty_background=dirty_context=registrations=cancellations=geometry=scheduled=prunes=draws=reports=0;}
int main(void){
  strcpy(card.provider,"provider");strcpy(card.id,"first");card.presentation=PAV_PRESENTATION_MEDIA;
  s_pose_ready=true;s_pose=s_pose_target=watch_pose_context(false);
  s_layout_active=s_slide_active=true;
  s_motion_timer=&timers[0];s_activity_timer=&timers[1];s_presentation_timer=&timers[2];
  prv_focus_will_change(false);
  assert(!s_app_focused&&!s_layout_active&&!s_slide_active);
  assert(!s_motion_timer&&!s_activity_timer&&!s_presentation_timer&&cancellations==3);
  reset_counts();
  prv_motion_frame(NULL);prv_activity_clock_tick(NULL);
  assert(!registrations&&!dirty_context&&!dirty_background&&!prunes);
  for(unsigned i=0;i<20;++i){++card.visual_token[0];prv_apply_unobstructed_layout();prv_dirty(&background);}
  assert(!registrations&&!dirty_context&&!dirty_background&&!geometry);
  assert(!s_slide_active&&!s_layout_active);
  prv_context_expired(NULL);assert(prunes==1&&!registrations&&!dirty_context&&!dirty_background);
  s_native->activity_plan.layout.main.count=1;
  prv_draw_context(&context,NULL);
  assert(draws==1&&!reports&&!s_native->activity_plan.cache.drawn);
  /* A cover does not manufacture a navigation withdrawal or another lease. */
  now+=61;prv_focus_did_change(true);
  assert(s_app_focused&&!s_slide_active&&!s_layout_active);
  assert(dirty_context&&dirty_background&&geometry&&prunes==2);
  assert(s_time_buffer[0]&&s_numeric_date_buffer[0]);
  prv_draw_context(&context,NULL);assert(reports==1&&s_native->activity_plan.cache.drawn);
  /* A static activity is not repainted just because the wall clock changes. */
  s_native->activity_plan.cache=(ActivityPlanCache){selected,locale,(uint32_t)now,(uint32_t)now+300,true,true,false,true};
  reset_counts();now+=60;prv_update_clock();
  assert(dirty_background==1&&!dirty_context&&!geometry);
  /* Locale, datum expiry and a changing clock still invalidate the plan. */
  locale=czech;prv_update_clock();assert(dirty_context==1);
  s_native->activity_plan.cache.language=locale;s_native->activity_plan.cache.valid_until=(uint32_t)now;
  prv_update_clock();assert(dirty_context==2);
  s_native->activity_plan.cache.valid_until=(uint32_t)now+300;
  s_native->activity_plan.cache.ticking=true;
  prv_update_clock();assert(dirty_context==3);
  /* Expiry/promotion back to idle still updates geometry on a minute boundary. */
  selected=NULL;prv_update_clock();assert(s_pose_target.context==0&&geometry);
  /* Failed decorative timer allocation settles immediately, including the
   * initial transition and a failure after a frame has already been drawn. */
  prv_cancel_motion();s_layout_active=s_slide_active=true;s_pose=watch_pose_context(false);
  fail_timer=true;prv_schedule_motion();
  assert(!s_layout_active&&!s_slide_active&&!s_motion_timer);
  assert(!memcmp(&s_pose,&s_pose_target,sizeof(s_pose)));
  s_layout_active=s_slide_active=true;prv_motion_frame(NULL);
  assert(!s_layout_active&&!s_slide_active&&!s_motion_timer);
  puts("covered callbacks: zero decorative registrations/dirty requests; fresh return and static-minute/expiry/locale gates passed");
  return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dynamic-native-lifecycle-') as tmp:
            path = Path(tmp) / 'callbacks.c'
            path.write_text(harness)
            output = Path(tmp) / 'callbacks'
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-I' + str(ROOT / 'src/c'), str(path), '-o', str(output)], check=True)
            subprocess.run([str(output)], check=True)


if __name__ == '__main__':
    unittest.main()
