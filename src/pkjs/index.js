var MessageKeys = require('message_keys');
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var core = require('pebble-watchface-core/readings');
var activityLibrary = require('pebble-activities');
var activityPresentation = require('./activity-presentation');
var navigation = require('../common/activity-navigation').create({storage:localStorage,now:function(){return Date.now();}});
var createComplications = core.createComplications;
var createContext = activityLibrary.createContext;
var createMessageQueue = core.createMessageQueue;
var localization = require('./localization');
var previewAssets = require('./settings-preview');
var clay = new Clay(clayConfig, require('./settings-page'), {autoHandleEvents: false});

function settingsLanguage() {
  var watchLanguage;
  try {
    var watch = typeof Pebble.getActiveWatchInfo === 'function' && Pebble.getActiveWatchInfo();
    watchLanguage = watch && watch.language;
  } catch (ignored) {}
  var phoneLanguage = typeof navigator !== 'undefined' &&
    (navigator.language || navigator.userLanguage || (navigator.languages && navigator.languages[0]));
  return localization.resolveLanguage(watchLanguage, phoneLanguage);
}
function messageValue(payload, name) {
  return payload[name] !== undefined ? payload[name] : payload[MessageKeys[name]];
}
function savedSettings() {
  try { return JSON.parse(localStorage.getItem('clay-settings')) || {}; }
  catch (error) { return {}; }
}
function clockFormat(settings) {
  var value = messageValue(settings, 'CLOCK_FORMAT');
  if (value === 0 || value === 12 || value === 24) { return value; }
  var previous = messageValue(settings, 'USE_12_HOUR');
  if (previous === true || previous === 1) { return 12; }
  if (previous === false || previous === 0) { return 24; }
  return 0;
}
var send = createMessageQueue({
  keys: MessageKeys,
  snapshotChannels:[{name:'activities',dataKey:'ACTIVITY_DATA',sidecarKeys:['REQUEST_ACTIVITIES']}],
  send: function(payload, success, failure) { Pebble.sendAppMessage(payload, success, failure); },
  later: function(callback, delay) { return setTimeout(callback, delay); },
  cancelLater: function(timer) { clearTimeout(timer); }
});
function log(message) { console.log(message); }
function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, function(character) {
    return {'&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#39;'}[character];
  });
}
function privateReading(slot) { return slot && (slot.category === 'music' || slot.category === 'notifications'); }

// The default readings follow global complication choices. Context uses the
// same source API for music, optional new notifications and phone battery.
// There is no direct weather client, provider override or local unit rewrite.
var complications = createComplications({
  pebble: Pebble, keys: MessageKeys, storage: localStorage,
  cacheKey: 'dynamic-complications-v1',
  send: send, now: function() { return Date.now(); },
  later: function(callback, delay) { return setTimeout(callback, delay); },
  cancelLater: function(timer) { clearTimeout(timer); }, log: log,
  getSettings: function() {
    var format = clockFormat(savedSettings());
    return {use12Hour: format === 12, clockFormat: format, language: settingsLanguage()};
  },
  localizeBoolean: function(value) {
    return localization.t(value ? 'stateOn' : 'stateOff', settingsLanguage());
  }
});

