
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
#include "quad_encoder.h"
#include "m0110a.h"
#include "lisa_kb.h"

#define PROTOCOL_STATUS_NOT_AVAILABLE 0
#define PROTOCOL_STATUS_ENABLED 1
#define PROTOCOL_STATUS_DISABLED 2
#define PROTOCOL_LOOKUP_SIZE 16

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim14;
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

const uint8_t board_id = 3;
const uint8_t version_major = 0;
const uint8_t version_minor = 0;
const uint8_t version_patch = 1;
uint8_t hw_revision = 0;

uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
kb_buf my_kb_buf;
m0110a_cmd_buf my_m0110a_buf;

mouse_buf my_mouse_buf;
mouse_event latest_mouse_event;
uint8_t spi_error_occured;
uint8_t protocol_status_lookup[PROTOCOL_LOOKUP_SIZE];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM16_Init(void);
static void MX_TIM17_Init(void);
static void MX_TIM14_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
const uint8_t lisa_byte_lookup[256] = {0xff, 0xf7, 0xfb, 0xf3, 0xfd, 0xf5, 0xf9, 0xf1, 0xfe, 0xf6, 0xfa, 0xf2, 0xfc, 0xf4, 0xf8, 0xf0, 0x7f, 0x77, 0x7b, 0x73, 0x7d, 0x75, 0x79, 0x71, 0x7e, 0x76, 0x7a, 0x72, 0x7c, 0x74, 0x78, 0x70, 0xbf, 0xb7, 0xbb, 0xb3, 0xbd, 0xb5, 0xb9, 0xb1, 0xbe, 0xb6, 0xba, 0xb2, 0xbc, 0xb4, 0xb8, 0xb0, 0x3f, 0x37, 0x3b, 0x33, 0x3d, 0x35, 0x39, 0x31, 0x3e, 0x36, 0x3a, 0x32, 0x3c, 0x34, 0x38, 0x30, 0xdf, 0xd7, 0xdb, 0xd3, 0xdd, 0xd5, 0xd9, 0xd1, 0xde, 0xd6, 0xda, 0xd2, 0xdc, 0xd4, 0xd8, 0xd0, 0x5f, 0x57, 0x5b, 0x53, 0x5d, 0x55, 0x59, 0x51, 0x5e, 0x56, 0x5a, 0x52, 0x5c, 0x54, 0x58, 0x50, 0x9f, 0x97, 0x9b, 0x93, 0x9d, 0x95, 0x99, 0x91, 0x9e, 0x96, 0x9a, 0x92, 0x9c, 0x94, 0x98, 0x90, 0x1f, 0x17, 0x1b, 0x13, 0x1d, 0x15, 0x19, 0x11, 0x1e, 0x16, 0x1a, 0x12, 0x1c, 0x14, 0x18, 0x10, 0xef, 0xe7, 0xeb, 0xe3, 0xed, 0xe5, 0xe9, 0xe1, 0xee, 0xe6, 0xea, 0xe2, 0xec, 0xe4, 0xe8, 0xe0, 0x6f, 0x67, 0x6b, 0x63, 0x6d, 0x65, 0x69, 0x61, 0x6e, 0x66, 0x6a, 0x62, 0x6c, 0x64, 0x68, 0x60, 0xaf, 0xa7, 0xab, 0xa3, 0xad, 0xa5, 0xa9, 0xa1, 0xae, 0xa6, 0xaa, 0xa2, 0xac, 0xa4, 0xa8, 0xa0, 0x2f, 0x27, 0x2b, 0x23, 0x2d, 0x25, 0x29, 0x21, 0x2e, 0x26, 0x2a, 0x22, 0x2c, 0x24, 0x28, 0x20, 0xcf, 0xc7, 0xcb, 0xc3, 0xcd, 0xc5, 0xc9, 0xc1, 0xce, 0xc6, 0xca, 0xc2, 0xcc, 0xc4, 0xc8, 0xc0, 0x4f, 0x47, 0x4b, 0x43, 0x4d, 0x45, 0x49, 0x41, 0x4e, 0x46, 0x4a, 0x42, 0x4c, 0x44, 0x48, 0x40, 0x8f, 0x87, 0x8b, 0x83, 0x8d, 0x85, 0x89, 0x81, 0x8e, 0x86, 0x8a, 0x82, 0x8c, 0x84, 0x88, 0x80, 0x0f, 0x07, 0x0b, 0x03, 0x0d, 0x05, 0x09, 0x01, 0x0e, 0x06, 0x0a, 0x02, 0x0c, 0x04, 0x08, 0x00};

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (unsigned char *)&ch, 1, 10);
    return ch;
}

int16_t byte_to_int16_t(uint8_t lsb, uint8_t msb)
{
  return (int16_t)((msb << 8) | lsb);
}

