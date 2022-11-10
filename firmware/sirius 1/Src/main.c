
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

const char boot_message[] = "USB4VC Protocol Card\nSirius 1 / Victor 9000\ndekuNukem 2022";

void protocol_status_lookup_init(void)
{
  memset(protocol_status_lookup, PROTOCOL_STATUS_NOT_AVAILABLE, PROTOCOL_LOOKUP_SIZE);
  protocol_status_lookup[PROTOCOL_SIRIUS1_KB] = PROTOCOL_STATUS_ENABLED;
}

#define KB_WRITE_SUCCESS 0
#define KB_WRITE_TIMEOUT 1
#define KB_WRITE_ERROR 2
#define SIRIUS_1_KB_BIT_ACK_TIMEOUT_MS 20

void release_kb_line(void)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(KBRDY_GPIO_Port, KBRDY_Pin, GPIO_PIN_SET);
}

uint8_t wait_for_KBACK(uint8_t level)
{
  return KB_WRITE_SUCCESS;
  uint32_t entry_time = HAL_GetTick();
  while(1)
  {
    if(HAL_GPIO_ReadPin(KBACK_GPIO_Port, KBACK_Pin) == level)
      return KB_WRITE_SUCCESS;
    if(HAL_GetTick() - entry_time > SIRIUS_1_KB_BIT_ACK_TIMEOUT_MS)
      return KB_WRITE_TIMEOUT;
  }
  return KB_WRITE_ERROR;
}

uint8_t write_bit(uint8_t bit)
{
  HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, bit);
  HAL_GPIO_WritePin(KBRDY_GPIO_Port, KBRDY_Pin, GPIO_PIN_RESET);
  if(wait_for_KBACK(GPIO_PIN_RESET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  delay_us(400);
  HAL_GPIO_WritePin(KBRDY_GPIO_Port, KBRDY_Pin, GPIO_PIN_SET);
  if(wait_for_KBACK(GPIO_PIN_SET) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT;
  delay_us(400);
  return KB_WRITE_SUCCESS;
}

uint8_t get_bit(uint8_t x, uint8_t n)
{
  return (x & (1 << n)) ? 1 : 0;
}

uint8_t send_key(uint8_t data, uint8_t key_status)
{
  data &= 0x7f;
  if(key_status)
    data |= 0x80;
  for (int i = 0; i < 8; ++i)
    write_bit(get_bit(data, i));
  write_bit(0); // stop bit
  release_kb_line();
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
  while (1)
  {
    if(spi_error_occured)
      spi_error_dump_reboot();
    if(HAL_GetTick() > led_off_after)
      HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      send_key(0x28, buffered_value);
      kb_buf_pop(&my_kb_buf);
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
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, KBDATA_Pin|KBRDY_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KBDATA_Pin KBRDY_Pin */
  GPIO_InitStruct.Pin = KBDATA_Pin|KBRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SLAVE_REQ_Pin */
  GPIO_InitStruct.Pin = SLAVE_REQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SLAVE_REQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KBACK_Pin */
  GPIO_InitStruct.Pin = KBACK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KBACK_GPIO_Port, &GPIO_InitStruct);

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
