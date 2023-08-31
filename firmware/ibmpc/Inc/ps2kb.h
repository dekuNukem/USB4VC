#ifndef __PS2KB_H
#define __PS2KB_H

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
void ps2kb_reset(void);

#define LINUX_KEYCODE_SYSRQ 99
#define LINUX_KEYCODE_PAUSE 119

#define LINUX_KEYCODE_ENTER 0x1C
#define LINUX_KEYCODE_CTRL 0x1D
#define LINUX_KEYCODE_SLASH 0x35
#define LINUX_KEYCODE_ALT 0x38
#define LINUX_KEYCODE_KP7 71
#define LINUX_KEYCODE_KP8 72
#define LINUX_KEYCODE_KP9 73
#define LINUX_KEYCODE_KP4 75
#define LINUX_KEYCODE_KP5 76
#define LINUX_KEYCODE_KP6 77
#define LINUX_KEYCODE_KP1 79
#define LINUX_KEYCODE_KP2 80
#define LINUX_KEYCODE_KP3 81
#define LINUX_KEYCODE_KP0 82
#define LINUX_KEYCODE_KPDOT 83

#define LINUX_KEYCODE_F11 87
#define LINUX_KEYCODE_F12 88

#define LINUX_KEYCODE_KPENTER 96
#define LINUX_KEYCODE_RIGHTCTRL 97
#define LINUX_KEYCODE_KPSLASH 98
#define LINUX_KEYCODE_RIGHTALT 100
#define LINUX_KEYCODE_LINEFEED 101
#define LINUX_KEYCODE_HOME 102
#define LINUX_KEYCODE_UP 103
#define LINUX_KEYCODE_PAGEUP 104
#define LINUX_KEYCODE_LEFT 105
#define LINUX_KEYCODE_RIGHT 106
#define LINUX_KEYCODE_END 107
#define LINUX_KEYCODE_DOWN 108
#define LINUX_KEYCODE_PAGEDOWN 109
#define LINUX_KEYCODE_INSERT 110
#define LINUX_KEYCODE_DELETE 111

#define PS2_OK 0
#define PS2_ERROR_SCAN_DISABLED 1
#define PS2_ERROR_UNUSED_CODE 2
#define PS2_ERROR_UNKNOWN 3
#define PS2_ERROR_TIMEOUT 4
#define PS2_ERROR_UNKNOWN_EV 5
#define PS2_ERROR_UNKNOWN_SCANCODE 6
#define PS2_ERROR_UNKNOWN_CODE_SET 7
#define PS2_ERROR_HOST_INHIBIT 8

#ifdef __cplusplus
}
#endif

#endif


