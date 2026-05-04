//
// Created by yangwei on 2026/5/4.
//

#include "led_gpio.h"



void led_gpio_init(led_gpio_t *led_gpio, char *name,GPIO_TypeDef *port, uint32_t pin_num, uint8_t on_state) {

	led_base_init(&(led_gpio->base), name, 0);

	led_gpio->port_addr = port;
	led_gpio->pin_num = pin_num;
	led_gpio->on_state = on_state;
}

void led_gpio_deinit(led_gpio_t *led_gpio) {
	led_base_deinit(&(led_gpio->base));
	led_gpio->pin_num = 0;
	led_gpio->on_state = 0;
	led_gpio->port_addr = 0;
}

void led_gpio_on(led_gpio_t *led_gpio) {
	HAL_GPIO_WritePin(led_gpio->port_addr, led_gpio->pin_num, led_gpio->on_state);
}

void led_gpio_off(led_gpio_t *led_gpio) {
	HAL_GPIO_WritePin(led_gpio->port_addr, led_gpio->pin_num, 1 - led_gpio->on_state);
}