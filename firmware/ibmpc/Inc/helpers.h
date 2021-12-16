#ifndef __HELPERS_H
#define __HELPERS_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define SPI_BUF_SIZE 32

#define SPI_MOSI_MAGIC 0xde
#define SPI_MISO_MAGIC 0xcd

#define SPI_BUF_INDEX_MAGIC 0
#define SPI_BUF_INDEX_SEQNUM 1
#define SPI_BUF_INDEX_MSG_TYPE 2

#define SPI_MOSI_MSG_TYPE_NOP 0
#define SPI_MOSI_MSG_TYPE_INFO_REQUEST 1
#define SPI_MOSI_MSG_TYPE_PROTOCOL_CONTROL 2
#define SPI_MOSI_MSG_TYPE_REQ_ACK 3

#define SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT 8
#define SPI_MOSI_MSG_TYPE_MOUSE_EVENT 9
#define SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW 10
#define SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED 11

#define SPI_MISO_MSG_TYPE_NOP 0
#define SPI_MISO_MSG_TYPE_INFO_REQUEST 128
#define SPI_MISO_MSG_TYPE_KB_LED_REQUEST 129

#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE    0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

typedef struct
{
  uint8_t head;
  uint8_t tail;
  uint8_t size;
  uint8_t* keycode_buf;
  uint8_t* keyvalue_buf;
} ps2kb_buf;

typedef struct
{
  int16_t movement_x;
  int16_t movement_y;
  int16_t scroll_vertical;
  uint8_t button_left;
  uint8_t button_middle;
  uint8_t button_right;
  uint8_t button_side;
  uint8_t button_extra;
} mouse_event;

typedef struct
{
  uint8_t head;
  uint8_t tail;
  uint8_t size;
  mouse_event* mouse_events;
} ps2mouse_buf;

#define PS2_OUT_BUF_MAXSIZE 8

typedef struct
{
  uint8_t size;
  uint8_t data[PS2_OUT_BUF_MAXSIZE];
} ps2_outgoing_buf;

void ps2kb_buf_init(ps2kb_buf *lb, uint8_t size);
uint8_t ps2kb_buf_add(ps2kb_buf *lb, uint8_t code, uint8_t value);
uint8_t ps2kb_buf_peek(ps2kb_buf *lb, uint8_t* code, uint8_t* value);
void ps2kb_buf_pop(ps2kb_buf *lb);

void ps2mouse_buf_init(ps2mouse_buf *lb, uint8_t size);
uint8_t ps2mouse_buf_add(ps2mouse_buf *lb, mouse_event* event);
mouse_event* ps2mouse_buf_peek(ps2mouse_buf *lb);
void ps2mouse_buf_pop(ps2mouse_buf *lb);

#ifdef __cplusplus
}
#endif

#endif


