
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
#include <string.h>
#include <stdlib.h>
#include "delay_us.h"
#include "shared.h"
#include "helpers.h"
#include "adb.h"

#define PROTOCOL_STATUS_NOT_AVAILABLE 0
#define PROTOCOL_STATUS_ENABLED 1
#define PROTOCOL_STATUS_DISABLED 2
#define PROTOCOL_LOOKUP_SIZE 16

#define IS_ADB_HOST_PRESENT() HAL_GPIO_ReadPin(ADB_5V_DET_GPIO_Port, ADB_5V_DET_Pin)

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
const uint8_t board_id = 2;
const uint8_t version_major = 0;
const uint8_t version_minor = 1;
const uint8_t version_patch = 1;
uint8_t hw_revision = 0;

uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
kb_buf my_kb_buf;
mouse_buf my_mouse_buf;
mouse_event latest_mouse_event;
mouse_event consolidated_mouse_event;
uint8_t spi_error_occured;
uint8_t protocol_status_lookup[PROTOCOL_LOOKUP_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (unsigned char *)&ch, 1, 10);
    return ch;
}

void cap_to_127(int32_t *number)
{
  if(*number > 127)
    *number = 127;
  if(*number < -127)
    *number = -127;
}

void get_consolidated_mouse_event(mouse_buf* mbuf, mouse_event* cme_result)
{
  cme_result->movement_x = 0;
  cme_result->movement_y = 0;
  cme_result->scroll_vertical = 0;
  cme_result->button_left = 0;
  cme_result->button_right = 0;
  cme_result->button_middle = 0;
  cme_result->button_side = 0;
  cme_result->button_extra = 0;
  while(1)
  {
    mouse_event* this_mouse_event = mouse_buf_peek(mbuf);
    if(this_mouse_event == NULL)
      break;
    cme_result->movement_x += this_mouse_event->movement_x;
    cme_result->movement_y += this_mouse_event->movement_y;
    cme_result->scroll_vertical += this_mouse_event->scroll_vertical;
    cme_result->button_left |= this_mouse_event->button_left;
    cme_result->button_right |= this_mouse_event->button_right;
    cme_result->button_middle |= this_mouse_event->button_middle;
    cme_result->button_side |= this_mouse_event->button_side;
    cme_result->button_extra |= this_mouse_event->button_extra;
    mouse_buf_pop(mbuf);
  }
  cap_to_127(&cme_result->movement_x);
  cap_to_127(&cme_result->movement_y);
}

uint8_t is_protocol_enabled(uint8_t this_protocol)
{
  if(this_protocol >= PROTOCOL_LOOKUP_SIZE)
    return 0;
  return protocol_status_lookup[this_protocol] == PROTOCOL_STATUS_ENABLED;
}

int16_t byte_to_int16_t(uint8_t lsb, uint8_t msb)
{
  return (int16_t)((msb << 8) | lsb);
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
      case PROTOCOL_ADB_KB:
        kb_buf_reset(&my_kb_buf);
        break;
      case PROTOCOL_ADB_MOUSE:
        mouse_buf_reset(&my_mouse_buf);
        break;
    }
    protocol_status_lookup[index] = PROTOCOL_STATUS_ENABLED;
  }
  // switching protocol OFF
  else if((onoff == 0) && protocol_status_lookup[index] == PROTOCOL_STATUS_ENABLED)
    protocol_status_lookup[index] = PROTOCOL_STATUS_DISABLED;
}

void parse_spi_buf(uint8_t* spibuf)
{
  if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_MOUSE_EVENT)
  {
    latest_mouse_event.movement_x = byte_to_int16_t(spibuf[4], spibuf[5]);
    latest_mouse_event.movement_y = byte_to_int16_t(spibuf[6], spibuf[7]);
    latest_mouse_event.scroll_vertical = -1 * spibuf[8];
    latest_mouse_event.button_left = spibuf[13];
    latest_mouse_event.button_right = spibuf[14];
    latest_mouse_event.button_middle = spibuf[15];
    latest_mouse_event.button_side = spibuf[16];
    latest_mouse_event.button_extra = spibuf[17];
    mouse_buf_add(&my_mouse_buf, &latest_mouse_event);
  }
  else if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT)
  {
    kb_buf_add(&my_kb_buf, spibuf[4], spibuf[6]);
  }
  else if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_INFO_REQUEST)
  {
    memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
    spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
    spi_transmit_buf[SPI_BUF_INDEX_SEQNUM] = spibuf[SPI_BUF_INDEX_SEQNUM];
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
  else if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_SET_PROTOCOL)
  {
    for (int i = 3; i < SPI_BUF_SIZE; ++i)
    {
      if(spibuf[i] == 0)
        break;
      handle_protocol_switch(spibuf[i]);
    }
  }
}

