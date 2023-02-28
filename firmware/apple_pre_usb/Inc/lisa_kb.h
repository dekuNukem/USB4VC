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

extern lisa_handle lh;

void lisa_kb_update(void);
void lisa_buf_reset(void);

#ifdef __cplusplus
}
#endif

#endif


