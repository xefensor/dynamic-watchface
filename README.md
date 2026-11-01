# Dynamic

A Pebble Time 2 watchface with chunky rounded time, two horizontal readings
and a layout that moves to make room for music, notifications and Timeline
Peek. Version 0.3.2 uses Fredoka clock outlines and Roboto Condensed text to
keep a consistent face as its layout changes. Dynamic is an independent
watchface by Xef.

It uses [Pebble Watchface Core](https://github.com/xefensor/pebble-watchface-core)
0.1.0, the same data engine used by
[Halftone](https://github.com/xefensor/halftone-watchface).

Install [Dynamic-0.3.2.pbw](dist/Dynamic-0.3.2.pbw); the
[source archive](dist/Dynamic-0.3.2-source.zip) is included. Dynamic has its own UUID,
`cc1281f1-3b4d-45b5-b235-6881b1fd8fbd`, so it can be installed alongside Halftone.
This is an early sideloaded build for the Pebble plugin preview. Public-store
submission remains on hold while Pebble's preview restriction applies.

## Layout

![Normal, music, notification and Timeline Peek states](docs/images/layout-states.png)

| State | Presentation |
| --- | --- |
| Normal | A shortened localized weekday and numeric date, large rounded clock and two equal reading columns |
| Music | A compact clock/header and both readings sit above a title that grows upward from a bottom-anchored artist/artwork footer |
| New notification | A source heading/icon and optional large message text briefly take priority over music |
| Timeline Peek | The face follows Pebble's changing available area, retaining its clock and both readings above the native event panel |

New installs use a white page, black text and a Picton Blue accent. Saved page,
text and accent colors are retained when upgrading. The chosen accent colors
reading symbols. Recognized rain-probability gauges and notification symbols
use yellow. The same composition works with a dark page.

The clock keeps the same Fredoka shapes while it scales and moves.
Temperature and percent units share that rounded style, with small muted
reading labels. Body text uses bundled Roboto Condensed. Both fonts come
with their SIL Open Font Licenses. The date stays plain, and the layout
shares one canvas without a dot border or text shadows. See
[the design notes](docs/NATIVE-STYLE.md) for the reference sources and decisions.

The first two global complication choices keep their left/right positions,
including when one is empty. Selected readings without a usable value show
`--`. Timeline events remain native rather than being duplicated inside the face.

The compact music/notification header stacks the weekday above the date.
Idle and Timeline Peek use a single-line date. Music titles can use up to
three lines, growing upward with a fixed gap above the footer. Artwork and
up to two artist lines share a fixed bottom edge, so their position stays
stable across short and long tracks.

Rain-probability percentages can use a 0–100% gauge when the source identifies
them as precipitation data. Other percentages keep their ordinary reading
presentation. Temperatures use a source image when supplied, or a thermometer.

Big text prefers larger date, reading and message text within the available
space. The clock fits its rounded digits to the width in both modes. Long text
wraps or ends in an ellipsis. Music text keeps the preferred Big size when its
measured native ink fits, with a smaller fallback only when needed. With
notification previews disabled, the app/title appears without the message body.

Short transitions connect layout and content changes. In the active layout,
fresh tracks and notices slide within the context area while the clock and
readings stay stable. The face settles after each transition. Turn
**Animations** off to disable Dynamic's own transitions; the layout still
follows native Timeline Peek.

## Set up

[Settings preview: light](docs/images/settings-light.png) · [dark](docs/images/settings-dark.png)

1. Enable the Pebble app's plugin preview if your version requires it.
2. Configure slots 1–2 in the phone app's **Complications** settings. A weather
   arrangement can use Temperature on the left and Precipitation on the right;
   other supported readings can replace either choice.
3. Open Dynamic's gear/settings page to choose page, text and accent colors,
   12-hour time, text size, animations, music, notification and status options,
   then save.
4. Allow the selected sources' permissions in Pebble. Music uses `ReadMusic`.

Music, animations and compact status indicators default to on. Notification
summaries and message previews default to off. Enable them explicitly if
wanted, and use the app-name filter to limit which notifications appear.
Startup and reconnect establish a fresh baseline so accumulated notifications
are not replayed.

Dynamic follows the selected sources' formatted values and units. Weather
comes through Pebble's chosen plugin; this face has no separate HTTP weather
client, provider override or local temperature conversion.

## Dynamic content

Notifications last at most 12 seconds. Paused music releases its panel after a
five-second grace period. Music and phone battery need confirmation at least
every two minutes, with a one-minute phone-side refresh. Expiry, disconnection
and privacy changes clear transient content. Music and notification text is
not persisted or logged. The renderer keeps only the temporary RAM snapshots
needed for a bounded transition; clearing private or expired data takes
priority over showing an outgoing animation.

The two selected complication readings can remain cached for up to two hours
from confirmed receipt, capped by an earlier source expiry. Reloading a cache
does not make it fresh again. Receipt time is not the provider's observation
time. Clock and date continue without the phone.

Artwork is optional. Sources provide small images capped at 24 pixels; scaling
uses the screen space but does not add image detail. Music is an information
panel: use Pebble's Music app for play/pause and track controls. The current
source does not supply playback position/duration, so there is no invented
progress bar. Timer integration is not part of this version.

## Languages

Weekdays and phone settings support English, Czech, Slovak, German, French,
Spanish, Italian, Portuguese, Dutch, Polish, Danish, Swedish, Norwegian,
Finnish, Hungarian, Romanian and Turkish. The watch language is preferred,
then the phone language for settings, with English as fallback. Provider text,
names and units remain as supplied. Bundled body fonts include Greek,
Cyrillic and Latin Extended Additional coverage alongside the Latin characters
used by these translations. Characters outside bundled coverage use a visible
missing-character glyph. The UI translations remain the languages listed
above. Language support and font coverage are distinct from physical
verification of every device/language.

## Build and install

Use Node.js, a C99 compiler for portable tests and a Pebble C SDK with Emery
support. The release audit and optional font regeneration also require the
Python packages pinned in
[scripts/requirements-font-assets.txt](scripts/requirements-font-assets.txt),
installed in your build environment.

```sh
npm ci
npm test
pebble build
python3 scripts/package-release.py
python3 scripts/check-native-style.py --pbw dist/Dynamic-0.3.2.pbw
python3 scripts/package-source.py
```

`npm test` covers phone-side behavior and portable native layout/core use;
`npm run test:native` runs only the native suites. Release packaging verifies
the permission manifest and exact SDK payloads before producing the PBW in
`dist/`. The audit checks the packaged build, pinned core and declared font
assets. Native captures check the appearance of the clock, text and motion.
Validation is recorded separately for each release. For a packaged build:

```sh
pebble install --emulator emery dist/Dynamic-0.3.2.pbw
```

The clock outline and body-baseline headers are committed with their source
fonts. With the pinned Python tools installed, check them using:

```sh
python3 scripts/generate-clock-glyphs.py --check
python3 scripts/generate-body-metrics.py --check
```

The body metrics use the SDK's FreeType rendering mode. The normal SDK build
uses the committed fonts and headers.

For a real PT2, enable Dev Connection in the phone app and install with your
normal Pebble developer connection. Native screenshots exercise deterministic
fixtures; live phone/plugin delivery and hardware behavior need device checks.
See [the build-specific validation record](docs/VALIDATION.md) for checks and
remaining limitations.

## Shared core

`vendor/pebble-watchface-core-0.1.0.tgz` is a committed, version-pinned Pebble
package. `package-lock.json` records its integrity and `vendor/CORE.json`
records upstream/version/checksum provenance. `npm ci` installs this archive;
users install only Dynamic's PBW, not a separate core app.

Core source handling, expiry, reconnect behavior and notification privacy are
shared with Halftone. Layout, font assets, transitions, settings presentation,
app identity and release artifacts belong to Dynamic. See
[the architecture notes](docs/ARCHITECTURE.md) for that boundary and the update
workflow. Updating the core repository alone does not change installed PBWs.

## License

[PolyForm Noncommercial License 1.0.0](LICENSE), retaining Xef's attribution.
Use, modification and sharing are permitted for noncommercial purposes under
that license. Commercial use requires separate permission. This is
source-available software rather than an OSI-approved open-source license.

Fredoka and Roboto Condensed use the SIL Open Font License 1.1. Their copyright
notices and licenses accompany the source fonts:
[Fredoka](resources/fonts/OFL-Fredoka.txt) and
[Roboto Condensed](resources/fonts/OFL-RobotoCondensed.txt). Earlier releases
remain in `dist/`, including 0.3.1. The 0.1 source archive preserves its Fira
Sans Condensed files and font license, while 0.2 uses PebbleOS system fonts.
