# Dynamic validation

## 0.3.2 bottom-anchored music and compact date

Music uses a measured native layout with a fixed visible bottom at y216.
A 40px artwork target and up to two artist lines share that bottom; up to
three title lines grow upward with an 8px gap. Metadata reserves at least
40px when artwork or artist text exists. If both are absent, the title uses
the bottom anchor itself. Preferred Big sizes fall back only when needed
for readable line spacing and the y86 content boundary.

- **48 JavaScript tests and four strict C99 suites pass.** The music suite
  checks UTF-8 character boundaries, word and unbroken-word wrapping, final
  ellipses, comma-aware artist breaks, fixed bottoms, ascent/descent spacing,
  missing metadata and Big fitting. Its geometry uses exact bundled metrics;
  deterministic test advances isolate wrapping, while emulator checks use the
  actual SDK text widths. Music tests also pass ASan/UBSan with leak detection
  disabled for this environment.
- All **7,360 ascents and 7,360 descents** match the exact packaged PFO bitmap
  metrics and linked ARM tables. A checksum-consistent wrong ascent is rejected.
  Source fonts, font resources, clock outlines, core pin, UUID, permission and
  all 33 wire-key bindings remain unchanged from 0.3.1.
- The compact weekday/date use baselines 19/40 with staged separation during
  layout transitions. Idle and Timeline Peek retain the inline date. The
  conditional status indicators remain below the compact date.
- Two current actual Clay captures pass at 390px: English/light and Czech/dark,
  showing version 0.3.2 with no page errors or overflow. Settings behavior and
  the existing 17 translations are unchanged.
- **50 effective native rendering cases and 68 native stills pass.** Short/
  three-line titles, two-line artists, 48-byte/unbroken/UTF-8 content, missing/
  transparent/padded/portrait art, descenders, Big fitting, sparse readings,
  12-hour/wide clocks and dates, idle/music/notices/Peek and light/dark pages
  were checked. Root reviewed all nine contact sheets, the corrected notice
  supplement and six wide-date/track motion filmstrips.
- Pixel checks confirm visible cover/last-artist bottoms at y216 in the music
  cases, primary opaque title-to-metadata gaps of 8px and title ink at or below
  y86. During both track slides, the header and both readings are pixel-constant,
  and every captured cover bottom remains at y216. Active dates have two bands;
  idle and Peek dates have one.
- Fifteen motion paths and two animation opt-out runs pass. One notice-expiry
  check restores the exact preceding music image. Artist/mid-track clears can
  first capture the prior framebuffer before the next redraw; every subsequent
  pose clears private content. Native stills are unchanged; raw HMP frames are
  retained and exact palette recovery removes only QEMU backlight scaling.
- The first positive notice fixtures mistakenly used 120-second music leases;
  the core correctly rejected them. Corrected 12-second notice states and
  three affected transitions replace them in the effective results. Five
  superseded raw stills/reports and two initial preview stills are retained
  separately; they are not counted as additional passing cases.
- Captures use the exact final PBW, production core codec fixtures and native
  AppMessage, with no phone JavaScript. The SDK locale is English; Czech date
  wording was not injected. Live phone/plugin/media/notification callbacks,
  actual Timeline event text, physical PT2 behavior and hardware FPS/power
  remain untested. These are emulator render/interaction checks.
- SDK 4.33.1 reports **246,928 resource bytes**, a **60,838-byte RAM footprint**
  and **70,234 bytes free heap**. The native binary is 51,404 bytes. The resource
  pack is byte-identical to 0.3.1.

PBW SHA-256:

`41ba35aab4ccf6c49f8416ad6cf3206fe29cfc58e956261edcb14507075b4cb8`

Native binary SHA-256:

`4681302d80ef430f4a3b0ce0d9e5b0a06d923e8a46b012f196cb8326936ecaf0`

Resources SHA-256:

`2bf4e3cbeaae59e48d65993ece095cc59364cc91528485a588c8fdd254bb8fbb`

## Historical 0.3.1 preview fidelity correction

The clock and reading values now use fractional 4×4 pixel coverage rather than
rounding every outline vertex to a whole pixel. The original font geometry,
advances and layout remain intact. Source-font equality and native raster
appearance are checked separately.

- **48 JavaScript tests and three strict C99 suites pass.** The new raster suite
  checks fractional area, counters, disjoint contours, negative clipping,
  capacity rejection before drawing, winding invariance and exact rational
  sample boundaries. It also checks all 20 actual glyphs at six sizes against
  their source polygon area. The raster/asset suites pass ASan/UBSan with leak
  detection disabled for this environment.
