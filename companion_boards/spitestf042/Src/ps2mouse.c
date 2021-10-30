#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "ps2mouse.h"
#include "delay_us.h"

// https://wiki.osdev.org/PS/2_Mouse

#define CLKHALF 18
#define CLKFULL 36
#define BYTEWAIT 700
#define BYTEWAIT_END 200
#define PS2MOUSE_BUS_TIMEOUT_MS 30
#define CODE_UNUSED 0xff
#define PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS 20

GPIO_TypeDef* ps2mouse_clk_port;
uint16_t ps2mouse_clk_pin;

GPIO_TypeDef* ps2mouse_data_port;
uint16_t ps2mouse_data_pin;
uint32_t ps2mouse_wait_start;
uint8_t ps2mouse_data_reporting_enabled;
uint8_t ps2mouse_sampling_rate;
uint8_t ps2mouse_resolution;
uint8_t ps2mouse_scale;
#define SAMPLE_RATE_HISTORY_BUF_SIZE 8
uint8_t sample_rate_history[SAMPLE_RATE_HISTORY_BUF_SIZE];
uint8_t sample_rate_history_index;
uint8_t mouse_device_id;

#define PS2MOUSE_CLK_HI() HAL_GPIO_WritePin(ps2mouse_clk_port, ps2mouse_clk_pin, GPIO_PIN_SET)
#define PS2MOUSE_CLK_LOW() HAL_GPIO_WritePin(ps2mouse_clk_port, ps2mouse_clk_pin, GPIO_PIN_RESET)

#define PS2MOUSE_DATA_HI() HAL_GPIO_WritePin(ps2mouse_data_port, ps2mouse_data_pin, GPIO_PIN_SET)
#define PS2MOUSE_DATA_LOW() HAL_GPIO_WritePin(ps2mouse_data_port, ps2mouse_data_pin, GPIO_PIN_RESET)

#define PS2MOUSE_READ_DATA_PIN() HAL_GPIO_ReadPin(ps2mouse_data_port, ps2mouse_data_pin)
#define PS2MOUSE_READ_CLK_PIN() HAL_GPIO_ReadPin(ps2mouse_clk_port, ps2mouse_clk_pin)

#define PS2MOUSE_SENDACK() ps2mouse_write(0xFA, 1, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS)

void ps2mouse_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
	ps2mouse_clk_port = clk_port;
	ps2mouse_clk_pin = clk_pin;
	ps2mouse_data_port = data_port;
	ps2mouse_data_pin = data_pin;
  ps2mouse_data_reporting_enabled = 0;
  ps2mouse_sampling_rate = 100;
  ps2mouse_resolution = 3;
  ps2mouse_scale = 1;
	PS2MOUSE_CLK_HI();
	PS2MOUSE_DATA_HI();
}

uint8_t ps2mouse_get_bus_status(void)
{
	if(PS2MOUSE_READ_DATA_PIN() == GPIO_PIN_SET && PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_SET)
		return PS2_BUS_IDLE;
	if(PS2MOUSE_READ_DATA_PIN() == GPIO_PIN_SET && PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_RESET)
		return PS2_BUS_INHIBIT;
	if(PS2MOUSE_READ_DATA_PIN() == GPIO_PIN_RESET && PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_SET)
		return PS2_BUS_REQ_TO_SEND;
	return PS2_BUS_UNKNOWN;
}

uint8_t ps2mouse_read(uint8_t* result, uint8_t timeout_ms)
{
  uint16_t data = 0x00;
  uint16_t bit = 0x01;

  ps2mouse_wait_start = HAL_GetTick();
  while(ps2mouse_get_bus_status() != PS2_BUS_REQ_TO_SEND)
  {
  	if(HAL_GetTick() - ps2mouse_wait_start >= timeout_ms)
  		return 1;
  }

  delay_us(CLKHALF);
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  while (bit < 0x0100)
  {
    if (PS2MOUSE_READ_DATA_PIN() == GPIO_PIN_SET)
	    data = data | bit;
    bit = bit << 1;
    delay_us(CLKHALF);
    PS2MOUSE_CLK_LOW();
    delay_us(CLKFULL);
    PS2MOUSE_CLK_HI();
    delay_us(CLKHALF);
  }

  // stop bit
  delay_us(CLKHALF);
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  delay_us(CLKHALF);
  PS2MOUSE_DATA_LOW();
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);
  PS2MOUSE_DATA_HI();

  *result = data & 0x00FF;
  return 0;
}

