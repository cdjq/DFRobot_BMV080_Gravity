/**
 * @file  DFRobot_BMV080_Gravity.h
 * @brief  Define the infrastructure of DFRobot_BMV080_Gravity class
 * @n      The Gravity version talks to the ESP32 BMV080 firmware through I2C slave short frames or Modbus RTU.
 * @n      The BMV080 handle is managed inside the ESP32 firmware, so this library only exposes start, stop and parameter control.
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author      DFRobot
 * @version     V1.0.0
 * @date        2026-06-09
 * @url         https://github.com/DFRobot/DFRobot_BMV080_Gravity
 */

#ifndef __DFROBOT_BMV080_GRAVITY_H
#define __DFROBOT_BMV080_GRAVITY_H

#include <Arduino.h>
#include <Wire.h>
#include "DFRobot_RTU.h"
#include <math.h>
#include <stdint.h>


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

#define DFRobot_BMV080_GRAVITY_DEFAULT_I2C_ADDR 0x57     ///< Default external I2C slave address.
#define DFRobot_BMV080_GRAVITY_DEFAULT_RTU_ADDR 0x57     ///< Factory default UART Modbus RTU slave address.

class DFRobot_BMV080_Gravity {
public:
#define RET_CODE_OK    0
#define RET_CODE_ERROR 1
#define ERR_OK         0
#define ERR_DATA_BUS   1
#define ERR_DATA_READ  2
#define ERR_IC_VERSION 3

  /**
   * @enum eMeasureMode_t
   * @brief BMV080 measurement mode cached in the ESP32 firmware.
   */
  typedef enum {
    eContinuousMode = 0,    ///< Continuous measurement mode
    eDutyCycleMode  = 1,    ///< Duty-cycle measurement mode
  } eMeasureMode_t;

