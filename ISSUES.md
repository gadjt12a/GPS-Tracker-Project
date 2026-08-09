# VALTRACK-V4 Issues Register

Opened 2026-07-28. Severity: **P1** = blocks current goal, **P2** = real defect worth fixing,
**P3** = latent / cosmetic. Status: OPEN / FIXED / WONTFIX.

---

## A. GPS / GNSS (current priority)

### G1 — Watchdog escalates while parked, causing repeated modem reinits
**P1 · FIXED in 2.3.34 (awaiting field confirmation) · introduced in 2.3.33**

**Fix applied:** escalation now requires `ign_on || MotionTimer <= 60`. While parked the
window is held anchored to now, so the grace period restarts when the vehicle moves off
rather than firing a power-cycle instantly at ignition-on. Added geometric backoff
(`gnss_backoff_mult`, doubling to a ceiling of 8 = 80 min) reset on any fix, so a genuinely
dead receiver is not power-cycled every 10 minutes indefinitely.

**Confirm by:** `gpsrec` should stay at 0 overnight on a parked vehicle. Any increment
should now coincide with `ignition=true`.


`GNSSRecover()` fires on elapsed time alone. It has no idea whether a missing fix means
"receiver wedged" or "vehicle parked where it cannot see sky".

Field data, van, night of 2026-07-27:

| Time | ignition | gpsage | gpsrec |
|---|---|---|---|
| 17:11 | false | 2078 | 4 |
| 18:01 | false | 5115 | 9 |
| 18:51 | false | 8099 | 13 |
| 19:30 | false | 10422 | 17 |
| 19:48 | false | 11539 | 19 |

19 recovery attempts across ~3.3 h, all while stationary. The ladder reaches stage 3
(`DisableGSM()` + `InitGSM()`) roughly every third attempt, so that is ~6 full modem
reinitialisations overnight on a parked vehicle — wasted battery, needless LTE
re-registration, and no benefit.

**Fix:** gate escalation on ignition or recent motion. A parked vehicle with no fix is not
a fault worth acting on. Consider also an exponential backoff so a genuinely dead receiver
does not retry forever at a fixed 10-minute cadence.

---

### G2 — Root cause of the original multi-day GNSS wedge still unconfirmed
**P1 · OPEN**

2026-07-25/26 both units stopped producing fixes and never recovered, including while
driving (van logged 258/285/107 `ignition=true` pings on 25/26/27 July with **zero** live
fixes). A V_RESET restored fixes within ~586 s, so it is software-recoverable.

What is *not* yet known: whether the overnight outage in G1 is the same fault recurring, or
simply a weak antenna under the carport roof. Evidence pulls both ways:

- *Against antenna:* the original outage persisted while driving in open sky.
- *For antenna:* unit 2 (bench, same property) holds a fix continuously with `gpsrec=0`,
  while the van under cover cannot.

**Next step:** now that G1 data separates parked from driving, check whether the van ever
loses a fix *while ignition is true*. If it never does, this is sky view and the fix is
G1 (stop escalating when parked). If it does, there is a real hardware or modem fault.

---

### G4 — Fix loss mid-drive publishes speed 0.0 as a real sample, faking a 1.16g stop
**P2 · OPEN · found 2026-08-09 in 2.3.49 field data**

The 2.3.33 fix zeroes `fLat`/`fLong`/`fSpeed` when a fix is lost, which correctly stopped
stale fixes being replayed as live. But a **track sample** recorded during that window is
still delivered to Traccar carrying `speed=0.0`, indistinguishable from a genuine stop.

Van, 2026-08-07 03:08 UTC, at open-road speed:

| GPS time (UTC) | speed | note |
|---|---|---|
| 03:07:56 | 82.8 km/h | last sample before a **10 s gap** (norm is 2-3 s on 2.3.47+) |
| 03:08:06 | 82.8 km/h | |
| 03:08:09 | 82.1 km/h | |
| 03:08:11 | **0.0 km/h** | fix lost - zeroed, not decelerated |

A vehicle does not go 82 km/h to *exactly* 0.0 in 2 s. Computing deceleration across that
pair yields **1.163 g**, which is above `HARSH_ACCIDENT_G` (1.85 g is the accident gate, so
this particular value does not trip it - but the margin is uncomfortable and a shorter gap
would produce a larger figure).

