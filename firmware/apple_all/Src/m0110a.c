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