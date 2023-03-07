#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"

void kb_buf_reset(kb_buf *lb)
{
  lb->head = 0;
  lb->tail = 0;
  memset(lb->keycode_buf, 0, KEYBOARD_EVENT_BUFFER_SIZE);
  memset(lb->keyvalue_buf, 0, KEYBOARD_EVENT_BUFFER_SIZE);
}

uint8_t kb_buf_is_full(kb_buf *lb)
{
	return lb->tail == (lb->head + 1) % KEYBOARD_EVENT_BUFFER_SIZE;
}

uint8_t kb_buf_is_empty(kb_buf *lb)
{
	return lb->tail == lb->head;
}

uint8_t kb_buf_add(kb_buf *lb, uint8_t code, uint8_t value)
{
	if(kb_buf_is_full(lb))
		return 1;
	lb->keycode_buf[lb->head] = code;
	lb->keyvalue_buf[lb->head] = value;
	lb->head = (lb->head + 1) % KEYBOARD_EVENT_BUFFER_SIZE;
	return 0;
}

uint8_t kb_buf_peek(kb_buf *lb, uint8_t* code, uint8_t* value)
{
	if(kb_buf_is_empty(lb))
		return 1;
	*code = lb->keycode_buf[lb->tail];
	*value = lb->keyvalue_buf[lb->tail];
	return 0;
}

void kb_buf_pop(kb_buf *lb)
{
	if(!kb_buf_is_empty(lb))
		lb->tail = (lb->tail + 1) % KEYBOARD_EVENT_BUFFER_SIZE;
}

void kb_buf_init(kb_buf *lb)
{
  lb->keycode_buf = malloc(KEYBOARD_EVENT_BUFFER_SIZE);
  lb->keyvalue_buf = malloc(KEYBOARD_EVENT_BUFFER_SIZE);
  kb_buf_reset(lb);
}

// ----------------------------

uint8_t mouse_buf_is_full(mouse_buf *lb)
{
	return lb->tail == (lb->head + 1) % MOUSE_EVENT_BUFFER_SIZE;
}

uint8_t mouse_buf_is_empty(mouse_buf *lb)
{
	return lb->tail == lb->head;
}

uint8_t mouse_buf_add(mouse_buf *lb, mouse_event* event)
{
	if(mouse_buf_is_full(lb))
		return 1;
	memcpy(&lb->mouse_events[lb->head], event, sizeof(mouse_event));
	lb->head = (lb->head + 1) % MOUSE_EVENT_BUFFER_SIZE;
	return 0;
}

mouse_event* mouse_buf_peek(mouse_buf *lb)
{
	if(mouse_buf_is_empty(lb))
		return NULL;
	return &lb->mouse_events[lb->tail];
}

void mouse_buf_pop(mouse_buf *lb)
{
	if(!mouse_buf_is_empty(lb))
		lb->tail = (lb->tail + 1) % MOUSE_EVENT_BUFFER_SIZE;
}

void mouse_buf_reset(mouse_buf *lb)
{
  lb->head = 0;
  lb->tail = 0;
  memset(lb->mouse_events, 0, MOUSE_EVENT_BUFFER_SIZE * sizeof(mouse_event));
}

void mouse_buf_init(mouse_buf *lb)
{
  lb->mouse_events = malloc(MOUSE_EVENT_BUFFER_SIZE * sizeof(mouse_event));
  mouse_buf_reset(lb);
}

// -------------

mouse_event consolidated_mouse_event;

void cap_to_127(int32_t *number)
{
  if(*number > 127)
    *number = 127;
  if(*number < -127)
    *number = -127;
}

uint8_t get_consolidated_mouse_event(mouse_buf* mbuf, mouse_event* cme_result)
{
	uint8_t this_count = 0;
  cme_result->movement_x = 0;
  cme_result->movement_y = 0;
  cme_result->button_left = 0;
  cme_result->button_right = 0;
  while(1)
  {
    mouse_event* this_mouse_event = mouse_buf_peek(mbuf);
    if(this_mouse_event == NULL)
      break;
    cme_result->movement_x += this_mouse_event->movement_x;
    cme_result->movement_y += this_mouse_event->movement_y;
    cme_result->button_left |= this_mouse_event->button_left;
    cme_result->button_right |= this_mouse_event->button_right;
    mouse_buf_pop(mbuf);
    this_count++;
  }
  cap_to_127(&cme_result->movement_x);
  cap_to_127(&cme_result->movement_y);
  return this_count;
}

