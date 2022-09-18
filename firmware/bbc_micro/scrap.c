/*
2260
bbc_ref

1458
stm_ref

first number is BBC micro reference voltage, supposed to be 1.8V but can change from machine to machine
second number is vrefint 1.23V for STM32F072

1230 / stm_ref = mV per bit

0.8436 mV per bit

0.8436 * 2259 = 1905mV reference

therefore DAC midpoint is should output 

----------

mid point = bbc_ref / 2 = 1130





*/


// void gamepad_update(void)
// {
//   gamepad_event* this_gamepad_event = gamepad_buf_peek(&my_gamepad_buf);
//   if(this_gamepad_event != NULL)
//   {
//     printf("%d %d %d %d %d %d %d %d\n---\n", this_gamepad_event->button_1, this_gamepad_event->button_2, this_gamepad_event->button_3, this_gamepad_event->button_4, this_gamepad_event->axis_x, this_gamepad_event->axis_y, this_gamepad_event->axis_rx, this_gamepad_event->axis_ry);
    
//     Joystick 1 = CH0 and CH1
//     Joystick 2 = CH2 and CH3
//     Wiper 0 = CH3 Joystick 2
//     Wiper 1 = CH0 Joystick 1
//     Wiper 2 = CH2 Joystick 2
//     Wiper 3 = CH1 Joystick 1
    
//     HAL_GPIO_WritePin(JS_PB0_GPIO_Port, JS_PB0_Pin, !(this_gamepad_event->button_1));
//     HAL_GPIO_WritePin(JS_PB1_GPIO_Port, JS_PB1_Pin, !(this_gamepad_event->button_2));
//     mcp4451_write_wiper(1, 255-this_gamepad_event->axis_x);
//     mcp4451_write_wiper(3, 255-this_gamepad_event->axis_y);
//     mcp4451_write_wiper(0, 255-this_gamepad_event->axis_rx);
//     mcp4451_write_wiper(2, 255-this_gamepad_event->axis_ry);
//     gamepad_buf_pop(&my_gamepad_buf);
//   }
// }
  while(1)
  {
    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, 100);
    uint32_t result1 = HAL_ADC_GetValue(&hadc);
    HAL_ADC_PollForConversion(&hadc, 100);
    uint32_t result2 = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);
    printf("%d %d\n", result1, result2);
    HAL_Delay(100);
  }
  // 2234 is 1.8V
  // 1117 is 0.9V
  uint8_t test = 0;
  while(1)
  {
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_lookup[test]);
    HAL_Delay(10);
    test++;
  }
        if(this_shift == BBC_SHIFT_OFF && (buffered_code == KEY_LEFTSHIFT || buffered_code == KEY_RIGHTSHIFT))
        {
          col_status[0] = 1;
          matrix_status[0][0] = 1;
        }
 // maybe dont pop it here, wait until conformation that W has been active for more than 10ms?

    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      get_bbc_code(buffered_code, &this_col, &this_row);
      if(buffered_value && read_duration > 0x400)
        kb_buf_pop(&my_kb_buf); // maybe dont pop it here, wait until conformation that W has been active for more than 10ms?
    }

  while(1)
  {
    DEBUG_HI();
    delay_us(10);
    DEBUG_LOW();
    delay_us(10);
  }

printf("%x:%x\n", kb_col, kb_row);
    continue;

if(kb_data & 0x38)
    {
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
      // printf("!");
    }
    else
    {
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
    }

if(kb_row == 0 && kb_col == 7)

    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      if(buffered_value)
      {
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
        delay_us(1);
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
      }
      kb_buf_pop(&my_kb_buf);
    }


  while (1)
  {
    continue;
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      if(buffered_value)
      {
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
        delay_us(1);
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
      }
      kb_buf_pop(&my_kb_buf);
    }

    if(HAL_GPIO_ReadPin(KB_EN_GPIO_Port, KB_EN_Pin) == GPIO_PIN_SET)
    {
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
      continue;
    }

    // bit 0-2: ROW A-C
    // bit 3-6: COL A-D
    kb_data = (GPIOB->IDR >> 8) & 0x7f;
    kb_row = kb_data & 0x7;
    kb_col = (kb_data >> 3) & 0xf;

    if(kb_col == 4)
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);

    if(kb_col == 4 && kb_row == 2)
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
  }



    for (int i = 0; i < ACTIVE_KEYS_BUF_SIZE; ++i)
    {
      if(active_keys[i])
        printf("%d,", i);
    }
    continue;

