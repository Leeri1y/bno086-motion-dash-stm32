/**
  ******************************************************************************
  * @file    stm32f4xx_it.h
  * @brief   Interrupt handler declarations (FreeRTOS owns SVC/PendSV/SysTick).
  ******************************************************************************
  */
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);

/* TIM6_DAC_IRQHandler is defined in stm32f4xx_hal_timebase_tim.c */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
