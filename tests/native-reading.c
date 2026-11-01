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
    covered(language->loading);covered(language->unavailable);covered(language->expired);
    covered(language->first);covered(language->second);
  }
  assert(strcmp(reading_language("nb-NO")->lang,"no")==0);
  assert(strcmp(reading_language("nn_NO")->lang,"no")==0);
  assert(strcmp(reading_language("xx")->lang,"en")==0);
  return 0;
}
