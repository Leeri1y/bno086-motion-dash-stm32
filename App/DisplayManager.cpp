/*
 * DisplayManager.cpp
 * Port of the original display code (menu/about/sleep/error + 9 data pages).
 * Text caches use fixed char buffers; layout metrics identical to Adafruit.
 */
#include "DisplayManager.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265358979323846f

/* Activity names: order matches the BNO086 classifier (index 0..8). */
static const char *ACT_NAMES[9] = {
  "Unknown","InVehicle","OnBicycle","OnFoot","Still",
  "Tilting","Walking","Running","OnStairs"
};

DisplayManager::DisplayManager()
    : _rowCount(0), _barCount(0), _connCache(false), _connCacheValid(false), _stabilityCache(-1) {
    _badgeCache[0] = '\x01'; _badgeCache[1] = 0;
    for (int i = 0; i < DM_MAX_ROWS; i++) {
        _cache[i][0] = '\x01'; _cache[i][1] = 0;
        _barValueCache[i][0] = '\x01'; _barValueCache[i][1] = 0;
        _barPercentCache[i] = -1;
    }
}

void DisplayManager::begin() {
    _tft.init(240, 240);
    _tft.setRotation(3);
    _tft.fillScreen(cBg());
}

int DisplayManager::clampPercent(int p) {
    if (p < 0) return 0;
    if (p > 100) return 100;
    return p;
}

const char *DisplayManager::f1(float v) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%6.1f", v);
    return buf;
}

const char *DisplayManager::f2(float v) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%6.2f", v);
    return buf;
}

int DisplayManager::pctMap(float v) {
    return (int)(((v + 19.6f) / 39.2f) * 100.0f);
}

void DisplayManager::printTitle(const char *title, uint8_t pageIdx, uint8_t pageCount, bool connected) {
    _tft.setFont(NULL);
    _tft.fillRect(0, 0, 240, 30, cAccent());
    _tft.setTextColor(cBg());
    _tft.setTextSize(2);
    _tft.setCursor(8, 7);
    _tft.print(title);

    if (pageCount > 0) {
        char idxBuf[8];
        snprintf(idxBuf, sizeof(idxBuf), "%d/%d", pageIdx + 1, pageCount);
        _tft.setTextSize(1);
        _tft.setCursor(178, 11);
        _tft.print(idxBuf);
    }

    _connCacheValid = false;
    _tft.fillCircle(226, 15, 5, connected ? cOk() : cWarn());
    _connCache = connected;
    _connCacheValid = true;
}

void DisplayManager::drawStabilityRing(uint8_t stability) {
    if ((int)stability == _stabilityCache) return;
    _stabilityCache = stability;

    const int cx = 120, cy = 120, innerR = 55, outerR = 65;
    const char *segNames[4]  = {"TABLE", "STILL", "STABLE", "MOTION"};
    const uint16_t segColor[4] = {0xF800, 0x8410, 0x07E0, 0xFFE0};
    const char *stabNames[5] = {"UNKNOWN","ON TABLE","STATIONARY","STABLE","IN MOTION"};

    uint8_t s = (stability < 5) ? stability : 0;

    for (int r = innerR; r <= outerR; r++) {
        for (int deg = 0; deg < 360; deg += 3) {
            float a = deg * PI / 180.0f;
            _tft.drawPixel((int16_t)(cx + r * cosf(a)), (int16_t)(cy + r * sinf(a)), cBarBg());
        }
    }

    if (s >= 1) {
        int segIdx = s - 1;
        int startDeg = -90 + segIdx * 90;
        for (int r = innerR; r <= outerR; r++) {
            for (int deg = startDeg; deg < startDeg + 90; deg += 3) {
                float a = deg * PI / 180.0f;
                _tft.drawPixel((int16_t)(cx + r * cosf(a)), (int16_t)(cy + r * sinf(a)), segColor[segIdx]);
            }
        }
    }

    _tft.fillCircle(cx, cy, innerR - 4, cBg());
    _tft.setTextSize(2);
    _tft.setTextColor(s >= 1 ? segColor[s - 1] : cLabel());
    int16_t x1, y1; uint16_t tw, th;
    _tft.getTextBounds(stabNames[s], 0, 0, &x1, &y1, &tw, &th);
    _tft.setCursor(cx - (int)tw / 2, cy - (int)th / 2 - y1);
    _tft.print(stabNames[s]);

    _tft.setTextSize(1);
    _tft.setTextColor(cLabel());
    _tft.fillRect(0, 210, 240, 14, cBg());
    int labelX = 12;
    for (int i = 0; i < 4; i++) {
        _tft.setTextColor((s >= 1 && i == s - 1) ? segColor[i] : cLabel());
        _tft.setCursor(labelX, 212);
        _tft.print(segNames[i]);
        labelX += 58;
    }
}

