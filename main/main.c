/***************************************************************************
* File name :   main.c                                           *
*                                                                          *
* Author    :   Ravi Y. Pujar                                              *
*                                                                          *
* Owner     :   Copyright (c) 2023 Valetron Systems Pvt Ltd,               *
*                all rights reserved                                       *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/i2c.h"
#include "SCI.h"
#include "CircularBuffer.h"
#include "FrontPanel.h"
#include "MotionSensor.h"
#include "ADC.h"
#include "DeepSleep.h"

#include "freertos/event_groups.h"
#include "esp_event.h"
#include <nvs_flash.h>

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "esp_wifi.h"
#include "esp_bt.h"
// #include "driver/adc.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "esp_rom_sys.h"   // esp_rom_delay_us for the I2C bus-clear pulse train
#include <math.h>
extern TaskHandle_t uart_event_task_handle;


#define BTBUFF_SIZE 500
unsigned char BTBuff[BTBUFF_SIZE];
unsigned short BTBuffIndex = 0;


static bool notify_state;

static uint16_t conn_handle;

/* Variable to simulate heart beats */
static uint8_t heartrate = 90;


static TimerHandle_t blehr_tx_timer;

uint16_t HR=9;
/*****************************************************************************
* Function Name : ResetBTBuffer                                              *
*                                                                            *
* Description   : This function is used to reset the BT  buffer              *
*                                                                            *
* Arguments     : None                                                       *
*                                                                            *
* Returns       : Nothing                                                    *
*****************************************************************************/
void ResetBTBuffer(void)
{
    unsigned short i;
    //USART_ITConfig(USART2,USART_IT_RXNE, DISABLE);
    for(i = 0; i < BTBUFF_SIZE; i++){ BTBuff[i] = 0; }
    BTBuffIndex = 0;
    //ProcessPacket = 0;
    //USART_ITConfig(USART2,USART_IT_RXNE, ENABLE);
}
//extern static PeerToPeerContext_t aPeerToPeerContext;
void putcharBT(char Data)
{
    
    char TResponsePacket[2];
    TResponsePacket[0] = 0x01; 
    TResponsePacket[1] = Data;            
    //P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)TResponsePacket);
    osDelay(100); // Needed for app thread character collection, app cant process all in sequence due to threading.
    
//         result = aci_gatt_update_char_value(aPeerToPeerContext.PeerToPeerSvcHdle,
//                             aPeerToPeerContext.P2PNotifyServerToClientCharHdle,
//                              0, /* charValOffset */
//                             2, /* charValueLen */
//                             (uint8_t *)  pPayload);
}
void PrintBT(char *pData)
{
    unsigned short i;   
 
    for(i = 0; i < strlen((void*)pData); i++)
    {
        putcharBT(pData[i]);
    }
        
}

//char *TAG = "BLE-Server";
uint8_t ble_addr_type;
void ble_app_advertise(void);
int c=0;
char ParamBeingRead[20];
//char strb[500];

char tbstr[300];
char tbstr1[300];

void splitString(char *input, char *delimiter) {
    char *token = strtok(input, delimiter);

    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, delimiter);
    }
}
// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char ParamBeingWritten[100];
    char delimiter[] = " | ";
    c+=ctxt->om->om_len;
    memset(tbstr1,0,sizeof(tbstr1));
    memcpy(tbstr1,ctxt->om->om_data,ctxt->om->om_len);
   
    
  
    ESP_LOGW(TAG,"Received BT data##########################");
    //printf("\r\nlen = %d\r\n",ctxt->om->om_len);   
   
    char *token = strtok(tbstr1, delimiter);

  
    sprintf(ParamBeingWritten,"%s", token);
    
    token = strtok(NULL, delimiter);
    //sprintf(ParamBeingWritten,"%s", token);
    //printf("\nWriting --> %s---%s\n",ParamBeingWritten,token);
    
    StoreParamString(ParamBeingWritten,token);

    //StoreEEParams();
    //char s[30]="Trying";//{8,8,8,8,8,8};
    //snprintf(s,_countof(s),"888888");
    // ESP_LOGW(TAG,"Sending BT data************************************");
    // os_mbuf_append(ctxt->om, "Trying", strlen("Trying"));
    //memset()
    return 0;
}
int btx=0;
    // Read data from ESP32 defined as server
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char s[30]="Hello";//{8,8,8,8,8,8};
    //snprintf(s,_countof(s),"888888");
    ESP_LOGW(TAG,"Sending BT data************************************");
    GetParams(&Params);
    os_mbuf_append(ctxt->om, Params.Fields.APNName, strlen(Params.Fields.APNName));
        // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    // os_mbuf_append(ctxt->om, s, strlen(s));
    return 0;
}

//device_notify is named by Ravi from gatt_svr_chr_access_heart_rate
static int
device_notify(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    /* Sensor location, set to "Chest" */
    static uint8_t body_sens_loc = 0x01;
    uint16_t uuid;
    int rc;

    uuid = ble_uuid_u16(ctxt->chr->uuid);

    if (uuid == 0xDEAF) {
        rc = os_mbuf_append(ctxt->om, &body_sens_loc, sizeof(body_sens_loc));

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}


/* This function simulates heart beat and notifies it to the client */
static void
//blehr_tx_hrate(TimerHandle_t ev)
SendParam(char *pValue)
{
    // static uint8_t hrm[2];
    int rc;
    struct os_mbuf *om;

    if (!notify_state) {
        //blehr_tx_hrate_stop();
        //heartrate = 90;
        return;
    }

    // hrm[0] = 0x06; /* contact of a sensor */
    // hrm[1] = heartrate; /* storing dummy data */

    /* Simulation of heart beats */
    // heartrate++;
    // if (heartrate == 160) {
    //     heartrate = 90;
    // }
    printf("Notifying\n");
    // sprintf(tbstr,"%s",heartrate);
    //om = ble_hs_mbuf_from_flat("hrm", sizeof(hrm));
    //om = ble_hs_mbuf_from_flat(tbstr, strlen(tbstr));
    om = ble_hs_mbuf_from_flat(pValue, strlen(pValue));
    rc = ble_gatts_notify_custom(conn_handle, HR, om);

    assert(rc == 0);

    // blehr_tx_hrate_reset();
}

// Write data to ESP32 defined as server
static int device_command(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    //char s[100];
    //c+=ctxt->om->om_len;
    //sprintf(s,"%.*s", ctxt->om->om_len, ctxt->om->om_data);
    //memcpy(s,ctxt->om->om_data,ctxt->om->om_len);
    //strcat(strb,s);
    
    //if(c>=100)
    // {
    //     printf(strb);c=0;
    // }
    // printf("%d-",c);
    //ESP_LOGW(TAG,"Received BT data##########################");
    //printf("\r\nlen = %d\r\n",ctxt->om->om_len);
    // for(int i =0;i<ctxt->om->om_len;i++)
    //     printf("%c,",s[i]);
    //strcpy(ParamBeingRead,(char*)ctxt->om->om_data);
    memcpy(ParamBeingRead,(char*)ctxt->om->om_data,ctxt->om->om_len);
    ParamBeingRead[ctxt->om->om_len] = 0;
    printf("ParamBeingRead = %s\n",ParamBeingRead);
    memset(tbstr,0,sizeof(tbstr));
    GetParamString(ParamBeingRead,tbstr);
    //SendParam(ParamBeingRead);
    SendParam(tbstr);
    //StoreEEParams();
    //char s[30]="Trying";//{8,8,8,8,8,8};
    //snprintf(s,_countof(s),"888888");
    // ESP_LOGW(TAG,"Sending BT data************************************");
    // os_mbuf_append(ctxt->om, "Trying", strlen("Trying"));
    
    return 0;
}
// static void
// blehr_tx_hrate_stop(void)
// {
//     xTimerStop( blehr_tx_timer, 1000 / portTICK_PERIOD_MS );
// }

// /* Reset heart rate measurement */
// static void
// blehr_tx_hrate_reset(void)
// {
//     int rc;

//     if (xTimerReset(blehr_tx_timer, 1000 / portTICK_PERIOD_MS ) == pdPASS) {
//         rc = 0;
//     } else {
//         rc = 1;
//     }

//     assert(rc == 0);

// }


// Array of pointers to other service definitions
// UUID - Universal Unique Identifier
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),                 // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_command},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
           {.uuid = BLE_UUID16_DECLARE(0xDEAF),           // Define UUID for writing
           .val_handle = &HR,
          .flags = BLE_GATT_CHR_F_NOTIFY,
          .access_cb = device_notify},
         {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        conn_handle = event->connect.conn_handle;
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "disconnect; reason=%d\n", event->disconnect.reason);

        /* Connection terminated; resume advertising */
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI("GAP", "subscribe event; cur_notify=%d\n value handle; "
                    "val_handle=%d\n",
                    event->subscribe.cur_notify, HR);
        if (event->subscribe.attr_handle == HR) {
            notify_state = event->subscribe.cur_notify;
            //blehr_tx_hrate_reset();
        } else if (event->subscribe.attr_handle != HR) {
            notify_state = event->subscribe.cur_notify;
            //blehr_tx_hrate_stop();
        }
        ESP_LOGI("GAP", "conn_handle from subscribe=%d", conn_handle);
        break;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.value);
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

#ifdef VALTRACK_V4_VTS
const char *TAG = "VALTRACK-V4-VTS";
#else
const char *TAG = "VALTRACK-V4-MF";
#endif


#define ECHO_TASK_STACK_SIZE    (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define TIMER_TASK_STACK_SIZE   (CONFIG_EXAMPLE_TASK_STACK_SIZE)`

// UART1 TX-----0
// UART1 RX-----1
// ANALOG IN----2
// INT1---------3
// TPS-ENABLE---4
// IIC-DATA-----5
// IIC-CLOCK----6
// PWRKEY-------7
// LED-SIGNAL---8
// SWITCH-SW2---9
// GSM-ENABLE---10
// USB DN-------18
// USB_DP-------19
// UART0 RX-----20
// UART0 TX-----21




#define EEPROM_FIFO


void SystemClock_Config(void);
void Error_Handler(void);
void DeepSleep (void);
void InitRTCAlarm(void);

const StringType ETypes[]=
{
    {"EVENT_ENGINE_OFF"}                     ,   // 0  00 
    {"EVENT_ENGINE_ON"}                     ,    // 1  01 
    {"REGULAR_TEST"}                        ,    // 2  02 
    {"RANDOM_TEST"}                         ,    // 3  03 
    {"EVENT_BYPASS"}                        ,    // 4  04 
    {"EVENT_LOG_CLEARED"}                   ,    // 5  05 
    {"EVENT_SERVICE_PEROD_SET"}             ,    // 6  06 
    {"EVENT_RESET_FOR_VIOLATION"}           ,    // 7  07 
    {"EVENT_USE_ONE_TIME_CODE"}             ,    // 8  08 
    {"EVENT_RE_DO"}                         ,    // 9  09 
    {"EVENT_12V_POWER_REMOVED"}             ,    // 10  0A
    {"EVENT_LOG_READ"}                      ,    // 11  0B
    {"EVENT_CONFIG_CHANGED"}                ,    // 12  0C
    {"EVENT_LOG_FULL"}                      ,    // 13  0D
    {"EVENT_SERVICE_ALERT"}                 ,    // 14  0E
    {"not_used"}                            ,    // 15  0F
    {"EVENT_WEAK_BLOW"}                     ,    // 16  10
    {"EVENT_WARNING_FAIL_RR"}               ,    // 17  11
    {"EVENT_WARNING_REFUSED"}               ,    // 18  12
    {"EVENT_WARNING_SERVICE_PERIOD_END"}    ,    // 19  13
    {"EVENT_CAR_BATTERY_ON"}                ,    // 20  14
    {"EVENT_CAR_BATTERY_OFF"}               ,    // 21  15
    {"EVENT_CALIBATION_DONE"}               ,    // 22  16
    {"EVENT_IGNITION_KEYED"}                ,    // 23  17
    {"EVENT_WARNING_GIVEN"}                 ,    // 24  18
    {"EVENT_STATER_NOT_ACTIVE"}             ,    // 25  19
    {"EVENT_INSUFFICIENT_BLOW"}             ,    // 26  1A
    {"EVENT_COOL_SAMPLE"}                   ,    // 27  1B
    {"EVENT_TAMPERED"}                      ,    // 28  1C
    {"EVENT_START_TEST_ATTEMPT"}            ,    // 29  1D
    {"EVENT_AB_FC_CONNECTED"}               ,    // 30  1E
    {"EVENT_AB_FC_REMOVED"}	              ,    // 31  1F
    {"EVENT_CAL_CHK_PASS"}	              ,    // 32  20
    {"EVENT_CAL_CHK_FAIL"}	              ,    // 33  21
    {"EVENT_ENGINE_NOT_STARTED"}			  ,    // 34  22
    {"G_PING"}							  ,	   // 35  23
    {"GPRS_PING"}							  ,	   // 36  24
    {"MOTION"}					          ,	   // 37  25
    {"REBOOT"}					          ,	   // 38  26
    {"SOS"}					              ,	   // 39  27
    
};
const StringType BootReasons[]= 
{
    {"RTC_RESET"},
    {"INT1_RESET"},
    {"BUFF2_RESET"},
    {"NETWORK_RESET"},
    {"LPUART_TIMER_RESET"},
    {"MOTION_RESET"},
    {"HARDFAULT_RESET"},
    {"DEEP_SLEEP_RESET"},
    {"CHARGER_RESET"}

};
const StringType Results[] = 
{
    {"FAIL"},
    {"PASS"},
    {"NA"}
};
const EngineStatusStringType EngineStatusStrings[]=
{
    {"OFF"}                     ,   // 0  00 
    {"ON"}                      ,    // 1  01 
};
/* USER CODE END 0 */


char tBuff[TBUFF_SIZE];
unsigned char x,SMSSent=1;
extern unsigned char RingCount;

unsigned char NeedBTAttention=0;
unsigned short millis = 0;
unsigned short tSeconds;
unsigned short SystemTimer;
unsigned short InactivityTimer;
unsigned long  IntervalTimer;
volatile int   force_ping_now = 0; // set by PING_NOW remote command; cleared after next ping
uint8_t ota_channel = OTA_CHANNEL_PRODUCTION; // OTA source; loaded from NVS at boot (2.3.51)
unsigned short MotionTimer=0;
uint32_t ParkLongTimer = 0;      // seconds since last motion; 48hr threshold → deep sleep
/* Motion confirmation for ParkLongTimer (2.3.52, ISSUES.md D1).
   ParkLongTimer used to be zeroed by ANY single INT1 assertion, so deep sleep
   needed 48 CONSECUTIVE hours with not one interrupt - unreachable anywhere
   with occasional vibration. Two undisturbed bench units ran 55h without
   sleeping (ipoll 47 and 19). These track distinct assertions inside a rolling
   window so an isolated knock decays instead of discarding 48h of stillness.
   MotionTimer is deliberately NOT debounced - it drives the 30s/5min adaptive
   cadence, where reacting to the first movement is correct. */
static uint32_t motion_events      = 0;  // distinct assertions in the current window
static uint32_t motion_window_left = 0;  // seconds remaining in that window
/* Matches SystemTimer's type (unsigned short). It wraps every ~18.2h and is
   zeroed at main.c:8386; both are harmless here, since a false "same second"
   only skips one count toward a confirmation that real motion re-earns within
   seconds. */
static unsigned short motion_last_second = 0; // de-dupes passes of one latched event
static uint32_t park_reset_count   = 0;  // times motion was CONFIRMED (reported as pltr)
static int heartbeat_wake = 0;   // 1 when woken by 8hr timer; cleared after first ping
uint32_t ota_check_timer = 0;   // seconds since last OTA check; 24hr periodic check
float last_good_lat = 0.0f; // last GPS fix — persisted to NVS, survives reboots
float last_good_lon = 0.0f;

/* GNSS health tracking (2.3.33).
   Field failure 2026-07-25/26: both units acquired normally after boot, then
   the modem's GNSS stopped returning fixes mid-session and never recovered.
   AT+CGNSSPWR=1 was only ever issued from InitGSM(), so with the network up
   (no restart trigger) the devices ran 4 days replaying one stale position.
   last_fix_us drives the gpsage attribute and the staleness cutoff; the
   no-fix counter drives escalating GNSS recovery in XCheckGPS. */
static int64_t last_fix_us = 0;         // esp_timer time of the last valid GPS fix; 0 = none this session
static int64_t gnss_last_action_us = 0; // boot / last fix / last recovery attempt — escalation window start

/* Frozen-clock detection (2.3.40).
   A second GNSS failure mode, distinct from the empty-response case 2.3.33
   handles: the modem keeps answering AT+CGPSINFO with a well-formed sentence
   whose contents never change. GPSStatus reads 'A', fLat/fLong are non-zero, so
   the position is reported as a live fix and nothing downstream can tell.
   Field case 2026-07-30: the van's GPS clock stopped at 09:14:05 UTC and the
   same fix was resent for 13h49m across 202 pings. A drive inside that window
   recorded no track samples, no speed and no trip - and gpsage never appeared,
   because gpsage is only emitted when live_fix is false.
   A healthy receiver reports current UTC on every poll, so a GPS clock that has
   not advanced in this long means the fix is stale however valid it looks. */
static long    last_gps_ts = 0;          // GPS unix ts from the previous poll
static int64_t last_gps_ts_change_us = 0;// esp_timer time that ts last changed
static bool    gnss_frozen = false;      // clock stopped advancing = receiver definitely broken
#define GPS_FROZEN_SECONDS  120
static int    gnss_recover_stage = 0;   // 0=none, 1=power-cycled, 2=cold-start, 3=modem reinit
static uint32_t gnss_recover_count = 0; // recovery attempts since boot — reported as gpsrec

/* No-reply detection (2.3.42) — the third variant of the stale-fix family, and
   the one that silently disabled the fixes for the other two.
   AT+CGPSINFO can also come back with no "+CGPSINFO:" line at all: a timeout
   (SendATCommand returns 3) or a bare OK. XCheckGPS used to default GPSStatus
   to 'A' and demote it only on the two literal strings "+CGPSINFO: ,," and
   "ERROR", then do all its work inside `if (pToken != NULL)`. So a missing
   reply was read as a valid fix AND skipped the whole body — the 2.3.40
   frozen-clock check never evaluated, the no-fix path never zeroed fLat/fLong
   (file-scope, so they kept their last values), and GNSSRecover() was never
   called. live_fix stayed true, so gpsage/gpsrec were never emitted either:
   the device reported a confident live fix that was days old and said nothing.
   Field case 2026-08-01: the van froze at 08:24:49 NZST and resent that fix for
   50h across 440 pings, straight through two drives, with no attribute anywhere
   showing a fault; unit -5783 froze the same way on 07-31. Absence of evidence
   is no longer treated as a good fix — 'V' is the default. */
static uint32_t gnss_no_reply_count = 0; // polls with no parseable reply — reported as gpsnr
static int64_t gnss_no_reply_since_us = 0; // start of the current no-reply run; 0 = modem answering
static int    gnss_backoff_mult = 1;    // window multiplier, doubles per failed recovery, reset on fix

/* A cached position older than this is worse than the cell-tower fix, which is
   ~550m but current. Below it, the last GPS fix still wins. */
#define GPS_STALE_SECONDS   600

/* Time without a fix before the next recovery stage fires. Escalation is driven
   by elapsed time, not poll count: XCheckGPS runs once per main-loop pass and
   that period swings from ~1s when idle to minutes when a ping is in flight.
   Two windows, because cold and warm acquisition differ by an order of
   magnitude. Measured on the van 2026-07-27: after a V_RESET following 4 days
   powered-off GNSS, time-to-first-fix was ~586s. A single 600s window would
   have fired a power-cycle just before that fix landed and restarted the clock
   — potentially looping forever without ever acquiring. A receiver that has
   already fixed this session reacquires in seconds, so the warm window stays
   tight enough to catch a real wedge quickly. */
#define GNSS_RECOVER_AFTER_S    600   /* warm — had a fix this session */
#define GNSS_COLDSTART_GRACE_S 1800   /* cold — no fix yet this session (TTFF ~10min observed) */
#define GNSS_BACKOFF_MAX          8   /* window multiplier ceiling: 600s*8 = 80min between attempts */

/* 1-second GPS track buffer. Samples recorded while moving; drained as a
   batch inside the ping HTTP session so track resolution is 1s while the
   radio only does full session setup once per ping interval.
   Single producer (timer task) / single consumer (main task) ring.

   2.3.47 - the "1s" above was aspiration, not fact. Measured on the van
   (2026-08-06 drive, sample timestamps straight out of the Traccar log):
   4-5s apart within a burst and 11-21s across ping boundaries, averaging
   ~4.6s. TrackSampleTick() does run at 1Hz, but it discards every tick where
   the GPS second has not advanced - and the GPS second only advances when
   XCheckGPS() polls the modem, which happens once per main-loop pass. So the
   real sampling interval is the main-loop period, and that is dominated by
   fixed osDelay()s in the AT helpers rather than by anything GNSS-related.
   See the CheckNetwork throttle in the main loop for the fix.

   tqd/tdrp exist to answer the question that opens up next: if sampling gets
   faster than the drain can deliver (one HTTPPARA + HTTPACTION round trip per
   sample), the buffer backs up and then silently overwrites. Measure before
   pushing the rate any further. */
#define TRACK_SAMPLE_SECONDS 1
#define TRACK_BUF_SIZE       256  // ~4 min of moving data if sends fail
typedef struct {
    float lat, lon, speed;
    unsigned char yy, mo, dd, hh, mi, ss;  // GPS UTC at sample time
} TrackSample;
static TrackSample track_buf[TRACK_BUF_SIZE];
static volatile unsigned short track_head = 0, track_tail = 0;
static float track_last_lat = 0.0f, track_last_lon = 0.0f;
static float track_live_speed_kmh = 0.0f; // latest track-derived speed; live ping reports it
static int64_t track_live_speed_us = 0;   // esp_timer time of that sample
static long track_prev_ts = 0;            // GPS unix ts of previous recorded sample
static unsigned long track_drop_count = 0; // samples lost to buffer wrap (tdrp)
static long osmand_unix_ts(int yy, int mo, int dd, int hh, int mi, int ss);

/* Ignition + external-power sensing from the main supply voltage (VCHG ADC).
   Alternator lifts the vehicle bus above ~13.3V when the engine runs; engine
   off sits ~12.x V; main power cut drops below ~1V (backup LiPo keeps the
   device alive). Reported as ignition=true/false; transitions of external
   power send alarm=powerCut / alarm=powerRestored on an immediate ping. */
static int ign_on = 0;        // debounced ignition state
static int extpwr_on = -1;    // -1 until first debounced reading after boot
static volatile int power_alarm = 0; // 1=powerCut, 2=powerRestored; cleared after successful send

/* Harsh driving detection (Phase 7b). Since 2.3.37 the threshold and duration
   test lives in the LIS3DH's second interrupt generator, not in a sampler task;
   the firmware only classifies the event when the sensor reports one. Delivery
   is the same pattern as power_alarm: rides a forced ping, cleared on success. */
static volatile int harsh_alarm = 0;        // 1=hardBraking 2=hardAcceleration 3=hardCornering
static int harsh_alarm_in_flight = 0;       // set while a harsh alarm is in the URL being sent

unsigned short HeartBeatTimer = 0;
unsigned short NoSignalTimer=0;
unsigned short ButtonPressTimer=0;
unsigned short AuthenticationTimer = 0;
unsigned short BatteryCheckTimer=950;
unsigned char BatteryTimeout=0;
unsigned short minVal,maxVal;
unsigned char ADCRunning = 0;
char savedNumber[11];
char rxNumber[16];
unsigned char ActivityLevel = 0;
unsigned short AlertTimer,SOSTimer,BatteryTimer,DebounceTimer,GSMResetTimer,LPUARTTimer,ConnectivityTimer=0;
int FrontPanelTimer=0;
unsigned short EEPROMReadTimer = 0;
unsigned char ticks;
unsigned char DeviceStatus = 1;
unsigned char SysClockConfigFlag=0;
unsigned char SleepModeEnabled=0,RTCSleepModeEnabled = 0;
unsigned char PowerButtonSleep = 0;

int ChargeStatus,ChargeLevel,ChargeVoltage,ChargingState;
float ChargeVoltageF=0;
unsigned long RTCTimeout=0;
const char ATCmgsToken[] = {"AT+CMGS=\""};
const char SetMsgTypeCmd[] = {"AT+CMGF=1\r\n"};
const char NewLineCmd[] = {"\r\n"};
const char NewLine2Cmd[] = {"\"\r\n"};
//unsigned char Count;


unsigned char FirstTime=0;
unsigned char SMSNumber=0;
unsigned char GPSAwake=0;

char tmpbuff[100];
unsigned char tmpvar;
uint16_t Counter = 0;
// HAL_StatusTypeDef Result = HAL_OK;
unsigned char VALREAD;
unsigned char iAddr; 

MotionStatusType INT1;
unsigned char SOS,POWER_BUTTON;
unsigned char GSM_STATUS;
unsigned char PG_STATUS;
unsigned char CHG_STATUS;

RTC_TimeTypeDef R;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;
unsigned char StandbyReset=0;
unsigned char LEDInhibit = 1;
RTC_DATA_ATTR BootReasonType BootReason;

unsigned char AlignInterval = 1;


#define DEVICE_ID "864287038316376"
//#define WATCHDOG_ENABLED
//#define SOSALERT


unsigned char SosAlert=0;

//#define ACCLEROMETER_I2C_ADDRESS 0x1D<<1   //MMA854

unsigned char ClockSource=0;

const char SYS_VERSION[]="VALTRACK-V4-21-02-25";

char devid[16] = DEVICE_ID;
char bstr[250];

#ifdef VALETRON_SYSTEMS

const char CUSTOMER[]="VALETRON_SYSTEMS";
#endif

unsigned char LoadDefaultParams = 0;
///NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
#ifdef PARAMS_NORMAL
const ParamsType DefaultParams = {
    
    .Fields=
    {
         10,//unsigned int PingInterval;
        "HTTP",//"TCP",//char WorkingMode[5];
        "NONE",//"CALL",//char MotionAlertMode[5];
        0x04,//char MotionThreshold;
        "http://domain.com/api/update",//char HTTPURL[150];,
        "Your-Api-Key: 1234456789",///char HTTPKey[100];
        "www",//"iot.1nce.net",//char APNName[20];
        "",//char APNUsername[20];
        "",//char APNPassword[20];
        "EGSM_MODE,ALL_BAND",//char Band[30];
        "1234567890",//char rxNumber[16];
        "test.mosquitto.com",//char MQTTHost[30];
        "1883",//char MQTTPort[10];
        "708a964577c84f5abac",//char MQTTClientID[20];
        "valtrack",//char MQTTTopic[30];
        "MQIsdp",//char MQTTProtocolName[10];
        0x03,//unsigned char MQTTLVL;
        0xC2,//unsigned char MQTTFlags;
        60,//unsigned int MQTTKeepAlive;
        "txzborlq",//char MQTTUsername[30];
        "Rr-kEclgx_M2"//char MQTTPassword[35];
    }
};
#endif
//NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
//ParamsType Params;


MotionStatusType INT1;



#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
/* Restored to 1000 in 2.3.33. Dropped to 50 for the 20Hz HarshDriveTask so a
   stuck bus couldn't stall the sampler; that task is compiled out since 2.3.32,
   but the short timeout stayed and still governs the LIS3DH and the EEPROM
   sharing this bus. */
#define I2C_MASTER_TIMEOUT_MS       1000

//#define MPU9250_SENSOR_ADDR                 0x68        /*!< Slave address of the MPU9250 sensor */
#define ACCLEROMETER_WHO_AM_I_REG_ADDR           0x0F        /*!< Register addresses of the "who am I" register */

#define MPU9250_PWR_MGMT_1_REG_ADDR         0x6B        /*!< Register addresses of the power managment register */
#define MPU9250_RESET_BIT                   7





unsigned long Count,Count2,LoopTimeout1,LoopTimeout2,LoopTimeout3;

int echo = 1;
int counter=0;
int temp;
unsigned char retVal;


char Speed[10]="",tSpeed[10]="";;


unsigned long PLTime,LTime,LDate;
double tfLat,tfLong;    
unsigned int fuel;
unsigned char QueEmpty;


 uint16_t  ADC1C7 = 0;
 uint16_t  ADC1C6 = 0;
 uint16_t  ADC1C5 = 0;

#define ADC1_DR_Address                0x40012440


 uint16_t RegularConvData_Tab[3];


unsigned char ISRstatus;
char RFIDCardNumber[20];
char ValidRFIDCardNumber[20];
unsigned char ValidRFIDCardLength;

unsigned char ImageSent = 0, ImageIndex = 0,ImagePresent=0,SendingImageIndex=0,ImageStored=0;    
//IDPacketType DeviceID;
GeneralEventType GEvent;
char IMEI[16],IMSI[16],/*time[21],ctime[21],*/etype,*presult,bac,filename[32];
char ble_device_name[32];
//PackedGeneralEventType PGEvent;
HWEventDataType CPacket,GPacket;
const char dummyevent[]={"0D00000000000000000033002300000000"};
unsigned long CheckSum,RLength;
unsigned short datalength;
unsigned short topiclength;
char topic[40];

HWEventDataType *pPacket;
char str[2500];
EventCodeType EventType;
char ImageRetryCount[64];
char Version[50];
char RFID[50];
char query[30];
unsigned char RFIDDataPresent;
unsigned char EngineStatus=0;

void XCheckGPS(void);


unsigned char MCU_ACOK,MCU_CHGOK,EnableCharge=0;
char Test1='C';
char ACCTest='T';
unsigned char LED=0,GSP=0;


extern unsigned char GSM_STATUS;
extern unsigned char PG_STATUS;
extern unsigned char CHG_STATUS;
 
extern unsigned short millis;
extern unsigned short tSeconds;
extern unsigned short InactivityTimer;
extern unsigned short MotionTimer;
extern unsigned short NoSignalTimer;
extern unsigned char SMSSent;
extern unsigned short BatteryCheckTimer;
extern unsigned char BatteryTimeout;
extern int FrontPanelTimer;

//unsigned short ButtonPressTimer = 0;
unsigned char pSOS = 0;
unsigned char tmpFlagClearer;
unsigned char BTInitialized = 0;



unsigned long lastTickValue=0;
//unsigned long ButtonLastTickValue=0;
char MResponsePacket[2] = {0x01,0x00};
extern unsigned char AppAuthenticated;
void DisconnectDevice(void);

unsigned short BTReceiveTimer = 0;
unsigned char SOSActivated = 0;
extern unsigned char AnswerCall;
/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
static esp_err_t motion_sensor_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, ACCLEROMETER_I2C_ADDRESS, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Write a byte to a MPU9250 sensor register
 */
static esp_err_t motion_sensor_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, ACCLEROMETER_I2C_ADDRESS, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    return ret;
}


void I2C_WrReg(uint8_t Reg, uint8_t Data)
{
    motion_sensor_register_write_byte(Reg,Data);

    
    //osDelay(100);
}
uint8_t I2C_RdReg(uint8_t Reg)
{
    //HAL_StatusTypeDef tResult = HAL_OK;
    uint8_t Data;
    motion_sensor_register_read(Reg,&Data,1);
    

    //osDelay(100);
    
    return Data;
    
}
typedef enum SensorTypes
{
    NOT_DETECTED = 0,
    MMA8652_SENSOR,
    MMA8653_SENSOR,
    MMA8452_SENSOR,
    LIS3DH_SENSOR
}SensorType;
SensorType MotionSensor = NOT_DETECTED;
unsigned char VALREAD=0;
/* Forward declarations (2.3.44). The harsh statics and HarshEventDetected live
   below this function, but InitAccelerometer now has to check for a pending
   event before it clobbers the generator - see the INT2_SRC block near the end. */
#ifdef ENABLE_HARSH_DRIVING
static void HarshEventDetected(void);
static uint32_t accel_poll_count = 0;   // reported as apoll
static unsigned char harsh_armed = 0;   // generator 2 configured at least once
#endif

void InitAccelerometer_LIS3D(void)
{
     unsigned char i;
    char ReadBuff[100];
    
  
//    I2C_WrReg(MMA8652_CTRL_REG2, RST_MASK);
    //DDelay();
    VALREAD = I2C_RdReg(0x0F);
    
   if(VALREAD == 0x33)
    {
        MotionSensor = LIS3DH_SENSOR;
        ESP_LOGI(TAG, "LIS3DH_SENSOR");
    }
    if(VALREAD == 0x4A)
    {
        MotionSensor = MMA8652_SENSOR;
        ESP_LOGI(TAG, "MMA8652_SENSOR");
    }
    if(VALREAD == 0x5A)
    {
        MotionSensor = MMA8653_SENSOR;
        ESP_LOGI(TAG, "MMA8653_SENSOR");
    }
    if(VALREAD == 0x2A)
    {
        MotionSensor = MMA8452_SENSOR;
        ESP_LOGI(TAG, "MMA8452_SENSOR");
    }


    /* The read of 0x26 (REFERENCE) that used to be here is removed in 2.3.39 -
       it reset the high-pass filter on every call. HPM=10 below now makes that
       harmless anyway, but there is no reason to do it. */
    VALREAD = I2C_RdReg(0x0F);//VALREAD = I2C_RdReg(0x0D);
    /* 2.3.48: derived from LIS3DH_ODR_HZ in SCI.h (was a bare 0x57 = 100Hz).
       ODR sets the high-pass cutoff and the DURATION tick as well as the sample
       rate, so it must not be edited here independently of the HARSH_* maths. */
    I2C_WrReg(REG_CTRL_REG1, LIS3DH_CTRL_REG1);// LPEN bit 3
    I2C_WrReg(REG_CTRL_REG4, 0x08);// HR bit 3
    
    osDelay(200);
   //  VALREAD = I2C_RdReg(REG_CTRL_REG1);
#ifdef ENABLE_HARSH_DRIVING
    /* High-pass filter feeding BOTH interrupt generators (HP_IA1 | HP_IA2) plus
       HPCLICK, so neither generator sees the 1g of gravity - thresholds apply to
       dynamic acceleration only. Without HP_IA2 the constant 1g would hold
       generator 2 permanently triggered.

       0x37, not 0x07: bits 5:4 are HPCF2:HPCF1, the filter cutoff. 2.3.37 left
       them at 00, which is the HIGHEST cutoff - about 2Hz at our 100Hz ODR. That
       was a mistake. Harsh braking and cornering are SUSTAINED forces lasting
       several hundred ms, i.e. low frequency (~1.7Hz for a 300ms event), so a
       2Hz high-pass attenuated the very signal being measured: a real 0.5g brake
       could reach the comparator as ~0.25g and never cross the 400mg threshold.
       Field-confirmed on 2.3.37 - hcnt stayed 0 across 44 ignition-on pings with
       driving up to 76km/h.
       11 selects the lowest cutoff available for a given ODR.

       2.3.48 - the sentence that used to end this paragraph ("passes anything
       shorter than roughly four seconds essentially intact") was WRONG, and it
       is why the filter went unsuspected for four versions. HPCF=11 is ODR/400,
       so at the old 100Hz ODR the cutoff was 0.25Hz - a 0.64s time constant,
       the same order as a real brake. Braking is a ramp, not a step: a ~1s ramp
       to 0.4g arrives at the comparator as roughly 0.15-0.2g. Field-proven on
       2.3.47 - a measured 0.409g stop left hraw at 0 while a vertical bump
       (high-frequency, passes intact) triggered fine.
       The cutoff is not set here; it follows ODR in CTRL_REG1. 25Hz puts it at
       0.0625Hz (tau 2.5s). See LIS3DH_ODR_HZ in SCI.h.

       Bits 7:6 are HPM, the filter mode. 2.3.38 left them at 00 - "normal mode,
       filter reset by reading REFERENCE" - which was the real 2.3.37/2.3.38
       failure: InitAccelerometer() reads REFERENCE (0x26), runs about once a
       second while driving because motion interrupts fire continuously, and so
       slammed the filter output back to zero before it could ever integrate the
       300ms needed to trigger. 10 selects plain normal mode, where reading
       REFERENCE has no such side effect. */
    I2C_WrReg(REG_CTRL_REG2, 0xB7);
    /* 0x60: route generator 1 (I1_IA1) AND generator 2 (I1_IA2) to the single
       physical INT1 pin. Only one interrupt line is wired to the ESP32, so both
       share it and INT1_SRC/INT2_SRC are read to tell them apart. */
    I2C_WrReg(REG_CTRL_REG3, 0x60);
    /* 0x0A: latch both (LIR_INT1 | LIR_INT2). Latching is what makes a slow poll
       safe - a 300ms event holds the pin until its SRC register is read, so it
       cannot be missed between polls however far apart they are.

       This comment used to say "the ~1Hz poll in StartMainTask". That rate was
       never measured and is wrong by a factor of ~650: the 2.3.43 counters put
       it at one poll every ~11 minutes on the driven van. Latching still saves
       us, but ONLY because nothing clears the latch without checking it first -
       which was not true until 2.3.44. Do not reintroduce a blind INT2_SRC
       read anywhere. */
    I2C_WrReg(REG_CTRL_REG5, 0x0A);
#else
    I2C_WrReg(REG_CTRL_REG2, 0x05);
    I2C_WrReg(REG_CTRL_REG3, 0x40);//    I2C_WrReg(MMA8652_CTRL_REG3, 0x39);

    I2C_WrReg(REG_CTRL_REG5, 0x08);
#endif
   // VALREAD = I2C_RdReg(REG_CTRL_REG5);
    I2C_WrReg(REG_CTRL_REG6, 0x02);
    //I2C_WrReg(REG_CTRL_REG6, 0xFF);
    I2C_WrReg(REG_INT1_THS,Params.Fields.MotionThreshold);
    I2C_WrReg(REG_INT1_DURATION,0x00);
    I2C_WrReg(REG_INT1_CFG,0x2A);
    I2C_RdReg(REG_INT1_SRC); // Clear any latched interrupt so INT1 pin is de-asserted before sleep

#ifdef ENABLE_HARSH_DRIVING
    /* Generator 2 = harsh driving, entirely in hardware. THS/DURATION/CFG are
       all derived from the HARSH_* macros in SCI.h. HARSH_INT2_CFG is 0x0A
       since 2.3.49 - XHIE|YHIE with OR combination, so a horizontal excursion
       on either axis fires. ZHIE was dropped because road-surface vertical
       jolts were swamping the signal; see SCI.h for the evidence. */
    /* THE FIX (2.3.44). This used to end with a blind `I2C_RdReg(REG_INT2_SRC);
       // clear latch` - and that single line is why hraw stayed 0 for six
       versions.

       Measured 2026-08-04 with the 2.3.43 counters: the StartMainTask INT1 poll
       runs about ONCE EVERY 11 MINUTES on the driven van (ipoll=98 over 17.9h
       including a drive at 82 km/h), not the ~1Hz every comment in this file
       claimed. This function is throttled to every 60 seconds. So the latch was
       being destroyed 11 times for every time anything looked at it - a harsh
       event was latched, then wiped long before the poll came round.

       Reading INT2_SRC is itself what clears the latch, so the read has to
       happen BEFORE the generator is rewritten and its result has to be acted
       on rather than discarded. That turns this function from the thing that
       destroyed events into the fastest poll in the system (60s), and because
       the interrupt is latched, a 60s poll cannot miss anything - the rate now
       only affects reporting latency, not detection.

       harsh_armed suppresses the very first call, where INT2_SRC holds whatever
       the sensor powered up with rather than a real event. */
    {
        unsigned char pending = I2C_RdReg(REG_INT2_SRC);   // read == clear latch
        accel_poll_count++;
        if (harsh_armed && (pending & 0x40))
            HarshEventDetected();
    }
    I2C_WrReg(REG_INT2_THS, HARSH_THS_COUNTS);
    I2C_WrReg(REG_INT2_DURATION, HARSH_DUR_COUNTS);
    I2C_WrReg(REG_INT2_CFG, HARSH_INT2_CFG);
    harsh_armed = 1;
#endif

    /* The debug sweep that used to live here - reading every register 0x07-0x3F
       into a buffer that was then discarded (ReadBuff[0]=ReadBuff[0];) - is gone
       in 2.3.39. Besides costing 57 pointless I2C reads on every call, it read:
         0x26 REFERENCE  - resetting the high-pass filter
         0x31 INT1_SRC   - clearing the motion latch
         0x35 INT2_SRC   - clearing a pending HARSH EVENT before anyone saw it
       Since this function runs on every motion interrupt, i.e. about once a
       second while driving, that combination made harsh detection impossible. */
}



