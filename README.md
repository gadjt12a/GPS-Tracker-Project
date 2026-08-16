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

> ⚠️ **DEEP SLEEP IS CURRENTLY A ONE-WAY TRIP — the 2 hr timer heartbeat never fires.** Only
> motion wakes a sleeping unit. Confirmed 2026-08-13 with a fast-cycle diagnostic build: a
> bench unit slept and stayed dark **9 hr 19 min across ~112 missed 5-minute heartbeats**,
> returning only when physically shaken. Entering deep sleep works and GPIO/motion wake works;
> the timer is armed (`DeepSleep.c:205`) and simply never triggers. Prime suspect is the
> ESP32-C3 `esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown()` path powering down the
> domain the RTC timer needs. **Practical consequence: a vehicle parked past
> `PARK_LONG_SECONDS` goes dark and looks identical to a stolen or disconnected unit.** Tracked
> as **D2 (P1)** in `ISSUES.md`. The v2.3.52 change below makes the gate *easier* to reach and
> is therefore **held back from production** until D2 is fixed.

**Motion must be *confirmed* before the 48 hr timer resets (v2.3.52 — staging only, blocked by D2).** `ParkLongTimer` used to be zeroed by any single INT1 assertion, which meant deep sleep required 48 *consecutive* hours with not one interrupt — unreachable anywhere a vehicle actually parks. Two undisturbed bench units ran 55 hr without ever sleeping, having logged 47 and 19 stray assertions from ordinary garage vibration. The timer now resets only on `MOTION_CONFIRM_COUNT` (3) distinct assertions within `MOTION_CONFIRM_WINDOW_S` (120 s). The two populations are about two orders of magnitude apart — bench noise runs 0.35–0.85 assertions/hour and uncorrelated, active driving ~10 per *minute* — so 3-in-120 s is met seconds into any real drive and effectively never by noise. `MotionTimer` is deliberately **not** debounced: it drives the 30 s/5 min cadence, where reacting to the first movement is correct. The 64 mg wake threshold is untouched, because motion wake is what *ends* deep sleep and desensitising it risks a unit sleeping through being driven away. Attributes `plt` (timer value) and `pltr` (confirmed resets) make the gate observable — read `pltr` against `ipoll`, since `ipoll` counts every assertion and `pltr` only those that survived confirmation.

### 5. OTA firmware updates
The device checks for a new firmware version on each boot, every 24 hours while running, and on demand via the `V_OTA` remote command. If the OTA server reports a newer version, the binary is downloaded over LTE and written to the inactive OTA partition. The device reboots into the new firmware; if it crashes before marking itself valid the bootloader auto-rolls back to the previous version. The partition is marked valid as soon as basic init completes — a transient modem failure does not trigger a rollback.

- Partition layout: dual OTA (`ota_0` @ 0x20000, `ota_1` @ 0x110000, each 960 KB) with `otadata` at 0xF000
- Version manifest: `GET http://ota.pawson.co.nz/version.json` → `{"ver":"x.y.z"}`
- Binary: downloaded once to the modem's HTTP buffer via a single `AT+HTTPACTION=0`, then read sequentially in 4096-byte blocks using `AT+HTTPREAD=<offset>,<len>` — no per-chunk HTTP reconnections
- Current firmware version reported as `fwver` attribute in every Traccar position

**Staging channel — per-device testing (v2.3.51).** `version.json` is global, but *which* manifest a device reads is per-device, and Traccar commands are addressed to a single device. That combination gives targeting the OTA path never had, so a test build can go to one bench unit without touching the fleet.

| Command | Effect |
|---|---|
| `Moved V_OTA_TEST` | Switch **this** device to the staging channel (`/staging/version.json`) and update |
| `Moved V_OTA` | Reset to production and update — also the way home from staging |

The channel persists in NVS (`ota_ch`), which is essential rather than convenient: without it the staging build reboots, the boot check reads *production*, sees a different version and pulls straight back — an OTA ping-pong burning cellular data every cycle. `ota_channel_load()` therefore runs before `CheckAndApplyOTA()`. A staged device reports `otach=1` on every ping so a forgotten test unit is visible; production pings are byte-identical to before. If the staging server is unreachable the device stays on staging and skips the update rather than auto-reverting, since auto-revert reintroduces the ping-pong. Returning home works because the version comparison is `strcmp`-inequality, not ordering, so a downgrade is just another update.

Validated end to end on unit 2 (2026-08-09): `V_OTA_TEST` → `2.3.51-rc1` with `otach=1` while the van and unit 3 stayed put, then `V_OTA` → back to `2.3.51` with `otach` cleared. Budget ~8 min per command round trip — a parked unit is on the 5 min ping cadence and Traccar only delivers commands in a ping response, so ~5 min to delivery plus ~2 min for the OTA.

### 6. Remote commands via Traccar
Send commands to a device through Traccar's **Custom Command** (`Moved <CMD>`):

| Command | Effect |
|---|---|
| `Moved V_RESET` | Reboot the device (triggers OTA check on next boot) |
| `Moved PING_NOW` | Force an immediate position report |
| `Moved V_OTA` | Trigger an OTA check immediately without rebooting; resets the OTA channel to production (v2.3.51) |
| `Moved V_OTA_TEST` | Move this device to the staging OTA channel and update (v2.3.51 — see §5) |

**The `Moved ` prefix is mandatory.** The whole command block is gated on the response body
containing `"logid"` or `"Moved"` (`main.c:5093`); a bare `V_OTA_TEST` is received and
**silently ignored**, with no error at either end. The Traccar log shows what was actually
delivered — `content-length: 16 ... Moved V_OTA_TEST` works, `content-length: 10 ...
V_OTA_TEST` does not. Grep `queuedCommandSent`.

