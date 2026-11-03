# Dynamic and the shared libraries

Dynamic 0.12.1 pins Pebble Watchface Core 0.5.0 and Pebble
Activities 0.4.0. Each PBW embeds its libraries and keeps its UUID, settings and
permission manifest. Halftone retains its own independently adopted pin.
Neither library is a background app.

## Boundary

| Component | Owner |
| --- | --- |
| Pebble subscriptions, source identity/observation metadata, receipt cache and transport queue | Shared core JavaScript |
| Packet validation, native snapshots/cache, expiry and reconnect/session state | Shared core C |
| Activity discovery, summary schema 4 and legacy adaptation, ACT3 projection | Activities JavaScript |
| ACT2/ACT3 validation, item visibility, independent clocks and source/view revision checks | Activities C |
| Clock/date, semantic labels, layout, fonts, artwork and transitions | Dynamic |
| Native AppMessage dispatcher, SDK services, timers and OS clock-format query | Dynamic host |
| Settings form, translations, permissions, preview and delivery status | Dynamic host |
| SDK graphics services, system/fallback glyphs and Timeline event panel | PebbleOS |

The libraries impose no coordinates or font family. Native callbacks forward
events to their state objects; Dynamic renders the resulting snapshots. Phone
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

Media context is anchored to bottom y216: optional 40px artwork and up to two
20px artist lines. The 24px title uses up to three lines alone or two with a
secondary. Its measured title block ends 8px above the footer; spare space
remains above that block. Every line starts from the received beginning and
ends in a measured ellipsis when necessary. There is no continuation state or
paging timer. The phone adapts legacy Music into this same ACT3 renderer.

Notification title text and app label are separate fields. The sender/title
keeps the primary heading; an app icon normally supplies context. A quieter app identity
sits in the bottom context footer. Long app labels do
not shrink the sender. A summary banner reduces song-title space and retains
its footer; an extra app-name line may reduce available title lines further.
A message preview uses a bounded body panel. App-only mode instead places the
app name in the heading and omits sender, message body and duplicate app label;
a missing app name may fall back to package identity, never sender text.

Content identity and image refreshes have different motion rules. An explicit media-item
token defines a track; legacy Music derives it from the received title. A track
change starts a 280ms incoming-only content slide; late artwork or artist
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
Pebble's event panel. Legacy notification snapshots remain transient for their bounded transition.
Activities retain no outgoing copy. Expiry, invalidation, disconnection and
privacy clearing always take priority over completing motion.

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

## ACT3 summary views

Dynamic enables `nativeFormat: 3`. The coordinator prefers `activity/summary`
schema 4, then adapts a provider's older schema 3/2/1 convention. It subscribes
to one representation per provider. Sources publish standard headline/detail or
media title/artist/artwork. Dynamic requests display fields and `open_app`,
excluding the separate `actions` and `entity_ref` properties.

ACT3 retains main/support typed datums internally, with independent observation,
expiry, retention, estimated flags and clock arithmetic. A clock's own running
state is separate from source lifecycle. Countdown zero is a due value, not
native completion or an alarm acknowledgement. Optional ranges generate a
meter only from a real referenced value and explicit bounds.

The frame remains at most 2,672 bytes with two records. ACT3 uses a 104-byte
record header, a shared 304-byte UTF-8 pool and at most 800 packed image bytes
plus a 16-entry palette per record. Field bounds share the pool; they are not
simultaneously available capacities. Explicit presentation, identity symbol,
compact-datum selector, directions, qualifier and an eight-byte media token
replace renderer inference. The decoder accepts ACT2 compatibility frames too;
legacy source adaptation remains on the phone rather than linking ACT1.

Provider/run identity and source revision describe lifecycle. A session-bound
view sequence orders projections. A new view cannot rewrite the source state,
observation, validity or privacy at the same source revision. Resident older
revisions are rejected; terminal runs cannot become active under the same ID.
The phone owns ordering across projection windows. Reconnecting and rendering
never renew source observations or validity.

The main datum controls visibility; optional detail expires independently.
Known public Timer snapshots can predict deadlines only through their original
native lease. Other unconfirmed sources requiring a trust annotation are
omitted. Private content clears on disconnect and privacy loss. An explicitly
allowed terminal result yields to ongoing activities and Music; zero alone
never creates one.

Any activity can occupy either role. Dynamic-local priority chooses providers,
and the shared selector preserves source ordering within a provider. Projection
filters ineligible records before selection so an omitted source cannot starve
a valid third candidate. The optional secondary occupies one static row above
the primary, with a selected complete datum and optional 16px glyph. Optional
context yields before an essential value; a secondary whose value cannot fit
is omitted. Whole formatted scores and paces use body text rather than an
inferred numeric prefix. Directional values reserve their arrow's width.

