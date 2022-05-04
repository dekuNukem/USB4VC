#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "ps2kb.h"
#include "delay_us.h"

#define CLKHALF 18
#define CLKFULL 36
#define BYTEWAIT 700
#define BYTEWAIT_END 200
#define PS2KB_BUS_TIMEOUT_MS 30
#define CODE_UNUSED 0xff
#define PS2KB_WRITE_DEFAULT_TIMEOUT_MS 20

const uint8_t linux_keycode_to_ps2_scancode_lookup_single_byte_codeset2[LINUX_KEYCODE_TO_PS2_SCANCODE_SINGLE_SIZE] = 
{
    CODE_UNUSED, // KEY_RESERVED       0
    0x76, // KEY_ESC            1
    0x16, // KEY_1          2
    0x1E, // KEY_2          3
    0x26, // KEY_3          4
    0x25, // KEY_4          5
    0x2E, // KEY_5          6
    0x36, // KEY_6          7
    0x3D, // KEY_7          8
    0x3E, // KEY_8          9
    0x46, // KEY_9          10
    0x45, // KEY_0          11
    0x4E, // KEY_MINUS      12
    0x55, // KEY_EQUAL      13
    0x66, // KEY_BACKSPACE      14
    0x0D, // KEY_TAB            15
    0x15, // KEY_Q          16
    0x1D, // KEY_W          17
    0x24, // KEY_E          18
    0x2D, // KEY_R          19
    0x2C, // KEY_T          20
    0x35, // KEY_Y          21
    0x3C, // KEY_U          22
    0x43, // KEY_I          23
    0x44, // KEY_O          24
    0x4D, // KEY_P          25
    0x54, // KEY_LEFTBRACE      26
    0x5B, // KEY_RIGHTBRACE     27
    0x5A, // KEY_ENTER      28
    0x14, // KEY_LEFTCTRL       29
    0x1C, // KEY_A          30
    0x1B, // KEY_S          31
    0x23, // KEY_D          32
    0x2B, // KEY_F          33
    0x34, // KEY_G          34
    0x33, // KEY_H          35
    0x3B, // KEY_J          36
    0x42, // KEY_K          37
    0x4B, // KEY_L          38
    0x4C, // KEY_SEMICOLON      39
    0x52, // KEY_APOSTROPHE     40
    0x0E, // KEY_GRAVE      41
    0x12, // KEY_LEFTSHIFT      42
    0x5D, // KEY_BACKSLASH      43
    0x1A, // KEY_Z          44
    0x22, // KEY_X          45
    0x21, // KEY_C          46
    0x2A, // KEY_V          47
    0x32, // KEY_B          48
    0x31, // KEY_N          49
    0x3A, // KEY_M          50
    0x41, // KEY_COMMA      51
    0x49, // KEY_DOT            52
    0x4A, // KEY_SLASH      53
    0x59, // KEY_RIGHTSHIFT     54
    0x7C, // KEY_KPASTERISK     55
    0x11, // KEY_LEFTALT        56
    0x29, // KEY_SPACE      57
    0x58, // KEY_CAPSLOCK       58
    0x05, // KEY_F1         59
    0x06, // KEY_F2         60
    0x04, // KEY_F3         61
    0x0C, // KEY_F4         62
    0x03, // KEY_F5         63
    0x0B, // KEY_F6         64
    0x83, // KEY_F7         65
    0x0A, // KEY_F8         66
    0x01, // KEY_F9         67
    0x09, // KEY_F10            68
    0x77, // KEY_NUMLOCK        69
    0x7E, // KEY_SCROLLLOCK     70
    0x6C, // KEY_KP7            71
    0x75, // KEY_KP8            72
    0x7D, // KEY_KP9            73
    0x7B, // KEY_KPMINUS        74
    0x6B, // KEY_KP4            75
    0x73, // KEY_KP5            76
    0x74, // KEY_KP6            77
    0x79, // KEY_KPPLUS     78
    0x69, // KEY_KP1            79
    0x72, // KEY_KP2            80
    0x7A, // KEY_KP3            81
    0x70, // KEY_KP0            82
    0x71, // KEY_KPDOT      83
    CODE_UNUSED, // KEY_UNUSED     84
    CODE_UNUSED, // KEY_ZENKAKUHANKAKU 85
    0x61, // KEY_102ND      86
    0x78, // KEY_F11            87
    0x07, // KEY_F12            88
};


