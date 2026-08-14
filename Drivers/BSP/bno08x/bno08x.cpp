/*
 * bno08x.cpp
 * BNO08x wrapper (HAL port of SparkFun BNO08x library).
 * All sensor values are decoded by the SH2 driver into `sensorValue`; the
 * getters simply read the appropriate union field (identical semantics to the
 * SparkFun library the original project used).
 */
#include "bno08x.h"

#include <math.h>

/* ---- begin / init ------------------------------------------------------ */

bool BNO08x::begin(I2C_HandleTypeDef *hi2c, uint8_t deviceAddress,
                   GPIO_TypeDef *intPort, uint16_t intPin,
                   GPIO_TypeDef *rstPort, uint16_t rstPin) {
    bno_hal_configure(hi2c, deviceAddress, intPort, intPin, rstPort, rstPin);
    bno_hal_init(&_HAL);

    if (!isConnected()) {
        _connected = false;
        return false;
    }
    _connected = true;
    return _init();
}

bool BNO08x::isConnected() {
    return bno_hal_ping() != 0;
}

void BNO08x::hardwareReset() {
    bno_hal_hardware_reset();
}

bool BNO08x::wasReset() {
    bool x = _reset_occurred;
    _reset_occurred = false;
    return x;
}

uint8_t BNO08x::getResetReason() {
    return prodIds.entry[0].resetCause;
}

bool BNO08x::_init() {
    int status;

    hardwareReset();

    status = sh2_open(&_HAL, shtpCallback, this);
    if (status != SH2_OK) return false;

    memset(&prodIds, 0, sizeof(prodIds));
    status = sh2_getProdIds(&prodIds);
    if (status != SH2_OK) return false;

    sh2_setSensorCallback(sensorCallback, this);
    return true;
}

void BNO08x::shtpCallback(void *cookie, sh2_AsyncEvent_t *pEvent) {
    BNO08x *self = (BNO08x *)cookie;
    if (pEvent->eventId == SH2_RESET) {
        self->_reset_occurred = true;
    }
}

void BNO08x::sensorCallback(void *cookie, sh2_SensorEvent_t *event) {
    BNO08x *self = (BNO08x *)cookie;
    int rc = sh2_decodeSensorEvent(&self->sensorValue, event);
    if (rc != SH2_OK) {
        self->sensorValue.timestamp = 0;
    }
}

/* ---- report enabling ---------------------------------------------------- */

bool BNO08x::enableReport(sh2_SensorId_t sensorId, uint32_t interval_us,
                          uint32_t sensorSpecific) {
    static sh2_SensorConfig_t config;
    config.changeSensitivityEnabled = false;
    config.wakeupEnabled = false;
    config.changeSensitivityRelative = false;
    config.alwaysOnEnabled = false;
    config.changeSensitivity = 0;
    config.batchInterval_us = 0;
    config.sensorSpecific = sensorSpecific;
    config.reportInterval_us = interval_us;

    int status = sh2_setSensorConfig(sensorId, &config);
    return (status == SH2_OK);
}

bool BNO08x::enableRotationVector(uint16_t t)          { return enableReport(SENSOR_REPORTID_ROTATION_VECTOR, t * 1000); }
bool BNO08x::enableAccelerometer(uint16_t t)           { return enableReport(SENSOR_REPORTID_ACCELEROMETER, t * 1000); }
bool BNO08x::enableLinearAccelerometer(uint16_t t)     { return enableReport(SENSOR_REPORTID_LINEAR_ACCELERATION, t * 1000); }
bool BNO08x::enableGravity(uint16_t t)                 { return enableReport(SENSOR_REPORTID_GRAVITY, t * 1000); }
bool BNO08x::enableGyro(uint16_t t)                    { return enableReport(SENSOR_REPORTID_GYROSCOPE_CALIBRATED, t * 1000); }
bool BNO08x::enableUncalibratedGyro(uint16_t t)        { return enableReport(SENSOR_REPORTID_UNCALIBRATED_GYRO, t * 1000); }
bool BNO08x::enableMagnetometer(uint16_t t)            { return enableReport(SENSOR_REPORTID_MAGNETIC_FIELD, t * 1000); }
bool BNO08x::enableStepCounter(uint16_t t)             { return enableReport(SENSOR_REPORTID_STEP_COUNTER, t * 1000); }
bool BNO08x::enableStabilityClassifier(uint16_t t)     { return enableReport(SENSOR_REPORTID_STABILITY_CLASSIFIER, t * 1000); }
bool BNO08x::enableActivityClassifier(uint16_t t, uint32_t mask) {
    return enableReport(SENSOR_REPORTID_PERSONAL_ACTIVITY_CLASSIFIER, t * 1000, mask);
}
bool BNO08x::enableRawAccelerometer(uint16_t t)        { return enableReport(SENSOR_REPORTID_RAW_ACCELEROMETER, t * 1000); }
bool BNO08x::enableRawGyro(uint16_t t)                 { return enableReport(SENSOR_REPORTID_RAW_GYROSCOPE, t * 1000); }
bool BNO08x::enableRawMagnetometer(uint16_t t)         { return enableReport(SENSOR_REPORTID_RAW_MAGNETOMETER, t * 1000); }

