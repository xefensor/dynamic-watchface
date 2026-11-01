var MessageKeys = require('message_keys');
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var core = require('pebble-watchface-core');
var createComplications = core.createComplications;
var createContext = core.createContext;
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
var context = createContext({
  pebble: Pebble, keys: MessageKeys, send: send,
  now: function() { return Date.now(); },
  later: function(callback, delay) { return setTimeout(callback, delay); },
  cancelLater: function(timer) { clearTimeout(timer); },
  getSettings: function() {
    var apps = messageValue(savedSettings(), 'NOTIFICATION_APPS');
    return {showMusic:enabledSetting('SHOW_MUSIC', true),
      showNotifications:enabledSetting('SHOW_NOTIFICATIONS', false),
      notificationPreview:enabledSetting('NOTIFICATION_PREVIEW', false),
      notificationApps:typeof apps === 'string' ? apps : '',
      showStatus:enabledSetting('SHOW_STATUS', true)};
  }
});

function requestContextSession(attempt) {
  var query = {}; query[MessageKeys.REQUEST_CONTEXT] = 1;
  send(query, null, function() {
    if (attempt < 3) { setTimeout(function() { requestContextSession(attempt + 1); }, attempt * 1000); }
  });
}
Pebble.addEventListener('ready', function() {
  retrySavedSettings();
  complications.refresh(); context.refresh(true); requestContextSession(1);
});
Pebble.addEventListener('appmessage', function(event) {
  retrySavedSettings();
  if (messageValue(event.payload || {}, 'REQUEST_COMPLICATIONS')) { complications.refresh(); }
  var requested = messageValue(event.payload || {}, 'REQUEST_CONTEXT');
  var token = Number(requested);
  if (Number.isInteger(token) && token >= 2 && token <= 2147483647) { context.connectionChanged(true, token); }
  else if (token === 1) { context.refresh(); }
});
Pebble.addEventListener('showConfiguration', function() {
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
      if (item.id === 'dynamic-settings-data') { item.defaultValue = escapeHtml(JSON.stringify(metadata)); return; }
      if (item.id === 'dynamic-preview-assets') { item.defaultValue = escapeHtml(JSON.stringify(previewAssets)); return; }
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
      item.defaultValue = escapeHtml('Dynamic 0.4.0. ' + summary +
        (issue ? ' · ' + t('complicationsError', {error: issue}) : ''));
    });
  }
  visit(clay.config);
  Pebble.openURL(clay.generateUrl());
});

function watchSettings(payload) {
  var result = {};
  ['BACKGROUND_COLOR', 'TEXT_COLOR', 'ACCENT_COLOR', 'LARGE_TEXT',
    'USE_12_HOUR', 'CLOCK_FORMAT', 'SHOW_STATUS', 'ANIMATIONS'].forEach(function(name) {
    var value = messageValue(payload, name);
    if (value === undefined) { return; }
    if (name === 'BACKGROUND_COLOR' || name === 'TEXT_COLOR' || name === 'ACCENT_COLOR') {
      if (typeof value !== 'number' || !isFinite(value) || Math.floor(value) !== value ||
          value < 0 || value > 0xFFFFFF) { return; }
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
    return record && typeof record.id === 'string' &&
      ['pending', 'failed', 'saved'].indexOf(record.state) >= 0 ? record : null;
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
    phone:source('phone', enabledSetting('SHOW_STATUS', true))},
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
Pebble.addEventListener('webviewclosed', function(event) {
  if (!event || !event.response) { return; }
  var payload;
  try { payload = watchSettings(clay.getSettings(event.response)); }
  catch (error) { log('Invalid settings response'); return; }
  // Apply phone-only source controls immediately, including privacy switches.
  // Theme delivery can retry independently without postponing a context clear.
  context.configure();
  if (!Object.keys(payload).length) { complications.configure(); return; }
  if (settingsRetry !== null) { clearTimeout(settingsRetry); settingsRetry = null; }
  var record = {id:String(Date.now()) + '-' + (++deliverySequence), state:'pending', payload:payload};
  storeDelivery(record); deliverSettings(record, 1);
});