void InitAccelerometer_MMA8XXX(void)
{
   
  
}
void InitAccelerometer(void)
{
    #ifdef LIS3DH_ENABLED
        InitAccelerometer_LIS3D();
    #else
        InitAccelerometer_MMA8XXX();
    #endif
    
    
}
/////////////////////////////////////////I2C

unsigned char SendATCommand(char *pCommand,char *pResponse1,char *pResponse2, unsigned short Timeout)
{
    unsigned char Response1Length,Response2Length;
    //osDelay(2000); // Between command delay
    osDelay(200);
    Response1Length = strlen((void*)pResponse1);
    Response2Length = strlen((void*)pResponse2);
    ResetBuffer();
    Print(pCommand);
    LoopTimeout1 = 0;
    {
        TickType_t _deadline = xTaskGetTickCount() + pdMS_TO_TICKS((uint32_t)Timeout * 1000);
        while(1)
        {
            vTaskDelay(1);
            if(MapForward(Buff2,BUFF2_SIZE,(char*)pResponse1,Response1Length) != NULL)
                return 1;
            if(MapForward(Buff2,BUFF2_SIZE,(char*)pResponse2,Response2Length) != NULL)
                return 2;
            if(xTaskGetTickCount() >= _deadline)
                return 3;
        }
    }

}
void ForceToSleep(void)
{
    ClearPackets();
    MotionTimer=TIME_TO_SLEEP+1;

}
char BatteryString[25];
void CheckBattery(void)
{
    char*pToken;
    unsigned char i;

                         
    i=0;
 
    SendATCommand("AT+CBC\r\n","+CBC:","ERROR",5);
    //#ifdef SIM800
    osDelay(500);
    //#endif
    pToken = MapForward(Buff2,70,(char*)"+CBC:",5);
    //pToken+=5;
    if(pToken != NULL)
    {
        
        //while(*pToken != '"')pToken--;
        //pToken++;
        
        // save sender number
        while(pToken[i] != '\n')
        {
            BatteryString[i] = pToken[i];
            i++;
            if(i>=20)break;
        }
        BatteryString[i] = '\0';
        BatteryString[20] = '\0';
        
        #ifdef SIM800
            sscanf( (void*)pToken, "+CBC: %d,%d,%d\r", &ChargeStatus,&ChargeLevel,&ChargeVoltage);
            ChargeVoltageF = ((float)ChargeVoltage/1000);
        #else
            sscanf( (void*)pToken, "+CBC: %f\r", &ChargeVoltageF);
        #endif
    }
    ChargeVoltageF = ADCBatteryVoltage; //OVERRIDING WITH ADC DATA. May be add app control later
   
 
        
    
}
char SignalStrength[25];
void CheckSignalStrength(void)
{
    char*pToken;
    unsigned char i;

  
    i=0;
    SendATCommand("AT+CSQ\r\n","+CSQ:","ERROR",5);
    //#ifdef SIM800
        osDelay(500);
    //#endif
    pToken = MapForward(Buff2,70,(char*)"+CSQ:",5);
    
    if(pToken != NULL)
    {
        pToken+=6;    
        //while(*pToken != '"')pToken--;
        //pToken++;
        
        // save sender number
        while(pToken[i] != '\r')
        {
            SignalStrength[i] = pToken[i];
            i++;
            if(i>=20)break;
        }
        //SignalStrength[2] = '/';
        SignalStrength[i] = '\0';
        SignalStrength[20] = '\0';
        

    }

    
}
//float nLat=0,nLon=0;
float NLat,NLong;
NetworkLocationStatusType NStatus;
int NAccuracy;
char NLPacket[80];

/* A7672G +CLBS prints negative coordinates uint32-wrapped (deg*1e6 stored
   unsigned, then printed /1e6): southern/western hemisphere values come out
   as true + 2^32/1e6 = true + 4294.967296. Confirmed against 4 field samples
   2026-07-19, all decode within CLBS's own reported accuracy. Must stay in
   double until after the subtraction — float32's ~7 significant digits can't
   hold a wrapped value like 4254.606934. */
static float clbs_coord(double v, double limit)
{
    if (v > limit) v -= 4294.967296; /* 2^32 / 1e6 */
    return (float)v;
}
char TowerPacket[100];
void CheckNetworkLocation(void)
{
    char*pToken;
    //unsigned char i;

    CheckSignalStrength();
    //i=0;
    

    SendATCommand((void*)"AT+CLBS=1\r\n","+CLBS:","ERROR",50);
    //#ifdef SIM800
        osDelay(500);
    //#endif
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CLBS:",6);
    if(pToken != NULL)
    {
        sscanf((void*)pToken,"+CLBS: %d",(int*)&NStatus);
        if(NStatus == NL_SUCCESS) /* non-zero (e.g. +CLBS: 10) = no coordinates, skip */
        {
            double dlat = 0, dlon = 0;
            pToken+=6;
            //osDelay(1000);//DelayProc(50000);
            sscanf((void*)pToken,"%d,%lf,%lf,%d",(int*)&NStatus,&dlat,&dlon,(int*)&NAccuracy);
            NLat  = clbs_coord(dlat, 90.0);
            NLong = clbs_coord(dlon, 180.0);
            snprintf((void*)NLPacket,_countof(NLPacket),",\"nlat\":\"%f\",\"nlon\":\"%f\",\"ncsq\":\"%s\"",NLat,NLong,SignalStrength);
            //sprintf((void*)NLPacket,",\"nlat\":\"%f\",\"nlon\":\"%f\",\"ltype\":\"NL\"",NLat,NLong);
        }
    }
    
        
    
}

void  Binary2Ascii(unsigned long HexValue);
int GetLength(char *p)
{
   int Length;
   Length=0;
   while(*p != '\0')
   {
     Length++;
     p++;
   }
   
   return Length;
  
}
void WakeUp(void);
void CheckAndApplyOTA(void);
void nvs_save_position(void);
void nvs_load_position(void);
void ota_channel_load(void);
void ota_channel_save(uint8_t ch);




unsigned char GSMEnabled = 0;
unsigned char GPSEnabled = 0;
void EnableGSM(void)
{
    // Enable power to GSM
    gpio_set_level(GPIO_GSM_ENABLE, 1); 
    GSMEnabled = 1;
    //osDelay(8000);
} 

void DisableGSM(void)
{
    // #ifdef SIM7600
    // SendATCommand("AT+CPOF\r\n","OK","ERROR",10);// Disconnect network and shutdown
    // #endif
    // #ifdef SIM7070
    // SendATCommand("AT+CPOWD=1\r\n","NORMAL","ERROR",10);// Disconnect network and shutdown
    // #endif

    //osDelay(1000);
    // Disable power to GSM
    gpio_set_level(GPIO_GSM_ENABLE, 0);   
    GSMEnabled = 0;
    //osDelay(3000);
    
}
void EnableMainPower(void)
{
    #ifdef ENABLE_TPS_CONTROL
    // Disable power to GSM
    gpio_set_level(GPIO_TPS_ENABLE, 1);   
    #endif
        
}
void DisableMainPower(void)
{
    #ifdef ENABLE_TPS_CONTROL
    // Disable power to GSM
    gpio_set_level(GPIO_TPS_ENABLE, 0);   
    #endif    
    
}
// void EnableCharger(void)
// {
//     // Enable Battery charger -> LOW = ENABLE
//     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_RESET); //EN
//     ChargingState = 1;
// }
// void DisableCharger(void)
// {
//     // Enable Battery charger -> HIGH = DISABLE
//     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET); //EN
//     ChargingState = 0;
// }

void HandleChargingState(void)
{
    #ifdef BATTERY_PRESENT
    
    //if(BatteryTimer > BATTERY_READ_INTERVAL)
    //{
            
        DisableCharger();
        osDelay(1000);
        CheckBattery();
     
        //if(ChargingState == 1)
        //    EnableCharger();
        //else
        //    DisableCharger();
        
        
        if(ChargeVoltage < 3750)
        {
            EnableCharger();
            ChargingState = 1;
        }
        else if(ChargeVoltage > 4100)
        {
            DisableCharger();
            ChargingState = 0;
        }
        
        //BatteryTimer = 0;
    //}    
    #endif
}
void LoadTimeStamp(HWEventDataType *pPacket)
{
    //FunctionCode;
    //EventType;
    pPacket->GEvent.Hours   = R.Hours;
    pPacket->GEvent.Minutes = R.Minutes;
    pPacket->GEvent.Seconds = R.Seconds;
    pPacket->GEvent.Month   = sDate.Month;
    pPacket->GEvent.Date    = sDate.Date;
    pPacket->GEvent.Year    = sDate.Year;
    pPacket->GEvent.Voltage = ChargeVoltageF;
    pPacket->GEvent.Lat     = fLat;
    pPacket->GEvent.Long    = fLong;
    pPacket->GEvent.Speed   = fSpeed;
}
void LoadGPSTimeStamp(HWEventDataType *pPacket)
{
    
    //FunctionCode;
    //EventType;
    pPacket->GEvent.Hours   = GPSHours;
    pPacket->GEvent.Minutes = GPSMinutes;
    pPacket->GEvent.Seconds = GPSSeconds;
    pPacket->GEvent.Month   = GPSMonth;
    pPacket->GEvent.Date    = GPSDay;
    pPacket->GEvent.Year    = GPSYear;
    pPacket->GEvent.Voltage = ChargeVoltageF;
    pPacket->GEvent.Lat     = fLat;
    pPacket->GEvent.Long    = fLong;
    pPacket->GEvent.Speed   = fSpeed;
}

/* Record one GPS track sample if we have a valid fix and have moved since
   the last recorded point. Called every TRACK_SAMPLE_SECONDS from the 1s
   timer tick. Parked (no movement) records nothing — the regular ping
   still reports the position. */
static void TrackSampleTick(void)
{
    /* LIS3DH gate: no physical motion for 60s = parked. GPS jitter while
       parked exceeds the 5m spacing gate and records phantom movement
       (0.5-2.5kn "speeds" on a stationary vehicle) without this. */
    if (MotionTimer > 60) return;
    if (GPSStatus != 'A') return;
    float lat = fLat, lon = fLong;
    if (lat == 0.0f && lon == 0.0f) return;
    if (lat > -1.0f && lat < 1.0f && lon > -1.0f && lon < 3.0f) return; // cold-start artifact
    if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) return;

    float dlat_m = (lat - track_last_lat) * 111000.0f;
    float dlon_m = (lon - track_last_lon) * 86000.0f;
    float dist_sq = dlat_m * dlat_m + dlon_m * dlon_m;
    if (track_last_lat != 0.0f && dist_sq < (5.0f * 5.0f)) return; // <5m — parked/idle

    long ts = osmand_unix_ts(GPSYear, GPSMonth, GPSDay,
                             GPSHours, GPSMinutes, GPSSeconds);
    if (ts != 0 && ts == track_prev_ts) return; // same GPS second — no new fix yet

    unsigned short next = (track_tail + 1) % TRACK_BUF_SIZE;
    if (next == track_head)
    {
        track_head = (track_head + 1) % TRACK_BUF_SIZE; // full — drop oldest
        track_drop_count++;                             // reported as tdrp
    }

    TrackSample *s = &track_buf[track_tail];
    s->lat = lat;
    s->lon = lon;
    /* Modem reports speed=0 even when moving; derive from distance covered
       over the actual elapsed time since the previous recorded sample. */
    s->speed = 0.0f;
    if (track_last_lat != 0.0f && track_prev_ts != 0 && ts > track_prev_ts) {
        float kmh = sqrtf(dist_sq) * 3.6f / (float)(ts - track_prev_ts);
        if (kmh < 300.0f) s->speed = kmh;
    }
    track_prev_ts = ts;
    track_live_speed_kmh = s->speed;
    track_live_speed_us = esp_timer_get_time();
    s->yy = GPSYear;  s->mo = GPSMonth;   s->dd = GPSDay;
    s->hh = GPSHours; s->mi = GPSMinutes; s->ss = GPSSeconds;
    track_tail = next;

    track_last_lat = lat;
    track_last_lon = lon;
}

/* Debounced ignition + external-power state from ADCBatteryVoltage.
   Called every second from the timer tick. */
static void PowerSenseTick(void)
{
    float v = ADCBatteryVoltage;
    static int ign_cnt = 0, pwr_cnt = 0;

    /* Ignition. Thresholds and the field data behind them are in SCI.h.
       Asymmetric debounce: prompt on, deliberate off. */
    if (ign_on ? (v < IGNITION_OFF_VOLTS) : (v > IGNITION_ON_VOLTS)) {
        int need = ign_on ? IGNITION_OFF_DEBOUNCE : IGNITION_ON_DEBOUNCE;
        if (++ign_cnt >= need) {
            ign_on = !ign_on;
            ign_cnt = 0;
            /* Report the transition immediately rather than waiting up to the
               parked 5-minute interval. Traccar derives trip start/end from
               ignition, so a late edge shifts the trip boundary by minutes.
               Costs a handful of extra pings per day. */
            force_ping_now = 1;
        }
    } else {
        ign_cnt = 0;
    }

    /* External power: present above 8V, lost below 7.5V (7.5–8V = hysteresis). */
    int present = (v > 8.0f) ? 1 : (v < 7.5f) ? 0 : -1;
    if (present < 0 || present == extpwr_on) { pwr_cnt = 0; return; }
    if (++pwr_cnt < 3) return;
    pwr_cnt = 0;
    if (extpwr_on != -1) {   // no alarm for the first reading after boot
        power_alarm = present ? 2 : 1;
        force_ping_now = 1;  // report the transition immediately
    }
    extpwr_on = present;
}

#ifdef ENABLE_HARSH_DRIVING
/* Harsh driving detection (Phase 7b), redesigned in 2.3.37.

   Until now a 20Hz task polled the accelerometer over I2C and did the threshold
   maths in software. Every build carrying that task wedged the device
   (2.3.28-2.3.31, and again 2.3.35). The 2.3.35 instrumentation proved it was
   not stack overflow, not heap exhaustion and not a stuck bus - which leaves
   contention between continuous I2C traffic and the BLE controller on this
   single-core part as the explanation.

   So the polling is gone. The LIS3DH's second interrupt generator is programmed
   with the same threshold and duration the software used to apply - 0.4g held
   for 300ms - and the sensor raises the shared INT1 pin when that happens (see
   InitAccelerometer_LIS3D). Because the interrupt is latched, the existing ~1Hz
   INT1 poll in StartMainTask cannot miss it. No new task, no periodic I2C, and
   nothing running at all between events. */

/* Speed history for classifying an event once it fires. Updated once per second
   from the track-derived speed we already maintain: no sensor access, no
   allocation, no I2C. */
static float    harsh_spd_hist[HARSH_SPD_HIST] = {0};
static int      harsh_spd_idx = 0;
static int64_t  harsh_last_alarm_us = 0;
static uint32_t harsh_event_count = 0;   // events that passed the gates - reported as hcnt
/* Raw generator-2 interrupts, counted before ANY gating (speed, holdoff,
   already-pending). Added in 2.3.38 because hcnt alone could not distinguish
   "the sensor never fired" from "it fired and we discarded it" - which is
   exactly the ambiguity that made 2.3.37's hcnt=0 result hard to act on.
   hraw > 0 with hcnt == 0 means the gates are wrong; hraw == 0 means the
   threshold or filter is wrong. */
static uint32_t harsh_raw_count = 0;     // reported as hraw
/* 2.3.54: events the sensor reported but that moved no vehicle - rejected for
   having no speed signature. Reported as hnod. Read it with hraw and hcnt:
   hraw = hnod + hcnt + (holdoff and min-speed rejects). A healthy drive should
   show hnod well above hcnt, because road transients outnumber real events. */
static uint32_t harsh_nodelta_count = 0; // reported as hnod
/* Last raw INT2_SRC byte seen by the poll, so the diagnostic shows the whole
   register rather than just the result of testing bit 6. 0xFF = never read. */
static unsigned char last_int2_src = 0xFF;

/* Poll-rate counters (2.3.43).
   i2src has read a constant 0x15 on every ping from both units, which is
   consistent with two very different worlds and cannot distinguish them:
     (a) the poll runs constantly and generator 2 genuinely never fires, or
     (b) the poll ran once early and has not run since, so 0x15 is a fossil.
   last_int2_src starts at 0xFF, so observing 0x15 proves only that it was
   updated AT LEAST ONCE. A constant value says nothing about frequency.

   Both sites that read INT2_SRC now count their executions, and they are kept
   separate because they answer different questions:
     ipoll - the StartMainTask INT1 handler, which runs only when the INT1 pin
             is asserted. Frozen here means nothing is asserting or servicing
             the pin at all, which would also mean motion wake is broken - a
             far bigger finding than a threshold being wrong, and it would make
             a threshold bisection meaningless.
     qpoll - the XHTTP_Request ping wait loop, which runs every ~30s while
             driving regardless of the pin. This is the control: it should
             always climb, so if qpoll moves and ipoll does not, the fault is
             the pin, not the code path.
   Measurement before theory - four inferred mechanisms have now been wrong. */
static uint32_t int1_poll_count = 0;     // reported as ipoll
static uint32_t ping_poll_count = 0;     // reported as qpoll

/* Diagnostic readback (2.3.41). Three fix attempts for "harsh detection never
   fires" have now failed - stack/heap/I2C (2.3.35), HPF cutoff (2.3.38), and
   InitAccelerometer resetting the generator state (2.3.39) - each based on an
   inferred mechanism rather than a measurement. This reads the sensor's own
   configuration back at ping time (not just after writing it, so an later
   overwrite is still caught) and reports it verbatim.

   Expected on 2.3.49: 33,37,B7,60,08,0A,0A,0F,02
     WHO_AM_I  0x33 = LIS3DH. Anything else means it is a different chip and
                      every harsh register write has gone somewhere meaningless
                      - the driver also supports MMA8652/8653/8452, where 0x34
                      is not INT2_CFG at all.
     CTRL_REG1 0x37 = 25Hz ODR, all axes   (2.3.48: was 0x57 = 100Hz. ODR sets
                      the DURATION tick to 40ms AND the HPF cutoff to ODR/400
                      = 0.0625Hz - the cutoff is the reason it changed.)
     CTRL_REG2 0xB7 = HPM=10, lowest cutoff for the ODR, HP on both generators
     CTRL_REG3 0x60 = IA1+IA2 routed to the INT1 pin
     CTRL_REG4 0x08 = +-2g, HR            (sets the THS step to 16mg)
     CTRL_REG5 0x0A = both interrupts latched
     INT2_CFG  0x0A = XHIE|YHIE, OR       (2.3.49: was 0x2A, ZHIE dropped -
                      vertical road jolts were swamping the braking signal)
     INT2_THS  0x0F = 15 counts = 240mg   (2.3.47: was 0x19 = 400mg)
     INT2_DUR  0x02 = 2 counts  = 80ms    (2.3.48: 100ms is 2.5 ticks at 25Hz) */
static void HarshRegReadback(char *out, size_t len)
{
    snprintf(out, len, "&lisreg=%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X&i2src=%02X",
             I2C_RdReg(0x0F),            // WHO_AM_I
             I2C_RdReg(REG_CTRL_REG1),
             I2C_RdReg(REG_CTRL_REG2),
             I2C_RdReg(REG_CTRL_REG3),
             I2C_RdReg(REG_CTRL_REG4),
             I2C_RdReg(REG_CTRL_REG5),
             I2C_RdReg(REG_INT2_CFG),
             I2C_RdReg(REG_INT2_THS),
             I2C_RdReg(REG_INT2_DURATION),
             last_int2_src);
}

/* Called once per second from the timer tick. */
static void HarshSpeedTick(void)
{
    float spd = (track_live_speed_us != 0 &&
                 esp_timer_get_time() - track_live_speed_us <= 10LL * 1000000LL)
                    ? track_live_speed_kmh : 0.0f;
    harsh_spd_hist[harsh_spd_idx] = spd;
    harsh_spd_idx = (harsh_spd_idx + 1) % HARSH_SPD_HIST;
}

static float harsh_speed_now(void)
{
    return harsh_spd_hist[(harsh_spd_idx + HARSH_SPD_HIST - 1) % HARSH_SPD_HIST];
}

static float harsh_speed_ago(void)   /* oldest entry, ~HARSH_SPD_HIST seconds back */
{
    return harsh_spd_hist[harsh_spd_idx];
}

/* Called from StartMainTask when the INT1 pin is asserted and INT2_SRC shows
   generator 2 was the source. Classifies from the speed trend - braking,
   acceleration, or neither (cornering) - and raises the alarm for the next
   ping. Does no I2C of its own: the caller has already read INT2_SRC, and the
   fact that it fired is the whole measurement. */
static void HarshEventDetected(void)
{
    int64_t now = esp_timer_get_time();

    /* Counted first, before every gate below, so hraw reflects what the sensor
       actually reported rather than what survived filtering. */
    harsh_raw_count++;

    /* Holdoff stops one rough stretch of road producing a burst of alarms. */
    if (now - harsh_last_alarm_us <= (int64_t)HARSH_HOLDOFF_S * 1000000LL)
        return;

    float spd_now = harsh_speed_now();
    float spd_old = harsh_speed_ago();
    float peak    = (spd_now > spd_old) ? spd_now : spd_old;

    /* Below urban pace this is a door slam, a kerb, or someone leaning on the
       vehicle - not driving behaviour. */
    if (peak < HARSH_MIN_SPEED) return;
    if (harsh_alarm != 0) return;          /* one alarm pending at a time */

    /* 2.3.54: REQUIRE A SPEED SIGNATURE.
       This used to fall through to hardCornering for anything without a speed
       change, which made cornering the catch-all - and that is exactly how a
       pothole at steady 70 km/h became an alarm. Measured on 2.3.49: of 18
       gated events, four fired at highway speed with 0.036-0.059g of actual
       deceleration, i.e. no deceleration at all.

       The interrupt says only "threshold crossed", never by how much, so the
       sensor cannot tell a 0.4g brake from a 0.4g jolt. The speed trend can:
       a brake moves the vehicle, a pothole does not. This reads the track
       history already recorded at 2-3s resolution, so it costs no I2C and no
       faster poll - which matters, because the obvious alternative (counting
       several interrupts in a window) cannot work here: the latch collapses
       every crossing between two polls into one, and raising the poll rate is
       what caused the btController livelock.

       KNOWN LIMITATION - cornering is deferred, not solved. Pure cornering at
       constant speed has no speed signature and is now rejected along with the
       potholes. That is deliberate for v1: hardBraking and hardAcceleration
       become trustworthy, and cornering was the one class that could never be
       verified from GPS anyway. Re-enabling it needs a real magnitude, i.e.
       the deferred FIFO capture, or heading change from the track buffer. */
    float ds = spd_now - spd_old;
    if (ds <= -HARSH_SPEED_DELTA)      harsh_alarm = 1;   /* hardBraking */
    else if (ds >= HARSH_SPEED_DELTA)  harsh_alarm = 2;   /* hardAcceleration */
    else {
        /* No speed signature: the sensor moved but the vehicle did not. */
        harsh_nodelta_count++;
        return;
    }
    harsh_last_alarm_us = now;
    harsh_event_count++;
    force_ping_now = 1;
    /* No logging here. printf on this path was a livelock suspect, and the
       event reaches the server as alarm= on the forced ping regardless. */
}
#endif // ENABLE_HARSH_DRIVING

void PostMotionEvent(void)
{
    
    //unsigned char CheckSum;
    #ifdef DEBUG_PRINT
        DebugPrint("Entered-PostMotionEvent\r\n"); 
    #endif

    LoadTimeStamp(&CPacket);
    CPacket.GEvent.EventType = MOTION_PING;
    //CheckSum = GetCheckSum(CPacket.Bytes,34);
    //CPacket.GEvent.CheckSum[1]=GetAscii(CheckSum&0x0F);  
    //CPacket.GEvent.CheckSum[0]=GetAscii((CheckSum>>4) & 0x0F);  

	  PostEvent( &CPacket);
   // if(PostEvent( &CPacket) == FIFO_OVERRUN_OCCURED)
      //  PostEEEvent(&CPacket);
    
    
}

void PostReboot(void)
{
    #ifdef DEBUG_PRINT
        DebugPrint("Entered-PostReboot\r\n"); 
    #endif

    LoadGPSTimeStamp(&CPacket);
    CPacket.GEvent.EventType = REBOOT_PING;
    //CheckSum = GetCheckSum(CPacket.Bytes,34);
    //CPacket.GEvent.CheckSum[1]=GetAscii(CheckSum&0x0F);  
    //CPacket.GEvent.CheckSum[0]=GetAscii((CheckSum>>4) & 0x0F);  

	  
    PostEvent( &CPacket);
//    if(PostEvent( &CPacket) == FIFO_OVERRUN_OCCURED)
//        PostEEEvent(&CPacket);
   
}
void PostGPing(void)
{
    
    //unsigned char CheckSum;//,str[50];
	#ifdef DEBUG_PRINT
        DebugPrint("Entered-PostGPing\r\n"); 
    #endif

    LoadGPSTimeStamp(&CPacket);
    CPacket.GEvent.EventType = G_PING;
    //CheckSum = GetCheckSum(CPacket.Bytes,34);
    //CPacket.GEvent.CheckSum[1]=GetAscii(CheckSum&0x0F);  
    //CPacket.GEvent.CheckSum[0]=GetAscii((CheckSum>>4) & 0x0F);  

    PostEvent( &CPacket);
//    if(PostEvent( &CPacket) == FIFO_OVERRUN_OCCURED)
//        PostEEEvent(&CPacket);
   
}

void PostGPRSPing(void)
{
    #ifdef DEBUG_PRINT
        DebugPrint("Entered-PostGPRSPing\r\n"); 
    #endif
    LTime = InactivityTimer;
    if(LTime-PLTime>=Params.Fields.PingInterval)
    {

        LoadTimeStamp(&CPacket);
        CPacket.GEvent.EventType = GPRS_PING;
        //CheckSum = GetCheckSum(CPacket.Bytes,34);
        //CPacket.GEvent.CheckSum[1]=GetAscii(CheckSum&0x0F);  
        //CPacket.GEvent.CheckSum[0]=GetAscii((CheckSum>>4) & 0x0F);  

        PostEvent( &CPacket);
//        if(PostEvent( &CPacket) == FIFO_OVERRUN_OCCURED)
//            PostEEEvent(&CPacket);
        PLTime=LTime;
    }
}

void PostSOSPing(void)
{
    #ifdef DEBUG_PRINT
        DebugPrint("Entered-PostSOSPing\r\n"); 
    #endif
    //unsigned char CheckSum;//,str[50];
	
    LoadTimeStamp(&CPacket);
    CPacket.GEvent.EventType = SOS_PING;
    //CheckSum = GetCheckSum(CPacket.Bytes,34);
    //CPacket.GEvent.CheckSum[1]=GetAscii(CheckSum&0x0F);  
    //CPacket.GEvent.CheckSum[0]=GetAscii((CheckSum>>4) & 0x0F);  

    PostEvent( &CPacket);
//    if(PostEvent( &CPacket) == FIFO_OVERRUN_OCCURED)
//        PostEEEvent(&CPacket);
   
}
void DDelay()
{
 
    osDelay(500);
}
void WakeUp(void)
{
    Print("AT\r\n");
    DDelay();//DelayProc(10000);
    //DDelay();
}

unsigned char CheckNetwork(void)
{
    //unsigned long LoopCount = 0;
    unsigned char Retries=0;
    char*pToken1,*pToken2,*pToken3, *pToken4;
    osDelay(1000);
    CHECK_NETWORK_AGAIN:    
    
    ResetBuffer();
    Print("AT+CREG?\r\n");
 
	  LoopTimeout1 = 0;
    while(1)
    {
			if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CREG:",6) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    }
		osDelay(500);
	
    pToken1 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,1",3);
    pToken2 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,5",3);
    
    if( (pToken1 != NULL) || (pToken2 != NULL))
    {
        ResetBuffer();
        ESP_LOGI(TAG,"Network registered/roaming");

        
        return 0;
        
    }

    pToken1 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,0",3);
    pToken2 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,2",3);
    pToken3 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,3",3);
    pToken4 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,4",3);
    pToken4 = MapForward(Buff2,BUFF2_SIZE,(char*)"0,6",3);
    
    if( (pToken1 != NULL) || (pToken2 != NULL) || (pToken3 != NULL) || (pToken4 != NULL) )
    {            
        ResetBuffer();
        ESP_LOGI(TAG,"Network not registered");
        #ifdef DEBUG_PRINT
            DebugPrint("Network Not registered -CheckNetwork\r\n"); 
        #endif

        
        if(++Retries<60)
        {
            osDelay(1000);
            goto CHECK_NETWORK_AGAIN;
        }
        else
            return 1;
    }
    
    return 1;
    
}

void DeleteAllSMS(void)
{
    unsigned char i;
    ResetBuffer();
    Print("AT\r\n");
    DelayProc(10000);
    for(i=1;i<10;i++)
    {
        ResetBuffer();
        Print("AT+CMGD=");
        WriteUART2(i|0x30);
        Print("\r\n");
        DelayProc(850000);
    }
}
unsigned char SendPMTKCommand(char *pCommand,char *pResponse1,char *pResponse2, unsigned short Timeout)
{
    unsigned char Response1Length,Response2Length;
    osDelay(200);
    Response1Length = strlen((void*)pResponse1);
    Response2Length = strlen((void*)pResponse2);
    ResetBuffer1();
    Print1(pCommand);
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff,BUFF_SIZE,(char*)pResponse1,Response1Length) != NULL)
            return 1;
        if(MapForward(Buff,BUFF_SIZE,(char*)pResponse2,Response2Length) != NULL)
            return 2;
        if(LoopTimeout1>Timeout)
            return 3; 
        
    }
    
}
 

void SystemPower_Config(void);
//static void SystemClock_ConfigMSI(void);
char tmpeebuff[10]="123456789";
unsigned char tmpeeaddr;


typedef struct
{
    char t1[10];
    char t2[10];
    char t3[100];
    char t4[10];
}TempType;

TempType ts;
char*pVar;

