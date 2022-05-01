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

#define XTKB_OK 0
#define XTKB_ERROR_SCAN_DISABLED 1
#define XTKB_ERROR_UNKNOWN_CODE_SET 9
#define XTKB_ERROR_HOST_INHIBIT 69
#define XTKB_ERROR_UNUSED_CODE 2
#define XTKB_ERROR_UNKNOWN 3
#define XTKB_ERROR_TIMEOUT 4

#ifdef __cplusplus
}
#endif

#endif