function enabledSetting(name, fallback) {
  var value = messageValue(savedSettings(), name);
  return value === undefined ? fallback : value === true || value === 1 || value === 'true' || value === '1';
}
var ACTIVITY_PREFERENCES_KEY = 'dynamic-activity-preferences-v1';
var ACTIVITY_PREFERENCES_RESPONSE = '__dynamicActivityPreferences';
function activityPreferences() {
  var value;
  try { value = JSON.parse(localStorage.getItem(ACTIVITY_PREFERENCES_KEY)); } catch (ignored) {}
  var result = activityLibrary.selection.normalizePreferences(value);
  // Existing source/privacy switches remain authoritative for both built-ins.
  result.hidden = result.hidden.filter(function(id) { return id !== 'builtin:music' && id !== 'builtin:notifications'; });
  if (!enabledSetting('SHOW_MUSIC', true)) { result.hidden.push('builtin:music'); }
  if (!enabledSetting('SHOW_NOTIFICATIONS', false)) { result.hidden.push('builtin:notifications'); }
  // Existing preferences predate this row. New arrivals lead by default until
  // the user saves an explicit Notifications position in Dynamic's own order.
  if (result.order.indexOf('builtin:notifications') < 0) {
    result.order = ['builtin:notifications'].concat(result.order).slice(0,64);
  }
  return result;
}
var contextProjectionSignature = '[null,null]';
function contextChanged() {
  if (!context) { return; }
  // Reproject only a changed canonical receipt. Battery/status traffic does
  // not resend a complete activity view; these RAM signatures contain no text.
  var signature = JSON.stringify([context.musicActivity(),
    typeof context.notificationActivity === 'function' ? context.notificationActivity() : null].map(function(card) {
      return card ? [card.provider,card.id,card.revision,card.observedAt,card.validUntil] : null;
    }));
  if (signature === contextProjectionSignature) { return; }
  contextProjectionSignature = signature;
  if (activities) { activities.reproject(); }
}
var context = createContext({
  pebble: Pebble, keys: MessageKeys, send: send, musicAsActivity:true,notificationAsActivity:true,
  now: function() { return Date.now(); },
  later: function(callback, delay) { return setTimeout(callback, delay); },
  cancelLater: function(timer) { clearTimeout(timer); },
  onChanged:contextChanged,
  getSettings: function() {
    var apps = messageValue(savedSettings(), 'NOTIFICATION_APPS');
    return {showMusic:enabledSetting('SHOW_MUSIC', true),
      showNotifications:enabledSetting('SHOW_NOTIFICATIONS', false),
      notificationPreview:enabledSetting('NOTIFICATION_PREVIEW', false),
      notificationAppOnly:enabledSetting('NOTIFICATION_APP_ONLY', false),
      notificationApps:typeof apps === 'string' ? apps : '',
      showStatus:enabledSetting('SHOW_STATUS', true)};
  }
});

// Dynamic displays summaries and opens apps. Action/entity descriptors belong
// to interactive hosts; avoid even requesting their separate source fields.
var activitySourceApi = {};
if (typeof Pebble.enumeratePlugins === 'function') {
  activitySourceApi.enumeratePlugins = function() { return Pebble.enumeratePlugins(); };
}
if (typeof Pebble.subscribeToSource === 'function') {
  activitySourceApi.subscribeToSource = function(request) {
    var display = {};
    Object.keys(request).forEach(function(key) { display[key] = request[key]; });
    if (Array.isArray(request.properties)) {
      display.properties = request.properties.filter(function(property) {
        return property !== 'actions' && property !== 'entity_ref';
      });
    }
    return Pebble.subscribeToSource(display);
  };
}
var activities = activityLibrary.createActivities({
  pebble:activitySourceApi,keys:MessageKeys,send:send,nativeFormat:3,
  onView:function(snapshot){navigation.view(snapshot);},
  // All generic providers share the same roles. Built-in privacy/source
  // controls are handled by their adapters and selection preferences.
  acceptActivity:function(activity){
    var kind=String(activity.kind).toLowerCase();
    var media=activity.type==='media'||activity.presentation===2||kind==='music';
    return enabledSetting('SHOW_ACTIVITIES',true)&&
      (!media||enabledSetting('SHOW_MUSIC',true));
  },
  select:function(cards){
    return activityPresentation.select(cards,activityPreferences(),context.musicActivity(),
      typeof context.notificationActivity === 'function' ? context.notificationActivity() : null);
  },
  now:function(){return Date.now();},
  later:function(callback,delay){return setTimeout(callback,delay);},
  cancelLater:function(timer){clearTimeout(timer);},
  // The coordinator also transports built-in music. Generic plugin visibility
  // is a host filter so music and idle-provider discovery remain available.
  getSettings:function(){return {enabled:true,
    providers:enabledSetting('SHOW_ACTIVITIES',true)?undefined:[],
    disabledProviders:activityPreferences().hidden.filter(function(id){return id!=='builtin:music'&&id!=='builtin:notifications';})};}
});

