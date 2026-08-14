/*
 * bno08x.h
 * BNO08x wrapper class (HAL port of the SparkFun BNO08x Arduino library),
 * exposing the same API surface the original IMUManager uses.
 */
#ifndef BNO08X_H
#define BNO08X_H

#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "bno_hal.h"
#include "stm32f4xx_hal.h"

#include <stdint.h>
#include <string.h>

#define BNO08x_DEFAULT_ADDRESS 0x4B

/* Report IDs (mirroring the SparkFun header). */
#define SENSOR_REPORTID_ACCELEROMETER               SH2_ACCELEROMETER
#define SENSOR_REPORTID_GYROSCOPE_CALIBRATED        SH2_GYROSCOPE_CALIBRATED
#define SENSOR_REPORTID_MAGNETIC_FIELD              SH2_MAGNETIC_FIELD_CALIBRATED
#define SENSOR_REPORTID_LINEAR_ACCELERATION         SH2_LINEAR_ACCELERATION
#define SENSOR_REPORTID_ROTATION_VECTOR             SH2_ROTATION_VECTOR
#define SENSOR_REPORTID_GRAVITY                     SH2_GRAVITY
#define SENSOR_REPORTID_UNCALIBRATED_GYRO           SH2_GYROSCOPE_UNCALIBRATED
#define SENSOR_REPORTID_STEP_COUNTER                SH2_STEP_COUNTER
#define SENSOR_REPORTID_STABILITY_CLASSIFIER        SH2_STABILITY_CLASSIFIER
#define SENSOR_REPORTID_RAW_ACCELEROMETER           SH2_RAW_ACCELEROMETER
#define SENSOR_REPORTID_RAW_GYROSCOPE               SH2_RAW_GYROSCOPE
#define SENSOR_REPORTID_RAW_MAGNETOMETER            SH2_RAW_MAGNETOMETER
#define SENSOR_REPORTID_PERSONAL_ACTIVITY_CLASSIFIER SH2_PERSONAL_ACTIVITY_CLASSIFIER

#define TARE_AXIS_ALL 0x07
#define TARE_AXIS_Z   0x04

class BNO08x {
public:
    BNO08x() : _connected(false), _reset_occurred(false) {}

    bool begin(I2C_HandleTypeDef *hi2c, uint8_t deviceAddress = BNO08x_DEFAULT_ADDRESS,
               GPIO_TypeDef *intPort = NULL, uint16_t intPin = 0,
               GPIO_TypeDef *rstPort = NULL, uint16_t rstPin = 0);

    bool isConnected();
    void hardwareReset();
    bool wasReset();
    uint8_t getResetReason();

    bool enableReport(sh2_SensorId_t sensor, uint32_t interval_us = 10000,
                      uint32_t sensorSpecific = 0);
    bool getSensorEvent();
    uint8_t getSensorEventID();

    bool softReset();
    bool modeOn();
    bool modeSleep();

    bool enableRotationVector(uint16_t t = 10);
    bool enableAccelerometer(uint16_t t = 10);
    bool enableLinearAccelerometer(uint16_t t = 10);
    bool enableGravity(uint16_t t = 10);
    bool enableGyro(uint16_t t = 10);
    bool enableUncalibratedGyro(uint16_t t = 10);
    bool enableMagnetometer(uint16_t t = 10);
    bool enableStepCounter(uint16_t t = 10);
    bool enableStabilityClassifier(uint16_t t = 10);
    bool enableActivityClassifier(uint16_t t, uint32_t activitiesToEnable);
    bool enableRawAccelerometer(uint16_t t = 10);
    bool enableRawGyro(uint16_t t = 10);
    bool enableRawMagnetometer(uint16_t t = 10);

    float getQuatI();
    float getQuatJ();
    float getQuatK();
    float getQuatReal();
    float getQuatRadianAccuracy();
    uint8_t getQuatAccuracy();

    float getAccelX(); float getAccelY(); float getAccelZ();
    uint8_t getAccelAccuracy();
    float getLinAccelX(); float getLinAccelY(); float getLinAccelZ();
    uint8_t getLinAccelAccuracy();
    float getGravityX(); float getGravityY(); float getGravityZ();
    uint8_t getGravityAccuracy();
    float getGyroX(); float getGyroY(); float getGyroZ();
    uint8_t getGyroAccuracy();
    float getUncalibratedGyroX(); float getUncalibratedGyroY(); float getUncalibratedGyroZ();
    float getUncalibratedGyroBiasX(); float getUncalibratedGyroBiasY(); float getUncalibratedGyroBiasZ();
    float getMagX(); float getMagY(); float getMagZ();
    uint8_t getMagAccuracy();

    uint16_t getStepCount();
    uint8_t getStabilityClassifier();
    uint8_t getActivityClassifier();
    uint8_t getActivityConfidence(uint8_t activity);

    int16_t getRawAccelX(); int16_t getRawAccelY(); int16_t getRawAccelZ();
    int16_t getRawGyroX();  int16_t getRawGyroY();  int16_t getRawGyroZ();
    int16_t getRawMagX();   int16_t getRawMagY();   int16_t getRawMagZ();

    float getRoll();
    float getPitch();
    float getYaw();

    bool tareNow(bool zAxis = false, sh2_TareBasis_t basis = SH2_TARE_BASIS_ROTATION_VECTOR);

    sh2_ProductIds_t prodIds;
    sh2_SensorValue_t sensorValue;

private:
    sh2_Hal_t _HAL;
    bool _connected;
    bool _reset_occurred;

    bool _init();
    static void shtpCallback(void *cookie, sh2_AsyncEvent_t *pEvent);
    static void sensorCallback(void *cookie, sh2_SensorEvent_t *event);
};

#endif
