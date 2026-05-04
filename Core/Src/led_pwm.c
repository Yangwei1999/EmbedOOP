//
// Created by yangwei on 2026/5/4.
//

#include "led_pwm.h"
#include "led_base_general.h"
#include "tim.h"


static led_ops pwm_ops = {
	.on = NULL,
	.off = NULL,
	.set_brightness = led_pwm_set,
};

void led_pwm_init(led_pwm_t *led_pwm, char *name, TIM_HandleTypeDef *htim, uint32_t channel, uint32_t brightness) {
	led_base_init(&(led_pwm->base), name, 0);
	led_pwm->channel = channel;
	led_pwm->htim = htim;
	led_pwm->base.ops = &pwm_ops;

	led_pwm_set(led_pwm, brightness);

	HAL_TIM_PWM_Start(htim, channel);
}


void led_pwm_deinit(void) {
	;
}

void led_pwm_set(led_pwm_t *led_pwm, uint32_t brightness) {
	// bright 0 100
	// 100 -> 1000 0 ->0
	uint32_t duty = brightness * 10;
	__HAL_TIM_SET_COMPARE(led_pwm->htim, led_pwm->channel, duty);
}
