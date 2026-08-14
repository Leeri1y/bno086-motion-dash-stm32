/*
 * IMUManager.h
 * BNO086 driver wrapper + data snapshot, page-based report subscription,
 * sleep/wake, stale self-healing. FreeRTOS mutex guards the shared snapshot
 * (IMU task writes, UI task reads).
 */
#ifndef IMU_MANAGER_H
#define IMU_MANAGER_H

#include "bno08x.h"
#include "Config.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

/* Unified snapshot: IMU task writes, display reads. */
struct IMUSnapshot {
    IMUSnapshot() { memset(this, 0, sizeof(*this)); }

    /* rotation vector / euler (degrees) */
    float qI, qJ, qK, qReal, qRadAcc;
    uint8_t qAccuracy;
    float roll, pitch, yaw;

    /* accel / linear / gravity */
    float accX, accY, accZ; uint8_t accAcc;
    float linX, linY, linZ; uint8_t linAcc;
    float gravX, gravY, gravZ; uint8_t gravAcc;

    /* gyro (calibrated + uncalibrated bias) */
    float gyroX, gyroY, gyroZ; uint8_t gyroAcc;
    float ugyroX, ugyroY, ugyroZ;
    float biasX, biasY, biasZ;

    /* magnetometer */
    float magX, magY, magZ; uint8_t magAcc;

    /* step / stability / activity */
    uint16_t stepCount;
    uint8_t stability;
    uint8_t activity;
    uint8_t activityConfidence[9];

    /* raw readings */
    int16_t rawAX, rawAY, rawAZ;
    int16_t rawGX, rawGY, rawGZ;
    int16_t rawMX, rawMY, rawMZ;

    /* system */
    bool wasReset;
    uint8_t resetReason;
    uint32_t lastUpdateMs;
};

class IMUManager {
public:
    IMUManager()
        : _mutex(NULL), _connected(false), _rotVecFresh(false),
          _currentEnabledPage((DashPage)255), _lastStepCount(0), _stepBaselineSet(false) {}

    bool begin(I2C_HandleTypeDef *hi2c, uint8_t addr,
               GPIO_TypeDef *intPort, uint16_t intPin,
               GPIO_TypeDef *rstPort, uint16_t rstPin);

    /* Subscribe reports for a page; PAGE_COUNT = heartbeat mode. */
    void enableForPage(DashPage page);
    void forceResubscribe(DashPage page);

    /* Non-blocking poll: called repeatedly by the IMU task. */
    bool poll();

    void tare();
    void sleep();
    void wake();

    bool attemptBusRecovery();
    bool i2cError() const { return bno_i2c_get_error() != 0; }

    /* Rotation-vector report "fresh since last consume" (for VOFA+). */
    bool consumeRotationVectorFresh();

    /* Thread-safe snapshot copy. */
    void getSnapshot(IMUSnapshot &out);

    uint32_t msSinceLastUpdate();
    bool isConnected() const { return _connected; }
    DashPage getEnabledPage() const { return _currentEnabledPage; }

private:
    BNO08x _imu;
    IMUSnapshot _snap;
    SemaphoreHandle_t _mutex;
    bool _connected;
    bool _rotVecFresh;
    DashPage _currentEnabledPage;
    uint16_t _lastStepCount;
    bool _stepBaselineSet;

    void disableAllOptional();
    void handleEvent();
};

#endif
