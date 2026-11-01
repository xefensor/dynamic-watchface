# GitHub releases

Versioned packages are published on [Dynamic's Releases page](https://github.com/xefensor/dynamic-watchface/releases). GitHub Actions publishes the exact committed, validated packages and checksums; it does not rebuild them in a different SDK environment.

The initial version is **v0.6.1**, tagged at the already-tested artifact commit `cae7b28ca29bb1ae8e6e65fbc5e38ec8d6bd2018`. Adding release automation does not change the frozen package/source archive or its checksums. The new automation files live on `main` and will be included in future source snapshots.

## Publishing the next version

1. Bump the package version and update release notes. Prepare and validate the versioned artifacts using the repository's existing packaging steps.
2. Refresh the checksum file, then commit the version, artifacts and notes to `main`. The **GitHub release** workflow checks the packages and portable tests before publishing.
3. Keep `.github/release.json` aligned with the exact filenames. Its bootstrap applies only to the initial version above; later versions use their own artifact commit.

Each release includes selected current assets and a flat `SHA256SUMS.txt` for downloaded files. Historical files are not uploaded by a broad glob. Published assets and existing tags are never overwritten. A rerun verifies an identical existing release; conflicting bytes, stale packages or a different existing tag target cause failure and require a new version.

The workflow can also be rerun from **Actions → GitHub release → Run workflow**. It uses GitHub's built-in token with repository-content write access only in the publication job. No personal access token or external scheduler is required.

Dynamic remains a **GitHub prerelease** while it depends on the plugin preview. This workflow does not submit it to the Pebble appstore or change the store hold.
