#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"

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

// ring buffer magic!
// I completely forgot how it works
uint8_t ps2kb_buf_add(ps2kb_buf *lb, uint8_t code, uint8_t value)
{
	if(ps2kb_buf_is_full(lb))
		return 1;
	lb->keycode_buf[lb->head] = code;
	lb->keyvalue_buf[lb->head] = value;
	lb->head = (lb->head + 1) % lb->size;
	return 0;
}

uint8_t ps2kb_buf_peek(ps2kb_buf *lb, uint8_t* code, uint8_t* value)
{
	if(ps2kb_buf_is_empty(lb))
		return 1;
	*code = lb->keycode_buf[lb->tail];
	*value = lb->keyvalue_buf[lb->tail];
	return 0;
}

void ps2kb_buf_pop(ps2kb_buf *lb)
{
	if(!ps2kb_buf_is_empty(lb))
		lb->tail = (lb->tail + 1) % lb->size;;
}

void ps2kb_buf_init(ps2kb_buf *lb, uint8_t size)
{
  lb->size = size;
  lb->keycode_buf = malloc(size);
  lb->keyvalue_buf = malloc(size);
  ps2kb_buf_reset(lb);
}

// ----------------------------

uint8_t ps2mouse_buf_is_full(ps2mouse_buf *lb)
{
	return lb->tail == (lb->head + 1) % lb->size;
}

uint8_t ps2mouse_buf_is_empty(ps2mouse_buf *lb)
{
	return lb->tail == lb->head;
}

uint8_t ps2mouse_buf_add(ps2mouse_buf *lb, mouse_event* event)
{
	if(ps2mouse_buf_is_full(lb))
		return 1;
	memcpy(&lb->mouse_events[lb->head], event, sizeof(mouse_event));
	lb->head = (lb->head + 1) % lb->size;
	return 0;
}

mouse_event* ps2mouse_buf_peek(ps2mouse_buf *lb)
{
	if(ps2mouse_buf_is_empty(lb))
		return NULL;
	return &lb->mouse_events[lb->tail];
}

void ps2mouse_buf_pop(ps2mouse_buf *lb)
{
	if(!ps2mouse_buf_is_empty(lb))
		lb->tail = (lb->tail + 1) % lb->size;
}

void ps2mouse_buf_reset(ps2mouse_buf *lb)
{
  lb->head = 0;
  lb->tail = 0;
  memset(lb->mouse_events, 0, lb->size * sizeof(mouse_event));
}

void ps2mouse_buf_init(ps2mouse_buf *lb, uint8_t size)
{
  lb->size = size;
  lb->mouse_events = malloc(size * sizeof(mouse_event));
  ps2mouse_buf_reset(lb);
}
