/**
 * @file  DFRobot_BMV080_Gravity.h
 * @brief  Define the infrastructure of DFRobot_BMV080_Gravity class
 * @n      The Gravity version talks to the ESP32 BMV080 firmware through I2C slave short frames or Modbus RTU.
 * @n      The BMV080 handle is managed inside the ESP32 firmware, so this library only exposes start, stop and parameter control.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-05-11
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */

#ifndef __DFROBOT_BMV080_GRAVITY_H
#define __DFROBOT_BMV080_GRAVITY_H

#include <math.h>
#include <stdint.h>

#include "Arduino.h"
#include "DFRobot_RTU.h"
#include "Wire.h"

// Open this macro to see the detailed running process of the program.
// #define ENABLE_DBG
#ifdef ENABLE_DBG
#define DBG(...)                 \
  {                              \
    Serial.print("[");           \
    Serial.print(__FUNCTION__);  \
    Serial.print("(): ");        \
    Serial.print(__LINE__);      \
    Serial.print(" ] ");         \
    Serial.println(__VA_ARGS__); \
  }
#else
#define DBG(...)
#endif

#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
#include "SoftwareSerial.h"
#else
#include "HardwareSerial.h"
#endif

#define DFRobot_BMV080_GRAVITY_DEFAULT_ADDR    0x57      ///< Default external I2C slave address and Modbus ID.
#define DFRobot_BMV080_GRAVITY_PID             0x0296    ///< Product ID of DFRobot BMV080 Gravity firmware (SEN0662).
#define DFRobot_BMV080_GRAVITY_VID             0x3343    ///< VID represents DFRobot.
#define DFRobot_BMV080_GRAVITY_VERSION         0x1000    ///< Firmware library register version V1.0.0.
#define DFRobot_BMV080_GRAVITY_REG_MAP_VERSION 0x0003    ///< Expected firmware register map version.

/**
 * @def CONTINUOUS_MODE
 * @brief Continuous mode, the ESP32 firmware keeps BMV080 measuring continuously.
 */
#define CONTINUOUS_MODE 0
/**
 * @def DUTY_CYCLE_MODE
 * @brief Duty cycle mode, the ESP32 firmware measures periodically.
 */
#define DUTY_CYCLE_MODE 1

#define FAST_RESPONSE  1    ///< Fast response algorithm.
#define BALANCED       2    ///< Balanced algorithm.
#define HIGH_PRECISION 3    ///< High precision algorithm.

class DFRobot_BMV080_Gravity {
public:
#define RET_CODE_OK    0
#define RET_CODE_ERROR 1
#define ERR_OK         0
#define ERR_DATA_BUS   1
#define ERR_DATA_READ  2
#define ERR_IC_VERSION 3

  /**
   * @enum eAction_t
   * @brief Values written to the action holding register.
   * @details Action is a transient command and is not saved by the firmware.
   */
  typedef enum {
    eStart = 1,    ///< Start measurement using REG_HOLDING_MEASURE_MODE
    eStop  = 2,    ///< Stop measurement
    eReset = 3,    ///< Reset BMV080 and restore default configuration
  } eAction_t;

  /**
   * @enum eMeasureMode_t
   * @brief BMV080 measurement mode cached in the ESP32 firmware.
   */
  typedef enum {
    eContinuousMode = CONTINUOUS_MODE,    ///< Continuous measurement mode
    eDutyCycleMode  = DUTY_CYCLE_MODE,    ///< Duty-cycle measurement mode
  } eMeasureMode_t;

  /**
   * @enum eMeasurementAlgorithm_t
   * @brief BMV080 measurement algorithm selection.
   */
  typedef enum {
    eFastResponse  = FAST_RESPONSE,     ///< Fast response algorithm
    eBalanced      = BALANCED,          ///< Balanced algorithm
    eHighPrecision = HIGH_PRECISION,    ///< High precision algorithm
  } eMeasurementAlgorithm_t;

  /**
   * @enum eBaud_t
   * @brief UART baud-rate register values used by the firmware.
   */
  typedef enum {
    e2400 = 0x0001,    ///< 2400 bps
    e4800,             ///< 4800 bps
    e9600,             ///< 9600 bps
    e14400,            ///< 14400 bps
    e19200,            ///< 19200 bps
    e38400,            ///< 38400 bps
    e57600,            ///< 57600 bps
    e115200,           ///< 115200 bps
  } eBaud_t;

