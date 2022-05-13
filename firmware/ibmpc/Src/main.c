
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2022 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"

/* USER CODE BEGIN Includes */
#include "delay_us.h"
#include "shared.h"
#include "helpers.h"
#include "ps2kb.h"
#include <string.h>
#include "ps2mouse.h"
#include "mcp4451.h"
#include "xt_kb.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
const uint8_t board_id = 1;
const uint8_t version_major = 0;
const uint8_t version_minor = 5;
const uint8_t version_patch = 2;
uint8_t hw_revision;

uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t backup_spi1_recv_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
kb_buf my_kb_buf;
mouse_buf my_mouse_buf;
gamepad_buf my_gamepad_buf;
uint16_t flash_size;

uint8_t ps2kb_host_cmd, ps2mouse_host_cmd, buffered_code, buffered_value, ps2mouse_bus_status, ps2kb_bus_status;
mouse_event latest_mouse_event;
gamepad_event latest_gamepad_event;
ps2_outgoing_buf my_ps2_outbuf;
#define MICROSOFT_SERIAL_MOUSE_BUF_SIZE 3
uint8_t microsoft_serial_mouse_output_buf[MICROSOFT_SERIAL_MOUSE_BUF_SIZE];
#define MOUSESYSTEMS_SERIAL_MOUSE_BUF_SIZE 5
uint8_t mousesystems_serial_mouse_output_buf[MOUSESYSTEMS_SERIAL_MOUSE_BUF_SIZE];

uint8_t serial_mouse_rts_response;
volatile uint8_t rts_active;
uint8_t spi_error_occured;

#define PROTOCOL_STATUS_NOT_AVAILABLE 0
#define PROTOCOL_STATUS_ENABLED 1
#define PROTOCOL_STATUS_DISABLED 2

#define PROTOCOL_LOOKUP_SIZE 16
uint8_t protocol_status_lookup[PROTOCOL_LOOKUP_SIZE];

#define IS_KB_PRESENT() HAL_GPIO_ReadPin(KB_DETECT_GPIO_Port, KB_DETECT_Pin)
#define IS_PS2MOUSE_PRESENT() HAL_GPIO_ReadPin(MOUSE_DETECT_GPIO_Port, MOUSE_DETECT_Pin)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_IWDG_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

void mouse_uart_switch_to_8bit(void)
{
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  HAL_UART_Init(&huart3);
}

void mouse_uart_switch_to_7bit(void)
{
  huart3.Init.WordLength = UART_WORDLENGTH_7B;
  HAL_UART_Init(&huart3);
}

int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (unsigned char *)&ch, 1, 100);
  return ch;
}

int16_t byte_to_int16_t(uint8_t lsb, uint8_t msb)
{
  return (int16_t)((msb << 8) | lsb);
}

uint8_t is_protocol_enabled(uint8_t this_protocol)
{
  if(this_protocol >= PROTOCOL_LOOKUP_SIZE)
    return 0;
  return protocol_status_lookup[this_protocol] == PROTOCOL_STATUS_ENABLED;
}

void gameport_init()
{
  // release all buttons, reset digital pot
  HAL_GPIO_WritePin(GAMEPAD_B1_GPIO_Port, GAMEPAD_B1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GAMEPAD_B2_GPIO_Port, GAMEPAD_B2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GAMEPAD_B3_GPIO_Port, GAMEPAD_B3_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GAMEPAD_B4_GPIO_Port, GAMEPAD_B4_Pin, GPIO_PIN_SET);
  mcp4451_reset();
}