void protocol_status_lookup_init(void)
{
  memset(protocol_status_lookup, PROTOCOL_STATUS_NOT_AVAILABLE, PROTOCOL_LOOKUP_SIZE);
  protocol_status_lookup[PROTOCOL_ADB_KB] = PROTOCOL_STATUS_ENABLED;
  protocol_status_lookup[PROTOCOL_ADB_MOUSE] = PROTOCOL_STATUS_ENABLED;
  protocol_status_lookup[PROTOCOL_M0100_MOUSE] = PROTOCOL_STATUS_DISABLED;
  protocol_status_lookup[PROTOCOL_M0110_KB] = PROTOCOL_STATUS_DISABLED;
  protocol_status_lookup[PROTOCOL_LISA_KB] = PROTOCOL_STATUS_DISABLED;
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
        break;

      case PROTOCOL_ADB_MOUSE:
        break;

      case PROTOCOL_M0100_MOUSE:
        quad_enable();
        break;

      case PROTOCOL_M0110_KB:
        break;

      case PROTOCOL_LISA_KB:
        break;
    }
    protocol_status_lookup[index] = PROTOCOL_STATUS_ENABLED;
  }
  // switching protocol OFF
  else if((onoff == 0) && protocol_status_lookup[index] == PROTOCOL_STATUS_ENABLED)
  {
    switch(index)
    {
      case PROTOCOL_ADB_KB:
        break;

      case PROTOCOL_ADB_MOUSE:
        break;

      case PROTOCOL_M0100_MOUSE:
        quad_disable();
        break;

      case PROTOCOL_M0110_KB:
        break;

      case PROTOCOL_LISA_KB:
        break;
    }
    protocol_status_lookup[index] = PROTOCOL_STATUS_DISABLED;
  }
}

void parse_spi_buf(uint8_t* spibuf)
{
  if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_MOUSE_EVENT)
  {
    latest_mouse_event.movement_x = byte_to_int16_t(spibuf[4], spibuf[5]);
    latest_mouse_event.movement_y = 1 * byte_to_int16_t(spibuf[6], spibuf[7]);
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

volatile uint32_t ACT_LED_off_ts;
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_SET);
  if(spi_recv_buf[0] != 0xde)
    spi_error_occured = 1;
  parse_spi_buf(spi_recv_buf);
  if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_REQ_ACK)
    HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  ACT_LED_off_ts = micros() + 10000;
}

void spi_error_dump_reboot(void)
{
  printf("SPI ERROR\n");
  for (int i = 0; i < SPI_BUF_SIZE; ++i)
    printf("%d ", spi_recv_buf[i]);
  printf("\nrebooting...\n");
  for (int i = 0; i < 100; ++i)
  {
    HAL_GPIO_TogglePin(ERR_LED_GPIO_Port, ERR_LED_Pin);
    HAL_Delay(100);
  }
  NVIC_SystemReset();
}

const char boot_message[] = "USB4VC Protocol Board\nApple Pre-USB\ndekuNukem 2022";

/*

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

REMEMBER TO ENABLE ARR PRELOAD ON TIMER 16

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

Configure GPIO pins : M0110_CLK_Pin

*/
uint8_t m0110a_inquiry_active;
uint32_t m0110a_last_inquiry;
uint8_t buffered_code, buffered_value;

void m0110a_write_1b_from_buf(void)
{
  uint8_t m0100a_byte;
  m0110a_cmd_buf_peek(&my_m0110a_buf, &m0100a_byte);
  m0110a_cmd_buf_pop(&my_m0110a_buf);
  m0110a_write(m0100a_byte);
}

void m0110a_update(void)
{
  static uint8_t m0110a_host_cmd, m0110a_status;

  m0110a_status = m0110a_read_host_cmd(&m0110a_host_cmd, 600);
  if(m0110a_status != M0110A_OK)
    return;

  if(m0110a_host_cmd == 0x10) // inquiry
  {
    m0110a_inquiry_active = 1;
    m0110a_last_inquiry = HAL_GetTick();
  }
  else if(m0110a_host_cmd == 0x14) // instant
  {
    if(m0110a_cmd_buf_is_empty(&my_m0110a_buf) == 0)
      m0110a_write_1b_from_buf();
    else
      m0110a_write(0x7b);
  }
  else if(m0110a_host_cmd == 0x16) // model number
    m0110a_write(0xb);
  else if(m0110a_host_cmd == 0x36) // test
    m0110a_write(0x7d);
  // printf("r%x", m0110a_host_cmd);
}

