#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "m0110a.h"
#include "delay_us.h"

#define CLK_HIGH_HOST_TO_KB 220
#define CLK_LOW_HOST_TO_KB 180
#define CLK_READ_DELAY_HOST_TO_KB 80

#define CLK_HIGH_KB_TO_HOST 170
#define CLK_LOW_KB_TO_HOST 160

#define M0110A_CLK_HI() HAL_GPIO_WritePin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin, GPIO_PIN_SET)
#define M0110A_CLK_LOW() HAL_GPIO_WritePin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin, GPIO_PIN_RESET)

#define M0110A_DATA_HI() HAL_GPIO_WritePin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin, GPIO_PIN_SET)
#define M0110A_DATA_LOW() HAL_GPIO_WritePin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin, GPIO_PIN_RESET)

#define M0110A_READ_DATA_PIN() HAL_GPIO_ReadPin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin)
#define M0110A_READ_CLK_PIN() HAL_GPIO_ReadPin(MAC_KB_CLK_GPIO_Port, MAC_KB_CLK_Pin)


const uint8_t linux_keycode_to_m0110a_scancode_lookup[LINUX_KEYCODE_TO_M0110A_SCANCODE_SIZE] = 
{
  CODE_UNUSED, // KEY_RESERVED    0
  CODE_UNUSED, // KEY_ESC    1
  0x25, // KEY_1    2
  0x27, // KEY_2    3
  0x29, // KEY_3    4
  0x2b, // KEY_4    5
  0x2f, // KEY_5    6
  0x2d, // KEY_6    7
  0x35, // KEY_7    8
  0x39, // KEY_8    9
  0x33, // KEY_9    10
  0x3b, // KEY_0    11
  0x37, // KEY_MINUS    12
  0x31, // KEY_EQUAL    13
  0x67, // KEY_BACKSPACE    14
  0x61, // KEY_TAB    15
  0x19, // KEY_Q    16
  0x1b, // KEY_W    17
  0x1d, // KEY_E    18
  0x1f, // KEY_R    19
  0x23, // KEY_T    20
  0x21, // KEY_Y    21
  0x41, // KEY_U    22
  0x45, // KEY_I    23
  0x3f, // KEY_O    24
  0x47, // KEY_P    25
  0x43, // KEY_LEFTBRACE    26
  0x3d, // KEY_RIGHTBRACE    27
  0x49, // KEY_ENTER    28
  0x75, // KEY_LEFTCTRL    29 !!!!! MAPPED TO LEFT OPTION
  0x01, // KEY_A    30
  0x03, // KEY_S    31
  0x05, // KEY_D    32
  0x07, // KEY_F    33
  0x0b, // KEY_G    34
  0x09, // KEY_H    35
  0x4d, // KEY_J    36
  0x51, // KEY_K    37
  0x4b, // KEY_L    38
  0x53, // KEY_SEMICOLON    39
  0x4f, // KEY_APOSTROPHE    40
  0x65, // KEY_GRAVE    41
  0x71, // KEY_LEFTSHIFT    42
  0x55, // KEY_BACKSLASH    43
  0x0d, // KEY_Z    44
  0x0f, // KEY_X    45
  0x11, // KEY_C    46
  0x13, // KEY_V    47
  0x17, // KEY_B    48
  0x5b, // KEY_N    49
  0x5d, // KEY_M    50
  0x57, // KEY_COMMA    51
  0x5f, // KEY_DOT    52
  0x59, // KEY_SLASH    53
  0x71, // KEY_RIGHTSHIFT    54
  0x05, // KEY_KPASTERISK    55
  0x6f, // KEY_LEFTALT    56 !!!!! MAPPED TO LEFT COMMAND
  0x63, // KEY_SPACE    57
  0x73, // KEY_CAPSLOCK    58
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
  0x0f, // KEY_NUMLOCK    69 !!!!! MAPPED TO CLEAR
  CODE_UNUSED, // KEY_SCROLLLOCK    70
  0x33, // KEY_KP7    71
  0x37, // KEY_KP8    72
  0x39, // KEY_KP9    73
  0x1d, // KEY_KPMINUS    74
  0x2d, // KEY_KP4    75
  0x2f, // KEY_KP5    76
  0x31, // KEY_KP6    77
  0x0d, // KEY_KPPLUS    78
  0x27, // KEY_KP1    79
  0x29, // KEY_KP2    80
  0x2b, // KEY_KP3    81
  0x25, // KEY_KP0    82
  0x03, // KEY_KPDOT    83
  CODE_UNUSED, // KEY_UNUSED    84
  CODE_UNUSED, // KEY_ZENKAKUHANKAKU    85
  0x55, // KEY_102ND    86
  CODE_UNUSED, // KEY_F11    87
  CODE_UNUSED, // KEY_F12    88
  CODE_UNUSED, // KEY_RO    89
  CODE_UNUSED, // KEY_KATAKANA    90
  CODE_UNUSED, // KEY_HIRAGANA    91
  CODE_UNUSED, // KEY_HENKAN    92
  CODE_UNUSED, // KEY_KATAKANAHIRAGANA    93
  CODE_UNUSED, // KEY_MUHENKAN    94
  CODE_UNUSED, // KEY_KPJPCOMMA    95
  0x19, // KEY_KPENTER    96
  0x75, // KEY_RIGHTCTRL    97 !!!!! MAPPED TO LEFT OPTION
  0x1b, // KEY_KPSLASH    98
  CODE_UNUSED, // KEY_SYSRQ    99
  0x6f, // KEY_RIGHTALT    100 !!!!! MAPPED TO LEFT COMMAND
  CODE_UNUSED, // KEY_LINEFEED    101
  CODE_UNUSED, // KEY_HOME    102
  0x1b, // KEY_UP    103
  CODE_UNUSED, // KEY_PAGEUP    104
  0x0d, // KEY_LEFT    105
  0x05, // KEY_RIGHT    106
  CODE_UNUSED, // KEY_END    107
  0x11, // KEY_DOWN    108
  CODE_UNUSED, // KEY_PAGEDOWN    109
  CODE_UNUSED, // KEY_INSERT    110
  CODE_UNUSED, // KEY_DELETE    111
  CODE_UNUSED, // KEY_MACRO    112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER    116
  CODE_UNUSED, // KEY_KPEQUAL    117
  CODE_UNUSED, // KEY_KPPLUSMINUS    118
  CODE_UNUSED, // KEY_PAUSE    119
  CODE_UNUSED, // KEY_SCALE    120
  CODE_UNUSED, // KEY_KPCOMMA    121
  CODE_UNUSED, // KEY_HANGEUL    122
  CODE_UNUSED, // KEY_HANJA    123
  CODE_UNUSED, // KEY_YEN    124
  0x6f, // KEY_LEFTMETA    125 !!!!! MAPPED TO LEFT COMMAND
  0x6f, // KEY_RIGHTMETA    126 !!!!! MAPPED TO LEFT COMMAND
  0x6f, // KEY_COMPOSE    127 !!!!! MAPPED TO LEFT COMMAND
};

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

