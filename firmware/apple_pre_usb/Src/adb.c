#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"
#include "adb.h"
#include "delay_us.h"

GPIO_TypeDef* adb_psw_port;
uint16_t adb_psw_pin;
GPIO_TypeDef* adb_data_port;
uint16_t adb_data_pin;
uint16_t adb_kb_reg2 = 0xffff; // all key released, all LED off
uint8_t adb_mouse_current_addr, adb_kb_current_addr, adb_rw_in_progress;
uint8_t adb_kb_enabled, adb_mouse_enabled;

#define ADB_PSW_HI() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_SET)
#define ADB_PSW_LOW() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_RESET)

#define ADB_DATA_HI() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_SET)
#define ADB_DATA_LOW() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_RESET)

#define ADB_READ_DATA_PIN() HAL_GPIO_ReadPin(adb_data_port, adb_data_pin)

const uint8_t linux_ev_to_adb_lookup[EV_TO_ADB_LOOKUP_SIZE] = 
{
  ADB_KEY_UNKNOWN, // EV0 KEY_RESERVED
  53, // EV1 KEY_ESC
  18, // EV2 KEY_1
  19, // EV3 KEY_2
  20, // EV4 KEY_3
  21, // EV5 KEY_4
  23, // EV6 KEY_5
  22, // EV7 KEY_6
  26, // EV8 KEY_7
  28, // EV9 KEY_8
  25, // EV10 KEY_9
  29, // EV11 KEY_0
  27, // EV12 KEY_MINUS
  24, // EV13 KEY_EQUAL
  51, // EV14 KEY_BACKSPACE
  48, // EV15 KEY_TAB
  12, // EV16 KEY_Q
  13, // EV17 KEY_W
  14, // EV18 KEY_E
  15, // EV19 KEY_R
  17, // EV20 KEY_T
  16, // EV21 KEY_Y
  32, // EV22 KEY_U
  34, // EV23 KEY_I
  31, // EV24 KEY_O
  35, // EV25 KEY_P
  33, // EV26 KEY_LEFTBRACE
  30, // EV27 KEY_RIGHTBRACE
  36, // EV28 KEY_ENTER
  54, // EV29 KEY_LEFTCTRL
  0, // EV30 KEY_A
  1, // EV31 KEY_S
  2, // EV32 KEY_D
  3, // EV33 KEY_F
  5, // EV34 KEY_G
  4, // EV35 KEY_H
  38, // EV36 KEY_J
  40, // EV37 KEY_K
  37, // EV38 KEY_L
  41, // EV39 KEY_SEMICOLON
  39, // EV40 KEY_APOSTROPHE
  50, // EV41 KEY_GRAVE
  56, // EV42 KEY_LEFTSHIFT
  ADB_KEY_UNKNOWN, // EV43 KEY_BACKSLASH
  6, // EV44 KEY_Z
  7, // EV45 KEY_X
  8, // EV46 KEY_C
  9, // EV47 KEY_V
  11, // EV48 KEY_B
  45, // EV49 KEY_N
  46, // EV50 KEY_M
  43, // EV51 KEY_COMMA
  47, // EV52 KEY_DOT
  44, // EV53 KEY_SLASH
  123, // EV54 KEY_RIGHTSHIFT
  67, // EV55 KEY_KPASTERISK
  58, // EV56 KEY_LEFTALT
  49, // EV57 KEY_SPACE
  57, // EV58 KEY_CAPSLOCK
  122, // EV59 KEY_F1
  120, // EV60 KEY_F2
  99, // EV61 KEY_F3
  118, // EV62 KEY_F4
  96, // EV63 KEY_F5
  97, // EV64 KEY_F6
  98, // EV65 KEY_F7
  100, // EV66 KEY_F8
  101, // EV67 KEY_F9
  109, // EV68 KEY_F10
  71, // EV69 KEY_NUMLOCK
  ADB_KEY_UNKNOWN, // EV70 KEY_SCROLLLOCK
  89, // EV71 KEY_KP7
  91, // EV72 KEY_KP8
  92, // EV73 KEY_KP9
  78, // EV74 KEY_KPMINUS
  86, // EV75 KEY_KP4
  87, // EV76 KEY_KP5
  88, // EV77 KEY_KP6
  69, // EV78 KEY_KPPLUS
  83, // EV79 KEY_KP1
  84, // EV80 KEY_KP2
  85, // EV81 KEY_KP3
  82, // EV82 KEY_KP0
  65, // EV83 KEY_KPDOT
  ADB_KEY_UNKNOWN, // EV84 UNKNOWN
  ADB_KEY_UNKNOWN, // EV85 KEY_ZENKAKUHANKAKU
  10, // EV86 KEY_102ND
  103, // EV87 KEY_F11
  111, // EV88 KEY_F12
  ADB_KEY_UNKNOWN, // EV89 KEY_RO
  ADB_KEY_UNKNOWN, // EV90 KEY_KATAKANA
  ADB_KEY_UNKNOWN, // EV91 KEY_HIRAGANA
  ADB_KEY_UNKNOWN, // EV92 KEY_HENKAN
  ADB_KEY_UNKNOWN, // EV93 KEY_KATAKANAHIRAGANA
  ADB_KEY_UNKNOWN, // EV94 KEY_MUHENKAN
  ADB_KEY_UNKNOWN, // EV95 KEY_KPJPCOMMA
  76, // EV96 KEY_KPENTER
  125, // EV97 KEY_RIGHTCTRL
  75, // EV98 KEY_KPSLASH
  ADB_KEY_UNKNOWN, // EV99 KEY_SYSRQ
  124, // EV100 KEY_RIGHTALT
  ADB_KEY_UNKNOWN, // EV101 KEY_LINEFEED
  115, // EV102 KEY_HOME
  62, // EV103 KEY_UP
  116, // EV104 KEY_PAGEUP
  59, // EV105 KEY_LEFT
  60, // EV106 KEY_RIGHT
  119, // EV107 KEY_END
  61, // EV108 KEY_DOWN
  121, // EV109 KEY_PAGEDOWN
  114, // EV110 KEY_INSERT
  117, // EV111 KEY_DELETE
  ADB_KEY_UNKNOWN, // EV112 KEY_MACRO
  ADB_KEY_UNKNOWN, // EV113 KEY_MUTE
  ADB_KEY_UNKNOWN, // EV114 KEY_VOLUMEDOWN
  ADB_KEY_UNKNOWN, // EV115 KEY_VOLUMEUP
  127, // EV116 KEY_POWER
  81, // EV117 KEY_KPEQUAL
  ADB_KEY_UNKNOWN, // EV118 KEY_KPPLUSMINUS
  ADB_KEY_UNKNOWN, // EV119 KEY_PAUSE
  ADB_KEY_UNKNOWN, // EV120 KEY_SCALE
  ADB_KEY_UNKNOWN, // EV121 KEY_KPCOMMA
  ADB_KEY_UNKNOWN, // EV122 KEY_HANGEUL
  ADB_KEY_UNKNOWN, // EV123 KEY_HANJA
  ADB_KEY_UNKNOWN, // EV124 KEY_YEN
  55, // EV125 KEY_LEFTMETA
  ADB_KEY_UNKNOWN, // EV126 KEY_RIGHTMETA
  ADB_KEY_UNKNOWN, // EV127 KEY_COMPOSE
  ADB_KEY_UNKNOWN, // EV128 KEY_STOP
  ADB_KEY_UNKNOWN, // EV129 KEY_AGAIN
  ADB_KEY_UNKNOWN, // EV130 KEY_PROPS
  ADB_KEY_UNKNOWN, // EV131 KEY_UNDO
  ADB_KEY_UNKNOWN, // EV132 KEY_FRONT
  ADB_KEY_UNKNOWN, // EV133 KEY_COPY
  ADB_KEY_UNKNOWN, // EV134 KEY_OPEN
  ADB_KEY_UNKNOWN, // EV135 KEY_PASTE
  ADB_KEY_UNKNOWN, // EV136 KEY_FIND
  ADB_KEY_UNKNOWN, // EV137 KEY_CUT
  ADB_KEY_UNKNOWN, // EV138 KEY_HELP
  ADB_KEY_UNKNOWN, // EV139 KEY_MENU
  ADB_KEY_UNKNOWN, // EV140 KEY_CALC
  ADB_KEY_UNKNOWN, // EV141 KEY_SETUP
  ADB_KEY_UNKNOWN, // EV142 KEY_SLEEP
  ADB_KEY_UNKNOWN, // EV143 KEY_WAKEUP
  ADB_KEY_UNKNOWN, // EV144 KEY_FILE
  ADB_KEY_UNKNOWN, // EV145 KEY_SENDFILE
  ADB_KEY_UNKNOWN, // EV146 KEY_DELETEFILE
  ADB_KEY_UNKNOWN, // EV147 KEY_XFER
  ADB_KEY_UNKNOWN, // EV148 KEY_PROG1
  ADB_KEY_UNKNOWN, // EV149 KEY_PROG2
  ADB_KEY_UNKNOWN, // EV150 KEY_WWW
  ADB_KEY_UNKNOWN, // EV151 KEY_MSDOS
  ADB_KEY_UNKNOWN, // EV152 KEY_COFFEE
  ADB_KEY_UNKNOWN, // EV153 KEY_ROTATE_DISPLAY
  ADB_KEY_UNKNOWN, // EV154 KEY_CYCLEWINDOWS
  ADB_KEY_UNKNOWN, // EV155 KEY_MAIL
  ADB_KEY_UNKNOWN, // EV156 KEY_BOOKMARKS
  ADB_KEY_UNKNOWN, // EV157 KEY_COMPUTER
  ADB_KEY_UNKNOWN, // EV158 KEY_BACK
  ADB_KEY_UNKNOWN, // EV159 KEY_FORWARD
  ADB_KEY_UNKNOWN, // EV160 KEY_CLOSECD
  ADB_KEY_UNKNOWN, // EV161 KEY_EJECTCD
  ADB_KEY_UNKNOWN, // EV162 KEY_EJECTCLOSECD
  ADB_KEY_UNKNOWN, // EV163 KEY_NEXTSONG
  ADB_KEY_UNKNOWN, // EV164 KEY_PLAYPAUSE
  ADB_KEY_UNKNOWN, // EV165 KEY_PREVIOUSSONG
  ADB_KEY_UNKNOWN, // EV166 KEY_STOPCD
  ADB_KEY_UNKNOWN, // EV167 KEY_RECORD
  ADB_KEY_UNKNOWN, // EV168 KEY_REWIND
  ADB_KEY_UNKNOWN, // EV169 KEY_PHONE
  ADB_KEY_UNKNOWN, // EV170 KEY_ISO
  ADB_KEY_UNKNOWN, // EV171 KEY_CONFIG
  ADB_KEY_UNKNOWN, // EV172 KEY_HOMEPAGE
  ADB_KEY_UNKNOWN, // EV173 KEY_REFRESH
  ADB_KEY_UNKNOWN, // EV174 KEY_EXIT
  ADB_KEY_UNKNOWN, // EV175 KEY_MOVE
  ADB_KEY_UNKNOWN, // EV176 KEY_EDIT
  ADB_KEY_UNKNOWN, // EV177 KEY_SCROLLUP
  ADB_KEY_UNKNOWN, // EV178 KEY_SCROLLDOWN
  ADB_KEY_UNKNOWN, // EV179 KEY_KPLEFTPAREN
  ADB_KEY_UNKNOWN, // EV180 KEY_KPRIGHTPAREN
  ADB_KEY_UNKNOWN, // EV181 KEY_NEW
  ADB_KEY_UNKNOWN, // EV182 KEY_REDO
  105, // EV183 KEY_F13
  107, // EV184 KEY_F14
  113, // EV185 KEY_F15
};

