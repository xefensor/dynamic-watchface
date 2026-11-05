# Dynamic 0.16.1 battery and activity reliability

The local candidate preserves the published 0.16.0 layouts, preferences,
protocols, identity and artifacts. Core stays at 0.5.1. Activities 0.4.3 is pinned
to source `fbd3ce42771a299e9ce79a23338ac254387f9d1b` and archive SHA-256
`049b5db1a20ccfec123291f8807302a6ee65835c5ca171bc302b546ad4b72123`.

While a system window covers Dynamic, source validation and expiry continue.
Dynamic cancels its decorative motion, activity-second and presentation timers,
and defers its own layer invalidations. The OS remains free to request a
background paint. Covered paints neither confirm nor withdraw the last recent
navigation hint; they do not renew its original source/role or 45-second limit.
Returning focus reconciles the latest identity, expiry, clock and palette before
painting, with no animation replay of changes that occurred beneath the cover.

A minute-only wall-clock update no longer repaints a current static activity.
Running clocks, locale changes, expired datums, changed geometry and promotion
still invalidate the activity plan. An unavailable decorative timer settles the
final composition immediately rather than leaving an intermediate slide.

Phone bootstrap attempts now have a generation and one tracked retry timer.
A native session marker cancels obsolete retries. Content-free FIFO tickets let
the shared transport discard an obsolete queued bootstrap request before it
could reset that new native session. These tickets never cross AppMessage or
enter persistence; unrelated settings and snapshot ordering remain unchanged.
Activities 0.4.3 additionally retains healthy subscriptions on configure, clears
private/current views on disconnect, and preserves bounded source-expiry and
reconnect delivery barriers. An explicit refresh still requests fresh sources.

The callback regression compiles the actual production C callbacks against a
counted fake SDK. Twenty covered incoming updates cause zero decorative timer
registrations and zero host dirty requests. It also checks late callbacks,
focus-return geometry, static minute updates, clock/locale/expiry invalidation,
and decorative allocation failure. Four phone regressions exercise cancelled
timers, duplicate Ready generations, held queue ordering and late transport
failure after native confirmation.

Evidence under `qa/0.16.1/` includes 125 JavaScript tests, 13 portable C suites,
the actual-callback harness and 50 Python checks. The Emery SDK build keeps the
resource pack at 246,928 bytes and reports a 62,478-byte footprint; this is SDK
accounting, not a live-heap or peak-memory measurement. Twenty-four preview
images are captured from the candidate's actual native payloads. Focused native,
final packaged-source and browser checks are recorded separately as completed.

Focused actual Emery captures verify an advancing local countdown without a
resend, an unchanged static activity during its 30-second status heartbeat,
an actual system-call cover, expiry/promotion beneath that cover and a stable
promoted view on return. The full native contact sheet was reviewed. Actual
minified packaged EN/CS Clay pages pass the browser suite, including preview,
privacy, save, responsive-layout work and twelve custom-colour pixel comparisons.
The final package audit checks exact native source/ELF, resources, fonts, pins,
licenses, optimized JavaScript and maps. Captured native executable/resources
match the final PBW; only SDK manifest generation timestamps differ after the
preview-bound rebuild. Loaded image is 62,064 bytes and static extent 62,480,
leaving 3,055 bytes under the old SDK's 65,535-byte static limit.

The final cross-suite packaged test covers six privacy/role cases, 48 selected
launches, 48 expired closures and 336 unselected closures, plus provider hiding
during deferred discovery and native-session reconnect invalidation/restoration.
These execute the actual optimized scripts with synthetic SDK/native input.

These are reduced work counts and lifecycle checks. No physical-watch battery
percentage, render timing, phone delivery reliability or end-to-end Quick Launch
success is inferred from synthetic or emulator evidence.
