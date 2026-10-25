
/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32 Industrial Controller - GPIO Start/Stop Control
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found
 * in the LICENSE file in the root directory of this software
 * component. If no LICENSE file comes with this software,
 * it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    SYSTEM_IDLE = 0,
    SYSTEM_RUNNING
} SystemState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define LED_PIN          GPIO_PIN_0
#define LED_PORT         GPIOA
#define BLINK_DELAY_MS   500U

#define BUTTON_PIN       GPIO_PIN_1
#define BUTTON_PORT      GPIOA
#define DEBOUNCE_MS      40U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

static SystemState_t system_state = SYSTEM_IDLE;

static GPIO_PinState button_last_raw = GPIO_PIN_SET;
static GPIO_PinState button_stable = GPIO_PIN_SET;

static uint32_t button_change_time = 0U;
static uint32_t led_toggle_time = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration------------------------------------------------------*/

    /* Reset peripherals and initialize Flash and SysTick */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize configured peripherals */
    MX_GPIO_Init();

    /* USER CODE BEGIN 2 */

    /* Initial system state: IDLE, LED OFF */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    /* Read initial button state */
    button_last_raw = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
    button_stable = button_last_raw;

    /* Initialize timing variables */
    button_change_time = HAL_GetTick();
    led_toggle_time = HAL_GetTick();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        uint32_t now = HAL_GetTick();

        /* Read START/STOP button */
        GPIO_PinState button_raw =
            HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

        /* Detect raw button changes */
        if (button_raw != button_last_raw)
        {
            button_last_raw = button_raw;
            button_change_time = now;
        }

        /* Button debounce */
        if ((now - button_change_time >= DEBOUNCE_MS) &&
            (button_raw != button_stable))
        {
            button_stable = button_raw;

            /* Active-low button: pressed = RESET */
            if (button_stable == GPIO_PIN_RESET)
            {
                if (system_state == SYSTEM_IDLE)
                {
                    /* Switch to RUNNING */
                    system_state = SYSTEM_RUNNING;

                    HAL_GPIO_WritePin(
                        LED_PORT,
                        LED_PIN,
                        GPIO_PIN_SET
                    );

                    led_toggle_time = now;
                }
                else
                {
                    /* Switch to IDLE */
                    system_state = SYSTEM_IDLE;

                    HAL_GPIO_WritePin(
                        LED_PORT,
                        LED_PIN,
                        GPIO_PIN_RESET
                    );
                }
            }
        }

        /* Non-blocking LED blinking */
        if (system_state == SYSTEM_RUNNING)
        {
            if ((now - led_toggle_time) >= BLINK_DELAY_MS)
            {
                led_toggle_time = now;

                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            }
        }

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

    /* Configure HSI internal oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure CPU, AHB and APB clocks */
    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                             FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */

    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure initial LED output level */
    HAL_GPIO_WritePin(
        LED_BLINKING_GPIO_Port,
        LED_BLINKING_Pin,
        GPIO_PIN_RESET
    );

    /* Configure PA0: LED output */
    GPIO_InitStruct.Pin = LED_BLINKING_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(
        LED_BLINKING_GPIO_Port,
        &GPIO_InitStruct
    );

    /* Configure PA1: START/STOP button input */
    GPIO_InitStruct.Pin = START_STOP_BTN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(
        START_STOP_BTN_GPIO_Port,
        &GPIO_InitStruct
    );

    /* USER CODE BEGIN MX_GPIO_Init_2 */

    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief Error Handler
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
        /* Stay here if an error occurs */
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Assert failure handler
 * @param file Source file
 * @param line Source line number
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */

    (void)file;
    (void)line;

    /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
