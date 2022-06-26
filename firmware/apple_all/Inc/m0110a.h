#ifndef __M0110A_H
#define __M0110A_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define M0110A_OK 0
#define M0110A_LINE_IDLE 1
#define M0110A_LINE_HOST_REQ 2
#define M0110A_TIMEOUT 3


uint8_t m0110a_get_line_status(void);
uint8_t m0110a_read_host_cmd(uint8_t* result, uint16_t timeout_ms);
uint8_t m0110a_write(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif


