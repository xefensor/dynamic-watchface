# Dynamic validation

## 0.2.0 native-style rebuild

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