#define TEMP_BUF_SIZE 8
uint8_t temp_kb_code_buf[TEMP_BUF_SIZE];
uint8_t temp_kb_value_buf[TEMP_BUF_SIZE];
uint8_t temp_kb_buf_index;

uint8_t temp_spi_msg_buf[SPI_BUF_SIZE];

/*
  a full rundown of this ISR takes around 30us, so if it comes in 
  while reading or writing ADB message, it will mess up the timing
  
  solution: cache the data and return immediately, only takes around 6us,
  then process the data in main loop.
*/
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DEBUG2_GPIO_Port, DEBUG2_Pin, GPIO_PIN_SET);

  if(spi_recv_buf[0] != 0xde)
    spi_error_occured = 1;
  if(adb_rw_in_progress)
  {
    // don't want to miss ANY keyboard events, so put them in a seperate buffer
    if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT)
    {
      temp_kb_code_buf[temp_kb_buf_index] = spi_recv_buf[4];
      temp_kb_value_buf[temp_kb_buf_index] = spi_recv_buf[6];
      if(temp_kb_buf_index < TEMP_BUF_SIZE)
        temp_kb_buf_index++;
    }
    else
    {
      // mouse event and other events. doesn't matter if we miss a few, so only keep the last one
      memcpy(temp_spi_msg_buf, spi_recv_buf, SPI_BUF_SIZE);
    }
    goto spi_isr_end;
  }
  parse_spi_buf(spi_recv_buf);
  spi_isr_end:
  if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_REQ_ACK)
    HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(DEBUG2_GPIO_Port, DEBUG2_Pin, GPIO_PIN_RESET);
}

void spi_error_dump_reboot(void)
{
  printf("SPI ERROR\n");
  for (int i = 0; i < SPI_BUF_SIZE; ++i)
    printf("%d ", spi_recv_buf[i]);
  printf("\nrebooting...\n");
  for (int i = 0; i < 100; ++i)
  {
    HAL_GPIO_TogglePin(USER_LED_GPIO_Port, USER_LED_Pin);
    HAL_Delay(100);
  }
  NVIC_SystemReset();
}

const char boot_message[] = "USB4VC Protocol Board\nApple Desktop Bus (ADB)\ndekuNukem 2022";

void protocol_status_lookup_init(void)
{
  memset(protocol_status_lookup, PROTOCOL_STATUS_NOT_AVAILABLE, PROTOCOL_LOOKUP_SIZE);
  protocol_status_lookup[PROTOCOL_ADB_KB] = PROTOCOL_STATUS_ENABLED;
  protocol_status_lookup[PROTOCOL_ADB_MOUSE] = PROTOCOL_STATUS_ENABLED;
}

uint8_t int16_to_uint6(int16_t value)
{
  if(abs(value) <= 3)
    return (uint8_t)value;
  value = value >> 1;
  if(value >= 63)
    value = 63;
  if(value <= -63)
    value = -63;
  return (uint8_t)value;
}

void adb_mouse_update(void)
{
  if(mouse_buf_peek(&my_mouse_buf) == NULL)
    return;

  get_consolidated_mouse_event(&my_mouse_buf, &consolidated_mouse_event);
  // mouse buffer now empty

  uint16_t response = 0;
  if(consolidated_mouse_event.button_left == 0)
    response |= 0x8000;
  if(consolidated_mouse_event.button_right == 0)
    response |= 0x80;
  response |= int16_to_uint6(consolidated_mouse_event.movement_x) & 0x7f;
  response |= (int16_to_uint6(consolidated_mouse_event.movement_y) & 0x7f) << 8;
  adb_send_response_16b(response);
}

