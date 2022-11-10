HAL_GPIO_TogglePin(KBDATA_GPIO_Port, KBDATA_Pin);
    HAL_GPIO_TogglePin(KBRDY_GPIO_Port, KBRDY_Pin);
    HAL_Delay(10);

// uint8_t write_stop_bit()
// {
//   HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_RESET);
//   HAL_GPIO_WritePin(KBRDY_GPIO_Port, KBRDY_Pin, GPIO_PIN_RESET);
//   if(wait_for_KBACK(GPIO_PIN_RESET) != KB_WRITE_SUCCESS)
//     return KB_WRITE_TIMEOUT;
//   HAL_GPIO_WritePin(KBDATA_GPIO_Port, KBDATA_Pin, GPIO_PIN_SET);
//   HAL_GPIO_WritePin(KBRDY_GPIO_Port, KBRDY_Pin, GPIO_PIN_SET);
//   HAL_Delay(1);
//   if(wait_for_KBACK(GPIO_PIN_SET) != KB_WRITE_SUCCESS)
//     return KB_WRITE_TIMEOUT;
//   return KB_WRITE_SUCCESS;
// }
uint8_t send_key(uint8_t data, uint8_t key_status)
{
  for (int i = 0; i < 7; ++i)
  {
    if(write_bit(get_bit(data, i)) != KB_WRITE_SUCCESS)
      return KB_WRITE_TIMEOUT;
  }
  if(write_bit(key_status) != KB_WRITE_SUCCESS)
    return KB_WRITE_TIMEOUT
  if(write_bit(0) != KB_WRITE_SUCCESS) // stop bit
    return KB_WRITE_TIMEOUT
  return KB_WRITE_SUCCESS;
}