#ifndef MY_BOARD_H
#define MY_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "nrf_gpio.h"

enum {
    my_led_first = 0,
    my_led_last = 3,
    my_led_amount = my_led_last - my_led_first + 1
};

enum { my_led_active_level = 0 }; /* must be 0 or 1 */

enum {
    my_btn_first = 0,
    my_btn_last = 0,
    my_btn_amount = my_btn_last - my_btn_first + 1
};

enum { my_btn_active_level = 0 }; /* must be 0 or 1 */

static const int my_led_mappings[] = {
    NRF_GPIO_PIN_MAP(0,  6),
    NRF_GPIO_PIN_MAP(0,  8),
    NRF_GPIO_PIN_MAP(1,  9),
    NRF_GPIO_PIN_MAP(0, 12),
};

static const int my_btn_mappings[] = {
    NRF_GPIO_PIN_MAP(1,  6)
};

void my_led_init(int idx);
void my_led_on(int idx);
void my_led_off(int idx);
void my_led_invert(int idx);

void my_btn_init(int idx);
bool my_btn_is_pressed(int idx);

void my_board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MY_BOARD_H */