void handle_protocol_switch(uint8_t spi_byte)
{
  uint8_t index = spi_byte & 0x7f;
  uint8_t onoff = spi_byte & 0x80;

  if(index >= PROTOCOL_LOOKUP_SIZE)
    return;
  // trying to change a protocol that is not available on this board
  if(protocol_status_lookup[index] == PROTOCOL_STATUS_NOT_AVAILABLE)
    return;
  // switching protocol ON
  if(onoff && protocol_status_lookup[index] == PROTOCOL_STATUS_DISABLED)
  {
    switch(index)
    {
      case PROTOCOL_AT_PS2_KB:
        ps2kb_init(PS2KB_CLK_GPIO_Port, PS2KB_CLK_Pin, PS2KB_DATA_GPIO_Port, PS2KB_DATA_Pin);
        break;

      case PROTOCOL_XT_KB:
        xtkb_enable();
        break;

      case PROTOCOL_PS2_MOUSE:
        ps2mouse_init(PS2MOUSE_CLK_GPIO_Port, PS2MOUSE_CLK_Pin, PS2MOUSE_DATA_GPIO_Port, PS2MOUSE_DATA_Pin);
        break;

      case PROTOCOL_MICROSOFT_SERIAL_MOUSE:
        mouse_uart_switch_to_7bit();
        break;

      case PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE:
        mouse_uart_switch_to_8bit();
        break;

      case PROTOCOL_GENERIC_GAMEPORT_GAMEPAD:
        gameport_init();
        break;
    }
    protocol_status_lookup[index] = PROTOCOL_STATUS_ENABLED;
  }
  // switching protocol OFF
  else if((onoff == 0) && protocol_status_lookup[index] == PROTOCOL_STATUS_ENABLED)
  {
    protocol_status_lookup[index] = PROTOCOL_STATUS_DISABLED;
    switch(index) 
    {
      case PROTOCOL_AT_PS2_KB:
        ps2kb_release_lines();
        ps2kb_reset();
        break;

      case PROTOCOL_XT_KB:
        xtkb_release_lines();
        break;

      case PROTOCOL_PS2_MOUSE:
        ps2mouse_reset();
        ps2mouse_release_lines();
        break;

      case PROTOCOL_MICROSOFT_SERIAL_MOUSE:
        break;

      case PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE:
        break;

      case PROTOCOL_GENERIC_GAMEPORT_GAMEPAD:
        gameport_init();
        break;
    }
  }
}

#define MOUSE_BUTTON_HISTORY_SIZE 5
uint8_t mouse_button_history[MOUSE_BUTTON_HISTORY_SIZE];

/*
  This is called when a new SPI packet is received
  This is part of an ISR, so keep it short!
*/
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_SET);
  memcpy(backup_spi1_recv_buf, spi_recv_buf, SPI_BUF_SIZE);

  if(backup_spi1_recv_buf[0] != 0xde)
  {
    spi_error_occured = 1;
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT)
  {
    kb_buf_add(&my_kb_buf, backup_spi1_recv_buf[4], backup_spi1_recv_buf[6]);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_MOUSE_EVENT)
  {
    latest_mouse_event.movement_x = byte_to_int16_t(backup_spi1_recv_buf[4], backup_spi1_recv_buf[5]);
    latest_mouse_event.movement_y = -1 * byte_to_int16_t(backup_spi1_recv_buf[6], backup_spi1_recv_buf[7]);
    latest_mouse_event.scroll_vertical = -1 * backup_spi1_recv_buf[8];
    latest_mouse_event.scroll_horizontal = backup_spi1_recv_buf[9];
    latest_mouse_event.button_left = backup_spi1_recv_buf[13];
    latest_mouse_event.button_right = backup_spi1_recv_buf[14];
    latest_mouse_event.button_middle = backup_spi1_recv_buf[15];
    latest_mouse_event.button_side = backup_spi1_recv_buf[16];
    latest_mouse_event.button_extra = backup_spi1_recv_buf[17];
    latest_mouse_event.has_button_transition = 0;
    if(memcmp(mouse_button_history, &backup_spi1_recv_buf[13], MOUSE_BUTTON_HISTORY_SIZE))
      latest_mouse_event.has_button_transition = 1;
    memcpy(mouse_button_history, &backup_spi1_recv_buf[13], MOUSE_BUTTON_HISTORY_SIZE);
    mouse_buf_add(&my_mouse_buf, &latest_mouse_event);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED_IBMPC)
  {
    latest_gamepad_event.button_1 = backup_spi1_recv_buf[4];
    latest_gamepad_event.button_2 = backup_spi1_recv_buf[5];
    latest_gamepad_event.button_3 = backup_spi1_recv_buf[6];
    latest_gamepad_event.button_4 = backup_spi1_recv_buf[7];
    latest_gamepad_event.button_lt = 0;
    latest_gamepad_event.button_rt = 0;
    latest_gamepad_event.button_lt2 = 0;
    latest_gamepad_event.button_rt2 = 0;
    latest_gamepad_event.axis_x = backup_spi1_recv_buf[8];
    latest_gamepad_event.axis_y = backup_spi1_recv_buf[9];
    latest_gamepad_event.axis_rx = backup_spi1_recv_buf[10];
    latest_gamepad_event.axis_ry = backup_spi1_recv_buf[11];
    gamepad_buf_add(&my_gamepad_buf, &latest_gamepad_event);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_REQ_ACK)
  {
    HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_INFO_REQUEST)
  {
    memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
    spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
    spi_transmit_buf[SPI_BUF_INDEX_SEQNUM] = backup_spi1_recv_buf[SPI_BUF_INDEX_SEQNUM];
    spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_TYPE_INFO_REQUEST;
    spi_transmit_buf[3] = board_id;
    spi_transmit_buf[4] = hw_revision;
    spi_transmit_buf[5] = version_major;
    spi_transmit_buf[6] = version_minor;
    spi_transmit_buf[7] = version_patch;
    uint8_t curr_index = 8;
    for (int i = 0; i < PROTOCOL_LOOKUP_SIZE; i++)
    {
      if(protocol_status_lookup[i] == PROTOCOL_STATUS_NOT_AVAILABLE)
        continue;
      else if(protocol_status_lookup[i] == PROTOCOL_STATUS_DISABLED)
        spi_transmit_buf[curr_index] = i;
      else if(protocol_status_lookup[i] == PROTOCOL_STATUS_ENABLED)
        spi_transmit_buf[curr_index] = i | 0x80;
      curr_index++;
    }
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_SET_PROTOCOL)
  {
    for (int i = 3; i < SPI_BUF_SIZE; ++i)
    {
      if(backup_spi1_recv_buf[i] == 0)
        break;
      handle_protocol_switch(backup_spi1_recv_buf[i]);
    }
  }
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);
}

