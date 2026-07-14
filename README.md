# GPS Tracker Project — VALTRACK-V4 Traccar Firmware

Modified firmware for the **VALTRACK-V4-VTS-ESP32-C3** GPS tracker, adapted to report location and telemetry to a self-hosted [Traccar](https://www.traccar.org/) server using the OsmAnd HTTP protocol.

Based on the original firmware by [Valetron Systems](https://www.valetron.com/).

---

## Hardware

- **Device:** VALTRACK-V4-VTS-ESP32-C3
- **MCU:** ESP32-C3 (RISC-V, BLE, Wi-Fi)
- **Modem:** SIMCom SIM7672G (4G LTE CAT1 + GNSS)
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
| Parked short | No motion 5 min – 48 hr | 5 min | Modem light sleep between reports |
| Parked long | No motion ≥ 48 hr | Deep sleep + 8 hr heartbeat | ~negligible |

- On each heartbeat wakeup (8 hr timer), the device sends one position report then returns to deep sleep
- Motion during deep sleep (LIS3DH INT1 asserted low) wakes the device and resumes normal reporting immediately
- The 48 hr threshold is intentional: a daily driver parked Friday–Monday stays in parked-short mode over the weekend rather than entering deep sleep mid-weekend

### 5. OTA firmware updates
The device checks for a new firmware version on each boot. If the OTA server reports a newer version, the binary is downloaded over LTE and written to the inactive OTA partition. The device reboots into the new firmware; if it crashes before marking itself valid the bootloader auto-rolls back to the previous version. The partition is marked valid as soon as basic init completes — a transient modem failure does not trigger a rollback.

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

### 7. Speed derivation
The SIM7672G/A7672 modem reports speed = 0 even when moving. Speed is now derived from consecutive lat/lon positions divided by the reporting interval, so Traccar trip detection works correctly.

### 8. GPS sanity filter + last-known-position cache
Cold-start artifacts near (0°, 0°–3°) are rejected before being sent. When the GNSS module has no current fix, the last known valid position is reported instead, keeping the device visible on the map. Reports are blocked only if no fix has ever been obtained in the current session.

### 9. Multi-constellation GNSS + AGPS (v2.3.8)
`AT+CGNSSMODE=15` enables GPS + GLONASS + BeiDou + Galileo simultaneously, giving 3–4× more visible satellites. `AT+CGPSXE=1` enables XTRA extended ephemeris download over LTE, reducing cold-start TTFF from 12+ minutes to seconds. Both commands are sent at GPS power-on during modem init.

---

## Works With

- **Traccar server** (self-hosted, any recent version) — OsmAnd HTTP protocol on port 5055
- **Valetron VALTRACK-V4E-SETUP** Android app — BLE configuration (server URL, reporting interval)
- **Traccar mobile/web client** — device tracking, event notifications

## Server Configuration

In Traccar, add the device using its IMEI as the identifier. The OsmAnd protocol is enabled by default on port 5055. No additional server-side configuration is required — custom attributes (`vbat`, `ncsq`) are stored automatically.

---

## Build Environment

- ESP-IDF (v5.x)
- PlatformIO with `valtrack_v4_vts_esp32_c3` board ID, or Arduino IDE with `esp32-c3-devkitm-1`
- USB programming via micro USB (no boot buttons required with `--before default-reset`)

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

## Original Firmware

[ValetronSystems/VALTRACK-V4-ESP32-C3](https://github.com/ValetronSystems/VALTRACK-V4-ESP32-C3)