**Two distinct harms:**
1. **Corrupts any GPS-derived analysis.** This artefact appeared twice in the 2.3.50 harsh
   cross-reference and initially read as two genuine hard brakes. Anyone repeating that
   analysis will hit it again.
2. **Pollutes Traccar.** A phantom stop-from-speed inside a trip affects trip segmentation
   and any server-side harsh/accident detection keyed on speed deltas.

**Fix:** do not emit a track sample when the fix is not live. The zeroing is correct as a
*safety* measure (better than replaying a stale position) but zero is being published as
data. Either drop the sample, or omit `speed=` so Traccar does not infer a deceleration.
Note `LoadGPSTimeStamp` (C4) is unguarded on the same globals - likely the same root path.

**Secondary question worth answering at the same time:** why a ~10 s GPS gap at 82 km/h on
2.3.49 at all, when the measured norm is 2-3 s. Could be an ordinary fix dropout, or the
sampling stall that G2 is about. Do not assume; check `gpsnr`/`gpsage` around the event.

---

### G3 — GNSS config not re-applied after a power-cycle
**P3 · OPEN**

`GNSSRecover()` stage 1 issues `AT+CGNSSPWR=0` then `=1`, but does not re-send
`AT+CGNSSMODE=15` / `AT+CGPSXE=1`. Harmless today because both return `ERROR` on the
shipped A7672G firmware (see K2), but this becomes a silent constellation downgrade the
moment Valetron ships a modem update.

---

## B. Known / previously documented

### K1 — btController livelock with `HarshDriveTask`
**P1 · OPEN · 2.3.35 hardening FAILED · rolled back to 2.3.34 on 2026-07-28**

**Result: all three suspected causes are disproven.**

2.3.35 wedged both units within minutes of updating. Neither exceeded 29 minutes uptime;
unit -5783 reset 8 times in 45 minutes, twice inside one minute. TWDT panic contained each
wedge to ~60s instead of the multi-hour hangs of 2.3.28-2.3.31, but tracking collapsed: the
van recorded **16 distinct positions on 2.3.35 against 176 on 2.3.34**, because every reboot
discards the track buffer and restarts GPS acquisition.

The instrumentation held steady through every single wedge:

| Metric | Value throughout | Rules out |
|---|---|---|
| `hstk` | 3320 of 4096 free (~776 used) | Stack overflow |
| `hmin` | 142,272 bytes | Heap exhaustion |
| `i2crec` | 0 | Stuck I2C bus - recovery never even ran |

So it is **not** the stack, **not** heap churn from the I2C driver delete/reinstall, and
**not** the bus sticking. The entire 2.3.33-era hardening plan targeted the wrong things.
Those were reasonable inferences from a TWDT trace, and they are now closed with data.

What remains is what the original trace literally showed: `btController` occupying the CPU
and starving everything else. The trigger appears to be running *any* continuous 20Hz I2C
sampler alongside the BLE controller on the single-core C3 - a scheduling/contention
interaction, not resource exhaustion.

**Next approach - stop polling, do not harden further.** The LIS3DH has its own interrupt
generators with configurable threshold and duration (`INT1_THS`, `INT1_DURATION`), and INT1
is already wired and already used for motion wake. Harsh-driving thresholds (0.4g sustained
300ms) map onto those registers directly, so detection can run in the sensor with no
periodic I2C traffic at all - reading a short burst only when the interrupt fires. That
removes the suspected cause instead of trying to survive it.

Trade-off: hardware thresholds act on raw axes, losing the software gravity-compensation
that makes the current detector mounting-angle agnostic. Either calibrate orientation once
at install, or use the interrupt purely as a trigger and keep the maths in the burst read.

**Cheap confirmation first (optional):** drop the sampler 20Hz -> 5Hz. If wedging stops or
slows markedly, rate-dependent contention is confirmed before committing to the rewrite.

**Testing constraint:** OTA has no per-device targeting, so any field test hits the van
(live trial) as well as unit 2. Prefer USB-flashing unit 2 for experiments.

**Original defect:**

**Hardening applied (2.3.35), harsh driving re-enabled:**
- Stack 3072 -> 4096 (suspect: overflow trampling adjacent BLE heap)
- All `ESP_LOGW`/printf removed from the 20Hz loop (stack-hungry float
  formatting, and takes the stdout lock while the BLE host runs)