void m0100a_handle_inquiry(void)
{
  // "If no key transition has occurred after 0.25 seconds
  // the keyboard sends back a Null response to let the computer know it's still there"
  if(m0110a_inquiry_active && HAL_GetTick() - m0110a_last_inquiry > 250)
  {
    m0110a_write(0x7b);
    m0110a_inquiry_active = 0;
  }
  else if(m0110a_inquiry_active && m0110a_cmd_buf_is_empty(&my_m0110a_buf) == 0)
  {
    m0110a_write_1b_from_buf();
    m0110a_inquiry_active = 0;
  }
  else if(m0110a_inquiry_active && kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
  {
    m0110a_cmd_buf_reset(&my_m0110a_buf);
    if(make_m0110a_scancode(buffered_code, buffered_value, &my_m0110a_buf) == M0110A_OK)
    {
      m0110a_write_1b_from_buf();
      m0110a_inquiry_active = 0;
    }
    kb_buf_pop(&my_kb_buf);
  }
}


#define LISA_DATA_HI() HAL_GPIO_WritePin(LISA_DATA_GPIO_Port, LISA_DATA_Pin, GPIO_PIN_SET)
#define LISA_DATA_LOW() HAL_GPIO_WritePin(LISA_DATA_GPIO_Port, LISA_DATA_Pin, GPIO_PIN_RESET)
#define LISA_READ_DATA_PIN() HAL_GPIO_ReadPin(LISA_DATA_GPIO_Port, LISA_DATA_Pin)
#define LISA_TIMEOUT -1

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
  // to_write = 0x55;
  // to_write = 0xaa;
  for (int i = 7; i >= 0; i--)
  {
    HAL_GPIO_WritePin(LISA_DATA_GPIO_Port, LISA_DATA_Pin, (to_write >> i) & 1);
    delay_us(16);
    if(i == 4)
      delay_us(14);
  }
}

void lisa_kb_update(void)
{
  // if(LISA_READ_DATA_PIN() == GPIO_PIN_SET)
  //   return;
  // int32_t low_duration = wait_until_change(50000);
  // if(low_duration == LISA_TIMEOUT)
  //   return;
  // now line is high
  printf("1");
  delay_us(15);
  LISA_DATA_LOW();
  delay_us(18); // ACK pulse
  lisa_write_byte(0x80);
  LISA_DATA_HI();
  printf("1");
  HAL_Delay(1000);
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
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  MX_TIM14_Init();
  /* USER CODE BEGIN 2 */
  printf("%s\nrev%d v%d.%d.%d\n", boot_message, hw_revision, version_major, version_minor, version_patch);
  delay_us_init(&htim2);
  protocol_status_lookup_init();

  m0110a_cmd_buf_init(&my_m0110a_buf);
  kb_buf_init(&my_kb_buf);
  mouse_buf_init(&my_mouse_buf);
  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);

  quad_init(&my_mouse_buf, &htim17, &htim16, &htim14, GPIOA, GPIO_PIN_8, GPIOB, GPIO_PIN_14, GPIOB, GPIO_PIN_15, GPIOB, GPIO_PIN_12);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (1)
  {
    // if(spi_error_occured)
    //   spi_error_dump_reboot();
    // if(micros() > ACT_LED_off_ts)
    //   HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

    // m0110 keyboard loop
    // if(HAL_GPIO_ReadPin(M0110_CLK_GPIO_Port, M0110_CLK_Pin) != GPIO_PIN_RESET)
    // {
    //   m0110a_update();
    //   m0100a_handle_inquiry();
    // }
    lisa_kb_update();
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

/* TIM14 init function */
static void MX_TIM14_Init(void)
{

  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 47;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 65535;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM16 init function */
static void MX_TIM16_Init(void)
{

  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 47;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 65535;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM17 init function */
static void MX_TIM17_Init(void)
{

  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 47;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 10000;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LISA_DATA_Pin|QMOUSE_X2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, M0110_GND_Pin|ERR_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, M0110_DATA_Pin|M0110_CLK_Pin|QMOUSE_Y2_Pin|QMOUSE_BUTTON_Pin 
                          |QMOUSE_X1_Pin|QMOUSE_Y1_Pin|ADB_PWR_Pin|ADB_DATA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SLAVE_REQ_Pin */
  GPIO_InitStruct.Pin = SLAVE_REQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SLAVE_REQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LISA_DATA_Pin QMOUSE_X2_Pin */
  GPIO_InitStruct.Pin = LISA_DATA_Pin|QMOUSE_X2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LISA_DET_Pin M0110_DET_Pin QMOUSE_DET_Pin ADB_DET_Pin */
  GPIO_InitStruct.Pin = LISA_DET_Pin|M0110_DET_Pin|QMOUSE_DET_Pin|ADB_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : M0110_GND_Pin ERR_LED_Pin */
  GPIO_InitStruct.Pin = M0110_GND_Pin|ERR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : M0110_DATA_Pin M0110_CLK_Pin QMOUSE_Y2_Pin QMOUSE_BUTTON_Pin 
                           QMOUSE_X1_Pin QMOUSE_Y1_Pin ADB_PWR_Pin ADB_DATA_Pin */
  GPIO_InitStruct.Pin = M0110_DATA_Pin|M0110_CLK_Pin|QMOUSE_Y2_Pin|QMOUSE_BUTTON_Pin 
                          |QMOUSE_X1_Pin|QMOUSE_Y1_Pin|ADB_PWR_Pin|ADB_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ACT_LED_Pin */
  GPIO_InitStruct.Pin = ACT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACT_LED_GPIO_Port, &GPIO_InitStruct);

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