  /**
   * @enum eParity_t
   * @brief UART parity field stored in the high byte of holding register 0x0001.
   */
  typedef enum {
    eParityNone = 0x00,    ///< No parity
    eParityEven = 0x01,    ///< Even parity
    eParityOdd  = 0x02,    ///< Odd parity
  } eParity_t;

  /**
   * @enum eStopBit_t
   * @brief UART stop-bit field stored in the low byte of holding register 0x0001.
   * @note ESP32 firmware rejects 0.5 stop bit, so only 1, 1.5 and 2 stop bits are exposed.
   */
  typedef enum {
    eStopBit1   = 0x01,    ///< 1 stop bit
    eStopBit1_5 = 0x02,    ///< 1.5 stop bits
    eStopBit2   = 0x03,    ///< 2 stop bits
  } eStopBit_t;

  /**
   * @enum eRunState_t
   * @brief Runtime state reported by input register 0x0004.
   */
  typedef enum {
    eRunStateBoot                = 0,    ///< Boot state
    eRunStateReady               = 1,    ///< Ready state
    eRunStateMeasuringContinuous = 2,    ///< Continuous measurement active
    eRunStateMeasuringDuty       = 3,    ///< Duty-cycle measurement active
    eRunStateStopped             = 4,    ///< Measurement stopped
    eRunStateError               = 5,    ///< Error state
  } eRunState_t;

  /**
   * @struct sBmv080Data_t
   * @brief PM data and state flags cached by the ESP32 firmware.
   */
  typedef struct {
    float    PM1;                          ///< PM1.0 concentration (ug/m3)
    float    PM2_5;                        ///< PM2.5 concentration (ug/m3)
    float    PM10;                         ///< PM10 concentration (ug/m3)
    float    runtime;                      ///< Sensor runtime (seconds)
    uint16_t runState;                     ///< Current run state (see eRunState_t)
    uint16_t status;                       ///< Last BMV080 SDK status code
    bool     isObstructed;                 ///< Obstruction detection flag
    bool     isOutsideMeasurementRange;    ///< Outside measurement range flag
    bool     dataReady;                    ///< New PM data available
    bool     measuring;                    ///< Sensor currently measuring
    bool     paramsVerified;               ///< Parameters confirmed applied to sensor
    bool     valueClamped;                 ///< Reserved compatibility flag (currently always false in float-register map)
    bool     valueInvalid;                 ///< Non-finite float from firmware side was sanitized before register write
    uint16_t sampleSeq;                    ///< Sample sequence number, increments each new measurement
  } sBmv080Data_t;

  DFRobot_BMV080_Gravity(void);
  virtual ~DFRobot_BMV080_Gravity(void);

  /**
   * @fn begin
   * @brief Check whether the ESP32 BMV080 Gravity firmware is reachable.
   * @return true if PID and VID are correct, false on bus error or version mismatch.
   */
  virtual bool begin(void);

  /**
   * @fn getPID
   * @brief Read product ID from input register 0x0000.
   * @return PID, expected value is 0x0296.
   */
  uint16_t getPID(void);

  /**
   * @fn getVID
   * @brief Read vendor ID from input register 0x0001.
   * @return VID, expected value is 0x3343.
   */
  uint16_t getVID(void);

  /**
   * @fn getVersion
   * @brief Read firmware register version.
   * @return Version, for example 0x1000 means V1.0.0.
   */
  uint16_t getVersion(void);

  /**
   * @fn getRegMapVersion
   * @brief Read firmware register map version.
   * @return Register map version. Library and firmware must match for correct operation.
   */
  uint16_t getRegMapVersion(void);

  /**
   * @fn getRunState
   * @brief Read current firmware run state.
   * @return See eRunState_t.
   */
  uint16_t getRunState(void);

  /**
   * @fn getStatus
   * @brief Read last BMV080 SDK status code cached by the firmware.
   * @return Last SDK status code.
   * @retval 0 Operation successful
   * @retval Non-zero Error code from Bosch BMV080 SDK
   */
  uint16_t getStatus(void);

  /**
   * @fn getBmv080DV
   * @brief Read BMV080 SDK driver version cached by the firmware.
   * @param major Major version.
   * @param minor Minor version.
   * @param patch Patch version.
   * @return true if read succeeds.
   */
  bool getBmv080DV(uint16_t &major, uint16_t &minor, uint16_t &patch);