void ps2mouse_update(void)
{
  ps2mouse_bus_status = ps2mouse_get_bus_status();
  if(ps2mouse_bus_status == PS2_BUS_INHIBIT)
  {
    ps2mouse_release_lines();
    return;
  }
  else if(ps2mouse_bus_status == PS2_BUS_REQ_TO_SEND)
  {
    ps2mouse_read(&ps2mouse_host_cmd, 10);
    ps2mouse_host_req_reply(ps2mouse_host_cmd, &latest_mouse_event);
    return;
  }

  mouse_event* this_mouse_event = mouse_buf_peek(&my_mouse_buf);
  if(this_mouse_event == NULL)
    return;

  if(ps2mouse_get_outgoing_data(this_mouse_event, &my_ps2_outbuf))
  {
    // if return value is not 0, no need to send out packets
    mouse_buf_pop(&my_mouse_buf);
    return;
  }

  if(ps2mouse_send_update(&my_ps2_outbuf) != PS2_OK)
  {
    HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_SET);
    uint32_t enter_time = HAL_GetTick();
    while(ps2mouse_get_bus_status() != PS2_BUS_IDLE)
    {
      if(HAL_GetTick() - enter_time > 25)
        break;
    }
    HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_RESET);
  }
  mouse_buf_reset(&my_mouse_buf); // don't change this!
}

void ps2kb_update(void)
{
  ps2kb_bus_status = ps2kb_get_bus_status();
  if(ps2kb_bus_status == PS2_BUS_INHIBIT)
  {
    ps2kb_release_lines();
    return;
  }
  else if(ps2kb_bus_status == PS2_BUS_REQ_TO_SEND)
  {
    uint8_t ps2kb_leds = 0xff;
    ps2kb_read(&ps2kb_host_cmd, 10);
    keyboard_reply(ps2kb_host_cmd, &ps2kb_leds);
    if(ps2kb_leds != 0xff)
    {
      memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
      spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
      spi_transmit_buf[SPI_BUF_INDEX_SEQNUM] = backup_spi1_recv_buf[SPI_BUF_INDEX_SEQNUM];
      spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_TYPE_KB_LED_REQUEST;
      if(ps2kb_leds & 0x1) // scroll lock LED
        spi_transmit_buf[3] = 1;
      if(ps2kb_leds & 0x2)  // num lock LED
        spi_transmit_buf[4] = 1;
      if(ps2kb_leds & 0x4)  // caps lock LED
        spi_transmit_buf[5] = 1;
      HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_SET);
    }
  }
  else if(ps2kb_bus_status == PS2_BUS_IDLE && (kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0))
  {
    if(ps2kb_press_key(buffered_code, buffered_value) == PS2_ERROR_HOST_INHIBIT) // host inhibited line during transmission
    {
      // HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_SET);
      HAL_Delay(1);
      // HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_RESET);
    }
    else
    {
      kb_buf_pop(&my_kb_buf);
    }
  }
}

// GPIO external interrupt callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == UART3_RTS_Pin && is_protocol_enabled(PROTOCOL_MICROSOFT_SERIAL_MOUSE))
    rts_active = 1;
}

