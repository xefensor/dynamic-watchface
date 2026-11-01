#include <assert.h>
#include "../src/c/native_reading.h"
#include "../src/c/generated_body_metrics.h"

static void covered(const char *text) {
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor) {
    const uint32_t code=body_metric_next_codepoint(&cursor);
    const size_t at=body_metric_index(code);
    assert(body_metric_codepoints[at]==code);
  }
}
static int label_width(const void *context,const char *text) {
  (void)context;int width=0;
  const unsigned char *cursor=(const unsigned char *)text;
  while(*cursor){body_metric_next_codepoint(&cursor);width+=10;}
  return width;
}

int main(void) {
  ComplicationSlot slot={0};
  slot.status=COMPLICATION_STATUS_LOADING;
  assert(reading_state(&slot,200)==READING_LOADING);
  slot.status=COMPLICATION_STATUS_UNAVAILABLE;
  assert(reading_state(&slot,200)==READING_UNAVAILABLE);
  slot.status=COMPLICATION_STATUS_DENIED;
  assert(reading_state(&slot,200)==READING_UNAVAILABLE);
  slot.status=COMPLICATION_STATUS_EXPIRED;
  assert(reading_state(&slot,200)==READING_EXPIRED);
  slot.status=COMPLICATION_STATUS_READY;slot.confirmed_at=100;slot.valid_until=1000;
  assert(reading_state(&slot,200)==READING_VALUE);
  slot.cached=true;
  assert(reading_state(&slot,200)==READING_VALUE);
  assert(reading_state(&slot,1000)==READING_EXPIRED);

  ReadingParts percent,temp;
  assert(reading_parts("73%",&percent)&&reading_parts("6°C",&temp));
  assert(reading_semantic(&slot,&percent,true)==COMPLICATION_SEMANTIC_GENERIC);
  assert(reading_semantic(&slot,&temp,true)==COMPLICATION_SEMANTIC_TEMPERATURE);
  slot.semantic_kind=COMPLICATION_SEMANTIC_HUMIDITY;
  assert(reading_semantic(&slot,&percent,true)==COMPLICATION_SEMANTIC_HUMIDITY);
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"cs"),"VLHKOST")==0);
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"HUMIDITY")==0);
  slot.semantic_kind=COMPLICATION_SEMANTIC_GENERIC;
  strcpy(slot.label,"Example source");
  assert(strcmp(reading_label(&slot,0,0,"cs"),"Example source")==0);
  slot.label[0]='\0';
  assert(strcmp(reading_label(&slot,0,1,"en"),"SLOT 2")==0);
  const unsigned named_kinds[]={COMPLICATION_SEMANTIC_GENERIC,COMPLICATION_SEMANTIC_NUMBER,
    COMPLICATION_SEMANTIC_RANGE,COMPLICATION_SEMANTIC_BOOLEAN,COMPLICATION_SEMANTIC_TIMESTAMP};
  char progress[COMPLICATION_MAX_LABEL_BYTES+1];
  for(size_t i=0;i<sizeof(named_kinds)/sizeof(named_kinds[0]);++i) {
    strcpy(slot.label,"Progress");strcpy(progress,reading_label(&slot,named_kinds[i],0,"en"));
    strcpy(slot.label,"Charge");
    assert(strcmp(progress,"Progress")==0&&strcmp(reading_label(&slot,named_kinds[i],1,"en"),"Charge")==0);
    assert(strcmp(progress,reading_label(&slot,named_kinds[i],1,"en"))!=0);
  }
  strcpy(slot.label,"Příliš žluťoučký");
  char fitted[COMPLICATION_MAX_LABEL_BYTES+16];
  strcpy(fitted,reading_label(&slot,COMPLICATION_SEMANTIC_NUMBER,0,"cs"));
  reading_label_ellipsize(fitted,sizeof(fitted),60,label_width,NULL);
  assert(strcmp(fitted,"Příli…")==0&&label_width(NULL,fitted)==60);
  struct {char label[8];char guard;} small={{0},'x'};
  strcpy(small.label,"ČČČ");
  reading_label_ellipsize(small.label,sizeof(small.label),20,label_width,NULL);
  assert(strcmp(small.label,"Č…")==0&&small.guard=='x');
  strcpy(slot.label,"Provider words");
  assert(strcmp(reading_label(&slot,COMPLICATION_SEMANTIC_HUMIDITY,0,"cs"),"VLHKOST")==0);

  // Accurate meaning survives value expiry and does not claim a measurement
  // is current merely because it was received/cached recently.
  slot.semantic_kind=COMPLICATION_SEMANTIC_TEMPERATURE;
  slot.detail=COMPLICATION_DETAIL_FEELS_LIKE;
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"FEELS LIKE")==0);
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"cs"),"POCITOVÁ")==0);
  slot.detail=COMPLICATION_DETAIL_HIGH;
  slot.status=COMPLICATION_STATUS_EXPIRED;
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"HIGH")==0);
  slot.detail=COMPLICATION_DETAIL_LOW;
  assert(strcmp(reading_label(&slot,slot.semantic_kind,1,"en"),"LOW")==0);
  slot.detail=COMPLICATION_DETAIL_TEMPERATURE;slot.qualifiers=COMPLICATION_QUALIFIER_FORECAST;
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"FORECAST")==0);
  slot.qualifiers=0;slot.detail=0;slot.label[0]='\0';
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"TEMP")==0);
  strcpy(slot.label,"Daily high");
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"cs"),"Daily high")==0); // v3 meaning.
  slot.semantic_kind=COMPLICATION_SEMANTIC_BATTERY;slot.source_kind=COMPLICATION_SOURCE_PHONE;
  assert(strcmp(reading_label(&slot,slot.semantic_kind,0,"en"),"PHONE")==0);
  slot.cached=true;slot.confirmed_at=100;slot.observed_at=0;
  assert(!reading_is_aged(&slot,5000));
  slot.observed_at=100;assert(!reading_is_aged(&slot,3699));assert(reading_is_aged(&slot,3700));
  slot.cached=false;assert(reading_is_aged(&slot,3700));
  slot.observed_at=5000;assert(!reading_is_aged(&slot,4000));

  // Guard the actual four-level display palette, not just CSS input colours.
  assert(reading_secondary_ink(0xc0,0xff)==0xd5);
  assert(reading_secondary_ink(0xc6,0xff)==0xc6); // Muted blue would wash out.
  assert(reading_accent_ink(0xdb,0xff,0xc0)==0xdb); // Default blue stays blue.
  assert(reading_accent_ink(0xff,0xff,0xc0)==0xc0);
  for(unsigned background=0xc0;background<=0xff;++background) {
    assert(reading_color_contrasts(reading_contrast_ink(background),background,4500));
    for(unsigned foreground=0xc0;foreground<=0xff;++foreground) {
      const uint8_t secondary=reading_secondary_ink(foreground,background);
      if(reading_color_contrasts(foreground,background,4500))
        assert(reading_color_contrasts(secondary,background,4500));
      assert(reading_color_contrasts(reading_accent_ink(foreground,background,foreground),background,2000));
    }
  }

  double range=-1;
  assert(!reading_range(&slot,&range)); // A plain73% has no implied meaning/range.
  slot.numeric_value=73;slot.minimum=0;slot.maximum=100;
  slot.semantic_flags=COMPLICATION_SEMANTIC_VALUE;
  assert(!reading_range(&slot,&range)); // A numeric value alone is still insufficient.
  slot.semantic_flags|=COMPLICATION_SEMANTIC_MIN|COMPLICATION_SEMANTIC_MAX;
  assert(reading_range(&slot,&range)&&range>0.729&&range<0.731);
  slot.numeric_value=150;
  assert(reading_range(&slot,&range)&&range==1);
  slot.numeric_value=-10;
  assert(reading_range(&slot,&range)&&range==0);
  slot.maximum=slot.minimum;
  assert(!reading_range(&slot,&range));
  slot.maximum=100;slot.numeric_value=NAN;
  assert(!reading_range(&slot,&range));
  slot.numeric_value=73;slot.maximum=INFINITY;
  assert(!reading_range(&slot,&range));

  const char *locales[]={"en","cs","sk","de","fr","es","it","pt","nl","pl","da","sv","no","fi","hu","ro","tr"};
  for(size_t i=0;i<sizeof(locales)/sizeof(locales[0]);++i) {
    const ReadingLanguage *language=reading_language(locales[i]);
    assert(strcmp(language->lang,locales[i])==0);
    assert(strcmp(language->loading,language->unavailable)!=0&&strcmp(language->expired,language->unavailable)!=0);
    for(unsigned kind=0;kind<12;++kind) {assert(language->labels[kind][0]);covered(language->labels[kind]);}
    for(unsigned detail=1;detail<12;++detail) {assert(language->details[detail][0]);covered(language->details[detail]);}
    covered(language->forecast);
    covered(language->loading);covered(language->unavailable);covered(language->expired);
    covered(language->first);covered(language->second);
  }
  assert(strcmp(reading_language("nb-NO")->lang,"no")==0);
  assert(strcmp(reading_language("nn_NO")->lang,"no")==0);
  assert(strcmp(reading_language("xx")->lang,"en")==0);
  return 0;
}
