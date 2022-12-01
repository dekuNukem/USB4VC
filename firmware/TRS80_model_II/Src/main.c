
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

#define PROTOCOL_STATUS_NOT_AVAILABLE 0
#define PROTOCOL_STATUS_ENABLED 1
#define PROTOCOL_STATUS_DISABLED 2
#define PROTOCOL_LOOKUP_SIZE 16

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
const uint8_t board_id = 5;
const uint8_t version_major = 0;
const uint8_t version_minor = 0;
const uint8_t version_patch = 1;

uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
kb_buf my_kb_buf;
uint8_t spi_error_occured;
uint8_t protocol_status_lookup[PROTOCOL_LOOKUP_SIZE];
uint32_t led_off_after;
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
      case PROTOCOL_SIRIUS1_KB:
        kb_buf_reset(&my_kb_buf);
        break;
    }
    protocol_status_lookup[index] = PROTOCOL_STATUS_ENABLED;
  }
  // switching protocol OFF
  else if((onoff == 0) && protocol_status_lookup[index] == PROTOCOL_STATUS_ENABLED)
    protocol_status_lookup[index] = PROTOCOL_STATUS_DISABLED;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);
  led_off_after = HAL_GetTick() + 30;
  if(spi_recv_buf[0] != 0xde)
    spi_error_occured = 1;
  
  if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT)
  {
    kb_buf_add(&my_kb_buf, spi_recv_buf[4], spi_recv_buf[6]);
  }
  else if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_INFO_REQUEST)
  {
    memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
    spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
    spi_transmit_buf[SPI_BUF_INDEX_SEQNUM] = spi_recv_buf[SPI_BUF_INDEX_SEQNUM];
    spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_TYPE_INFO_REQUEST;
    spi_transmit_buf[3] = board_id;
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
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
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

const char boot_message[] = "USB4VC Protocol Card\nTRS-80 Model II\ndekuNukem 2022";

void protocol_status_lookup_init(void)
{
  memset(protocol_status_lookup, PROTOCOL_STATUS_NOT_AVAILABLE, PROTOCOL_LOOKUP_SIZE);
  protocol_status_lookup[PROTOCOL_SIRIUS1_KB] = PROTOCOL_STATUS_ENABLED;
}

#define EV_LOOKUP_SIZE 128
#define KEY_UNUSED 126

const uint8_t linux_ev_to_sirius1_lookup[EV_LOOKUP_SIZE] = 
{
  KEY_UNUSED, // EV0 KEY_RESERVED
  0xb, // EV1 KEY_ESC
  0xd, // EV2 KEY_1
  0xe, // EV3 KEY_2
  0xf, // EV4 KEY_3
  0x10, // EV5 KEY_4
  0x11, // EV6 KEY_5
  0x12, // EV7 KEY_6
  0x13, // EV8 KEY_7
  0x14, // EV9 KEY_8
  0x15, // EV10 KEY_9
  0x16, // EV11 KEY_0
  0x17, // EV12 KEY_MINUS
  0x18, // EV13 KEY_EQUAL
  0x19, // EV14 KEY_BACKSPACE
  0x21, // EV15 KEY_TAB
  0x22, // EV16 KEY_Q
  0x23, // EV17 KEY_W
  0x24, // EV18 KEY_E
  0x25, // EV19 KEY_R
  0x26, // EV20 KEY_T
  0x27, // EV21 KEY_Y
  0x28, // EV22 KEY_U
  0x29, // EV23 KEY_I
  0x2a, // EV24 KEY_O
  0x2b, // EV25 KEY_P
  0x2c, // EV26 KEY_LEFTBRACE
  0x2d, // EV27 KEY_RIGHTBRACE
  0x57, // EV28 KEY_ENTER
  0x61, // EV29 KEY_LEFTCTRL
  0x37, // EV30 KEY_A
  0x38, // EV31 KEY_S
  0x39, // EV32 KEY_D
  0x3a, // EV33 KEY_F
  0x3b, // EV34 KEY_G
  0x3c, // EV35 KEY_H
  0x3d, // EV36 KEY_J
  0x3e, // EV37 KEY_K
  0x3f, // EV38 KEY_L
  0x40, // EV39 KEY_SEMICOLON
  0x41, // EV40 KEY_APOSTROPHE
  KEY_UNUSED, // EV41 KEY_GRAVE
  0x4a, // EV42 KEY_LEFTSHIFT
  0x4b, // EV43 KEY_BACKSLASH
  0x4c, // EV44 KEY_Z
  0x4d, // EV45 KEY_X
  0x4e, // EV46 KEY_C
  0x4f, // EV47 KEY_V
  0x50, // EV48 KEY_B
  0x51, // EV49 KEY_N
  0x52, // EV50 KEY_M
  0x53, // EV51 KEY_COMMA
  0x54, // EV52 KEY_DOT
  0x55, // EV53 KEY_SLASH
  0x56, // EV54 KEY_RIGHTSHIFT
  0x1f, // EV55 KEY_KPASTERISK
  0x5f, // EV56 KEY_LEFTALT
  0x60, // EV57 KEY_SPACE
  0x36, // EV58 KEY_CAPSLOCK
  0x1, // EV59 KEY_F1
  0x2, // EV60 KEY_F2
  0x3, // EV61 KEY_F3
  0x4, // EV62 KEY_F4
  0x5, // EV63 KEY_F5
  0x6, // EV64 KEY_F6
  0x7, // EV65 KEY_F7
  0x8, // EV66 KEY_F8
  KEY_UNUSED, // EV67 KEY_F9
  KEY_UNUSED, // EV68 KEY_F10
  0x1c, // EV69 KEY_NUMLOCK
  KEY_UNUSED, // EV70 KEY_SCROLLLOCK
  0x31, // EV71 KEY_KP7
  0x32, // EV72 KEY_KP8
  0x33, // EV73 KEY_KP9
  0x34, // EV74 KEY_KPMINUS
  0x45, // EV75 KEY_KP4
  0x46, // EV76 KEY_KP5
  0x47, // EV77 KEY_KP6
  0x48, // EV78 KEY_KPPLUS
  0x5a, // EV79 KEY_KP1
  0x5b, // EV80 KEY_KP2
  0x5c, // EV81 KEY_KP3
  0x64, // EV82 KEY_KP0
  0x66, // EV83 KEY_KPDOT
  KEY_UNUSED, // EV84 UNKNOWN
  KEY_UNUSED, // EV85 KEY_ZENKAKUHANKAKU
  KEY_UNUSED, // EV86 KEY_102ND
  KEY_UNUSED, // EV87 KEY_F11
  KEY_UNUSED, // EV88 KEY_F12
  KEY_UNUSED, // EV89 KEY_RO
  KEY_UNUSED, // EV90 KEY_KATAKANA
  KEY_UNUSED, // EV91 KEY_HIRAGANA
  KEY_UNUSED, // EV92 KEY_HENKAN
  KEY_UNUSED, // EV93 KEY_KATAKANAHIRAGANA
  KEY_UNUSED, // EV94 KEY_MUHENKAN
  KEY_UNUSED, // EV95 KEY_KPJPCOMMA
  0x5d, // EV96 KEY_KPENTER
  0x61, // EV97 KEY_RIGHTCTRL
  KEY_UNUSED, // EV98 KEY_KPSLASH
  KEY_UNUSED, // EV99 KEY_SYSRQ
  0x5f, // EV100 KEY_RIGHTALT
  KEY_UNUSED, // EV101 KEY_LINEFEED
  KEY_UNUSED, // EV102 KEY_HOME
  88, // EV103 KEY_UP
  KEY_UNUSED, // EV104 KEY_PAGEUP
  0x62, // EV105 KEY_LEFT
  99, // EV106 KEY_RIGHT
  KEY_UNUSED, // EV107 KEY_END
  0x59, // EV108 KEY_DOWN
  KEY_UNUSED, // EV109 KEY_PAGEDOWN
  47, // EV110 KEY_INSERT
  27, // EV111 KEY_DELETE
  KEY_UNUSED, // EV112 KEY_MACRO
  KEY_UNUSED, // EV113 KEY_MUTE
  KEY_UNUSED, // EV114 KEY_VOLUMEDOWN
  KEY_UNUSED, // EV115 KEY_VOLUMEUP
  KEY_UNUSED, // EV116 KEY_POWER
  KEY_UNUSED, // EV117 KEY_KPEQUAL
  KEY_UNUSED, // EV118 KEY_KPPLUSMINUS
  KEY_UNUSED, // EV119 KEY_PAUSE
  KEY_UNUSED, // EV120 KEY_SCALE
  KEY_UNUSED, // EV121 KEY_KPCOMMA
  KEY_UNUSED, // EV122 KEY_HANGEUL
  KEY_UNUSED, // EV123 KEY_HANJA
  KEY_UNUSED, // EV124 KEY_YEN
  KEY_UNUSED, // EV125 KEY_LEFTMETA
  KEY_UNUSED, // EV126 KEY_RIGHTMETA
  KEY_UNUSED, // EV127 KEY_COMPOSE
};

#define KB_WRITE_SUCCESS 0
#define KB_WRITE_TIMEOUT 1
#define KB_WRITE_ERROR 2
#define SIRIUS_1_KB_BIT_ACK_TIMEOUT_MS 20

void sirius1_release_kb_line(void)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_SET);
}