void DisplayManager::printBadge(int x, int y, int w, int h, const char *text, uint16_t bg, uint16_t fg) {
    if (strcmp(_badgeCache, text) == 0) return;
    _tft.fillRect(x, y, w, h, bg);
    _tft.setTextColor(fg);
    _tft.setTextSize(2);
    int16_t x1, y1; uint16_t tw, th;
    _tft.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
    _tft.setCursor(x + (w - (int)tw) / 2, y + (h - (int)th) / 2 - y1);
    _tft.print(text);
    strncpy(_badgeCache, text, sizeof(_badgeCache) - 1);
    _badgeCache[sizeof(_badgeCache) - 1] = 0;
}

/* ---------------- menu ---------------- */
void DisplayManager::drawMenu(int selectedIndex, bool connected) {
    _tft.fillScreen(cBg());
    printTitle("BNO086 MENU", 0, 0, connected);

    const int listTop = 32, rowH = 24, visibleRows = 7;
    const int trackX = 230, trackW = 6;

    int shown = (PAGE_COUNT < visibleRows) ? PAGE_COUNT : visibleRows;

    int scrollOffset = 0;
    if (selectedIndex >= shown) scrollOffset = selectedIndex - shown + 1;
    int maxScroll = PAGE_COUNT - shown;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;

    for (int row = 0; row < shown; row++) {
        int i = scrollOffset + row;
        int y = listTop + row * rowH;
        uint16_t bg = (i == selectedIndex) ? cMenuSel() : cBg();
        _tft.fillRect(0, y, 224, rowH, bg);
        _tft.setTextSize(2);
        _tft.setTextColor((i == selectedIndex) ? cAccent() : cValue());
        _tft.setCursor(10, y + 4);
        _tft.print(PAGE_NAMES[i]);
    }

    int listH = shown * rowH;
    _tft.fillRect(trackX, listTop, trackW, listH, cBarBg());
    int indicatorH = listH / PAGE_COUNT;
    if (indicatorH < 8) indicatorH = 8;
    int travel = listH - indicatorH;
    int indicatorY = listTop;
    if (PAGE_COUNT > 1) {
        indicatorY = listTop + (int)((long)selectedIndex * travel / (PAGE_COUNT - 1));
    }
    _tft.fillRect(trackX, indicatorY, trackW, indicatorH, cAccent());

    _tft.setTextSize(1);
    _tft.setTextColor(cLabel());
    _tft.setCursor(8, 226);
    _tft.print("SHORT:select  LONG:enter/about");
}

void DisplayManager::updateMenuConnStatus(bool connected) {
    if (_connCacheValid && _connCache == connected) return;
    _tft.fillCircle(226, 15, 5, connected ? cOk() : cWarn());
    _connCache = connected;
    _connCacheValid = true;
}

/* ---------------- about ---------------- */
void DisplayManager::drawAbout() {
    _tft.fillScreen(cBg());
    printTitle("ABOUT", 0, 0, _connCache);

    _tft.setTextSize(2);
    _tft.setTextColor(cAccent());
    _tft.setCursor(10, 40);
    _tft.print("BNO086 MotionDash");

    _tft.setTextSize(1);
    _tft.setTextColor(cValue());
    const char *lines[] = {
        "",
        "IMU: BNO086 (SH2/SHTP)",
        "MCU: STM32F407VET6",
        "Disp: ST7789 240x240 SPI",
        "",
        "NEXT short: menu next /",
        "  page cycle",
        "NEXT long : sleep toggle",
        "ACT  short: enter / tare",
        "ACT  long : back / about",
    };
    int y = 66;
    for (unsigned i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        _tft.setCursor(10, y);
        _tft.print(lines[i]);
        y += 14;
    }

    _tft.setTextColor(cLabel());
    _tft.setCursor(10, 228);
    _tft.print("Press any key to return");
}

