#ifndef __ADB_H
#define __ADB_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin);
void adb_recv_cmd(void);

#ifdef __cplusplus
}
#endif

#endif


