/***************************************************************************
* File name :   SCI.h                                                      *
*                                                                          *
* Author    :   Ravi Y. Pujar                                              *
*                                                                          *
* Owner     :   Copyright (c) 2023 Valetron Systems Pvt Ltd,               *
*                all rights reserved                                       *
***************************************************************************/
#ifndef _SCI_H
#define _SCI_H
#include <stdint.h>
#define VALETRON_SYSTEMS


#define VALTRACK_V4_VTS
//#define VALTRACK_V4MF



//#define V4MF_UM_WB
//#define V4MF_UM_L0

#define SIM7600
//#define SIM7070
//#define SIM800

#define A7672
//#define SIM7672


#ifdef SIM7070
    #define CNMP_13  // 2G
    //#define CNMP_38  // 4G NBIOT-CAT-M1
    //#define CNMP_2   // AUTO
    
    //#define CMNB_1 // CAT-M
    //#define CMNB_2 // NB
    #define CMNB_3 // M1 and NB
#endif


extern const char *TAG;

#define DISABLE_CHARGING_LOOP // Uncomment for debugging

// Enable TPS Buck converter control for disconnecting main battery when entering sleep mode. 
// This helps in saving main battery drain
//#define ENABLE_TPS_CONTROL 
#define DISABLE_TPS_CONTROL



//#define SE868_ENABLED
#define EXT_ANT_ENABLED

//#define NETWORK_LOCATION_ENABLED

//#define TIMER_ONLY_WAKEUP
#define MOTION_CONTROLLED_PINGS


// LED CONTROLS 

#define WB_PIN_CONTROLLED_LED
//#define IO_EXTENDER_CONTROLLED_LED

// CHARGER PIN CONTROL


#define GPIO_CHARGER_PIN     4
#define GPIO_TPS_ENABLE_PIN  4




//////////////RESETS////////////////////////
#define LPUART_TIMER_RESET_ENABLED
//#define LPUART_TIMER_RESET_DISABLED
#ifndef VALTRACK_V4MF
    #define NETWORK_FAIL_RESET_ENABLED
#else
    #define NETWORK_FAIL_RESET_DISABLED
#endif
/////////////////////////////////////////////

#define SLEEP_ENABLED
//#define SLEEP_DISABLED

#define PARAMS_NORMAL

#define CUSTOM_MQTT_CLIENT_ID


#define PAYLOAD_NORMAL


#ifndef VALTRACK_V4_VTS

#define RED_RANGE_LOW       0.0
#define RED_RANGE_HIGH      3.7

#define BLUE_RANGE_LOW      3.7
#define BLUE_RANGE_HIGH     3.8

#define GREEN_RANGE_LOW     3.8
#define GREEN_RANGE_HIGH    4

#define GREEN_F_RANGE_LOW   4.0
#define GREEN_F_RANGE_HIGH  4.7

#else

#define RED_RANGE_LOW       0.0
#define RED_RANGE_HIGH      8.0

#define BLUE_RANGE_LOW      8.0
#define BLUE_RANGE_HIGH     11.0

#define GREEN_RANGE_LOW     11.0
#define GREEN_RANGE_HIGH    18.0

#define GREEN_F_RANGE_LOW   11.0
#define GREEN_F_RANGE_HIGH  18.2

#endif

#ifdef SIM7600
    #define PACKET_COUNT 3
#else
    #define PACKET_COUNT 1
#endif



#ifdef SIM800
    #define LPUART_BAUDRATE 9600
#else
    #define LPUART_BAUDRATE 115200
#endif

//ifdef VALTRACK_V4_VTS
    #define LIS3DH_ENABLED
//#else
//    #define MMA854_ENABLED
//#endif

#ifdef MMA854_ENABLED
    #define ACCLEROMETER_I2C_ADDRESS 0x1D  //MMA854
#else 
    #define ACCLEROMETER_I2C_ADDRESS 0x19  //LIS3D
#endif

#define PCA_I2C_ADDRESS 0xC0<<1  //LIS3D

#define ADC_REFERENCE 3.3

// Battery voltage conversion defines START
#ifdef VALTRACK_V4_VTS

    #define R1 100000
    #define R2 3300
    #define ADC_OFFSET    0.4//3.3
    #define DIVIDER_FACTOR (float)(((float)R1+(float)R2)/(float)R2)
        
#else
    #define ADC_OFFSET    0//3.3
    #define DIVIDER_FACTOR 2
    
    
#endif
// Battery voltage conversion defines END

#ifdef PARAMS_MASSIMO
    #define POWER_BUTTON_SOS_SWAP
#endif
    
//#define TAMPER_DETECT_MODE //  Bracelet removal detection for POWER_BUTTON


/* Ignition detection from the vehicle bus (VCHG ADC).
   Measured on the van from 3 days of field telemetry, 2026-07-26..28:
       alternator running : 14.11 - 14.48 V   (hundreds of samples)
       engine off, resting: 12.67 - 13.10 V
       between 13.2 and 14.0: essentially empty
   The previous 13.3 / 13.0 pair put the OFF threshold *below* a freshly
   charged battery's resting voltage. After a drive the surface charge had to
   decay for 45-60 minutes before crossing it, so the device reported
   "engine running" for the best part of an hour after shutdown. Confirmed in
   the field: engine off around 19:53, ignition=false not until 20:37.
   These values sit in the empty band between the two measured clusters.

   Debounce is asymmetric. Switching ON should be prompt. Switching OFF needs
   to ride through brief sags - an idling engine under headlights/AC on a cold
   night can dip toward 13.5 - without flickering the state. Both are still
   vastly faster than the old behaviour. PowerSenseTick runs once per second,
   so these counts are seconds. */
