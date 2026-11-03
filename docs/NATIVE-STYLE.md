# Design references and motion

Dynamic 0.12.1 uses a chunky digital clock and a composition that makes room for
context. It is an independent watchface by Xef. Pebble's watchfaces, plugin
examples and design guidance inform its visual language; Dynamic owns its
layout, type choices and transitions.

## Normal and compact summaries in 0.12.1

Activities 0.4.0 supplies standard or media summaries. Dynamic chooses either
role from the same record. The optional secondary occupies exactly one static
row above the primary, with an optional 16px symbol and complete selected datum.
Its text truncates from the beginning; an essential value that cannot fit makes
that optional row yield. Primary content expands when the row is absent.

Primary content is bottom-aligned as one group above its context footer at
y216. Headline, labels, supporting text and an optional range indicator keep
their internal spacing and leave an 8px gap above a present footer. Short
content leaves spare space above the group. Standard context uses an instance
name or provider identity. Media uses a 24px title and optional
40px artwork beside up to two 20px artist lines. A title gets at most three
lines alone or two with a secondary. Text never scrolls, pages or cycles.
Useful detail wraps below a standard headline; meaningful value labels sit
above the value. Clocks keep independent running states and original leases.

ACT3 carries explicit media presentation, identity symbols, compact-datum
selection, value-direction symbols and a media-item token. Dynamic does not
infer media from a hidden label convention. Single-ink identity icons follow
the foreground; media artwork retains its supplied palette, including a
single-color cover. All media uses this renderer after phone-side adaptation.

A real primary source/run replacement or media-item change can start a bounded
280ms incoming slide. The renderer retains no outgoing activity copy. Clock
ticks, lease refreshes, artwork and artist corrections do not start motion.
The header remains still during an activity-only slide. The Animations setting
turns local motion off, and native Timeline Peek retains priority.

The 20 embedded settings presets and additional native summary cases use the
production encoders. Actual Emery captures separately verify fitting, symbols,
role promotion and motion. Known public Timer predictions retain original
validity; arbitrary unconfirmed saved sources are omitted. No Last reported
line is inserted into the composition. Source lifecycle and native alarm
ownership remain independent of presentation.

## First-party references

| Reference | Principle carried into Dynamic |
| --- | --- |
| Kickstart | Distinctive time typography, recognizable symbols and graphics tied to real data |
| TicToc | A clear visual identity that stays recognizable when the available area changes |
| Kickstarter clock example | The same clock shapes move and scale through a transformation, then settle |
| Weather Face plugin demo | Bold hierarchy, a prominent condition image and compact groups of related values |
| Complication Face plugin demo | Two horizontal readings, rounded symbols and gauges for meaningful numeric ranges |
| Weather and Workout apps | Readable hierarchy, deliberate alignment and restrained use of color |

