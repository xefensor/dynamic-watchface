# Dynamic validation

## 0.10.0 identified cards and Dynamic-local priority — 3 November 2026

Every activity now carries an app name, image and current content. The optional
secondary card has a fixed divided band instead of an anonymous footer. Timer
uses an hourglass and one countdown. Music participates in the same host order;
any activity can occupy either role. Settings and saved preferences belong to
Dynamic, while Activities 0.2.2 provides reusable selection without storage.
Core remains 0.5.0. The exact Activities source is
`645b781919673e1da421e1a594312f181983ebc9`.

- **95 JavaScript tests, 12 portable C suites and 40 Python regressions pass.**
  Integration covers both Timer/Music orders, hidden apps, idle-provider metadata,
  saved unavailable providers, restored preferences, strict malformed preference
  handling and private Music. Canonical ACT2 sources retain their own UUID
  preferences when the same provider also offers the legacy music convention.
  Unchanged Music no longer triggers an ACT2 transfer for each notice/battery update.
- **SDK 4.33.1 Emery build and final package/native-style audit pass.** The
  static virtual image is **64,708 bytes**, leaving **827 bytes** below 65,535.
  Loaded bytes are 64,288; the complete binary is 66,592 including 2,304 relocation
  bytes. Resources remain 246,928 bytes. These are image-size measurements,
  separate from runtime heap and stack watermarks.
- **20 native settings presets, 44 activity compositions and six local-clock
  follow-ups pass** on Emery QEMU. Sources are synthetic and packets use the
  pinned production encoder. The production C decoder now applies every fixture
  statefully in sequence, checking real source/run lifecycle restrictions as well
  as packet validity. A fixture which reused a finished Timer ID was corrected
  to create a new run; production correctly rejected resurrection.
- Raw native frames were inspected directly and at their original 200×228 size
  in a contact sheet. Independent renderer review covered both roles, cached
  Timer bands, dark icons, canonical Music, long clocks/units and a complete
  18-digit scalar next to 40px artwork. Pixel checks verify visible secondary
  bands and whole-band hiding for essential support or an overwide secondary
  clock. Expiring support removes its own meter; independent clocks advance
  without a source resend. Cached data retains explicit original-observation age.
- The actual offline Clay page passes at 360×800 in English and Czech: touch
  dragging, desktop drag/drop, arrow buttons, keyboard, 44px controls, literal
  untrusted names, dirty/save state and a 64-entry unavailable-provider boundary.
  Real Clay responses are replayed through production PKJS with simulated SDK
  coordinators, then restored in a fresh host. All 17 locales include the new
  controls. Source hashes and the reproducible browser driver are preserved.
- Embedded previews now show canonical named Music cards. The final rebuild uses
  native application timestamp `1793661953`; final binary/resources match the
  captured payload byte for byte. Native module/font/license/pin checks pass,
  and the five-image held store kit is refreshed. Prior 0.9.1 and earlier artifacts
  and tags remain unchanged. Existing Timer 0.5.0 needs no rebuild or update.

Evidence: [native previews](qa/0.10.0/native/native-previews/previews.json),
[activity captures](qa/0.10.0/native/activities/activities.json),
[secondary visibility](qa/0.10.0/native/role-visibility.json),
[final package audit](qa/0.10.0/native/bundle-audit.json),
[distribution identity](qa/0.10.0/native/final-distribution.json) and
[actual Clay settings](qa/0.10.0/settings/README.md).

Final PBW SHA-256:
`2c0c182c57b6f5a855d3bbaa718e83fa8613258a7e1e9f97a984222dd496a506`.

This is a GitHub development prerelease for sideload testing. Physical-watch
appearance, live phone/provider delivery, Bluetooth behavior, vibration and
power use remain device checks. No native workout access, Dashboard app, global
preferences service or new activity control API is claimed. Pebble appstore
submission remains held.

## 0.9.1 activity glance prerelease — 2 November 2026

This development prerelease responds to the user's busy Timer screenshot
and acknowledged-result feedback. It pins Core 0.5.0 and Activities 0.2.1 at
source `d2251a1e4e3d8543ad2e8be01fccc8758e785fb6`. Published 0.9.0 and earlier
tags and versioned artifacts stay unchanged.

- **85 JavaScript tests, 12 portable C suites and 40 Python regressions pass.**
  Targeted cases cover matching/differing item provenance, independent pause,
  completed clocks without a false pause icon, ongoing/music/result ordering,
  compact Timer projection and the production ACT2 decoder's capture inputs.
  The settings status version is checked against the package version.
- **SDK 4.33.1 Emery build and final package/native-style audit pass.**
  The native virtual image is **65,392 bytes**, leaving **143 bytes** below the
  65,535-byte SDK limit. Loaded bytes are 64,972; the complete binary is 67,276
  bytes including 2,304 relocation bytes. Resources remain 246,928 bytes.
  This is static image sizing, not runtime free heap or stack high-water.
- Actual Emery QEMU captured **20 native settings presets and 35 activity
  compositions**, plus **four no-resend clock follow-ups**. All 20 settings
  frames are RGB-identical to the previously reviewed 0.9.0 native captures.
  All 35 activity frames and four follow-ups were inspected directly.
- New compositions include the user's one-minute Timer case, matching and
  differing observation ages, completed-result/music priority, an ongoing
  activity with a completed alternate and music, and acknowledged hidden
  results restoring idle or music. Clock labels, long captions, 24-hour values,
  light/dark artwork and meters, Unicode units and Timeline bounds retain
  their earlier coverage. Supporting-item/meter expiry and independent clocks
  still execute in the native renderer without a source resend.