#define IGNITION_ON_VOLTS      13.8f
#define IGNITION_OFF_VOLTS     13.5f
#define IGNITION_ON_DEBOUNCE      3
#define IGNITION_OFF_DEBOUNCE    15

#define TIME_TO_SLEEP 300
#define PARK_LONG_SECONDS 172800UL   // 48 hours â€” threshold to enter deep sleep
#define HEART_BEAT_INTERVAL (2*3600) // 2hr deep-sleep heartbeat wakeup — each wake also delivers queued Traccar commands (~30mAh/day on vehicle battery)

// Harsh driving detection (Phase 7b) — horizontal-plane g thresholds.
// 0.4g = industry consensus for fleet products (DOT harsh = 0.45g); tune from
// the gmax attribute once field data accumulates. Accident capped just under
// the LIS3DH's ±2g full scale — a real crash clips at 2g in this range.
//
// DISABLED since 2.3.32: every build with HarshDriveTask running (2.3.28-2.3.31)
// wedged within hours — BLE btController livelocks at high priority and starves
// all other tasks (incl. main task = no pings). Trigger mechanism unproven;
// suspects are heap churn from the I2C driver delete/reinstall recovery path or
// HarshDriveTask stack overflow trampling BLE heap. Re-enable only with the
// 2.3.33 hardening plan (see phases roadmap / CLAUDE.md).
// DISABLED AGAIN in 2.3.36. The 2.3.35 re-enable FAILED in the field: both
// units wedged within minutes and never exceeded 29 minutes uptime, while
// 2.3.34 runs for 5.5+ hours untouched. TWDT panic contained each wedge to
// ~60s, but tracking collapsed (16 distinct positions vs 176 on 2.3.34) and
// the van had to be recovered by hand because the reboots kept aborting its
// own OTA download.
//
// Crucially the instrumentation stayed healthy through every wedge -
// hstk 3320/4096 free, hmin 142272 bytes, i2crec 0 - so it is NOT stack
// overflow, NOT heap exhaustion and NOT a stuck I2C bus. All three suspects
// below are disproven. Do not simply re-enable this; the polling design
// itself is implicated. See ISSUES.md K1 for the interrupt-driven redesign.
//
// ---------------------------------------------------------------------------
// 2.3.37 REDESIGN: detection moved into the accelerometer hardware.
//
// The sampler is gone. The LIS3DH has two independent interrupt generators that
// share the physical INT1 pin, and generator 2's threshold/duration registers
// express "0.4g sustained for 300ms" directly - exactly the condition the 20Hz
// task used to compute in software. The sensor now decides, and the firmware
// only reacts when the pin asserts.
//
// Why this should not livelock: there is no periodic I2C traffic at all. The
// interrupt is latched (LIR_INT2), so the existing ~1Hz INT1 poll in
// StartMainTask cannot miss an event, and no new task is created. I2C is
// touched only when something actually happens.
// ---------------------------------------------------------------------------
#define ENABLE_HARSH_DRIVING
/* 2.3.46 calibration - ONE VARIABLE AT A TIME.
   The bench bisection on 2026-08-05 settled it: nothing is broken, the defaults
   were simply too strict. A diagnostic build at 32mg/20ms (branch
   diag/harsh-bisect) flashed to unit 2 produced hraw 0 -> 5 from a 10-second
   hand shake, with i2src=0x65/0x66 - IA set, ZH and XH events. So generator 2
   fires, the pin asserts, the poll reads it and HarshEventDetected() runs. The
   whole chain is good.

   That test moved BOTH parameters, so it does not say which one mattered. This
   release changes only the duration and leaves the threshold alone.

   Duration is the prime suspect. 300ms at 100Hz ODR means 30 CONSECUTIVE
   samples all above threshold; a single sample dipping below resets the
   counter. Real braking ramps up and down and road vibration dithers the
   signal through the high-pass filter, so an unbroken 300ms plateau may
   essentially never occur - which matches the field evidence: a genuine test
   drive on 2026-08-05 (hard braking 76.9->7.6 km/h, hard L/R/L cornering, a
   judder bar at 50 km/h) produced hraw=0 while ipoll climbed 92->132, i.e. the
   sensor was polled throughout and simply never triggered.

   0.40g itself is defensible and stays: DOT calls 0.45g harsh, Verizon Connect
   uses 0.265g braking / 0.220g acceleration. If hraw stays 0 after a hard-brake
   run on this build, lower HARSH_EVENT_G next - still one variable at a time.

   2.3.47: that hard-brake run happened (2026-08-06 12:37-12:43 NZST, three
   deliberate stops) and hraw stayed 0 with ipoll climbing 191->233, so the
   sensor was polled throughout and never triggered at 0.40g/100ms. Traccar
   positions give the decelerations: 47.8->15.7, 68.2->36.7 and 70.9->42.8 km/h,
   i.e. 0.227g, 0.223g and 0.199g. Those are averages over a 4s sampling window
   so the true peaks are higher, but they put real braking on this vehicle in
   the 0.2-0.3g band, not above 0.4g. Threshold drops to 0.25g - which is also
   almost exactly Verizon Connect's 0.265g braking figure. Duration stays at
   100ms so this remains one variable. */
