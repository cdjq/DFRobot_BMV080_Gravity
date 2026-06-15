# DFRobot_BMV080_Gravity (Python, Raspberry Pi)

* [中文版](./README_CN.md)

DFRobot_BMV080_Gravity is a Python library for the DFRobot BMV080 Gravity PM2.5 sensor module on Raspberry Pi, used to read PM1.0, PM2.5 and PM10 particulate matter concentration data.

The BMV080 sensor core, developed by Bosch, is the world's smallest PM air quality sensor — over 450 times smaller than comparable products on the market. Despite its ultra-compact size, it delivers precise PM2.5, PM1.0 and PM10 measurements.

Unlike traditional PM sensors that rely on fans or ducts to draw particles into the detection zone (causing fan noise, dust accumulation, and maintenance headaches), the BMV080 uses a camera-like laser-optics principle to calculate mass concentration from particles freely moving in open space. It leverages ambient airflow to transport particles into the detection area, eliminating fans entirely — improving reliability and reducing maintenance.

The BMV080 sensor on the module is managed by an ESP32 firmware. This library communicates with the module firmware through:

- **I2C** — Short register frames with a 0xA5 header
- **UART** — Standard Modbus RTU protocol through `DFRobot_RTU.py`

The Python library does **not** include the Bosch BMV080 SDK and does not expose `open` or `close` APIs. The ESP32 firmware owns the BMV080 sensor handle.

## Product Link（[https://www.dfrobot.com](https://www.dfrobot.com)）

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
- Reads PM data and state flags through `DFRobot_BMV080_Gravity_Data`
- Configures integration time, duty-cycle period, algorithm, obstruction detection and vibration filtering
- Configures UART baud rate, parity and stop bits saved in module NVS
- Duty-cycle measurement starts with `FAST_RESPONSE` as required by the BMV080 SDK

## Installation

Install the required packages on Raspberry Pi:

```bash
sudo apt update
sudo apt install -y i2c-tools python3-serial python3-rpi.gpio
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

Each example defaults to I2C. To use UART Modbus RTU, set `communication_mode = "UART"` in the example and adjust `uart_addr` and `uart_baud` as needed. The external `DFRobot_RTU.py` library opens `/dev/ttyAMA0` by default.

To run the duty-cycle example:

```bash
python duty_cycle_read.py
```

To configure UART baud rate and frame format:

```bash
python set_baud.py
```

To run the interrupt examples:

```bash
python continuous_interrupt.py
python duty_cycle_interrupt.py
```

## Methods

`get_data()` returns a `DFRobot_BMV080_Gravity_Data` object only when a new PM sample is ready; otherwise it returns `None`. The returned data fields are:

- `pm1`, `pm2_5`, `pm10`: PM1.0, PM2.5 and PM10 mass concentration in ug/m3
- `runtime`: Sensor runtime in seconds
- `run_state`: Current firmware run state, see `RUN_STATE_*` constants
- `status`: Last BMV080 SDK/firmware status code
- `is_obstructed`: Obstruction detected
- `is_outside_measurement_range`: PM value is outside the reliable measurement range
- `data_ready`: New PM data is available; `get_data()` returns an object only when this flag is set
- `measuring`: Sensor is currently measuring
- `params_verified`: Measurement parameters have been applied to the sensor
- `value_clamped`: Reserved compatibility flag
- `value_invalid`: Non-finite PM/runtime value was sanitized by firmware
- `sample_seq`: Sample sequence number, increments for each new measurement

```python

def begin(self):
  '''!
    @brief Initialize the module and check firmware compatibility
    @return Initialization status
    @retval True Initialization succeeded
    @retval False Initialization failed
  '''

def get_data(self):
  '''!
    @brief Read particulate matter measurement data
    @return DFRobot_BMV080_Gravity_Data object with PM concentration, runtime, run state and flags when new data is available, otherwise None
  '''

def set_measure_mode(self, mode):
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

