#ifndef __ADB_H
#define __ADB_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define ADB_LINE_STATUS_ATTEN 1
#define ADB_LINE_STATUS_IDLE 2
#define ADB_LINE_STATUS_RESET 3
#define ADB_LINE_STATUS_BUSY 4
#define ADB_LINE_STATUS_ERROR 5
#define ADB_LINE_STATUS_COLLISION 6

#define ADB_TIMEOUT 255
#define ADB_OK 0
#define ADB_KB_POLL 2
#define ADB_KB_POLL_REG2 3
#define ADB_KB_CHANGE_LED 4
#define ADB_MOUSE_POLL 5
#define ADB_ERROR ADB_LINE_STATUS_ERROR

#define ADB_KB_DEFAULT_ADDR 2
#define ADB_MOUSE_DEFAULT_ADDR 3

#define ADB_CMD_TYPE_FLUSH 0
#define ADB_CMD_TYPE_LISTEN 2
#define ADB_CMD_TYPE_TALK 3

#define ADB_CHANGE_ADDR 0xFE

#define DEBUG0_HI() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET)
#define DEBUG0_LOW() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET)

#define DEBUG1_HI() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_SET)
#define DEBUG1_LOW() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_RESET)

#define KEY_DELETE 111
#define KEY_CAPSLOCK 58
#define KEY_LEFTCTRL 29
#define KEY_LEFTSHIFT 42
#define KEY_LEFTALT 56
#define KEY_LEFTMETA 125
#define KEY_NUMLOCK 69
#define KEY_SCROLLLOCK 70

#define KEY_RIGHTCTRL 97
#define KEY_RIGHTSHIFT 54
#define KEY_RIGHTALT 100
#define KEY_RIGHTMETA 126

#define ADB_CLK_35 34
#define ADB_CLK_65 64
#define EV_TO_ADB_LOOKUP_SIZE 186
#define ADB_DEFAULT_TIMEOUT_US 25000
#define ADB_KEY_UNKNOWN 255
#define ADB_KEY_CAPSLOCK 57
void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin);
uint8_t adb_recv_cmd(uint8_t* data);
uint8_t parse_adb_cmd(uint8_t data);
void adb_reset(void);
void adb_release_lines(void);
uint8_t adb_send_response_16b(uint16_t data);
void send_srq(void);
int32_t wait_until_change(int32_t timeout_us);

extern uint8_t adb_mouse_current_addr, adb_kb_current_addr, adb_rw_in_progress;
extern const uint8_t linux_ev_to_adb_lookup[EV_TO_ADB_LOOKUP_SIZE];
extern uint16_t adb_kb_reg2;
extern uint8_t kb_enabled, mouse_enabled;
extern volatile uint32_t next_busy_off;

#define PCARD_BUSY_HI() HAL_GPIO_WritePin(BUSY_GPIO_Port, BUSY_Pin, GPIO_PIN_SET)
#define PCARD_BUSY_LOW() HAL_GPIO_WritePin(BUSY_GPIO_Port, BUSY_Pin, GPIO_PIN_RESET)

#ifdef __cplusplus
}
#endif

#endif


