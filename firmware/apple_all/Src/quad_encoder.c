#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "quad_encoder.h"
#include "shared.h"


quad_output quad_x;
quad_output quad_y;

TIM_HandleTypeDef* avg_timer;
TIM_HandleTypeDef* arr_timer;

int32_t avg_speed;
#define ARR_LOOKUP_SIZE 40
const uint16_t arr_lookup[ARR_LOOKUP_SIZE] = {500, 12500, 10245, 8926, 7990, 7264, 6671, 6170, 5735, 5352, 5010, 4699, 4416, 4156, 3915, 3691, 3481, 3283, 3098, 2922, 2755, 2596, 2445, 2300, 2162, 2029, 1901, 1779, 1660, 1546, 1436, 1329, 1226, 1126, 1029, 934, 843, 754, 667, 582};
const uint8_t grey_code_lookup[4] = {0, 1, 3, 2};

void quad_write(quad_output *qo)
{
  uint8_t current_code = grey_code_lookup[qo->current_index];
  if(current_code & 0x1)
    HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_RESET);
  if(current_code & 0x2)
    HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_RESET);
}

void quad_reset(quad_output *qo)
{
  qo->current_index = 0;
  quad_write(qo);
}

// void quad_init(quad_output *qo, GPIO_TypeDef* this_A_port, uint16_t this_A_pin, GPIO_TypeDef* this_B_port, uint16_t this_B_pin)
// {
//   qo->A_port = this_A_port;
//   qo->A_pin = this_A_pin;
//   qo->B_port = this_B_port;
//   qo->B_pin = this_B_pin;
//   quad_reset(qo);
// }

void quad_init(TIM_HandleTypeDef* avg_tim, TIM_HandleTypeDef* arr_tim, GPIO_TypeDef* x1_port, uint16_t x1_pin, GPIO_TypeDef* x2_port, uint16_t x2_pin)//, GPIO_TypeDef* y1_port, uint16_t y1_pin, GPIO_TypeDef* y2_port, uint16_t y2_pin)
{
  quad_x->A_port = x1_port;
  quad_x->A_pin = x1_pin;
  quad_x->B_port = x2_port;
  quad_x->B_pin = x2_pin;
  avg_timer = avg_tim;
  arr_timer = arr_tim;
  quad_reset(qo);
  HAL_TIM_Base_Start_IT(avg_timer);
  HAL_TIM_Base_Start_IT(arr_timer);
}

void quad_increment(quad_output *qo)
{
  qo->current_index = (uint8_t)(qo->current_index + 1) % 4;
  quad_write(qo);
}

void quad_decrement(quad_output *qo)
{
  qo->current_index = (uint8_t)(qo->current_index - 1) % 4;
  quad_write(qo);
}
