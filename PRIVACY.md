# Dynamic privacy

Dynamic displays readings supplied by Pebble and the plugins you select. It
has no analytics, advertising, account service or separate weather server.

## Data used

- The first two Pebble complication choices provide the readings on the face.
  Their providers may use a network service or location according to their own
  settings and permissions. Dynamic has no direct weather HTTP client.
- Compatible activity plugins provide main/footer text, optional artwork and
  clock or numeric fields. Providers control their source permissions and expiry.
- Music uses the current title, artist, playing state and optional artwork.
- Optional watchface notification banners use the app identity, sender/title,
  optional message text and optional icon. Notification access is controlled
  by Pebble and must be allowed explicitly. Banners default to Off.
- Phone battery and connection state support the optional status indicators.

Music requires Pebble's `ReadMusic` permission. Notification sources require
notification access; location-based readings may require location access.
Pebble controls those permissions and the selected providers.

## Storage and display choices

Appearance settings, delivery state and selected app names are stored locally
on the phone. Appearance settings and bounded reading caches are also stored
on the watch. Selected readings other than music and notifications may remain
cached for up to two hours from confirmed receipt, capped by an earlier source
expiry; restoring a cache does not renew it.

Music and notification contents remain transient. Dynamic does not persist or
log their text or artwork. They clear on expiry, disconnection or relevant
privacy changes. Notification history is used to establish a baseline, so it
is not replayed when the face starts or reconnects.

App-only privacy keeps sender and message text out of watch notification
packets. Pebble's phone-side notification reader still needs notification
access, and Dynamic can examine an update locally to tell it from an unchanged
notification. The app-name filter controls which banners Dynamic displays; it
does not replace Pebble's permission controls or system notification settings.

Generic activities stay in transient consumer memory, including after reconnect.
A public clock may remain explicitly unconfirmed only through its original
validity; private content clears on disconnection. The separate reference timer
plugin stores a bounded non-private timer snapshot on the phone. Dynamic does
not own or read the timer watchapp's native storage.

## Other software and support

Your chosen plugins and the Pebble app have their own data handling. This
document describes Dynamic, not those services. Dynamic's settings page is
bundled in the PBW and does not need an external configuration website.

Questions or reports can be sent through
[the repository's issue tracker](https://github.com/xefensor/dynamic-watchface/issues).
