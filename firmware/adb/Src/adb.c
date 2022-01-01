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
uint8_t adb_mouse_current_addr, adb_kb_current_addr;

#define ADB_PSW_HI() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_SET)
#define ADB_PSW_LOW() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_RESET)

#define ADB_DATA_HI() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_SET)
#define ADB_DATA_LOW() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_RESET)

#define ADB_READ_DATA_PIN() HAL_GPIO_ReadPin(adb_data_port, adb_data_pin)

#define ADB_DEFAULT_TIMEOUT_US 10000

uint8_t adb_kb_scancode[] = 
{
    0x00, // A
    0x0B, // B
    0x08, // C
    0x02, // D
    0x0E, // E
    0x03, // F
    0x05, // G
    0x04, // H
    0x22, // I
    0x26, // J
    0x28, // K
    0x25, // L
    0x2E, // M
    0x2D, // N
    0x1F, // O
    0x23, // P
    0x0C, // Q
    0x0F, // R
    0x01, // S
    0x11, // T
    0x20, // U
    0x09, // V
    0x0D, // W
    0x07, // X
    0x10, // Y
    0x06, // Z
    0x1D, // 0
    0x12, // 1
    0x13, // 2
    0x14, // 3
    0x15, // 4
    0x17, // 5
    0x16, // 6
    0x1A, // 7
    0x1C, // 8
    0x19, // 9
    0x32, // `
    0x1B, // -
    0x18, // =
    0x2A, // backslash
    0x33, // bkspc
    0x31, // space
    0x30, // tab
    0x39, // caps
    0x38, // left shift
    0x7B, // right shift
    0x3A, // left alt = opt
    0x24, // enter
    0x2B, // ,
    0x2F, // .
    0x2C, // /
    0x36, // L control
    0x35, // ESC
    0x27, // '
    0x29, // ;
    0x43, // keypad *
    0x4E, // keypad -
    0x45, // keypad +
    0x41, // keypad .
    0x52, // keypad 0
    0x53, // keypad 1
    0x54, // keypad 2
    0x55, // keypad 3
    0x56, // keypad 4
    0x57, // keypad 5
    0x58, // keypad 6
    0x59, // keypad 7
    0x5B, // keypad 8
    0x5C, // keypad 9
    0x7A, // F1
    0x78, // F2
    0x63, // F3
    0x76, // F4
    0x60, // F5
    0x61, // F6
    0x62, // F7
    0x64, // F8
    0x65, // F9
    0x6D, // F10
    0x67, // F11
    0x6F, // F12
    0x21, // [
    0x1E, // ]
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

int32_t wait_until_change(int32_t timeout_us)
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
  if(ADB_READ_DATA_PIN() == GPIO_PIN_SET)
    wait_until_change(0);

  // now data line is low
  int32_t atten_duration = wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(atten_duration > 2000 || atten_duration == ADB_TIMEOUT)
    return ADB_LINE_STATUS_RESET;
  if(atten_duration < 600)
    return ADB_LINE_STATUS_BUSY;  // not an attention signal
	return ADB_LINE_STATUS_ATTEN;
}

uint8_t adb_read_bit(void)
{
  if(ADB_READ_DATA_PIN() != GPIO_PIN_RESET)
    return ADB_ERROR;
  int32_t lo_time = wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  int32_t hi_time = wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(lo_time == ADB_TIMEOUT || hi_time == ADB_TIMEOUT)
    return ADB_ERROR;
  return hi_time > lo_time;
}

uint8_t adb_recv_cmd(uint8_t* data, uint8_t srq)
{
  *data = 0;
  uint8_t atten_result = look_for_atten();
  if(atten_result != ADB_LINE_STATUS_ATTEN)
    return atten_result;
  int32_t sync_duration = wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(sync_duration > 90 || sync_duration < 50)
    return ADB_ERROR;
  
  uint8_t temp = 0;
  for (int i = 0; i < 8; ++i)
  {
    uint8_t this_bit = adb_read_bit();
    if(this_bit == ADB_ERROR)
      return ADB_ERROR;
    temp |= this_bit << (7 - i);
  }

  if(srq == 0)
    wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  else
  {
    ADB_DATA_LOW();
    delay_us(300);
    ADB_DATA_HI();
  }

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
      delay_us(35);
      ADB_DATA_HI();
      // if the line doesnt actually go high, then there has been a bus collision
      if(ADB_READ_DATA_PIN() != GPIO_PIN_SET) 
        return ADB_LINE_STATUS_COLLISION;
      delay_us(65);
    }
    else
    {
      ADB_DATA_LOW();
      delay_us(65);
      ADB_DATA_HI();
      if(ADB_READ_DATA_PIN() != GPIO_PIN_SET)
        return ADB_LINE_STATUS_COLLISION;
      delay_us(35);
    }
  }
  return ADB_OK;
}