  /**
   * @fn getBmv080ID
   * @brief Read the BMV080 sensor ID cached by the firmware.
   * @param id Buffer to store sensor ID. The buffer must contain at least 13 bytes.
   * @return true if read succeeds.
   */
  bool getBmv080ID(char *id);

  /**
   * @fn readBmv080Data
   * @brief Read the full PM data cache from the ESP32 firmware.
   * @param data Pointer to data structure.
   * @return true if register read succeeds. Check dataReady for valid PM data.
   */
  bool readBmv080Data(sBmv080Data_t *data);

  /**
   * @fn getBmv080Data
   * @brief Read PM data. Returns true only when new data is available.
   * @param data Pointer to data structure. PM1, PM2_5, PM10, runtime, flags are filled on success.
   * @return true if dataReady is set by the firmware, false otherwise.
   */
  bool getBmv080Data(sBmv080Data_t *data);

  /**
   * @fn setBmv080Mode
   * @brief Start measurement by mode. This writes the action register.
   * @param mode CONTINUOUS_MODE (0) or DUTY_CYCLE_MODE (1).
   * @return 0 successful, -1 mode is invalid, other values are communication or firmware errors.
   */
  int setBmv080Mode(uint8_t mode);

  /**
   * @fn stopBmv080
   * @brief Stop current BMV080 measurement through action value 2.
   * @return true if the firmware accepts the action.
   */
  bool stopBmv080(void);

  /**
   * @fn resetBmv080
   * @brief Reset BMV080 and restore default configuration through action value 3.
   * @return true if the firmware accepts the action.
   */
  bool resetBmv080(void);

  /**
   * @fn setIntegrationTime
   * @brief Set measurement integration time.
   * @param integration_time Integration time in seconds.
   * @return 0 successful, -1 invalid value, other values are firmware/communication errors.
   */
  int setIntegrationTime(float integration_time);

  /**
   * @fn getIntegrationTime
   * @brief Read integration time from holding register.
   * @return Integration time in seconds, or NAN on read error.
   */
  float getIntegrationTime(void);

  /**
   * @fn setDutyCyclingPeriod
   * @brief Set duty-cycle period.
   * @param duty_cycling_period Duty-cycle period in seconds.
   * @return 0 successful, other values are firmware/communication errors.
   */
  int setDutyCyclingPeriod(uint16_t duty_cycling_period);

  /**
   * @fn getDutyCyclingPeriod
   * @brief Read duty-cycle period from holding register.
   * @return Duty-cycle period in seconds, or 0 on read error.
   */
  uint16_t getDutyCyclingPeriod(void);

  /**
   * @fn setObstructionDetection
   * @brief Enable or disable obstruction detection.
   * @param enable true to enable, false to disable.
   * @return true if the firmware accepts the value.
   */
  bool setObstructionDetection(bool enable);

  /**
   * @fn getObstructionDetection
   * @brief Read obstruction detection setting from holding register.
   * @return 1 enabled, 0 disabled, -1 on read error.
   */
  int getObstructionDetection(void);

  /**
   * @fn setDoVibrationFiltering
   * @brief Enable or disable vibration filtering.
   * @param enable true to enable, false to disable.
   * @return true if the firmware accepts the value.
   */
  bool setDoVibrationFiltering(bool enable);

  /**
   * @fn getDoVibrationFiltering
   * @brief Read vibration filtering setting from holding register.
   * @return 1 enabled, 0 disabled, -1 on read error.
   */
  int getDoVibrationFiltering(void);

  /**
   * @fn setMeasurementAlgorithm
   * @brief Set BMV080 measurement algorithm.
   * @param measurement_algorithm FAST_RESPONSE, BALANCED or HIGH_PRECISION.
   * @return 0 successful, -1 invalid value, other values are firmware errors.
   */
  int setMeasurementAlgorithm(uint8_t measurement_algorithm);

  /**
   * @fn getMeasurementAlgorithm
   * @brief Read BMV080 measurement algorithm from holding register.
   * @return FAST_RESPONSE, BALANCED, HIGH_PRECISION, or 0 on read error.
   */
  uint8_t getMeasurementAlgorithm(void);

  /**
   * @fn setBaud
   * @brief Save UART baud-rate setting in firmware NVS.
   * @note The new baud rate takes effect after the ESP32 module restarts in UART mode.
   * @param baud See eBaud_t.
   * @return uint8_t 0 on success, other values are communication or firmware errors.
   */
  uint8_t setBaud(eBaud_t baud);

