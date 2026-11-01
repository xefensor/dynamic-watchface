'use strict';

// Clay copies only this function to its offline page. Everything used below is
// either supplied as a transport component or declared inside the function.
module.exports = function() {
  var clay = this;
  var style = document.createElement('style');
  style.id = 'dynamic-settings-style';
  style.textContent = [
    ':root{color-scheme:light;--page:#f4f5f7;--card:#fff;--ink:#101216;--muted:#5a6068;--line:#dde1e6;--accent:#145ca3;--field:#f4f5f7;--track:#cad0d6}',
    '@media(prefers-color-scheme:dark){:root{color-scheme:dark;--page:#141618;--card:#212529;--ink:#f6f8fa;--muted:#b8c0c9;--line:#3c444d;--accent:#77bbff;--field:#2a3037;--track:#565f6a}}',
    'html{font-size:16px!important;height:auto;background:var(--page)}',
    'body{box-sizing:border-box;height:auto;max-width:512px;margin:0 auto;padding:20px 16px 104px;padding-bottom:calc(104px + env(safe-area-inset-bottom));background:var(--page);color:var(--ink);font:16px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}',
    '*,*:before,*:after{box-sizing:border-box}',
    '#main-form>.component-heading:first-child{padding:0 2px 20px}',
    '.component-heading h1,.component-heading h2,.component-heading h3,.component-heading h4{margin:0;color:var(--ink);font:700 17px/1.3 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;text-transform:none;letter-spacing:0}',
    '#main-form>.component-heading:first-child h1,#main-form>.component-heading:first-child h2,#main-form>.component-heading:first-child h4{font-size:28px;letter-spacing:-.6px}',
    '#main-form>.component-heading:first-child:after{content:"";display:block;width:48px;height:5px;margin-top:12px;background:var(--accent)}',
    '.section{margin:0 0 16px;border:1px solid var(--line);border-radius:14px;background:var(--card);box-shadow:none;overflow:hidden}',
    '.section>.component{margin-top:0!important;padding:13px 16px;position:relative}',
    '.section>.component-heading:first-child{padding:16px 16px 10px;background:none;border-radius:0}',
    '.section>.component-heading:first-child h1,.section>.component-heading:first-child h2,.section>.component-heading:first-child h4{color:var(--accent)}',
    '.section>.component:after{left:16px;right:16px;background:var(--line)}',
    '.component-text p{margin:0;font-size:14px;line-height:1.5;color:var(--muted);overflow-wrap:break-word;word-wrap:break-word}',
    'label{min-height:28px;padding:0;gap:14px;color:var(--ink);font-size:15px;line-height:1.4}',
    'label>.label{min-width:0;flex:1;overflow-wrap:break-word;word-wrap:break-word}',
    'label>.input{flex-shrink:0;margin-left:0}',
    '.description{padding:0;margin-top:7px;color:var(--muted);font-size:13px;line-height:1.45;overflow-wrap:break-word;word-wrap:break-word}',
    '.component-toggle label{min-height:44px}',
    '.component-toggle input{display:block;position:absolute;opacity:0;width:44px;height:44px;right:12px;top:13px;margin:0}',
    '.component-toggle input:focus-visible+.graphic{outline:2px solid var(--accent);outline-offset:5px;border-radius:16px}',
    '.component-select:focus-within .value,.component-color label:focus-visible{outline:2px solid var(--accent);outline-offset:3px}',
    '.color-box.selectable:focus-visible{outline:3px solid var(--ink);outline-offset:-3px}',
    '.component-toggle .graphic .slide{background:var(--track)}',
    '.component-toggle .graphic .marker{box-shadow:0 1px 3px #0003}',
    '.component-toggle input:checked+.graphic .slide,.component-toggle input:checked+.graphic .marker{background:#145ca3}',
    '.component-color .value{flex-shrink:0;border:1px solid #8886;box-shadow:none}',
    '.component-color .picker-wrap .picker{max-width:416px;background:var(--card);border:1px solid var(--line);border-radius:14px}',
    '.component-input label{display:block}',
    '.component-input label>.label{display:block;padding:0}',
    '.component-input .input{display:block;min-width:100%;max-width:none;margin:9px 0 0}',
    '.component-input input{min-height:44px;padding:10px 12px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--ink);font-size:15px}',
    '.component-input input:focus{outline:2px solid var(--accent);outline-offset:1px;border:1px solid var(--line)}',
    '.component-select label,.dynamic-choice label{display:block}',
    '.component-select .value{margin-top:9px;padding:10px 28px 10px 12px;min-height:44px;border:1px solid var(--line);border-radius:8px;background:var(--field);font-size:15px}',
    '.component-select .value:after{right:12px;border-top-color:var(--accent)}',
    '.dynamic-choice select{display:block;width:100%;margin-top:9px;min-height:44px;padding:10px 12px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--ink);font:inherit;font-size:15px}',
    '.dynamic-choice select:focus-visible,button:focus-visible,summary:focus-visible{outline:2px solid var(--accent);outline-offset:3px}',
    '.dynamic-preview-tabs{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:6px;margin:0 0 16px}',
    '.dynamic-preview-tabs button{min-width:0;width:100%;margin:0;text-transform:none;letter-spacing:0;box-shadow:none;min-height:40px;padding:7px 8px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--ink);font:600 13px/1.3 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;white-space:normal}',
    '.dynamic-preview-tabs button[aria-selected=true]{background:#145ca3;border-color:#145ca3;color:#fff}',
    '.dynamic-preview-stage{width:212px;max-width:100%;margin:0 auto 14px;border:6px solid #101216;border-radius:12px;overflow:hidden;background:#fff;line-height:0}',
    '.dynamic-preview-stage canvas{display:block;width:200px;height:228px;max-width:100%;image-rendering:pixelated}',
    '.dynamic-preview-note{margin:7px 0 0!important;font-size:12px!important}',
    '.dynamic-reading-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}',
    '.dynamic-reading{padding:12px;border:1px solid var(--line);border-radius:10px;background:var(--field);min-width:0}',
    '.dynamic-reading small,.dynamic-health small{display:block;font-size:12px;color:var(--muted)}',
    '.dynamic-reading strong{display:block;margin:4px 0;font-size:15px;color:var(--ink);overflow-wrap:anywhere}',
    '.dynamic-reading .dynamic-example{font-size:19px;line-height:1.2;color:var(--ink);margin:8px 0}',
    '.dynamic-state{display:block;font-size:12px;color:var(--muted)}',
    '.dynamic-health{display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:10px 0;color:var(--ink);font-size:14px}',
    '.dynamic-health+.dynamic-health{border-top:1px solid var(--line)}',
    '.dynamic-health .dynamic-state{text-align:right;max-width:60%}',
    '.dynamic-health-help{font-size:12px!important;margin-top:4px!important}',
    '.dynamic-details{padding:13px 16px;border-top:1px solid var(--line)}',
    '.dynamic-details summary{min-height:32px;color:var(--ink);font-size:14px;cursor:pointer}',
    '.dynamic-details .component{padding:10px 0!important}',
    '.dynamic-details p{font-size:12px;line-height:1.5;color:var(--muted);overflow-wrap:anywhere}',
    '.dynamic-warning p{color:#a84d00}',
    '@media(prefers-color-scheme:dark){.dynamic-warning p{color:#ffbd77}}',
    '.component-submit{position:fixed;left:0;right:0;bottom:0;max-width:512px;margin:0 auto;padding:10px 16px;padding-bottom:calc(10px + env(safe-area-inset-bottom));z-index:20;background:var(--page);border-top:1px solid var(--line);box-shadow:0 -6px 18px #00000008}',
    'body.dynamic-picker-open .component-submit{visibility:hidden}',
    '.component-input input{scroll-margin-bottom:104px}',
    '.component-submit button{display:block;width:100%;min-width:0;min-height:48px;margin:0;padding:12px 16px;border:0;border-radius:12px;background:#145ca3;color:#fff;font:700 16px/1.4 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;text-transform:none;letter-spacing:0}',
    '.component-submit button:disabled{opacity:.65}',
    '[hidden]{display:none!important}',
    'a{color:var(--accent)}',
    '@media(max-width:340px){body{padding-left:12px;padding-right:12px}.section>.component{padding-left:12px;padding-right:12px}.dynamic-reading-grid{gap:6px}.dynamic-reading{padding:9px}.component-submit{padding-left:12px;padding-right:12px}}',
    '@media(prefers-reduced-motion:reduce){*{transition:none!important}}'
  ].join('\n');
  document.head.appendChild(style);
  var viewport = document.querySelector('meta[name=viewport]');
  if (viewport) { viewport.setAttribute('content','width=device-width,initial-scale=1'); }

  clay.on(clay.EVENTS.AFTER_BUILD, function() {
    function item(id) { return clay.getItemById(id); }
    function field(key) { return clay.getItemByMessageKey(key); }
    function node(value) { return value && value.$element && value.$element[0]; }
    function clear(element) { while (element.firstChild) { element.removeChild(element.firstChild); } }
    function make(tag, text, className) {
      var element = document.createElement(tag);
      if (text !== undefined && text !== null) { element.textContent = String(text); }
      if (className) { element.className = className; }
      return element;
    }
    function transport(id) {
      var element = node(item(id)), value = {};
      if (element) {
        try { value = JSON.parse(element.textContent || '{}'); } catch (ignored) {}
        element.hidden = true;
      }
      return value && typeof value === 'object' ? value : {};
    }
    var data = transport('dynamic-settings-data');
    var words = transport('dynamic-settings-strings');
    if (typeof data.language === 'string') { document.documentElement.lang = data.language; }
    var assets = transport('dynamic-preview-assets');
    function t(key,params) {
      var text = typeof words[key] === 'string' ? words[key] : key;
      return text.replace(/\{([a-zA-Z][a-zA-Z0-9_]*)\}/g,function(match,name) { return params && params[name] !== undefined ? String(params[name]) : match; });
    }
    function show(value, visible) { var element = node(value); if (element) { element.hidden = !visible; } }
    function choice(id, label, options, value) {
      var host = node(item(id));
      if (!host) { return null; }
      clear(host); host.className += ' dynamic-choice';
      var wrapper = make('label'), title = make('span', label), select = make('select');
      select.id = id + '-select';
      wrapper.setAttribute('for', select.id);
      options.forEach(function(option) {
        var child = make('option', option[1]); child.value = option[0]; select.appendChild(child);
      });
      select.value = String(value); wrapper.appendChild(title); wrapper.appendChild(select); host.appendChild(wrapper);
      return select;
    }
    function state(value) {
      var names = {empty:'sourceEmpty',loading:'sourceLoading',ready:'sourceReady',unavailable:'sourceUnavailable',
        denied:'sourceDenied',unsupported:'sourceUnsupported',disabled:'sourceDisabled',playing:'sourcePlaying',
        idle:'sourceIdle',paused:'sourcePaused',off:'sourceOff',waiting:'sourceWaiting',permission:'sourcePermission'};
      return t(names[value] || 'sourceWaiting');
    }
    function nativeColor(value) {
      var number = typeof value === 'number' ? value : parseInt(String(value).replace(/^#|^0x/i,''),16);
      if (!isFinite(number)) { return 0; }
      return [16,8,0].reduce(function(result,shift) {
        return result | (Math.max(0,Math.min(3,Math.round(((number >> shift) & 255) / 85))) * 85 << shift);
      },0);
    }
    function color(key) { return nativeColor(field(key).get()); }
    function css(value) { return '#' + ('000000' + value.toString(16)).slice(-6); }
    clay.getItemsByType('color').forEach(function(value) {
      var element = node(value), opener = element && element.querySelector('label');
      if (!opener) { return; }
      opener.tabIndex = 0; opener.setAttribute('role','button'); opener.setAttribute('aria-haspopup','true'); opener.setAttribute('aria-expanded','false');
      opener.addEventListener('keydown',function(event) {
        if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); opener.click(); }
        if (event.key === 'Escape') { var picker = element.querySelector('.picker-wrap'); if (picker) { picker.classList.remove('show'); } }
      });
      Array.prototype.forEach.call(element.querySelectorAll('.color-box.selectable'),function(box) {
        box.tabIndex = 0; box.setAttribute('role','button');
        box.setAttribute('aria-label',css(Number(box.getAttribute('data-value'))));
        box.addEventListener('keydown',function(event) {
          if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); box.click(); opener.focus(); }
          if (event.key === 'Escape') { var picker = element.querySelector('.picker-wrap'); if (picker) { picker.classList.remove('show'); } opener.focus(); }
        });
      });
    });
    var themeChanging = false;
    var bg = field('BACKGROUND_COLOR'), fg = field('TEXT_COLOR'), accent = field('ACCENT_COLOR');
    var initialTheme = color('BACKGROUND_COLOR') === 0xFFFFFF && color('TEXT_COLOR') === 0 ? 'light' :
      color('BACKGROUND_COLOR') === 0 && color('TEXT_COLOR') === 0xFFFFFF ? 'dark' : 'custom';
    var theme = choice('dynamic-theme',t('theme'),[['light',t('themeLight')],['dark',t('themeDark')],['custom',t('themeCustom')]],initialTheme);
    function updateColors() {
      show(bg,theme.value === 'custom'); show(fg,theme.value === 'custom');
      var contrast = node(item('dynamic-contrast'));
      function luminance(value) {
        return [16,8,0].map(function(shift) { var channel = ((value >> shift) & 255) / 255;
          return channel <= .04045 ? channel / 12.92 : Math.pow((channel + .055) / 1.055,2.4);
        }).reduce(function(sum,channel,index) { return sum + channel * [.2126,.7152,.0722][index]; },0);
      }
      var a = luminance(color('BACKGROUND_COLOR')), b = luminance(color('TEXT_COLOR'));
      var low = (Math.max(a,b)+.05)/(Math.min(a,b)+.05) < 4.5;
      if (contrast) { contrast.className += contrast.className.indexOf('dynamic-warning') < 0 ? ' dynamic-warning' : '';
        contrast.hidden = !low; clear(contrast); contrast.appendChild(make('p',t('contrastHelp'))); }
      renderPreview();
    }
    if (theme) { theme.addEventListener('change',function() {
      themeChanging = true;
      if (theme.value === 'light') { bg.set(0xFFFFFF); fg.set(0); }
      if (theme.value === 'dark') { bg.set(0); fg.set(0xFFFFFF); }
      themeChanging = false; updateColors();
    }); }
    [bg,fg,accent].forEach(function(value) { if (value) { value.on('change',function() {
      if (!themeChanging && value !== accent) { theme.value = 'custom'; }
      updateColors();
    }); } });

    var notification = field('SHOW_NOTIFICATIONS'), preview = field('NOTIFICATION_PREVIEW'), apps = field('NOTIFICATION_APPS');
    var noticeMode = choice('dynamic-notification-mode',t('notifications'),[
      ['off',t('stateOff')],['summary',t('notificationSummary')],['preview',t('notificationPreview')]
    ],notification.get() ? preview.get() ? 'preview' : 'summary' : 'off');
    var appMode = choice('dynamic-app-mode',t('notificationAppsMode'),[
      ['all',t('allApps')],['selected',t('selectedApps')]
    ],String(apps.get() || '').trim() ? 'selected' : 'all');
    var retainedApps = String(apps.get() || '');
    show(notification,false); show(preview,false);
    function updateNotifications() {
      var enabled = noticeMode.value !== 'off';
      notification.set(enabled); preview.set(noticeMode.value === 'preview');
      show(item('dynamic-notification-help'),enabled);
      show(item('dynamic-app-mode'),enabled); show(apps,enabled && appMode.value === 'selected');
      renderPreview();
      var input = apps.$manipulatorTarget && apps.$manipulatorTarget[0];
      if (input) { input.required = enabled && appMode.value === 'selected';
        input.setCustomValidity(input.required && !String(apps.get() || '').split(',').some(function(name) { return name.trim(); }) ? t('appFilterRequired') : ''); }
    }
    noticeMode.addEventListener('change',updateNotifications);
    appMode.addEventListener('change',function() {
      if (appMode.value === 'all') { retainedApps = String(apps.get() || retainedApps); apps.set(''); }
      else { apps.set(retainedApps); }
      updateNotifications();
    });
    apps.on('input change',function() { if (appMode.value === 'selected') { retainedApps = String(apps.get() || ''); } updateNotifications(); });

    var readings = node(item('dynamic-readings'));
    if (readings) {
      clear(readings); var grid = make('div',null,'dynamic-reading-grid');
      [0,1].forEach(function(index) {
        var slot = (data.slots || [])[index] || {}, card = make('div',null,'dynamic-reading');
        card.appendChild(make('small',t(index ? 'rightReading' : 'leftReading')));
        var propertyKeys = {temperature:'readingTemperature',feels_like:'readingFeelsLike',high:'readingHigh',low:'readingLow',condition:'readingCondition',condition_code:'readingCondition',humidity:'readingHumidity',precipitation:'readingRain',uv_index:'readingUv',battery_level:'readingBattery',title:'readingTitle',artist:'readingArtist',name:'readingName'};
        var property = propertyKeys[slot.property] ? t(propertyKeys[slot.property]) : '';
        var label = slot.label && String(slot.label).toLowerCase() !== String(slot.property || '').toLowerCase() ? slot.label : property || slot.label;
        card.appendChild(make('strong',label || state(slot.status || 'empty')));
        if (property && property !== label) { card.appendChild(make('small',property)); }
        if (slot.value !== undefined && slot.value !== '') { card.appendChild(make('div',slot.value,'dynamic-example')); }
        if (label) { card.appendChild(make('span',state(slot.status || 'empty'),'dynamic-state')); }
        var age = typeof slot.observationAgeSec === 'number' && isFinite(slot.observationAgeSec) && slot.observationAgeSec >= 0 ? ['observedAge',slot.observationAgeSec] :
          typeof slot.receivedAgeSec === 'number' && isFinite(slot.receivedAgeSec) && slot.receivedAgeSec >= 0 ? ['receivedAge',slot.receivedAgeSec] : null;
        if (age) { card.appendChild(make('small',t(age[0],{age:Math.floor(age[1]/60)}))); }
        grid.appendChild(card);
      }); readings.appendChild(grid);
    }
    var health = node(item('dynamic-source-status'));
    if (health) {
      clear(health);
      [['music','musicSource'],['notifications','notificationsSource'],['phone','phoneSource']].forEach(function(pair) {
        var source = data.sources && data.sources[pair[0]] || {}, row = make('div',null,'dynamic-health');
        row.appendChild(make('span',t(pair[1]))); row.appendChild(make('span',state(pair[0] !== 'music' && source.state === 'idle' ? 'ready' : source.state),'dynamic-state'));
        health.appendChild(row);
        var recovery = source.message === 'DISCONNECTED' ? 'connectionHelp' : source.state === 'denied' || source.state === 'permission' ? 'permissionHelp' :
          source.state === 'disabled' || source.state === 'unavailable' ? 'pluginHelp' : '';
        if (recovery) { health.appendChild(make('p',t(recovery),'dynamic-health-help')); }
      });
    }
    var saveStatus = node(item('dynamic-save-status'));
    if (saveStatus) {
      clear(saveStatus); var saveNames = {saved:'saveSaved',pending:'savePending',failed:'saveFailed',none:'saveNone'};
      saveStatus.appendChild(make('p',t(saveNames[data.save && data.save.state] || 'saveNone')));
    }
    var technical = node(item('complications-status')), context = node(item('context-status'));
    if (technical && context) {
      var details = make('details',null,'dynamic-details'); details.appendChild(make('summary',t('technicalDetails')));
      technical.parentNode.insertBefore(details,technical); details.appendChild(technical); details.appendChild(context);
      [['music','musicSource'],['notifications','notificationsSource'],['phone','phoneSource']].forEach(function(pair) {
        var source = data.sources && data.sources[pair[0]];
        if (source && source.message) { details.appendChild(make('p',t(pair[1]) + ': ' + source.message)); }
      });
      (data.slots || []).forEach(function(slot,index) {
        var lines = [t(index ? 'rightReading' : 'leftReading') + ': ' + [slot.category,slot.item,slot.property].filter(Boolean).join('/')];
        [['receivedAge','receivedAgeSec'],['observedAge','observationAgeSec']].forEach(function(pair) {
          var age = slot[pair[1]]; if (typeof age === 'number' && isFinite(age) && age >= 0) { lines.push(t(pair[0],{age:Math.floor(age/60)})); }
        });
        if (slot.help) { lines.push(slot.help); }
        details.appendChild(make('p',lines.join(' · ')));
      });
    }

    var previewHost = node(item('dynamic-preview')), canvas, previewCaption, timelineCaption, previewState = 'idle', request = 0, imageCache = {};
    if (previewHost) {
      clear(previewHost); var tabs = make('div',null,'dynamic-preview-tabs'); tabs.setAttribute('role','tablist');
      tabs.setAttribute('aria-label',t('samplePreview'));
      [['idle','previewIdle'],['music','previewMusic'],['notification','previewNotification'],['timeline','previewTimeline']].forEach(function(pair) {
        var button = make('button',t(pair[1])); button.type = 'button'; button.setAttribute('role','tab');
        button.setAttribute('aria-selected',pair[0] === previewState ? 'true' : 'false');
        button.tabIndex = pair[0] === previewState ? 0 : -1; button.setAttribute('aria-controls','dynamic-preview-canvas');
        button.addEventListener('click',function() { previewState = pair[0];
          Array.prototype.forEach.call(tabs.children,function(child) { child.setAttribute('aria-selected',child === button ? 'true' : 'false'); child.tabIndex = child === button ? 0 : -1; });
          renderPreview(); });
        button.addEventListener('keydown',function(event) {
          var shift = event.key === 'ArrowRight' || event.key === 'ArrowDown' ? 1 : event.key === 'ArrowLeft' || event.key === 'ArrowUp' ? -1 : 0;
          if (shift) { event.preventDefault(); var next = tabs.children[(Array.prototype.indexOf.call(tabs.children,button) + shift + 4) % 4]; next.click(); next.focus(); }
        }); tabs.appendChild(button);
      });
      previewHost.appendChild(tabs); var stage = make('div',null,'dynamic-preview-stage');
      canvas = make('canvas'); canvas.id = 'dynamic-preview-canvas'; canvas.width = 200; canvas.height = 228; canvas.setAttribute('role','img');
      stage.appendChild(canvas); previewHost.appendChild(stage);
      previewHost.appendChild(make('p',t('sampleHelp')));
      previewCaption = make('p',t('previewSettingsNote'),'dynamic-preview-note'); previewHost.appendChild(previewCaption);
      timelineCaption = make('p',t('previewTimelineHelp'),'dynamic-preview-note'); timelineCaption.hidden = true; previewHost.appendChild(timelineCaption);
    }
    function renderPreview() {
      if (!canvas) { return; }
      var size = field('LARGE_TEXT').get() ? 'large' : 'normal';
      var format = Number(field('CLOCK_FORMAT').get());
      if (format !== 12 && format !== 24) { format = Number(assets.systemFormat) === 12 ? 12 : 24; }
      var background = color('BACKGROUND_COLOR'), foreground = color('TEXT_COLOR'), highlight = color('ACCENT_COLOR');
      var exactTheme = highlight === 0x55AAFF && background === 0xFFFFFF && foreground === 0 ? 'light' :
        highlight === 0x55AAFF && background === 0 && foreground === 0xFFFFFF ? 'dark' : '';
      var sampleState = previewState === 'notification' && noticeMode.value !== 'preview' ? 'notification-summary' : previewState;
      var key = sampleState + '-' + size + '-' + format + '-' + (exactTheme || 'light');
      var frame = (assets.frames || {})[key];
      var ctx = canvas.getContext('2d'), serial = ++request;
      canvas.setAttribute('aria-label',t('samplePreview') + ': ' + t({idle:'previewIdle',music:'previewMusic',notification:'previewNotification',timeline:'previewTimeline'}[previewState]));
      previewCaption.textContent = t(exactTheme ? 'previewSettingsNote' : 'previewCustomHelp');
      timelineCaption.hidden = previewState !== 'timeline';
      ctx.fillStyle = css(background); ctx.fillRect(0,0,200,228);
      if (!frame) { previewCaption.textContent = t('noExample'); return; }
      if (typeof frame === 'string') { frame = {src:frame}; }
      function draw(image) {
        if (serial !== request) { return; }
        if (image.naturalWidth !== 200 || image.naturalHeight !== 228) { previewCaption.textContent = t('noExample'); return; }
        ctx.drawImage(image,0,0,200,228);
        if (exactTheme) { return; }
        // Custom colours reuse the actual native geometry. Reverse-mapping a
        // quantised capture cannot recover every coverage sample: the page
        // explicitly identifies these colours as approximate. Provider images
        // and the OS-owned Timeline card retain their captured pixels.
        var pixels = ctx.getImageData(0,0,200,228), keep = frame.preserve || [];
        function blend(front, back, amount) {
          var result = 0;
          [16,8,0].forEach(function(shift) {
            var a = ((front >> shift) & 255) / 85, b = ((back >> shift) & 255) / 85;
            result |= Math.max(0,Math.min(3,Math.floor((a * amount + b * (16-amount) + 8) / 16))) * 85 << shift;
          });
          return result;
        }
        for (var y=0;y<228;y++) { for (var x=0;x<200;x++) {
          var preserve = keep.some(function(rect) { return x >= rect.x && x < rect.x + rect.width && y >= rect.y && y < rect.y + rect.height; });
          if (preserve) { continue; }
          var i=(y*200+x)*4, r=pixels.data[i]/85, g=pixels.data[i+1]/85, b=pixels.data[i+2]/85, mapped;
          if (r === g && g === b) { mapped = blend(foreground,background,Math.round((3-r)*16/3)); }
          else if (b === 3 && r < g && g <= b) { mapped = blend(highlight,background,Math.round((3-r)*8)); }
          else if (r === 3 && g === 3 && b < 3) { mapped = blend(0xFFFF00,background,Math.round((3-b)*16/3)); }
          else { continue; }
          pixels.data[i]=(mapped>>16)&255; pixels.data[i+1]=(mapped>>8)&255; pixels.data[i+2]=mapped&255;
        } }
        ctx.putImageData(pixels,0,0);
      }
      if (imageCache[key] && imageCache[key].complete) { draw(imageCache[key]); return; }
      var image = new Image(); imageCache[key] = image; image.onload = function() { draw(image); }; image.onerror = function() { if (serial === request) { previewCaption.textContent = t('noExample'); } }; image.src = frame.src;
    }
    ['LARGE_TEXT','CLOCK_FORMAT'].forEach(function(key) { field(key).on('change',renderPreview); });
    updateNotifications(); updateColors();
    var form = document.getElementById('main-form');
    if (form) {
      function saveVisibility() {
        document.body.classList.toggle('dynamic-picker-open',!!document.querySelector('.picker-wrap.show'));
        clay.getItemsByType('color').forEach(function(value) { var element = node(value), opener = element && element.querySelector('label');
          if (opener) { opener.setAttribute('aria-expanded',element.querySelector('.picker-wrap.show') ? 'true' : 'false'); }
        });
      }
      if (typeof MutationObserver !== 'undefined') { new MutationObserver(saveVisibility).observe(form,{subtree:true,attributes:true,attributeFilter:['class']}); }
      else { document.addEventListener('click',function() { setTimeout(saveVisibility,0); }); }
      form.addEventListener('focusin',function(event) {
        if (event.target.tagName !== 'INPUT' || event.target.type === 'checkbox' || event.target.type === 'hidden') { return; }
        setTimeout(function() { var rect = event.target.getBoundingClientRect();
          if (rect.bottom > window.innerHeight - 100 || rect.top < 0) { event.target.scrollIntoView({block:'center'}); }
        },0);
      });
      form.addEventListener('submit',function() {
      if (!form.checkValidity()) { return; }
      var button = form.querySelector('.component-submit button');
      if (button) { button.textContent = t('saveSending'); button.disabled = true; }
    }); }
  });
};
