# Dynamic and the shared core

Dynamic 0.2.0 uses Pebble Watchface Core 0.1.0, the same pinned engine release
used by Halftone 1.9.0 rc.6. This redesign changes Dynamic's presentation; it
keeps the data engine, UUID and existing source/cache contract. Each PBW embeds
its own library code and has its own identity and saved settings.

## Boundary

| Component | Owner |
| --- | --- |
| Pebble source subscriptions, receipt cache and message queue | Shared core JavaScript |
| Packet validation, native snapshots/cache, expiry and reconnect/session state | Shared core C |
| Clock/date, layout, system-font selection, artwork drawing and appearance | Dynamic |
| One native AppMessage dispatcher, SDK services and timers | Dynamic host |
| Settings form, translations, permissions and manifest | Dynamic host |
| System fonts, fallback glyphs and the Timeline event panel | PebbleOS |

The core imposes no coordinates, dot border or font family. Native callbacks
forward events to its state object; Dynamic renders the current snapshots.
Phone coordinators receive generated keys, storage, settings and clocks
explicitly. Halftone's renderer is unchanged by Dynamic's 0.2 design.

## Native-style presentation

Dynamic uses system LECO numerals for time and numeric readings, with Gothic
labels and contextual text. New installs use white/black page colors and a
Picton Blue accent. Existing saved page and text colors are retained. A dark
page uses the same geometry. The accent panel chooses black or white ink for
contrast; notifications use a yellow panel.

The idle view gives the weekday/date and clock a clear header and hero area,
followed by two equal readings in the lower band. Active context compresses
the clock/date and readings above a larger panel for music or notifications.
Music keeps the title wide and the artist/artwork together. A notice uses its
source app/title/icon and optional body. Timeline Peek hides that custom panel
and retains both readings above Pebble's own event area.

Thin rules and column alignment provide the structure. The 0.2 renderer has
no dot decoration or text shadows. The old `TEXT_SHADOW` message/persistent
setting remains a compatibility input, but is not exposed or rendered. The
new `ACCENT_COLOR` key is appended after the existing 31 message keys so their
numeric mapping stays intact; this is an appearance setting, not a new data
packet or core version.

System glyph resources belong to firmware and language packs. Dynamic no
longer ships Fira files or the matching generated ink table. Its build audit
checks system-font use and packaged SDK payloads; actual rendering checks
cover line fitting, clipping, colors, images and state changes. The historical
0.1 custom-font corpus is recorded under that version rather than relabeled.
See [native-style references](NATIVE-STYLE.md) and the
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
