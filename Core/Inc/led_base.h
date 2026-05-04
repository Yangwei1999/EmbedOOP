//
// Created by yangwei on 2026/5/4.
//

#ifndef AGENTAI_LED_BASE_H
#define AGENTAI_LED_BASE_H
#include "main.h"

typedef struct {
	char *name;
	uint8_t state;
} led_base;

void led_base_init(led_base *led, char *name, uint8_t state);

void led_base_deinit(led_base *led);


char *led_base_name(led_base *led);

#endif //AGENTAI_LED_BASE_H