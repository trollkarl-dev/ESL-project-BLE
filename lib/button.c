#include "button.h"

static void button_click_check(button_t *b)
{
    if (!b->pressed_flag &&
        b->clicks_cnt != 0)
    {
        b->callbacks.onclick(b->clicks_cnt);
        b->clicks_cnt = 0;
    }
}   

static void button_hold_check(button_t *b)
{
    if (!(b->callbacks.is_pressed() && b->pressed_flag))
        return;

    b->clicks_cnt = 0;
    b->callbacks.onhold();
    app_timer_start(*(b->hold_timer_ptr), APP_TIMER_TICKS(b->timings.hold_pause_short_ms), (void *) b);
}

static void button_debounce_check(button_t *b)
{
    /* rising */
    if (b->callbacks.is_pressed() && !b->pressed_flag)
    {
        b->pressed_flag = true;
        b->clicks_cnt++;
        app_timer_start(*(b->hold_timer_ptr), APP_TIMER_TICKS(b->timings.hold_pause_long_ms), (void *) b);
    }

    /* falling */
    if (!b->callbacks.is_pressed() && b->pressed_flag)
    {
        b->pressed_flag = false;
        app_timer_start(*(b->click_timer_ptr), APP_TIMER_TICKS(b->timings.dblclk_pause_ms), (void *) b);
    }
}

static void button_debounce_timer_handler(void *ctx)
{
    button_debounce_check((button_t *) ctx);
}

static void button_click_timer_handler(void *ctx)
{
    button_click_check((button_t *) ctx);
}

static void button_hold_timer_handler(void *ctx)
{
    button_hold_check((button_t *) ctx);
}

void button_init(button_t *b,
                 button_timings_t const * timings,
                 button_callbacks_t const * callbacks)
{
    b->pressed_flag = false;
    b->clicks_cnt = 0;
    b->timings = *timings;
    b->callbacks = *callbacks;

    app_timer_create(b->debounce_timer_ptr, APP_TIMER_MODE_SINGLE_SHOT, button_debounce_timer_handler);
    app_timer_create(b->click_timer_ptr, APP_TIMER_MODE_SINGLE_SHOT, button_click_timer_handler);
    app_timer_create(b->hold_timer_ptr, APP_TIMER_MODE_SINGLE_SHOT, button_hold_timer_handler);
}

void button_first_run(button_t *b)
{
    app_timer_start(*(b->debounce_timer_ptr), APP_TIMER_TICKS(b->timings.debounce_period_ms), (void *) b);
}

