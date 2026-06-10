# DFRobot_BMV080_Gravity（Python，Raspberry Pi）

* [English Version](./README.md)

DFRobot_BMV080_Gravity 是 DFRobot BMV080 Gravity 模块在 Raspberry Pi 上使用的 Python 库，用于读取 PM1.0、PM2.5 和 PM10 颗粒物浓度数据。

这款 BMV080 PM2.5 传感器模块设计紧凑、精度高且测量范围广。其核心采用博世最新研发的 BMV080 传感器元件——全球最小的 PM 空气质量传感器，体积比市场上同类产品小 450 多倍。尽管尺寸大幅缩减，但性能丝毫不减。它不仅能精确测量空气中 PM2.5 颗粒的质量浓度，还支持 PM1.0 和 PM10 的检测。

传统的 PM2.5 传感器通常依靠风扇或风道将自由漂浮的颗粒引入检测区域，因此体积较大，并伴有风扇噪音和灰尘堆积问题，这增加了维护成本和故障风险。而 BMV080 采用类似于相机的测量原理，运用激光光学技术，根据自由空间中颗粒的数量和相对速度来计算质量浓度。它巧妙地利用周围自然气流驱动颗粒进入检测区域进行直接测量，无需风扇或强制气流系统，从而消除了维护麻烦，避免了风扇造成的灰尘堆积，显著提高了设备的可靠性。

BMV080 传感器由模块上的 ESP32 固件管理。本 Python 库通过以下两种方式与模块固件通信：

- **I2C** — 带 0xA5 帧头的短寄存器帧
- **UART** — 通过 `DFRobot_RTU.py` 实现的标准 Modbus RTU 协议

本 Python 库不包含 Bosch BMV080 SDK，也不暴露 `open` 或 `close` API。BMV080 传感器句柄由 ESP32 固件持有。

## 产品链接（[https://www.dfrobot.com.cn](https://www.dfrobot.com.cn)）

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
- 通过 `DFRobot_BMV080_Gravity_Data` 读取 PM 数据和状态标志
- 支持配置积分时间、duty-cycle 周期、算法、遮挡检测、振动滤波
- 支持配置 UART 波特率、校验位、停止位，并保存到模块 NVS
- 启动 duty-cycle 测量时，固件会按 BMV080 SDK 要求强制使用 `FAST_RESPONSE` 算法

## 安装

在 Raspberry Pi 上安装依赖：

```bash
sudo apt update
sudo apt install -y i2c-tools python3-serial python3-rpi.gpio
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

各示例默认使用 I2C。如需使用 UART Modbus RTU，请在示例中将 `communication_mode` 改为 `"UART"`，并按需要调整 `uart_addr` 和 `uart_baud`。外部 `DFRobot_RTU.py` 库默认打开 `/dev/ttyAMA0`。

运行 duty-cycle 测量示例：

```bash
python duty_cycle_read.py
```

配置 UART 波特率和帧格式：

```bash
python set_baud.py
```

运行中断示例：

```bash
python continuous_interrupt.py
python duty_cycle_interrupt.py
```

## 方法

`get_data()` 仅在有新的 PM 数据时返回 `DFRobot_BMV080_Gravity_Data` 对象；否则返回 `None`。返回对象包含以下字段：

- `pm1`、`pm2_5`、`pm10`：PM1.0、PM2.5、PM10 质量浓度，单位 ug/m3
- `runtime`：传感器运行时间，单位秒
- `run_state`：当前固件运行状态，参见 `RUN_STATE_*` 常量
- `status`：最近一次 BMV080 SDK/固件状态码
- `is_obstructed`：检测到遮挡
- `is_outside_measurement_range`：PM 数值超出可靠测量范围
- `data_ready`：有新的 PM 数据；只有该标志置位时 `get_data()` 才返回对象
- `measuring`：传感器当前正在测量
- `params_verified`：测量参数已应用到传感器
- `value_clamped`：预留兼容标志
- `value_invalid`：固件检测到非有限 PM/runtime 值并已做安全处理
- `sample_seq`：样本序号，每产生一次新测量结果递增

```python