- The Timer fixture uses the installed Activities production projection with
  explicit `timerPresentation: 'compact'`. It removes the inverse elapsed
  supporting clock and uses Timer identity as the clock label. Other providers
  retain their independent supporting items and clock labels. These captures
  prove rendering of synthetic source states, not a live Timer dismissal
  delivered through the phone; Timer's own tests cover its source lifecycle.
- Embedded native settings previews were rebuilt into the final PBW using the
  original native application timestamp, `1793656733`. Final native binary and
  resource hashes match the captured payload exactly. Only the manifest's
  `generatedAt` changes; the reports preserve original capture metadata and
  separately record final distribution identity.
- All raw screenshot hashes were rechecked. The five-image store kit was
  refreshed and `prepare-store.py --check` passes. Source packaging and
  committed-head release preparation follow final documentation review.
- A subsequent Timer QA boot reported a Dynamic not-responding overlay, but
  its original screenshot/log was overwritten before the installed binary or
  crash location could be established. A controlled follow-up installed the
  exact final native binary/resources, checked seven actual startup/relaunch
  handshakes, and automatically started Dynamic after two separate cold QEMU
  boots on the same flash. All 15 raw screenshots were inspected (one stock
  pre-install control and 14 Dynamic frames); preserved firmware logs contain
  no observed Dynamic fault. The original event did not reproduce and remains
  undiagnosed. These checks changed no app source or candidate bytes.
- The native process header records SDK tuple `(5, 106)`, independently of the
  `PBL_SDK_3` build define. The identified published PebbleOS source classifies
  this as a 4.x app and selects a nominal 4 KiB Emery stack before its guard.
  This is a source-level classification, not proof of exact emulator-firmware
  source identity or a measured runtime watermark. A standalone current-SDK
  compile reports 1,464-byte scheduling, 1,448-byte drawing and 480-byte
  preparation frames, unchanged from 0.9.0; nested calls add stack usage.

Evidence: [settings captures](qa/0.9.1/native/native-previews/previews.json),
[activity captures and review](qa/0.9.1/native/activities/activities.json),
[final native/package audit](qa/0.9.1/native/bundle-audit.json) and
[current image provenance](asset-provenance.json). The follow-up
[cold-start audit](qa/0.9.1/native/cold-start/cold-start-audit.json) preserves
its separate phase reports, screenshots and firmware logs; the
[stack interpretation](qa/0.9.1/native/cold-start/stack-interpretation.json)
links the exact published-source predicates and current compiler frame report.

Final PBW SHA-256:
`b810b5818b9aa7d04295f75252c62538bb0611fdd900fbea0e6adec7aee5028d`.

Physical-watch appearance, actual button feel, vibration, battery use and live
phone/plugin transport remain separate device checks. A disconnected Timer
cannot immediately retract a cached card on another native app; original
expiry and eventual acknowledged-state delivery remain relevant. No new Clay
browser session, Dashboard app, built-in Workout access or firmware integration
is claimed. Pebble appstore submission remains held.

## 0.9.0 activity views prerelease — 2 November 2026

This release uses Activities 0.2.0 and its bounded ACT2 projection. An
activity supplies a main item and optional supporting item, each with its own
text, value or clock, observation time, expiry and retention policy. Artwork
and a source-derived meter remain optional. Schema 1 providers are adapted on
the phone. Version 0.9.0 was published as a GitHub development prerelease from
commit `25724d4dc561ec61d5d887b4c37d5e237d95b222`. Both GitHub workflows passed,
and all three downloaded assets match the prepared bytes and checksums.
Publication-status documentation updates preserve these artifacts. The 0.8.0
release and its earlier evidence remain available.

- **85 JavaScript tests and 12 strict native suites pass.** Coverage includes
  source/view revisions, independent item clocks, expiry, privacy, provider
  ordering, retained results, units, readable fallback and two-card bounds.
- **40 Python release/pinning regressions and the final bundle audit pass.**
  The audit verifies the exact pinned Activities source/SDK bytes, all five
  host, four core and seven activity JavaScript modules, native headers,
  message keys, licenses, embedded preview identity and loaded/virtual image
  limits. All 17 weekday locales, 119 abbreviations, Norwegian aliases and
  packaged font glyphs are checked after the compact date-table change.
- The **SDK 4.33.1 Emery build passes**. The native virtual image is 65,120
  bytes, leaving 415 bytes below the SDK's 65,535-byte limit. Loaded bytes are
  64,700; the complete binary is 67,004 bytes including relocation data, within
  its separate 128 KiB file limit. Resources remain 246,928 bytes.
- The SDK reports 65,954 bytes of heap before startup allocations.
  `NativeState` is 22,472 bytes on ARM. Neither number measures runtime free
  memory. Static compiler reports show a 1,464-byte scheduling frame and a
  1,448-byte activity drawing frame; nested calls and SDK callers add stack
  use. Runtime heap use and stack high-water have not been measured.
- Actual Emery QEMU produced **20 settings presets and 28 activity scenarios**
  using production ACT2 packets. All 20 settings frames are RGB-identical to
  the previously reviewed 0.8.0 local-recovery frames. All 28 activity images
  were inspected directly, including artwork, light/dark meters, Unicode
  values, long/24-hour clocks, two activities, music and Timeline reservation.
