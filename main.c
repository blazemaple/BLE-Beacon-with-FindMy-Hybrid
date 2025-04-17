/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_beacon_main main.c
 * @{
 * @ingroup ble_sdk_app_beacon
 * @brief Beacon Transmitter Sample Application main file.
 *
 * This file contains the source code for an Beacon transmitter sample application.
 */

#include "app_timer.h"
#include "ble_advdata.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_soc.h"
#include <stdbool.h>
#include <stdint.h>

#include "openhaystack.h"
// printer
#include "dong_printer.h"

/** 
 * advertising interval in milliseconds 
 */
#define ADVERTISING_INTERVAL  100     // unit 1ms
#define ADVERTISING_TIMEOUT   100     // unit 10ms
#define BREAK_TIME            10000    // unit 1ms

static char public_key[28] = { 0x13, 0x29, 0xDA, 0x4C, 0xBF, 0xBC, 0x82, 0x55, 0x3D, 0xE8, 0x60, 0xCA, 0x50, 0x88, 0x78, 0x09, 0x34, 0x9B, 0xA2, 0xE1, 0xD5, 0xF0, 0x2C, 0x24, 0x92, 0x53, 0x58, 0xD2 };
static char deviceID[2] = { 0x00, 0x08 };

uint8_t which_template = 0; // 0 == findmy, 1 == ibeacon

static uint8_t offline_finding_adv_template[] = {
	0x1e, /* Length (30) */
	0xff, /* Manufacturer Specific Data (type 0xff) */
	0x4c, 0x00, /* Company ID (Apple) */
	0x12, 0x19, /* Offline Finding type and length */
	0xf1, /* State */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, /* First two bits */
	0x00, /* Hint (0x00) */
};

static uint8_t ibeacon_adv_template[] = {
        0x02,                         // 00     Device type
        0x15,                         // 01     advertising packet length
        0x55, 0x43, 0x4c,             // 02-04  UUID. identify
        0x74, 0x6f, 0x6f, 0x6c,       // 05-08  UUID. device type
        0x64,                         // 09     UUID. vBat
        0x00, 0x00, 0x00, 0x00,       // 10-13  UUID. no USE
        0x00, 0x00, 0x00, 0x00,       // 14-17  UUID. no USE
        0x00, 0x01,                   // 18-19  Major. Device ID
        0x10, 0x00,                   // 20-21  Minor.
        0xC3                          // 22     TX power reference
};

#define APP_BLE_CONN_CFG_TAG 1 /**< A tag identifying the SoftDevice BLE configuration. */

#define NON_CONNECTABLE_ADV_INTERVAL MSEC_TO_UNITS(100, UNIT_0_625_MS) /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

/**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_OBSERVER_PRIO 3

#define APP_BEACON_INFO_LENGTH 0x17   /**< Total length of information advertised by the Beacon. */

#define APP_COMPANY_IDENTIFIER 0x004C /**< Company identifier for Nordic Semiconductor ASA. as per www.bluetooth.org. */

#define DEAD_BEEF 0xDEADBEEF /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#if defined(USE_UICR_FOR_MAJ_MIN_VALUES)
#define MAJ_VAL_OFFSET_IN_BEACON_INFO 18 /**< Position of the MSB of the Major Value in m_beacon_info array. */
#define UICR_ADDRESS 0x10001080          /**< Address of the UICR register used by this example. The major and minor versions to be encoded into the advertising data will be picked up from this location. */
#endif

static ble_gap_adv_params_t m_adv_params;                     /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */

/**@brief Struct that contains pointers to the encoded advertising data. */
//static ble_gap_adv_data_t m_adv_data =
//    {
//        .adv_data =
//            {
//                .p_data = m_enc_advdata,
//                .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX},
//        .scan_rsp_data =
//            {
//                .p_data = NULL,
//                .len = 0

//            }};
/****************************************************************************/
/**
 *                              watch dog
 *
 *******************************************************************************/
#include "nrf_drv_wdt.h"

static nrf_drv_wdt_channel_id m_channel_id;
void                          wdt_event_handler(void) {}


/****************************************************************************/
/**
 *                              Timer
 *
 *******************************************************************************/
#include "app_timer.h"
#include "nrf_drv_clock.h"

APP_TIMER_DEF(BLE_timer);
enum main_evt_flags
{
    CPU_IDLE,
    BLE_ACTION
};
typedef enum main_evt_flags evtMachine_t;

static evtMachine_t StateMachine = CPU_IDLE;
static void BLE_ctrl(void* p_context) { StateMachine = BLE_ACTION; }
/**@brief Function for initializing timers. */
static void timers_init(void) {
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void lfclk_request(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}

/****************************************************************************/
/**
 *                              saadc
 *
 *******************************************************************************/
#include "nrf_drv_saadc.h"
#include "nrf_saadc.h"

#define SAMPLES_IN_BUFFER 1
#define MAX_BATT_VOLTAGE   3000   // mV
#define MIN_BATT_VOLTAGE   2000   // mV
static nrf_saadc_value_t m_buffer_pool[2][SAMPLES_IN_BUFFER];
static uint16_t          vBat = 0;
static uint16_t          battery_percent = 0;
static void              saadc_init(void);

static void saadc_callback(nrf_drv_saadc_evt_t const* p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;

        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        int battt = 0;
        int adc_value = p_event->data.done.p_buffer[0];
        vBat = (adc_value * 3600) / 1024;
        // 限制計算範圍，避免溢出
        if (vBat > MAX_BATT_VOLTAGE) vBat = MAX_BATT_VOLTAGE;
        if (vBat < MIN_BATT_VOLTAGE) vBat = MIN_BATT_VOLTAGE;

        // 計算剩餘電量百分比
        battery_percent = (vBat - MIN_BATT_VOLTAGE) * 100 / (MAX_BATT_VOLTAGE - MIN_BATT_VOLTAGE);

        data_print("[vBat] %d mV, [Battery] %d%%\n", vBat, battery_percent);
        ibeacon_adv_template[9] = battery_percent;
    }
}

void saadc_uninit(void) {
    nrf_drv_saadc_uninit();
    NRF_SAADC->ENABLE = 0;
}


static void saadc_init(void)
{
    ret_code_t err_code;

    nrf_saadc_channel_config_t channel_config =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_VDD); // batt

    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name) {
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_findmy_init(void) {
    which_template = 0;
    uint32_t err_code;
    // 設定廣播數據
    ble_gap_adv_data_t m_adv_data = {
        .adv_data.p_data = offline_finding_adv_template,
        .adv_data.len = sizeof(offline_finding_adv_template),
        .scan_rsp_data.p_data = NULL, // 無掃描響應數據
        .scan_rsp_data.len = 0
    };

    // LOG: 廣播數據
    msg_print("Setting advertising data...\n");
    for (int i = 0; i < sizeof(offline_finding_adv_template); i++) {
        data_print("%02X ", offline_finding_adv_template[i]);
    }
    data_print("\n");
    // 配置廣播數據
    //ble_advdata_t advdata;
    //memset(&advdata, 0, sizeof(advdata));
    //advdata.name_type = BLE_ADVDATA_NO_NAME; // 不廣播設備名稱
    //advdata.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED | BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;
    //advdata.p_manuf_specific_data = &manuf_specific_data;
    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr = NULL;  // Undirected advertisement.
    m_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval = MSEC_TO_UNITS(ADVERTISING_INTERVAL, UNIT_0_625_MS);
    m_adv_params.duration = ADVERTISING_TIMEOUT;
    m_adv_params.primary_phy = BLE_GAP_PHY_1MBPS;

    //err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    //APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);
}

static void advertising_ibeacon_init(void) {
    which_template = 1;
    static ble_advdata_t            advdata;
    static ble_advdata_manuf_data_t manuf_specific_data;
    static ble_gap_adv_data_t m_adv_data = {
        .adv_data      = {.p_data = m_enc_advdata, .len = BLE_GAP_ADV_SET_DATA_SIZE_MAX},
        .scan_rsp_data = {.p_data = NULL, .len = 0}
    };
    uint32_t err_code;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;

    manuf_specific_data.data.p_data = (uint8_t*)ibeacon_adv_template;
    manuf_specific_data.data.size = APP_BEACON_INFO_LENGTH;

    // LOG: 廣播數據
    msg_print("Setting advertising data...\n");
    for (int i = 0; i < sizeof(ibeacon_adv_template); i++) {
        data_print("%02X ", ibeacon_adv_template[i]);
    }
    data_print("\n");

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type = BLE_ADVDATA_NO_NAME;
    advdata.flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    advdata.p_manuf_specific_data = &manuf_specific_data;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr = NULL;  // Undirected advertisement.
    m_adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;

    // NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.interval = MSEC_TO_UNITS(ADVERTISING_INTERVAL, UNIT_0_625_MS);
    m_adv_params.duration = ADVERTISING_TIMEOUT;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void) {
    ret_code_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
}


void ble_evt_handler(ble_evt_t const* p_ble_evt, void* p_context)
{
    const ble_gap_evt_t*                    p_gap_evt = &p_ble_evt->evt.gap_evt;
    const ble_gap_evt_adv_set_terminated_t* adv_set_terminated =
        &p_gap_evt->params.adv_set_terminated;

    switch (adv_set_terminated->reason)
    {
        case BLE_GAP_EVT_ADV_SET_TERMINATED_REASON_TIMEOUT:
        {
            
            // Start the timer to put the CPU in idle state.
            APP_ERROR_CHECK(app_timer_start(BLE_timer, APP_TIMER_TICKS(BREAK_TIME), NULL));
        }
        
        default:
            break;
    }
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void) {
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**
 * setMacAddress will set the bluetooth address
 */
void setMacAddress(uint8_t *addr) {
    ble_gap_addr_t gap_addr;
    uint32_t err_code;

    memcpy(gap_addr.addr, addr, sizeof(gap_addr.addr));
    gap_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
    ret_code_t ret_code = sd_ble_gap_addr_set(&gap_addr);
    APP_ERROR_CHECK(ret_code);
}

/**@brief Function for initializing LEDs. */
static void leds_init(void) {
    ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void) {
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void) {
    if (NRF_LOG_PROCESS() == false) {
        nrf_pwr_mgmt_run();
    }
}

void print_ble_address(uint8_t const *ble_address) {
    msg_print("BLE Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 ble_address[5], ble_address[4], ble_address[3],
                 ble_address[2], ble_address[1], ble_address[0]);
}

/*
 * fill_adv_template_from_key will set the advertising data based on the remaining bytes from the advertised key
 */
void fill_adv_template_from_key(char key[28]) {
	/* copy last 22 bytes */
	memcpy(&offline_finding_adv_template[7], &key[6], 22);
	/* append two bits of public key */
	offline_finding_adv_template[29] = key[0] >> 6;
}

void BLE_process_exec(void) {
    // update vBat
    for (int i = 0; i < SAMPLES_IN_BUFFER; i++)
        APP_ERROR_CHECK(nrf_drv_saadc_sample());
    
    if (which_template == 0) {
        advertising_ibeacon_init();
        msg_print("Change to iBeacon\n");
    } else {
        advertising_findmy_init();
        msg_print("Change to FindMy\n");
    }
    // start adv
    advertising_start();
}

/**
 * @brief Function for application main entry.
 */
int main(void) {
    // Variable to hold the data to advertise
    uint8_t *ble_address;
    uint8_t *raw_data;
    uint8_t data_len;
    // Initialize.
    memcpy(&ibeacon_adv_template[18], deviceID, 2);
    log_init();
    timers_init();
    lfclk_request();
    // Battery: vBat detect
    saadc_init();
    // leds_init();
    power_management_init();
    ble_stack_init();
    // Set key to be advertised
    data_len = setAdvertisementKey(public_key, &ble_address, &raw_data);
    setMacAddress(ble_address);
    print_ble_address(ble_address);
    
    fill_adv_template_from_key(public_key);
    advertising_findmy_init();

    // setup a watch dog
    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
    config.reload_value         = 15 * 1000;
    APP_ERROR_CHECK(nrf_drv_wdt_init(&config, wdt_event_handler));
    APP_ERROR_CHECK(nrf_drv_wdt_channel_alloc(&m_channel_id));
    nrf_drv_wdt_enable();

    // Start execution.
    msg_print("Beacon example started.\n");
    advertising_start();
    APP_ERROR_CHECK(app_timer_create(&BLE_timer, APP_TIMER_MODE_SINGLE_SHOT, BLE_ctrl));

    // Enter main loop.
    for (;;) {
        switch (StateMachine) {
            case CPU_IDLE:
                idle_state_handle();
                // feed Watch dog
                nrf_drv_wdt_channel_feed(m_channel_id);
                break;
            case BLE_ACTION:
                BLE_process_exec();
                StateMachine = CPU_IDLE;
                break;
            default:
                StateMachine = CPU_IDLE;
                break;
        }
    }
}

/**
 * @}
 */