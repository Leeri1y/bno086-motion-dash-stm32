/*
 * FreeRTOS V10.x configuration for STM32F407 (Cortex-M4F), MDK-ARM.
 * Uses the RVDS/ARM_CM4F port; SysTick/PendSV/SVC are owned by FreeRTOS.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
/* SysTick clock = SYSCLK (168 MHz). Hardcoded because port.c (a standalone
   compilation unit) has no device header, so SystemCoreClock is not visible
   there. Keep this in sync with SystemClock_Config() if the clock changes. */
#define configCPU_CLOCK_HZ                      ( 168000000UL )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    8
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 24 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TIMERS                        0
#define configUSE_TIME_SLICING                  1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configUSE_MALLOC_FAILED_HOOK            0
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS       __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS       4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
   function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
/* The highest interrupt priority that can be used by any interrupt service
   routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT
   CALL INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A
   HIGHER PRIORITY THAN THIS! (higher priorities are lower numeric values). */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )

/* FreeRTOS handlers take over the CMSIS vector names. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/*-----------------------------------------------------------
 * API functions included by the kernel.
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet           1
#define INCLUDE_uxTaskPriorityGet          1
#define INCLUDE_vTaskDelete                1
#define INCLUDE_vTaskCleanUpResources      0
#define INCLUDE_vTaskSuspend               1
#define INCLUDE_vTaskDelayUntil            1
#define INCLUDE_vTaskDelay                 1
#define INCLUDE_xTaskGetSchedulerState     1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetIdleTaskHandle     0

/* configASSERT behaviour (portDISABLE_INTERRUPTS is visible in every
   FreeRTOS source file via FreeRTOS.h -> portmacro.h). */
#define configASSERT( x ) if( ( x ) == 0 ) { portDISABLE_INTERRUPTS(); for( ;; ); }

#endif /* FREERTOS_CONFIG_H */