function requestContextSession(attempt) {
  var query = {}; query[MessageKeys.REQUEST_CONTEXT] = 1;
  send(query, null, function() {
    if (attempt < 3) { setTimeout(function() { requestContextSession(attempt + 1); }, attempt * 1000); }
  });
}
Pebble.addEventListener('ready', function() {
  resyncPresentation();
  complications.refresh(); context.refresh(true); activities.refresh(); requestContextSession(1);
});
var SYSTEM_CLOCK_KEY = 'dynamic-system-clock-v1';
function systemClockFormat() {
  try {
    var value = JSON.parse(localStorage.getItem(SYSTEM_CLOCK_KEY));
    return value === 12 || value === 24 ? value : null;
  } catch (ignored) { return null; }
}
Pebble.addEventListener('appmessage', function(event) {
  var presented=messageValue(event.payload||{},'ACTIVITY_PRESENTED');
  if(presented!==undefined)navigation.receive(presented);
  var format = messageValue(event.payload || {}, 'CLOCK_FORMAT');
  if (format === 12 || format === 24) {
    // This is the OS format, not the watchface's saved override.
    try { localStorage.setItem(SYSTEM_CLOCK_KEY, JSON.stringify(format)); } catch (ignored) {}
  }
  retrySavedSettings();
  if (messageValue(event.payload || {}, 'REQUEST_COMPLICATIONS')) { complications.refresh(); }
  var requested = messageValue(event.payload || {}, 'REQUEST_CONTEXT');
  var token = Number(requested);
  if (Number.isInteger(token) && token >= 2 && token <= 2147483647) {
    context.connectionChanged(true, token);
    // A reset or another watch can reuse the same counter. Every marked
    // baseline request may need recovery; ordinary refreshes use token 1.
    resyncPresentation();
  }
  else if (token === 1) { context.refresh(); }
  var activityToken=Number(messageValue(event.payload || {},'REQUEST_ACTIVITIES'));
  if(Number.isInteger(activityToken)&&activityToken>=2&&activityToken<=2147483647)
    activities.connectionChanged(true,activityToken);
  else if(activityToken===1)activities.refresh();
});
Pebble.addEventListener('showConfiguration', function() {
  var clockQuery = {}; clockQuery[MessageKeys.CLOCK_FORMAT] = 255;
  send(clockQuery); // Native replies separately; this offline page remains a snapshot.
  var language = settingsLanguage();
  clay.config = localization.translateConfig(clayConfig, language);
  var saved = savedSettings();
  var status = complications.status();
  var dynamic = context.status();
  var metadata = settingsMetadata(status, dynamic);
  metadata.language = language;
  function t(key, params) { return localization.t(key, language, params); }
  function visit(items) {
    items.forEach(function(item) {
      if (item.items) { visit(item.items); }
      // Clay restores named keys. Also present legacy numeric-key preferences
      // without rewriting storage merely because the user opened settings.
      var previous = item.messageKey && messageValue(saved, item.messageKey);
      if (item.type === 'toggle' && (previous === true || previous === false || previous === 0 || previous === 1)) {
        item.defaultValue = !!previous;
      }
      if (item.type === 'color' && typeof previous === 'number' && isFinite(previous) && previous >= 0 && previous <= 0xFFFFFF) {
        item.defaultValue = previous;
      }
      if (item.messageKey === 'NOTIFICATION_APPS' && typeof previous === 'string') { item.defaultValue = previous; }
      if (item.messageKey === 'CLOCK_FORMAT') { item.defaultValue = clockFormat(saved); }
      if (item.messageKey === 'NOTIFICATION_VIBRATION' && [0,1,2].indexOf(previous) >= 0) { item.defaultValue = previous; }
      if (item.id === 'dynamic-settings-data') { item.defaultValue = escapeHtml(JSON.stringify(metadata)); return; }
      if (item.id === 'dynamic-preview-assets') {
        var assets = Object.assign({}, previewAssets, {systemFormat:systemClockFormat()});
        item.defaultValue = escapeHtml(JSON.stringify(assets)); return;
      }
      if (item.id === 'dynamic-settings-strings') {
        item.defaultValue = escapeHtml(JSON.stringify(localization.settingsStrings(language))); return;
      }
      if (item.id === 'context-status') {
        item.defaultValue = escapeHtml(t(dynamic.error ? 'contextError' : dynamic.ready ? 'contextReady' : 'contextWaiting',
          {error:dynamic.error || ''}));
        return;
      }
      if (item.id !== 'complications-status') { return; }
      var summary = status.available === false ? t('complicationsDisabled') :
        status.configured ? t('complicationsStatus', {ready: status.ready || 0, configured: status.configured}) :
        t('complicationsWaiting');
      var issue = status.error;
      var problem = (status.slots || []).filter(function(slot) { return slot.error === issue; })[0];
      if (issue && problem) {
        issue = '#' + problem.slot + ' · ' + problem.category + '/' + problem.item +
          (problem.property ? '/' + problem.property : '') + ' · ' + issue +
          (problem.message && !privateReading(problem) ? ' · ' + problem.message : '');
      } else if (issue && status.message) { issue += ' · ' + status.message; }
      // Clay text fields support HTML. Source diagnostics remain literal text,
      // including provider-supplied error messages and category/item names.
      item.defaultValue = escapeHtml('Dynamic ' + previewAssets.version + '. ' + summary +
        (issue ? ' · ' + t('complicationsError', {error: issue}) : ''));
    });
  }
  visit(clay.config);
  Pebble.openURL(clay.generateUrl());
});