uint8_t sirius1_wait_for_KBACK(uint8_t level)
{
  // return KB_WRITE_SUCCESS;
  uint32_t entry_time = HAL_GetTick();
  while(1)
  {
    if(HAL_GPIO_ReadPin(KBACK_BUSY_GPIO_Port, KBACK_BUSY_Pin) == level)
      return KB_WRITE_SUCCESS;
    if(HAL_GetTick() - entry_time > SIRIUS_1_KB_BIT_ACK_TIMEOUT_MS)
      return KB_WRITE_TIMEOUT;
  }
  return KB_WRITE_ERROR;
}

uint8_t sirius1_write_stop_bit()
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_RESET);
  if(sirius1_wait_for_KBACK(GPIO_PIN_RESET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_SET);
  HAL_Delay(1);
  if(sirius1_wait_for_KBACK(GPIO_PIN_SET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  return KB_WRITE_SUCCESS;
}

uint8_t sirius1_write_bit(uint8_t bit)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, bit);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_RESET);
  if(sirius1_wait_for_KBACK(GPIO_PIN_RESET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  delay_us(400);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_SET);
  if(sirius1_wait_for_KBACK(GPIO_PIN_SET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  delay_us(400);
  return KB_WRITE_SUCCESS;
}

uint8_t get_bit(uint8_t x, uint8_t n)
{
  return (x & (1 << n)) ? 1 : 0;
}

uint8_t sirius1_send_key(uint8_t key_code, uint8_t key_status)
{
  if(key_code == KEY_UNUSED)
    return KB_WRITE_SUCCESS;
  key_code &= 0x7f;
  if(key_status)
    key_code |= 0x80;
  for (int i = 0; i < 8; ++i)
    sirius1_write_bit(get_bit(key_code, i));
  sirius1_write_stop_bit();
  sirius1_release_kb_line();
  return KB_WRITE_SUCCESS;
}

void trs80m2_release_kb_line(void)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_RESET);
}

