# Native-style design references

Dynamic 0.2 takes inspiration from Pebble's built-in Weather and Workout apps.
It is an independent watchface by Xef, not a Core Devices system app. The
references guide its typography and information hierarchy; Dynamic keeps its
own watchface layout and shared data engine.

The source reference is pinned to PebbleOS commit
[`7bf58aca187860b122d7ef4f7dba287d24aa887f`](https://github.com/coredevices/PebbleOS/tree/7bf58aca187860b122d7ef4f7dba287d24aa887f).
Official Emery test images were inspected during design. They are reference
material, not images shipped in this repository or watchface.

## What the native apps do

| Reference | Visual language used in Dynamic |
| --- | --- |
| Weather | A light page, black Gothic text, LECO numbers, colored symbols and compact information strips |
| Workout | Strong number/label hierarchy, equal metric columns, thin dividers and a full-width accent region |
| Both | System typography, deliberate alignment and colors tied to information rather than decoration |

Weather's main temperature uses LECO 36 Bold Numbers. Its detail heading uses
Gothic 28 Bold, with smaller Gothic labels and values. The current forecast
layout places black type on white and uses a thin rule beneath its main row.

Workout uses LECO for timers and metrics, Gothic for labels, a narrow accent
rule below its status area, and a colored lower metric band. Its activity and
recording indicators represent real workout state. Dynamic does not imitate
those indicators when no workout is being tracked.

Exact sources:

- [Weather detail layout](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/system/weather/weather_app_layout.c)
- [Weather forecast layout](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/system/weather/forecast_list.c)
- [Workout dashboard and active views](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/system/workout/active.c)
- [Workout shared style](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/system/workout/style.h)
- [Official Weather redesign announcement](https://repebble.com/blog/new-pebble-weather-app-software-updates)

## Dynamic's interpretation

- **Native type:** LECO time and numbers; Gothic date, headings and messages.
  Fonts are loaded from PebbleOS rather than bundled as custom resources.
- **One adaptive layout:** a large idle clock and two readings; a compact
  header and reading row when a song or notice needs the lower display.
- **Full-width information bands:** the chosen accent groups readings/music;
  a yellow notification panel makes its brief priority easy to identify.
- **Useful contrast:** page colors remain configurable; panel ink is selected
  automatically for its background. Color source images keep their palette.
- **Plain structure:** thin rules, equal columns and aligned edges, with no
  dot border, enclosing date box or text shadows.
- **Native Timeline priority:** Pebble's event panel owns its space. The face
  retains both selected readings and yields its own contextual panel.

The style is also functional: source changes, expiry and privacy clearing
keep the existing shared-core behavior. No playback controls, timer source or
continuous decorative animation is added by this redesign.

## Checking the result

System glyphs and fallback fonts come from firmware and installed language
packs, so an app-only PFO comparison cannot prove their exact ink bounds.
The 0.2 audit and native fixture screenshots are described in
[validation](VALIDATION.md). Old 0.1 font measurements remain historical results.