function watchSettings(payload) {
  var result = {};
  ['BACKGROUND_COLOR', 'TEXT_COLOR', 'ACCENT_COLOR',
    'USE_12_HOUR', 'CLOCK_FORMAT', 'SHOW_STATUS', 'ANIMATIONS', 'NOTIFICATION_VIBRATION',
    'NIGHT_PALETTE', 'NIGHT_BACKGROUND_COLOR', 'NIGHT_TEXT_COLOR', 'NIGHT_ACCENT_COLOR'].forEach(function(name) {
    var value = messageValue(payload, name);
    if (value === undefined) { return; }
    if (/_COLOR$/.test(name)) {
      if (typeof value !== 'number' || !isFinite(value) || Math.floor(value) !== value ||
          value < 0 || value > 0xFFFFFF) { return; }
    } else if (name === 'NOTIFICATION_VIBRATION') {
      if (value !== 0 && value !== 1 && value !== 2) { return; }
    } else if (name === 'CLOCK_FORMAT') {
      if (value !== 0 && value !== 12 && value !== 24) { return; }
    } else {
      if (value === true || value === false || value === 0 || value === 1) { value = Number(value); }
      else { return; }
    }
    result[MessageKeys[name]] = value;
  });
  return result;
}

var DELIVERY_KEY = 'dynamic-settings-delivery-v1';
var settingsDelivery = null;
var settingsRetry = null;
var deliverySequence = 0;
function deliveryRecord() {
  try {
    var record = JSON.parse(localStorage.getItem(DELIVERY_KEY));
    if (!record || typeof record.id !== 'string' ||
        ['pending', 'failed', 'saved'].indexOf(record.state) < 0) { return null; }
    // An earlier build may have queued only its now-retired text-size choice.
    // There is no active setting to deliver or to show as waiting in this UI.
    if (record.state !== 'saved' && !Object.keys(watchSettings(record.payload || {})).length) { return null; }
    return record;
  } catch (ignored) { return null; }
}
function storeDelivery(record) {
  try { localStorage.setItem(DELIVERY_KEY, JSON.stringify(record)); }
  catch (ignored) { log('Settings delivery status could not be stored'); }
}
function settingsMetadata(status, dynamic) {
  var details = dynamic.sourceStates || {};
  function source(name, enabled) {
    if (!enabled) { return {state:'off', message:''}; }
    if (details[name]) { return {state:details[name].state, message:details[name].message || ''}; }
    var error = dynamic.sources && dynamic.sources[name] || '';
    return {state:error === 'PERMISSION_DENIED' ? 'denied' : error || dynamic.available === false ? 'unavailable' :
      dynamic.connected === false ? 'waiting' : name === 'music' ? dynamic.music ? 'playing' : dynamic.ready ? 'idle' : 'waiting' :
      dynamic.ready ? 'ready' : 'waiting', message:error};
  }
  var slots = status.slots || [];
  var record = deliveryRecord();
  return {slots:[0,1].map(function(index) {
    var slot = slots[index] || {};
    var privateValue = privateReading(slot);
    return {side:index ? 'right' : 'left', label:privateValue ? slot.property || '' : slot.label || slot.property || '',
      value:privateValue ? '' : slot.text || '',
      status:slot.status || (status.available === false ? 'disabled' : 'empty'), help:privateValue ? '' : slot.message || '',
      receivedAgeSec:typeof slot.receivedAgeSec === 'number' && isFinite(slot.receivedAgeSec) && slot.receivedAgeSec >= 0 ? slot.receivedAgeSec : null,
      observationAgeSec:typeof slot.observationAgeSec === 'number' && isFinite(slot.observationAgeSec) && slot.observationAgeSec >= 0 ? slot.observationAgeSec : null,
      category:slot.category || '', item:slot.item || '', property:slot.property || ''};
  }), sources:{music:source('music', enabledSetting('SHOW_MUSIC', true)),
    notifications:source('notifications', enabledSetting('SHOW_NOTIFICATIONS', false)),
    phone:source('phone', enabledSetting('SHOW_STATUS', true)),
    activities:(function(){
      var status=activities.status();
      return {state:!enabledSetting('SHOW_ACTIVITIES',true)?'off':
        status.error==='PERMISSION_DENIED'?'denied':status.error?'unavailable':
        status.available===false?'disabled':status.ready?(status.count?'ready':'idle'):'waiting',
        message:status.error || ''};
    })()},
    activityPreferences:activityPreferences(),
    activityProviders:typeof activities.providers==='function'?activities.providers():[],
    save:{state:record ? record.state : 'none', message:''}};
}
function deliverSettings(record, attempt) {
  settingsDelivery = record.id;
  function current() { var latest = deliveryRecord(); return latest && latest.id === record.id; }
  send(record.payload, function() {
    if (settingsDelivery === record.id) { settingsDelivery = null; }
    if (current()) { storeDelivery({id:record.id, state:'saved'}); }
    complications.configure();
  }, function() {
    if (settingsDelivery === record.id) { settingsDelivery = null; }
    if (!current()) { return; }
    if (attempt < 3) {
      settingsRetry = setTimeout(function() {
        settingsRetry = null;
        if (current()) { deliverSettings(record, attempt + 1); }
      }, attempt * 1000);
    } else {
      record.state = 'failed'; storeDelivery(record);
      log('Settings are saved on the phone; waiting for watch delivery'); complications.configure();
    }
  });
}
function retrySavedSettings() {
  if (settingsDelivery || settingsRetry !== null) { return; }
  var record = deliveryRecord();
  if (!record || record.state === 'saved') { return; }
  var payload = watchSettings(record.payload || {});
  if (!Object.keys(payload).length) { return; }
  record.payload = payload; record.state = 'pending'; storeDelivery(record);
  deliverSettings(record, 1);
}
function resyncPresentation() {
  // Phone settings survive watch resets and switching watches. Reapply only
  // validated presentation values; private source controls never leave PKJS.
  // A current pending save wins over older stored values. A fresh record also
  // prevents an old watch/session's late ACK from marking this sync delivered.
  var payload = watchSettings(savedSettings());
  var previous = deliveryRecord();
  if (previous && previous.state !== 'saved') {
    var waiting = watchSettings(previous.payload || {});
    Object.keys(waiting).forEach(function(key) { payload[key] = waiting[key]; });
  }
  if (!Object.keys(payload).length) { return; }
  if (settingsRetry !== null) { clearTimeout(settingsRetry); settingsRetry = null; }
  var record = {id:String(Date.now()) + '-' + (++deliverySequence), state:'pending', payload:payload};
  storeDelivery(record); deliverSettings(record, 1);
}
Pebble.addEventListener('webviewclosed', function(event) {
  if (!event || !event.response) { return; }
  var payload;
  try {
    var response = JSON.parse(/^\{/.test(event.response) ? event.response : decodeURIComponent(event.response));
    var preferences = response[ACTIVITY_PREFERENCES_RESPONSE];
    delete response[ACTIVITY_PREFERENCES_RESPONSE];
    if (preferences && typeof preferences === 'object' && typeof preferences.value === 'string') {
      preferences = JSON.parse(preferences.value);
    }
    payload = watchSettings(clay.getSettings(JSON.stringify(response)));
    if (preferences && typeof preferences === 'object' && !Array.isArray(preferences)) {
      // Only stable provider keys and visibility flags survive. Generic titles,
      // private music content and temporary run IDs never enter phone storage.
      localStorage.setItem(ACTIVITY_PREFERENCES_KEY,
        JSON.stringify(activityLibrary.selection.normalizePreferences(preferences)));
    }
  }
  catch (error) { log('Invalid settings response'); return; }
  // Apply phone-only source controls immediately, including privacy switches.
  // Theme delivery can retry independently without postponing a context clear.
  context.configure(); activities.reproject(); activities.configure();
  if (!Object.keys(payload).length) { complications.configure(); return; }
  if (settingsRetry !== null) { clearTimeout(settingsRetry); settingsRetry = null; }
  var record = {id:String(Date.now()) + '-' + (++deliverySequence), state:'pending', payload:payload};
  storeDelivery(record); deliverSettings(record, 1);
});
