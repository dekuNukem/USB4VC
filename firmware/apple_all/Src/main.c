
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

#define PROTOCOL_STATUS_NOT_AVAILABLE 0
#define PROTOCOL_STATUS_ENABLED 1
#define PROTOCOL_STATUS_DISABLED 2
#define PROTOCOL_LOOKUP_SIZE 16

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
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

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

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

void parse_spi_buf(uint8_t* spibuf)
{
  if(spibuf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_MOUSE_EVENT)
  {
    latest_mouse_event.movement_x = byte_to_int16_t(spibuf[4], spibuf[5]);
    latest_mouse_event.movement_y = 1 * byte_to_int16_t(spibuf[6], spibuf[7]);
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
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{

  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_SET);
  if(spi_recv_buf[0] != 0xde)
    spi_error_occured = 1;
  parse_spi_buf(spi_recv_buf);
  if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_REQ_ACK)
    HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);
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

const char boot_message[] = "USB4VC Protocol Board\nEarly Macintosh & Apple Desktop Bus\ndekuNukem 2022";

#define AVG_BUF_SIZE 8
int32_t avg_buf[AVG_BUF_SIZE];
uint8_t avg_buf_index;

void avg_buf_add(int32_t value)
{
  avg_buf[avg_buf_index] = value;
  avg_buf_index++;
  if (avg_buf_index >= AVG_BUF_SIZE)
    avg_buf_index = 0;
}

int32_t get_buf_avg(void)
{
  int32_t sum = 0;
  for (int i = 0; i < AVG_BUF_SIZE; ++i)
    sum += avg_buf[i];
  if (sum > 0 && sum < AVG_BUF_SIZE)
    sum = AVG_BUF_SIZE;
  else if (sum < 0 && abs(sum) < AVG_BUF_SIZE)
    sum = AVG_BUF_SIZE * -1;
  return (int32_t)(sum/AVG_BUF_SIZE);
}

/*
each speed has a corresponding duration before the next increment or decrement

make sure to enable autoreload preload to prevent glitches

ARR = Auto Reload Register
value = us
*/

uint16_t calc_arr(int32_t speed_val)
{
  speed_val = abs(speed_val);
  if(speed_val <= 0 || speed_val >= ARR_LOOKUP_SIZE)
    return 500;
  return arr_lookup[speed_val];
}

/*
  this gets called every 10ms, fetches mouse event and put them into a running buffer
  a window average is calculated, used to adjust the timer autoreload register
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // every 10ms
  if(htim == &htim17)
  {
    HAL_GPIO_TogglePin(MX1_GPIO_Port, MX1_Pin);
    mouse_event* this_mouse_event = mouse_buf_peek(&my_mouse_buf);
    if(this_mouse_event == NULL)
    {
      avg_buf_add(0);
    }
    else
    {
      avg_buf_add(this_mouse_event->movement_x);
      mouse_buf_pop(&my_mouse_buf);
    }
    avg_speed = get_buf_avg();
    htim16.Instance->ARR = calc_arr(avg_speed);
  }
  // every ARR overflow
  if(htim == &htim16 && avg_speed != 0)
  {
    HAL_GPIO_TogglePin(MX2_GPIO_Port, MX2_Pin);
    // if(avg_speed > 0)
    //   quad_increment(&quad_x);
    // else
    //   quad_decrement(&quad_x);
  }
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
  /* USER CODE BEGIN 2 */
  printf("%s\nrev%d v%d.%d.%d\n", boot_message, hw_revision, version_major, version_minor, version_patch);
  delay_us_init(&htim2);
  kb_buf_init(&my_kb_buf, 16);
  mouse_buf_init(&my_mouse_buf, 16);
  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  /*
    instead of all at once, we remove data from buffer
    at a regular interval, say 5ms, if empty, then theres no movement
    then every 50ms for example the average movement is calculated
    and that is used to update quad encoder?
  */
  quad_init(&htim17, &htim16, GPIOB, GPIO_PIN_13, GPIOB, GPIO_PIN_12);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    if(spi_error_occured)
      spi_error_dump_reboot();
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    // printf("hello\n");
    // HAL_Delay(500);
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
  HAL_GPIO_WritePin(GPIOB, MAC_KB_CLK_Pin|MAC_KB_DATA_Pin|MOUSE_BUTTON_Pin|MY2_Pin 
                          |MY1_Pin|MX2_Pin|ADB_PWR_Pin|ADB_DATA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MX1_GPIO_Port, MX1_Pin, GPIO_PIN_SET);

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

  /*Configure GPIO pins : MAC_KB_CLK_Pin MAC_KB_DATA_Pin MOUSE_BUTTON_Pin MY2_Pin 
                           MY1_Pin MX2_Pin ADB_PWR_Pin ADB_DATA_Pin */
  GPIO_InitStruct.Pin = MAC_KB_CLK_Pin|MAC_KB_DATA_Pin|MOUSE_BUTTON_Pin|MY2_Pin 
                          |MY1_Pin|MX2_Pin|ADB_PWR_Pin|ADB_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : MAC_KB_DET_Pin ADB_DET_Pin */
  GPIO_InitStruct.Pin = MAC_KB_DET_Pin|ADB_DET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MX1_Pin */
  GPIO_InitStruct.Pin = MX1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MX1_GPIO_Port, &GPIO_InitStruct);

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
