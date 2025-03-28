#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "fds_internal_defs.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "pwm_wrap.h"
#include "estc_service.h"
#include "led_common.h"

#define DEVICE_NAME                     "Custom LED controller"                 /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                   /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    { BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE },
    { ESTC_SERVICE_UUID, BLE_UUID_TYPE_BLE }
};

ble_estc_service_t m_estc_service; /**< ESTC example BLE service */

#define NOTIFYING_DELAY_MS 100

APP_TIMER_DEF(notify_led_timer);

static const led_params_t led_params_default = {
    .color = (rgb_t) {0xff, 0x00, 0xff},
    .state = 0x01
};

static volatile led_params_t led_params = led_params_default;

static void notify_led_timer_handler(void *ctx)
{
    ble_gatts_hvx_params_t hvx_params;

    char strbuf[LED_READ_LEN + 1] = {0};
    uint16_t len;

    snprintf(strbuf,
             LED_READ_LEN,
             LED_READ_TEMPLATE,
             led_params.color.r,
             led_params.color.g,
             led_params.color.b,
             led_params.state ? "on" : "off");

    len = strlen(strbuf);

    hvx_params.handle = m_estc_service.led_notify_char_handles.value_handle;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len = &len;
    hvx_params.p_data = (uint8_t *) strbuf;

    sd_ble_gatts_hvx(m_estc_service.connection_handle, &hvx_params);

    NRF_LOG_INFO("LED Notify");
}

enum {
    pwm_channel_indicator = 0,
    pwm_channel_red,
    pwm_channel_green,
    pwm_channel_blue
};

static nrfx_pwm_t my_pwm_instance = NRFX_PWM_INSTANCE(0);
static nrf_pwm_values_individual_t my_pwm_seq_values;
static nrf_pwm_sequence_t const my_pwm_seq =
{
    .values.p_individual = &my_pwm_seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(my_pwm_seq_values),
    .repeats             = 0,
    .end_delay           = 0
};

static pwm_wrapper_t my_pwm =
{
    .pwm = &my_pwm_instance,
    .seq_values = &my_pwm_seq_values,
    .seq = &my_pwm_seq
};

static void led_set_state(uint8_t state)
{
    (state ? pwm_start : pwm_stop)(&my_pwm);
}

static void led_set_color(rgb_t color)
{
    pwm_set_duty_cycle(&my_pwm, pwm_channel_red, ((uint16_t) color.r * pwm_max_pct) / 255);
    pwm_set_duty_cycle(&my_pwm, pwm_channel_green, ((uint16_t) color.g * pwm_max_pct) / 255);
    pwm_set_duty_cycle(&my_pwm, pwm_channel_blue, ((uint16_t) color.b * pwm_max_pct) / 255);
}

static void advertising_start(bool erase_bonds);

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    app_timer_create(&notify_led_timer,
                     APP_TIMER_MODE_SINGLE_SHOT,
                     notify_led_timer_handler);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
	APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    err_code = estc_ble_service_init(&m_estc_service, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
}

/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    UNUSED_VARIABLE(err_code);

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("ADV Event: Start fast advertising");
            break;

        case BLE_ADV_EVT_IDLE:
            NRF_LOG_INFO("ADV Event: idle, no connectable advertising is ongoing");
            sleep_mode_enter();
            break;

        default:
            break;
    }
}

void on_led_color_char_write(const uint8_t *data, uint16_t len, bool on_connected);
void on_led_state_char_write(const uint8_t *data, uint16_t len, bool on_connected);

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);

            break;

        case BLE_GAP_EVT_CONNECTED:
            on_led_color_char_write((uint8_t *) &(led_params.color), 3, true);
            on_led_state_char_write((uint8_t *) &(led_params.state), 1, true);

            NRF_LOG_INFO("Connected (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);

            APP_ERROR_CHECK(err_code);

            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);

            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request (conn_handle: %d)", p_ble_evt->evt.gap_evt.conn_handle);
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout (conn_handle: %d)", p_ble_evt->evt.gattc_evt.conn_handle);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout (conn_handle: %d)", p_ble_evt->evt.gatts_evt.conn_handle);
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }

    estc_ble_service_on_ble_event(p_ble_evt, p_context);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
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

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated when button is pressed.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break; // BSP_EVENT_SLEEP

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break; // BSP_EVENT_KEY_0

        default:
            break;
    }
}

/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Function for initializing buttons and leds.
 */
static void button_init(void)
{
    ret_code_t err_code;

    err_code = bsp_init(BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
	LOG_BACKEND_USB_PROCESS();
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event
    }
    else
    {
        ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

        APP_ERROR_CHECK(err_code);
    }
}

#define FILE_ID 0xBEEF
#define RECORD_KEY 0xBABE

static void display_storage_state(void)
{
    fds_stat_t stat;

    if (NRF_SUCCESS == fds_stat(&stat))
    {
        NRF_LOG_INFO("\e[32mFDS stat\e[0m: %d valid record, %d dirty records, %d words used, %d words freeable",
                     stat.valid_records,
                     stat.dirty_records,
                     stat.words_used,
                     stat.freeable_words);
    }
    else
    {
        NRF_LOG_INFO("Unable to retrieve file system statistics");
    }
}

#define LED_SAVES_FDS_USAGE_LIMIT ((uint32_t) FDS_PHY_PAGE_SIZE * 75 / 100)

static void check_and_trigger_gc(void)
{
    fds_stat_t stat;

    if (NRF_SUCCESS == fds_stat(&stat))
    {
        if (stat.freeable_words > LED_SAVES_FDS_USAGE_LIMIT)
        {
            if (NRF_SUCCESS == fds_gc())
            {
                NRF_LOG_INFO("Trigger garbage collection");
            }
            else
            {
                NRF_LOG_WARNING("Unable to trigger garbage collection!");
            }
        }
    }
    else
    {
        NRF_LOG_INFO("Unable to retrieve file system statistics");
    }
}