- I2C recovery rewritten: SCL clock-pulse bus clear + `i2c_param_config()`
  instead of `i2c_driver_delete()`/`i2c_master_init()`/`InitAccelerometer_LIS3D()`.
  No heap churn, no interrupt re-registration next to BLE allocations.
- **New finding:** the accel burst read used the general 1000ms I2C timeout while
  TWDT is 5s with panic enabled, and recovery only triggered after 20 consecutive
  failures. A genuinely stuck bus would therefore panic-reboot at ~5s, long before
  recovery was ever attempted - the recovery path could never run in the exact
  situation it existed for. Now a dedicated 100ms timeout
  (`I2C_ACCEL_TIMEOUT_MS`) with a 5-failure threshold: 500ms worst case, well
  inside the watchdog.
- Instrumentation on every ping: `hmin` (min free heap since boot), `hstk`
  (HarshDriveTask stack bytes remaining), `i2crec` (bus clears since boot)

**Confirm by:** `hstk` should stay comfortably above 0 and flat; `hmin` should not
slide downward over hours; `i2crec` should stay 0 or near it. A wedge now
panic-reboots (uptime resets), and the last ping before it says which suspect was
real.

**Original defect:**

Every build carrying `HarshDriveTask` (2.3.28–2.3.31) wedged within hours, including
stationary on the bench. TWDT capture showed `CPU 0: btController` starving all other
tasks. Harsh driving is compiled out since 2.3.32 (`ENABLE_HARSH_DRIVING` undefined in
SCI.h:195), and TWDT panic-reboot limits any recurrence to ~60 s.

Trigger unproven. Suspects: heap/interrupt churn from `accel_i2c_recover()`
(i2c_driver_delete/reinstall next to BLE allocations), or `HarshDriveTask` overflowing its
3072-byte stack via `ESP_LOGW` printf with floats.

Hardening plan (now **2.3.34**, renumbered — 2.3.33 was spent on the GNSS fix): stack
3072→4096; remove ESP_LOGW/printf from the 20 Hz loop; replace I2C delete/reinstall with an
SCL clock-pulse bus clear; add free-heap low-water, stack high-water and I2C-recovery-count
attributes. One change at a time.

### K2 — AGPS/XTRA and multi-constellation commands rejected by modem
**P3 · WONTFIX (vendor)** — `AT+CGNSSMODE=15` and `AT+CGPSXE=1` return `ERROR`.

### K3 — Full framework recompile fails
**P2 · OPEN (worked around)** — mbedtls 4.x in ESP-IDF 6.0.1, `mbedtls/ssl.h:1573`
`unknown type name 'mbedtls_x509_crt'`. Worked around via `.ninja_log` reconstruction.

### K4 — cmake cache picks up the wrong GCC
**P2 · OPEN (worked around)** — manual cmake runs grab local Espressif GCC 14.2.0, which
lacks `-mtune=esp-base`. Fix documented in CLAUDE.md.

---

## D. Power management

### D1 — Deep sleep can never engage: any single motion interrupt resets the 48 h counter
**P2 · CONFIRMED 2026-08-09 on two undisturbed bench units · mechanism located in code**

**The 48 h deep-sleep gate is effectively unreachable in any environment with occasional
vibration.** This is a design fragility, not a tuning problem.

`main.c:3287-3291`, on every pass where the INT1 pin reads asserted:

```c
if(INT1 == 0)
{
    MotionTimer=0;
    ParkLongTimer = 0;      // <-- 48 h counter reset to zero by ONE interrupt
```

`ParkLongTimer` increments once per second (`main.c:3313`) and `DeepSleep()` requires
`ParkLongTimer >= PARK_LONG_SECONDS` (`main.c:6923`). So deep sleep needs **48 consecutive
hours with not one INT1 assertion**. A single 64 mg blip anywhere in that window restarts
the count from zero.

**Field measurement (2.3.49, units 2 and 3, sitting on a bench PSU in a garage, undisturbed
by the owner for the whole window):**

| Unit | Boot duration | uptime reached | `ipoll` | implied resets |
|---|---|---|---|---|
| -5783 | 55.0 h wall | 198,167 s (55.0 h) | 0 -> **47** | ~0.85 / h |
| -5742 | 55.0 h wall | 198,109 s (55.0 h) | 0 -> **19** | ~0.35 / h |