const uint8_t linux_keycode_to_ps2_scancode_lookup_special_codeset2[LINUX_KEYCODE_TO_PS2_SCANCODE_SPECIAL_SIZE] = 
{
  0x5A, // KPENTER    96
  0x14, // KEY_RIGHTCTRL    97
  0x4A, // KEY_KPSLASH    98
  CODE_UNUSED, // 99
  0x11, // KEY_RIGHTALT   100
  CODE_UNUSED, // 101
  0x6C, // KEY_HOME   102
  0x75, // KEY_UP     103
  0x7D, // KEY_PAGEUP   104
  0x6B, // KEY_LEFT   105
  0x74, // KEY_RIGHT    106
  0x69, // KEY_END      107
  0x72, // KEY_DOWN   108
  0x7A, // KEY_PAGEDOWN   109
  0x70, // KEY_INSERT   110
  0x71, // KEY_DELETE   111
  CODE_UNUSED, // KEY_MACRO   112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER   116 
  CODE_UNUSED, // KEY_KPEQUAL   117
  CODE_UNUSED, // KEY_KPPLUSMINUS   118
  CODE_UNUSED, // KEY_PAUSE   119
  CODE_UNUSED, // KEY_SCALE   120 
  CODE_UNUSED, // KEY_KPCOMMA   121
  CODE_UNUSED, // KEY_HANGEUL   122
  CODE_UNUSED, // KEY_HANJA   123
  CODE_UNUSED, // KEY_YEN     124
  0x1F, // KEY_LEFTMETA   125
  0x27, // KEY_RIGHTMETA    126
  0x2F, // KEY_COMPOSE    127
};

GPIO_TypeDef* ps2kb_clk_port;
uint16_t ps2kb_clk_pin;

GPIO_TypeDef* ps2kb_data_port;
uint16_t ps2kb_data_pin;
uint32_t ps2kb_wait_start;

uint8_t ps2kb_current_scancode_set = 2;
uint8_t ps2kb_data_reporting_enabled = 1;

#define PS2KB_CLK_HI() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_SET)
#define PS2KB_CLK_LOW() HAL_GPIO_WritePin(ps2kb_clk_port, ps2kb_clk_pin, GPIO_PIN_RESET)

#define PS2KB_DATA_HI() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_SET)
#define PS2KB_DATA_LOW() HAL_GPIO_WritePin(ps2kb_data_port, ps2kb_data_pin, GPIO_PIN_RESET)

#define PS2KB_READ_DATA_PIN() HAL_GPIO_ReadPin(ps2kb_data_port, ps2kb_data_pin)
#define PS2KB_READ_CLK_PIN() HAL_GPIO_ReadPin(ps2kb_clk_port, ps2kb_clk_pin)

#define PS2KB_SENDACK() ps2kb_write(0xFA, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS)

void ps2kb_release_lines(void)
{
  PS2KB_CLK_HI();
  PS2KB_DATA_HI();
}

void ps2kb_reset(void)
{
  ps2kb_current_scancode_set = 2;
  ps2kb_data_reporting_enabled = 1;
}

void ps2kb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
	ps2kb_clk_port = clk_port;
	ps2kb_clk_pin = clk_pin;
	ps2kb_data_port = data_port;
	ps2kb_data_pin = data_pin;
	ps2kb_release_lines();
  ps2kb_reset();
}

uint8_t ps2kb_get_bus_status(void)
{
  uint8_t clk_stat = PS2KB_READ_CLK_PIN();
  uint8_t data_stat = PS2KB_READ_DATA_PIN();
	if(data_stat == GPIO_PIN_SET && clk_stat == GPIO_PIN_SET)
		return PS2_BUS_IDLE;
	if(data_stat == GPIO_PIN_SET && clk_stat == GPIO_PIN_RESET)
		return PS2_BUS_INHIBIT;
	if(data_stat == GPIO_PIN_RESET && clk_stat == GPIO_PIN_SET)
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
  		return PS2_ERROR_TIMEOUT;
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
  return PS2_OK;
}

uint8_t ps2kb_write_nowait(uint8_t data)
{
  uint8_t parity = 1;

  PS2KB_DATA_LOW();
  delay_us(CLKHALF);
  // device sends on falling clock
  PS2KB_CLK_LOW(); // start bit
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2kb_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  for (int i=0; i < 8; i++)
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
    if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
    {
      ps2kb_release_lines();
      return PS2_ERROR_HOST_INHIBIT;
    }

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
  if(PS2KB_READ_CLK_PIN() == GPIO_PIN_RESET)
  {
    ps2kb_release_lines();
    return PS2_ERROR_HOST_INHIBIT;
  }

  // stop bit
  PS2KB_DATA_HI();
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(BYTEWAIT_END);

  return PS2_OK;
}
uint8_t ps2kb_write(uint8_t data, uint8_t delay_start, uint8_t timeout_ms)
{
  ps2kb_wait_start = HAL_GetTick();
  while(ps2kb_get_bus_status() != PS2_BUS_IDLE)
  {
  	if(HAL_GetTick() - ps2kb_wait_start >= timeout_ms)
  		return 1;
  }

  // if responding to host, wait a little while for it to get ready
  if(delay_start)
    delay_us(BYTEWAIT);

  return ps2kb_write_nowait(data);
}

uint8_t scancode_set_reply_lookup(uint8_t set)
{
  if(set == 1)
    return 0x43;
  if(set == 3)
    return 0x3f;
  return 0x41;
}

