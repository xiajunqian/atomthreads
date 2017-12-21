#ifndef __BSP_H__
#define __BSP_H__

#include "stm8s.h"






void bsp_init();


#define BSP_ID_LED0     0
void bsp_led_toggle(uint8_t id);
void bsp_led_set(uint8_t id,uint8_t state);



#endif