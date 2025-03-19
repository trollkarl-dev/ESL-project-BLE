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

#define ESTC_GATT_CHAR_1_UUID 0xDBF3 /* Set LED color */
#define ESTC_GATT_CHAR_2_UUID 0xDBF4 /* Set LED state */
#define ESTC_GATT_CHAR_3_UUID 0xDBF5 /* Get LED color */
#define ESTC_GATT_CHAR_4_UUID 0xDBF6 /* Get LED state */

#define CHAR_1_DESCRIPTION "Three-byte characteristic for setting the LED color"
#define CHAR_2_DESCRIPTION "One-byte characteristic for setting the LED state"
#define CHAR_3_DESCRIPTION "Characteristic for reading the color of the LED"
#define CHAR_4_DESCRIPTION "Characteristic for reading the LED state"

#define CHAR_3_READ_TEMPLATE "LED color: (R: 0x%02X, G: 0x%02X, B: 0x%02X)"
#define CHAR_3_READ_LEN (strlen(CHAR_3_READ_TEMPLATE) - 6)

#define CHAR_4_READ_TEMPLATE "LED state: %3s"
#define CHAR_4_READ_LEN (strlen(CHAR_3_READ_TEMPLATE))

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
