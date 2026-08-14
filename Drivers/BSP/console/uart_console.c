/*
 * uart_console.c
 * Thread-safe UART output (FreeRTOS mutex once the scheduler runs).
 */
#include "uart_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static UART_HandleTypeDef *s_uart = NULL;
static SemaphoreHandle_t s_uart_mutex = NULL;

void console_init(UART_HandleTypeDef *huart) {
    s_uart = huart;
    s_uart_mutex = xSemaphoreCreateMutex();
}

static void console_transmit(const uint8_t *p, uint16_t len) {
    if (!s_uart || len == 0) return;
    if (s_uart_mutex && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
        if (xSemaphoreTake(s_uart_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
        HAL_UART_Transmit(s_uart, (uint8_t *)p, len, 100);
        xSemaphoreGive(s_uart_mutex);
    } else {
        HAL_UART_Transmit(s_uart, (uint8_t *)p, len, 100);
    }
}

void console_printf(const char *fmt, ...) {
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    console_transmit((const uint8_t *)buf, (uint16_t)n);
}

void dbg(const char *tag, const char *fmt, ...) {
    char msg[112];
    char out[144];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    int n = snprintf(out, sizeof(out), "[%s] DEBUG: %s\r\n", tag, msg);
    if (n <= 0) return;
    if (n > (int)sizeof(out) - 1) n = (int)sizeof(out) - 1;
    console_transmit((const uint8_t *)out, (uint16_t)n);
}

/* VOFA+ Firewater protocol: comma separated + newline. */
void vofaSend3(float a, float b, float c) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%.1f,%.1f,%.1f\r\n", a, b, c);
    if (n <= 0) return;
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    console_transmit((const uint8_t *)buf, (uint16_t)n);
}

void vofaSend4(float a, float b, float c, float d) {
    char buf[96];
    int n = snprintf(buf, sizeof(buf), "%.4f,%.4f,%.4f,%.4f\r\n", a, b, c, d);
    if (n <= 0) return;
    if (n > (int)sizeof(buf) - 1) n = (int)sizeof(buf) - 1;
    console_transmit((const uint8_t *)buf, (uint16_t)n);
}
