#ifndef __LISA_KB_H
#define __LISA_KB_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"
#include "helpers.h"

typedef struct
{
  GPIO_TypeDef* data_port;
  uint16_t data_pin;
  GPIO_TypeDef* pwr_port;
  uint16_t pwr_pin;
} lisa_handle;

#define LISA_DATA_HI() HAL_GPIO_WritePin(lh.data_port, lh.data_pin, GPIO_PIN_SET)
#define LISA_DATA_LOW() HAL_GPIO_WritePin(lh.data_port, lh.data_pin, GPIO_PIN_RESET)
#define LISA_READ_DATA_PIN() HAL_GPIO_ReadPin(lh.data_port, lh.data_pin)
#define LISA_TIMEOUT -1

extern lisa_handle lh;

void lisa_kb_update(void);
void lisa_buf_init(void);
void lisa_buf_add(uint8_t linux_keycode, uint8_t value);
void lisa_kb_reset(void);

#ifdef __cplusplus
}
#endif

#endif


