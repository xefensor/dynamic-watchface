# Dynamic maintainer notes

Dynamic targets Pebble Time 2 (`emery`, 200×228). Preserve its UUID and all
existing AppMessage key numbers. Append new keys; never reuse retired ones.

## Product intent

- Activity cards have normal and compact host presentations for any provider.
  Normal uses a shallow 24px image/16px identity header, then full-width content
  below it; never reserve an empty icon column beside the main content. Compact
  uses a separate band with 20px image, 14px identity and 16px current content.
  Artwork may fill either image position. The internal ACT2 caption carries app
  identity; exact identity labels suppress field prefixes while meaningful
  compatibility labels such as Rest or ETA remain visible.
- Main clocks use centered Fredoka numerals; body titles stay left-aligned at
  24px over the full 180px and continue once without looping. The primary's own
  essential detail determines its layout; optional detail and its independent
  provenance yield together if they cannot fit beneath a notification banner.
  If an extreme compatibility card's own essentials still cannot fit at the
  readable size, the whole card yields until the brief banner clears.
  an optional secondary never changes its size or position. Hide the whole
  secondary when its identity, complete clock/value units or provenance cannot
  fit. Standalone compatibility music retains its 24px title, 20px artist,
  40px artwork and footer bottom at y216.
- ACT2 main and supporting items may each be text, a formatted value or an
  independent clock. Any activity can occupy either activity role. Keep
  supporting content, secondary activities, artwork and meters conditional on
  space; never infer a meter range or truncate essential clock/value units.
- Preserve per-item observation/expiry and source revision separately from the
  view sequence. Retained public content needs visible uncertainty and original
  observation age; unavailable private content clears immediately. Overall
  activity pause must not freeze a separately running clock. Zero is not a
  lifecycle transition or alarm acknowledgement.
- Share an age/uncertainty indication only when the items' observation and
  provenance match; preserve independent estimates and pause state. Completed
  clocks are final values, not paused clocks. The phone ranks all ACT2 cards,
  including the compatibility Music bridge; native trusts that wire order.
  Retained cards use explicit Last reported wording and their original age.
  A short normal-card status may share its identity header when both full
  strings fit; compact provenance retains measured room beside app identity.
- Two shared Pebble readings retain their left/right positions. Timeline Peek
  uses the native OS panel rather than a second event renderer.
- Notification banners and additional vibration default to Off. They do not
  replace or automatically deduplicate PebbleOS notifications.
- Music and notification contents stay transient. Source redaction, privacy,
  disconnection and expiry take priority over animations and queued updates.
- Phone settings colours are independent of watch colours. Preview controls
  must not silently change saved appearance or notification privacy.

## Source boundaries

- `src/c/`: Dynamic's renderer, SDK lifecycle and host presentation.
- `src/pkjs/`: offline Clay settings, translations and phone host integration.
- `vendor/CORE.json` and `vendor/ACTIVITIES.json`: exact pinned
  library archive/commit/checksum.
  Change the upstream core repository first, then rebuild and pin its tarball.
- `scripts/release_licenses.py`: exact notices carried in each standalone PBW.
- `.github/release.json`, `.github/workflows/release.yml` and
  `scripts/github-release.py`: independent GitHub release configuration,
  validation and publication. Shared helper changes should remain aligned with
  Halftone, Core and ČHMÚ; project configuration and release identity stay separate.
- `docs/VALIDATION.md`, `docs/asset-provenance.json`, `docs/qa/`: release-specific
  evidence. Preserve older evidence as historical; do not relabel it as new.
- Native activity decoding uses `pebble-activity-view.h` and `Pav*` only. Keep
  legacy provider adaptation on the phone; do not link a second ACT1 decoder
  into the limited native image. Reuse the bounded activity plan for drawing
  and clock scheduling; do not copy complete cards per frame.

## Checks and release order

1. `npm ci --ignore-scripts` and `npm test` run portable JS/C checks. Release
   automation has separate tests:
   `python3 -m unittest discover -s tests -p test_github_release.py`.
2. `pebble build` uses the pinned font sources and generated metrics. Inspect
   native load-image size after native edits and keep it below the
   SDK's 65,535-byte limit. Heap availability is a separate measurement.
3. `python3 scripts/package-release.py` packages exact SDK payloads and notices.
4. `python3 scripts/check-native-style.py --pbw dist/Dynamic-VERSION.pbw`
   checks bundle/source/font/core/license identity. Its Python font dependencies
   are pinned in `scripts/requirements-font-assets.txt`.
5. Capture affected native states and actual Clay pages. Reuse unchanged prior
   tests; repeat only what changes or an unresolved concern requires. Never
   treat emulator/fixture results as live-phone or physical-watch verification.
6. Refresh native preview assets, rebuild, and verify the final binary/resources
   match the captured renderer. Save final image/provenance evidence.
7. For a new, unpublished version, run `python3 scripts/prepare-store.py` and
   its `--check` mode after final provenance, then
   `python3 scripts/package-source.py` last. Any source/docs edit to that
   unpublished snapshot requires regenerating its archive/checksums.
8. Commit the bumped version, checked artifacts/checksums and notes locally,
   then run
   `python3 scripts/github-release.py prepare --output /tmp/dynamic-release`.
   It validates committed `HEAD`, not uncommitted working files. Push that commit
   to `main` after the check. The workflow validates the pushed commit and
   publishes exact artifacts without an SDK rebuild. If a GitHub connector
   creates/publishes the commit directly, rely on its CI check or fetch/check out
   that commit before a local `prepare`; an old-HEAD run does not validate it.
   Use the manual Run workflow control for an identical-version retry.

## Published release immutability

- Keep published assets and tags unchanged. Later README/docs/AGENTS or
  release-automation improvements on `main` do not require rewriting an older
  source archive or republishing its version.
- The bootstrap in `.github/release.json` ties existing v0.6.1 artifacts to their
  original tested commit. Do not repoint it to a documentation commit. A later
  version uses its own artifact commit; refresh the finite filenames/checksums.
- Published-byte changes need a new version. The workflow refuses changed or
  incomplete published assets, conflicting tags and RC/beta promotion to stable.
- Source ZIPs exclude generated `build/`, dependency, Git and `.pebble/` folders,
  generated `docs/qa/**/*.png` copies, and all PBW binaries. QA PNGs remain
  versioned in Git; buildable embedded previews and product screenshots stay in
  the source ZIP. Committed source recipes are allowed; archive path
  and runtime/build-input checks still apply.
- This repository pins Core 0.5.0 and Activities 0.2.2 explicitly. Upstream
  releases do not automatically repin or rebuild the watchface.
- Version 0.9.0 is a published GitHub development prerelease. Keep its tested
  artifacts, tag and the earlier 0.8.0 download links/bytes available.
  Prior user success on 0.8.0 is useful history, not a physical 0.9.0 check.
- Version 0.9.1 is a GitHub development prerelease. Preserve published 0.9.0
  artifacts and their original source snapshot while validating the new bytes.

- Version 0.10.0 adds identified primary/secondary cards and Dynamic-local
  provider preferences. Preserve 0.9.1 and earlier published artifacts/tags.
  Phone preferences never become new native message keys or private-content cache.

`store/listing.json` is a local publication record, not a Pebble CLI schema.
GitHub builds remain prereleases. The current official
[plugin-preview guide](https://developer.repebble.com/guides/plugins/preview/)
holds appstore submission. Recheck
the linked official rule before changing that status; preparing a consistent
store kit does not publish the app or remove the restriction.
