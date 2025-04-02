#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

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

    void (*schedule_click_check)(uint32_t);
    void (*schedule_hold_check)(uint32_t);
    void (*schedule_debounce_check)(uint32_t);
} button_callbacks_t;

typedef struct {
    bool pressed_flag;
    uint8_t clicks_cnt;

    button_timings_t timings;
    button_callbacks_t callbacks;
} button_t;

void button_init(button_t *b,
                 button_timings_t const * timings,
                 button_callbacks_t const * callbacks);

void button_click_check(button_t *b);
void button_hold_check(button_t *b);
void button_debounce_check(button_t *b);
void button_first_run(button_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
