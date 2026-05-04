//
// Created by yangwei on 2026/5/4.
//


/*
 * led bsae
 */
#include "led.h"
// #include "main.h"


void led_init(led_t *led, GPIO_TypeDef * port, uint32_t pin_num, uint8_t on_state) {
	led->port_addr = port;
	led->pin_num = pin_num;
	led->on_state = on_state;
}

void led_on(led_t *led) {
	HAL_GPIO_WritePin(led->port_addr, led->pin_num, led->on_state);
}

void led_off(led_t *led) {
	HAL_GPIO_WritePin((GPIO_TypeDef *)led->port_addr, led->pin_num, 1 - led->on_state);
}