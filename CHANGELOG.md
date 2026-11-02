# Changelog

## 0.8.0

- Development prerelease for phone/watch testing. Portable, package and emulator
  checks pass; real-device alarm vibration, Bluetooth behavior and live plugin
  delivery remain to be verified.
- Give generic countdown and elapsed activities large centered Fredoka numerals,
  a directly grouped caption and optional secondary music or activity summary.
- Keep pause and last-reported marks with the caption, without reserving empty
  artwork space for imageless clocks. Long values use bounded readable sizes.
- Select the soonest running deadline within a provider, then its paused timers,
  while preserving other provider positions and the two-card native limit.
- Pin separately released Pebble Activities 0.1.2 for compatible multi-slot Timer
  providers. Preserve UUID, AppMessage key numbers, music and privacy controls.
- Refresh native settings previews and capture notification/Timeline edge cases.

## 0.7.0

- Discovers compatible activity providers through independent Pebble Activities
  0.1.0, with a shared main/footer layout and bounded two-card projection.
- Shows native timer snapshots with local countdowns and an explicit last-reported
  mark. Pause/cancel authority and alarms remain in the originating watchapp.
- Adds a translated Plugin activities switch and separate provider health.
- Pins lean Core 0.5.0 for readings and keeps music/notification privacy behavior
  through the optional Activities compatibility adapter.
- Deduplicates identical font measurement rows without changing any glyph metric,
  font resource, text size or footer position.
- Preserves the UUID and all 41 existing AppMessage key numbers; appends 3 activity
  keys. No firmware fork or native Workout integration is included.

## 0.6.1

- Clears fully redacted, malformed or newly filtered notifications immediately,
  including queued replacements, through pinned Core 0.4.1.
- Keeps closed colour-picker swatches out of keyboard navigation and restores
  saved appearance settings after startup or a new watch session.
- Clarifies that watchface banners and vibration are additional to PebbleOS
  alerts; keeps both defaulted to Off and preserves existing choices.
- Adds an independent day/Quiet Time palette preview in settings.
- Protects warning visibility on every native palette background, preserves app
  identity when notification icons are invisible, and pauses title continuation
  underneath system windows. Hardens incoming boolean setting validation.
- Includes third-party licenses and attribution in the standalone PBW; prepares
  store copy, native screenshots, privacy/support documents and release checks.
- Keeps the plugin-preview appstore restriction explicit. Store submission is
  not enabled by preparing these materials.

## 0.6.0

- Fixes settings colours and control contrast in phone light and dark appearances,
  independently of the watch palette. Updates all 17 supported locales.
- Adds App-only notification privacy and optional Off / Short / Double vibration.
  Vibration defaults to Off, uses the existing app filter, respects watch Quiet
  Time and fires once per new burst rather than on history, retries or artwork.
- Gives summary banners a three-second deadline and actual message previews up
  to eight seconds. Replacements never extend the first notice's deadline.
- Continues long received song titles through fixed-size pages once without
  looping or shrinking type. Keeps a 40px fallback cover tile and stable artist
  position when artwork is missing, transparent or late; marks paused music
  during its existing five-second grace.
- Adds an optional palette following watch Quiet Time and restores daytime
  colours afterward. Shares/imports only the three main palette colours with
  a validated DYN1 colour code.
- Pins Core 0.4.0. Keeps v4 reading packets, UUID, existing key numbers and the
  unified layout; appends settings and transient context-state keys.

## 0.5.0

- Preserves precise Temperature / Feels like / High / Low / Forecast / Phone
  labels and public reading identity through loading, failure and expiry.
- Marks a known observation at least an hour old, rather than marking cache
  origin as age. Receipt and observation times stay distinct.
- Uses source/accent colour for ordinary content, amber for warnings, guarded
  secondary ink and visible accent fallbacks. Artist text is quieter; idle
  values and labels sit closer to their symbols.
- Separates notification sender/title from app identity. Late artwork/artist
  updates repaint quietly; a notification burst enters once and coalesces
  replacements for a 900ms stable interval within its existing eight-second
  maximum. Privacy clearing remains immediate.
- Groups settings into Appearance, Content and Connection & help. Five example
  states are independent of enabled features and privacy choices.
- Uses the watch's last reported OS clock style for Follow Pebble examples;
  labels an unknown-format example and adds a read-only CLOCK_FORMAT=255 query.
- Shows unsaved edits and last delivery beside Save, with automatic-retry
  wording. Restores the session's Custom palette when switching themes.
- Replaces the raw app filter with removable entries, explains shared Pebble
  readings and shows source recovery help nearby. Updates all 17 languages.
- Pins Core 0.3.0 and strict v4 packets with precise detail/forecast/observation
  metadata, explicit Expired status and identity-only public missing states.
  Legacy v1–v3 decode remains supported. Keeps the UUID, 34 message keys,
  unified typography, two reading homes and fixed music footer.

