#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ble_uuid128_t const m_base_uuid128 = { ESTC_BASE_UUID };

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);

extern ble_estc_service_t m_estc_service; 

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

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
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

    return estc_ble_add_characteristics(service);
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ble_add_char_params_t add_char_params;
    ble_add_char_user_desc_t add_char_user_desc;

    ret_code_t error_code;
    
    const char char_1_user_description[] = CHAR_1_DESCRIPTION;
    const char char_2_user_description[] = CHAR_2_DESCRIPTION;

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
    add_char_params.char_props.read = 1;
    add_char_params.char_props.write = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.read_access = SEC_OPEN;
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
    add_char_params.char_props.read = 1;
    add_char_params.char_props.write = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.read_access = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_2_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

/*
    memset(&add_char_user_desc, 0, sizeof(ble_add_char_user_desc_t));

    add_char_user_desc.max_size = strlen(char_3_user_description);
    add_char_user_desc.size = strlen(char_3_user_description);
    add_char_user_desc.p_char_user_desc = (uint8_t *) char_3_user_description;
    add_char_user_desc.is_value_user = false;
    add_char_user_desc.is_var_len = false;
    add_char_user_desc.char_props.read = 1;
    add_char_user_desc.read_access = SEC_OPEN;

    memset(&add_char_params, 0, sizeof(ble_add_char_params_t));

    add_char_params.uuid = ESTC_GATT_CHAR_3_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = sizeof(uint16_t);
    add_char_params.max_len = sizeof(uint16_t);
    add_char_params.char_props.notify = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

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

    add_char_params.uuid = ESTC_GATT_CHAR_4_UUID;
    add_char_params.uuid_type = BLE_UUID_TYPE_BLE;
    add_char_params.init_len = sizeof(int16_t);
    add_char_params.max_len = sizeof(int16_t);
    add_char_params.char_props.indicate = 1;
    add_char_params.is_var_len = false;
    add_char_params.is_value_user = false;
    add_char_params.cccd_write_access = SEC_OPEN;
    add_char_params.p_user_descr = &add_char_user_desc;

    error_code = characteristic_add(service->service_handle,
                                    &add_char_params,
                                    &service->char_4_handles);

    if (error_code != NRF_SUCCESS)
    {
        return error_code;
    }

*/
    return NRF_SUCCESS;
}
