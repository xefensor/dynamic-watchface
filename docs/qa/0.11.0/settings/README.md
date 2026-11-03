# Dynamic activity settings validation

`en.png` and `cs.png` show the actual offline Clay page at 360 × 800 with synthetic installed, idle and unavailable providers. Provider names use literal text, including a deliberately hostile test name. No live music or notification content is captured.

`validation.json` records the source hashes, real Clay save responses and checks. The browser exercises buttons, keyboard arrows, native touch dragging, desktop drag/drop, ordering-only dirty state and Music visibility. Each real save response is replayed through production PKJS with simulated SDK coordinators to check both Timer/Music orders, hiding, phone storage and restoration in a fresh host. A separate fixture starts with 64 unavailable saved providers and confirms that installed apps and Music remain controllable.

Run a separate local browser, then execute the driver from the repository:

```sh
chromium --headless --no-sandbox --disable-gpu --remote-debugging-port=9227 --user-data-dir=/tmp/dynamic-settings-browser about:blank
```

```sh
node scripts/check-settings-activities.cjs
```

The driver uses the pinned Clay bundle from `node_modules` (including its packaged `dist.zip`). It requires Chromium, `unzip`, and Node with built-in `fetch` and `WebSocket`. An optional output directory is the first argument. `DYNAMIC_CHROMIUM_ENDPOINT` overrides the local debugging endpoint. Temporary HTML files are removed after success. This verifies browser rendering and simulated phone integration, not a physical phone or watch.
