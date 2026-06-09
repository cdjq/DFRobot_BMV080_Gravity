# DFRobot_BMV080_Gravity (Python, Raspberry Pi)

* [中文版](./README_CN.md)

DFRobot_BMV080_Gravity is a Python library for the DFRobot BMV080 Gravity module firmware on Raspberry Pi.

The BMV080 is a particulate matter sensor managed by ESP32 firmware on the module. This Python library communicates with the firmware register table through:

- **I2C** — Short register frames with a 0xA5 header
- **UART** — Standard Modbus RTU protocol through `DFRobot_RTU.py`

The Python library does **not** include the Bosch BMV080 SDK and does not expose `open` or `close` APIs. The ESP32 firmware owns the BMV080 sensor handle.

## Product Link (https://www.dfrobot.com)

    SKU: SEN0662

## Table of Contents

* [Summary](#summary)
* [Installation](#installation)
* [Methods](#methods)
* [Compatibility](#compatibility)
* [History](#history)
* [Credits](#credits)

## Summary

Python driver for DFRobot BMV080 Gravity modules on Raspberry Pi. It provides I2C short-frame and UART Modbus RTU transports, starts and stops measurement, reads PM1.0 / PM2.5 / PM10 data, and configures measurement parameters through the module firmware.

- Supports continuous and duty-cycle measurement modes
- Reads PM data and state flags through `sData_t`
- Configures integration time, duty-cycle period, algorithm, obstruction detection and vibration filtering
- Configures UART baud rate, parity and stop bits saved in module NVS
- Duty-cycle measurement starts with `FAST_RESPONSE` as required by the BMV080 SDK

## Installation

Install the required packages on Raspberry Pi:

```bash
sudo apt update
sudo apt install -y i2c-tools python3-serial
pip install smbus2
```

Fallback when `smbus2` cannot be installed:

```bash
sudo apt install -y python3-smbus
```

Enable I2C before using the I2C transport:

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

To run an example:

```bash
cd python/raspberrypi/examples
python continuous_read.py
```

To run the duty-cycle example:

```bash
python duty_cycle_read.py
```

To configure UART baud rate and frame format:

```bash
python set_baud_uart_format.py
```

## Methods

```python

def begin(self):
  '''!
    @brief Initialize the module and check firmware compatibility
    @return Initialization status
    @retval True Initialization succeeded
    @retval False Initialization failed
  '''

def getData(self):
  '''!
    @brief Read particulate matter measurement data
    @return sData_t object when new data is available, otherwise None
  '''

def setMeasureMode(self, mode):
  '''!
    @brief Set measurement mode and start measurement
    @param mode Measurement mode
    @n          CONTINUOUS_MODE: Continuous measurement mode
    @n          DUTY_CYCLE_MODE: Duty-cycle measurement mode
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error or start-state timeout
    @note When duty-cycle measurement is started, the firmware forces FAST_RESPONSE as required by the BMV080 SDK.
  '''

def stopMeasurement(self):
  '''!
    @brief Stop the current measurement
    @return Stop command execution status
    @retval True Stop command succeeded
    @retval False Stop command failed
  '''

def reset(self):
  '''!
    @brief Reset the sensor and restore default configuration
    @return Reset command execution status
    @retval True Reset command succeeded
    @retval False Reset command failed
  '''

def setIntegrationTime(self, integration_time):
  '''!
    @brief Set measurement integration time
    @param integration_time Integration time in seconds
    @n                      The value must be greater than 0 and must not be NAN or INF.
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter or duty-cycle period constraint was not met
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When increasing integration time beyond the current period margin, set duty-cycle period first.
  '''

def setDutyCyclingPeriod(self, duty_cycling_period):
  '''!
    @brief Set duty-cycle measurement period
    @param duty_cycling_period Duty-cycle measurement period in seconds
    @n                         The value must satisfy current integration time plus 2 seconds.
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter or integration time read failed
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When shortening duty-cycle period, lower integration time first if needed.
  '''

def getIntegrationTime(self):
  '''!
    @brief Read measurement integration time
    @return Integration time in seconds
    @retval math.nan Read failed
  '''

def getDutyCyclingPeriod(self):
  '''!
    @brief Read duty-cycle measurement period
    @return Duty-cycle measurement period in seconds
    @retval 0 Read failed
  '''

def setObstructionDetection(self, enable):
  '''!
    @brief Enable or disable obstruction detection
    @param enable Obstruction detection switch
    @n            True: Enable obstruction detection
    @n            False: Disable obstruction detection
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
  '''

def getObstructionDetection(self):
  '''!
    @brief Read obstruction detection switch state
    @return Obstruction detection switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
  '''

def setVibrationFiltering(self, enable):
  '''!
    @brief Enable or disable vibration filtering
    @param enable Vibration filtering switch
    @n            True: Enable vibration filtering
    @n            False: Disable vibration filtering
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
  '''

def getVibrationFiltering(self):
  '''!
    @brief Read vibration filtering switch state
    @return Vibration filtering switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
  '''

def setMeasurementAlgorithm(self, measurement_algorithm):
  '''!
    @brief Set measurement algorithm
    @param measurement_algorithm Measurement algorithm
    @n                           FAST_RESPONSE: Fast response algorithm
    @n                           BALANCED: Balanced algorithm
    @n                           HIGH_PRECISION: High precision algorithm
    @return Setting status
    @retval 0 Setting succeeded
    @retval -1 Invalid parameter
    @retval 1 Communication error or firmware returned an error
    @retval 2 Data read error
    @note When duty-cycle measurement is started, the firmware forces FAST_RESPONSE as required by the BMV080 SDK.
  '''

def getMeasurementAlgorithm(self):
  '''!
    @brief Read measurement algorithm
    @return Current measurement algorithm
    @retval FAST_RESPONSE Fast response algorithm
    @retval BALANCED Balanced algorithm
    @retval HIGH_PRECISION High precision algorithm
    @retval 0 Read failed or register value is invalid
  '''

def setBaud(self, baud):
  '''!
    @brief Save UART baud-rate setting to firmware NVS
    @param baud Baud-rate enum value
    @n          e2400, e4800, e9600, e14400, e19200, e38400, e57600, e115200
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new baud rate takes effect after module restart.
  '''

def getBaud(self):
  '''!
    @brief Read UART baud rate
    @return Current baud rate in bps
    @retval 0 Read failed
    @note Invalid register values are parsed as the default 9600 bps.
  '''

def setUartFormat(self, parity, stop_bit=eStopBit1):
  '''!
    @brief Save UART parity and stop-bit setting to firmware NVS
    @param parity Parity configuration
    @n            eParityNone: No parity
    @n            eParityEven: Even parity
    @n            eParityOdd: Odd parity
    @param stop_bit Stop-bit configuration
    @n              eStopBit1: 1 stop bit
    @n              eStopBit1_5: 1.5 stop bits
    @n              eStopBit2: 2 stop bits
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new UART frame format takes effect after module restart.
  '''

def getUartFormat(self):
  '''!
    @brief Read UART parity and stop-bit register value
    @return UART frame-format register value
    @retval 0 Read failed
    @note The high byte is parity and the low byte is stop bits.
  '''

def setTimeoutTimeMs(self, timeout_ms):
  '''!
    @brief Set I2C communication timeout
    @param timeout_ms Timeout in milliseconds
  '''

def setTimeoutTimeS(self, timeout_s):
  '''!
    @brief Set UART Modbus RTU timeout
    @param timeout_s Timeout in seconds
  '''
```

## Compatibility

| MCU           | Work Well | Work Wrong | Untested | Remarks |
| ------------- | :-------: | :--------: | :------: | ------- |
| Raspberry Pi  |     √     |            |          |         |

* Python version

| Python  | Work Well | Work Wrong | Untested | Remarks |
| ------- | :-------: | :--------: | :------: | ------- |
| Python2 |           |            |    √     |         |
| Python3 |     √     |            |          |         |

## History

- Date 2026-06-09
- Version V1.0.0

## Credits

Written by thdyyl<yuanlong.yu@dfrobot.com>, 2026. (Welcome to our website)
