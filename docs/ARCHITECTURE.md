# Dynamic and the shared core

Dynamic 0.4.0 pins Pebble Watchface Core 0.2.0. Halftone retains its own
0.1.0 pin until independently upgraded. Each PBW embeds its library and keeps
its UUID, settings and permission manifest. The core is not a background app.

## Boundary

| Component | Owner |
| --- | --- |
| Pebble source subscriptions, receipt cache and message queue | Shared core JavaScript |
| Packet validation, native snapshots/cache, expiry and reconnect/session state | Shared core C |
| Clock/date, layout, generated clock outlines, body fonts, artwork and transitions | Dynamic |
| One native AppMessage dispatcher, SDK services and timers | Dynamic host |
| Settings form, translations, permissions and manifest | Dynamic host |
| SDK graphics services, system/fallback glyphs and the Timeline event panel | PebbleOS |

The core imposes no coordinates, dot border or font family. Native callbacks
forward events to its state object; Dynamic renders the current snapshots.
Phone coordinators receive generated keys, storage, settings and clocks
explicitly. Halftone's renderer and installed engine remain unchanged by this release.

## Native-style presentation

The clock uses generated outlines from Fredoka. Keeping one set of shapes
allows the clock to scale and move continuously between its idle and compact
positions. Roboto Condensed resources provide the body text. Font sources,
licenses and generation provenance belong to this repository.

Temperature and percent units use Fredoka outlines with the reading numbers,
and reading labels use muted 11px text. Body assets add Greek, Cyrillic and
Latin Extended Additional coverage and a visible missing-character glyph for
unsupported text.

Fredoka clock, number and unit rendering uses 4×4 fractional pixel coverage.
Edge pixels blend the chosen foreground and background without adding stroke
weight. Reusable bounded row buffers support this rendering without
per-frame heap allocation. Font-source geometry and native raster appearance
are checked separately.

New installs use white/black page colors and a Picton Blue accent. Saved page,
text and accent colors are retained. Both light and dark pages share one
canvas. The chosen accent colors reading symbols and recognized rain-probability
gauges; notification symbols retain their attention colour and tiny status
warnings use foreground ink for legibility. Generic readings retain their
supplied text, units and images. A percentage becomes a 0–100% gauge only when
the provider identifies it as precipitation probability. Temperatures use
source images when available or a semantic thermometer symbol.

The idle view gives the weekday/date and rounded clock a hero area, followed
by two horizontal readings. Active context contracts the clock, stacks the
weekday/date and moves the readings together above the title/body area.
Idle and Peek keep the date on one line. A heading-only notice uses a compact source banner; with music present it
reduces title space while retaining the fixed artist/artwork footer. A message
preview uses a bounded panel. Summary-only notices without music preserve
more space for the clock and readings. Within the active layout, fresh tracks or notices slide
inside that context area while the surrounding clock/readings retain their
positions.

Music is anchored from the bottom. The 40px artwork and up to two artist
lines share a visible bottom at y=216. When artwork or artist text is present,
the footer reserves at least 40px, expanding for taller artist ink. A title of
up to three lines grows upward, ending 8px above that footer. With neither
artwork nor artist text, the title can extend to y=216. Track length changes
the title's top edge while the footer remains fixed. Measured native ink
bounds determine fit within the title/footer areas, including a smaller
Big-font fallback when needed.

Animations run for bounded transitions and stop when settled. The
`ANIMATIONS` setting controls Dynamic's own transitions. Native Timeline Peek
remains driven by the actual unobstructed area on each update, including
temporary overshoot. The renderer yields its context area and retains the
clock and both readings above Pebble's event panel.

The old `TEXT_SHADOW` message/persistent setting remains a compatibility input
without a visible effect. `ACCENT_COLOR` retains its 0.2 number, and
`ANIMATIONS` retains its existing number and `CLOCK_FORMAT` is appended.
CLOCK_FORMAT uses 0 (follow Pebble), 12 or 24. The native migration preserves
legacy explicit USE_12_HOUR choices. Unset new installations follow the watch.

