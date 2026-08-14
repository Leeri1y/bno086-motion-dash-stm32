/*
 * sh2_hal.c
 * SH2 HAL over STM32 HAL I2C (blocking), INT/RST via GPIO.
 * Mirrors the SparkFun BNO08x i2chal_* transport semantics.
 */
#include "bno_hal.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

#define BNO_I2C_TIMEOUT     200   /* ms per I2C transaction */
#define BNO_INT_WAIT_MAX_MS 500   /* INT assertion timeout */

static I2C_HandleTypeDef *s_i2c = NULL;
static uint8_t            s_addr = 0x4B;
static GPIO_TypeDef      *s_intPort = NULL;
static uint16_t           s_intPin = 0;
static GPIO_TypeDef      *s_rstPort = NULL;
static uint16_t           s_rstPin = 0;
static volatile uint8_t   s_i2c_error = 0;

void bno_hal_configure(I2C_HandleTypeDef *hi2c, uint8_t addr,
                       GPIO_TypeDef *intPort, uint16_t intPin,
                       GPIO_TypeDef *rstPort, uint16_t rstPin) {
    s_i2c = hi2c;
    s_addr = addr;
    s_intPort = intPort;
    s_intPin = intPin;
    s_rstPort = rstPort;
    s_rstPin = rstPin;
}

/* Delay that yields to FreeRTOS once the scheduler is running. */
static void hal_delay_ms(uint32_t ms) {
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        vTaskDelay(pdMS_TO_TICKS(ms));
    else
        HAL_Delay(ms);
}

static int int_is_low(void) {
    if (!s_intPort) return 1; /* no INT pin: assume always ready */
    return (HAL_GPIO_ReadPin(s_intPort, s_intPin) == GPIO_PIN_RESET);
}

void bno_hal_hardware_reset(void) {
    if (!s_rstPort) return;
    HAL_GPIO_WritePin(s_rstPort, s_rstPin, GPIO_PIN_SET);
    hal_delay_ms(10);
    HAL_GPIO_WritePin(s_rstPort, s_rstPin, GPIO_PIN_RESET);
    hal_delay_ms(10);
    HAL_GPIO_WritePin(s_rstPort, s_rstPin, GPIO_PIN_SET);
    hal_delay_ms(10);
}

/* Wait for INT to go LOW. 1 = ready, 0 = timeout (device reset attempted). */
static int hal_wait_for_int(void) {
    if (!s_intPort) return 1;
    uint32_t start = HAL_GetTick();
    for (;;) {
        if (int_is_low()) return 1;
        if ((HAL_GetTick() - start) >= BNO_INT_WAIT_MAX_MS) {
            s_i2c_error = 1;
            bno_hal_hardware_reset();
            return 0;
        }
        hal_delay_ms(1);
    }
}

static int bno_i2c_write_raw(const uint8_t *p, uint16_t len) {
    if (!s_i2c) return -1;
    if (HAL_I2C_Master_Transmit(s_i2c, (uint16_t)(s_addr << 1),
                                (uint8_t *)p, len, BNO_I2C_TIMEOUT) != HAL_OK) {
        s_i2c_error = 1;
        return -1;
    }
    return (int)len;
}

static int bno_i2c_read_raw(uint8_t *p, uint16_t len) {
    if (!s_i2c) return -1;
    if (HAL_I2C_Master_Receive(s_i2c, (uint16_t)(s_addr << 1),
                               p, len, BNO_I2C_TIMEOUT) != HAL_OK) {
        s_i2c_error = 1;
        return -1;
    }
    return (int)len;
}

int bno_hal_ping(void) {
    if (!s_i2c) return 0;
    return (HAL_I2C_IsDeviceReady(s_i2c, (uint16_t)(s_addr << 1), 2, 50) == HAL_OK);
}

static int sh2_hal_open(sh2_Hal_t *self) {
    (void)self;
    if (s_intPort) hal_wait_for_int();

    /* SHTP "reset" request on channel 1 (same packet as SparkFun HAL). */
    uint8_t softreset_pkt[] = {5, 0, 1, 0, 1};
    for (int attempts = 0; attempts < 5; attempts++) {
        if (bno_i2c_write_raw(softreset_pkt, sizeof(softreset_pkt)) > 0)
            return 0;
        hal_delay_ms(30);
    }
    return -1;
}

static void sh2_hal_close(sh2_Hal_t *self) {
    (void)self;
}

static int sh2_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len,
                        uint32_t *t_us) {
    (void)self;
    if (s_intPort && !hal_wait_for_int()) return 0;

    /* Peek the 4-byte SHTP header. */
    uint8_t header[4];
    if (bno_i2c_read_raw(header, 4) < 0) return 0;

    uint16_t packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    packet_size &= ~0x8000; /* clear "continue" bit */
    if (packet_size == 0 || packet_size > len) return 0;

    /* Read the full transfer in one transaction: the device re-sends the
       header at the start of each I2C read, so pBuffer[0..3]=header. */
    if (bno_i2c_read_raw(pBuffer, packet_size) < 0) return 0;

    if (t_us) *t_us = HAL_GetTick() * 1000UL;
    return (int)packet_size;
}

static int sh2_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    (void)self;
    if (s_intPort && !hal_wait_for_int()) return -1;
    if (len > 250) len = 250;
    if (bno_i2c_write_raw(pBuffer, (uint16_t)len) < 0) return -1;
    return (int)len;
}

static uint32_t sh2_hal_getTimeUs(sh2_Hal_t *self) {
    (void)self;
    return HAL_GetTick() * 1000UL;
}

void bno_hal_init(sh2_Hal_t *hal) {
    memset(hal, 0, sizeof(*hal));
    hal->open = sh2_hal_open;
    hal->close = sh2_hal_close;
    hal->read = sh2_hal_read;
    hal->write = sh2_hal_write;
    hal->getTimeUs = sh2_hal_getTimeUs;
}

/* Bus self-healing: reset I2C1, bit-bang SCL to unstick the slave, reinit.
   Hardcoded to I2C1 on PB6 (SCL) / PB7 (SDA) for this board. */
int bno_i2c_recover(void) {
    if (!s_i2c) return 0;

    HAL_I2C_DeInit(s_i2c);

    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); /* SDA high */

    for (int i = 0; i < 9; i++) {                        /* 9 SCL pulses */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    /* Restore I2C1 alternate function. */
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);

    HAL_I2C_Init(s_i2c);
    return 1;
}

uint8_t bno_i2c_get_error(void) { return s_i2c_error; }
void    bno_i2c_clear_error(void) { s_i2c_error = 0; }
