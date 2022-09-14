
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
#include <string.h>

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
const uint8_t board_id = 4;
const uint8_t version_major = 0;
const uint8_t version_minor = 0;
const uint8_t version_patch = 1;
const char boot_message[] = "USB4VC Protocol Board\nBBC Micro/Master\ndekuNukem 2022";

uint8_t spi_transmit_buf[SPI_BUF_SIZE];
uint8_t backup_spi1_recv_buf[SPI_BUF_SIZE];
uint8_t spi_recv_buf[SPI_BUF_SIZE];
kb_buf my_kb_buf;
mouse_buf my_mouse_buf;
gamepad_buf my_gamepad_buf;
uint8_t spi_error_occured;
uint8_t buffered_code, buffered_value;
gamepad_event latest_gamepad_event;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */


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
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT && backup_spi1_recv_buf[6] <= 1)
  {
    kb_buf_add(&my_kb_buf, backup_spi1_recv_buf[4], backup_spi1_recv_buf[6]);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW_SUPPORTED)
  {
    latest_gamepad_event.button_1 = (backup_spi1_recv_buf[8] & 0x1) != 0;
    latest_gamepad_event.button_2 = (backup_spi1_recv_buf[8] & 0x2) != 0;
    latest_gamepad_event.button_3 = (backup_spi1_recv_buf[8] & 0x4) != 0;
    latest_gamepad_event.button_4 = (backup_spi1_recv_buf[8] & 0x8) != 0;
    latest_gamepad_event.axis_x = backup_spi1_recv_buf[10];
    latest_gamepad_event.axis_y = backup_spi1_recv_buf[11];
    latest_gamepad_event.axis_rx = backup_spi1_recv_buf[12];
    latest_gamepad_event.axis_ry = backup_spi1_recv_buf[13];
    gamepad_buf_add(&my_gamepad_buf, &latest_gamepad_event);
  }
  else if(backup_spi1_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MOSI_MSG_TYPE_INFO_REQUEST)
  {
    memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
    spi_transmit_buf[SPI_BUF_INDEX_MAGIC] = SPI_MISO_MAGIC;
    spi_transmit_buf[SPI_BUF_INDEX_MSG_TYPE] = SPI_MISO_MSG_TYPE_INFO_REQUEST;
    spi_transmit_buf[3] = board_id;
    // spi_transmit_buf[4] = hw_revision;
    spi_transmit_buf[5] = version_major;
    spi_transmit_buf[6] = version_minor;
    spi_transmit_buf[7] = version_patch;
    // uint8_t curr_index = 8;
    // for (int i = 0; i < PROTOCOL_LOOKUP_SIZE; i++)
    // {
    //   if(protocol_status_lookup[i] == PROTOCOL_STATUS_NOT_AVAILABLE)
    //     continue;
    //   else if(protocol_status_lookup[i] == PROTOCOL_STATUS_DISABLED)
    //     spi_transmit_buf[curr_index] = i;
    //   else if(protocol_status_lookup[i] == PROTOCOL_STATUS_ENABLED)
    //     spi_transmit_buf[curr_index] = i | 0x80;
    //   curr_index++;
    // }
  }
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  HAL_GPIO_WritePin(ACT_LED_GPIO_Port, ACT_LED_Pin, GPIO_PIN_RESET);
}

int fputc(int ch, FILE *f)
{
  // HAL_UART_Transmit(&huart1, (unsigned char *)&ch, 1, 100);
  return ch;
}

#define DEBUG_HI() (GPIOA->BSRR = 0x00000400)
#define DEBUG_LOW() (GPIOA->BSRR = 0x04000000)

#define DEBUG2_HI() (GPIOA->BSRR = 0x00000200)
#define DEBUG2_LOW() (GPIOA->BSRR = 0x02000000)

#define CA2_HI() (GPIOB->BSRR = 0x00008000)
#define CA2_LOW() (GPIOB->BSRR = 0x80000000)
#define W_HI() (GPIOA->BSRR = 0x00000100)
#define W_LOW() (GPIOA->BSRR = 0x01000000)
#define IS_KB_EN_HI() (GPIOB->IDR & 0x4)