The build audit checks the packaged SDK payload, pinned core, declared font
resources and generated clock assets. Native captures check text fitting,
glyph appearance, colors, images and motion. Validation from earlier versions
remains historical rather than being relabeled for 0.3.2. See
[the design references](NATIVE-STYLE.md) and the
[build-specific validation record](VALIDATION.md).

## Pinned dependency

The dependency is `file:vendor/pebble-watchface-core-0.1.0.tgz`, committed with
the lockfile and `vendor/CORE.json`. Release updates use a new versioned core
archive, updated integrity/provenance and a consumer rebuild. The core is not
fetched from a moving branch at startup and does not run as another watch app.

To update: test/build a new core package, copy that exact tarball into this
repository, update the file dependency and lockfile/provenance, then run
Dynamic's portable tests, SDK build, native-style audit and rendering checks.
Halftone can update to the same release independently. Shared engine tests do
not replace visual tests of either face's renderer.

## Data contract

Two global complication choices feed the horizontal readings. A separate
fixed five-record packet carries music title/art, artist, notification title/icon,
notification body and low phone battery. Notification context takes priority
briefly; Timeline Peek owns the native lower panel.

Context is transient and has explicit confirmation leases. Notification access
and message preview default to off. Reconnect/startup markers prevent old queued
frames from reviving private notification summaries. Cache loading never
renews the original reading timestamps. These behaviors live in the core;
Dynamic decides their screen presentation.

For a content transition, Dynamic can keep an outgoing rendering snapshot in
RAM until the bounded slide ends. These snapshots are never persisted or
logged. Expiry, invalidation, disconnect and privacy clearing take priority
over an outgoing animation, so cleared context is not kept visible merely to
complete a transition.

## Core 0.2 contract

The core preserves provider-formatted text and units alongside optional semantic
kind, source kind, raw numeric value, range, unit and short label. Version 3
extends record text to 128 UTF-8 bytes and imagery to 40×40/4bpp, while strictly
decoding legacy versions 1/2. The maximum packet is 5,186 bytes; Dynamic reserves
a 6,144-byte native inbox. Unknown fields, invalid numeric/UTF-8/bitmap data and
CRC mismatches fail closed.

Eight 256-byte persistence keys remain sufficient because cache compaction drops
image payloads while retaining scalar data, metadata and original receipt/expiry.
Live snapshots retain images. A cached-origin marker identifies native cache
restores only; it is not the weather station's measurement timestamp.

The shared queue replaces obsolete pending snapshots in their own source channel.
Settings keep FIFO ordering. Coordinators treat SUPERSEDED as terminal, never as
a source failure or a successful transport acknowledgement. Context snapshots
retain their matching reconnect boundary/session token.

Music/phone confirmation remains bounded to 120 seconds and paused music to five
seconds. Notification bursts have an eight-second maximum; updates do not renew
the original interruption. Baseline, app filtering and session privacy remain.
Music, notification text and artwork are never persisted.

## Settings ownership

The Clay page groups sample preview, Display, two source summaries, Music &
notifications and Help. Dependent notification fields are conditional. Source
health is a snapshot at open, with human-readable recovery and collapsed raw
errors. All 17 supported locales cover new controls. Preview assets are native
rendered samples for idle, music, summary/body notifications and Timeline,
each with two text sizes, two clock formats and Light/Dark presets;
Custom recolouring is explicitly approximate and preserves artwork/system areas.

Only validated appearance/time preferences reach the watch. Phone-only privacy
changes configure context immediately. A separate nonprivate delivery record
stores the latest pending appearance packet, bounded retries and watch ACK;
reconnection retries the latest pending save. An old ACK cannot mark a newer save
as delivered. The next configuration visit reports the delivery state.

The SDK load image remains below its 16-bit virtual-size limit by allocating one
23,552-byte NativeState at startup for core state, transition snapshots and
fixed raster buffers. Initialization failure exits before subscribing services;
teardown cancels callbacks and frees the block. Rendering allocates no per-frame
state buffers. SDK load usage and runtime allocation are reported separately.