#define HARSH_EVENT_G       0.25f   // 2.3.47: was 0.40 (never triggered on real hard braking)
#define HARSH_EVENT_MS      80      // 2.3.54: back to 2 ticks - the only value proven to detect braking
/* 2.3.50 - THE DURATION GATE, not the threshold, is the remaining lever.

   2.3.49 (ZHIE dropped) cut the gated event rate 3x - hcnt/moving-ping fell
   0.57 -> 0.19 and hraw/moving-ping 1.10 -> 0.26 across ~52h and five drives.
   But cross-referencing all 18 surviving hcnt events against the GPS speed
   trace (2026-08-09) showed the residue is still road noise, now arriving
   through X/Y instead of Z:
     0 events >= 0.25g, 5 in the 0.15-0.24g band, 11 below 0.15g.
   Four of the 11 fired at highway speed with no deceleration at all - 0.036g
   (70.5->65.4 km/h in 4s), 0.040g x2 (71.6->68.8 in 2s), 0.059g (76.6->72.5
   in 2s). The sensor is not wrong there: those are genuine >0.25g TRANSIENTS
   (road joints, potholes) that a 2-3s GPS average cannot see.

   Do NOT raise HARSH_EVENT_G to fix this. The five plausible brakes measure
   0.15-0.24g GPS-averaged, i.e. at or BELOW the current 0.25g threshold, so
   the two populations overlap in amplitude - raising it removes the real
   events before the noise. They separate cleanly on DURATION instead:
   braking is sustained 1-3s, a road jolt is under 100ms.

   The gate was doing nothing. At 25Hz a DUR tick is 40ms, so the old 100ms
   truncated to 2 ticks = 80ms. 480ms = 12 ticks EXACTLY (no truncation, which
   is why 480 and not 500). Well inside the HPF tau of 2.5s, so sustained
   braking still reaches the comparator.

   Threshold and ODR unchanged - one variable. If hraw drops back toward 0 on
   real braking, the gate is too long: step down 480 -> 320ms (8 ticks) before
   touching anything else. Expected lisreg: 33,37,B7,60,08,0A,0A,0F,0C
   (last byte 0C = 12, was 02).

   2.3.53 - THAT PREDICTION FIRED. 480ms suppressed everything, including real
   braking. Van drive 2026-08-09/10 on 2.3.50: hraw=0 and hcnt=0 across 29
   moving pings, with i2src never showing IA set at all (0x25/0x26 only). The
   drive contained a **0.518g** stop - 71.8 -> 35.2 km/h in 2s - plus 0.243g
   and 0.221g. A 0.518g deceleration sustained for 2s is more than double the
   0.24g threshold and four times the 480ms requirement, and it produced
   nothing.

   MECHANISM, and the thing that was underweighted when 480ms was chosen:
   INT2_DURATION counts CONSECUTIVE samples above threshold. One sample dipping
   below resets the counter. At 25Hz, 480ms is 12 unbroken samples - and real
   braking ramps while road vibration dithers the signal through the high-pass
   filter, so a 12-sample unbroken run may essentially never occur. This is the
   SAME failure that killed 2.3.46's 300ms gate at 100Hz (30 consecutive
   samples); the note about it is a few lines above, and it should have carried
   more weight here. The duration lever is real - noise suppression worked,
   expected ~7.5 events at the 2.3.49 rate and got 0 - but it is far more
   fragile than "braking is 1-3s, jolts are <100ms" suggests.

   320ms = 8 ticks exactly at 25Hz. Threshold and ODR still untouched.
   Expected lisreg: 33,37,B7,60,08,0A,0A,0F,08 (last byte 08 = 8).

   2.3.54 - IT DID, AND THE REGISTER IS NOW SETTLED AT 80ms. Van drive
   2026-08-12 on 2.3.53-rc1 (320ms): hraw=0 and hcnt=0 with ipoll=454 over 382
   moving samples, and the drive contained a **0.391g** stop (68.9 -> 27.5 km/h
   in 3s) plus 0.289g. Three durations measured on real drives:
     2 ticks  (80ms)  - detects real braking (2.3.48: 0.294/0.252/0.338g all
                        registered) but also road transients
     8 ticks  (320ms) - nothing; a 0.391g brake ignored
     12 ticks (480ms) - nothing; a 0.518g brake ignored
   The consecutive-sample requirement is hostile to a dithered signal, so there
   is no usable window here. **Duration is NOT the discriminator. Do not
   bisect this register again** - the separation is done in firmware instead,
   by requiring a speed signature (see HarshEventDetected). 80ms stays because
   it is the only value proven to detect braking at all.

   Superseded stopping rule, kept for the reasoning:
   Noise fires at 80ms and real braking dies by 320ms would mean the two
   populations do not separate on consecutive-sample duration at all, and the
   answer moves to firmware-side gating - e.g. requiring N hraw events inside a
   window, where a dip between them is survivable - or to FIFO capture. Do not
   keep bisecting a lever that has been shown not to work.

   Two artefacts to ignore when re-running this analysis: the "1.163g" pair on
   08-07 03:08 is GNSS fix-loss zeroing (82 km/h -> exactly 0.0 in 2s, after a
   10s sample gap), not braking - the 2.3.33 zero-on-fix-loss path. Separate
   issue, tracked in ISSUES.md. */
