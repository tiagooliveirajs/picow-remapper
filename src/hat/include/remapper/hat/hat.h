#ifndef REMAPPER_HAT_HAT_H
#define REMAPPER_HAT_HAT_H

#include <stdbool.h>
#include <stdint.h>

#define REMAPPER_HAT_PIN_JOY_UP 2u
#define REMAPPER_HAT_PIN_JOY_PRESS 3u
#define REMAPPER_HAT_PIN_KEY_A 15u
#define REMAPPER_HAT_PIN_JOY_LEFT 16u
#define REMAPPER_HAT_PIN_KEY_B 17u
#define REMAPPER_HAT_PIN_JOY_DOWN 18u
#define REMAPPER_HAT_PIN_KEY_X 19u
#define REMAPPER_HAT_PIN_JOY_RIGHT 20u
#define REMAPPER_HAT_PIN_KEY_Y 21u

typedef enum {REMAPPER_HAT_JOY_UP=0,REMAPPER_HAT_JOY_DOWN,REMAPPER_HAT_JOY_LEFT,REMAPPER_HAT_JOY_RIGHT,REMAPPER_HAT_JOY_PRESS,REMAPPER_HAT_KEY_A,REMAPPER_HAT_KEY_B,REMAPPER_HAT_KEY_X,REMAPPER_HAT_KEY_Y,REMAPPER_HAT_CONTROL_COUNT} remapper_hat_control_t;
typedef struct {remapper_hat_control_t control;bool pressed;} remapper_hat_event_t;

bool remapper_hat_control_for_pin(uint8_t pin, remapper_hat_control_t *control);
uint8_t remapper_hat_pin_for_control(remapper_hat_control_t control);
void remapper_hat_pico_init(void);
void remapper_hat_pico_task(void);
bool remapper_hat_pico_poll_event(remapper_hat_event_t *event);

#endif
