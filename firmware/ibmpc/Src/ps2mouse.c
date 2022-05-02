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
#define BYTEWAIT_END 250
#define PS2MOUSE_BUS_TIMEOUT_MS 30
#define CODE_UNUSED 0xff
#define PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS 20

#define PS2MOUSE_MODE_STREAM 0
#define PS2MOUSE_MODE_REMOTE 1
#define PS2MOUSE_MODE_WRAP 2

GPIO_TypeDef* ps2mouse_clk_port;
uint16_t ps2mouse_clk_pin;

GPIO_TypeDef* ps2mouse_data_port;
uint16_t ps2mouse_data_pin;
uint8_t ps2mouse_data_reporting_enabled;
uint8_t ps2mouse_sampling_rate;
uint8_t ps2mouse_resolution;
uint8_t ps2mouse_scale;
#define SAMPLE_RATE_HISTORY_BUF_SIZE 8
uint8_t sample_rate_history[SAMPLE_RATE_HISTORY_BUF_SIZE];
uint8_t sample_rate_history_index;
uint8_t mouse_device_id;
uint8_t ps2mouse_current_mode;
uint8_t ps2mouse_prev_mode;
uint8_t x_accumulator, y_accumulator, scroll_accumulator;

#define PS2MOUSE_PACKET_SIZE_GENERIC 3
#define PS2MOUSE_PACKET_SIZE_INTELLIMOUSE 4

#define PS2MOUSE_CLK_HI() HAL_GPIO_WritePin(ps2mouse_clk_port, ps2mouse_clk_pin, GPIO_PIN_SET)
#define PS2MOUSE_CLK_LOW() HAL_GPIO_WritePin(ps2mouse_clk_port, ps2mouse_clk_pin, GPIO_PIN_RESET)

#define PS2MOUSE_DATA_HI() HAL_GPIO_WritePin(ps2mouse_data_port, ps2mouse_data_pin, GPIO_PIN_SET)
#define PS2MOUSE_DATA_LOW() HAL_GPIO_WritePin(ps2mouse_data_port, ps2mouse_data_pin, GPIO_PIN_RESET)

#define PS2MOUSE_READ_DATA_PIN() HAL_GPIO_ReadPin(ps2mouse_data_port, ps2mouse_data_pin)
#define PS2MOUSE_READ_CLK_PIN() HAL_GPIO_ReadPin(ps2mouse_clk_port, ps2mouse_clk_pin)

#define PS2MOUSE_SENDACK() ps2mouse_write_delay_start(0xFA, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS)

void ps2mouse_release_lines(void)
{
  PS2MOUSE_CLK_HI();
  PS2MOUSE_DATA_HI();
}

void reset_accumulators(void)
{
  x_accumulator = 0;
  y_accumulator = 0;
  scroll_accumulator = 0;
}

void ps2mouse_restore_defaults(void)
{
  ps2mouse_sampling_rate = 100;
  ps2mouse_resolution = 2;
  ps2mouse_scale = 1;
  ps2mouse_data_reporting_enabled = 0;
  ps2mouse_current_mode = PS2MOUSE_MODE_STREAM;
  ps2mouse_prev_mode = PS2MOUSE_MODE_STREAM;
  reset_accumulators();
}

void ps2mouse_reset(void)
{
  ps2mouse_restore_defaults();
  sample_rate_history_index = 0;
  memset(sample_rate_history, 0, SAMPLE_RATE_HISTORY_BUF_SIZE);
  mouse_device_id = 0;
}

void ps2mouse_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
  ps2mouse_clk_port = clk_port;
  ps2mouse_clk_pin = clk_pin;
  ps2mouse_data_port = data_port;
  ps2mouse_data_pin = data_pin;
  ps2mouse_reset();
	ps2mouse_release_lines();
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

  uint32_t ps2mouse_wait_start = HAL_GetTick();
  while(ps2mouse_get_bus_status() != PS2_BUS_REQ_TO_SEND)
  {
  	if(HAL_GetTick() - ps2mouse_wait_start >= timeout_ms)
  		return PS2_ERROR_TIMEOUT;
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

  *result = data & 0xFF;
  return PS2_OK;
}

