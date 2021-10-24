#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"

uint8_t spi_recv_buf[SPI_BUF_SIZE];

ps2kb_buf my_ps2kb_buf;

void ps2kb_buf_reset(ps2kb_buf *lb)
{
  lb->curr_index = -1;
  memset(lb->keycode_buf, 0, lb->buf_size);
  memset(lb->keyvalue_buf, 0, lb->buf_size);
}

void ps2kb_buf_init(ps2kb_buf *lb, uint8_t size)
{
  lb->buf_size = size;
  lb->keycode_buf = malloc(size);
  lb->keyvalue_buf = malloc(size);
  ps2kb_buf_reset(lb);
}