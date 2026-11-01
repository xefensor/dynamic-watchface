# Design references and motion

Dynamic 0.4.1 uses a chunky digital clock and a composition that makes room for
context. It is an independent watchface by Xef. Pebble's watchfaces, plugin
examples and design guidance inform its visual language; Dynamic owns its
layout, type choices and transitions.

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
- **One canvas:** Roboto Condensed text, two horizontal readings and small
  symbols share the page. Accent colors identify meaningful graphics and
  status; the date remains plain.
- **Space follows the task:** the compact header stacks weekday/date while
  keeping both horizontal readings. Music's artwork and up to two artist
  lines stay anchored to the bottom; a full-width title grows upward through
  up to three lines with an 8px gap above the footer. Notifications keep their
  source heading and optional bounded body. Summary banners retain the music
  footer and briefly reduce the title area instead of taking over the whole face.
  A heading-only banner can retain two title lines with a gap above that footer.
- **Motion explains the change:** the clock and readings move together as
  context enters or leaves. In the active layout, fresh track and notice
  updates move inside the context area while the surrounding layout stays
  stable.
- **Calm resting states:** transitions are bounded and stop when complete.
  The Animations setting disables Dynamic's local transitions.

A single typography system applies to new and existing installations. Idle
uses an 80px clock, 20px date and 36px reading values; 14px labels explain each
source. Context contracts the clock to 42px while retaining 22px values and
readable 14px symbols. Timeline Peek uses 26px values and 16px symbols.

Music titles use a fixed 24px size and artists a fixed 20px size. Track length
changes wrapping through up to three title lines and two artist lines, then
ellipsis; it never changes the type size. Native ink measurements showed
that 26px cannot reliably fit three title lines above the footer, so 24px
preserves both readability and the fixed bottom composition. Artwork stays
40px and shares y216 with the bottom of the artist text. The former Larger
text mode is retired, including existing saved choices, and sample previews
have no size selector.

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
