# Dynamic and the shared core

Dynamic 0.1.0 uses Pebble Watchface Core 0.1.0, the same pinned engine release
used by Halftone 1.9.0 rc.6. Each PBW embeds its own library code and has its own
UUID and saved settings.

## Boundary

| Component | Owner |
| --- | --- |
| Pebble source subscriptions, receipt cache and message queue | Shared core JavaScript |
| Packet validation, native snapshots/cache, expiry and reconnect/session state | Shared core C |
| Clock/date, layout, font fitting/metrics, artwork drawing and appearance | Dynamic |
| One native AppMessage dispatcher, SDK services and timers | Dynamic host |
| Settings form, translations, permissions and manifest | Dynamic host |
| Timeline event panel | PebbleOS |

The core never imposes Halftone's dot border, coordinates or fonts. Dynamic can
change its content area while both faces retain the same data validation and
privacy behavior. Native callbacks forward events to the shared state object;
rendering consumes its snapshots. Phone coordinators receive generated keys,
storage, settings and clocks explicitly.

## Content-first layout

The idle view gives the weekday/date, clock and two horizontal readings the
screen. Active context moves clock/date into a compact header and keeps both
readings in a slim row. Music uses a full-width title area of up to two lines,
then a separate bottom artist/artwork row. Notifications use an app/title/icon
heading and a full-width body of up to two lines when previews are enabled.
The default colors are white on black; 6px side padding protects readability
at the physical edge without a decorative border. Big text scales the face's
own typography. Timeline Peek hides the custom context area and retains both
readings above Pebble's panel.

## Reproducible dependency

The dependency is `file:vendor/pebble-watchface-core-0.1.0.tgz`, committed with
the lockfile and `vendor/CORE.json`. Release updates use a new versioned core
archive, updated integrity/provenance and a consumer rebuild. The core is not
fetched from a moving branch at startup and does not run as another watch app.

To update: test/build a new core package, copy that exact tarball into this
repository, update the file dependency and lockfile/provenance, then run
Dynamic's portable tests, SDK build, font checks and native rendering checks.
Halftone can update to the same release independently. Shared engine tests do
not replace visual tests of either face's separate renderer.

## Initial data contract

Two global complication choices feed the horizontal readings. A separate
fixed five-record packet carries music title/art, artist, notification title/icon,
notification body and low phone battery. Notification context takes priority
briefly; Timeline Peek owns the native lower panel and temporarily hides the
watchface's own context.

Context is transient and has explicit confirmation leases. Notification access
and message preview default to off. Reconnect/startup markers prevent old queued
frames from reviving private notification summaries. Cache loading never
renews the original reading timestamps. These behaviors live in the core;
Dynamic decides their screen presentation.
