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