#pragma once

#include "reading_presentation.h"
#include "watch_layout.h"

typedef enum { READING_VALUE, READING_LOADING, READING_UNAVAILABLE, READING_EXPIRED } ReadingState;

static inline ReadingState reading_state(const ComplicationSlot *slot,time_t now) {
  if(slot->status==COMPLICATION_STATUS_LOADING)return READING_LOADING;
  if(slot->status==COMPLICATION_STATUS_READY)
    return complication_is_fresh(slot,now)?READING_VALUE:READING_EXPIRED;
  return READING_UNAVAILABLE;
}

static inline unsigned reading_semantic(const ComplicationSlot *slot,const ReadingParts *parts,
                                         bool numeric) {
  if(slot->semantic_kind!=COMPLICATION_SEMANTIC_GENERIC)return slot->semantic_kind;
  if(numeric&&(strcmp(parts->unit,"°C")==0||strcmp(parts->unit,"°F")==0))
    return COMPLICATION_SEMANTIC_TEMPERATURE;
  if(numeric&&slot->display_kind==COMPLICATION_DISPLAY_PRECIPITATION&&reading_is_probability(parts))
    return COMPLICATION_SEMANTIC_PRECIPITATION;
  return COMPLICATION_SEMANTIC_GENERIC;
}

// Optional source bounds are authoritative. A plain percent is never enough
// to infer a rain/battery/humidity range or a weather condition.
static inline bool reading_range(const ComplicationSlot *slot,double *amount) {
  const unsigned required=COMPLICATION_SEMANTIC_VALUE|COMPLICATION_SEMANTIC_MIN|COMPLICATION_SEMANTIC_MAX;
  if((slot->semantic_flags&required)!=required||!isfinite(slot->numeric_value)||
      !isfinite(slot->minimum)||!isfinite(slot->maximum)||slot->maximum<=slot->minimum)return false;
  double value=(slot->numeric_value-slot->minimum)/(slot->maximum-slot->minimum);
  if(!isfinite(value))return false;
  *amount=value<0?0:value>1?1:value;
  return true;
}

typedef struct {
  const char *lang;
  const char *labels[12];
  const char *loading,*unavailable,*expired,*first,*second;
} ReadingLanguage;