`ipoll` counts executions of that same `if(INT1 == 0)` block, so it *is* the reset count.
Neither unit deep-slept, and at those rates neither ever could: the longest possible gap
between resets is far under 48 h. `hraw=0` on both, so every one of those assertions came
from generator 1 (motion wake), not the harsh generator.

**These are almost certainly genuine environmental vibration** - a garage has doors, vehicles
and footfall, and 64 mg is sensitive. That is the point: the feature is specified for a
vehicle parked for days, which is exactly where stray vibration is guaranteed.

**Not yet attributed to 2.3.48.** Suggestive but not conclusive: 2.3.48 ran 13.3 h with
`ipoll=0` on both units, where the 2.3.49 rates predict ~11 and ~4.6. Against that, 2.3.49
changed only `INT2_CFG` (generator **2**), which should not affect motion wake at all. So
the HPF-cutoff theory is unproven and the honest reading is that the rate is not constant.
**Do not assume 2.3.48 caused this** - the gate is fragile either way.

**Second, unverified gate:** `DeepSleep()` also requires
`Params.Fields.WorkingMode[0] == 'T' || 'H'` (`main.c:6921`). The bench units' `WorkingMode`
has not been checked. If it is neither, deep sleep never runs regardless of the timer and
everything above is moot. **Check this first - it is one BLE config read and it could make
the rest of this issue irrelevant.**

**Fix direction (needs a decision, not just a patch):** one interrupt should not discard 48 h
of accumulated stillness. Options, cheapest first:
1. **Debounce the reset** - require N assertions within a window, or a sustained
   `MotionTimer` run, before zeroing `ParkLongTimer`. Isolated blips then decay instead of
   resetting.
2. **Decay instead of reset** - subtract a penalty rather than zeroing, so real driving still
   clears it quickly but a door slam costs minutes.
3. Raise the generator-1 threshold above 64 mg. **Least preferred** - motion wake is what
   *ends* deep sleep, and desensitising it risks a unit that sleeps through being driven away.

**Instrument first regardless:** add a `plt` (ParkLongTimer) attribute. Right now
"not yet 48 h stationary" and "timer keeps resetting" are indistinguishable from server data,
which is why this sat as a suspicion for so long.

---

## C. Defects found in review 2026-07-28

### C11 — Ignition reports "running" for ~50 minutes after shutdown
**P2 · FIXED in 2.3.36 (awaiting field confirmation)**

`PowerSenseTick` used on `>13.3 V` / off `<13.0 V`. A freshly charged battery holds surface
charge and rests at 13.0-13.2 V, i.e. **above** the OFF threshold, so after a drive the
voltage had to decay for the best part of an hour before the flag cleared.

Field evidence, van 2026-07-27:

| Time | vbat | ignition |
|---|---|---|
| 19:53 | 14.39 | true (engine on) |
| 20:37 | 13.015 | false - **44 min later** |
| 21:41 | 14.455 | true |
| 22:40 | 12.98 | false - **59 min later** |

Three days of telemetry give a clean separation with an empty band between:

| State | vbat |
|---|---|
| Alternator running | 14.11 - 14.48 V |
| Engine off, resting | 12.67 - 13.10 V |
| 13.2 - 14.0 | essentially empty |

**Fix:** thresholds moved into the empty band - `IGNITION_ON_VOLTS` 13.8,
`IGNITION_OFF_VOLTS` 13.5 (SCI.h, with the measured data recorded alongside). Debounce is
now asymmetric: 3 s on, 15 s off, so an idling engine sagging under headlights/AC does not
flicker the state while still clearing in seconds rather than an hour. Ignition transitions
also set `force_ping_now`, so Traccar sees the edge immediately instead of up to 5 minutes
later - trip start/end boundaries were being shifted by the parked ping interval.

**Note:** this makes the alternator-output measurement in the roadmap unnecessary. The
running voltage (14.11-14.48 V) is already established from field data across hundreds of
samples on the actual vehicle.

### C10 — Release tooling is not under version control
**P2 · FIXED 2026-07-28**

`git init` at the project root. Tracks `CLAUDE.md`, `ota-server/publish.ps1`,
`docker-compose.yml`, `nginx.conf`, `zimaboard-setup.sh` and the sync-docs skill. Excludes
the firmware repo (own remote), built `.bin` files and machine-local Claude permissions.
`.gitattributes` pins LF on files that execute on the Zimaboard, since a CRLF
`zimaboard-setup.sh` would fail with "bad interpreter". No remote configured yet - local
history only.

