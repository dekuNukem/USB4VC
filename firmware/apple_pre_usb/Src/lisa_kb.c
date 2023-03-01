#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lisa_kb.h"
#include "shared.h"
#include "helpers.h"
#include "delay_us.h"
#include "m0110a.h"

m0110a_cmd_buf lisa_buf;

#define LINUX_KEYCODE_TO_LISA_SIZE 128

uint32_t last_send_us;

const uint8_t lisa_byte_lookup[256] = {0xff, 0xf7, 0xfb, 0xf3, 0xfd, 0xf5, 0xf9, 0xf1, 0xfe, 0xf6, 0xfa, 0xf2, 0xfc, 0xf4, 0xf8, 0xf0, 0x7f, 0x77, 0x7b, 0x73, 0x7d, 0x75, 0x79, 0x71, 0x7e, 0x76, 0x7a, 0x72, 0x7c, 0x74, 0x78, 0x70, 0xbf, 0xb7, 0xbb, 0xb3, 0xbd, 0xb5, 0xb9, 0xb1, 0xbe, 0xb6, 0xba, 0xb2, 0xbc, 0xb4, 0xb8, 0xb0, 0x3f, 0x37, 0x3b, 0x33, 0x3d, 0x35, 0x39, 0x31, 0x3e, 0x36, 0x3a, 0x32, 0x3c, 0x34, 0x38, 0x30, 0xdf, 0xd7, 0xdb, 0xd3, 0xdd, 0xd5, 0xd9, 0xd1, 0xde, 0xd6, 0xda, 0xd2, 0xdc, 0xd4, 0xd8, 0xd0, 0x5f, 0x57, 0x5b, 0x53, 0x5d, 0x55, 0x59, 0x51, 0x5e, 0x56, 0x5a, 0x52, 0x5c, 0x54, 0x58, 0x50, 0x9f, 0x97, 0x9b, 0x93, 0x9d, 0x95, 0x99, 0x91, 0x9e, 0x96, 0x9a, 0x92, 0x9c, 0x94, 0x98, 0x90, 0x1f, 0x17, 0x1b, 0x13, 0x1d, 0x15, 0x19, 0x11, 0x1e, 0x16, 0x1a, 0x12, 0x1c, 0x14, 0x18, 0x10, 0xef, 0xe7, 0xeb, 0xe3, 0xed, 0xe5, 0xe9, 0xe1, 0xee, 0xe6, 0xea, 0xe2, 0xec, 0xe4, 0xe8, 0xe0, 0x6f, 0x67, 0x6b, 0x63, 0x6d, 0x65, 0x69, 0x61, 0x6e, 0x66, 0x6a, 0x62, 0x6c, 0x64, 0x68, 0x60, 0xaf, 0xa7, 0xab, 0xa3, 0xad, 0xa5, 0xa9, 0xa1, 0xae, 0xa6, 0xaa, 0xa2, 0xac, 0xa4, 0xa8, 0xa0, 0x2f, 0x27, 0x2b, 0x23, 0x2d, 0x25, 0x29, 0x21, 0x2e, 0x26, 0x2a, 0x22, 0x2c, 0x24, 0x28, 0x20, 0xcf, 0xc7, 0xcb, 0xc3, 0xcd, 0xc5, 0xc9, 0xc1, 0xce, 0xc6, 0xca, 0xc2, 0xcc, 0xc4, 0xc8, 0xc0, 0x4f, 0x47, 0x4b, 0x43, 0x4d, 0x45, 0x49, 0x41, 0x4e, 0x46, 0x4a, 0x42, 0x4c, 0x44, 0x48, 0x40, 0x8f, 0x87, 0x8b, 0x83, 0x8d, 0x85, 0x89, 0x81, 0x8e, 0x86, 0x8a, 0x82, 0x8c, 0x84, 0x88, 0x80, 0x0f, 0x07, 0x0b, 0x03, 0x0d, 0x05, 0x09, 0x01, 0x0e, 0x06, 0x0a, 0x02, 0x0c, 0x04, 0x08, 0x00};
const uint8_t linux_keycode_to_lisa_lookup[LINUX_KEYCODE_TO_LISA_SIZE] = 
{
  CODE_UNUSED, // KEY_RESERVED    0
  CODE_UNUSED, // KEY_ESC    1
  0x74, // KEY_1    2
  0x71, // KEY_2    3
  0x72, // KEY_3    4
  0x73, // KEY_4    5
  0x64, // KEY_5    6
  0x61, // KEY_6    7
  0x62, // KEY_7    8
  0x63, // KEY_8    9
  0x50, // KEY_9    10
  0x51, // KEY_0    11
  0X40, // KEY_MINUS    12
  0X41, // KEY_EQUAL    13
  0x45, // KEY_BACKSPACE    14
  0x78, // KEY_TAB    15
  0x75, // KEY_Q    16
  0x77, // KEY_W    17
  0x60, // KEY_E    18
  0x65, // KEY_R    19
  0x66, // KEY_T    20
  0x67, // KEY_Y    21
  0x52, // KEY_U    22
  0x53, // KEY_I    23
  0x5f, // KEY_O    24
  0X44, // KEY_P    25
  0x56, // KEY_LEFTBRACE    26
  0x57, // KEY_RIGHTBRACE    27
  0x48, // KEY_ENTER    28
  0x7c, // KEY_LEFTCTRL    29 !!!!! MAPPED TO LEFT OPTION
  0x70, // KEY_A    30
  0x76, // KEY_S    31
  0x7b, // KEY_D    32
  0x69, // KEY_F    33
  0x6a, // KEY_G    34
  0x6b, // KEY_H    35
  0x54, // KEY_J    36
  0x55, // KEY_K    37
  0x59, // KEY_L    38
  0x5a, // KEY_SEMICOLON    39
  0x5b, // KEY_APOSTROPHE    40
  0X68, // KEY_GRAVE    41
  0x7e, // KEY_LEFTSHIFT    42
  0x42, // KEY_BACKSLASH    43
  0x79, // KEY_Z    44
  0x7a, // KEY_X    45
  0x6d, // KEY_C    46
  0x6c, // KEY_V    47
  0x6e, // KEY_B    48
  0x6f, // KEY_N    49
  0x58, // KEY_M    50
  0x5d, // KEY_COMMA    51
  0x5e, // KEY_DOT    52
  0x4c, // KEY_SLASH    53
  0x7e, // KEY_RIGHTSHIFT    54
  0X22, // KEY_KPASTERISK    55
  0x7f, // KEY_LEFTALT    56 !!!!! MAPPED TO LEFT COMMAND
  0x5c, // KEY_SPACE    57
  0x7d, // KEY_CAPSLOCK    58
  CODE_UNUSED, // KEY_F1    59
  CODE_UNUSED, // KEY_F2    60
  CODE_UNUSED, // KEY_F3    61
  CODE_UNUSED, // KEY_F4    62
  CODE_UNUSED, // KEY_F5    63
  CODE_UNUSED, // KEY_F6    64
  CODE_UNUSED, // KEY_F7    65
  CODE_UNUSED, // KEY_F8    66
  CODE_UNUSED, // KEY_F9    67
  CODE_UNUSED, // KEY_F10    68
  0X20, // KEY_NUMLOCK    69 !!!!! MAPPED TO CLEAR
  CODE_UNUSED, // KEY_SCROLLLOCK    70
  0X24, // KEY_KP7    71
  0X26, // KEY_KP8    72
  0X25, // KEY_KP9    73
  0X21, // KEY_KPMINUS    74
  0X28, // KEY_KP4    75
  0x29, // KEY_KP5    76
  0X2A, // KEY_KP6    77
  0X2B, // KEY_KPPLUS    78
  0x4D, // KEY_KP1    79
  0X2D, // KEY_KP2    80
  0X2E, // KEY_KP3    81
  0x49, // KEY_KP0    82
  0X2C, // KEY_KPDOT    83
  CODE_UNUSED, // KEY_UNUSED    84
  CODE_UNUSED, // KEY_ZENKAKUHANKAKU    85
  CODE_UNUSED, // KEY_102ND    86
  CODE_UNUSED, // KEY_F11    87
  CODE_UNUSED, // KEY_F12    88
  CODE_UNUSED, // KEY_RO    89
  CODE_UNUSED, // KEY_KATAKANA    90
  CODE_UNUSED, // KEY_HIRAGANA    91
  CODE_UNUSED, // KEY_HENKAN    92
  CODE_UNUSED, // KEY_KATAKANAHIRAGANA    93
  CODE_UNUSED, // KEY_MUHENKAN    94
  CODE_UNUSED, // KEY_KPJPCOMMA    95
  0X2F, // KEY_KPENTER    96
  0x7c, // KEY_RIGHTCTRL    97 !!!!! MAPPED TO LEFT OPTION
  0X27, // KEY_KPSLASH    98
  CODE_UNUSED, // KEY_SYSRQ    99
  0x46, // KEY_RIGHTALT    100 !!!!! MAPPED TO LEFT COMMAND
  CODE_UNUSED, // KEY_LINEFEED    101
  CODE_UNUSED, // KEY_HOME    102
  0x27, // KEY_UP    103
  CODE_UNUSED, // KEY_PAGEUP    104
  0x22, // KEY_LEFT    105
  0x23, // KEY_RIGHT    106
  CODE_UNUSED, // KEY_END    107
  0x2b, // KEY_DOWN    108
  CODE_UNUSED, // KEY_PAGEDOWN    109
  CODE_UNUSED, // KEY_INSERT    110
  CODE_UNUSED, // KEY_DELETE    111
  CODE_UNUSED, // KEY_MACRO    112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER    116
  0X23, // KEY_KPEQUAL    117
  CODE_UNUSED, // KEY_KPPLUSMINUS    118
  CODE_UNUSED, // KEY_PAUSE    119
  CODE_UNUSED, // KEY_SCALE    120
  CODE_UNUSED, // KEY_KPCOMMA    121
  CODE_UNUSED, // KEY_HANGEUL    122
  CODE_UNUSED, // KEY_HANJA    123
  CODE_UNUSED, // KEY_YEN    124
  CODE_UNUSED, // KEY_LEFTMETA    125 !!!!! MAPPED TO LEFT COMMAND
  CODE_UNUSED, // KEY_RIGHTMETA    126 !!!!! MAPPED TO LEFT COMMAND
  CODE_UNUSED, // KEY_COMPOSE    127 !!!!! MAPPED TO LEFT COMMAND
};

