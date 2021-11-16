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


#define ADB_PSW_HI() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_SET)
#define ADB_PSW_LOW() HAL_GPIO_WritePin(adb_psw_port, adb_psw_pin, GPIO_PIN_RESET)

#define ADB_DATA_HI() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_SET)
#define ADB_DATA_LOW() HAL_GPIO_WritePin(adb_data_port, adb_data_pin, GPIO_PIN_RESET)

#define ADB_READ_DATA_PIN() HAL_GPIO_ReadPin(adb_data_port, adb_data_pin)
#define ADB_READ_CLK_PIN() HAL_GPIO_ReadPin(adb_psw_port, adb_psw_pin)

#define ADB_DEFAULT_TIMEOUT_US 10000

void adb_release_lines(void)
{
  ADB_PSW_HI();
  ADB_DATA_HI();
}

void adb_reset(void)
{
  ;
}

void adb_init(GPIO_TypeDef* data_port, uint16_t data_pin, GPIO_TypeDef* psw_port, uint16_t psw_pin)
{
  adb_psw_port = psw_port;
  adb_psw_pin = psw_pin;
  adb_data_port = data_port;
  adb_data_pin = data_pin;
  adb_reset();
  adb_release_lines();
}

#define ADB_LINE_STATUS_ATTEN 1
#define ADB_LINE_STATUS_IDLE 2
#define ADB_LINE_STATUS_RESET 3
#define ADB_LINE_STATUS_BUSY 4

#define ADB_OK 0
#define ADB_TIMEOUT -1
#define ADB_LINE_STATUS_ERROR -2
#define ADB_ERROR ADB_LINE_STATUS_ERROR


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

int8_t look_for_atten(void)
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

uint8_t adb_recv_cmd(uint8_t srq)
{
  int8_t atten_result = look_for_atten();
  if(atten_result != ADB_LINE_STATUS_ATTEN)
    return atten_result;
  int32_t sync_duration = wait_until_change(ADB_DEFAULT_TIMEOUT_US);
  if(sync_duration > 90 || sync_duration < 50)
    return ADB_ERROR;
  
  for (int i = 0; i < 8; ++i)
  {
    uint8_t this_bit = adb_read_bit();
    if(this_bit == ADB_ERROR)
      return ADB_ERROR;
  }

  if(srq == 0)
    wait_until_change(ADB_DEFAULT_TIMEOUT_US);

  return ADB_OK;
}