uint8_t m0110a_read(uint8_t* result)
{
	uint8_t data = 0x00;
  uint8_t bit = 0x80;

	while(bit)
	{
		M0110A_CLK_LOW();
		delay_us(CLK_LOW_HOST_TO_KB);
		M0110A_CLK_HI();
		delay_us(CLK_READ_DELAY_HOST_TO_KB); // "The keyboard reads the data bit 80 µs after the rising edge of the Keyboard Clock signal." page 282, Guide to Macintosh Family Hardware 2nd edition.
		if(M0110A_READ_DATA_PIN() == GPIO_PIN_SET)
      data = data | bit;
		bit = bit >> 1;
		delay_us(CLK_HIGH_HOST_TO_KB - CLK_READ_DELAY_HOST_TO_KB);
	}
	*result = data;
  return M0110A_OK;
}

uint8_t wait_for_data_idle(uint16_t timeout_ms)
{
	uint32_t entry_time = HAL_GetTick();
	while(1)
	{
		if(HAL_GetTick() - entry_time > timeout_ms)
			return M0110A_TIMEOUT;
		if(HAL_GPIO_ReadPin(MAC_KB_DATA_GPIO_Port, MAC_KB_DATA_Pin) == GPIO_PIN_SET)
			break;
	}
	return M0110A_OK;
}

uint8_t m0110a_read_host_cmd(uint8_t* result, uint16_t timeout_ms)
{
	if(m0110a_get_line_status() == M0110A_LINE_IDLE)
		return M0110A_LINE_IDLE;
	m0110a_read(result);
	return wait_for_data_idle(timeout_ms);
}

uint8_t m0110a_write(uint8_t data)
{
	M0110A_CLK_HI();
	if(wait_for_data_idle(200) == M0110A_TIMEOUT)
		return M0110A_TIMEOUT;
	// clk is high at the start
	for(int i=7; i>=0; i--)
	{
		// write data bit on falling edge, host latches on rising edge
		if(data & (1 << i))
			M0110A_DATA_HI();
		else
			M0110A_DATA_LOW();
		M0110A_CLK_LOW();
		delay_us(CLK_LOW_KB_TO_HOST);
		M0110A_CLK_HI();
		delay_us(CLK_HIGH_KB_TO_HOST);
	}
  return M0110A_OK;
}


#define KEY_CAPSLOCK  58

#define KEY_KPASTERISK  55
#define KEY_KPPLUS  78
#define KEY_KPSLASH  98

#define KEY_NUMLOCK  69
#define KEY_KP7  71
#define KEY_KP8  72
#define KEY_KP9  73
#define KEY_KPMINUS  74
#define KEY_KP4  75
#define KEY_KP5  76
#define KEY_KP6  77
#define KEY_KP1  79
#define KEY_KP2  80
#define KEY_KP3  81
#define KEY_KP0  82
#define KEY_KPDOT  83
#define KEY_KPENTER  96
#define KEY_UP 103
#define KEY_LEFT 105
#define KEY_RIGHT 106
#define KEY_DOWN 108