void CheckBLE(void)
{
	
RTC_TimeTypeDef BTime;
RTC_DateTypeDef BDate;
int Year=0,Month=0,Date=0,Hour=0,Minute=0,Sec=0;
    #ifdef BLUETOOTH_ENABLED
    
    char*pToken;
    unsigned char i;
    SOS = gpio_get_level(GPIO_SOS);
    if(SOS == 0)
    {
        LEDInhibit=1;
        osDelay(3000);
        SOS = gpio_get_level(GPIO_SOS);
        if(SOS == 0)
        {
            
            if(DeviceStatus == 0)
                return;
            
            FrontPanelTimer = -600;
            UpdateBluetooth(1);

            
            NeedBTAttention = 1;
            //osDelay(10000);
            //osDelay(10000);
            
            WakeUp();
            ResetBuffer();
            Print("AT+BTPOWER=1\r\n");
            osDelay(5000);
     
 
            
            ResetBuffer();
            Print("AT+BTPAIRCFG=1\r\n");
            osDelay(1000);
            
            ResetBuffer();
            Print("AT+BTVIS=1\r\n");
            osDelay(1000);
        
            while(1)
            {
                WakeUp();
                osDelay(1000);
                SOS = gpio_get_level(GPIO_SOS);
                if(SOS == 0) 
                {
                    goto TURN_OFF_BLE;
                }
                if(MapForward(Buff2,BUFF2_SIZE,(char*)"CONNECTING",10) != NULL)
                {
                    
                    Print("AT+BTACPT=1\r\n");
                    osDelay(1000);
                    while(1)
                    {
                        FrontPanelTimer = -600;
                        
                        // TBD//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_SET);
                        osDelay(500);
                        if(SystemTimer%10 == 0)
                        {
                            WakeUp();
                            ResetBuffer();
                            Print("AT+BTSPPSEND\r\n");
                            osDelay(1000);
                            
                            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5);
                            if(pToken != NULL)
                            {
                                goto TURN_OFF_BLE;
                            }
                            //sprintf(ts.t1,"WM: %d\n",Params.Fields.WorkingMode);
                            //sprintf(ts.t2,"AM: %d\n",Params.Fields.MotionAlertMode);
                            //sprintf(ts.t3,"URL: %s\n",Params.Fields.HTTPURL);
                            //sprintf(ts.t4,"PI: %d\n",Params.Fields.PingInterval);
                            UpdateBluetooth(2);
                            Print("Am Listening#\n");
                            
                            sprintf((void*)bstr, "\nBand: %s",Params.Fields.Band);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nWM: %s",Params.Fields.WorkingMode);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMAlert: %s",Params.Fields.MotionAlertMode);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMThresh: %hhu",Params.Fields.MotionThreshold);   
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nContact: %s",Params.Fields.rxNumber);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nAPN: %s",Params.Fields.APNName);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nAPNUser: %s",Params.Fields.APNUsername);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nAPNPass: %s",Params.Fields.APNPassword);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nHTTPURL: %s",Params.Fields.HTTPURL);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nHTTPKey: %s",Params.Fields.HTTPKey);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nInterval: %u",Params.Fields.PingInterval);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMHost: %s",Params.Fields.MQTTHost);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMPort: %s",Params.Fields.MQTTPort);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMCID: %s",Params.Fields.MQTTClientID);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMTopic: %s",Params.Fields.MQTTTopic);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMProt: %s",Params.Fields.MQTTProtocolName);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMLVL: 0x%hhX",Params.Fields.MQTTLVL);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMFlag: 0x%hhX",Params.Fields.MQTTFlags);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMKAlive: %u",Params.Fields.MQTTKeepAlive);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMUser: %s",Params.Fields.MQTTUsername);
                            Print((void*)bstr);
                            sprintf((void*)bstr, "\nMPass: %s",Params.Fields.MQTTPassword);
                            Print((void*)bstr);
                            sprintf((void*)bstr,"\nTime: 20%02d-%02d-%02d  %02d:%02d:%02d",sDate.Year,sDate.Month,sDate.Date,R.Hours,R.Minutes,R.Seconds);
                            Print((void*)bstr);
                            //Print(ts.t1);
                            //Print(ts.t2);
                            //Print(ts.t3);
                            //Print(ts.t4);
                            
                            putcchar(0x1A);
                            UpdateBluetooth(1);
                            
                        }
                        //TBD// HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_RESET);
                        osDelay(500);
                        //ResetBuffer();
                        Count=0;
                        pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"VALETRON",8);
                        if(pToken != NULL)
                        {
                            osDelay(2000);
                            memset((void*)bstr,0,sizeof(bstr));
                            i=0;
                            while(pToken[11+i] != '#')
                            {
                                bstr[i] = pToken[11+i];
                                i++;
                                if(i>250)goto DISCARD;
                            }
                            bstr[i] = '\0';
                            switch(pToken[9])
                            {
                                case '0':
                                    Params.Fields.Band[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.Band);
                                break;
                                case '1':
                                    sscanf((void*)bstr, "%s",Params.Fields.WorkingMode);
                                break;
                                case '2':
                                    sscanf((void*)bstr, "%s",Params.Fields.MotionAlertMode);
                                break;
                                case '3':
                                    //bstr[i] = ' ';
                                    sscanf((void*)bstr, "%hhu",(char*)&Params.Fields.MotionThreshold);   
//                                    #ifdef LIS3DH_ENABLED
//                                    ISRstatus = I2C_RdReg(REG_INT1_SRC);
//                                    InitAccelerometer();
//                                    #else
//                                    InitAccelerometer_mma84();
//                                    #endif
                                      InitAccelerometer();
                                    //InitAccelerometer();
                                    //Params.Fields.MotionThreshold = strtoul((void*)bstr);
                                break;
                                case '4':
                                    sscanf((void*)bstr, "%s",Params.Fields.rxNumber);
                                break;
                                case '5':
                                    Params.Fields.APNName[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.APNName);
                                break;
                                case '6':
                                    Params.Fields.APNUsername[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.APNUsername);
                                break;
                                case '7':
                                    Params.Fields.APNPassword[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.APNPassword);
                                break;
                                case '8':
                                    Params.Fields.HTTPURL[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.HTTPURL);
                                break;
                                case '9':
                                    Params.Fields.HTTPKey[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.HTTPKey);
                                break;
                                case 'A':
                                    //bstr[i] = ' ';
                                    sscanf((void*)bstr, "%u",&Params.Fields.PingInterval);
                                    //Params.Fields.PingInterval = strtoul((void*)bstr);
                                break;
                                case 'B':
                                    Params.Fields.MQTTHost[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTHost);
                                break;
                                case 'C':
                                    Params.Fields.MQTTPort[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTPort);
                                break;
                                case 'D':
                                    Params.Fields.MQTTClientID[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTClientID);
                                break;
                                case 'E':
                                    Params.Fields.MQTTTopic[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTTopic);
                                break;
                                case 'F':
                                    Params.Fields.MQTTProtocolName[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTProtocolName);
                                break;
                                case 'G':
                                    sscanf((void*)bstr, "%hhx",(char*)&Params.Fields.MQTTLVL);
                                break;
                                case 'H':
                                    sscanf((void*)bstr, "%hhx",(char*)&Params.Fields.MQTTFlags);
                                break;
                                case 'I':
                                    sscanf((void*)bstr, "%u",&Params.Fields.MQTTKeepAlive);
                                break;
                                case 'J':
                                    Params.Fields.MQTTUsername[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTUsername);
                                break;
                                case 'K':
                                    Params.Fields.MQTTPassword[0] = '\0';
                                    sscanf((void*)bstr, "%s",Params.Fields.MQTTPassword);
                                break;
																
                                case 'L':
                                    sscanf((void*)bstr,"20%02d-%02d-%02d  %02d:%02d:%02d",
                                    &Year,&Month,&Date,&Hour,&Minute,&Sec);
                                BDate.Year=Year;
                                BDate.Month=Month;
                                BDate.Date=Date;
                                BTime.Hours=Hour;
                                BTime.Minutes=Minute;
                                BTime.Seconds=Sec;
                                //TBD//HAL_RTC_SetDate(&hrtc, &BDate, RTC_FORMAT_BIN);
                                //TBD//HAL_RTC_SetTime(&hrtc, &BTime, RTC_FORMAT_BIN);
                                break;

                            }
                            
                            /*sscanf( (void*)&pToken[10], 
                                "%d %d %s %d", 
                                (void*)&Params.Fields.Params.Fields.WorkingMode ,
                                (void*)&Params.Fields.Params.Fields.MotionAlertMode,
                                (void*)Params.Fields.API_URL,
                                (void*)&Params.Fields.Params.Fields.PingInterval);
                            osDelay(100);*/
                            /*pVar = strtok(&pToken[10], ",");                               
                            sscanf(pVar, "%s",ts.t1);
                            pVar = strtok(NULL, ",");
                            sscanf(pVar, "%s",ts.t2);
                            pVar = strtok(NULL, ",");
                            sscanf(pVar, "%s",ts.t3);
                            pVar = strtok(NULL, "\n");
                            sscanf(pVar, "%s",ts.t4);
                            
                            //sscanf(ts.t1, "%1d",&Params.Fields.Params.Fields.WorkingMode);
                            //sscanf(ts.t2, "%1d",&Params.Fields.Params.Fields.MotionAlertMode);
                            ts.t1[0]-=0x30;
                            if(ts.t1[0] < 3)
                            Params.Fields.WorkingMode = ts.t1[0];
                            ts.t2[0]-=0x30;
                            if(ts.t2[0] < 3)
                            Params.Fields.MotionAlertMode = ts.t2[0];
                            
                            sscanf(ts.t3, "%s",Params.Fields.HTTPURL);
                            sscanf(ts.t4, "%u",&Params.Fields.PingInterval);
                            
                            Params.Fields.HTTPURL[149] = '\0';
                            */
                            StoreEEParams();
                        
                        GetEEParams();
                            InitRTCAlarm();
                    DISCARD: 
                            ResetBuffer();
                            //__disable_irq();
                            //if( (pToken[8] == 'A') && (pToken[9] == 'P') )
                            /*{
                                
                                i=0;
                                j=0;
                                while(pToken[10+i] != '#')
                                {                    
                                    pEEData->APN[j] = pToken[10+i];
                                    i++;j++;
                                    if(i>=100)break;
                                }                
                                pEEData->APN[j] = '\0';
                                //i=0;
                                i++;
                                j=0;
                                while(pToken[10+i] != '#')
                                {                    
                                    pEEData->username[j] = pToken[10+i];
                                    i++;j++;
                                    if(i>=150)break;
                                }
                                i++;                        
                                pEEData->username[j] = '\0';
                                j=0;
                                while(pToken[10+i] != '#')
                                {                    
                                    pEEData->password[j] = pToken[10+i];
                                    i++;
                                    j++;
                                    if(i>=200)break;
                                }                
                                pEEData->password[j] = '\0';
                                
                                
                                
                                UpdateFlash();
                                
                                Print("AT+BTSPPSEND\r\n");
                                DDelay();
                                DDelay();
                                Print("Settings changed#");
                                putcchar(0x1A);
                                ResetBuffer();
                                NeedBTAttention = 0;
                                break;
                                
                            }*/
                        }
                        pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"DISCONN",7);
                        if(pToken != NULL)
                        {
                            goto TURN_OFF_BLE;
                        }
                        SOS = gpio_get_level(GPIO_SOS);
                        if(SOS == 0) 
                        {
                            TURN_OFF_BLE:
                            UpdateBluetooth(0);
                            FrontPanelTimer = 0;
                            //TBD//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_RESET);
                            NeedBTAttention = 0;
                            WakeUp();
                            ResetBuffer();
                            Print("AT+BTPOWER=0\r\n");
                            osDelay(3000);
                            
                            goto EXIT_BLE;
                        }
                         
                    }                        
                }
            }
        }
    }
    EXIT_BLE:
    LEDInhibit = 0;
    #endif
}
unsigned char ResponsePacket[2] = {0x01,0x00};
void WriteBTParams(char Byte)
{
    char *pToken;
    char bstr[250];
    unsigned char i;    
    
    if(Byte == '$')
    {
        BTBuffIndex = 0;
    }
    BTBuff[BTBuffIndex] = Byte;
    
    ResponsePacket[0] = 0x01; ResponsePacket[1] = 0x01;        
    ResponsePacket[0] = '$'; 
    //HAL_NVIC_DisableIRQ(USART1_IRQn);
    //HAL_NVIC_DisableIRQ(LPUART1_IRQn);
    //P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
    //HAL_NVIC_EnableIRQ(LPUART1_IRQn);
//            
//            for(unsigned char i = 0;i<15;i++)
//            {
//                ResponsePacket[0] = IMEI[i]; 
//                P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
//            }
//            ResponsePacket[0] = '#'; 
//            P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
    

    
    if(BTBuff[BTBuffIndex] == '#')
    {
        pToken = MapForward((char*)BTBuff,BTBUFF_SIZE,(char*)"VALETRON",8);
        if(pToken != NULL)
        {
            
            //HAL_Delay(2000);
            memset(bstr,0,sizeof(bstr));
            i=0;
            while(pToken[11+i] != '#')
            {
                bstr[i] = pToken[11+i];
                i++;
                if(i>250)goto DISCARD;
            }
            bstr[i] = '\0';
            //FieldUpdated = pToken[9];
            switch(pToken[9])
            {
                case '0':
                    Params.Fields.Band[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.Band);
                break;
                case '1':
                    sscanf((void*)bstr, "%s",Params.Fields.WorkingMode);
                    BTReceiveTimer = 0;
                break;
                case '2':
                    sscanf((void*)bstr, "%s",Params.Fields.MotionAlertMode);
                break;
                case '3':
                    //bstr[i] = ' ';
                    sscanf((void*)bstr, "%hhu",(char*)&Params.Fields.MotionThreshold);   
//                            #ifdef LIS3DH_ENABLED
//                                ISRstatus = I2C_RdReg(REG_INT1_SRC);
//                                InitAccelerometer();
//                            #else
//                                InitAccelerometer_mma84();
//                            #endif
                    InitAccelerometer();
                break;
                case '4':
                    sscanf((void*)bstr, "%s",Params.Fields.rxNumber);
                break;
                case '5':
                    Params.Fields.APNName[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.APNName);
                break;
                case '6':
                    Params.Fields.APNUsername[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.APNUsername);
                break;
                case '7':
                    Params.Fields.APNPassword[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.APNPassword);
                break;
                case '8':
                    Params.Fields.HTTPURL[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.HTTPURL);
                break;
                case '9':
                    Params.Fields.HTTPKey[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.HTTPKey);
                break;
                case 'A':
                    //bstr[i] = ' ';
                    sscanf((void*)bstr, "%u",&Params.Fields.PingInterval);
                    //Params.Fields.PingInterval = strtoul(bstr);
                break;
                case 'B':
                    Params.Fields.MQTTHost[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTHost);
                break;
                case 'C':
                    Params.Fields.MQTTPort[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTPort);
                break;
                case 'D':
                    Params.Fields.MQTTClientID[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTClientID);
                break;
                case 'E':
                    Params.Fields.MQTTTopic[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTTopic);
                break;
                case 'F':
                    Params.Fields.MQTTProtocolName[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTProtocolName);
                break;
                case 'G':
                    sscanf((void*)bstr, "%hhx",(char*)&Params.Fields.MQTTLVL);
                break;
                case 'H':
                    sscanf((void*)bstr, "%hhx",(char*)&Params.Fields.MQTTFlags);
                break;
                case 'I':
                    sscanf((void*)bstr, "%u",&Params.Fields.MQTTKeepAlive);
                break;
                case 'J':
                    Params.Fields.MQTTUsername[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTUsername);
                break;
                case 'K':
                    Params.Fields.MQTTPassword[0] = '\0';
                    sscanf((void*)bstr, "%s",Params.Fields.MQTTPassword);
                break;
                case 'L':
                        //     sscanf((void*)bstr,"20%02d-%02d-%02d  %02d:%02d:%02d",
                        //     &Year,&Month,&Date,&Hour,&Minute,&Sec);
                        // BDate.Year=Year;
                        // BDate.Month=Month;
                        // BDate.Date=Date;
                        // BTime.Hours=Hour;
                        // BTime.Minutes=Minute;
                        // BTime.Seconds=Sec;
                        // HAL_RTC_SetDate(&hrtc, &BDate, RTC_FORMAT_BIN);
                        // HAL_RTC_SetTime(&hrtc, &BTime, RTC_FORMAT_BIN);
                        break;
                    case 'Z':
                    return;
                break;
                
            }
            StoreEEParams();
            GetEEParams();
            
            
            // HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN);
            // HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            
            
            
            putcharBT('$');
            
            switch(pToken[9])
            {
                case '0':
                    sprintf((void*)bstr, "Band: %s",Params.Fields.Band);
                    PrintBT(bstr);
                break;
                case '1':
                    sprintf((void*)bstr, "WM: %s",Params.Fields.WorkingMode);
                    PrintBT(bstr);
                break;
                case '2':
                    sprintf((void*)bstr, "MAlert: %s",Params.Fields.MotionAlertMode);
                    PrintBT(bstr);
                break;
                case '3':
                    sprintf((void*)bstr, "MThresh: %hhu",Params.Fields.MotionThreshold);   
                    PrintBT(bstr);
                    
                break;
                case '4':
                    sprintf((void*)bstr, "Contact: %s",Params.Fields.rxNumber);
                    PrintBT(bstr);
                break;
                case '5':
                    sprintf((void*)bstr, "APN: %s",Params.Fields.APNName);
                    PrintBT(bstr);
                break;
                case '6':
                    sprintf((void*)bstr, "APNUser: %s",Params.Fields.APNUsername);
                    PrintBT(bstr);
                break;
                case '7':
                    sprintf((void*)bstr, "APNPass: %s",Params.Fields.APNPassword);
                    PrintBT(bstr);
                break;
                case '8':
                    sprintf((void*)bstr, "HTTPURL: %s",Params.Fields.HTTPURL);
                    PrintBT(bstr);
                break;
                case '9':
                    sprintf((void*)bstr, "HTTPKey: %s",Params.Fields.HTTPKey);
                    PrintBT(bstr);
                break;
                case 'A':
                    sprintf((void*)bstr, "Interval: %u",Params.Fields.PingInterval);
                    PrintBT(bstr);
                break;
                case 'B':
                    sprintf((void*)bstr, "MHost: %s",Params.Fields.MQTTHost);
                    PrintBT(bstr);
                break;
                case 'C':
                    sprintf((void*)bstr, "MPort: %s",Params.Fields.MQTTPort);
                    PrintBT(bstr);
                break;
                case 'D':
                    sprintf((void*)bstr, "MCID: %s",Params.Fields.MQTTClientID);
                    PrintBT(bstr);
                break;
                case 'E':
                    sprintf((void*)bstr, "MTopic: %s",Params.Fields.MQTTTopic);
                    PrintBT(bstr);
                break;
                case 'F':
                    sprintf((void*)bstr, "MProt: %s",Params.Fields.MQTTProtocolName);
                    PrintBT(bstr);
                break;
                case 'G':
                    sprintf((void*)bstr, "MLVL: 0x%hhX",Params.Fields.MQTTLVL);
                    PrintBT(bstr);
                break;
                case 'H':
                    sprintf((void*)bstr, "MFlag: 0x%hhX",Params.Fields.MQTTFlags);
                    PrintBT(bstr);
                break;
                case 'I':
                    sprintf((void*)bstr, "MKAlive: %u",Params.Fields.MQTTKeepAlive);
                    PrintBT(bstr);
                break;
                case 'J':
                    sprintf((void*)bstr, "MUser: %s",Params.Fields.MQTTUsername);
                    PrintBT(bstr);
                break;
                case 'K':
                    sprintf((void*)bstr, "MPass: %s",Params.Fields.MQTTPassword);
                    PrintBT(bstr);
                break;
                case 'L':
                        sprintf((void*)bstr,"Time: 20%02d-%02d-%02d  %02d:%02d:%02d",sDate.Year,sDate.Month,sDate.Date,R.Hours,R.Minutes,R.Seconds);
                        PrintBT(bstr);
                        break;
                    case 'Z':
                    return;
                break;
                
            }
                    
            putcharBT('#');
            putcharBT('\n');
        }
        
        
    }
        
    BTBuffIndex++;
    if(BTBuffIndex >= BTBUFF_SIZE-1)
        BTBuffIndex = 0;
    
    return;
    //DisconnectDevice();
    //UpdateLED3(RED_COLOR);
    //SystemState = State_IdleState;
DISCARD: ResetBTBuffer();
        
        
    
}
void ReadBTParams(char Byte)
{
    char *pToken;
    char bstr[250];
    unsigned char i;

    if(Byte == '$')
    {
        BTBuffIndex = 0;
    }
    BTBuff[BTBuffIndex] = Byte;
    
    ResponsePacket[0] = 0x02; ResponsePacket[1] = 0x01;        
    ResponsePacket[0] = '$'; 
    //HAL_NVIC_DisableIRQ(USART1_IRQn);
    //HAL_NVIC_DisableIRQ(LPUART1_IRQn);
    //P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
    //HAL_NVIC_EnableIRQ(LPUART1_IRQn);
//            
//            for(unsigned char i = 0;i<15;i++)
//            {
//                ResponsePacket[0] = IMEI[i]; 
//                P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
//            }
//            ResponsePacket[0] = '#'; 
//            P2PS_STM_App_Update_Char(P2P_NOTIFY_CHAR_UUID, (uint8_t *)ResponsePacket);
    

    
    if(BTBuff[BTBuffIndex] == '#')
    {
        pToken = MapForward((char*)BTBuff,BTBUFF_SIZE,(char*)"VALETRON",8);
        if(pToken != NULL)
        {
            
            //HAL_Delay(2000);
            memset(bstr,0,sizeof(bstr));
            i=0;
            while(pToken[11+i] != '#')
            {
                bstr[i] = pToken[11+i];
                i++;
                if(i>250)goto DISCARD2;
            }
            bstr[i] = '\0';
            //FieldUpdated = pToken[9];
            
            
            GetEEParams();
            
            
            
            
            
            putcharBT('$');
            
            switch(pToken[9])
            {
                case '0':
                    sprintf((void*)bstr, "Band: %s",Params.Fields.Band);
                    PrintBT(bstr);
                break;
                case '1':
                    sprintf((void*)bstr, "WM: %s",Params.Fields.WorkingMode);
                    PrintBT(bstr);
                break;
                case '2':
                    sprintf((void*)bstr, "MAlert: %s",Params.Fields.MotionAlertMode);
                    PrintBT(bstr);
                break;
                case '3':
                    sprintf((void*)bstr, "MThresh: %hhu",Params.Fields.MotionThreshold);   
                    PrintBT(bstr);
                    
                break;
                case '4':
                    sprintf((void*)bstr, "Contact: %s",Params.Fields.rxNumber);
                    PrintBT(bstr);
                break;
                case '5':
                    sprintf((void*)bstr, "APN: %s",Params.Fields.APNName);
                    PrintBT(bstr);
                break;
                case '6':
                    sprintf((void*)bstr, "APNUser: %s",Params.Fields.APNUsername);
                    PrintBT(bstr);
                break;
                case '7':
                    sprintf((void*)bstr, "APNPass: %s",Params.Fields.APNPassword);
                    PrintBT(bstr);
                break;
                case '8':
                    sprintf((void*)bstr, "HTTPURL: %s",Params.Fields.HTTPURL);
                    PrintBT(bstr);
                break;
                case '9':
                    sprintf((void*)bstr, "HTTPKey: %s",Params.Fields.HTTPKey);
                    PrintBT(bstr);
                break;
                case 'A':
                    sprintf((void*)bstr, "Interval: %u",Params.Fields.PingInterval);
                    PrintBT(bstr);
                break;
                case 'B':
                    sprintf((void*)bstr, "MHost: %s",Params.Fields.MQTTHost);
                    PrintBT(bstr);
                break;
                case 'C':
                    sprintf((void*)bstr, "MPort: %s",Params.Fields.MQTTPort);
                    PrintBT(bstr);
                break;
                case 'D':
                    sprintf((void*)bstr, "MCID: %s",Params.Fields.MQTTClientID);
                    PrintBT(bstr);
                break;
                case 'E':
                    sprintf((void*)bstr, "MTopic: %s",Params.Fields.MQTTTopic);
                    PrintBT(bstr);
                break;
                case 'F':
                    sprintf((void*)bstr, "MProt: %s",Params.Fields.MQTTProtocolName);
                    PrintBT(bstr);
                break;
                case 'G':
                    sprintf((void*)bstr, "MLVL: 0x%hhX",Params.Fields.MQTTLVL);
                    PrintBT(bstr);
                break;
                case 'H':
                    sprintf((void*)bstr, "MFlag: 0x%hhX",Params.Fields.MQTTFlags);
                    PrintBT(bstr);
                break;
                case 'I':
                    sprintf((void*)bstr, "MKAlive: %u",Params.Fields.MQTTKeepAlive);
                    PrintBT(bstr);
                break;
                case 'J':
                    sprintf((void*)bstr, "MUser: %s",Params.Fields.MQTTUsername);
                    PrintBT(bstr);
                break;
                case 'K':
                    sprintf((void*)bstr, "MPass: %s",Params.Fields.MQTTPassword);
                    PrintBT(bstr);
                break;
                case 'L':
                        sprintf((void*)bstr,"Time: 20%02d-%02d-%02d  %02d:%02d:%02d",sDate.Year,sDate.Month,sDate.Date,R.Hours,R.Minutes,R.Seconds);
                        PrintBT(bstr);
                        break;
                    case 'Z':
                    return;
                break;
                
            }
                    
            putcharBT('#');
            putcharBT('\n');
        }
        
        
    }
        
        BTBuffIndex++;
        if(BTBuffIndex >= BTBUFF_SIZE-1)
            BTBuffIndex = 0;
        
        return;
        //DisconnectDevice();
        //UpdateLED3(RED_COLOR);
        //SystemState = State_IdleState;
DISCARD2: ResetBTBuffer();
        
        
    
}
void GetNetworkData(void);

unsigned char FirstBoot = 0;
unsigned char GSMInactiveCount=0;

unsigned char InitGSM(void)
{
    unsigned char i;
    char*pToken;
    //char str[50];
    #ifdef DEBUG_PRINT
        DebugPrint("Entered-InitGSM\r\n"); 
    #endif
    //printf("InitGSM-ENtered\n");
    //while(1){osDelay(10);}
    //int cYear,cMonth,cDate,cHour,cMinute,cSecond;
    // Make PWRKEY High
    UpdateNetwork(0);
    //GSM_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
    
    //osDelay(5000);
    EnableGSM();
    
    if(FirstBoot == 0)
        goto SKIP_RESET;
    RESTART:
    #ifdef DEBUG_PRINT
        DebugPrint("Goto-RESTART-InitGSM\r\n"); 
    #endif
    DisableGSM();
    osDelay(500);
    EnableGSM();
    osDelay(3000);
    //GSM_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
    gpio_set_level(GPIO_PWRKEY,1);
   
    osDelay(1000);
    gpio_set_level(GPIO_PWRKEY,0);
    //GSM_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);

    
    SKIP_RESET:
    FirstBoot = 1;
    
    osDelay(3000);
    //ResetBuffer();    
    LoopTimeout1 = 0;
    
    while(1)
    {
        
        
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"DONE",4) != NULL) || (LoopTimeout1>10))
        {       break; }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ATREADY",7) != NULL) || (LoopTimeout1>10))
        {       break; }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"QCRDY",5) != NULL) || (LoopTimeout1>10))
        {       break; }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"CPIN: READY",11) != NULL) || (LoopTimeout1>10))
        {       break; }
        osDelay(1);
        
    }
    //printf("init wait done\n");
    UpdateNetwork(0);
    osDelay(5000);
     if(DeviceStatus == 0)
        return 3;
    //osDelay(1000);
    LoopTimeout2 = 0;
    while(SendATCommand("AT\r\n","OK","ERROR",3)==3)//Print("AAAT\r\n");
    {
        if(LoopTimeout2 > 20)
        {
            //printf("No response for AT-retrying\n");
            //esp_restart();
            //goto RESTART;
            //MotionTimer = TIME_TO_SLEEP+1;
            return 3; // return due to no response


        }
    }
    //ResetBuffer();
    if(SendATCommand("AT+CPIN?\r\n","OK","ERROR",3) !=1)
    {
        MotionTimer = TIME_TO_SLEEP+1;
        return 3;
    }        
   
    SendATCommand("AT+CMGF=1\r\n","OK","ERROR",3);//    Print("AT+CMGF=1\r\n");
   
    SendATCommand("AT+CMGF=1\r\n","OK","ERROR",3);//Print("AT+CMGF=1\r\n");
 
    
    
    ESP_LOGI(TAG," Buff2Index = %d",Buff2Index);



    if(Buff2Index == 0) 
    {
        #ifdef DEBUG_PRINT
            DebugPrint("BuffIndex=0-InitGSM\r\n"); 
        #endif
        if(++GSMInactiveCount>10)
        {
            #ifdef DEBUG_PRINT
                DebugPrint("GSMInactiveCount>10-InitGSM\r\n"); 
            #endif
            //BackupPackets();
            //WriteSRAM(BUFF2_RESET); //TBD
            ESP_LOGW(TAG,"Rebooting from InitGSM-GSM Inactive count");
            esp_restart();
        }
        printf("Buff2Index=0\n");
        
        
        goto RESTART;
    } 
        
    SendATCommand("AT+CGMR\r\n","OK","ERROR",3);//Print("AT+CGMR\r\n");

    // Shifted up for faster location
    #ifdef EXT_ANT_ENABLED
    
            // #if defined(SIM7600)  
            //     SendATCommand("AT+CGPS=1,1\r\n","OK","ERROR",3);
            // #endif
            #if defined(SIM7070)  
                SendATCommand("AT+CGNSPWR=1\r\n","OK","ERROR",3);     
            #endif       
            #if defined(A7672)
                SendATCommand("AT+CGNSSPWR=1\r\n","READY","ERROR",60);
                SendATCommand("AT+CGNSSPWR?\r\n","OK","ERROR",60);
                SendATCommand("AT+CGNSSMODE=15\r\n","OK","ERROR",5);   // GPS+GLONASS+BeiDou+Galileo
                SendATCommand("AT+CGPSXE=1\r\n","OK","ERROR",10);      // XTRA extended ephemeris via LTE
            #endif
            #if defined(SIM7672)
                
                SendATCommand("AT+CGNSSPWR=1\r\n","OK","ERROR",60);
                SendATCommand("AT+CGNSSPWR?\r\n","OK","ERROR",60);
                // SendATCommand("AT+CGNSSPORTSWITCH=1,1\r\n","OK","ERROR",5);
                // SendATCommand("AT+CGNSSTST=1\r\n","OK","ERROR",30);
                // SendATCommand("AT+CGPSCOLD\r\n","OK","ERROR",30);
                //SendATCommand("AT+CGNSSFTM=1\r\n","OK","ERROR",30);
                //SendATCommand("AT+CGNSSINFO\r\n","OK","ERROR",30);
                // SendATCommand("AT+CGNSSIPR?\r\n","OK","ERROR",30);
                // SendATCommand("AT+SIMCOMATI\r\n","OK","ERROR",30);
                //  osDelay(5000);
                //  SendATCommand("AT+BT\r\n","OK","ERROR",30);
                // osDelay(5000);
                
                
            #endif
            // SendATCommand("AT+CGPSINFO\r\n","OK","ERROR",3);
        
    #endif


    osDelay(1000);
    // 

    ResetBuffer();
    Print("AT+CSQ\r\n");
    osDelay(500);;
    
    #ifdef SIM800
        SendATCommand("AT+IPR=9600\r\n","OK","ERROR",3);
//        ResetBuffer();
//        Print("AT+IPR=9600\r\n");
//        osDelay(500);
    #endif
    #ifdef SIM7070
        SendATCommand("AT+IPR=115200\r\n","OK","ERROR",3);
//        ResetBuffer();
//        Print("AT+IPREX=115200\r\n");
//        osDelay(500);; 
    #endif
    #ifdef SIM7600
        SendATCommand("AT+IPREX=115200\r\n","OK","ERROR",3);
    #endif

    SendATCommand("AT+CLIP=1\r\n","OK","ERROR",3);
    

//    osDelay(500);
	SendATCommand("AT+CNMI=2,1,0,0,0\r\n","OK","ERROR",3);	

    #ifdef SIM800
        SendATCommand("AT+CLTS=1\r\n","OK","ERROR",3);
        //osDelay(500);
    #else
        SendATCommand("AT+CTZU=1\r\n","OK","ERROR",3);
        //osDelay(500);
    #endif
    
//    ResetBuffer();
    #ifdef SIM800
        SendATCommand("AT+CBAND=\"EGSM_MODE,ALL_BAND\"\r\n","OK","ERROR",3);
    #endif
    #ifdef SIM7070
        #ifdef CNMP_13
        SendATCommand("AT+CNMP=13\r\n","OK","ERROR",3);
        #endif
        #ifdef CNMP_38
        SendATCommand("AT+CNMP=38\r\n","OK","ERROR",3);
        #endif
        #ifdef CNMP_2
        SendATCommand("AT+CNMP=2\r\n","OK","ERROR",3);
        #endif
        
        // NB selection
        #ifdef CMNB_1
        SendATCommand("AT+CMNB=1\r\n","OK","ERROR",3);
        #endif
        
        // NB selection
        #ifdef CMNB_2
        SendATCommand("AT+CMNB=2\r\n","OK","ERROR",3);
        #endif
        
        // NBM1 selection
        #ifdef CMNB_3
        SendATCommand("AT+CMNB=3\r\n","OK","ERROR",3);
        #endif
        
    #endif
    #ifdef SIM7600
        SendATCommand("AT+CNMP=2\r\n","OK","ERROR",3);
    #endif

    sprintf((void*)str,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n",Params.Fields.APNName);
    SendATCommand(str,"OK","ERROR",5);
    
    #ifdef SIM7600
        SendATCommand("ATS0=003\r\n","OK","ERROR",3);
    #endif
    
    // TAISYS
    // SendATCommand("AT+ENSTK=1\r\n","OK","ERROR",3);
    // SendATCommand("AT+MSTK?\r\n","OK","ERROR",3);
    // SendATCommand("AT+MSTK=0,0\r\n","OK","ERROR",3);
    // SendATCommand("AT+MSTK=11,810301250082028281830100\r\n","OK","ERROR",3);
    // SendATCommand("AT+MSTK=4,D30782020181900101\r\n","OK","ERROR",3);
    // SendATCommand("AT+MSTK=11,810301240082028281830100900101\r\n","OK","ERROR",3); // First profile
    // SendATCommand("AT+MSTK=11,810301240082028281830100900102\r\n","OK","ERROR",3); // Second profile
    // TAISYS END

    SendATCommand("AT+CPSI?\r\n","OK","ERROR",3); // For checking network info

    #ifdef SIM800
        SendATCommand("AT&W0\r\n","OK","ERROR",3);
    #endif
       // reset all 

    CheckBattery();

  
    SendATCommand("AT+CIMI\r\n","OK","ERROR",5);
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"\n",1);
    if(pToken != NULL)
    {
        for(i = 0; i < 15; i++)
        {
            IMSI[i] = pToken[i+1];
        }
        IMSI[15] = '\0';
    }
    
    // SendATCommand("AT+COPS=?\r\n","OK","ERROR",500);    
    
    // #ifdef SIM7070
    // osDelay(10000);
    // #endif
    if(CheckNetwork() == 1)
    {
        #ifdef DEBUG_PRINT
            DebugPrint("CheckNetwork returned 1-InitGSM\r\n"); 
        #endif
 
        
        //goto CHECK_NETWORK_AGAIN;//goto RESTART;
        #ifdef NETWORK_FAIL_RESET_ENABLED
            // BackupPackets(); // TBD
            // WriteSRAM(NETWORK_RESET);
            ESP_LOGW(TAG,"Rebooting from InitGSM-Network Fail Reset");
             esp_restart();
        #else
            return 3;
        #endif
        

    }
    UpdateNetwork(1);
    
    
    
    osDelay(1000);
    
    
    SendATCommand("AT+CGSN\r\n","OK","ERROR",5);
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"\n",1);
    if(pToken != NULL)
    {
        for(i = 0; i < 15; i++)
        {
            IMEI[i] = pToken[i+1];
        }
        IMEI[15] = '\0';
        /* Advertise "V4E-<full IMEI>" as the BLE name: the prefix identifies
           the device type in a scan list, the IMEI identifies the unit —
           no serial cable needed. */
        snprintf(ble_device_name, sizeof(ble_device_name), "V4E-%s", IMEI);
        ble_svc_gap_device_name_set(ble_device_name);
        ble_gap_adv_stop();
        ble_app_advertise();
    }

    SendATCommand("AT+CIMI\r\n","OK","ERROR",5);
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"\n",1);
    if(pToken != NULL)
    {
        for(i = 0; i < 15; i++)
        {
            IMSI[i] = pToken[i+1];
        }
        IMSI[15] = '\0';
    }
    
    
    if(Params.Fields.WorkingMode[0]=='S')
    {
        DeleteAllSMS();        
        DDelay();
    }
    //GetNetworkData();
    CheckNetworkLocation();
    
    // Keep modem awake (CSCLK=0) so XCheckGPS AT+CGPSINFO succeeds reliably
    SendATCommand("AT+CSCLK=0\r\n","OK","ERROR",5);
    // CheckNetworkLocation();

    #ifdef EXT_ANT_ENABLED
        XCheckGPS();    // Here for getting time stamp in reboot ping
    #endif
    
    return 0;
}

void InitGPIO(void)
{
    //////////////////////////////////////////////////////////GPIO
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_PWRKEY_GSM_ENABLE_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);


    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_LED_SIGNAL_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

#ifdef ENABLE_TPS_CONTROL

    // gpio_reset_pin(GPIO_NUM_4); Didnt have any effect

    //////////////////////////////////////////////////////////GPIO
    //zero-initialize the config structure.
    //gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_TPS_ENABLE_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
#endif
    ///////////////////////////INT1 and SOS
    //no interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

#ifdef ENABLE_TPS_CONTROL
    /* Initialize selected GPIO as RTC IO, enable output, disable pullup and pulldown, enable hold*/
    gpio_hold_dis(GPIO_TPS_ENABLE);
#endif 
    // gpio_hold_en(GPIO_GSM_ENABLE);
    gpio_hold_dis(GPIO_GSM_ENABLE);
    gpio_deep_sleep_hold_dis();

}

void SystemInit(void) //static void echo_task(void *arg)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    memcpy(Params.Bytes,DefaultParams.Bytes,sizeof(Params));
   
    InitGPIO();
    EnableMainPower();
 
    configure_led();
 

    InitUART();


    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    

    InitAccelerometer();
    
    
}

