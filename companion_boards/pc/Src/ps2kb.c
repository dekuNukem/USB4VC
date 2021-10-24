#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "ps2kb.h"
#include "delay_us.h"

#define CLKHALF 18
#define CLKFULL 36
#define BYTEWAIT 750
#define PS2KB_BUS_TIMEOUT_MS 30

GPIO_TypeDef* ps2kb_clk_port;
uint16_t ps2kb_clk_pin;

GPIO_TypeDef* ps2kb_data_port;
uint16_t ps2kb_data_pin;
uint32_t ps2kb_wait_start;

#define PS2KB_CLK_HI() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_SET)
#define PS2KB_CLK_LOW() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_RESET)

#define PS2KB_DATA_HI() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_SET)
#define PS2KB_DATA_LOW() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_RESET)

#define PS2KB_READ_DATA_PIN() HAL_GPIO_ReadPin(ps2kb_data_port, ps2kb_data_pin)
#define PS2KB_READ_CLK_PIN() HAL_GPIO_ReadPin(ps2kb_clk_port, ps2kb_clk_pin)

#define PS2KB_SENDACK() ps2kb_write(0xFA, 10)

void ps2kb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
	ps2kb_clk_port = clk_port;
	ps2kb_clk_pin = clk_pin;
	ps2kb_data_port = data_port;
	ps2kb_data_pin = data_pin;
	PS2KB_CLK_HI();
	PS2KB_DATA_HI();
}

uint8_t ps2kb_get_bus_status(void)
{
	if(PS2KB_READ_DATA_PIN() == GPIO_PIN_SET && PS2KB_READ_CLK_PIN() == GPIO_PIN_SET)
		return PS2_BUS_IDLE;
	if(PS2KB_READ_DATA_PIN() == GPIO_PIN_SET && PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
		return PS2_BUS_INHIBIT;
	if(PS2KB_READ_DATA_PIN() == GPIO_PIN_RESET && PS2KB_READ_CLK_PIN() == GPIO_PIN_SET)
		return PS2_BUS_REQ_TO_SEND;
	return PS2_BUS_UNKNOWN;
}

uint8_t ps2kb_read(uint8_t* result, uint8_t timeout_ms)
{
  uint16_t data = 0x00;
  uint16_t bit = 0x01;

  ps2kb_wait_start = HAL_GetTick();
  while(ps2kb_get_bus_status() != PS2_BUS_REQ_TO_SEND)
  {
  	if(HAL_GetTick() - ps2kb_wait_start >= timeout_ms)
  		return 1;
  }

  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  while (bit < 0x0100)
  {
    if (PS2KB_READ_DATA_PIN() == GPIO_PIN_SET)
	    data = data | bit;
    bit = bit << 1;
    delay_us(CLKHALF);
    PS2KB_CLK_LOW();
    delay_us(CLKFULL);
    PS2KB_CLK_HI();
    delay_us(CLKHALF);
  }

  // stop bit
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(CLKHALF);
  PS2KB_DATA_LOW();
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  PS2KB_DATA_HI();

  *result = data & 0x00FF;
  return 0;
}

uint8_t ps2kb_write(uint8_t data, uint8_t timeout_ms)
{
  uint8_t i;
  uint8_t parity = 1;

  ps2kb_wait_start = HAL_GetTick();
  while(ps2kb_get_bus_status() != PS2_BUS_IDLE)
  {
  	if(HAL_GetTick() - ps2kb_wait_start >= timeout_ms)
  		return 1;
  }

  delay_us(BYTEWAIT);

  PS2KB_DATA_LOW();
  delay_us(CLKHALF);
  // device sends on falling clock
  PS2KB_CLK_LOW();	// start bit
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  for (i=0; i < 8; i++)
  {
    if (data & 0x01)
      PS2KB_DATA_HI();
    else
      PS2KB_DATA_LOW();

    delay_us(CLKHALF);
    PS2KB_CLK_LOW();
    delay_us(CLKFULL);
    PS2KB_CLK_HI();
    delay_us(CLKHALF);

    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }

  // parity bit
  if (parity)
    PS2KB_DATA_HI();
  else
    PS2KB_DATA_LOW();

  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  // stop bit
  PS2KB_DATA_HI();
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(BYTEWAIT);

  return 0;
}

void keyboard_reply(uint8_t cmd, uint8_t *leds)
{
  uint8_t received;
  switch (cmd)
  {
	  case 0xFF: //reset
	    PS2KB_SENDACK();
	    ps2kb_write(0xAA, 250);
	    break;
	  case 0xFE: //resend
	    PS2KB_SENDACK();
	    break;
	  case 0xF6: //set defaults
	    //enter stream mode
	    PS2KB_SENDACK();
	    break;
	  case 0xF5: //disable data reporting
	    PS2KB_SENDACK();
	    break;
	  case 0xF4: //enable data reporting
	    PS2KB_SENDACK();
	    break;
	  case 0xF3: //set typematic rate
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == 0) 
	    	PS2KB_SENDACK(); //do nothing with the rate
	    break;
	  case 0xF2: //get device id
	    PS2KB_SENDACK();
	    ps2kb_write(0xAB, 30);
	    ps2kb_write(0x83, 30);
	    break;
	  case 0xF0: //set scan code set
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == 0)
	    	PS2KB_SENDACK();
	    break;
	  case 0xEE: //echo
	    ps2kb_write(0xEE, 30);
	    break;
	  case 0xED: // set/reset LEDs
	    PS2KB_SENDACK();
	    if(ps2kb_read(leds, 30) == 0)
	    	PS2KB_SENDACK();
	    break;
  }
}