- Eight packaged body sizes contain **919 source codepoints plus a visible
  fallback**. Cyrillic, Greek and the font's Latin Extended Additional
  repertoire are retained. There are **4,312 direct glyph-ink checks** and
  **7,360 exact packaged bitmap-descent checks**. Missing characters use the
  authentic upstream `.notdef` outline rather than disappearing silently.
- The 20 Fredoka vector glyphs include `°CF` for temperature and percent units.
  The linked point/contour/glyph and body-metric tables match their generated
  source exactly. Original wire keys, UUID, ReadMusic and the core pin remain
  unchanged. Four bitmap/fallback corruption checks are correctly rejected.
- Same-content native/browser comparisons use 10:49 idle and 10:48 music,
  6°C/100% and the saved orange accent. The rejected outline experiment added
  unwanted weight. The accepted fractional raster closely matches the
  grayscale-quantized preview without expanding the stroke; native screenshots
  remain native pixels. This does not claim browser-pixel identity.
- Reading units use Fredoka, labels use muted 11px text, and label opacity
  follows the existing layout animation. All raster scratch is bounded and
  reused; it adds no per-frame heap allocation.
- Two current actual Clay captures pass at 390px: English/light and Czech/dark,
  with the 0.3.1 version and no page errors or overflow. Settings behavior is
  otherwise unchanged; the earlier 68-case matrix is recorded below.
- **25 focused native rendering cases and 36 still captures pass.** Normal/Big
  idle, music, notices, Peek, C/F/percent/range values, multiple clock families,
  light/dark/custom colors, Cyrillic, Greek and supported Vietnamese text were
  visually checked. All five contact sheets, including the corrected positive
  Vietnamese sample, were reviewed. Rare unsupported characters remain a
  separate visible-fallback case; coverage is not universal Unicode support.
- Eight captured paths settle, and all eight normalized final frames equal
  native Screenshot-service endpoints. Notice transitions preserve the clock
  and both readings. Two opt-out runs, including relaunch, show endpoint-only
  geometry. Artist/body clears are blank from the first captured frame; a
  mid-slide clear retains one pre-redraw framebuffer before every returning
  pose clears. This is normal dispatch/redraw latency, not a zero-latency claim.
- Captures use the exact final PBW, production-packed fixtures and native
  AppMessage, with no phone JavaScript. Raw HMP frames are retained; exact
  per-frame palette decoding removes backlight scaling before geometry counts.
  The SDK locale is English; Czech date/label wording was not injected. Live
  plugin delivery, physical playback, real notification callbacks, actual
  Timeline event text and hardware power/FPS remain untested. Prior expiry
  lifecycle results below were not repeated as new 0.3.1 checks.
- SDK 4.33.1 reports **246,928 resource bytes**, a **51,474-byte RAM footprint**
  and **79,598 bytes free heap**. The final native binary is 42,072 bytes.

Final PBW SHA-256:

`f98a6bc977e176d121d391ae98b5d6305d40aa0394dd90f33babc92585e3f781`

Native binary SHA-256:

`da6c02dccebebadb4d1159d68f653fb88fdd94ddf7a3777c0511828cbf5130b1`

Resources SHA-256:

`2bf4e3cbeaae59e48d65993ece095cc59364cc91528485a588c8fdd254bb8fbb`

## Historical 0.3.0 chunky animated build

This is the native Emery implementation of the approved motion study, using
Fredoka outlines and bundled Roboto Condensed text. The original UUID and
Pebble Watchface Core 0.1.0 remain unchanged.

- **48 phone-side tests and two strict C99 suites pass.** The native suites
  cover pose interpolation, icon/value separation, unobstructed heights down
  to 160px, UTF-8 handling, probability validation and generated font contracts.
- **68 actual Clay browser cases pass:** 17 languages × 320/390px × light/dark,
  with no page errors or overflowing/clipped controls. Real animation-toggle
  saving and named/numeric preference restoration pass. These are fixture
  sessions of the actual form, not physical-phone captures.
- The exact packaged build passes SDK payload, source-map, UUID, permission,
  pinned-core and message-key audits. All original 32 bindings are preserved;
  `ANIMATIONS` is appended at 10032. Four host and five core modules match their
  sources. Six intentional corruption checks were correctly rejected.
- Seven packaged body fonts provide 396 selected source codepoints plus the
  fallback glyph. **686 required localized glyph checks** and **2,779 exact
  packaged bitmap-descent checks** pass. The Fredoka point/contour/glyph arrays
  and body-metric tables are verified inside the ARM binary. Both asset
  generators reproduce their committed headers. The clock has 17 glyphs,
  1,376 points and a maximum 145-point contour. Native font rasterization was
  inspected; the open counter in the digit 2 matches the original font.
