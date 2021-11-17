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
      delay_us(65);
    }
    else
    {
      ADB_DATA_LOW();
      delay_us(65);
      ADB_DATA_HI();
      delay_us(35);
    }
  }
  return ADB_OK;
}

uint8_t adb_write_16(uint16_t data)
{
  adb_write_byte((uint8_t)(data >> 8)); // MSB
  adb_write_byte((uint8_t)(data & 0xff)); // LSB
  return ADB_OK;
}

void write_test(void)
{

  // adb_mouse_reg[3] = 0x6001;
  // uint16_t rand_id = (rand() % 0xf) << 8;
  // adb_mouse_reg[3] |= rand_id;

  // delay_us(200); // stop-to-start time
  // TEST_ADB_DATA_LOW();
  // delay_us(35);
  // TEST_ADB_DATA_HI();
  // delay_us(65);

  // adb_write_16(adb_mouse_reg[3]);

  // TEST_ADB_DATA_LOW();
  // delay_us(65);
  // TEST_ADB_DATA_HI();
}

void adb_send_response_16b(uint16_t data)
{
  delay_us(170); // stop-to-start time
  ADB_DATA_LOW();
  delay_us(35);
  ADB_DATA_HI();
  delay_us(65);
  adb_write_16(data);
  ADB_DATA_LOW();
  delay_us(65);
  ADB_DATA_HI();
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
    uint16_t response = 0x6001; // 0110 0000 0000 0001
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

  if(cmd == ADB_CMD_TYPE_TALK && reg == 0 && addr == adb_mouse_current_addr && HAL_GetTick() - last_send > 500)
  {
    uint16_t response = 0x80fc;
    DEBUG1_HI();
    adb_send_response_16b(response);
    last_send = HAL_GetTick();
    printf("sending...\n");
    DEBUG1_LOW();
  }

  return ADB_OK;
}






