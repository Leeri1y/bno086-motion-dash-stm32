/*
 * IMUManager.cpp
 * BNO086 init, per-page report subscription, event parsing into the snapshot.
 */
#include "IMUManager.h"

#include "task.h"

#define PI 3.14159265358979323846f

bool IMUManager::begin(I2C_HandleTypeDef *hi2c, uint8_t addr,
                       GPIO_TypeDef *intPort, uint16_t intPin,
                       GPIO_TypeDef *rstPort, uint16_t rstPin) {
    _mutex = xSemaphoreCreateMutex();

    dbg("IMU", "initializing BNO086...");
    if (!_imu.begin(hi2c, addr, intPort, intPin, rstPort, rstPin)) {
        _connected = false;
        dbg("IMU", "begin() failed, device not detected");
        return false;
    }
    _connected = true;
    dbg("IMU", "BNO086 init OK");
    return true;
}

void IMUManager::disableAllOptional() {
    const sh2_SensorId_t ids[] = {
        SENSOR_REPORTID_ROTATION_VECTOR,
        SENSOR_REPORTID_ACCELEROMETER,
        SENSOR_REPORTID_LINEAR_ACCELERATION,
        SENSOR_REPORTID_GRAVITY,
        SENSOR_REPORTID_GYROSCOPE_CALIBRATED,
        SENSOR_REPORTID_UNCALIBRATED_GYRO,
        SENSOR_REPORTID_MAGNETIC_FIELD,
        SENSOR_REPORTID_STEP_COUNTER,
        SENSOR_REPORTID_STABILITY_CLASSIFIER,
        SENSOR_REPORTID_PERSONAL_ACTIVITY_CLASSIFIER,
        SENSOR_REPORTID_RAW_ACCELEROMETER,
        SENSOR_REPORTID_RAW_GYROSCOPE,
        SENSOR_REPORTID_RAW_MAGNETOMETER,
    };
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        _imu.enableReport(ids[i], 0); /* interval 0 = disable */
        HAL_Delay(2);
    }
}

void IMUManager::enableForPage(DashPage page) {
    if (page == _currentEnabledPage) return;
    disableAllOptional();
    dbg("IMU", "switch report subscription -> %d", (int)page);

    switch (page) {
        case PAGE_COUNT: /* heartbeat */
            _imu.enableRotationVector(200);
            break;
        case PAGE_EULER:
        case PAGE_ROTATION_VECTOR:
            _imu.enableRotationVector(20);
            break;
        case PAGE_ACCEL_GROUP:
            _imu.enableAccelerometer(50);
            HAL_Delay(2);
            _imu.enableLinearAccelerometer(50);
            HAL_Delay(2);
            _imu.enableGravity(50);
            break;
        case PAGE_GYRO_GROUP:
            _imu.enableGyro(20);
            HAL_Delay(2);
            _imu.enableUncalibratedGyro(20);
            break;
        case PAGE_MAGNETOMETER:
            _imu.enableMagnetometer(50);
            break;
        case PAGE_STEP:
            _imu.enableStepCounter(200);
            break;
        case PAGE_STABILITY:
            _imu.enableStabilityClassifier(200);
            break;
        case PAGE_ACTIVITY_CLASSIFIER:
            _imu.enableActivityClassifier(200, 0x1F);
            break;
        case PAGE_RAW_READINGS:
            _imu.enableAccelerometer(50);
            HAL_Delay(2);
            _imu.enableRawAccelerometer(50);
            HAL_Delay(2);
            _imu.enableGyro(50);
            HAL_Delay(2);
            _imu.enableRawGyro(50);
            HAL_Delay(2);
            _imu.enableMagnetometer(50);
            HAL_Delay(2);
            _imu.enableRawMagnetometer(50);
            break;
        default:
            break;
    }
    _currentEnabledPage = page;
}

void IMUManager::forceResubscribe(DashPage page) {
    dbg("IMU", "report stale, force resubscribe");
    _currentEnabledPage = (DashPage)255; /* break "same page skip" */
    enableForPage(page);
}

bool IMUManager::poll() {
    if (_imu.wasReset()) {
        if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
        _snap.wasReset = true;
        _snap.resetReason = _imu.getResetReason();
        if (_mutex) xSemaphoreGive(_mutex);
        dbg("IMU", "BNO086 reset (code %u), resubscribing", _snap.resetReason);
        DashPage p = _currentEnabledPage;
        _currentEnabledPage = (DashPage)255;
        enableForPage(p);
    }
    if (!_imu.getSensorEvent()) return false;

    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
    handleEvent();
    _snap.lastUpdateMs = HAL_GetTick();
    if (_mutex) xSemaphoreGive(_mutex);
    return true;
}