uint8_t adb_write_16(uint16_t data)
{
  if(adb_write_byte((uint8_t)(data >> 8)) == ADB_LINE_STATUS_COLLISION) // MSB
    return ADB_LINE_STATUS_COLLISION;
  if(adb_write_byte((uint8_t)(data & 0xff)) == ADB_LINE_STATUS_COLLISION) // LSB
    return ADB_LINE_STATUS_COLLISION;
  return ADB_OK;
}

void write_test(void)
{
  ;
}

// to be called right after a LISTEN command from host
uint8_t adb_send_response_16b(uint16_t data)
{
  delay_us(170); // stop-to-start time
  ADB_DATA_LOW();
  delay_us(35);
  ADB_DATA_HI();
  delay_us(65);
  if(adb_write_16(data) == ADB_LINE_STATUS_COLLISION)
    return ADB_LINE_STATUS_COLLISION;
  ADB_DATA_LOW();
  delay_us(65);
  ADB_DATA_HI();
  return ADB_OK;
}


uint8_t adb_listen_16b(uint16_t* data)
{
  *data = 0;
  if(ADB_READ_DATA_PIN() != GPIO_PIN_SET)
    return ADB_ERROR;
  // stop-to-start
  if(wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;
  // start pulse
  if(wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;
  if(wait_until_change(ADB_DEFAULT_TIMEOUT_US) == ADB_TIMEOUT)
    return ADB_ERROR;

  uint16_t temp = 0;
  for (int i = 0; i < 16; ++i)
  {
    uint8_t this_bit = adb_read_bit();
    if(this_bit == ADB_ERROR)
      return ADB_ERROR;
    temp |= this_bit << (15 - i);
  }
  wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  *data = temp;
  return ADB_OK;
}

#define ADB_CMD_TYPE_FLUSH 0
#define ADB_CMD_TYPE_LISTEN 2
#define ADB_CMD_TYPE_TALK 3
// addr 2 keyboard, 3 mouse

#define ADB_CHANGE_ADDR 0xFE

uint32_t last_send;

uint8_t parse_adb_cmd(uint8_t data)
{
  uint8_t addr = data >> 4;
  uint8_t cmd = (data >> 2) & 0x3;
  uint8_t reg = data & 0x3;

  if(addr == 0)
    return ADB_ERROR;

  if(cmd == ADB_CMD_TYPE_TALK && reg == 3 && addr == adb_mouse_current_addr)
  {
    uint16_t response = 0x6001; // 0110 0000 0000 0001, device handler 0x1, 100dps apple desktop bus mouse
    uint16_t rand_id = (rand() % 0xf) << 8;
    response |= rand_id;
    DEBUG1_HI();
    adb_send_response_16b(response);
    DEBUG1_LOW();
  }

  if(cmd == ADB_CMD_TYPE_TALK && reg == 3 && addr == adb_kb_current_addr)
  {
    uint16_t response = 0x6005; // 0110 0000 0000 0101, device handler 0x5, appledesign keyboard
    uint16_t rand_id = (rand() % 0xf) << 8;
    response |= rand_id;
    DEBUG1_HI();
    adb_send_response_16b(response);
    DEBUG1_LOW();
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

  if(cmd == ADB_CMD_TYPE_TALK && reg == 0 && addr == adb_mouse_current_addr && HAL_GetTick() - last_send > 500)
  {
    uint16_t response = 0x80fc;
    DEBUG1_HI();
    adb_send_response_16b(response);
    last_send = HAL_GetTick();
    printf("sending...\n");
    DEBUG1_LOW();
  }

  // if(cmd == ADB_CMD_TYPE_TALK && reg == 0 && addr == adb_kb_current_addr && HAL_GetTick() - last_send > 500)
  // {
  //   uint16_t response = 0x580;
  //   DEBUG1_HI();
  //   adb_send_response_16b(response);
  //   last_send = HAL_GetTick();
  //   printf("sending...\n");
  //   DEBUG1_LOW();
  // }

  return ADB_OK;
}






