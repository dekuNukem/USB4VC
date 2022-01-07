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
#define SPI_MOSI_MSG_TYPE_SET_PROTOCOL 2
#define SPI_MOSI_MSG_TYPE_REQ_ACK 3

#define SPI_MOSI_MSG_TYPE_KEYBOARD_EVENT 8
#define SPI_MOSI_MSG_TYPE_MOUSE_EVENT 9
#define SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_RAW 10
#define SPI_MOSI_MSG_TYPE_GAMEPAD_EVENT_MAPPED_IBMPC 11

#define SPI_MISO_MSG_TYPE_NOP 0
#define SPI_MISO_MSG_TYPE_INFO_REQUEST 128
#define SPI_MISO_MSG_TYPE_KB_LED_REQUEST 129

#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE    0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

#define PROTOCOL_AT_PS2_KB 1
#define PROTOCOL_XT_KB 2
#define PROTOCOL_ADB_KB 3
#define PROTOCOL_PS2_MOUSE 4
#define PROTOCOL_MICROSOFT_SERIAL_MOUSE 5
#define PROTOCOL_ADB_MOUSE 6
#define PROTOCOL_GENERIC_GAMEPORT_GAMEPAD 7
#define PROTOCOL_GAMEPORT_GRAVIS_GAMEPAD  8
#define PROTOCOL_GAMEPORT_MICROSOFT_SIDEWINDER 9


typedef struct
{
  uint8_t head;
  uint8_t tail;
  uint8_t size;
  uint8_t* keycode_buf;
  uint8_t* keyvalue_buf;
} kb_buf;

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
} mouse_buf;

void kb_buf_init(kb_buf *lb, uint8_t size);
uint8_t kb_buf_add(kb_buf *lb, uint8_t code, uint8_t value);
uint8_t kb_buf_peek(kb_buf *lb, uint8_t* code, uint8_t* value);
void kb_buf_pop(kb_buf *lb);
uint8_t kb_buf_is_empty(kb_buf *lb);
void kb_buf_reset(kb_buf *lb);

void mouse_buf_init(mouse_buf *lb, uint8_t size);
uint8_t mouse_buf_add(mouse_buf *lb, mouse_event* event);
mouse_event* mouse_buf_peek(mouse_buf *lb);
void mouse_buf_pop(mouse_buf *lb);
void mouse_buf_reset(mouse_buf *lb);
uint8_t mouse_buf_is_empty(mouse_buf *lb);

#ifdef __cplusplus
}
#endif

#endif


