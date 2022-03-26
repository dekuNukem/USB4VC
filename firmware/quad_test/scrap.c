  // printf("%d %d\n", avg_buf_index, value);
y=−306.4516129032258x+20306.451612903224

    if(avg_speed > 64)
      avg_speed = 64
    elif(avg_speed < -64)
      avg_speed = -64
  for (int i = 0; i < AVG_BUF_SIZE; ++i)
    printf("%d ", avg_buf[i]);
  printf("\n");
  printf("%d %d %d\n", (uint8_t)qo->current_index, (uint8_t)(qo->current_index - 1), (uint8_t)(qo->current_index - 1)%4);
void quad_write(quad_output *qo)
{
  uint8_t current_code = grey_code_lookup[qo->current_index];
  switch(current_code)
  {
      case 0:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_RESET);
        break;
      case 1:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_SET);
        break;
      case 2:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_RESET);
        break;
      case 3:
        HAL_GPIO_WritePin(qo->A_port, qo->A_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(qo->B_port, qo->B_pin, GPIO_PIN_SET);
        break;
      default:
        break;
  }
}