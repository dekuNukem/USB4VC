  if(micros() - atten_start < 700)
    printf("%d\n", micros() - atten_start);

void adb_recv_cmd(void)
{
  HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_SET);
  uint8_t result = look_for_atten(1);
  HAL_GPIO_WritePin(DEBUG0_GPIO_Port, DEBUG0_Pin, GPIO_PIN_RESET);
  // printf("%d\n", result);

  uint8_t atten_result = look_for_atten(1);

  if(atten_result != )
}

  // if ADB data line is high
  while(ADB_READ_DATA_PIN() == GPIO_PIN_SET)
  {
    if(blocking == 0)
      return ADB_LINE_STATUS_IDLE;
  }
  printf("%d %d\n", lo_time, hi_time);