void StartTimerTask(void *argument)
{
//    #ifdef DEBUG_PRINT
//        DebugPrint("Entered -StartTimerTask\r\n"); 
//    #endif

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    //vTaskDelay(1);
      if(SleepModeEnabled == 1) 
      {

            //osThreadFlagsWait( 1, osFlagsWaitAny, osWaitForever);// TBD
      }
      //HandleGPSData();  
      //   
//    unsigned char i;
    //millis++;
    
    if(SystemTimer > 15 && Params.Bytes[0] == 0)
    {
        ESP_LOGI(TAG,"SystemTimer>15");
        while(1);
    }
    
    /* USER CODE BEGIN SysTick_IRQn 1 */
   
    INT1 = (MotionStatusType)gpio_get_level(GPIO_INT1);
    #ifndef VALTRACK_V4_VTS
        ChargingStatus = (ChargingStatusType)gpio_get_level(GPIO_CHARGER_PIN);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
        if(ChargingStatus == CONNECTED)// && pChargingStatus == DISCONNECTED)
        {
            FrontPanelTimer = 0; // Battery LED Always ON during charging
            UpdateNetwork(3);
            UpdateLocation(0);
            WriteLEDStatus();
            //pChargingStatus = ChargingStatus;
        }
    #endif
        //if(LEDInhibit == 0) 
    //    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,!INT1);
    
    SOS = gpio_get_level(GPIO_SOS);

    
    
    if(POWER_BUTTON == 1) 
        SOSActivated = 1; 
        
    #ifdef POWER_BUTTON_SOS_SWAP
        SOS = POWER_BUTTON;
    #endif
    // reboot on press code
    #ifndef VALTRACK_V4_VTS
        if(SOS == 1)
        {
            ButtonPressTimer = 0;
        }
        if(ButtonPressTimer > 4) 
        {
            ESP_LOGI(TAG,"Rebooting from StartTimer - ButtonLongPress");
            MakeAllLED(PURPLE);
            WriteLEDStatus();
            osDelay(2000);
            //esp_restart();
            //PowerButtonSleep = 1;
            //DeepSleep();
        }
    #endif
    // end
    if(SOS != pSOS)
    {
        
        if(SOS == 0)
        {
            FrontPanelTimer = 0;
      
            #ifndef WB_PIN_CONTROLLED_LED
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET);
            BackupPCAStatus();
            #endif
        }
        if(SOS == 1)
        {
            //ButtonPressTimer = 0;
            #ifndef WB_PIN_CONTROLLED_LED
            if(DeviceStatus == 1)
            {
                RestorePCAStatus();
                //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_RESET);
            }
            #endif
        }
        
        
        pSOS = SOS;
    }
    if(INT1 == 0)
    {
        #ifndef TIMER_ONLY_WAKEUP
            MotionTimer=0;   // NOT debounced - adaptive cadence must react at once

            /* 2.3.52 (D1): ParkLongTimer only surrenders to CONFIRMED motion.
               The de-dupe on SystemTimer matters because this block runs on
               every main-loop pass while the latched pin is still low, so one
               physical event can otherwise be counted many times and defeat
               the confirmation entirely. */
            if (SystemTimer != motion_last_second)
            {
                motion_last_second = SystemTimer;
                if (motion_window_left == 0) {
                    motion_window_left = MOTION_CONFIRM_WINDOW_S;
                    motion_events      = 1;
                } else {
                    motion_events++;
                }
                if (motion_events >= MOTION_CONFIRM_COUNT) {
                    ParkLongTimer = 0;
                    park_reset_count++;
                    motion_window_left = MOTION_CONFIRM_WINDOW_S; // stay armed while moving
                }
            }

            #ifdef VALTRACK_V4_VTS
                if(FrontPanelTimer > 120)
                {
                    MakeAllLED(PURPLE);
                }
                FrontPanelTimer=0;
            #endif
        
        #endif
       
    }
    if(xTaskGetTickCount() - lastTickValue >= 100)//++tSeconds>=1000)
    {
        lastTickValue = xTaskGetTickCount();
        SystemTimer++;
        InactivityTimer++;
        RTCTimeout++;
        IntervalTimer++;
        #ifndef TIMER_ONLY_WAKEUP
        MotionTimer++;
        if (ParkLongTimer <= PARK_LONG_SECONDS)
            ParkLongTimer++;
        /* 2.3.52 (D1): expire the confirmation window. When it lapses the
           partial count is discarded, so isolated knocks never accumulate
           across hours into a false confirmation. */
        if (motion_window_left > 0) {
            motion_window_left--;
            if (motion_window_left == 0)
                motion_events = 0;
        }
        #endif
        ota_check_timer++;
        if (SystemTimer % TRACK_SAMPLE_SECONDS == 0)
            TrackSampleTick();
        PowerSenseTick();
#ifdef ENABLE_HARSH_DRIVING
        HarshSpeedTick();   // 1Hz speed history for classifying harsh events
#endif
//        #ifndef MOTION_CONTROLLED_PINGS
//        MotionTimer=0;
//        #endif
        BatteryTimer++;
        //HeartBeatTimer++;
        LoopTimeout1++;
        LoopTimeout2++;
        LoopTimeout3++;
        FrontPanelTimer++;
        DebounceTimer++;
        ButtonPressTimer++;
        //GSMResetTimer++;
        LPUARTTimer++;
        ConnectivityTimer++;
        
        #ifdef TAMPER_DETECT_MODE 
            if(SOSActivated == 1)
            {
                if(SystemTimer%15 == 0)
                {
                    PostSOSPing();
                    SOSActivated = 0;
                }
            }
        
        #endif
        if(BootTimer<10)
            BootTimer++;
        
        if(FrontPanelTimer == 1)
        {
            //VALTRACK_BLE_Advertise(1); //TBD
            
        }
        if(FrontPanelTimer == 180)
        {
           //VALTRACK_BLE_Advertise(0); //TBD
        }
        {
            // Moving: Params.Fields.PingInterval (30s from BT app)
            // Parked short (5min–48hr): TIME_TO_SLEEP (300s = 5min)
            // Heartbeat wake: use fast interval for the one ping before re-sleeping
            uint32_t effectiveInterval = (ParkLongTimer < TIME_TO_SLEEP || heartbeat_wake)
                ? Params.Fields.PingInterval
                : TIME_TO_SLEEP;
            if((IntervalTimer > effectiveInterval || force_ping_now) && ChargingStatus == DISCONNECTED)
            {
                #ifdef MOTION_CONTROLLED_PINGS
                    if(MotionTimer < TIME_TO_SLEEP
                       || ParkLongTimer < PARK_LONG_SECONDS
                       || heartbeat_wake
                       || force_ping_now)
                    {
                        PostGPing();
                        heartbeat_wake = 0;
                    }
                #else
                    PostGPing();
                    heartbeat_wake = 0;
                #endif

                IntervalTimer = 0;
                force_ping_now = 0;
            }
        }
        #ifdef LPUART_TIMER_RESET_ENABLED
        // If no events and no cache means nothing to send so timer expire is not valid
        if( ((LPUARTTimer>180) && (SleepModeEnabled!=1)) && ( (HeadIndex != TailIndex) || (GPacketCacheIndex != 0)) )//&& MotionTimer < TIME_TO_SLEEP)
        {
            MotionTimer = TIME_TO_SLEEP+10;
           
            DisableGSM();
            BackupPackets();
            //WriteSRAM(LPUART_TIMER_RESET); // TBD
            ESP_LOGI(TAG,"Rebooting from StartTimer - LPUARTTimer>180");
            esp_restart();
        }
        
        // If no events and no cache means nothing to send so timer expire is not valid
        if( (LPUARTTimer>60 && SleepModeEnabled!=1) && ( (HeadIndex != TailIndex) || (GPacketCacheIndex != 0)))// TRYING TO RESET UART AND SEE IF IT RECOVERS
        {
            // HAL_UART_DeInit(&hlpuart1);
            // MX_LPUART1_UART_Init();
        }
        #endif
        if( (HeadIndex == TailIndex) && (GPacketCacheIndex == 0) )
        {
            LPUARTTimer = 0;// To prevent reset on always on long intervals. bracelet project
        }
        
         if(SystemState  == State_ConnectedState)
         {
             MakeAllLED(BLUE);
         }   
         if(AnswerCall == 1)
         {
             Print("ATA\r\n");
             AnswerCall = 0;
         }

        EEPROMReadTimer++;
        

        
        
        if(DeviceStatus == 1)
        {
            UpdateBattery(ADCBatteryVoltage);
            if(FrontPanelTimer>120)
            {
                MakeAllLED(TURN_OFF);
                WriteLEDStatus();
               
            }
            else
            {
                if(BootTimer == 1)
                    MakeAllLED(BLUE);
                if(BootTimer == 2)
                    MakeAllLED(GREEN);
                if(BootTimer == 3)
                    MakeAllLED(RED);
                if(LEDTouched == 1)
                {
                    WriteLEDStatus();
                    LEDTouched = 0;
                }
            }
            // AverageADCSamples();
            //ADCBatteryVoltage = (((float)BatteryADCCount*ADC_REFERENCE*(float)DIVIDER_FACTOR)/4096);
            //ADCBatteryVoltage +=0.3;
            //ADCBatteryVoltage = (((float)BatteryADCCount*3.3*(float)DIVIDER_FACTOR)/4096);
            
            
            //WriteLEDStatus();
        }

        if(SMSSent==0)
            NoSignalTimer++;

        
        
        

        
        tSeconds=0; 
        //HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN);
        //HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        ESP_LOGE(TAG," ST = %d, MT= %d, FT = %d, INT1 = %d, SE = %d, V = %0.2f, HT/LT = %d/%d, G = %c, SW = %d, C = %d",
         SystemTimer,MotionTimer,FrontPanelTimer,INT1,SleepModeEnabled,ADCBatteryVoltage,HeadIndex,TailIndex,GPSStatus,SOS,ChargingStatus);
         
         vTaskDelay(1);
        
    }
   
    
   // PG_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
   // CHG_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
		
		
  /* USER CODE END SysTick_IRQn 1 */
    //osThreadFlagsWait(1,osFlagsWaitAll,osWaitForever);
    //ESP_LOGI(TAG,"End of StartTimerTask %d,SystemTimer = %d",xTaskGetTickCount(),SystemTimer);
  }
  /* USER CODE END 5 */ 
}

uint16_t Addr;
//uint8_t WriteBuffer[256],ReadBuffer[256];
//extern unsigned char Link[];
//extern unsigned char D1,D2,GPSStatus;
//extern double fLat,fLong; 
void SendLocation(void)
{
    
    //unsigned char i;
    WakeUp();
    CheckBattery();
    ResetBuffer();            
    Print((char*)"AT+CMGS=\"");
    Print(rxNumber);
    Print((char*)"\"\r\n");
    DelayProc(850000);
    Print("Location: \r\n");
    Print(Link);
    Print("\r\n");
    Print(BatteryString);
    Print("\r\n -VALTRACK V2 SMS\r\n-www.valetron.com\r\n-www.raviyp.com\r\n");
    putcchar(0x1A);
    DelayProc(850000);            
    
}
void SendLastLocation(void)
{
    
    //unsigned char i;
    WakeUp();
    CheckBattery();
    ResetBuffer();            
    Print((char*)"AT+CMGS=\"");
    Print(Params.Fields.rxNumber);
    Print((char*)"\"\r\n");
    DelayProc(850000);
		#ifdef SOSALERT
	  if(SosAlert==1){
		Print("SOS ALERT:  \r\n");
		}
	  #endif
    Print("Location: \r\n");
    Print("Last Known: \r\n");
    sprintf
    (
        (void*)Link,
        "http://maps.google.com/maps?z=18&q=%lf,%lf",
        pfLat,pfLong
    );
    Print(Link);
    Print("\r\n");
    Print(BatteryString);
    Print("\r\n -VALTRACK V2 SMS\r\n-www.valetron.com\r\n-www.raviyp.com\r\n");
    putcchar(0x1A);
    DelayProc(850000);            
    
}

void SendAlert(void)
{
    
    //unsigned char i;
    WakeUp();
    CheckBattery();
    ResetBuffer();            
    Print((char*)"AT+CMGS=\"");
    Print(Params.Fields.rxNumber);
    Print((char*)"\"\r\n");
    DelayProc(850000);
    Print("Motion Detected: \r\n");
    Print("Location: \r\n");
    
    if(GPSStatus != 'A')
    {
        Print("Last Known: \r\n");
        sprintf
        (
            (void*)Link,
            "http://maps.google.com/maps?z=18&q=%lf,%lf",
            pfLat,pfLong
        );
    }
    else
    {
        sprintf
        (
            (void*)Link,
            "http://maps.google.com/maps?z=18&q=%lf,%lf",
            fLat,fLong
        );
    }
    Print(Link);
    Print("\r\n");
    Print(BatteryString);
    Print("\r\n -VALTRACK V2 SMS\r\n-www.valetron.com\r\n-www.raviyp.com\r\n");
    putcchar(0x1A);
    DelayProc(850000);            
    
}

void GetNetworkLocation(void)
{
    char *pToken;
    if(GPSStatus == 'V')
    {
        ResetBuffer();
        
        SendATCommand("AT+CLBS=1\r\n","+CLBS:","ERROR",15);
        //Print("AT+HTTPREAD=0,50\r\n");
        //DelayProc(850000);
        osDelay(500);
        LoopTimeout1 = 0;
        while(1)
        {
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CLBS:",6);
            if(pToken != NULL)
            {
                sscanf((void*)pToken,"+CLBS: %d",(int*)&NStatus);
                if(NStatus == NL_SUCCESS)
                {
                    double dlat = 0, dlon = 0;
                    //osDelay(1000);//DelayProc(50000);
                    sscanf((void*)Buff2,"+CLBS: %d,%lf,%lf,%d",(int*)&NStatus,&dlat,&dlon,(int*)&NAccuracy);
                    NLat  = clbs_coord(dlat, 90.0);
                    NLong = clbs_coord(dlon, 180.0);
                    sprintf((void*)NLPacket,",\"nlat\":\"%f\",\"nlon\":\"%f\",\"ltype\":\"NL\"",NLat,NLong);
                }
            }
            if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
            {       
                break; 
            }
        }
    }
    else
    {
        NLPacket[0] = 0;
    }
                //ResetBuffer2();    
                //Print
}
typedef struct NetworkDataField
{
    unsigned char Bytes[25];
}NetworkDataFieldType;

typedef union CENGTypeStruct
{
    NetworkDataFieldType Params[6];
    struct FieldsStruct
    {
        unsigned char   Index[25];
        unsigned char mcc_mnc[25];
        unsigned char     lac[25];
        unsigned char  cellid[25];
        unsigned char    rsrp[25];
        unsigned char    rsrq[25];
//        unsigned char mnc[6];
//        unsigned char lac[6];
//        unsigned char c1[6];
//        unsigned char c2[6];
    }Fields;
    
    // unsigned char  Index[2];
    // unsigned char bcch[5];
    // unsigned char rxl[5];
    // unsigned char bsic[5];
    // unsigned char cellid[6];
    // unsigned char mcc[5];
    // unsigned char mnc[5];
    // unsigned char lac[6];
    // unsigned char c1[5];
    // unsigned char c2[5];
}NetworkDataType;
#define NETWORK_DATA_SIZE 6
#define NETWORK_PARAMS_COUNT 6

NetworkDataType NetworkData[NETWORK_DATA_SIZE];

void HandleNetworkTowerData(char *pData,NetworkDataType *pNetworkData)
{
    char *pToken;
    unsigned char i;
    char seps[] =": ,\t\n";
    
    pToken = strtok (pData,seps);
    i=0;
    //if(pToken!= NULL) pToken = strtok (NULL, seps);
    while (pToken != NULL)
    {
        
        pToken = strtok (NULL, seps);
        sscanf (pToken, "%s", (char*)&pNetworkData->Params[i]);
        
        pToken = strtok (NULL, seps);
        
        
        i++;
        if(i>=NETWORK_PARAMS_COUNT) break;
    }
}
char cmdstr[200];
unsigned char TowerCount = 0;
void NetworkJSONData(char *pData)
{
    unsigned char i;
    
    sprintf((void*)pData,"[");
    for(i = 0; i < TowerCount; i++)
    {      
        
        if(i>=3) break; // Sending max 3 tower
            
        sprintf(cmdstr,"{%s,%s,%s}",NetworkData[i].Fields.mcc_mnc,NetworkData[i].Fields.lac,NetworkData[i].Fields.cellid);
        strcat(pData,cmdstr);
        
    }
    strcat(pData,"]");
//    sprintf((void*)pData,"[{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"},{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"},{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"}]",
//        NetworkData[0].Fields.cellid,
//        NetworkData[0].Fields.mcc,
//        NetworkData[0].Fields.mnc,
//        NetworkData[0].Fields.lac,
//        NetworkData[1].Fields.cellid,
//        NetworkData[1].Fields.mcc,
//        NetworkData[1].Fields.mnc,
//        NetworkData[1].Fields.lac,
//        NetworkData[2].Fields.cellid,
//        NetworkData[2].Fields.mcc,
//        NetworkData[2].Fields.mnc,
//        NetworkData[2].Fields.lac
//    );
}
typedef enum TowerDataTypes
{
    NON_INFO_TYPE,
    INTRA_INFO_TYPE
}TowerDataType;
// void GetNetworkData(void)
// {
//     char *pToken,*pToken1,*pToken2;
//     unsigned char i,j,retries;
//     NetworkDataType *pNetworkData;
//     TowerDataType TowerData=NON_INFO_TYPE;
//     retries = 0;
// RETRY_NWD:    
    
//     ResetBuffer();
//     Print( "AT+CNETCI?\r\n"); 
//     osDelay(2000);
    
//     ResetBuffer();
//     Print( "AT+CNETCI=1\r\n"); 
//     osDelay(2000);    
    
//     ResetBuffer();
//     Print( "AT+CNETCI?\r\n");    
//     LoopTimeout1 = 0;
    
//     while(1)
//     {
//         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCINONINFO:",15);
//         if(pToken != NULL)
//         {
//             TowerData=NON_INFO_TYPE;
//             break;
//         }
//         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCIINTRAINFO:",17);
//         if(pToken != NULL)
//         {
//             TowerData=INTRA_INFO_TYPE;
//             break;
//         }
//         if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
//         {       return; }
        
//     }
//     osDelay(5000);
//     j=0;
//     // if(TowerData==NON_INFO_TYPE)
//     {
//         pToken2 = MapForward(Buff2,BUFF2_SIZE,(char*)"MCC-MNC",7);
//         for(i=0;i<9;i++) // Check for 9 towers
//         {   

//             pToken1 = MapForward(pToken2,BUFF2_SIZE,(char*)"MCC-MNC",7);

//             sprintf(cmdstr,"+CNETCINONINFO: %d",i);
//             pToken = MapForward(pToken1-25,25,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length
//                 if(j>=NETWORK_DATA_SIZE) 
//                     break;
//             }
//             sprintf(cmdstr,"+CNETCIINTRAINFO: %d",i);
//             pToken = MapForward(pToken1-25,25,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length
//                 if(j>=NETWORK_DATA_SIZE) 
//                     break;
//             }
//             sprintf(cmdstr,"+CNETCISRVINFO: %d",i);
//             pToken = MapForward(pToken1-25,25,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length
//                 if(j>=NETWORK_DATA_SIZE) 
//                     break;
//             }
//             pToken2=pToken1+50;
//         }
//     }
//     // else if(TowerData==INTRA_INFO_TYPE)
//     // {
//     //     // j=0;
//     //     for(i=0;i<9;i++) // Check for 9 towers
//     //     {
//     //         sprintf(cmdstr,"+CNETCIINTRAINFO: %d",i);
//     //         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
//     //         if(pToken != NULL)
//     //         {
//     //             printf("found = %d\n",i);
//     //             pNetworkData = &NetworkData[j];
//     //             HandleNetworkTowerData(pToken,pNetworkData);
                
//     //             j++; // j is network array length
//     //             if(j>=NETWORK_DATA_SIZE) 
//     //                 break;
//     //         }
//     //     }
//     //     if(j<NETWORK_DATA_SIZE) 
//     //     {
//     //         sprintf(cmdstr,"+CNETCISRVINFO: ");
//     //         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
//     //         if(pToken != NULL)
//     //         {
//     //             printf("found = %d\n",i);
                
//     //             pNetworkData = &NetworkData[j];
//     //             HandleNetworkTowerData(pToken,pNetworkData);
                
//     //             j++; // j is network array length           
                
//     //         }
//     //     }

//     // }
//     TowerCount = j;// For reading later
//     if(j == 0 && retries  == 0)
//     {
//         retries++;
//         goto RETRY_NWD; // Try again once more if no tower data arrived
//     }
//     NetworkJSONData(TowerPacket);
// //    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 1",17);
// //    if(pToken != NULL)
// //    {
// //        pNetworkData = &NetworkData[1];
// //        HandleNetworkTowerData(pToken,pNetworkData);
// //    }
// //    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 2",17);
// //    if(pToken != NULL)
// //    {
// //        pNetworkData = &NetworkData[2];
// //        HandleNetworkTowerData(pToken,pNetworkData);
// //    }
     
// }
void GetNetworkData(void)
{
    char *pToken;
    unsigned char i,j,retries;
    NetworkDataType *pNetworkData;
    TowerDataType TowerData=NON_INFO_TYPE;
    retries = 0;
RETRY_NWD:    
    
    ResetBuffer();
    Print( "AT+CNETCI?\r\n"); 
    osDelay(2000);
    
    ResetBuffer();
    Print( "AT+CNETCI=1\r\n"); 
    osDelay(2000);    
    
    ResetBuffer();
    Print( "AT+CNETCI?\r\n");    
    LoopTimeout1 = 0;
    
    while(1)
    {
        pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCINONINFO:",15);
        if(pToken != NULL)
        {
            TowerData=NON_INFO_TYPE;
            break;
        }
        pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCIINTRAINFO:",17);
        if(pToken != NULL)
        {
            TowerData=INTRA_INFO_TYPE;
            break;
        }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
        {       return; }
        
    }
    osDelay(5000);
    j=0;
    // for(int x=0;x<BUFF2_SIZE;x++)
    // {
    //     printf("%c",Buff2[x]);
    // }

    sprintf(cmdstr,"+CNETCISRVINFO: ");
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,16);
    if(pToken != NULL)
    {   

        // printf("found = srv - %c%c-%c%c%c%c\n",pToken[0],pToken[1],pToken[16],pToken[17],pToken[18],pToken[19]);
        printf("found srv\n");
        pToken[17]='0';
        pToken[18]=',';
        pToken[19]=' ';
        // pToken[20]=',';
        
        
        pNetworkData = &NetworkData[j];
        HandleNetworkTowerData(pToken,pNetworkData);
        
        j++; // j is network array length           
        
    }
    // if(TowerData==NON_INFO_TYPE)
    {
        
        for(i=0;i<9;i++) // Check for 9 towers
        {
            sprintf(cmdstr,"+CNETCINONINFO: %d",i);
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
            if(pToken != NULL)
            {
                printf("found noninfo = %d\n",i);
                pNetworkData = &NetworkData[j];
                HandleNetworkTowerData(pToken,pNetworkData);
                
                j++; // j is network array length
                if(j>=NETWORK_DATA_SIZE) 
                    break;
            }
        }
    }
    // for(int x=0;x<BUFF2_SIZE;x++)
    // {
    //     printf("%c",Buff2[x]);
    // }
    // else if(TowerData==INTRA_INFO_TYPE)
    {
        // j=0;
        for(i=0;i<9;i++) // Check for 9 towers
        {
            sprintf(cmdstr,"+CNETCIINTRAINFO: %d",i);
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,19);
            if(pToken != NULL)
            {
                printf("found intrainfo= %d\n",i);
                pNetworkData = &NetworkData[j];
                HandleNetworkTowerData(pToken,pNetworkData);
                
                j++; // j is network array length
                if(j>=NETWORK_DATA_SIZE) 
                    break;
            }
        }
        // for(int x=0;x<BUFF2_SIZE;x++)
        // {
        //     printf("%c",Buff2[x]);
        // }
        // printf("before j=%d<\n",j);
        // if(j<NETWORK_DATA_SIZE) 
        {
            // printf("inside j<\n");
            
        }
        // for(int x=0;x<BUFF2_SIZE;x++)
        // {
        //     printf("%c",Buff2[x]);
        // }

    }
    TowerCount = j;// For reading later
    if(j == 0 && retries  == 0)
    {
        retries++;
        goto RETRY_NWD; // Try again once more if no tower data arrived
    }
    NetworkJSONData(TowerPacket);
//    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 1",17);
//    if(pToken != NULL)
//    {
//        pNetworkData = &NetworkData[1];
//        HandleNetworkTowerData(pToken,pNetworkData);
//    }
//    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 2",17);
//    if(pToken != NULL)
//    {
//        pNetworkData = &NetworkData[2];
//        HandleNetworkTowerData(pToken,pNetworkData);
//    }
     
}
// void GetNetworkData(void)
// {
//     char *pToken;
//     unsigned char i,j,retries;
//     NetworkDataType *pNetworkData;
//     TowerDataType TowerData=NON_INFO_TYPE;
//     retries = 0;
// RETRY_NWD:    
    
//     ResetBuffer();
//     Print( "AT+CNETCI?\r\n"); 
//     osDelay(2000);
    
//     ResetBuffer();
//     Print( "AT+CNETCI=1\r\n"); 
//     osDelay(2000);    
    
//     ResetBuffer();
//     Print( "AT+CNETCI?\r\n");    
//     LoopTimeout1 = 0;
    
//     while(1)
//     {
//         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCINONINFO:",15);
//         if(pToken != NULL)
//         {
//             TowerData=NON_INFO_TYPE;
//             break;
//         }
//         pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CNETCIINTRAINFO:",17);
//         if(pToken != NULL)
//         {
//             TowerData=INTRA_INFO_TYPE;
//             break;
//         }
//         if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
//         {       return; }
        
//     }
//     osDelay(5000);
//     j=0;
//     if(TowerData==NON_INFO_TYPE)
//     {
        
//         for(i=0;i<9;i++) // Check for 9 towers
//         {
//             sprintf(cmdstr,"+CNETCINONINFO: %d",i);
//             pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length
//                 if(j>=NETWORK_DATA_SIZE) 
//                     break;
//             }
//         }
//     }
//     else if(TowerData==INTRA_INFO_TYPE)
//     {
//         // j=0;
//         for(i=0;i<9;i++) // Check for 9 towers
//         {
//             sprintf(cmdstr,"+CNETCIINTRAINFO: %d",i);
//             pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length
//                 if(j>=NETWORK_DATA_SIZE) 
//                     break;
//             }
//         }
//         if(j<NETWORK_DATA_SIZE) 
//         {
//             sprintf(cmdstr,"+CNETCISRVINFO: ");
//             pToken = MapForward(Buff2,BUFF2_SIZE,(char*)cmdstr,17);
//             if(pToken != NULL)
//             {
//                 printf("found = %d\n",i);
                
//                 pNetworkData = &NetworkData[j];
//                 HandleNetworkTowerData(pToken,pNetworkData);
                
//                 j++; // j is network array length           
                
//             }
//         }

//     }
//     TowerCount = j;// For reading later
//     if(j == 0 && retries  == 0)
//     {
//         retries++;
//         goto RETRY_NWD; // Try again once more if no tower data arrived
//     }
//     NetworkJSONData(TowerPacket);
// //    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 1",17);
// //    if(pToken != NULL)
// //    {
// //        pNetworkData = &NetworkData[1];
// //        HandleNetworkTowerData(pToken,pNetworkData);
// //    }
// //    pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)"+CNETCINONINFO: 2",17);
// //    if(pToken != NULL)
// //    {
// //        pNetworkData = &NetworkData[2];
// //        HandleNetworkTowerData(pToken,pNetworkData);
// //    }
     
// }
// void NetworkJSONData(char *pData)
// {
//     sprintf((void*)pData,"[{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"},{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"},{\"cid\":\"%s\",\"mcc\":\"%s\",\"mnc\":\"%s\",\"lac\":\"%s\"}]",
//         NetworkData[0].Fields.cellid,
//         NetworkData[0].Fields.mcc,
//         NetworkData[0].Fields.mnc,
//         NetworkData[0].Fields.lac,
//         NetworkData[1].Fields.cellid,
//         NetworkData[1].Fields.mcc,
//         NetworkData[1].Fields.mnc,
//         NetworkData[1].Fields.lac,
//         NetworkData[2].Fields.cellid,
//         NetworkData[2].Fields.mcc,
//         NetworkData[2].Fields.mnc,
//         NetworkData[2].Fields.lac
//     );
// }

void ConvertToJSONPacket(HWEventDataType *pPacket,unsigned short *pDataLength, char *pStr)
{
    
query[0] = 0;
//// \"time\":\"20%02d-%02d-%02d %02d:%02d:%02d\",
#ifdef PAYLOAD_NORMAL
    *pDataLength = sprintf((void*)pStr,
    "{\"devid\":\"%s\",\
\"time\":\"20%02d-%02d-%02d %02d:%02d:%02d\",\
\"etype\":\"%s\",\
\"lat\":\"%f\",\
\"lon\":\"%f\",\
\"vbat\":\"%f\",\
\"speed\":\"%f\"%s%s%s}",
        
        IMEI,
        pPacket->GEvent.Year,pPacket->GEvent.Month,pPacket->GEvent.Date,pPacket->GEvent.Hours,pPacket->GEvent.Minutes,pPacket->GEvent.Seconds,
        (ETypes[pPacket->GEvent.EventType].Bytes),
        pPacket->GEvent.Lat,//tfLat,//pPacket->GEvent.fLat,
        pPacket->GEvent.Long,//tfLong,//pPacket->GEvent.fLong,
        pPacket->GEvent.Voltage,//ChargeVoltage,
        pPacket->GEvent.Speed,//pPacket->GEvent.Speed//,
        TowerPacket,
        NLPacket,
        query
    );
#endif
    
}
void ConvertToJSON(HWEventDataType *pPacket,unsigned short *pDataLength, char *pStr)
{
    unsigned short tDataLength=0;
    unsigned char topic[50];
query[0] = 0;


#ifdef PAYLOAD_NORMAL

    memset((void*)topic,0,50);
    #ifdef SIM800
    if(Params.Fields.WorkingMode[0] == 'T')
        sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);
    
    #endif        
    
    *pDataLength = 0;
    tDataLength = sprintf((void*)&pStr[*pDataLength],"%s{\"resource\":[",topic);
    *pDataLength+=tDataLength;
    ConvertToJSONPacket(pPacket,&tDataLength,&pStr[*pDataLength]);
    *pDataLength+=tDataLength;
    for(int i=0;i<PACKET_COUNT-1;i++)
    {
        if(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS)
        {
            tDataLength = sprintf((void*)&pStr[*pDataLength],",");
            *pDataLength+=tDataLength;
            pPacket=&GPacket;
            ConvertToJSONPacket(pPacket,&tDataLength,&pStr[*pDataLength]);
            *pDataLength+=tDataLength;
        }
    }
    tDataLength = sprintf((void*)&pStr[*pDataLength],"]}");
    *pDataLength+=tDataLength;

#endif
    
}
float ADCvalue;
//char cmdstr[200];
unsigned char TCPRetries = 0;
#ifdef SIM7600
char XUDP_Request(char *pFilename, unsigned char pingtype)
{

//    char eventnumber[5];
//    char *pResult;
    //unsigned char retries,i;
//    char *pToken;
//    unsigned char CheckSum;
//    unsigned char cs[2];
    HWEventDataType *pPacket;
    pPacket = &GPacket;
    //retries = 0;
    TCPRetries = 0;
    //unsigned char encodedByte;
    //int X; 
    // unsigned short MQTTProtocolNameLength;
    // unsigned short MQTTClientIDLength;
    // unsigned short MQTTUsernameLength;
    // unsigned short MQTTPasswordLength;
    

    
/*
    #ifndef STANDALONE_DEMO
    CheckSum = GetCheckSum(pPacket->Bytes,34);
    cs[1]=GetAscii(CheckSum&0x0F);  
    cs[0]=GetAscii((CheckSum>>4) & 0x0F);  
    if((pPacket->GEvent.CheckSum[0] != cs[0]) || (pPacket->GEvent.CheckSum[1] != cs[1]))
    {
        retVal=0;
        return 0;
    }
#endif
    */
    
  RESEND_UDP:
    WakeUp();
    SOS = gpio_get_level(GPIO_SOS);
//    if(SOS == 0)
//    {
//        goto SUCCESS;
//    }
    ////Print4("Resending\r\n");
    ////IWDG_ReloadCounter();
    ResetBuffer();
    Print( "AAAAAAAAAAAAAT\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       goto exit; }

    }
    osDelay(1000);
    ResetBuffer();
    Print( "AT+CSCLK=0\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       goto exit; }
        
    }
    osDelay(1000);//DelayProc(250000);
    
//    osDelay(1000);
    ResetBuffer();    
    Print( "AT+NETOPEN\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"+NETOPEN: 0",11) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    }
    osDelay(3000);
////  restart:

    ResetBuffer();
    Print( "AT+IPADDR\r\n");
    osDelay(1000);        
    if(MapForward(Buff2,BUFF2_SIZE,(char*)"0.0.0.0",7) != NULL)
    {

        osDelay(1000);
        ResetBuffer();
        Print("AT+CIPCLOSE\r\n");
        LoopTimeout1 = 0;
        while(1)
        {
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                    goto exit;
            if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
            {       goto exit; }
            
        }  
        
    }
    
    RECONNECT:
    SendATCommand("AT+CIPOPEN=1,\"UDP\",,,5000\r\n","OK","ERROR",15);
    
    while(1)
    {
        if(DeviceStatus == 0)
            return 0;
//        osDelay(3000);
        
        
        if(RTCTimeout>(Params.Fields.PingInterval+20))
         {
             //SystemInternalClock_Config_LP();
             InitRTCAlarm();
         }
         
        if((MotionTimer > TIME_TO_SLEEP) && (QueEmpty==1))
        {
             goto SUCCESS;
            
        }
        CheckBattery();
        
        // HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN); // TBD
        // HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); // TBD
        
        
        osDelay(1000);
//       
//        INT1 = (MotionStatusType)gpio_get_level(GPIO_INT1);
        if(INT1 == 0)
        {
            //for(i=0;i<=0x31;i++)
            //{
            //    VALREAD = I2C_RdReg(i);
            //    INT1 = (MotionStatusType)gpio_get_level(GPIO_INT1);
            //}
//            #ifdef LIS3DH_ENABLED
//            ISRstatus = I2C_RdReg(REG_INT1_SRC);
//            InitAccelerometer();
//            #else
//            InitAccelerometer_mma84();
//            #endif
#ifdef ENABLE_HARSH_DRIVING
            /* DEAD CODE - this is XUDP_Request, not XHTTP_Request (2.3.44).
               OsmAnd pings go through XHTTP_Request (line ~4474+), which never
               touches the accelerometer at all. XUDP_Request is the legacy UDP
               path and is not on the ping route, which is why qpoll measured
               exactly 0 on all three units including the driven van.

               So 2.3.39's "XHTTP_Request's wait loop no longer clears INT2_SRC
               without checking it (it ran every 30s while driving)" was applied
               to the wrong function - it patched code that never runs. Left in
               place and correct-by-inspection rather than deleted, so that if
               this path is ever revived it does not resurrect the old bug.
               qpoll is retained as the standing proof that it stays dead. */
            ping_poll_count++;                     // qpoll - see 2.3.43 note
            last_int2_src = I2C_RdReg(REG_INT2_SRC);
            if (last_int2_src & 0x40)
                HarshEventDetected();
            ISRstatus = I2C_RdReg(REG_INT1_SRC);   // release the pin
#else
            InitAccelerometer();
#endif
            if(MotionTimer > 120)
            {
                PostMotionEvent();
                #ifndef TIMER_ONLY_WAKEUP
                    MotionTimer=0;
                #endif
            }
        }
        
        if(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS)
        {
            
                
            pPacket=&GPacket;
         
            //if(tfLat == 0)tfLat = 8;
            memset(str,0,500);
            //pPacket->GEvent.Speed[9] = '\0';
            //devid[8] = '\0';
            topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);
            

            ConvertToJSON(pPacket,&datalength,str);
            //datalength = sprintf((void*)str,"%s{\"resource\":[{\"devid\":\"%s\"",topic,devid);
            ResetBuffer();
            //Print("AT+CIPSEND\r\n");
            sprintf((void*)cmdstr,"AT+CIPSEND=1,,\"%s\",%s\r\n",Params.Fields.MQTTHost,Params.Fields.MQTTPort);
            
            SendATCommand(cmdstr,">","ERROR",6);
            

//            Print(Params.Fields.MQTTHost);
//            Print("AT+CIPSEND=");
//            Print(Params.Fields.MQTTHost);
//            Print("\r\n");
            LoopTimeout1 = 0;
            while(1)
            {
                if(MapForward(Buff2,BUFF2_SIZE,(char*)">",1) != NULL)
                        break;
                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
                {
                      
                    goto exit; 
                }
                
            }
            
            Print((void*)str);
            putcchar(0x1A);
            //osDelay(3000);
            osDelay(3000);
            
            
        }
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"CLOSED",6) != NULL)
        {
            goto RECONNECT;
        }
        if(Params.Fields.WorkingMode[0] !='U')
            goto SUCCESS;
        
//        SOS = gpio_get_level(GPIO_SOS);
//        if(SOS == 0)
//        {
//            goto SUCCESS;
//        }
      //  CheckBattery();
    }
    
    
    ////Print4(Buff);   
   
    //DelayProc(850000);
    //DelayProc(850000);
    //DelayProc(850000);
    //DelayProc(850000);
    goto SUCCESS;
    //free(string);
SUCCESS: 
    ClearEventCache();
    ConnectivityTimer = 0;
    ResetBuffer();
    Print("AT+CIPCLOSE=1\r\n");
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CIPCLOSE: 1,0",14) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    }          
    osDelay(1000);
    ResetBuffer();
    Print("AT+NETCLOSE\r\n");
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"+NETCLOSE:",10) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    }     
    osDelay(1000);
    
    //Print4("SUCCESS\r\n");
    
    retVal=0;
    return 0;
exit: 
    osDelay(1000);
    ResetBuffer();
    Print("AT+CIPCLOSE=1\r\n");
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    } 
    osDelay(1000);
    ResetBuffer();
    Print("AT+NETCLOSE\r\n");
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
        {       break; }
        
    }
    osDelay(1000);
    //Print4("FAILED\r\n");
    
  
    if(++TCPRetries < 8) 
        goto RESEND_UDP;
    else
    {        
        retVal = 1;
        DisableGSM();
        InitGSM();
    }
    
//    #ifdef EEPROM_FIFO
//    PostEvent( pPacket);//PostEEEvent(pPacket);
//    #endif
    
    RestoreEventCache();


    return 0;
}

/* Convert 2-digit GPS year + month/day/time (UTC) to Unix epoch seconds. */
static long osmand_unix_ts(int yy, int mo, int dd, int hh, int mi, int ss)
{
    int y = 2000 + yy;
    int leap = (y % 4 == 0) && ((y % 100 != 0) || (y % 400 == 0));
    const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long d = (long)(y - 1970) * 365L + (y-1)/4 - (y-1)/100 + (y-1)/400 - 477;
    for (int i = 0; i < mo - 1; i++) d += mdays[i] + (i == 1 ? leap : 0);
    d += dd - 1;
    return d * 86400L + hh * 3600L + mi * 60L + ss;
}

