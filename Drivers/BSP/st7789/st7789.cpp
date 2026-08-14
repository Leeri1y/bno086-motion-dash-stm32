/*
 * st7789.cpp
 * ST7789 240x240 SPI driver (HAL), text metrics mirroring Adafruit_GFX's
 * classic 5x7 font behaviour so the original layout reproduces exactly.
 */
#include "st7789.h"
#include "font5x7.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_INVON   0x21
#define ST7789_NORON   0x13
#define ST7789_DISPON  0x29
#define ST7789_RAMWR   0x2C

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_RGB 0x00

void St7789::csLow()  { HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET); }
void St7789::csHigh() { HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET); }
void St7789::dcLow()  { HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET); }
void St7789::dcHigh() { HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET); }

void St7789::writeCommand(uint8_t c) {
    csLow(); dcLow();
    HAL_SPI_Transmit(&hspi1, &c, 1, HAL_MAX_DELAY);
    csHigh();
}

void St7789::writeData(const uint8_t *p, uint32_t len) {
    csLow(); dcHigh();
    while (len > 0) {
        uint16_t chunk = (len > 65535U) ? 65535U : (uint16_t)len;
        HAL_SPI_Transmit(&hspi1, (uint8_t *)p, chunk, HAL_MAX_DELAY);
        p += chunk;
        len -= chunk;
    }
    csHigh();
}

void St7789::writeData8(uint8_t d) {
    csLow(); dcHigh();
    HAL_SPI_Transmit(&hspi1, &d, 1, HAL_MAX_DELAY);
    csHigh();
}

void St7789::init(uint16_t w, uint16_t h) {
    _w = w; _h = h;

    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(150);

    writeCommand(ST7789_SWRESET); HAL_Delay(150);
    writeCommand(ST7789_SLPOUT);  HAL_Delay(500);
    writeCommand(ST7789_COLMOD);  writeData8(0x55); HAL_Delay(10);
    writeCommand(ST7789_MADCTL);  writeData8(0x00);
    {
        uint8_t d[4] = {0x00, 0x00, 0x00, 0xF0};
        writeCommand(ST7789_CASET); writeData(d, 4);
        writeCommand(ST7789_RASET); writeData(d, 4);
    }
    writeCommand(ST7789_INVON);
    writeCommand(ST7789_NORON);  HAL_Delay(10);
    writeCommand(ST7789_DISPON); HAL_Delay(500);

    HAL_GPIO_WritePin(TFT_BL_GPIO_Port, TFT_BL_Pin, GPIO_PIN_SET); /* backlight on */

    setRotation(3);
}

void St7789::setRotation(uint8_t r) {
    _rotation = r & 3;
    uint8_t madctl;
    switch (_rotation) {
        case 0:  madctl = MADCTL_MX | MADCTL_MY | MADCTL_RGB; break;
        case 1:  madctl = MADCTL_MY | MADCTL_MV | MADCTL_RGB; break;
        case 2:  madctl = MADCTL_RGB; break;
        default: madctl = MADCTL_MX | MADCTL_MV | MADCTL_RGB; break;
    }
    writeCommand(ST7789_MADCTL); writeData8(madctl);
}

void St7789::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t d[4];
    writeCommand(ST7789_CASET);
    d[0] = x0 >> 8; d[1] = x0 & 0xFF; d[2] = x1 >> 8; d[3] = x1 & 0xFF;
    writeData(d, 4);
    writeCommand(ST7789_RASET);
    d[0] = y0 >> 8; d[1] = y0 & 0xFF; d[2] = y1 >> 8; d[3] = y1 & 0xFF;
    writeData(d, 4);
    writeCommand(ST7789_RAMWR);
}

void St7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((x + w) > (int16_t)_w) w = _w - x;
    if ((y + h) > (int16_t)_h) h = _h - y;
    if (w <= 0 || h <= 0) return;

    setAddrWindow((uint16_t)x, (uint16_t)y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

    uint32_t npix = (uint32_t)w * (uint32_t)h;
    uint8_t buf[64];
    for (int i = 0; i < 64; i += 2) { buf[i] = color >> 8; buf[i + 1] = color & 0xFF; }

    csLow(); dcHigh();
    while (npix >= 32) {
        HAL_SPI_Transmit(&hspi1, buf, 64, HAL_MAX_DELAY);
        npix -= 32;
    }
    while (npix > 0) {
        HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
        npix--;
    }
    csHigh();
}

void St7789::fillScreen(uint16_t color) {
    fillRect(0, 0, _w, _h, color);
}

void St7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= (int16_t)_w || y >= (int16_t)_h) return;
    setAddrWindow((uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y);
    uint8_t d[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    writeData(d, 2);
}

void St7789::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    fillRect(x, y, 1, h, color);
    fillRect(x + w - 1, y, 1, h, color);
}

void St7789::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        fillRect(x0 - x, y0 - y, 2 * x + 1, 1, color);
        fillRect(x0 - y, y0 - x, 2 * y + 1, 1, color);
        fillRect(x0 - y, y0 + x, 2 * y + 1, 1, color);
        fillRect(x0 - x, y0 + y, 2 * x + 1, 1, color);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else       { d = d + 4 * x + 6; }
    }
}

void St7789::drawChar(int16_t x, int16_t y, uint8_t c, uint16_t color, uint8_t size) {
    if ((x >= (int16_t)_w) || (y >= (int16_t)_h) ||
        ((x + 6 * size - 1) < 0) || ((y + 8 * size - 1) < 0))
        return;

    const uint8_t *glyph = &font5x7[c * FONT5X7_BYTES];
    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (int8_t j = 0; j < 8; j++, line >>= 1) {
            if (line & 1) {
                if (size == 1)
                    drawPixel(x + i, y + j, color);
                else
                    fillRect(x + i * size, y + j * size, size, size, color);
            }
        }
    }
}

void St7789::print(const char *s) {
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        if (c == '\n') {
            _curX = 0;
            _curY += (int16_t)(_textSize * 8);
        } else if (c != '\r') {
            if ((_curX + _textSize * 6) > (int16_t)_w) { /* wrap */
                _curX = 0;
                _curY += (int16_t)(_textSize * 8);
            }
            drawChar(_curX, _curY, c, _fg, _textSize);
            _curX += (int16_t)(_textSize * 6);
        }
    }
}

void St7789::getTextBounds(const char *str, int16_t x, int16_t y,
                           int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) {
    int16_t minx = 32767, miny = 32767, maxx = -1, maxy = -1;
    int16_t cx = x, cy = y;
    *x1 = x; *y1 = y; *w = 0; *h = 0;

    while (*str) {
        uint8_t c = (uint8_t)*str++;
        if (c == '\n') {
            cx = 0;
            cy += (int16_t)(_textSize * 8);
        } else if (c != '\r') {
            if ((cx + _textSize * 6) > (int16_t)_w) {
                cx = 0;
                cy += (int16_t)(_textSize * 8);
            }
            int16_t x2 = cx + _textSize * 6 - 1;
            int16_t y2 = cy + _textSize * 8 - 1;
            if (x2 > maxx) maxx = x2;
            if (y2 > maxy) maxy = y2;
            if (cx < minx) minx = cx;
            if (cy < miny) miny = cy;
            cx += (int16_t)(_textSize * 6);
        }
    }

    if (maxx >= minx) { *x1 = minx; *w = (uint16_t)(maxx - minx + 1); }
    if (maxy >= miny) { *y1 = miny; *h = (uint16_t)(maxy - miny + 1); }
}