uint8_t serial_mouse_is_tx_in_progress;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  serial_mouse_is_tx_in_progress = 0;
}

// https://everything2.com/title/Mouse+protocol
void microsoft_serial_mouse_update(void)
{
  if(rts_active)
  {
    serial_mouse_rts_response = 0x4d; // 0x4d = 'M'
    HAL_UART_Transmit_IT(&huart3, &serial_mouse_rts_response, 1);
    rts_active = 0;
  }

  mouse_event* this_mouse_event = mouse_buf_peek(&my_mouse_buf);
  if(this_mouse_event == NULL)
    return;

  if(serial_mouse_is_tx_in_progress)
  {
    if(this_mouse_event->has_button_transition == 0) // dont throw away events that have button transitions
      mouse_buf_pop(&my_mouse_buf);
    return;
  }

  memset(microsoft_serial_mouse_output_buf, 0, MICROSOFT_SERIAL_MOUSE_BUF_SIZE);
  microsoft_serial_mouse_output_buf[0] = 0xc0;
  if(this_mouse_event->button_left)
    microsoft_serial_mouse_output_buf[0] |= 0x20;
  if(this_mouse_event->button_right)
    microsoft_serial_mouse_output_buf[0] |= 0x10;

  int16_t serial_x = this_mouse_event->movement_x;
  int16_t serial_y = -1* this_mouse_event->movement_y;

  if(serial_y & 0x80)
    microsoft_serial_mouse_output_buf[0] |= 0x8;
  if(serial_y & 0x40)
    microsoft_serial_mouse_output_buf[0] |= 0x4;
  if(serial_x & 0x80)
    microsoft_serial_mouse_output_buf[0] |= 0x2;
  if(serial_x & 0x40)
    microsoft_serial_mouse_output_buf[0] |= 0x1;

  microsoft_serial_mouse_output_buf[1] = 0x3f & serial_x;
  microsoft_serial_mouse_output_buf[2] = 0x3f & serial_y;
  mouse_buf_pop(&my_mouse_buf);
  HAL_UART_Transmit_IT(&huart3, microsoft_serial_mouse_output_buf, MICROSOFT_SERIAL_MOUSE_BUF_SIZE);
  serial_mouse_is_tx_in_progress = 1;
  if(this_mouse_event->has_button_transition)
    HAL_Delay(10);
}

void spi_error_dump_reboot(void)
{
  printf("SPI ERROR\n");
  for (int i = 0; i < SPI_BUF_SIZE; ++i)
    printf("%d ", backup_spi1_recv_buf[i]);
  printf("\nrebooting...\n");
  for (int i = 0; i < 100; ++i)
  {
    HAL_GPIO_TogglePin(ERR_LED_GPIO_Port, ERR_LED_Pin);
    HAL_Delay(100);
  }
  NVIC_SystemReset();
}

const char boot_message[] = "USB4VC Protocol Board\nIBM PC Compatible\ndekuNukem 2022";

void protocol_status_lookup_init(void)
{
  memset(protocol_status_lookup, PROTOCOL_STATUS_NOT_AVAILABLE, PROTOCOL_LOOKUP_SIZE);
  protocol_status_lookup[PROTOCOL_AT_PS2_KB] = PROTOCOL_STATUS_ENABLED;
  protocol_status_lookup[PROTOCOL_XT_KB] = PROTOCOL_STATUS_DISABLED;
  protocol_status_lookup[PROTOCOL_PS2_MOUSE] = PROTOCOL_STATUS_ENABLED;
  protocol_status_lookup[PROTOCOL_MICROSOFT_SERIAL_MOUSE] = PROTOCOL_STATUS_DISABLED;
  protocol_status_lookup[PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE] = PROTOCOL_STATUS_DISABLED;
  protocol_status_lookup[PROTOCOL_GENERIC_GAMEPORT_GAMEPAD] = PROTOCOL_STATUS_ENABLED;
}

