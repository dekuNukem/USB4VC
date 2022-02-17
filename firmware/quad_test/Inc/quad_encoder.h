#ifndef __QUAD_ENCODER_H
#define __QUAD_ENCODER_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

typedef struct
{
  GPIO_TypeDef* A_port;
  uint16_t A_pin;
  GPIO_TypeDef* B_port;
  uint16_t B_pin;
  uint8_t current_index;
} quad_output;

void quad_init(quad_output *qo, GPIO_TypeDef* this_A_port, uint16_t this_A_pin, GPIO_TypeDef* this_B_port, uint16_t this_B_pin);
void quad_increment(quad_output *qo);
void quad_decrement(quad_output *qo);
#ifdef __cplusplus
}
#endif

#endif