- **Four additional no-resend captures** verify that independent main,
  supporting and secondary clocks advance locally. A paused main remains
  fixed while its running wall clock advances. A timed four-second case
  removes an expired supporting value and its meter while the main clock
  continues. These fixtures exercise the actual native renderer without
  running phone JavaScript.
- Raw screenshot hashes and the final C/resource identity are verified.
  Embedded settings previews were rebuilt into the final phone bundle. The
  native manifest's `generatedAt` changes during that rebuild; the reports
  retain the original capture metadata and record the final PBW separately.
  Native binary and resource bytes are identical to those captured.
- The five-image store kit was regenerated and its provenance check passes.
  Native screenshots retain their original 200 × 228 pixels. The existing
  appstore submission hold remains in place.

Evidence: [native settings previews](qa/0.9.0/native/native-previews/previews.json),
[activity renders and ticking checks](qa/0.9.0/native/activities/activities.json),
[final bundle audit](qa/0.9.0/native/bundle-audit.json) and
[artifact/image provenance](asset-provenance.json).

Final PBW SHA-256:
`1e097d404819fe4de5455955da1f6e9bc22cf5458389552223987a9142da79f5`.
Native binary SHA-256:
`c29f6e0343b6fc0012c7d3c56efc575309dd5bb4c5b65d74723558b10989c8ca`.

The successful capture sequence does not establish physical-watch behavior,
battery life, radio behavior or live plugin delivery. No new Clay browser
session is claimed; unchanged settings-control evidence keeps its earlier
scope. Timer 0.3.0 has separate native migration, alarm and deadline evidence
in its own validation record. Actual Timer-to-Dynamic delivery, phone restart,
Bluetooth loss and native alarm/vibration still need the phone/watch procedure.
No Dashboard app, firmware integration or built-in Workout access is claimed.

## Historical 0.8.0 testing prerelease

Xef authorized publishing 0.8.0 as a GitHub development prerelease for actual
phone/watch testing on 2 November 2026. The prepared PBW retains the exact native
and packaged-runtime bytes validated below. Real-phone/plugin delivery,
Bluetooth behavior, physical vibration and power use remain unverified. Public
appstore submission stays held; this authorization is for the GitHub prerelease.

## Historical local CLI recovery — 2 November 2026

At this recovery stage, Dynamic 0.8.0 was unpublished. GitHub `main` was checked at
`2ccea6101a84ede7d7ccf7dec6ddffe505767caa`; its newer recovery documentation
was merged into `recovery/0.8.0-2026-11-02`. The supplied patch passed
`git apply --check` against its documented 0.7.0 base, and all 224 snapshot
file hashes matched before changes. Existing releases and tags were preserved.

Fresh local checks pass: exact dependency installation with
`npm ci --ignore-scripts`, 82 JavaScript tests, 11 strict native suites,
29 Python release/pinning tests, the SDK 4.33.1 Emery build, permission packaging
and the package/native-style audit. The audit now accepts SDK source-map entries
with a directory prefix while still requiring the exact reviewed JavaScript
bytes and rejecting duplicate entries; its regression checks cover both forms.

The SDK reports a 62,834-byte RAM footprint and 68,238 available heap bytes
before runtime allocations. The load image is 62,420 bytes; the complete native
binary, including relocations, is 65,212 bytes. Resources are 246,928 bytes.
These measurements do not establish runtime heap use or power consumption.

All 20 settings presets and 17 activity scenarios were recaptured on actual
Emery QEMU over local FIFOs, with phone JavaScript omitted. The captured native
binary and resource hashes match the final distribution. All 20 settings frames
and 16 activity frames are RGB-identical to the saved captures; the long-heading
banner frame was reviewed directly as well. Clock numerals, two readings, fixed
music footer, long countdown/caption and native Timeline Peek remain intact.
The generated phone preview assets were rebuilt into the final PBW.

Current evidence is in [native previews](qa/0.8.0/local-recovery/native-previews/previews.json),
[activity captures](qa/0.8.0/local-recovery/activities/activities.json) and
[bundle audit](qa/0.8.0/local-recovery/bundle-audit.json).
`docs/asset-provenance.json` identifies the current candidate and store images.
Its PBW SHA-256 is
`a56c198fca4e7b5fcec2e438ecc0d678f8f29cdcc818972332b4fb9fea6f4bd8`.
The prior 0.8.0 reports below retain their historical scope. Local build metadata,
linker build identity and source-map paths differ from the recovered PBW; old
binary hashes have not been relabelled as current verification.

The local SDK uses isolated mutable state and a fixed metadata timestamp for
the capture/final-build cycle. Local tooling repairs the SDK's string timestamp
option and redirects its temporary symlink into `/tmp`. The sandbox also denies
Node's captured synchronous subprocess pipes, so portable tests use a runner
adapter with temporary files and `--test-isolation=none`. None of these tooling
adjustments changes the app's C implementation or its approved layout.

After final documentation, store provenance and source packaging, the committed
candidate is checked with `scripts/github-release.py prepare`. Publication stays
pending the local workflow. Hardware access was unavailable: physical vibration,
real Bluetooth loss, live mobile plugin delivery, battery use and Timer/Dynamic
sharing on the actual phone/watch still require Xef's device checks. No fresh
Clay browser session is claimed; existing unchanged browser evidence retains its
original scope.

## 0.8.0 grouped activity clocks

- 82 JavaScript tests, 11 strict native suites and 29 Python release/pinning
  regressions pass. Clock layout tests include long values, image/no-image,
  accented captions and bounded secondary content.