#define HARSH_RESET_G       0.25f   // UNUSED - leftover from the pre-2.3.37 software sampler
#define HARSH_ACCIDENT_G    1.85f   // near-clip spike = accident (stage 2, see below)
#define HARSH_HOLDOFF_S     15      // min gap between harsh alarms
/* 2.3.56: 6.0 -> 15.0. This constant changed JOB in 2.3.54 and the old value
   did not follow. It was written to *classify* an event (braking vs
   acceleration vs cornering), where 6 km/h is a sensible dividing line. Since
   2.3.54 it also *rejects* - an event that clears neither direction is thrown
   away as a road transient - and as a rejection threshold 6 km/h over the
   HARSH_SPD_HIST 4s window is only 0.042g. That is far too permissive: the
   worst false positive on the 2026-08-13 drive measured 0.043g (69.9 -> 65.4
   km/h), clearing the bar almost exactly.

   15 km/h over 4s is ~0.106g average. Checked against every accepted event on
   that drive: it keeps all four >=0.25g brakes (which showed 32-36 km/h drops)
   and the six mid-band events, and rejects the four weakest - 4.5, 7.4, 8.5
   and 8.5 km/h. 10 of 14 survive, and the ones lost are the ones with no
   deceleration worth reporting.

   Averaging note: ds is measured over the full 4s history, so a sharp 2s brake
   is UNDERSTATED here - a 32 km/h drop in 2s reads as 32 over 4s only if the
   vehicle stays slow. That is the safe direction (it under-rejects), but it is
   why this is not simply "0.106g". Do not convert this constant to a g figure
   and tune it as if it were an accelerometer threshold.

   2.3.57 - DO NOT TRUST THE ANALYSIS ABOVE. Every measurement it rests on was
   taken while HarshSpeedTick was zero-filling the history on any GPS gap over
   10s (fixed in 2.3.57 - see the comment on that function). A fabricated 0 for
   spd_old makes ds large and POSITIVE, so events were pushed across this
   threshold that should never have cleared it, and the drive used to justify
   15.0 cannot distinguish a real 32 km/h brake from a transient that happened
   to land after a ping boundary. The value is LEFT AT 15.0 deliberately: it is
   one variable, and changing it in the same build as the zero-fill fix would
   make the result unreadable. Re-measure on clean data before touching it.

   Known counter-evidence already, from the van on 2026-08-18: a real
   acceleration (60.7 -> 91.5 km/h over 21s) was rejected because any 4s window
   across it shows only ~+6 km/h. Sustained real-world vehicle acceleration is
   0.04-0.09g, well under the ~0.106g this represents, so the window length may
   be the wrong lever rather than the number. Do not "fix" that by raising
   HARSH_SPD_HIST without re-reading the averaging note above. */
#define HARSH_SPEED_DELTA   15.0f   // km/h over HARSH_SPD_HIST: classifies AND rejects (2.3.56)
#define HARSH_MIN_SPEED     15.0f   // ignore events when peak involved speed below this (door slams etc.)

/* Hardware register values, derived from the thresholds above so the two can
   never drift apart.
     THS  LSB = 16mg   at +-2g full scale (CTRL_REG4 = 0x08)
     DUR  LSB = 1/ODR  = 10ms at 100Hz    (CTRL_REG1 = 0x57)
   0.25g -> 15 counts. Both well inside the 7-bit fields.

   2.3.48: ODR 100Hz -> 25Hz, to move the HIGH-PASS FILTER CUTOFF, not the
   sample rate. The HPF cutoff scales with ODR (HPCF=11 selects ODR/400), so
   100Hz put it at 0.25Hz - a 0.64s time constant. That is the same order as a
   real brake, so the filter cancels the very signal being measured.

   Evidence (2026-08-06 drive, 2.3.47 at 0.25g): a measured 0.409g stop
   (51.3 -> 22.4 km/h in 2s) and a 0.364g stop both left hraw at 0, with ipoll
   climbing 8 polls past each event and the interrupt latched - so the sensor
   genuinely never triggered. A 0.409g brake failing a 0.25g threshold means
   the threshold was never the blocker. What DID fire, minutes later, had ZH/ZL
   set (vertical axis) with no deceleration near it - i.e. a bump, which is
   high-frequency and passes the filter intact.

   Braking is a RAMP, not a step. A step would pass at full amplitude and decay;
   a ~1s ramp to 0.4g with tau=0.64s reaches the comparator at roughly 0.15-0.2g
   - under threshold, which is exactly what the field shows. At 25Hz the cutoff
   drops to 0.0625Hz (tau 2.5s) and that same ramp presents ~0.3g.

   FALSIFIABLE: if the HPF is the cause, hraw fires on braking at the SAME 0.25g
   threshold. If hraw is still 0, the filter is not the mechanism - stop
   adjusting it and look elsewhere. Do not also change the threshold. */