def begin(self):
  '''!
    @brief 初始化模块并检查固件兼容性
    @return 初始化状态
    @retval True 初始化成功
    @retval False 初始化失败
  '''

def get_data(self):
  '''!
    @brief 读取颗粒物测量数据
    @return 有新数据时返回包含 PM 浓度、运行时间、运行状态和状态标志的 DFRobot_BMV080_Gravity_Data 对象，否则返回 None
  '''

def set_measure_mode(self, mode):
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

def stop_measurement(self):
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

def set_integration_time(self, integration_time):
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

def set_duty_cycling_period(self, duty_cycling_period):
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

def get_integration_time(self):
  '''!
    @brief 读取测量积分时间
    @return 积分时间，单位为秒
    @retval math.nan 读取失败
  '''

def get_duty_cycling_period(self):
  '''!
    @brief 读取 duty-cycle 测量周期
    @return duty-cycle 测量周期，单位为秒
    @retval 0 读取失败
  '''

def set_obstruction_detection(self, enable):
  '''!
    @brief 启用或禁用遮挡检测
    @param enable 遮挡检测开关
    @n            True: 启用遮挡检测
    @n            False: 禁用遮挡检测
    @return 设置状态
    @retval True 设置成功
    @retval False 设置失败
  '''

def get_obstruction_detection(self):
  '''!
    @brief 读取遮挡检测开关状态
    @return 遮挡检测开关状态
    @retval 1 已启用
    @retval 0 已禁用
    @retval -1 读取失败
  '''

def set_vibration_filtering(self, enable):
  '''!
    @brief 启用或禁用振动滤波
    @param enable 振动滤波开关
    @n            True: 启用振动滤波
    @n            False: 禁用振动滤波
    @return 设置状态
    @retval True 设置成功
    @retval False 设置失败
  '''

def get_vibration_filtering(self):
  '''!
    @brief 读取振动滤波开关状态
    @return 振动滤波开关状态
    @retval 1 已启用
    @retval 0 已禁用
    @retval -1 读取失败
  '''

def set_measurement_algorithm(self, measurement_algorithm):
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

def get_measurement_algorithm(self):
  '''!
    @brief 读取测量算法
    @return 当前测量算法
    @retval FAST_RESPONSE 快速响应算法
    @retval BALANCED 平衡算法
    @retval HIGH_PRECISION 高精度算法
    @retval 0 读取失败或寄存器值无效
  '''

def set_baud(self, baud):
  '''!
    @brief 保存 UART 波特率设置到固件 NVS
    @param baud 波特率枚举值
    @n          BAUD_2400, BAUD_4800, BAUD_9600, BAUD_14400, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200
    @return 设置状态
    @retval 0 设置成功
    @retval 1 参数错误、通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 新波特率在模块重启后生效。
  '''

def get_baud(self):
  '''!
    @brief 读取 UART 波特率
    @return 当前波特率，单位为 bps
    @retval 0 读取失败
    @note 无效寄存器值会按默认 9600 bps 解析。
  '''

def set_uart_format(self, parity, stop_bit=STOP_BIT_1):
  '''!
    @brief 保存 UART 校验位和停止位设置到固件 NVS
    @param parity 校验位配置
    @n            PARITY_NONE: 无校验
    @n            PARITY_EVEN: 偶校验
    @n            PARITY_ODD: 奇校验
    @param stop_bit 停止位配置
    @n              STOP_BIT_1: 1 位停止位
    @n              STOP_BIT_1_5: 1.5 位停止位
    @n              STOP_BIT_2: 2 位停止位
    @return 设置状态
    @retval 0 设置成功
    @retval 1 参数错误、通信错误或固件返回错误
    @retval 2 数据读取错误
    @note 新 UART 帧格式在模块重启后生效。
  '''

def get_uart_format(self):
  '''!
    @brief 读取 UART 校验位和停止位寄存器值
    @return UART 帧格式寄存器值
    @retval 0 读取失败
    @note 高字节为校验位，低字节为停止位。
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