  /**
   * @fn getBaud
   * @brief Read UART baud-rate register value.
   * @return See eBaud_t, or 0 on read error.
   */
  uint16_t getBaud(void);

  /**
   * @fn getBaudValue
   * @brief Convert UART baud-rate register value to bps.
   * @return Baud rate in bps, defaults to 9600 on invalid register value.
   */
  uint32_t getBaudValue(void);

  /**
   * @fn setUartFormat
   * @brief Save UART parity and stop-bit setting in firmware NVS.
   * @note The new UART format takes effect after the ESP32 module restarts in UART mode.
   * @param parity See eParity_t.
   * @param stopBit See eStopBit_t, default eStopBit1.
   * @return uint8_t 0 on success, other values are communication or firmware errors.
   */
  uint8_t setUartFormat(eParity_t parity, eStopBit_t stopBit = eStopBit1);

  /**
   * @fn getUartFormat
   * @brief Read UART parity/stop-bit register.
   * @return High byte is parity, low byte is stop bit, or 0 on read error.
   */
  uint16_t getUartFormat(void);

  /**
   * @fn getLastError
   * @brief Get the last transport or firmware exception code.
   * @return uint8_t 0 for success. Modbus-style exception codes are returned when the firmware rejects a request.
   */
  uint8_t getLastError(void) const;

protected:
  virtual uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count) = 0;
  virtual uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count)        = 0;
  virtual uint8_t readInputReg(uint16_t reg, uint16_t *data, uint16_t count)          = 0;

  bool            readInputValue(uint16_t reg, uint16_t &value);
  bool            readHoldingValue(uint16_t reg, uint16_t &value);
  uint8_t         writeHoldingValue(uint16_t reg, uint16_t value);
  uint8_t         writeHoldingValues(uint16_t reg, const uint16_t *data, uint16_t count);
  static uint32_t baudRegToValue(uint16_t baudReg);

  uint8_t       _lastError;
  sBmv080Data_t _data;

private:
  uint8_t writeAction(eAction_t action);

  enum {
    REG_INPUT_PID             = 0x0000,    ///< Input: Product ID
    REG_INPUT_VID             = 0x0001,    ///< Input: Vendor ID
    REG_INPUT_VERSION         = 0x0002,    ///< Input: Firmware version
    REG_INPUT_REG_MAP_VERSION = 0x0003,    ///< Input: Register map version
    REG_INPUT_RUN_STATE       = 0x0004,    ///< Input: Run state
    REG_INPUT_LAST_STATUS     = 0x0005,    ///< Input: Last SDK status
    REG_INPUT_PM1_F32_HI      = 0x0006,    ///< Input: PM1.0 float32 high word
    REG_INPUT_PM1_F32_LO      = 0x0007,    ///< Input: PM1.0 float32 low word
    REG_INPUT_PM25_F32_HI     = 0x0008,    ///< Input: PM2.5 float32 high word
    REG_INPUT_PM25_F32_LO     = 0x0009,    ///< Input: PM2.5 float32 low word
    REG_INPUT_PM10_F32_HI     = 0x000A,    ///< Input: PM10 float32 high word
    REG_INPUT_PM10_F32_LO     = 0x000B,    ///< Input: PM10 float32 low word
    REG_INPUT_RUNTIME_F32_HI  = 0x000C,    ///< Input: Runtime float32 high word
    REG_INPUT_RUNTIME_F32_LO  = 0x000D,    ///< Input: Runtime float32 low word
    REG_INPUT_FLAGS           = 0x000E,    ///< Input: Status flags
    REG_INPUT_SAMPLE_SEQ      = 0x000F,    ///< Input: Sample sequence counter
    REG_INPUT_DRIVER_MAJOR    = 0x0010,    ///< Input: Driver major version
    REG_INPUT_DRIVER_MINOR    = 0x0011,    ///< Input: Driver minor version
    REG_INPUT_DRIVER_PATCH    = 0x0012,    ///< Input: Driver patch version
    REG_INPUT_SENSOR_ID0      = 0x0013,    ///< Input: Sensor ID (first word)

    REG_HOLDING_BAUDRATE           = 0x0000,    ///< Holding: Baud rate enum
    REG_HOLDING_VERIFY_STOP        = 0x0001,    ///< Holding: Parity/stop bits
    REG_HOLDING_ACTION             = 0x0002,    ///< Holding: Action command
    REG_HOLDING_MEASURE_MODE       = 0x0003,    ///< Holding: Measurement mode
    REG_HOLDING_ALGORITHM          = 0x0004,    ///< Holding: Algorithm selection
    REG_HOLDING_OBSTRUCTION        = 0x0005,    ///< Holding: Obstruction detection
    REG_HOLDING_VIBRATION          = 0x0006,    ///< Holding: Vibration filtering
    REG_HOLDING_INTEGRATION_F32_HI = 0x0007,    ///< Holding: Integration time float32 high word
    REG_HOLDING_INTEGRATION_F32_LO = 0x0008,    ///< Holding: Integration time float32 low word
    REG_HOLDING_DUTY_PERIOD_S      = 0x0009,    ///< Holding: Duty cycle period

    INPUT_FLAG_OBSTRUCTED      = 1U << 0,    ///< Obstruction detected
    INPUT_FLAG_OUTSIDE_RANGE   = 1U << 1,    ///< Outside measurement range
    INPUT_FLAG_DATA_READY      = 1U << 2,    ///< New PM data ready
    INPUT_FLAG_MEASURING       = 1U << 4,    ///< Sensor measuring
    INPUT_FLAG_PARAMS_VERIFIED = 1U << 6,    ///< Parameters verified on sensor
    INPUT_FLAG_VALUE_CLAMPED   = 1U << 8,    ///< PM/runtime value clamped to valid register range
    INPUT_FLAG_VALUE_INVALID   = 1U << 9,    ///< Non-finite PM/runtime input detected and sanitized
  };
};