- **77 native rendering cases pass**, including normal/Big text, sparse/empty/
  stale/loading readings, long text, 12-hour and wide clocks, numeric units,
  Unicode minus, generic percentages, ranges, bitmap/image-only readings,
  artwork shapes, light/dark/custom colors, descenders, four localized content
  samples and native Timeline Peek. All 12 contact sheets were visually reviewed.
- Three lifecycle checks pass: music expires without menu navigation, a notice
  expires back to fresh music, and relaunch clears private context. Three
  privacy-clear paths remove content during motion. Idle/music pixel stability
  and the ignored legacy shadow preference pass.
- Nine initial motion paths and nine focused repeat captures were inspected.
  The focused runs show **4–10 distinct geometry poses including endpoints**
  (2–8 intermediate poses). Track/notice changes retain the clock and readings;
  all focused runs settle. Two Animations Off checks, including relaunch,
  show no intermediate geometry. Native Timeline motion remains OS-controlled.
- Motion fixtures are prepared before their capture trigger. Raw HMP captures
  are retained; exact per-frame four-level palette decoding removes the QEMU
  backlight fade before counting geometry. Settled images match the native
  Screenshot service. This is a capture correction, not a change to artwork
  or positions. An earlier harness assertion counted ordinary old-to-settled
  redraw latency as an animation; the corrected focused check is recorded
  separately from that retained diagnostic result.
- A supplemental launch/status-toggle check shows both battery badges with
  the virtual watch at 5% and a phone fixture at 15%. Real hardware battery
  callbacks and power use have not been measured.
- SDK **4.33.1**, with emulator firmware **4.33.2**, targets Emery only.
  The resource pack is **98,449 bytes**, including 94,341 bytes of font payloads.
  The SDK reports a **40,280-byte RAM footprint** and **90,792 bytes free heap**.
- Native captures use the exact release C/resources with production-packed
  fixtures; phone JavaScript is not executed during those captures. The final
  release changes only phone help text and SDK manifest timestamps relative
  to the tested native candidate. Native binary/resource bytes and remaining
  platform-manifest fields are equal. Live plugin delivery, physical playback,
  Bluetooth, real Timeline event text, full installed-language-pack coverage
  and physical PT2 behavior still need a device check.

Release PBW SHA-256:

`04489566a5f843ee611ac83df905ba91435c6e8e175a2ea3cd9d18f74c1f9c11`

Native binary SHA-256:

`ffb6fe2b7602e7b57d85e023fdcc5743185b691e86d3a955a5f1471675258f52`

Resources SHA-256:

`8867b114b67ac3c9eb83340cc5093018aef4e9026e0ec81dea15a940d24cf7f6`

Current image, font and core provenance is in
[asset-provenance.json](asset-provenance.json). Historical validation below is
retained with each corresponding source archive in `dist/`.


## Historical 0.2.0 native-style rebuild

Dynamic 0.2.0 replaces its bundled fonts with PebbleOS LECO and Gothic,
introduces native-style information bands, and retains Pebble Watchface Core
0.1.0 unchanged. It remains an Emery-only, sideloaded plugin-preview build.

- **44 phone-side tests and one strict C99 layout suite pass.** Geometry covers
  normal/Big text, idle/context states and every unobstructed height from
  171–228px. UTF-8 weekday abbreviation, 12-hour parsing and numeric minus
  normalization are also checked. The layout suite passes ASan/UBSan with
  leak detection disabled because this container blocks LeakSanitizer's
  process inspection. This suite does not allocate dynamic memory.
- **68 actual Clay browser cases pass:** 17 languages × 320/390px × light/dark.
  No page errors or clipped/overflowing controls were found. Real toggles,
  app filters, the 64-color palette, black accent selection, saving and legacy
  preference restoration pass. Preview images use the actual form with a
  fixture bridge, rather than a physical phone session.
- The packaged-build audit checks identity/version, manifestVersion 2,
  ReadMusic, all 32 message keys and the original 31 key numbers. Native,
  resource, JavaScript and source-map payloads match the SDK build. Four host
  modules and five pinned core modules match their sources. There is no
  independent legacy weather HTTP client.
- **50 final native rendering cases pass**, covering normal/Big text,
  midnight/12-hour/wide clocks, long text, sparse/empty/stale/loading readings,
  Unicode minus, units/ranges, missing/rectangular/padded artwork, dark/custom
  colors, status, descenders and Timeline Peek. All eight contact sheets were
  visually reviewed. **17,920 exact source-art pixel checks pass.**
- Three native lifecycle checks pass: music expires without menu navigation,
  a notice expires back to fresh music, and relaunch clears private context.
  Idle/music frames remain stable and the legacy shadow setting has no visible
  effect. Music and notification Peek states are pixel-identical at each text
  size. The final matrix contains **60 captures** including lifecycle/stability
  snapshots; the release activated normally without developer-slot recovery.
