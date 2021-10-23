#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "shared.h"

GPIO_TypeDef* ps2kb_clk_port;
uint16_t ps2kb_clk_pin;

GPIO_TypeDef* ps2kb_data_port;
uint16_t ps2kb_data_pin;

void ps2kb_init(GPIO_TypeDef* clk_port, uint16_t clk_pin, GPIO_TypeDef* data_port, uint16_t data_pin)
{
	ps2kb_clk_port = clk_port;
	ps2kb_clk_pin = clk_pin;
	ps2kb_data_port = data_port;
	ps2kb_data_pin = data_pin;
}