- The official SDK 4.33.1 Emery build and final bundle/style audit pass. The
  native binary is 65,212 bytes (323 bytes below the SDK cap); the SDK reports
  a 62,834-byte footprint and 68,238 bytes of heap before runtime allocations.
  Native resources remain 246,928 bytes.
- Captured 20 standard settings previews and 17 generic activity scenarios from
  the actual native renderer at 200 × 228. Running/paused clocks, long durations,
  a second timer, music, dark colours, artwork, long accented captions and compact
  notifications fit. Timeline cases keep time/readings above the reserved native
  OS area and omit the activity rather than squeeze it into that area.
- Settings previews reference the final native binary/resource hashes. The final
  PBW audit verifies these hashes after rebuilding the embedded preview assets.
- Provider selection tests choose the nearest running deadline even from the
  eighth Timer slot, put paused clocks after running clocks within that provider,
  and retain other provider positions. Core 0.5.0 and Activities 0.1.2 are pinned.

Evidence: [settings previews](qa/0.8.0/native-previews/previews.json),
[activity renders](qa/0.8.0/activities/activities.json), and
[final bundle audit](qa/0.8.0/bundle-audit.json).
The [recovery audit](qa/0.8.0/recovery-audit.json) records the fresh checks after
two interrupted Work chats. The rebuilt PBW is byte-identical to the recovered
release; its original native captures remain valid and were re-inspected.

These are emulator/sample-data and packaged-runtime checks. They do not certify
physical vibration, live mobile plugin delivery, battery use or native Workout
session access. Timer’s own alarm/migration checks live in its separate repo.

## 0.7.0 stock activity integration

Dynamic pins Core 0.5.0 and independent Pebble Activities 0.1.0. The UUID and all
41 existing keys remain unchanged; three activity keys are appended.

- 80 JavaScript tests, 11 strict native suites and 29 Python packaging/audit
  regressions pass. The optional music/notification adapter retains the existing
  baseline and privacy behavior. Core and Activities have their own test suites.
- The official SDK 4.33.1 Emery build passes. Load image is 61,508 bytes, below
  65,535. Resources remain 246,928 bytes; reported total native footprint is
  61,922 bytes with 69,150 bytes available before startup. All 14,720 original
  font ascent/descent values are preserved by the smaller deduplicated tables.
- 20 actual native settings presets and six generic layouts were captured through
  the Screenshot service. Main/footer text, two configured readings, paused
  timers, numeric glucose, elapsed time, compact banners and native Timeline Peek
  fit without shrinking the agreed text sizes. The elapsed session is a plugin
  fixture, not built-in Workout access.
- An actual QEMU/pypkjs phone handoff from the native timer demonstrates shared
  storage, cold plugin reads, automatic discovery, local countdown, pause and
  cancellation restoring music. Normal button navigation passes. Recorded
  Dynamic exits release all heap allocations; the sampled log reports 29,540
  used of 69,144 bytes before cleanup. This is not a power or hardware test.
- New settings controls and provider status are translated in all 17 locales.
  The current portable settings checks pass; the extensive browser checks below
  remain historical to their stated releases and are not relabelled as 0.7.0.

