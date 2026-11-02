# Redesign recovery — 2 November 2026

## Incident

Xef reported that two ChatGPT Work conversations stopped with **“Unknown error”**
and could no longer be continued. The interrupted work was the approved Dynamic
grouped countdown and standalone Timer redesign. One run displayed an elapsed
time of 1 h 12 min 51 s. Its last visible tasks included release preparation,
validation documentation and an upload manifest.

The exact error has no diagnostic detail. No backend error logs, request ID or
confirmed cause are available. Do not attribute it to the watch code, the build,
conversation length or a service outage without further evidence. Progress text
such as “preparing release” is not evidence that files reached GitHub.

## Confirmed state before recovery

The repositories and releases were inspected directly:

| Project | GitHub state | Recovered local state |
| --- | --- | --- |
| Dynamic | 0.7.0, `2ce8c1cf1bb69f685b2610e3d481168d93f5404e` | Unpublished 0.8.0 commit `71d8190`, native captures and build/test records |
| Timer | 0.1.1, `23f8c369973aeade578ced5a07e6e4346e456995` | Uncommitted 0.2.0 multi-timer implementation, QA captures and smoke scripts |
| Pebble Activities | 0.1.2, `baac66dc88a6be6062dc8fe57156fdb96007b3c7` | Matches the published multi-slot library |

The first recovery check found only the older Dynamic/Timer versions on GitHub.
A subsequent workspace search recovered the unpublished files. Their absence
from GitHub therefore did **not** mean that the redesign source had been lost.
Earlier assistant reports of successful checks are historical reports until
their artifacts or fresh executions establish the result.

## Approved scope retained

- Dynamic uses the approved “Grouped value” countdown and the clock's rounded
  numerals. The clock, two readings and music footer retain their agreed layout.
- Timer has its own repository and independent list, duration editor, countdown
  controls and alarm interface, inspired by Pebble's Timer design.
- Up to eight native timer slots share state through Activities 0.1.2. Reusing
  a slot replaces that slot's previous phone snapshot.
- Native watch state, persistence and alarms remain authoritative. Nearby
  deadlines use an earliest-deadline Wakeup broker on stock PebbleOS.
- No firmware fork or built-in Workout integration is included.

## Recovery procedure and evidence

1. Preserve the recovered source and inspect each repository's current HEAD and
   uncommitted changes before modifying anything.
2. Review the native timer collection, migration, shared Wakeup broker and
   phone transport independently of the visual work.
3. Install the exact pinned dependencies and build clean distribution packages.
   A prior handoff warned that an incremental Timer package could retain the old
   single-timer phone bridge; validate the **packaged** PKJS/provider behavior,
   including numeric message keys and slot replacement.
4. Run the relevant portable, packaging and emulator checks. Preserve historical
   evidence with its original scope; distinguish new recovery checks from it.
5. Regenerate source archives after final source and documentation edits. Check
   the committed release inputs before publication and inspect GitHub CI and
   the resulting release assets afterwards.

Fresh results and final artifacts belong in
[Dynamic validation](VALIDATION.md) and
[Timer's documentation](https://github.com/xefensor/pebble-app-timer/tree/main/docs).
Neither emulator captures nor fixture handoffs certify physical vibration,
battery use or live phone/watch delivery.

## Continuation checkpoints

Keep coherent source and documentation checkpoints in Git, including evidence
paths and the exact remaining step. Save a remote checkpoint before a lengthy
validation or release phase when possible. A local commit is not a remote
backup, and a pushed commit is not a published release. Preserve existing tags
and release bytes; resume publication from the checked commit rather than
rewriting a previous release.
