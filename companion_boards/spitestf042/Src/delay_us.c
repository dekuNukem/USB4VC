#include "stm32f0xx_hal.h"
#include "delay_us.h"

TIM_HandleTypeDef* us_timer;

// initializes timer pointer and start the interrupt
void delay_us_init(TIM_HandleTypeDef* htim_base)
{
	us_timer = htim_base;
	HAL_TIM_Base_Start_IT(us_timer);
}

// combine the upper 16 bit stored in upper16
// with the lower 16 bit in the counter
// to return a 32-bit microsecond timestamp
uint32_t micros(void)
{
  return us_timer->Instance->CNT;
}

void delay_us(uint32_t delay)
{
  uint32_t end_time = micros() + delay;
  while(micros() < end_time);
}