/* ---------------- sleep ---------------- */
void DisplayManager::sleepScreen() {
    _tft.fillScreen(cBg());
}

/* ---------------- init error ---------------- */
void DisplayManager::drawInitError(const char *msg) {
    _tft.fillScreen(cWarn());
    _tft.setTextColor(cBg());
    _tft.setTextSize(2);
    _tft.setCursor(10, 20);
    _tft.print("INIT FAILED");

    _tft.setTextSize(1);
    _tft.setCursor(10, 60);
    _tft.print(msg);

    _tft.setCursor(10, 90);
    _tft.print("Check:");
    _tft.setCursor(10, 106);
    _tft.print("1) I2C address 0x4B/0x4A");
    _tft.setCursor(10, 122);
    _tft.print("2) SDA/SCL wiring (PB7/PB6)");
    _tft.setCursor(10, 138);
    _tft.print("3) Power cycle BNO086");
    _tft.setCursor(10, 154);
    _tft.print("   if I2C bus is stuck");

    _tft.setCursor(10, 190);
    _tft.print("See Serial log for details");
}

/* ---------------- text rows ---------------- */
void DisplayManager::layoutTextRows(const char *labels[], uint8_t count, int startY, int rowH) {
    _rowCount = (count > DM_MAX_ROWS) ? DM_MAX_ROWS : count;
    _tft.setTextSize(2);
    _tft.setTextColor(cLabel());
    for (uint8_t i = 0; i < _rowCount; i++) {
        _rowY[i] = startY + i * rowH;
        _tft.setCursor(8, _rowY[i]);
        _tft.print(labels[i]);
        _cache[i][0] = '\x01'; _cache[i][1] = 0;
    }
}

void DisplayManager::printRow(uint8_t idx, const char *text, uint16_t color) {
    if (idx >= _rowCount) return;
    if (strcmp(_cache[idx], text) == 0) return;
    _tft.setFont(NULL);
    _tft.fillRect(126, _rowY[idx], 108, 18, cBg());
    _tft.setTextSize(2);
    _tft.setTextColor(color);
    _tft.setCursor(126, _rowY[idx]);
    _tft.print(text);
    strncpy(_cache[idx], text, sizeof(_cache[idx]) - 1);
    _cache[idx][sizeof(_cache[idx]) - 1] = 0;
}

/* ---------------- bar rows ---------------- */
void DisplayManager::layoutBarRows(const char *labels[], uint8_t count, int startY, int rowH, bool big) {
    _barCount = (count > DM_MAX_ROWS) ? DM_MAX_ROWS : count;
    _tft.setTextSize(1);
    _tft.setTextColor(cLabel());
    for (uint8_t i = 0; i < _barCount; i++) {
        _barY[i] = startY + i * rowH;
        _tft.setCursor(8, _barY[i]);
        _tft.print(labels[i]);
        if (big) {
            _tft.drawRect(8, _barY[i] + 50, 224, 10, cBarBg());
        } else {
            _tft.drawRect(8, _barY[i] + 11, 126, 6, cBarBg());
        }
        _barPercentCache[i] = -1;
        _barValueCache[i][0] = '\x01'; _barValueCache[i][1] = 0;
    }
}

