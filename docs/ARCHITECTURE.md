# Dynamic and the shared core

Dynamic 0.3.0 uses Pebble Watchface Core 0.1.0, the same pinned engine release
used by Halftone 1.9.0 rc.6. This redesign changes Dynamic's presentation; it
keeps the data engine, UUID and existing source/cache contract. Each PBW embeds
its own library code and has its own identity and saved settings.

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
explicitly. Halftone's renderer is unchanged by Dynamic's 0.3 design.

## Native-style presentation

The clock uses generated outlines from Fredoka. Keeping one set of shapes
allows the clock to scale and move continuously between its idle and compact
positions. Roboto Condensed resources provide the body text. Font sources,
licenses and generation provenance belong to this repository.

New installs use white/black page colors and a Picton Blue accent. Saved page,
text and accent colors are retained. Both light and dark pages share one
canvas. The chosen accent colors reading symbols; recognized rain-probability
gauges and notification symbols use yellow. Generic readings retain their
supplied text, units and images. A percentage becomes a 0–100% gauge only when
the provider identifies it as precipitation probability. Temperatures use
source images when available or a semantic thermometer symbol.

The idle view gives the weekday/date and rounded clock a hero area, followed
by two horizontal readings. Active context contracts the clock and moves the
readings together above a full-width title/body area. Music keeps a separate
artist/artwork row. A notice uses its source heading/icon and optional body.
Within the active layout, fresh tracks or notices slide inside that context
area while the surrounding clock/readings retain their positions.

Animations run for bounded transitions and stop when settled. The
`ANIMATIONS` setting controls Dynamic's own transitions. Native Timeline Peek
remains driven by the actual unobstructed area on each update, including
temporary overshoot. The renderer yields its context area and retains the
clock and both readings above Pebble's event panel.

The old `TEXT_SHADOW` message/persistent setting remains a compatibility input
without a visible effect. `ACCENT_COLOR` retains its 0.2 number, and
`ANIMATIONS` is appended after the existing 32 keys. These are appearance
settings; the core data packet and dependency remain unchanged.

The build audit checks the packaged SDK payload, pinned core, declared font
resources and generated clock assets. Native captures check text fitting,
glyph appearance, colors, images and motion. Validation from earlier versions
remains historical rather than being relabeled for 0.3. See
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
