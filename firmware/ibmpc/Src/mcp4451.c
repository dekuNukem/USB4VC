#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mcp4451.h"
#include "shared.h"
#include "main.h"

#define MCP4451_WIPER_COUNT 4
const uint8_t mcp4451_wiper_id_to_reg_addr_lookup[MCP4451_WIPER_COUNT] = {0x0, 0x1, 0x6, 0x7};

uint8_t mcp4451_is_available(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c2, MCP4451_I2C_ADDRESS, 1, 100) == 0;
}

void mcp4451_reset(void)
{
  HAL_GPIO_WritePin(POT_RESET_GPIO_Port, POT_RESET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(POT_RESET_GPIO_Port, POT_RESET_Pin, GPIO_PIN_SET);
}

uint8_t mcp4451_write_wiper(uint8_t wiper_id, uint8_t value)
{
    if(wiper_id >= MCP4451_WIPER_COUNT)
        return 255;
    uint8_t i2c_mem_addr = mcp4451_wiper_id_to_reg_addr_lookup[wiper_id] << 4;
    return HAL_I2C_Mem_Write(&hi2c2,MCP4451_I2C_ADDRESS,i2c_mem_addr,1,&value,1,100);
}