#define COL_SIZE 16
#define ROW_SIZE 8
uint8_t col_status[COL_SIZE];
uint8_t matrix_status[COL_SIZE][ROW_SIZE];

uint8_t has_active_keys(void)
{
  for(int i = 0; i < COL_SIZE; ++i)
    if(col_status[i])
      return 1;
  return 0;
}

uint32_t last_ca2;

#define CODE_UNUSED 0
#define CODE_SPECIAL 0xff
// top 4 bits column, lower 4 bits row
#define LINUX_KEYCODE_TO_BBC_SIZE 128
const uint8_t linux_keycode_to_bbc_matrix_lookup[LINUX_KEYCODE_TO_BBC_SIZE] = 
{
  CODE_UNUSED, // KEY_RESERVED    0
  0x07, // KEY_ESC    1
  0x03, // KEY_1    2
  0x13, // KEY_2    3
  0x11, // KEY_3    4
  0x21, // KEY_4    5
  0x31, // KEY_5    6
  0x43, // KEY_6    7
  0x42, // KEY_7    8
  0x51, // KEY_8    9
  0x62, // KEY_9    10
  0x72, // KEY_0    11
  0x71, // KEY_MINUS    12
  CODE_UNUSED, // KEY_EQUAL    13
  0x95, // KEY_BACKSPACE    14
  0x06, // KEY_TAB    15
  0x01, // KEY_Q    16
  0x12, // KEY_W    17
  0x22, // KEY_E    18
  0x33, // KEY_R    19
  0x32, // KEY_T    20
  0x44, // KEY_Y    21
  0x53, // KEY_U    22
  0x52, // KEY_I    23
  0x63, // KEY_O    24
  0x73, // KEY_P    25
  0x83, // KEY_LEFTBRACE    26
  0x85, // KEY_RIGHTBRACE    27
  0x94, // KEY_ENTER    28
  0x10, // KEY_LEFTCTRL    29
  0x14, // KEY_A    30
  0x15, // KEY_S    31
  0x23, // KEY_D    32
  0x34, // KEY_F    33
  0x35, // KEY_G    34
  0x45, // KEY_H    35
  0x54, // KEY_J    36
  0x64, // KEY_K    37
  0x65, // KEY_L    38
  0X75, // KEY_SEMICOLON    39
  CODE_UNUSED, // KEY_APOSTROPHE    40
  CODE_UNUSED, // KEY_GRAVE    41
  0x00, // KEY_LEFTSHIFT    42
  0x87, // KEY_BACKSLASH    43
  0x16, // KEY_Z    44
  0x24, // KEY_X    45
  0x25, // KEY_C    46
  0x36, // KEY_V    47
  0x46, // KEY_B    48
  0x55, // KEY_N    49
  0x56, // KEY_M    50
  0x66, // KEY_COMMA    51
  0x76, // KEY_DOT    52
  0x86, // KEY_SLASH    53
  0x00, // KEY_RIGHTSHIFT    54
  CODE_UNUSED, // KEY_KPASTERISK    55
  CODE_UNUSED, // KEY_LEFTALT    56
  0x26, // KEY_SPACE    57
  0x04, // KEY_CAPSLOCK    58
  0x17, // KEY_F1    59
  0x27, // KEY_F2    60
  0x37, // KEY_F3    61
  0x41, // KEY_F4    62
  0x47, // KEY_F5    63
  0x57, // KEY_F6    64
  0x61, // KEY_F7    65
  0x67, // KEY_F8    66
  0x77, // KEY_F9    67
  0x02, // KEY_F10    68
  CODE_UNUSED, // KEY_NUMLOCK    69
  CODE_UNUSED, // KEY_SCROLLLOCK    70
  CODE_UNUSED, // KEY_KP7    71
  CODE_UNUSED, // KEY_KP8    72
  CODE_UNUSED, // KEY_KP9    73
  CODE_UNUSED, // KEY_KPMINUS    74
  CODE_UNUSED, // KEY_KP4    75
  CODE_UNUSED, // KEY_KP5    76
  CODE_UNUSED, // KEY_KP6    77
  CODE_UNUSED, // KEY_KPPLUS    78
  CODE_UNUSED, // KEY_KP1    79
  CODE_UNUSED, // KEY_KP2    80
  CODE_UNUSED, // KEY_KP3    81
  CODE_UNUSED, // KEY_KP0    82
  CODE_UNUSED, // KEY_KPDOT    83
  CODE_UNUSED, // KEY_UNUSED    84
  CODE_UNUSED, // KEY_ZENKAKUHANKAKU    85
  CODE_UNUSED, // KEY_102ND    86
  CODE_UNUSED, // KEY_F11    87
  CODE_UNUSED, // KEY_F12    88
  CODE_UNUSED, // KEY_RO    89
  CODE_UNUSED, // KEY_KATAKANA    90
  CODE_UNUSED, // KEY_HIRAGANA    91
  CODE_UNUSED, // KEY_HENKAN    92
  CODE_UNUSED, // KEY_KATAKANAHIRAGANA    93
  CODE_UNUSED, // KEY_MUHENKAN    94
  CODE_UNUSED, // KEY_KPJPCOMMA    95
  CODE_UNUSED, // KEY_KPENTER    96
  0x96, // KEY_RIGHTCTRL    97
  CODE_UNUSED, // KEY_KPSLASH    98
  CODE_UNUSED, // KEY_SYSRQ    99
  CODE_UNUSED, // KEY_RIGHTALT    100
  CODE_UNUSED, // KEY_LINEFEED    101
  CODE_UNUSED, // KEY_HOME    102
  0x93, // KEY_UP    103
  CODE_UNUSED, // KEY_PAGEUP    104
  0x91, // KEY_LEFT    105
  0x97, // KEY_RIGHT    106
  CODE_UNUSED, // KEY_END    107
  0x92, // KEY_DOWN    108
  CODE_UNUSED, // KEY_PAGEDOWN    109
  CODE_UNUSED, // KEY_INSERT    110
  0x95, // KEY_DELETE    111
  CODE_UNUSED, // KEY_MACRO    112
  CODE_UNUSED, // KEY_MUTE    113
  CODE_UNUSED, // KEY_VOLUMEDOWN    114
  CODE_UNUSED, // KEY_VOLUMEUP    115
  CODE_UNUSED, // KEY_POWER    116
  CODE_UNUSED, // KEY_KPEQUAL    117
  CODE_UNUSED, // KEY_KPPLUSMINUS    118
  CODE_UNUSED, // KEY_PAUSE    119
  CODE_UNUSED, // KEY_SCALE    120
  CODE_UNUSED, // KEY_KPCOMMA    121
  CODE_UNUSED, // KEY_HANGEUL    122
  CODE_UNUSED, // KEY_HANJA    123
  CODE_UNUSED, // KEY_YEN    124
  CODE_UNUSED, // KEY_LEFTMETA    125
  CODE_UNUSED, // KEY_RIGHTMETA    126
  CODE_UNUSED, // KEY_COMPOSE    127
};