class DFRobot_BMV080_Gravity_I2C : public DFRobot_BMV080_Gravity {
public:
  /**
   * @fn DFRobot_BMV080_Gravity_I2C
   * @brief Constructor of I2C transport class.
   * @param pWire Pointer to TwoWire object.
   * @param addr External I2C slave address selected by A0/A1, default is 0x57.
   */
  DFRobot_BMV080_Gravity_I2C(TwoWire *pWire, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_ADDR);
  virtual ~DFRobot_BMV080_Gravity_I2C(void);

  bool begin(void);
  void setTimeoutTimeMs(uint32_t timeout);

protected:
  uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count);
  uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count);
  uint8_t readInputReg(uint16_t reg, uint16_t *data, uint16_t count);

private:
  uint8_t readRegs(uint8_t func, uint16_t reg, uint16_t *data, uint16_t count);
  uint8_t writeSingleReg(uint16_t reg, uint16_t value);
  uint8_t writeMultiRegs(uint16_t reg, const uint16_t *data, uint16_t count);
  bool    transferShortFrame(const uint8_t *request, uint8_t requestLen, uint8_t *response, uint8_t responseLen);

  TwoWire *_pWire;
  uint8_t  _i2cAddr;
  uint32_t _timeout;
};

class DFRobot_BMV080_Gravity_UART : public DFRobot_BMV080_Gravity, public DFRobot_RTU {
public:
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  /**
   * @fn DFRobot_BMV080_Gravity_UART
   * @brief Constructor of UART Modbus RTU transport class for SoftwareSerial platforms.
   */
  DFRobot_BMV080_Gravity_UART(SoftwareSerial *sSerial, uint32_t baud, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_ADDR);
#else
  /**
   * @fn DFRobot_BMV080_Gravity_UART
   * @brief Constructor of UART Modbus RTU transport class.
   * @param hSerial Pointer to HardwareSerial object.
   * @param baud Current UART baud rate.
   * @param addr External Modbus ID selected by A0/A1, default is 0x57.
   * @param rxpin MCU RX pin connected to module TX. On ESP32 default is GPIO25.
   * @param txpin MCU TX pin connected to module RX. On ESP32 default is GPIO26.
   */
  DFRobot_BMV080_Gravity_UART(HardwareSerial *hSerial, uint32_t baud, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_ADDR, uint8_t rxpin = 0, uint8_t txpin = 0);
#endif
  virtual ~DFRobot_BMV080_Gravity_UART(void);

  bool begin(void);

protected:
  uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count);
  uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count);
  uint8_t readInputReg(uint16_t reg, uint16_t *data, uint16_t count);

private:
#if defined(ARDUINO_AVR_UNO) || defined(ESP8266)
  SoftwareSerial *_serial;
#else
  HardwareSerial *_serial;
#endif
  uint32_t _baud;
  uint8_t  _addr;
  uint8_t  _rxpin;
  uint8_t  _txpin;
};

#endif
