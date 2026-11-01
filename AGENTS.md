# Dynamic maintainer notes

Dynamic targets Pebble Time 2 (`emery`, 200×228). Preserve its UUID and all
existing AppMessage key numbers. Append new keys; never reuse retired ones.

## Product intent

- One typography system: 24px music titles, 20px artists, a 40px artwork tile
  and a footer bottom at y216. Titles continue once and never loop or shrink.
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
- `vendor/CORE.json`: exact separately released Core archive/commit/checksum.
  Change the upstream core repository first, then rebuild and pin its tarball.
- `scripts/release_licenses.py`: exact notices carried in each standalone PBW.
- `.github/release.json`, `.github/workflows/release.yml` and
  `scripts/github-release.py`: independent GitHub release configuration,
  validation and publication. Shared helper changes should remain aligned with
  Halftone, Core and ČHMÚ; project configuration and release identity stay separate.
- `docs/VALIDATION.md`, `docs/asset-provenance.json`, `docs/qa/`: release-specific
  evidence. Preserve older evidence as historical; do not relabel it as new.

## Checks and release order

1. `npm ci --ignore-scripts` and `npm test` run portable JS/C checks. Release
   automation has separate tests:
   `python3 -m unittest discover -s tests -p test_github_release.py`.
2. `pebble build` uses the pinned font sources and generated metrics. Inspect
   native load-image size after native edits: the 0.6.1 image is close to the
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
  and contain no PBW binary. Committed source recipes are allowed; archive path
  and runtime/build-input checks still apply.
- This repository pins Core 0.4.1 explicitly. A new Core release does not
  automatically repin or rebuild either watchface.

`store/listing.json` is a local publication record, not a Pebble CLI schema.
GitHub v0.6.1 is published as a prerelease. The current official
[plugin-preview guide](https://developer.repebble.com/guides/plugins/preview/)
holds appstore submission. Recheck
the linked official rule before changing that status; preparing a consistent
store kit does not publish the app or remove the restriction.
