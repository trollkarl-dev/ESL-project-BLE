#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

/* UUID: 0f9cxxxx-c952-426b-950e-2f1cb01a1885 */
#define ESTC_BASE_UUID { 0x85, 0x18, 0x1A, 0xB0, \
                         0x1C, 0x2F, 0x0E, 0x95, \
                         0x6B, 0x42, 0x52, 0xC9, \
                         0x00, 0x00, 0x9C, 0x0F }

#define ESTC_SERVICE_UUID 0xDBF2

#define ESTC_GATT_CHAR_1_UUID 0xDBF3 /* read and write single-byte */
#define ESTC_GATT_CHAR_2_UUID 0xDBF4 /* read-only two-bytes */
#define ESTC_GATT_CHAR_3_UUID 0xDBF5 /* read-only four-bytes with notify capability */
#define ESTC_GATT_CHAR_4_UUID 0xDBF6 /* read-only four-bytes with indicate capability */

#define CHAR_1_LABEL "\e[32mCharacteristic 1\e[0m"
#define CHAR_2_LABEL "\e[33mCharacteristic 2\e[0m"
#define CHAR_3_LABEL "\e[34mCharacteristic 3\e[0m"
#define CHAR_4_LABEL "\e[35mCharacteristic 4\e[0m"

#define CHAR_1_DESCRIPTION "Test single-byte "\
                           "custom characteristic "\
                           "with read and write capabilities"
#define CHAR_2_DESCRIPTION "Test two-bytes "\
                           "custom characteristic "\
                           "with read capability"
#define CHAR_3_DESCRIPTION "Test four-bytes "\
                           "custom characteristic "\
                           "with notify capability"
#define CHAR_4_DESCRIPTION "Test four-bytes "\
                           "custom characteristic "\
                           "with indicate capability"

#define NOTIFY_CAPTION "\e[36mNotify\e[0m"
#define INDICATE_CAPTION "\e[36mIndicate\e[0m"

typedef struct
{
    uint16_t service_handle;
    uint16_t connection_handle;
    ble_gatts_char_handles_t char_1_handles;
    ble_gatts_char_handles_t char_2_handles;
    ble_gatts_char_handles_t char_3_handles;
    ble_gatts_char_handles_t char_4_handles;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service);
void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);
void estc_update_characteristic_1_value(ble_estc_service_t *service, int32_t *value);

#endif /* ESTC_SERVICE_H__ */
