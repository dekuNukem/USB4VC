
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
  * COPYRIGHT(c) 2021 STMicroelectronics
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

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t backup_spi1_recv_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
ps2kb_buf my_ps2kb_buf;
ps2mouse_buf my_ps2mouse_buf;
uint8_t ps2kb_host_cmd, ps2mouse_host_cmd, buffered_code, buffered_value, ps2mouse_bus_status, ps2kb_bus_status;
mouse_event latest_mouse_event;
ps2_outgoing_buf my_ps2_outbuf;
#define SERIAL_MOUSE_BUF_SIZE 3
uint8_t serial_mouse_output_buf[SERIAL_MOUSE_BUF_SIZE];
uint8_t serial_mouse_rts_response;
volatile uint8_t rts_active;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (unsigned char *)&ch, 1, 100);
    return ch;
}

int16_t byte_to_int16_t(uint8_t lsb, uint8_t msb)
{
  return (int16_t)((msb << 8) | lsb);
}

/*
  This is called when a new SPI packet is received
  This is part of an ISR, so keep it short!
*/
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_SET);
  memcpy(backup_spi1_recv_buf, spi_recv_buf, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);

  if(backup_spi1_recv_buf[0] != 0xde)
  {
    for (int i = 0; i < 20; ++i)
    {
      HAL_GPIO_TogglePin(ERR_LED_GPIO_Port, ERR_LED_Pin);
      HAL_Delay(100);
    }
    HAL_NVIC_SystemReset();
  }

  if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_KB_EVENT)
    ps2kb_buf_add(&my_ps2kb_buf, backup_spi1_recv_buf[4], backup_spi1_recv_buf[6]);

  if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_MOUSE_EVENT)
  {
    latest_mouse_event.movement_x = byte_to_int16_t(backup_spi1_recv_buf[4], backup_spi1_recv_buf[5]);
    latest_mouse_event.movement_y = -1 * byte_to_int16_t(backup_spi1_recv_buf[6], backup_spi1_recv_buf[7]);
    latest_mouse_event.scroll_vertical = -1 * byte_to_int16_t(backup_spi1_recv_buf[8], backup_spi1_recv_buf[9]);
    latest_mouse_event.button_left = backup_spi1_recv_buf[13];
    latest_mouse_event.button_right = backup_spi1_recv_buf[14];
    latest_mouse_event.button_middle = backup_spi1_recv_buf[15];
    latest_mouse_event.button_side = backup_spi1_recv_buf[16];
    latest_mouse_event.button_extra = backup_spi1_recv_buf[17];
    ps2mouse_buf_add(&my_ps2mouse_buf, &latest_mouse_event);
  }

  if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_REQ_ACK)
    HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
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

  mouse_event* this_mouse_event = ps2mouse_buf_peek(&my_ps2mouse_buf);
  if(this_mouse_event == NULL)
    return;

  if(ps2mouse_get_outgoing_data(this_mouse_event, &my_ps2_outbuf))
  {
    // if return value is not 0, no need to send out packets
    ps2mouse_buf_pop(&my_ps2mouse_buf);
    return;
  }

  // only pop the item if sending is complete
  if(ps2mouse_send_update(&my_ps2_outbuf) == 0)
    ps2mouse_buf_pop(&my_ps2mouse_buf);
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
      spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_KB_LED_REQ;
      spi_transmit_buf[3] = ps2kb_leds;
      HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_SET);
    }
  }

  if(ps2kb_buf_peek(&my_ps2kb_buf, &buffered_code, &buffered_value) == 0)
  {
    ps2kb_press_key(buffered_code, buffered_value);
    ps2kb_buf_pop(&my_ps2kb_buf);
  }
}


// GPIO external interrupt callback
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == UART3_RTS_Pin)
    rts_active = 1;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  ;
}

// https://everything2.com/title/Mouse+protocol
void serial_mouse_update(void)
{
  if(rts_active)
  {
    serial_mouse_rts_response = 0x4d; // 0x4d = 'M'
    HAL_UART_Transmit_IT(&huart3, &serial_mouse_rts_response, 1);
    rts_active = 0;
  }

  mouse_event* this_mouse_event = ps2mouse_buf_peek(&my_ps2mouse_buf);
  if(this_mouse_event == NULL)
    return;

  memset(serial_mouse_output_buf, 0, SERIAL_MOUSE_BUF_SIZE);
  serial_mouse_output_buf[0] = 0xc0;
  if(this_mouse_event->button_left)
    serial_mouse_output_buf[0] |= 0x20;
  if(this_mouse_event->button_right)
    serial_mouse_output_buf[0] |= 0x10;

  uint8_t serial_y = -1 * this_mouse_event->movement_y;
  if(serial_y & 0x80)
    serial_mouse_output_buf[0] |= 0x8;
  if(serial_y & 0x40)
    serial_mouse_output_buf[0] |= 0x4;
  if(this_mouse_event->movement_x & 0x80)
    serial_mouse_output_buf[0] |= 0x2;
  if(this_mouse_event->movement_x & 0x40)
    serial_mouse_output_buf[0] |= 0x1;

  serial_mouse_output_buf[1] = 0x3f & this_mouse_event->movement_x;
  serial_mouse_output_buf[2] = 0x3f & serial_y;

  ps2mouse_buf_pop(&my_ps2mouse_buf);
  HAL_UART_Transmit_IT(&huart3, serial_mouse_output_buf, 3);
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
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  delay_us_init(&htim2);
  ps2kb_init(PS2KB_CLK_GPIO_Port, PS2KB_CLK_Pin, PS2KB_DATA_GPIO_Port, PS2KB_DATA_Pin);
  ps2mouse_init(PS2MOUSE_CLK_GPIO_Port, PS2MOUSE_CLK_Pin, PS2MOUSE_DATA_GPIO_Port, PS2MOUSE_DATA_Pin);
  ps2kb_buf_init(&my_ps2kb_buf, 16);
  ps2mouse_buf_init(&my_ps2mouse_buf, 16);
  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  mcp4451_reset();
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  printf("hello world\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // DONT FORGET
  // huart3.Init.WordLength = UART_WORDLENGTH_7B;

  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    ps2mouse_update();
    // serial_mouse_update();
    ps2kb_update();
    // printf("%d\n", mcp4451_write_wiper(3, 20));
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
  huart3.Init.WordLength = UART_WORDLENGTH_7B;
  huart3.Init.StopBits = UART_STOPBITS_1;
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

  /*Configure GPIO pins : POT_RESET_Pin ERR_LED_Pin */
  GPIO_InitStruct.Pin = POT_RESET_Pin|ERR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ACT_LED_Pin */
  GPIO_InitStruct.Pin = ACT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACT_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PS2MOUSE_DATA_Pin PS2MOUSE_CLK_Pin PS2KB_DATA_Pin PS2KB_CLK_Pin */
  GPIO_InitStruct.Pin = PS2MOUSE_DATA_Pin|PS2MOUSE_CLK_Pin|PS2KB_DATA_Pin|PS2KB_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