char XHTTP_Request(char *pFilename, unsigned char pingtype)
{

//    char eventnumber[5];
//    char *pResult;
    //unsigned char retries,i;
    HWEventDataType *pPacket;
    char *pToken;
    TickType_t _dl;
    //unsigned char CheckSum,cs[2];
    pPacket = &GPacket;
    //retries = 0; 
    
    if(ChargingStatus == CONNECTED) return 0;
    
//#ifndef STANDALONE_DEMO
//    CheckSum = GetCheckSum(pPacket->Bytes,34);
//    cs[1]=GetAscii(CheckSum&0x0F);  
//    cs[0]=GetAscii((CheckSum>>4) & 0x0F);  
//    if((pPacket->GEvent.CheckSum[0] != cs[0]) || (pPacket->GEvent.CheckSum[1] != cs[1]))
//    {
//        retVal=0;
//        return 0;
//    }
//#endif
    
//   RESEND_HTTP: // TBD
    if(SystemState == State_ConnectedState) return 0; // Return and idle for proper configuration and prevent EEPROM access
    WakeUp();
    if(DeviceStatus == 0)
        return 0;
    float send_lat = pPacket->GEvent.Lat;
    float send_lon = pPacket->GEvent.Long;
    /* Allow pinging for 5 minutes after boot even with no GPS fix so Traccar
       can receive the device and send remote commands (V_RESET, OTA rollback).
       Positions with lat=0/lon=0 are filtered server-side by filter.zero=true
       so they don't pollute the map, but the HTTP response still arrives. */
    bool in_boot_window = (esp_timer_get_time() < 300ULL * 1000000ULL);
    bool live_fix = true; // false = cached/cell position; timestamp omitted so Traccar uses server time
    /* Range guard kept as defense even though the uint32-wrap decode
       (clbs_coord) now yields in-range values; also covers the zero-init
       NStatus == NL_SUCCESS ambiguity via the 0,0 check. */
    bool cell_valid = (NStatus == NL_SUCCESS &&
                       NLat >= -90.0f && NLat <= 90.0f &&
                       NLong >= -180.0f && NLong <= 180.0f &&
                       !(NLat == 0.0f && NLong == 0.0f));
    /* Age of the cached GPS position; <0 means no fix at all this session. */
    long gps_age_s = (last_fix_us != 0)
        ? (long)((esp_timer_get_time() - last_fix_us) / 1000000LL) : -1;

    /* Fix A: SIM7672G cold-start artifact — GNSS reports a "valid" fix near the
       origin while still acquiring. Treated as no-fix. */
    bool origin_artifact = (send_lat > -1.0f && send_lat < 1.0f &&
                            send_lon > -1.0f && send_lon < 3.0f);

    if ((send_lat == 0.0f && send_lon == 0.0f) || origin_artifact) {
        live_fix = false;
        bool cache_valid = (last_good_lat != 0.0f);
        /* Priority corrected in 2.3.33. The cached GPS fix used to win
           unconditionally, so once GNSS died the device reported a coordinate
           that grew days stale while a current cell fix sat unused in NLat/NLong.
           A cache older than GPS_STALE_SECONDS now yields to the cell position:
           ~550m and current beats metre-accurate and three days wrong. */
        bool cache_fresh = cache_valid && gps_age_s >= 0 && gps_age_s < GPS_STALE_SECONDS;

        if (cache_fresh) {
            send_lat = last_good_lat;
            send_lon = last_good_lon;
        } else if (cell_valid) {
            send_lat = NLat;
            send_lon = NLong;
        } else if (cache_valid) {
            /* Stale, but the only position we have — still better than nothing. */
            send_lat = last_good_lat;
            send_lon = last_good_lon;
        } else if (!in_boot_window) {
            return 0;
        }
    } else {
        last_good_lat = send_lat;
        last_good_lon = send_lon;
        nvs_save_position();
    }
    /* Modem CGPSINFO speed is unreliable (0 or a stale value even when moving).
       Report the 1s track-derived speed when fresh; no recent sample = parked = 0. */
    pPacket->GEvent.Speed =
        (track_live_speed_us != 0 &&
         esp_timer_get_time() - track_live_speed_us <= 10LL * 1000000LL)
            ? track_live_speed_kmh : 0.0f;
//    SOS = gpio_get_level(GPIO_SOS);
//    if(SOS == 0)
//    {
//        goto SUCCESS;
//    }
    ////Print4("Resending\r\n");
    ////IWDG_ReloadCounter();
    ResetBuffer();
    //Print( "AAAAAAAAAAAAAT\r\n");
    Print( "AT\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       goto exit; }

    }
    ResetBuffer();
    Print( "AT+CSCLK=0\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       goto exit; }

    }
    // SendATCommand("AT+CFUN=1\r\n","OK","ERROR",5); // Sleep exit
    osDelay(1000);
    if(CheckNetwork() == 1)
    {
        /* No network right now: fail this attempt but KEEP queued packets —
           the exit path re-queues the event and it is retried next interval.
           (ForceToSleep() here used to ClearPackets(), silently losing data.) */
        ESP_LOGI(TAG,"Exiting from XHTTP due to no network\n");
        goto exit;
    }
    

    sprintf((void*)str,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n",Params.Fields.APNName);
    SendATCommand(str,"OK","ERROR",5);
    SendATCommand("AT+CGACT=1,1\r\n","OK","ERROR",10);
    if(SendATCommand("AT+CGACT?\r\n","+CGACT: 1,1","ERROR",10) != 1)
    {
        goto exit;
    }
    /* Fix B: clear any session left open by a previous interrupted call.
       SIM7672G accumulates open sessions if HTTPTERM is missed; each
       subsequent HTTPACTION fires all of them, causing a request storm. */
    ResetBuffer();
    Print("AT+HTTPTERM\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(5000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL) break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl)) break;
    }
    ResetBuffer();
    Print("AT+HTTPINIT\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {
            ResetBuffer();

            break;
        }
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
        {       break; }

    }

    ResetBuffer();
    Print("AT+HTTPPARA=\"CID\",1\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {      break; }

    }
    //IWDG_ReloadCounter();

    /* Drain buffered 1s track samples inside this HTTP session — one
       HTTPPARA/HTTPACTION per sample, each with its own timestamp, no
       per-sample session setup. On any failure, remaining samples stay
       buffered and are retried on the next ping. Snapshot the tail so
       samples recorded during the drain go out next cycle (at 1Hz the
       producer could otherwise keep this loop running indefinitely). */
    unsigned short drain_end = track_tail;
    while (track_head != drain_end)
    {
        TrackSample *smp = &track_buf[track_head];
        size_t _ulen = strlen(Params.Fields.HTTPURL);
        const char *_sep = (_ulen > 0 && Params.Fields.HTTPURL[_ulen-1] == '/') ? "" : "/";
        snprintf(str, sizeof(str),
            "%s%s?id=%s&lat=%f&lon=%f&speed=%f&timestamp=%ld&fwver=" FW_VERSION,
            Params.Fields.HTTPURL, _sep, IMEI,
            smp->lat, smp->lon, smp->speed / 1.852f, // OsmAnd speed param is knots
            osmand_unix_ts(smp->yy, smp->mo, smp->dd, smp->hh, smp->mi, smp->ss));
        ResetBuffer();
        Print("AT+HTTPPARA=\"URL\",\"");
        Print(str);
        Print("\"\r\n");
        LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
        while(1)
        {
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL) break;
            if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
            {   goto exit; }
        }
        ResetBuffer();
        Print("AT+HTTPACTION=0\r\n");
        LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(60000);
        while(1)
        {
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"ACTION:",7) != NULL) break;
            if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
            {   goto exit; }
        }
        osDelay(300); // let the rest of the +HTTPACTION: 0,<code>,<size> URC arrive
        {
            char *_p = MapForward(Buff2, BUFF2_SIZE, (char*)"+HTTPACTION:", 12);
            if (!_p) goto exit;
            while (*_p && *_p != ',') _p++;
            if (*_p == ',') _p++;
            if (atoi(_p) != 200) goto exit;
        }
        track_head = (track_head + 1) % TRACK_BUF_SIZE;
    }

    ResetBuffer();
    /* OsmAnd GET URL: base URL (from BT-app config) + query params.
       Always insert '/' before '?' so the request URI is valid (some HTTP stacks
       reject queries without a path component). */
    {
        size_t _ulen = strlen(Params.Fields.HTTPURL);
        const char *_sep = (_ulen > 0 && Params.Fields.HTTPURL[_ulen-1] == '/') ? "" : "/";
        /* One alarm per ping; accident outranks power, power outranks other
           harsh events (rare collision — the loser stays pending and rides
           the next ping). harsh_alarm_in_flight tells SUCCESS which to clear. */
        const char *_alarm;
        harsh_alarm_in_flight = 0;
        if (harsh_alarm == 4)      { _alarm = "&alarm=accident";         harsh_alarm_in_flight = 1; }
        else if (power_alarm == 1) { _alarm = "&alarm=powerCut"; }
        else if (power_alarm == 2) { _alarm = "&alarm=powerRestored"; }
        else if (harsh_alarm == 1) { _alarm = "&alarm=hardBraking";      harsh_alarm_in_flight = 1; }
        else if (harsh_alarm == 2) { _alarm = "&alarm=hardAcceleration"; harsh_alarm_in_flight = 1; }
        else if (harsh_alarm == 3) { _alarm = "&alarm=hardCornering";    harsh_alarm_in_flight = 1; }
        else                       { _alarm = ""; }
        /* gmax is gone in 2.3.37. It was the peak horizontal g measured by the
           20Hz sampler, and there is no sampler any more. Restoring it needs the
           FIFO capture described in SCI.h (stage 2). */
        char g_part[1] = "";
        /* Harsh-driving health. hstk and i2crec are also gone with the task -
           there is no task stack to watch and no polling to stick the bus.
           What remains is worth keeping: hmin is a general leak/fragmentation
           canary, and hcnt says whether detection is actually firing, which is
           the thing to check after a drive. */
        char h_part[160] = "";
        char lis_part[64] = "";
#ifdef ENABLE_HARSH_DRIVING
        /* ipoll/qpoll added in 2.3.43 - a constant i2src cannot distinguish
           "polling constantly, never fires" from "polled once, never again".
           They immediately found the cause: ipoll ~1 per 11 min (not ~1Hz) and
           qpoll exactly 0 (dead XUDP_Request path).
           apoll (2.3.44) counts InitAccelerometer's now-checked INT2_SRC read.
           NOTE: it does NOT climb at uptime/60. The throttled re-init lives
           inside the same `if(INT1 == 0)` block as ipoll, so 60s is a ceiling
           on how often it re-inits WHEN the pin fires, not a poll rate - it
           inherits ipoll's ~1-per-11-min. Measured 2026-08-04: apoll stuck at 2
           (the two boot-path calls) after 20 min parked with ipoll=0.

           That does not weaken the fix. What matters is that no unchecked
           INT2_SRC read remains anywhere (this site, InitAccelerometer, and the
           dead XUDP_Request one all test bit 6 first), so a latched event
           survives until something counts it. Poll rate now sets reporting
           latency, not whether detection works.
           If hraw is still 0 after a drive where ipoll clearly climbed, the
           sensor genuinely is not triggering and the threshold is next. */
        snprintf(h_part, sizeof(h_part), "&hmin=%lu&hcnt=%lu&hraw=%lu&hnod=%lu&ipoll=%lu&qpoll=%lu&apoll=%lu",
                 (unsigned long)esp_get_minimum_free_heap_size(),
                 (unsigned long)harsh_event_count,
                 (unsigned long)harsh_raw_count,
                 (unsigned long)harsh_nodelta_count,
                 (unsigned long)int1_poll_count,
                 (unsigned long)ping_poll_count,
                 (unsigned long)accel_poll_count);
        /* 2.3.41 diagnostic - see HarshRegReadback(). Temporary: remove once
           the cause of hraw staying 0 is identified. */
        HarshRegReadback(lis_part, sizeof(lis_part));
#endif
        /* Timestamp only for a live GPS fix with a sane date. Cached/cell
           positions previously carried stale (or year-2000) fix times, so
           Traccar's "latest position" stayed pinned on old data; omitting
           the param makes the server use receive time. */
        char ts_part[36] = "";
        if (live_fix && pPacket->GEvent.Year >= 20)
            snprintf(ts_part, sizeof(ts_part), "&timestamp=%ld",
                osmand_unix_ts(pPacket->GEvent.Year, pPacket->GEvent.Month, pPacket->GEvent.Date,
                               pPacket->GEvent.Hours, pPacket->GEvent.Minutes, pPacket->GEvent.Seconds));
        /* Cell-tower position as extra attributes whenever the modem has one
           (AT+CLBS, refreshed every 300s in the main loop). Doubles as the
           compatibility probe: if this modem rejects CLBS they never appear. */
        char nl_part[80] = "";
        if (cell_valid)
            snprintf(nl_part, sizeof(nl_part), "&nlat=%f&nlon=%f&nacc=%d", NLat, NLong, NAccuracy);
        /* GNSS health, so a dead receiver can never masquerade as a parked
           vehicle again (2.3.33). gpsage = seconds since the last real fix,
           -1 = none this session; gpsrec = recovery attempts since boot.
           Only emitted when the position is NOT a live fix, keeping healthy
           pings unchanged.
           gpsnr = polls where AT+CGPSINFO returned nothing parseable (2.3.42);
           it was this case being misread as a valid fix that hid the 50h freeze
           of 2026-08-01, so it now has to be visible in telemetry rather than
           only findable by reading the source. */
        char gps_part[72] = "";
        if (!live_fix)
            snprintf(gps_part, sizeof(gps_part), "&gpsage=%ld&gpsrec=%lu&gpsnr=%lu",
                     gps_age_s, (unsigned long)gnss_recover_count,
                     (unsigned long)gnss_no_reply_count);
        /* Periodic-OTA visibility. The 24h check at the bottom of StartMainTask
           fires on ota_check_timer >= 86400, but "up to date" is indistinguishable
           from "never ran" - both are silent - so a 2026-07-17 note claiming the
           periodic check does not fire has sat unresolved for ~20 versions with
           no way to test it. Static reading cleared all three suspects (the tick
           runs in its own task and keeps 0.3% time; the main-task trap at 7435 is
           inside #ifndef VALTRACK_V4_VTS and so compiled out; the check sits at
           the top level of the main loop), which means the note may simply be
           stale - but four wrong harsh-driving theories say measure, do not argue.
           Reporting the counter makes it observable: watch it climb, cross 86400
           and reset. Comparing it against uptime on the same ping also detects any
           tick under-count for free, since the two should track. Temporary -
           remove once the periodic path is confirmed either way. */
        char ota_part[24] = "";
        snprintf(ota_part, sizeof(ota_part), "&otat=%lu", (unsigned long)ota_check_timer);

        /* Track-buffer health (2.3.47). tqd is the queue depth at ping time -
           i.e. samples recorded but not yet delivered - and tdrp is the running
           count lost to buffer wrap. Both read at the top of the ping, BEFORE
           the drain loop below empties it, so tqd is the backlog the drain is
           about to face rather than whatever is left afterwards.
           tqd ~0 and tdrp 0 means delivery is keeping up and the sample rate
           can safely go higher. tqd climbing during a drive means the ceiling
           is the per-sample HTTP round trip, and the fix is fewer/fatter
           requests, not a faster sampler. */
        char trk_part[32] = "";
        snprintf(trk_part, sizeof(trk_part), "&tqd=%u&tdrp=%lu",
                 (unsigned)((track_tail - track_head + TRACK_BUF_SIZE) % TRACK_BUF_SIZE),
                 (unsigned long)track_drop_count);

        /* Emitted ONLY on the staging channel (2.3.51), so production pings are
           byte-identical to before and nothing has to be un-learned. Its
           presence is the whole signal: a unit showing otach=1 is on a test
           build and must be brought home with `Moved V_OTA` before it is
           forgotten about. Absence means production. */
        char och_part[16] = "";
        if (ota_channel == OTA_CHANNEL_STAGING)
            snprintf(och_part, sizeof(och_part), "&otach=1");

        /* Deep-sleep observability (2.3.52, D1). Until now "not yet 48h
           stationary" and "the timer keeps resetting" were indistinguishable
           from server data, which is why D1 sat as a suspicion for weeks.
             plt  - ParkLongTimer, seconds of confirmed stillness. Deep sleep
                    fires at PARK_LONG_SECONDS (172800). Should climb 1:1 with
                    uptime on an undisturbed unit.
             pltr - times motion was CONFIRMED and plt was zeroed.
           Read pltr AGAINST ipoll: ipoll counts every assertion, pltr counts
           only those that survived confirmation. On the bench ipoll should
           climb while pltr stays 0 - that gap IS the fix working. If pltr
           tracks ipoll, the window is too permissive. */
        char plt_part[40] = "";
        snprintf(plt_part, sizeof(plt_part), "&plt=%lu&pltr=%lu",
                 (unsigned long)ParkLongTimer, (unsigned long)park_reset_count);

        snprintf(str, sizeof(str),
            "%s%s?id=%s&lat=%f&lon=%f&speed=%f%s&vbat=%f&ncsq=%s&ignition=%s&uptime=%lu%s%s%s%s%s%s%s%s%s%s&fwver=" FW_VERSION,
            Params.Fields.HTTPURL, _sep, IMEI,
        send_lat, send_lon, pPacket->GEvent.Speed / 1.852f, // OsmAnd speed param is knots
        ts_part,
        ADCBatteryVoltage, // live ADC read — GEvent.Voltage (ChargeVoltageF) is never refreshed
        SignalStrength,
        ign_on ? "true" : "false",
        (unsigned long)(esp_timer_get_time() / 1000000ULL), // reboots visible server-side
        nl_part,
        gps_part,
        g_part,
        h_part,
        lis_part,
        trk_part,
        ota_part,
        och_part,
        plt_part,
        _alarm);
    }
    Print("AT+HTTPPARA=\"URL\",\"");
    Print(str);
    Print("\"\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       break; }

    }


    ////IWDG_ReloadCounter();
    ResetBuffer();
    Print("AT+HTTPACTION=0\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(60000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"ACTION:",7) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {
            goto exit;
        }

    }
    if(Version[0]!=0)
    {
        memset(Version,0,sizeof(Version));
    }
    if(query[0]!=0)
    {
        memset(query,0,sizeof(query));
    }
    // OsmAnd protocol returns HTTP 200 with 0-byte body — HTTPREAD would ERROR.
    // Parse +HTTPACTION: 0,<code>,<size>; if 200 + empty body, accept immediately.
    osDelay(300); // let the rest of the +HTTPACTION: 0,<code>,<size> URC arrive
    {
        char *_p = MapForward(Buff2, BUFF2_SIZE, (char*)"+HTTPACTION:", 12);
        if (_p) {
            while (*_p && *_p != ',') _p++;
            if (*_p == ',') _p++;
            if (atoi(_p) == 200) {
                while (*_p && *_p != ',') _p++;
                if (*_p == ',') _p++;
                if (atoi(_p) == 0) goto SUCCESS;
            }
        }
    }

    osDelay(1000);
    ResetBuffer();

    SendATCommand("AT+HTTPREAD=0,50\r\n","+HTTPREAD: ","ERROR",10);
    //Print("AT+HTTPREAD=0,50\r\n");
    //DelayProc(850000);
    osDelay(500);
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        // #ifdef SHEETS_ENABLED
        // if(MapForward(Buff2,BUFF2_SIZE,(char*)"Moved",5) != NULL)
        // #else
        if( (MapForward(Buff2,BUFF2_SIZE,(char*)"logid",5) != NULL) || (MapForward(Buff2,BUFF2_SIZE,(char*)"Moved",5) != NULL))
        // #endif
        {
            ConnectivityTimer = 0;
            osDelay(1000);//DelayProc(50000);
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"SET_RELAY",9) != NULL)
            {
                //ResetBuffer2();    
                //Print2((void*)CSetRelay);
            }
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"RESET_RELAY",11) != NULL)
            {
                //ResetBuffer2();     
                //Print2((void*)CResetRelay);
            }
            // Dont process while sending image
            if(ImageSent == 1)
            {
                pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"IMG_",4);
                if(pToken != NULL)
                {
                    DelayProc(850000);
                    DelayProc(850000);
                    //ReadImagePacket(&pToken[4],&EEImagePacket);//"20160112_2016_09_03_17_01_33_11_15",&EEImagePacket,0);                
                }
            }
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"V_RESET",7) != NULL)
            {
                /* Remote reboot via Traccar custom command "Moved V_RESET".
                   Save position to NVS so next boot can ping immediately. */
                nvs_save_position();
                Print("AT+HTTPTERM\r\n");
                osDelay(500);
                esp_restart();
            }
            /* ORDER MATTERS: "V_OTA" is a prefix of "V_OTA_TEST", and MapForward
               is a substring search, so a bare V_OTA test would also fire on
               V_OTA_TEST. Match the longer command first and make the V_OTA
               branch conditional on it having missed. */
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"V_OTA_TEST",10) != NULL)
            {
                /* "Moved V_OTA_TEST" - move THIS device to the staging channel
                   and update from it. Per-device by construction: Traccar sends
                   the command to one device. Channel is written to NVS first so
                   it survives the reboot the update causes; otherwise the next
                   boot check would read production and pull straight back. */
                ota_channel_save(OTA_CHANNEL_STAGING);
                Print("AT+HTTPTERM\r\n");
                osDelay(500);
                ota_check_timer = 86400UL; // trigger periodic OTA path on next loop
            }
            else if(MapForward(Buff2,BUFF2_SIZE,(char*)"V_OTA",5) != NULL)
            {
                /* Remote OTA check via Traccar custom command "Moved V_OTA".
                   Runs CheckAndApplyOTA immediately; reboots if update found.
                   Also the way home from staging: it resets the channel to
                   production, so a staged unit comes back on the next check.
                   The version comparison is strcmp-inequality, so returning to
                   an older production build works as a downgrade. */
                ota_channel_save(OTA_CHANNEL_PRODUCTION);
                Print("AT+HTTPTERM\r\n");
                osDelay(500);
                ota_check_timer = 86400UL; // trigger periodic OTA path on next loop
            }
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"PING_NOW",8) != NULL)
            {
                /* Remote immediate ping via Traccar custom command "Moved PING_NOW".
                   Sets flag checked by main loop; next tick sends a position report
                   without waiting for the normal interval. */
                force_ping_now = 1;
            }
            if(MapForward(Buff2,BUFF2_SIZE,(char*)"GET_VER",7) != NULL)
            {
                sprintf((void*)Version,"\"ver\":\"%s-%s\",",SYS_VERSION,CUSTOMER);
            }
            
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"SET_INT_",8);
            if(pToken != NULL)
            {
                //ResetBuffer2();     
                switch(pToken[8])
                {
                    case 'A':
                        //Print2((void*)CSetInt005);
                        Params.Fields.PingInterval=5;
                    break;
                    case 'B':
                        //Print2((void*)CSetInt010);
                        Params.Fields.PingInterval=10;
                    break;
                    case 'C':
                        //Print2((void*)CSetInt015);
                        Params.Fields.PingInterval=15;
                    break;
                    case 'D':
                        //Print2((void*)CSetInt030);
                        Params.Fields.PingInterval=30;
                    break;
                    case 'E':
                        //Print2((void*)CSetInt045);
                        Params.Fields.PingInterval=45;
                    break;
                    case 'F':
                        //Print2((void*)CSetInt060);
                        Params.Fields.PingInterval=60;
                    break;
                    case 'G':
                        //Print2((void*)CSetInt120);
                        Params.Fields.PingInterval=120;
                    break;
                    case 'H':
                        //Print2((void*)CSetInt180);
                        Params.Fields.PingInterval=180;
                    break;
                    case 'I':
                        //Print2((void*)CSetInt240);
                        Params.Fields.PingInterval=240;
                    break;
                }
                /*DelayProc(850000);
                DelayProc(850000);
                DelayProc(850000);
                DelayProc(850000);
                DelayProc(850000);
                DelayProc(850000);*/
									
            }                 
           
        
            break;
        }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       goto exit; }
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL))
        {
                break;
        }
//        goto exit; }


    }
    
      
    ////Print4(Buff);   
   
    //DelayProc(850000);
    //DelayProc(850000);
    //DelayProc(850000);
    //DelayProc(850000);
    goto SUCCESS;
    //free(string);
SUCCESS:
    esp_ota_mark_app_valid_cancel_rollback();
    /* Clear only the alarm source that was in this ping's URL; a pending alarm
       from the other source rides the next ping. Failed sends keep both. */
    if (harsh_alarm_in_flight) { harsh_alarm = 0; harsh_alarm_in_flight = 0; }
    else                       { power_alarm = 0; }
    ClearEventCache();
    if(RFIDDataPresent==1)
    {
        if(RFID[0]!=0)
        {
            memset(RFID,0,20);
        }
    }
    ResetBuffer();
    Print("AT+HTTPTERM\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       break; }

    }
    // SendATCommand("AT+CFUN=0\r\n","OK","ERROR",5);
    // Modem stays at CSCLK=0 so XCheckGPS GPS reads succeed in the main loop

    //Print4("SUCCESS\r\n");
    retVal=0;
    return 0;
exit:
    ResetBuffer();
    Print("AT+HTTPTERM\r\n");
    LoopTimeout1 = 0; _dl = xTaskGetTickCount() + pdMS_TO_TICKS(30000);
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (xTaskGetTickCount() >= _dl))
        {       break; }

    }
    //Print4("FAILED\r\n");
    
    /*if(PGEvent.EventType == 35 && retries >=1 && Params.Fields.PingInterval <=10)
    {
        retVal=0;
        #ifdef EEPROM_FIFO
        PostEvent( pPacket);//PostEEEvent(pPacket);
        #endif
        return 0;
    }*/
//    if(++retries < 8) goto RESEND_HTTP;
//    else retVal = 1;
//    
    //#ifdef EEPROM_FIFO
    //PostEEEvent(pPacket);
    RestoreEventCache();
    //PostEvent(pPacket);
    //#endif
//    if(Params.Fields.PingInterval > 30)
//    {
//        ResetBuffer();
//        Print( "AT+CSCLK=0\r\n");    
//        LoopTimeout1 = 0;
//        while(1)
//        {
//            if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
//                    break;
//            if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>30))
//            {       break; }
//        
//        }
//
//    }
    return 1; // failure: caller counts retries and power-cycles the modem after 3
}
#endif // SIM7600
#ifdef SIM7070
void URLDivider(char *pURL,int URLSize, char *pDomain,int DomainSize, char *pPath, int PathSize)
{

    char seps[3] ="/";
    char *pToken;
    unsigned char i=0;
    unsigned short dlength=0,plength=0;
    char URL[200];
    memcpy(URL,pURL,URLSize); // Because strtok destroys original string
    // dlength = sizeof(pDomain);
    memset(pDomain,0,DomainSize);
    //ESP_LOGI(TAG,"%d,%d\r\n",DomainSize,PathSize);
    // plength = sizeof(pPath);
    memset(pPath,0,PathSize);
    
    pToken = strtok (URL,seps);
    if(pToken != NULL)
    {   
        strcat(pDomain,pToken);
        strcat(pDomain,"//");
        pToken = strtok (NULL,seps);
        //ESP_LOGI(TAG,"%s\r\n",pToken);
        if(pToken != NULL)
            strcat(pDomain,pToken);
    }
  
    while (pToken != NULL)
    {
        
        pToken = strtok (NULL, seps);
        
        if(pToken != NULL) 
        {
            strcat(pPath,"/");
            strcat(pPath,(void*)pToken);
        }            
        else
        {
            break;
        }

        i++;
        if(i>10) break;
    }
}

char YHTTP_Request(char *pFilename, unsigned char pingtype)
{
    //char eventnumber[5];
    //char *pResult;
    //unsigned char retries,i;
    char *pToken;//,*pData;
    //char *pToken1;
    //unsigned char CheckSum,cs[2];
    pPacket = &CPacket;
    //retries = 0;
    //TCPRetries = 0;
//    unsigned char encodedByte;
//    int X; 
    int ResponseLength=0;
    //unsigned short MQTTProtocolNameLength;
    unsigned short MQTTClientIDLength;
    //unsigned short MQTTUsernameLength;
    //unsigned short MQTTPasswordLength;
//    unsigned char k=0;
    char Domain[150],Path[150],Header[200],KeyName[50],KeyValue[100];
    #ifdef DEBUG_PRINT
        
        DebugPrint("Entered-TCP_request\r\n"); 
    #endif
    
    
/*
    #ifndef STANDALONE_DEMO
    CheckSum = GetCheckSum(pPacket->Bytes,34);
    cs[1]=GetAscii(CheckSum&0x0F);  
    cs[0]=GetAscii((CheckSum>>4) & 0x0F);  
    if((pPacket->GEvent.CheckSum[0] != cs[0]) || (pPacket->GEvent.CheckSum[1] != cs[1]))
    {
        retVal=0;
        return 0;
    }
#endif
    */
    
//   RESEND_TCP:
    WakeUp();
    SOS = gpio_get_level(GPIO_SOS);
    if(SOS == 0)
    {
        goto SUCCESS;
    }
    ////Print4("Resending\r\n");
    ////IWDG_ReloadCounter();
    ResetBuffer();
    Print( "AAAAAAAAAAAAAT\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
        {       goto EXIT_MQTT; }
        Count++;
    }
   
    // RECONNECT:

    #ifdef DEBUG_PRINT
        
        DebugPrint("CCHSTART OK-TCP_request"); 
    #endif
    // RESTART_MQTT:
    
  /////////////////////////////////////////////////////////////////////////
    MQTTClientIDLength = strlen((void*)IMEI);//strlen(Params.Fields.MQTTClientID);
    topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);
    
    // if(SendATCommand("AT+CNACT=0,1\r\n","0,ACTIVE","ERROR",10) != 1) goto EXIT_MQTT;
    SendATCommand("AT+CNACT=0,1\r\n","0,ACTIVE","ERROR",10);
    
//RECHECK_IP:    
    if(SendATCommand("AT+CNACT?\r\n","+CNACT: 0,0,\"0.0.0.0\"","OK",10) == 1)
    //if(SendATCommand("AT+CNACT?\r\n","+CNACT: 0,1","OK",10) != 1)
    {
        sprintf((void*)str,"NO IP ADDRESS");
        //if(SendATCommand("AT+CFUN=0\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        sprintf(
            (void*)cmdstr,
            "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",
            Params.Fields.APNName
        );
        if(SendATCommand(cmdstr,"OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        //if(SendATCommand("AT+CFUN=1\r\n","+CPIN: READY","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CFUN=1\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        if(SendATCommand("AT+CGATT=1\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CGATT?\r\n","+CGATT: 1","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CGNAPN\r\n","+CGNAPN: 1,","ERROR",10) != 1) goto EXIT_MQTT;
        sprintf(
            (void*)cmdstr,
            "AT+CNCFG=0,1,\"%s\",\"%s\",\"%s\"\r\n",
            Params.Fields.APNName,
            Params.Fields.APNUsername,
            Params.Fields.APNPassword
        );
        if(SendATCommand(cmdstr,"OK","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CNACT=0,1\r\n","+APP PDP: 0,ACTIVE","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CNACT?\r\n","+CNACT: 0,1,\"0.0.0.0\"","OK",10) == 1) goto EXIT_MQTT;
    }
    sprintf((void*)str,"IP ADDRESS OK");
    ///////////////////////////////////////////////////////////////
//    if(Params.Fields.MQTTPort[0] == '8')
//        sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\",1\r\n",IMEI);
//    else
//        sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\"\r\n",IMEI);
//    
//    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
//    {
//        sprintf((void*)str,"CLIEND ID FAILED");
//        goto EXIT_MQTT;
//    }
//    sprintf((void*)str,"CLIENT ID OK");

    ////////////////////////////////////////////////////////////////

// RECONNECT_MQTT:    
    
    URLDivider(Params.Fields.HTTPURL,sizeof((Params.Fields.HTTPURL)),Domain,sizeof(Domain),Path, sizeof(Path));
    ESP_LOGI(TAG,"%s---\r\n",Path);
    if(MapForward(Params.Fields.HTTPURL,sizeof(Params.Fields.HTTPURL),(char*)"https",5) != NULL)
    {
        SendATCommand("AT+CSSLCFG=\"sslversion\",1,3\r\n","OK","ERROR",10);
        SendATCommand("AT+SHSSL=1\r\n","OK","ERROR",10);

    }
    sprintf((void*)cmdstr,"AT+SHCONF=\"URL\",\"%s\"\r\n",Domain);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"URL FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SHCONF=\"BODYLEN\",1024\r\n");
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"BODYLEN FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SHCONF=\"HEADERLEN\",350\r\n");
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"HEADER FAILED");
        goto EXIT_MQTT;
    }

//    sprintf(
//        (void*)cmdstr,
//        "AT+CMQTTCONNECT=0,\"tcp://%s:%s\",60,1,\"%s\",\"%s\"\r\n",
//        Params.Fields.MQTTHost,
//        Params.Fields.MQTTPort,
//        Params.Fields.MQTTUsername,
//        Params.Fields.MQTTPassword
//        );
    if(SendATCommand("AT+SHCONN\r\n","OK","ERROR",20) != 1)
    {
        sprintf((void*)str,"CONNECT FAILED");
        goto EXIT_MQTT;
    }
    if(SendATCommand("AT+SHSTATE?\r\n","+SHSTATE: 1","ERROR",20) != 1)
    {
        sprintf((void*)str,"CONNECT CHECK FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)str,"CONNECT OK");
    
    if(SendATCommand("AT+SHCHEAD\r\n","OK","ERROR",20) != 1)
    {
        sprintf((void*)str,"CLEAR HEADER FAILED");
        goto EXIT_MQTT;
    }
    if(SendATCommand("AT+SHAHEAD=\"Content-Type\",\"application/json\"\r\n","OK","ERROR",20) != 1)
    {
        sprintf((void*)str,"JSON HEAD FAILED");
        goto EXIT_MQTT;
    }
    memcpy(Header,Params.Fields.HTTPKey,sizeof(Header)); // Because strtok destroys original string
    pToken = (void*)strtok ((void*)Header,":");
    if(pToken != NULL)
    {
        sscanf((void*)pToken,"%s",KeyName);
    }
    pToken = (void*)strtok (NULL,":");
    if(pToken != NULL)
    {
        sscanf((void*)pToken,"%s",KeyValue);
    }
    GetEEParams(); // Restore params
    //sprintf((void*)cmdstr,"AT+SHAHEAD=\"X-DreamFactory-Api-Key\",\"%s\"\r\n","699cef40368daa8e98d2684830aef4e2fe2d8b9a6c0c1f9da9125cec266c479e");//Params.Fields.HTTPKey);
    sprintf((void*)cmdstr,"AT+SHAHEAD=\"%s\",\"%s\"\r\n",KeyName,KeyValue);//Params.Fields.HTTPKey);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"API KEY FAILED");
        goto EXIT_MQTT;
    }
   
    #ifdef DEBUG_PRINT
        
        DebugPrint("ConnectPKT_OK-TCP_request\r\n"); 
    #endif
   while(1)
    {
        #ifdef DEBUG_PRINT
        
            DebugPrint("PUB while 1 Enter-TCP_request\r\n"); 
        #endif
        CheckBattery();
        
        if(CheckNetwork() == 1)
            UpdateNetwork(0);
        else
            UpdateNetwork(1);
        if(MotionTimer > TIME_TO_SLEEP)
        {
            retVal=0;
            return 0;
        }
//////////////////////////////////////////////////////////
    #ifdef EXT_ANT_ENABLED 
        XCheckGPS();

    #endif
    #ifdef NETWORK_LOCATION_ENABLED
        GetNetworkLocation();
    #endif
///////////////////////////////////////////////////////////      
        
        if(SystemState == State_ConnectedState) return 0; // Return and idle for proper configuration and prevent EEPROM access
        
      
            memset(str,0,sizeof(str));
       
            ConvertToJSON(pPacket,&datalength,str);
            //osDelay(2000);
            sprintf((void*)cmdstr,"AT+SHBOD=%d,10000\r\n",datalength);
            // Print(cmdstr);
            //osDelay(1000);
           if(SendATCommand(cmdstr,">","ERROR",20) != 1)
           {
               // Dont use str here
               ESP_LOGI(TAG,"BODY failed");
               goto EXIT_MQTT;
           }
            //osDelay(4000);
            Print(str); 
            LoopTimeout1 = 0;
            while(1)
            {
                if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                {
                    ESP_LOGI(TAG,"PAYLOAD SUCCESS");
                    //TCPRetries=0;
                    ConnectivityTimer = 0;
                    break;
                }
                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
                {   
                    ESP_LOGI(TAG,"PAYLOAD ERROR");                    
                    goto EXIT_MQTT; 
                        
                }
                
            }
//            sprintf((void*)cmdstr,"AT+SHBOD?\r\n");
//            if(SendATCommand(cmdstr,"+SHBOD:","ERROR",20) != 1)
//            {
//                sprintf((void*)str,"BODY READ FAILED");
//                goto EXIT_MQTT;
//            }
            
            sprintf((void*)cmdstr,"AT+SHREQ=\"%s\",3\r\n",Path);
            //sprintf((void*)cmdstr,"AT+SHREQ=\"/api/v2/update\",3\r\n");
            SendATCommand(cmdstr,"POST","ERROR",200);
            
            
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"\"POST\",200",10);
            if(pToken!= NULL) 
            {                
                sscanf(pToken+11,"%3d",&ResponseLength);
                ESP_LOGI(TAG,"PUBLISH SUCCESS-200");
            }
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"\"POST\",302",10);
            if(pToken!= NULL)             
            {
                sscanf(pToken+11,"%3d",&ResponseLength);
                ESP_LOGI(TAG,"PUBLISH SUCCESS-302");
            }
            else
            {
                ESP_LOGI(TAG,"PUBLISH FAILED");
                goto EXIT_MQTT;
            }
            sprintf((void*)cmdstr,"AT+SHREAD=0,%d\r\n",ResponseLength);
            if(SendATCommand(cmdstr,"+SHREAD:","ERROR",20) != 1)
            {
                ESP_LOGI(TAG,"READ FAILED");
                //goto EXIT_MQTT;
            }
            osDelay(2000);
