# Dynamic maintainer notes

Dynamic targets Pebble Time 2 (`emery`, 200×228). Preserve its UUID and all
existing AppMessage key numbers. Append new keys; never reuse retired ones.

## Product intent

- ACT3 activities use the agreed notification-style composition: primary content
  above contextual footer, secondary above primary in exactly one static row.
  The footer ends at y216. Media has title plus optional 40px cover/artist footer;
  other activities use instance context, falling back to provider identity.
- Keep the complete primary block bottom-aligned directly above its footer,
  with an 8px gap when a footer is present. Move standard value labels, headline,
  detail and range indicator together. Media title ink ends above the cover/artist
  footer. Spare space stays above the block; the secondary keeps its own position.
- Any source can occupy either role. The primary expands when secondary is
  absent. Compact reduction keeps its selected clock/value structured and full;
  an essential value that cannot fit causes the optional secondary to yield.
  Both datums and media artwork remain available for local promotion.
- Text always starts at its beginning and truncates at the last visible line.
  Do not scroll, marquee, automatically page or cycle text. Do not restore old
  title continuation timers, identity headers or Last reported UI.
- Preserve immutable source/run identity, independent clocks and original
  observation/validity. Timer's known native adapter may display a bounded public
  deadline prediction without renewing its observation. Other unconfirmed legacy
  sources are omitted when their required provenance cannot be displayed.
- Media presentation, identity symbols, compact selector, semantic directions and
  media-item visual token are explicit ACT3 fields. Do not infer styling from a
  private label marker. Legacy source adaptation belongs on the phone.
- All media uses the shared activity renderer. Keep one bounded render plan and
  no parallel native Music renderer. Incoming-only 280ms transitions follow run
  replacement or media item changes; ticks, lease refresh and artist/art arrivals
  do not start them. Activity clock ticks dirty only the activity layer.
- Two shared Pebble readings keep their positions. Timeline Peek uses the OS
  panel. Notification banners retain their existing baseline/privacy policy and
  default to Off. They do not replace OS notifications.
- Activity presentation status confirms actual visible original record indices,
  native session and view/presentation sequences. A small 30-second status refresh
  maintains the routing lease; clock ticks never write storage or send messages.
- The bundled read-only navigation source exposes one recent route record for
  Open Activity. Persist only two destination app UUIDs and session/expiry data,
  never provider/run IDs, titles, notification text, artwork or content hashes.
  Validity is capped at 45 seconds and source/role expiry; reads never renew it.
- Dynamic requests summary display fields and `open_app`, excluding separate
  `actions` and `entity_ref` descriptors. Interactive hosts may request those
  capabilities later; this watchface never invokes provider actions.
- Open Activity means Dynamic's recent Primary/Secondary from any watchface;
  stock SDK cannot prove the immediately preceding face. No firmware/mobile-app
  fork, custom mobile build, Dashboard implementation or global Quick Launch
  remapping is required. Automatic app opening uses the separate helper repo.
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
- This repository pins Core 0.5.0 and Activities 0.4.0 explicitly. Upstream
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

Version 0.12.0 adds the independently callable navigation plugin. Distribution
packaging must include plugin.js, its manifest source declaration and exact
source identity, alongside the unchanged SDK native/JS payloads and licences.
Append ACTIVITY_PRESENTED/REQUEST_PRESENTATION after all existing message keys.
Native screenshots prove rendering only; real-phone app switching, permissions,
physical-watch latency and battery remain separate checks.