static inline const ReadingLanguage *reading_language(const char *locale) {
  static const ReadingLanguage languages[]={
    {"en",{"READING","NOW","RAIN","BATTERY","HUMIDITY","WIND","UV","WEATHER","RANGE","VALUE","STATUS","TIME"},"Loading","Unavailable","Expired","SLOT 1","SLOT 2"},
    {"cs",{"HODNOTA","NYNÍ","DÉŠŤ","BATERIE","VLHKOST","VÍTR","UV","POČASÍ","ROZSAH","HODNOTA","STAV","ČAS"},"Načítání","Nedostupné","Vypršelo","POZICE 1","POZICE 2"},
    {"sk",{"HODNOTA","TERAZ","DÁŽĎ","BATÉRIA","VLHKOSŤ","VIETOR","UV","POČASIE","ROZSAH","HODNOTA","STAV","ČAS"},"Načítanie","Nedostupné","Vypršalo","POZÍCIA 1","POZÍCIA 2"},
    {"de",{"ANZEIGE","JETZT","REGEN","AKKU","FEUCHTE","WIND","UV","WETTER","BEREICH","WERT","STATUS","ZEIT"},"Lädt","Nicht da","Abgelaufen","FELD 1","FELD 2"},
    {"fr",{"DONNÉE","ACTUEL","PLUIE","BATTERIE","HUMIDITÉ","VENT","UV","MÉTÉO","PLAGE","VALEUR","ÉTAT","HEURE"},"Chargement","Indisponible","Expiré","ZONE 1","ZONE 2"},
    {"es",{"DATO","AHORA","LLUVIA","BATERÍA","HUMEDAD","VIENTO","UV","TIEMPO","RANGO","VALOR","ESTADO","HORA"},"Cargando","No disponible","Caducado","ESPACIO 1","ESPACIO 2"},
    {"it",{"DATO","ORA","PIOGGIA","BATTERIA","UMIDITÀ","VENTO","UV","METEO","INTERVALLO","VALORE","STATO","ORA"},"Caricamento","Non disponibile","Scaduto","SPAZIO 1","SPAZIO 2"},
    {"pt",{"DADO","AGORA","CHUVA","BATERIA","UMIDADE","VENTO","UV","TEMPO","INTERVALO","VALOR","ESTADO","HORA"},"Carregando","Indisponível","Expirado","ESPAÇO 1","ESPAÇO 2"},
    {"nl",{"WAARDE","NU","REGEN","ACCU","VOCHT","WIND","UV","WEER","BEREIK","WAARDE","STATUS","TIJD"},"Laden","Niet beschikbaar","Verlopen","VAK 1","VAK 2"},
    {"pl",{"DANE","TERAZ","DESZCZ","BATERIA","WILGOTNOŚĆ","WIATR","UV","POGODA","ZAKRES","WARTOŚĆ","STAN","CZAS"},"Ładowanie","Niedostępne","Wygasło","POLE 1","POLE 2"},
    {"da",{"VÆRDI","NU","REGN","BATTERI","FUGT","VIND","UV","VEJR","OMRÅDE","VÆRDI","STATUS","TID"},"Indlæser","Utilgængelig","Udløbet","FELT 1","FELT 2"},
    {"sv",{"VÄRDE","NU","REGN","BATTERI","FUKT","VIND","UV","VÄDER","INTERVALL","VÄRDE","STATUS","TID"},"Laddar","Ej tillgänglig","Utgånget","FÄLT 1","FÄLT 2"},
    {"no",{"VERDI","NÅ","REGN","BATTERI","FUKT","VIND","UV","VÆR","OMRÅDE","VERDI","STATUS","TID"},"Laster","Utilgjengelig","Utløpt","FELT 1","FELT 2"},
    {"fi",{"ARVO","NYT","SADE","AKKU","KOSTEUS","TUULI","UV","SÄÄ","ALUE","ARVO","TILA","AIKA"},"Ladataan","Ei saatavilla","Vanhentunut","PAIKKA 1","PAIKKA 2"},
    {"hu",{"ÉRTÉK","MOST","ESŐ","AKKU","PÁRA","SZÉL","UV","IDŐJÁRÁS","TARTOMÁNY","ÉRTÉK","ÁLLAPOT","IDŐ"},"Betöltés","Nem elérhető","Lejárt","HELY 1","HELY 2"},
    {"ro",{"VALOARE","ACUM","PLOAIE","BATERIE","UMIDITATE","VÂNT","UV","VREME","INTERVAL","VALOARE","STARE","ORĂ"},"Se încarcă","Indisponibil","Expirat","POZIȚIA 1","POZIȚIA 2"},
    {"tr",{"DEĞER","ŞİMDİ","YAĞMUR","PİL","NEM","RÜZGÂR","UV","HAVA","ARALIK","DEĞER","DURUM","SAAT"},"Yükleniyor","Kullanılamıyor","Süresi doldu","ALAN 1","ALAN 2"}
  };
  if(locale)for(size_t i=1;i<sizeof(languages)/sizeof(languages[0]);++i) {
    const char *code=languages[i].lang;
    if((locale[0]==code[0]&&locale[1]==code[1]&&(!locale[2]||locale[2]=='_'||locale[2]=='-'))||
        (code[0]=='n'&&code[1]=='o'&&locale[0]=='n'&&(locale[1]=='b'||locale[1]=='n')&&
         (!locale[2]||locale[2]=='_'||locale[2]=='-')))return &languages[i];
  }
  return &languages[0];
}

static inline const char *reading_label(const ComplicationSlot *slot,unsigned kind,
                                         unsigned index,const char *locale) {
  const ReadingLanguage *language=reading_language(locale);
  if(kind>COMPLICATION_SEMANTIC_TIMESTAMP)kind=COMPLICATION_SEMANTIC_GENERIC;
  if(slot->label[0]&&(kind==COMPLICATION_SEMANTIC_GENERIC||kind==COMPLICATION_SEMANTIC_NUMBER||
      kind==COMPLICATION_SEMANTIC_RANGE||kind==COMPLICATION_SEMANTIC_BOOLEAN||
      kind==COMPLICATION_SEMANTIC_TIMESTAMP))return slot->label;
  if(kind==COMPLICATION_SEMANTIC_GENERIC)return index?language->second:language->first;
  return language->labels[kind];
}

typedef int (*ReadingLabelWidth)(const void *context,const char *text);
// This is the measured label-fitting path used by the renderer. Preserve
// whole source scalars and reserve all three ellipsis bytes plus its NUL.
static inline void reading_label_ellipsize(char *text,size_t capacity,int width,
                                           ReadingLabelWidth measure,const void *context) {
  if(!text||!capacity||!measure||measure(context,text)<=width)return;
  if(capacity<4){text[0]='\0';return;}
  char prefix[COMPLICATION_MAX_LABEL_BYTES+16];
  const size_t prefix_capacity=capacity-3<sizeof(prefix)?capacity-3:sizeof(prefix);
  watch_layout_copy_utf8(text,prefix,prefix_capacity);
  while(watch_layout_pop_utf8(prefix)) {
    const size_t length=strlen(prefix);
    memcpy(text,prefix,length);memcpy(text+length,"…",4);
    if(measure(context,text)<=width)return;
  }
  memcpy(text,"…",4);
}

static inline const char *reading_state_label(ReadingState state,const char *locale) {
  const ReadingLanguage *language=reading_language(locale);
  return state==READING_LOADING?language->loading:state==READING_EXPIRED?language->expired:language->unavailable;
}