The PebbleOS source references are pinned to
[`7bf58aca187860b122d7ef4f7dba287d24aa887f`](https://github.com/coredevices/PebbleOS/tree/7bf58aca187860b122d7ef4f7dba287d24aa887f).
The plugin examples are pinned to
[`0e348c2a07da0642c8ac3be5a161b9a999938841`](https://github.com/coredevices/pebble-apps/tree/0e348c2a07da0642c8ac3be5a161b9a999938841).
Their source and reference images were used for design research. Reference
images are not redistributed as Dynamic assets.

Sources:

- [Kickstart](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/watch/kickstart/kickstart.c)
- [TicToc](https://github.com/coredevices/PebbleOS/blob/7bf58aca187860b122d7ef4f7dba287d24aa887f/fw/apps/watch/tictoc/default/tictoc_default.c)
- [Official historical Kickstarter clock example](https://github.com/pebble-examples/ks-clock-face)
- [Weather Face renderer](https://github.com/coredevices/pebble-apps/blob/0e348c2a07da0642c8ac3be5a161b9a999938841/weather-face/src/c/main.c)
- [Complication Face renderer](https://github.com/coredevices/pebble-apps/blob/0e348c2a07da0642c8ac3be5a161b9a999938841/complication-face/src/c/main.c)
- [Pebble UI design guidance](https://developer.repebble.com/guides/design-and-interaction/recommended/)
- [Eric Migicovsky on PebbleOS's design identity](https://ericmigi.com/blog/how-to-build-a-smartwatch-software-setting-expectations-and-roadmap/)

The plugin demos are visual and data-source references. Their renderers
primarily redraw as data arrives. Dynamic adds its own coordinated layout
transitions and retains the first two global choices in their left/right
positions. The Complication Face demo selects slots by data shape.

## Dynamic's interpretation

- **One recognizable clock:** Fredoka outlines give the time its rounded
  character. The same shapes scale and move between the large idle clock and
  compact contextual clock. Temperature and percent units share that style.
- **One canvas:** Roboto Condensed text, two horizontal readings and rounded
  symbols share the page. The date remains plain. Ordinary content uses the
  accent; amber is reserved for low-battery/connection warnings.
- **Literal information:** Temperature, Feels like, High, Low and Phone labels
  retain what the data means. Forecast temperature is identified as such.
  Loading/failure/expiry retains public reading identity without stale values.
  Known observation age is distinct from a recent transport receipt.
- **Space follows the task:** the compact header stacks weekday/date while
  keeping both horizontal readings. Music's artwork and up to two artist
  lines stay anchored to the bottom; a full-width title grows upward through
  up to three lines with an 8px gap above the footer. Notifications keep their
  source heading and optional bounded body. Summary banners retain the music
  footer and briefly reduce the title area instead of taking over the whole face.
  A heading-only banner can retain two title lines with a gap above that footer.
- **Motion explains priority:** the clock and readings move together as
  context enters or leaves. An explicit media-item token defines a track
  change; legacy Music derives that token from its received title. Late artwork/artist metadata and notification icons repaint in
  place. A notification burst enters once; rapid replacements are coalesced
  for a stable reading interval instead of repeating its entrance.
- **Calm resting states:** transitions are bounded and stop when complete.
  The Animations setting disables Dynamic's local transitions.

A single typography system applies to new and existing installations. Idle
uses an 80px clock, 20px date and 36px reading values; 14px labels explain each
source. Context contracts the clock to 42px while retaining 22px values and
readable 14px symbols. Timeline Peek uses 26px values and 16px symbols.

Media uses fixed 24px titles and 20px artists. Native ink measurements showed
that 26px cannot reliably fit three title lines above the footer. The final
fitting line ellipsizes; no continuation state or timer is retained. Artwork is
optional and stays 40px when present. The former Larger text mode is retired,
including existing saved choices, and sample previews have no size selector.

Idle values and labels sit 4px and 6px closer to their symbols than in 0.4.1.
Media artist text uses a quieter secondary treatment at 20px.
After native palette rounding, secondary ink falls back to foreground when
muting would lose 4.5:1 contrast. Accent graphics have a 2:1 visibility guard;
provider artwork retains its colours.

Notification sender/title and app identity are separate. The sender is primary,
with app identity in its quieter bottom context footer.
Long app names do not reduce sender text size. A banner preserves the music
footer while its available title lines adjust to the bounded notice heading.
App-only privacy shows app identity while omitting sender and body. Summary
bursts get at most three seconds and actual message previews eight; later
updates never extend the first notice's deadline. Optional Short / Double
vibration defaults to Off, uses the same app filter and respects watch Quiet
Time. It announces a new burst once, not each replacement or redraw.

An optional Quiet Time palette follows the watch's own state and restores the
daytime colours afterward. Both palettes retain the same contrast fallbacks and
geometry; the change does not create another text or layout mode.

The settings page follows the same principles: one page with Appearance,
Content and Connection & help. Safe layout/colour samples are independent of
privacy settings. The Save area describes draft versus last delivery, theme
exploration is reversible, source ownership/recovery is explicit and app filters
are visible individual entries. Read-only OS clock-style queries support
Follow Pebble examples without changing the watchface override; unknown format
is labelled rather than implied. The page uses its own explicit phone light/dark
colours, including controls, independently of chosen watch colours. Sharing a
validated three-colour preset changes only the main palette; privacy and alert
choices are never included.

The face renders source-provided data and recognized semantic symbols. Its
gauges use a validated provider-supplied range, with source-identified rain
probability retaining its 0–100% presentation. Arbitrary percentages never acquire
an invented range. A temperature can use a source
image or a thermometer symbol. Source artwork retains the limitations of the
image actually received.

## Native Timeline priority

Pebble owns the Timeline event panel. Dynamic follows the actual unobstructed
area throughout the native movement, retaining its clock and both readings
above the event. Local context yields its space. This remains true when
Dynamic's own animations are disabled.

The [Unobstructed Area documentation](https://developer.repebble.com/docs/c/User_Interface/UnobstructedArea/)
notes that Peek first moves slightly, then snaps with bounce-back. The
available height can briefly be smaller than the settled height. Native
rendering checks therefore need intermediate movement as well as endpoint
screenshots.

## Assets and verification

Fredoka and Roboto Condensed are Dynamic's font choices. Pebble's built-in
faces use several different type systems. Font sources and SIL Open Font
Licenses accompany Dynamic's generated clock-outline and body-baseline assets.
The optional regeneration tools are pinned in
[`scripts/requirements-font-assets.txt`](../scripts/requirements-font-assets.txt).

The type details include Fredoka temperature/percent units and muted 14px
reading labels. The body font adds Greek, Cyrillic and Latin Extended
Additional coverage for provider text.
Unsupported characters use a visible missing-character glyph.

The native Fredoka renderer uses 4×4 fractional pixel coverage, preserving the
contour geometry and weight while smoothing its edges. It reuses bounded row
buffers without per-frame heap allocation. Source-outline and advance checks
establish the font geometry; native captures separately check the resulting
raster and layout.

Release validation separately checks the C build, font/outline assets and
native captures. Physical-phone delivery and hardware battery life require
separate device checks. See
[the build-specific validation record](VALIDATION.md).

Earlier releases used other presentations: 0.1 bundled Fira Sans Condensed,
and 0.2 used system LECO/Gothic with information bands. Their archived release
artifacts and historical validation remain available.

## Release refinements

Warning ink keeps its amber role only when it has at least 3:1 contrast against
the quantized watch background; otherwise a visible black/white ink wins.
Invisible notification artwork uses the generic icon and app-name fallback.
Activity clock work pauses under covering system windows and recomputes when
the face is visible. Text retains the same static prefix. These refinements
preserve type sizes and the bottom artwork/artist composition.
