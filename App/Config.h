/*
 * Config.h
 * Global pin/page/color constants and timing for BNO086 MotionDash (port).
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "main.h"
#include "uart_console.h"

/* ------------------ Display refresh cadence ------------------ */
#define DISPLAY_REFRESH_MS   100   /* dynamic region refresh interval */
#define IMU_POLL_MAX_PER_LOOP 8    /* max IMU events per task loop */
#define IMU_STALE_MS         1500  /* stale threshold for continuous reports */
#define IMU_STALE_SLOW_MS    6000  /* stale threshold for event-type reports */
#define IMU_BEGIN_RETRY_MS   3000  /* (kept for parity with original) */
#define MENU_STATUS_REFRESH_MS 500

/* ------------------ Buttons ------------------ */
#define BTN_DEBOUNCE_MS  150
#define BTN_LONGPRESS_MS 600

/* ------------------ Pages (menu items, excludes menu/about/sleep) --------- */
enum DashPage {
  PAGE_EULER = 0,
  PAGE_ROTATION_VECTOR,
  PAGE_ACCEL_GROUP,
  PAGE_GYRO_GROUP,
  PAGE_MAGNETOMETER,
  PAGE_STEP,
  PAGE_STABILITY,
  PAGE_ACTIVITY_CLASSIFIER,
  PAGE_RAW_READINGS,
  PAGE_COUNT   /* sentinel: heartbeat/menu mode */
};

static const char *PAGE_NAMES[PAGE_COUNT] = {
  "Euler Angles", "Rotation Vector", "Accel/Lin/Grav", "Gyroscope",
  "Magnetometer", "Step Counter", "Stability", "Activity", "Raw Readings"
};

/* ------------------ Application state ------------------ */
enum AppState { APP_MENU, APP_PAGE, APP_ABOUT, APP_SLEEP };

/* ------------------ Colors (RGB565) ------------------ */
inline uint16_t cBg()      { return 0x0000; } /* background black */
inline uint16_t cTitle()   { return 0x07FF; } /* title cyan */
inline uint16_t cLabel()   { return 0x8410; } /* label grey */
inline uint16_t cValue()   { return 0xFFFF; } /* value white */
inline uint16_t cAccent()  { return 0xFFE0; } /* accent yellow */
inline uint16_t cWarn()    { return 0xF800; } /* warning red */
inline uint16_t cOk()      { return 0x07E0; } /* ok green */
inline uint16_t cBarBg()   { return 0x2104; } /* bar background */
inline uint16_t cBarFill() { return 0x04FF; } /* bar fill */
inline uint16_t cMenuSel() { return 0x2965; } /* selected row background */

#endif
