#ifndef __ADB_H
#define __ADB_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define PS2_BUS_IDLE 3
#define PS2_BUS_INHIBIT 2
#define PS2_BUS_REQ_TO_SEND 1
#define PS2_BUS_UNKNOWN 0

void ps2kb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin);
uint8_t ps2kb_get_bus_status(void);
uint8_t ps2kb_read(uint8_t* result, uint8_t timeout_ms);
uint8_t ps2kb_write(uint8_t data, uint8_t delay_start, uint8_t timeout_ms);
void keyboard_reply(uint8_t cmd, uint8_t *leds);
uint8_t ps2kb_press_key(uint8_t linux_keycode, uint8_t press1release0);
void ps2kb_release_lines(void);

#define LINUX_KEYCODE_TO_PS2_SCANCODE_SINGLE_SIZE 89
#define LINUX_KEYCODE_TO_PS2_SCANCODE_SPECIAL_SIZE 32

#define LINUX_KEYCODE_SYSRQ 99
#define LINUX_KEYCODE_PAUSE 119

extern const uint8_t linux_keycode_to_ps2_scancode_lookup_single_byte_codeset2[LINUX_KEYCODE_TO_PS2_SCANCODE_SINGLE_SIZE];


#ifdef __cplusplus
}
#endif

#endif


