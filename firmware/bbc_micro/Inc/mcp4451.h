#ifndef __MCP4451_H
#define __MCP4451_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define MCP4451_I2C_ADDRESS 0x58

uint8_t mcp4451_is_available(void);
void mcp4451_reset(void);
uint8_t mcp4451_write_wiper(uint8_t wiper_id, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif


