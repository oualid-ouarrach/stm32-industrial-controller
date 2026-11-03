
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32 Industrial Controller - ADC, Start/Stop and PWM
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
#define LED_PIN               GPIO_PIN_0
#define LED_PORT              GPIOA
#define BLINK_DELAY_MS        500U

#define BUTTON_PIN            GPIO_PIN_1
#define BUTTON_PORT           GPIOA
#define DEBOUNCE_MS           40U

#define ADC_SAMPLE_PERIOD_MS  100U
#define BLINK_MIN_MS          150U
#define BLINK_RANGE_MS        850U
#define PWM_PERIOD_COUNTS     1000U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
static SystemState_t system_state = SYSTEM_IDLE;

static GPIO_PinState button_last_raw = GPIO_PIN_SET;
static GPIO_PinState button_stable = GPIO_PIN_SET;

static uint32_t button_change_time = 0U;
static uint32_t led_toggle_time = 0U;

static volatile uint32_t adc_raw = 0U;
static volatile uint32_t adc_mV = 0U;

static uint32_t adc_sample_time = 0U;
static uint32_t blink_delay_ms = BLINK_DELAY_MS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief Application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM3_Init();

    /* USER CODE BEGIN 2 */

    /* Initial state: IDLE, LED OFF */
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    button_last_raw = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
    button_stable = button_last_raw;

    button_change_time = HAL_GetTick();
    led_toggle_time = HAL_GetTick();

    /* Calibrate ADC1 before first conversion */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    adc_sample_time = HAL_GetTick();

    /* Safe startup: PWM at 0% in IDLE */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);

    /* Start TIM3 Channel 1 PWM on PA6 */
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */

        uint32_t now = HAL_GetTick();

        /* ADC acquisition every 100 ms */
        if ((now - adc_sample_time) >= ADC_SAMPLE_PERIOD_MS)
        {
            adc_sample_time = now;

            if (HAL_ADC_Start(&hadc1) == HAL_OK)
            {
                if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
                {
                    adc_raw = HAL_ADC_GetValue(&hadc1);

                    adc_mV = (adc_raw * 3300U) / 4095U;

                    /* ADC-controlled LED blinking speed */
                    blink_delay_ms = BLINK_MIN_MS
                        + ((4095U - adc_raw) * BLINK_RANGE_MS) / 4095U;
                }

                (void)HAL_ADC_Stop(&hadc1);
            }
        }

        /* Read START/STOP button */
        GPIO_PinState button_raw =
            HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);

        /* Detect raw button changes */
        if (button_raw != button_last_raw)
        {
            button_last_raw = button_raw;
            button_change_time = now;
        }

        /* Debounce and state management */
        if (((now - button_change_time) >= DEBOUNCE_MS)
            && (button_raw != button_stable))
        {
            button_stable = button_raw;

            if (button_stable == GPIO_PIN_RESET)
            {
                if (system_state == SYSTEM_IDLE)
                {
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
                    system_state = SYSTEM_IDLE;

                    HAL_GPIO_WritePin(
                        LED_PORT,
                        LED_PIN,
                        GPIO_PIN_RESET
                    );
                }
            }
        }

        /* PWM control using ADC and system state */
        uint32_t pwm_pulse = 0U;

        if (system_state == SYSTEM_RUNNING)
        {
            /* Convert ADC 0-4095 to PWM 0-1000 */
            pwm_pulse = (adc_raw * PWM_PERIOD_COUNTS) / 4095U;
        }

        /* IDLE = 0%; RUNNING = ADC-controlled duty cycle */
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_pulse);

        /* Non-blocking LED blinking */
        if (system_state == SYSTEM_RUNNING)
        {
            if ((now - led_toggle_time) >= blink_delay_ms)
            {
                led_toggle_time = now;
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            }
        }

        /* USER CODE END 3 */
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief ADC1 Initialization Function
  * @retval None
  */
static void MX_ADC1_Init(void)
{
    /* USER CODE BEGIN ADC1_Init 0 */
    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */
    /* USER CODE END ADC1_Init 1 */

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;

    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /* ADC Channel 2: PA2 */
    sConfig.Channel = ADC_CHANNEL_2;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN ADC1_Init 2 */
    /* USER CODE END ADC1_Init 2 */
}

/**
  * @brief TIM3 Initialization Function
  * @retval None
  */
static void MX_TIM3_Init(void)
{
    /* USER CODE BEGIN TIM3_Init 0 */
    /* USER CODE END TIM3_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM3_Init 1 */
    /* USER CODE END TIM3_Init 1 */

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim3,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* TIM3 Channel 1: PWM 1 kHz */
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim3,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN TIM3_Init 2 */
    /* USER CODE END TIM3_Init 2 */

    HAL_TIM_MspPostInit(&htim3);
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

    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(
        LED_BLINKING_GPIO_Port,
        LED_BLINKING_Pin,
        GPIO_PIN_RESET
    );

    /* PA0 - LED output */
    GPIO_InitStruct.Pin = LED_BLINKING_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LED_BLINKING_GPIO_Port, &GPIO_InitStruct);

    /* PA1 - START/STOP button */
    GPIO_InitStruct.Pin = START_STOP_BTN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(START_STOP_BTN_GPIO_Port, &GPIO_InitStruct);

    /* PA2: ADC configured in HAL_ADC_MspInit() */
    /* PA6: PWM configured in HAL_TIM_MspPostInit() */

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
        /* Stop here if an unrecoverable error occurs */
    }

    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    (void)file;
    (void)line;
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