uint8_t is_capslock_on;

uint8_t make_m0110a_scancode(uint8_t linux_key_code, uint8_t linux_key_value, m0110a_cmd_buf *lb)
{
  uint8_t lookup_result = CODE_UNUSED;
  if(linux_key_code < LINUX_KEYCODE_TO_M0110A_SCANCODE_SIZE)
    lookup_result = linux_keycode_to_m0110a_scancode_lookup[linux_key_code];
  // m0110a does not generate key autorepeat itself
  if(linux_key_value == 2 || lookup_result == CODE_UNUSED)
    return M0110A_UNKNOWN_CODE;

  if(linux_key_code == KEY_CAPSLOCK)
  {
  	if(linux_key_value == 0)
  		return M0110A_UNKNOWN_CODE;
  	is_capslock_on = (is_capslock_on + 1) % 2;
  	if(is_capslock_on)
      m0110a_cmd_buf_add(lb, lookup_result);
    else
      m0110a_cmd_buf_add(lb, lookup_result | 0x80);
  }
  else if(linux_key_code == KEY_KPASTERISK || linux_key_code == KEY_KPPLUS || linux_key_code == KEY_KPSLASH)
  {
    if(linux_key_value)
    {
      m0110a_cmd_buf_add(lb, 0x71);
      m0110a_cmd_buf_add(lb, 0x79);
      m0110a_cmd_buf_add(lb, lookup_result);
    }
    else
    {
      m0110a_cmd_buf_add(lb, 0xf1);
      m0110a_cmd_buf_add(lb, 0x79);
      m0110a_cmd_buf_add(lb, lookup_result | 0x80);
    }
  }
  else if(linux_key_code == KEY_NUMLOCK || 
    linux_key_code == KEY_KP7 || 
    linux_key_code == KEY_KP8 || 
    linux_key_code == KEY_KP9 || 
    linux_key_code == KEY_KPMINUS || 
    linux_key_code == KEY_KP4 || 
    linux_key_code == KEY_KP5 || 
    linux_key_code == KEY_KP6 || 
    linux_key_code == KEY_KP1 || 
    linux_key_code == KEY_KP2 || 
    linux_key_code == KEY_KP3 || 
    linux_key_code == KEY_KP0 || 
    linux_key_code == KEY_KPDOT || 
    linux_key_code == KEY_KPENTER || 
    linux_key_code == KEY_UP || 
    linux_key_code == KEY_LEFT || 
    linux_key_code == KEY_RIGHT || 
    linux_key_code == KEY_DOWN)
  {
    if(linux_key_value)
    {
      m0110a_cmd_buf_add(lb, 0x79);
      m0110a_cmd_buf_add(lb, lookup_result);
    }
    else
    {
      m0110a_cmd_buf_add(lb, 0x79);
      m0110a_cmd_buf_add(lb, lookup_result | 0x80);
    }
    return M0110A_OK;
  }
  else
  {
    if(linux_key_value)
      m0110a_cmd_buf_add(lb, lookup_result);
    else
      m0110a_cmd_buf_add(lb, lookup_result | 0x80);
  }
  return M0110A_OK;
}

// -----------------------

void m0110a_cmd_buf_reset(m0110a_cmd_buf *lb)
{
  lb->head = 0;
  lb->tail = 0;
  memset(lb->cmd_buf, 0, M0110A_KB_TO_PC_CMD_BUF_SIZE);
}

uint8_t m0110a_cmd_buf_is_full(m0110a_cmd_buf *lb)
{
	return lb->tail == (lb->head + 1) % M0110A_KB_TO_PC_CMD_BUF_SIZE;
}

uint8_t m0110a_cmd_buf_is_empty(m0110a_cmd_buf *lb)
{
	return lb->tail == lb->head;
}

uint8_t m0110a_cmd_buf_add(m0110a_cmd_buf *lb, uint8_t code)
{
	if(m0110a_cmd_buf_is_full(lb))
		return 1;
	lb->cmd_buf[lb->head] = code;
	lb->head = (lb->head + 1) % M0110A_KB_TO_PC_CMD_BUF_SIZE;
	return 0;
}

uint8_t m0110a_cmd_buf_peek(m0110a_cmd_buf *lb, uint8_t* code)
{
	if(m0110a_cmd_buf_is_empty(lb))
		return 1;
	*code = lb->cmd_buf[lb->tail];
	return 0;
}

void m0110a_cmd_buf_pop(m0110a_cmd_buf *lb)
{
	if(!m0110a_cmd_buf_is_empty(lb))
		lb->tail = (lb->tail + 1) % M0110A_KB_TO_PC_CMD_BUF_SIZE;
}

void m0110a_cmd_buf_init(m0110a_cmd_buf *lb)
{
  lb->cmd_buf = malloc(M0110A_KB_TO_PC_CMD_BUF_SIZE);
  m0110a_cmd_buf_reset(lb);
}
