//
// Created by yangwei on 2026/5/4.
//

#ifndef AGENTAI_LED_BASE_H
#define AGENTAI_LED_BASE_H
#include "main.h"
// #include "led_base_general.h"

// struct led_ops;
/* 1. 前向声明 struct */
struct led_base;
struct led_ops;

/* 2. typedef 提前定义类型名 */
typedef struct led_base led_base;
typedef struct led_ops led_ops;

/* 3. 函数指针定义（可以用前向声明的类型） */
typedef void (*led_on_fn)(led_base *led);
typedef void (*led_off_fn)(led_base *led);
typedef void (*led_set_brightness)(led_base *led, uint32_t brightness);

/* 4. 定义 led_ops */
struct led_ops {
	led_on_fn on;
	led_off_fn off;
	led_set_brightness set_brightness;
};

/* 5. 定义 led_base */
struct led_base {
	char *name;
	uint8_t state;
	led_ops *ops;
};


void led_base_init(led_base *led, char *name, uint8_t state);

void led_base_deinit(led_base *led);


char *led_base_name(led_base *led);

#endif //AGENTAI_LED_BASE_H