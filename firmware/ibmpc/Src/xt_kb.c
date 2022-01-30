#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "xt_kb.h"
#include "delay_us.h"

GPIO_TypeDef* xtkb_clk_port;
uint16_t xtkb_clk_pin;

GPIO_TypeDef* xtkb_data_port;
uint16_t xtkb_data_pin;

#define XTKB_CLK_HI() HAL_GPIO_WritePin(xtkb_clk_port, xtkb_clk_pin, GPIO_PIN_SET)
#define XTKB_CLK_LOW() HAL_GPIO_WritePin(xtkb_clk_port, xtkb_clk_pin, GPIO_PIN_RESET)

#define XTKB_DATA_HI() HAL_GPIO_WritePin(xtkb_data_port, xtkb_data_pin, GPIO_PIN_SET)
#define XTKB_DATA_LOW() HAL_GPIO_WritePin(xtkb_data_port, xtkb_data_pin, GPIO_PIN_RESET)

#define XTKB_READ_DATA_PIN() HAL_GPIO_ReadPin(xtkb_data_port, xtkb_data_pin)
#define XTKB_READ_CLK_PIN() HAL_GPIO_ReadPin(xtkb_clk_port, xtkb_clk_pin)

uint32_t last_clk_high, last_typematic;

void xtkb_release_lines(void)
{
  XTKB_CLK_HI();
  XTKB_DATA_HI();
}

void xtkb_reset_bus(void)
{
  XTKB_CLK_HI();
  XTKB_DATA_LOW();
}

void xtkb_enable(void)
{
  xtkb_reset_bus();
  last_clk_high = HAL_GetTick();
}

void xtkb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
  xtkb_clk_port = clk_port;
  xtkb_clk_pin = clk_pin;
  xtkb_data_port = data_port;
  xtkb_data_pin = data_pin;
}

#define CLK_HI_DURATION_US 65
#define CLK_LOW_DURATION_US 30
#define CLK_RTS_DURATION_US 110

// turns out linux input keycode is based on XT! no need for lookup table!
uint8_t xtkb_write(uint8_t data)
{
  // if clk is low, then host is holding it, inhibiting transmission
  if(XTKB_READ_CLK_PIN() == GPIO_PIN_RESET)
    return 1;

  XTKB_CLK_LOW();
  delay_us(5);
  XTKB_DATA_HI();
  delay_us(CLK_RTS_DURATION_US);
  // if data pin is still low, host is inhibiting transmission
  if(XTKB_READ_DATA_PIN() == GPIO_PIN_RESET)
    return 2;
  XTKB_CLK_HI();
  delay_us(CLK_HI_DURATION_US);
  XTKB_CLK_LOW();
  delay_us(CLK_LOW_DURATION_US);
  
  for (int i = 0; i < 8; ++i)
  {
    if (data & 0x01)
      XTKB_DATA_HI();
    else
      XTKB_DATA_LOW();

    XTKB_CLK_HI();
    delay_us(CLK_HI_DURATION_US);
    XTKB_CLK_LOW();
    delay_us(CLK_LOW_DURATION_US);
    data = data >> 1;
  }

  XTKB_CLK_HI();
  XTKB_DATA_LOW();

  return 0;
}

uint8_t wait_for_clk_high(uint32_t timeout_ms)
{
  uint32_t start_time = HAL_GetTick();
  while(XTKB_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    if(HAL_GetTick() - start_time > timeout_ms)
      return 1;
  }
  return 0;
}

void xtkb_check_for_softreset(void)
{
  if(XTKB_READ_CLK_PIN() == GPIO_PIN_SET)
    last_clk_high = HAL_GetTick();
  if(HAL_GetTick() - last_clk_high > 20)
  {
    wait_for_clk_high(200);
    HAL_Delay(20);
    xtkb_write(0xaa);
    HAL_Delay(10);
  }
}

#define KEY_UP      103
#define KEY_PAGEUP    104
#define KEY_LEFT    105
#define KEY_RIGHT   106
#define KEY_END     107
#define KEY_DOWN    108
#define KEY_PAGEDOWN    109
#define KEY_HOME    102
#define KEY_INSERT    110
#define KEY_DELETE    111

// status 1 pressed 0 released
uint8_t xtkb_press_key(uint8_t code, uint8_t status)
{
  // on XT keyboard those keys are on numpads, so need to translate over
  switch(code)
  {
    case KEY_UP:
      code = 72;
      break;
    case KEY_PAGEUP:
      code = 74;
      break;
    case KEY_LEFT:
      code = 75;
      break;
    case KEY_RIGHT:
      code = 78;
      break;
    case KEY_END:
      code = 79;
      break;
    case KEY_DOWN:
      code = 80;
      break;
    case KEY_PAGEDOWN:
      code = 81;
      break;
    case KEY_HOME:
      code = 71;
      break;
    case KEY_INSERT:
      code = 82;
      break;
    case KEY_DELETE:
      code = 83;
      break;
  }

  if(code > 83) // not on XT keyboard
    return 0;
  if(status == 2) // typematic
  {
    if(HAL_GetTick() - last_typematic <= 80)
      return 0;
    last_typematic = HAL_GetTick();
  }
  if(!status)
    code = code | 0x80;
  return xtkb_write(code);
}

