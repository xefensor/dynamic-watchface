# Dynamic support

[Report a problem or request a change](https://github.com/xefensor/dynamic-watchface/issues).
Include the Dynamic version, Pebble model/OS version, phone app version, what
you expected and the steps that reproduce the problem. For a missing reading,
include the selected plugin and the state shown in **Connection & help**.
Keep private notification text out of reports and screenshots.

## Setup

Dynamic targets Pebble Time 2 (`emery`). Music and notification integration use
the Android Pebble app's plugin preview. Enable it in the phone app, select
complication slots 1 and 2, and allow the sources you want to use. Source and
permission availability depend on your Pebble app and plugins.

The clock and date work without a phone. Live readings, music and notification
banners require their sources. Music is a display; use Pebble's Music app for
playback controls. Timeline Peek is the watch's native panel.

## Notification overlap

Dynamic's optional banners and **additional** vibration do not replace
PebbleOS notifications. If both are enabled, PebbleOS may show or vibrate for
the same event too. Leave Dynamic's vibration Off when you want only the
normal system alert. Dynamic's app filter does not change PebbleOS forwarding.

## Release status

This is a sideloaded preview. Public Pebble store submission is held while
Pebble's published plugin-preview restriction applies. See the
[publication kit](store/README.md) and [validation record](docs/VALIDATION.md)
for the exact build's checks and remaining device-test limits.
