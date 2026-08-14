/*
 * DisplayManager.h
 * ST7789 display wrapper: menu / about / sleep / error / data pages.
 * String caches replaced with fixed char buffers (no Arduino String).
 */
#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "st7789.h"
#include "Config.h"
#include "IMUManager.h"

#define DM_MAX_ROWS 9

class DisplayManager {
public:
    DisplayManager();

    void begin();

    void drawMenu(int selectedIndex, bool connected);
    void updateMenuConnStatus(bool connected);
    void drawAbout();
    void sleepScreen();
    void drawInitError(const char *msg);

    void drawStatic(DashPage page);
    void drawDynamic(DashPage page, const IMUSnapshot &d, bool imuConnected);

private:
    St7789 _tft;

    uint8_t _rowCount;
    int16_t _rowY[DM_MAX_ROWS];
    char _cache[DM_MAX_ROWS][16];

    uint8_t _barCount;
    int16_t _barY[DM_MAX_ROWS];
    int _barPercentCache[DM_MAX_ROWS];
    char _barValueCache[DM_MAX_ROWS][16];

    bool _connCache;
    bool _connCacheValid;
    char _badgeCache[16];
    int _stabilityCache;

    void printTitle(const char *title, uint8_t pageIdx, uint8_t pageCount, bool connected);
    void printBadge(int x, int y, int w, int h, const char *text, uint16_t bg, uint16_t fg = 0x0000);
    void drawStabilityRing(uint8_t stability);
    void layoutTextRows(const char *labels[], uint8_t count, int startY = 46, int rowH = 24);
    void printRow(uint8_t idx, const char *text, uint16_t color);
    void layoutBarRows(const char *labels[], uint8_t count, int startY, int rowH, bool big = false);
    void printBarRow(uint8_t idx, const char *valueText, int percent, uint16_t color, bool big = false);

    static int clampPercent(int p);
    static const char *f1(float v);
    static const char *f2(float v);
    static int pctMap(float v);
};

#endif
