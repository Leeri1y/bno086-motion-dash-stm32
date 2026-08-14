/*
 * bno_hal.h
 * Platform HAL for the BNO08x SH2/SHTP driver (STM32 HAL I2C).
 */
#ifndef BNO_HAL_H
#define BNO_HAL_H

#include <stdint.h>
#include "sh2_hal.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configure the I2C bus / pins used by the SH2 HAL. */
void bno_hal_configure(I2C_HandleTypeDef *hi2c, uint8_t addr,
                       GPIO_TypeDef *intPort, uint16_t intPin,
                       GPIO_TypeDef *rstPort, uint16_t rstPin);

/* Fill a sh2_Hal_t with the function pointers. */
void bno_hal_init(sh2_Hal_t *hal);

/* Pulse the reset pin (used by BNO08x::hardwareReset). */
void bno_hal_hardware_reset(void);

/* I2C probe: 1 if the device ACKs, 0 otherwise. */
int bno_hal_ping(void);

/* I2C bus self-healing: reset I2C1 and bit-bang SCL pulses. */
int bno_i2c_recover(void);

/* I2C error flag (set on timeout/error, cleared by bno_i2c_clear_error). */
uint8_t bno_i2c_get_error(void);
void    bno_i2c_clear_error(void);

#ifdef __cplusplus
}
#endif

#endif
