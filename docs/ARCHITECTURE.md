# Dynamic and the shared core

Dynamic 0.6.0 pins Pebble Watchface Core 0.4.0. Halftone retains its own
0.1.0 pin until independently upgraded. Each PBW embeds its library and keeps
its UUID, settings and permission manifest. The core is not a background app.

## Boundary

| Component | Owner |
| --- | --- |
| Pebble subscriptions, source identity/observation metadata, receipt cache and transport queue | Shared core JavaScript |
| Packet validation, native snapshots/cache, expiry and reconnect/session state | Shared core C |
| Clock/date, semantic labels, layout, fonts, artwork and transitions | Dynamic |
| Native AppMessage dispatcher, SDK services, timers and OS clock-format query | Dynamic host |
| Settings form, translations, permissions, preview and delivery status | Dynamic host |
| SDK graphics services, system/fallback glyphs and Timeline event panel | PebbleOS |

The core imposes no coordinates, dot border or font family. Native callbacks
forward events to its state object; Dynamic renders its snapshots. Phone
coordinators receive generated keys, storage, settings and clocks explicitly.
Halftone's installed renderer/engine are unaffected by this release.

## Information and presentation

One hierarchy keeps time dominant when idle, song title dominant during music,
and sender/message dominant during a notice. The two configured reading homes
remain left and right. Native Timeline Peek owns its lower event panel.

Fredoka outlines provide the clock and temperature/percent numbers and units;
Roboto Condensed provides body text. The clock and readings use 4×4 fractional
pixel coverage, blending foreground/background without expanding stroke weight.
Reusable bounded row buffers avoid per-frame state-buffer allocation. Generated
outline/baseline tables, licensed source fonts and native raster appearance are
checked separately.

A unified layout applies to new and existing installations. The nominal idle
clock is 80px, date 20px, values 36px, symbols 22px and labels 14px. Context uses
a 42px clock, 22px values and 14px symbols. Peek uses a 68px clock, 26px values
and 16px symbols. Width fitting still prevents wide clocks/readings clipping.
Idle values and labels are 4px and 6px closer to their symbols than in 0.4.1.
The compact weekday/date are stacked; idle and Peek retain one date line.

Reading meaning survives separately from its current value. Native detail
codes select localized Temperature, Feels like, High, Low, Phone battery and
other specific labels. Forecast temperature is identified as Forecast. Legacy
v3 temperature labels remain usable and are never blindly replaced with Now.
Public loading/failure/expired records retain identity without a stale value;
unconfigured slots stay empty. A clock mark means a known observation is at
least an hour old, never merely that native cache was loaded. Unknown
observation age does not imply either a fresh measurement or an old one.

New installs use white/black page colours and a Picton Blue accent. Saved
colours survive upgrades. Ordinary reading/notification symbols and meaningful
gauges use the accent; low-battery/connection warnings use amber. Provider
images retain their supplied colours. Muted labels and quieter artists use
foreground ink when their quantized secondary colour would fall below 4.5:1
contrast. Accent graphics require 2:1 against the background, falling back to
foreground or contrasting ink. Settings warn about low primary contrast;
these fallbacks do not silently replace the user's saved palette. The optional
Quiet Time palette keeps separate persisted colours. Effective rendering follows
`quiet_time_is_active()` locally and returns to the daytime palette afterward;
no phone timer or schedule is inferred.

A percentage becomes a rain gauge only when source semantics identify
precipitation probability. Numeric precipitation amounts retain their units
and do not become invented probabilities or ranges. Generic readings retain
provider text/units/images and meaningful provider labels.

## Music and notification motion

Music remains anchored from the bottom. The 40px artwork and up to two artist
lines share visible bottom y216. The footer always reserves its 40px cover tile,
using a simple fallback when received artwork is absent or transparent. Artist
position does not change when artwork arrives. The title ends 8px above the
footer and grows upward through up to three lines per page. Titles stay 24px
and artists 20px across tracks. A long received title advances through its
remaining fixed-size pages once and holds the final page; it never loops.
Artwork or artist corrections preserve continuation progress. Context/privacy
boundaries prevent obsolete title pages from appearing. A small pause mark
identifies the existing five-second grace before paused music yields its panel.

