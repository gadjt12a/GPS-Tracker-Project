# GPS Tracker Project — VALTRACK-V4 Traccar Firmware

Modified firmware for the **VALTRACK-V4-VTS-ESP32-C3** GPS tracker, adapted to report location and telemetry to a self-hosted [Traccar](https://www.traccar.org/) server using the OsmAnd HTTP protocol.

Based on the original firmware by [Valetron Systems](https://www.valetron.com/).

---

## Hardware

- **Device:** VALTRACK-V4-VTS-ESP32-C3
- **MCU:** ESP32-C3 (RISC-V, BLE, Wi-Fi)
- **Modem:** SIMCom A7672G (4G LTE CAT1 + GNSS)
- **Power input:** 12V–42V DC (VCHG) with reverse polarity protection
- **Connectivity:** 4G LTE — requires SIM card with data

## What Was Changed

The original Valetron firmware sends a proprietary JSON POST to a GPS-Gate or Valetron-compatible server. This is not natively understood by Traccar.

### 1. OsmAnd HTTP protocol (replaces native JSON POST)
The HTTP reporting function was changed to emit an OsmAnd-format GET request:
```
http://<server>:5055/?id=<IMEI>&lat=<lat>&lon=<lon>&speed=<speed>&timestamp=<unix_ts>&vbat=<voltage>&ncsq=<signal>
```
- `id` — device IMEI, used as the Traccar device identifier
- `vbat` — main supply voltage (vehicle battery), measured via onboard ADC
- `ncsq` — network signal quality (RSSI,BER from modem)
- All extra parameters are stored as custom attributes in Traccar

### 2. No-fix guard
Prevents the device from sending reports when the GNSS module has no fix (lat/lon = 0,0). This avoids wasting SIM data and stops invalid positions and year-2019 timestamps accumulating in Traccar.

### 3. BLE device name with IMEI suffix
The BLE GAP initialisation order was corrected and the device now advertises as `VALTRACK-V4-VTS-XXXX` (last 4 digits of IMEI). This allows identifying individual units via BLE scan without a serial cable, and lets the Valetron config app find the device correctly.

### 4. Adaptive reporting + deep sleep (Phase 7a)
Three-tier power/reporting state machine driven by the onboard LIS3DH accelerometer (INT1 motion interrupt on GPIO 3):

| State | Condition | Report interval | Power |
|---|---|---|---|
| Moving | Motion within last 5 min | 30 s (configured via BT app) | Normal |
| Parked short | No motion 5 min – 48 hr | 5 min | Normal (modem stays awake — see §10) |
| Parked long | No motion ≥ 48 hr | Deep sleep + 8 hr heartbeat | ~negligible |

- On each heartbeat wakeup (8 hr timer), the device sends one position report then returns to deep sleep
- Motion during deep sleep (LIS3DH INT1 asserted low) wakes the device and resumes normal reporting immediately
- The 48 hr threshold is intentional: a daily driver parked Friday–Monday stays in parked-short mode over the weekend rather than entering deep sleep mid-weekend

### 5. OTA firmware updates
The device checks for a new firmware version on each boot, every 24 hours while running, and on demand via the `V_OTA` remote command. If the OTA server reports a newer version, the binary is downloaded over LTE and written to the inactive OTA partition. The device reboots into the new firmware; if it crashes before marking itself valid the bootloader auto-rolls back to the previous version. The partition is marked valid as soon as basic init completes — a transient modem failure does not trigger a rollback.

- Partition layout: dual OTA (`ota_0` @ 0x20000, `ota_1` @ 0x110000, each 960 KB) with `otadata` at 0xF000
- Version manifest: `GET http://ota.pawson.co.nz/version.json` → `{"ver":"x.y.z"}`
- Binary: downloaded once to the modem's HTTP buffer via a single `AT+HTTPACTION=0`, then read sequentially in 4096-byte blocks using `AT+HTTPREAD=<offset>,<len>` — no per-chunk HTTP reconnections
- Current firmware version reported as `fwver` attribute in every Traccar position

### 6. Remote commands via Traccar
Send commands to a device through Traccar's **Custom Command** (`Moved <CMD>`):

| Command | Effect |
|---|---|
| `Moved V_RESET` | Reboot the device (triggers OTA check on next boot) |
| `Moved PING_NOW` | Force an immediate position report |
| `Moved V_OTA` | Trigger an OTA check immediately without rebooting |

### 7. Speed derivation
The A7672G modem reports speed = 0 even when moving. Speed is derived from consecutive lat/lon positions divided by the interval between them, and sent in knots (which Traccar's OsmAnd decoder expects), so Traccar trip detection and speed display work correctly.

### 8. GPS sanity filter + last-known-position cache
Cold-start artifacts near (0°, 0°–3°) are rejected before being sent. When the GNSS module has no current fix, the last known valid position is reported instead, keeping the device visible on the map. Reports are blocked only if no fix has ever been obtained in the current session.

### 9. Multi-constellation GNSS + AGPS (v2.3.8)
`AT+CGNSSMODE=15` and `AT+CGPSXE=1` are sent at GPS power-on to request GPS + GLONASS + BeiDou + Galileo and XTRA extended ephemeris. **Note:** both commands return `ERROR` on the A7672G modem firmware shipped with this hardware — they are prepared but have no effect until Valetron ships a modem firmware update that supports them.

### 10. Modem always-on (no CSCLK sleep) (v2.3.14)
`AT+CSCLK=2` (modem slow-clock sleep) was removed from all code paths. The A7672G modem now stays at `AT+CSCLK=0` (always awake). Previously, the modem was put to sleep after each HTTP ping, but the GPS polling loop (`AT+CGPSINFO` every second) was waking it again immediately — causing a rapid wake-sleep cycle under full LTE RF load that both ran the modem hot and caused intermittent GPS read failures. With the modem always awake, GPS reads succeed reliably on every main loop tick.

### 11. 10-second track recording with batched send (v2.3.17)
While moving, a GPS sample (lat/lon/derived speed/GPS timestamp) is recorded every 10 seconds into a ring buffer — but only if the device has moved ≥25 m since the last recorded point, so parking records nothing. At ping time the whole buffer is sent inside a single HTTP session, one request per point with its own `timestamp=`, giving 10-second track resolution on the map while the radio only performs session setup once per ping interval. Failed sends keep unsent samples buffered for the next ping (up to ~10 minutes of history).

### 12. Ignition detection + power-cut alarm (v2.3.18)
Both derived from the main-supply voltage already measured for `vbat`:
- **Ignition** — the alternator lifts the vehicle bus above ~13.3 V when the engine runs; below 13.0 V it's off (3-second debounce). Reported as `ignition=true/false` on every ping.
- **Power-cut alarm** — if the main supply drops below 7.5 V (device running on backup Li-Po), an immediate ping carries `alarm=powerCut`; restoration above 8 V sends `alarm=powerRestored`. Traccar recognises these natively — add a Notification of type *Alarm* to get alerted. The alarm is cleared only after a confirmed successful send.

---

## Works With

- **Traccar server** (self-hosted, any recent version) — OsmAnd HTTP protocol on port 5055
- **Valetron VALTRACK-V4E-SETUP** Android app — BLE configuration (server URL, reporting interval)
- **Traccar mobile/web client** — device tracking, event notifications

## Server Configuration

In Traccar, add the device using its IMEI as the identifier. The OsmAnd protocol is enabled by default on port 5055. No additional server-side configuration is required — custom attributes (`vbat`, `ncsq`, `fwver`, `ignition`) are stored automatically, and `alarm=powerCut`/`powerRestored` raise native Traccar alarm events (add a Notification of type *Alarm* to be alerted).

Recommended `traccar.xml` filter settings (note `filter.future` takes **seconds**, not a boolean — a non-numeric value throws a per-position exception that silently disables all filtering):

```xml
<entry key="filter.enable">true</entry>
<entry key="filter.zero">true</entry>
<entry key="filter.duplicate">true</entry>
<entry key="filter.future">86400</entry>
<entry key="filter.maxSpeed">300</entry>
<entry key="filter.distance">5</entry>
```

---

## Build Environment

- ESP-IDF v6.0.1 (via PlatformIO's framework-espidf package; build with ninja in `build/`)
- PlatformIO with `valtrack_v4_vts_esp32_c3` board ID, or Arduino IDE with `esp32-c3-devkitm-1`
- USB programming via micro USB (no boot buttons required with `--before default-reset`; some units need `--no-stub` at 115200 baud)

## Flashing

Initial flash (sets up OTA partition table — required once per unit before OTA can be used):

```
esptool --chip esp32c3 -p <PORT> -b 460800 --before default-reset write-flash \
    --flash-mode dio --flash-freq 80m --flash-size 2MB \
    0x0      build/bootloader/bootloader.bin \
    0x8000   build/partition_table/partition-table.bin \
    0xf000   build/ota_data_initial.bin \
    0x20000  build/VALTRACK-V4-ESP32C3.bin
```

After the initial flash, subsequent updates are delivered via OTA — no USB access needed.

---

## Version History

| Version | Changes |
|---|---|
| **2.3.29** | I2C stability fix for harsh driving (2.3.28 rollback recovery): I2C master timeout reduced 1000 ms → 50 ms so a stuck bus (SDA held low by LIS3DH during vibration) blocks at most 50 ms instead of 1 s per read attempt; after 20 consecutive I2C failures the driver is deleted and re-initialised (bus recovery). `HarshDriveTask` now registered with TWDT (`esp_task_wdt_add`/`reset`) so a genuine hang is visible in the watchdog log. All 2.3.28 harsh-driving logic (20 Hz sampler, gravity-vector EWMA, event classification, `gmax` attribute) restored unchanged. |
| **2.3.28** | Harsh driving detection (Phase 7b): 20 Hz LIS3DH sampler task measures horizontal-plane g (mounting-angle agnostic via gravity EWMA). Sustained ≥0.4 g for 300 ms while above 15 km/h → classified by GPS speed trend over 2 s as `hardBraking`, `hardAcceleration`, or `hardCornering`; ≥1.85 g spike = `accident`. 15 s holdoff between alarms. `gmax` attribute reports peak horizontal g per ping for threshold tuning. **Rolled back: I2C bus lock-up during harsh driving caused TWDT/BLE lockup. Fixed in 2.3.29.** |
| **2.3.27** | Cell-tower positions (`nlat`) are now correct in the southern hemisphere: the A7672G prints negative CLBS coordinates uint32-wrapped (a latitude of −39.63 arrived as 4255.34, i.e. true + 2³²/10⁶), so values out of range are decoded by subtracting 4294.967296 — done in double precision, since float32 can't hold the wrapped 10-digit values. Field-verified on two units: decoded positions land within the modem's own reported accuracy. Temporary `nraw` debug attribute removed. |
| **2.3.26** | CLBS coordinates range-guarded so unconfirmed values are never reported or used as a position; temporary `nraw` attribute added carrying the sanitized raw `+CLBS` response — the debug data that let 2.3.27 crack the encoding. |
| **2.3.25** | Positions sent from the cache or cell towers no longer carry a stale/year-2000 `timestamp` — the param is omitted so Traccar uses receive time and the map always shows the device as current. Cell-tower location (`AT+CLBS`, refreshed every 5 min) is reported as `nlat`/`nlon`/`nacc` attributes whenever available, and used as the actual position if the device has never had a GPS fix. Deep-sleep heartbeat shortened 8 hr → 2 hr, so a long-parked vehicle calls home (and picks up queued Traccar commands) every 2 hours. |
| **2.3.24** | Track recording gated on LIS3DH motion (60 s window): parked GPS jitter exceeded the 5 m spacing gate and produced phantom movement/speeds on stationary vehicles. |
| **2.3.23** | BLE name is now `V4E-<full IMEI>` (e.g. `V4E-869731054075783`) so any BLE scan list — including the V4E-SETUP app — identifies the exact unit without a serial cable. Falls back to `VALTRACK-V4-VTS` until the modem reports the IMEI (~30 s after boot). |
| **2.3.22** | BLE name experiment (bare IMEI) — superseded by 2.3.23 same day. |
| **2.3.21** | **Parked-silence fix**: the event sender was dequeuing and *discarding* every ping once `MotionTimer` passed the 5-minute parked threshold — a parked device went completely silent (hours-long gaps) instead of reporting every 5 minutes. Dequeued events are now always sent. Removed `ForceToSleep()` from the send-failure paths — it cleared the whole packet queue on any network blip, silently losing data; failures now keep the queue, and 3 consecutive failures power-cycle the modem instead. Live pings report `vbat` from a live ADC read (the old source was sampled once at boot and frozen — e.g. stuck at 12.545 V for days) and speed from the track recorder (the modem's CGPSINFO speed field freezes; parked now reports a true 0). Track recording upgraded from 10 s to **1 s resolution** (≥5 m spacing, 256-sample buffer) with speed derived over the actual time between samples (fixes bogus 250 km/h spikes when samples were >10 s apart). New `uptime=<seconds>` attribute on every ping makes reboots visible server-side. |
| **2.3.20** | Fixed phantom deep sleep after software resets: the wakeup-cause register survives `esp_restart`, and a stale TIMER bit at boot was misread as an 8 hr heartbeat wake — sending a stationary device into real deep sleep minutes after an OTA/V_RESET reboot. Wakeup cause is now only trusted when the reset reason is deep sleep. |
| **2.3.19** | GPS field buffers cleared after each parse — the modem leaves the speed field empty when stationary, so the last driving speed was frozen and re-reported forever while parked. Speed now sent in knots (Traccar's OsmAnd decoder expects knots; km/h values were over-reported 1.85×). |
| **2.3.18** | Ignition detection from main-supply voltage (>13.3 V = engine on, <13.0 V = off, 3 s debounce), reported as `ignition=true/false`. Power-cut alarm: main supply lost (<7.5 V, running on backup LiPo) sends an immediate `alarm=powerCut` ping; restore sends `alarm=powerRestored`. Alarms survive failed sends. |
| **2.3.17** | 10-second GPS track recording: positions sampled into a 64-entry ring buffer while moving (≥25 m spacing), drained as a batch inside the ping HTTP session with per-point timestamps — 10 s track resolution at one session setup per ping. OTA download fix: stale `+HTTPREAD: 0` end markers no longer abort the download (root cause of OTA never completing). **First firmware delivered fully over the air.** |
| **2.3.16** | HTTP 200 with empty body now treated as success. Traccar's OsmAnd endpoint returns `200` with zero-byte body; firmware was treating the subsequent `AT+HTTPREAD` failure as a ping failure and retrying every ~11 s instead of every 30 s. |
| **2.3.15** | OTA startup reliability: 10 s delay after network init before first OTA check (gives LTE data plane time to stabilise); version fetch retried once after 5 s on failure. Removed remaining `AT+CSCLK=2` calls from SMS/SOS legacy code paths. |
| **2.3.14** | Removed `AT+CSCLK=2` from modem init and `XHTTP_Request` — modem stays at `CSCLK=0` so GPS polling succeeds reliably (root cause of missing trip data). Motion threshold default lowered from 18 → 4 counts with auto-migration on first boot. Periodic OTA check every 24 hr without reboot. `Moved V_OTA` Traccar command for immediate OTA check. |
| **2.3.13** | OTA rollback-timing fix: partition marked valid before `InitGSM()` so modem failures don't trigger rollback. OTA download redesigned: full binary buffered once in modem HTTP RAM via single `AT+HTTPACTION=0`, read sequentially with `AT+HTTPREAD=<offset>,<len>` — eliminates 165 per-chunk HTTP connections and the `+HTTPREAD: 0` residual bug that was aborting downloads. |
| **2.3.8** | Multi-constellation GNSS (`AT+CGNSSMODE=15`) and XTRA AGPS (`AT+CGPSXE=1`) commands added at GPS init (commands return ERROR on current modem firmware — no-op until modem update). |
| **2.3.7** | Fixed `uart_event_task` crash on null byte in modem URC stream. |
| **2.3.6** | NVS position persistence across reboots. 5-minute boot window allows pinging Traccar before GPS fix. |
| **2.3.5** | OTA partition marked valid immediately at boot rather than after first GPS fix, preventing spurious rollbacks on slow GPS acquisition. |
| **2.3.4** | OTA chunk download bug fix. HTTP session storm fix (defensive `AT+HTTPTERM` before `AT+HTTPINIT`). |
| **2.3.2** | Remote command support: `Moved V_RESET` (reboot) and `Moved PING_NOW` (force report) via Traccar custom commands. |
| **2.3.0** | Speed derivation from consecutive positions. GPS cold-start artifact filter. Zero-speed trip detection fix. |
| **2.2.0** | OTA version check comma-count fix. Flash address corrected to `0x20000` (ota_0). |
| **2.1.0** | Initial OTA implementation (dual-partition layout, modem HTTP download). |

---

## Original Firmware

[ValetronSystems/VALTRACK-V4-ESP32-C3](https://github.com/ValetronSystems/VALTRACK-V4-ESP32-C3)