void DisplayManager::printBarRow(uint8_t idx, const char *valueText, int percent, uint16_t color, bool big) {
    if (idx >= _barCount) return;
    percent = clampPercent(percent);
    bool textChanged = (strcmp(_barValueCache[idx], valueText) != 0);
    bool percentChanged = (_barPercentCache[idx] != percent);
    if (!textChanged && !percentChanged) return;

    if (big) {
        if (textChanged) {
            _tft.fillRect(8, _barY[idx] + 12, 224, 34, cBg());
            _tft.setTextSize(4);
            _tft.setTextColor(color);
            _tft.setCursor(8, _barY[idx] + 12);
            _tft.print(valueText);
            strncpy(_barValueCache[idx], valueText, sizeof(_barValueCache[idx]) - 1);
            _barValueCache[idx][sizeof(_barValueCache[idx]) - 1] = 0;
        }
        if (percentChanged) {
            int bx = 8, by = _barY[idx] + 50, bw = 224, bh = 10;
            _tft.fillRect(bx + 1, by + 1, bw - 2, bh - 2, cBg());
            int fillW = (int)((bw - 2) * (percent / 100.0f));
            if (fillW > 0) _tft.fillRect(bx + 1, by + 1, fillW, bh - 2, color);
            _barPercentCache[idx] = percent;
        }
    } else {
        if (textChanged) {
            _tft.fillRect(140, _barY[idx] - 1, 90, 10, cBg());
            _tft.setTextSize(1);
            _tft.setTextColor(color);
            _tft.setCursor(140, _barY[idx]);
            _tft.print(valueText);
            strncpy(_barValueCache[idx], valueText, sizeof(_barValueCache[idx]) - 1);
            _barValueCache[idx][sizeof(_barValueCache[idx]) - 1] = 0;
        }
        if (percentChanged) {
            int bx = 8, by = _barY[idx] + 11, bw = 126, bh = 6;
            _tft.fillRect(bx + 1, by + 1, bw - 2, bh - 2, cBg());
            int fillW = (int)((bw - 2) * (percent / 100.0f));
            if (fillW > 0) _tft.fillRect(bx + 1, by + 1, fillW, bh - 2, color);
            _barPercentCache[idx] = percent;
        }
    }
}

/* ---------------- static page layout ---------------- */
void DisplayManager::drawStatic(DashPage page) {
    _tft.fillScreen(cBg());
    printTitle(PAGE_NAMES[page], page, PAGE_COUNT, _connCache);
    _badgeCache[0] = '\x01'; _badgeCache[1] = 0;
    _stabilityCache = -1;
    _rowCount = 0;
    _barCount = 0;

    switch (page) {
        case PAGE_ROTATION_VECTOR: {
            const char *labels[] = {"I:", "J:", "K:", "Real:", "AccRad:", "AccLvl:"};
            layoutTextRows(labels, 6);
            break;
        }
        case PAGE_EULER: {
            const char *labels[] = {"ROLL", "PITCH", "YAW"};
            layoutBarRows(labels, 3, 34, 66, true);
            break;
        }
        case PAGE_ACCEL_GROUP: {
            const char *labels[] = {"AccX","AccY","AccZ","LinX","LinY","LinZ","GravX","GravY","GravZ"};
            layoutBarRows(labels, 9, 34, 22, false);
            break;
        }
        case PAGE_GYRO_GROUP: {
            const char *labels[] = {"GyroX:","GyroY:","GyroZ:","BiasX:","BiasY:","BiasZ:"};
            layoutTextRows(labels, 6);
            break;
        }
        case PAGE_MAGNETOMETER: {
            const char *labels[] = {"MagX:","MagY:","MagZ:","AccLvl:"};
            layoutTextRows(labels, 4);
            break;
        }
        case PAGE_STEP: {
            const char *labels[] = {"STEPS (goal 10000)"};
            layoutBarRows(labels, 1, 90, 90, true);
            break;
        }
        case PAGE_STABILITY:
            break; /* ring drawn in drawDynamic */
        case PAGE_ACTIVITY_CLASSIFIER:
            layoutBarRows(ACT_NAMES, 9, 34, 22, false);
            break;
        case PAGE_RAW_READINGS: {
            const char *labels[] = {"rAX:","rAY:","rAZ:","rGX:","rGY:","rGZ:","rMX:","rMY:","rMZ:"};
            layoutTextRows(labels, 9, 40, 22);
            break;
        }
        default: break;
    }
}

