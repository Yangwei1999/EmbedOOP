//
// Created by yangwei on 2026/5/4.
//

#ifndef AGENTAI_LED_BASE_GENERAL_H
#define AGENTAI_LED_BASE_GENERAL_H

#include "main.h"
#include "led_base.h"
typedef void (*led_on_fn)(led_base *led);
typedef void (*led_off_fn)(led_base *led);
typedef void (*led_set_brightness)(led_base *led, uint32_t brightness);


typedef struct  {
	led_on_fn on;
	led_off_fn off;
	led_set_brightness set_brightness;
} led_ops;

// typedef struct {
// 	char *name;
// 	uint8_t state;
// 	void (*on)(void);
// 	void (*off)(void);
// 	void (*set_brightness)(led_device_t *led, uint8_t bright);
// } led_device_t;

#endif //AGENTAI_LED_BASE_GENERAL_H