void keyboard_reply(uint8_t cmd, uint8_t *leds)
{
  uint8_t received = 255;
  switch (cmd)
  {
	  case 0xFF: //reset
	    PS2KB_SENDACK();
      ps2kb_reset();
	    ps2kb_write(0xAA, 0, 250);
	    break;
	  case 0xFE: //resend
	    PS2KB_SENDACK();
	    break;
    case 0xFA:
    case 0xF9:
    case 0xF8:
    case 0xF7:
      if(ps2kb_current_scancode_set == 3)
        PS2KB_SENDACK();
      else
        ps2kb_write(0xFE, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
      break;
	  case 0xF6: //set defaults
	    PS2KB_SENDACK();
	    break;
	  case 0xF5: //disable data reporting
	    PS2KB_SENDACK();
      ps2kb_data_reporting_enabled = 0;
	    break;
	  case 0xF4: //enable data reporting
	    PS2KB_SENDACK();
      ps2kb_data_reporting_enabled = 1;
	    break;
	  case 0xF3: //set typematic rate
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == PS2_OK) 
	    	PS2KB_SENDACK(); //do nothing with the rate
	    break;
	  case 0xF2: //get device id
	    PS2KB_SENDACK();
	    ps2kb_write(0xAB, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    ps2kb_write(0x83, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xF0: //get/change scan code set
	    PS2KB_SENDACK();
	    if(ps2kb_read(&received, 30) == PS2_OK)
      {
	    	PS2KB_SENDACK();
        if(received == 0)
          ps2kb_write(scancode_set_reply_lookup(ps2kb_current_scancode_set), 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
        else if(received <= 3)
          ps2kb_current_scancode_set = received;
      }
	    break;
	  case 0xEE: //echo
	    ps2kb_write(0xEE, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS);
	    break;
	  case 0xED: // set/reset LEDs
	    PS2KB_SENDACK();
	    if(ps2kb_read(leds, 30) == PS2_OK)
	    	PS2KB_SENDACK();
	    break;
  }
}

#define LINUX_KEYCODE_F11 87
#define LINUX_KEYCODE_F12 88
uint8_t ps2kb_press_key_scancode_1(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  // XT codes
  if(linux_keycode <= 83 || linux_keycode == LINUX_KEYCODE_F11 || linux_keycode == LINUX_KEYCODE_F12)
  {
    if(linux_keyvalue)
    {
      if(ps2kb_write(linux_keycode, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(linux_keycode | 0x80, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  return PS2_ERROR_UNKNOWN;
}

uint8_t ps2kb_press_key_scancode_2(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  // linux_keyvalue: release 0 press 1 autorepeat 2
  uint8_t lookup_result;
  if(linux_keycode < LINUX_KEYCODE_TO_PS2_SCANCODE_SINGLE_SIZE)
  {
    lookup_result = linux_keycode_to_ps2_scancode_lookup_single_byte_codeset2[linux_keycode];
    if(lookup_result == CODE_UNUSED)
      return PS2_ERROR_UNUSED_CODE;
    if(linux_keyvalue)
    {
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }

  if(linux_keycode == LINUX_KEYCODE_SYSRQ)
  {
    if(linux_keyvalue)
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x12, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x7c, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x7c, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0x12, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  else if(linux_keycode == LINUX_KEYCODE_PAUSE && linux_keyvalue)
  {
    if(ps2kb_write(0xe1, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x14, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x77, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xe1, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x14, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    if(ps2kb_write(0x77, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
      return PS2_ERROR_HOST_INHIBIT;
    return PS2_OK;
  }
  else if(linux_keycode >= 96 && linux_keycode <= 127)
  {
    lookup_result = linux_keycode_to_ps2_scancode_lookup_special_codeset2[linux_keycode-96];
    if(lookup_result == CODE_UNUSED)
      return PS2_ERROR_UNUSED_CODE;
    if(linux_keyvalue)
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 1, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    else
    {
      if(ps2kb_write(0xe0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(0xf0, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
      if(ps2kb_write(lookup_result, 0, PS2KB_WRITE_DEFAULT_TIMEOUT_MS))
        return PS2_ERROR_HOST_INHIBIT;
    }
    return PS2_OK;
  }
  return PS2_ERROR_UNKNOWN;
}

uint8_t ps2kb_press_key(uint8_t linux_keycode, uint8_t linux_keyvalue)
{
  if(ps2kb_data_reporting_enabled == 0)
    return PS2_ERROR_SCAN_DISABLED;
  switch(ps2kb_current_scancode_set)
  {
    case 1:
      return ps2kb_press_key_scancode_1(linux_keycode, linux_keyvalue);
    case 2:
      return ps2kb_press_key_scancode_2(linux_keycode, linux_keyvalue);
    default:
      return PS2_ERROR_UNKNOWN_CODE_SET;
  }
}
