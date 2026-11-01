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
    'html{font-size:16px!important;height:auto;background:var(--page)!important;color:var(--ink)!important}',
    'body{box-sizing:border-box;height:auto;max-width:512px;margin:0 auto;padding:20px 16px 140px;padding-bottom:calc(140px + env(safe-area-inset-bottom));background:var(--page);color:var(--ink);font:16px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}',
    '*,*:before,*:after{box-sizing:border-box}',
    // Clay adds component styles after this hook. Scope colour rules to the
    // form so late defaults cannot reintroduce white inputs or orange text.
    'body{background:var(--page)!important;color:var(--ink)!important}',
    '#main-form{color:var(--ink);font:inherit}',
    '#main-form .section{background:var(--card);color:var(--ink)}',
    '#main-form label,#main-form .label,#main-form .component-select .value{color:var(--ink);font-family:inherit}',
    '#main-form strong{color:var(--ink);font-family:inherit}',
    '#main-form .description,#main-form .component-text p{color:var(--muted)}',
    '#main-form input,#main-form select,#main-form option{color:var(--ink);background-color:var(--field);font-family:inherit}',
    '#main-form input::placeholder{color:var(--muted);opacity:1}',
    '#main-form .component-select .value{background:var(--field)}',
    '#main-form .component-toggle input{background:none}',
    '#main-form .component-toggle .graphic .marker{background:#fff;border:1px solid #59636f}',
    '#main-form .component-toggle input:checked+.graphic .marker{background:#fff;border-color:#fff}',
    '#main-form .component-color .picker-wrap .picker{background:var(--card);color:var(--ink)}',
    '#main-form .dynamic-save-state p{color:var(--muted)}',
    '#main-form .dynamic-warning p{color:#934300}',
    '@media(prefers-color-scheme:dark){#main-form .dynamic-warning p{color:#ffbd77}}',
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
    '.dynamic-preview-tabs{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:6px;margin:0 0 16px}',
    '.dynamic-preview-tabs button{min-width:0;width:100%;margin:0;text-transform:none;letter-spacing:0;box-shadow:none;min-height:40px;padding:7px 8px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--ink);font:600 13px/1.3 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;white-space:normal}',
    '.dynamic-preview-tabs button[aria-selected=true]{background:#145ca3;border-color:#145ca3;color:#fff}',
    '.dynamic-preview-stage{width:212px;max-width:100%;margin:0 auto 14px;border:6px solid #101216;border-radius:12px;overflow:hidden;background:#fff;line-height:0}',
    '.dynamic-preview-stage canvas{display:block;width:200px;height:228px;max-width:100%;image-rendering:pixelated}',
    '.dynamic-preview-note{margin:7px 0 0!important;font-size:12px!important}',
    '.dynamic-preview-title{margin:0 0 12px;color:var(--ink);font-size:15px}',
    '.dynamic-preview-palette{display:block;margin:0 0 14px}',
    '.dynamic-preview-palette>.description{display:block;margin-top:7px}',
    '.dynamic-example-disabled{padding:7px 9px;border-radius:7px;background:var(--field);color:var(--muted)}',
    '.dynamic-save-state{padding:0 0 7px!important;margin:0;color:var(--muted);font-size:12px;line-height:1.3}',
    '.dynamic-save-state p{margin:0;font-size:12px;line-height:1.3}',
    '.dynamic-save-state:after{display:none!important}',
    '.dynamic-palette-code label{display:block;margin-bottom:7px}',
    '.dynamic-palette-code input{display:block;width:100%;min-height:44px;padding:10px;border:1px solid var(--line);border-radius:8px;font:15px/1.4 ui-monospace,SFMono-Regular,Consolas,monospace}',
    '.dynamic-palette-actions{display:flex;gap:8px;margin-top:9px}',
    '.dynamic-palette-actions button{flex:1;min-width:0;width:auto;margin:0;min-height:44px;padding:9px 12px;border:1px solid var(--line);border-radius:8px;box-shadow:none;background:var(--field);color:var(--ink);font:600 14px/1.35 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;text-transform:none;letter-spacing:0}',
    '#main-form .dynamic-palette-code input:focus-visible{outline:2px solid var(--accent);outline-offset:2px}',
    '.dynamic-palette-code p{margin:7px 0 0!important}',
    '.dynamic-app-filter label{display:block;min-height:0;margin-bottom:7px}',
    '.dynamic-app-entry{display:flex;gap:7px;align-items:stretch}',
    '.dynamic-app-entry input{flex:1;min-width:0;width:0;padding:10px;border:1px solid var(--line);border-radius:8px;background:var(--field);color:var(--ink);font:inherit;font-size:15px}',
    '.dynamic-app-entry button,.dynamic-app-chip button{width:auto;margin:0;min-width:0;box-shadow:none;text-transform:none;letter-spacing:0;font:inherit}',
    '.dynamic-app-entry button{min-height:44px;padding:9px 13px;border:0;border-radius:8px;background:#145ca3;color:white}',
    '.dynamic-app-chips{display:flex;flex-wrap:wrap;gap:6px;margin:10px 0 0;padding:0;list-style:none}',
    '.dynamic-app-chip{display:flex;align-items:center;max-width:100%;padding:3px 3px 3px 10px;border:1px solid var(--line);border-radius:9px;background:var(--field);font-size:13px}',
    '.dynamic-app-chip span{min-width:0;overflow-wrap:anywhere}',
    '.dynamic-app-chip button{flex-shrink:0;min-height:36px;min-width:36px;padding:3px;border:0;background:none;color:var(--ink);font-size:20px}',
    '.dynamic-app-filter .dynamic-app-error{margin:7px 0 0;color:var(--ink);font-size:12px}',
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
    '.component-input input,.dynamic-app-entry input{scroll-margin-bottom:140px}',
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
        expired:'sourceExpired',denied:'sourceDenied',unsupported:'sourceUnsupported',disabled:'sourceDisabled',playing:'sourcePlaying',
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
        box.tabIndex = -1; box.setAttribute('role','button');
        box.setAttribute('aria-label',css(Number(box.getAttribute('data-value'))));
        box.addEventListener('keydown',function(event) {
          if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); box.click(); opener.focus(); }
          if (event.key === 'Escape') { var picker = element.querySelector('.picker-wrap'); if (picker) { picker.classList.remove('show'); } opener.focus(); }
        });
      });
    });
    var initialSnapshot = '', appEntry;
    function updateDirty() {}
    var themeChanging = false;
    var bg = field('BACKGROUND_COLOR'), fg = field('TEXT_COLOR'), accent = field('ACCENT_COLOR');
    var initialTheme = color('BACKGROUND_COLOR') === 0xFFFFFF && color('TEXT_COLOR') === 0 ? 'light' :
      color('BACKGROUND_COLOR') === 0 && color('TEXT_COLOR') === 0xFFFFFF ? 'dark' : 'custom';
    var customPalette = {background:bg.get(),foreground:fg.get()};
    var previousTheme = initialTheme;
    var paletteInput, paletteStatus;
    var night = field('NIGHT_PALETTE');
    var previewPalette;
    var theme = choice('dynamic-theme',t('theme'),[['light',t('themeLight')],['dark',t('themeDark')],['custom',t('themeCustom')]],initialTheme);
    function updateColors() {
      show(bg,theme.value === 'custom'); show(fg,theme.value === 'custom');
      var contrast = node(item('dynamic-contrast'));
      function luminance(value) {
        return [16,8,0].map(function(shift) { var channel = ((value >> shift) & 255) / 255;
          return channel <= .04045 ? channel / 12.92 : Math.pow((channel + .055) / 1.055,2.4);
        }).reduce(function(sum,channel,index) { return sum + channel * [.2126,.7152,.0722][index]; },0);
      }
      var background = color('BACKGROUND_COLOR'), foreground = color('TEXT_COLOR');
      function ratio(first,second) { var a=luminance(first),b=luminance(second); return (Math.max(a,b)+.05)/(Math.min(a,b)+.05); }
      var secondary = [16,8,0].reduce(function(result,shift) {
        var front=(foreground>>shift)&255,back=(background>>shift)&255;
        return result | (Math.floor((2*(front/85)+(back/85)+1)/3)*85 << shift);
      },0);
      var low = ratio(background,foreground) < 4.5 || night.get() && ratio(color('NIGHT_BACKGROUND_COLOR'),color('NIGHT_TEXT_COLOR')) < 4.5;
      var adjusted = ratio(background,secondary) < 4.5 || ratio(background,color('ACCENT_COLOR')) < 2 ||
        night.get() && ratio(color('NIGHT_BACKGROUND_COLOR'),color('NIGHT_ACCENT_COLOR')) < 2;
      if (contrast) { contrast.className += contrast.className.indexOf('dynamic-warning') < 0 ? ' dynamic-warning' : '';
        contrast.hidden = !low && !adjusted; clear(contrast); contrast.appendChild(make('p',t(low ? 'contrastHelp' : 'accentContrastHelp'))); }
      updatePaletteCode(); renderPreview();
    }
    if (theme) { theme.addEventListener('change',function() {
      if (previousTheme === 'custom') { customPalette = {background:bg.get(),foreground:fg.get()}; }
      themeChanging = true;
      if (theme.value === 'light') { bg.set(0xFFFFFF); fg.set(0); }
      if (theme.value === 'dark') { bg.set(0); fg.set(0xFFFFFF); }
      if (theme.value === 'custom') { bg.set(customPalette.background); fg.set(customPalette.foreground); }
      previousTheme = theme.value;
      themeChanging = false; updateColors(); updateDirty();
    }); }
    [bg,fg,accent].forEach(function(value) { if (value) { value.on('change',function() {
      if (!themeChanging && value !== accent) { theme.value = previousTheme = 'custom'; customPalette = {background:bg.get(),foreground:fg.get()}; }
      updateColors();
    }); } });

    function paletteCode() {
      return 'DYN1-' + ['BACKGROUND_COLOR','TEXT_COLOR','ACCENT_COLOR'].map(function(key) { return css(color(key)).slice(1).toUpperCase(); }).join('-');
    }
    function updatePaletteCode() {
      if (paletteInput) { paletteInput.value = paletteCode(); paletteInput.setCustomValidity(''); }
      if (paletteStatus) { paletteStatus.textContent = ''; }
    }
    var paletteHost = node(item('dynamic-palette-code'));
    if (paletteHost) {
      clear(paletteHost); paletteHost.className += ' dynamic-palette-code';
      var paletteLabel = make('label',t('paletteCode')); paletteLabel.setAttribute('for','dynamic-palette-input'); paletteHost.appendChild(paletteLabel);
      paletteInput = make('input'); paletteInput.id = 'dynamic-palette-input'; paletteInput.type = 'text'; paletteInput.maxLength = 32;
      paletteInput.autocomplete = 'off'; paletteInput.spellcheck = false; paletteInput.setAttribute('autocapitalize','characters');
      paletteInput.setAttribute('aria-label',t('paletteInput')); paletteInput.setAttribute('aria-describedby','dynamic-palette-help dynamic-palette-status');
      paletteHost.appendChild(paletteInput);
      var paletteActions = make('div',null,'dynamic-palette-actions');
      var copyPalette = make('button',t('paletteCopy')), applyPalette = make('button',t('paletteApply'));
      copyPalette.type = applyPalette.type = 'button'; copyPalette.id = 'dynamic-palette-copy'; applyPalette.id = 'dynamic-palette-apply';
      paletteActions.appendChild(copyPalette); paletteActions.appendChild(applyPalette); paletteHost.appendChild(paletteActions);
      var paletteHelp = make('p',t('paletteCodeHelp'),'description'); paletteHelp.id = 'dynamic-palette-help'; paletteHost.appendChild(paletteHelp);
      paletteStatus = make('p','','description'); paletteStatus.id = 'dynamic-palette-status'; paletteStatus.setAttribute('role','status'); paletteStatus.setAttribute('aria-live','polite'); paletteHost.appendChild(paletteStatus);
      paletteInput.addEventListener('input',function() { paletteInput.setCustomValidity(''); paletteStatus.textContent = ''; });
      applyPalette.addEventListener('click',function() {
        var match = /^DYN1-([0-9A-F]{6})-([0-9A-F]{6})-([0-9A-F]{6})$/i.exec(paletteInput.value.trim());
        if (!match) { paletteStatus.textContent = t('paletteInvalid'); paletteInput.focus(); return; }
        themeChanging = true;
        [bg,fg,accent].forEach(function(value,index) { value.set(nativeColor(parseInt(match[index+1],16))); });
        theme.value = previousTheme = 'custom'; customPalette = {background:bg.get(),foreground:fg.get()};
        themeChanging = false; updateColors(); updateDirty(); paletteStatus.textContent = t('paletteApplied');
      });
      copyPalette.addEventListener('click',function() {
        updatePaletteCode(); paletteInput.focus(); paletteInput.select();
        if (typeof navigator !== 'undefined' && navigator.clipboard && typeof navigator.clipboard.writeText === 'function') {
          navigator.clipboard.writeText(paletteInput.value).then(function() { paletteStatus.textContent = t('paletteCopied'); },function() { paletteStatus.textContent = t('paletteSelectCopy'); });
        } else {
          var copied = false; try { copied = document.execCommand('copy'); } catch (ignored) {}
          paletteStatus.textContent = t(copied ? 'paletteCopied' : 'paletteSelectCopy');
        }
      });
      updatePaletteCode();
    }
    function updateNight() {
      ['NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR'].forEach(function(key) { show(field(key),!!night.get()); });
      if (previewPalette) {
        previewPalette.parentNode.hidden = !night.get();
        if (!night.get()) { previewPalette.value = 'day'; }
      }
      updateColors(); updateDirty();
    }
    night.on('change',updateNight);
    ['NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR'].forEach(function(key) { field(key).on('change',updateColors); });
    updateNight();

    var notification = field('SHOW_NOTIFICATIONS'), preview = field('NOTIFICATION_PREVIEW'), appOnly = field('NOTIFICATION_APP_ONLY'), vibration = field('NOTIFICATION_VIBRATION'), apps = field('NOTIFICATION_APPS');
    var noticeMode = choice('dynamic-notification-mode',t('watchfaceBanners'),[
      ['off',t('stateOff')],['app',t('notificationAppOnly')],['summary',t('notificationSummary')],['preview',t('notificationPreview')]
    ],notification.get() ? appOnly.get() ? 'app' : preview.get() ? 'preview' : 'summary' : 'off');
    var appMode = choice('dynamic-app-mode',t('notificationAppsMode'),[
      ['all',t('allApps')],['selected',t('selectedApps')]
    ],String(apps.get() || '').trim() ? 'selected' : 'all');
    function parsedApps(value) {
      var seen = Object.create(null); return String(value || '').split(',').map(function(name) { return name.trim(); }).filter(function(name) {
        if (!name || seen[name.toLowerCase()]) { return false; } seen[name.toLowerCase()] = true; return true;
      });
    }
    var entries = parsedApps(apps.get()), appHost = make('div',null,'component dynamic-app-filter');
    var appLabel = make('label',t('appEntryLabel')); appLabel.setAttribute('for','dynamic-app-entry'); appHost.appendChild(appLabel);
    var entryRow = make('div',null,'dynamic-app-entry');
    appEntry = make('input'); appEntry.id = 'dynamic-app-entry'; appEntry.type = 'text'; appEntry.maxLength = 160;
    appEntry.autocomplete = 'off'; appEntry.setAttribute('autocapitalize','none'); appEntry.spellcheck = false;
    var addApp = make('button',t('appAdd')); addApp.type = 'button'; addApp.id = 'dynamic-app-add';
    entryRow.appendChild(appEntry); entryRow.appendChild(addApp); appHost.appendChild(entryRow);
    var chips = make('ul',null,'dynamic-app-chips'); chips.setAttribute('aria-label',t('selectedApps')); appHost.appendChild(chips);
    var appHelp = make('p',t('appEntryHelp'),'description'); appHelp.id='dynamic-app-help';appHost.appendChild(appHelp);
    appEntry.setAttribute('aria-describedby','dynamic-app-help dynamic-app-error');
    var appError = make('p','','dynamic-app-error'); appError.id='dynamic-app-error';appError.setAttribute('aria-live','polite');appHost.appendChild(appError);
    var canonicalInput = apps.$manipulatorTarget && apps.$manipulatorTarget[0];
    if (canonicalInput) { canonicalInput.required = false; canonicalInput.setCustomValidity(''); }
    var appNode = node(apps); appNode.parentNode.insertBefore(appHost,appNode);
    show(notification,false); show(preview,false); show(appOnly,false); show(apps,false);
    function appValidity(message) {
      var enabled = noticeMode.value !== 'off' && appMode.value === 'selected';
      appEntry.required = enabled && !entries.length;
      if (enabled && !entries.length && !appEntry.value.trim() && !message) { message = t('appFilterRequired'); }
      appEntry.setCustomValidity(enabled ? message || '' : '');
      appError.textContent = enabled ? message || '' : '';
    }
    function draftAppError() {
      var value = appEntry.value.trim();
      if (value && /[,\x00-\x1f\x7f]/.test(value)) { return t('appInvalid'); }
      if (value && entries.some(function(name) { return name.toLowerCase() === value.toLowerCase(); })) { return t('appDuplicate'); }
      if (entries.concat(value ? [value] : []).join(', ').length > 160) { return t('appLimit'); }
      return '';
    }
    function renderApps() {
      clear(chips);
      entries.forEach(function(name,index) {
        var chip = make('li',null,'dynamic-app-chip'), text = make('span',name), remove = make('button','×');
        remove.type='button'; remove.setAttribute('aria-label',t('appRemove',{app:name}));
        remove.addEventListener('click',function() { entries.splice(index,1); renderApps(); appEntry.focus(); updateDirty(); });
        chip.appendChild(text);chip.appendChild(remove);chips.appendChild(chip);
      });
      apps.set(appMode.value === 'selected' ? entries.join(', ') : '');
      appValidity(draftAppError()); updateDirty();
    }
    function commitApp(report) {
      var value = appEntry.value.trim(), error = draftAppError();
      if (error) { appValidity(error); if (report) { appEntry.reportValidity(); } return false; }
      if (!value) { appValidity(''); if (report && !entries.length) { appEntry.reportValidity(); } return entries.length > 0; }
      entries.push(value);appEntry.value='';renderApps();return true;
    }
    addApp.addEventListener('click',function() { commitApp(true); });
    appEntry.addEventListener('keydown',function(event) { if (event.key === 'Enter') { event.preventDefault();commitApp(true); } });
    appEntry.addEventListener('input',function() { appValidity(draftAppError()); updateDirty(); });
    function updateNotifications() {
      var enabled = noticeMode.value !== 'off';
      notification.set(enabled);
      if (enabled) {
        appOnly.set(noticeMode.value === 'app');
        if (noticeMode.value !== 'app') { preview.set(noticeMode.value === 'preview'); }
      }
      show(vibration,enabled);
      show(item('dynamic-notification-help'),enabled);
      var privacy = node(item('dynamic-notification-help'));
      if (privacy) { clear(privacy); privacy.appendChild(make('p',t('notificationsHelp')));
        if (noticeMode.value === 'summary') { privacy.appendChild(make('p',t('notificationSummaryHelp'))); }
        if (noticeMode.value === 'app') { privacy.appendChild(make('p',t('notificationAppOnlyHelp'))); }
      }
      show(item('dynamic-app-mode'),enabled); appHost.hidden = !enabled || appMode.value !== 'selected';
      appValidity(draftAppError()); renderPreview(); updateDirty();
    }
    noticeMode.addEventListener('change',updateNotifications);
    appMode.addEventListener('change',function() { renderApps(); updateNotifications(); });
    renderApps();

    var readings = node(item('dynamic-readings'));
    if (readings) {
      clear(readings); var grid = make('div',null,'dynamic-reading-grid');
      [0,1].forEach(function(index) {
        var slot = (data.slots || [])[index] || {}, card = make('div',null,'dynamic-reading');
        card.appendChild(make('small',t(index ? 'rightReading' : 'leftReading')));
        var propertyKeys = {temperature:'readingTemperature',feels_like:'readingFeelsLike',high:'readingHigh',low:'readingLow',condition:'readingCondition',condition_code:'readingCondition',humidity:'readingHumidity',wind_speed:'readingWindSpeed',wind_direction:'readingWindDirection',precipitation:'readingRain',uv_index:'readingUv',battery_level:'readingBattery',title:'readingTitle',artist:'readingArtist',name:'readingName'};
        var property = slot.category === 'phone' && slot.property === 'battery_level' ? t('phoneSource') : propertyKeys[slot.property] ? t(propertyKeys[slot.property]) : '';
        var label = slot.label && String(slot.label).toLowerCase() !== String(slot.property || '').toLowerCase() ? slot.label : property || slot.label;
        card.appendChild(make('strong',label || state(slot.status || 'empty')));
        if (property && property !== label) { card.appendChild(make('small',property)); }
        if (slot.value !== undefined && slot.value !== '') { card.appendChild(make('div',slot.value,'dynamic-example')); }
        if (label) { card.appendChild(make('span',state(slot.status || 'empty'),'dynamic-state')); }
        var age = typeof slot.observationAgeSec === 'number' && isFinite(slot.observationAgeSec) && slot.observationAgeSec >= 0 ? ['observedAge',slot.observationAgeSec] :
          typeof slot.receivedAgeSec === 'number' && isFinite(slot.receivedAgeSec) && slot.receivedAgeSec >= 0 ? ['receivedAge',slot.receivedAgeSec] : null;
        if (age) { card.appendChild(make('small',age[1] < 60 ? t(age[0] === 'observedAge' ? 'justObserved' : 'justReceived') : t(age[0],{age:Math.floor(age[1]/60)}))); }
        var recovery = slot.status === 'denied' || slot.status === 'permission' ? 'permissionHelp' :
          slot.status === 'empty' || !slot.status ? 'readingSetupHelp' :
          ['expired','unavailable','unsupported','disabled'].indexOf(slot.status) >= 0 ? 'sourceNoUpdateHelp' : '';
        if (recovery) { card.appendChild(make('small',t(recovery))); }
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
    var submitHost = document.querySelector('.component-submit');
    if (saveStatus && submitHost) {
      saveStatus.className += ' dynamic-save-state'; saveStatus.setAttribute('role','status'); saveStatus.setAttribute('aria-live','polite');
      submitHost.insertBefore(saveStatus,submitHost.firstChild);
    }
    function signature() {
      return JSON.stringify(['BACKGROUND_COLOR','TEXT_COLOR','ACCENT_COLOR','CLOCK_FORMAT','ANIMATIONS','SHOW_STATUS','SHOW_MUSIC','SHOW_NOTIFICATIONS','NOTIFICATION_PREVIEW','NOTIFICATION_APP_ONLY','NOTIFICATION_VIBRATION','NOTIFICATION_APPS','NIGHT_PALETTE','NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR'].map(function(key) { return field(key).get(); }).concat([noticeMode.value !== 'off' && appMode.value === 'selected' ? appEntry.value.trim() : '']));
    }
    updateDirty = function() {
      if (!initialSnapshot) { return; }
      var dirty = signature() !== initialSnapshot;
      var saved = data.save && data.save.state || 'none';
      var key = dirty ? 'saveUnsaved' : saved === 'saved' ? 'saveLastDelivered' : saved === 'pending' || saved === 'failed' ? 'savePhoneWaiting' : 'saveNone';
      if (saveStatus) { clear(saveStatus); saveStatus.appendChild(make('p',t(key))); }
      var button = submitHost && submitHost.querySelector('button');
      if (button) { button.disabled = !dirty && saved !== 'none'; }
    };
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

    var previewHost = node(item('dynamic-preview')), canvas, previewCaption, timelineCaption, disabledCaption, formatCaption, previewState = 'idle', request = 0, imageCache = {};
    if (previewHost) {
      clear(previewHost); previewHost.appendChild(make('h3',t('layoutPreview'),'dynamic-preview-title')); var tabs = make('div',null,'dynamic-preview-tabs'); tabs.setAttribute('role','tablist');
      tabs.setAttribute('aria-label',t('samplePreview'));
      [['idle','previewIdle'],['music','previewMusic'],['notification-summary','previewBanner'],['notification','previewMessage'],['timeline','previewTimeline']].forEach(function(pair) {
        var button = make('button',t(pair[1])); button.type = 'button'; button.setAttribute('role','tab');
        button.setAttribute('aria-selected',pair[0] === previewState ? 'true' : 'false');
        button.tabIndex = pair[0] === previewState ? 0 : -1; button.setAttribute('aria-controls','dynamic-preview-canvas');
        button.addEventListener('click',function() { previewState = pair[0];
          Array.prototype.forEach.call(tabs.children,function(child) { child.setAttribute('aria-selected',child === button ? 'true' : 'false'); child.tabIndex = child === button ? 0 : -1; });
          renderPreview(); });
        button.addEventListener('keydown',function(event) {
          var shift = event.key === 'ArrowRight' || event.key === 'ArrowDown' ? 1 : event.key === 'ArrowLeft' || event.key === 'ArrowUp' ? -1 : 0;
          if (shift) { event.preventDefault(); var next = tabs.children[(Array.prototype.indexOf.call(tabs.children,button) + shift + 5) % 5]; next.click(); next.focus(); }
        }); tabs.appendChild(button);
      });
      previewHost.appendChild(tabs); var stage = make('div',null,'dynamic-preview-stage');
      var paletteChoice = make('label',null,'dynamic-choice dynamic-preview-palette');
      paletteChoice.setAttribute('for','dynamic-preview-palette-select');
      paletteChoice.appendChild(make('span',t('previewPalette')));
      previewPalette = make('select'); previewPalette.id = 'dynamic-preview-palette-select';
      [['day','previewDay'],['quiet','previewQuietTime']].forEach(function(pair) {
        var option = make('option',t(pair[1])); option.value = pair[0]; previewPalette.appendChild(option);
      });
      previewPalette.setAttribute('aria-describedby','dynamic-preview-palette-help');
      paletteChoice.appendChild(previewPalette);
      var paletteHelp = make('span',t('previewPaletteHelp'),'description'); paletteHelp.id = 'dynamic-preview-palette-help';
      paletteChoice.appendChild(paletteHelp); paletteChoice.hidden = !night.get();
      previewHost.appendChild(paletteChoice); previewPalette.addEventListener('change',renderPreview);
      canvas = make('canvas'); canvas.id = 'dynamic-preview-canvas'; canvas.width = 200; canvas.height = 228; canvas.setAttribute('role','img');
      stage.appendChild(canvas); previewHost.appendChild(stage);
      previewHost.appendChild(make('p',t('sampleHelp')));
      previewHost.appendChild(make('p',t('previewSafeHelp'),'dynamic-preview-note'));
      disabledCaption = make('p',t('previewDisabled'),'dynamic-preview-note dynamic-example-disabled'); previewHost.appendChild(disabledCaption);
      formatCaption = make('p',t('previewUnknownFormat'),'dynamic-preview-note'); previewHost.appendChild(formatCaption);
      previewCaption = make('p',t('previewSettingsNote'),'dynamic-preview-note'); previewHost.appendChild(previewCaption);
      timelineCaption = make('p',t('previewTimelineHelp'),'dynamic-preview-note'); timelineCaption.hidden = true; previewHost.appendChild(timelineCaption);
    }
    function renderPreview() {
      if (!canvas) { return; }
      var format = Number(field('CLOCK_FORMAT').get());
      if (format !== 12 && format !== 24) { format = Number(assets.systemFormat) === 12 ? 12 : 24; }
      var quietPreview = night.get() && previewPalette && previewPalette.value === 'quiet';
      var background = color(quietPreview ? 'NIGHT_BACKGROUND_COLOR' : 'BACKGROUND_COLOR');
      var foreground = color(quietPreview ? 'NIGHT_TEXT_COLOR' : 'TEXT_COLOR');
      var highlight = color(quietPreview ? 'NIGHT_ACCENT_COLOR' : 'ACCENT_COLOR');
      var exactTheme = highlight === 0x55AAFF && background === 0xFFFFFF && foreground === 0 ? 'light' :
        highlight === 0x55AAFF && background === 0 && foreground === 0xFFFFFF ? 'dark' : '';
      var sampleState = previewState;
      var key = sampleState + '-' + format + '-' + (exactTheme || 'light');
      var frame = (assets.frames || {})[key];
      var ctx = canvas.getContext('2d'), serial = ++request;
      canvas.setAttribute('aria-label',t('samplePreview') + ': ' + t({idle:'previewIdle',music:'previewMusic','notification-summary':'previewBanner',notification:'previewMessage',timeline:'previewTimeline'}[previewState]) +
        (night.get() ? ' · ' + t(quietPreview ? 'previewQuietTime' : 'previewDay') : ''));
      previewCaption.textContent = t(exactTheme ? 'previewSettingsNote' : 'previewCustomHelp');
      timelineCaption.hidden = previewState !== 'timeline';
      disabledCaption.hidden = !(previewState === 'music' && !field('SHOW_MUSIC').get() ||
        previewState === 'notification' && noticeMode.value !== 'preview' || previewState === 'notification-summary' && (noticeMode.value === 'off' || noticeMode.value === 'app'));
      formatCaption.hidden = Number(field('CLOCK_FORMAT').get()) !== 0 || Number(assets.systemFormat) === 12 || Number(assets.systemFormat) === 24;
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
    field('CLOCK_FORMAT').on('change',renderPreview);
    field('SHOW_MUSIC').on('change',renderPreview);
    updateNotifications(); updateColors();
    initialSnapshot = signature();
    ['BACKGROUND_COLOR','TEXT_COLOR','ACCENT_COLOR','CLOCK_FORMAT','ANIMATIONS','SHOW_STATUS','SHOW_MUSIC','SHOW_NOTIFICATIONS','NOTIFICATION_PREVIEW','NOTIFICATION_APP_ONLY','NOTIFICATION_VIBRATION','NOTIFICATION_APPS','NIGHT_PALETTE','NIGHT_BACKGROUND_COLOR','NIGHT_TEXT_COLOR','NIGHT_ACCENT_COLOR'].forEach(function(key) { field(key).on('input change',updateDirty); });
    updateDirty();
    var form = document.getElementById('main-form');
    if (form) {
      function saveVisibility() {
        document.body.classList.toggle('dynamic-picker-open',!!document.querySelector('.picker-wrap.show'));
        clay.getItemsByType('color').forEach(function(value) { var element = node(value), opener = element && element.querySelector('label');
          if (!element) { return; }
          var picker = element.querySelector('.picker-wrap'), open = !!element.querySelector('.picker-wrap.show');
          if (opener) { opener.setAttribute('aria-expanded',open ? 'true' : 'false'); }
          if (picker) {
            if (!open && picker.contains(document.activeElement) && opener) { opener.focus(); }
            picker.setAttribute('aria-hidden',open ? 'false' : 'true');
            Array.prototype.forEach.call(picker.querySelectorAll('.color-box.selectable'),function(box) { box.tabIndex = open ? 0 : -1; });
          }
        });
      }
      saveVisibility();
      if (typeof MutationObserver !== 'undefined') { new MutationObserver(saveVisibility).observe(form,{subtree:true,attributes:true,attributeFilter:['class']}); }
      else { document.addEventListener('click',function() { setTimeout(saveVisibility,0); }); }
      form.addEventListener('focusin',function(event) {
        if (event.target.tagName !== 'INPUT' || event.target.type === 'checkbox' || event.target.type === 'hidden') { return; }
        setTimeout(function() { var rect = event.target.getBoundingClientRect();
          if (rect.bottom > window.innerHeight - 140 || rect.top < 0) { event.target.scrollIntoView({block:'center'}); }
        },0);
      });
      form.addEventListener('submit',function(event) {
      if (noticeMode.value !== 'off' && appMode.value === 'selected' && !commitApp(true)) { event.preventDefault(); event.stopImmediatePropagation(); return; }
      if (!form.checkValidity()) { event.preventDefault(); event.stopImmediatePropagation(); return; }
      var button = form.querySelector('.component-submit button');
      if (button) { button.textContent = t('saveSending'); button.disabled = true; }
    },true); }
  });
};