Notification title text and app label are separate fields. The sender/title
keeps the primary heading; an app icon normally supplies context. If no app
icon is available, a quieter app-name line sits above it. Long app labels do
not shrink the sender. A summary banner reduces song-title space and retains
its footer; an extra app-name line may reduce available title lines further.
A message preview uses a bounded body panel. App-only mode instead places the
app name in the heading and omits sender, message body and duplicate app label;
a missing app name may fall back to package identity, never sender text.

Content identity and image refreshes have different motion rules. New primary
song title text defines a new track because the current source supplies no
track identifier. A track change slides its content; late artwork or artist
metadata repaints in place. A notification burst enters once and subsequent
heading/body/icon changes stay in place. Lease-only refreshes already avoid
replaying motion. Core coalescing provides a 900ms stable reading interval
inside the first notice's deadline: at most three seconds for a summary/app-only
banner or eight seconds for an actual nonempty message preview. An earlier
source expiry can shorten either; later body/icon arrival cannot extend it.
Removing private content,
invalidating a source or clearing permissions bypasses that interval.

Animations stop when settled. The ANIMATIONS preference disables Dynamic's
transitions. Peek continues to follow every actual unobstructed-area frame,
including temporary overshoot, retaining the clock and both readings above
Pebble's event panel. Outgoing rendering snapshots exist only in RAM for a
bounded transition; expiry, invalidation, disconnection and privacy clearing
always take priority over completing it.

## Compatibility and clock-format query

The retired Larger text choice is neither loaded nor saved; live LARGE_TEXT
inputs have no visual effect. Its wire-key number remains reserved. Legacy
TEXT_SHADOW is also a compatibility input without a visible effect. Existing
message-key numbers and Dynamic's UUID remain unchanged. Keys 34–40 append
notification vibration, Quiet Time palette enable/colours, app-only privacy and
transient context state.

CLOCK_FORMAT preferences remain 0 (Follow Pebble), 12 or 24. Legacy explicit
USE_12_HOUR choices still migrate. CLOCK_FORMAT=255 is a read-only query, not
a preference: native replies with the OS clock style, without changing the
saved watchface override. Normal source replies also report that OS style.
The phone caches the last 12/24 reply separately from saved preferences.
Settings remain an offline snapshot; Follow Pebble uses the last reported
style, or labels its illustrative format when none has been received.

## Pinned dependency and v4 data contract

The committed dependency is file:vendor/pebble-watchface-core-0.4.0.tgz.
package-lock.json and vendor/CORE.json record version/integrity/provenance.
The exact source commit and archive SHA-256 are recorded in `vendor/CORE.json`
and the build-specific validation record.
Updating the core repository alone does not update installed PBWs. Each
consumer adopts and validates a versioned archive independently.

Core 0.4 preserves provider-formatted text and units, semantic/source kind,
raw numeric value/range, short label and precise detail/forecast/observation
metadata. V4 has five fixed records with a 56-byte header per record. Byte25
carries detail, byte26 the forecast qualifier, byte27 remains reserved, and
observation time is uint32 at offset52. Text remains bounded to 128 UTF-8
bytes and imagery to 40×40/4bpp. The maximum snapshot is 5,206 bytes; Dynamic
reserves a 6,144-byte native inbox. Strict v1/v2/v3 decoding remains supported.

Selected non-ready v4 public records may carry identity only: no text/value,
image, numeric flags, receipt/expiry or observation-time claim. EXPIRED=7 is
an explicit status. Forecast and observation claims cannot contradict each
other, and known observation time cannot exceed confirmed receipt. Unknown
fields, invalid numeric/UTF-8/bitmap data and CRC failures are rejected before
changing destination state. Native v3/v4 cache loading remains supported.