#define LIS3DH_ODR_HZ            25
/* CTRL_REG1[7:4] ODR selector. MUST agree with LIS3DH_ODR_HZ above - the
   preprocessor cannot derive one from the other, so they are checked by eye.
   0001=1Hz 0010=10Hz 0011=25Hz 0100=50Hz 0101=100Hz 0110=200Hz 0111=400Hz.
   Low nibble 0x7 = LPen off (high-resolution) + Zen|Yen|Xen. */
#define LIS3DH_ODR_BITS          0x3
#define LIS3DH_CTRL_REG1   ((unsigned char)((LIS3DH_ODR_BITS << 4) | 0x07))

/* Generator 2 axis enables (INT2_CFG). Bit1 XHIE, bit3 YHIE, bit5 ZHIE, OR
   combination (AOI=0). The LOW enables (XLIE/YLIE/ZLIE) stay off and must not
   be turned on: under AOI=0 they mean "fire whenever this axis is BELOW
   threshold", which is trivially true at rest. 0x3F was tried and gave hraw
   8 -> 23 on a motionless bench (i2src=0x55). Recorded on diag/harsh-bidir.

   2.3.49: ZHIE dropped, 0x2A -> 0x0A. 2.3.48 fixed the filter and braking
   started registering (three stops at 0.294/0.252/0.338g each produced an hraw
   increment at the next poll, on the same 0.25g that had given hraw=0 for a
   0.409g stop at 100Hz ODR). But the lower cutoff also passes road-surface
   vertical jolts: hraw ran 3 -> 23 and hcnt 0 -> 12 in eight minutes of
   ordinary commuting, including two increments during steady 50-58 km/h cruise
   at +-0.02g longitudinal. Every i2src read with IA actually set involved Z -
   0x65 (ZH), 0x59 (ZL), 0x56 (ZL) - four for four, matching the bump-triggered
   alarm on 2.3.47.

   Braking and cornering are horizontal, so Z contributes noise and no signal.
   ASSUMES Z IS THE VERTICAL AXIS - inferred from bumps firing it, not from a
   known mounting orientation. If hraw goes back to 0 on braking, that
   assumption was wrong: revert to 0x2A before changing anything else.

   Accident detection (stage 2, unimplemented) will want vertical sensing back;
   it should use FIFO capture rather than this generator. */
#define HARSH_INT2_CFG           0x0A
#define LIS3DH_THS_MG_PER_LSB    16
#define HARSH_THS_COUNTS   ((unsigned char)((HARSH_EVENT_G * 1000.0f) / LIS3DH_THS_MG_PER_LSB))
#define HARSH_DUR_COUNTS   ((unsigned char)((HARSH_EVENT_MS * LIS3DH_ODR_HZ) / 1000))
#define HARSH_SPD_HIST           4   // seconds of speed history for classification

/* STAGE 2 (not implemented): accident severity. The hardware interrupt tells us
   the threshold was crossed but not by how much, and because the pin is polled
   at ~1Hz the event is over before we could read the peak. The clean answer is
   the LIS3DH FIFO in Stream-to-FIFO mode: it continuously overwrites until the
   interrupt triggers it, then freezes, preserving the event waveform for a
   leisurely read. That also restores the gmax attribute. Deliberately deferred
   so this build changes one thing only - whether removing the polling stops the
   livelock. See ISSUES.md K1. */

// OTA firmware update
/* 2.3.55 was consumed by the D2 deep-sleep diagnostic (2.3.55-diag1, branch
   diag/deepsleep-fast, staging only, never released). Skipped here so the
   number cannot be confused with a fleet release. */
#define FW_VERSION          "2.3.57"
#define OTA_VERSION_URL     "http://ota.pawson.co.nz/version.json"
#define OTA_FIRMWARE_URL    "http://ota.pawson.co.nz/firmware.bin"

/* 2.3.51 - STAGING OTA CHANNEL.
   Lets one device be moved to a test build without touching the fleet.
   `Moved V_OTA_TEST` switches this device to the staging channel and updates;
   `Moved V_OTA` switches it back to production and updates (a downgrade is
   fine - the version comparison is strcmp-inequality, not ordering).

   Traccar commands are addressed to a single device, so this gives the
   per-device targeting that `version.json` alone cannot: production
   version.json is global, but which URL a device reads is now per-device.

   THE CHANNEL MUST PERSIST IN NVS. Without that, the staging build reboots,
   `CheckAndApplyOTA()` runs at boot against the production URL, sees a
   different version and immediately pulls the device back - an OTA ping-pong
   that burns cellular data on every cycle. Stored as `ota_ch` in the existing
   "valtrack" namespace.

   If the staging server is unreachable the device STAYS on the staging
   channel and simply skips the update. Auto-reverting would reintroduce the
   ping-pong. Recovery is `Moved V_OTA` (back to production) or `V_RESET`,
   neither of which depends on the staging host being up. */
#define OTA_STAGING_VERSION_URL   "http://ota.pawson.co.nz/staging/version.json"
#define OTA_STAGING_FIRMWARE_URL  "http://ota.pawson.co.nz/staging/firmware.bin"