**Original defect:**

Only `VALTRACK-V4-ESP32-C3/` is a git repository. The project root is not, so
`ota-server/publish.ps1`, `ota-server/`, `scripts/rebuild_ninja_log.py` and `CLAUDE.md`
have no history, no backup and no way to recover a bad edit.

That is uncomfortable for `publish.ps1` in particular: it is the only thing that can push
firmware to live vehicles, and it has a documented encoding hazard (PowerShell 5.1 reads
UTF-8 without BOM as Windows-1252, so a stray em dash silently breaks parsing). A corrupted
copy today is unrecoverable.

**Fix:** `git init` at the project root, or move `ota-server/` and `scripts/` into the
firmware repo. Either gives the release path a history.

### C1 — `publish.ps1` never commits, so tags point at the previous version
**P1 · FIXED 2026-07-28 (pending a live release)**

**Fix applied:** two guards.
1. Preflight aborts if `git status --porcelain --untracked-files=no` is non-empty, before
   anything is built or written.
2. After the FW_VERSION bump, aborts if that bump actually changed `SCI.h` (meaning the
   version was never committed), printing the exact commit command and leaving the bump on
   disk so the user only has to commit and re-run.

Together these make it impossible to tag a commit that is not what was built. Both paths
tested 2026-07-28: dirty tree aborts, uncommitted version aborts, neither uploads anything.

**Original defect:**

Step 8 (publish.ps1:174-188) tags whatever `HEAD` is and pushes. With a dirty tree it
silently tags the *previous* release's commit. Hit for real today: `v2.3.33` initially
pointed at `50ba33e` (the 2.3.32 commit) while the published binary was 2.3.33. Corrected
manually to `b364723`.

This silently corrupts the documented rollback path — "Rebuilding an older version" in
CLAUDE.md assumes `git checkout vX.Y.Z` restores matching source.

**Fix:** make `publish.ps1` abort on a dirty working tree. Preferred over auto-committing —
failing loudly is safer than committing on the user's behalf.

### C9 — `publish.ps1` cannot build after a git commit
**P1 · FIXED 2026-07-28**

**Fix applied:** `publish.ps1` now sets `IDF_PATH`, `ESP_ROM_ELF_DIR` and prepends the
PlatformIO toolchain, the Espressif ninja directory and the ESP-IDF python directory to
`PATH` before invoking ninja, so the cmake regeneration succeeds unattended.

Note on ninja: `CMAKE_MAKE_PROGRAM` in `CMakeCache.txt` is the Espressif ninja **1.12.1**,
while PlatformIO ships **1.9.0**. The script deliberately puts only the 1.12.1 directory on
`PATH`, matching cmake and the hash seed documented for `rebuild_ninja_log.py`. Do not add
PlatformIO's `tool-ninja`.

**Do not run `publish.ps1` with `2>&1`.** The script sets `$ErrorActionPreference = "Stop"`,
and in PowerShell 5.1 redirecting a native command's stderr wraps each line in an
ErrorRecord, turning harmless output into a terminating error. ESP-IDF's version detection
prints `fatal: not a git repository` to stderr during cmake regeneration (it probes
`IDF_PATH`, which PlatformIO ships as a tarball rather than a git checkout). Harmless
unredirected; fatal under `2>&1`. Cost 15 minutes chasing a non-bug on 2026-07-28.

**Original defect:**

Committing changes `git describe`, which trips ESP-IDF's `RERUN_CMAKE` rule. `publish.ps1`
then invokes bare `ninja` (publish.ps1:115) with no ESP-IDF environment, so the cmake
regeneration fails:

```
OSError: ESP_ROM_ELF_DIR environment variable is not defined.
CMake Error at CMakeLists.txt:5 (include)
```

This bites on **every** release, because C1's fix (commit before publishing) guarantees a
fresh commit immediately before the build. Hit twice on 2026-07-28.

**Workaround used:** run `ninja` manually first with `IDF_PATH`, `ESP_ROM_ELF_DIR` and the
PlatformIO toolchain/ninja/python on `PATH`, letting cmake regenerate; then `publish.ps1`'s
ninja call is a no-op and succeeds.

**Fix:** set those three variables inside `publish.ps1` before invoking ninja.

Related, seen twice and self-resolving on retry: `ninja: error: failed recompaction:
Permission denied` on `.ninja_log` during regeneration. Probably an AV or file-handle race.
A retry cleared it both times, but if it ever sticks, `.ninja_log` is the one file that must
not be lost (see K3).

