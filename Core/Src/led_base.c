//
// Created by yangwei on 2026/5/4.
//

#include "led_base.h"


void led_base_init(led_base *led, char *name, uint8_t state) {
	led->name = name;
	led->state = state;
}

void led_base_deinit(led_base *led) {
	led->name = NULL;
	led->state = 0;
}

char *led_base_name(led_base *led) {
	return led->name;
}