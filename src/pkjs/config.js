'use strict';

// Clay retains the established preference keys. Page-only choices translate to
// these fields, so notification privacy remains compatible with earlier builds.
module.exports = [
  {type:'heading',defaultValue:'Dynamic Settings'},
  {type:'section',items:[
    {type:'heading',defaultValue:'Appearance'},
    {type:'text',id:'dynamic-preview',defaultValue:'Layout & colour preview'},
    {type:'text',id:'dynamic-theme',defaultValue:'Theme'},
    {type:'color',messageKey:'BACKGROUND_COLOR',defaultValue:'0xFFFFFF',label:'Background color',layout:'COLOR',sunlight:false},
    {type:'color',messageKey:'TEXT_COLOR',defaultValue:'0x000000',label:'Text color',layout:'COLOR',sunlight:false},
    {type:'color',messageKey:'ACCENT_COLOR',defaultValue:'0x55AAFF',label:'Accent color',description:'Color for reading symbols and rain gauges.',layout:'COLOR',sunlight:false},
    {type:'text',id:'dynamic-contrast',defaultValue:''},
    {type:'text',id:'dynamic-palette-code',defaultValue:'Share colours'},
    {type:'toggle',messageKey:'NIGHT_PALETTE',defaultValue:false,label:'Quiet Time colours',description:'Use these watch colours while Pebble Quiet Time is active.'},
    {type:'color',messageKey:'NIGHT_BACKGROUND_COLOR',defaultValue:'0x000000',label:'Quiet Time background',layout:'COLOR',sunlight:false},
    {type:'color',messageKey:'NIGHT_TEXT_COLOR',defaultValue:'0xFFFFFF',label:'Quiet Time text',layout:'COLOR',sunlight:false},
    {type:'color',messageKey:'NIGHT_ACCENT_COLOR',defaultValue:'0x55AAFF',label:'Quiet Time accent',layout:'COLOR',sunlight:false},
    {type:'select',messageKey:'CLOCK_FORMAT',defaultValue:0,label:'Time format',serializeValueAs:'integer',options:[
      {label:'Follow Pebble',value:0},{label:'12-hour time',value:12},{label:'24-hour time',value:24}
    ]},
    {type:'toggle',messageKey:'ANIMATIONS',defaultValue:true,label:'Animations',description:'Short transitions when music or notifications change.'},
    {type:'toggle',messageKey:'SHOW_STATUS',defaultValue:true,label:'Status indicators',description:'Low watch or phone battery and lost connection.'},
    {type:'text',defaultValue:'Language follows your watch.'}
  ]},
  {type:'section',items:[
    {type:'heading',defaultValue:'Content'},
    {type:'text',defaultValue:'Your two readings'},
    {type:'text',id:'dynamic-readings',defaultValue:'Complications 1 and 2 sit side by side. Both shrink above music or notifications.'},
    {type:'text',defaultValue:'These are shared Pebble slots. Changing them also affects other watchfaces that use them.'},
    {type:'text',defaultValue:'Choose slots 1 and 2 in Pebble → Settings → Complications.'},
    {type:'text',defaultValue:'Units come from Pebble and the selected plugin.'},
    {type:'toggle',messageKey:'SHOW_ACTIVITIES',defaultValue:true,label:'Show activity plugins',description:'Compatible activity plugins connect automatically. Manage access permissions in Pebble.'},
    {type:'text',id:'dynamic-activity-order',defaultValue:'Activity priority'},
    {type:'text',defaultValue:'Clock mark: last-reported activity state. The source app owns tracking and alarms.'},
    {type:'text',defaultValue:'Music & notifications'},
    {type:'toggle',messageKey:'SHOW_MUSIC',defaultValue:true,label:'Show music while playing'},
    {type:'text',id:'dynamic-notification-mode',defaultValue:'Notification activity'},
    {type:'toggle',messageKey:'SHOW_NOTIFICATIONS',defaultValue:false,label:'New notifications'},
    {type:'toggle',messageKey:'NOTIFICATION_PREVIEW',defaultValue:false,label:'Include message text'},
    {type:'toggle',messageKey:'NOTIFICATION_APP_ONLY',defaultValue:false,label:'App name only'},
    {type:'select',messageKey:'NOTIFICATION_VIBRATION',defaultValue:0,label:'Additional vibration',description:'One additional vibration per new notification burst, using your app filter. Respects Quiet Time. Turn this off if Pebble already vibrates for those notifications.',serializeValueAs:'integer',options:[
      {label:'Off',value:0},{label:'Short pulse',value:1},{label:'Double pulse',value:2}
    ]},
    {type:'text',id:'dynamic-notification-help',defaultValue:'New notifications use the same activity priority and primary/secondary layouts. Allow notification access in Pebble. Pebble’s system notifications remain separate.'},
    {type:'text',id:'dynamic-app-mode',defaultValue:'Notification apps'},
    {type:'input',messageKey:'NOTIFICATION_APPS',defaultValue:'',label:'Selected apps',description:'Use exact app names or Android package names, separated by commas. For example: Signal, org.thoughtcrime.securesms',attributes:{type:'text',maxlength:160,autocapitalize:'none',autocomplete:'off',spellcheck:'false'}}
  ]},
  {type:'section',items:[
    {type:'heading',defaultValue:'Connection & help'},
    {type:'text',id:'dynamic-save-status',defaultValue:'Changes apply after you save.'},
    {type:'text',defaultValue:'Status when settings opened. Reopen settings to refresh.'},
    {type:'text',id:'dynamic-source-status',defaultValue:'Dynamic content: waiting for Pebble'},
    {type:'text',id:'complications-status',defaultValue:'Complications: waiting for Pebble'},
    {type:'text',id:'context-status',defaultValue:'Dynamic content: waiting for Pebble'},
    {type:'text',defaultValue:'Timeline Peek keeps the clock and complications 1 and 2. Its event panel takes priority over music and notifications.'}
  ]},
  {type:'submit',defaultValue:'Save settings'},
  // Transport components have no preference keys and are never submitted.
  {type:'text',id:'dynamic-settings-data',defaultValue:'{}'},
  {type:'text',id:'dynamic-settings-strings',defaultValue:'{}'},
  {type:'text',id:'dynamic-preview-assets',defaultValue:'{}'},
  {type:'input',messageKey:'__dynamicActivityPreferences',defaultValue:'',attributes:{type:'hidden'}}
];