### C2 — Out-of-bounds read in the UART receive path
**P2 · OPEN**

`SCI.c:102-105` reads `Buff2[Buff2Index-3]` … `[Buff2Index-1]`. `Buff2Index` is
`unsigned short`; when it is 0, 1 or 2 the expression promotes to `int` and goes negative,
indexing before the array. Reads adjacent memory and can spuriously match the `CLIP` ring
detector, setting `AnswerCall`. Guard with `Buff2Index >= 3`.

### C3 — `at24c` EEPROM size overflows to zero
**P2 · OPEN (dead code)**

`at24c.c:64` — `dev->_bytes = 128 * EEPROM_SIZE` with `EEPROM_SIZE` 512 gives 65536, stored
in a `uint16_t` (`at24c.h:15`) → **0**. The bounds checks at `at24c.c:151,167`
(`if (data_addr > dev->_bytes) return 0;`) then reject every address above 0.

Currently harmless: `ReadRom`/`WriteRom`/`InitRom` are never called from `main/`. But the
AT24C512 driver is silently non-functional, so any future use (e.g. the Phase 15 vehicle
profile byte) will fail in a confusing way. Compiler already warns:
`-Woverflow ... changes value from '65536' to '0'`.

### C4 — `LoadGPSTimeStamp` still copies position with no validity check
**P3 · OPEN**

`main.c:1235` copies `fLat`/`fLong`/GPS clock into the packet unconditionally. 2.3.33 fixed
the *symptom* by zeroing those globals at the source in `XCheckGPS`, but the function itself
remains unguarded — any future caller that populates `fLat` without setting `GPSStatus`
reintroduces the stale-fix replay. Add a `GPSStatus == 'A'` guard for defence in depth.

### C5 — Unsafe casts hide type mismatches in GPS parsing
**P3 · OPEN**

`main.c:5571-5573` — `sscanf(GPSDate, "%2d%2d%2d", (int*)&GPSDay, ...)`. The targets *are*
`int` today (`SCI.c:49`), so this is currently correct, but the casts suppress exactly the
warning that would catch it if anyone narrowed those types. `%2d` writes 4 bytes; narrowing
to `uint8_t`/`uint16_t` would corrupt adjacent globals silently. Drop the casts.

### C6 — Torn lat/lon pair between tasks
**P3 · OPEN**

`TrackSampleTick()` runs in `StartTimerTask` and reads `fLat`/`fLong`, which `XCheckGPS()`
writes from `StartMainTask`. Individual aligned 32-bit reads are atomic on RISC-V, but the
*pair* is not — a sample can combine latitude from one fix with longitude from the next.
Bounded by the 1 s sample rate and 5 m spacing gate, so impact is small.

### C7 — Inconsistent search bound
**P3 · OPEN** — `CheckSignalStrength` uses `MapForward(Buff2, 70, ...)` while every other
caller passes `BUFF2_SIZE` (350). A `+CSQ:` response arriving past byte 70 is missed.

### C8 — Deprecated sleep API
**P3 · OPEN** — `DeepSleep.c:118` uses `esp_sleep_get_wakeup_cause()`; ESP-IDF 6.0.1 wants
`esp_sleep_get_wakeup_causes()`. Builds with a warning. `main.c:8250` already uses the new
form.

---

## Suggested order of work

1. **G1** — stop the watchdog escalating while parked. Small, and it stops the van
   reinitialising its modem all night.
2. **G2** — read a day of post-G1 data to decide: sky view, or real hardware fault.
3. **C1** — dirty-tree guard in `publish.ps1`, before the next release repeats the tag bug.
4. **K1 / 2.3.34** — harsh driving hardening. The actual project goal.
5. **C2, C3** — small correctness fixes, bundle into 2.3.34.
6. **C4-C8** — cleanup, low priority.

**Updated 2026-08-09:**

1. **D1** — costs nothing but an undisturbed bench unit, and the failure mode (a vehicle
   that never sleeps) is invisible until a battery is flat. Do this while 2.3.50 is being
   road-tested; the two do not conflict.
2. **G4** — fix before the next GPS-derived analysis, not after. It has already produced two
   false "hard brake" readings and it will do so again. Bundle with **C4**, which is the same
   unguarded-globals path.