uint8_t ps2mouse_wait_for_idle(uint8_t timeout_ms)
{
  uint32_t ps2mouse_wait_start = HAL_GetTick();
  while(ps2mouse_get_bus_status() != PS2_BUS_IDLE)
  {
    if(HAL_GetTick() - ps2mouse_wait_start >= timeout_ms)
      return PS2_ERROR_TIMEOUT;
  }
  return PS2_OK;
}

uint8_t ps2mouse_write_delay_start(uint8_t data, uint8_t timeout_ms)
{
  if(ps2mouse_wait_for_idle(timeout_ms) != 0)
    return PS2_ERROR_TIMEOUT;
  delay_us(BYTEWAIT);
  return ps2mouse_write_nowait(data);
}

uint8_t ps2mouse_write(uint8_t data, uint8_t timeout_ms)
{
  if(ps2mouse_wait_for_idle(timeout_ms) != 0)
    return PS2_ERROR_TIMEOUT;
  return ps2mouse_write_nowait(data);
}

void ps2mouse_host_req_reply(uint8_t cmd, mouse_event* mevent)
{
  uint8_t first_byte = 0;
  if(cmd == 0xFF) // reset
  {
    ps2mouse_reset();
    PS2MOUSE_SENDACK();
    ps2mouse_write(0xAA, 250);
    ps2mouse_write(0, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
    return;
  }
  if(cmd == 0xEC) // reset wrap mode
  {
    ps2mouse_current_mode = ps2mouse_prev_mode;
    reset_accumulators();
    PS2MOUSE_SENDACK();
    return;
  }
  if(ps2mouse_current_mode == PS2MOUSE_MODE_WRAP)
  {
    ps2mouse_write(cmd, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
    return;
  }

  switch (cmd)
  {
	  case 0xFE: //resend
	    PS2MOUSE_SENDACK();
	    break;
	  case 0xF6: //set defaults
      ps2mouse_restore_defaults();
	    PS2MOUSE_SENDACK();
	    break;
	  case 0xF5: //disable data reporting
      ps2mouse_data_reporting_enabled = 0;
      reset_accumulators();
	    PS2MOUSE_SENDACK();
	    break;
    case 0xF4: //enable data reporting
      ps2mouse_data_reporting_enabled = 1;
      reset_accumulators();
      PS2MOUSE_SENDACK();
      break;
	  case 0xF3: //set sampling rate
	    PS2MOUSE_SENDACK();
      reset_accumulators();
	    if(ps2mouse_read(&ps2mouse_sampling_rate, 150) == 0)
      {
        sample_rate_history[sample_rate_history_index] = ps2mouse_sampling_rate;
        if(sample_rate_history_index < SAMPLE_RATE_HISTORY_BUF_SIZE-1)
          sample_rate_history_index++;
	    	PS2MOUSE_SENDACK();
      }
	    break;
	  case 0xF2: //get device id
      reset_accumulators();
	    PS2MOUSE_SENDACK();
      mouse_device_id = 0; // standard ps/2 mouse
      // if (sample_rate_history_index > 2 && sample_rate_history[sample_rate_history_index-1] == 80 && sample_rate_history[sample_rate_history_index-2] == 100 && sample_rate_history[sample_rate_history_index-3] == 200)
      //   mouse_device_id = 3; // intellimouse with scroll wheel
	    ps2mouse_write(mouse_device_id, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
    case 0xF0: // set remote mode
      reset_accumulators();
      ps2mouse_prev_mode = ps2mouse_current_mode;
      ps2mouse_current_mode = PS2MOUSE_MODE_REMOTE;
      PS2MOUSE_SENDACK();
      break;
    case 0xEE: // set wrap mode
      if(ps2mouse_current_mode != PS2MOUSE_MODE_WRAP)
        ps2mouse_prev_mode = ps2mouse_current_mode;
      ps2mouse_current_mode = PS2MOUSE_MODE_WRAP;
      reset_accumulators();
      PS2MOUSE_SENDACK();
      break;
    case 0xEB: // read data
      PS2MOUSE_SENDACK();
      // do stuff
      reset_accumulators();
      break;
    case 0xEA: // set stream mode
      ps2mouse_prev_mode = ps2mouse_current_mode;
      ps2mouse_current_mode = PS2MOUSE_MODE_STREAM;
      reset_accumulators();
      PS2MOUSE_SENDACK();
      break;
    case 0xE9: // status request
      PS2MOUSE_SENDACK();
      if(ps2mouse_current_mode == PS2MOUSE_MODE_REMOTE)
        first_byte |= 0x40;
      if(ps2mouse_data_reporting_enabled)
        first_byte |= 0x20;
      if(ps2mouse_scale == 2)
        first_byte |= 0x10;
      if(mevent->button_left)
        first_byte |= 0x4;
      if(mevent->button_middle)
        first_byte |= 0x2;
      if(mevent->button_right)
        first_byte |= 0x1;
      ps2mouse_write(first_byte, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
      ps2mouse_write(ps2mouse_resolution, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
      ps2mouse_write(ps2mouse_sampling_rate, PS2MOUSE_WRITE_DEFAULT_TIMEOUT_MS);
      reset_accumulators();
      break;
	  case 0xE8: // set resolution
	    PS2MOUSE_SENDACK();
	    if(ps2mouse_read(&ps2mouse_resolution, 150) == 0)
	    	PS2MOUSE_SENDACK();
      reset_accumulators();
	    break;
    case 0xE6: // reset scale
      PS2MOUSE_SENDACK();
      ps2mouse_scale = 1;
      break;
    case 0xE7: // set scale 2 to 1
      PS2MOUSE_SENDACK();
      ps2mouse_scale = 2;
      break;
    default:
      PS2MOUSE_SENDACK();
  }
}

uint8_t ps2mouse_get_outgoing_data(mouse_event* this_event, ps2_outgoing_buf* pbuf)
{
  if(ps2mouse_current_mode == PS2MOUSE_MODE_REMOTE)
  {
    x_accumulator += (uint8_t)(this_event->movement_x);
    y_accumulator += (uint8_t)(this_event->movement_y);
    scroll_accumulator += (uint8_t)(this_event->scroll_vertical);
    return 1;
  }
  if(ps2mouse_current_mode == PS2MOUSE_MODE_WRAP)
    return 2;
  if(ps2mouse_data_reporting_enabled == 0)
    return 3;

  memset(pbuf->data, 0, PS2_OUT_BUF_MAXSIZE);
  pbuf->size = PS2MOUSE_PACKET_SIZE_GENERIC;
  pbuf->data[0] = 0x8; // bit 3 is always 1
  // https://wiki.osdev.org/PS/2_Mouse
  if(this_event->button_left)
    pbuf->data[0] = pbuf->data[0] | 0x1;
  if(this_event->button_right)
    pbuf->data[0] = pbuf->data[0] | 0x2;
  if(this_event->button_middle)
    pbuf->data[0] = pbuf->data[0] | 0x4;
  if(this_event->movement_x < 0)
    pbuf->data[0] = pbuf->data[0] | 0x10;
  if(this_event->movement_y < 0)
    pbuf->data[0] = pbuf->data[0] | 0x20;
  pbuf->data[1] = (uint8_t)(this_event->movement_x);
  pbuf->data[2] = (uint8_t)(this_event->movement_y);
  pbuf->data[3] = (uint8_t)(this_event->scroll_vertical);
  if(mouse_device_id != 0)
    pbuf->size = PS2MOUSE_PACKET_SIZE_INTELLIMOUSE;
  return PS2_OK;
}

uint8_t ps2mouse_write_nowait(uint8_t data)
{
  uint8_t parity = 1;

  PS2MOUSE_DATA_LOW();
  delay_us(CLKHALF);
  // device sends on falling clock
  PS2MOUSE_CLK_LOW(); // start bit
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);
  if(PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2mouse_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  for (int i=0; i < 8; i++)
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
    if(PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_RESET)
    {
      ps2mouse_release_lines();
      return PS2_ERROR_HOST_INHIBIT;
    }

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
  if(PS2MOUSE_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2mouse_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  // stop bit
  PS2MOUSE_DATA_HI();
  delay_us(CLKHALF);
  PS2MOUSE_CLK_LOW();
  delay_us(CLKFULL);
  PS2MOUSE_CLK_HI();
  delay_us(CLKHALF);

  delay_us(BYTEWAIT_END);

  return PS2_OK;
}

uint8_t ps2mouse_send_update(ps2_outgoing_buf* pbuf)
{
  uint8_t write_result;
  for (int i = 0; i < pbuf->size; ++i)
  {
    // return error if inhibited or interrupted while transmitting
    write_result = ps2mouse_write(pbuf->data[i], 255);
    if(write_result)
      return write_result;
  }
  return PS2_OK;
}
