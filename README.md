# DFRobot_BMV080_Gravity

* [中文版](./README_CN.md)

DFRobot_BMV080_Gravity is an Arduino library for the DFRobot BMV080 Gravity PM2.5 sensor module, used to read PM1.0, PM2.5 and PM10 particulate matter concentration data.

The BMV080 sensor core, developed by Bosch, is the world's smallest PM air quality sensor — over 450 times smaller than comparable products on the market. Despite its ultra-compact size, it delivers precise PM2.5, PM1.0 and PM10 measurements.

Unlike traditional PM sensors that rely on fans or ducts to draw particles into the detection zone (causing fan noise, dust accumulation, and maintenance headaches), the BMV080 uses a camera-like laser-optics principle to calculate mass concentration from particles freely moving in open space. It leverages ambient airflow to transport particles into the detection area, eliminating fans entirely — improving reliability and reducing maintenance.

The BMV080 sensor on the module is managed by an ESP32 firmware. This library communicates with the module firmware through:

- **I2C** — Short register frames (0xA5 prefix), fast for local communication
- **UART** — Standard Modbus RTU protocol, suitable for longer distances

The library does **not** include the Bosch BMV080 SDK and does not expose `open` or `close` APIs — the ESP32 firmware owns the BMV080 sensor handle.

<p align="center">
  <img src="./resources/images/[SEN0662]V1.0.0  B.png" width="45%">
   &nbsp; &nbsp; &nbsp;
  <img src="./resources/images/[SEN0662]V1.0.0  F.png" width="45%">
</p>

## Product Link（[https://www.dfrobot.com](https://www.dfrobot.com)）
    SKU:SEN0662

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
- Read PM data and state flags through `sData_t`
- Configure UART address, baud rate, parity and stop bits (saved to module NVS)
- Duty-cycle measurement starts with the `eFastResponse` algorithm as required by the BMV080 SDK
- Single-register reads include a 3-attempt retry mechanism to improve communication reliability

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

`getData()` fills `sData_t` only when a new PM sample is ready. The returned data fields are:

- `PM1`, `PM2_5`, `PM10`: PM1.0, PM2.5 and PM10 mass concentration in ug/m3
- `runtime`: Sensor runtime in seconds
- `runState`: Current firmware run state, see `eRunState_t`
- `status`: Last BMV080 SDK/firmware status code
- `isObstructed`: Obstruction detected
- `isOutsideMeasurementRange`: PM value is outside the reliable measurement range
- `dataReady`: New PM data is available; `getData()` returns `true` only when this flag is set
- `measuring`: Sensor is currently measuring
- `paramsVerified`: Measurement parameters have been applied to the sensor
- `valueClamped`: Reserved compatibility flag
- `valueInvalid`: Non-finite PM/runtime value was sanitized by firmware
- `sampleSeq`: Sample sequence number, increments for each new measurement

```C++

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
 * @param data Pointer to sData_t used to store PM concentration, runtime, run state and state flags.
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
 * @note If the firmware enters eRunStateError, this function returns the firmware status register value, or 1 when that value is 0.
 *       Firmware compatibility is checked by begin().
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
 * @details Write the reset command to the action register. The firmware stops measurement, resets the BMV080,
 *          restores the default holding registers, then saves those defaults.
 * @n     Default UART RTU address: 0x57.
 * @n     Default UART settings: 9600 bps, 8-N-1.
 * @n     Default measurement settings: eContinuousMode, eBalanced, obstruction detection enabled,
 *        vibration filtering enabled, integration time 10.0 s, duty-cycle period 30 s.
 * @note If UART communication settings were changed before reset, reconnect with the restored defaults after reset or module restart as needed.
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
 * @fn setUartAddress
 * @brief Set UART Modbus RTU slave address.
 * @details Save the UART device address to firmware NVS.
 * @param addr UART Modbus RTU slave address.
 * @n     Valid range: 0x01 to 0xF7. 0x00 is the Modbus broadcast address and is not allowed.
 * @return Setting status.
 * @retval 0 Setting succeeded.
 * @retval 1 Invalid parameter, communication error or firmware returned an error.
 * @retval 2 Data read error.
 * @note The new UART address takes effect after restart. Reconnect with the new address after restart.
 */
uint8_t setUartAddress(uint8_t addr);

/**
 * @fn getUartAddress
 * @brief Read UART Modbus RTU slave address.
 * @return Current UART Modbus RTU slave address.
 * @retval 0 Read failed or register value is invalid.
 */
uint8_t getUartAddress(void);

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
```

For library debugging, uncomment `ENABLE_DBG` in `src/DFRobot_BMV080_Gravity.h`. Internal failures will then print their error codes through `DBG`.

Use `setUartAddress()` to save a new UART Modbus RTU address, then restart the module in UART mode and pass the new address to `DFRobot_BMV080_Gravity_UART`.

## Examples

- `continuousRead`: Continuous measurement and PM data read example. Demonstrates `begin()`, `setMeasureMode()`, `getData()`, and `sData_t` fields.
- `continuousInterrupt`: Continuous measurement with external interrupt. Demonstrates interrupt-driven data collection using the BMV080 INT pin.
- `dutyCycleRead`: Duty-cycle measurement with parameter configuration. Demonstrates `setIntegrationTime()`, `setDutyCyclingPeriod()`, algorithm and filter settings, and `setMeasureMode()`.
- `dutyCycleInterrupt`: Duty-cycle measurement with external interrupt. Demonstrates interrupt-driven data collection in periodic measurement mode.
- `configUart`: UART address, baud rate, parity and stop-bit configuration example. Demonstrates `setUartAddress()` / `getUartAddress()`, `setBaud()` / `getBaud()`, and `setUartFormat()` / `getUartFormat()`.

## Compatibility

| MCU                | Work Well | Work Wrong | Untested | Remarks |
| ------------------ |:---------:|:----------:|:--------:| ------- |
| Arduino uno        |  √        |            |          |         |
| Mega2560           |  √        |            |          |         |
| Leonardo           |  √        |            |          |         |
| ESP32              |  √        |            |          |         |
| ESP8266            |  √        |            |          |         |
| FireBeetle M0      |  √        |            |          |         |
| micro:bit          |  √        |            |          |         |
| Raspberry Pi 4B    |  √        |            |          |         |

## History

- Date 2026-06-09
- Version V1.0.0

## Credits

Written by thdyyl<yuanlong.yu@dfrobot.com>, 2026. (Welcome to our website)
