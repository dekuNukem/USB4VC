#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"

uint8_t spi_recv_buf[SPI_BUF_SIZE];
ps2kb_buf my_ps2kb_buf;

void ps2kb_buf_reset(ps2kb_buf *lb)
{
  lb->head = 0;
  lb->tail = 0;
  memset(lb->keycode_buf, 0, lb->size);
  memset(lb->keyvalue_buf, 0, lb->size);
}

uint8_t ps2kb_buf_is_full(ps2kb_buf *lb)
{
	return lb->tail == (lb->head + 1) % lb->size;
}

uint8_t ps2kb_buf_is_empty(ps2kb_buf *lb)
{
	return lb->tail == lb->head;
}

uint8_t ps2kb_buf_add(ps2kb_buf *lb, uint8_t code, uint8_t value)
{
	// printf("add before: %d %d\n", lb->tail, lb->head);
	if(ps2kb_buf_is_full(lb))
		return 1;
	lb->keycode_buf[lb->head] = code;
	lb->keyvalue_buf[lb->head] = value;
	lb->head = (lb->head + 1) % lb->size;
	// printf("add after: %d %d\n", lb->tail, lb->head);
	return 0;
}

uint8_t ps2kb_buf_get(ps2kb_buf *lb, uint8_t* code, uint8_t* value)
{
	if(ps2kb_buf_is_empty(lb))
		return 1;
	// printf("get before: %d %d\n", lb->tail, lb->head);
	*code = lb->keycode_buf[lb->tail];
	*value = lb->keyvalue_buf[lb->tail];
	lb->tail = (lb->tail + 1) % lb->size;
	// printf("get after: %d %d\n---\n", lb->tail, lb->head);
	return 0;
}

void ps2kb_buf_init(ps2kb_buf *lb, uint8_t size)
{
  lb->size = size;
  lb->keycode_buf = malloc(size);
  lb->keyvalue_buf = malloc(size);
  ps2kb_buf_reset(lb);
}