//            if(SendATCommand("AT+SHREQ=\"%s\",3\r\n","\"POST\",200","ERROR",20) != 1)
//            {
//                sprintf((void*)str,"PUBLISH FAILED");
//                goto EXIT_MQTT;
//            }
            ESP_LOGI(TAG,"PUBLISH SUCCESS");   

            osDelay(3000);
            #ifdef DEBUG_PRINT
        
                DebugPrint("PUB complete -TCP_request\r\n"); 
            #endif
            goto SUCCESS;
        


        if(MapForward(Buff2,BUFF2_SIZE,(char*)"CLOSE",5) != NULL)
        {
            #ifdef DEBUG_PRINT
        
                DebugPrint("CLOSED-TCP_request\r\n"); 
            #endif
            goto EXIT_MQTT;//goto RECONNECT;
        }
        if(Params.Fields.WorkingMode[0] !='H')
            goto SUCCESS;
        
        SOS = gpio_get_level(GPIO_SOS);
        if(SOS == 0)
        {
            goto SUCCESS;
        }
        
        //CheckBattery();
     
        
    }
    
    
    goto SUCCESS;
    //free(string);
SUCCESS: 
    ClearEventCache();
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_SUCCESS -TCP_request\r\n"); 
    #endif
    //SendATCommand("AT+CMQTTREL=0\r\n","OK","ERROR",10);
    //osDelay(1000);
    SendATCommand("AT+SHDISC\r\n","OK","ERROR",10);
    // SendATCommand("AT+CNACT=0,0\r\n","DEACTIVE","ERROR",10);
//    #ifndef TIMER_ONLY_WAKEUP
//    goto RECONNECT_MQTT;
//    #endif
//    
    //osDelay(1000);
    //SendATCommand("AT+CMQTTSTOP\r\n","+CMQTTSTOP:","ERROR",10);
    //osDelay(1000);
    
    
    //Print4("SUCCESS\r\n");
   
    retVal=0;
    return 0;
//exit: 
EXIT_MQTT:
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_EXIT -TCP_request\r\n"); 
    #endif
    //if(Params.Fields.MQTTPort[0] != '1')
    {
        
        SendATCommand("AT+SHDISC\r\n","OK","ERROR",10);
        // SendATCommand("AT+CNACT=0,0\r\n","DEACTIVE","ERROR",10);
        //osDelay(1000);
        
    }
//    ResetBuffer();
//    Print("AT+NETCLOSE\r\n");
//    LoopTimeout1 = 0;
//    while(1)
//    {
//        if(MapForward(Buff2,BUFF2_SIZE,(char*)"NETCLOSE",8) != NULL)
//                break;
//        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>5))
//        {       break; }
//        Count++;
//    }
//    osDelay(1000);
    //Print4("FAILED\r\n");
    
  
//    if(++TCPRetries < 8) 
//        goto RESEND_TCP;
//    else
//    {        
//        #ifdef DEBUG_PRINT
//        
//            DebugPrint("TCP_Retry_exceeded -TCP_request\r\n"); 
//        #endif
//        retVal = 1;
////        HAL_UART_MspDeInit(&hlpuart1);
////        MX_LPUART1_UART_Init();
//        DisableGSM();
//        InitGSM();
//        TCPRetries = 0;
//    }
      
//    #ifdef EEPROM_FIFO
//    PostEEEvent(pPacket);
//    #endif
    
    //PostEvent(pPacket);
    RestoreEventCache();
    
    
    return 0;
}

#endif // SIM7070
#ifdef SIM800
char ZHTTP_Request(char *pFilename, unsigned char pingtype)
{}
#endif // SIM800
const unsigned char ConnectPacket[46]=
{
    0x10,0x2C,0x00,0x06,0x4D,0x51,0x49,0x73,0x64,0x70,0x03,0xC2,0x00,0x3C,0x00,0x06,0x41,0x42,0x43,0x44,0x45,0x46,0x00,
    0x08,0x74,0x63,0x6F,0x71,0x61,0x7A,0x63,0x7A,0x00,0x0C,0x77,0x51,0x30,0x54,0x57,0x6B,0x4C,0x58,0x49,0x51,0x44,0x7A
};

const unsigned char PublishPacket[21]=
{
    0x30,0x13,0x00,0x08,0x76,0x61,0x6C,0x65,0x74,0x72,0x6F,0x6E,0x68,0x65,0x6C,0x6C,0x6F,0x72,0x61,0x76,0x69
};

const unsigned char SubscribePacket[15]=
{
    0x82,0x0D,0x00,0x01,0x00,0x08,0x76,0x61,0x6C,0x65,0x74,0x72,0x6F,0x6E,0x00
};


const unsigned char PingPacket[2]=
{
    0xC0,0x00
};

#ifdef SIM7070

void XCheckGPS(void)
{
    char *pToken;
    // char *pData; // TBD

    SendATCommand("AT+CGNSINF\r\n","OK","ERROR",3);
    osDelay(500);



    Count=0;

    // if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CGPSINFO: ,,",13) != NULL)
    //     GPSStatus = 'V';
    // else
    //     GPSStatus = 'A';
    GPSStatus = 'A';
    if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CGNSINF: 0,,",13) != NULL)
        GPSStatus = 'V';
    else if(MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL)
        GPSStatus = 'V';

    UpdateLocation(GPSStatus);

    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CGNSINF:",9);
    if(pToken != NULL)
    {
        unsigned char in = 0;
        f=0;
        //pData = tBuff; // TBD
        memset(Lat,0,sizeof(Lat));
        memset(Long,0,sizeof(Long));
        memset(Speed,0,sizeof(Speed));
        memset(Altitude,0,sizeof(Altitude));
        
        while(1)
        {
            
            HandleGPSINFData(pToken[in]);
            if(pToken[in] == 0x0D)break;
            in++; if(in >200)break;
            
        }
        if(GPSStatus == 'A')
        {
            // ESP_LOGW(TAG,"Date %s",GPSDate);
            ESP_LOGW(TAG,"Time now is %d-%d-20%d,%02d:%02d:%02d",GPSDay,GPSMonth,GPSYear,GPSHours,GPSMinutes,GPSSeconds);
        }
    }
    Speed[9] = '\0';

}

#else // SIM7070

/* Escalating GNSS recovery (2.3.33). Called from XCheckGPS once the modem has
   gone long enough without a fix that a wedged GNSS engine is the likely cause.
   Each stage is tried once per escalation cycle; the cycle restarts (back to
   stage 1) after the next successful fix. Deliberately does NOT esp_restart —
   a reboot drops the packet queue and track buffer, and the modem outlives it
   anyway. Time-to-first-fix after a cold start is ~30-60s, so the caller must
   allow that before escalating again. */
static void GNSSRecover(void)
{
    /* Re-entrancy guard: stage 3 calls InitGSM(), which itself calls XCheckGPS()
       for the reboot-ping timestamp. Without this, a stage-3 recovery that still
       finds no fix recurses into GNSSRecover from inside InitGSM and overflows
       the stack. Window is also restarted up-front (not on exit) so the nested
       poll sees a fresh window even if it somehow bypasses the guard. */
    static int gnss_recovering = 0;
    if (gnss_recovering) return;
    gnss_recovering = 1;
    gnss_last_action_us = esp_timer_get_time();

    gnss_recover_count++;
    switch (++gnss_recover_stage)
    {
        case 1: /* Power-cycle just the GNSS engine — cheapest, fixes a wedged session. */
            ESP_LOGW(TAG,"GNSS: no fix in window — power-cycling GNSS (attempt %lu)",
                     (unsigned long)gnss_recover_count);
            SendATCommand("AT+CGNSSPWR=0\r\n","OK","ERROR",10);
            osDelay(2000);
            SendATCommand("AT+CGNSSPWR=1\r\n","READY","ERROR",60);
            break;

        case 2: /* Ephemeris/almanac may be corrupt — force a cold start. */
            ESP_LOGW(TAG,"GNSS: still no fix — cold start");
            SendATCommand("AT+CGNSSPWR=1\r\n","READY","ERROR",60);
            SendATCommand("AT+CGPSCOLD\r\n","OK","ERROR",30);
            break;

        default: /* Whole modem is suspect — full reinit, then start the ladder over. */
            ESP_LOGW(TAG,"GNSS: still no fix — reinitialising modem");
            DisableGSM();
            InitGSM();
            gnss_recover_stage = 0;
            break;
    }
    /* Back off geometrically so a receiver that is genuinely dead (or a vehicle
       driven all day under heavy tree cover) is not power-cycled every 10
       minutes indefinitely. Reset to 1 on the next successful fix. */
    if (gnss_backoff_mult < GNSS_BACKOFF_MAX)
        gnss_backoff_mult *= 2;

    /* Restart the window again on exit: stage 3 can spend a minute inside
       InitGSM, and TTFF must be measured from when that finished, not from
       when recovery began. */
    gnss_last_action_us = esp_timer_get_time();
    gnss_recovering = 0;
}

void XCheckGPS(void)
{
    char *pToken;
    float tfSpeed=0;
    unsigned char at_rc;
    // char *pData; // TBD

    at_rc = SendATCommand("AT+CGPSINFO\r\n","OK","ERROR",3);
    osDelay(500);
    //VALTRACK-V4-VTS: AT+CGPSINFO
//+CGPSINFO: ,,,,,,,,
    // AT+CGPSINFO
    // sprintf(Buff2,"+CGPSINFO: 1528.20726,N,07501.25250,E,220225,080229.00,624.4,0.000,52.27");
// +CGPSINFO: 1528.20726,N,07501.25250,E,220225,080229.00,624.4,0.000,52.27

    Count=0;

    // if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CGPSINFO: ,,",13) != NULL)
    //     GPSStatus = 'V';
    // else
    //     GPSStatus = 'A';
    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"+CGPSINFO:",10);

    /* CGPSINFO carries no A/V field, so status is inferred. Inverted in 2.3.42:
       'V' is the default and a fix must be positively proven, rather than 'A'
       being assumed and demoted only by two known-bad strings. A timeout or a
       reply with no "+CGPSINFO:" line now reads as no-fix instead of as a valid
       one - see gnss_no_reply_count. */
    GPSStatus = 'V';
    if(pToken != NULL && at_rc != 3 &&
       MapForward(Buff2,BUFF2_SIZE,(char*)"+CGPSINFO: ,,",13) == NULL &&
       MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) == NULL)
        GPSStatus = 'A';

    /* Track how long the no-reply run has lasted (2.3.45), not just how many
       there have been. Field case 2026-08-04: the van sat on gpsnr=7938 and
       gpsage=34580 (9.6h with no fix) while gpsrec stayed 0 - the recovery
       ladder never fired once, because a no-reply does not set gnss_frozen and
       so never cleared the parked gate below. 2.3.42 made this failure visible
       but not self-healing; the receiver would have stayed dead until the next
       drive. Time-based rather than count-based so it does not depend on how
       often XCheckGPS happens to run, and it reuses GPS_FROZEN_SECONDS so both
       unambiguous-failure paths share one constant. */
    if(pToken == NULL || at_rc == 3)
    {
        gnss_no_reply_count++;
        if (gnss_no_reply_since_us == 0)
            gnss_no_reply_since_us = esp_timer_get_time();
    }
    else
    {
        gnss_no_reply_since_us = 0;   // modem is answering again
    }

    UpdateLocation(GPSStatus);

    /* Parse only when there is something to parse. Everything after this block
       runs unconditionally (2.3.42) - it used to be nested inside the
       pToken != NULL test, which is what let a missing reply bypass the
       frozen-clock check, the no-fix zeroing and GNSS recovery all at once. */
    if(pToken != NULL)
    {
        unsigned char in = 0;
        f=0;
        //pData = tBuff; // TBD
        memset(Lat,0,sizeof(Lat));
        memset(Long,0,sizeof(Long));
        memset(Speed,0,sizeof(Speed));
        memset(Altitude,0,sizeof(Altitude));
        
        sscanf(pToken,"+CGPSINFO: %f,%c,%f,%c,%[^,],%[^,],%f,%f,%f\r\n",&fLat,&NS,&fLong,&EW,GPSDate,GPSTime,&fAltitude,&tfSpeed,&fCourse);

        /* Date/time parsed here (moved ahead of the validity check in 2.3.40) so
           the frozen-clock test below can run before anything acts on the fix.
           On a no-fix response GPSDate/GPSTime are empty, sscanf converts
           nothing, and the block below zeroes these anyway. */
        sscanf( (void*)GPSDate, "%2d%2d%2d", (int*)&GPSDay,(int*)&GPSMonth,(int*)&GPSYear);
        sscanf( (void*)GPSTime, "%2d%2d%2d", (int*)&GPSHours,(int*)&GPSMinutes,(int*)&GPSSeconds);
    }
    {
        /* Frozen-clock check. The modem can keep answering with a well-formed
           sentence whose contents never change - a stale fix that looks
           completely valid. A working receiver stamps every poll with current
           UTC, so a GPS clock that has not moved for GPS_FROZEN_SECONDS means
           the fix is dead no matter how healthy the sentence looks. Demote it to
           'V' and let the existing no-fix path below do the rest: zero the
           position, surface gpsage, and let the watchdog power-cycle the
           receiver. Field case: the van's clock stopped at 2026-07-30 09:14:05
           UTC and the same fix was resent for 13h49m, straight through a drive,
           with no attribute anywhere showing a problem. */
        if (GPSStatus == 'A' && GPSYear >= 20) {
            long _ts = osmand_unix_ts(GPSYear, GPSMonth, GPSDay,
                                      GPSHours, GPSMinutes, GPSSeconds);
            int64_t _now = esp_timer_get_time();
            if (_ts != last_gps_ts) {
                last_gps_ts = _ts;
                last_gps_ts_change_us = _now;
                gnss_frozen = false;          // clock moving again
            } else if (last_gps_ts_change_us != 0 &&
                       _now - last_gps_ts_change_us >
                           (int64_t)GPS_FROZEN_SECONDS * 1000000LL) {
                GPSStatus = 'V';
                gnss_frozen = true;
            }
        }

        // while(1)
        // {
        if(GPSStatus!='A')
        {
            /* No fix — invalidate everything the ping path reads.
               Before 2.3.33 only the vestigial Lat/Long char arrays were cleared
               here, while fLat/fLong and the GPS clock kept their last values:
               sscanf against "+CGPSINFO: ,,,,,,,," converts no fields and leaves
               its targets untouched. LoadGPSTimeStamp() copies fLat/fLong with no
               status check, so XHTTP_Request saw a non-zero lat, classified it as
               a live fix, and shipped the stale coordinate with a frozen
               timestamp — a dead GNSS was indistinguishable from a parked
               vehicle. Field-confirmed 2026-07-27: unit -5783 resent the fix from
               2026-07-25 03:58:22 UTC for two days straight.
               Zeroed, the ping falls into the existing cached/cell path, which
               already omits the timestamp so Traccar uses receive time. */
            memset(Lat,0,sizeof(Lat));
            memset(Long,0,sizeof(Long));
            GPSDate[0] = '\0';   // extern char[] in main.c — no sizeof available
            GPSTime[0] = '\0';
            fLat = 0.0f; fLong = 0.0f; fSpeed = 0.0f;
            GPSDay = GPSMonth = GPSYear = 0;
            GPSHours = GPSMinutes = GPSSeconds = 0;

            /* Escalate GNSS recovery once the no-fix window elapses — but only
               while the vehicle is actually active (ignition on, or physical
               motion within the last 60s).

               A parked vehicle with no fix is usually parked somewhere with no
               sky view (carport, garage), not carrying a wedged receiver, and
               recovery cannot help: stage 3 power-cycles the whole modem, which
               costs battery and a fresh LTE registration for nothing. Field data
               from the van, night of 2026-07-27, drove this change: 19 recovery
               attempts over 3.3h — roughly 6 full modem reinits — every one of
               them with ignition=false and none of which could have worked.

               While parked the window is held open (anchored to now) rather than
               left to expire. That way the grace period starts fresh the moment
               the vehicle moves off, instead of firing a power-cycle instantly at
               ignition-on and restarting time-to-first-fix just as the receiver
               finally gets a clear view of the sky. */
            /* gnss_frozen bypasses the parked gate (2.3.40). "No fix" is
               ambiguous - a parked vehicle under a carport roof looks identical
               to a dead receiver, which is why escalation waits for activity.
               A clock that has stopped advancing is not ambiguous: the receiver
               is broken, and it will stay broken until something restarts it.
               Waiting for the next drive would mean the fault persists through
               it - exactly what happened on 2026-07-30, where the freeze began
               while parked and the whole morning commute went unrecorded.

               A sustained no-reply run is equally unambiguous and now bypasses
               it too (2.3.45). A modem that has not returned a parseable
               +CGPSINFO for GPS_FROZEN_SECONDS is not a vehicle under a carport
               - sky view has nothing to do with whether the AT command answers.
               Field case 2026-08-04: van on gpsnr=7938, gpsage=34580 (9.6h with
               no fix), gpsrec=0 - the ladder never fired once, because a
               no-reply leaves gnss_frozen false and the gate held. 2.3.42 made
               that failure visible; this makes it self-healing. */
            bool gnss_no_reply_dead =
                (gnss_no_reply_since_us != 0 &&
                 esp_timer_get_time() - gnss_no_reply_since_us >
                     (int64_t)GPS_FROZEN_SECONDS * 1000000LL);
            bool gnss_active = ign_on || (MotionTimer <= 60)
                               || gnss_frozen || gnss_no_reply_dead;
            int64_t window_s = (last_fix_us != 0)
                ? GNSS_RECOVER_AFTER_S : GNSS_COLDSTART_GRACE_S;
            window_s *= gnss_backoff_mult;

            if (gnss_last_action_us == 0 || !gnss_active)
                gnss_last_action_us = esp_timer_get_time();
            else if (esp_timer_get_time() - gnss_last_action_us >
                     window_s * 1000000LL)
                GNSSRecover();
        }
        // pData = tBuff;
        // HeaderReceived = 0;
        // CommaCount = 0;
        // f = 0;						
        // sscanf( (void*)Lat, "%f", &fLat);
        // sscanf( (void*)Long, "%f", &fLong);
        // sscanf( (void*)Altitude, "%f", &fAltitude);
        // sscanf( (void*)Speed, "%f", &tfSpeed);
        if(tfSpeed>1)
            fSpeed=tfSpeed*1.852;// nM to kmph 
        else
            fSpeed=0;
        // sscanf( (void*)Course, "%f", &fCourse);
        // (GPSDate/GPSTime are parsed further up since 2.3.40 - see the
        //  frozen-clock check.)

        if(NS == 'S')fLat *= -1;
        if(EW == 'W')fLong *= -1;
        fLat = GPRMC2Degrees(fLat);
        fLong = GPRMC2Degrees(fLong); 
        sprintf
        (
            (void*)Link,
            "http://maps.google.com/maps?z=18&q=%f,%f",
            fLat,fLong
        );
            
        if(GPSStatus == 'A')
        {
            pfLat = fLat;
            pfLong = fLong;
            /* Valid fix: reset the recovery ladder and restart the no-fix window
               so a later failure starts again at the cheapest stage, at the
               full cadence rather than a backed-off one. */
            last_fix_us = esp_timer_get_time();
            gnss_last_action_us = last_fix_us;
            gnss_recover_stage = 0;
            gnss_backoff_mult = 1;
        }
        if(GPSStatus != prevGPSStatus)
        {
            UpdateLocation(GPSStatus);
            prevGPSStatus = GPSStatus;
        }    
        //     HandleGPSINFData(pToken[in]);
        //     if(pToken[in] == 0x0D)break;
        //     in++; if(in >200)break;
            
        // }
        if(GPSStatus == 'A')
        {
            // ESP_LOGW(TAG,"Date %s",GPSDate);
            ESP_LOGW(TAG,"Time now is %d-%d-20%d,%02d:%02d:%02d",GPSDay,GPSMonth,GPSYear,GPSHours,GPSMinutes,GPSSeconds);
            ESP_LOGW(TAG,"Location is %f,%f,S=%f,A=%f,C=%f",fLat,fLong,fSpeed,fAltitude,fCourse);
        }
    }
    Speed[9] = '\0';

}
#endif // SIM7070

#ifdef SIM7600 //#ifdef SSL_BROKER
char XMQTT_Request(char *pFilename, unsigned char pingtype)
{
    //char eventnumber[5];
    //char *pResult;
    //unsigned char retries,i;
    // char *pToken;
    // char *pData;
    //char *pToken1;
    //unsigned char CheckSum,cs[2];
    pPacket = &CPacket;
    //retries = 0;
    TCPRetries = 0;
    // unsigned char encodedByte;
    // int X; 
    //unsigned short MQTTProtocolNameLength;
    unsigned short MQTTClientIDLength;
    //unsigned short MQTTUsernameLength;
    //unsigned short MQTTPasswordLength;
    // unsigned char k=0;
    #ifdef DEBUG_PRINT
        
        DebugPrint("Entered-TCP_request\r\n"); 
    #endif
    
/*
    #ifndef STANDALONE_DEMO
    CheckSum = GetCheckSum(pPacket->Bytes,34);
    cs[1]=GetAscii(CheckSum&0x0F);  
    cs[0]=GetAscii((CheckSum>>4) & 0x0F);  
    if((pPacket->GEvent.CheckSum[0] != cs[0]) || (pPacket->GEvent.CheckSum[1] != cs[1]))
    {
        retVal=0;
        return 0;
    }
#endif
    */
    
//   RESEND_TCP: // TBD
    WakeUp();
//    SOS = gpio_get_level(GPIO_SOS);
//    if(SOS == 0)
//    {
//        goto SUCCESS;
//    }
    ////Print4("Resending\r\n");
    ////IWDG_ReloadCounter();
    ResetBuffer();
    Print( "AAAAAAAAAAAAAT\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
        {       goto EXIT_MQTT; }
        Count++;
    }
   
    // RECONNECT: // TBD
////  Adding for A7670
//    sprintf((void*)str,"AT+CGDCONT=1,\"IP\",\"%s\"\r\n",Params.Fields.APNName);
//    SendATCommand(str,"OK","ERROR",5);
//    SendATCommand("AT+CGACT=1,1\r\n","OK","ERROR",10);
//    if(SendATCommand("AT+CGACT?\r\n","+CGACT: 1,1","ERROR",10) != 1)
//    {
//        goto exit;
//    }
////  End - Adding for A7670    
    #ifdef DEBUG_PRINT
        
        DebugPrint("CCHSTART OK-TCP_request"); 
    #endif
    // RESTART_MQTT: //TBD
    
  /////////////////////////////////////////////////////////////////////////
    MQTTClientIDLength = strlen((void*)IMEI);//strlen(Params.Fields.MQTTClientID);
    topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);

    
    if(SendATCommand("AT+CMQTTSTART\r\n","OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"MQTT START FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)str,"MQTT START OK");
    ///////////////////////////////////////////////////////////////
    if(Params.Fields.MQTTPort[0] == '8')
    {
        #ifdef CUSTOM_MQTT_CLIENT_ID
            sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\",1\r\n",Params.Fields.MQTTClientID);
        #else
            sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\",1\r\n",IMEI);
        #endif
        
    }
    else
    {
        #ifdef CUSTOM_MQTT_CLIENT_ID
            sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\"\r\n",Params.Fields.MQTTClientID);
        #else
            sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\"\r\n",IMEI);
        #endif
        
    }
    
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        sprintf((void*)str,"CLIEND ID FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)str,"CLIENT ID OK");

    ////////////////////////////////////////////////////////////////
    
// RECONNECT_MQTT:    // TBD 
    sprintf(
        (void*)cmdstr,
        "AT+CMQTTCONNECT=0,\"tcp://%s:%s\",60,1,\"%s\",\"%s\"\r\n",
        Params.Fields.MQTTHost,
        Params.Fields.MQTTPort,
        Params.Fields.MQTTUsername,
        Params.Fields.MQTTPassword
        );
    if(SendATCommand(cmdstr,"+CMQTTCONNECT: 0,0","ERROR",20) != 1)
    {
        sprintf((void*)str,"CONNECT FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)str,"CONNECT OK");
    
    
   
    #ifdef DEBUG_PRINT
        
        DebugPrint("ConnectPKT_OK-TCP_request\r\n"); 
    #endif
    /////////////////////////////////////////////////////////////////
    sprintf((void*)cmdstr,"AT+CMQTTTOPIC=0,%d\r\n",topiclength);
    if(SendATCommand(cmdstr,">","ERROR",10) != 1)
    {
        sprintf((void*)str,"TOPIC FAILED");
        goto EXIT_MQTT;
    }
    Print(Params.Fields.MQTTTopic); 
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
        {       goto EXIT_MQTT; }
        
    }
    sprintf((void*)str,"TOPIC OK");
    /////////////////////////////////////////////////////////////////
    while(1)
    {
        #ifdef DEBUG_PRINT
        
            DebugPrint("PUB while 1 Enter-TCP_request\r\n"); 
        #endif
        CheckBattery();
        
        if(CheckNetwork() == 1)
            UpdateNetwork(0);
        else
            UpdateNetwork(1);
        if( (MotionTimer > TIME_TO_SLEEP) && (IsQueueEmpty(RAMQueue)==0) )
        {
            retVal=0;
            return 0;
        }
//////////////////////////////////////////////////////////
        #ifdef EXT_ANT_ENABLED 
        XCheckGPS();
        
        #endif
        #ifdef NETWORK_LOCATION_ENABLED
            GetNetworkLocation();
        #endif
///////////////////////////////////////////////////////////    
        
        if(SystemState == State_ConnectedState) return 0; // Return and idle for proper configuration and prevent EEPROM access
        
        
     
       
        if(RTCTimeout>(Params.Fields.PingInterval+20))
         {
             
             InitRTCAlarm();
         }
        
       
        
        if(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS)
        {
            pPacket=&GPacket;
            #ifdef DEBUG_PRINT
        
                DebugPrint("PUB Got Packet-TCP_request\r\n"); 
            #endif    
            
 
            //if(tfLat == 0)tfLat = 8;
            memset(str,0,sizeof(str));
            //pPacket->GEvent.Speed[9] = '\0';
            //devid[8] = '\0';
            topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);

            
            
//            ChargeVoltage = (int)(ChargeVoltageF*1000);
            
//            datalength = sprintf((void*)str,

            ConvertToJSON(pPacket,&datalength,str);
            // datalength = sprintf(str,"x 1\ny 2\n");
            // printf("After convert\n");
            sprintf((void*)cmdstr,"AT+CMQTTPAYLOAD=0,%d\r\n",datalength);
            // printf("After cmdstr\n");
            if(SendATCommand(cmdstr,">","ERROR",10) != 1)
            {
                // Dont use str here
                goto EXIT_MQTT;
            }
            // printf("After sendat\n");
            Print(str); 
            LoopTimeout1 = 0;
            while(1)
            {
                if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                {
                    sprintf((void*)str,"PAYLOAD OK");
                    break;
                }
                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
                {   
                    sprintf((void*)str,"PAYLOAD ERROR");                    
                    goto EXIT_MQTT; 
                        
                }
                
            }
            // printf("After payload\n");
            if(SendATCommand("AT+CMQTTPUB=0,1,60\r\n","OK","ERROR",10) != 1)
            {
                sprintf((void*)str,"PUBLISH NOT OK");
                goto EXIT_MQTT;
            }
            LoopTimeout1 = 0;
            while(1)
            {
                if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CMQTTPUB: 0,0",14) != NULL)
                {
                    sprintf((void*)str,"PUBLISH SUCCESS");
                    TCPRetries=0;
                    ConnectivityTimer = 0;
                    break;
                }
                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
                {       
                    sprintf((void*)str,"PUBLISH FAILED");
                    goto EXIT_MQTT; 
                }
                
            } 
            osDelay(3000);
            #ifdef DEBUG_PRINT
        
                DebugPrint("PUB complete -TCP_request\r\n"); 
            #endif
            goto SUCCESS;
        }


        if(MapForward(Buff2,BUFF2_SIZE,(char*)"CLOSE",5) != NULL)
        {
            #ifdef DEBUG_PRINT
        
                DebugPrint("CLOSED-TCP_request\r\n"); 
            #endif
            goto EXIT_MQTT;//goto RECONNECT;
        }
        if(Params.Fields.WorkingMode[0] !='T')
            goto SUCCESS;
        
        SOS = gpio_get_level(GPIO_SOS);
        if(SOS == 0)
        {
            goto SUCCESS;
        }
        
        //CheckBattery();
     
        
    }
    
    
    goto SUCCESS;
    //free(string);
SUCCESS: 
    ClearEventCache();
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_SUCCESS -TCP_request\r\n"); 
    #endif
    //SendATCommand("AT+CMQTTREL=0\r\n","OK","ERROR",10);
    //osDelay(1000);
    SendATCommand("AT+CMQTTDISC=0,120\r\n","+CMQTTDISC:","ERROR",10);
    SendATCommand("AT+CMQTTREL=0\r\n","OK","ERROR",10);
//    #ifndef TIMER_ONLY_WAKEUP
//    goto RECONNECT_MQTT;
//    #endif
//    
    //osDelay(1000);
    SendATCommand("AT+CMQTTSTOP\r\n","+CMQTTSTOP:","ERROR",10);
    //osDelay(1000);
    
    
    //Print4("SUCCESS\r\n");
    retVal=0;
    return 0;
// exit:  // TBD
EXIT_MQTT:
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_EXIT -TCP_request\r\n"); 
    #endif
    //if(Params.Fields.MQTTPort[0] != '1')
    {
        
        //osDelay(1000);
        SendATCommand("AT+CMQTTDISC=0,120\r\n","+CMQTTDISC:","ERROR",10);
        SendATCommand("AT+CMQTTREL=0\r\n","OK","ERROR",10);
        //osDelay(1000);
        SendATCommand("AT+CMQTTSTOP\r\n","+CMQTTSTOP:","ERROR",10);
        //osDelay(1000);
        
    }
//    ResetBuffer();
//    Print("AT+NETCLOSE\r\n");
//    LoopTimeout1 = 0;
//    while(1)
//    {
//        if(MapForward(Buff2,BUFF2_SIZE,(char*)"NETCLOSE",8) != NULL)
//                break;
//        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>5))
//        {       break; }
//        Count++;
//    }
//    osDelay(1000);
    //Print4("FAILED\r\n");
    
  
//    if(++TCPRetries < 8) 
//        goto RESEND_TCP;
//    else
//    {        
//        #ifdef DEBUG_PRINT
//        
//            DebugPrint("TCP_Retry_exceeded -TCP_request\r\n"); 
//        #endif
//        retVal = 1;
////        HAL_UART_MspDeInit(&hlpuart1);
////        MX_LPUART1_UART_Init();
//        DisableGSM();
//        InitGSM();
//        TCPRetries = 0;
//    }
      
//    #ifdef EEPROM_FIFO
//    PostEEEvent(pPacket);
//    #endif
    
    RestoreEventCache();//PostEvent(pPacket);
    
    return 0;
}
#endif // SIM7600 XMQTT

#ifdef SIM7070 



char YMQTT_Request(char *pFilename, unsigned char pingtype)
{
    //char eventnumber[5];
    //char *pResult;
    //unsigned char retries,i;
//    char *pToken,*pData;
    //char *pToken1;
    //unsigned char CheckSum,cs[2];
    pPacket = &CPacket;
    //retries = 0;
    TCPRetries = 0;
//    unsigned char encodedByte;
//    int X; 
    //unsigned short MQTTProtocolNameLength;
    unsigned short MQTTClientIDLength;
    //unsigned short MQTTUsernameLength;
    //unsigned short MQTTPasswordLength;
//    unsigned char k=0;
    #ifdef DEBUG_PRINT
        
        DebugPrint("Entered-TCP_request\r\n"); 
    #endif
    
/*
    #ifndef STANDALONE_DEMO
    CheckSum = GetCheckSum(pPacket->Bytes,34);
    cs[1]=GetAscii(CheckSum&0x0F);  
    cs[0]=GetAscii((CheckSum>>4) & 0x0F);  
    if((pPacket->GEvent.CheckSum[0] != cs[0]) || (pPacket->GEvent.CheckSum[1] != cs[1]))
    {
        retVal=0;
        return 0;
    }
#endif
    */
    
//   RESEND_TCP:
    WakeUp();
    SOS = gpio_get_level(GPIO_SOS);
    if(SOS == 0)
    {
        goto SUCCESS;
    }
    ////Print4("Resending\r\n");
    ////IWDG_ReloadCounter();
    ResetBuffer();
    Print( "AAAAAAAAAAAAAT\r\n");    
    LoopTimeout1 = 0;
    while(1)
    {
        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                break;
        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>10))
        {       goto EXIT_MQTT; }
        Count++;
    }
   
    // RECONNECT:

    #ifdef DEBUG_PRINT
        
        DebugPrint("CCHSTART OK-TCP_request"); 
    #endif
    // RESTART_MQTT:
    
  /////////////////////////////////////////////////////////////////////////
    MQTTClientIDLength = strlen((void*)IMEI);//strlen(Params.Fields.MQTTClientID);
    topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);
    
    SendATCommand("AT+CNACT=0,1\r\n","OK","ERROR",10);
    
//RECHECK_IP:    
    if(SendATCommand("AT+CNACT?\r\n","+CNACT: 0,0,\"0.0.0.0\"","OK",10) == 1)
    {
        sprintf((void*)str,"NO IP ADDRESS");
        if(SendATCommand("AT+CFUN=0\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        sprintf(
            (void*)cmdstr,
            "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",
            Params.Fields.APNName
        );
        if(SendATCommand(cmdstr,"OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        //if(SendATCommand("AT+CFUN=1\r\n","+CPIN: READY","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CFUN=1\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        ////
        if(SendATCommand("AT+CGATT=1\r\n","OK","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CGATT?\r\n","+CGATT: 1","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CGNAPN\r\n","+CGNAPN: 1,","ERROR",10) != 1) goto EXIT_MQTT;
        sprintf(
            (void*)cmdstr,
            "AT+CNCFG=0,1,\"%s\",\"%s\",\"%s\"\r\n",
            Params.Fields.APNName,
            Params.Fields.APNUsername,
            Params.Fields.APNPassword
        );
        if(SendATCommand(cmdstr,"OK","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CNACT=0,1\r\n","+APP PDP: 0,ACTIVE","ERROR",10) != 1) goto EXIT_MQTT;
        if(SendATCommand("AT+CNACT?\r\n","+CNACT: 0,1,\"0.0.0.0\"","OK",10) == 1) goto EXIT_MQTT;
    }
    sprintf((void*)str,"IP ADDRESS OK");
    ///////////////////////////////////////////////////////////////
//    if(Params.Fields.MQTTPort[0] == '8')
//        sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\",1\r\n",IMEI);
//    else
//        sprintf((void*)cmdstr,"AT+CMQTTACCQ=0,\"%s\"\r\n",IMEI);
//    
//    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
//    {
//        sprintf((void*)str,"CLIEND ID FAILED");
//        goto EXIT_MQTT;
//    }
//    sprintf((void*)str,"CLIENT ID OK");

    ////////////////////////////////////////////////////////////////
    
// RECONNECT_MQTT:    
    
    sprintf((void*)cmdstr,"AT+SMCONF=\"URL\",%s,%s\r\n",Params.Fields.MQTTHost,Params.Fields.MQTTPort);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"URL FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SMCONF=\"KEEPTIME\",%d\r\n",Params.Fields.MQTTKeepAlive);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"KEEPTIME FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SMCONF=\"CLEANSS\",1\r\n");
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"CLEAN SESSSION FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SMCONF=\"USERNAME\",\"%s\"\r\n",Params.Fields.MQTTUsername);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"USERNAME FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SMCONF=\"PASSWORD\",\"%s\"\r\n",Params.Fields.MQTTPassword);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"PASSWORD FAILED");
        goto EXIT_MQTT;
    }
    sprintf((void*)cmdstr,"AT+SMCONF=\"CLIENTID\",\"%s\"\r\n",IMEI);
    if(SendATCommand(cmdstr,"OK","ERROR",10) != 1)
    {
        ESP_LOGI(TAG,"CLIENT ID FAILED");
        goto EXIT_MQTT;
    }