-----------


    if(has_active_keys())

    if(kb_en_active)
    {
      // bit 0-2: ROW A-C
      // bit 3-6: COL A-D
      /*
        7   6   5   4   3   2   1   0
            CD  CC  CB  CA  RC  RB  RA
      */
      kb_data = (GPIOB->IDR >> 8) & 0x7f;
      kb_row = kb_data & 0x7;
      kb_col = (kb_data >> 3) & 0xf;

      if(kb_col == 4)
      {
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
        if(kb_row == 2)
          HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
        else
          HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
      }
      else
      {
        idle_kb_line();
      }
    }
    // KB_EN inactive
    else
    {
      if(has_active_keys())
      {
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
        delay_us(1);
      }
      idle_kb_line();
    }
  }

  -=----------------


    if(kb_en_active == 1 && has_active_keys())
    {
      // bit 0-2: ROW A-C
      // bit 3-6: COL A-D
      /*
        7   6   5   4   3   2   1   0
            CD  CC  CB  CA  RC  RB  RA
      */
      kb_data = (GPIOB->IDR >> 8) & 0x7f;
      kb_row = kb_data & 0x7;
      kb_col = (kb_data >> 3) & 0xf;

      if(kb_col == 4)
      {
        HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
        if(kb_row == 2)
          HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
        else
          HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
      }
      else
      {
        idle_kb_line();
      }
    }
    else if((kb_en_active == 1) && (has_active_keys() == 0))
    {
      idle_kb_line();
    }
    else if((kb_en_active == 0) && (has_active_keys() == 0))
    {
      idle_kb_line();
    }
    else if((kb_en_active == 0) && has_active_keys())
    {
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
      delay_us(1);
      idle_kb_line();
    }


--------------


void handle_kb_request(void)
{
  if(!kb_en_active)
    return;
  while(HAL_GPIO_ReadPin(KB_EN_GPIO_Port, KB_EN_Pin) == GPIO_PIN_RESET)
    handle_kb_en();
  kb_en_active = 0;
}

HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, has_active_keys());
  if(!has_active_keys())
  {
    idle_kb_line();
    return;
  }
  HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, has_active_keys);
------------


  while (1)
  {
    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      if(buffered_value)
        active_keys[buffered_code] = 1;
      else
        active_keys[buffered_code] = 0;
      kb_buf_pop(&my_kb_buf);
    }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

    if(check_active_keys())
    {
      has_active_keys = 1;
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
      delay_us(1);
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
    }
    else
    {
      has_active_keys = 0;
      HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
    }
  }

------------


  if(kb_col == 1)
  {
    HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
    if(kb_row == 3)
    {
      delay_us(10);
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
    }
    else if(kb_row == 4)
    {
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
    }
    // HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, kb_row == 4);
  }

-----------


void handle_kb_en(void)
{
  // if(HAL_GetTick() - event_start > 20)
  // {
  //   DEBUG_LOW();
  //   return;
  // }

  // bit 0-2: ROW A-C
  // bit 3-6: COL A-D
  /*
    7   6   5   4   3   2   1   0
        CD  CC  CB  CA  RC  RB  RA
  */

  kb_data = (GPIOB->IDR >> 8) & 0x7f;
  kb_row = kb_data & 0x7;
  kb_col = (kb_data >> 3) & 0xf;

  if(kb_col == 1)
  {
    HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, kb_row == 4);
  }
  else
  {
    HAL_GPIO_WritePin(KB_CA2_GPIO_Port, KB_CA2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, GPIO_PIN_RESET);
  }
}

-------

      // HAL_GPIO_WritePin(W_GPIO_Port, W_Pin, kb_row == 4);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  while(1)
  {
    kb_data = (GPIOB->IDR >> 8) & 0x7f;
    kb_row = kb_data & 0x7;
    kb_col = (kb_data >> 3) & 0xf;

    if(kb_col == 1)
    {
      CA2_SET();
      if(kb_row == 4)
        W_SET();
      else
        W_RESET();
    }
    else
    {
      CA2_RESET();
      W_RESET();
    }
  }
}

-----------