Primary content grows upward when alone. The measured primary block is packed
against its footer with an 8px gap; meaningful labels, headline, supporting text
and a range indicator move together. Standard text may have two detail lines.
Context ends at y216, using an instance name or provider identity, or optional
artwork and artist for media. All media uses the shared renderer. Native Timeline
and notification interruptions retain
priority. One reusable heap-owned render plan borrows cards instead of copying
full cards or allocating artwork per frame. A second-boundary timer exists only
for visible running clocks. Covered/unfocused states suspend clock work.

## Recent app navigation

After planning actual visible roles, native sends a 24-byte presentation status:
version, two original record indices, flags, native session, view sequence,
presentation sequence, UTC report time and earliest role-changing expiry. Flag1
identifies a full legacy notification, only as the fixed Notifications app;
small banners preserve the underlying activity route. Motion reports no route
until the primary is settled. Local expiry/promotion sends new original indices.
A low-rate timer belongs to the latest report and refreshes after 30 seconds;
clock ticks do not send messages or write phone storage.

The phone freezes app destinations for each exact encoded view. Only matching
native status can publish a route. Persisted data contains destination UUIDs and
session/freshness metadata only, capped at 45 seconds, source expiry and native
role expiry. Reads cannot renew it; invalid/expired records are removed on
access. Dynamic's bundled read-only `navigation/presented_activities` source
returns one `current` instance with a bounded `hint.longText` record.

The separate Open Activity app opens the configured recent Primary or Secondary.
It may run from any watchface. Stock SDK cannot prove the immediately preceding
face or cancel an already-dispatched app launch. No firmware/mobile fork,
Dashboard implementation, run-specific deep link or per-watch routing is added.
Timer's watchapp retains state, pause/cancel truth and native alarm ownership.

## Pinned dependencies and v4 reading contract

The committed dependencies are `file:vendor/pebble-watchface-core-0.5.0.tgz`
and `file:vendor/pebble-activities-0.4.0.tgz`. `package-lock.json`,
`vendor/CORE.json` and `vendor/ACTIVITIES.json` record their integrity and
provenance, including exact source commits and archive SHA-256 values.
Updating either library repository alone does not update installed PBWs. Each
consumer adopts and validates a versioned archive independently.

Core preserves provider-formatted text and units, semantic/source kind,
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
If AppMessage cannot open, transport-dependent requests and replies remain
disabled while the local clock continues.

For the fixed-timestamp 0.9.1 native candidate, `NativeState` remains 22,472 bytes
on ARM. The SDK reports a 65,392-byte virtual image, 143 bytes below its
65,535-byte limit; loaded image bytes are 64,972. These are different from
runtime heap and stack usage. New native changes need another size check;
this is a tight code budget, not evidence of spare runtime memory. Rendering
and scheduling use a single bounded plan per call and the codec allocates no
heap memory or full temporary snapshot on the stack.

Build/package/source checks establish artifact identity. Native stills and
intermediate transitions check appearance and fitting; browser checks exercise
actual Clay controls and saving. Emulator/plugin fixtures are separate from
physical-phone delivery, hardware animation and battery-life verification.
See [the build-specific validation record](VALIDATION.md).
Historical 0.9.1 evidence: its package/native-style audit passed against the captured native
binary and resources. Its source audit checks the compact weekday table's
608 native bytes, all 17 locales and 119 abbreviations, Norwegian aliases,
regional/fallback lookup and exact packaged font coverage. Runtime evidence
covers 20 settings presets, 35 activity compositions and four clock follow-up
frames without a source resend, including supporting-item and meter expiry.
The same native session completes these checks without an observed app fault.
This does not measure runtime heap, stack high-water or battery use. Physical
0.9.1/live-provider behavior has not been verified. A successful user test of
the earlier release is recorded separately from this candidate's evidence.

## Publication refinements in 0.6.1

Core 0.4.1 revalidates active and queued notifications against each current
source snapshot. Redacted, malformed and newly filtered instances clear private
content immediately, including a queued coalesced replacement. The v4 packet
contract is unchanged.

Phone presentation resync restores validated saved watch settings on startup
and a new native session, using the same delivery generation/ACK protections as
a normal save. It never includes phone-only notification text/privacy fields.
The palette preview selector is page-only; previewing Quiet Time colours changes
no persisted watch preference or actual OS Quiet Time state.

The release PBW adds deterministic readable license entries while retaining each
SDK native/resource/JavaScript payload byte for byte. Store metadata and images
are prepared separately and validated against the final PBW/provenance. Preparing
a kit does not submit the app or remove Pebble's plugin-preview restriction.

## GitHub distribution boundary

The [release workflow](GITHUB-RELEASES.md) is distribution automation. It runs
portable checks and validates committed versioned files, then publishes those
exact bytes with checksums. It does not build another native renderer, repin
Core, regenerate source archives or submit a Pebble store listing.

The v0.6.1 tag/source archive remain at their original tested artifact commit.
Later main-branch documentation and automation updates do not rewrite published
files. A new runtime/package version gets a new prepared artifact commit;
existing published tags/assets are verified rather than replaced.