void led_save_state(void)
{
    fds_record_desc_t record_desc;
    fds_find_token_t record_token;
    fds_record_t record;

    ret_code_t ret_code;

    memset(&record_token, 0, sizeof(fds_find_token_t));

    record.file_id = FILE_ID;
    record.key = RECORD_KEY;
    record.data.p_data = (void *) &led_params;
    record.data.length_words = sizeof(led_params_t) / 4;

    if (NRF_SUCCESS == fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &record_token))
    {
        ret_code = fds_record_update(&record_desc, &record);
    }
    else
    {
        ret_code = fds_record_write(&record_desc, &record);
    }

    if (ret_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("LED parameters saved to flash memory");
    }
    else
    {
        fds_gc();
        NRF_LOG_INFO("Unable to save LED parameters!");
    }

    display_storage_state();
    check_and_trigger_gc();
}

void on_led_color_char_write(const uint8_t *data, uint16_t len, bool on_connected)
{
    if (len != ESTC_GATT_LED_COLOR_CHAR_LEN)
    {
        return;
    }

    led_params.color = *(rgb_t *) data;
    led_set_color(led_params.color);
    
    if (on_connected)
    {
        ble_gatts_value_t value;

        value.len = ESTC_GATT_LED_COLOR_CHAR_LEN;
        value.offset = 0;
        value.p_value = (uint8_t *) &(led_params.color);

        sd_ble_gatts_value_set(m_estc_service.connection_handle,
                               m_estc_service.led_color_char_handles.value_handle,
                               &value);
    }
    else
    {
        led_save_state();

        app_timer_start(notify_led_timer,
                        APP_TIMER_TICKS(NOTIFYING_DELAY_MS),
                        NULL);
    }

    NRF_LOG_INFO("LED color has been updated");
}

void on_led_state_char_write(const uint8_t *data, uint16_t len, bool on_connected)
{
    if (len != ESTC_GATT_LED_STATE_CHAR_LEN)
    {
        return;
    }

    led_params.state = *(uint8_t *) data;
    led_set_state(led_params.state);
    
    if (on_connected)
    {
        ble_gatts_value_t value;

        value.len = ESTC_GATT_LED_STATE_CHAR_LEN;
        value.offset = 0;
        value.p_value = (uint8_t *) &(led_params.state);

        sd_ble_gatts_value_set(m_estc_service.connection_handle,
                               m_estc_service.led_state_char_handles.value_handle,
                               &value);
    }
    else
    {
        led_save_state();

        app_timer_start(notify_led_timer,
                        APP_TIMER_TICKS(NOTIFYING_DELAY_MS),
                        NULL);
    }

    NRF_LOG_INFO("LED state has been updated");
}

void fds_on_init(void)
{
    fds_record_desc_t record_desc;
    fds_find_token_t record_token;
    fds_flash_record_t flash_record;

    memset(&record_token, 0, sizeof(fds_find_token_t));

    if (NRF_SUCCESS == fds_record_find(FILE_ID, RECORD_KEY, &record_desc, &record_token))
    {
        if (NRF_SUCCESS == fds_record_open(&record_desc, &flash_record))
        {
            led_params = *(led_params_t *) flash_record.p_data;
            fds_record_close(&record_desc);

            NRF_LOG_INFO("Read LED parameters from flash memory");
        }
    }

    led_set_color(led_params.color);
    led_set_state(led_params.state);
}

void fds_events_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            fds_on_init();
            break;

        case FDS_EVT_GC:
            NRF_LOG_INFO("Garbage collection is done");
            display_storage_state();
            break;

        default:
            break;
    }
}

void led_save_init(void)
{
    fds_register(fds_events_handler);

    if (NRF_SUCCESS != fds_init())
    {
        pwm_set_duty_cycle(&my_pwm, pwm_channel_indicator, pwm_max_pct);
    }
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(bool erase_bonds)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    if (erase_bonds)
    {
        pm_peers_delete();
    }

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = false;
    sec_param.mitm           = false;
    sec_param.lesc           = 0;
    sec_param.keypress       = 0;
    sec_param.io_caps        = BLE_GAP_IO_CAPS_NONE;
    sec_param.oob            = false;
    sec_param.min_key_size   = 7;
    sec_param.max_key_size   = 16;
    sec_param.kdist_own.enc  = 0;
    sec_param.kdist_own.id   = 0;
    sec_param.kdist_peer.enc = 0;
    sec_param.kdist_peer.id  = 0;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

static void pwm_hw_init(void)
{
    const uint8_t channels[NRF_PWM_CHANNEL_COUNT] = {
        NRF_GPIO_PIN_MAP(0,  6), /* Indicator Channel */
        NRF_GPIO_PIN_MAP(0,  8), /* Red Channel */
        NRF_GPIO_PIN_MAP(1,  9), /* Green Channel */
        NRF_GPIO_PIN_MAP(0, 12), /* Blue Channel */
    };

    pwm_init(&my_pwm, channels, pwm_max_pct, true);
    pwm_start(&my_pwm);
    pwm_set_duty_cycle(&my_pwm, pwm_channel_indicator, 0);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    pwm_hw_init();
    led_save_init();

    // Initialize.
    log_init();
    timers_init();
    button_init();
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    peer_manager_init(true);

    // Start execution.
    NRF_LOG_INFO("ESTC custom LED service started");
    application_timers_start();

    advertising_start(false);

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}

/**
 * @}
 */
