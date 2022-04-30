#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "quad_encoder.h"
#include "shared.h"
#include "helpers.h"

quad_output quad_x;
quad_output quad_y;

TIM_HandleTypeDef* avg_timer;
TIM_HandleTypeDef* arr_timer;

mouse_buf* mouse_buffer;

#define ARR_LOOKUP_SIZE 40
const uint16_t arr_lookup[ARR_LOOKUP_SIZE] = {500, 12500, 10245, 8926, 7990, 7264, 6671, 6170, 5735, 5352, 5010, 4699, 4416, 4156, 3915, 3691, 3481, 3283, 3098, 2922, 2755, 2596, 2445, 2300, 2162, 2029, 1901, 1779, 1660, 1546, 1436, 1329, 1226, 1126, 1029, 934, 843, 754, 667, 582};
const uint8_t grey_code_lookup[4] = {0, 1, 3, 2};

/*
  instead of all at once, we remove data from buffer
  at a regular interval, say 5ms, if empty, then theres no movement
  then every 50ms for example the average movement is calculated
  and that is used to update quad encoder?
*/
void avg_buf_add(quad_output* qo, int16_t value)
{
  qo->avg_buf[qo->avg_buf_index] = value;
  qo->avg_buf_index++;
  if(qo->avg_buf_index >= AVG_BUF_SIZE)
    qo->avg_buf_index = 0;
}

int32_t get_buf_avg(quad_output* qo)
{
  int32_t sum = 0;
  for (int i = 0; i < AVG_BUF_SIZE; ++i)
    sum += qo->avg_buf[i];
  if (sum > 0 && sum < AVG_BUF_SIZE)
    sum = AVG_BUF_SIZE;
  else if (sum < 0 && abs(sum) < AVG_BUF_SIZE)
    sum = AVG_BUF_SIZE * -1;
  return (int32_t)(sum/AVG_BUF_SIZE);
}

/*
each speed has a corresponding duration before the next increment or decrement

make sure to enable autoreload preload to prevent glitches

ARR = Auto Reload Register
value = us
*/

uint16_t calc_arr(int32_t speed_val)
{
  speed_val = abs(speed_val);
  if(speed_val <= 0 || speed_val >= ARR_LOOKUP_SIZE)
    return 500;
  return arr_lookup[speed_val];
}

void quad_write(quad_output *qo)
{
  uint8_t current_code = grey_code_lookup[qo->gray_code_index];
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
  qo->gray_code_index = 0;
  qo->avg_buf_index = 0;
  qo->avg_speed = 0;
  memset(qo->avg_buf, 0, AVG_BUF_SIZE);
  quad_write(qo);
}

void quad_init(mouse_buf* mbuf, TIM_HandleTypeDef* avg_tim, TIM_HandleTypeDef* arr_tim, GPIO_TypeDef* x1_port, uint16_t x1_pin, GPIO_TypeDef* x2_port, uint16_t x2_pin)//, GPIO_TypeDef* y1_port, uint16_t y1_pin, GPIO_TypeDef* y2_port, uint16_t y2_pin)
{
  quad_x.A_port = x1_port;
  quad_x.A_pin = x1_pin;
  quad_x.B_port = x2_port;
  quad_x.B_pin = x2_pin;
  avg_timer = avg_tim;
  arr_timer = arr_tim;
  mouse_buffer = mbuf;
  quad_reset(&quad_x);
  HAL_TIM_Base_Start_IT(avg_timer);
  HAL_TIM_Base_Start_IT(arr_timer);
}

void quad_increment(quad_output *qo)
{
  qo->gray_code_index = (uint8_t)(qo->gray_code_index + 1) % 4;
  quad_write(qo);
}

void quad_decrement(quad_output *qo)
{
  qo->gray_code_index = (uint8_t)(qo->gray_code_index - 1) % 4;
  quad_write(qo);
}

/*
  this gets called every 10ms, fetches mouse event and put them into a running buffer
  a window average is calculated, used to adjust the timer autoreload register
*/


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // every 10ms
  if(htim == avg_timer)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);
    mouse_event* this_mouse_event = mouse_buf_peek(mouse_buffer);
    if(this_mouse_event == NULL)
    {
      avg_buf_add(&quad_x, 0);
    }
    else
    {
      avg_buf_add(&quad_x, this_mouse_event->movement_x);
      mouse_buf_pop(mouse_buffer);
    }
    quad_x.avg_speed = get_buf_avg(&quad_x);
    arr_timer->Instance->ARR = calc_arr(quad_x.avg_speed);
  }
  // every ARR overflow
  if(htim == arr_timer && quad_x.avg_speed != 0)
  {
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);
    if(quad_x.avg_speed > 0)
      quad_increment(&quad_x);
    else
      quad_decrement(&quad_x);
  }
}

