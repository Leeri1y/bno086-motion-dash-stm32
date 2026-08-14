/**
  ******************************************************************************
  * @file    main.c
  * @brief   BNO086 MotionDash - STM32F407VET6 + FreeRTOS entry point.
  ******************************************************************************
  */
#include "main.h"
#include "stm32f4xx_it.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app.h"
#include "uart_console.h"

I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
/* htim6 is defined in stm32f4xx_hal_timebase_tim.c (HAL tick source). */

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    /* Re-derive the HAL (TIM6) timebase now that the PLL has brought the bus
       clocks from the reset HSI rate up to the 168 MHz configuration. */
    HAL_InitTick(TICK_INT_PRIORITY);

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    console_init(&huart1);
    app_rtos_init();
    app_setup();

    xTaskCreate(app_imu_task, "imu", 512, NULL, 3, NULL);
    xTaskCreate(app_ui_task,  "ui",  1024, NULL, 2, NULL);

    vTaskStartScheduler();

    Error_Handler(); /* should never reach */
}

/* SYSCLK = 168 MHz (HSE 8 MHz, PLLM=8, PLLN=336, PLLP=2) */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* TFT_RST (PA4) + TFT_BL (PA8) output high */
    HAL_GPIO_WritePin(GPIOA, TFT_RST_Pin | TFT_BL_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = TFT_RST_Pin | TFT_BL_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* TFT_CS (PC4) + TFT_DC (PC5) + BNO_RST (PC7) output high */
    HAL_GPIO_WritePin(GPIOC, TFT_CS_Pin | TFT_DC_Pin | BNO_RST_Pin, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = TFT_CS_Pin | TFT_DC_Pin | BNO_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* BNO_INT (PC6) input with pull-up (polled by the driver). */
    GPIO_InitStruct.Pin = BNO_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Buttons PB0/PB1: EXTI on both edges (press + release). */
    GPIO_InitStruct.Pin = BTN_NEXT_Pin | BTN_ACT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE; /* BNO086 uses clock stretching */
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_SPI1_Init(void) {
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; /* 84/4 = 21 MHz */
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}
