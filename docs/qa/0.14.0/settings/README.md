# Dynamic settings cleanup validation

These are actual offline Clay pages with synthetic provider metadata. The form
uses the pinned Clay bundle, current 0.14.0 settings code and the refreshed native
watch previews. No live music or notification content is captured.

- `appearance-light.png` and `appearance-dark.png`: section shortcuts and sample
  preview, using phone light/dark colours independently from watch colours.
- `en.png` and `cs.png`: priority list, including installed and unavailable apps.
- `plugins-off-en.png` and `plugins-off-cs.png`: generic rows explain the disabled
  master switch while Music/Notifications remain independently controllable.
- `notification-details-en.png` and `notification-details-cs.png`: the separate
  three-choice privacy control, optional vibration and source app filter.
- `connection-en.png` and `connection-cs.png`: one source overview and collapsed
  diagnostics, with the fixed Save area visible.
- `reopened-app-only-320.png`: a narrower page after reopening saved hidden
  app-only settings and re-enabling Notifications without changing privacy.

`validation.json` records the exact current source hashes, real Clay responses
and browser assertions. Interactions cover native touch drag, desktop drag/drop,
keyboard/buttons, order-only dirty state, explicit source hides and the 64-provider
boundary. Production PKJS replays the actual Clay responses with simulated SDK
coordinators and restores saved priority on a fresh host. Provider names render
as text, including a deliberately hostile test name.

The three section shortcuts use the translated section headings. Native Enter-key
events verify target focus, unchanged browser history/hash and visible Save.
320/360px checks reject horizontal overflow and undersized controls. Notification
visibility has one checkbox; privacy has three choices with no duplicate Off.
Off/on and fresh-page restoration preserve app-only and the saved body preference.
The plugin master switch changes neither saved order nor per-app visibility and
does not disable Music/Notifications. Opening saved hidden settings is not dirty.
Page-evaluation exceptions fail the driver rather than silently returning no value.

All active copy and control identifiers also pass the 17-locale portable tests.

```sh
DYNAMIC_CHROMIUM_ENDPOINT=http://127.0.0.1:9243 \
  node scripts/check-settings-activities.cjs docs/qa/0.14.0/settings
```

The driver needs a local Chromium debugging endpoint, `unzip`, and Node with
`fetch`/`WebSocket`. It removes temporary local HTML after success. Screenshots
wait for control transitions to settle. Browser rendering and simulated host
integration do not establish a real phone, watch or Quick Launch result.
