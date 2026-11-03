# Dynamic notification activity settings validation

`en.png` and `cs.png` show the actual offline Clay page at 360 × 800 with synthetic installed, idle and unavailable providers. Provider names use literal text, including a deliberately hostile test name. No live music or notification content is captured.

`appearance-light.png` and `appearance-dark.png` show the final form and refreshed embedded native preview before editing. Native preview identity is checked separately by the final package audit.

`validation.json` records exact source hashes and real Clay save responses. The browser exercises buttons, keyboard arrows, native touch dragging, desktop drag/drop, ordering-only dirty state and Music visibility. Notifications starts Off. Tests enable it as secondary, move it to primary, save and restore both roles through production PKJS, synchronize row visibility with privacy Off, and preserve app-only privacy after off/on. Each save response is replayed with simulated SDK coordinators; the response itself comes from the actual Clay serializer. A separate fixture begins with 64 unavailable saved providers and confirms that installed apps, Music and Notifications remain controllable. The checks also reject horizontal overflow, small row buttons, script exceptions and HTML interpretation of provider names.

Start a local Chromium with loopback debugging, then run:

```sh
node scripts/check-settings-activities.cjs docs/qa/0.13.0/settings
```

The driver uses the pinned Clay bundle from `node_modules` (including its packaged `dist.zip`). It requires Chromium, `unzip`, and Node with built-in `fetch` and `WebSocket`. `DYNAMIC_CHROMIUM_ENDPOINT` can override the default loopback endpoint `http://127.0.0.1:9227`. Temporary local HTML files are removed after success. This proves browser rendering and simulated phone integration, not a physical phone, watch or Quick Launch hold.