def stop_measurement(self):
  '''!
    @brief Stop the current measurement
    @return Stop command execution status
    @retval True Stop command succeeded
    @retval False Stop command failed
  '''

def reset(self):
  '''!
    @brief Reset the sensor and restore default configuration
    @details The firmware stops measurement, resets the BMV080, restores the default holding registers, then saves those defaults.
    @n       Default UART RTU address: 0x57.
    @n       Default UART settings: 9600 bps, 8-N-1.
    @n       Default measurement settings: CONTINUOUS_MODE, BALANCED, obstruction detection enabled,
    @n       vibration filtering enabled, integration time 10.0 s, duty-cycle period 30 s.
    @note If UART communication settings were changed before reset, reconnect with the restored defaults after reset or module restart as needed.
    @return Reset command execution status
    @retval True Reset command succeeded
    @retval False Reset command failed
  '''

def set_integration_time(self, integration_time):
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

def set_duty_cycling_period(self, duty_cycling_period):
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

def get_integration_time(self):
  '''!
    @brief Read measurement integration time
    @return Integration time in seconds
    @retval math.nan Read failed
  '''

def get_duty_cycling_period(self):
  '''!
    @brief Read duty-cycle measurement period
    @return Duty-cycle measurement period in seconds
    @retval 0 Read failed
  '''

def set_obstruction_detection(self, enable):
  '''!
    @brief Enable or disable obstruction detection
    @param enable Obstruction detection switch
    @n            True: Enable obstruction detection
    @n            False: Disable obstruction detection
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
  '''

def get_obstruction_detection(self):
  '''!
    @brief Read obstruction detection switch state
    @return Obstruction detection switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
  '''

def set_vibration_filtering(self, enable):
  '''!
    @brief Enable or disable vibration filtering
    @param enable Vibration filtering switch
    @n            True: Enable vibration filtering
    @n            False: Disable vibration filtering
    @return Setting status
    @retval True Setting succeeded
    @retval False Setting failed
  '''

def get_vibration_filtering(self):
  '''!
    @brief Read vibration filtering switch state
    @return Vibration filtering switch state
    @retval 1 Enabled
    @retval 0 Disabled
    @retval -1 Read failed
  '''

def set_measurement_algorithm(self, measurement_algorithm):
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

def get_measurement_algorithm(self):
  '''!
    @brief Read measurement algorithm
    @return Current measurement algorithm
    @retval FAST_RESPONSE Fast response algorithm
    @retval BALANCED Balanced algorithm
    @retval HIGH_PRECISION High precision algorithm
    @retval 0 Read failed or register value is invalid
  '''

def set_baud(self, baud):
  '''!
    @brief Save UART baud-rate setting to firmware NVS
    @param baud Baud-rate enum value
    @n          BAUD_2400, BAUD_4800, BAUD_9600, BAUD_14400, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new baud rate takes effect after module restart.
  '''

def get_baud(self):
  '''!
    @brief Read UART baud rate
    @return Current baud rate in bps
    @retval 0 Read failed
    @note Invalid register values are parsed as the default 9600 bps.
  '''

def set_uart_format(self, parity, stop_bit=STOP_BIT_1):
  '''!
    @brief Save UART parity and stop-bit setting to firmware NVS
    @param parity Parity configuration
    @n            PARITY_NONE: No parity
    @n            PARITY_EVEN: Even parity
    @n            PARITY_ODD: Odd parity
    @param stop_bit Stop-bit configuration
    @n              STOP_BIT_1: 1 stop bit
    @n              STOP_BIT_1_5: 1.5 stop bits
    @n              STOP_BIT_2: 2 stop bits
    @return Setting status
    @retval 0 Setting succeeded
    @retval 1 Invalid parameter, communication error or firmware returned an error
    @retval 2 Data read error
    @note The new UART frame format takes effect after module restart.
  '''

def get_uart_format(self):
  '''!
    @brief Read UART parity and stop-bit register value
    @return UART frame-format register value
    @retval 0 Read failed
    @note The high byte is parity and the low byte is stop bits.
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
