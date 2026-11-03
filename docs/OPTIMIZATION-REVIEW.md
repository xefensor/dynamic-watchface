# Dynamic 0.16.0 optimization review

This local candidate preserves the approved layouts, static text, artwork,
bottom-aligned primary content, one-row secondary and bounded transitions.
Published 0.15.0 assets and tags remain unchanged. The complete suite review
is also recorded in the workspace's optimization validation folder.

## Native rendering and memory

One existing activity plan is reused across scheduling, painting and motion.
It invalidates on a received snapshot/session, connection, locale, relevant
clock second or earliest source/datum expiry. All candidate clocks participate,
including a secondary that temporarily yields because its essential value does
not fit; only actually visible changing data schedules one-second callbacks.
No second plan or complete-card copy is allocated.

A prepared plan is distinct from a stable painted plan. Heartbeats reuse only
the last stable native paint after rechecking focus, Peek, motion, visibility,
identity, time and expiry. Preparing an incoming plan never confirms it as
visible. Partial animation paints are ineligible. Final unobstructed geometry
is explicitly synchronized. The existing 30-second status refresh, 45-second
maximum routing lease and helper launch confirmation remain intact.

Activity-only messages no longer update the wall clock and unchanged palette.
Unchanged settings skip persistence writes; original scalar/bool formats remain.
Reading/status changes repaint the background, and activity changes repaint the
activity layer unless geometry also changes. SDK dirty calls can coalesce, so
the claim is reduced work and invalidation, not a multiplied frame-rate saving.
Artwork and reading bitmaps paint equal-colour horizontal runs with the same
sampling, scaling, palette, alpha and monochrome handling as the previous pixels.

Core 0.5.1 validates the entire legacy context packet, including discarded
music/notification records, then retains only a 12-byte public phone-status
record. Native session/baseline and notification effects stay host-owned.
This also shares the decoder with readings instead of retaining another native
copy. The wire contract and full Core API remain compatible.

| Native measurement | 0.15.0 | 0.16.0 |
| --- | ---: | ---: |
| Loaded image | 62,248 B | 61,768 B |
| Static virtual extent | 62,664 B | 62,184 B |
| Headroom under 65,535-byte static limit | 2,871 B | 3,351 B |
| NativeState allocation | 20,536 B | 15,288 B |
| One actual startup free-heap sample | 40,708 B | 46,436 B |
| Resource pack | 246,928 B | 246,928 B |

The SDK separately reports a 62,182-byte footprint. The two-byte difference from
the audited static extent is alignment/accounting; these are not interchangeable
with live heap or a peak-memory measurement.

## Phone and release work

Activities 0.4.2 supplies a supported host entry without unused publisher code.
Its consumer avoids redundant catalogue copies, image serialization for status,
and repeated metadata lookup, preserving defensive public copies and immutable
source revisions. Dynamic caches settings/preferences until actual settings
events, invalidating even after a partial storage failure. It defers Clay,
configuration, custom page and preview initialization until configuration opens.
This defers execution; those modules still belong to the offline bundle.

Phone preview and footer measurements coalesce within a browser frame.
Unchanged footer geometry is not rewritten. Preview recolouring reuses mappings
for repeated colours and avoids per-pixel callback allocations, preserving the
original result and protected artwork/Timeline areas.

Release packaging uses pinned build-only Terser 5.46.0. Expression compression
and property mangling remain disabled because Clay serializes functions as
source. UTF-8 translations remain UTF-8. Generated identifiers exclude `$`: Clay
inserts function source through replacement-string semantics, where a generated
`$&` could expand into its template marker. A real Clay-generated-page regression
and the actual packaged browser test exercise this compatibility requirement. Normal PBWs exclude external maps;
the separate debug ZIP retains original SDK/plugin inputs, exact chained maps
and minifier provenance. Artifact and committed-HEAD checks independently
reproduce the runtime code and maps instead of trusting claimed hashes.
Required licences remain bundled; development SDK output stays original.

## Validation

- 121 JavaScript tests, 13 strict portable C suites and 49 Python checks pass.
- SDK 4.33.1 Emery build and complete source/pin/font/resource/minification
  artifact audit pass. The executable and resources match captured bytes.
  SDK manifest generation timestamps are verified separately; SDK files were
  not restored or rewritten to hide that metadata difference.
- 24 native settings presets, 64 activity compositions and 24 local-clock
  follow-ups pass. All 88 activity-region comparisons match 0.15.0 using
  explicitly recorded clock-frame alternatives. Idle wall-clock/date pixels
  remain separately time-dependent. Root and implementing agent inspected
  representative native frames.
- Native expiry/promotion, confirmed role indices, a no-resend 30-second
  heartbeat, routine/late-art stillness and bounded track-change motion pass.
- The final packaged cross-app test covers six notification privacy/role cases,
  48 selected launches, 48 expired closures and 336 unselected closures.
  It executes the installed optimized scripts with synthetic SDK/native input.
- Browser evidence in `qa/0.16.0/settings/` includes 24 captures, actual minified
  packaged EN/CS pages, source-page privacy/order/touch/keyboard and responsive
  checks. Forty window or visual-viewport resize events produce one deferred
  measurement and zero unchanged geometry writes; forty palette events produce
  one draw/recolour. All twelve state/format custom-colour RGBA arrays match the
  original 0.15.0 recolouring algorithm exactly.

No physical watch or real phone measurement is claimed. Native draw timing,
peak stack/heap, phone cold-start latency and physical battery gains remain
device measurements. Additional glyph/framebuffer caches would consume RAM;
the Timer raster profile records work without pretending host timings justify
that extra allocation.
