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

byte PS2_Victor_ger[] = {
   /* F9 */ 0x5e,
   /* F5 */ 0x5,
   /* F3 */ 0x3,
   /* F1 */ 0x1,
   /* F2 */ 0x2,
   /* F12 */ 0x20,
   /* F10 */ 0x49,
   /* F8 */ 0x8,
   /* F6 */ 0x6,
   /* F4 */ 0x4,
   /* ^ */ 0xc,
   /* L ALT */ 0x5f,
   /* L SHFT */ 0x4a,
   /* L CTRL */ 0x61,
   /* Z */ 0x4c,
   /* S */ 0x38,
   /* A */ 0x37,
   /* W */ 0x23,
   /* C */ 0x4e,
   /* X */ 0x4d,
   /* D */ 0x39,
   /* E */ 0x24,
   /* SPACE */ 0x60,
   /* V */ 0x4f,
   /* F */ 0x3a,
   /* T */ 0x26,
   /* R */ 0x25,
   /* N */ 0x51,
   /* B */ 0x50,
   /* H */ 0x3c,
   /* G */ 0x3b,
   /* Y */ 0x27,
   /* M */ 0x52,
   /* J */ 0x3d,
   /* U */ 0x28,
   /* , */ 0x53,
   /* K */ 0x3e,
   /* I */ 0x29,
   /* O */ 0x2a,
   /* . */ 0x54,
   /* L */ 0x3f,
   /* Ö */ 0x40,
   /* P */ 0x2b,
   /* ß */ 0x17,
   /* Ä */ 0x41,
   /* Ü */ 0x2c,
  /* 58 CAPS */ 0x36,
   /* R SHFT */ 0x56,
   /* ENTER */ 0x57,
   /* # */ 0x18,
   /* KP1 */ 0x5a,
   /* KP4 */ 0x45,
   /* KP7 */ 0x31,
  /* 70 KP0 */ 0x64,
   /* KP. */ 0x66,
   /* KP2 */ 0x5b,
   /* KP5 */ 0x46,
   /* KP6 */ 0x47,
   /* KP8 */ 0x32,
   /* NUM */ 0x1c,
  /* 78 F11 */ 0x35,
   /* KP+ */ 0x48,
   /* KP3 */ 0x5c,
   /* KP- */ 0x34,
   /* KP* */ 0x1f,
   /* KP9 */ 0x33,
   /* SCROLL */ 0x43,
   /* F7 */ 0x7,
};