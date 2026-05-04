//
// Created by yangwei on 2026/5/4.
//

#ifndef AGENTAI_LED_GPIO_H
#define AGENTAI_LED_GPIO_H

#include "led_base.h"
#include "led_base_general.h"

typedef struct {
	led_base base;
	GPIO_TypeDef *port_addr;
	uint32_t pin_num;
	uint8_t on_state;
} led_gpio_t;

void led_gpio_init(led_gpio_t *led_gpio, char *name,GPIO_TypeDef *port, uint32_t pin_num, uint8_t on_state);
void led_gpio_deinit(led_gpio_t *led_gpio);


void led_gpio_on(led_gpio_t *led_gpio);

void led_gpio_off(led_gpio_t *led_gpio);

#endif //AGENTAI_LED_GPIO_H