void gamepad_update(void)
{
  gamepad_event* this_gamepad_event = gamepad_buf_peek(&my_gamepad_buf);
  if(this_gamepad_event != NULL)
  {
    // printf("%d %d %d %d %d %d %d %d\n---\n", this_gamepad_event->button_1, this_gamepad_event->button_2, this_gamepad_event->button_3, this_gamepad_event->button_4, this_gamepad_event->axis_x, this_gamepad_event->axis_y, this_gamepad_event->axis_rx, this_gamepad_event->axis_ry);
    /*
    X1 = Wiper 3
    Y1 = Wiper 0
    X2 = Wiper 2
    Y2 = Wiper 1
    */
    HAL_GPIO_WritePin(GAMEPAD_B1_GPIO_Port, GAMEPAD_B1_Pin, !(this_gamepad_event->button_1));
    HAL_GPIO_WritePin(GAMEPAD_B2_GPIO_Port, GAMEPAD_B2_Pin, !(this_gamepad_event->button_2));
    HAL_GPIO_WritePin(GAMEPAD_B3_GPIO_Port, GAMEPAD_B3_Pin, !(this_gamepad_event->button_3));
    HAL_GPIO_WritePin(GAMEPAD_B4_GPIO_Port, GAMEPAD_B4_Pin, !(this_gamepad_event->button_4));
    mcp4451_write_wiper(3, 255-this_gamepad_event->axis_x);
    mcp4451_write_wiper(0, 255-this_gamepad_event->axis_y);
    mcp4451_write_wiper(2, 255-this_gamepad_event->axis_rx);
    mcp4451_write_wiper(1, 255-this_gamepad_event->axis_ry);
    gamepad_buf_pop(&my_gamepad_buf);
  }
}

void xtkb_update(void)
{
  xtkb_check_for_softreset();
  if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
  {
    if(xtkb_press_key(buffered_code, buffered_value) == XTKB_ERROR_HOST_INHIBIT)
      HAL_Delay(1);
    else
      kb_buf_pop(&my_kb_buf);
  }
}

/*
!!!!!!!!!!!!!!!!!!!!!! AFTER CODE RE-GENERATION
!!!!!!!!!!!!!!!!!!!!!! CHANGE THE FOLLOWING

huart3.Init.BaudRate = 1200;
if(flash_size != 64)
  huart3.Init.BaudRate = 2032;

huart1.Init.BaudRate = 115200;
if(flash_size != 64)
  huart1.Init.BaudRate = 195134;

huart3.Init.WordLength = UART_WORDLENGTH_7B;

!!!!!!!!!!!!!!!!!!!!!!
*/

