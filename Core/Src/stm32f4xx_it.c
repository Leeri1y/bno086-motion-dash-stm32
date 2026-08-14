/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt handlers (faults + button EXTI). FreeRTOS owns
  *          SVC/PendSV/SysTick; TIM6 handler lives in the timebase file.
  ******************************************************************************
  */
#include "main.h"
#include "stm32f4xx_it.h"
#include "app.h"

void NMI_Handler(void)          { while (1) {} }
void HardFault_Handler(void)    { while (1) {} }
void MemManage_Handler(void)    { while (1) {} }
void BusFault_Handler(void)     { while (1) {} }
void UsageFault_Handler(void)   { while (1) {} }
void DebugMon_Handler(void)     {}

void EXTI0_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(BTN_NEXT_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(BTN_NEXT_Pin);
        btnNextISR();
    }
}

void EXTI1_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(BTN_ACT_Pin)) {
        __HAL_GPIO_EXTI_CLEAR_IT(BTN_ACT_Pin);
        btnActISR();
    }
}
