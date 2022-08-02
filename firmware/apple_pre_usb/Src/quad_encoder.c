#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "quad_encoder.h"
#include "shared.h"
#include "helpers.h"

quad_output quad_x;
quad_output quad_y;

TIM_HandleTypeDef* avg_timer;
TIM_HandleTypeDef* arr_timer_x;
TIM_HandleTypeDef* arr_timer_y;

mouse_buf* mouse_buffer;

#define ARR_LOOKUP_SIZE 64
const uint16_t arr_lookup[ARR_LOOKUP_SIZE] = {
12217, 10241, 8829, 7700, 6758, 5958, 5347, 4782, 4311, 3888, 3558, 3276, 3041, 2852, 2711, 2570, 2429, 2335, 2241, 2147, 2052, 1958, 1911, 1817, 1770, 1723, 1629, 1582, 1535, 1488, 1441, 1394, 1347, 1300, 1252, 1205, 1158, 1158, 1111, 1064, 1017, 1017, 970, 923, 923, 876, 876, 829, 829, 782, 782, 735, 735, 688, 688, 641, 641, 641, 594, 594, 594, 547, 547, 547
};
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
  // if (sum > 0 && sum < AVG_BUF_SIZE)
  //   sum = AVG_BUF_SIZE;
  // else if (sum < 0 && abs(sum) < AVG_BUF_SIZE)
  //   sum = AVG_BUF_SIZE * -1;
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

void quad_init(mouse_buf* mbuf, TIM_HandleTypeDef* avg_tim, TIM_HandleTypeDef* arr_tim_x, TIM_HandleTypeDef* arr_tim_y, GPIO_TypeDef* x1_port, uint16_t x1_pin, GPIO_TypeDef* x2_port, uint16_t x2_pin, GPIO_TypeDef* y1_port, uint16_t y1_pin, GPIO_TypeDef* y2_port, uint16_t y2_pin)
{
  quad_x.A_port = x1_port;
  quad_x.A_pin = x1_pin;
  quad_x.B_port = x2_port;
  quad_x.B_pin = x2_pin;

  quad_y.A_port = y1_port;
  quad_y.A_pin = y1_pin;
  quad_y.B_port = y2_port;
  quad_y.B_pin = y2_pin;

  avg_timer = avg_tim;
  arr_timer_x = arr_tim_x;
  arr_timer_y = arr_tim_y;
  mouse_buffer = mbuf;
  quad_reset(&quad_x);
  HAL_TIM_Base_Start_IT(avg_timer);
  HAL_TIM_Base_Start_IT(arr_timer_x);
  HAL_TIM_Base_Start_IT(arr_timer_y);
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
  uint8_t cme_count;
  // every 10ms
  if(htim == avg_timer)
  {
    cme_count = get_consolidated_mouse_event(mouse_buffer, &consolidated_mouse_event);
    // mouse buffer now empty

    avg_buf_add(&quad_x, consolidated_mouse_event.movement_x);
    avg_buf_add(&quad_y, consolidated_mouse_event.movement_y);

    quad_x.avg_speed = get_buf_avg(&quad_x);
    quad_y.avg_speed = get_buf_avg(&quad_y);
    arr_timer_x->Instance->ARR = calc_arr(quad_x.avg_speed);
    arr_timer_y->Instance->ARR = calc_arr(quad_y.avg_speed);

    if(cme_count != 0)
    {
      if(consolidated_mouse_event.button_left || consolidated_mouse_event.button_right)
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
      else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    }
  }
  // every ARR overflow
  if(htim == arr_timer_x && quad_x.avg_speed != 0)
  {
    if(quad_x.avg_speed > 0)
      quad_increment(&quad_x);
    else
      quad_decrement(&quad_x);
  }
  if(htim == arr_timer_y && quad_y.avg_speed != 0)
  {
    if(quad_y.avg_speed > 0)
      quad_increment(&quad_y);
    else
      quad_decrement(&quad_y);
  }
}

// HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_15);