uint8_t trs80m2_write_bit(uint8_t bit)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, bit);
  delay_us(81);
  HAL_GPIO_WritePin(KBRDY_CLK_GPIO_Port, KBRDY_CLK_Pin, GPIO_PIN_SET);
  delay_us(108);
  trs80m2_release_kb_line();
  return KB_WRITE_SUCCESS;
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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  printf("%s v%d.%d.%d\n", boot_message, version_major, version_minor, version_patch);
  delay_us_init(&htim2);
  protocol_status_lookup_init();
  kb_buf_init(&my_kb_buf);
  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t buffered_code, buffered_value;
  trs80m2_release_kb_line();

  while (1)
  {
    if(spi_error_occured)
      spi_error_dump_reboot();
    if(HAL_GetTick() > led_off_after)
      HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

    trs80m2_write_bit(0);
    HAL_Delay(10);

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    // if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    // {
    //   if(buffered_code < EV_LOOKUP_SIZE && buffered_value != 2)
    //     sirius1_send_key(linux_ev_to_sirius1_lookup[buffered_code], buffered_value);
    //   kb_buf_pop(&my_kb_buf);
    // }
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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, KBDATA_Pin|KBRDY_CLK_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KBDATA_Pin KBRDY_CLK_Pin */
  GPIO_InitStruct.Pin = KBDATA_Pin|KBRDY_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : KB_5V_Pin */
  GPIO_InitStruct.Pin = KB_5V_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KB_5V_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KBACK_BUSY_Pin */
  GPIO_InitStruct.Pin = KBACK_BUSY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KBACK_BUSY_GPIO_Port, &GPIO_InitStruct);

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