#define BBC_SHIFT_OFF 0
#define BBC_SHIFT_ON 1
#define BBC_SHIFT_SAME_AS_BEFORE 2

#define KEY_2     3
#define KEY_6     7
#define KEY_7     8
#define KEY_8     9
#define KEY_9     10
#define KEY_0     11
#define KEY_MINUS 12
#define KEY_GRAVE   41
#define KEY_EQUAL   13
#define KEY_SEMICOLON   39
#define KEY_APOSTROPHE    40
#define KEY_SYSRQ   99
#define KEY_RIGHTALT    100

void get_bbc_code(uint8_t linux_code, uint8_t is_shift, uint8_t* bbc_col, uint8_t* bbc_row, uint8_t *bbc_shift)
{
  *bbc_col = 0;
  *bbc_row = 0;
  *bbc_shift = BBC_SHIFT_SAME_AS_BEFORE;

  if(linux_code >= LINUX_KEYCODE_TO_BBC_SIZE)
    return;
  *bbc_col = linux_keycode_to_bbc_matrix_lookup[linux_code] >> 4;
  *bbc_row = linux_keycode_to_bbc_matrix_lookup[linux_code] & 0xf;
  if(*bbc_col >= COL_SIZE)
    *bbc_col = 0;
  if(*bbc_row >= ROW_SIZE)
    *bbc_row = 0;

  if(linux_code == KEY_2 && is_shift)
  {
    *bbc_col = 7;
    *bbc_row = 4;
    *bbc_shift = BBC_SHIFT_OFF;
  }
  else if(linux_code == KEY_6 && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 1;
    *bbc_shift = BBC_SHIFT_OFF;
  }
  else if(linux_code == KEY_GRAVE && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 1;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_7 && is_shift)
  {
    *bbc_col = 4;
    *bbc_row = 3;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_8 && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 4;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_8 && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 4;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_9 && is_shift)
  {
    *bbc_col = 5;
    *bbc_row = 1;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_0 && is_shift)
  {
    *bbc_col = 6;
    *bbc_row = 2;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_MINUS && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 2;
    *bbc_shift = BBC_SHIFT_OFF;
  }
  else if(linux_code == KEY_EQUAL && is_shift)
  {
    *bbc_col = 7;
    *bbc_row = 5;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_EQUAL && (is_shift == 0))
  {
    *bbc_col = 7;
    *bbc_row = 1;
    *bbc_shift = BBC_SHIFT_ON;
  }
  else if(linux_code == KEY_SEMICOLON && is_shift)
  {
    *bbc_col = 8;
    *bbc_row = 4;
    *bbc_shift = BBC_SHIFT_OFF;
  }
  else if(linux_code == KEY_APOSTROPHE && is_shift)
  {
    *bbc_col = 1;
    *bbc_row = 3;
    *bbc_shift = BBC_SHIFT_ON;
  }
}