#define KEY_DELETE 111
#define KEY_CAPSLOCK 58
#define KEY_LEFTCTRL 29
#define KEY_LEFTSHIFT 42
#define KEY_LEFTALT 56
#define KEY_LEFTMETA 125
#define KEY_NUMLOCK 69
#define KEY_SCROLLLOCK 70

#define KEY_RIGHTCTRL 97
#define KEY_RIGHTSHIFT 54
#define KEY_RIGHTALT 100
#define KEY_RIGHTMETA 126

uint8_t capslock_counter;
void adb_keyboard_update(void)
{
  uint8_t buffered_code, buffered_value, adb_code;
  uint16_t response = 0x8080;
  if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
  {
    if(buffered_code >= EV_TO_ADB_LOOKUP_SIZE)
      goto adb_kb_end;
    adb_code = linux_ev_to_adb_lookup[buffered_code];
    if(adb_code == ADB_KEY_UNKNOWN)
      goto adb_kb_end;
    /*
    adb mac keyboard capslock key seems to behave differently than PC keyboards
    On PC: Capslock down then up = Caps lock on, Capslock down then up again = Caps lock off
    on adb mac: Capslock down = Capslock on, Capslock up = Caps lock off
    looks like mac keyboard uses a locking capslock, so need to convert the PC code to suit
    */
    if(adb_code == ADB_KEY_CAPSLOCK)
    {
      if(buffered_value != 1)
        goto adb_kb_end;
      capslock_counter++;
      buffered_value = capslock_counter % 2;
    }
    if(buffered_value)
      response &= 0x7fff;
    response |= ((uint16_t)adb_code & 0x7f) << 8;
    adb_send_response_16b(response);
    adb_kb_end:
    kb_buf_pop(&my_kb_buf);

    if(buffered_code == KEY_DELETE)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x4000;
      else
        adb_kb_reg2 &= 0xbfff;
    }
    if(buffered_code == KEY_CAPSLOCK)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x2000;
      else
        adb_kb_reg2 &= 0xdfff;
    }
    if(buffered_code == KEY_LEFTCTRL || buffered_code == KEY_RIGHTCTRL)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x800;
      else
        adb_kb_reg2 &= 0xf7ff;
    }
    if(buffered_code == KEY_LEFTSHIFT || buffered_code == KEY_RIGHTSHIFT)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x400;
      else
        adb_kb_reg2 &= 0xfbff;
    }
    if(buffered_code == KEY_LEFTALT || buffered_code == KEY_RIGHTALT)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x200;
      else
        adb_kb_reg2 &= 0xfdff;
    }
    if(buffered_code == KEY_LEFTMETA || buffered_code == KEY_RIGHTMETA)
    {
      if(buffered_value == 0)
        adb_kb_reg2 |= 0x100;
      else
        adb_kb_reg2 &= 0xfeff;
    }
    // if(buffered_code == KEY_NUMLOCK)
    // {
    //   if(buffered_value == 0)
    //     adb_kb_reg2 |= 0x80;
    //   else
    //     adb_kb_reg2 &= 0xff7f;
    // }
    // if(buffered_code == KEY_SCROLLLOCK)
    // {
    //   if(buffered_value == 0)
    //     adb_kb_reg2 |= 0x40;
    //   else
    //     adb_kb_reg2 &= 0xffbf;
    // }
  }
}

void process_spi_kb_events(void)
{
  if(temp_kb_buf_index == 0)
    return;
  for (uint8_t i = 0; i < temp_kb_buf_index; ++i)
    kb_buf_add(&my_kb_buf, temp_kb_code_buf[i], temp_kb_value_buf[i]);
  temp_kb_buf_index = 0;
  memset(temp_kb_code_buf, 0, TEMP_BUF_SIZE);
  memset(temp_kb_value_buf, 0, TEMP_BUF_SIZE);
}

