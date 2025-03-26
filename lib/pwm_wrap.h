#ifndef PWM_WRAP_H_
#define PWM_WRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nrfx_pwm.h"

enum { pwm_max_pct = 100 };

typedef struct {
    nrfx_pwm_t *pwm;
    nrf_pwm_values_individual_t *seq_values;
    nrf_pwm_sequence_t const * seq;
} pwm_wrapper_t;

bool pwm_init(pwm_wrapper_t *pwm,
                     uint8_t const *channels,
                     uint16_t pwm_top_value,
                     bool invert);

void pwm_start(pwm_wrapper_t *pwm);

void pwm_stop(pwm_wrapper_t *pwm);

void pwm_set_duty_cycle(pwm_wrapper_t *pwm,
                        uint8_t channel,
                        uint32_t duty_cycle);

#ifdef __cplusplus
}
#endif

#endif /* PWM_WRAP_H_ */
