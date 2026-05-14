# DFRobot_BMV080_Gravity

* [中文版](./README_CN.md)

DFRobot_BMV080_Gravity is an Arduino library for the DFRobot BMV080 Gravity module firmware.

The BMV080 is a particulate matter sensor managed by an ESP32 firmware on the module. This user library communicates with the firmware's register table through:

- **I2C** — Short register frames (0xA5 prefix), fast for local communication
- **UART** — Standard Modbus RTU protocol, suitable for longer distances

The library does **not** include the Bosch BMV080 SDK and does not expose `open` or `close` APIs — the ESP32 firmware owns the BMV080 sensor handle.

## Table of Contents

- [Summary](#summary)
- [Installation](#installation)
- [Methods](#methods)
- [Compatibility](#compatibility)
- [History](#history)
- [Credits](#credits)

## Summary

- This library provides I2C and UART Modbus RTU transport layers for DFRobot's BMV080 Gravity module
- Supports continuous and duty-cycle measurement modes
- Read PM1.0, PM2.5 and PM10 mass concentration data
- Configure measurement parameters: integration time, duty-cycle period, algorithm selection, obstruction detection, vibration filtering
- Read runtime state and flags from `sBmv080Data_t`
- `sBmv080Data_t` includes `valueInvalid` to indicate non-finite source data sanitization
- Configure UART communication parameters: baud rate, parity, stop bits (saved to module NVS)

## Installation

This library depends on the `DFRobot_RTU` library. Install it before compiling UART examples.

To use this library, download the zip file and extract it to the `libraries` directory under your Arduino sketchbook directory, or install it through the Arduino Library Manager.

1. Open Arduino IDE
2. Search for `DFRobot_BMV080_Gravity` in Library Manager
3. Click Install

Or install manually:

```bash
git clone https://github.com/DFRobot/DFRobot_BMV080_Gravity.git
```

## Methods

```C++

/**
 * @fn begin
 * @brief Check whether the ESP32 BMV080 Gravity firmware is reachable.
 * @return true if PID and VID are correct, false on bus error or version mismatch.
 */
virtual bool begin(void);

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
 * @brief Stop current BMV080 measurement.
 * @return true if the firmware accepts the action.
 */
bool stopBmv080(void);

/**
 * @fn resetBmv080
 * @brief Reset BMV080 and restore default configuration.
 * @return true if the firmware accepts the action.
 */
bool resetBmv080(void);

/**
 * @fn setIntegrationTime
 * @brief Set measurement integration time.
 * @param integration_time Integration time in seconds.
 * @return 0 successful, -1 invalid value, other values are firmware errors.
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
 * @return 0 successful, other values are firmware errors.
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
 * @param measurement_algorithm FAST_RESPONSE(1), BALANCED(2) or HIGH_PRECISION(3).
 * @return 0 successful, -1 invalid value, other values are firmware errors.
 */
int setMeasurementAlgorithm(uint8_t measurement_algorithm);

/**
 * @fn getMeasurementAlgorithm
 * @brief Read BMV080 measurement algorithm from holding register.
 * @return FAST_RESPONSE(1), BALANCED(2), HIGH_PRECISION(3), or 0 on read error.
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
```

## Examples

- `consecutiveRead`: Continuous measurement and PM data read example. Demonstrates `begin()`, `setBmv080Mode()`, `getBmv080Data()`, `getLastError()`, `sBmv080Data_t` fields.
- `consecutiveInterrupt`: Continuous measurement with external interrupt. Demonstrates interrupt-driven data collection using the BMV080 INT pin.
- `dutyCycleRead`: Duty-cycle measurement with full parameter configuration example. Demonstrates `setIntegrationTime()` / `getIntegrationTime()`, `setDutyCyclingPeriod()` / `getDutyCyclingPeriod()`, `setMeasurementAlgorithm()` / `getMeasurementAlgorithm()`, `setObstructionDetection()` / `getObstructionDetection()`, `setDoVibrationFiltering()` / `getDoVibrationFiltering()`, `setBmv080Mode()`, `stopBmv080()`.
- `dutyCycleInterrupt`: Duty-cycle measurement with external interrupt. Demonstrates interrupt-driven data collection in periodic measurement mode.
- `readModuleInfo`: Module information read example. Demonstrates `getPID()`, `getVID()`, `getVersion()`, `getRegMapVersion()`, `getRunState()`, `getStatus()`, `getBmv080DV()`, `getBmv080ID()`.
- `setBaudUartFormat`: UART baud rate, parity and stop-bit configuration example. Demonstrates `setBaud()` / `getBaud()` / `getBaudValue()`, `setUartFormat()` / `getUartFormat()`.

## Compatibility

| MCU                | Work Well | Work Wrong | Untested | Remarks |
| ------------------ |:---------:|:----------:|:--------:| ------- |
| ATmega328          |     √     |            |          |         |
| ATmega2560         |     √     |            |          |         |
| ESP32              |     √     |            |          |         |
| ESP8266             |           |            |    √     |         |
| micro:bit          |           |            |    √     |         |
| Raspberry Pi Pico  |           |            |    √     |         |

## History

- Date 2026-05-11
- Version V1.0.0

## Credits

Written by DFRobot (welcom@dfrobot.com), 2026. (Welcome to our website)
