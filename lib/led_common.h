#ifndef LED_COMMON_H
#define LED_COMMON_H

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct {
    rgb_t color;
    uint8_t state;
} led_params_t;

#endif /* LED_COMMON_H */
