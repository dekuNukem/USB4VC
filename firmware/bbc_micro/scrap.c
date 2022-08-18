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