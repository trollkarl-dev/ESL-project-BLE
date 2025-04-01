#include "estc_service.h"

#include "app_error.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_usb.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

#include "fds.h"
#include "fds_internal_defs.h"

#include "nrf_gpio.h"

#include "pwm_wrap.h"
#include "led_common.h"

#define NOTIFYING_DELAY_MS 100
APP_TIMER_DEF(notify_led_timer);

static ble_uuid128_t const m_base_uuid128 = { ESTC_BASE_UUID };

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, void *ctx);

extern ble_estc_service_t m_estc_service; 

#define FILE_ID 0xBEEF
#define RECORD_KEY 0xBABE

static const led_params_t led_params_default = {
    .color = (rgb_t) {0xff, 0x00, 0xff},
    .state = 0x01
};

static volatile led_params_t led_params = led_params_default;

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

    if (NRF_SUCCESS != fds_stat(&stat))
    {
        NRF_LOG_INFO("Unable to retrieve file system statistics");
        return;
    }

    if (stat.freeable_words > LED_SAVES_FDS_USAGE_LIMIT)
    {
        ret_code_t ret_code = fds_gc();

        switch (ret_code)
        {
            case NRF_SUCCESS:
                NRF_LOG_INFO("Trigger garbage collection");
                break;

            default:
                NRF_LOG_WARNING("Unable to trigger garbage collection!");
                break;
        }
    }
}

static void led_save_state(void)
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

static void fds_on_init(void)
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

static void fds_events_handler(fds_evt_t const * p_evt)
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

static void on_led_color_char_write(const uint8_t *data, uint16_t len, bool on_connected)
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

    NRF_LOG_INFO("LED color has been updated (RGB: #%02X%02X%02X)",
                 led_params.color.r,
                 led_params.color.g,
                 led_params.color.b);
}

static void on_led_state_char_write(const uint8_t *data, uint16_t len, bool on_connected)
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

    NRF_LOG_INFO("LED state has been updated (%s)",
                 led_params.state ? "on" : "off");
}

static void on_write(const ble_evt_t *ble_evt)
{
    const ble_gatts_evt_write_t * p_evt_write = &ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == m_estc_service.led_color_char_handles.value_handle)
    {
        on_led_color_char_write(p_evt_write->data, p_evt_write->len, false);
    }

    if (p_evt_write->handle == m_estc_service.led_state_char_handles.value_handle)
    {
        on_led_state_char_write(p_evt_write->data, p_evt_write->len, false);
    }
}

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx)
{
    switch (ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(ble_evt);
            break;
        
        case BLE_GAP_EVT_CONNECTED:
            on_led_color_char_write((uint8_t *) &(led_params.color), ESTC_GATT_LED_COLOR_CHAR_LEN, true);
            on_led_state_char_write((uint8_t *) &(led_params.state), ESTC_GATT_LED_STATE_CHAR_LEN, true);

            break;

        default:
            break;
    }
}

ret_code_t estc_ble_service_init(ble_estc_service_t *service, void *ctx)
{
    ret_code_t error_code;
    ble_uuid_t service_uuid = { .uuid = ESTC_SERVICE_UUID,
                                .type = ESTC_UUID_TYPE };

    app_timer_create(&notify_led_timer,
                     APP_TIMER_MODE_SINGLE_SHOT,
                     notify_led_timer_handler);

    error_code = sd_ble_uuid_vs_add(&m_base_uuid128, &service_uuid.type);
    APP_ERROR_CHECK(error_code);

    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                          &service_uuid,
                                          &m_estc_service.service_handle);
    APP_ERROR_CHECK(error_code);

    NRF_LOG_DEBUG("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    NRF_LOG_DEBUG("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    NRF_LOG_DEBUG("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);

    return estc_ble_add_characteristics(service, ctx);
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, void *ctx)
{
    ble_add_char_params_t add_char_params;
    ble_add_char_user_desc_t add_char_user_desc;
    ble_gatts_char_pf_t char_pf;

    ret_code_t error_code;
    
    const char led_color_char_user_description[] = LED_COLOR_CHAR_DESCRIPTION;
    const char led_state_char_user_description[] = LED_STATE_CHAR_DESCRIPTION;
    const char led_notify_char_user_description[] = LED_NOTIFY_CHAR_DESCRIPTION;

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(led_color_char_user_description);
    add_char_user_desc.size = strlen(led_color_char_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) led_color_char_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    add_char_params.uuid = ESTC_GATT_LED_COLOR_CHAR_UUID;
    add_char_params.uuid_type = ESTC_UUID_TYPE;
    add_char_params.init_len = ESTC_GATT_LED_COLOR_CHAR_LEN;
    add_char_params.max_len = ESTC_GATT_LED_COLOR_CHAR_LEN;
    add_char_params.char_props.write = 1;
    add_char_params.char_props.read = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.write_access = SEC_JUST_WORKS;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->led_color_char_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(led_state_char_user_description);
    add_char_user_desc.size = strlen(led_state_char_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) led_state_char_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    add_char_params.uuid = ESTC_GATT_LED_STATE_CHAR_UUID;
    add_char_params.uuid_type = ESTC_UUID_TYPE;
    add_char_params.init_len = ESTC_GATT_LED_STATE_CHAR_LEN;
    add_char_params.max_len = ESTC_GATT_LED_STATE_CHAR_LEN;
    add_char_params.char_props.write = 1;
    add_char_params.char_props.read = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.write_access = SEC_JUST_WORKS;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->led_state_char_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(led_notify_char_user_description);
    add_char_user_desc.size = strlen(led_notify_char_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) led_notify_char_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    memset(&char_pf, 0, sizeof(ble_gatts_char_pf_t));
    char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;

    add_char_params.uuid = ESTC_GATT_LED_NOTIFY_CHAR_UUID;
    add_char_params.uuid_type = ESTC_UUID_TYPE;
    add_char_params.init_len = LED_READ_LEN - 1;
    add_char_params.max_len = LED_READ_LEN - 1;
    add_char_params.char_props.notify = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.cccd_write_access = SEC_JUST_WORKS;
    add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.p_presentation_format = &char_pf;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->led_notify_char_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    return NRF_SUCCESS;
}

static void estc_ble_service_led_save_init(void)
{
    fds_register(fds_events_handler);

    if (NRF_SUCCESS != fds_init())
    {
        pwm_set_duty_cycle(&my_pwm, pwm_channel_indicator, pwm_max_pct);
    }
}

static void estc_ble_service_pwm_hw_init(void)
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

void estc_ble_service_deps_init(void)
{
    estc_ble_service_pwm_hw_init();
    estc_ble_service_led_save_init();
}
