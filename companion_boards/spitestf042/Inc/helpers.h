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

#define SPI_MOSI_MSG_INFO_REQUEST 0
#define SPI_MOSI_MSG_KB_EVENT 1
#define SPI_MOSI_MSG_MOUSE_EVENT 2
#define SPI_MOSI_MSG_GAMEPAD_EVENT 3
#define SPI_MOSI_MSG_REQ_ACK 4

#define SPI_MISO_MSG_INFO_REPLY 0
#define SPI_MISO_MSG_KB_LED_REQ 1

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
  uint16_t button;
  uint8_t button_state;
} mouse_event;

typedef struct
{
  uint8_t head;
  uint8_t tail;
  uint8_t size;
  mouse_event* mouse_events;
} ps2mouse_buf;

void ps2kb_buf_init(ps2kb_buf *lb, uint8_t size);
uint8_t ps2kb_buf_add(ps2kb_buf *lb, uint8_t code, uint8_t value);
uint8_t ps2kb_buf_get(ps2kb_buf *lb, uint8_t* code, uint8_t* value);

void ps2mouse_buf_init(ps2mouse_buf *lb, uint8_t size);
uint8_t ps2mouse_buf_add(ps2mouse_buf *lb, mouse_event* event);
mouse_event* ps2mouse_buf_get(ps2mouse_buf *lb);

#ifdef __cplusplus
}
#endif

#endif


