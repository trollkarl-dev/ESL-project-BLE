#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

#include "led_common.h"

/* UUID: 0f9cxxxx-c952-426b-950e-2f1cb01a1885 */
#define ESTC_BASE_UUID { 0x85, 0x18, 0x1A, 0xB0, \
                         0x1C, 0x2F, 0x0E, 0x95, \
                         0x6B, 0x42, 0x52, 0xC9, \
                         0x00, 0x00, 0x9C, 0x0F }

#define ESTC_SERVICE_UUID 0xDBF2

#define ESTC_UUID_TYPE BLE_UUID_TYPE_VENDOR_BEGIN

#define ESTC_GATT_LED_COLOR_WR_CHAR_UUID 0xDBF3 /* Set LED color */
#define ESTC_GATT_LED_STATE_WR_CHAR_UUID 0xDBF4 /* Set LED state */
#define ESTC_GATT_LED_COLOR_RD_CHAR_UUID 0xDBF5 /* Get LED color */
#define ESTC_GATT_LED_STATE_RD_CHAR_UUID 0xDBF6 /* Get LED state */
#define ESTC_GATT_LED_COLOR_NOTIFY_CHAR_UUID 0xDBF7 /* Notify LED color */
#define ESTC_GATT_LED_STATE_NOTIFY_CHAR_UUID 0xDBF8 /* Notify LED state */

#define LED_COLOR_WR_CHAR_DESCRIPTION "Three-byte characteristic for setting the LED color. "\
                                      "Send three bytes corresponding "\
                                      "to the color components in 24-bit RGB format. "\
                                      "All colors from #000000 to #FFFFFF are valid."

#define LED_STATE_WR_CHAR_DESCRIPTION "One-byte characteristic for setting the LED state. "\
                                      "Send zero to turn off the LED. "\
                                      "Send any non-zero number to turn on the LED."

#define LED_COLOR_RD_CHAR_DESCRIPTION "Characteristic for reading the LED color"

#define LED_STATE_RD_CHAR_DESCRIPTION "Characteristic for reading the LED state"

#define LED_COLOR_NOTIFY_CHAR_DESCRIPTION "Characteristic for notifying the LED color"

#define LED_STATE_NOTIFY_CHAR_DESCRIPTION "Characteristic for notifying the LED state"

#define LED_COLOR_READ_TEMPLATE "LED color: #%02X%02X%02X"
#define LED_COLOR_READ_LEN (sizeof(LED_COLOR_READ_TEMPLATE) - 6)

#define LED_STATE_READ_TEMPLATE "LED state: %3s"
#define LED_STATE_READ_LEN (sizeof(LED_STATE_READ_TEMPLATE))

typedef struct
{
    uint16_t service_handle;
    uint16_t connection_handle;

    ble_gatts_char_handles_t led_color_wr_char_handles;
    ble_gatts_char_handles_t led_state_wr_char_handles;
    ble_gatts_char_handles_t led_color_rd_char_handles;
    ble_gatts_char_handles_t led_state_rd_char_handles;
    ble_gatts_char_handles_t led_color_notify_char_handles;
    ble_gatts_char_handles_t led_state_notify_char_handles;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service, void *ctx);
void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);

#endif /* ESTC_SERVICE_H__ */
