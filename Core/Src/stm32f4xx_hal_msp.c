/**
  ******************************************************************************
  * @file    stm32f4xx_hal_msp.c
  * @brief   MSP (peripheral GPIO/clock) initialisation.
  ******************************************************************************
  */
#include "main.h"

void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* FreeRTOS requires 4-bit preemption priority. */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        __HAL_RCC_USART1_CLK_ENABLE();
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        __HAL_RCC_TIM6_CLK_ENABLE();
    }
}