Eight 256-byte persistence keys remain sufficient because compaction drops
images while retaining public scalar/identity metadata and original receipt/
expiry. Private music/notification sources never persist. Image-only public
readings retain identity but become unavailable when their image is omitted.
Reloading a cache never renews a lease or observation timestamp.

Two global complication choices feed the readings. A separate fixed five-slot
context snapshot carries music title/art, artist, notification sender/icon,
notification body and low phone battery. Its notification record label carries
app identity separately from sender text. Music/phone confirmation is bounded
to 120 seconds and paused music has a five-second grace period. Notification
summary/app-only bursts last at most three seconds and actual message previews
up to eight seconds; updates do not renew the first notice's deadline. Reconnect
and startup establish a baseline so old notification shade contents do not replay.

An optional `CONTEXT_STATE` tuple accompanies the same queued context snapshot:
exactly nine bytes with little-endian uint32 session token, uint32 burst counter
and a flags byte whose bit0 indicates retained paused music. Boundary/history
packets carry event0. A new independent allowed burst increments the counter;
coalesced text/icon updates retain it. Rebaselining within a native session does
not reuse event identities. This side tuple leaves the v4 record format intact.
Native accepts presentation state only with the matching session and accepted
context snapshot. Vibration additionally requires an open notification gate and
fresh notice; repeated or older burst identities cannot buzz again. The native
Off / Short / Double preference defaults to Off, and Quiet Time suppresses a
burst without deferring it. The source supplies no native already-alerted flag,
so this cannot deduplicate the OS's own notification vibration.

The shared queue replaces obsolete waiting snapshots within each source
channel; settings remain FIFO. SUPERSEDED is terminal, not a source failure or
transport acknowledgement. Transient snapshots retain their reconnect epoch.

## Settings ownership

The phone page uses three groups: Appearance, Content and Connection & help.
Appearance keeps sample preview and colour/time choices together. Its five
examples are Idle, Music, Banner, Message and Timeline, independent of feature
and privacy choices. Disabled features receive a nearby note; viewing their
examples never enables them. There are 20 native presets: five states, two
clock formats and two themes. Custom recolouring is explicitly approximate
and preserves provider artwork/OS-owned areas.

Theme exploration retains the session's Custom palette. Content explains that
left/right readings follow shared Pebble slots, and source recovery appears
beside the affected summary. Notification choices explain app/sender visibility.
Selected app filters are removable entries, matched by exact case-insensitive
name/package; they are not validated against installed apps. All new strings
and controls cover the existing 17 supported locales.

The phone page has explicit light/dark foreground, background and form-control
colours independent of watch palettes. This includes native selected values and
options, avoiding mixed inherited Clay colours. A `DYN1-FFFFFF-000000-55AAFF`
preset contains only background, text and accent. Import validates all three
before changing the main palette; it cannot alter privacy, filters, haptics or
other settings.

The fixed Save area distinguishes unsaved edits from last-delivery state.
It does not claim a new draft has reached the watch merely because a previous
save was delivered. Phone-only privacy choices configure context immediately
when submitted. Valid appearance/time fields are kept in a nonprivate pending
delivery record with bounded retries and acknowledgement. Reconnection retries
the latest save automatically; an older ACK cannot mark a newer save delivered.
Source health and delivery are snapshots at page opening, not a live bridge.

## Allocation and validation

One bounded NativeState allocation at startup holds core state, rendering
snapshots and fixed raster scratch outside the SDK's limited load image.
Initialization failure exits before subscribing services; teardown cancels
callbacks and frees that block. Rendering adds no per-frame state buffers.
SDK load usage and runtime allocation are reported separately.

Build/package/source checks establish artifact identity. Native stills and
intermediate transitions check appearance and fitting; browser checks exercise
actual Clay controls and saving. Emulator/plugin fixtures are separate from
physical-phone delivery, hardware animation and battery-life verification.
See [the build-specific validation record](VALIDATION.md).
