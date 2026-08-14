/**
  ******************************************************************************
  * @file    stm32f4xx_hal_conf.h
  * @brief   HAL configuration file for BNO086 MotionDash (STM32F407VET6)
  ******************************************************************************
  */
#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* ########################## Oscillator Values ############################# */
#define HSE_VALUE            8000000U /* STM32F407VET6 board crystal: 8 MHz */
#define LSE_VALUE            32768U
#define HSE_STARTUP_TIMEOUT  100U
#define LSE_STARTUP_TIMEOUT  5000U
#define EXTERNAL_CLOCK_VALUE 12288000U
#define HSI_VALUE            16000000U
#define LSI_VALUE            32000U
#define VDD_VALUE            3300U
#define PREFETCH_ENABLE      1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE    1U
#define USE_SPI_CRC          0U
#define USE_RTOS             0U
#define USE_HAL_ADC_REGISTER_CALLBACKS     0U
#define USE_HAL_CEC_REGISTER_CALLBACKS     0U
#define USE_HAL_DAC_REGISTER_CALLBACKS     0U
#define USE_HAL_I2C_REGISTER_CALLBACKS     0U
#define USE_HAL_SPI_REGISTER_CALLBACKS     0U
#define USE_HAL_TIM_REGISTER_CALLBACKS     0U
#define USE_HAL_UART_REGISTER_CALLBACKS    0U

/* ########################## SysTick/HAL timebase ######################### */
/* With FreeRTOS, HAL timebase is provided by TIM6 (see
   stm32f4xx_hal_timebase_tim.c). Priority must be the lowest. */
#define TICK_INT_PRIORITY    0x0FU

/* ########################## Assert Selection ############################## */
#define USE_FULL_ASSERT      0U

#if (USE_FULL_ASSERT == 1U)
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32f4xx_hal_rcc.h"
#endif /* HAL_RCC_MODULE_ENABLED */

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32f4xx_hal_gpio.h"
#endif /* HAL_GPIO_MODULE_ENABLED */

#ifdef HAL_EXTI_MODULE_ENABLED
  #include "stm32f4xx_hal_exti.h"
#endif /* HAL_EXTI_MODULE_ENABLED */

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32f4xx_hal_dma.h"
#endif /* HAL_DMA_MODULE_ENABLED */

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32f4xx_hal_cortex.h"
#endif /* HAL_CORTEX_MODULE_ENABLED */

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32f4xx_hal_flash.h"
#endif /* HAL_FLASH_MODULE_ENABLED */

#ifdef HAL_I2C_MODULE_ENABLED
  #include "stm32f4xx_hal_i2c.h"
#endif /* HAL_I2C_MODULE_ENABLED */

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32f4xx_hal_pwr.h"
#endif /* HAL_PWR_MODULE_ENABLED */

#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32f4xx_hal_spi.h"
#endif /* HAL_SPI_MODULE_ENABLED */

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32f4xx_hal_tim.h"
#endif /* HAL_TIM_MODULE_ENABLED */

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32f4xx_hal_uart.h"
#endif /* HAL_UART_MODULE_ENABLED */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
