#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "m0110a.h"

#define CLK_HIGH_HOST_TO_KB 220
#define CLK_LOW_HOST_TO_KB 180

#define CLK_HIGH_KB_TO_HOST 170
#define CLK_LOW_KB_TO_HOST 160

#define M0110A_CLK_HI() HAL_GPIO_WritePin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin, GPIO_PIN_SET)
#define M0110A_CLK_LOW() HAL_GPIO_WritePin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin, GPIO_PIN_RESET)

#define M0110A_DATA_HI() HAL_GPIO_WritePin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin, GPIO_PIN_SET)
#define M0110A_DATA_LOW() HAL_GPIO_WritePin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin, GPIO_PIN_RESET)

#define M0110A_READ_DATA_PIN() HAL_GPIO_ReadPin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin)
#define M0110A_READ_CLK_PIN() HAL_GPIO_ReadPin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin)


uint8_t m0110a_get_line_status(void)
{
  uint32_t entry_time = HAL_GetTick();
  while(HAL_GetTick() - entry_time < 10)
  {
    if(HAL_GPIO_ReadPin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin))
      return M0110A_LINE_IDLE;
  }
  return M0110A_LINE_HOST_REQ;
}

uint8_t m0110a_read(uint8_t* result, uint8_t timeout_ms)
{
  uint16_t data = 0x00;
  uint16_t bit = 0x01;

  delay_us(CLKHALF);
  M0110A_CLK_LOW();
  delay_us(CLKFULL);
  M0110A_CLK_HI();
  delay_us(CLKHALF);

  while(bit < 0x0100)
  {
    if (M0110A_READ_DATA_PIN() == GPIO_PIN_SET)
      data = data | bit;
    bit = bit << 1;
    delay_us(CLKHALF);
    M0110A_CLK_LOW();
    delay_us(CLKFULL);
    M0110A_CLK_HI();
    delay_us(CLKHALF);
  }

  // stop bit
  delay_us(CLKHALF);
  M0110A_CLK_LOW();
  delay_us(CLKFULL);
  M0110A_CLK_HI();
  delay_us(CLKHALF);

  delay_us(CLKHALF);
  M0110A_DATA_LOW();
  M0110A_CLK_LOW();
  delay_us(CLKFULL);
  M0110A_CLK_HI();
  delay_us(CLKHALF);
  M0110A_DATA_HI();

  *result = data & 0x00FF;
  return PS2_OK;
}


