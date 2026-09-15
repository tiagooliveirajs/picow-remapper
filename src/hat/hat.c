#include "remapper/hat/hat.h"
#include <stddef.h>
typedef struct {remapper_hat_control_t control;uint8_t pin;} hat_pin_map_t;
static const hat_pin_map_t k_pin_map[]={{REMAPPER_HAT_JOY_UP,REMAPPER_HAT_PIN_JOY_UP},{REMAPPER_HAT_JOY_DOWN,REMAPPER_HAT_PIN_JOY_DOWN},{REMAPPER_HAT_JOY_LEFT,REMAPPER_HAT_PIN_JOY_LEFT},{REMAPPER_HAT_JOY_RIGHT,REMAPPER_HAT_PIN_JOY_RIGHT},{REMAPPER_HAT_JOY_PRESS,REMAPPER_HAT_PIN_JOY_PRESS},{REMAPPER_HAT_KEY_A,REMAPPER_HAT_PIN_KEY_A},{REMAPPER_HAT_KEY_B,REMAPPER_HAT_PIN_KEY_B},{REMAPPER_HAT_KEY_X,REMAPPER_HAT_PIN_KEY_X},{REMAPPER_HAT_KEY_Y,REMAPPER_HAT_PIN_KEY_Y}};
bool remapper_hat_control_for_pin(uint8_t pin,remapper_hat_control_t *control){if(control==NULL)return false;for(size_t i=0u;i<sizeof(k_pin_map)/sizeof(k_pin_map[0]);++i){if(k_pin_map[i].pin==pin){*control=k_pin_map[i].control;return true;}}return false;}
uint8_t remapper_hat_pin_for_control(remapper_hat_control_t control){for(size_t i=0u;i<sizeof(k_pin_map)/sizeof(k_pin_map[0]);++i)if(k_pin_map[i].control==control)return k_pin_map[i].pin;return UINT8_MAX;}
