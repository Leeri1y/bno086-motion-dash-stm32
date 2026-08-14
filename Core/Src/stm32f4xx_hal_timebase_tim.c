/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_tim.c
  * @brief   HAL timebase on TIM6 (FreeRTOS owns SysTick). HAL_Delay/HAL_GetTick
  *          are driven by a 1 kHz TIM6 interrupt at the lowest priority.
  ******************************************************************************
  */
#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim6;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    RCC_ClkInitTypeDef clkconfig;
    uint32_t uwTimclock = 0;
    uint32_t uwPrescalerValue = 0;
    uint32_t pFLatency = 0;

    __HAL_RCC_TIM6_CLK_ENABLE();

    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    /* Timer clock is 2x PCLK1 when APB1 prescaler != 1. */
    if (clkconfig.APB1CLKDivider == RCC_HCLK_DIV1)
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    else
        uwTimclock = 2UL * HAL_RCC_GetPCLK1Freq();

    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U); /* 1 us tick */

    htim6.Instance = TIM6;
    htim6.Init.Period = (1000000U / 1000U) - 1U; /* 1 ms period */
    htim6.Init.Prescaler = uwPrescalerValue;
    htim6.Init.ClockDivision = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim6);
    HAL_TIM_Base_Start_IT(&htim6);

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    return HAL_OK;
}

void HAL_SuspendTick(void) {
    __HAL_TIM_DISABLE_IT(&htim6, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void) {
    __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
}

void TIM6_DAC_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        HAL_IncTick();
    }
}
