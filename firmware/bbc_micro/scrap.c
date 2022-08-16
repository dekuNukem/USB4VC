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