#define OTA_CHANNEL_PRODUCTION    0
#define OTA_CHANNEL_STAGING       1
#define OTA_CHUNK_SIZE      4096
#define OTA_MAX_FIRMWARE    0xEE000  // must fit in one OTA partition

// uart_event_task_handle declared in SCI.c; extern in main.c (see OTA functions)

#define BLUETOOTH_ENABLED

//#define BATTERY_PRESENT

#define BATTERY_READ_INTERVAL 300

// #define SHEETS_ENABLED
/////////////////////////////////////////////////////////////////////////////////////////////

// PWRKEY 7
// GSM ENABLE 10
// LED SIGNAL 8
// TPS ENABLE 4 //OR CHG IN
// INT1 3
// ANALOG IN 2
// IIC DATA 5
// IIC CLOCK 6
#define GPIO_LED_SIGNAL 8
#define GPIO_PWRKEY    7
#define GPIO_GSM_ENABLE    10
#define GPIO_TPS_ENABLE    4
#define GPIO_LED_SIGNAL_PIN_SEL ((1ULL<<GPIO_LED_SIGNAL))
#define GPIO_PWRKEY_GSM_ENABLE_PIN_SEL  ((1ULL<<GPIO_PWRKEY) | (1ULL<<GPIO_GSM_ENABLE))
#define GPIO_TPS_ENABLE_PIN_SEL ((1ULL<<GPIO_TPS_ENABLE))
#define GPIO_INT1     3
#define GPIO_SOS      9
#define GPIO_CHG_IN   4

#ifndef VALTRACK_V4_VTS
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INT1) | (1ULL<<GPIO_SOS) | (1ULL<<GPIO_CHG_IN)) 
#else
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INT1) | (1ULL<<GPIO_SOS) )  
#endif



#define UART_PORT_NUM UART_NUM_1


#define ECHO_TEST_TXD 0//21//(CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD 1//20//(CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      UART_NUM_1//(CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE     115200//(CONFIG_EXAMPLE_UART_BAUD_RATE)

#define CONFIG_I2C_MASTER_SCL 6
#define CONFIG_I2C_MASTER_SDA 5



//#define BUF_SIZE (1024)


#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const char *TAG;

#define BUFF_SIZE 250

extern char Buff[];
extern unsigned short BuffIndex;


#define BUFF2_SIZE 350

extern char Buff2[];
extern unsigned short Buff2Index;

#define TBUFF_SIZE 252

extern char Buff[];
extern unsigned short BuffIndex;

//#define DEBUG_PRINT
#ifdef DEBUG_PRINT
extern unsigned char DebugPrintEnabled;
#endif   
typedef enum 
{
    EVENT_ENGINE_OFF                    ,    // 0   00 
    EVENT_ENGINE_ON                     ,    // 1   01 
    REGULAR_TEST                        ,    // 2   02 
    RANDOM_TEST                         ,    // 3   03 
    EVENT_BYPASS                        ,    // 4   04 
    EVENT_LOG_CLEARED                   ,    // 5   05 
    EVENT_SERVICE_PEROD_SET             ,    // 6   06 
    EVENT_RESET_FOR_VIOLATION           ,    // 7   07 
    EVENT_USE_ONE_TIME_CODE             ,    // 8   08 
    EVENT_RE_DO                         ,    // 9   09 
    EVENT_12V_POWER_REMOVED             ,    // 10  0A
    EVENT_LOG_READ                      ,    // 11  0B
    EVENT_CONFIG_CHANGED                ,    // 12  0C
    EVENT_LOG_FULL                      ,    // 13  0D
    EVENT_SERVICE_ALERT                 ,    // 14  0E
    not_used                            ,    // 15  0F
    EVENT_WEAK_BLOW                     ,    // 16  10
    EVENT_WARNING_FAIL_RR               ,    // 17  11
    EVENT_WARNING_REFUSED               ,    // 18  12
    EVENT_WARNING_SERVICE_PERIOD_END    ,    // 19  13
    EVENT_CAR_BATTERY_ON                ,    // 20  14
    EVENT_CAR_BATTERY_OFF               ,    // 21  15
    EVENT_CALIBATION_DONE               ,    // 22  16
    EVENT_IGNITION_KEYED                ,    // 23  17
    EVENT_WARNING_GIVEN                 ,    // 24  18
    EVENT_STATER_NOT_ACTIVE             ,    // 25  19
    EVENT_INSUFFICIENT_BLOW             ,    // 26  1A
    EVENT_COOL_SAMPLE                   ,    // 27  1B
    EVENT_TAMPERED                      ,    // 28  1C
    EVENT_START_TEST_ATTEMPT            ,    // 29  1D
    EVENT_AB_FC_CONNECTED               ,    // 30  1E
    EVENT_AB_FC_REMOVED	                ,    // 31  1F
    EVENT_CAL_CHK_PASS	                ,    // 32  20
    EVENT_CAL_CHK_FAIL	                ,    // 33  21
    EVENT_ENGINE_NOT_STARTED		    ,    // 34  22
    G_PING      = 35 				    ,    // 35  23
    GPRS_PING   = 36  					,	 // 36  24
    MOTION_PING = 37			        ,	 // 37  25
    REBOOT_PING = 38			        ,	 // 38  26
    SOS_PING    = 39		            ,	 // 39  27
}EventCodeType;
    
