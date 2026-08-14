/*
 * app.cpp
 * Port of the original BNO086_MotionDash.ino application, split into two
 * FreeRTOS tasks (IMU polling + UI state machine) plus EXTI button ISRs.
 */
#include "app.h"

#include "task.h"

IMUManager imu;
DisplayManager display;
Button btnNext;
Button btnAct;

QueueHandle_t imuCmdQueue = NULL;
SemaphoreHandle_t uiEventSem = NULL;

AppState appState = APP_MENU;
AppState stateBeforeSleep = APP_MENU;
int menuIndex = 0;
DashPage currentPage = PAGE_EULER;

static uint32_t lastDrawMs = 0;
static uint32_t lastMenuStatusMs = 0;

void app_rtos_init(void) {
    imuCmdQueue = xQueueCreate(8, sizeof(ImuCmdMsg));
    uiEventSem = xSemaphoreCreateBinary();
}

void app_setup(void) {
    dbg("SYS", "boot");

    btnNext.begin(BTN_NEXT_GPIO_Port, BTN_NEXT_Pin);
    btnAct.begin(BTN_ACT_GPIO_Port, BTN_ACT_Pin);
    dbg("BTN", "buttons mounted, debounce %dms long %dms", BTN_DEBOUNCE_MS, BTN_LONGPRESS_MS);

    display.begin();

    bool ok = imu.begin(&hi2c1, BNO_I2C_ADDR,
                        BNO_INT_GPIO_Port, BNO_INT_Pin,
                        BNO_RST_GPIO_Port, BNO_RST_Pin);
    if (!ok) {
        display.drawInitError("BNO086 not detected");
        dbg("IMU", "BNO086 not detected, check addr/wiring");
        HAL_Delay(2500); /* show the error screen before entering the menu */
    }
    imu.enableForPage(PAGE_COUNT); /* heartbeat mode */

    display.drawMenu(menuIndex, imu.isConnected());
}

/* Non-blocking command send from the UI task. */
static void imuCmd(uint8_t cmd, DashPage page) {
    ImuCmdMsg m;
    m.cmd = cmd;
    m.page = (uint8_t)page;
    xQueueSend(imuCmdQueue, &m, 0);
}

