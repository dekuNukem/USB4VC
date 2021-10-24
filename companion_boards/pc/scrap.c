printf("waiting for host request...\n");
  // wait for data line to go low and clock line to go high (request to send)
  while(!(PS2KB_READ_DATA_PIN() == GPIO_PIN_RESET && PS2KB_READ_CLK_PIN() == GPIO_PIN_SET))
    ;
  printf("host request!\n");
  // return 0;  


  // if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
  //   HAL_SPI_Receive_DMA(&hspi1, spi_recv_buf, SPI_BUF_SIZE);
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
      printf("hh\n");
    // printf("%d\n", HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4));
    // HAL_Delay(200);

  // printf("got!\n");
  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);

#define PS2_BUS_IDLE 0
#define PS2_BUS_INHIBIT 1
#define PS2_BUS_REQ_TO_SEND 2
#define PS2_BUS_UNKNOWN 3

uint8_t get_bus_status()
{

}


// HAS PARAITY CHECK


uint8_t ps2kb_read(uint8_t* result)
{
  uint16_t data = 0x00;
  uint16_t bit = 0x01;

  uint8_t calculated_parity = 1;
  uint8_t received_parity = 0;

  printf("waiting for host request...\n");
  // wait for data line to go low and clock line to go high (request to send)
  while(!(PS2KB_READ_DATA_PIN() == GPIO_PIN_RESET && PS2KB_READ_CLK_PIN() == GPIO_PIN_SET))
    ;
  printf("host request!\n");
  // return 0;

  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  while (bit < 0x0100)
  {
    if (PS2KB_READ_DATA_PIN() == GPIO_PIN_SET)
    {
      data = data | bit;
      calculated_parity = calculated_parity ^ 1;
    } 
    else
    {
      calculated_parity = calculated_parity ^ 0;
    }
    bit = bit << 1;
    delay_us(CLKHALF);
    PS2KB_CLK_LOW();
    delay_us(CLKFULL);
    PS2KB_CLK_HI();
    delay_us(CLKHALF);
  }
  // we do the delay at the end of the loop, so at this point we have
  // already done the delay for the parity bit

  // parity bit
  if (PS2KB_READ_DATA_PIN() == GPIO_PIN_SET)
    received_parity = 1;

  // stop bit
  delay_us(CLKHALF);
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);

  delay_us(CLKHALF);
  PS2KB_DATA_LOW();
  PS2KB_CLK_LOW();
  delay_us(CLKFULL);
  PS2KB_CLK_HI();
  delay_us(CLKHALF);
  PS2KB_DATA_HI();

  *result = data & 0x00FF;
  printf("0x%02x\n", *result);
  return 0;

}
