# Dynamic

A Pebble Time 2 watchface with native LECO numbers, Gothic text and full-width
information bands. Version 0.2.0 takes its visual cues from Pebble's built-in
Weather and Workout apps while giving music and notifications room to read.
Dynamic is an independent watchface by Xef.

It uses [Pebble Watchface Core](https://github.com/xefensor/pebble-watchface-core)
0.1.0, the same data engine used by
[Halftone](https://github.com/xefensor/halftone-watchface).

Install [Dynamic-0.2.0.pbw](dist/Dynamic-0.2.0.pbw); the
[source archive](dist/Dynamic-0.2.0-source.zip) is included. Dynamic has its own UUID,
`cc1281f1-3b4d-45b5-b235-6881b1fd8fbd`, so it can be installed alongside Halftone.
This is an early sideloaded build for the Pebble plugin preview. Public-store
submission remains on hold while Pebble's preview restriction applies.

## Layout

![Normal, music, notification and Timeline Peek states](docs/images/layout-states.png)

| State | Presentation |
| --- | --- |
| Normal | A plain weekday/date header, large LECO clock and two equal readings in an accent band |
| Music | A compact clock/date header and reading row leave a full-width accent panel for the song, artist and optional artwork |
| New notification | A yellow panel with the source app/title/icon and optional message text; notification context briefly takes priority over music |
| Timeline Peek | Pebble owns its event panel; the face adapts the clock/date and both readings to the remaining space |

New installs use a white page, black text and a Picton Blue accent. Saved page
and text colors are retained when upgrading. The accent color controls the
information band; its text switches automatically between black and white for
contrast. Notification panels use yellow. Dark page colors give the same
layout a Workout-like presentation.

The face uses PebbleOS system fonts rather than bundled Fira fonts. Thin rules,
aligned columns and restrained accents separate information. The date stays
plain, with no enclosing box, dot border or text shadow. See
[the native-style notes](docs/NATIVE-STYLE.md) for the reference sources and
design decisions.

The first two global complication choices keep their left/right positions,
including when one is empty. Selected readings without a usable value show
`--`. Timeline events remain native rather than being duplicated inside the face.

Big text uses larger date, reading and message text, with bolder time digits.
Long text fits the available area or ends in an ellipsis. With notification
previews disabled, the app/title appears without the message body.

## Set up

[Settings preview: light](docs/images/settings-light.png) · [dark](docs/images/settings-dark.png)

1. Enable the Pebble app's plugin preview if your version requires it.
2. Configure slots 1–2 in the phone app's **Complications** settings. A weather
   arrangement can use Temperature on the left and Precipitation on the right;
   other supported readings can replace either choice.
3. Open Dynamic's gear/settings page to choose page, text and accent colors, 12-hour time, text size,
   music, notification and status options, then save.
4. Allow the selected sources' permissions in Pebble. Music uses `ReadMusic`.

Music and compact status indicators default to on. Notification summaries and
message previews default to off. Enable them explicitly if wanted, and use the
app-name filter to limit which notifications appear. Startup and reconnect
establish a fresh baseline so accumulated notifications are not replayed.

Dynamic follows the selected sources' formatted values and units. Weather
comes through Pebble's chosen plugin; this face has no separate HTTP weather
client, provider override or local temperature conversion.

## Dynamic content

Notifications last at most 12 seconds. Paused music releases its panel after a
five-second grace period. Music and phone battery need confirmation at least
every two minutes, with a one-minute phone-side refresh. Expiry, disconnection
and privacy changes clear transient content. Music and notification text is
not persisted or logged by the shared core.

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
names and units remain as supplied. Glyphs come from PebbleOS and its installed
language packs; the PBW does not carry a second set of fonts. Supporting text is
distinct from physical verification of every firmware language pack.

## Build and install

Use Node.js, a C99 compiler for portable tests and a Pebble C SDK with Emery
support.

```sh
npm ci
npm test
pebble build
python3 scripts/package-release.py
python3 scripts/check-native-style.py --pbw dist/Dynamic-0.2.0.pbw
python3 scripts/package-source.py
```

`npm test` covers phone-side behavior and portable native layout/core use;
`npm run test:native` runs only the native suites. Release packaging verifies
the permission manifest and exact SDK payloads before producing the PBW in
`dist/`. The native-style audit checks the packaged build and use of system
fonts. Firmware glyphs are not embedded PFO resources, so the old custom-font
corpus counts are not presented as 0.2 checks. For a packaged build:

```sh
pebble install --emulator emery dist/Dynamic-0.2.0.pbw
```

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
shared with Halftone. Layout, system-font choices, settings presentation, app
identity and release artifacts belong to Dynamic. See
[the architecture notes](docs/ARCHITECTURE.md) for that boundary and the update
workflow. Updating the core repository alone does not change installed PBWs.

## License

[PolyForm Noncommercial License 1.0.0](LICENSE), retaining Xef's attribution.
Use, modification and sharing are permitted for noncommercial purposes under
that license. Commercial use requires separate permission. This is
source-available software rather than an OSI-approved open-source license.

Version 0.2 loads fonts already supplied by PebbleOS; it does not redistribute
font files. The historical 0.1 release remains in `dist/`; its source archive preserves
the Fira Sans Condensed font files and SIL Open Font License.