/* ---------------- IMU task: poll + commands + self-heal ---------------- */
void app_imu_task(void *arg) {
    (void)arg;
    uint32_t lastWatchdogMs = 0;

    for (;;) {
        for (uint8_t n = 0; n < IMU_POLL_MAX_PER_LOOP; n++) {
            if (!imu.poll()) break;
        }

        /* Runtime bus self-heal (I2C error == old scl-stretch timeout). */
        if (imu.i2cError()) {
            dbg("I2C", "I2C error flag set, bus recovery");
            imu.attemptBusRecovery();
        }

        /* Process pending commands. */
        ImuCmdMsg m;
        while (xQueueReceive(imuCmdQueue, &m, 0) == pdTRUE) {
            switch (m.cmd) {
                case IMU_CMD_SET_PAGE: imu.enableForPage((DashPage)m.page); break;
                case IMU_CMD_TARE:     imu.tare(); break;
                case IMU_CMD_SLEEP:    imu.sleep(); break;
                case IMU_CMD_WAKE:     imu.wake(); break;
                default: break;
            }
        }

        /* Stale watchdog: only while a data page is subscribed. */
        DashPage page = imu.getEnabledPage();
        if (page != PAGE_COUNT) {
            bool isEventPage = (page == PAGE_STEP || page == PAGE_STABILITY ||
                                page == PAGE_ACTIVITY_CLASSIFIER);
            uint32_t threshold = isEventPage ? IMU_STALE_SLOW_MS : IMU_STALE_MS;
            uint32_t now = HAL_GetTick();
            if (imu.isConnected() && imu.msSinceLastUpdate() > threshold &&
                (now - lastWatchdogMs) > threshold) {
                lastWatchdogMs = now;
                imu.attemptBusRecovery();
                imu.forceResubscribe(page);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

/* ---------------- UI task: buttons + state machine + display ---------------- */
void app_ui_task(void *arg) {
    (void)arg;

    for (;;) {
        /* Wake on button (ISR) or every 10ms for periodic refresh. */
        xSemaphoreTake(uiEventSem, pdMS_TO_TICKS(10));

        /* -------- sleep: wait for any key -------- */
        if (appState == APP_SLEEP) {
            bool wakeEvent = btnNext.consumeShort() || btnNext.consumeLong() ||
                             btnAct.consumeShort() || btnAct.consumeLong();
            if (wakeEvent) {
                dbg("SYS", "key detected, exit sleep");
                btnNext.clearPending();
                btnAct.clearPending();
                imuCmd(IMU_CMD_WAKE, PAGE_COUNT);
                vTaskDelay(pdMS_TO_TICKS(50)); /* let the chip recover */
                appState = stateBeforeSleep;
                if (appState == APP_PAGE) {
                    imuCmd(IMU_CMD_SET_PAGE, currentPage);
                    display.drawStatic(currentPage);
                } else {
                    imuCmd(IMU_CMD_SET_PAGE, PAGE_COUNT);
                    display.drawMenu(menuIndex, imu.isConnected());
                }
            }
            continue;
        }

        /* -------- global: NEXT long = sleep -------- */
        if (btnNext.consumeLong()) {
            dbg("SYS", "enter sleep, black screen");
            stateBeforeSleep = appState;
            appState = APP_SLEEP;
            imuCmd(IMU_CMD_SLEEP, PAGE_COUNT);
            display.sleepScreen();
            btnAct.clearPending();
            continue;
        }

        uint32_t now = HAL_GetTick();

        switch (appState) {

            case APP_MENU:
                if (btnNext.consumeShort()) {
                    menuIndex = (menuIndex + 1) % PAGE_COUNT;
                    display.drawMenu(menuIndex, imu.isConnected());
                }
                if (btnAct.consumeShort()) {
                    currentPage = (DashPage)menuIndex;
                    dbg("SYS", "enter page -> %s", PAGE_NAMES[currentPage]);
                    appState = APP_PAGE;
                    imuCmd(IMU_CMD_SET_PAGE, currentPage);
                    display.drawStatic(currentPage);
                }
                if (btnAct.consumeLong()) {
                    dbg("SYS", "enter about");
                    appState = APP_ABOUT;
                    display.drawAbout();
                }
                if (now - lastMenuStatusMs >= MENU_STATUS_REFRESH_MS) {
                    lastMenuStatusMs = now;
                    display.updateMenuConnStatus(imu.isConnected());
                }
                break;

            case APP_PAGE: {
                if (btnNext.consumeShort()) {
                    currentPage = (DashPage)((currentPage + 1) % PAGE_COUNT);
                    dbg("SYS", "switch page -> %s", PAGE_NAMES[currentPage]);
                    imuCmd(IMU_CMD_SET_PAGE, currentPage);
                    display.drawStatic(currentPage);
                }
                if (btnAct.consumeShort()) {
                    dbg("BTN", "ACT -> tare");
                    imuCmd(IMU_CMD_TARE, PAGE_COUNT);
                }
                if (btnAct.consumeLong()) {
                    dbg("SYS", "back to menu");
                    appState = APP_MENU;
                    menuIndex = currentPage;
                    imuCmd(IMU_CMD_SET_PAGE, PAGE_COUNT);
                    display.drawMenu(menuIndex, imu.isConnected());
                    break;
                }

                /* Display refresh, decoupled from sensor sample rate. */
                if (now - lastDrawMs >= DISPLAY_REFRESH_MS) {
                    lastDrawMs = now;
                    IMUSnapshot snap;
                    imu.getSnapshot(snap);
                    display.drawDynamic(currentPage, snap, imu.isConnected());
                }

                /* VOFA+ output only on a fresh rotation-vector report. */
                if (currentPage == PAGE_EULER && imu.consumeRotationVectorFresh()) {
                    IMUSnapshot snap;
                    imu.getSnapshot(snap);
                    vofaSend3(snap.roll, snap.pitch, snap.yaw);
                }
                if (currentPage == PAGE_ROTATION_VECTOR && imu.consumeRotationVectorFresh()) {
                    IMUSnapshot snap;
                    imu.getSnapshot(snap);
                    vofaSend4(snap.qI, snap.qJ, snap.qK, snap.qReal);
                }
                break;
            }

            case APP_ABOUT:
                if (btnNext.consumeShort() || btnAct.consumeShort() || btnAct.consumeLong()) {
                    appState = APP_MENU;
                    display.drawMenu(menuIndex, imu.isConnected());
                }
                break;

            default:
                break;
        }
    }
}

/* ---------------- FreeRTOS hook ---------------- */
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    portDISABLE_INTERRUPTS();
    for (;;) {}
}

/* ---------------- button ISR callbacks ---------------- */
void btnNextISR(void) {
    btnNext.onInterrupt();
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(uiEventSem, &woken);
    portYIELD_FROM_ISR(woken);
}

void btnActISR(void) {
    btnAct.onInterrupt();
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(uiEventSem, &woken);
    portYIELD_FROM_ISR(woken);
}