typedef enum 
{   
    NL_SUCCESS                                        =0,    
    NL_PARAMETER_ERROR_RETURNED_BY_SERVER             =1,   
    NL_SERVICE_OUT_OF_TIME_RETURNED_BY_SERVER         =2,    
    NL_LOCATION_FAILED_RETURNED_BY_SERVER             =3,    
    NL_QUERY_TIMEOUT_RETURNED_BY_SERVER               =4,    
    NL_CERTIFICATION_FAILED_RETURNED_BY_SERVER        =5,    
    NL_SERVER_LBS_ERROR_SUCCESS                       =6,    
    NL_SERVER_LBS_ERROR_FAILED                        =7,    
    NL_LBS_IS_BUSY                                    =8,    
    NL_OPEN_NETWORK_ERROR                             =9,    
    NL_CLOSE_NETWORK_ERROR                           =10,   
    NL_OPERATION_TIMEOUT                             =11,   
    NL_DNS_ERROR                                     =12,   
    NL_CREATE_SOCKET_ERROR                           =13,   
    NL_CONNECT_SOCKET_ERROR                          =14,   
    NL_CLOSE_SOCKET_ERROR                            =15,   
    NL_GET_CELL_INFO_ERROR                           =16,   
    NL_GET_IMEI_ERROR                                =17,   
    NL_SEND_DATA_ERROR                               =18,   
    NL_RECEIVE_DATA_ERROR                            =19,   
    NL_NONET_ERROR                                   =20,   
    NL_NET_NOT_OPENED                                =21,   
    NL_REPORT_LBS_TO_SERVER_SUCCESS                  =80,   
    NL_REPORT_LBS_TO_SERVER_PARAMETER_ERROR          =81,   
    NL_REPORT_LBS_TO_SERVER_FAILED                   =82,   
    NL_OTHER_ERROR                                   =110 

}NetworkLocationStatusType;
typedef struct
{
    char Bytes[5];
}EngineStatusStringType;

typedef struct
{
    char Bytes[50];
}StringType;
typedef enum
{
    MOTION_DETECTED = 0,
    NO_ACTIVITY = 1

}MotionStatusType;
//
// Overrun event
//
#define SYSTEM_EVENT_FIFO_OVERRUN  0xFF

//
// Message data length (fixed at 16 to fully support STS current 
// message structure)
//             
#define MESSAGE_LENGTH          16
#define MESSAGE_DATA_LENGTH     (MESSAGE_LENGTH-8)

//
// HW data length.  This data is smaller so we can store more
// events in a smaller space using a circular buffer FIFO
//
#define HW_EVENT_LENGTH         24//32+16+10+2

typedef struct
{
    EventCodeType EventType;
    unsigned char Hours;
    unsigned char Minutes;
    unsigned char Seconds;
    unsigned char Date;
    unsigned char Month;
    unsigned char Year;
    float Voltage;
    float Lat;
    float Long;
    float Speed;
    
}GeneralEventType;

typedef union
{
    unsigned char Bytes[HW_EVENT_LENGTH];    
  
    GeneralEventType GEvent;
         
} HWEventDataType;

//extern double fLat,fLong;  
typedef enum 
{
    RTC_RESET,          // 0
    INT1_RESET,         // 1
    BUFF2_RESET,        // 2
    NETWORK_RESET,      // 3
    LPUART_TIMER_RESET, // 4
    MOTION_RESET,       // 5
    HARDFAULT_RESET,    // 6    
    DEEP_SLEEP_RESET,   // 7
    CHARGER_RESET       // 8

}BootReasonType;

#define _countof(a) (sizeof(a)/sizeof(*(a)))

extern char Link[];
extern char D1,D2,GPSStatus;
extern char Lat[10],Long[10];
extern char Altitude[10];
extern char Course[10];
extern char NS,EW;
extern unsigned char f;
extern float fLat,fLong,pfLat,pfLong,fSpeed,fAltitude,fCourse; 
extern unsigned char prevGPSStatus;
extern char GPSTime[],GPSDate[],GPSSpeed[];
extern int GPSHours,GPSMinutes,GPSSeconds,GPSDay,GPSMonth,GPSYear;

extern unsigned char Byte1,Byte2,Byte3,Byte4,Byte5,Byte6;

unsigned char GetAscii(unsigned char Data);

void  Binary2Ascii(unsigned long HexValue);