void IMUManager::handleEvent() {
    uint8_t id = _imu.getSensorEventID();
    switch (id) {
        case SENSOR_REPORTID_ROTATION_VECTOR:
            _snap.qI = _imu.getQuatI();
            _snap.qJ = _imu.getQuatJ();
            _snap.qK = _imu.getQuatK();
            _snap.qReal = _imu.getQuatReal();
            _snap.qRadAcc = _imu.getQuatRadianAccuracy();
            _snap.qAccuracy = _imu.getQuatAccuracy();
            _snap.roll  = _imu.getRoll()  * 180.0f / PI;
            _snap.pitch = _imu.getPitch() * 180.0f / PI;
            _snap.yaw   = _imu.getYaw()   * 180.0f / PI;
            _rotVecFresh = true;
            break;

        case SENSOR_REPORTID_ACCELEROMETER:
            _snap.accX = _imu.getAccelX();
            _snap.accY = _imu.getAccelY();
            _snap.accZ = _imu.getAccelZ();
            _snap.accAcc = _imu.getAccelAccuracy();
            break;

        case SENSOR_REPORTID_LINEAR_ACCELERATION:
            _snap.linX = _imu.getLinAccelX();
            _snap.linY = _imu.getLinAccelY();
            _snap.linZ = _imu.getLinAccelZ();
            _snap.linAcc = _imu.getLinAccelAccuracy();
            break;

        case SENSOR_REPORTID_GRAVITY:
            _snap.gravX = _imu.getGravityX();
            _snap.gravY = _imu.getGravityY();
            _snap.gravZ = _imu.getGravityZ();
            _snap.gravAcc = _imu.getGravityAccuracy();
            break;

        case SENSOR_REPORTID_GYROSCOPE_CALIBRATED:
            _snap.gyroX = _imu.getGyroX();
            _snap.gyroY = _imu.getGyroY();
            _snap.gyroZ = _imu.getGyroZ();
            _snap.gyroAcc = _imu.getGyroAccuracy();
            break;

        case SENSOR_REPORTID_UNCALIBRATED_GYRO:
            _snap.ugyroX = _imu.getUncalibratedGyroX();
            _snap.ugyroY = _imu.getUncalibratedGyroY();
            _snap.ugyroZ = _imu.getUncalibratedGyroZ();
            _snap.biasX = _imu.getUncalibratedGyroBiasX();
            _snap.biasY = _imu.getUncalibratedGyroBiasY();
            _snap.biasZ = _imu.getUncalibratedGyroBiasZ();
            break;

        case SENSOR_REPORTID_MAGNETIC_FIELD:
            _snap.magX = _imu.getMagX();
            _snap.magY = _imu.getMagY();
            _snap.magZ = _imu.getMagZ();
            _snap.magAcc = _imu.getMagAccuracy();
            break;

        case SENSOR_REPORTID_STEP_COUNTER: {
            uint16_t newCount = _imu.getStepCount();
            int32_t delta = (int32_t)newCount - (int32_t)_lastStepCount;
            if (delta > 50 || delta < -50) {
                dbg("IMU", "step count jump (%u->%u), discard", _lastStepCount, newCount);
            } else {
                if (newCount != _lastStepCount)
                    dbg("IMU", "step count %u -> %u", _lastStepCount, newCount);
                _lastStepCount = newCount;
                _snap.stepCount = newCount;
            }
            break;
        }

        case SENSOR_REPORTID_STABILITY_CLASSIFIER:
            _snap.stability = _imu.getStabilityClassifier();
            break;

        case SENSOR_REPORTID_PERSONAL_ACTIVITY_CLASSIFIER:
            _snap.activity = _imu.getActivityClassifier();
            for (uint8_t i = 0; i < 9; i++)
                _snap.activityConfidence[i] = _imu.getActivityConfidence(i);
            break;

        case SENSOR_REPORTID_RAW_ACCELEROMETER:
            _snap.rawAX = _imu.getRawAccelX();
            _snap.rawAY = _imu.getRawAccelY();
            _snap.rawAZ = _imu.getRawAccelZ();
            break;

        case SENSOR_REPORTID_RAW_GYROSCOPE:
            _snap.rawGX = _imu.getRawGyroX();
            _snap.rawGY = _imu.getRawGyroY();
            _snap.rawGZ = _imu.getRawGyroZ();
            break;

        case SENSOR_REPORTID_RAW_MAGNETOMETER:
            _snap.rawMX = _imu.getRawMagX();
            _snap.rawMY = _imu.getRawMagY();
            _snap.rawMZ = _imu.getRawMagZ();
            break;

        default:
            break;
    }
}

bool IMUManager::consumeRotationVectorFresh() {
    bool fresh = false;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        fresh = _rotVecFresh;
        _rotVecFresh = false;
        xSemaphoreGive(_mutex);
    }
    return fresh;
}

void IMUManager::getSnapshot(IMUSnapshot &out) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        out = _snap;
        xSemaphoreGive(_mutex);
    } else {
        out = _snap;
    }
}

uint32_t IMUManager::msSinceLastUpdate() {
    uint32_t last;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        last = _snap.lastUpdateMs;
        xSemaphoreGive(_mutex);
    } else {
        last = _snap.lastUpdateMs;
    }
    return HAL_GetTick() - last;
}

void IMUManager::tare() {
    _imu.tareNow(true, SH2_TARE_BASIS_ROTATION_VECTOR);
    dbg("IMU", "tare done (heading zeroed)");
}

void IMUManager::sleep() {
    _imu.modeSleep();
    dbg("IMU", "BNO086 sleep (chip side)");
}

void IMUManager::wake() {
    _imu.modeOn();
    dbg("IMU", "BNO086 wake");
}

bool IMUManager::attemptBusRecovery() {
    dbg("I2C", "bus recovery (manual SCL pulses)...");
    bool ok = bno_i2c_recover() != 0;
    bno_i2c_clear_error();
    dbg("I2C", ok ? "bus recovered" : "bus still abnormal");
    return ok;
}
