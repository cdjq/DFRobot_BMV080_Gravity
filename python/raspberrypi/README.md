# DFRobot_BMV080_Gravity (Python, Raspberry Pi)

* [中文版](./README_CN.md)

Python library for `DFRobot_BMV080_Gravity`.

- I2C path: short frame (0xA5), implemented with `smbus`
- UART path: Modbus RTU, implemented through bundled `DFRobot_RTU.py`

## Product

SKU: **SEN0662**

## Directory

```text
python/raspberrypi/
├─ DFRobot_BMV080_Gravity.py
├─ DFRobot_RTU.py
├─ examples/
│  ├─ consecutive_read.py
│  ├─ duty_cycle_read.py
│  ├─ read_module_info.py
│  └─ set_baud_uart_format.py
└─ README.md
```

## Install Dependency

```bash
sudo apt update
sudo apt install -y python3-smbus i2c-tools python3-serial
```

Optional fallback (for adapters needing `i2c_rdwr`):

```bash
pip install smbus2
```

Enable I2C first on Raspberry Pi:

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

## Quick Start

```python
from DFRobot_BMV080_Gravity import DFRobot_BMV080_Gravity_I2C, CONTINUOUS_MODE

sensor = DFRobot_BMV080_Gravity_I2C(bus=1, addr=0x57)
if sensor.begin():
    sensor.setBmv080Mode(CONTINUOUS_MODE)
    data = sensor.getBmv080Data()
    if data is not None:
        print(data.PM1, data.PM2_5, data.PM10)
```

## API (Main)

- `begin()`: check PID/VID and bus connectivity.
- `getPID()`, `getVID()`, `getVersion()`, `getRegMapVersion()`
- `getRunState()`, `getStatus()`
- `getBmv080DV()`: returns `(major, minor, patch)` or `None`
- `getBmv080ID()`: returns sensor id string or `None`
- `getBmv080Data()`: returns `sBmv080Data_t` when `dataReady` is set, else `None`
  - `sBmv080Data_t.valueInvalid`: source float from firmware was non-finite (NaN/Inf)
- `setBmv080Mode(CONTINUOUS_MODE | DUTY_CYCLE_MODE)`
- `stopBmv080()`, `resetBmv080()`
- `setIntegrationTime()`, `getIntegrationTime()`
- `setDutyCyclingPeriod()`, `getDutyCyclingPeriod()`
- `setMeasurementAlgorithm()`, `getMeasurementAlgorithm()`
- `setObstructionDetection()`, `getObstructionDetection()`
- `setDoVibrationFiltering()`, `getDoVibrationFiltering()`
- `setBaud()`, `getBaud()`, `getBaudValue()`
- `setUartFormat()`, `getUartFormat()`
- `getLastError()`
- Transport classes:
- `DFRobot_BMV080_Gravity_I2C(bus=1, addr=0x57)`
- `DFRobot_BMV080_Gravity_UART(baud=9600, addr=0x57)` (depends on `DFRobot_RTU.py`)

## Notes

- I2C address is selected by module `A0/A1` pins (`0x54 ~ 0x57`).
- UART parameter registers (`setBaud`/`setUartFormat`) are saved in module NVS and take effect after module restart in UART mode.
- `begin()` validates `PID=0x0296` with `VID=0x3343`.