//extern unsigned char ProcessPacket;
void ResetBuffer(void);
void ResetBuffer1(void);
void InitSCI(void);
void putcchar(unsigned char Data);
void putcchar1(unsigned char Data);
void Print1(char *pData);
unsigned char ReadUART2(void);
void WriteUART2(unsigned char Data);
void PrintNumber(unsigned short Data);
unsigned char GetCheckSum(char*pData,unsigned char len);
void SCIError_Handler(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void PrintPacket(char*pData,unsigned short Length);
void DebugPrint(char*pData);
/*******************************************************************************
* Funtion name : MapForward                                                    *
*                                                                              *
* Description  : This function performs forward mapping and returns pointer    *
*                to the start of the found data or else returns NULL pointer   *
*                                                                              *
* Arguments    : 1) Poshorter to the data in which mapping is to be made       *
*                2) Length of the data in which mapping is to be made          *
*                3) Poshorter to the data points to be mapped                  *
*                4) Length of the data points to be mapped                     *
*                                                                              *
* Returns      : Pointer in which result is returned                           *
*******************************************************************************/
char * MapForward
(
	char *pMapData,
	unsigned short   MapDataLength,
	char *pMapPoints,
	unsigned short   MapPointsLength	
);
/*****************************************************************************
* Function Name : Print                                                      *
*                                                                            *
* Description   : This function is used to print data onto the serial        *
*                 communications interface of 8051 type MCU                  *
*                                                                            *
* Arguments     : 1) Pointer to the data to be printed                       *
*                                                                            *
* Returns       : Nothing                                                    *
*****************************************************************************/
void Print(char *pData);
/*****************************************************************************
* Function Name : PrintNumber                                                *
*                                                                            *
* Description   : This function is used to print data onto the serial        *
*                 communications interface of 8051 type MCU                  *
*                                                                            *
* Arguments     : 1) Number to be printed                                    *
*                                                                            *
* Returns       : Nothing                                                    *
*****************************************************************************/
void PrintNumber(unsigned short Data);

void DelayProc(unsigned long Count);
void HandleGPSData(void);
void HandleGPSINFData(unsigned char Byte);
void HandleGSMData(void);
void RTOSDelay(unsigned int millis);
void InitUART(void);
void osDelay(unsigned int Value);
double GPRMC2Degrees(double Value);


/** 
  * @brief  RTC Time structure definition  
  */
typedef struct
{
  uint8_t Hours;            /*!< Specifies the RTC Time Hour.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the RTC_HourFormat_12 is selected.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the RTC_HourFormat_24 is selected */

  uint8_t Minutes;          /*!< Specifies the RTC Time Minutes.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

  uint8_t Seconds;          /*!< Specifies the RTC Time Seconds.
                                 This parameter must be a number between Min_Data = 0 and Max_Data = 59 */

  uint8_t TimeFormat;       /*!< Specifies the RTC AM/PM Time.
                                 This parameter can be a value of @ref RTC_AM_PM_Definitions */
  
  uint32_t SubSeconds;     /*!< Specifies the RTC_SSR RTC Sub Second register content.
                                 This parameter corresponds to a time unit range between [0-1] Second
                                 with [1 Sec / SecondFraction +1] granularity */
 
  uint32_t SecondFraction;  /*!< Specifies the range or granularity of Sub Second register content
                                 corresponding to Synchronous pre-scaler factor value (PREDIV_S)
                                 This parameter corresponds to a time unit range between [0-1] Second
                                 with [1 Sec / SecondFraction +1] granularity.
                                 This field will be used only by HAL_RTC_GetTime function */
  
  uint32_t DayLightSaving;  /*!< Specifies RTC_DayLightSaveOperation: the value of hour adjustment.
                                 This parameter can be a value of @ref RTC_DayLightSaving_Definitions */

  uint32_t StoreOperation;  /*!< Specifies RTC_StoreOperation value to be written in the BCK bit 
                                 in CR register to store the operation.
                                 This parameter can be a value of @ref RTC_StoreOperation_Definitions */
}RTC_TimeTypeDef;

/** 
  * @brief  RTC Date structure definition
  */
typedef struct
{
  uint8_t WeekDay;  /*!< Specifies the RTC Date WeekDay.
                         This parameter can be a value of @ref RTC_WeekDay_Definitions */

  uint8_t Month;    /*!< Specifies the RTC Date Month (in BCD format).
                         This parameter can be a value of @ref RTC_Month_Date_Definitions */

  uint8_t Date;     /*!< Specifies the RTC Date.
                         This parameter must be a number between Min_Data = 1 and Max_Data = 31 */

  uint8_t Year;     /*!< Specifies the RTC Date Year.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 99 */

}RTC_DateTypeDef;

/** 
  * @brief  RTC Alarm structure definition
  */
typedef struct
{
  RTC_TimeTypeDef AlarmTime;     /*!< Specifies the RTC Alarm Time members */

  uint32_t AlarmMask;            /*!< Specifies the RTC Alarm Masks.
                                      This parameter can be a value of @ref RTC_AlarmMask_Definitions */
  
  uint32_t AlarmSubSecondMask;   /*!< Specifies the RTC Alarm SubSeconds Masks.
                                      This parameter can be a value of @ref RTC_Alarm_Sub_Seconds_Masks_Definitions */

  uint32_t AlarmDateWeekDaySel;  /*!< Specifies the RTC Alarm is on Date or WeekDay.
                                     This parameter can be a value of @ref RTC_AlarmDateWeekDay_Definitions */

  uint8_t AlarmDateWeekDay;      /*!< Specifies the RTC Alarm Date/WeekDay.
                                      If the Alarm Date is selected, this parameter must be set to a value in the 1-31 range.
                                      If the Alarm WeekDay is selected, this parameter can be a value of @ref RTC_WeekDay_Definitions */

  uint32_t Alarm;                /*!< Specifies the alarm .
                                      This parameter can be a value of @ref RTC_Alarms_Definitions */
}RTC_AlarmTypeDef;

#endif
