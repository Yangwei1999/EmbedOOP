//
// Created by yangwei on 2026/5/4.
//

#ifndef AGENTAI_LED_H
#define AGENTAI_LED_H

#include "led_base.h"

typedef struct {
	led_base base;
	uint32_t brightness;
	TIM_HandleTypeDef *htim;
	uint32_t channel;
} led_pwm_t;

void led_pwm_init(led_pwm_t *led_pwm, char *name, TIM_HandleTypeDef *htim, uint32_t channel, uint32_t brightness);

void led_pwm_set(led_pwm_t *led_pwm, uint32_t brightness);

#endif //AGENTAI_LED_H