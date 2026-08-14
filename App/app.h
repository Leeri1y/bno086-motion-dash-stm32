/*
 * app.h
 * Application glue: global objects, FreeRTOS objects, tasks, ISRs.
 * C-compatible: the C-callable entry points are exported with C linkage so
 * main.c / stm32f4xx_it.c (C) can call them; the C++ parts are guarded.
 */
#ifndef APP_H
#define APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_rtos_init(void);   /* create queues/semaphores (before scheduler) */
void app_setup(void);       /* init display/imu/buttons (before scheduler) */
void app_imu_task(void *arg);
void app_ui_task(void *arg);

/* Button EXTI callbacks (from stm32f4xx_it.c). */
void btnNextISR(void);
void btnActISR(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include "Config.h"
#include "IMUManager.h"
#include "DisplayManager.h"
#include "ButtonManager.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

extern IMUManager imu;
extern DisplayManager display;
extern Button btnNext;
extern Button btnAct;

extern QueueHandle_t imuCmdQueue;
extern SemaphoreHandle_t uiEventSem;

/* IMU task commands (UI -> IMU). */
enum ImuCmd {
    IMU_CMD_SET_PAGE = 0,
    IMU_CMD_TARE,
    IMU_CMD_SLEEP,
    IMU_CMD_WAKE,
};
struct ImuCmdMsg {
    uint8_t cmd;
    uint8_t page;
};

#endif /* __cplusplus */

#endif /* APP_H */
