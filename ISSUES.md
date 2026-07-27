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

### G3 — GNSS config not re-applied after a power-cycle
**P3 · OPEN**

`GNSSRecover()` stage 1 issues `AT+CGNSSPWR=0` then `=1`, but does not re-send
`AT+CGNSSMODE=15` / `AT+CGPSXE=1`. Harmless today because both return `ERROR` on the
shipped A7672G firmware (see K2), but this becomes a silent constellation downgrade the
moment Valetron ships a modem update.

---

## B. Known / previously documented

### K1 — btController livelock with `HarshDriveTask`
**P1 · OPEN (mitigated)**

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

## C. Defects found in review 2026-07-28

### C10 — Release tooling is not under version control
**P2 · OPEN**

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