Evidence: [native presets](qa/0.7.0/native-previews/previews.json),
[generic layouts](qa/0.7.0/activities/activities.json) and
[reference timer handoff](https://github.com/xefensor/pebble-activities/blob/main/docs/qa/0.1.0/handoff.json).
Native preview binary/resource identity is checked again against the final PBW.

The reference timer owns its native alarm. A clean emulator run confirms offline Wakeup
launch at the deadline; a displayed zero on Dynamic remains separate from that
originating-app completion. A watchdog dialog occurred during repeated forced developer
launch commands; subsequent normal button roundtrips passed. Real mobile-app
integration, physical alarm/vibration, power use and launch stress are not
certified. No firmware fork, built-in Workout access or dashboard app is shipped.

## GitHub distribution after 0.6.1

[GitHub v0.6.1](https://github.com/xefensor/dynamic-watchface/releases/tag/v0.6.1)
is published as a prerelease from the original tested artifact commit. Its PBW
and source ZIP bytes/hashes remain those recorded below. Later main-branch
documentation and release-automation updates are separate from that snapshot.

The release helper passes 18 targeted tests. Live GitHub checks/publication and
identical-version reruns passed; the published asset IDs, sizes, creation times
and hashes stayed unchanged. Source archives reject generated/private working
folders and embedded PBW binaries. These are distribution checks, not additional
watch-render, live-provider or physical-device results. See the
[release guide](GITHUB-RELEASES.md) for triggers, validation and retry behaviour.

## 0.6.1 publication refinement candidate

The prepared release remains a **sideloaded plugin preview**. Its local store kit
is consistent and ready for review, but public submission is held by Pebble's
[current plugin-preview rule](https://developer.repebble.com/guides/plugins/preview/)
(checked 2026-11-01). No appstore upload has been performed.

- **72 Dynamic JavaScript tests and eight strict native C suites pass.**
  Core 0.4.1 passes **126 JavaScript tests and three native C suites**. Its
  native C is unchanged from 0.4.0, so the earlier Core ASan/UBSan result is
  retained; it is not claimed as a new sanitizer run on Dynamic's host changes.
- Core revalidates active and pending notification data on every source
  snapshot. Fully redacted, malformed and newly filtered notices clear their
  private text and cancel queued coalesced replacements. Regression cases cover
  full/app-only modes, same/different IDs and recovery within the original burst.
- Native preference parsing respects integer type, width and valid boolean
  range. The retired shadow preference no longer stores an unused value; its
  wire key stays reserved. All 41 existing key numbers and the UUID are retained.
- Warning ink passes at least **3:1 across all 64 native backgrounds**. The
  difficult #AAAA55 case renders black at **8.59:1** in QEMU. Invisible image
  and monochrome notification icons use the generic icon/app-name fallback.
- **74 native captures and 48 focused checks pass.** A real SDK system
  notification covers the face during the title test: the current page is
  preserved, receives a fresh reading dwell on return, then continues. This is
  focus-aware title paging, not automatic deduplication of PebbleOS alerts.
- **20 native state/format/appearance presets** are embedded in Clay. Five
  additional native Screenshot-service captures from the exact final PBW match
  the reviewed presets pixel for pixel. Candidate/final binary and resources
  are byte-identical; mapped scanout was checked against the Screenshot service.
- Actual Clay passes **68 layouts, 69 main contrast scenarios and 5,385
  text/control checks** across all 17 languages, 320/390px and light/dark.
  **34 additional Quiet Time preview cases add 2,380 checks**; picker coverage
  adds **128 checks**. Minimum measured text contrast remains **5.82:1**.
  No page errors or external requests occur.
- Five targeted browser checks cover closed-picker Tab/Escape/ARIA, independent
  day/night preview RGBA identity without saved/private changes, exclusion of
  preview-only controls from Save, explicit additional-alert copy, and actual
  phone bootstrap resync containing only validated watch presentation values.
  Startup/new marked baselines restore unchanged saved appearance; pending-save
  precedence and stale-ACK handling have portable regression coverage.
- The **13-entry PBW installs successfully** and contains seven complete
  notice/license documents for Dynamic, Core, Clay and bundled helpers, and
  both fonts. Deterministic packaging preserves SDK payload bytes except the
  intended permission-manifest transform. Repeated fixture packaging is byte
  identical and corrupted notice inputs are rejected.
- The exact SDK/source/core/font/license audit passes, including a new check
  binding native preview metadata to the actual packaged binary/resources.
  A mismatched renderer/preview pair is rejected. Resources remain **246,928
  bytes**; native footprint is **65,398 bytes**, leaving **65,674 heap bytes
  before startup**. The older runtime heap and migration results are reused
  below with their original scope.

Evidence is saved in [native QA](qa/0.6.1/native.json),
[settings QA](qa/0.6.1/settings.json),
[targeted settings checks](qa/0.6.1/settings-refinements.json),
[night preview checks](qa/0.6.1/night-preview.json), and
[the exact bundle audit](qa/0.6.1/bundle-audit.json).
The [publication kit](../store/README.md) identifies the final five raw native
screenshots, product copy, privacy/support documents and the explicit store hold.

Final PBW SHA-256:

`1328f66c3dff4f80d8590b7adb2d2f4b0c509a196201bb8ffebf396ce1a0123f`

Core 0.4.1 source commit: `6a6fb32e5a52d146e60d32fd6ee1b92e67722ada`.
Core archive SHA-256:
`9bc0b432a40293a5551dc78991fd6817c523683a8f7a5d02f9bb96dfcfd6209c`.

Physical PT2 vibration, power use, actual OS Quiet Time transitions and live
phone/provider delivery remain unverified. The phone browser uses a fixture
bridge; QEMU pulse requests do not measure a physical motor. Native system
notification identity/alert correlation is unavailable to the current face,
so additional banners/vibration may accompany PebbleOS alerts. No automatic
cross-system deduplication is claimed.

## Historical 0.6.0 notification attention and readable settings

Dynamic 0.6.0 pins Core 0.4.0. The protocol stays v4; seven appended AppMessage
keys add preferences and an ephemeral context sidecar while all previous 34 key
numbers, the UUID and permission contract remain unchanged.

- All **68 Dynamic JavaScript tests and seven strict native C suites pass**.
  Core passes **122 JavaScript tests and three strict native C suites**. All
  three Core C suites also pass AddressSanitizer and UndefinedBehaviorSanitizer;
  LeakSanitizer is disabled because this environment blocks its process inspection.
- Summary/app-only notifications receive a three-second budget; actual message
  text receives up to eight seconds. A burst retains its initial deadline.
  App-only packets contain no sender/body. Pause state lasts only for the
  existing five-second grace. Tests cover baseline history, app filtering,
  expiry, redaction, atomic queued metadata and reconnect/restart boundaries.
- Explicit phone bootstrap rotates the native context session and requires a
  fresh notification baseline. Old event metadata and late request ACKs cannot
  open the new notification gate. Burst IDs are consumed even when vibration
  is Off or Quiet Time is active; those events cannot become delayed alerts.
- **40 native captures and 19 focused checks pass**, including missing/transparent
  artwork, late art/artist updates, pause expiry, one-pass whole-word title
  continuation, three/eight-second notification expiry and app-only rendering.
  QEMU reports exactly one Short or Double pattern per fresh burst; retries,
  foreign sessions, consumed disabled events and a fresh bootstrap remain silent.
  **20 native preset captures** cover five states × 12/24-hour time × light/dark.
  They are embedded in the actual Clay page and all 20 canvases match exactly.
  Five further captures from the final PBW match the reviewed presets pixel for
  pixel. The candidate and final native binary/resources are byte-identical.
- **68 actual Clay layouts, 69 contrast scenarios and 5,382 text/control checks pass** across
  all 17 languages, 320/390px widths and light/dark appearances. The minimum
  text contrast is **5.82:1**; toggle thumb/outline contrast is at least 3.927:1.
  Another 128 text checks pass with the colour picker open.
  No page errors or external requests occur. Tests cover visible select values,
  inputs, descriptions, technical details, low-contrast watch-colour warnings,
  valid/invalid colour-code import, privacy isolation, app-only mode, vibration
  visibility/default, Quiet Time fields and the real Clay Save payload.
- Fixed 24px titles continue through the remaining received text once and hold
  the final page. The artist stays 20px; the artwork slot stays 40px and footer
  bottom stays at y216. Missing/transparent artwork uses the same fallback.
  Artist line-count corrections may change title spacing slightly; they do not
  restart pagination. Native focused evidence is recorded in asset-provenance.json.
- The final SDK/package/source audit passes, including all 41 message keys,
  source-map identity, pinned core archive, fonts and generated clock geometry.
  Resources remain **246,928 bytes**. Native footprint is **65,314 bytes**, with
  **65,758 bytes** available for heap before startup. The previous successful
  heap check remains historical below; it has not been repeated or relabelled
  as a new runtime allocation measurement.

Final PBW SHA-256:

`26a62a9c6209ebb65fd5f129685204d267d19d6dc0490c6837a77598c3b271bf`

Core source commit: `6894a97347b1e1101650f4244038296ab475a24e`.
Core archive SHA-256:
`702529c71fafe790a6fb6cb03a26a5aadee9569f77d35e17998c1efbd4a3a46c`.

The SDK emulator exposes no Quiet Time setting. Actual OS palette transitions
and Quiet Time suppression are therefore unverified; native decision tests
cover the active/inactive input and the production code queries the SDK API.

Physical PT2 vibration strength/timing, power use, live phone/provider delivery
and native Timeline event text remain untested. QEMU pulse requests do not
measure a physical motor. The notification source does not report whether the
system already vibrated, so Dynamic does not claim to prevent duplicate native
system alerts. Earlier heap, migration and rendering checks below retain their
original release scope.

## Historical 0.5.0 meaning, calmer updates and settings clarity

The full UX pass keeps Dynamic's unified typography, chunky clock, two reading
homes and fixed music footer. It adopts Core 0.3.0 for precise reading identity,
explicit expiry and separate notification app/sender fields. It also revises
attention/motion rules, colour roles and the relationships between settings
preview, privacy choices, drafts and delivery.

- **65 Dynamic JavaScript tests and six strict C99 suites pass.** The complete
  release's final native/browser/package identity results are recorded below.
- **Core 0.3.0 passes 116 JavaScript tests and three native suites**, with
  ASan/UBSan checks passing. Strict legacy v1/v2/v3 support remains alongside
  v4 detail, forecast and observation fields. V4 has 56-byte record headers
  and a 5,206-byte maximum packet. Non-ready public records retain identity
  only, EXPIRED=7 is explicit, and private source values/labels do not persist.
  Core source commit: `e5d99b5033c6aa461023ec0674a1802deb5c0a04`.
  Pinned archive SHA-256:
  `c352cd062c062b3888941194aad571c3a9046ffcc271485267cc2f711443e7a1`.
- Reading labels distinguish Temperature, Feels like, High, Low, Phone and
  forecast temperature. Failure/expiry retains configured public identity.
  A clock mark means a known observation at least an hour old, not native
  cache origin. Unknown observation age is not represented as known freshness.
- Nominal fonts and the bottom anchor stay fixed. Idle values/labels move
  4px/6px closer to symbols. Quieter artist/label ink has a 4.5:1 guard with
  foreground fallback; accent graphics have a 2:1 guard. Normal symbols use
  accent/source treatment and actual low-battery/connection warnings use amber.
- New primary title text defines a track change because the source supplies
  no track ID. Late cover/artist/icon updates repaint without a content slide.
  A notice burst enters once; replacements coalesce for a 900ms stable interval
  within its eight-second maximum. Privacy/body removal and invalidation clear
  immediately. Sender and app identity are transported/rendered separately.
- Settings use Appearance, Content and Connection & help with five independent
  sample states. Exploring an example does not enable its feature or change
  privacy. Follow Pebble examples use the last reported OS style; an unknown
  style receives an explicit illustrative-format label. CLOCK_FORMAT=255 only
  queries OS style, and all 34 existing message-key bindings remain intact.
- Dirty state and last save/delivery appear near Save. Automatic-retry wording
  matches actual reconnection behavior. Custom palette exploration is reversible;
  app filters are removable entries with no invented installed-app validation.
  Shared Pebble readings and nearby recovery help are explicit. New strings
  cover all 17 supported settings languages.
- All 20 state/clock/theme presets, seven edge sheets and six motion sheets
  were visually reviewed. Precise details, public failure/expiry identity,
  long international text, missing artwork and custom colours fit without
  concrete clipping, collisions or footer drift.
- Twelve upgrade captures and two exact pixel comparisons passed from the
  released 0.4.1 PBW, covering legacy v3 cache, retained 12/24-hour preference
  and saved palette. Historical seed captures are separate from new-render
  validation.

- **71 native stills, nine pixel/clock-bridge checks and six motion paths pass.**
  All 26 late-artwork frames preserve every pixel outside the cover rectangle;
  all 26 notice-update frames keep the header pixel-identical. Clock-format
  queries report actual 12/24-hour system style while the forced clock image
  remains unchanged. The music footer, notification expiry and private-context
  relaunch checks pass. The source supplies no stable track ID: a changed
  primary title remains the boundary for a track transition.
- **20 actual Clay native-preview identities and 68 phone layouts pass:**
  seventeen languages × 320/390px × light/dark. Independent examples do not
  change privacy; known 12-hour and unknown-format examples are honest.
  Dirty/revert/reconnect copy, shared-source identity/recovery/ages, reversible
  custom palettes, app-chip add/remove/deduplication and pending-entry real
  Clay submission all pass with no page errors or external requests.
- The native renderer was captured from candidate PBW
  `ebff5110f0dc3983313d27ff5dbf2fe26fcc6936bd6e4395b6c85106a767cdd6`.
  The final PBW's native binary and resource pack are byte-identical; only
  phone assets change when those captures are embedded. A final install adds
  five native smoke captures and five exact RGB comparisons covering Idle,
  Music, Banner, Message and Timeline Peek. They all match the reviewed samples.
- The exact final SDK/package/source audit passes: 34 unchanged wire keys,
  UUID, permissions, committed Core 0.3 pin, packaged font/glyph metrics,
  generated native outlines and source-map identity. Resources are 246,928 bytes;
  static RAM footprint 63,974 bytes, available heap 67,098 bytes before startup
  allocation. A native run reports 67,096 heap bytes total and 30,828 bytes used;
  shutdown reports zero still allocated. These observations are not a claim
  about physical power draw or worst-case native heap usage.

PBW SHA-256:

`fb2841fdf0a18072e269e79e4381ee962b5a9d94ae2ec3d985e60cb117e827a7`

Native binary SHA-256:

`44b7d16acfeea0eda26cf7b41cb0296779291204b322c38f0772cab0fb9ed6ce`


Physical PT2 behavior, live plugin/media and notification delivery, actual
Timeline event text and hardware animation FPS/power remain untested. Native
emulator fixtures and actual Clay browser checks do not establish those device
results. Earlier release checks below remain historical and are not relabelled
as coverage of 0.5.0.

## Historical 0.4.1 unified typography and fixed music sizes

Dynamic 0.4.1 retires the Larger text mode for new and existing installations.
Its persisted choice is no longer read or written, live size inputs have no
rendering effect, and the reserved LARGE_TEXT wire-key number keeps later
bindings intact. Core 0.2.0 and source-label behavior remain unchanged.

The unified idle layout uses an 80px clock, 20px date, 36px values and 14px
labels. Context uses a 42px clock, 22px values and 14px symbols; Timeline Peek
uses 26px values and 16px symbols. Song titles stay at 24px and artist text at
20px across tracks. Native metric inspection showed that 26px cannot reliably
fit three title lines above the footer; 24px preserves that line budget without
per-track font changes. Titles wrap through up to three lines and artists
through up to two, then use an ellipsis. The 40px artwork/footer remains
anchored at y216. Heading-only banners can retain two title lines with a gap
above the same fixed footer.

- **61 JavaScript tests and six strict C99 suites pass.** Updated native tests
  require one pose hierarchy, clear icon/value separation, fixed music font
  sizes, stable footer geometry and bounded UTF-8 wrapping. Settings tests
  prove both named and numeric obsolete size inputs are inert, including old
  queued saves, while current preferences still deliver normally.
- **58 native stills, eight exact pixel comparisons and four motion paths**
  cover the unified renderer, light/dark and 12/24-hour states, long Latin,
  Czech, Cyrillic and Greek text, missing/rectangular artwork, typed readings,
  summary/body notices, Timeline Peek and expiry. All 20 preset images and
  five edge sheets were visually reviewed, alongside track/layout motion.
  Notification summaries retain the exact footer and expiry restores the
  preceding music image.
- **28 migration captures and 23 pixel checks** exercise upgrades from the real
  released 0.4.0 PBW with each old size preference. Four historical seed images
  demonstrate that the old modes really differed; both upgrades then produce
  identical unified idle/music/Timeline images. Live obsolete size packets
  and relaunch cannot restore either retired mode. The 23 checks comprise 21
  equalities and two old-mode inequalities.
- **20 actual Clay preview/canvas identities and 10 saved-size preview checks**
  pass, together with five clock migrations, conditional notification/app
  controls and the real Clay save round trip without LARGE_TEXT. English/light
  and Czech/dark at 390×844 captures have no overflow and keep Save visible.
  There are no page errors or external requests. Existing 17-language
  translation checks remain part of the complete JavaScript suite.
- The native images were captured from candidate PBW
  `9f8265d3167ee7fd791bddac40e6a02241c47326e2dacfeb48fdcc0e92bc02e4`.
  Bundling those images changes only phone assets: the final native executable
  and resource pack are byte-identical to the captured renderer. Eight further
  final-package captures check low battery/wide dates and sample identity;
  five current sample states match their preview captures pixel for pixel.
  One status capture records the immediate return from music, not a settled
  idle-layout claim.
- The final SDK/source/package audit passes all 34 wire-key bindings, the UUID,
  permission manifest, pinned Core 0.2.0 and font tables. The SDK reports 246,928
  resource bytes, a 60,858-byte RAM footprint and 70,214 bytes free heap before
  startup allocation. The native executable is 62,248 bytes.
- Core 0.2.0 is unchanged. Its saved 103 JavaScript tests and three native suites
  remain applicable. Previous Dynamic releases and the first 26px prototype
  are historical; they are not counted as current unified-layout coverage.

PBW SHA-256:

`12da079f1040cbef88460f2326f88b2690e6b651342b6af134205eed25082625`

Native binary SHA-256:

`187bf03a7c27ade41c0d1e55a58e2cc134bd6ee243c160a9f57a003dafb6e596`

Physical PT2 behavior, live plugin/media and notification delivery, actual
Timeline event text and hardware animation FPS/power require device checks.
Emulator fixture delivery and browser settings checks are reported separately.

## Historical 0.4.0 settings, richer readings and proportional notifications

Dynamic 0.4.0 adopts the pinned Core 0.2.0 package while retaining its UUID,
existing message-key numbers, two horizontal reading slots and fixed music
footer. CLOCK_FORMAT is appended after the existing keys. Settings add native
sample previews, per-source health, conditional notification controls and
acknowledged delivery with reconnect recovery.

- **59 JavaScript tests and six strict C99 suites pass.** The saved Dynamic
  results cover settings acknowledgement ordering and retries, clock-format
  choices, source health and privacy, all 17 settings languages, native preview
  integrity, notification presentation, reading semantics, text/raster assets
  and the music/watch layouts.
- **Core 0.2.0 passes 103 JavaScript tests and three native suites.** Coverage
  includes legacy and v3 packet decoding, UTF-8 metadata limits, typed reading
  metadata, larger artwork, cache/freshness rules, transient private context,
  reconnect baselines and replacement of obsolete waiting source snapshots.
  The committed core source is
  `e5510d4998973d5bd339eb9c771594313f6764a3`; the pinned archive SHA-256 is
  `1525575a2e8e17201fb3625c48a9752553f54ab34ac7b72a7ab455cb36949c3e`.
- **68 settings interaction cases pass**, including all 17 languages. The
  final actual Clay checks verify all **40 native sample presets** against
  their canvas RGBA output, five clock-preference migration cases, conditional
  controls, comma-only filter rejection and the save round trip. English,
  Czech and German final pages have no page errors or external requests.
  English/light and Czech/dark at 390×844 have no overflow and retain a visible
  fixed Save control. These checks use a fixture source bridge, not a physical
  phone.
- Upgrades from the released 0.3.2 PBW preserve both legacy explicit clock
  choices: legacy 0 migrates to 24-hour, and legacy 1 to 12-hour. Each migrated
  render matches the corresponding explicit choice. Selecting Follow Pebble
  overrides the legacy preference and remains effective after relaunch.
- The saved pre-label-fix matrix covers **35 rendering cases, 68 native stills
  and 11 motion paths**, plus two animation opt-out runs. It includes normal
  and Big text, long/Cyrillic/Greek music, missing artwork, sparse/stale and
  typed readings, light/dark pages, notifications and native Timeline Peek.
  Lifecycle checks retain the exact music footer beneath a summary and
  restore the exact prior music image after notification expiry. These saved
  results are reused for unaffected behavior; they are not described as a
  rerun against the final binary.
- That matrix used PBW
  `3cb33a351e2471367b8dbda6f90cf84bb55146d7c447f640997be67ddcc71f4e`.
  Its visual approval was superseded specifically because neutral NUMBER
  providers labelled `Progress` and `Charge` displayed the fallback `VALUE`.
  Final provider-label checks against the rebuilt PBW are recorded below.

- **34 final-PBW native captures and 16 exact RGB comparisons pass.** Six
  typed/untyped neutral-label comparisons prove `Progress` and `Charge` remain
  distinct in idle, music and Timeline Peek at both text sizes. Czech and
  wide Latin labels end cleanly at their measured ellipsis; range and timestamp
  readings preserve `Today` and `Sunset`. All five focused contact sheets were
  visually reviewed. Ten unaffected state/size captures match the saved matrix
  pixel for pixel, including music, summary/body notices and Timeline Peek.
- All six native suites were rerun after the provider-label fix. The final
  package audit passes SDK/source/native identity, all 34 message keys, UUID,
  permission manifest, pinned core and the original font/resource tables.
  Settings and core JavaScript are unchanged, so their saved passing results
  are reused. Native sample previews retain their original capture hashes;
  the later label fix leaves their known-semantic example content unchanged.
- Reinspection of the saved dense motion sheets found partially drawn headers
  in early QEMU framebuffer captures of two notice transitions. Their settled
  native screenshots are correct. These asynchronous captures do not establish
  whether any corresponding flash occurs on hardware; no hardware motion or
  frame-rate claim is made.

- SDK 4.33.1 reports **246,928 resource bytes**, a **61,798-byte static RAM
  footprint** and **69,274 bytes available heap before the startup allocation**.
  The existing runtime heap check succeeded with approximately 30.7 kB used.
  Larger transient buffers use a single startup allocation; the label fix
  does not add per-frame heap allocation.
- Native fixtures use the production pinned core encoder and native
  AppMessage. Phone JavaScript is tested separately and is not executed by
  these emulator fixture runs. The SDK firmware locale is English; Greek and
  Cyrillic provider text comes from literal fixtures. Live plugin/media and
  notification callbacks, actual Timeline event text, physical PT2 behavior
  and hardware animation FPS/power remain untested.

Rebuilt PBW SHA-256:

`b83feab5e9ceef828632a95dd9ac25d45e29a6b3bdb392a8e309fc3a5b581946`

## Historical 0.3.2 bottom-anchored music and compact date

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