lisa_handle lh;

#define LISA_DATA_HI() HAL_GPIO_WritePin(lh.data_port, lh.data_pin, GPIO_PIN_SET)
#define LISA_DATA_LOW() HAL_GPIO_WritePin(lh.data_port, lh.data_pin, GPIO_PIN_RESET)
#define LISA_READ_DATA_PIN() HAL_GPIO_ReadPin(lh.data_port, lh.data_pin)
#define LISA_TIMEOUT -1

void lisa_buf_add(uint8_t linux_keycode, uint8_t value)
{
  if(value == 2)
    return;
  printf("%d %d", linux_keycode, value);
  // printf("1");
  uint8_t lisa_code = linux_keycode_to_lisa_lookup[linux_keycode];
  if(lisa_code == CODE_UNUSED)
    return;
  if(value)
    lisa_code |= 0x80;
  m0110a_cmd_buf_add(&lisa_buf, lisa_code);
}

void lisa_buf_reset(void)
{
  m0110a_cmd_buf_init(&lisa_buf);
}

int32_t wait_until_change(int32_t timeout_us)
{
  uint32_t start_time = micros();
  uint8_t start_state = LISA_READ_DATA_PIN();
  uint32_t duration;
  while(LISA_READ_DATA_PIN() == start_state)
  {
    duration = micros() - start_time;
    if(timeout_us != 0 && duration > timeout_us)
      return LISA_TIMEOUT;
  }
  return duration;
}