//    sprintf(
//        (void*)cmdstr,
//        "AT+CMQTTCONNECT=0,\"tcp://%s:%s\",60,1,\"%s\",\"%s\"\r\n",
//        Params.Fields.MQTTHost,
//        Params.Fields.MQTTPort,
//        Params.Fields.MQTTUsername,
//        Params.Fields.MQTTPassword
//        );
    if(SendATCommand("AT+SMCONN\r\n","OK","ERROR",20) != 1)
    {
        ESP_LOGI(TAG,"CONNECT FAILED");
        goto EXIT_MQTT;
    }
    ESP_LOGI(TAG,"CONNECT OK");
    
    
   
    #ifdef DEBUG_PRINT
        
        DebugPrint("ConnectPKT_OK-TCP_request\r\n"); 
    #endif
    /////////////////////////////////////////////////////////////////
//    sprintf((void*)cmdstr,"AT+CMQTTTOPIC=0,%d\r\n",topiclength);
//    if(SendATCommand(cmdstr,">","ERROR",10) != 1)
//    {
//        ESP_LOGI(TAG,"TOPIC FAILED");
//        goto EXIT_MQTT;
//    }
//    Print(Params.Fields.MQTTTopic); 
//    LoopTimeout1 = 0;
//    while(1)
//    {
//        if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
//                break;
//        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
//        {       goto EXIT_MQTT; }
//        
//    }
//    ESP_LOGI(TAG,"TOPIC OK");
    /////////////////////////////////////////////////////////////////
    while(1)
    {
        #ifdef DEBUG_PRINT
        
            DebugPrint("PUB while 1 Enter-TCP_request\r\n"); 
        #endif
        CheckBattery();
        
        if(CheckNetwork() == 1)
            UpdateNetwork(0);
        else
            UpdateNetwork(1);
        if( (MotionTimer > TIME_TO_SLEEP) && (IsQueueEmpty(RAMQueue)==0) )
        {
            retVal=0;
            return 0;
        }
//////////////////////////////////////////////////////////
    #ifdef EXT_ANT_ENABLED 
        XCheckGPS();

    #endif
    #ifdef NETWORK_LOCATION_ENABLED
        GetNetworkLocation();
    #endif
///////////////////////////////////////////////////////////      
        
        if(SystemState == State_ConnectedState) return 0; // Return and idle for proper configuration and prevent EEPROM access
        
        //ReadGPS();
     
     
          //uwADCxConvertedValue = HAL_ADC_GetValue(&hadc);
          //ADCvalue=(uwADCxConvertedValue*3.3)/4096;
          
        
            

            //if(tfLat == 0)tfLat = 8;
            memset(str,0,500);
            //pPacket->GEvent.Speed[9] = '\0';
            //devid[8] = '\0';
            topiclength = sprintf((void*)topic,(void*)Params.Fields.MQTTTopic);

            
            

            ConvertToJSON(pPacket,&datalength,str);
            sprintf((void*)cmdstr,"AT+SMPUB=\"%s\",%d,1,1\r\n",Params.Fields.MQTTTopic,datalength);
            if(SendATCommand(cmdstr,">","ERROR",10) != 1)
            {
                // Dont use str here
                goto EXIT_MQTT;
            }
            Print(str); 
            LoopTimeout1 = 0;
            while(1)
            {
                if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                {
                    ESP_LOGI(TAG,"PUBLISH SUCCESS");
                    TCPRetries=0;
                    ConnectivityTimer = 0;
                    break;
                }
                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
                {   
                    ESP_LOGI(TAG,"PUBLISH ERROR");                    
                    goto EXIT_MQTT; 
                        
                }
                
            }
//            if(SendATCommand("AT+CMQTTPUB=0,1,60\r\n","OK","ERROR",10) != 1)
//            {
//                ESP_LOGI(TAG,"PUBLISH NOT OK");
//                goto EXIT_MQTT;
//            }
//            LoopTimeout1 = 0;
//            while(1)
//            {
//                if(MapForward(Buff2,BUFF2_SIZE,(char*)"+CMQTTPUB: 0,0",14) != NULL)
//                {
//                    ESP_LOGI(TAG,"PUBLISH SUCCESS");
//                    TCPRetries=0;
//                    ConnectivityTimer = 0;
//                    break;
//                }
//                if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>20))
//                {       
//                    ESP_LOGI(TAG,"PUBLISH FAILED");
//                    goto EXIT_MQTT; 
//                }
//                
//            } 
            osDelay(3000);
            #ifdef DEBUG_PRINT
        
                DebugPrint("PUB complete -TCP_request\r\n"); 
            #endif
            goto SUCCESS;
        


        if(MapForward(Buff2,BUFF2_SIZE,(char*)"CLOSE",5) != NULL)
        {
            #ifdef DEBUG_PRINT
        
                DebugPrint("CLOSED-TCP_request\r\n"); 
            #endif
            goto EXIT_MQTT;//goto RECONNECT;
        }
        if(Params.Fields.WorkingMode[0] !='T')
            goto SUCCESS;
        
        SOS = gpio_get_level(GPIO_SOS);
        if(SOS == 0)
        {
            goto SUCCESS;
        }
        
        //CheckBattery();
     
        
    }
    
    
//    goto SUCCESS;
    //free(string);
SUCCESS: 
    ClearEventCache();
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_SUCCESS -TCP_request\r\n"); 
    #endif
    //SendATCommand("AT+CMQTTREL=0\r\n","OK","ERROR",10);
    //osDelay(1000);
    SendATCommand("AT+SMDISC\r\n","OK","ERROR",10);
    // SendATCommand("AT+CNACT=0,0\r\n","DEACTIVE","ERROR",10);
//    #ifndef TIMER_ONLY_WAKEUP
//    goto RECONNECT_MQTT;
//    #endif
//    
    //osDelay(1000);
    //SendATCommand("AT+CMQTTSTOP\r\n","+CMQTTSTOP:","ERROR",10);
    //osDelay(1000);
    
    
    //Print4("SUCCESS\r\n");
    retVal=0;
    return 0;
//exit: 
EXIT_MQTT:
    #ifdef DEBUG_PRINT
        
        DebugPrint("TCP_EXIT -TCP_request\r\n"); 
    #endif
    //if(Params.Fields.MQTTPort[0] != '1')
    {
        
        SendATCommand("AT+SMDISC\r\n","OK","ERROR",10);
        // SendATCommand("AT+CNACT=0,0\r\n","DEACTIVE","ERROR",10);
        //osDelay(1000);
        
    }
//    ResetBuffer();
//    Print("AT+NETCLOSE\r\n");
//    LoopTimeout1 = 0;
//    while(1)
//    {
//        if(MapForward(Buff2,BUFF2_SIZE,(char*)"NETCLOSE",8) != NULL)
//                break;
//        if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (LoopTimeout1>5))
//        {       break; }
//        Count++;
//    }
//    osDelay(1000);
    //Print4("FAILED\r\n");
    
  
//    if(++TCPRetries < 8) 
//        goto RESEND_TCP;
//    else
//    {        
//        #ifdef DEBUG_PRINT
//        
//            DebugPrint("TCP_Retry_exceeded -TCP_request\r\n"); 
//        #endif
//        retVal = 1;
////        HAL_UART_MspDeInit(&hlpuart1);
////        MX_LPUART1_UART_Init();
//        DisableGSM();
//        InitGSM();
//        TCPRetries = 0;
//    }
      
//    #ifdef EEPROM_FIFO
//    PostEEEvent(pPacket);
//    #endif
    
    //PostEvent(pPacket);
    RestoreEventCache();
    
    return 0;
}
#endif // SIM7070



#ifdef SIM800 
char TCP_request(char *pFilename, unsigned char pingtype)
{}


#endif


void SyncRTC (void)
{
    //TBD
//     int cYear,cMonth,cDate,cHour,cMinute,cSecond;
//     char *pToken;
//     unsigned char DecrementCount;
//     ResetBuffer();
// //    Print("AT+CCLK?\r\n");
// //    osDelay(1000);
// //    osDelay(2000);
//     SendATCommand("AT+CCLK?\r\n","OK","ERROR",3);
//     pToken = MapForward(Buff2,BUFF2_SIZE,(unsigned char*)",",1);
//     if(pToken != NULL)
//     {
// 					//		sscanf(Buff2,"20%02d-%02d-%02d  %02d:%02d:%02d",
// 					//																	&Year,&Month,&Date,&Hour,&Minute,&Sec);
// 		DecrementCount = 0;	
//         while(*pToken!='/')
//         {
//             pToken--;
//             if(++DecrementCount>100)return;
//         }
//         pToken-=5;
//         sscanf((void*)pToken,"%02d/%02d/%2d,%02d:%02d:%02d:",&cYear,&cMonth,&cDate,&cHour,&cMinute,&cSecond);
//         sDate.Year=cYear;
//         sDate.Month=cMonth;
//         sDate.Date=cDate;
//         R.Hours=cHour;
//         R.Minutes=cMinute;
//         R.Seconds=cSecond;

//         __HAL_RCC_RTC_ENABLE();
//         /**Initialize RTC Only */
//         hrtc.Instance = RTC;
//         hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
//         hrtc.Init.AsynchPrediv = 127;
//         hrtc.Init.SynchPrediv = 255;
//         hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
//         hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
//         hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
//         if (HAL_RTC_Init(&hrtc) != HAL_OK)
//         {
//             Error_Handler();
//         }

            

//         /**Initialize RTC and set the Time and Date 
//         */ 
//         //if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
//         {
//             if (HAL_RTC_SetTime(&hrtc, &R, RTC_FORMAT_BIN) != HAL_OK)
//             {
//                 Error_Handler();
//             }
           
//             if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
//             {
//                 Error_Handler();
//             }

//             HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);
//         }


//         HAL_RTCEx_EnableBypassShadow(&hrtc);
                    
//         }

}

void DeepSleep (void)
{
    
    if((Params.Fields.WorkingMode[0]=='T'  || Params.Fields.WorkingMode[0]=='H') || PowerButtonSleep == 1)
    {
      if((ParkLongTimer >= PARK_LONG_SECONDS) || PowerButtonSleep == 1)
        {
//            if(GSMEnabled == 1)
//            {    
//              //  HandleChargingState(); 
//            }
            //Print1("Tada\r\n");
            DisableGSM();
            
            FrontPanelTimer = 500; // Turn off LED and wait
            // osDelay(3000);
  

//        UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);

        //Print1("Hello\r\n");
        //if(SleepModeEnabled == 1) 
        {
            MakeAllLED(TURN_OFF); // Because Disable GSM turns off power to LED

            DisableGSM();           
            
            ESP_LOGI(TAG,"Init Accelerometer : Before Entering Sleep");
            InitAccelerometer();


            RTCSleepModeEnabled = 1;
            
            
            //InitRTCAlarm();
            SleepModeEnabled = 1;
            DisableMainPower();

            esp_wifi_stop();
            esp_wifi_deinit();

            esp_bt_controller_disable();
            esp_bt_controller_deinit();

            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
            nimble_port_freertos_deinit();  
            nvs_flash_deinit();
            //nimble_port_stop(); // BLE Causes crash
            
            /////////////////////////////////////
            /* Initialize selected GPIO as RTC IO, enable output, disable pullup and pulldown, enable hold*/
            gpio_hold_en(GPIO_TPS_ENABLE);
            gpio_hold_en(GPIO_GSM_ENABLE);
            gpio_deep_sleep_hold_en();
            /////////////////////////////////
           
            EnterDeepSleep();

            
            // osThreadFlagsWait( 1, osFlagsWaitAny, osWaitForever); // TBD
        }

        }
        else
        {
              if(SleepModeEnabled == 1)
              {
                //   BackupPackets(); // TBD
                //   WriteSRAM(DEEP_SLEEP_RESET); // TBD
                ESP_LOGW(TAG,"Rebooting DeepSleep");
                esp_restart();
              }
                  //            EnableGPS();
//           // EnableCharger();
//            if(GSMEnabled == 0)
//            {
//                while(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS);
//                
//               // while(GetEEEvent(&GPacket) == GET_SUCCESS);
//                
//                InitGSM();
//                MotionTimer = 0;
//                PostMotionEvent();
//            }
//            EnableGSM();
            
        }
    
    }
}
unsigned char AlarmString[100];
void InitRTCAlarm(void)
{
    // TBD
//     //unsigned char RTCStr[100];
//     //#ifdef TIMER_ONLY_WAKEUP
//     unsigned short hour=0,minute=0,second=0;
//     //#endif
//     #ifdef DEBUG_PRINT
        
//         DebugPrint("Entered-InitRTCAlarm\r\n"); 
//     #endif
    
//     __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);

//     HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN);
//     HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
//     //##-3- Configure the RTC Alarm peripheral #################################
//     // Set Alarm to 02:20:30 
//     //RTC Alarm Generation: Alarm on Hours, Minutes and Seconds 

//     salarmstructure.Alarm = RTC_ALARM_A;
//     salarmstructure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
//     salarmstructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
//     salarmstructure.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY ;//| RTC_ALARMMASK_HOURS;
//     //salarmstructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
//     salarmstructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_PM;

// #ifndef TIMER_ONLY_WAKEUP
//     salarmstructure.AlarmTime.Hours = R.Hours;
//     salarmstructure.AlarmTime.Minutes =  R.Minutes+1;
//     if(salarmstructure.AlarmTime.Minutes >= 60)
//         salarmstructure.AlarmTime.Minutes = 0;
    
//     salarmstructure.AlarmTime.Seconds = 0;
//     //salarmstructure.AlarmTime.SubSeconds = 0x56;
//     RTCTimeout=0;
//     #ifdef DEBUG_PRINT
//         sprintf((void*)RTCStr,"\r\nCTime-%0.2d:%0.2d:%0.2d",R.Hours,R.Minutes,R.Seconds);
//         DebugPrint(RTCStr);
//         sprintf((void*)RTCStr,"\r\nATime%0.2d:%0.2d:%0.2d",salarmstructure.AlarmTime.Hours,salarmstructure.AlarmTime.Minutes,salarmstructure.AlarmTime.Seconds);
//         DebugPrint(RTCStr);
//         DebugPrint("-InitRTCAlarm\r\n"); 
//     #endif
    
    
// #else
//         //Params.Fields.PingInterval = 3;
    
//     hour=R.Hours;
//     minute=R.Minutes;
//     second=R.Seconds;
    
//     hour+=(Params.Fields.PingInterval/60);
//     minute+=(Params.Fields.PingInterval%60);
    
    
//     if(second>=60)
//     {
//         second-=60;
//         minute+=1;
        
        
//     }
    
//     if(minute>=60)
//     {
//         minute-=60;
//         hour+=1;
        
        
//     }
//     if(hour>=24)
//     {
//             hour-=24;
//     }
    
//     salarmstructure.AlarmTime.Hours = hour;//R.Hours;
//     salarmstructure.AlarmTime.Minutes = minute;
//     salarmstructure.AlarmTime.Seconds = second;
    
//     sprintf((void*)AlarmString,"ST: %0.2d:%0.2d:%0.2d|AT: %0.2d:%0.2d:%0.2d",
//                     R.Hours,R.Minutes,R.Seconds,
//                     salarmstructure.AlarmTime.Hours,
//                     salarmstructure.AlarmTime.Minutes,
//                     salarmstructure.AlarmTime.Seconds);
    
//     #ifdef DEBUG_PRINT
//         sprintf((void*)RTCStr,"\r\nCTime-%0.2d:%0.2d:%0.2d",R.Hours,R.Minutes,R.Seconds);
//         DebugPrint(RTCStr);
//         sprintf((void*)RTCStr,"\r\nATime%0.2d:%0.2d:%0.2d",salarmstructure.AlarmTime.Hours,salarmstructure.AlarmTime.Minutes,salarmstructure.AlarmTime.Seconds);
//         DebugPrint(RTCStr);
//         DebugPrint("-InitRTC_IRQHandler\r\n"); 
//     #endif
    
// #endif
//     if(RTCSleepModeEnabled == 1)
//     {      
//         hour=R.Hours;
//         minute=R.Minutes;
//         second=R.Seconds;
        
//         hour+=(1440/60); // 24 hours interval
//         minute+=(1440%60); // 24 hours interval
//         minute-=1;
        
//         if(second>=60)
//         {
//             second-=60;
//             minute+=1;
            
            
//         }
        
//         if(minute>=60)
//         {
//             minute-=60;
//             hour+=1;
            
            
//         }
//         if(hour>=24)
//         {
//                 hour-=24;
//         }
        
//         salarmstructure.AlarmTime.Hours = hour;//R.Hours;
//         salarmstructure.AlarmTime.Minutes = minute;
//         salarmstructure.AlarmTime.Seconds = second;
        
//         sprintf((void*)AlarmString,"ST: %0.2d:%0.2d:%0.2d|AT: %0.2d:%0.2d:%0.2d",
//                         R.Hours,R.Minutes,R.Seconds,
//                         salarmstructure.AlarmTime.Hours,
//                         salarmstructure.AlarmTime.Minutes,
//                         salarmstructure.AlarmTime.Seconds);
//         Print1("HelloRTC\r\n");
//         Print1(AlarmString);
        
//     }
//     if(HAL_RTC_SetAlarm_IT(&hrtc,&salarmstructure,RTC_FORMAT_BIN) != HAL_OK)
//     {
//         /* Initialization Error */
//         Error_Handler(); 
//     }

//     //HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 22, 0);
//     HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 3, 0);
//     HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

unsigned char FirstRun = 1;
void RTC_Alarm_IRQHandler(void)
{
    // TBD

    
    
    
}
unsigned char VALTRACK_BLE_Status = 0,Prev_VALTRACK_BLE_Status = 0;
// void VALTRACK_BLE_Advertise(unsigned char Status);
unsigned char ForceEraseEEPROM  = 0;

unsigned char ServerRetries = 0;
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartMainTask(void *argument)
{
   
   
    char*pToken;
    char*pFilename;   
    unsigned char i,EEPROMReadCount;
    
  /* USER CODE BEGIN 1 */
    
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
ESP_LOGI(TAG,"Entered main task");
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
//  HAL_Init();

//  /* USER CODE BEGIN Init */
//  Reset_Device();
//  Config_HSE();
//  /* USER CODE END Init */

//  /* Configure the system clock */
//  SystemClock_Config();
    // if(BootReason <= 8)
    // {
    //     // Print1("\r\n\r\n");
    //     ESP_LOGI(TAG,"Boot Reason = %s",(char*)(BootReasons[BootReason].Bytes)); // TBD
    //     // Print1("\r\n\r\n");
    // }
//  /* USER CODE BEGIN SysInit */
//  PeriphClock_Config();
//  Init_Exti(); /**< Configure the system Power Mode */
  /* USER CODE END SysInit */

    #ifdef DEBUG_PRINT
        DebugPrint("Entered -StartMainTask\r\n"); 
    #endif
  /* USER CODE BEGIN 2 */
//HAL_ADC_Start_DMA(&hadc, (void*)&BatteryADCCount , 1);
//    InitAccelerometer();
//    MotionTimer=800;
//    DeepSleep();
    #ifndef WB_PIN_CONTROLLED_LED
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET); // LED Controller Enable
    #endif
    //DisableGSM();
    EnableGSM(); // For LED
    // UpdateLED1(RED_COLOR);
    // UpdateLED3(RED_COLOR);
    
    // UpdateNetwork(0);
    
    // TBD
    // ADCBatteryVoltage = (((float)BatteryADCCount*ADC_REFERENCE*(float)DIVIDER_FACTOR)/4096);
    // ADCBatteryVoltage+=ADC_OFFSET;//ADCBatteryVoltage -=0.4;
    //ADCBatteryVoltage = (((float)BatteryADCCount*2*3)/4096);

   
    
    UpdateBattery(ADCBatteryVoltage);
    
    


    //GSM_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
    // ESP_LOGI(TAG,"Reached Enable GSM");
    #ifndef VALTRACK_V4_VTS
    ChargingStatus = (ChargingStatusType)gpio_get_level(GPIO_CHARGER_PIN);
    
    if( (ChargingStatus == CONNECTED) || (ADCBatteryVoltage < 3.0))
    {
        goto CHECK_CHARGER_STATE;
    }
    #endif

    EnableGSM();
    osDelay(1000);// For SIM7672
    gpio_set_level(GPIO_PWRKEY,1);
    osDelay(1000);
    gpio_set_level(GPIO_PWRKEY,0);
    //osDelay(5000);
    //while(1){SendATCommand("AT\r\n","OK","ERROR",5);osDelay(1000);};
    // while(1)
    // {
    //     osDelay(1);
    // }
    // ESP_LOGI(TAG,"After Enable GSM");
    //while(1){LPUARTTimer=0;}
    // STANDBY DELAY
    //osDelay(5000);
    
    // while(1)
    // {
    //     Print("TestText\r\n");
    //     osDelay(500);

    // }
    // TBD
    // ADCBatteryVoltage = (((float)BatteryADCCount*(float)ADC_REFERENCE*(float)DIVIDER_FACTOR)/4096);//ADCBatteryVoltage = (((float)BatteryADCCount*2*3)/4096);
    // ADCBatteryVoltage -=0.4;
    UpdateBattery(ADCBatteryVoltage);

    //if(ADCBatteryVoltage < 3.65)
    //    DeviceStatus = 0;
 
    FrontPanelTimer = 0;
    // Clear reset flags in any cases 
    //__HAL_RCC_CLEAR_RESET_FLAGS();
 
    osDelay(300);
    

    ReadEEIndexes();
    GetEEParams();
    // Init Default values
    //if( (Params.Fields.Band[0] == 0xFF) || (LoadDefaultParams == 1) )
    //LoadDefaultParams = 1;
    printf("%d",LoadDefaultParams);
    if( (GetParamString("APNName",cmdstr) != ESP_OK) || (LoadDefaultParams == 1) )
    {
        //memcpy(Params.Bytes,DefaultParams.Bytes,sizeof(Params));
        //StoreEEParams();
        StoreParams((void*)&DefaultParams);
        LoadDefaultParams = 0;
    }
    if(ForceEraseEEPROM == 1)
    {
        EraseEEPROMPackets();
        ForceEraseEEPROM = 0;
    }
    #ifdef DEBUG_PRINT
        DebugPrint("ParamsRead -StartMainTask\r\n"); 
    #endif  
    
//    PostReboot();

    //EnableGSM();
    
//    while(1)
//    {
//        if(VALTRACK_BLE_Status != Prev_VALTRACK_BLE_Status)
//        {
//            VALTRACK_BLE_Advertise(VALTRACK_BLE_Status);
//            Prev_VALTRACK_BLE_Status = VALTRACK_BLE_Status;
//        }
//    }
    InitAccelerometer();

    /* Mark this partition valid as soon as basic init completes.
       A transient modem failure must not trigger a rollback — that
       would cause a 2.3.N → 2.3.N-1 loop every time InitGSM() is slow. */
    esp_ota_mark_app_valid_cancel_rollback();

//goto DIE;
    osDelay(5000);

    if(InitGSM() == 3)
    {
        MotionTimer = TIME_TO_SLEEP+1; //  Make sure no events are in queue to enter sleep
        ClearPackets();
        printf("entering stop due to init gsm\n");
        goto ENTER_STOP_MODE;
    }
    // printf("entering light sleep after init gsm\n");
    // esp_sleep_enable_timer_wakeup(1000000);
    // esp_light_sleep_start();

    // MakeAllLED(PURPLE);
    // osDelay(10000);

    //InitGPS();
    // CheckSignalStrength();
    if(CheckNetwork() == 1)
    {
        ESP_LOGI(TAG,"Forcing Sleep after Init GSM due to no network\n");
        ForceToSleep();
    }


    //
    //GetNetworkData();
    //
    SyncRTC();
    osDelay(10000); // Let LTE data routing stabilize before attempting OTA HTTP
    /* Must precede CheckAndApplyOTA - it decides which server that reads.
       A staged unit that loaded production here would revert on every boot. */
    ota_channel_load();
    CheckAndApplyOTA();
    /* Restore last known position from NVS so we can ping Traccar immediately
       after reboot without waiting for a GPS fix. */
    nvs_load_position();
    InitRTCAlarm();
    ADCRunning = 0;
    Count=0;
    SMSNumber = 0;
    //minVal = 0; maxVal = 600;
    rxNumber[10] = '\0';
    AlertTimer = 60000;
    SOSTimer = 0;
    
    //EnterStandyMode();
    LEDInhibit = 1;
    /*for(i=0;i<5;i++)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_SET); //LED
        osDelay(250);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_RESET); //LED
        osDelay(250);
    }  */  
    LEDInhibit = 0;
    
    MotionTimer = 0;
	
//		i=0;
//		while(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS)
//		{
//			i++;
//			if(i>200)
//				break;
//		}
     
    
    PostReboot();
    
		//
        
    //RestorePackets();

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    ChargingStatus = DISCONNECTED;
        // DIE: // TBD
        //MotionTimer=500;
//        SleepModeEnabled = 1;
	while(1)
	{
//        if(SleepModeEnabled == 1) 
//        {
//            DisableGSM();
//            
//            MakeAllLED(TURN_OFF);
//            //VALTRACK_BLE_Advertise(0);
//            
//            __HAL_RCC_GPIOC_CLK_DISABLE();
//            __HAL_RCC_GPIOH_CLK_DISABLE();
//            __HAL_RCC_GPIOB_CLK_DISABLE();
//            __HAL_RCC_GPIOA_CLK_DISABLE();
//            __HAL_RCC_GPIOE_CLK_DISABLE();
//            __HAL_RCC_ADC_CLK_DISABLE();
//            __HAL_RCC_I2C1_CLK_DISABLE();
//            __HAL_RCC_LPUART1_CLK_DISABLE();
//            __HAL_RCC_USART1_CLK_DISABLE();
//            __HAL_RCC_DMAMUX1_CLK_DISABLE();
//            __HAL_RCC_DMA1_CLK_DISABLE();
//            __HAL_RCC_DMA2_CLK_DISABLE();
//            HAL_UART_DeInit(&hlpuart1);
//            HAL_UART_DeInit(&huart1);
//            HAL_DMA_DeInit(&hdma_adc1);
//            HAL_ADC_DeInit(&hadc1);
//            HAL_I2C_DeInit(&hi2c1);
//            
//            
//            osThreadFlagsWait( 1, osFlagsWaitAny, osWaitForever);;
//        }
            #ifdef DEBUG_PRINT
            DebugPrint("While Loop -StartMainTask\r\n"); 
        #endif
        // HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN); //TBD
        // HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); //TBD
        
        
        osDelay(1000);
        if(SystemState == State_ConnectedState)
        {
            // VALTRACK_BLE_Advertise(1); // TBD
            #ifndef WB_PIN_CONTROLLED_LED
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET);
            #endif
            MakeAllLED(BLUE);
            DisableGSM();
            
            while(SystemState == State_ConnectedState)
            {
                FrontPanelTimer=2; // To prevent 1, it will write advertisement repeatedly if it increments to 1. 
                LPUARTTimer=0;
                //Make LED blue here 
                // VTS uses IO so writing here, change to I2C for V4-MF
                // TO BE IMPLEMENTED FOR V4MF
                // TO BE IMPLEMENTED FOR V4MF
                // TO BE IMPLEMENTED FOR V4MF
                // TO BE IMPLEMENTED FOR V4MF
                // TO BE IMPLEMENTED FOR V4MF
                #ifdef VALTRACK_V4_VTS
                    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_SET); // TBD
                    // osDelay(500);
                    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_RESET);
                    // osDelay(500);
                #else
                      //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET); //TBD
//                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_SET);
//                    osDelay(500);
//                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5,GPIO_PIN_RESET);
//                    osDelay(500);
//                    
                #endif
                // TO BE IMPLEMENTED FOR V4MF
                // TO BE IMPLEMENTED FOR V4MF
                
            }
        }
     
#ifndef VALTRACK_V4_VTS
            CHECK_CHARGER_STATE:
            if( (ChargingStatus == CONNECTED) || (ADCBatteryVoltage < 3.0))
            {
                //DeviceStatus = 0;
                
                ESP_LOGI(TAG,"Entered Charging Loop");
                DisableGSM();
                osDelay(1000);
                EnableGSM(); // For LED
                UpdateNetwork(3);
                UpdateLocation(0);

                //DisableGSM();
                // printf("before delay\n");
                // osDelay(20000);
                // printf("after delay\n");
                
                // wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
                // esp_wifi_init(&wifi_config);
                // esp_wifi_start();
                
                
                //  esp_wifi_stop();
                //  esp_wifi_deinit();


                
                // esp_bt_controller_disable();
                // esp_bt_controller_deinit();
                // // #define GPIO_PWRKEY    7
                // // #define GPIO_GSM_ENABLE    10
                // // #define GPIO_TPS_ENABLE    4
                // // #define GPIO_INT1     3
                // // #define GPIO_SOS      9
                // // #define GPIO_CHG_IN   4
                // gpio_set_level(GPIO_PWRKEY, 0);   
                // gpio_set_level(GPIO_GSM_ENABLE, 0);   
                // gpio_set_level(GPIO_TPS_ENABLE, 0); 

                // //gpio_wakeup_disable(2);

                // led_deinit();
                //gpio_set_level(GPIO_INT1, 0);   
                //gpio_set_level(GPIO_SOS, 0);   
                //gpio_set_level(GPIO_CHG_IN, 0);   

                //  for(int i = 0;i<=21;i++)
                // {
                //     gpio_set_direction(i, GPIO_MODE_INPUT);
                // }


                


        //         const esp_pm_config_esp32_t config = {
        //     .max_freq_mhz = 40,
        //     .min_freq_mhz = 8,
        //     .light_sleep_enable = 1,
        // };
        //  ESP_ERROR_CHECK(esp_pm_configure(&config));
    //              esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
    //   esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);
    //   //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);
    //  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
    // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);


                // nimble_port_freertos_deinit();  
                //  nvs_flash_deinit();



                // esp_sleep_config_gpio_isolate();
                //rtc_sleep_enable_ultra_low(true);
                // printf("1");
                // esp_sleep_enable_timer_wakeup(10000000);
                // printf("2");
                // esp_light_sleep_start();




                // printf("3");
                //esp_sleep_enable_timer_wakeup(10000000);
                //esp_deep_sleep_start();
                //esp_deep_sleep(10000000);
                // printf("4");
                // esp_err_t err= esp_deep_sleep_try(10000000);
                // printf("Failed sleep\n");
                // //led_deinit();

                // phy_bbpll_en_usb(true);

                // MotionTimer=TIME_TO_SLEEP+10;
                // DeepSleep();
                // // phy_bbpll_en_usb(true);
                //  SleepWakeupReason();

                // // printf("returned\n");
                // esp_restart();
                // while(1);
                // EnterDeepSleep();
                /*
                __disable_irq();
                LPM_EnterStopMode();
                //LPM_ExitStopMode();
                //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
                
                __enable_irq();
                // Configures system clock after wake-up from STOP: enable HSE, PLL and select
                //PLL as system clock source (HSE and PLL are disabled in STOP mode) 
                SystemClockConfig_STOP();*/
                while(1)
                {
                    if(ChargingStatus == CONNECTED)
                    {
                        LPUARTTimer = 0;// Feed UART Timer
                        
                        osDelay(5000);
                        // Was disconnected if was here
                        
                    }
                    if(ChargingStatus == DISCONNECTED && ADCBatteryVoltage > 3.5)
                    {
                        // WriteSRAM(CHARGER_RESET); // TBD
                        ESP_LOGI(TAG,"------Rebooting from StartMainTask ChargingStatus=1");
                        MakeAllLED(PURPLE);
                        osDelay(1000);
                        esp_restart();// NVIC_SystemReset();//break;// TBD
                        //osDelay(5000);
                    }
                    //HAL_ADC_Start_DMA(&hadc1, (void*)&BatteryADCCount , 1); //TBD
                    osDelay(500);
                    
                    // TBD
                    // ADCBatteryVoltage = (((float)BatteryADCCount*ADC_REFERENCE*(float)DIVIDER_FACTOR)/4096);
                    // ADCBatteryVoltage+=ADC_OFFSET;//ADCBatteryVoltage -=0.4;    //ADCBatteryVoltage = (((float)BatteryADCCount*2*3)/4096);
                    UpdateBattery(ADCBatteryVoltage);  
                }
            }
        #endif               
      // CheckBLE();
		#ifdef SOSALERT
            SOS = gpio_get_level(GPIO_SOS);
            if(SOS == 0)
            {
                LEDInhibit=1;
                osDelay(3000);
                SOS = gpio_get_level(GPIO_SOS);
                if(SOS == 0)
                {
                    for(i=0;i<5;i++)
                    {
                        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_RESET); //LED
                        osDelay(200);
                        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_SET); //LED
                        osDelay(200);
                        
                    }
                    LEDInhibit=0;
                 // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,GPIO_PIN_RESET); //LED
                    SosAlert=1;
                    
                    osDelay(1000);
                    ResetBuffer();
                    Print("AT+CSCLK=0\r\n");
                    
                    osDelay(2000);//DDelay();
        //                DDelay();
        //                DDelay();
                    ResetBuffer1();
                    Print1("$PMTK161,1*29\r\n");
                    osDelay(1000);
                    SendLastLocation();
                    SosAlert=0;
                    osDelay(3000);
                    ResetBuffer();
                    DDelay();
                }
            }

        #endif
        
      
        
        // HAL_RTC_GetTime(&hrtc, &R, RTC_FORMAT_BIN); //TBD
        // HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); //TBD
        osDelay(1000);
        //GSM_STATUS = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12);//GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0);
        INT1 = (MotionStatusType)gpio_get_level(GPIO_INT1);
        
        
        /////////////////////////////////
        
        /////////////////////////////////
        if(INT1 == 0)
        {
            #ifdef DEBUG_PRINT
                DebugPrint("INT1=0 -StartMainTask\r\n"); 
            #endif
            //for(i=0;i<=0x31;i++)
            //{
            //    VALREAD = I2C_RdReg(i);
            //    INT1 = (MotionStatusType)gpio_get_level(GPIO_INT1);
            //}

#ifdef ENABLE_HARSH_DRIVING
            /* Both LIS3DH interrupt generators share this pin, so work out which
               one raised it. Generator 2 = harsh driving. Read this first: the
               throttled re-init below can also clear the latch.
               Bit 6 (0x40) is IA - "one or more interrupts generated". */
            int1_poll_count++;                     // ipoll - see 2.3.43 note
            last_int2_src = I2C_RdReg(REG_INT2_SRC);
            if (last_int2_src & 0x40)
                HarshEventDetected();
#endif

            /* Clearing the latch is the part that matters here - it releases the
               pin so the next event can be seen. */
            ISRstatus = I2C_RdReg(REG_INT1_SRC);

            /* Full re-initialisation is now throttled to once a minute (2.3.39).
               It used to run on EVERY motion interrupt, which while driving means
               roughly once a second, and each run rewrote INT2_CFG - resetting
               generator 2's duration counter before it could ever accumulate the
               300ms needed to fire. Together with the REFERENCE read and the
               register sweep (both removed above) that made harsh detection
               impossible: hraw stayed 0 across a full test drive on 2.3.38.
               Kept at a low rate because it is the only recovery path if the
               sensor loses its configuration, and motion wake depends on it. */
            {
                static int64_t last_accel_init_us = 0;
                int64_t _now = esp_timer_get_time();
                if (_now - last_accel_init_us > 60LL * 1000000LL) {
                    last_accel_init_us = _now;
                    InitAccelerometer();
                }
            }

            if(MotionTimer > TIME_TO_SLEEP)
            {
                //PostMotionEvent();
                #ifndef TIMER_ONLY_WAKEUP
                    MotionTimer=0;
                #endif
                if(Params.Fields.MotionAlertMode[0]=='S')
                {
                     if(GPSEnabled == 0)
                    {    
                        
                        DDelay();
                        ResetBuffer();
                        
                        Print("AT+CSCLK=0\r\n");
                            
                        
                    }
                   
                    WakeUp();
                    DDelay();
                    ResetBuffer();
                    SendAlert();
                    DDelay();
                    osDelay(3000);
                    ResetBuffer();
                    DDelay();

                }
                if(Params.Fields.MotionAlertMode[0]=='C')
                {
                    WakeUp();
                    DDelay();
                    ResetBuffer();
                    //Print("AT+CSCLK=2\r\n");
                    //DDelay();
                    Print("ATD");
                    Print(Params.Fields.rxNumber);
                    Print(";\r\n");
                    for(i=0;i<30;i++)
                    {
                        osDelay(1000);
                        SOS = gpio_get_level(GPIO_SOS);
                        if(SOS == 0) break;
                        
                    }
                    WakeUp();
                    Print("ATH\r\n");
                }
                  
            }
            #ifndef TIMER_ONLY_WAKEUP
                MotionTimer=0;
            #endif
        }

