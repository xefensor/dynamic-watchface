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
- `docs/VALIDATION.md`, `docs/asset-provenance.json`, `docs/qa/`: release-specific
  evidence. Preserve older evidence as historical; do not relabel it as new.

## Checks and release order

1. `npm ci --ignore-scripts` and `npm test` run portable JS/C checks.
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
7. Run `python3 scripts/prepare-store.py` and its `--check` mode after final
   provenance, then `python3 scripts/package-source.py` last. Any source/docs
   edit after source packaging requires regenerating that archive/checksums.

`store/listing.json` is a local publication record, not a Pebble CLI schema.
The current official plugin-preview guide holds appstore submission. Recheck
the linked official rule before changing that status; preparing a consistent
store kit does not publish the app or remove the restriction.
