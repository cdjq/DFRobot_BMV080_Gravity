# DFRobot_BMV080_Gravity（Python，Raspberry Pi）

* [English Version](./README.md)

DFRobot_BMV080_Gravity 是 DFRobot BMV080 Gravity 模块固件的 Raspberry Pi Python 库。

BMV080 颗粒物传感器由模块上的 ESP32 固件管理。本 Python 库通过以下两种方式与固件寄存器表通信：

- **I2C** — 带 0xA5 帧头的短寄存器帧
- **UART** — 通过 `DFRobot_RTU.py` 实现的标准 Modbus RTU 协议

本 Python 库不包含 Bosch BMV080 SDK，也不暴露 `open` 或 `close` API。BMV080 传感器句柄由 ESP32 固件持有。

## 产品链接 (https://www.dfrobot.com)

    SKU: SEN0662

## 目录

* [概述](#概述)
* [安装](#安装)
* [方法](#方法)
* [兼容性](#兼容性)
* [历史](#历史)
* [创作者](#创作者)

## 概述

这是 DFRobot BMV080 Gravity 模块在 Raspberry Pi 上使用的 Python 驱动。它提供 I2C 短帧和 UART Modbus RTU 两种传输方式，可通过模块固件启动/停止测量、读取 PM1.0 / PM2.5 / PM10 数据，并配置测量参数。

- 支持连续测量和 duty-cycle 测量模式
- 通过 `sData_t` 读取 PM 数据和状态标志
- 支持配置积分时间、duty-cycle 周期、算法、遮挡检测、振动滤波
- 支持配置 UART 波特率、校验位、停止位，并保存到模块 NVS
- 启动 duty-cycle 测量时，固件会按 BMV080 SDK 要求强制使用 `FAST_RESPONSE` 算法

## 安装

在 Raspberry Pi 上安装依赖：

```bash
sudo apt update
sudo apt install -y i2c-tools python3-serial
pip install smbus2
```

无法安装 `smbus2` 时可使用的兜底依赖：

```bash
sudo apt install -y python3-smbus
```

使用 I2C 传输前，请先打开 Raspberry Pi 的 I2C：

```bash
sudo raspi-config
# Interface Options -> I2C -> Enable
```

运行连续测量示例：

```bash
cd python/raspberrypi/examples
python continuous_read.py
```

运行 duty-cycle 测量示例：

```bash
python duty_cycle_read.py
```

配置 UART 波特率和帧格式：

```bash
python set_baud_uart_format.py
```

## 方法

```python

def begin(self):
  '''!
    @brief 初始化模块并检查固件兼容性
    @return 初始化状态
    @retval True 初始化成功
    @retval False 初始化失败
  '''

def getData(self):
  '''!
    @brief 读取颗粒物测量数据
    @return 有新数据时返回 sData_t 对象，否则返回 None
  '''

def setMeasureMode(self, mode):
  '''!
    @brief 设置测量模式并启动测量
    @param mode 测量模式
    @n          CONTINUOUS_MODE: 连续测量模式
    @n          DUTY_CYCLE_MODE: duty-cycle 测量模式
    @return 设置状态
    @retval 0 设置成功
    @retval -1 参数错误
    @retval 1 通信错误或固件返回错误
    @retval 2 数据读取错误或启动状态等待超时
    @note 启动 duty-cycle 测量时，固件会按 BMV080 SDK 要求强制使用 FAST_RESPONSE。
  '''

def stopMeasurement(self):
  '''!
    @brief 停止当前测量
    @return 停止命令执行状态
    @retval True 停止成功
    @retval False 停止失败
  '''

def reset(self):
  '''!
    @brief 复位传感器并恢复默认配置
    @return 复位命令执行状态
    @retval True 复位成功
    @retval False 复位失败
  '''

def setIntegrationTime(self, integration_time):
  '''!
    @brief 设置测量积分时间
    @param integration_time 积分时间，单位为秒
    @n                      取值必须大于 0，且不能为 NAN 或 INF。
    @return 设置状态
    @retval 0 设置成功
    @retval -1 参数错误或 duty-cycle 周期约束不满足
    @retval 1 通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 增大积分时间并超过当前周期余量时，应先设置 duty-cycle 周期。
  '''

def setDutyCyclingPeriod(self, duty_cycling_period):
  '''!
    @brief 设置 duty-cycle 测量周期
    @param duty_cycling_period duty-cycle 测量周期，单位为秒
    @n                         取值必须满足当前积分时间加 2 秒的约束。
    @return 设置状态
    @retval 0 设置成功
    @retval -1 参数错误或积分时间读取失败
    @retval 1 通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 缩短 duty-cycle 周期时，如有需要应先降低积分时间。
  '''

def getIntegrationTime(self):
  '''!
    @brief 读取测量积分时间
    @return 积分时间，单位为秒
    @retval math.nan 读取失败
  '''

def getDutyCyclingPeriod(self):
  '''!
    @brief 读取 duty-cycle 测量周期
    @return duty-cycle 测量周期，单位为秒
    @retval 0 读取失败
  '''

def setObstructionDetection(self, enable):
  '''!
    @brief 启用或禁用遮挡检测
    @param enable 遮挡检测开关
    @n            True: 启用遮挡检测
    @n            False: 禁用遮挡检测
    @return 设置状态
    @retval True 设置成功
    @retval False 设置失败
  '''

def getObstructionDetection(self):
  '''!
    @brief 读取遮挡检测开关状态
    @return 遮挡检测开关状态
    @retval 1 已启用
    @retval 0 已禁用
    @retval -1 读取失败
  '''

def setVibrationFiltering(self, enable):
  '''!
    @brief 启用或禁用振动滤波
    @param enable 振动滤波开关
    @n            True: 启用振动滤波
    @n            False: 禁用振动滤波
    @return 设置状态
    @retval True 设置成功
    @retval False 设置失败
  '''

def getVibrationFiltering(self):
  '''!
    @brief 读取振动滤波开关状态
    @return 振动滤波开关状态
    @retval 1 已启用
    @retval 0 已禁用
    @retval -1 读取失败
  '''

def setMeasurementAlgorithm(self, measurement_algorithm):
  '''!
    @brief 设置测量算法
    @param measurement_algorithm 测量算法
    @n                           FAST_RESPONSE: 快速响应算法
    @n                           BALANCED: 平衡算法
    @n                           HIGH_PRECISION: 高精度算法
    @return 设置状态
    @retval 0 设置成功
    @retval -1 参数错误
    @retval 1 通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 启动 duty-cycle 测量时，固件会按 BMV080 SDK 要求强制使用 FAST_RESPONSE。
  '''

def getMeasurementAlgorithm(self):
  '''!
    @brief 读取测量算法
    @return 当前测量算法
    @retval FAST_RESPONSE 快速响应算法
    @retval BALANCED 平衡算法
    @retval HIGH_PRECISION 高精度算法
    @retval 0 读取失败或寄存器值无效
  '''

def setBaud(self, baud):
  '''!
    @brief 保存 UART 波特率设置到固件 NVS
    @param baud 波特率枚举值
    @n          e2400, e4800, e9600, e14400, e19200, e38400, e57600, e115200
    @return 设置状态
    @retval 0 设置成功
    @retval 1 参数错误、通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 新波特率在模块重启后生效。
  '''

def getBaud(self):
  '''!
    @brief 读取 UART 波特率
    @return 当前波特率，单位为 bps
    @retval 0 读取失败
    @note 无效寄存器值会按默认 9600 bps 解析。
  '''

def setUartFormat(self, parity, stop_bit=eStopBit1):
  '''!
    @brief 保存 UART 校验位和停止位设置到固件 NVS
    @param parity 校验位配置
    @n            eParityNone: 无校验
    @n            eParityEven: 偶校验
    @n            eParityOdd: 奇校验
    @param stop_bit 停止位配置
    @n              eStopBit1: 1 位停止位
    @n              eStopBit1_5: 1.5 位停止位
    @n              eStopBit2: 2 位停止位
    @return 设置状态
    @retval 0 设置成功
    @retval 1 参数错误、通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 新 UART 帧格式在模块重启后生效。
  '''

def getUartFormat(self):
  '''!
    @brief 读取 UART 校验位和停止位寄存器值
    @return UART 帧格式寄存器值
    @retval 0 读取失败
    @note 高字节为校验位，低字节为停止位。
  '''

def setTimeoutTimeMs(self, timeout_ms):
  '''!
    @brief 设置 I2C 通信超时时间
    @param timeout_ms 超时时间，单位为毫秒
  '''

def setTimeoutTimeS(self, timeout_s):
  '''!
    @brief 设置 UART Modbus RTU 超时时间
    @param timeout_s 超时时间，单位为秒
  '''
```

## 兼容性

| MCU          | 正常 | 异常 | 未测试 | 备注 |
| ------------ | :--: | :--: | :----: | ---- |
| Raspberry Pi |  √   |      |        |      |

* Python 版本

| Python  | 正常 | 异常 | 未测试 | 备注 |
| ------- | :--: | :--: | :----: | ---- |
| Python2 |      |      |   √    |      |
| Python3 |  √   |      |        |      |

## 历史

- 日期 2026-06-09
- 版本 V1.0.0

## 创作者

Written by thdyyl<yuanlong.yu@dfrobot.com>, 2026. (Welcome to our website)