//          UTIL_LPM_SetStopMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_ENABLE);
        ENTER_STOP_MODE:
        #ifdef SLEEP_ENABLED
            DeepSleep();
        #endif
        if(SystemTimer%300==0)
        {
            SendATCommand("AT\r\n","OK","ERROR",5); // Sleep exit
            osDelay(1000);
            // SendATCommand("AT+CFUN=1\r\n","OK","ERROR",5);
            // osDelay(1000);
            {
                static uint8_t net_fail_count = 0;
                if(CheckNetwork() == 1)
                {
                    ESP_LOGW(TAG,"No network at 300s check (%d/3)", ++net_fail_count);
                    if(net_fail_count >= 3)
                    {
                        ESP_LOGW(TAG,"Network lost 15+ min — restarting");
                        esp_restart();
                    }
                }
                else
                {
                    net_fail_count = 0;
                }
            }
            //CheckSignalStrength();
            CheckNetworkLocation();
            // SendATCommand("AT+CFUN=0\r\n","OK","ERROR",5);
        }
        // Periodic OTA check every 24 hours (86400s). Startup check at boot covers the first run.
        if (ota_check_timer >= 86400UL) {
            ota_check_timer = 0;
            CheckAndApplyOTA();
        }
        if(Params.Fields.WorkingMode[0]=='S')
        {            
            
            
            if(GPSStatus == 'A' && SMSSent == 0)
            {
							   
                DDelay();
                DDelay();
              
                DDelay();
                ResetBuffer1();
                Print1("$PMTK161,1*29\r\n");
                DDelay();
                
                //ResetBuffer1();
                //Print1("$PMTK225,0*2B\r\n");
                
                DDelay();
                //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_RESET);
                GPSStatus = 0;
                D1 = 0;
                D2 = 0;
                SendLocation();
                DDelay();
                ResetBuffer();
				osDelay(3000);
                SMSSent = 1;
                DDelay();
                NoSignalTimer=0;

            }
            if(GPSStatus != 'A' && SMSSent == 0 && NoSignalTimer > 120)
            {
							
								
                DDelay();
                DDelay();
                //USART_ITConfig(USART1,USART_IT_RXNE, DISABLE);
                //Print1("$PMTK161,1*29\r\n");
                
                //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_SET);
                ResetBuffer1();
                Print1("$PMTK161,1*29\r\n");
                DDelay();
                
                //ResetBuffer1();
                //Print1("$PMTK225,0*2B\r\n");
                
                //DDelay();
                //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_RESET);
                GPSStatus = 0;
                D1 = 0;
                D2 = 0;
                SendLastLocation();
                DDelay();
                osDelay(1000);
                ResetBuffer();
                osDelay(3000);
                SMSSent = 1;
                NoSignalTimer = 0;
                DDelay();

            }
                
            //if( GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_6) != 0 ) 
            //    GPSAwake = 1;
            //else
            //    GPSAwake = 0;
            //if( Buff[i] == '+')i=0;
            
            //if( (Buff[i-21] == 'C') &&(Buff[i-20] == 'L') && (Buff[i-19] == 'I') 
            //        && (Buff[i-18] == 'P'))
            /*if(MapForward(Buff2,BUFF2_SIZE,(char*)"CLIP",4)!= NULL)
            {
                if(++RingCount>=2)
                {
                    //DDelay();
                    RingCount=0;
                    WakeUp();
                    Print("ATA\r\n");
                    //DDelay();
                    if(FirstTime == 0)
                    {
                        pToken = MapForward(Buff2,BUFF2_SIZE,(char*)": \"",3);
                        if(pToken != NULL)
                        {
                            j=3;
                            while(pToken[j] != '"')
                            {
                                rxNumber[j-3] = pToken[j];
                                j++;
                            }
                        }
                        rxNumber[j-3] = '\0';
                        FirstTime = 1;
                        //SendSMS();
                        FirstTime = 2;
                    }

                }
                DDelay();
                ResetBuffer();
            }*/
            //else if( (Buff[i-11] == 'C') &&(Buff[i-10] == 'M') && (Buff[i-9] == 'T') 
            //        && (Buff[i-8] == 'I'))
            pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"CMTI",4);
            if(pToken!= NULL)
            {
                DDelay();
                //pToken = MapForward(pToken,15,(char*)"\",",2);
                //if(pToken!= NULL)
                //{
                SMSNumber = pToken[11];
                //}
                
                
                WakeUp();
                //SendSMS();
                ResetBuffer();
                Print("AT+CMGR=");
                WriteUART2(SMSNumber);
                Print("\r\n");                        
                
                i=0;
                
                Count = 0;
                while(1)
                {
                    if(MapForward(Buff2,BUFF2_SIZE,(char*)"OK",2) != NULL)
                            break;
                    if((MapForward(Buff2,BUFF2_SIZE,(char*)"ERROR",5) != NULL) || (Count>20000))
                    {   break; }
                    Count++;
                }
                
                //
                // Get rx Number
                //
                pToken = MapForward(Buff2+35,70,(char*)"\",",2);
                if(pToken != NULL)
                {
                    //pToken-=10;
                    pToken--;
                    while(*pToken != '"')pToken--;
                    pToken++;
                    
                    // save sender number
                    while(pToken[i] != '"')
                    {
                        rxNumber[i] = pToken[i];
                        i++;
                    }
                    //for(i=0;i<10;i++)
                    //    rxNumber[i] = pToken[i];
                    rxNumber[i] = '\0';           
                }
                
                if( MapForward(Buff2,BUFF2_SIZE,(char*)"00000",5) != NULL)
                {                    
                    //minVal = MIN_NOALERT; maxVal = MAX_NOALERT;
                    Print1("\r\n");//USART_ITConfig(USART1,USART_IT_RXNE, ENABLE);
                    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_SET);
                    DDelay();
                    DDelay();
                   // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_RESET);
                    GPSStatus = 0;
                    D1 = 0;
                    D2 = 0;
                    SMSSent = 0;
                    if(GPSEnabled == 0)
                    {    
                        
                        DDelay();
                        ResetBuffer();
                        
                        Print("AT+CSCLK=0\r\n");
                            
                        
                    }
                }
                
               
                //SendSMS();
                
								
				if( MapForward(Buff2,BUFF2_SIZE,(char*)"$VALETRON_2_NUMBER",18) != NULL)
                { 
					


                    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"NUMBER:",7);
                    if(pToken != NULL)
                    {
                        osDelay(2000);
                        memset((void*)bstr,0,sizeof(bstr));
                        i=0;
                        while(pToken[7+i] != '#')
                        {
                            bstr[i] = pToken[7+i];
                            i++;
                            //if(i>250)goto DISCARD;
                        }
                        bstr[i] = '\0';
                    }
                    sscanf((void*)bstr, "%s",Params.Fields.rxNumber);
                    
                    StoreEEParams();
                    
                    Print("AT+CSCLK=0\r\n");
                    osDelay(200);
                    WakeUp();
                    ResetBuffer();            
                    Print((char*)"AT+CMGS=\"");
                    Print(rxNumber);
                    Print((char*)"\"\r\n");
                    DelayProc(850000);
                    Print((void*)bstr);
                    Print("  NUMBER UPDATE OK\r\n");
                    putcchar(0x1A);
                    DelayProc(850000);            
    
                }
                
                if( MapForward(Buff2,BUFF2_SIZE,(char*)"$VALETRON_2_THRESHOLD",21) != NULL)
                { 
					


                    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"THRESHOLD:",10);
                    if(pToken != NULL)
                    {
                        osDelay(2000);
                        memset((void*)bstr,0,sizeof(bstr));
                        i=0;
                        while(pToken[10+i] != '#')
                        {
                            bstr[i] = pToken[10+i];
                            i++;
                            //if(i>250)goto DISCARD;
                        }
                        bstr[i] = '\0';
                    }   
                    sscanf((void*)bstr, "%hhu",(char*)&Params.Fields.MotionThreshold);   
                    StoreEEParams();
                    	
                    Print("AT+CSCLK=0\r\n");
                    osDelay(200);
                    WakeUp();
                    ResetBuffer();            
                    Print((char*)"AT+CMGS=\"");
                    Print(rxNumber);
                    Print((char*)"\"\r\n");
                    DelayProc(850000);
                    Print((void*)bstr);
                    Print("  THRESHOLD SET OK\r\n");
                    putcchar(0x1A);
                    DelayProc(850000);            
    
                }
                
                
                if( MapForward(Buff2,BUFF2_SIZE,(char*)"$VALETRON_2_MOTIONALERT",23) != NULL)
                { 
                 
                    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"MOTIONALERT:",12);
                    if(pToken != NULL)
                    {
                        osDelay(2000);
                        memset((void*)bstr,0,sizeof(bstr));
                        i=0;
                        while(pToken[12+i] != '#')
                        {
                            bstr[i] = pToken[12+i];
                            i++;
                            //if(i>250)goto DISCARD;
                        }
                        bstr[i] = '\0';
                    }
                    sscanf((void*)bstr, "%s",Params.Fields.MotionAlertMode);
                    //sprintf(Params.Fields.MotionAlertMode,"%s","NULL");
                    StoreEEParams();
                    	
                    Print("AT+CSCLK=0\r\n");
                    osDelay(200);
                    WakeUp();
                    ResetBuffer();            
                    Print((char*)"AT+CMGS=\"");
                    Print(rxNumber);
                    Print((char*)"\"\r\n");
                    DelayProc(850000);
                    //Print("  THRESHOLD SET OK\r\n");
                    Print("MotionAlert:");
                    Print((void*)bstr);
                    Print("  UPDATED\r\n");
                    putcchar(0x1A);
                    DelayProc(850000);            
    
                }
                
                if( MapForward(Buff2,BUFF2_SIZE,(char*)"$VALETRON_2_WORKINGMODE",23) != NULL)
                { 
                    
                    pToken = MapForward(Buff2,BUFF2_SIZE,(char*)"WORKINGMODE:",12);
                    if(pToken != NULL)
                    {
                        osDelay(2000);
                        memset((void*)bstr,0,sizeof(bstr));
                        i=0;
                        while(pToken[12+i] != '#')
                        {
                            bstr[i] = pToken[12+i];
                            i++;
                            //if(i>250)goto DISCARD;
                        }
                        bstr[i] = '\0';
                    }
                    sscanf((void*)bstr, "%s",Params.Fields.WorkingMode);
                    //sprintf(Params.Fields.MotionAlertMode,"%s","NULL");
                    StoreEEParams();
                    
                    Print("AT+CSCLK=0\r\n");
                    osDelay(200);
                    WakeUp();
                    ResetBuffer();            
                    Print((char*)"AT+CMGS=\"");
                    Print(rxNumber);
                    Print((char*)"\"\r\n");
                    DelayProc(850000);
                    //Print("  THRESHOLD SET OK\r\n");
                    Print("WORKINGMODE:");
                    Print((void*)bstr);
                    Print("  UPDATED\r\n");
                    putcchar(0x1A);
                    DelayProc(850000);            
    
                }
                
                
                
//                 if( MapForward(Buff2,BUFF2_SIZE,(char*)"$VALETRON_2_ENABLE_M_ALERT#",27) != NULL)
//                { 
//									
//                    sprintf(Params.Fields.MotionAlertMode,"%s","CALL");
//                    StoreEEParams();
//                    
//                    Print("AT+CSCLK=0\r\n");
//                    osDelay(200);
//                    WakeUp();
//                    ResetBuffer();            
//                    Print((char*)"AT+CMGS=\"");
//                    Print(rxNumber);
//                    Print((char*)"\"\r\n");
//                    DelayProc(850000);
//                    Print("MotionAlert:Enabled \r\n");
//                    putcchar(0x1A);
//                    DelayProc(850000);
//                }
                 i=0;
								
                DeleteAllSMS();
                SMSNumber = 0;
                //LoopCount = 0;
                
                //i=0;
                ResetBuffer();
            }

        }
        
        else if(Params.Fields.WorkingMode[0]=='T')
        {
            if(GSMEnabled == 1)
            {
                if((MotionTimer < TIME_TO_SLEEP) || (IsQueueEmpty(RAMQueue)!=0))
                {
                    //if(SysClockConfigFlag!='H')
                    {
                            //SystemClock_Config();
                            //MX_USART2_UART_Init();
                            //MX_USART1_UART_Init();
                    }
                    #ifdef TIMER_ONLY_WAKEUP
                       while(SystemTimer<240){LPUARTTimer=0;};
                    #endif
                    
                    #ifdef DEBUG_PRINT
                        DebugPrint("Entering TCP -StartMainTask\r\n"); 
                    #endif
                    #ifdef SIM7600
                        XMQTT_Request(pFilename,1);
                    #endif
                    #ifdef SIM7070
                        YMQTT_Request(pFilename,1);
                    #endif
                    #ifdef SIM800
                        TCP_request(pFilename,1);
                    #endif
                    #ifdef TIMER_ONLY_WAKEUP
                        MotionTimer = TIME_TO_SLEEP+10;
                    #endif
                }
            }
        }
        else if(Params.Fields.WorkingMode[0]=='U')
        {
            if(GSMEnabled == 1)
            {
                if((MotionTimer < TIME_TO_SLEEP) || (IsQueueEmpty(RAMQueue)!=0))
                {
                    //if(SysClockConfigFlag!='H')
                    {
                            //SystemClock_Config();
                            //MX_USART2_UART_Init();
                            //MX_USART1_UART_Init();
                    }
                    #ifdef TIMER_ONLY_WAKEUP
                       while(SystemTimer<240){LPUARTTimer=0;};
                    #endif
                    
                    #ifdef DEBUG_PRINT
                        DebugPrint("Entering TCP -StartMainTask\r\n"); 
                    #endif
                    #ifdef SIM7600
                        XUDP_Request(pFilename,1);
                    #endif
//                    #ifdef SIM7070
//                        YMQTT_Request(pFilename,1);
//                    #endif
//                    #ifdef SIM800
//                        TCP_request(pFilename,1);
//                    #endif
                    #ifdef TIMER_ONLY_WAKEUP
                        MotionTimer = TIME_TO_SLEEP+10;
                    #endif
                }
            }
        }
        else //if(Params.Fields.WorkingMode[0]=='H')
        {
            if(GSMEnabled == 1)
            {
                // PostReboot();
                if(GetEvent(&GPacket,EVENT_QUEUE) == GET_SUCCESS)
                {
									  	
											
//                    #ifdef BATTERY_PRESENT
//                    DisableCharger();
//                    #endif
                    
                    //CheckBattery();

//                    #ifdef BATTERY_PRESENT
//                    EnableCharger();
//                    #endif
                    /* Always send dequeued events. The old gate
                       (MotionTimer < TIME_TO_SLEEP || queue non-empty) dropped
                       every parked-interval ping right after popping it —
                       total silence while parked. This unit is powered from
                       the vehicle battery; parked pings must go out too. */
                    if(1)
                    {
                        //if(SysClockConfigFlag!='H')
                        {
                            //SystemClock_Config();
                            //MX_USART2_UART_Init();
                            //MX_USART1_UART_Init();
                            
                            
                        }
                        //UpdateNetwork(2);
                        #ifdef TIMER_ONLY_WAKEUP
                            SystemTimer = 0;
                            while(SystemTimer<300 && GPSStatus != 'A'){LPUARTTimer=0;};
                            osDelay(5000);
                            PostReboot();    // Posting here after GPS is available
                            GetEvent(&GPacket,EVENT_QUEUE);
                
                        #endif
                        
                        #ifdef DEBUG_PRINT
                            DebugPrint("Entering HTTP -StartMainTask\r\n"); 
                        #endif
                        
                        #ifdef SIM7600
                            if((XHTTP_Request(pFilename,1)) != 0)
                            {
                                ServerRetries++;
                                if(ServerRetries > 2)
                                {
                                    /* Modem likely wedged: power-cycle it but
                                       KEEP queued events and track samples —
                                       the old path cleared everything and went
                                       to stop mode, losing all buffered data. */
                                    ServerRetries = 0;
                                    ESP_LOGW(TAG,"3 consecutive send failures - reinit modem");
                                    DisableGSM();
                                    InitGSM();
                                }
                            }
                            else
                            {
                                ServerRetries = 0;
                            }
                        #endif
                        #ifdef SIM7070
                            YHTTP_Request(pFilename,1);
                        
                        #endif    
                        #ifdef SIM800
                            ZHTTP_Request(pFilename,1);
                        #endif
                        
                        //UpdateNetwork(1);
                        #ifdef TIMER_ONLY_WAKEUP
                            MotionTimer = TIME_TO_SLEEP+10;
                        #endif
                        
                    }
                    else
                    {
                        ESP_LOGW(TAG,"MotionTimerexpired");
                    }
                }
                
                #ifdef EEPROM_FIFO
                if(EEPROMReadTimer > 900) // Check EEPROM every 15 min
                {
                    while(I2CBusyFlag == 1);
                    I2CBusyFlag = 1;
                    EEPROMReadCount = 0;
                    
                    while( (GetEEEvent(&GPacket) == GET_SUCCESS) && (EEPROMReadCount <5) )
                    {
                        
                        pFilename = filename;
                        //ProcessEventPacket(&GPacket);
                        
    //                    #ifdef BATTERY_PRESENT
    //                    DisableCharger();
    //                    #endif
                        
                        CheckBattery();
                        
    //                    #ifdef BATTERY_PRESENT
    //                    EnableCharger();
    //                    #endif
                        #ifdef SIM7600
                            XHTTP_Request(pFilename,1);                        
                        #endif
                        #ifdef SIM7070
                            YHTTP_Request(pFilename,1);                        
                        #endif    
                        #ifdef SIM800
                            ZHTTP_Request(pFilename,1);
                        #endif
                        EEPROMReadCount++;
                        
                    }
                    
                    I2CBusyFlag = 0;
                    EEPROMReadTimer = 0;
                }
                #endif
            }
        }// //if HTTP MODE end
        
//        if(GSMResetTimer > 300)
//        {
//            SendATCommand("AT+CRESET\r\n","RDY","ERROR",20);
//            osDelay(3000);
//            #ifdef EXT_ANT_ENABLED
//    
//                #ifdef SIM7600
//                    SendATCommand("AT+CGPS=1,1\r\n","OK","ERROR",3);
//                    SendATCommand("AT+CGPSINFO\r\n","OK","ERROR",3);
//                #endif
//            #endif
//            GSMResetTimer = 0;
//        }        
        // #ifdef VALTRACK_V4_VTS // No need to check and exit sleep mode of GSM for V4MF-MU

        /* 2.3.47 - throttle the registration check while GNSS is live.
           CheckNetwork() costs osDelay(1000) on entry plus osDelay(500) after
           the CREG reply, so calling it every pass put ~1.5s of pure sleep into
           a loop whose period IS the GPS track sampling interval (XCheckGPS()
           below runs once per pass). Measured effect on the van: track samples
           4-5s apart instead of the intended 1s.

           Registration state does not change second to second, so 30s is ample.
           When GNSS is NOT live we fall back to checking every pass: that is
           the state where a network fault is the likely cause and where track
           resolution does not matter anyway.

           Safe to skip because the call is side-effect free - it returns
           registration status and UpdateNetwork() only sets the status LED.
           Nothing in the network-loss auto-restart path depends on it. */
        {
            static int64_t last_net_check_us = 0;
            static unsigned char last_net_state = 1;
            int64_t now_us = esp_timer_get_time();

            if (GPSStatus != 'A' ||
                last_net_check_us == 0 ||
                now_us - last_net_check_us >= 30LL * 1000000LL)
            {
                last_net_state = CheckNetwork();
                last_net_check_us = now_us;
            }

            if(last_net_state == 1)
            {
                UpdateNetwork(0);
            }
            else
            {
                UpdateNetwork(1);
            }
        }
        #ifdef EXT_ANT_ENABLED
            XCheckGPS();
        #endif

        // #endif
        vTaskDelay(1);
        
    }
    
  /* USER CODE END 3 */ 

//  /* USER CODE BEGIN 5 */
//  /* Infinite loop */
//  for(;;)
//  {
//    osThreadFlagsWait(1,osFlagsWaitAll,osWaitForever);
//  }
  /* USER CODE END 5 */ 
}
// =========================================================
// Phase 8 — OTA Firmware Update
// Downloads firmware.bin in OTA_CHUNK_SIZE Range requests.
// uart_event_task is suspended only during the binary read
// of each chunk so uart_read_bytes() gets unfiltered bytes.
// =========================================================

// Read bytes directly from UART (task suspended) until the
// "+HTTPREAD: N\r\n" header arrives; returns N, or -1 on timeout.
static int ota_read_httpread_header(uint32_t timeout_ms)
{
    char hbuf[64] = {0};
    int  hlen = 0;
    bool in_digits = false;
    uint32_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        uint8_t b;
        if (uart_read_bytes(UART_PORT_NUM, &b, 1, pdMS_TO_TICKS(100)) != 1) continue;

        // Accumulate into a small rolling window to detect "+HTTPREAD: "
        hbuf[hlen % sizeof(hbuf)] = (char)b;
        hlen++;

        if (!in_digits) {
            // Look for "+HTTPREAD: " (11 chars) in recent bytes
            if (hlen >= 11) {
                int tail = (hlen - 11) % (int)sizeof(hbuf);
                char win[12] = {0};
                for (int i = 0; i < 11; i++)
                    win[i] = hbuf[(tail + i) % sizeof(hbuf)];
                if (memcmp(win, "+HTTPREAD: ", 11) == 0) {
                    in_digits = true;
                    hlen = 0; // repurpose buffer to collect the number
                    memset(hbuf, 0, sizeof(hbuf));
                }
            }
        } else {
            // Collecting the decimal number until \r\n
            if (hlen < (int)sizeof(hbuf))
                hbuf[hlen - 1] = (char)b;
            if (b == '\n' && hlen >= 2) {
                // Null-terminate and parse
                for (int i = (int)sizeof(hbuf) - 1; i >= 0; i--) {
                    if (hbuf[i] == '\r' || hbuf[i] == '\n') hbuf[i] = '\0';
                }
                return atoi(hbuf);
            }
        }
    }
    return -1;
}

// Read exactly n binary bytes directly from UART into buf.
static bool ota_read_exact(uint8_t *buf, int n, uint32_t timeout_ms)
{
    int got = 0;
    uint32_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (got < n) {
        if (xTaskGetTickCount() >= deadline) return false;
        int r = uart_read_bytes(UART_PORT_NUM, buf + got, n - got, pdMS_TO_TICKS(500));
        if (r > 0) got += r;
    }
    return true;
}

// Bring up PDP context for OTA (mirrors the setup in XHTTP_Request).
void nvs_save_position(void)
{
    nvs_handle_t h;
    if (nvs_open("valtrack", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, "last_lat", (int32_t)(last_good_lat * 1e6f));
        nvs_set_i32(h, "last_lon", (int32_t)(last_good_lon * 1e6f));
        nvs_commit(h);
        nvs_close(h);
    }
}

void nvs_load_position(void)
{
    nvs_handle_t h;
    if (nvs_open("valtrack", NVS_READONLY, &h) == ESP_OK) {
        int32_t lat_raw = 0, lon_raw = 0;
        nvs_get_i32(h, "last_lat", &lat_raw);
        nvs_get_i32(h, "last_lon", &lon_raw);
        nvs_close(h);
        if (lat_raw != 0 || lon_raw != 0) {
            last_good_lat = lat_raw / 1e6f;
            last_good_lon = lon_raw / 1e6f;
        }
    }
}

/* ---- OTA channel (2.3.51) --------------------------------------------
   Which OTA server this device reads. Persisted in NVS because the channel
   has to survive the reboot that the update itself causes - see SCI.h.
   Defaults to production, so a unit that has never been staged, or whose NVS
   was wiped, behaves exactly as before. */
void ota_channel_save(uint8_t ch)
{
    ota_channel = ch;
    nvs_handle_t h;
    if (nvs_open("valtrack", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, "ota_ch", ch);
        nvs_commit(h);
        nvs_close(h);
    }
}

void ota_channel_load(void)
{
    nvs_handle_t h;
    if (nvs_open("valtrack", NVS_READONLY, &h) == ESP_OK) {
        uint8_t ch = OTA_CHANNEL_PRODUCTION;
        /* Absent key leaves ch untouched, i.e. production. */
        nvs_get_u8(h, "ota_ch", &ch);
        nvs_close(h);
        ota_channel = (ch == OTA_CHANNEL_STAGING) ? OTA_CHANNEL_STAGING
                                                  : OTA_CHANNEL_PRODUCTION;
    }
}

static const char *ota_version_url(void)
{
    return (ota_channel == OTA_CHANNEL_STAGING) ? OTA_STAGING_VERSION_URL
                                                : OTA_VERSION_URL;
}

static const char *ota_firmware_url(void)
{
    return (ota_channel == OTA_CHANNEL_STAGING) ? OTA_STAGING_FIRMWARE_URL
                                                : OTA_FIRMWARE_URL;
}

static bool ota_network_up(void)
{
    sprintf(str, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", Params.Fields.APNName);
    SendATCommand(str, "OK", "ERROR", 5);
    SendATCommand("AT+CGACT=1,1\r\n", "OK", "ERROR", 10);
    return SendATCommand("AT+CGACT?\r\n", "+CGACT: 1,1", "ERROR", 10) == 1;
}

// Open HTTP session for the given URL (no Range).
static void ota_http_open(const char *url)
{
    /* Defensive teardown in case a previous session was left open by an abort. */
    ResetBuffer();
    Print("AT+HTTPTERM\r\n");
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"OK",    2) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR", 5) != NULL || LoopTimeout1 > 5) break;
    }
    ResetBuffer();
    Print("AT+HTTPINIT\r\n");
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"OK",    2) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR", 5) != NULL || LoopTimeout1 > 30) break;
    }
    ResetBuffer();
    Print("AT+HTTPPARA=\"CID\",1\r\n");
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"OK",    2) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR", 5) != NULL || LoopTimeout1 > 30) break;
    }
    ResetBuffer();
    snprintf(str, sizeof(str), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    Print(str);
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"OK",    2) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR", 5) != NULL || LoopTimeout1 > 30) break;
    }
}

static void ota_http_close(void)
{
    ResetBuffer();
    Print("AT+HTTPTERM\r\n");
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"OK",    2) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR", 5) != NULL || LoopTimeout1 > 30) break;
    }
}

// Trigger GET and return Content-Length from +HTTPACTION, or -1.
static int ota_http_action_size(void)
{
    ResetBuffer();
    Print("AT+HTTPACTION=0\r\n");
    LoopTimeout1 = 0;
    while (1) {
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ACTION:", 7) != NULL) break;
        if (MapForward(Buff2, BUFF2_SIZE, (char*)"ERROR",   5) != NULL || LoopTimeout1 > 60) return -1;
    }
    osDelay(300);
    // +HTTPACTION: 0,<code>,<size>
    char *p = MapForward(Buff2, BUFF2_SIZE, (char*)"+HTTPACTION:", 12);
    if (!p) return -1;
    int commas = 0;
    while (*p && commas < 2) { if (*p++ == ',') commas++; }
    return (*p) ? atoi(p) : -1;
}

// Parse {"ver":"X.Y.Z"} from Buff2; copies version string into out[maxlen].
static bool ota_parse_version(char *out, int maxlen)
{
    const char *key = "\"ver\":\"";
    char *p = MapForward(Buff2, BUFF2_SIZE, (char*)key, (unsigned short)strlen(key));
    if (!p) return false;
    p += strlen(key);
    char *end = p;
    while (end < Buff2 + BUFF2_SIZE && *end != '"' && *end != '\0') end++;
    int len = (int)(end - p);
    if (len <= 0 || len >= maxlen) return false;
    memcpy(out, p, len);
    out[len] = '\0';
    return true;
}

void CheckAndApplyOTA(void)
{
    char server_ver[32]  = {0};
    esp_ota_handle_t ota_handle = 0;
    esp_err_t        err;
    uint8_t         *chunk_buf = NULL;
    bool             ota_begun = false;

    ESP_LOGI(TAG, "OTA: check %s (running %s, channel %s)", ota_version_url(),
             FW_VERSION,
             ota_channel == OTA_CHANNEL_STAGING ? "STAGING" : "production");

    // ---- Step 1: fetch version.json (retry once if first attempt fails) ----
    bool version_ok = false;
    for (int otry = 0; otry < 2 && !version_ok; otry++) {
        if (otry > 0) {
            ESP_LOGW(TAG, "OTA: version fetch failed, retrying in 5s");
            osDelay(5000);
        }
        if (!ota_network_up()) {
            ESP_LOGW(TAG, "OTA: PDP context failed (attempt %d)", otry + 1);
            continue;
        }
        ota_http_open(ota_version_url());
        if (ota_http_action_size() < 0) { ota_http_close(); continue; }
        osDelay(500);
        ResetBuffer();
        SendATCommand("AT+HTTPREAD=0,200\r\n", "+HTTPREAD:", "ERROR", 10);
        osDelay(300);
        if (ota_parse_version(server_ver, sizeof(server_ver))) {
            ota_http_close();
            version_ok = true;
        } else {
            ota_http_close();
        }
    }
    if (!version_ok) {
        ESP_LOGW(TAG, "OTA: version check failed, skipping");
        return;
    }

    ESP_LOGI(TAG, "OTA: running=%s server=%s", FW_VERSION, server_ver);
    if (strcmp(FW_VERSION, server_ver) == 0) {
        ESP_LOGI(TAG, "OTA: up to date");
        return;
    }
    ESP_LOGI(TAG, "OTA: update available, downloading...");

    // ---- Step 2: determine firmware size ----
    // AT+HTTPACTION=0 downloads the full binary into the modem's HTTP buffer.
    // Keep the session open — Step 4 reads directly from this buffer.
    ota_http_open(ota_firmware_url());
    int firmware_size = ota_http_action_size();

    if (firmware_size <= 0 || firmware_size > (int)OTA_MAX_FIRMWARE) {
        ESP_LOGE(TAG, "OTA: bad firmware size %d", firmware_size);
        goto ota_abort;
    }

    // ---- Step 3: begin OTA write ----
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next) { ESP_LOGE(TAG, "OTA: no OTA partition"); return; }

    err = esp_ota_begin(next, firmware_size, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA: begin failed %s", esp_err_to_name(err));
        return;
    }
    ota_begun = true;

    chunk_buf = (uint8_t*)heap_caps_malloc(OTA_CHUNK_SIZE, MALLOC_CAP_DEFAULT);
    if (!chunk_buf) { ESP_LOGE(TAG, "OTA: malloc failed"); goto ota_abort; }

    // ---- Step 4: single-session sequential HTTPREAD ----
    // The modem already has the full binary buffered from the AT+HTTPACTION=0 in Step 2.
    // Suspend uart_event_task once for the entire download.
    vTaskSuspend(uart_event_task_handle);
    uart_flush_input(UART_PORT_NUM);

    int offset = 0;
    bool download_ok = true;
    while (offset < firmware_size) {
        int end_byte   = offset + OTA_CHUNK_SIZE - 1;
        if (end_byte >= firmware_size) end_byte = firmware_size - 1;
        int this_chunk = end_byte - offset + 1;

        // AT+HTTPREAD=<offset>,<len> reads from the modem's buffered response.
        // The modem delivers data in ≤1024-byte sub-blocks, each prefixed by "+HTTPREAD: N\r\n".
        snprintf(str, sizeof(str), "AT+HTTPREAD=%d,%d\r\n", offset, this_chunk);
        uart_write_bytes(UART_PORT_NUM, str, strlen(str));

        int total_received = 0;
        int zero_headers = 0;
        while (total_received < this_chunk) {
            int header_n = ota_read_httpread_header(30000);
            if (header_n < 0) { download_ok = false; break; }
            if (header_n == 0) {
                /* "+HTTPREAD: 0" is the modem's end-of-response marker. One
                   left over from the previous chunk can arrive after the
                   inter-chunk flush (timing race) — skip it. More than a few
                   means the modem actually ended the data stream early. */
                if (++zero_headers > 3) { download_ok = false; break; }
                continue;
            }
            if (!ota_read_exact(chunk_buf + total_received, header_n, 60000)) { download_ok = false; break; }
            total_received += header_n;
        }
        if (total_received != this_chunk) download_ok = false;
        if (!download_ok) break;

        err = esp_ota_write(ota_handle, chunk_buf, this_chunk);
        if (err != ESP_OK) { download_ok = false; break; }
        offset += this_chunk;
        ESP_LOGI(TAG, "OTA: %d/%d bytes", offset, firmware_size);

        // The modem appends "+HTTPREAD: 0\r\n" + "OK\r\n" after each AT+HTTPREAD response.
        // Drain these before sending the next AT+HTTPREAD or they poison the next header parse.
        vTaskDelay(pdMS_TO_TICKS(200));
        uart_flush_input(UART_PORT_NUM);
    }

    // Close HTTP session in direct UART mode before restoring uart_event_task.
    uart_write_bytes(UART_PORT_NUM, "AT+HTTPTERM\r\n", 13);
    { uint8_t _b; uint32_t _dl = xTaskGetTickCount() + pdMS_TO_TICKS(3000);
      while (xTaskGetTickCount() < _dl) uart_read_bytes(UART_PORT_NUM, &_b, 1, pdMS_TO_TICKS(100)); }

    vTaskResume(uart_event_task_handle);

    if (!download_ok) {
        ESP_LOGE(TAG, "OTA: download failed at offset %d/%d", offset, firmware_size);
        goto ota_abort;
    }
    ESP_LOGI(TAG, "OTA: all %d bytes downloaded", firmware_size);

    // ---- Step 5: commit and reboot ----
    if (esp_ota_end(ota_handle) != ESP_OK ||
        esp_ota_set_boot_partition(next) != ESP_OK) {
        ESP_LOGE(TAG, "OTA: commit failed");
        ota_begun = false; // already ended, don't abort again
        goto ota_abort;
    }

    free(chunk_buf);
    ESP_LOGI(TAG, "OTA: complete — rebooting into new firmware");
    esp_restart();
    return;  // unreachable

ota_abort:
    if (ota_begun) esp_ota_abort(ota_handle);
    if (chunk_buf) free(chunk_buf);
    ota_http_close();
    ESP_LOGW(TAG, "OTA: aborted, continuing with current firmware");
}

void InitFlash(void)
{
    esp_err_t err;
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
}
void app_main(void)
{
    
    
 
    //SleepHere();
    // vTaskDelay(5000 / portTICK_PERIOD_MS);    
    SystemInit();
    //esp_sleep_cpu_retention_init(); //  Also return true in sleep_modem.c  modem_domain_pd_allowed function.
    ///
    // EnableGSM();// For LED
    // MakeAllLED(PURPLE);
    // WriteLEDStatus();
    InitFlash();
    // /* name, period/time,  auto reload, timer ID, callback */
    // blehr_tx_timer = xTimerCreate("blehr_tx_timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, blehr_tx_hrate);
    //StoreEEParams();
    GetEEParams();

    // Migrate MotionThreshold from old default (0x12=18) to new sensitive value (0x04=4)
    if (Params.Fields.MotionThreshold == 0x12) {
        char mstr[10];
        Params.Fields.MotionThreshold = 0x04;
        sprintf(mstr, "%d", Params.Fields.MotionThreshold);
        StoreParamString("MotionThreshold", mstr);
        ESP_LOGI(TAG, "Migrated MotionThreshold 0x12->0x04");
    }

    //esp_nimble_hci_and_controller_init();      // 2 - Initialize ESP controller
    nimble_port_init();                        // 3 - Initialize the host stack
    ble_svc_gap_init();                        // 4 - Initialize NimBLE configuration - gap service
    ble_svc_gap_device_name_set(ble_device_name[0] ? ble_device_name : TAG); // 4 - Set device name AFTER gap init (gap init resets to default)
    ble_svc_gatt_init();                       // 4 - Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);            // 4 - Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);             // 4 - Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;      // 5 - Initialize application
    nimble_port_freertos_init(host_task);      // 6 - Run the thread
    //
    SleepWakeupReason();
    // Timer wakeup = 8hr heartbeat from parked-long deep sleep.
    // Set ParkLongTimer to threshold so DeepSleep() fires after one ping.
    // MUST check reset reason: the wakeup-cause register survives software
    // resets (esp_restart after OTA/V_RESET) and a stale TIMER bit here sent
    // the device into phantom deep sleep minutes after an OTA reboot.
    if (esp_reset_reason() == ESP_RST_DEEPSLEEP &&
        (esp_sleep_get_wakeup_causes() & (1ULL << ESP_SLEEP_WAKEUP_TIMER))) {
        ParkLongTimer = PARK_LONG_SECONDS;
        heartbeat_wake = 1;
    }
    // osDelay(5000);
    // DisableGSM();
    // DisableMainPower();
    // esp_bluedroid_disable();
    // esp_bt_controller_disable();
    // esp_wifi_stop();
    // EnterDeepSleep();
    //ESP_LOGI(TAG,"Before tasks");
    xTaskCreate(ADCTask, "ADCTask", 2048, NULL, 10, NULL);
    xTaskCreate(StartTimerTask, "StartTimerTask", 4096, NULL, 10, NULL);
    xTaskCreate(StartMainTask, "StartMainTask", 8192, NULL, 10, NULL); //TIMER_TASK_STACK_SIZE
    /* No harsh-driving task since 2.3.37 - detection is done by the LIS3DH's
       own interrupt generator and handled from the existing INT1 poll. */
    
    
    
}
