#ifndef __M0110A_H
#define __M0110A_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define M0110A_OK 0
#define M0110A_LINE_IDLE 1
#define M0110A_LINE_HOST_REQ 2
#define M0110A_TIMEOUT 3
#define M0110A_UNKNOWN_CODE 4

#define M0110A_KB_TO_PC_CMD_BUF_SIZE 8
#define LINUX_KEYCODE_TO_M0110A_SCANCODE_SIZE 128

#define CODE_UNUSED 0

extern const uint8_t linux_keycode_to_m0110a_scancode_lookup[LINUX_KEYCODE_TO_M0110A_SCANCODE_SIZE];

typedef struct
{
  uint8_t head;
  uint8_t tail;
  uint8_t* cmd_buf;
} m0110a_cmd_buf;

uint8_t m0110a_get_line_status(void);
uint8_t m0110a_read_host_cmd(uint8_t* result, uint16_t timeout_ms);
uint8_t m0110a_write(uint8_t data);

void m0110a_cmd_buf_init(m0110a_cmd_buf *lb);
uint8_t m0110a_cmd_buf_add(m0110a_cmd_buf *lb, uint8_t code);
uint8_t m0110a_cmd_buf_peek(m0110a_cmd_buf *lb, uint8_t* code);
void m0110a_cmd_buf_pop(m0110a_cmd_buf *lb);
uint8_t m0110a_cmd_buf_is_empty(m0110a_cmd_buf *lb);
void m0110a_cmd_buf_reset(m0110a_cmd_buf *lb);
uint8_t make_m0110a_scancode(uint8_t linux_key_code, uint8_t linux_key_value, m0110a_cmd_buf *lb);
void m0110a_reset(m0110a_cmd_buf *lb);

#ifdef __cplusplus
}
#endif

#endif