void adb_release_lines(void)
{
  ADB_PSW_HI();
  ADB_DATA_HI();
}

void adb_reset(void)
{
  adb_kb_current_addr = ADB_KB_DEFAULT_ADDR;
  adb_mouse_current_addr = ADB_MOUSE_DEFAULT_ADDR;
  adb_release_lines();
}

void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin)
{
  adb_psw_port = psw_port;
  adb_psw_pin = psw_pin;
  adb_data_port = data_port;
  adb_data_pin = data_pin;
  adb_reset();
}

int32_t adb_wait_until_change(int32_t timeout_us)
{
  uint32_t start_time = micros();
  uint8_t start_state = ADB_READ_DATA_PIN();
  uint32_t duration;
  while(ADB_READ_DATA_PIN() == start_state)
  {
    duration = micros() - start_time;
    if(timeout_us != 0 && duration > timeout_us)
      return ADB_TIMEOUT;
  }
  return duration;
}

uint8_t look_for_atten(void)
{
  // if ADB data line is high
  if(ADB_READ_DATA_PIN() == GPIO_PIN_SET && adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_TIMEOUT;
  // now data line is low
  adb_rw_in_progress = 1;
  int32_t atten_duration = adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(atten_duration > 2000 || atten_duration == ADB_TIMEOUT)
    return ADB_LINE_STATUS_RESET;
  if(atten_duration < 500)
    return ADB_LINE_STATUS_BUSY;  // not an attention signal
	return ADB_LINE_STATUS_ATTEN;
}

uint8_t adb_read_bit(void)
{
  if(ADB_READ_DATA_PIN() != GPIO_PIN_RESET)
    return ADB_ERROR;
  int32_t lo_time = adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  int32_t hi_time = adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(lo_time == ADB_TIMEOUT || hi_time == ADB_TIMEOUT)
    return ADB_ERROR;
  return hi_time > lo_time;
}

uint8_t adb_recv_cmd(uint8_t* data)
{
  *data = 0;
  uint8_t atten_result = look_for_atten();
  if(atten_result != ADB_LINE_STATUS_ATTEN)
    return atten_result;
  int32_t sync_duration = adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(sync_duration > 90 || sync_duration < 50)
    return ADB_ERROR;
  
  uint8_t temp = 0;
  PCARD_BUSY_HI();
  for (int i = 0; i < 8; ++i)
  {
    uint8_t this_bit = adb_read_bit();
    if(this_bit == ADB_ERROR)
      return ADB_ERROR;
    temp |= this_bit << (7 - i);
  }
  PCARD_BUSY_LOW();
  *data = temp;
  return ADB_OK;
}

uint8_t adb_write_byte(uint8_t data)
{
  for (int i = 0; i < 8; ++i)
  {
    if((data >> (7-i)) & 0x1)
    {
      ADB_DATA_LOW();
      delay_us(ADB_CLK_35);
      ADB_DATA_HI();
      // if the line doesnt actually go high, then there has been a bus collision
      if(ADB_READ_DATA_PIN() != GPIO_PIN_SET) 
        return ADB_LINE_STATUS_COLLISION;
      delay_us(ADB_CLK_65);
    }
    else
    {
      ADB_DATA_LOW();
      delay_us(ADB_CLK_65);
      ADB_DATA_HI();
      if(ADB_READ_DATA_PIN() != GPIO_PIN_SET)
        return ADB_LINE_STATUS_COLLISION;
      delay_us(ADB_CLK_35);
    }
  }
  return ADB_OK;
}

uint8_t adb_write_16(uint16_t data)
{
  PCARD_BUSY_HI();
  if(adb_write_byte((uint8_t)(data >> 8)) == ADB_LINE_STATUS_COLLISION) // MSB
  {
    PCARD_BUSY_LOW();
    return ADB_LINE_STATUS_COLLISION;
  }
  if(adb_write_byte((uint8_t)(data & 0xff)) == ADB_LINE_STATUS_COLLISION) // LSB
  {
    PCARD_BUSY_LOW();
    return ADB_LINE_STATUS_COLLISION;
  }
  PCARD_BUSY_LOW();
  return ADB_OK;
}

// to be called right after a LISTEN command from host
uint8_t adb_send_response_16b(uint16_t data)
{
  adb_rw_in_progress = 1;
  delay_us(200); // stop-to-start time
  ADB_DATA_LOW();
  delay_us(ADB_CLK_35);
  ADB_DATA_HI();
  delay_us(ADB_CLK_65);
  if(adb_write_16(data) == ADB_LINE_STATUS_COLLISION)
  {
    adb_rw_in_progress = 0;
    return ADB_LINE_STATUS_COLLISION;
  }
  ADB_DATA_LOW();
  delay_us(ADB_CLK_65);
  ADB_DATA_HI();
  adb_rw_in_progress = 0;
  return ADB_OK;
}

uint8_t adb_listen_16b(uint16_t* data)
{
  *data = 0;
  if(ADB_READ_DATA_PIN() != GPIO_PIN_SET)
    return ADB_ERROR;
  // stop-to-start
  if(adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;
  // start pulse
  if(adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;
  if(adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;

  uint16_t temp = 0;
  PCARD_BUSY_HI();
  for (int i = 0; i < 16; ++i)
  {
    uint8_t this_bit = adb_read_bit();
    if(this_bit == ADB_ERROR)
      return ADB_ERROR;
    temp |= this_bit << (15 - i);
  }
  PCARD_BUSY_LOW();
  adb_wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  *data = temp;
  return ADB_OK;
}

// addr 2 keyboard, 3 mouse
uint8_t parse_adb_cmd(uint8_t data)
{
  uint8_t addr = data >> 4;
  uint8_t cmd = (data >> 2) & 0x3;
  uint8_t reg = data & 0x3;

  if(addr == 0)
    return ADB_ERROR;

  if(cmd == ADB_CMD_TYPE_TALK && reg == 3 && addr == adb_mouse_current_addr && adb_mouse_enabled)
  {
    uint16_t response = 0x6001; // 0110 0000 0000 0001, device handler 0x1, 100dps apple desktop bus mouse
    uint16_t rand_id = (rand() % 0xf) << 8;
    response |= rand_id;
    adb_send_response_16b(response);
  }

  if(cmd == ADB_CMD_TYPE_TALK && reg == 3 && addr == adb_kb_current_addr && adb_kb_enabled)
  {
    uint16_t response = 0x6005; // 0110 0000 0000 0101, device handler 0x5, appledesign keyboard
    uint16_t rand_id = (rand() % 0xf) << 8;
    response |= rand_id;
    adb_send_response_16b(response);
  }

  if(cmd == ADB_CMD_TYPE_LISTEN && reg == 3 && addr == adb_mouse_current_addr)
  {
    uint16_t host_cmd;
    adb_listen_16b(&host_cmd);
    if((host_cmd & ADB_CHANGE_ADDR) == ADB_CHANGE_ADDR)
      adb_mouse_current_addr = (host_cmd & 0xf00) >> 8;
  }

  if(cmd == ADB_CMD_TYPE_LISTEN && reg == 3 && addr == adb_kb_current_addr)
  {
    uint16_t host_cmd;
    adb_listen_16b(&host_cmd);
    if((host_cmd & ADB_CHANGE_ADDR) == ADB_CHANGE_ADDR)
      adb_kb_current_addr = (host_cmd & 0xf00) >> 8;
  }

  if(cmd == ADB_CMD_TYPE_TALK && reg == 0 && addr == adb_mouse_current_addr)
    return ADB_MOUSE_POLL;

  if(cmd == ADB_CMD_TYPE_TALK && reg == 0 && addr == adb_kb_current_addr)
    return ADB_KB_POLL;

  if(cmd == ADB_CMD_TYPE_TALK && reg == 2 && addr == adb_kb_current_addr)
    return ADB_KB_POLL_REG2;

  if(cmd == ADB_CMD_TYPE_LISTEN && reg == 2 && addr == adb_kb_current_addr)
  {
    DEBUG1_HI();
    adb_listen_16b(&adb_kb_reg2);
    DEBUG1_LOW();
    return ADB_KB_CHANGE_LED;
  }
  return ADB_OK;
}

void send_srq(void)
{
  ADB_DATA_LOW();
  delay_us(300);
  ADB_DATA_HI();
}




