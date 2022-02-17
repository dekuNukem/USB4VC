#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "quad_encoder.h"
#include "shared.h"

const uint8_t grey_code_lookup[4] = {0, 1, 3, 2};

void quad_write(quad_output *qo)
{
  // printf("%d\n", qo->current_index);
  uint8_t current_code = grey_code_lookup[qo->current_index];
  switch(current_code)
  {
      case 0:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_RESET);
        break;
      case 1:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_SET);
        break;
      case 2:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_RESET);
        break;
      case 3:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_SET);
        break;
      default:
        break;
  }
}

void quad_reset(quad_output *qo)
{
  qo->current_index = 0;
  quad_write(qo);
}

void quad_init(quad_output *qo, GPIO_TypeDef* this_A_port, uint16_t this_A_pin, GPIO_TypeDef* this_B_port, uint16_t this_B_pin)
{
  qo->A_port = this_A_port;
  qo->A_pin = this_A_pin;
  qo->B_port = this_B_port;
  qo->B_pin = this_B_pin;
  quad_reset(qo);
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