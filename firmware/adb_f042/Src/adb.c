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

#define ADB_LINE_STATUS_ATTEN 0
#define ADB_LINE_STATUS_IDLE 1
#define ADB_LINE_STATUS_RESET 2
#define ADB_LINE_STATUS_IN_PROGRESS 3

void wait_until_line_low(void)
{
  
}

uint8_t look_for_atten(uint8_t blocking)
{
  // if ADB data line is high
  while(ADB_READ_DATA_PIN() == GPIO_PIN_SET)
  {
    if(blocking == 0)
      return ADB_LINE_STATUS_IDLE;
  }
  // now data line is low
  uint32_t atten_start = micros();

  while(ADB_READ_DATA_PIN() == GPIO_PIN_RESET)
  {
    // reset condition
    if(micros() - atten_start > 3000)
      return ADB_LINE_STATUS_RESET;
  }

  uint32_t duration = micros() - atten_start;
  
  // not an attention signal
  if(duration < 700)
    return ADB_LINE_STATUS_IN_PROGRESS;
  if(duration > 2000)
    return ADB_LINE_STATUS_RESET;
	return ADB_LINE_STATUS_ATTEN;
}

void adb_recv_cmd(void)
{

  HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET);
  uint8_t result = look_for_atten(1);
  HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET);
  // printf("%d\n", result);
}