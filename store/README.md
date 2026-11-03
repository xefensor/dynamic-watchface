# Dynamic publication kit

**Status: 0.16.0 prepared locally; Pebble appstore submission is held.** Pebble's
[plugin-preview guide](https://developer.repebble.com/guides/plugins/preview/)
is the recorded reason for retaining the submission hold.
Earlier published releases retain their own checked PBW/source downloads. This kit does not submit or
upload anything to the Pebble appstore. Recheck that rule before a public submission.

`listing.json` is the local publication record, not an official Pebble CLI
schema. It contains the product identity, supported platform, URLs, plain-text
copy files and the explicit submission hold. The description and release notes
avoid claiming live-phone or hardware checks that have not been performed.

## Assets

The five `emery_*.png` files are raw **200×228** watch captures. They contain no
frame, labels or invented UI. Idle, Music, Title only, Message and a dark appearance
show the actual native renderer with synthetic fixture data. The dark capture
uses the same verified renderer as the final PBW. Native Timeline geometry is
covered in the validation evidence; its empty emulator event panel is not a
promotional screenshot. No separate watchface icon or banner is included.

The asset manifest records exact screenshot hashes, matching final PBW/native
payload hashes and the source images' provenance. The phone settings pictures
in `docs/images/` are supplementary documentation, not watch screenshots.

Refresh the kit after the final package and native provenance are saved:

```sh
python3 scripts/prepare-store.py
python3 scripts/prepare-store.py --check
```

The check rejects stale version/PBW hashes, altered or incorrectly sized
screenshots, missing copy/support/privacy documents, missing bundled licenses,
and disagreement with the final native provenance. It keeps the preview hold
explicit; a successful check means the local kit is consistent, not that Pebble
has allowed public submission.

## Before public submission

1. Confirm that Pebble permits plugin-based store releases.
2. Check the final build on a physical PT2 with the intended phone and providers,
   including permissions, live music/notifications, Quiet Time and vibration.
3. Use the final PBW and screenshots identified by `asset-manifest.json`.
   Review the [validation record](../docs/VALIDATION.md),
   [privacy statement](../PRIVACY.md), [support information](../SUPPORT.md) and
   [third-party notices](../THIRD-PARTY-NOTICES.md).

## Frozen release and current documentation

The 0.16.0 package and asset manifest identify this local candidate; earlier
published versions retain their own original source and artifacts.
Later documentation-only changes on `main` do not rewrite those published
packages, hashes or tags. Regenerate the kit/source snapshot while preparing a
new version, not to replace an existing public release. The separate
[GitHub release workflow](../docs/GITHUB-RELEASES.md) publishes verified packages
and does not remove the Pebble appstore hold.
