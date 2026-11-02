# GitHub releases

The current [v0.7.0 prerelease](https://github.com/xefensor/dynamic-watchface/releases/tag/v0.7.0)
is live with the checked PBW, source ZIP and flat `SHA256SUMS.txt`.
[All versions](https://github.com/xefensor/dynamic-watchface/releases) remain
available on the repository's Releases page.

GitHub distribution is separate from Pebble appstore submission. Dynamic is
still a plugin-preview build; the appstore hold and remaining physical-device
checks are recorded in [validation](VALIDATION.md) and the
[publication kit](../store/README.md).

## What is published

| Asset | Purpose |
| --- | --- |
| `Dynamic-VERSION.pbw` | The already-built, checked watchface |
| `Dynamic-VERSION-source.zip` | Its prepared source snapshot and pinned dependency archives |
| `SHA256SUMS.txt` | SHA-256 checksums using download basenames |

The `.github/release.json` configuration selects exact current filenames.
The workflow never uploads a broad `dist/*` glob or rebuilds a PBW with another
SDK. Release notes come from the matching version section in `CHANGELOG.md`.

The initial v0.6.1 tag points to the tested artifact commit
`cae7b28ca29bb1ae8e6e65fbc5e38ec8d6bd2018`. The bootstrap entry preserves that
identity. Automation and later documentation live on `main`; they do not change
the already-published source ZIP, PBW, checksums or tag. Future source snapshots
include the automation files when the next version is prepared.

## Prepare the next version

1. Bump `package.json`, its lockfile metadata and the strict release auditor's
   version expectations. Update `CHANGELOG.md` and any affected product docs.
   A library upgrade deliberately changes its archive, lockfile and
   `vendor/CORE.json` or `vendor/ACTIVITIES.json`; publishing a library does not
   upgrade Dynamic.
2. Run the portable checks, build with the intended SDK, then package and audit
   the PBW. Follow [AGENTS.md](../AGENTS.md) for the full order, native size
   constraint, license packaging, preview capture and store-kit provenance.
3. Finish source/docs for this unpublished snapshot, prepare its source archive
   and update `dist/SHA256SUMS.txt`. Source ZIPs exclude generated `build/`,
   dependency, Git and `.pebble/` folders, generated QA PNG copies, and all PBW
   binaries. QA images remain in the repository; embedded preview data and
   product screenshots stay in the archive. Runtime
   and build inputs must match the selected release commit.
4. Commit the version, checked artifacts, checksums and notes locally. Then
   check the committed release inputs without publishing:

   ```sh
   python3 scripts/github-release.py prepare --output /tmp/dynamic-release
   ```

   `prepare` reads committed `HEAD`, not uncommitted working files. When using
   a GitHub connector to create/publish the commit directly, CI checks that
   pushed commit; fetch/check it out first if also running `prepare` locally.
5. Push the checked commit to `main`. The **GitHub release** workflow runs on
   matching package/artifact/config or automation changes. Ordinary
   README/docs-only changes do not create a new release.

Keep the bootstrap entry for the initial version unchanged. A later version
uses its own artifact commit. Keep `prerelease: true` while Dynamic is distributed
as a preview; changing store eligibility is an explicit product/platform review.
RC/beta identifiers are also required to stay prereleases.

## Checks and permissions

The workflow first runs `npm ci --ignore-scripts`, `npm test`, the release
helper's Python tests, and artifact preparation with read-only repository
access. The separate publishing job receives only the repository-content write
permission needed for tags/releases. Official checkout/setup actions are pinned
to reviewed commits. It uses GitHub's built-in token; no personal access token
or external scheduler is required.

The helper checks the selected version, package identity, archive integrity,
source/runtime inputs, PBW JavaScript/source-map pairing and checksums. It does
not replace the SDK build, native visual review or live device/provider checks.
The compiled packages are the committed artifacts reviewed before publication.

Local release-helper regressions run with:

```sh
python3 -m unittest discover -s tests -p test_github_release.py
```

## Retries and frozen releases

Use **Actions → GitHub release → Run workflow** on `main` to verify or retry the
current packaged version. Publishing creates a draft, uploads the exact assets,
verifies their server-side hashes, then makes the release public. A compatible
interrupted draft can resume. Per-version publication jobs are serialized.

An identical published release is verified and left unchanged. Conflicting tag
targets, changed/missing published assets or the wrong prerelease flag cause
failure; the workflow does not move tags or replace published files. Use a new
version for changed package bytes. Documentation-only updates belong on `main`
and do not require repacking an existing public version.