/* ---- device commands ---------------------------------------------------- */

bool BNO08x::softReset() { return sh2_devReset() == SH2_OK; }
bool BNO08x::modeOn()    { return sh2_devOn() == SH2_OK; }
bool BNO08x::modeSleep() { return sh2_devSleep() == SH2_OK; }
bool BNO08x::tareNow(bool zAxis, sh2_TareBasis_t basis) {
    return sh2_setTareNow(zAxis ? TARE_AXIS_Z : TARE_AXIS_ALL, basis) == SH2_OK;
}

/* ---- sensor event poll --------------------------------------------------- */

bool BNO08x::getSensorEvent() {
    sensorValue.timestamp = 0;
    sh2_service();
    if (sensorValue.timestamp == 0 && sensorValue.sensorId != SH2_GYRO_INTEGRATED_RV) {
        return false; /* no new events */
    }
    return true;
}

uint8_t BNO08x::getSensorEventID() {
    return sensorValue.sensorId;
}

/* ---- rotation vector getters -------------------------------------------- */

float BNO08x::getQuatI()               { return sensorValue.un.rotationVector.i; }
float BNO08x::getQuatJ()               { return sensorValue.un.rotationVector.j; }
float BNO08x::getQuatK()               { return sensorValue.un.rotationVector.k; }
float BNO08x::getQuatReal()            { return sensorValue.un.rotationVector.real; }
float BNO08x::getQuatRadianAccuracy()  { return sensorValue.un.rotationVector.accuracy; }
uint8_t BNO08x::getQuatAccuracy()      { return sensorValue.status; }

float BNO08x::getRoll() {
    float dqw = getQuatReal(), dqx = getQuatI(), dqy = getQuatJ(), dqz = getQuatK();
    float norm = sqrtf(dqw * dqw + dqx * dqx + dqy * dqy + dqz * dqz);
    dqw /= norm; dqx /= norm; dqy /= norm; dqz /= norm;
    float ysqr = dqy * dqy;
    float t0 = +2.0f * (dqw * dqx + dqy * dqz);
    float t1 = +1.0f - 2.0f * (dqx * dqx + ysqr);
    return atan2f(t0, t1);
}

float BNO08x::getPitch() {
    float dqw = getQuatReal(), dqx = getQuatI(), dqy = getQuatJ(), dqz = getQuatK();
    float norm = sqrtf(dqw * dqw + dqx * dqx + dqy * dqy + dqz * dqz);
    dqw /= norm; dqx /= norm; dqy /= norm; dqz /= norm;
    float t2 = +2.0f * (dqw * dqy - dqz * dqx);
    if (t2 > 1.0f) t2 = 1.0f;
    if (t2 < -1.0f) t2 = -1.0f;
    return asinf(t2);
}

float BNO08x::getYaw() {
    float dqw = getQuatReal(), dqx = getQuatI(), dqy = getQuatJ(), dqz = getQuatK();
    float norm = sqrtf(dqw * dqw + dqx * dqx + dqy * dqy + dqz * dqz);
    dqw /= norm; dqx /= norm; dqy /= norm; dqz /= norm;
    float ysqr = dqy * dqy;
    float t3 = +2.0f * (dqw * dqz + dqx * dqy);
    float t4 = +1.0f - 2.0f * (ysqr + dqz * dqz);
    return atan2f(t3, t4);
}

