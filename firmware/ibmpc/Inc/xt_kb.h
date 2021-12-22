#ifndef __XT_KB_H
#define __XT_KB_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

void xtkb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin);
void xtkb_check_for_softreset(void);
uint8_t xtkb_press_key(uint8_t code, uint8_t status);
void xtkb_reset_bus(void);
void xtkb_release_lines(void);
void xtkb_enable(void);

#ifdef __cplusplus
}
#endif

#endif