void mousesystems_serial_mouse_update(void)
{
  mouse_event* this_mouse_event = mouse_buf_peek(&my_mouse_buf);
  if(this_mouse_event == NULL)
    return;

  if(serial_mouse_is_tx_in_progress)
  {
    if(this_mouse_event->has_button_transition == 0) // don't throw away mouse events that has button transitions
      mouse_buf_pop(&my_mouse_buf);
    return;
  }

  memset(mousesystems_serial_mouse_output_buf, 0, MOUSESYSTEMS_SERIAL_MOUSE_BUF_SIZE);
  mousesystems_serial_mouse_output_buf[0] = 0x87;
  if(this_mouse_event->button_left)
    mousesystems_serial_mouse_output_buf[0] &= 0xfb;
  if(this_mouse_event->button_middle)
    mousesystems_serial_mouse_output_buf[0] &= 0xfd;
  if(this_mouse_event->button_right)
    mousesystems_serial_mouse_output_buf[0] &= 0xfe;
  mousesystems_serial_mouse_output_buf[1] = (int8_t)(this_mouse_event->movement_x);
  mousesystems_serial_mouse_output_buf[2] = (int8_t)(this_mouse_event->movement_y);
  mousesystems_serial_mouse_output_buf[3] = (int8_t)(this_mouse_event->movement_x);
  mousesystems_serial_mouse_output_buf[4] = (int8_t)(this_mouse_event->movement_y);
  mouse_buf_pop(&my_mouse_buf);
  HAL_UART_Transmit_IT(&huart3, mousesystems_serial_mouse_output_buf, MOUSESYSTEMS_SERIAL_MOUSE_BUF_SIZE);
  serial_mouse_is_tx_in_progress = 1;
  if(this_mouse_event->has_button_transition)
    HAL_Delay(10); // wait a bit for the computer to register button press
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  flash_size = *(uint16_t*)0x1FFFF7CC;
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  HAL_IWDG_Refresh(&hiwdg);
  printf("%s\nrev%d v%d.%d.%d\n", boot_message, hw_revision, version_major, version_minor, version_patch);
  delay_us_init(&htim2);
  protocol_status_lookup_init();

  ps2kb_init(PS2KB_CLK_GPIO_Port, PS2KB_CLK_Pin, PS2KB_DATA_GPIO_Port, PS2KB_DATA_Pin);
  xtkb_init(PS2KB_CLK_GPIO_Port, PS2KB_CLK_Pin, PS2KB_DATA_GPIO_Port, PS2KB_DATA_Pin);
  ps2mouse_init(PS2MOUSE_CLK_GPIO_Port, PS2MOUSE_CLK_Pin, PS2MOUSE_DATA_GPIO_Port, PS2MOUSE_DATA_Pin);
  
  if(is_protocol_enabled(PROTOCOL_XT_KB))
    xtkb_enable();

  kb_buf_init(&my_kb_buf, 16);
  mouse_buf_init(&my_mouse_buf, 16);
  gamepad_buf_init(&my_gamepad_buf, 16);

  mcp4451_reset();

  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("flash_size: %d\n", flash_size);

  if(!mcp4451_is_available())
  {
    printf("Digital pot not responding!\n");
    for (int i = 0; i < 10; ++i)
    {
      HAL_GPIO_TogglePin(ERR_LED_GPIO_Port, ERR_LED_Pin);
      HAL_Delay(50);
    }
    HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_SET);
  }

  while (1)
  {
    HAL_IWDG_Refresh(&hiwdg);
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

    // If more than one enabled, PS2 mouse takes priority
    if(is_protocol_enabled(PROTOCOL_PS2_MOUSE) && IS_PS2MOUSE_PRESENT())
      ps2mouse_update();
    else if(is_protocol_enabled(PROTOCOL_MICROSOFT_SERIAL_MOUSE))
      microsoft_serial_mouse_update();
    else if(is_protocol_enabled(PROTOCOL_MOUSESYSTEMS_SERIAL_MOUSE))
      mousesystems_serial_mouse_update();
    // If both enabled, PS2 keyboard takes priority
    if(is_protocol_enabled(PROTOCOL_AT_PS2_KB) && IS_KB_PRESENT())
      ps2kb_update();
    else if(is_protocol_enabled(PROTOCOL_XT_KB) && IS_KB_PRESENT())
      xtkb_update();

    if(is_protocol_enabled(PROTOCOL_GENERIC_GAMEPORT_GAMEPAD))
      gamepad_update();

    if(spi_error_occured)
      spi_error_dump_reboot();
  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* I2C2 init function */
static void MX_I2C2_Init(void)
{

  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x20303E5D;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Analogue filter 
    */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Digital filter 
    */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* IWDG init function */
static void MX_IWDG_Init(void)
{

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 47;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  if(flash_size != 64)
    huart1.Init.BaudRate = 195134;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART3 init function */
static void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 1200;
  if(flash_size != 64)
    huart3.Init.BaudRate = 2032;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.WordLength = UART_WORDLENGTH_7B;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT|UART_ADVFEATURE_DMADISABLEONERROR_INIT;
  huart3.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
  huart3.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GAMEPAD_B4_Pin|GAMEPAD_B2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GAMEPAD_B3_Pin|GAMEPAD_B1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, POT_RESET_Pin|PS2MOUSE_DATA_Pin|PS2MOUSE_CLK_Pin|PS2KB_DATA_Pin 
                          |PS2KB_CLK_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ERR_LED_GPIO_Port, ERR_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SLAVE_REQ_Pin */
  GPIO_InitStruct.Pin = SLAVE_REQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SLAVE_REQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GAMEPAD_B4_Pin GAMEPAD_B2_Pin */
  GPIO_InitStruct.Pin = GAMEPAD_B4_Pin|GAMEPAD_B2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : GAMEPAD_B3_Pin GAMEPAD_B1_Pin */
  GPIO_InitStruct.Pin = GAMEPAD_B3_Pin|GAMEPAD_B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : UART3_RTS_Pin */
  GPIO_InitStruct.Pin = UART3_RTS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(UART3_RTS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : POT_RESET_Pin PS2MOUSE_DATA_Pin PS2MOUSE_CLK_Pin PS2KB_DATA_Pin 
                           PS2KB_CLK_Pin */
  GPIO_InitStruct.Pin = POT_RESET_Pin|PS2MOUSE_DATA_Pin|PS2MOUSE_CLK_Pin|PS2KB_DATA_Pin 
                          |PS2KB_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MOUSE_DETECT_Pin */
  GPIO_InitStruct.Pin = MOUSE_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(MOUSE_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KB_DETECT_Pin */
  GPIO_InitStruct.Pin = KB_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KB_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ACT_LED_Pin */
  GPIO_InitStruct.Pin = ACT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACT_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ERR_LED_Pin */
  GPIO_InitStruct.Pin = ERR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ERR_LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
