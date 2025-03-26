#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ble_uuid128_t const m_base_uuid128 = { ESTC_BASE_UUID };

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, void *ctx);

extern ble_estc_service_t m_estc_service; 

extern void on_led_color_char_write(const uint8_t *data, uint16_t len, bool on_connected);
extern void on_led_state_char_write(const uint8_t *data, uint16_t len, bool on_connected);

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
        
        default:
            break;
    }
}

ret_code_t estc_ble_service_init(ble_estc_service_t *service, void *ctx)
{
    ret_code_t error_code;
    ble_uuid_t service_uuid = { .uuid = ESTC_SERVICE_UUID,
                                .type = ESTC_UUID_TYPE };

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
