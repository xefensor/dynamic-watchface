# Changelog

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
