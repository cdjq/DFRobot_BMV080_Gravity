# DFRobot_BMV080_Gravity（Python，Raspberry Pi）

* [English Version](./README.md)

这是 `DFRobot_BMV080_Gravity` 的 Python 版本。

- I2C 路径：0xA5 短帧，使用 `smbus`
- UART 路径：Modbus RTU，依赖并复用 `DFRobot_RTU.py`

## 产品

SKU: **SEN0662**

## 目录结构

```text
python/raspberrypi/
├─ DFRobot_BMV080_Gravity.py
├─ DFRobot_RTU.py
├─ examples/
│  ├─ consecutive_read.py
│  ├─ duty_cycle_read.py
│  ├─ read_module_info.py
│  └─ set_baud_uart_format.py
└─ README_CN.md
```

## 依赖安装

```bash
sudo apt update
sudo apt install -y python3-smbus i2c-tools python3-serial
```

可选依赖（部分适配器需要 `i2c_rdwr` 时建议安装）：

```bash
pip install smbus2
```

使用前请在树莓派打开 I2C：

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

## 快速使用

```python
from DFRobot_BMV080_Gravity import DFRobot_BMV080_Gravity_I2C, CONTINUOUS_MODE

sensor = DFRobot_BMV080_Gravity_I2C(bus=1, addr=0x57)
if sensor.begin():
    sensor.setBmv080Mode(CONTINUOUS_MODE)
    data = sensor.getBmv080Data()
    if data is not None:
        print(data.PM1, data.PM2_5, data.PM10)
```

## 主要接口

- `begin()`：检查 PID/VID 和通信是否正常
- `getPID()`、`getVID()`、`getVersion()`、`getRegMapVersion()`
- `getRunState()`、`getStatus()`
- `getBmv080DV()`：返回 `(major, minor, patch)`，失败返回 `None`
- `getBmv080ID()`：返回传感器 ID 字符串，失败返回 `None`
- `getBmv080Data()`：当 `dataReady` 置位时返回 `sBmv080Data_t`，否则返回 `None`
  - `sBmv080Data_t.valueInvalid`：固件源浮点为非有限值（NaN/Inf）
- `setBmv080Mode(CONTINUOUS_MODE | DUTY_CYCLE_MODE)`
- `stopBmv080()`、`resetBmv080()`
- `setIntegrationTime()`、`getIntegrationTime()`
- `setDutyCyclingPeriod()`、`getDutyCyclingPeriod()`
- `setMeasurementAlgorithm()`、`getMeasurementAlgorithm()`
- `setObstructionDetection()`、`getObstructionDetection()`
- `setDoVibrationFiltering()`、`getDoVibrationFiltering()`
- `setBaud()`、`getBaud()`、`getBaudValue()`
- `setUartFormat()`、`getUartFormat()`
- `getLastError()`
- 传输类：
- `DFRobot_BMV080_Gravity_I2C(bus=1, addr=0x57)`
- `DFRobot_BMV080_Gravity_UART(baud=9600, addr=0x57)`（依赖 `DFRobot_RTU.py`）

## 说明

- 模块地址由 `A0/A1` 选择（`0x54 ~ 0x57`）。
- `setBaud`/`setUartFormat` 写入模块 NVS，需模块以 UART 模式重启后生效。
- `begin()` 仅校验 `PID=0x0296` 且 `VID=0x3343`。