// falling edge, KB_EN is low
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  while(1)
  {
    // kb_data = (GPIOB->IDR >> 8) & 0x7f;
    // kb_row = kb_data & 0x7;
    // kb_col = (kb_data >> 3) & 0xf;

    uint32_t kb_data = GPIOB->IDR & 0x7f00;
    printf("%x\n", kb_data);
    if(kb_data & 0x7800 == 0x800) // kb_col == 1
    {
      CA2_SET();
      if(kb_data & 0x700 == 0x400) // kb_row == 4
        W_SET();
      else
        W_RESET();
    }
    else
    {
      CA2_RESET();
      W_RESET();
    }
  }
}

#define ACTIVE_KEYS_BUF_SIZE 256
volatile uint8_t active_keys[ACTIVE_KEYS_BUF_SIZE];


uint8_t check_active_keys(void)
{
  for(int i = 0; i < ACTIVE_KEYS_BUF_SIZE; ++i)
    if(active_keys[i])
      return 1;
  return 0;
}

when T and H are pressed together:

T: col 3 row 2
H: col 4 row 5

7: col 4 row 2

col_status
0: 0
1: 0
2: 0
3: 1
4: 1
5: 0
6: 0
7: 0
8: 0
9: 0
10: 0
11: 0
12: 0
13: 0
14: 0
15: 0

row_status:
0: 0
1: 0
2: 1
3: 0
4: 0
5: 1
6: 0
7: 0

use a multidimensional array? faster lookup


each active key needs to be acklowdged by bbc micro before being cleared from
releasing

////
uint8_t has_unprocessed_keys(void)
{
  for (uint8_t this_col = 0; this_col < COL_SIZE; ++this_col)
    for (uint8_t this_row = 0; this_row < ROW_SIZE; ++this_row)
      if(matrix_status[this_col][this_row] == 1)
        return 1;
  return 0;
}
else if(buffered_value == 2 || has_unprocessed_keys() == 0)
      {
        kb_buf_pop(&my_kb_buf);
      }


if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      uint32_t ms_now = HAL_GetTick();
      get_bbc_code(buffered_code, &this_col, &this_row);
      if(buffered_value)
      {
        col_status[this_col] = 1;
        matrix_status[this_col][this_row] = 1;
        DEBUG_HI();
        if(ms_now - last_keypress_pop > 40)
        {
          DEBUG_LOW();
          kb_buf_pop(&my_kb_buf);
          last_keypress_pop = ms_now;
        }
      }
      else
      {
        col_status[this_col] = 0;
        matrix_status[this_col][this_row] = 0;
        kb_buf_pop(&my_kb_buf);
      }
    }

if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      uint32_t ms_now = HAL_GetTick();
      get_bbc_code(buffered_code, &this_col, &this_row);
      if(buffered_value)
      {
        col_status[this_col] = 1;
        matrix_status[this_col][this_row] = 1;
        DEBUG_HI();
        HAL_Delay(20);
        DEBUG_LOW();
        kb_buf_pop(&my_kb_buf);
      }
      else
      {
        col_status[this_col] = 0;
        matrix_status[this_col][this_row] = 0;
        kb_buf_pop(&my_kb_buf);
      }
    }



    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      get_bbc_code(buffered_code, &this_col, &this_row);
      if(buffered_value)
      {
        col_status[this_col] = 1;
        matrix_status[this_col][this_row] = 1;
        DEBUG_HI();
        CA2_HI();
        delay_us(1);
        CA2_LOW();
        last_ca2 = micros_now;
        HAL_Delay(20);
        DEBUG_LOW();
        kb_buf_pop(&my_kb_buf);
      }
      else
      {
        col_status[this_col] = 0;
        matrix_status[this_col][this_row] = 0;
        // DEBUG_HI();
        // HAL_Delay(20);
        // DEBUG_LOW();
        kb_buf_pop(&my_kb_buf);
      }
    }
////

    if(kb_buf_peek(&my_kb_buf, &buffered_code, &buffered_value) == 0)
    {
      get_bbc_code(buffered_code, &this_col, &this_row);
      if(buffered_value)
      {
        col_status[this_col] = 1;
        matrix_status[this_col][this_row] = 1;
        kb_buf_pop(&my_kb_buf);
        DEBUG_HI();
        delay_us(1);
        DEBUG_LOW();
      }
      else
      {
        col_status[this_col] = 0;
        matrix_status[this_col][this_row] = 0;
        kb_buf_pop(&my_kb_buf);
        DEBUG_HI();
        delay_us(5);
        DEBUG_LOW();
      }
    }