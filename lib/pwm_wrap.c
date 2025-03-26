#include "pwm_wrap.h"

bool pwm_init(pwm_wrapper_t *pwm,
              uint8_t const *channels,
              uint16_t pwm_top_value,
              bool invert)
{
    int i;

    nrfx_pwm_config_t my_pwm_config =
    {
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = NRF_PWM_CLK_1MHz,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = pwm_top_value,
        .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
        .step_mode    = NRF_PWM_STEP_AUTO,
    };

    for (i = 0; i < NRF_PWM_CHANNEL_COUNT; i++)
    {
        my_pwm_config.output_pins[i] = channels[i];
        if (invert)
        {
            my_pwm_config.output_pins[i] |= NRFX_PWM_PIN_INVERTED;
        }
    }

    return (nrfx_pwm_init(pwm->pwm,
                          &my_pwm_config,
                          NULL) == NRF_SUCCESS);
}

void pwm_start(pwm_wrapper_t *pwm)
{
    if (nrfx_pwm_is_stopped(pwm->pwm))
    {
        nrfx_pwm_simple_playback(pwm->pwm, pwm->seq, 1, NRFX_PWM_FLAG_LOOP);
    }
}

void pwm_stop(pwm_wrapper_t *pwm)
{
    if (!nrfx_pwm_is_stopped(pwm->pwm))
    {
        nrfx_pwm_stop(pwm->pwm, false);
    }
}

void pwm_set_duty_cycle(pwm_wrapper_t *pwm,
                        uint8_t channel,
                        uint32_t duty_cycle)
{
    duty_cycle %= 1 + pwm_max_pct;

    switch (channel)
    {
        case 0:
            pwm->seq_values->channel_0 = duty_cycle;
            break;

        case 1:
            pwm->seq_values->channel_1 = duty_cycle;
            break;

        case 2:
            pwm->seq_values->channel_2 = duty_cycle;
            break;

        case 3:
            pwm->seq_values->channel_3 = duty_cycle;
            break;

        default:
            break;
    }
}