## 0.4.1

- Uses one unified typography/layout for new and existing installations,
  retiring Larger text and the settings preview size selector.
- Tunes the idle hierarchy with an 80px clock, 20px date, 36px readings and 14px
  labels. Compact context uses a 42px clock, 14px symbols and 22px values;
  Timeline Peek keeps 16px symbols and 26px values.
- Keeps song titles at 24px and artists at 20px, wrapping before ellipsis
  without changing font size between tracks. Three title lines and two artist
  lines share the fixed 40px artwork/footer anchor at y216.
- Lets heading-only notification banners retain up to two song-title lines
  and a gap above the fixed music footer at the same typography sizes.
- Reduces native settings samples to 20 presets: five states, two clock
  formats and two themes, with no separate size choice.
- Retains provider labels, Core 0.2.0, the app UUID and message-key numbers.
  LARGE_TEXT remains reserved for wire compatibility and no longer changes
  the rendering or stores an active size preference.

## 0.4.0

- Makes short notifications proportional to their content, retaining the fixed
  music footer while giving message previews a bounded reading area.
- Preserves provider labels for neutral numbers, ranges and times, with safe
  UTF-8 ellipses for long labels.
- Preserves two horizontal readings with semantic labels, clearer missing-data
  states and a consistent reading accent, including rain gauges.
- Rebuilds settings around native sample previews, themes, left/right source
  summaries, individual feature health and conditional privacy/app controls.
- Adds Follow Pebble / 12-hour / 24-hour time, preserving existing explicit
  choices, and acknowledged settings delivery with reconnect recovery.
- Translates the complete settings experience into 17 languages.
- Adopts Core 0.2.0: 128-byte UTF-8 text, requested 40px artwork, typed reading
  metadata and replacement of obsolete waiting source snapshots.
- Caps notification bursts at eight seconds without replaying historical
  notifications. Private context stays transient and clears on disconnect.
- Keeps the UUID and existing message-key numbers; appends CLOCK_FORMAT.

## 0.3.2

- Stacks the weekday and date in the compact music/notification header,
  retaining both horizontal complications.
- Anchors artwork and up to two artist lines to a fixed bottom edge. Song
  titles grow upward through up to three lines with a fixed gap above them.
- Fits the music text using measured native ink bounds, keeping the preferred
  Big font when it fits.
- Retains the single-line idle/Peek date, notification content layout and
  native Timeline priority.

## 0.3.1

- Expands body-font coverage for Greek, Cyrillic and Latin Extended Additional
  characters in source-provided text.
- Makes the missing-character glyph visible when text falls outside bundled
  font coverage.
- Uses Fredoka outlines for temperature and percent units, with muted
  11px reading labels.
- Smooths the clock, numeric readings and units with fractional pixel
  coverage while preserving their contour weight.
- Keeps the 0.3.0 layout geometry, bounded motion, data sources and settings.

## 0.3.0

- Adds a chunky Fredoka clock whose shapes scale and move together as the
  layout changes, with Roboto Condensed body text.
- Uses one canvas with two horizontal readings and rounded symbols, with a
  0–100% gauge for source-identified rain probability.
- Makes room for full-width song titles, artist/artwork and notification
  content with bounded layout transitions and context-only content slides.
- Follows native Timeline Peek's available area throughout its movement,
  retaining the clock and both readings.
- Adds a translated Animations toggle for Dynamic's own transitions. The
  native Timeline layout continues to adapt when local animations are off.
- Keeps the shared core, source choices, cache/expiry rules, notification
  privacy defaults, permissions and app UUID unchanged.
- Appends `ANIMATIONS` after the existing 32 message keys and retains the
  earlier release artifacts.

## 0.2.0

- Rebuilds Dynamic with PebbleOS LECO numbers and Gothic text, inspired by the
  built-in Weather and Workout apps.
- Introduces a light default page, thin rules, equal reading columns and
  full-width accent panels for readings and contextual content.
- Adds an accent color setting with automatic panel ink contrast; new notices
  use yellow. Existing saved page and text colors remain usable.
- Keeps music titles wide, artist/artwork together, and both selected readings
  visible above native Timeline Peek.
- Retires the visible shadow setting, custom Fira resources and their generated
  ink table. Legacy message-key numbers are retained; `ACCENT_COLOR` is appended.
- Keeps the pinned shared core, source choices, caches, expiry, permissions,
  notification privacy defaults and app UUID unchanged.

## 0.1.0

- Introduced the borderless content-first watchface using Pebble Watchface Core
  0.1.0, shared with Halftone.
- Added two horizontal complications, adaptive music/notification panels,
  native Timeline Peek handling and translated phone settings.
- Used bundled Fira Sans Condensed fonts. The historical PBW/source archive
  remain in `dist/`; the source archive preserves its font license and validation
  record.
