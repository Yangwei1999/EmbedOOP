/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "w25q64jv.h"

#include <stdio.h>
// #include "led.h"
#include "led_gpio.h"
#include "led_pwm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FLASH_TEST_ADDRESS  0x000001F0UL
#define FLASH_TEST_LENGTH   300U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// static const char *FlashStatusToString(W25Q64JV_Status status);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// extern led_ops gpio_ops;
// extern led_ops pwm_ops;

/*Configure GPIO pin Output Level */
// HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);


void led_on(led_base *led) {
	led_ops *ops = led->ops;
	if (ops->on != NULL) {
		ops->on(led);
		return;
	}

	if (ops->set_brightness != NULL) {
		ops->set_brightness(led, 100);
		return;
	}

	return;
}

void led_off(led_base *led) {
	led_ops *ops = led->ops;
	if (ops->off != NULL) {
		ops->off(led);
		return;
	}

	if (ops->set_brightness != NULL) {
		ops->set_brightness(led, 0);
		return;
	}

	return;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	// TIM2_CH2

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

	// HAL_TIM_PWM_Init(&htim2);
	// HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);


	led_gpio_t led_red,led_green = { 0 };
	led_pwm_t led_pwm,led_pwm2;

	// led_gpio_init(&led_red, "led_red", LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
	led_gpio_init(&led_green, "led_green", LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
	// led_pwm_init(&led_pwm, "led_pwm", &htim2, TIM_CHANNEL_2, 50);
	led_pwm_init(&led_pwm2, "led_pwm2", &htim4, TIM_CHANNEL_1, 10);

	// led_t led_red,led_green;
	// led_init(&led_red,LED_RED_GPIO_Port,LED_RED_Pin,GPIO_PIN_RESET);
	// led_init(&led_green,LED_GREEN_GPIO_Port,LED_GREEN_Pin,GPIO_PIN_RESET);


	// printf("[GPIO] addr is 11 %x \r\n", &((GPIO_TypeDef *)LED_RED_GPIO_Port)->ODR);
	// printf("led name  %s \r\n", led_base_name(&led_red.base));
	printf("led name  %s \r\n", led_base_name(&led_green.base));

	// printf("led name  %s \r\n", led_base_name(&led_red));
	printf("led name  %s \r\n", led_base_name(&led_green));

	// printf("base %x total %x\r\n", &led_red.base, &led_red);
	// __containerof()

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  	led_on(&led_green);
	// led_gpio_on(&led_green);
  	// led_gpio_off(&led_red);
  	// led_pwm_set(&led_pwm2, 0);
  	led_on(&led_pwm2);
    HAL_Delay(1000);
  	led_off(&led_green);
  	// led_off(&led_pwm2);
	led_gpio_off(&led_green);
  	// led_gpio_on(&led_red);
  	// led_pwm_set(&led_pwm2, 100);
    HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
  uint8_t data = (uint8_t)ch;

  HAL_UART_Transmit(&huart1, &data, 1U, HAL_MAX_DELAY);

  return ch;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
