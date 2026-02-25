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

#define ADB_TIMEOUT -1
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

// Register 3 commands (lower 8 bits)
#define ADB_LISTEN3_SELF_TEST   0xFF  // Perform self-test
#define ADB_LISTEN3_CHANGE_ADDR 0xFE  // Change address only
#define ADB_LISTEN3_ACTIVATOR   0xFD  // Activator relocation
#define ADB_LISTEN3_SET_FLAGS   0x00  // Update address and flags

// Register 3 flags
#define ADB_REG3_SRQ_ENABLE 0x2000  // Bit 13: Service Request Enable

// #define DEBUG0_HI() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET)
// #define DEBUG0_LOW() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET)

// #define DEBUG1_HI() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_SET)
// #define DEBUG1_LOW() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_RESET)

#define DEBUG0_HI() {}
#define DEBUG0_LOW() {}

#define DEBUG1_HI() {}
#define DEBUG1_LOW() {}

#define IS_ADB_HOST_PRESENT() HAL_GPIO_ReadPin(ADB_DET_GPIO_Port, ADB_DET_Pin)

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

#define KEY_POWER 116

#define ADB_CLK_35 34
#define ADB_CLK_65 64
#define ADB_BUS_SETTLE_US 1
#define EV_TO_ADB_LOOKUP_SIZE 186
#define ADB_DEFAULT_TIMEOUT_US 25000
#define ADB_KEY_UNKNOWN 255
#define ADB_KEY_CAPSLOCK 57
#define ADB_KEY_POWER 127

// Left-side modifier scan codes (standard)
#define ADB_KEY_LEFT_CONTROL 54
#define ADB_KEY_LEFT_SHIFT 56
#define ADB_KEY_LEFT_OPTION 58

// Right-side modifier scan codes (extended, only when handler ID == 0x03)
#define ADB_KEY_RIGHT_SHIFT 123
#define ADB_KEY_RIGHT_OPTION 124
#define ADB_KEY_RIGHT_CONTROL 125

void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin);
uint8_t adb_recv_cmd(uint8_t* data);
uint8_t parse_adb_cmd(uint8_t data);
void adb_reset(void);
void adb_release(void);
uint8_t adb_send_response_16b(uint16_t data);
void send_srq(void);
int32_t adb_wait_until_change(int32_t timeout_us);
void adb_psw_assert(void);
void adb_psw_release(void);

extern uint8_t adb_mouse_current_addr, adb_kb_current_addr, adb_rw_in_progress;
extern const uint8_t linux_ev_to_adb_lookup[EV_TO_ADB_LOOKUP_SIZE];
extern uint16_t adb_kb_reg2;
extern uint8_t adb_kb_enabled, adb_mouse_enabled;
extern uint8_t adb_kb_srq_enabled, adb_mouse_srq_enabled;
extern uint8_t adb_kb_handler_id;
extern volatile uint32_t next_busy_off;
#ifdef __cplusplus
}
#endif

#endif


