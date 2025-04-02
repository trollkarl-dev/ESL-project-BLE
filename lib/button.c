#include "button.h"

void button_init(button_t *b,
                 button_timings_t const * timings,
                 button_callbacks_t const * callbacks)
{
    b->pressed_flag = false;
    b->clicks_cnt = 0;
    b->timings = *timings;
    b->callbacks = *callbacks;
}

void button_click_check(button_t *b)
{
    if (!b->pressed_flag &&
        b->clicks_cnt != 0)
    {
        b->callbacks.onclick(b->clicks_cnt);
        b->clicks_cnt = 0;
    }
}   

void button_hold_check(button_t *b)
{
    if (!(b->callbacks.is_pressed() && b->pressed_flag))
        return;

    b->clicks_cnt = 0;
    b->callbacks.onhold();
    b->callbacks.schedule_hold_check(b->timings.hold_pause_short_ms);
}

void button_debounce_check(button_t *b)
{
    /* rising */
    if (b->callbacks.is_pressed() && !b->pressed_flag)
    {
        b->pressed_flag = true;
        b->clicks_cnt++;
        b->callbacks.schedule_hold_check(b->timings.hold_pause_long_ms);
    }

    /* falling */
    if (!b->callbacks.is_pressed() && b->pressed_flag)
    {
        b->pressed_flag = false;
        b->callbacks.schedule_click_check(b->timings.dblclk_pause_ms);
    }
}

void button_first_run(button_t *b)
{
    b->callbacks.schedule_debounce_check(b->timings.debounce_period_ms);
}