uint8_t ps2mouse_write(uint8_t data, uint8_t delay_start, uint8_t timeout_ms)
{
  uint8_t i;
  uint8_t parity = 1;

  ps2mouse_wait_start = HAL_GetTick();
  while(ps2mouse_get_bus_status() != PS2_BUS_IDLE)
  {
  	if(HAL_GetTick() - ps2mouse_wait_start >= timeout_ms)
  		return 1;
  }

  // if responding to host, wait a little while for it to get ready
  if(delay_start)
    delay_us(BYTEWAIT);

  PS2MOUSE_DATA_LOW();
  delay_us(CLKHALF);
  // device sends on falling clock
  PS2MOUSE_CLK_LOW();	// start bit
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  for (i=0; i < 8; i++)
  {
    if (data & 0x01)
      PS2MOUSE_DATA_HI();
    else
      PS2MOUSE_DATA_LOW();

    delay_us(CLKHALF);
    PS2MOUSE_CLK_LOW();
    delay_us(CLKFULL);
    PS2MOUSE_CLK_HI();
    delay_us(CLKHALF);

    parity = parity ^ (data & 0x01);
    data = data >> 1;
  }

  // parity bit
  if (parity)
    PS2MOUSE_DATA_HI();
  else
    PS2MOUSE_DATA_LOW();

  delay_us(CLKHALF);
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  // stop bit
  PS2MOUSE_DATA_HI();
  delay_us(CLKHALF);
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  delay_us(BYTEWAIT_END);

  return 0;
}

void mouse_reply(uint8_t cmd)
{
  switch (cmd)
  {
	  case 0xFF: //reset
      sample_rate_history_index = 0;
      memset(sample_rate_history, 0, SAMPLE_RATE_HISTORY_BUF_SIZE);
	    PS2MOUSE_SENDACK();
	    ps2mouse_write(0xAA, 0, 250);
      ps2mouse_write(0, 0, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xFE: //resend
	    PS2MOUSE_SENDACK();
	    break;
	  case 0xF6: //set defaults
	    PS2MOUSE_SENDACK();
	    break;
	  case 0xF5: //disable data reporting
	    PS2MOUSE_SENDACK();
      ps2mouse_data_reporting_enabled = 0;
	    break;
    case 0xF4: //enable data reporting
      PS2MOUSE_SENDACK();
      ps2mouse_data_reporting_enabled = 1;
      break;
	  case 0xF3: //set sampling rate
	    PS2MOUSE_SENDACK();
	    if(ps2mouse_read(&ps2mouse_sampling_rate, 30) == 0)
      {
        sample_rate_history[sample_rate_history_index] = ps2mouse_sampling_rate;
        if(sample_rate_history_index < SAMPLE_RATE_HISTORY_BUF_SIZE-1)
          sample_rate_history_index++;
	    	PS2MOUSE_SENDACK();
      }
	    break;
	  case 0xF2: //get device id
	    PS2MOUSE_SENDACK();
      mouse_device_id = 0; // standard ps/2 mouse
      if (sample_rate_history_index > 2 && sample_rate_history[sample_rate_history_index-1] == 80 && sample_rate_history[sample_rate_history_index-2] == 100 && sample_rate_history[sample_rate_history_index-3] == 200)
        mouse_device_id = 3; // intellimouse with scroll wheel
	    ps2mouse_write(mouse_device_id, 0, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xE8: // set resolution
	    PS2MOUSE_SENDACK();
	    if(ps2mouse_read(&ps2mouse_resolution, 30) == 0)
	    	PS2MOUSE_SENDACK();
	    break;
    case 0xE6: // reset scale
      PS2MOUSE_SENDACK();
      ps2mouse_scale = 1;
      break;
    case 0xE7: // set scale 2 to 1
      PS2MOUSE_SENDACK();
      ps2mouse_scale = 2;
      break;
	  case 0xEE: //echo
	    ps2mouse_write(0xEE, 1, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
    default:
      PS2MOUSE_SENDACK();
  }
}
