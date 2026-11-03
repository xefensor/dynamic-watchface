# Dynamic UI and UX review — 0.15.0

The review covers normal watch use, activity replacement/promotion, paused
sources, long and missing metadata, both roles, notification privacy, Timeline
Peek, setup, settings input, ordering and recovery. The agreed activity contract
and source lifecycle are retained.

| Finding | Change |
| --- | --- |
| The compact row relied on whitespace alone for its boundary | A one-pixel divider uses the existing gap, without moving content/footer anchors |
| Paused primary state was English and a compact frozen clock looked active | Localized Paused is required before optional text in either role; compact content yields when value plus state cannot fit |
| Settings previews omitted Timer | A production-encoded Timer+Music composition joins the six preview tabs and 24 native presets |
| Colour sharing competed with ordinary appearance controls | The code and Copy/Apply actions are disclosed under Share colours |
| Preview tabs lacked Home/End and had a fixed five-tab wrap | Keyboard selection uses the actual six-tab count and retains preference independence |
| Fixed Save spacing and input-only focus handling could cover a select or enlarged text | Measured action height, visual viewport bounds and short-viewport document flow keep fields visible |

Root and the implementing agent reviewed 64 fresh native compositions and 24
native settings presets. Actual EN/CS Clay was checked at 320/360px, including
short viewports, doubled CSS text and visual-viewport-only resizing. Portable
tests, SDK/font/package identity and final cross-app fixtures pass. The
[validation record](VALIDATION.md) links the exact evidence and limits.

No scrolling text, extra watch headers, provider-specific layout API, parallel
notification renderer, global priority service or Dashboard implementation was
introduced. Primary content remains grouped immediately above its bottom
context, and secondary activity remains one static row above it.

Real phone WebView/permissions, physical button timing, vibration, display
appearance under real lighting and battery remain device checks. Source health
is still an opening-time snapshot; sample previews do not represent live data.
