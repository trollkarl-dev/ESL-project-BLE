#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "app_timer.h"

typedef struct {
    uint16_t dblclk_pause_ms; 
    uint16_t hold_pause_short_ms; 
    uint16_t hold_pause_long_ms; 
    uint16_t debounce_period_ms;
} button_timings_t;

typedef struct {
    void (*onclick)(uint8_t);
    void (*onhold)(void);
    bool (*is_pressed)(void);
} button_callbacks_t;

typedef struct {
    bool pressed_flag;
    uint8_t clicks_cnt;

    button_timings_t timings;
    button_callbacks_t callbacks;

    app_timer_id_t * debounce_timer_ptr;
    app_timer_id_t * click_timer_ptr;
    app_timer_id_t * hold_timer_ptr;
} button_t;

#define BUTTON_DEF(BUTTON_NAME)\
APP_TIMER_DEF(CONCAT_2(BUTTON_NAME, _debounce_timer));\
APP_TIMER_DEF(CONCAT_2(BUTTON_NAME, _click_timer));\
APP_TIMER_DEF(CONCAT_2(BUTTON_NAME, _hold_timer));\
\
static volatile button_t BUTTON_NAME = \
{\
    false,\
    0,\
    (button_timings_t) {0, 0, 0, 0},\
    (button_callbacks_t) {NULL, NULL, NULL},\
    (app_timer_id_t * const) &CONCAT_2(BUTTON_NAME, _debounce_timer),\
    (app_timer_id_t * const) &CONCAT_2(BUTTON_NAME, _click_timer),\
    (app_timer_id_t * const) &CONCAT_2(BUTTON_NAME, _hold_timer)\
}

void button_init(button_t *b,
                 button_timings_t const * timings,
                 button_callbacks_t const * callbacks);

void button_first_run(button_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
