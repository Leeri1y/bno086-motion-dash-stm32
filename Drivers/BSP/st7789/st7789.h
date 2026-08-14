/*
 * st7789.h
 * Minimal ST7789 240x240 SPI driver for BNO086 MotionDash (STM32F407).
 * Reproduces the exact drawing/text API surface used by the original
 * Adafruit_GFX + Adafruit_ST7789 based code (single built-in 5x7 font).
 */
#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stddef.h>

class St7789 {
public:
    St7789() : _w(240), _h(240), _rotation(0), _fg(0xFFFF), _textSize(1), _curX(0), _curY(0) {}

    void init(uint16_t w = 240, uint16_t h = 240);
    void setRotation(uint8_t r);

    void fillScreen(uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    void setFont(const void *f) { (void)f; }   // single built-in font only
    void setTextColor(uint16_t fg) { _fg = fg; }
    void setTextSize(uint8_t s) { _textSize = (s > 0) ? s : 1; }
    void setCursor(int16_t x, int16_t y) { _curX = x; _curY = y; }
    void print(const char *s);

    // Mirrors Adafruit_GFX::getTextBounds for the classic 5x7 font.
    void getTextBounds(const char *str, int16_t x, int16_t y,
                       int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h);

private:
    void writeCommand(uint8_t c);
    void writeData(const uint8_t *p, uint32_t len);
    void writeData8(uint8_t d);
    void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void drawChar(int16_t x, int16_t y, uint8_t c, uint16_t color, uint8_t size);
    void csLow(); void csHigh();
    void dcLow(); void dcHigh();

    uint16_t _w, _h;
    uint8_t _rotation;
    uint16_t _fg;
    uint8_t _textSize;
    int16_t _curX, _curY;
};

#endif
