/**
  ******************************************************************************
  * @file    main.h
  * @brief   Pin/peripheral definitions for BNO086 MotionDash (STM32F407VET6).
  *
  *  Pin map (BNO086 MotionDash -> STM32F407VET6):
  *    BNO086 SCL  -> PB6  (I2C1_SCL, AF4)
  *    BNO086 SDA  -> PB7  (I2C1_SDA, AF4)
  *    BNO086 INT  -> PC6  (EXTI6, input, pull-up)
  *    BNO086 RST  -> PC7  (GPIO output)
  *    ST7789 SCK  -> PA5  (SPI1_SCK, AF5)
  *    ST7789 MOSI -> PA7  (SPI1_MOSI, AF5)
  *    ST7789 CS   -> PC4  (GPIO output)
  *    ST7789 DC   -> PC5  (GPIO output)
  *    ST7789 RST  -> PA4  (GPIO output)
  *    ST7789 BL   -> PA8  (GPIO output, backlight)
  *    BTN NEXT    -> PB0  (EXTI0, input, pull-up)
  *    BTN ACT     -> PB1  (EXTI1, input, pull-up)
  *    USART1 TX   -> PA9  (AF7), USART1 RX -> PA10 (AF7)
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* Peripheral handles (generated-style, also used by the BSP drivers) */
extern I2C_HandleTypeDef  hi2c1;
extern SPI_HandleTypeDef  hspi1;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef  htim6;

#define BNO_I2C_ADDR 0x4BU

/* BNO086 INT (PC6) */
#define BNO_INT_Pin        GPIO_PIN_6
#define BNO_INT_GPIO_Port  GPIOC
#define BNO_INT_EXTI_IRQn  EXTI9_5_IRQn
/* BNO086 RST (PC7) */
#define BNO_RST_Pin        GPIO_PIN_7
#define BNO_RST_GPIO_Port  GPIOC

/* ST7789 SPI control pins */
#define TFT_CS_Pin         GPIO_PIN_4
#define TFT_CS_GPIO_Port   GPIOC
#define TFT_DC_Pin         GPIO_PIN_5
#define TFT_DC_GPIO_Port   GPIOC
#define TFT_RST_Pin        GPIO_PIN_4
#define TFT_RST_GPIO_Port  GPIOA
#define TFT_BL_Pin         GPIO_PIN_8
#define TFT_BL_GPIO_Port   GPIOA

/* Buttons */
#define BTN_NEXT_Pin       GPIO_PIN_0
#define BTN_NEXT_GPIO_Port GPIOB
#define BTN_NEXT_EXTI_IRQn EXTI0_IRQn
#define BTN_ACT_Pin        GPIO_PIN_1
#define BTN_ACT_GPIO_Port  GPIOB
#define BTN_ACT_EXTI_IRQn  EXTI1_IRQn

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