- One supplemental launch capture verifies both low-battery badges with the
  emulator watch at 5% and a 15% phone fixture. A live watch battery-change
  callback remains unverified; physical battery behavior is not claimed.
- All 17 translated source-content fixtures were production-packed. Native
  Czech, Hungarian, Romanian and Turkish content samples render cleanly.
  The emulator exposes no locale/date-language injection, so these samples
  do not establish date coverage for every installed language pack.
- SDK 4.33.1 reports **4,092 resource bytes**, a **22,245-byte RAM footprint**
  and **108,827 bytes free heap**. The resource pack contains the SDK empty
  table, with zero custom entries.
- No custom font resources are bundled. System font references are checked
  against SDK 4.33.1's catalog and the actual native binary. Firmware glyphs
  and installed language packs are outside the PBW; the historical 0.1 font
  corpus counts below do not apply to this version.
- Native screenshots use the exact release C/resources, idle phone JavaScript
  and production-packed fixtures. Live plugin delivery, physical playback,
  Bluetooth behavior, actual Timeline event text and every installed firmware
  language pack still need a real phone/watch check. The emulator's native
  Peek card is blank.

Release hashes:

- PBW: `afb2c9ba0cfd2882668c5581e44571459b4f3d16d8acd4ede121c8708804daac`
- Native binary: `73d9f9a3a844358b68213ca3866852efa589ef372c932fd89b1199a1bc382b93`
- Resources: `b58d8f0fc04367f823045aafa7724dee47b1fe6674411d117e32909b3f527a31`

Image and core provenance is recorded in
[asset-provenance.json](asset-provenance.json).

## Historical 0.1.0 validation

This first borderless design uses the same pinned Pebble Watchface Core 0.1.0
release as Halftone rc.6. It is a sideloaded plugin-preview prototype.

- **39 phone-side tests and one strict C99 geometry suite pass.** Core's own
  83 JavaScript tests and three native suites cover source subscriptions,
  transport, packet validation, cache failures, expiry and reconnect/privacy
  handshakes. Dynamic's geometry suite also passes ASan/UBSan.
- **68 actual Clay browser cases pass:** 17 languages × 320/390px × light/dark.
  There are no page errors or clipped/overflowing controls; real toggles,
  app filters, 64-color palette changes and Save pass. Settings previews use
  this actual form with a fixture bridge, not a physical-phone session.
- The exact-PBW font verifier covers **13 clock fonts, 25,920 formatted clock
  presentations, 522,648 localized weekday/date presentations and 820 local
  context-glyph metrics**. Minimum date/clock gap is 6px and accent ink starts
  at least 3px below the top. Native font resources match the selected SDK build.
- **40 final rendering cases pass**, covering standard/Big text, wide/12-hour
  clocks, music and notifications, two-line long titles/body, sparse/empty
  readings, missing/rectangular/padded artwork, colors/status and Timeline Peek.
  **10,848 source-pixel checks pass.** All contact sheets were visually reviewed.
- Three native lifecycle checks pass: music expires without menu navigation,
  a notice expires back to fresh music, and relaunch clears private context.
  The final run has **45 native captures** (40 rendering + five lifecycle
  snapshots), and activated normally without SDK launch recovery.
- Earlier diagnostic installations hit developer-slot/AppFetch activation
  failures. Those are kept separate from the final clean build. Switching
  directly between both UUIDs without reinstall also could not be verified
  through this emulator developer slot; no real coexistence/isolation result
  is claimed. The two app manifests have distinct UUIDs and phone cache keys.
- Native screenshots use exact release C/resources with idle phone JavaScript
  and production-packed fixtures. Live plugin data, physical playback,
  Bluetooth callbacks and real Timeline event text still need a phone/watch
  check. The emulator's native Peek card is blank.
- SDK 4.33.1 targets Emery only: **149,964 resource bytes**, a **25,328-byte RAM
  footprint**, and **105,744 bytes free heap**. Permission packaging retains
  the exact SDK payloads, original Dynamic UUID, all 31 message keys,
  manifestVersion 2 and ReadMusic. Four local phone modules plus five core
  modules are active; there is no independent legacy weather HTTP client.

PBW SHA-256:

`e7719433e2cc2f5e4025888190078164eabbed82abf61dba927fab7f1439e91a`

Native binary SHA-256:

`d041950361c75fd26a6620f31a6e1de169c3350df134f3e0c76519813ae8a21c`

Resources SHA-256:

`b52b3456e45f5bdeb4e97ecec2ddf188953f6cdade42aa33feaf58070433e803`

Image and core provenance is recorded in [asset-provenance.json](asset-provenance.json).
