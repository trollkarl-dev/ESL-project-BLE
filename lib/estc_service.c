#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ble_uuid128_t const m_base_uuid128 = { ESTC_BASE_UUID };

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service, void *ctx);

extern ble_estc_service_t m_estc_service; 

extern void prepare_char_3_value(char *outbuf, rgb_t color);
extern void prepare_char_4_value(char *outbuf, uint8_t state);
extern void on_char_1_write(const uint8_t *data, uint16_t len);
extern void on_char_2_write(const uint8_t *data, uint16_t len);

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx)
{
    const ble_gatts_evt_write_t * p_evt_write;

    switch (ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            p_evt_write = &ble_evt->evt.gatts_evt.params.write;

            if (p_evt_write->handle == m_estc_service.char_1_handles.value_handle)
            {
                on_char_1_write(p_evt_write->data, p_evt_write->len);
            }

            if (p_evt_write->handle == m_estc_service.char_2_handles.value_handle)
            {
                on_char_2_write(p_evt_write->data, p_evt_write->len);
            }

            break;
        
        default:
            break;
    }
}

void estc_update_characteristic_1_value(ble_estc_service_t *service, int32_t *value)
{

}

ret_code_t estc_ble_service_init(ble_estc_service_t *service, void *ctx)
{
    ret_code_t error_code;
    ble_uuid_t service_uuid = { .uuid = ESTC_SERVICE_UUID,
                                .type = BLE_UUID_TYPE_BLE };

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

    led_params_t *led_params = (led_params_t *) ctx;

    ret_code_t error_code;
    
    const char char_1_user_description[] = CHAR_1_DESCRIPTION;
    const char char_2_user_description[] = CHAR_2_DESCRIPTION;
    const char char_3_user_description[] = CHAR_3_DESCRIPTION;
    const char char_4_user_description[] = CHAR_4_DESCRIPTION;
    const char char_5_user_description[] = CHAR_5_DESCRIPTION;
    const char char_6_user_description[] = CHAR_6_DESCRIPTION;

    static char strbuf_char_3[CHAR_3_READ_LEN + 1];
    static char strbuf_char_4[CHAR_4_READ_LEN + 1];

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_1_user_description);
    add_char_user_desc.size = strlen(char_1_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_1_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    add_char_params.uuid = ESTC_GATT_CHAR_1_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = sizeof(uint8_t) * 3;
    add_char_params.max_len = sizeof(uint8_t) * 3;
    add_char_params.char_props.write = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_1_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_2_user_description);
    add_char_user_desc.size = strlen(char_2_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_2_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    add_char_params.uuid = ESTC_GATT_CHAR_2_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = sizeof(uint8_t);
    add_char_params.max_len = sizeof(uint8_t);
    add_char_params.char_props.write = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_2_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_3_user_description);
    add_char_user_desc.size = strlen(char_3_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_3_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    memset(&char_pf, 0, sizeof(ble_gatts_char_pf_t));
    char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;

    prepare_char_3_value(strbuf_char_3, led_params->color);

    add_char_params.uuid = ESTC_GATT_CHAR_3_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = CHAR_3_READ_LEN - 1;
    add_char_params.max_len = CHAR_3_READ_LEN - 1;
    add_char_params.char_props.read = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.p_presentation_format = &char_pf;
    add_char_params.p_init_value = (uint8_t *) strbuf_char_3;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_3_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_4_user_description);
    add_char_user_desc.size = strlen(char_4_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_4_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    memset(&char_pf, 0, sizeof(ble_gatts_char_pf_t));
    char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;

    prepare_char_4_value(strbuf_char_4, led_params->state);

    add_char_params.uuid = ESTC_GATT_CHAR_4_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = CHAR_4_READ_LEN - 1;
    add_char_params.max_len = CHAR_4_READ_LEN - 1;
    add_char_params.char_props.read = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.p_presentation_format = &char_pf;
    add_char_params.p_init_value = (uint8_t *) strbuf_char_4;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_4_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_5_user_description);
    add_char_user_desc.size = strlen(char_5_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_5_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    memset(&char_pf, 0, sizeof(ble_gatts_char_pf_t));
    char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;

    add_char_params.uuid = ESTC_GATT_CHAR_5_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = CHAR_3_READ_LEN - 1;
    add_char_params.max_len = CHAR_3_READ_LEN - 1;
    add_char_params.char_props.notify = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.p_presentation_format = &char_pf;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_5_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_6_user_description);
    add_char_user_desc.size = strlen(char_6_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_6_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    memset(&char_pf, 0, sizeof(ble_gatts_char_pf_t));
    char_pf.format = BLE_GATT_CPF_FORMAT_UTF8S;

    add_char_params.uuid = ESTC_GATT_CHAR_6_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = CHAR_4_READ_LEN - 1;
    add_char_params.max_len = CHAR_4_READ_LEN - 1;
    add_char_params.char_props.notify = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;
    add_char_params.p_presentation_format = &char_pf;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_6_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }
    return NRF_SUCCESS;
}