/* ---------------- dynamic values ---------------- */
void DisplayManager::drawDynamic(DashPage page, const IMUSnapshot &d, bool imuConnected) {
    updateMenuConnStatus(imuConnected);

    switch (page) {
        case PAGE_ROTATION_VECTOR: {
            char buf[8];
            printRow(0, f2(d.qI), cValue());
            printRow(1, f2(d.qJ), cValue());
            printRow(2, f2(d.qK), cValue());
            printRow(3, f2(d.qReal), cValue());
            printRow(4, f2(d.qRadAcc), cValue());
            snprintf(buf, sizeof(buf), "%u", d.qAccuracy);
            printRow(5, buf, cAccent());
            break;
        }
        case PAGE_EULER: {
            int pRoll  = (int)(((d.roll  + 180.0f) / 360.0f) * 100.0f);
            int pPitch = (int)(((d.pitch + 180.0f) / 360.0f) * 100.0f);
            int pYaw   = (int)(((d.yaw   + 180.0f) / 360.0f) * 100.0f);
            printBarRow(0, f1(d.roll),  pRoll,  cValue(),  true);
            printBarRow(1, f1(d.pitch), pPitch, cValue(),  true);
            printBarRow(2, f1(d.yaw),   pYaw,   cAccent(), true);
            break;
        }
        case PAGE_ACCEL_GROUP:
            printBarRow(0, f1(d.accX),  pctMap(d.accX),  cBarFill(), false);
            printBarRow(1, f1(d.accY),  pctMap(d.accY),  cBarFill(), false);
            printBarRow(2, f1(d.accZ),  pctMap(d.accZ),  cBarFill(), false);
            printBarRow(3, f1(d.linX),  pctMap(d.linX),  cAccent(),  false);
            printBarRow(4, f1(d.linY),  pctMap(d.linY),  cAccent(),  false);
            printBarRow(5, f1(d.linZ),  pctMap(d.linZ),  cAccent(),  false);
            printBarRow(6, f1(d.gravX), pctMap(d.gravX), cOk(),      false);
            printBarRow(7, f1(d.gravY), pctMap(d.gravY), cOk(),      false);
            printBarRow(8, f1(d.gravZ), pctMap(d.gravZ), cOk(),      false);
            break;
        case PAGE_GYRO_GROUP:
            printRow(0, f2(d.gyroX), cValue());
            printRow(1, f2(d.gyroY), cValue());
            printRow(2, f2(d.gyroZ), cValue());
            printRow(3, f2(d.biasX), cValue());
            printRow(4, f2(d.biasY), cValue());
            printRow(5, f2(d.biasZ), cValue());
            break;
        case PAGE_MAGNETOMETER: {
            char buf[8];
            printRow(0, f1(d.magX), cValue());
            printRow(1, f1(d.magY), cValue());
            printRow(2, f1(d.magZ), cValue());
            snprintf(buf, sizeof(buf), "%u", d.magAcc);
            printRow(3, buf, cAccent());
            break;
        }
        case PAGE_STEP: {
            char buf[8];
            int stepPct = (int)(d.stepCount / 100.0f);
            snprintf(buf, sizeof(buf), "%u", d.stepCount);
            printBarRow(0, buf, stepPct, cValue(), true);
            break;
        }
        case PAGE_STABILITY:
            drawStabilityRing(d.stability);
            break;
        case PAGE_ACTIVITY_CLASSIFIER:
            for (uint8_t i = 0; i < 9; i++) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%3d%%", d.activityConfidence[i]);
                uint16_t color = (d.activity == i) ? cAccent() : cBarFill();
                printBarRow(i, buf, d.activityConfidence[i], color, false);
            }
            break;
        case PAGE_RAW_READINGS: {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", d.rawAX); printRow(0, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawAY); printRow(1, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawAZ); printRow(2, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawGX); printRow(3, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawGY); printRow(4, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawGZ); printRow(5, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawMX); printRow(6, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawMY); printRow(7, buf, cValue());
            snprintf(buf, sizeof(buf), "%d", d.rawMZ); printRow(8, buf, cValue());
            break;
        }
        default: break;
    }
}