// falling edge, KB_EN is low
// this ISR has to be as fast as possible to beat the clock
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  CA2_LOW();
  while(1)
  {
    uint32_t kb_data = (GPIOB->IDR >> 8) & 0x7f;
    uint8_t kb_row = kb_data & 0x7;
    uint8_t kb_col = (kb_data >> 3) & 0xf;

    if(col_status[kb_col])
    {
      CA2_HI();
      if(matrix_status[kb_col][kb_row])
        W_HI();
      else
        W_LOW();
    }
    else
    {
      CA2_LOW();
      W_LOW();
    }
    if(IS_KB_EN_HI())
      break;
  }
  CA2_LOW();
  W_HI();
}

void col_status_update(uint8_t this_col)
{
  if(this_col >= COL_SIZE)
    return;
  uint8_t is_this_col_active = 0;
  for(uint8_t i = 0; i < ROW_SIZE; i++)
    if(matrix_status[this_col][i])
      is_this_col_active = 1;
  col_status[this_col] = is_this_col_active;
}

#define KEY_LEFTSHIFT   42
#define KEY_RIGHTSHIFT    54

uint8_t is_left_shift_on, is_right_shift_on, is_shift_on;

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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  kb_buf_init(&my_kb_buf);
  gamepad_buf_init(&my_gamepad_buf);
  delay_us_init(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  printf("%s\nv%d.%d.%d\n", boot_message, version_major, version_minor, version_patch);

  CA2_LOW();
  W_HI();

  memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);

  uint8_t this_col, this_row, this_shift;
  memset(matrix_status, 0, COL_SIZE * ROW_SIZE);
  memset(col_status, 0, COL_SIZE);
  /*
  the IC3 is a BCD decoder that takes 3 bits input and select one of the 10 lines
  the output ACTIVE LOW, meaning selected line is LOW while others are high

  the CA2 line becomes active when BBC micro manually scans the 
  
  once BBC micro receives a keyboard interrupt, it will start manually scanning the keyboard
  starting with column from 9 to 0, when the column with the pressed key is selected,
  the CA2 line will go low, indicating the active column.

  once column is known, bbc will then start scanning rows

  read_duration valid above 0x400

  */

  while (1)
  {
    uint32_t micros_now = micros();
    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      if(buffered_code == KEY_RIGHTALT)
      {
        if(buffered_value)
          HAL_GPIO_WritePin(KB_BREAK_GPIO_Port, KB_BREAK_Pin, GPIO_PIN_RESET);
        else
          HAL_GPIO_WritePin(KB_BREAK_GPIO_Port, KB_BREAK_Pin, GPIO_PIN_SET);
        kb_buf_pop(&my_kb_buf);
        continue;
      }
      if(buffered_code == KEY_LEFTSHIFT)
          is_left_shift_on = buffered_value;
      if(buffered_code == KEY_RIGHTSHIFT)
          is_right_shift_on = buffered_value;
      is_shift_on = is_right_shift_on | is_left_shift_on;
      get_bbc_code(buffered_code, is_shift_on, &this_col, &this_row, &this_shift);
      if(buffered_value)
      {
        if(this_shift == BBC_SHIFT_OFF)
        {
          matrix_status[0][0] = 0;
          col_status_update(0);
        }
        if(this_shift == BBC_SHIFT_ON)
        {
          matrix_status[0][0] = 1;
          col_status[0] = 1;
        }
        col_status[this_col] = 1;
        matrix_status[this_col][this_row] = 1;
        kb_buf_pop(&my_kb_buf);
      }
      else if(buffered_value == 0)
      {
        matrix_status[this_col][this_row] = 0;
        col_status_update(this_col);
        if(buffered_code == KEY_LEFTSHIFT || buffered_code == KEY_RIGHTSHIFT)
        {
          memset(matrix_status, 0, COL_SIZE * ROW_SIZE);
          memset(col_status, 0, COL_SIZE);
        }
        kb_buf_pop(&my_kb_buf);
        delay_us(10000);
      }
    }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    if(is_right_shift_on | is_left_shift_on)
      DEBUG_HI();
    else
      DEBUG_LOW();

    if(has_active_keys() && micros_now - last_ca2 > 20)
    {
      CA2_HI();
      delay_us(1);
      CA2_LOW();
      last_ca2 = micros_now;
    }
    // gamepad_update();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
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
  htim2.Init.Prescaler = 63;
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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, JS_PB0_Pin|JS_PB1_Pin|W_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, POT_RESET_Pin|KB_CA2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(KB_BREAK_GPIO_Port, KB_BREAK_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, DEBUG2_Pin|DEBUG_Pin|ERR_LED_Pin|ACT_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SLAVE_REQ_Pin */
  GPIO_InitStruct.Pin = SLAVE_REQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SLAVE_REQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : JS_PB0_Pin JS_PB1_Pin W_Pin */
  GPIO_InitStruct.Pin = JS_PB0_Pin|JS_PB1_Pin|W_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : POT_RESET_Pin */
  GPIO_InitStruct.Pin = POT_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(POT_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KB_BREAK_Pin KB_CA2_Pin */
  GPIO_InitStruct.Pin = KB_BREAK_Pin|KB_CA2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : KB_EN_Pin */
  GPIO_InitStruct.Pin = KB_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(KB_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KB_ROW_C_Pin KB_COL_A_Pin KB_COL_B_Pin KB_COL_C_Pin 
                           KB_COL_D_Pin KB_ROW_A_Pin KB_ROW_B_Pin */
  GPIO_InitStruct.Pin = KB_ROW_C_Pin|KB_COL_A_Pin|KB_COL_B_Pin|KB_COL_C_Pin 
                          |KB_COL_D_Pin|KB_ROW_A_Pin|KB_ROW_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DEBUG2_Pin DEBUG_Pin ERR_LED_Pin ACT_LED_Pin */
  GPIO_InitStruct.Pin = DEBUG2_Pin|DEBUG_Pin|ERR_LED_Pin|ACT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

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
