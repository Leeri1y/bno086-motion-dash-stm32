/*
 * uart_console.h
 * UART debug / VOFA+ console output (replaces Arduino Serial + dbg/vofa).
 */
#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void console_init(UART_HandleTypeDef *huart);
void console_printf(const char *fmt, ...);
void dbg(const char *tag, const char *fmt, ...);
void vofaSend3(float a, float b, float c);
void vofaSend4(float a, float b, float c, float d);

#ifdef __cplusplus
}
#endif

#endif