### 7. Speed derivation
The A7672G modem reports speed = 0 even when moving. Speed is derived from consecutive lat/lon positions divided by the interval between them, and sent in knots (which Traccar's OsmAnd decoder expects), so Traccar trip detection and speed display work correctly.

### 8. GPS sanity filter + last-known-position cache
Cold-start artifacts near (0°, 0°–3°) are rejected before being sent. When the GNSS module has no current fix, the last known valid position is reported instead, keeping the device visible on the map. Reports are blocked only if no fix has ever been obtained in the current session.

### 9. Multi-constellation GNSS + AGPS (v2.3.8)
`AT+CGNSSMODE=15` and `AT+CGPSXE=1` are sent at GPS power-on to request GPS + GLONASS + BeiDou + Galileo and XTRA extended ephemeris. **Note:** both commands return `ERROR` on the A7672G modem firmware shipped with this hardware — they are prepared but have no effect until Valetron ships a modem firmware update that supports them.

### 10. Modem always-on (no CSCLK sleep) (v2.3.14)
`AT+CSCLK=2` (modem slow-clock sleep) was removed from all code paths. The A7672G modem now stays at `AT+CSCLK=0` (always awake). Previously, the modem was put to sleep after each HTTP ping, but the GPS polling loop (`AT+CGPSINFO` every second) was waking it again immediately — causing a rapid wake-sleep cycle under full LTE RF load that both ran the modem hot and caused intermittent GPS read failures. With the modem always awake, GPS reads succeed reliably on every main loop tick.

### 11. Track recording with batched send (v2.3.17, rates corrected v2.3.47)
While moving, a GPS sample (lat/lon/derived speed/GPS timestamp) is recorded into a 256-entry ring buffer — but only if the device has moved ≥5 m since the last recorded point, so parking records nothing. At ping time the whole buffer is drained inside a single HTTP session, one request per point with its own `timestamp=`, so track resolution is independent of the ping interval and the radio only performs session setup once per ping. Failed sends keep unsent samples buffered for the next ping.

**The actual sample rate is not the configured one, and this is worth understanding before tuning it.** `TRACK_SAMPLE_SECONDS` is 1 and `TrackSampleTick()` does run at 1 Hz, but it discards every tick where the GPS second has not advanced — and that only advances when `XCheckGPS()` polls the modem, which happens **once per main-loop pass**. The real sampling interval is therefore the main-loop period. Measured on the van from Traccar's own sample timestamps (2026-08-06): **4–5 s apart within a burst, 11–21 s across ping boundaries, ~4.6 s average** — not the 1 s the constant implies.

The loop period is dominated by fixed `osDelay()`s in the AT helpers rather than by anything GNSS-related, so 2.3.47 throttles the worst offender (`CheckNetwork()`, 1.5 s of sleep per call) to 30 s while GNSS is live. Two caveats for anyone tempted to push further:

- **1 Hz is a hard floor.** The modem emits one fix per second and `+CGPSINFO` timestamps are whole seconds. Sub-second sampling is not reachable without a higher GNSS output rate, which this modem firmware does not appear to offer.
- **Delivery may not keep up.** Each buffered sample costs its own `AT+HTTPPARA` + `AT+HTTPACTION` round trip. Faster sampling with the same drain rate just grows the backlog until the ring wraps and silently overwrites. `tqd` and `tdrp` (2.3.47) exist to measure this — check them before raising the rate again.

Data cost scales with sample count: roughly 600–800 bytes on the wire per sample, so ~550 KB per driving hour at the current ~4.6 s, and ~2.5 MB/h if it ever reaches a true 1 Hz.

### 12. Ignition detection + power-cut alarm (v2.3.18, thresholds revised v2.3.36)
Both derived from the main-supply voltage already measured for `vbat`:
- **Ignition** — the alternator lifts the vehicle bus above ~14.1 V when the engine runs; a resting battery sits at 12.7–13.1 V. Switches on above **13.8 V** (3 s debounce) and off below **13.5 V** (15 s debounce, so an idling engine sagging under load doesn't flicker the state). Reported as `ignition=true/false` on every ping, and a transition forces an immediate ping so Traccar's trip boundaries are accurate. The original 13.3/13.0 pair put the off-threshold below a charged battery's resting voltage, so the device reported "engine running" for 45–60 minutes after shutdown while surface charge decayed.
- **Power-cut alarm** — if the main supply drops below 7.5 V (device running on backup Li-Po), an immediate ping carries `alarm=powerCut`; restoration above 8 V sends `alarm=powerRestored`. Traccar recognises these natively — add a Notification of type *Alarm* to get alerted. The alarm is cleared only after a confirmed successful send.

### 13. GNSS watchdog + honest no-fix reporting (v2.3.33–2.3.34)
The modem's GNSS can stop producing fixes mid-session and never restart on its own — `AT+CGNSSPWR=1` was only ever issued at modem init. Two parts:
- **No-fix is now reported honestly.** On fix loss `fLat`/`fLong` and the GPS clock are cleared, so the ping falls into the cached/cell path (timestamp omitted, Traccar uses receive time) instead of replaying the last fix as if it were current. Previously a dead receiver was indistinguishable from a parked vehicle — two units sat frozen for days looking healthy.
- **Escalating recovery** — `AT+CGNSSPWR` power-cycle → `AT+CGPSCOLD` → modem reinit, reset on any successful fix, with geometric backoff. Only runs while the vehicle is active (ignition on, or motion within 60 s): a parked vehicle with no fix is usually just parked where it can't see sky, and escalating there cost ~6 modem reinits a night for nothing.

New attributes on non-live-fix pings: `gpsage` (seconds since last real fix, −1 = never), `gpsrec` (recovery attempts since boot) and `gpsnr` (polls where `AT+CGPSINFO` returned nothing parseable — a timeout or a reply with no `+CGPSINFO:` line).

### 14. Harsh driving via LIS3DH hardware interrupt (Phase 7b, v2.3.37+)
Detection runs **in the accelerometer**, not in firmware. The LIS3DH's second interrupt generator is programmed with the threshold and duration directly:

```
INT2_THS      = 15 counts × 16 mg = 240 mg  (HARSH_EVENT_G 0.25g, v2.3.47)
INT2_DURATION =  2 counts × 40 ms =  80 ms  (HARSH_EVENT_MS,     v2.3.54 — SETTLED)
CTRL_REG1     = 0x37 = 25 Hz ODR            (LIS3DH_ODR_HZ,      v2.3.48)
INT2_CFG      = 0x0A = XHIE|YHIE, OR        (HARSH_INT2_CFG,     v2.3.49)
```

All four derived from macros in `SCI.h` so they can't drift apart. **ODR is not just a sample rate** — it sets the `DURATION` tick (1/ODR) *and* the high-pass cutoff (ODR/400 at `HPCF=11`), which is why 2.3.48 changed it. At 25 Hz a tick is 40 ms, and `HARSH_EVENT_MS` is **settled at 80 ms (2 ticks) — do not tune it again**, see the status note below. Both generators share the single wired INT1 pin (`CTRL_REG3 = 0x60`) and both are latched (`CTRL_REG5 = 0x0A`), so an INT1 poll can't miss a short event. Events are classified as `hardBraking` / `hardAcceleration` / `hardCornering` from the track-derived speed trend, and delivered as `alarm=` on a forced ping.

**Why it's built this way:** every earlier attempt used a 20 Hz polling task, and every build carrying it wedged the device within hours — the BLE `btController` task livelocks and starves everything else. Instrumentation in 2.3.35 disproved all three suspected causes (stack high-water stayed at 3320/4096, free heap at 142 KB, I2C bus recoveries at 0), leaving contention between continuous I2C traffic and the BLE controller. Removing the polling entirely is the fix, not hardening it: 2.3.37+ runs for hours over rough roads with no resets.

Attributes: `hmin` (min free heap since boot), `hcnt` (events passing the gates), `hraw` (raw sensor interrupts before gating — `hraw > 0` with `hcnt = 0` means the gates are wrong, `hraw = 0` means the sensor isn't triggering), plus `lisreg`/`i2src` from 2.3.41 (the sensor's own config read back at ping time) and `ipoll`/`qpoll` from 2.3.43 (how often each `INT2_SRC` read site actually executes — `qpoll` is the control, so `qpoll` moving while `ipoll` is frozen isolates the fault to the INT1 pin rather than the code path).

`i2src = FF` is the "never read" sentinel, not a fault — `last_int2_src` initialises to `0xFF` and only the INT1 poll writes it, so a freshly booted unit with `ipoll = 0` correctly shows `FF`. Equally, a *constant* `i2src` proves only that it was written at least once; that ambiguity is why `ipoll` exists.

**Status (2.3.56): braking is detected and road noise is filtered in firmware. Final calibration on test.**

The mechanism turned out to be the **high-pass filter**, not the threshold. Four versions settled it:

- **2.3.47** (0.25 g) produced the first ever field alarm, but not on braking — a 0.409 g and a 0.364 g stop both left `hraw` at 0, with `ipoll` climbing eight polls past each event and the interrupt latched, so the sensor genuinely never triggered. A 0.409 g brake cannot fail a 0.25 g threshold. What *did* fire had `i2src` 0x65/0x56 — the vertical axis — with no deceleration near it.
- **2.3.48** dropped ODR 100 Hz → 25 Hz, moving the filter cutoff from 0.25 Hz to 0.0625 Hz. Braking immediately started registering at the *same* 0.25 g: three stops at 0.294/0.252/0.338 g each produced an `hraw` increment.
- **2.3.49** drops `ZHIE`, because the lower cutoff also let road-surface vertical jolts through — `hcnt` reached 12 in eight minutes of ordinary driving. Confirmed over five drives: the gated rate fell **3×** (`hcnt` per moving ping 0.57 → 0.19), and every `IA`-set `i2src` afterwards involved a horizontal axis.
- **2.3.50** raises the duration gate 80 ms → 480 ms. Cross-referencing all 18 surviving `hcnt` events against the GPS speed trace showed **0 events ≥ 0.25 g, 5 at 0.15–0.24 g and 11 below 0.15 g** — four of those at highway speed with no deceleration at all (0.036 g, 0.040 g twice, 0.059 g). The sensor isn't wrong there: those are genuine transients that a 2–3 s GPS average cannot see.

- **2.3.53** stepped the gate back to 320 ms — and 480 ms was already proven wrong by then: a **0.518 g** stop (71.8 → 35.2 km/h in 2 s) produced nothing.
- **2.3.54** abandoned the duration approach entirely and moved the discrimination into firmware.

**Duration is NOT the discriminator — settled, do not bisect this register again.** Three values, each measured on a real drive:

| `INT2_DURATION` | Result |
|---|---|
| 2 ticks (80 ms) | Detects real braking (2.3.48: 0.294/0.252/0.338 g) — but also road transients |
| 8 ticks (320 ms) | Nothing. A **0.391 g** brake ignored |
| 12 ticks (480 ms) | Nothing. A **0.518 g** brake ignored |

The register counts **consecutive** samples above threshold and one dip resets the counter, which is hostile to a signal dithered by the high-pass filter. There is no usable window, so 80 ms stays — it is the only value proven to detect braking at all. Raising `HARSH_EVENT_G` is equally wrong: real brakes measure 0.15–0.24 g GPS-averaged, at or *below* the threshold the noise is already clearing, so the populations overlap in amplitude too.

**The separation now happens in firmware (2.3.54): an event with no speed signature is rejected.** `HarshEventDetected()` used to fall through to `hardCornering` for anything without a speed change, which made cornering the catch-all — and is exactly how a pothole at steady 70 km/h became an alarm. The sensor cannot tell a 0.4 g brake from a 0.4 g jolt (the interrupt reports only *that* the threshold was crossed), but the speed trend can: a brake moves the vehicle, a pothole does not. This reads the track history already recorded at 2–3 s resolution, so it costs no I2C and no faster poll — which matters, because the obvious alternative of counting several interrupts in a window cannot work here: the latch collapses every crossing between two polls into one, and raising the poll rate is what caused the btController livelock.

**Result (2.3.54, 2026-08-13 drive):** `hraw` 29 = `hcnt` 11 + `hnod` 18 — **62 % of sensor events rejected**, and for the first time genuine hard stops came through as alarms (0.464 g, 0.458 g, 0.471 g, 0.338 g). Compare 2.3.49 on the same detection: 18 accepted, **zero** ≥ 0.25 g.

**Known limitation, deliberate:** pure cornering at constant speed has no speed signature and is rejected along with the potholes. `hardBraking` and `hardAcceleration` become trustworthy; cornering needs a real magnitude (deferred FIFO capture) or heading change from the track buffer.

**Normalise before comparing versions.** Raw `hraw`/`hcnt` totals made 2.3.49 look no better than 2.3.48; per unit of driving exposure it was 3× better. The versions ran for very different durations, so per-moving-ping is the only fair measure.

**Two analysis traps in the Traccar data.** Roughly 40% of rows are buffered track samples delivered *out of order*, carrying `hraw=0` and stale `uptime` — naively they look like scores of reboots and a stream of counter resets, so filter to live pings (uptime strictly increasing) before counting anything. And wire speed is **knots**, not km/h (`Speed / 1.852f`), so every deceleration is 1.85× too small if you forget.

**Why it took seven versions:** every theory before this explained *absence* — why nothing fired. The one that held had to explain something harder, that a 0.409 g brake produced nothing while a vertical bump produced an alarm, on the same drive, minutes apart, at the same threshold. Frequency selectivity is the only mechanism that does both. Note also that the 2.3.47 track-resolution fix is what made this legible: at the old ~4.6 s sampling that 0.409 g stop would have measured ~0.2 g, looked consistent with "threshold still too high", and bought the wrong theory another version.

A bench bisection on unit 2 (2026-08-05, USB, `FW_VERSION` pinned so it could not self-revert) settled the earlier question of whether the chain worked at all. At **32 mg / 20 ms** a 10-second hand shake produced `hraw` 0 → 5, with `i2src` reading `0x65`/`0x66` — `IA` set, `ZH` and `XH` events. Generator 2 fires, the pin asserts, the poll reads it, `HarshEventDetected()` runs. Nothing is broken.

What the bench could **not** settle is where between 32 mg and 400 mg real events sit. Hand shaking is oscillatory — every reversal takes the acceleration through zero — so it is a poor match for a duration gate, and sharp 90° flips (a genuine sustained step through the high-pass filter) also produced nothing at 400 mg / 100 ms. Only real braking, which is sustained and unidirectional for 1–3 s, can answer it.

**`INT2_CFG` stays `0x2A`.** A hypothesis that `0x2A` is direction-blind — that `XL`/`YL`/`ZL` fire on `axis < −THS` and were disabled — was **disproven on the bench**. In the LIS3DH the low events mean *below threshold in magnitude*; they exist for freefall/6D detection, where `AOI=1` and a low threshold make the interrupt fire when all axes approach zero. Under `AOI=0` they mean "fire whenever any axis is below 400 mg", which is trivially true at rest. `0x3F` produced `hraw` 8 → 23 on a **motionless** bench with `i2src=0x55` (`XL|YL|ZL|IA`) — every poll finding `IA` set. Recorded on branch `diag/harsh-bidir` so it is not retried.

2.3.41's readback settled what the problem *isn't* — the registers come back exactly as intended, so the sensor is correctly configured and stays that way.

2.3.43's poll counters then found the real problem. Measured 2026-08-04, van, 17.9 h uptime including a drive at 82 km/h:

| Unit | uptime | `ipoll` | `qpoll` | rate |
|---|---|---|---|---|
| Van (driven) | 64269 s | 98 | 0 | ~1 per 11 min |
| Unit 2 (bench) | 64383 s | 27 | 0 | ~1 per 40 min |
| Unit 3 (bench) | 29618 s | 13 | 0 | ~1 per 38 min |

Two things fall out:

- **The INT1 poll is nothing like ~1 Hz.** Every comment describing "the ~1Hz poll in StartMainTask" (including the one justifying latching at `main.c:1006`) is wrong. *Corrected 2026-08-05:* the ~1-per-11-minutes figure above is the **parked / gentle-commute** rate. Under genuinely hard driving it rises to ~10/min — `ipoll` climbed 92 → 132 in four minutes during a hard-braking run. So the poll is far more responsive than first stated when it matters, but still orders of magnitude off 1 Hz, and the latching justification remains built on a rate nobody measured.
- **`qpoll` is 0 on every unit, including the driven van.** The `XHTTP_Request` read site — documented as running every ~30 s while driving, and intended as the backstop — has never executed at all.

Against that, `InitAccelerometer()` reads `INT2_SRC` at `main.c:1031` (`// clear latch`) and is throttled to run **once every 60 seconds** (`main.c:7490`). So the latch is destroyed every 60 s while the counter that would record it is read every ~660 s — roughly 11:1 against ever catching an event. A harsh event is latched, then wiped long before anyone looks.

This was the first explanation derived from measurement rather than inference, after four inferred ones were wrong (stack/heap/I2C 2.3.35, HPF cutoff 2.3.38, re-init state reset 2.3.39, register drift 2.3.41). Fixed in 2.3.44, which made the re-init test bit 6 before discarding `INT2_SRC`.

**It was a real bug but not the reason `hraw` stayed 0.** A test drive on 2.3.44 with deliberate hard braking still produced `hraw=0`, and the bench bisection then showed the thresholds were the actual blocker. Worth keeping in mind: fixing a genuine defect that measurement points at does not guarantee it was *the* defect. Note the trial is also gated on GNSS — `hcnt`'s speed gate needs real GPS speed.

**Not implemented:** accident severity. The interrupt reports that the threshold was crossed, not by how much, and the event is over before a ~1 Hz poll could read the peak. The intended answer is the LIS3DH FIFO in Stream-to-FIFO mode, which freezes on trigger and preserves the waveform.

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
<entry key="filter.skipLimit">600</entry>
<entry key="filter.skipAttributes">alarm</entry>
```

`filter.skipLimit` matters more than it looks. With only `filter.distance` set, a **parked** vehicle never moves 5 m, so Traccar discards the whole position — and with it every attribute riding on it (`uptime`, `vbat`, `ignition`, GNSS health). The device appears online but nothing ever updates. `skipLimit` lets one position through whenever the gap since the last stored one exceeds 600 s, so parked telemetry still lands every 10 minutes. `filter.skipAttributes=alarm` guarantees a position carrying an alarm is never filtered.

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
| **2.3.56** | **Harsh speed-delta 6 → 15 km/h — the constant changed job and the value did not follow.** `HARSH_SPEED_DELTA` was written to *classify* an event (braking vs acceleration vs cornering), where 6 km/h is a sensible dividing line. Since 2.3.54 it also *rejects*: an event clearing neither direction is discarded as a road transient. As a rejection threshold, 6 km/h over the 4 s `HARSH_SPD_HIST` window is only **0.042 g** — and the worst false positive on the 2026-08-13 drive measured 0.043 g (69.9 → 65.4 km/h), clearing the bar almost exactly. 15 km/h is ~0.106 g averaged. Checked against every accepted event on that drive: keeps all four ≥ 0.25 g brakes (32–36 km/h drops) and the six mid-band events, rejects the four weakest (4.5, 7.4, 8.5, 8.5 km/h) — 10 of 14 survive and the losses have no deceleration worth reporting. **Averaging caveat:** `ds` spans the full 4 s history, so a sharp 2 s brake is *understated*. That errs toward under-rejecting, which is the safe direction, but it is why this must not be converted to a g figure and tuned as if it were an accelerometer threshold. Version 2.3.55 is skipped — consumed by the D2 deep-sleep diagnostic (`2.3.55-diag1`, branch `diag/deepsleep-fast`, staging only, never released). |
| **2.3.54** | **Duration back to 80 ms; harsh events with no speed signature are now rejected.** Settles `INT2_DURATION` permanently: it counts *consecutive* samples above threshold and one dip resets it, which is hostile to a signal dithered by the high-pass filter, so there is no window that keeps braking while rejecting jolts (80 ms detects both, 320 ms and 480 ms detect neither — see the status section). 80 ms stays because it is the only value proven to detect braking. The discrimination moves into `HarshEventDetected()`, which used to fall through to `hardCornering` for anything without a speed change — making cornering the catch-all, and exactly how a pothole at steady 70 km/h became an alarm. An event with no speed signature is now dropped. Chosen over counting several interrupts in a window because the latch collapses every crossing between two polls into one `hraw`, so at ~10 polls/minute a 2 s brake and a 100 ms jolt are indistinguishable — and fixing *that* needs a faster poll, which is what caused the btController livelock. New `hnod` attribute counts the rejections; `hraw` = `hnod` + `hcnt` + holdoff/min-speed rejects. **Result: 62 % of events rejected and real 0.46 g brakes delivered as alarms for the first time.** Known limitation, deliberate: pure cornering at constant speed has no speed signature and is rejected too — that needs FIFO capture or heading change. |
| **2.3.53** | Harsh duration gate 480 ms → 320 ms, on the pre-registered rule from 2.3.50. **The prediction fired and the lever is now closed:** the 2026-08-12 drive gave `hraw=0` and `hcnt=0` across 382 moving samples with `ipoll=454`, and it contained a **0.391 g** stop (68.9 → 27.5 km/h in 3 s) plus 0.289 g. `i2src` never showed `IA` set at all. Combined with 480 ms ignoring a 0.518 g stop, three measured values prove duration cannot separate the two populations — see 2.3.54 for the replacement approach. Records a stopping rule so the register is not bisected a fourth time. |
| **2.3.52** | ⚠️ **HELD BACK FROM PRODUCTION — blocked by D2** (deep sleep is a one-way trip; making the gate easier to reach is worse than the bug it fixes). **Deep sleep could never engage — any single motion interrupt discarded the 48 hr timer.** `ParkLongTimer` was zeroed on every INT1 assertion (`main.c:3287`), so the gate required 48 *consecutive* hours with not one interrupt. Two bench units, undisturbed on a garage PSU, ran **55 hr** without sleeping; `ipoll` (which counts that same block, so it counts the resets) reached **47 and 19** — one stray assertion every ~70 and ~170 minutes from ordinary ambient vibration. Unreachable anywhere a vehicle actually parks, and invisible until a battery is flat. The timer now resets only on **confirmed** motion: `MOTION_CONFIRM_COUNT` (3) distinct assertions within `MOTION_CONFIRM_WINDOW_S` (120 s), with a de-dupe on `SystemTimer` because the INT1 block runs on every main-loop pass while the latched pin is still low and one physical event would otherwise be counted many times, defeating the confirmation entirely. The thresholds come from the measured two-orders-of-magnitude gap between the populations: bench noise is 0.35–0.85 assertions/hour and uncorrelated, active driving is ~10 per *minute*. `MotionTimer` is deliberately left undebounced (it drives the 30 s/5 min cadence, where reacting immediately is correct), and the 64 mg wake threshold is untouched — motion wake is what *ends* deep sleep, so blunting the sensor risks a unit sleeping through being driven away. Debouncing the consumer is safe; desensitising the sensor is not. Adds `plt`/`pltr`, which make the gate observable for the first time — until now "not yet 48 hr stationary" and "the timer keeps resetting" were indistinguishable from server data, which is exactly why this went unnoticed for weeks. |
| **2.3.51** | **Staging OTA channel — per-device targeting at last.** `version.json` is global, but *which* manifest a device reads is now per-device, and Traccar commands are addressed to one device: `Moved V_OTA_TEST` moves this unit to `/staging/`, `Moved V_OTA` brings it home and resets the channel. Risky experiments no longer force a live trial on the van, and no longer need a USB flash. The channel persists in NVS (`ota_ch`) — essential, not convenient: without it the staging build reboots, the boot check reads production, sees a different version and pulls straight back, an OTA ping-pong burning cellular data every cycle, so `ota_channel_load()` runs before `CheckAndApplyOTA()`. Note `V_OTA` is a *substring* of `V_OTA_TEST` and the command matcher is a substring search, so the handlers are ordered longest-first with `else if`; reversed, `V_OTA_TEST` would be silently handled as a plain `V_OTA`. If staging is unreachable the device stays on staging and skips the update rather than auto-reverting, since auto-revert reintroduces the ping-pong; recovery is `V_OTA` or `V_RESET`, neither of which needs the staging host up. New `otach=1` attribute, emitted **only** on staging, so production pings are byte-identical and a forgotten test unit is visible. Validated end to end on unit 2 with a `2.3.51-rc1` build identical bar the version string: out to staging with `otach=1` while the other two units stayed put, then home with `otach` cleared. |
| **2.3.50** | **Harsh duration gate 80 ms → 480 ms — the threshold was the wrong lever.** 2.3.49's `ZHIE` drop was confirmed over five drives (gated rate down 3×, `hcnt` per moving ping 0.57 → 0.19), but cross-referencing all 18 surviving events against the GPS speed trace showed the residue is still road noise, now arriving through X/Y: **0 events ≥ 0.25 g, 5 at 0.15–0.24 g, 11 below 0.15 g**, including four at highway speed with essentially no deceleration (0.036 g, 0.040 g twice, 0.059 g). The sensor is not wrong there — those are genuine >0.25 g *transients* that a 2–3 s GPS average cannot see. Raising `HARSH_EVENT_G` would remove the real events first, because the five plausible brakes measure 0.15–0.24 g GPS-averaged, at or *below* the threshold the noise is clearing: the populations overlap in amplitude. They separate on duration — braking is sustained 1–3 s, a jolt is under 100 ms — and the gate had been doing nothing anyway, since 100 ms truncated to 2 ticks (80 ms) at 25 Hz. 480 ms is 12 ticks exactly (hence not 500), well inside the 2.5 s HPF time constant so sustained braking still reaches the comparator. Threshold and ODR unchanged — one variable. Expected `lisreg`: `33,37,B7,60,08,0A,0A,0F,0C`. If `hraw` drops back toward 0 on real braking the gate is too long: step down to 320 ms (8 ticks) before touching anything else. |
| **2.3.49** | **`ZHIE` dropped from `INT2_CFG` (`0x2A` → `0x0A`) — vertical road jolts were swamping the braking signal.** 2.3.48 confirmed the filter was the mechanism: on the 2026-08-07 test drive three deliberate stops at **0.294 g, 0.252 g and 0.338 g** each produced an `hraw` increment at the next poll — on the *same* 0.25 g threshold that had given `hraw=0` for a 0.409 g stop at 100 Hz ODR. Only the cutoff changed. But the lower cutoff also passes road-surface vertical jolts: `hraw` ran **3 → 23** and `hcnt` **0 → 12** across eight minutes of ordinary commuting, including two increments during steady 50–58 km/h cruise at ±0.02 g longitudinal. Twelve alarms in eight minutes is unusable. Every `i2src` read with `IA` actually set involved Z — `0x65` (ZH), `0x59` (ZL), `0x56` (ZL), four for four, matching the bump-triggered alarm on 2.3.47. Braking and cornering are horizontal, so Z contributes noise and no signal. Threshold and ODR unchanged — one variable. The value moved into `HARSH_INT2_CFG` in `SCI.h` alongside the other generator-2 constants. **Assumes Z is the vertical axis** — inferred from bumps firing it, not from a known mounting orientation; if `hraw` returns to 0 on braking, revert to `0x2A` before changing anything else. Expected `lisreg`: `33,37,B7,60,08,0A,0A,0F,02`. |
| **2.3.48** | **The high-pass filter was cancelling the braking signal.** 2.3.47 proved the threshold was never the blocker: a measured **0.409 g** stop (51.3 → 22.4 km/h in 2 s) and a 0.364 g stop both left `hraw` at 0, with `ipoll` climbing eight polls past each event and the interrupt *latched* — so the sensor genuinely never triggered. A 0.409 g brake cannot fail a 0.25 g threshold unless something attenuates the signal before the comparator. `HPCF=11` is ODR/400, so the old 100 Hz ODR put the cutoff at 0.25 Hz — a 0.64 s time constant, the same order as a real brake. **Braking is a ramp, not a step:** a step would pass at full amplitude and decay, but a ~1 s ramp to 0.4 g reaches the comparator at roughly 0.15–0.2 g. The corroborating detail is what *did* fire minutes later — `i2src` 0x65/0x56, ZH and ZL set, no deceleration anywhere near it. A vertical bump is high-frequency and passes the filter intact, which is also the pothole false-positive the per-axis design was always going to risk. The filter can't be removed (without it static gravity holds generator 2 permanently triggered), so the lever is ODR, which the cutoff scales with: **100 Hz → 25 Hz** puts the corner at 0.0625 Hz (τ = 2.5 s) and that same ramp presents ~0.3 g. `CTRL_REG1` now derives from `LIS3DH_ODR_HZ` instead of a bare `0x57`, so ODR can't drift from the `HARSH_*` arithmetic that depends on it. Threshold unchanged at 0.25 g — one variable. `INT2_DURATION` truncates to 2 ticks = **80 ms** at 25 Hz (100 ms is 2.5 ticks), the more permissive direction, so it can't manufacture a positive result. **Falsifiable:** if the HPF is the mechanism, `hraw` fires on braking at the same 0.25 g; if it's still 0, the filter is not it — stop adjusting it. Expected `lisreg`: `33,37,B7,60,08,0A,2A,0F,02`. |
| **2.3.47** | **Track resolution: the "1-second" recording was actually ~4.6 s.** `TrackSampleTick()` runs at 1 Hz but discards ticks where the GPS second hasn't advanced, and that only advances when `XCheckGPS()` polls the modem — once per main-loop pass. So the main-loop period *is* the sampling interval, and `CheckNetwork()` was being called every pass while spending 1.5 s in `osDelay()` (1000 ms on entry, 500 ms after the `+CREG` reply). Measured from Traccar's own sample timestamps on the van: 4–5 s within a burst, 11–21 s across ping boundaries. Registration state doesn't change second to second, so the check is now throttled to 30 s while GNSS is live and left at every-pass when it isn't (a network fault is the likely cause then, and track resolution doesn't matter). Safe to skip because the call is side-effect free — it returns registration status and `UpdateNetwork()` only sets the status LED. **Harsh threshold 0.40 g → 0.25 g** (`INT2_THS` `0x19` → `0x0F`): the hard-brake run finally happened (2026-08-06, three deliberate stops) and gave `hraw=0` with `ipoll` climbing 191 → 233, so the sensor was polled throughout and never triggered. Traccar positions put those stops at 0.227 g, 0.223 g and 0.199 g — 4 s averages, so true peaks are higher, but real braking on this vehicle sits in the 0.2–0.3 g band, not above 0.4 g. 0.25 g is also close to Verizon Connect's 0.265 g braking figure. Duration stays at 100 ms — still one variable at a time. Adds `tqd` (track queue depth at ping time, read *before* the drain) and `tdrp` (samples lost to ring wrap), since raising the sample rate is only safe if delivery keeps up — every buffered sample costs its own `HTTPPARA`+`HTTPACTION` round trip. |
| **2.3.46** | Harsh-driving duration `300 ms` → `100 ms` (`INT2_DURATION` `0x1E` → `0x0A`); threshold left at 0.40 g. One variable, deliberately. 300 ms at 100 Hz ODR means **30 consecutive samples all above threshold**, and a single sample dipping below resets the counter — real braking ramps, and road vibration dithers the signal through the high-pass filter, so an unbroken plateau may essentially never occur. Matches the field evidence: a genuine test drive (hard braking 76.9 → 7.6 km/h, hard L/R/L cornering, judder bar at 50 km/h) gave `hraw=0` while `ipoll` climbed 92 → 132, i.e. the sensor was polled throughout and simply never triggered. 0.40 g stays because it is defensible (DOT harsh = 0.45 g, Verizon Connect uses 0.265 g); if `hraw` is still 0 after a hard-brake run, lower `HARSH_EVENT_G` next — still one variable at a time. |
| **2.3.45** | A dead-silent modem now clears the parked gate, so GNSS recovery actually runs. 2.3.42 made the no-reply failure *visible* but not self-healing: `gnss_frozen` is set only by the frozen-clock detector, which needs a well-formed sentence to have parsed, so a no-reply left it false and `gnss_active = ign_on \|\| (MotionTimer <= 60) \|\| gnss_frozen` stayed false on a parked vehicle — the window was re-anchored on every pass and escalation could never fire. Field case: van on `gpsnr=7938`, `gpsage=34580` (9.6 h with no fix), `gpsrec=0` — it reported the CLBS cell position instead, about two blocks from where it actually was. A no-reply run exceeding `GPS_FROZEN_SECONDS` now bypasses the gate exactly as a frozen clock does; time-based, not count-based, so it does not depend on how often `XCheckGPS` runs. The parked gate itself stays — 2.3.34 added it after 19 useless modem reinits in one night. |
| **2.3.44** | Stops `InitAccelerometer()` destroying harsh events before anything counts them. It ended with a blind `I2C_RdReg(REG_INT2_SRC); // clear latch` and runs every 60 s, while the poll that would record the event ran — measured with the 2.3.43 counters on the driven van — about **once every 11 minutes** (`ipoll=98` over 17.9 h). The latch was destroyed ~11× for every time anything looked at it. Now reads `INT2_SRC` *before* rewriting the generator and acts on the result instead of discarding it; reading is what clears the latch, so the read had to move ahead of the rewrite. Also settled why `qpoll` measured exactly 0 on every unit: that site is in `XUDP_Request`, the legacy UDP path — OsmAnd pings go through `XHTTP_Request`, which never touches the accelerometer at all, so 2.3.39's "XHTTP_Request's wait loop no longer clears INT2_SRC" fix had been applied to code that never runs. Corrected the `CTRL_REG5` comment that justified latching on "the ~1 Hz poll in StartMainTask" — wrong by a factor of ~650. Adds `apoll`. |
| **2.3.43** | Diagnostic: counts how often the two sites that read `INT2_SRC` actually execute. `i2src` has read a constant `0x15` on every ping from both units, which cannot distinguish "the poll runs constantly and generator 2 never fires" from "the poll ran once early and `0x15` is a fossil" — `last_int2_src` starts at `0xFF`, so a constant value proves only that it was updated *at least once*. `ipoll` counts the StartMainTask INT1 handler, which runs only when the pin is asserted; `qpoll` counts the ping-wait-loop read, which runs every ~30 s while driving regardless of the pin and so acts as the control. `qpoll` climbing while `ipoll` stays frozen would mean nothing is asserting or servicing INT1 at all — which would also mean motion wake is broken, and would make a threshold bisection meaningless. Pure telemetry, no behaviour change, so it runs on the live van and uses real driving rather than a bench shake. |
| **2.3.42** | A dead GNSS can no longer report itself as a healthy one. `XCheckGPS()` defaulted `GPSStatus` to `'A'` and demoted it only on the literal strings `"+CGPSINFO: ,,"` or `"ERROR"`, then did all its work inside `if (pToken != NULL)`. A reply containing no `+CGPSINFO:` line at all — an AT timeout, or a bare `OK` — matched neither string and skipped the entire body, so it was read as a valid fix *and* bypassed every safety net at once: the 2.3.40 frozen-clock check never evaluated, the no-fix path never zeroed `fLat`/`fLong` (file-scope, so they kept their last values), and `GNSSRecover()` was never called. `live_fix` stayed true, so `gpsage`/`gpsrec` were never emitted either. Field case: the van froze at 2026-08-01 08:24:49 NZST and resent that one fix for 50 h across 440 pings, through two drives, with no attribute anywhere showing a fault — Traccar dropped every one as a duplicate, so no trip was recorded. Unit -5783 froze the same way on 07-31; other AT commands (`CBC`, `CSQ`, `CLBS`) kept working throughout, which is what isolates it to the CGPSINFO path. Fixes: `'V'` is now the default and a fix must be positively proven; `SendATCommand`'s return is checked, with `3` (timeout) counting as no-fix; and the frozen-clock check, no-fix zeroing and recovery ladder all moved out of the `pToken` guard so they run on every poll. Adds `gpsnr`, counting polls with no parseable reply, so this variant is visible in telemetry rather than only in the source. |
| **2.3.41** | Diagnostic: reads the LIS3DH's own configuration back at ping time (`lisreg`, `i2src`) rather than trusting that the writes took, after three inferred-mechanism fixes for "harsh detection never fires" all failed. Field result — registers read back exactly as intended (`33,57,B7,60,08,0A,2A,19,1E`), so the sensor is correctly configured and stays configured, retiring that whole class of theory. |
| **2.3.40** | Detects a frozen GPS clock — a second stale-fix mode distinct from the empty response 2.3.33 handles: the modem keeps answering with a well-formed sentence whose contents never change, so `GPSStatus` reads `'A'` and nothing downstream can tell. A GPS clock that has not advanced for 120 s now demotes the fix to `'V'`. `gnss_frozen` also bypasses the parked gate, since a stopped clock is unambiguous where "no fix" is not — waiting for the next drive would carry the fault straight through it. Date/time parsing moved ahead of the validity check so the test can run before anything acts on the fix. |
| **2.3.39** | Harsh driving finally able to fire: `InitAccelerometer()` was destroying the sensor's own detection state roughly once a second. Motion wake (64 mg, `DURATION=0`) fires continuously while driving, and the main loop re-initialised the accelerometer on every assertion — each call read `REFERENCE` (resetting the high-pass filter, since `HPM=00` means "reset on REFERENCE read"), rewrote `INT2_CFG` (resetting the 300 ms duration counter), and swept registers 0x07–0x3F, which reads `INT2_SRC` and so cleared any harsh event that *had* fired before anyone saw it. Generator 2 needs 30 consecutive above-threshold samples and never got near it. Fixes: `HPM` → `10`; explicit `REFERENCE` read removed; the register sweep deleted entirely (dead code — `ReadBuff` was written and discarded — costing 57 I2C reads per call); full re-init throttled to once per 60 s, since clearing the latch is the part that matters; and `XHTTP_Request`'s wait loop no longer clears `INT2_SRC` without checking it (it ran every 30 s while driving). |
| **2.3.38** | Harsh-driving high-pass cutoff lowered (`CTRL_REG2` `0x07` → `0x37`): `HPCF` was left at `00`, the *highest* cutoff (~2 Hz at 100 Hz ODR), which attenuated the sustained sub-second forces being measured. Did not fix detection on its own — see 2.3.39 — but the setting was wrong regardless. Adds `hraw`, counting raw sensor interrupts before any gating, so "the sensor never fired" can be distinguished from "it fired and we discarded it". |
| **2.3.37** | **Harsh driving redesigned and re-enabled.** The 20 Hz sampler task is gone; detection now runs in the LIS3DH's second interrupt generator (`INT2_THS` = 400 mg, `INT2_DURATION` = 300 ms, both derived from the `HARSH_*` macros). Both generators share the wired INT1 pin and both are latched, so the existing ~1 Hz poll can't miss an event — no new task, no periodic I2C, nothing running between events. This is what finally beat the livelock: 3+ hours over judder bars and rumble strips at 96 km/h with zero resets, where every polling build wedged within the hour. Classification (`hardBraking`/`hardAcceleration`/`hardCornering`) now uses a 1 Hz speed history that touches no hardware. `gmax`, `hstk` and `i2crec` removed (no sampler to source them); `hmin` kept, `hcnt` added. Accident severity deferred — needs FIFO capture. |
| **2.3.36** | Ignition no longer reports "engine running" for 45–60 minutes after shutdown. The old off-threshold (13.0 V) sat *below* a freshly charged battery's resting voltage (12.7–13.1 V), so surface charge had to decay for the best part of an hour before the flag cleared — field-confirmed twice on the van. Three days of telemetry separate cleanly with an empty band between (alternator 14.11–14.48 V, resting 12.67–13.10 V), so thresholds moved to 13.8 V on / 13.5 V off with asymmetric debounce (3 s on, 15 s off, so an idling engine under load doesn't flicker). Ignition transitions now force an immediate ping, sharpening Traccar's trip boundaries. |
| **2.3.35** | Harsh-driving re-enable attempt with livelock hardening — **failed, rolled back**. Stack 3072→4096, all `ESP_LOGW`/printf removed from the 20 Hz loop, I2C recovery rewritten as an SCL clock-pulse bus clear instead of driver delete/reinstall, and the accel read given its own 100 ms timeout (the general 1000 ms timeout with a 20-failure recovery threshold meant a stuck bus would panic-reboot at ~5 s before recovery could ever run). Both units still wedged within minutes, never exceeding 29 minutes uptime. **The value was the instrumentation:** `hstk` held at 3320/4096 free, `hmin` at 142 KB, `i2crec` at 0 through every wedge — disproving all three suspected causes and pointing at contention between continuous I2C traffic and the BLE controller. That result is what motivated the 2.3.37 redesign. |
| **2.3.34** | GNSS recovery no longer escalates while the vehicle is parked. 2.3.33's watchdog fired on elapsed time alone and couldn't tell "receiver wedged" from "parked where it can't see sky" — 19 recovery attempts in one night on a stationary van, roughly 6 full modem reinitialisations, all with `ignition=false`. Escalation now requires ignition on or motion within 60 s, and the no-fix window is held open while parked so the grace period restarts when the vehicle moves off rather than firing a power-cycle at ignition-on. Adds geometric backoff (×2 per failed recovery, ceiling 8) reset on any fix. |
| **2.3.33** | **GNSS watchdog, and stale fixes no longer replayed as live.** Both units stopped producing fixes mid-session on 25/26 July and never recovered — `AT+CGNSSPWR=1` was only ever issued from `InitGSM()`, so with the network healthy nothing triggered a restart and the devices ran four days replaying one stale position. Worse, the failure was invisible: `XCheckGPS` cleared only the vestigial `Lat`/`Long` char arrays on fix loss, while `fLat`/`fLong` and the GPS clock kept their last values (`sscanf` against an empty `+CGPSINFO` converts nothing), and `LoadGPSTimeStamp` copied them with no status check — so a dead receiver shipped a stale coordinate flagged as a live fix with a frozen timestamp. Unit −5783 resent its 25 July 03:58 UTC fix for two days straight. Now: everything is zeroed on fix loss, and an escalating recovery ladder (`CGNSSPWR` cycle → `CGPSCOLD` → modem reinit) runs with cold/warm windows sized from a measured ~586 s time-to-first-fix. New `gpsage`/`gpsrec` attributes make a dead GNSS visible server-side. |
| **2.3.32** | Stability release after repeated lockups on every build carrying `HarshDriveTask` (2.3.28–2.3.31, wedges within hours even stationary on the bench). Serial capture of a live wedge showed the true failure mode: the BLE `btController` task livelocks at high priority and starves every other task on the single-core ESP32-C3 — main task (no pings), `HarshDrive`, and `IDLE` — with the task watchdog printing but never rebooting. Two changes: (1) `CONFIG_ESP_TASK_WDT_PANIC=y` — a task-watchdog trip now panics and reboots (~60 s outage) instead of leaving the device wedged for hours; (2) harsh driving detection compiled out behind `ENABLE_HARSH_DRIVING` (off by default) pending root-cause hardening — earlier "modem hang" and "I2C lockup" diagnoses were symptoms of the same CPU starvation. All 2.3.30/2.3.31 recovery and timeout fixes retained. |
| **2.3.31** | AT command timeout fix: `LoopTimeout1` (the modem-response timeout counter) is only incremented in the main loop timer block and is never updated while the HTTP send function's `while(1)` wait loops are running — making all timeouts inside `XHTTP_Request` and `SendATCommand` silently broken. When the modem stopped responding (typically `AT+CGACT=1,1` hanging on a dropped PDP context), the device would lock up indefinitely with no pings but no TWDT (HarshDriveTask kept feeding the watchdog), appearing dead for hours until HarshDriveTask itself hit an I2C fault. Fixed by replacing `LoopTimeout1 > N` in all `SendATCommand` and `XHTTP_Request` wait loops with a direct `xTaskGetTickCount() >= deadline` comparison using a local `TickType_t` variable, making timeouts wall-clock accurate regardless of task context. |
| **2.3.30** | Network-loss self-recovery: the 300 s parked-interval check now counts consecutive `CheckNetwork()` failures; after 3 in a row (15 min of no cellular registration) the device calls `esp_restart()`. Prevents the device silently locking up in the modem-init state for hours when the modem enters `+CREG: 0,0` (not registered, not searching) during normal operation — previously only the boot-time `InitGSM` path had this reset, leaving runtime outages unrecoverable without a power cycle. |
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
