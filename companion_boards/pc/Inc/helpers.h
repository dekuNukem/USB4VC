#ifndef __HELPERS_H
#define __HELPERS_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "stm32f0xx_hal.h"

#define SPI_BUF_SIZE 32

extern uint8_t spi_recv_buf[SPI_BUF_SIZE];

#ifdef __cplusplus
}
#endif

#endif


