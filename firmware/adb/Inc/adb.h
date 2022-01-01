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

#define ADB_OK 0
#define ADB_TIMEOUT -1
#define ADB_ERROR ADB_LINE_STATUS_ERROR

#define ADB_KB_DEFAULT_ADDR 2
#define ADB_MOUSE_DEFAULT_ADDR 3

// #define DEBUG0_HI() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET)
// #define DEBUG0_LOW() HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET)

// #define DEBUG1_HI() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_SET)
// #define DEBUG1_LOW() HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_RESET)

#define DEBUG0_HI() {}
#define DEBUG0_LOW() {}

#define DEBUG1_HI() {}
#define DEBUG1_LOW() {}


void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin);
uint8_t adb_recv_cmd(uint8_t* data, uint8_t srq);
uint8_t parse_adb_cmd(uint8_t data);
void adb_reset(void);
void write_test(void);

#ifdef __cplusplus
}
#endif

#endif


