# Changelog

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