  /**
   * @enum eMeasurementAlgorithm_t
   * @brief BMV080 measurement algorithm selection.
   */
  typedef enum {
    eFastResponse  = 1,    ///< Fast response algorithm
    eBalanced      = 2,    ///< Balanced algorithm
    eHighPrecision = 3,    ///< High precision algorithm
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
   * @brief UART parity field stored in the high byte of holding register 0x0002.
   */
  typedef enum {
    eParityNone = 0x00,    ///< No parity
    eParityEven = 0x01,    ///< Even parity
    eParityOdd  = 0x02,    ///< Odd parity
  } eParity_t;

  /**
   * @enum eStopBit_t
   * @brief UART stop-bit field stored in the low byte of holding register 0x0002.
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
   * @struct sData_t
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
  } sData_t;

  /**
   * @fn DFRobot_BMV080_Gravity
   * @brief Constructor.
   */
  DFRobot_BMV080_Gravity(void);

  /**
   * @fn ~DFRobot_BMV080_Gravity
   * @brief Destructor.
   */
  virtual ~DFRobot_BMV080_Gravity(void);

  /**
   * @fn begin
   * @brief Initialize the module.
   * @details Check whether compatible BMV080 Gravity firmware is reachable.
   * @return Initialization status.
   * @retval true Initialization succeeded.
   * @retval false Initialization failed.
   */
  virtual bool begin(void);

  /**
   * @fn getData
   * @brief Read particulate matter measurement data.
   * @details The function returns true only when the firmware reports new valid data.
   * @param data Pointer to the data structure used to store PM1.0, PM2.5, PM10 and state flags.
   * @return Whether new valid data was read.
   * @retval true New data was read.
   * @retval false No new data is available, or the read failed.
   */
  bool getData(sData_t *data);

  /**
   * @fn setMeasureMode
   * @brief Set measurement mode and start measurement.
   * @details Write the measurement-mode register, write the start action, then wait until the firmware reports the target run state.
   * @param mode Measurement mode. See eMeasureMode_t.
   * @n     eContinuousMode: Continuous measurement mode.
   * @n     eDutyCycleMode: Duty-cycle measurement mode.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval -1 Invalid parameter.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error or start-state timeout.
   * @retval 3 Firmware version or device information error.
   * @note When duty-cycle measurement is started, the firmware forces the algorithm to eFastResponse as required by the BMV080 SDK.
   */
  int setMeasureMode(eMeasureMode_t mode);

  /**
   * @fn stopMeasurement
   * @brief Stop the current measurement.
   * @details Write the stop command to the action register.
   * @return Stop command execution status.
   * @retval true Stop command succeeded.
   * @retval false Stop command failed.
   */
  bool stopMeasurement(void);

  /**
   * @fn reset
   * @brief Reset the sensor and restore default configuration.
   * @details Write the reset command to the action register.
   * @return Reset command execution status.
   * @retval true Reset command succeeded.
   * @retval false Reset command failed.
   */
  bool reset(void);

  /**
   * @fn setIntegrationTime
   * @brief Set measurement integration time.
   * @details In duty-cycle mode, the duty-cycle period must be at least integration time plus 2 seconds.
   * @param integration_time Integration time in seconds.
   * @n     The value must be greater than 0 and must not be NAN or INF.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval -1 Invalid parameter or duty-cycle period constraint was not met.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   * @note When increasing integration time beyond the current period margin, call setDutyCyclingPeriod() first.
   */
  int setIntegrationTime(float integration_time);

  /**
   * @fn setDutyCyclingPeriod
   * @brief Set duty-cycle measurement period.
   * @details The period must be at least the current integration time plus 2 seconds.
   * @param duty_cycling_period Duty-cycle measurement period in seconds.
   * @n     The value must satisfy the current integration time plus 2 seconds constraint.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval -1 Invalid parameter or integration time read failed.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   * @note When shortening the duty-cycle period, call setIntegrationTime() first to lower integration time if needed.
   */
  int setDutyCyclingPeriod(uint16_t duty_cycling_period);

  /**
   * @fn getIntegrationTime
   * @brief Read measurement integration time.
   * @return Integration time in seconds.
   * @retval NAN Read failed.
   */
  float getIntegrationTime(void);

  /**
   * @fn getDutyCyclingPeriod
   * @brief Read duty-cycle measurement period.
   * @return Duty-cycle measurement period in seconds.
   * @retval 0 Read failed.
   */
  uint16_t getDutyCyclingPeriod(void);


  /**
   * @fn setObstructionDetection
   * @brief Enable or disable obstruction detection.
   * @param enable Obstruction detection switch.
   * @n     true: Enable obstruction detection.
   * @n     false: Disable obstruction detection.
   * @return Setting status.
   * @retval true Setting succeeded.
   * @retval false Setting failed.
   */
  bool setObstructionDetection(bool enable);

  /**
   * @fn getObstructionDetection
   * @brief Read obstruction detection switch state.
   * @return Obstruction detection switch state.
   * @retval 1 Enabled.
   * @retval 0 Disabled.
   * @retval -1 Read failed.
   */
  int getObstructionDetection(void);

  /**
   * @fn setVibrationFiltering
   * @brief Enable or disable vibration filtering.
   * @param enable Vibration filtering switch.
   * @n     true: Enable vibration filtering.
   * @n     false: Disable vibration filtering.
   * @return Setting status.
   * @retval true Setting succeeded.
   * @retval false Setting failed.
   */
  bool setVibrationFiltering(bool enable);

  /**
   * @fn getVibrationFiltering
   * @brief Read vibration filtering switch state.
   * @return Vibration filtering switch state.
   * @retval 1 Enabled.
   * @retval 0 Disabled.
   * @retval -1 Read failed.
   */
  int getVibrationFiltering(void);

  /**
   * @fn setMeasurementAlgorithm
   * @brief Set measurement algorithm.
   * @param measurement_algorithm Measurement algorithm.
   * @n     eFastResponse: Fast response algorithm.
   * @n     eBalanced: Balanced algorithm.
   * @n     eHighPrecision: High precision algorithm.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval -1 Invalid parameter.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   * @note When duty-cycle measurement is started, the firmware forces eFastResponse as required by the BMV080 SDK.
   */
  int setMeasurementAlgorithm(eMeasurementAlgorithm_t measurement_algorithm);

  /**
   * @fn getMeasurementAlgorithm
   * @brief Read measurement algorithm.
   * @return Current measurement algorithm.
   * @retval eFastResponse Fast response algorithm.
   * @retval eBalanced Balanced algorithm.
   * @retval eHighPrecision High precision algorithm.
   * @retval 0 Read failed or register value is invalid.
   */
  eMeasurementAlgorithm_t getMeasurementAlgorithm(void);

  /**
   * @fn setBaud
   * @brief Set UART baud rate.
   * @details Save the UART baud-rate setting to firmware NVS.
   * @param baud Baud-rate enum value.
   * @n     Available values: e2400, e4800, e9600, e14400, e19200, e38400, e57600, e115200.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   * @note The new baud rate takes effect after restart.
   */
  uint8_t setBaud(eBaud_t baud);

  /**
   * @fn getBaud
   * @brief Read UART baud rate.
   * @return Current baud rate in bps.
   * @retval 0 Read failed.
   * @note Invalid register values are parsed as the default 9600 bps.
   */
  uint32_t getBaud(void);

  /**
   * @fn setUartFormat
   * @brief Set UART parity and stop bits.
   * @details Save the UART frame-format setting to firmware NVS.
   * @param parity Parity configuration.
   * @n     eParityNone: No parity.
   * @n     eParityEven: Even parity.
   * @n     eParityOdd: Odd parity.
   * @param stopBit Stop-bit configuration.
   * @n     eStopBit1: 1 stop bit.
   * @n     eStopBit1_5: 1.5 stop bits.
   * @n     eStopBit2: 2 stop bits.
   * @return Setting status.
   * @retval 0 Setting succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   * @note The new UART frame format takes effect after restart.
   */
  uint8_t setUartFormat(eParity_t parity, eStopBit_t stopBit = eStopBit1);

  /**
   * @fn getUartFormat
   * @brief Read UART parity and stop-bit register value.
   * @return UART frame-format register value.
   * @retval 0 Read failed.
   * @note The high byte is parity and the low byte is stop bits.
   */
  uint16_t getUartFormat(void);

protected:
  /**
   * @fn writeHoldingReg
   * @brief Write holding registers.
   * @param reg Start holding register address.
   * @param data Pointer to the data buffer to write.
   * @param count Number of registers to write.
   * @n     The value must be greater than 0.
   * @return Register write status.
   * @retval 0 Write succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  virtual uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count) = 0;

  /**
   * @fn readHoldingReg
   * @brief Read holding registers.
   * @param reg Start holding register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  virtual uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count)        = 0;

  /**
   * @fn readInputReg
   * @brief Read input registers.
   * @param reg Start input register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  virtual uint8_t readInputReg(uint16_t reg, uint16_t *data, uint16_t count)          = 0;

private:
  typedef enum {
    eStart = 1,
    eStop  = 2,
    eReset = 3,
  } eAction_t;

  /**
   * @fn writeAction
   * @brief Write an action command.
   * @param action Action command.
   * @n     eStart: Start measurement.
   * @n     eStop: Stop measurement.
   * @n     eReset: Reset the sensor and restore default configuration.
   * @return Action command write status.
   * @retval 0 Write succeeded.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeAction(eAction_t action);

  /**
   * @fn readData
   * @brief Read the full particulate matter data cache.
   * @details Read PM data, run state and state flags from input registers.
   * @param data Pointer to the data structure.
   * @return Read status.
   * @retval true Read succeeded.
   * @retval false Invalid parameter or read failed.
   */
  bool readData(sData_t *data);


  /**
   * @fn readHoldingValue
   * @brief Read one holding register value.
   * @param reg Holding register address.
   * @param value Reference used to store the read value.
   * @return Read status.
   * @retval true Read succeeded.
   * @retval false Read failed.
   */
  bool readHoldingValue(uint16_t reg, uint16_t &value);

  /**
   * @fn writeHoldingValue
   * @brief Write one holding register value.
   * @param reg Holding register address.
   * @param value Register value to write.
   * @return Write status.
   * @retval 0 Write succeeded.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeHoldingValue(uint16_t reg, uint16_t value);

  /**
   * @fn writeHoldingValues
   * @brief Write multiple holding register values.
   * @param reg Start holding register address.
   * @param data Pointer to the data buffer to write.
   * @param count Number of registers to write.
   * @n     The value must be greater than 0.
   * @return Write status.
   * @retval 0 Write succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeHoldingValues(uint16_t reg, const uint16_t *data, uint16_t count);

  /**
   * @fn baudRegToValue
   * @brief Convert a baud-rate register enum value to the actual baud rate.
   * @param baudReg Baud-rate register enum value.
   * @n     Available values: e2400, e4800, e9600, e14400, e19200, e38400, e57600, e115200.
   * @return Actual baud rate in bps.
   * @retval 9600 Default baud rate returned when the register value is invalid.
   */
  static uint32_t baudRegToValue(uint16_t baudReg);

  sData_t _data;

  enum {
    EXPECTED_PID             = 0x0296,
    EXPECTED_VID             = 0x3343,
    EXPECTED_REG_MAP_VERSION = 0x0004,

    REG_INPUT_PID       = 0x0000,    ///< Input: Product ID
    REG_INPUT_RUN_STATE = 0x0004,    ///< Input: Run state

    REG_HOLDING_BAUDRATE           = 0x0001,    ///< Holding: Baud rate enum
    REG_HOLDING_VERIFY_STOP        = 0x0002,    ///< Holding: Parity/stop bits
    REG_HOLDING_ACTION             = 0x0003,    ///< Holding: Action command
    REG_HOLDING_MEASURE_MODE       = 0x0004,    ///< Holding: Measurement mode
    REG_HOLDING_ALGORITHM          = 0x0005,    ///< Holding: Algorithm selection
    REG_HOLDING_OBSTRUCTION        = 0x0006,    ///< Holding: Obstruction detection
    REG_HOLDING_VIBRATION          = 0x0007,    ///< Holding: Vibration filtering
    REG_HOLDING_INTEGRATION_F32_HI = 0x0008,    ///< Holding: Integration time float32 high word
    REG_HOLDING_INTEGRATION_F32_LO = 0x0009,    ///< Holding: Integration time float32 low word
    REG_HOLDING_DUTY_PERIOD_S      = 0x000A,    ///< Holding: Duty cycle period

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
   * @brief I2C transport class constructor.
   * @param pWire Pointer to the TwoWire object.
   * @param addr Module I2C slave address.
   * @n     Default value is 0x57.
   */
  DFRobot_BMV080_Gravity_I2C(TwoWire *pWire, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_I2C_ADDR);

  /**
   * @fn ~DFRobot_BMV080_Gravity_I2C
   * @brief I2C transport class destructor.
   */
  virtual ~DFRobot_BMV080_Gravity_I2C(void);

  /**
   * @fn begin
   * @brief Initialize I2C communication and detect the module.
   * @details Initialize the TwoWire bus, then call the base begin() to finish firmware compatibility checks.
   * @return Initialization status.
   * @retval true Initialization succeeded.
   * @retval false Initialization failed.
   */
  bool begin(void);

  /**
   * @fn setTimeoutTimeMs
   * @brief Set I2C communication timeout.
   * @param timeout Timeout in milliseconds.
   */
  void setTimeoutTimeMs(uint32_t timeout);

protected:
  /**
   * @fn writeHoldingReg
   * @brief Write holding registers through I2C.
   * @param reg Start holding register address.
   * @param data Pointer to the data buffer to write.
   * @param count Number of registers to write.
   * @n     The value must be greater than 0.
   * @return Register write status.
   * @retval 0 Write succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count);

  /**
   * @fn readHoldingReg
   * @brief Read holding registers through I2C.
   * @param reg Start holding register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count);

  /**
   * @fn readInputReg
   * @brief Read input registers through I2C.
   * @param reg Start input register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t readInputReg(uint16_t reg, uint16_t *data, uint16_t count);

private:
  /**
   * @fn readRegs
   * @brief Read registers through an I2C short frame.
   * @param func Modbus function code.
   * @n     0x03: Read holding registers.
   * @n     0x04: Read input registers.
   * @param reg Start register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     Range: 1 to BMV080_I2C_MAX_READ_REGS.
   * @return Read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t readRegs(uint8_t func, uint16_t reg, uint16_t *data, uint16_t count);

  /**
   * @fn writeSingleReg
   * @brief Write one holding register through an I2C short frame.
   * @param reg Holding register address.
   * @param value Register value to write.
   * @return Write status.
   * @retval 0 Write succeeded.
   * @retval 1 Communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeSingleReg(uint16_t reg, uint16_t value);

  /**
   * @fn writeMultiRegs
   * @brief Write multiple holding registers through an I2C short frame.
   * @param reg Start holding register address.
   * @param data Pointer to the data buffer to write.
   * @param count Number of registers to write.
   * @n     Range: 1 to BMV080_I2C_MAX_WRITE_REGS.
   * @return Write status.
   * @retval 0 Write succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   * @retval 2 Data read error.
   */
  uint8_t writeMultiRegs(uint16_t reg, const uint16_t *data, uint16_t count);

  /**
   * @fn transferShortFrame
   * @brief Transfer an I2C short frame.
   * @param request Pointer to the request frame buffer.
   * @param requestLen Request frame length.
   * @n     Range: 1 to BMV080_I2C_MAX_FRAME_LEN.
   * @param response Pointer to the response frame buffer.
   * @param responseLen Expected response frame length.
   * @n     Range: 1 to BMV080_I2C_MAX_FRAME_LEN.
   * @return Short-frame transfer status.
   * @retval true Transfer succeeded.
   * @retval false Transfer failed.
   */
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
   * @brief UART Modbus RTU transport class constructor.
   * @param sSerial Pointer to the SoftwareSerial object.
   * @param baud Current UART baud rate.
   * @param addr Current UART Modbus RTU slave address.
   * @n     Default value is 0x57.
   */
  DFRobot_BMV080_Gravity_UART(SoftwareSerial *sSerial, uint32_t baud, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_RTU_ADDR);
#else
  /**
   * @fn DFRobot_BMV080_Gravity_UART
   * @brief UART Modbus RTU transport class constructor.
   * @param hSerial Pointer to the HardwareSerial object.
   * @param baud Current UART baud rate.
   * @param addr Current UART Modbus RTU slave address.
   * @n     Default value is 0x57.
   * @param rxpin MCU RX pin connected to the module TX pin.
   * @n     ESP32 uses GPIO25 by default.
   * @param txpin MCU TX pin connected to the module RX pin.
   * @n     ESP32 uses GPIO26 by default.
   */
  DFRobot_BMV080_Gravity_UART(HardwareSerial *hSerial, uint32_t baud, uint8_t addr = DFRobot_BMV080_GRAVITY_DEFAULT_RTU_ADDR, uint8_t rxpin = 0, uint8_t txpin = 0);
#endif

  /**
   * @fn ~DFRobot_BMV080_Gravity_UART
   * @brief UART Modbus RTU transport class destructor.
   */
  virtual ~DFRobot_BMV080_Gravity_UART(void);

  /**
   * @fn begin
   * @brief Initialize UART communication and detect the module.
   * @details Initialize the serial port, then call the base begin() to finish firmware compatibility checks.
   * @return Initialization status.
   * @retval true Initialization succeeded.
   * @retval false Initialization failed.
   */
  bool begin(void);

protected:
  /**
   * @fn writeHoldingReg
   * @brief Write holding registers through UART Modbus RTU.
   * @param reg Start holding register address.
   * @param data Pointer to the data buffer to write.
   * @param count Number of registers to write.
   * @n     The value must be greater than 0.
   * @return Register write status.
   * @retval 0 Write succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   */
  uint8_t writeHoldingReg(uint16_t reg, const uint16_t *data, uint16_t count);

  /**
   * @fn readHoldingReg
   * @brief Read holding registers through UART Modbus RTU.
   * @param reg Start holding register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   */
  uint8_t readHoldingReg(uint16_t reg, uint16_t *data, uint16_t count);

  /**
   * @fn readInputReg
   * @brief Read input registers through UART Modbus RTU.
   * @param reg Start input register address.
   * @param data Pointer to the read data buffer.
   * @param count Number of registers to read.
   * @n     The value must be greater than 0.
   * @return Register read status.
   * @retval 0 Read succeeded.
   * @retval 1 Invalid parameter, communication error or firmware returned an error.
   */
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