uint8_t power_button_status;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  printf("%s\nrev%d v%d.%d.%d\n", boot_message, hw_revision, version_major, version_minor, version_patch);
  delay_us_init(&htim2);
  protocol_status_lookup_init();
  kb_buf_init(&my_kb_buf, KEYBOARD_EVENT_BUFFER_SIZE);
  mouse_buf_init(&my_mouse_buf, MOUSE_EVENT_BUFFER_SIZE);
  uint8_t adb_data, adb_status, this_addr;
  uint8_t kb_srq = 0;
  uint8_t mouse_srq = 0;
  adb_init(ADB_DATA_GPIO_Port, ADB_DATA_Pin, ADB_PSW_GPIO_Port, ADB_PSW_Pin);
  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    power_button_status = HAL_GPIO_ReadPin(ADB_PSW_GPIO_Port, ADB_PSW_Pin);
    // printf("%d", power_button_status);
    // HAL_Delay(5);
    if(spi_error_occured)
      spi_error_dump_reboot();
    process_spi_kb_events();
    if(temp_spi_msg_buf[0] != 0)
    {
      parse_spi_buf(temp_spi_msg_buf);
      memset(temp_spi_msg_buf, 0, SPI_BUF_SIZE);
    }
    
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    if(IS_ADB_HOST_PRESENT() == 0)
      continue;

    kb_enabled = is_protocol_enabled(PROTOCOL_ADB_KB);
    mouse_enabled = is_protocol_enabled(PROTOCOL_ADB_MOUSE);

    if(kb_enabled == 0 && mouse_enabled == 0)
    {
      adb_reset();
      continue;
    }

    adb_status = adb_recv_cmd(&adb_data);
    adb_rw_in_progress = 0;
    if(adb_status == ADB_LINE_STATUS_RESET)
      adb_reset();
    else if(adb_status != ADB_OK)
      continue;

    // we are just at the end of a ADB command, check address, assert SRQ if needed
    this_addr = adb_data >> 4;
    if(!kb_buf_is_empty(&my_kb_buf))
      kb_srq = (this_addr != adb_kb_current_addr);
    else if(!mouse_buf_is_empty(&my_mouse_buf))
      mouse_srq = (this_addr != adb_mouse_current_addr);
    else
    {
      kb_srq = 0;
      mouse_srq = 0;
    }

    HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, kb_srq || mouse_srq);
    if((kb_srq && kb_enabled) || (mouse_srq && mouse_enabled))
      send_srq();
    else
      wait_until_change(ADB_DEFAULT_TIMEOUT_US);

    adb_status = parse_adb_cmd(adb_data);
    if(adb_status == ADB_MOUSE_POLL && mouse_enabled)
      adb_mouse_update();
    else if(adb_status == ADB_KB_POLL && kb_enabled)
      adb_keyboard_update();
    else if(adb_status == ADB_KB_POLL_REG2 && kb_enabled)
      adb_send_response_16b(adb_kb_reg2);
    else if(adb_status == ADB_KB_CHANGE_LED)
    {
      // kb reg2 top 13 bits is button status, 1 = released 0 = pressed
      // bottom 3 bits is LED, 1 = off, 0 = on
      memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
      spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
      spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_TYPE_KB_LED_REQUEST;
      if(!(adb_kb_reg2 & 0x4)) // scroll lock LED
        spi_transmit_buf[3] = 1;
      if(!(adb_kb_reg2 & 0x1))  // num lock LED
        spi_transmit_buf[4] = 1;
      if(!(adb_kb_reg2 & 0x2))  // caps lock LED
        spi_transmit_buf[5] = 1;
      HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_SET);
    }
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

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
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

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
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

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_HalfDuplex_Init(&huart2) != HAL_OK)
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, DEBUG2_Pin|DEBUG3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ADB_PSW_GPIO_Port, ADB_PSW_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SLAVE_REQ_Pin|DEBUG1_Pin|DEBUG0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ADB_DATA_GPIO_Port, ADB_DATA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DEBUG2_Pin DEBUG3_Pin */
  GPIO_InitStruct.Pin = DEBUG2_Pin|DEBUG3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : ADB_PSW_Pin */
  GPIO_InitStruct.Pin = ADB_PSW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ADB_PSW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SLAVE_REQ_Pin DEBUG1_Pin DEBUG0_Pin */
  GPIO_InitStruct.Pin = SLAVE_REQ_Pin|DEBUG1_Pin|DEBUG0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ADB_5V_DET_Pin */
  GPIO_InitStruct.Pin = ADB_5V_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(ADB_5V_DET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ADB_DATA_Pin */
  GPIO_InitStruct.Pin = ADB_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ADB_DATA_GPIO_Port, &GPIO_InitStruct);

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