void lisa_write_byte(uint8_t data)
{
  uint8_t to_write = lisa_byte_lookup[data];
  for (int i = 7; i >= 0; i--)
  {
    HAL_GPIO_WritePin(lh.data_port, lh.data_pin, (to_write >> i) & 1);
    delay_us(15);
    if(i == 4)
      delay_us(15);
  }
}

#define LISA_KB_ID0 0x80
uint8_t lisa_kb_id1 = 0xaf;

void lisa_kb_update(void)
{
  if(LISA_READ_DATA_PIN() == GPIO_PIN_SET)
    return;
  int32_t low_duration_us = wait_until_change(4500);
  if(low_duration_us == LISA_TIMEOUT) // soft reset
  {
    m0110a_cmd_buf_reset(&lisa_buf);
    m0110a_cmd_buf_add(&lisa_buf, LISA_KB_ID0);
    m0110a_cmd_buf_add(&lisa_buf, lisa_kb_id1); // UK layout
    return;
  }
  else if(low_duration_us <= 10)
    return;

  if(m0110a_cmd_buf_is_empty(&lisa_buf))
    return;

  uint8_t this_byte;
  m0110a_cmd_buf_peek(&lisa_buf, &this_byte);

  if(micros() - last_send_us < 75000 && this_byte != LISA_KB_ID0 && this_byte != lisa_kb_id1)
    return;

  m0110a_cmd_buf_pop(&lisa_buf);
  // now line is high
  delay_us(9);
  LISA_DATA_LOW();
  delay_us(15); // ACK pulse
  lisa_write_byte(this_byte);
  LISA_DATA_HI();
  last_send_us = micros();
  printf("s");
}
