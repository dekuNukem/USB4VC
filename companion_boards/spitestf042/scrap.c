void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
 //  HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, GPIO_PIN_SET);
  HAL_SPI_TransmitReceive_IT(&hspi1, spi_transmit_buf, spi_recv_buf, SPI_BUF_SIZE);
  // HAL_GPIO_WritePin(DEBUG_GPIO_Port, DEBUG_Pin, GPIO_PIN_RESET);

  if(spi_recv_buf[0] != 0xde)
    HAL_GPIO_WritePin(ERROR_GPIO_Port, ERROR_Pin, GPIO_PIN_SET);
  else
    HAL_GPIO_WritePin(ERROR_GPIO_Port, ERROR_Pin, GPIO_PIN_RESET);
}


    if(spi_recv_buf[SPI_BUF_INDEX_MAGIC] != SPI_MAGIC_NUM)
    {
      printf("unknown command!\n");
      goto parse_end;
    }
// event_type = backup_spi1_recv_buf[4];
    // event_code = backup_spi1_recv_buf[6];
    // event_value = backup_spi1_recv_buf[8];
    // for (int i = 0; i < SPI_BUF_SIZE; ++i)
    //     printf("0x%02x ", backup_spi1_recv_buf[i]);
    // printf("\n\n");

  // if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
  //   HAL_SPI_Receive_DMA(&hspi1, spi_recv_buf, SPI_BUF_SIZE);
      // printf("ps2kb_leds: %d\n", ps2kb_leds);
    // printf("0x%02x\n", ps2kb_host_cmd);

memset(spi_transmit_buf, 0, SPI_BUF_SIZE);
      HAL_GPIO_WritePin(SLAVE_REQ_GPIO_Port, SLAVE_REQ_Pin, GPIO_PIN_RESET);
printf("\n---ps2kb_buf_get---\n%d %d\n", my_ps2kb_buf.tail, my_ps2kb_buf.head);
    printf("%d %d---\n", buffered_code, buffered_value);

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  for (int i = 0; i < SPI_BUF_SIZE; ++i)
      printf("0x%02x ", spi_recv_buf[i]);
  printf("\n");

  if(spi_recv_buf[SPI_BUF_INDEX_MAGIC] != SPI_MAGIC_NUM)
    goto parse_end;

  if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MSG_KB_EVENT)
  {
    // event_type = spi_recv_buf[4];
    // event_code = spi_recv_buf[6];
    // event_value = spi_recv_buf[8];
    ps2kb_buf_add(&my_ps2kb_buf, spi_recv_buf[6], spi_recv_buf[8]);
  }

  spi_data_available = 1;

  parse_end:
  HAL_SPI_Receive_DMA(&hspi1, spi_recv_buf, SPI_BUF_SIZE);
}
if(spi_recv_buf[SPI_BUF_INDEX_MSG_TYPE] == SPI_MSG_KB_EVENT)
  {
    event_type = spi_recv_buf[4];
    event_code = spi_recv_buf[6];
    event_value = spi_recv_buf[8];
    // printf("%02d %d\n", event_code, event_value);
    ps2kb_press_key(event_code, event_value);
  }


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
