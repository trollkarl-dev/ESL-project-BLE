/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ble_uuid128_t const m_base_uuid128 = { ESTC_BASE_UUID };

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service);

extern ble_estc_service_t m_estc_service; 

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx)
{
    const ble_gatts_evt_write_t * p_evt_write;

    switch (ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            p_evt_write = &ble_evt->evt.gatts_evt.params.write;
            if (p_evt_write->handle == m_estc_service.char_1_handles.value_handle)
            {
                NRF_LOG_INFO("\e[32mCharacteristic 1\e[0m value is %d (0x%02X)",
                             p_evt_write->data[0],
                             p_evt_write->data[0]);
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
    ret_code_t error_code = NRF_SUCCESS;
    ble_uuid_t ble_uuid = { .uuid = ESTC_GATT_CHAR_1_UUID,
                            .type = BLE_UUID_TYPE_BLE };

    ble_gatts_char_md_t char_md = { 0 };

    char_md.char_props.read = 1;
    char_md.char_props.write = 1;

    /*
     * Configures attribute metadata
     * For now we only specify that the attribute will be stored in the softdevice
     * Set read/write security levels to our attribute metadata
     */
    ble_gatts_attr_md_t attr_md = { 0 };

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    /* Configure the characteristic value attribute */
    ble_gatts_attr_t attr_char_value = { 0 };

    attr_char_value.p_uuid = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
    attr_char_value.max_len = sizeof(uint8_t);

    /* Add new characteristic to the service */
    error_code = sd_ble_gatts_characteristic_add(service->service_handle,
                                                 &char_md,
                                                 &attr_char_value,
                                                 &service->char_1_handles);

    return error_code;
}
