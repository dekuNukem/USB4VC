

  printf("%d %d %d\n", index, protocol_status_lookup[index], onoff);


printf("%d %d\n--\n", before & 0x80, after & 0x80);
  if(before == after)
    return;
  if((before & 0x7f) != (after & 0x7f))
    return;
  if((before & 0x80) > (after & 0x80)) // switching off
  {
    switch(after & 0x7f) 
    {
      case PROTOCOL_AT_PS2_KB:
        printf("PS2KB off\n");
        ps2kb_release_lines();
        ps2kb_reset();
        break;

      // case PROTOCOL_XT_KB:
      //  printf("XTKB off\n");
      //  break;

      case PROTOCOL_PS2_MOUSE:
        printf("PS2MOUSE off\n");
        ps2mouse_reset();
        ps2mouse_release_lines();
        break;

      // case PROTOCOL_MICROSOFT_SERIAL_MOUSE:
      //  printf("SERMOUSE off\n");
      //  break;

      // case PROTOCOL_GENERIC_GAMEPORT_GAMEPAD:
      //  printf("GGP off\n");
      //  release all buttons, reset digital pot to middle
      //  break;
    }
  }

uint8_t* find_in_array_7bit(uint8_t value, uint8_t* start, uint8_t* end)
{
  uint8_t* curr = start;
  while(curr <= end)
  {
    if(((*curr) & 0x7f) == (value & 0x7f))
      return curr;
    curr++;
  }
  return NULL;
}

#define SUPPORTED_PROTOCOL_SIZE 5
uint8_t protocol_status[SUPPORTED_PROTOCOL_SIZE] = 
{
  PROTOCOL_AT_PS2_KB | 0x80,
  PROTOCOL_XT_KB | 0x80,
  PROTOCOL_PS2_MOUSE | 0x80,
  PROTOCOL_MICROSOFT_SERIAL_MOUSE | 0x80,
  PROTOCOL_GENERIC_GAMEPORT_GAMEPAD | 0x80,
};
  // else // switching on
  // {
  //   switch(after & 0x7f) 
  //   {
  //     case PROTOCOL_AT_PS2_KB:
  //       printf("PS2KB on\n");
  //       break;

  //     case PROTOCOL_XT_KB:
  //       printf("XTKB on\n");
  //       break;

  //     case PROTOCOL_PS2_MOUSE:
  //       printf("PS2MOUSE on\n");
  //       break;

  //     case PROTOCOL_MICROSOFT_SERIAL_MOUSE:
  //       printf("SERMOUSE on\n");
  //       break;

  //     case PROTOCOL_GENERIC_GAMEPORT_GAMEPAD:
  //       printf("GGP on\n");
  //       break;
  //   }
  // }      
      // printf("mouse req: %d", ps2mouse_host_cmd);
uint8_t value = 32;
    uint8_t addr = ((value >> 8 & 0x01) | 0);
    value = (value & 0xFF);
    printf("w %d\n", HAL_I2C_Mem_Write(&hi2c2,MCP4451_I2C_ADDRESS,addr,1,&value,1,100));
    printf("%d\n--\n", after & 0x7f);

    printf("  f %d %d\n", ((*curr) & 0x7f), (value & 0x7f));
      printf("%d %d\n", i, backup_spi1_recv_buf[i]);
  printf("%d %d\n", before, after);
  printf("%d %d\n", before & 0x80, after & 0x80);

    // printf("0x%x\n", this_mouse_event->button_left);
    // printf("0x%x\n", this_mouse_event->button_right);
    // printf("0x%x\n", this_mouse_event->movement_x);
    // printf("0x%x\n", this_mouse_event->movement_y);
    // printf("out 0x%x\n", serial_mouse_output_buf[0]);
    // printf("out 0x%x\n", serial_mouse_output_buf[1]);
    // printf("out 0x%x\n", serial_mouse_output_buf[2]);
    // printf("-----\n");

  // printf("add before: %d %d\n", lb->tail, lb->head);
  // printf("add after: %d %d\n", lb->tail, lb->head);
      if(this_mouse_event->button == BTN_LEFT)
        mouse_status[MOUSE_LEFT] = this_mouse_event->button_state;
      else if(this_mouse_event->button == BTN_RIGHT)
        mouse_status[MOUSE_RIGHT] = this_mouse_event->button_state;
      else if(this_mouse_event->button == BTN_MIDDLE)
        mouse_status[MOUSE_MIDDLE] = this_mouse_event->button_state;
      for (int i = 0; i < MOUSE_BUTTONS_SIZE; ++i)
        printf("%d ", mouse_status[i]);
      printf("\n");
// printf("%d %d %d %d %d\n", this_event->movement_x, this_event->movement_y, this_event->scroll_vertical);
  // printf("%d %d %d %d %d\n-------\n\n", this_event->button_left, this_event->button_middle, this_event->button_right, this_event->button_side, this_event->button_extra);
  // printf("%d %d\n",this_event->movement_y, ps2mouse_out_buf[2]);
  // return 0;
      // for (int i = 0; i < SAMPLE_RATE_HISTORY_BUF_SIZE; ++i)
      //   printf("%d ", sample_rate_history[i]);
      // printf("\n");
        sample_rate_history_index = (sample_rate_history_index + 1) % SAMPLE_RATE_HISTORY_BUF_SIZE;
uint8_t get_prev(uint8_t index, uint8_t size)
{
  return (index + size - 1) % size;
}

      last = get_prev(sample_rate_history_index, SAMPLE_RATE_HISTORY_BUF_SIZE);
      second_last = get_prev(last, SAMPLE_RATE_HISTORY_BUF_SIZE);
      third_last = get_prev(second_last, SAMPLE_RATE_HISTORY_BUF_SIZE);
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
