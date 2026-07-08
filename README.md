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

Three modifications were made to `main/main.c`:

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

### 3. BLE device name fix
The BLE GAP initialisation order was corrected so the device advertises as `VALTRACK-V4-VTS` rather than the NimBLE default name `nimble`. This allows the Valetron BT config app to find and configure the device correctly.

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

```
esptool --chip esp32c3 -p <PORT> -b 460800 --before default-reset write-flash \
    --flash-mode dio --flash-freq 80m --flash-size 2MB \
    0x0     build/bootloader/bootloader.bin \
    0x8000  build/partition_table/partition-table.bin \
    0x10000 build/VALTRACK-V4-ESP32C3.bin
```

---

## Original Firmware

[ValetronSystems/VALTRACK-V4-ESP32-C3](https://github.com/ValetronSystems/VALTRACK-V4-ESP32-C3)
