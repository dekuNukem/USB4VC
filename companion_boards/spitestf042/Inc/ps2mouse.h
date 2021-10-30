#ifndef __PS2MOUSE_H
#define __PS2MOUSE_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define PS2_BUS_IDLE 3
#define PS2_BUS_INHIBIT 2
#define PS2_BUS_REQ_TO_SEND 1
#define PS2_BUS_UNKNOWN 0

void ps2mouse_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin);
uint8_t ps2mouse_get_bus_status(void);
uint8_t ps2mouse_read(uint8_t* result, uint8_t timeout_ms);
uint8_t ps2mouse_write(uint8_t data, uint8_t delay_start, uint8_t timeout_ms);
void ps2mouse_host_req_reply(uint8_t cmd);
uint8_t ps2mouse_send_update(mouse_event* this_event);

#ifdef __cplusplus
}
#endif

#endif