/* ---- accelerometer / linear / gravity ------------------------------------ */

float BNO08x::getAccelX()    { return sensorValue.un.accelerometer.x; }
float BNO08x::getAccelY()    { return sensorValue.un.accelerometer.y; }
float BNO08x::getAccelZ()    { return sensorValue.un.accelerometer.z; }
uint8_t BNO08x::getAccelAccuracy() { return sensorValue.status; }

float BNO08x::getLinAccelX() { return sensorValue.un.linearAcceleration.x; }
float BNO08x::getLinAccelY() { return sensorValue.un.linearAcceleration.y; }
float BNO08x::getLinAccelZ() { return sensorValue.un.linearAcceleration.z; }
uint8_t BNO08x::getLinAccelAccuracy() { return sensorValue.status; }

float BNO08x::getGravityX()  { return sensorValue.un.gravity.x; }
float BNO08x::getGravityY()  { return sensorValue.un.gravity.y; }
float BNO08x::getGravityZ()  { return sensorValue.un.gravity.z; }
uint8_t BNO08x::getGravityAccuracy() { return sensorValue.status; }

/* ---- gyroscope ------------------------------------------------------------ */

float BNO08x::getGyroX()     { return sensorValue.un.gyroscope.x; }
float BNO08x::getGyroY()     { return sensorValue.un.gyroscope.y; }
float BNO08x::getGyroZ()     { return sensorValue.un.gyroscope.z; }
uint8_t BNO08x::getGyroAccuracy() { return sensorValue.status; }

float BNO08x::getUncalibratedGyroX()    { return sensorValue.un.gyroscopeUncal.x; }
float BNO08x::getUncalibratedGyroY()    { return sensorValue.un.gyroscopeUncal.y; }
float BNO08x::getUncalibratedGyroZ()    { return sensorValue.un.gyroscopeUncal.z; }
float BNO08x::getUncalibratedGyroBiasX(){ return sensorValue.un.gyroscopeUncal.biasX; }
float BNO08x::getUncalibratedGyroBiasY(){ return sensorValue.un.gyroscopeUncal.biasY; }
float BNO08x::getUncalibratedGyroBiasZ(){ return sensorValue.un.gyroscopeUncal.biasZ; }

/* ---- magnetometer --------------------------------------------------------- */

float BNO08x::getMagX()      { return sensorValue.un.magneticField.x; }
float BNO08x::getMagY()      { return sensorValue.un.magneticField.y; }
float BNO08x::getMagZ()      { return sensorValue.un.magneticField.z; }
uint8_t BNO08x::getMagAccuracy() { return sensorValue.status; }

/* ---- step / stability / activity ----------------------------------------- */

uint16_t BNO08x::getStepCount()         { return sensorValue.un.stepCounter.steps; }
uint8_t  BNO08x::getStabilityClassifier(){ return sensorValue.un.stabilityClassifier.classification; }
uint8_t  BNO08x::getActivityClassifier() { return sensorValue.un.personalActivityClassifier.mostLikelyState; }
uint8_t  BNO08x::getActivityConfidence(uint8_t activity) {
    return sensorValue.un.personalActivityClassifier.confidence[activity];
}

/* ---- raw readings ---------------------------------------------------------- */

int16_t BNO08x::getRawAccelX() { return sensorValue.un.rawAccelerometer.x; }
int16_t BNO08x::getRawAccelY() { return sensorValue.un.rawAccelerometer.y; }
int16_t BNO08x::getRawAccelZ() { return sensorValue.un.rawAccelerometer.z; }
int16_t BNO08x::getRawGyroX()  { return sensorValue.un.rawGyroscope.x; }
int16_t BNO08x::getRawGyroY()  { return sensorValue.un.rawGyroscope.y; }
int16_t BNO08x::getRawGyroZ()  { return sensorValue.un.rawGyroscope.z; }
int16_t BNO08x::getRawMagX()   { return sensorValue.un.rawMagnetometer.x; }
int16_t BNO08x::getRawMagY()   { return sensorValue.un.rawMagnetometer.y; }
int16_t BNO08x::getRawMagZ()   { return sensorValue.un.rawMagnetometer.z; }
